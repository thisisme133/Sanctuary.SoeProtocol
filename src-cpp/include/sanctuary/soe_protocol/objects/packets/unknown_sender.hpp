#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::objects::packets {

/// <summary>
/// Represents a packet used to indicate that the receiving party
/// does not have a session associated with the sender's address.
/// </summary>
struct UnknownSender {
    /// <summary>
    /// Gets the buffer size required to serialize a
    /// UnknownSender packet.
    /// </summary>
    static constexpr size_t SIZE = sizeof(uint16_t); // SoeOpCode

    /// <summary>
    /// Serializes an UnknownSender packet to a buffer.
    /// This method writes the OP code to the buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    static void serialize(std::span<uint8_t> buffer);
};

} // namespace sanctuary::soe_protocol::objects::packets
