#include "sanctuary/soe_protocol/objects/packets/acknowledge_all.hpp"
#include <bit>
#include <cstring>

namespace sanctuary::soe_protocol::objects::packets {

AcknowledgeAll AcknowledgeAll::deserialize(std::span<const uint8_t> buffer) {
    AcknowledgeAll packet{};

    // Read sequence (big-endian uint16_t)
    uint16_t sequence_be;
    std::memcpy(&sequence_be, buffer.data(), sizeof(uint16_t));
    packet.sequence = std::byteswap(sequence_be);

    return packet;
}

void AcknowledgeAll::serialize(std::span<uint8_t> buffer) const {
    // Write sequence (big-endian uint16_t)
    uint16_t sequence_be = std::byteswap(sequence);
    std::memcpy(buffer.data(), &sequence_be, sizeof(uint16_t));
}

} // namespace sanctuary::soe_protocol::objects::packets
