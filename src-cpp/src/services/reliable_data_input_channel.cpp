#include "sanctuary/soe_protocol/services/reliable_data_input_channel.hpp"
#include "sanctuary/soe_protocol/abstractions/soe_session.hpp"
#include "sanctuary/soe_protocol/services/rc4_cipher.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include "sanctuary/soe_protocol/util/data_utils.hpp"
#include "sanctuary/soe_protocol/util/native_span_pool.hpp"
#include <algorithm>
#include <bit>
#include <cstring>

namespace sanctuary::soe_protocol::services {

// Forward declaration for SoeProtocolHandler - we'll treat it as a SoeSession
// This is a temporary workaround until SoeProtocolHandler is fully implemented in C++
class SoeProtocolHandler : public abstractions::SoeSession {
public:
    virtual const objects::SessionParameters& session_params() const = 0;
    virtual const objects::ApplicationParameters& application_params() const = 0;
};

ReliableDataInputChannel::ReliableDataInputChannel(
    SoeProtocolHandler& handler,
    const objects::SessionParameters& session_params,
    const objects::ApplicationParameters& app_params,
    util::NativeSpanPool& span_pool,
    DataHandler data_handler
)
    : handler_(handler)
    , session_params_(session_params)
    , application_params_(app_params)
    , span_pool_(span_pool)
    , data_handler_(std::move(data_handler))
    , stash_(session_params.max_queued_incoming_reliable_data_packets())
    , last_ack_all_time_(std::chrono::steady_clock::now())
{
    if (app_params.encryption_key_state()) {
        cipher_state_ = app_params.encryption_key_state()->copy();
    }
}

ReliableDataInputChannel::~ReliableDataInputChannel() {
    // Clean up stashed spans
    for (auto& stashed : stash_) {
        if (stashed.span) {
            span_pool_.return_span(std::move(stashed.span));
            stashed.span = nullptr;
        }
    }
}

void ReliableDataInputChannel::run_tick() {
    if (buffered_ack_all_) {
        send_ack_all(*buffered_ack_all_);
        buffered_ack_all_.reset();
    }

    int64_t to_ack = window_start_sequence_ - 1;

    // No need to perform an ack all if we're acking everything individually, or we've already acked up to the
    // current window start sequence
    if (session_params_.acknowledge_all_data() || to_ack <= last_ack_all_sequence_) {
        return;
    }

    // Ack if:
    // - at least MAX_ACK_DELAY have passed since the last ack time and
    // - our seq to ack is greater than the last ack seq + half of the ack window
    auto elapsed = std::chrono::steady_clock::now() - last_ack_all_time_;
    bool need_ack = elapsed > session_params_.maximum_acknowledge_delay()
        || to_ack >= last_ack_all_sequence_ + session_params_.data_ack_window() / 2;

    if (need_ack) {
        send_ack_all(objects::packets::AcknowledgeAll{static_cast<uint16_t>(to_ack)});
    }
}

void ReliableDataInputChannel::handle_reliable_data(std::span<uint8_t> data) {
    if (!preprocess_data(data, false)) {
        return;
    }

    process_data(data);
    // We've now processed another packet, so we can increment the window
    window_start_sequence_ += 1;

    consume_stashed_data_fragments();
}

void ReliableDataInputChannel::handle_reliable_data_fragment(std::span<uint8_t> data) {
    if (!preprocess_data(data, true)) {
        return;
    }

    // At this point we know this fragment can be written directly to the buffer as it is next in the sequence.
    write_immediate_fragment_to_buffer(data);
    // We've now processed another packet, so we can increment the window
    window_start_sequence_ += 1;

    // Attempt to process the current buffer now, as the stashed fragments may belong to a new buffer
    // consume_stashed_data_fragments will attempt to process the current buffer as it releases stashes
    try_process_current_buffer();
    consume_stashed_data_fragments();
}

void ReliableDataInputChannel::send_ack_all(const objects::packets::AcknowledgeAll& ack_all) {
    uint8_t buffer[objects::packets::AcknowledgeAll::SIZE];
    ack_all.serialize(buffer);
    handler_.send_contextual_packet(SoeOpCode::AcknowledgeAll, buffer);
    input_stats_.acknowledge_count++;

    last_ack_all_sequence_ = ack_all.sequence;
    last_ack_all_time_ = std::chrono::steady_clock::now();
}

bool ReliableDataInputChannel::preprocess_data(std::span<uint8_t>& data, bool is_fragment) {
    input_stats_.total_received++;

    int64_t sequence;
    uint16_t packet_sequence;
    if (!is_valid_reliable_data(data, sequence, packet_sequence)) {
        return false;
    }

    bool ahead = sequence != window_start_sequence_;

    // Ack this data if we are in ack-all mode, or it is ahead of our expectations
    if (session_params_.acknowledge_all_data() || ahead) {
        uint8_t buffer[objects::packets::Acknowledge::SIZE];
        objects::packets::Acknowledge{packet_sequence}.serialize(buffer);
        handler_.send_contextual_packet(SoeOpCode::Acknowledge, buffer);
    }

    // Remove the sequence bytes
    data = data.subspan(sizeof(uint16_t));

    // We can process this immediately.
    if (!ahead) {
        return true;
    }

    // We've received this data out-of-order, so stash it
    input_stats_.out_of_order_count++;
    int64_t stash_spot = sequence % session_params_.max_queued_incoming_reliable_data_packets();

    // Grab our stash item. We may have already stashed this packet ahead of time, so check for that
    StashedData& stash_item = stash_[stash_spot];
    if (stash_item.span) {
        input_stats_.duplicate_count++;
        return false;
    }

    auto data_span = span_pool_.rent();
    data_span->copy_data_into(data);

    // Update our stash item
    stash_item.is_fragment = is_fragment;
    stash_item.span = std::move(data_span);
    return false;
}

bool ReliableDataInputChannel::is_valid_reliable_data(
    std::span<const uint8_t> data,
    int64_t& sequence,
    uint16_t& packet_sequence
) {
    packet_sequence = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    sequence = util::DataUtils::get_true_incoming_sequence(
        packet_sequence,
        window_start_sequence_,
        session_params_.max_queued_incoming_reliable_data_packets()
    );

    // If this is too far ahead of our window, just drop it
    if (sequence > window_start_sequence_ + session_params_.max_queued_incoming_reliable_data_packets()) {
        return false;
    }

    // Great, we're inside the window
    if (sequence >= window_start_sequence_) {
        return true;
    }

    // We're receiving data we've already fully processed, so inform the remote about this.
    // However, because data is usually received in clumps, ensure we don't send acks too quickly
    auto elapsed = std::chrono::steady_clock::now() - last_ack_all_time_;
    if (elapsed < session_params_.maximum_acknowledge_delay()) {
        send_ack_all(objects::packets::AcknowledgeAll{static_cast<uint16_t>(window_start_sequence_ - 1)});
    }
    input_stats_.duplicate_count++;

    return false;
}

void ReliableDataInputChannel::write_immediate_fragment_to_buffer(std::span<const uint8_t> data) {
    if (!current_buffer_.empty()) {
        std::copy(data.begin(), data.end(), std::back_inserter(current_buffer_));
        running_data_length_ += data.size();
    } else {
        // Otherwise, create a new buffer by assuming this is a master fragment and reading the length
        expected_data_length_ = (static_cast<uint32_t>(data[0]) << 24)
                              | (static_cast<uint32_t>(data[1]) << 16)
                              | (static_cast<uint32_t>(data[2]) << 8)
                              | static_cast<uint32_t>(data[3]);
        current_buffer_.reserve(expected_data_length_);
        data = data.subspan(sizeof(uint32_t));
        current_buffer_.assign(data.begin(), data.end());
        running_data_length_ = data.size();
    }
}

void ReliableDataInputChannel::try_process_current_buffer() {
    if (current_buffer_.empty() || running_data_length_ < expected_data_length_) {
        return;
    }

    // Process the buffer, free it, and reset fields
    std::span<uint8_t> buffer_span(current_buffer_.data(), running_data_length_);
    process_data(buffer_span);
    current_buffer_.clear();
    current_buffer_.shrink_to_fit();
    running_data_length_ = 0;
    expected_data_length_ = 0;
}

void ReliableDataInputChannel::consume_stashed_data_fragments() {
    // Grab the stash index of our current window start sequence
    int64_t stash_spot = window_start_sequence_ % session_params_.max_queued_incoming_reliable_data_packets();
    StashedData& stashed_item = stash_[stash_spot];

    // Iterate through the stash until we reach an empty slot
    while (stashed_item.span) {
        if (stashed_item.is_fragment) {
            write_immediate_fragment_to_buffer(stashed_item.span->used_span());
            try_process_current_buffer();
        } else {
            auto used = stashed_item.span->used_span();
            std::vector<uint8_t> mutable_buffer(used.begin(), used.end());
            process_data(mutable_buffer);
        }

        // Release our stash reference
        span_pool_.return_span(std::move(stashed_item.span));
        stashed_item.span = nullptr;

        // Increment the window
        window_start_sequence_++;
        stash_spot = window_start_sequence_ % session_params_.max_queued_incoming_reliable_data_packets();
        stashed_item = stash_[stash_spot];
    }
}

void ReliableDataInputChannel::process_data(std::span<uint8_t> data) {
    if (util::DataUtils::check_for_multi_data(data)) {
        int offset = 2;
        while (offset < static_cast<int>(data.size())) {
            uint32_t length = util::DataUtils::read_variable_length(data, offset);
            decrypt_and_call_data_handler(data.subspan(offset, length));
            offset += static_cast<int>(length);
        }
    } else {
        decrypt_and_call_data_handler(data);
    }
}

void ReliableDataInputChannel::decrypt_and_call_data_handler(std::span<uint8_t> data) {
    if (application_params_.is_encryption_enabled()) {
        // A single 0x00 byte may be used to prefix encrypted data. We must ignore it
        if (data.size() > 1 && data[0] == 0) {
            data = data.subspan(1);
        }

        // We can assume the key state is not null, as encryption cannot be enabled
        // by the application without setting a key state
        Rc4Cipher::transform(data, data, *cipher_state_);
    }

    input_stats_.total_received_bytes += data.size();
    data_handler_(data);
}

} // namespace sanctuary::soe_protocol::services
