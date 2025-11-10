#include "sanctuary/soe_protocol/objects/packets/remap_connection.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include <bit>
#include <cstring>

namespace sanctuary::soe_protocol::objects::packets {

RemapConnection RemapConnection::deserialize(std::span<const uint8_t> buffer, bool has_opcode) {
    RemapConnection packet{};
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

    return packet;
}

void RemapConnection::serialize(std::span<uint8_t> buffer) const {
    size_t offset = 0;

    // Write SoeOpCode::RemapConnection (big-endian uint16_t)
    uint16_t opcode_be = std::byteswap(static_cast<uint16_t>(sanctuary::soe_protocol::SoeOpCode::RemapConnection));
    std::memcpy(buffer.data() + offset, &opcode_be, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Write session_id (big-endian uint32_t)
    uint32_t session_id_be = std::byteswap(session_id);
    std::memcpy(buffer.data() + offset, &session_id_be, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Write crc_seed (big-endian uint32_t)
    uint32_t crc_seed_be = std::byteswap(crc_seed);
    std::memcpy(buffer.data() + offset, &crc_seed_be, sizeof(uint32_t));
}

} // namespace sanctuary::soe_protocol::objects::packets
