#include "sanctuary/soe_protocol/services/reliable_data_output_channel.hpp"
#include "sanctuary/soe_protocol/abstractions/soe_session.hpp"
#include "sanctuary/soe_protocol/services/rc4_cipher.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include "sanctuary/soe_protocol/util/data_utils.hpp"
#include "sanctuary/soe_protocol/util/native_span_pool.hpp"
#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

namespace sanctuary::soe_protocol::services {

// Forward declaration for SoeProtocolHandler
class SoeProtocolHandler : public abstractions::SoeSession {
public:
    virtual const objects::SessionParameters& session_params() const = 0;
    virtual const objects::ApplicationParameters& application_params() const = 0;
};

ReliableDataOutputChannel::ReliableDataOutputChannel(
    SoeProtocolHandler& handler,
    util::NativeSpanPool& span_pool,
    int32_t max_data_length
)
    : handler_(handler)
    , session_params_(handler.session_params())
    , application_params_(handler.application_params())
    , span_pool_(span_pool)
    , dispatch_stash_(session_params_.max_queued_outgoing_reliable_data_packets())
{
    if (application_params_.encryption_key_state()) {
        cipher_state_ = application_params_.encryption_key_state()->copy();
    }
    set_max_data_length(max_data_length);
    setup_new_multi_buffer();
}

ReliableDataOutputChannel::~ReliableDataOutputChannel() {
    // Clean up dispatch stash
    for (auto& packet : dispatch_stash_) {
        if (packet.data_span) {
            span_pool_.return_span(std::move(packet.data_span));
        }
    }

    // Clean up multi buffer
    if (multi_buffer_) {
        span_pool_.return_span(std::move(multi_buffer_));
    }
}

void ReliableDataOutputChannel::enqueue_data(std::span<const uint8_t> data) {
    enqueue_data_internal(data, false);
}

void ReliableDataOutputChannel::run_tick(std::stop_token stop_token) {
    int32_t stash_index;
    {
        std::lock_guard<std::mutex> lock(packet_output_queue_lock_);

        // Attempt to load packets from the dispatch queue into the dispatch stash (the current window)
        // while there is unused space in the window
        while (!dispatch_queue_.empty()) {
            auto& [seq, data] = dispatch_queue_.front();
            stash_index = get_sequence_index_in_dispatch_buffer(seq);
            if (dispatch_stash_[stash_index].data_span) {
                break;
            }

            dispatch_stash_[stash_index] = std::move(data);
            dispatch_queue_.pop_front();
        }

        // Just in case we've received a late ack, after reverting to repeat
        if (current_sequence_ < window_start_sequence_) {
            current_sequence_ = window_start_sequence_;
        }

        // Pull anything from the multi-buffer
        enqueue_multi_buffer();
    }

    // Check how long it's been since we sent the first item in the window. If it hasn't been cleared after
    // our ack window has timed out, then it hasn't been acknowledged, and we need to re-send it
    int64_t resending_up_to = 0;
    stash_index = get_sequence_index_in_dispatch_buffer(window_start_sequence_);
    auto earliest_sent = dispatch_stash_[stash_index].sent_at;
    if (earliest_sent != std::chrono::steady_clock::time_point::min()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - earliest_sent
        );
        if (elapsed.count() >= ACK_WAIT_MILLISECONDS) {
            current_sequence_ = window_start_sequence_;
            resending_up_to = current_sequence_;
        }
    }

    // Send everything we haven't sent from the current window
    assert(current_sequence_ <= total_sequence_);
    int64_t last_sequence_to_send = std::min(
        total_sequence_,
        current_sequence_ + session_params_.max_queued_outgoing_reliable_data_packets()
    );

    while (current_sequence_ < last_sequence_to_send) {
        if (stop_token.stop_requested()) {
            return;
        }

        output_stats_.total_sent_reliable_packets++;
        if (current_sequence_ < resending_up_to) {
            output_stats_.total_resent_reliable_packets++;
        }

        stash_index = get_sequence_index_in_dispatch_buffer(current_sequence_++);
        StashedOutputPacket& stashed_packet = dispatch_stash_[stash_index];
        if (!stashed_packet.data_span) {
            continue; // Packets ahead of current_sequence_ may have been individually acked & cleared
        }

        SoeOpCode op_code = stashed_packet.is_fragment ? SoeOpCode::ReliableDataFragment : SoeOpCode::ReliableData;
        stashed_packet.sent_at = std::chrono::steady_clock::now();
        handler_.send_contextual_packet(op_code, stashed_packet.data_span->used_span());
    }
}

void ReliableDataOutputChannel::notify_of_acknowledge(const objects::packets::Acknowledge& ack) {
    int64_t seq = get_true_incoming_sequence(ack.sequence);
    output_stats_.incoming_acknowledge_count++;
    process_ack(seq);
}

void ReliableDataOutputChannel::notify_of_acknowledge_all(const objects::packets::AcknowledgeAll& ack_all) {
    int64_t seq = get_true_incoming_sequence(ack_all.sequence);
    output_stats_.incoming_acknowledge_count++;

    for (int64_t i = window_start_sequence_; i <= seq; i++) {
        process_ack(i);
    }
}

void ReliableDataOutputChannel::set_max_data_length(int32_t max_data_length) {
    if (current_sequence_ > 0) {
        throw std::runtime_error("The maximum length may not be changed after data has been enqueued");
    }

    max_data_length_ = max_data_length;
}

void ReliableDataOutputChannel::process_ack(int64_t sequence) {
    int32_t index = get_sequence_index_in_dispatch_buffer(sequence);

    StashedOutputPacket& packet = dispatch_stash_[index];
    if (packet.data_span) {
        span_pool_.return_span(std::move(packet.data_span));
        packet.data_span = nullptr;
        output_stats_.actual_acknowledge_count++;
    }

    // Walk the window forward to our current_sequence_, until we find a packet that hasn't been acked
    while (window_start_sequence_ < current_sequence_) {
        index = get_sequence_index_in_dispatch_buffer(window_start_sequence_);
        if (dispatch_stash_[index].data_span) {
            break;
        }

        window_start_sequence_++;
    }
}

int32_t ReliableDataOutputChannel::get_sequence_index_in_dispatch_buffer(int64_t sequence) const noexcept {
    return static_cast<int32_t>(sequence % session_params_.max_queued_outgoing_reliable_data_packets());
}

void ReliableDataOutputChannel::enqueue_data_internal(std::span<const uint8_t> data, bool is_recursing) {
    std::vector<uint8_t> encrypted_span;
    std::span<const uint8_t> data_to_use = data;

    if (!is_recursing) {
        packet_output_queue_lock_.lock();
    }

    if (!is_recursing && application_params_.is_encryption_enabled()) {
        encrypted_span = encrypt(data, data_to_use);
    }

    int multi_length = util::DataUtils::get_variable_length_size(static_cast<int>(data_to_use.size())) + data_to_use.size();
    if (max_data_length_ - multi_buffer_offset_ >= multi_length) { // We can fit in the current multi-buffer
        util::DataUtils::write_variable_length(
            multi_buffer_->full_span(),
            static_cast<uint32_t>(data_to_use.size()),
            multi_buffer_offset_
        );
        std::copy(data_to_use.begin(), data_to_use.end(), multi_buffer_->full_span().begin() + multi_buffer_offset_);

        if (multi_buffer_first_item_offset_ == -1) {
            multi_buffer_first_item_offset_ = multi_buffer_offset_;
        }
        multi_buffer_offset_ += static_cast<int32_t>(data_to_use.size());
        multi_buffer_item_count_++;
    } else {
        // We must enqueue the current multi-buffer, in order to maintain order
        enqueue_multi_buffer();

        // Now that we've cleared the multi-buffer, can we fit?
        if (max_data_length_ - multi_buffer_offset_ >= multi_length) {
            enqueue_data_internal(data_to_use, true);
        } else {
            auto mutable_data = data_to_use;
            stash_fragment(mutable_data, true);
            while (!mutable_data.empty()) {
                stash_fragment(mutable_data, false);
            }
        }
    }

    if (!is_recursing) {
        packet_output_queue_lock_.unlock();
    }
}

void ReliableDataOutputChannel::stash_fragment(std::span<const uint8_t>& data, bool is_master) {
    auto span = span_pool_.rent();
    auto full = span->full_span();

    // Write sequence
    full[0] = static_cast<uint8_t>(total_sequence_ >> 8);
    full[1] = static_cast<uint8_t>(total_sequence_ & 0xFF);
    int32_t offset = 2;

    int32_t amount_to_take = std::min(
        static_cast<int32_t>(data.size()),
        max_data_length_ - static_cast<int32_t>(sizeof(uint16_t))
    );

    if (is_master) {
        // Write total length
        uint32_t total_len = static_cast<uint32_t>(data.size());
        full[offset++] = static_cast<uint8_t>(total_len >> 24);
        full[offset++] = static_cast<uint8_t>((total_len >> 16) & 0xFF);
        full[offset++] = static_cast<uint8_t>((total_len >> 8) & 0xFF);
        full[offset++] = static_cast<uint8_t>(total_len & 0xFF);
        amount_to_take -= sizeof(uint32_t);
    }

    std::copy(data.begin(), data.begin() + amount_to_take, full.begin() + offset);
    span->set_used_length(offset + amount_to_take);

    add_to_dispatch(std::move(span), true);
    data = data.subspan(amount_to_take);
}

void ReliableDataOutputChannel::enqueue_multi_buffer() {
    switch (multi_buffer_item_count_) {
        case 0:
            return;
        case 1: { // Just send a non-multi data packet in this case
            // Overwrite the multi-data indicator with the sequence
            multi_buffer_->set_start_offset(multi_buffer_first_item_offset_ - sizeof(uint16_t));
            multi_buffer_->set_used_length(multi_buffer_offset_ - multi_buffer_->start_offset());
            auto used = multi_buffer_->used_span();
            used[0] = static_cast<uint8_t>(total_sequence_ >> 8);
            used[1] = static_cast<uint8_t>(total_sequence_ & 0xFF);
            break;
        }
        default: {
            // Write the current sequence to the head of the buffer
            auto full = multi_buffer_->full_span();
            full[0] = static_cast<uint8_t>(total_sequence_ >> 8);
            full[1] = static_cast<uint8_t>(total_sequence_ & 0xFF);
            multi_buffer_->set_used_length(multi_buffer_offset_);
            break;
        }
    }

    add_to_dispatch(std::move(multi_buffer_), false);
    setup_new_multi_buffer();
}

void ReliableDataOutputChannel::add_to_dispatch(std::shared_ptr<objects::NativeSpan> buffer, bool is_fragment) {
    int32_t dispatch_index = get_sequence_index_in_dispatch_buffer(total_sequence_);
    StashedOutputPacket& packet = dispatch_stash_[dispatch_index];

    // Check if the stash contains backed-up data
    if (packet.data_span) {
        StashedOutputPacket new_packet;
        new_packet.is_fragment = is_fragment;
        new_packet.data_span = std::move(buffer);
        dispatch_queue_.emplace_back(total_sequence_, std::move(new_packet));
    } else {
        packet.is_fragment = is_fragment;
        packet.data_span = std::move(buffer);
        packet.sent_at = std::chrono::steady_clock::time_point::min();
    }

    total_sequence_++;
}

void ReliableDataOutputChannel::setup_new_multi_buffer() {
    multi_buffer_ = span_pool_.rent();
    multi_buffer_item_count_ = 0;
    multi_buffer_first_item_offset_ = -1;
    multi_buffer_offset_ = sizeof(uint16_t); // Space for sequence
    util::DataUtils::write_multi_data_indicator(multi_buffer_->full_span(), multi_buffer_offset_);
}

std::vector<uint8_t> ReliableDataOutputChannel::encrypt(
    std::span<const uint8_t> data,
    std::span<const uint8_t>& output
) {
    // Note the logic for ensuring that encrypted data which begins with a zero,
    // gets prefixed with a 0

    std::vector<uint8_t> storage(data.size() + 1);
    storage[0] = 0;

    // We can assume the key state is not null, as encryption cannot be enabled
    // by the application without setting a key state
    std::span<uint8_t> output_span(storage.data() + 1, data.size());
    Rc4Cipher::transform(data, output_span, *cipher_state_);

    if (storage[1] == 0) {
        output = storage;
    } else {
        output = std::span<const uint8_t>(storage.data() + 1, data.size());
    }

    return storage;
}

int64_t ReliableDataOutputChannel::get_true_incoming_sequence(uint16_t packet_sequence) const noexcept {
    return util::DataUtils::get_true_incoming_sequence(
        packet_sequence,
        window_start_sequence_,
        session_params_.max_queued_outgoing_reliable_data_packets()
    );
}

} // namespace sanctuary::soe_protocol::services
