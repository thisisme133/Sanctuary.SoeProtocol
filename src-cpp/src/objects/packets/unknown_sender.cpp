#include "sanctuary/soe_protocol/objects/packets/unknown_sender.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include <bit>
#include <cstring>

namespace sanctuary::soe_protocol::objects::packets {

void UnknownSender::serialize(std::span<uint8_t> buffer) {
    // Write SoeOpCode::UnknownSender (big-endian uint16_t)
    uint16_t opcode_be = std::byteswap(static_cast<uint16_t>(sanctuary::soe_protocol::SoeOpCode::UnknownSender));
    std::memcpy(buffer.data(), &opcode_be, sizeof(uint16_t));
}

} // namespace sanctuary::soe_protocol::objects::packets
