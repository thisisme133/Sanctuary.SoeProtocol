#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::objects {
    enum class SessionMode : uint8_t; // Forward declaration
    enum class SessionState : uint8_t; // Forward declaration
    enum class DisconnectReason : uint8_t; // Forward declaration
}

namespace sanctuary::soe_protocol::abstractions {

using sanctuary::soe_protocol::objects::SessionMode;
using sanctuary::soe_protocol::objects::SessionState;
using sanctuary::soe_protocol::objects::DisconnectReason;

/// <summary>
/// Represents an object capable of handling an SOE protocol session.
/// </summary>
class SessionHandler {
public:
    virtual ~SessionHandler() = default;

    /// <summary>
    /// Gets the operating mode of the session handler.
    /// </summary>
    [[nodiscard]] virtual SessionMode mode() const = 0;

    /// <summary>
    /// Gets the current state of the session handler.
    /// </summary>
    [[nodiscard]] virtual SessionState state() const = 0;

    /// <summary>
    /// Gets the ID of the session. This will return <c>0</c>
    /// if a session has not yet been negotiated.
    /// </summary>
    [[nodiscard]] virtual uint32_t session_id() const = 0;

    /// <summary>
    /// Gets the reason that the session handler was terminated.
    /// Will return <see cref="DisconnectReason::None"/> if the handler
    /// has not yet been terminated.
    /// </summary>
    [[nodiscard]] virtual DisconnectReason termination_reason() const = 0;

    /// <summary>
    /// Indicates whether the session was terminated by the remote party.
    /// </summary>
    [[nodiscard]] virtual bool terminated_by_remote() const = 0;

    /// <summary>
    /// Enqueues data to be sent to the other party.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <returns><c>True</c> if the data was enqueued, otherwise <c>false</c>.</returns>
    [[nodiscard]] virtual bool enqueue_data(std::span<const uint8_t> data) = 0;

    /// <summary>
    /// Terminates the session handler.
    /// </summary>
    virtual void terminate_session() = 0;
};

} // namespace sanctuary::soe_protocol::abstractions
