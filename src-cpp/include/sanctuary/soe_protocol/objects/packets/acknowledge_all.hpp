#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::objects::packets {

/// <summary>
/// Represents a packet used to acknowledge all outstanding data sequences up to the given sequence.
/// </summary>
struct AcknowledgeAll {
    /// <summary>
    /// The sequence number.
    /// </summary>
    uint16_t sequence;

    /// <summary>
    /// Gets the buffer size required to serialize an
    /// AcknowledgeAll packet.
    /// </summary>
    static constexpr size_t SIZE = sizeof(uint16_t);

    /// <summary>
    /// Deserializes an AcknowledgeAll packet from a buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    /// <returns>The deserialized packet.</returns>
    static AcknowledgeAll deserialize(std::span<const uint8_t> buffer);

    /// <summary>
    /// Serializes this AcknowledgeAll packet to a buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    void serialize(std::span<uint8_t> buffer) const;
};

} // namespace sanctuary::soe_protocol::objects::packets
