#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::services {

/// <summary>
/// Contains functions to calculate CRC-32 hashes.
/// </summary>
class Crc32 {
public:
    /// <summary>
    /// Initializes a new instance of the Crc32 class.
    /// </summary>
    /// <param name="seed">The seed used to calculate CRC hashes.</param>
    explicit constexpr Crc32(uint32_t seed) noexcept
        : seed_(schedule_seed(seed)) {}

    /// <summary>
    /// Calculates a CRC-32 hash of the given data.
    /// </summary>
    /// <param name="data">The data to hash.</param>
    /// <returns>The CRC-32 hash.</returns>
    /// <remarks>
    /// This uses an adapted version of the "Slicing-by-16" CRC-32 algorithm - (c) Stephan Brumme and Bulat Ziganshin.
    /// See https://create.stephan-brumme.com/crc32/ for further details.
    /// </remarks>
    [[nodiscard]] uint32_t hash(std::span<const uint8_t> data) const noexcept;

private:
    uint32_t seed_;

    /// <summary>
    /// Schedules the seed value for CRC calculation.
    /// </summary>
    /// <param name="seed">The seed to schedule.</param>
    /// <returns>The scheduled seed.</returns>
    [[nodiscard]] static constexpr uint32_t schedule_seed(uint32_t seed) noexcept;

    /// <summary>
    /// Calculates CRC-32 for little-endian systems.
    /// </summary>
    /// <param name="data_ptr">Pointer to the data.</param>
    /// <param name="count">Number of bytes to process.</param>
    /// <param name="seed">The scheduled seed.</param>
    /// <returns>The CRC-32 hash.</returns>
    [[nodiscard]] static uint32_t crc32_little_endian(
        const uint8_t* data_ptr,
        size_t count,
        uint32_t seed
    ) noexcept;

    /// <summary>
    /// Calculates CRC-32 for big-endian systems.
    /// </summary>
    /// <param name="data_ptr">Pointer to the data.</param>
    /// <param name="count">Number of bytes to process.</param>
    /// <param name="seed">The scheduled seed.</param>
    /// <returns>The CRC-32 hash.</returns>
    [[nodiscard]] static uint32_t crc32_big_endian(
        const uint8_t* data_ptr,
        size_t count,
        uint32_t seed
    ) noexcept;

    /// <summary>
    /// Swaps the endianness of a uint32_t value.
    /// </summary>
    /// <param name="x">The value.</param>
    /// <returns>The value with its endianness reversed.</returns>
    [[nodiscard]] static constexpr uint32_t reverse_endianness(uint32_t x) noexcept {
        return (x >> 24) |
               ((x >> 8) & 0x0000FF00) |
               ((x << 8) & 0x00FF0000) |
               (x << 24);
    }

    // CRC-32 lookup table (16 slices of 256 entries each = 4096 entries)
    static constexpr std::array<uint32_t, 4096> crc32_lookup = {
        #include "crc32_lookup_table.inc"
    };
};

} // namespace sanctuary::soe_protocol::services
