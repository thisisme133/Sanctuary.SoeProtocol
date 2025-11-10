#include "sanctuary/soe_protocol/services/crc32.hpp"
#include <cstring>

namespace sanctuary::soe_protocol::services {

uint32_t Crc32::hash(std::span<const uint8_t> data) const noexcept {
    if constexpr (std::endian::native == std::endian::little) {
        return crc32_little_endian(data.data(), data.size(), seed_);
    } else {
        return crc32_big_endian(data.data(), data.size(), seed_);
    }
}

constexpr uint32_t Crc32::schedule_seed(uint32_t seed) noexcept {
    uint32_t state = crc32_lookup[~seed & 0xFF];
    state ^= 0x00FFFFFF;
    uint32_t index = (seed >> 8) ^ state;
    state = (state >> 8) & 0x00FFFFFF;
    state ^= crc32_lookup[index & 0xFF];
    index = (seed >> 16) ^ state;
    state = (state >> 8) & 0x00FFFFFF;
    state ^= crc32_lookup[index & 0xFF];
    index = (seed >> 24) ^ state;
    state = (state >> 8) & 0x00FFFFFF;
    state ^= crc32_lookup[index & 0xFF];

    return state;
}

uint32_t Crc32::crc32_little_endian(
    const uint8_t* data_ptr,
    size_t count,
    uint32_t seed
) noexcept {
    constexpr size_t unroll = 4;
    constexpr size_t bytes_at_once = 16 * unroll;

    uint32_t crc = seed;

    // Get pointers to the lookup table slices
    const uint32_t* lookup_0 = crc32_lookup.data();
    const uint32_t* lookup_1 = lookup_0 + 0x100;
    const uint32_t* lookup_2 = lookup_0 + 0x200;
    const uint32_t* lookup_3 = lookup_0 + 0x300;
    const uint32_t* lookup_4 = lookup_0 + 0x400;
    const uint32_t* lookup_5 = lookup_0 + 0x500;
    const uint32_t* lookup_6 = lookup_0 + 0x600;
    const uint32_t* lookup_7 = lookup_0 + 0x700;
    const uint32_t* lookup_8 = lookup_0 + 0x800;
    const uint32_t* lookup_9 = lookup_0 + 0x900;
    const uint32_t* lookup_10 = lookup_0 + 0xA00;
    const uint32_t* lookup_11 = lookup_0 + 0xB00;
    const uint32_t* lookup_12 = lookup_0 + 0xC00;
    const uint32_t* lookup_13 = lookup_0 + 0xD00;
    const uint32_t* lookup_14 = lookup_0 + 0xE00;
    const uint32_t* lookup_15 = lookup_0 + 0xF00;

    // Process blocks of bytes_at_once
    const uint32_t* uint_ptr = reinterpret_cast<const uint32_t*>(data_ptr);
    const uint32_t* uint_end_ptr = uint_ptr + (count / bytes_at_once) * (bytes_at_once / 4);

    while (uint_ptr < uint_end_ptr) {
        for (size_t unrolling = 0; unrolling < unroll; ++unrolling) {
            // Little endian
            uint32_t one, two, three, four;
            std::memcpy(&one, uint_ptr++, sizeof(uint32_t));
            std::memcpy(&two, uint_ptr++, sizeof(uint32_t));
            std::memcpy(&three, uint_ptr++, sizeof(uint32_t));
            std::memcpy(&four, uint_ptr++, sizeof(uint32_t));

            one ^= crc;

            crc = lookup_0[(four >> 24) & 0xFF] ^
                  lookup_1[(four >> 16) & 0xFF] ^
                  lookup_2[(four >> 8) & 0xFF] ^
                  lookup_3[four & 0xFF] ^
                  lookup_4[(three >> 24) & 0xFF] ^
                  lookup_5[(three >> 16) & 0xFF] ^
                  lookup_6[(three >> 8) & 0xFF] ^
                  lookup_7[three & 0xFF] ^
                  lookup_8[(two >> 24) & 0xFF] ^
                  lookup_9[(two >> 16) & 0xFF] ^
                  lookup_10[(two >> 8) & 0xFF] ^
                  lookup_11[two & 0xFF] ^
                  lookup_12[(one >> 24) & 0xFF] ^
                  lookup_13[(one >> 16) & 0xFF] ^
                  lookup_14[(one >> 8) & 0xFF] ^
                  lookup_15[one & 0xFF];
        }
    }

    // Process remaining 1 to 63 bytes (standard algorithm)
    for (size_t i = (count / bytes_at_once) * bytes_at_once; i < count; ++i) {
        crc = (crc >> 8) ^ lookup_0[(crc & 0xFF) ^ data_ptr[i]];
    }

    return ~crc;
}

uint32_t Crc32::crc32_big_endian(
    const uint8_t* data_ptr,
    size_t count,
    uint32_t seed
) noexcept {
    constexpr size_t unroll = 4;
    constexpr size_t bytes_at_once = 16 * unroll;

    // Schedule seed for big endian
    uint32_t crc = crc32_lookup[~seed & 0xFF];
    crc ^= 0x00FFFFFF;
    uint32_t index = (seed >> 8) ^ crc;
    crc = (crc >> 8) & 0x00FFFFFF;
    crc ^= crc32_lookup[index & 0xFF];
    index = (seed >> 16) ^ crc;
    crc = (crc >> 8) & 0x00FFFFFF;
    crc ^= crc32_lookup[index & 0xFF];
    index = (seed >> 24) ^ crc;
    crc = (crc >> 8) & 0x00FFFFFF;
    crc ^= crc32_lookup[index & 0xFF];

    // Get pointers to the lookup table slices
    const uint32_t* lookup_0 = crc32_lookup.data();
    const uint32_t* lookup_1 = lookup_0 + 0x100;
    const uint32_t* lookup_2 = lookup_0 + 0x200;
    const uint32_t* lookup_3 = lookup_0 + 0x300;
    const uint32_t* lookup_4 = lookup_0 + 0x400;
    const uint32_t* lookup_5 = lookup_0 + 0x500;
    const uint32_t* lookup_6 = lookup_0 + 0x600;
    const uint32_t* lookup_7 = lookup_0 + 0x700;
    const uint32_t* lookup_8 = lookup_0 + 0x800;
    const uint32_t* lookup_9 = lookup_0 + 0x900;
    const uint32_t* lookup_10 = lookup_0 + 0xA00;
    const uint32_t* lookup_11 = lookup_0 + 0xB00;
    const uint32_t* lookup_12 = lookup_0 + 0xC00;
    const uint32_t* lookup_13 = lookup_0 + 0xD00;
    const uint32_t* lookup_14 = lookup_0 + 0xE00;
    const uint32_t* lookup_15 = lookup_0 + 0xF00;

    // Process blocks of bytes_at_once
    const uint32_t* uint_ptr = reinterpret_cast<const uint32_t*>(data_ptr);
    const uint32_t* uint_end_ptr = uint_ptr + (count / bytes_at_once) * (bytes_at_once / 4);

    while (uint_ptr < uint_end_ptr) {
        for (size_t unrolling = 0; unrolling < unroll; ++unrolling) {
            // Big endian
            uint32_t one, two, three, four;
            std::memcpy(&one, uint_ptr++, sizeof(uint32_t));
            std::memcpy(&two, uint_ptr++, sizeof(uint32_t));
            std::memcpy(&three, uint_ptr++, sizeof(uint32_t));
            std::memcpy(&four, uint_ptr++, sizeof(uint32_t));

            one ^= reverse_endianness(crc);

            crc = lookup_0[four & 0xFF] ^
                  lookup_1[(four >> 8) & 0xFF] ^
                  lookup_2[(four >> 16) & 0xFF] ^
                  lookup_3[(four >> 24) & 0xFF] ^
                  lookup_4[three & 0xFF] ^
                  lookup_5[(three >> 8) & 0xFF] ^
                  lookup_6[(three >> 16) & 0xFF] ^
                  lookup_7[(three >> 24) & 0xFF] ^
                  lookup_8[two & 0xFF] ^
                  lookup_9[(two >> 8) & 0xFF] ^
                  lookup_10[(two >> 16) & 0xFF] ^
                  lookup_11[(two >> 24) & 0xFF] ^
                  lookup_12[one & 0xFF] ^
                  lookup_13[(one >> 8) & 0xFF] ^
                  lookup_14[(one >> 16) & 0xFF] ^
                  lookup_15[(one >> 24) & 0xFF];
        }
    }

    // Process remaining 1 to 63 bytes (standard algorithm)
    for (size_t i = (count / bytes_at_once) * bytes_at_once; i < count; ++i) {
        crc = (crc >> 8) ^ lookup_0[(crc & 0xFF) ^ data_ptr[i]];
    }

    return ~crc;
}

} // namespace sanctuary::soe_protocol::services
