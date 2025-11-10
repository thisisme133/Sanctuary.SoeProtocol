#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::objects::packets {

/// <summary>
/// Represents a packet used to confirm a session request.
/// </summary>
struct SessionResponse {
    /// <summary>
    /// The ID of the session to confirm.
    /// </summary>
    uint32_t session_id;

    /// <summary>
    /// A randomly generated seed used to calculate the CRC-32 check value on relevant packets.
    /// </summary>
    uint32_t crc_seed;

    /// <summary>
    /// The number of bytes that should be used to store the CRC-32 check value on relevant packets.
    /// </summary>
    uint8_t crc_length;

    /// <summary>
    /// A value indicating whether relevant packets may be compressed.
    /// </summary>
    bool is_compression_enabled;

    /// <summary>
    /// Unknown. Always observed to be 0.
    /// </summary>
    uint8_t unknown_value1;

    /// <summary>
    /// The maximum length of a UDP packet that the sender can receive.
    /// </summary>
    uint32_t udp_length;

    /// <summary>
    /// The version of the SOE protocol that is in use.
    /// </summary>
    uint32_t soe_protocol_version;

    /// <summary>
    /// Gets the buffer size required to serialize a
    /// SessionResponse packet.
    /// </summary>
    static constexpr size_t SIZE = sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(bool) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t);

    /// <summary>
    /// Deserializes a SessionResponse packet from a buffer.
    /// This method does not expect the OP code in the buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    /// <param name="has_opcode">Indicates whether the buffer contains an OP code.</param>
    /// <returns>The deserialized packet.</returns>
    static SessionResponse deserialize(std::span<const uint8_t> buffer, bool has_opcode);

    /// <summary>
    /// Serializes this SessionResponse packet to a buffer.
    /// This method writes the OP code to the buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    void serialize(std::span<uint8_t> buffer) const;
};

} // namespace sanctuary::soe_protocol::objects::packets
