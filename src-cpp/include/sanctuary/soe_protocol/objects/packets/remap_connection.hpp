#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::objects::packets {

/// <summary>
/// Represents a packet used to remap an existing session to a new port.
/// </summary>
struct RemapConnection {
    /// <summary>
    /// The ID of the session to remap.
    /// </summary>
    uint32_t session_id;

    /// <summary>
    /// The CRC seed being used in the session.
    /// </summary>
    uint32_t crc_seed;

    /// <summary>
    /// Gets the buffer size required to serialize a
    /// RemapConnection packet.
    /// </summary>
    static constexpr size_t SIZE = sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t); // SoeOpCode + SessionId + CrcSeed

    /// <summary>
    /// Deserializes a RemapConnection packet from a buffer.
    /// This method does not expect the OP code in the buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    /// <param name="has_opcode">Indicates whether the buffer contains an OP code.</param>
    /// <returns>The deserialized packet.</returns>
    static RemapConnection deserialize(std::span<const uint8_t> buffer, bool has_opcode);

    /// <summary>
    /// Serializes this RemapConnection packet to a buffer.
    /// This method writes the OP code to the buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    void serialize(std::span<uint8_t> buffer) const;
};

} // namespace sanctuary::soe_protocol::objects::packets
