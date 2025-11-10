#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace sanctuary::soe_protocol::objects::packets {

/// <summary>
/// Represents a packet used to request a session.
/// </summary>
struct SessionRequest {
    /// <summary>
    /// The version of the SOE protocol in use.
    /// </summary>
    uint32_t soe_protocol_version;

    /// <summary>
    /// A randomly generated session identifier.
    /// </summary>
    uint32_t session_id;

    /// <summary>
    /// The maximum length of a UDP packet that the sender can receive.
    /// </summary>
    uint32_t udp_length;

    /// <summary>
    /// The application protocol that the sender wishes to transport.
    /// </summary>
    std::string application_protocol;

    /// <summary>
    /// Gets the minimum size of a buffer required to serialize
    /// a SessionRequest packet.
    /// </summary>
    static constexpr size_t MIN_SIZE = sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + 1;

    /// <summary>
    /// Deserializes a SessionRequest packet from a buffer.
    /// This method does not expect an OP code in the buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    /// <param name="has_opcode">Indicates whether the buffer contains an OP code.</param>
    /// <returns>The deserialized packet.</returns>
    static SessionRequest deserialize(std::span<const uint8_t> buffer, bool has_opcode);

    /// <summary>
    /// Gets the buffer size required to serialize this
    /// SessionRequest packet.
    /// </summary>
    [[nodiscard]] size_t get_size() const;

    /// <summary>
    /// Serializes this SessionRequest packet to a buffer.
    /// This method writes the OP code to the buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    void serialize(std::span<uint8_t> buffer) const;
};

} // namespace sanctuary::soe_protocol::objects::packets
