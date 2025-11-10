#pragma once

#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::util {

/// <summary>
/// Contains utility methods for working with MultiPacket data.
/// </summary>
class MultiPacketUtils {
public:
    /// <summary>
    /// Reads a MultiData variable-length integer.
    /// </summary>
    /// <param name="data">The buffer to read the value from.</param>
    /// <param name="offset">
    /// The offset into the buffer at which the variable length value begins.
    /// Will be incremented by the amount of bytes consumed by the value.
    /// </param>
    /// <returns>The value.</returns>
    [[nodiscard]] static uint32_t read_variable_length(
        std::span<const uint8_t> data,
        int& offset);

    /// <summary>
    /// Gets the amount of space in a buffer that a variable-length integer
    /// will consume.
    /// </summary>
    /// <param name="length">The length value.</param>
    /// <returns>The required buffer size.</returns>
    [[nodiscard]] static constexpr int get_variable_length_size(int length) noexcept;

    /// <summary>
    /// Writes a MultiPacket variable-length integer to a buffer.
    /// </summary>
    /// <param name="buffer">The buffer to write to.</param>
    /// <param name="length">The length value to write.</param>
    /// <param name="offset">
    /// The offset into the buffer at which to write the value.
    /// The offset will be incremented by the amount of bytes consumed by the value.
    /// </param>
    static void write_variable_length(
        std::span<uint8_t> buffer,
        uint32_t length,
        int& offset);
};

// Inline implementation

constexpr int MultiPacketUtils::get_variable_length_size(int length) noexcept {
    if (length <= UINT8_MAX) {
        return sizeof(uint8_t);
    } else if (length < UINT16_MAX) {
        return sizeof(uint16_t) + 1;
    } else {
        return sizeof(uint32_t) + 3;
    }
}

} // namespace sanctuary::soe_protocol::util
