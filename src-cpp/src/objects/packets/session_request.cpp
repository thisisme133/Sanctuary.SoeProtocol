#include "sanctuary/soe_protocol/objects/packets/session_request.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include <bit>
#include <cstring>

namespace sanctuary::soe_protocol::objects::packets {

SessionRequest SessionRequest::deserialize(std::span<const uint8_t> buffer, bool has_opcode) {
    SessionRequest packet{};
    size_t offset = 0;

    // Skip OP code if present
    if (has_opcode) {
        offset += sizeof(uint16_t);
    }

    // Read soe_protocol_version (big-endian uint32_t)
    uint32_t soe_protocol_version_be;
    std::memcpy(&soe_protocol_version_be, buffer.data() + offset, sizeof(uint32_t));
    packet.soe_protocol_version = std::byteswap(soe_protocol_version_be);
    offset += sizeof(uint32_t);

    // Read session_id (big-endian uint32_t)
    uint32_t session_id_be;
    std::memcpy(&session_id_be, buffer.data() + offset, sizeof(uint32_t));
    packet.session_id = std::byteswap(session_id_be);
    offset += sizeof(uint32_t);

    // Read udp_length (big-endian uint32_t)
    uint32_t udp_length_be;
    std::memcpy(&udp_length_be, buffer.data() + offset, sizeof(uint32_t));
    packet.udp_length = std::byteswap(udp_length_be);
    offset += sizeof(uint32_t);

    // Read application_protocol (null-terminated string)
    const char* str_start = reinterpret_cast<const char*>(buffer.data() + offset);
    packet.application_protocol = std::string(str_start);

    return packet;
}

size_t SessionRequest::get_size() const {
    return sizeof(uint16_t) // SoeOpCode
        + sizeof(uint32_t)  // soe_protocol_version
        + sizeof(uint32_t)  // session_id
        + sizeof(uint32_t)  // udp_length
        + application_protocol.length() + 1; // +1 for null termination
}

void SessionRequest::serialize(std::span<uint8_t> buffer) const {
    size_t offset = 0;

    // Write SoeOpCode::SessionRequest (big-endian uint16_t)
    uint16_t opcode_be = std::byteswap(static_cast<uint16_t>(sanctuary::soe_protocol::SoeOpCode::SessionRequest));
    std::memcpy(buffer.data() + offset, &opcode_be, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // Write soe_protocol_version (big-endian uint32_t)
    uint32_t soe_protocol_version_be = std::byteswap(soe_protocol_version);
    std::memcpy(buffer.data() + offset, &soe_protocol_version_be, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Write session_id (big-endian uint32_t)
    uint32_t session_id_be = std::byteswap(session_id);
    std::memcpy(buffer.data() + offset, &session_id_be, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Write udp_length (big-endian uint32_t)
    uint32_t udp_length_be = std::byteswap(udp_length);
    std::memcpy(buffer.data() + offset, &udp_length_be, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Write application_protocol (null-terminated string)
    std::memcpy(buffer.data() + offset, application_protocol.c_str(), application_protocol.length() + 1);
}

} // namespace sanctuary::soe_protocol::objects::packets
