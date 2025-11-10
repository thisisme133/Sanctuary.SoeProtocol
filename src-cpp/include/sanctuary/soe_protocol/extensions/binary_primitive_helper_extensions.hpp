#pragma once

#include <bit>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace sanctuary::soe_protocol::extensions {

/// <summary>
/// Contains extension functions for binary primitive readers and writers.
/// These functions work with buffers that have an offset pointer being advanced.
/// </summary>
class BinaryPrimitiveHelperExtensions {
public:
    /// <summary>
    /// Reads an unsigned 24-bit integer in big endian format from a buffer,
    /// advancing the offset by 3 bytes.
    /// </summary>
    /// <param name="buffer">The buffer to read from.</param>
    /// <param name="offset">The offset into the buffer, will be incremented by 3.</param>
    /// <returns>A uint32_t value.</returns>
    [[nodiscard]] static constexpr uint32_t read_uint24_be(
        std::span<const uint8_t> buffer,
        int& offset) {
        if (buffer.size() < static_cast<size_t>(offset) + 3) {
            throw std::out_of_range("Buffer does not contain enough bytes at the given offset");
        }

        uint32_t value = 0;
        value |= static_cast<uint32_t>(buffer[offset++]) << 16;
        value |= static_cast<uint32_t>(buffer[offset++]) << 8;
        value |= static_cast<uint32_t>(buffer[offset++]);
        return value;
    }

    /// <summary>
    /// Writes an unsigned 24-bit integer value in big endian form to a buffer,
    /// advancing the offset by 3 bytes.
    /// </summary>
    /// <param name="buffer">The buffer to write to.</param>
    /// <param name="offset">The offset into the buffer, will be incremented by 3.</param>
    /// <param name="value">The value to write.</param>
    static constexpr void write_uint24_be(
        std::span<uint8_t> buffer,
        int& offset,
        uint32_t value) {
        if (buffer.size() < static_cast<size_t>(offset) + 3) {
            throw std::out_of_range("Buffer does not contain enough space at the given offset");
        }

        buffer[offset++] = static_cast<uint8_t>(value >> 16);
        buffer[offset++] = static_cast<uint8_t>(value >> 8);
        buffer[offset++] = static_cast<uint8_t>(value);
    }

    /// <summary>
    /// Reads an unsigned 24-bit integer in little endian format from a buffer,
    /// advancing the offset by 3 bytes.
    /// </summary>
    /// <param name="buffer">The buffer to read from.</param>
    /// <param name="offset">The offset into the buffer, will be incremented by 3.</param>
    /// <returns>A uint32_t value.</returns>
    [[nodiscard]] static constexpr uint32_t read_uint24_le(
        std::span<const uint8_t> buffer,
        int& offset) {
        if (buffer.size() < static_cast<size_t>(offset) + 3) {
            throw std::out_of_range("Buffer does not contain enough bytes at the given offset");
        }

        uint32_t value = 0;
        value |= static_cast<uint32_t>(buffer[offset++]);
        value |= static_cast<uint32_t>(buffer[offset++]) << 8;
        value |= static_cast<uint32_t>(buffer[offset++]) << 16;
        return value;
    }

    /// <summary>
    /// Writes an unsigned 24-bit integer value in little endian form to a buffer,
    /// advancing the offset by 3 bytes.
    /// </summary>
    /// <param name="buffer">The buffer to write to.</param>
    /// <param name="offset">The offset into the buffer, will be incremented by 3.</param>
    /// <param name="value">The value to write.</param>
    static constexpr void write_uint24_le(
        std::span<uint8_t> buffer,
        int& offset,
        uint32_t value) {
        if (buffer.size() < static_cast<size_t>(offset) + 3) {
            throw std::out_of_range("Buffer does not contain enough space at the given offset");
        }

        buffer[offset++] = static_cast<uint8_t>(value);
        buffer[offset++] = static_cast<uint8_t>(value >> 8);
        buffer[offset++] = static_cast<uint8_t>(value >> 16);
    }

    /// <summary>
    /// Reads a single byte from a buffer, advancing the offset by 1.
    /// </summary>
    /// <param name="buffer">The buffer to read from.</param>
    /// <param name="offset">The offset into the buffer, will be incremented by 1.</param>
    /// <returns>The byte value.</returns>
    [[nodiscard]] static constexpr uint8_t read_byte(
        std::span<const uint8_t> buffer,
        int& offset) {
        if (buffer.size() < static_cast<size_t>(offset) + 1) {
            throw std::out_of_range("Buffer does not contain enough bytes at the given offset");
        }

        return buffer[offset++];
    }

    /// <summary>
    /// Writes a single byte to a buffer, advancing the offset by 1.
    /// </summary>
    /// <param name="buffer">The buffer to write to.</param>
    /// <param name="offset">The offset into the buffer, will be incremented by 1.</param>
    /// <param name="value">The byte value to write.</param>
    static constexpr void write_byte(
        std::span<uint8_t> buffer,
        int& offset,
        uint8_t value) {
        if (buffer.size() < static_cast<size_t>(offset) + 1) {
            throw std::out_of_range("Buffer does not contain enough space at the given offset");
        }

        buffer[offset++] = value;
    }
};

// Free function aliases for convenience
[[nodiscard]] inline constexpr uint32_t read_uint24_be(
    std::span<const uint8_t> buffer,
    int& offset) {
    return BinaryPrimitiveHelperExtensions::read_uint24_be(buffer, offset);
}

inline constexpr void write_uint24_be(
    std::span<uint8_t> buffer,
    int& offset,
    uint32_t value) {
    BinaryPrimitiveHelperExtensions::write_uint24_be(buffer, offset, value);
}

[[nodiscard]] inline constexpr uint32_t read_uint24_le(
    std::span<const uint8_t> buffer,
    int& offset) {
    return BinaryPrimitiveHelperExtensions::read_uint24_le(buffer, offset);
}

inline constexpr void write_uint24_le(
    std::span<uint8_t> buffer,
    int& offset,
    uint32_t value) {
    BinaryPrimitiveHelperExtensions::write_uint24_le(buffer, offset, value);
}

[[nodiscard]] inline constexpr uint8_t read_byte(
    std::span<const uint8_t> buffer,
    int& offset) {
    return BinaryPrimitiveHelperExtensions::read_byte(buffer, offset);
}

inline constexpr void write_byte(
    std::span<uint8_t> buffer,
    int& offset,
    uint8_t value) {
    BinaryPrimitiveHelperExtensions::write_byte(buffer, offset, value);
}

} // namespace sanctuary::soe_protocol::extensions
