#pragma once

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Enumerates the states that a SoeProtocolHandler can be in.
/// </summary>
enum class SessionState {
    /// <summary>
    /// The handler is negotiating a session.
    /// </summary>
    Negotiating,

    /// <summary>
    /// The handler is running.
    /// </summary>
    Running,

    /// <summary>
    /// The handler has terminated.
    /// </summary>
    Terminated
};

} // namespace sanctuary::soe_protocol::objects
