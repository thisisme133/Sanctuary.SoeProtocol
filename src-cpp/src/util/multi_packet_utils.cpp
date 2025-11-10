#include "sanctuary/soe_protocol/util/multi_packet_utils.hpp"

namespace sanctuary::soe_protocol::util {

namespace {
    // Helper function to read uint16 in big endian
    [[nodiscard]] uint16_t read_uint16_be(std::span<const uint8_t> buffer) {
        return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
    }

    // Helper function to read uint32 in big endian
    [[nodiscard]] uint32_t read_uint32_be(std::span<const uint8_t> buffer) {
        return (static_cast<uint32_t>(buffer[0]) << 24) |
               (static_cast<uint32_t>(buffer[1]) << 16) |
               (static_cast<uint32_t>(buffer[2]) << 8) |
               buffer[3];
    }

    // Helper function to write uint16 in big endian
    void write_uint16_be(std::span<uint8_t> buffer, uint16_t value) {
        buffer[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
        buffer[1] = static_cast<uint8_t>(value & 0xFF);
    }

    // Helper function to write uint32 in big endian
    void write_uint32_be(std::span<uint8_t> buffer, uint32_t value) {
        buffer[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
        buffer[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        buffer[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        buffer[3] = static_cast<uint8_t>(value & 0xFF);
    }
}

uint32_t MultiPacketUtils::read_variable_length(std::span<const uint8_t> data, int& offset) {
    uint32_t value;

    if (data[offset] < UINT8_MAX) {
        value = data[offset++];
    } else if (data[offset] == UINT8_MAX && data[offset + 1] == 0) {
        // We only offset by one, as the implied 0x00 in front of all core OP codes given big endian, allows us to
        // use that as an indicator for a length value of 0xFF. This works because the byte immediately following
        // the length is going to be the start of the nested SOE packet!
        // Note that this assumes the protocol will never have more than 256 OP codes.
        value = data[offset++];
    } else if (data[offset + 1] == UINT8_MAX && data[offset + 2] == UINT8_MAX) {
        offset += 3;
        value = read_uint32_be(data.subspan(offset));
        offset += sizeof(uint32_t);
    } else {
        offset += 1;
        value = read_uint16_be(data.subspan(offset));
        offset += sizeof(uint16_t);
    }

    return value;
}

void MultiPacketUtils::write_variable_length(std::span<uint8_t> buffer, uint32_t length, int& offset) {
    if (length <= UINT8_MAX) { // See read_variable_length for a description of why <= is valid here
        buffer[offset++] = static_cast<uint8_t>(length);
    } else if (length < UINT16_MAX) {
        buffer[offset++] = UINT8_MAX;
        write_uint16_be(buffer.subspan(offset), static_cast<uint16_t>(length));
        offset += sizeof(uint16_t);
    } else {
        buffer[offset++] = UINT8_MAX;
        buffer[offset++] = UINT8_MAX;
        buffer[offset++] = UINT8_MAX;
        write_uint32_be(buffer.subspan(offset), length);
        offset += sizeof(uint32_t);
    }
}

} // namespace sanctuary::soe_protocol::util
