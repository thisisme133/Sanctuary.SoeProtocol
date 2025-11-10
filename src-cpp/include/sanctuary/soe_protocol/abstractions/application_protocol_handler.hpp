#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::objects {
    struct ApplicationParameters; // Forward declaration
    enum class DisconnectReason : uint8_t; // Forward declaration
}

namespace sanctuary::soe_protocol::abstractions {
    class SessionHandler; // Forward declaration
}

namespace sanctuary::soe_protocol::abstractions {

using sanctuary::soe_protocol::objects::ApplicationParameters;
using sanctuary::soe_protocol::objects::DisconnectReason;

/// <summary>
/// Represents an application protocol handler, with appropriate hooks for the
/// underlying session handler.
/// </summary>
class ApplicationProtocolHandler {
public:
    virtual ~ApplicationProtocolHandler() = default;

    /// <summary>
    /// Gets the parameters used to control the underlying SOE session.
    /// </summary>
    [[nodiscard]] virtual const ApplicationParameters& session_params() const = 0;

    /// <summary>
    /// Initializes the application handler.
    /// </summary>
    /// <param name="session_handler">The underlying SOE session handler.</param>
    virtual void initialise(SessionHandler& session_handler) = 0;

    /// <summary>
    /// Notifies the handler that the underlying SOE session has opened.
    /// </summary>
    virtual void on_session_opened() = 0;

    /// <summary>
    /// Allows the handler to process app data. This method should not
    /// perform any long-running work.
    /// </summary>
    /// <param name="data">The application data.</param>
    virtual void handle_app_data(std::span<const uint8_t> data) = 0;

    /// <summary>
    /// Notifies the handler that the underlying SOE session has closed.
    /// </summary>
    /// <param name="disconnect_reason">The reason that the session was terminated.</param>
    virtual void on_session_closed(DisconnectReason disconnect_reason) = 0;
};

} // namespace sanctuary::soe_protocol::abstractions
