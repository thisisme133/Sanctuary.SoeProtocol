#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::objects::packets {

/// <summary>
/// Represents a packet used to acknowledge a data sequence.
/// </summary>
struct Acknowledge {
    /// <summary>
    /// The sequence number.
    /// </summary>
    uint16_t sequence;

    /// <summary>
    /// Gets the buffer size required to serialize an
    /// Acknowledge packet.
    /// </summary>
    static constexpr size_t SIZE = sizeof(uint16_t);

    /// <summary>
    /// Deserializes an Acknowledge packet from a buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    /// <returns>The deserialized packet.</returns>
    static Acknowledge deserialize(std::span<const uint8_t> buffer);

    /// <summary>
    /// Serializes this Acknowledge packet to a buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    void serialize(std::span<uint8_t> buffer) const;
};

} // namespace sanctuary::soe_protocol::objects::packets
