#pragma once

#include "sanctuary/soe_protocol/objects/application_parameters.hpp"
#include "sanctuary/soe_protocol/objects/data_input_stats.hpp"
#include "sanctuary/soe_protocol/objects/native_span.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge_all.hpp"
#include "sanctuary/soe_protocol/objects/rc4_key_state.hpp"
#include "sanctuary/soe_protocol/objects/session_parameters.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

// Forward declarations
namespace sanctuary::soe_protocol {
    class SoeProtocolHandler;
    enum class SoeOpCode : uint16_t;
}

namespace sanctuary::soe_protocol::util {
    class NativeSpanPool;
}

namespace sanctuary::soe_protocol::services {

/// <summary>
/// Contains logic to handle reliable data packets and extract the proxied application data.
/// </summary>
class ReliableDataInputChannel {
public:
    /// <summary>
    /// A delegate that will be called when application data is received.
    /// </summary>
    using DataHandler = std::function<void(std::span<const uint8_t>)>;

    /// <summary>
    /// Initializes a new instance of the ReliableDataInputChannel.
    /// </summary>
    /// <param name="handler">The protocol handler that owns this channel.</param>
    /// <param name="session_params">The session parameters.</param>
    /// <param name="app_params">The application parameters.</param>
    /// <param name="span_pool">A pool that may be used to stash out-of-order fragments.</param>
    /// <param name="data_handler">The handler for processed data.</param>
    ReliableDataInputChannel(
        SoeProtocolHandler& handler,
        const objects::SessionParameters& session_params,
        const objects::ApplicationParameters& app_params,
        util::NativeSpanPool& span_pool,
        DataHandler data_handler
    );

    ~ReliableDataInputChannel();
    ReliableDataInputChannel(const ReliableDataInputChannel&) = delete;
    ReliableDataInputChannel(ReliableDataInputChannel&&) noexcept = default;
    ReliableDataInputChannel& operator=(const ReliableDataInputChannel&) = delete;
    ReliableDataInputChannel& operator=(ReliableDataInputChannel&&) noexcept = default;

    /// <summary>
    /// Gets the input statistics.
    /// </summary>
    [[nodiscard]] const objects::DataInputStats& input_stats() const noexcept {
        return input_stats_;
    }

    /// <summary>
    /// Runs a tick of the ReliableDataInputChannel operations. This includes acknowledging processed
    /// data packets.
    /// </summary>
    void run_tick();

    /// <summary>
    /// Handles a ReliableData packet.
    /// </summary>
    /// <param name="data">The reliable data.</param>
    void handle_reliable_data(std::span<uint8_t> data);

    /// <summary>
    /// Handles a ReliableDataFragment packet.
    /// </summary>
    /// <param name="data">The reliable data fragment.</param>
    void handle_reliable_data_fragment(std::span<uint8_t> data);

private:
    /// <summary>
    /// Stores fragments that compose the current piece of reliable data.
    /// </summary>
    struct StashedData {
        std::shared_ptr<objects::NativeSpan> span;
        bool is_fragment{false};
    };

    SoeProtocolHandler& handler_;
    const objects::SessionParameters& session_params_;
    const objects::ApplicationParameters& application_params_;
    util::NativeSpanPool& span_pool_;
    DataHandler data_handler_;
    std::vector<StashedData> stash_;

    std::shared_ptr<objects::Rc4KeyState> cipher_state_;
    /// The next reliable data sequence that we expect to receive.
    int64_t window_start_sequence_{0};
    std::optional<objects::packets::AcknowledgeAll> buffered_ack_all_;
    /// Stores fragments that compose the current piece of reliable data.
    std::vector<uint8_t> current_buffer_;
    /// The current length of the data that has been received into the current_buffer_.
    size_t running_data_length_{0};
    /// The expected length of the data that should be received into the current_buffer_.
    size_t expected_data_length_{0};
    /// The last reliable data sequence that we acknowledged.
    int64_t last_ack_all_sequence_{-1};
    std::chrono::steady_clock::time_point last_ack_all_time_;

    objects::DataInputStats input_stats_;

    void send_ack_all(const objects::packets::AcknowledgeAll& ack_all);

    /// <summary>
    /// Pre-processes reliable data, and stashes it if required.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <param name="is_fragment">Indicates whether this data is a reliable fragment.</param>
    /// <returns>True if the processed data should be used, otherwise false.</returns>
    bool preprocess_data(std::span<uint8_t>& data, bool is_fragment);

    /// <summary>
    /// Checks whether the given reliable data is valid for processing, by ensuring
    /// that it is within the current window, and we haven't already processed it.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <param name="sequence">The true sequence.</param>
    /// <param name="packet_sequence">The embedded packet sequence.</param>
    /// <returns>Whether we should process this data.</returns>
    bool is_valid_reliable_data(std::span<const uint8_t> data, int64_t& sequence, uint16_t& packet_sequence);

    /// <summary>
    /// Writes a fragment to the current_buffer_. If this is not allocated, the fragment in the
    /// data will be assumed to be a master fragment (i.e. has the length of the full data packet)
    /// and a new current_buffer_ will be allocated to this len.
    /// </summary>
    /// <param name="data">The data.</param>
    void write_immediate_fragment_to_buffer(std::span<const uint8_t> data);

    void try_process_current_buffer();
    void consume_stashed_data_fragments();
    void process_data(std::span<uint8_t> data);
    void decrypt_and_call_data_handler(std::span<uint8_t> data);
};

} // namespace sanctuary::soe_protocol::services
