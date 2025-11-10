#pragma once

#include <bit>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace sanctuary::soe_protocol::extensions {

/// <summary>
/// Contains utility functions for reading and writing binary primitives,
/// extending the standard library functionality.
/// </summary>
class BinaryPrimitivesExtensions {
public:
    /// <summary>
    /// Reads an unsigned 24-bit integer in big endian format.
    /// </summary>
    /// <param name="source">The buffer to read the value from.</param>
    /// <returns>A uint32_t value.</returns>
    [[nodiscard]] static constexpr uint32_t read_uint24_be(std::span<const uint8_t> source) {
        if (source.size() < 3) {
            throw std::out_of_range("Source buffer must contain at least 3 bytes");
        }

        uint32_t value = 0;
        value |= static_cast<uint32_t>(source[0]) << 16;
        value |= static_cast<uint32_t>(source[1]) << 8;
        value |= static_cast<uint32_t>(source[2]);
        return value;
    }

    /// <summary>
    /// Writes an unsigned 24-bit integer value in big endian form.
    /// </summary>
    /// <param name="target">The buffer to write the value to.</param>
    /// <param name="value">The value.</param>
    static constexpr void write_uint24_be(std::span<uint8_t> target, uint32_t value) {
        if (target.size() < 3) {
            throw std::out_of_range("Target buffer must contain at least 3 bytes");
        }

        target[0] = static_cast<uint8_t>(value >> 16);
        target[1] = static_cast<uint8_t>(value >> 8);
        target[2] = static_cast<uint8_t>(value);
    }

    /// <summary>
    /// Reads an unsigned 24-bit integer in little endian format.
    /// </summary>
    /// <param name="source">The buffer to read the value from.</param>
    /// <returns>A uint32_t value.</returns>
    [[nodiscard]] static constexpr uint32_t read_uint24_le(std::span<const uint8_t> source) {
        if (source.size() < 3) {
            throw std::out_of_range("Source buffer must contain at least 3 bytes");
        }

        uint32_t value = 0;
        value |= static_cast<uint32_t>(source[0]);
        value |= static_cast<uint32_t>(source[1]) << 8;
        value |= static_cast<uint32_t>(source[2]) << 16;
        return value;
    }

    /// <summary>
    /// Writes an unsigned 24-bit integer value in little endian form.
    /// </summary>
    /// <param name="target">The buffer to write the value to.</param>
    /// <param name="value">The value.</param>
    static constexpr void write_uint24_le(std::span<uint8_t> target, uint32_t value) {
        if (target.size() < 3) {
            throw std::out_of_range("Target buffer must contain at least 3 bytes");
        }

        target[0] = static_cast<uint8_t>(value);
        target[1] = static_cast<uint8_t>(value >> 8);
        target[2] = static_cast<uint8_t>(value >> 16);
    }
};

// Free function aliases for convenience
[[nodiscard]] inline constexpr uint32_t read_uint24_be(std::span<const uint8_t> source) {
    return BinaryPrimitivesExtensions::read_uint24_be(source);
}

inline constexpr void write_uint24_be(std::span<uint8_t> target, uint32_t value) {
    BinaryPrimitivesExtensions::write_uint24_be(target, value);
}

[[nodiscard]] inline constexpr uint32_t read_uint24_le(std::span<const uint8_t> source) {
    return BinaryPrimitivesExtensions::read_uint24_le(source);
}

inline constexpr void write_uint24_le(std::span<uint8_t> target, uint32_t value) {
    BinaryPrimitivesExtensions::write_uint24_le(target, value);
}

} // namespace sanctuary::soe_protocol::extensions
