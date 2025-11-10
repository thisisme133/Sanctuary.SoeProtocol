#pragma once

#include <cstdint>

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Enumerates the possible results of validating an SOE packet.
/// </summary>
enum class SoePacketValidationResult : uint8_t {
    /// <summary>
    /// The packet is valid.
    /// </summary>
    Valid = 0,

    /// <summary>
    /// The packet is too short, for its type.
    /// </summary>
    TooShort = 1,

    /// <summary>
    /// The packet failed CRC validation.
    /// </summary>
    CrcMismatch = 2,

    /// <summary>
    /// The packet had an unknown OP code.
    /// </summary>
    InvalidOpCode = 3
};

} // namespace sanctuary::soe_protocol::objects
