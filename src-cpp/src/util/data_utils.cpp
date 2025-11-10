#include "sanctuary/soe_protocol/util/data_utils.hpp"
#include <bit>
#include <stdexcept>

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

uint32_t DataUtils::read_variable_length(std::span<const uint8_t> buffer, int& offset) {
    uint32_t value;

    if (buffer[offset] < 0xFF) {
        value = buffer[offset++];
    } else if (buffer[offset + 1] == 0xFF && buffer[offset + 2] == 0xFF) {
        offset += 3;
        value = read_uint32_be(buffer.subspan(offset));
        offset += sizeof(uint32_t);
    } else {
        offset += 1;
        value = read_uint16_be(buffer.subspan(offset));
        offset += sizeof(uint16_t);
    }

    return value;
}

void DataUtils::write_variable_length(std::span<uint8_t> buffer, uint32_t length, int& offset) {
    if (length < 0xFF) { // TODO: This may need to be 0xFE (254) instead?
        buffer[offset++] = static_cast<uint8_t>(length);
    } else if (length < 0xFFFF) {
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
