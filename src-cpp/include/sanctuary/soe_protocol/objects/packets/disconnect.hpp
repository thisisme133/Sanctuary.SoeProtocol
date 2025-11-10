#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::objects::packets {

/// <summary>
/// Enumerates the possible session termination codes.
/// </summary>
enum class DisconnectReason : uint16_t {
    /// <summary>
    /// No reason can be given for the disconnect.
    /// </summary>
    None = 0,

    /// <summary>
    /// An ICMP error occured, forcing the disconnect.
    /// </summary>
    IcmpError = 1,

    /// <summary>
    /// The other party has let the session become inactive.
    /// </summary>
    Timeout = 2,

    /// <summary>
    /// An internal use code, used to indicate that the other party
    /// has sent a disconnect.
    /// </summary>
    OtherSideTerminated = 3,

    /// <summary>
    /// Indicates that the session manager has been disposed of.
    /// Generally occurs when the server/client is shutting down.
    /// </summary>
    ManagerDeleted = 4,

    /// <summary>
    /// An internal use code, indicating a session request attempt has failed.
    /// </summary>
    ConnectFail = 5,

    /// <summary>
    /// The application is terminating the session.
    /// </summary>
    Application = 6,

    /// <summary>
    /// An internal use code, indicating that the session must disconnect
    /// as the other party is unreachable.
    /// </summary>
    UnreachableConnection = 7,

    /// <summary>
    /// Indicates that the session has been closed because a data sequence
    /// was not acknowledged quickly enough.
    /// </summary>
    UnacknowledgedTimeout = 8,

    /// <summary>
    /// Indicates that a session request has failed (often due to the connecting
    /// party attempting a reconnection too quickly), and a new attempt should be
    /// made after a short delay.
    /// </summary>
    NewConnectionAttempt = 9,

    /// <summary>
    /// Indicates that the application did not accept a session request.
    /// </summary>
    ConnectionRefused = 10,

    /// <summary>
    /// Indicates that the proper session negotiation flow has not been observed.
    /// </summary>
    ConnectError = 11,

    /// <summary>
    /// Indicates that a session request has probably been looped back to the sender,
    /// and it should not continue with the connection attempt.
    /// </summary>
    ConnectingToSelf = 12,

    /// <summary>
    /// Indicates that reliable data is being sent too fast to be processed.
    /// </summary>
    ReliableOverflow = 13,

    /// <summary>
    /// Indicates that the session manager has been orphaned by the application.
    /// </summary>
    ApplicationReleased = 14,

    /// <summary>
    /// Indicates that a corrupt packet was received.
    /// </summary>
    CorruptPacket = 15,

    /// <summary>
    /// Indicates that the requested SOE protocol version or
    /// application protocol is invalid.
    /// </summary>
    ProtocolMismatch = 16
};

/// <summary>
/// Represents a packet used to terminate a session.
/// </summary>
struct Disconnect {
    /// <summary>
    /// The ID of the session that is being terminated.
    /// </summary>
    uint32_t session_id;

    /// <summary>
    /// The reason for the termination.
    /// </summary>
    DisconnectReason reason;

    /// <summary>
    /// Gets the buffer size required to serialize a
    /// Disconnect packet.
    /// </summary>
    static constexpr size_t SIZE = sizeof(uint32_t) + sizeof(DisconnectReason);

    /// <summary>
    /// Deserializes a Disconnect packet from a buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    /// <returns>The deserialized packet.</returns>
    static Disconnect deserialize(std::span<const uint8_t> buffer);

    /// <summary>
    /// Serializes this Disconnect packet to a buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    void serialize(std::span<uint8_t> buffer) const;
};

} // namespace sanctuary::soe_protocol::objects::packets
