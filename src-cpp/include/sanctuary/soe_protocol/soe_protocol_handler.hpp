#pragma once

#include "sanctuary/soe_protocol/abstractions/application_protocol_handler.hpp"
#include "sanctuary/soe_protocol/abstractions/services/network_writer.hpp"
#include "sanctuary/soe_protocol/abstractions/session_handler.hpp"
#include "sanctuary/soe_protocol/abstractions/soe_session.hpp"
#include "sanctuary/soe_protocol/endpoint.hpp"
#include "sanctuary/soe_protocol/objects/application_parameters.hpp"
#include "sanctuary/soe_protocol/objects/data_input_stats.hpp"
#include "sanctuary/soe_protocol/objects/data_output_stats.hpp"
#include "sanctuary/soe_protocol/objects/native_span.hpp"
#include "sanctuary/soe_protocol/objects/session_mode.hpp"
#include "sanctuary/soe_protocol/objects/session_parameters.hpp"
#include "sanctuary/soe_protocol/objects/session_state.hpp"
#include "sanctuary/soe_protocol/services/reliable_data_input_channel.hpp"
#include "sanctuary/soe_protocol/services/reliable_data_output_channel.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <span>
#include <stop_token>
#include <vector>

// Forward declarations
namespace sanctuary::soe_protocol::util {
    class NativeSpanPool;
}

namespace sanctuary::soe_protocol {

/// <summary>
/// Represents a generic handler for the SOE protocol.
/// </summary>
class SoeProtocolHandler
    : public abstractions::SessionHandler
    , public abstractions::SoeSession {
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SoeProtocolHandler"/> class.
    /// </summary>
    /// <param name="remote">The address of the remote that this session is connected to.</param>
    /// <param name="mode">The mode that the handler should operate in.</param>
    /// <param name="session_parameters">
    /// The session parameters to conform to. This object will be disposed with the handler.
    /// </param>
    /// <param name="span_pool">The native span pool to use.</param>
    /// <param name="network_writer">The network writer to send packets on.</param>
    /// <param name="application">The proxied application.</param>
    SoeProtocolHandler(
        EndPoint remote,
        objects::SessionMode mode,
        std::unique_ptr<objects::SessionParameters> session_parameters,
        util::NativeSpanPool& span_pool,
        std::unique_ptr<abstractions::services::NetworkWriter> network_writer,
        std::unique_ptr<abstractions::ApplicationProtocolHandler> application
    );

    ~SoeProtocolHandler() override;
    SoeProtocolHandler(const SoeProtocolHandler&) = delete;
    SoeProtocolHandler(SoeProtocolHandler&&) = delete;
    SoeProtocolHandler& operator=(const SoeProtocolHandler&) = delete;
    SoeProtocolHandler& operator=(SoeProtocolHandler&&) = delete;

    /// <summary>
    /// Gets the address of the remote that this session is connected to.
    /// </summary>
    [[nodiscard]] const EndPoint& remote() const noexcept { return remote_; }

    /// <summary>
    /// Gets a mutable reference to the remote endpoint.
    /// </summary>
    [[nodiscard]] EndPoint& remote() noexcept { return remote_; }

    /// <summary>
    /// Gets the session parameters in use by the session.
    /// </summary>
    [[nodiscard]] const objects::SessionParameters& session_params() const noexcept {
        return *session_params_;
    }

    /// <summary>
    /// Gets the application parameters in use by the session.
    /// </summary>
    [[nodiscard]] const objects::ApplicationParameters& application_params() const noexcept {
        return application_params_;
    }

    // SessionHandler interface implementation
    [[nodiscard]] objects::SessionMode mode() const override { return mode_; }
    [[nodiscard]] objects::SessionState state() const override { return state_; }
    [[nodiscard]] uint32_t session_id() const override { return session_id_; }
    [[nodiscard]] objects::DisconnectReason termination_reason() const override { return termination_reason_; }
    [[nodiscard]] bool terminated_by_remote() const override { return terminated_by_remote_; }
    [[nodiscard]] bool enqueue_data(std::span<const uint8_t> data) override;
    void terminate_session() override;

    // SoeSession interface implementation
    void send_contextual_packet(SoeOpCode op_code, std::span<const uint8_t> packet_data) override;

    /// <summary>
    /// Enqueues a packet for processing. The packet may be dropped
    /// if the handler is overloaded.
    /// </summary>
    /// <param name="packet_data">The packet data.</param>
    /// <returns><c>True</c> if the packet was successfully enqueued, otherwise <c>false</c>.</returns>
    [[nodiscard]] bool enqueue_packet(std::unique_ptr<objects::NativeSpan> packet_data);

    /// <summary>
    /// Initializes the handler. This method should be called before
    /// <see cref="run_tick"/>.
    /// </summary>
    void initialize();

    /// <summary>
    /// Runs a single iteration of the handler's logic.
    /// </summary>
    /// <param name="needs_more_time">
    /// A value indicating whether <see cref="run_tick"/> should be called again as soon as possible.
    /// </param>
    /// <param name="stop_token">A <see cref="stop_token"/> that can be used to stop the operation.</param>
    /// <returns>
    /// <c>True</c> if the iteration ran successfully, otherwise <c>false</c>. In this case,
    /// the handler should be considered terminated.
    /// </returns>
    [[nodiscard]] bool run_tick(bool& needs_more_time, std::stop_token stop_token);

    /// <summary>
    /// Terminates the session. This may be called whenever the session needs to close,
    /// e.g. when the other party has disconnected, or an internal error has occurred.
    /// </summary>
    /// <param name="reason">The termination reason.</param>
    /// <param name="notify_remote">Whether to notify the remote party.</param>
    /// <param name="terminated_by_remote">Indicates whether this termination has come from the remote party.</param>
    void terminate_session(objects::DisconnectReason reason, bool notify_remote, bool terminated_by_remote = false);

    /// <summary>
    /// Sends a session request to the remote. The underlying network writer must be connected,
    /// and the handler must be in client mode, and ready for negotiation.
    /// </summary>
    /// <exception cref="std::runtime_error">
    /// Thrown if the handler is not ready, or the <see cref="SessionParameters::application_protocol"/> is too long.
    /// </exception>
    void send_session_request();

    /// <summary>
    /// Gets statistics related to receiving reliable data.
    /// </summary>
    [[nodiscard]] const objects::DataInputStats& reliable_data_receive_stats() const noexcept {
        return data_input_channel_->input_stats();
    }

    /// <summary>
    /// Gets statistics related to sending reliable data.
    /// </summary>
    [[nodiscard]] const objects::DataOutputStats& reliable_data_send_stats() const noexcept {
        return data_output_channel_->output_stats();
    }

private:
    util::NativeSpanPool& span_pool_;
    std::unique_ptr<abstractions::services::NetworkWriter> network_writer_;
    std::unique_ptr<abstractions::ApplicationProtocolHandler> application_;
    std::queue<std::unique_ptr<objects::NativeSpan>> packet_queue_;
    std::mutex packet_queue_mutex_;
    std::unique_ptr<services::ReliableDataInputChannel> data_input_channel_;
    std::unique_ptr<services::ReliableDataOutputChannel> data_output_channel_;

    EndPoint remote_;
    std::unique_ptr<objects::SessionParameters> session_params_;
    objects::ApplicationParameters application_params_;
    objects::SessionMode mode_;
    objects::SessionState state_;
    uint32_t session_id_{0};
    objects::DisconnectReason termination_reason_{objects::DisconnectReason::None};
    bool terminated_by_remote_{false};

    bool open_session_on_next_client_packet_{false};
    std::chrono::steady_clock::time_point last_received_packet_time_;
    std::vector<uint8_t> contextual_send_buffer_;

    // Contextless packet handling
    void handle_contextless_packet(SoeOpCode op_code, std::span<const uint8_t> packet_data);
    void handle_session_request(std::span<const uint8_t> packet_data);
    void handle_session_response(std::span<const uint8_t> packet_data);

    // Contextual packet handling
    void handle_contextual_packet(SoeOpCode op_code, std::span<uint8_t> packet_data);
    void handle_contextual_packet_internal(SoeOpCode op_code, std::span<uint8_t> packet_data);
    void send_heartbeat_if_required();

    // Core processing
    [[nodiscard]] bool process_one_from_packet_queue();
    void process_packet_core(std::span<uint8_t> packet_data, bool validate, SoeOpCode op_code = SoeOpCode::Invalid);
    [[nodiscard]] int32_t calculate_max_data_length() const;
};

} // namespace sanctuary::soe_protocol
