#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol {
    enum class SoeOpCode : uint16_t; // Forward declaration
}

namespace sanctuary::soe_protocol::abstractions {

/// <summary>
/// Represents an SOE session.
/// </summary>
class SoeSession {
public:
    virtual ~SoeSession() = default;

    /// <summary>
    /// The ID of the session. This will return <c>0</c> if a session has not yet been negotiated.
    /// </summary>
    [[nodiscard]] virtual uint32_t session_id() const = 0;

    /// <summary>
    /// Send a contextual SOE packet to the remote.
    /// </summary>
    /// <param name="op_code">The OP code of the packet.</param>
    /// <param name="packet_data">The packet data. Do not wrap the data with frame details such as the CRC.</param>
    virtual void send_contextual_packet(SoeOpCode op_code, std::span<const uint8_t> packet_data) = 0;
};

} // namespace sanctuary::soe_protocol::abstractions
