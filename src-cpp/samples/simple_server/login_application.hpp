#pragma once

#include <sanctuary/soe_protocol/abstractions/application_protocol_handler.hpp>
#include <sanctuary/soe_protocol/abstractions/session_handler.hpp>
#include <sanctuary/soe_protocol/objects/application_parameters.hpp>
#include <sanctuary/soe_protocol/objects/packets/disconnect.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <span>

namespace simple_server {

/// <summary>
/// Represents the login application protocol handler.
/// Handles incoming connections and application data for login sessions.
/// </summary>
class login_application : public sanctuary::soe_protocol::application_protocol_handler {
public:
    /// <summary>
    /// Constructs a new login application handler.
    /// </summary>
    explicit login_application();

    /// <summary>
    /// Gets the application session parameters.
    /// </summary>
    /// <returns>The session parameters including encryption settings.</returns>
    [[nodiscard]] auto session_params() const -> const sanctuary::soe_protocol::application_parameters& override;

    /// <summary>
    /// Initializes the application with a session handler.
    /// </summary>
    /// <param name="session_handler">The session handler to use for this application.</param>
    auto initialise(std::shared_ptr<sanctuary::soe_protocol::session_handler> session_handler) -> void override;

    /// <summary>
    /// Called when the session is successfully opened.
    /// </summary>
    auto on_session_opened() -> void override;

    /// <summary>
    /// Handles incoming application data.
    /// </summary>
    /// <param name="data">The application data received.</param>
    auto handle_app_data(std::span<const std::byte> data) -> void override;

    /// <summary>
    /// Called when the session is closed.
    /// </summary>
    /// <param name="disconnect_reason">The reason for disconnection.</param>
    auto on_session_closed(sanctuary::soe_protocol::disconnect_reason disconnect_reason) -> void override;

private:
    std::shared_ptr<sanctuary::soe_protocol::session_handler> session_handler_;
    sanctuary::soe_protocol::application_parameters session_params_;

    /// <summary>
    /// Terminates the login session.
    /// N.B. login sessions don't terminate at the app level, only at the SOE level
    /// </summary>
    auto terminate_login_session() -> void;
};

} // namespace simple_server
