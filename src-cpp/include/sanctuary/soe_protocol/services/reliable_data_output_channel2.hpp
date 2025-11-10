#pragma once

#include "sanctuary/soe_protocol/objects/application_parameters.hpp"
#include "sanctuary/soe_protocol/objects/native_span.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge_all.hpp"
#include "sanctuary/soe_protocol/objects/rc4_key_state.hpp"
#include "sanctuary/soe_protocol/objects/session_parameters.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
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
/// A much-simplified version of the ReliableDataOutputChannel
/// that does not support multi-packets and is less performant, but also
/// isn't continuously spawning bugs and hard-to-debug logic.
/// </summary>
class ReliableDataOutputChannel2 {
public:
    /// <summary>
    /// Gets the maximum amount of time to wait for an acknowledgement
    /// </summary>
    static constexpr int ACK_WAIT_MILLISECONDS = 500; // TODO: High ping could mess this up. Needs to be dynamic

    /// <summary>
    /// Initializes a new instance of the ReliableDataOutputChannel2 class.
    /// </summary>
    /// <param name="handler">The parent handler.</param>
    /// <param name="span_pool">The native span pool to use.</param>
    /// <param name="max_data_length">The maximum length of data that may be sent by the output channel.</param>
    ReliableDataOutputChannel2(
        SoeProtocolHandler& handler,
        util::NativeSpanPool& span_pool,
        int32_t max_data_length
    );

    ~ReliableDataOutputChannel2();
    ReliableDataOutputChannel2(const ReliableDataOutputChannel2&) = delete;
    ReliableDataOutputChannel2(ReliableDataOutputChannel2&&) noexcept = default;
    ReliableDataOutputChannel2& operator=(const ReliableDataOutputChannel2&) = delete;
    ReliableDataOutputChannel2& operator=(ReliableDataOutputChannel2&&) noexcept = default;

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
    void set_max_data_length(int32_t max_data_length) noexcept;

private:
    struct StashedOutputPacket {
        bool is_fragment{false};
        std::shared_ptr<objects::NativeSpan> data_span;
    };

    SoeProtocolHandler& handler_;
    const objects::SessionParameters& session_params_;
    const objects::ApplicationParameters& application_params_;
    util::NativeSpanPool& span_pool_;
    std::vector<std::pair<int64_t, StashedOutputPacket>> dispatch_queue_;
    std::mutex packet_output_queue_lock_;

    // Data-related
    std::shared_ptr<objects::Rc4KeyState> cipher_state_;
    int32_t max_data_length_{0};

    /// The total number of sequences that have been output.
    int64_t total_sequence_{0};
    /// The maximum sequence number that the client knows about
    int64_t max_client_sequence_{0};
    std::atomic<int32_t> current_dispatch_index_{0};

    std::chrono::steady_clock::time_point last_ack_at_;

    void stash_fragment(std::span<const uint8_t>& data, bool is_master, bool is_fragment);
    [[nodiscard]] std::vector<uint8_t> encrypt(std::span<const uint8_t> data, std::span<const uint8_t>& output);
    [[nodiscard]] int64_t get_true_incoming_sequence(uint16_t packet_sequence) const noexcept;
};

} // namespace sanctuary::soe_protocol::services
