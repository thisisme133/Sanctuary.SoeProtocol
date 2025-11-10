#pragma once

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Enumerates the modes that a SoeProtocolHandler can be in.
/// </summary>
enum class SessionMode {
    /// <summary>
    /// The handler should act as a client.
    /// </summary>
    Client,

    /// <summary>
    /// The handler should act as a server.
    /// </summary>
    Server
};

} // namespace sanctuary::soe_protocol::objects
