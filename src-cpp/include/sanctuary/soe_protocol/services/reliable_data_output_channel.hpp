#pragma once

#include "sanctuary/soe_protocol/objects/application_parameters.hpp"
#include "sanctuary/soe_protocol/objects/data_output_stats.hpp"
#include "sanctuary/soe_protocol/objects/native_span.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge_all.hpp"
#include "sanctuary/soe_protocol/objects/rc4_key_state.hpp"
#include "sanctuary/soe_protocol/objects/session_parameters.hpp"
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <span>
#include <stop_token>
#include <vector>

// Forward declarations
namespace sanctuary::soe_protocol {
    class SoeProtocolHandler;
}

namespace sanctuary::soe_protocol::util {
    class NativeSpanPool;
}

namespace sanctuary::soe_protocol::services {

/// <summary>
/// Contains logic to convert application data into reliable data packets.
/// </summary>
class ReliableDataOutputChannel {
public:
    /// <summary>
    /// Gets the maximum amount of time to wait for an acknowledgement
    /// </summary>
    static constexpr int ACK_WAIT_MILLISECONDS = 500; // TODO: High ping could mess this up. Needs to be dynamic

    /// <summary>
    /// Initializes a new instance of the ReliableDataOutputChannel class.
    /// </summary>
    /// <param name="handler">The parent handler.</param>
    /// <param name="span_pool">The native span pool to use.</param>
    /// <param name="max_data_length">The maximum length of data that may be sent by the output channel.</param>
    ReliableDataOutputChannel(
        SoeProtocolHandler& handler,
        util::NativeSpanPool& span_pool,
        int32_t max_data_length
    );

    ~ReliableDataOutputChannel();
    ReliableDataOutputChannel(const ReliableDataOutputChannel&) = delete;
    ReliableDataOutputChannel(ReliableDataOutputChannel&&) noexcept = default;
    ReliableDataOutputChannel& operator=(const ReliableDataOutputChannel&) = delete;
    ReliableDataOutputChannel& operator=(ReliableDataOutputChannel&&) noexcept = default;

    /// <summary>
    /// Gets the data output statistics.
    /// </summary>
    [[nodiscard]] const objects::DataOutputStats& output_stats() const noexcept {
        return output_stats_;
    }

    /// <summary>
    /// Enqueues data to be sent on the reliable channel.
    /// </summary>
    /// <param name="data">The data.</param>
    void enqueue_data(std::span<const uint8_t> data);

    /// <summary>
    /// Runs a tick of the output channel, which will send queued data.
    /// </summary>
    /// <param name="stop_token">A stop_token that can be used to stop the operation.</param>
    void run_tick(std::stop_token stop_token);

    /// <summary>
    /// Notifies the channel of an acknowledgement packet.
    /// </summary>
    /// <param name="ack">The acknowledgement.</param>
    void notify_of_acknowledge(const objects::packets::Acknowledge& ack);

    /// <summary>
    /// Notifies the channel of an acknowledgement packet.
    /// </summary>
    /// <param name="ack_all">The acknowledgement.</param>
    void notify_of_acknowledge_all(const objects::packets::AcknowledgeAll& ack_all);

    /// <summary>
    /// Sets the maximum length of data that may be output in a single packet.
    /// </summary>
    /// <remarks>
    /// This method should not be used after any data has been enqueued on the channel, to ensure that previously
    /// queued packets do not exceed the new limit.
    /// </remarks>
    /// <param name="max_data_length">The maximum data length.</param>
    /// <exception cref="std::runtime_error">
    /// Thrown if this method is called after data has been enqueued.
    /// </exception>
    void set_max_data_length(int32_t max_data_length);

private:
    struct StashedOutputPacket {
        bool is_fragment{false};
        std::shared_ptr<objects::NativeSpan> data_span;
        std::chrono::steady_clock::time_point sent_at{std::chrono::steady_clock::time_point::min()};
    };

    SoeProtocolHandler& handler_;
    const objects::SessionParameters& session_params_;
    const objects::ApplicationParameters& application_params_;
    util::NativeSpanPool& span_pool_;
    /// Holds packets that are currently within the dispatch window.
    std::vector<StashedOutputPacket> dispatch_stash_;
    /// Holds backed-up data that can't fit into the dispatch_stash_, including the true sequence of the data
    std::deque<std::pair<int64_t, StashedOutputPacket>> dispatch_queue_;
    std::mutex packet_output_queue_lock_;

    // Data-related
    std::shared_ptr<objects::Rc4KeyState> cipher_state_;
    int32_t max_data_length_{0};

    /// The sequence number that the remote has most recently acknowledged.
    int64_t window_start_sequence_{0};
    /// The sequence number that we need to output from.
    int64_t current_sequence_{0};
    /// The total number of sequences that have been output.
    int64_t total_sequence_{0};

    /// The current multi-buffer
    std::shared_ptr<objects::NativeSpan> multi_buffer_;
    /// The current offset into the multi-buffer at which data should be written.
    int32_t multi_buffer_offset_{0};
    /// The number of items that have been written to the current multi-buffer.
    int32_t multi_buffer_item_count_{0};
    /// The offset into the multi-buffer at which the first item has been written.
    int32_t multi_buffer_first_item_offset_{-1};

    objects::DataOutputStats output_stats_;

    void process_ack(int64_t sequence);
    [[nodiscard]] int32_t get_sequence_index_in_dispatch_buffer(int64_t sequence) const noexcept;
    void enqueue_data_internal(std::span<const uint8_t> data, bool is_recursing);
    void stash_fragment(std::span<const uint8_t>& data, bool is_master);

    /// <summary>
    /// Queues the current multi-buffer as a full data packet.
    /// </summary>
    void enqueue_multi_buffer();

    void add_to_dispatch(std::shared_ptr<objects::NativeSpan> buffer, bool is_fragment);
    void setup_new_multi_buffer();
    [[nodiscard]] std::vector<uint8_t> encrypt(std::span<const uint8_t> data, std::span<const uint8_t>& output);
    [[nodiscard]] int64_t get_true_incoming_sequence(uint16_t packet_sequence) const noexcept;
};

} // namespace sanctuary::soe_protocol::services
