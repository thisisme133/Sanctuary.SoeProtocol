#include "sanctuary/soe_protocol/services/reliable_data_output_channel2.hpp"
#include "sanctuary/soe_protocol/abstractions/soe_session.hpp"
#include "sanctuary/soe_protocol/services/rc4_cipher.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include "sanctuary/soe_protocol/util/data_utils.hpp"
#include "sanctuary/soe_protocol/util/native_span_pool.hpp"
#include <algorithm>
#include <stdexcept>

namespace sanctuary::soe_protocol::services {

// Forward declaration for SoeProtocolHandler
class SoeProtocolHandler : public abstractions::SoeSession {
public:
    virtual const objects::SessionParameters& session_params() const = 0;
    virtual const objects::ApplicationParameters& application_params() const = 0;
};

ReliableDataOutputChannel2::ReliableDataOutputChannel2(
    SoeProtocolHandler& handler,
    util::NativeSpanPool& span_pool,
    int32_t max_data_length
)
    : handler_(handler)
    , session_params_(handler.session_params())
    , application_params_(handler.application_params())
    , span_pool_(span_pool)
    , last_ack_at_(std::chrono::steady_clock::now())
{
    if (application_params_.encryption_key_state()) {
        cipher_state_ = application_params_.encryption_key_state()->copy();
    }
    set_max_data_length(max_data_length);
}

ReliableDataOutputChannel2::~ReliableDataOutputChannel2() {
    // Clean up dispatch queue
    for (auto& [seq, packet] : dispatch_queue_) {
        if (packet.data_span) {
            span_pool_.return_span(std::move(packet.data_span));
        }
    }
    dispatch_queue_.clear();
}

void ReliableDataOutputChannel2::enqueue_data(std::span<const uint8_t> data) {
    if (data.empty()) {
        return;
    }

    std::vector<uint8_t> encrypted_span;
    std::span<const uint8_t> data_to_use = data;

    if (application_params_.is_encryption_enabled()) {
        encrypted_span = encrypt(data, data_to_use);
    }

    std::lock_guard<std::mutex> lock(packet_output_queue_lock_);

    auto mutable_data = data_to_use;
    bool needs_fragmentation = data_to_use.size() > static_cast<size_t>(max_data_length_ - sizeof(uint16_t));
    stash_fragment(mutable_data, true, needs_fragmentation);
    while (!mutable_data.empty()) {
        stash_fragment(mutable_data, false, true);
    }
}

void ReliableDataOutputChannel2::run_tick(std::stop_token stop_token) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - last_ack_at_
    );
    if (elapsed.count() > ACK_WAIT_MILLISECONDS) {
        current_dispatch_index_.store(0, std::memory_order_release);
    }

    int32_t current_idx = current_dispatch_index_.load(std::memory_order_acquire);
    int32_t max_outgoing_index = std::min(
        static_cast<int32_t>(dispatch_queue_.size()),
        session_params_.max_queued_outgoing_reliable_data_packets() + current_idx
    );

    while (current_idx < max_outgoing_index) {
        if (stop_token.stop_requested()) {
            return;
        }

        const StashedOutputPacket& packet = dispatch_queue_[current_idx].second;
        SoeOpCode op_code = packet.is_fragment ? SoeOpCode::ReliableDataFragment : SoeOpCode::ReliableData;
        handler_.send_contextual_packet(op_code, packet.data_span->used_span());

        current_dispatch_index_.fetch_add(1, std::memory_order_release);
        current_idx++;
    }
}

void ReliableDataOutputChannel2::notify_of_acknowledge(const objects::packets::Acknowledge& ack) {
    int64_t seq = get_true_incoming_sequence(ack.sequence);

    std::lock_guard<std::mutex> lock(packet_output_queue_lock_);

    auto it = std::find_if(
        dispatch_queue_.begin(),
        dispatch_queue_.end(),
        [seq](const auto& pair) { return pair.first == seq; }
    );

    if (it != dispatch_queue_.end()) {
        span_pool_.return_span(std::move(it->second.data_span));
        current_dispatch_index_.fetch_sub(1, std::memory_order_release);
        dispatch_queue_.erase(it);
    }

    if (seq > max_client_sequence_) {
        max_client_sequence_ = seq;
    }

    last_ack_at_ = std::chrono::steady_clock::now();
}

void ReliableDataOutputChannel2::notify_of_acknowledge_all(const objects::packets::AcknowledgeAll& ack_all) {
    int64_t seq = get_true_incoming_sequence(ack_all.sequence);

    std::lock_guard<std::mutex> lock(packet_output_queue_lock_);

    while (!dispatch_queue_.empty() && dispatch_queue_.front().first <= seq) {
        span_pool_.return_span(std::move(dispatch_queue_.front().second.data_span));
        current_dispatch_index_.fetch_sub(1, std::memory_order_release);
        dispatch_queue_.erase(dispatch_queue_.begin());
    }

    if (seq > max_client_sequence_) {
        max_client_sequence_ = seq;
    }

    last_ack_at_ = std::chrono::steady_clock::now();
}

void ReliableDataOutputChannel2::set_max_data_length(int32_t max_data_length) noexcept {
    max_data_length_ = max_data_length;
}

void ReliableDataOutputChannel2::stash_fragment(
    std::span<const uint8_t>& data,
    bool is_master,
    bool is_fragment
) {
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

    if (is_master && is_fragment) {
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

    StashedOutputPacket packet;
    packet.is_fragment = is_fragment;
    packet.data_span = std::move(span);
    dispatch_queue_.emplace_back(total_sequence_, std::move(packet));

    total_sequence_++;
    data = data.subspan(amount_to_take);
}

std::vector<uint8_t> ReliableDataOutputChannel2::encrypt(
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

int64_t ReliableDataOutputChannel2::get_true_incoming_sequence(uint16_t packet_sequence) const noexcept {
    return util::DataUtils::get_true_incoming_sequence(
        packet_sequence,
        max_client_sequence_,
        session_params_.max_queued_outgoing_reliable_data_packets()
    );
}

} // namespace sanctuary::soe_protocol::services
