#include "sanctuary/soe_protocol/objects/packets/session_response.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include <bit>
#include <cstring>

namespace sanctuary::soe_protocol::objects::packets {

SessionResponse SessionResponse::deserialize(std::span<const uint8_t> buffer, bool has_opcode) {
    SessionResponse packet{};
    size_t offset = 0;

    // Skip OP code if present
    if (has_opcode) {
        offset += sizeof(uint16_t);
    }

    // Read session_id (big-endian uint32_t)
    uint32_t session_id_be;
    std::memcpy(&session_id_be, buffer.data() + offset, sizeof(uint32_t));
    packet.session_id = std::byteswap(session_id_be);
    offset += sizeof(uint32_t);

    // Read crc_seed (big-endian uint32_t)
    uint32_t crc_seed_be;
    std::memcpy(&crc_seed_be, buffer.data() + offset, sizeof(uint32_t));
    packet.crc_seed = std::byteswap(crc_seed_be);
    offset += sizeof(uint32_t);

    // Read crc_length (uint8_t)
    packet.crc_length = buffer[offset];
    offset += sizeof(uint8_t);

    // Read is_compression_enabled (bool/uint8_t)
    packet.is_compression_enabled = static_cast<bool>(buffer[offset]);
    offset += sizeof(uint8_t);

    // Read unknown_value1 (uint8_t)
    packet.unknown_value1 = buffer[offset];
    offset += sizeof(uint8_t);

    // Read udp_length (big-endian uint32_t)
    uint32_t udp_length_be;
    std::memcpy(&udp_length_be, buffer.data() + offset, sizeof(uint32_t));
    packet.udp_length = std::byteswap(udp_length_be);
    offset += sizeof(uint32_t);

    // Read soe_protocol_version (big-endian uint32_t)
    uint32_t soe_protocol_version_be;
    std::memcpy(&soe_protocol_version_be, buffer.data() + offset, sizeof(uint32_t));
    packet.soe_protocol_version = std::byteswap(soe_protocol_version_be);

    return packet;
}

void SessionResponse::serialize(std::span<uint8_t> buffer) const {
    size_t offset = 0;

    // Write SoeOpCode::SessionResponse (big-endian uint16_t)
    uint16_t opcode_be = std::byteswap(static_cast<uint16_t>(sanctuary::soe_protocol::SoeOpCode::SessionResponse));
    std::memcpy(buffer.data() + offset, &opcode_be, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Write session_id (big-endian uint32_t)
    uint32_t session_id_be = std::byteswap(session_id);
    std::memcpy(buffer.data() + offset, &session_id_be, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Write crc_seed (big-endian uint32_t)
    uint32_t crc_seed_be = std::byteswap(crc_seed);
    std::memcpy(buffer.data() + offset, &crc_seed_be, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Write crc_length (uint8_t)
    buffer[offset] = crc_length;
    offset += sizeof(uint8_t);

    // Write is_compression_enabled (bool/uint8_t)
    buffer[offset] = static_cast<uint8_t>(is_compression_enabled);
    offset += sizeof(uint8_t);

    // Write unknown_value1 (uint8_t)
    buffer[offset] = unknown_value1;
    offset += sizeof(uint8_t);

    // Write udp_length (big-endian uint32_t)
    uint32_t udp_length_be = std::byteswap(udp_length);
    std::memcpy(buffer.data() + offset, &udp_length_be, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Write soe_protocol_version (big-endian uint32_t)
    uint32_t soe_protocol_version_be = std::byteswap(soe_protocol_version);
    std::memcpy(buffer.data() + offset, &soe_protocol_version_be, sizeof(uint32_t));
}

} // namespace sanctuary::soe_protocol::objects::packets
