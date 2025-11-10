#include "sanctuary/soe_protocol/objects/packets/disconnect.hpp"
#include <bit>
#include <cstring>

namespace sanctuary::soe_protocol::objects::packets {

Disconnect Disconnect::deserialize(std::span<const uint8_t> buffer) {
    Disconnect packet{};

    // Read session_id (big-endian uint32_t)
    uint32_t session_id_be;
    std::memcpy(&session_id_be, buffer.data(), sizeof(uint32_t));
    packet.session_id = std::byteswap(session_id_be);

    // Read reason (big-endian uint16_t)
    uint16_t reason_be;
    std::memcpy(&reason_be, buffer.data() + sizeof(uint32_t), sizeof(uint16_t));
    packet.reason = static_cast<DisconnectReason>(std::byteswap(reason_be));

    return packet;
}

void Disconnect::serialize(std::span<uint8_t> buffer) const {
    // Write session_id (big-endian uint32_t)
    uint32_t session_id_be = std::byteswap(session_id);
    std::memcpy(buffer.data(), &session_id_be, sizeof(uint32_t));

    // Write reason (big-endian uint16_t)
    uint16_t reason_be = std::byteswap(static_cast<uint16_t>(reason));
    std::memcpy(buffer.data() + sizeof(uint32_t), &reason_be, sizeof(uint16_t));
}

} // namespace sanctuary::soe_protocol::objects::packets
