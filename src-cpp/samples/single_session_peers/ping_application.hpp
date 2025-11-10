#pragma once

#include <sanctuary/soe_protocol/abstractions/application_protocol_handler.hpp>
#include <sanctuary/soe_protocol/abstractions/session_handler.hpp>
#include <sanctuary/soe_protocol/objects/application_parameters.hpp>
#include <sanctuary/soe_protocol/objects/packets/disconnect.hpp>
#include <memory>
#include <span>
#include <chrono>

namespace single_session_peers {

/// <summary>
/// Represents a protocol handler for the Ping application.
/// Implements a ping-pong throughput test between client and server.
/// </summary>
class ping_application : public sanctuary::soe_protocol::application_protocol_handler {
public:
    /// <summary>
    /// Constructs a new ping application handler.
    /// </summary>
    explicit ping_application();

    /// <summary>
    /// Gets the application session parameters.
    /// </summary>
    /// <returns>The session parameters (no encryption for this simple example).</returns>
    [[nodiscard]] auto session_params() const -> const sanctuary::soe_protocol::application_parameters& override;

    /// <summary>
    /// Initializes the application with a session handler.
    /// </summary>
    /// <param name="session_handler">The session handler to use for this application.</param>
    auto initialise(std::shared_ptr<sanctuary::soe_protocol::session_handler> session_handler) -> void override;

    /// <summary>
    /// Called when the session is successfully opened.
    /// Starts the ping-pong test by sending the first "Ping!" message (client only).
    /// </summary>
    auto on_session_opened() -> void override;

    /// <summary>
    /// Handles incoming application data.
    /// Responds with "Pong!" if received "Ping!", or "Ping!" if received "Pong!".
    /// </summary>
    /// <param name="data">The application data received.</param>
    auto handle_app_data(std::span<const std::byte> data) -> void override;

    /// <summary>
    /// Called when the session is closed.
    /// Logs the throughput statistics.
    /// </summary>
    /// <param name="disconnect_reason">The reason for disconnection.</param>
    auto on_session_closed(sanctuary::soe_protocol::disconnect_reason disconnect_reason) -> void override;

private:
    std::shared_ptr<sanctuary::soe_protocol::session_handler> session_handler_;
    sanctuary::soe_protocol::application_parameters session_params_;
    std::chrono::steady_clock::time_point session_start_;
    int64_t receive_count_{0};

    static constexpr std::chrono::seconds ping_pong_duration_{10};

    /// <summary>
    /// Gets the mode prefix string for logging (Client/Server/Unknown).
    /// </summary>
    [[nodiscard]] auto get_mode_prefix() const -> std::string;
};

} // namespace single_session_peers
