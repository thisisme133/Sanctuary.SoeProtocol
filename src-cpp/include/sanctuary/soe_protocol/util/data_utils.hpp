#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace sanctuary::soe_protocol::util {

/// <summary>
/// Contains utility methods for working with reliable multi-data.
/// </summary>
class DataUtils {
public:
    /// <summary>
    /// Gets the byte sequence that indicates a reliable data packet is carrying multi-data.
    /// </summary>
    static constexpr std::array<uint8_t, 2> MULTI_DATA_INDICATOR = {0x00, 0x19};

    /// <summary>
    /// Gets the true sequence from an incoming packet sequence.
    /// </summary>
    /// <param name="packet_sequence">The packet sequence.</param>
    /// <param name="current_sequence">The last known (general, expected window) sequence.</param>
    /// <param name="max_queued_reliable_data_packets">
    /// The maximum number of reliable data packets that may be queued for dispatch/receive.
    /// </param>
    /// <returns>The true sequence number.</returns>
    [[nodiscard]] static constexpr int64_t get_true_incoming_sequence(
        uint16_t packet_sequence,
        int64_t current_sequence,
        int16_t max_queued_reliable_data_packets) noexcept;

    /// <summary>
    /// Checks whether a buffer starts with the MULTI_DATA_INDICATOR.
    /// </summary>
    /// <param name="buffer">The buffer to check.</param>
    /// <returns>True if the buffer contains multi-data.</returns>
    [[nodiscard]] static constexpr bool check_for_multi_data(
        std::span<const uint8_t> buffer) noexcept;

    /// <summary>
    /// Writes the MULTI_DATA_INDICATOR to a buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    /// <param name="offset">The offset into the buffer at which to write the multi-data indicator.</param>
    static constexpr void write_multi_data_indicator(
        std::span<uint8_t> buffer,
        int& offset) noexcept;

    /// <summary>
    /// Reads a variable length value from a buffer.
    /// </summary>
    /// <param name="buffer">The buffer to read from.</param>
    /// <param name="offset">
    /// The offset into the buffer at which to read the length value.
    /// The offset will be incremented by the amount of bytes used by the length value.
    /// </param>
    /// <returns>The length value.</returns>
    [[nodiscard]] static uint32_t read_variable_length(
        std::span<const uint8_t> buffer,
        int& offset);

    /// <summary>
    /// Gets the amount of space in a buffer that a variable-length value will consume.
    /// </summary>
    /// <param name="length">The length value.</param>
    /// <returns>The required buffer size.</returns>
    [[nodiscard]] static constexpr int get_variable_length_size(int length);

    /// <summary>
    /// Gets the amount of space in a buffer that a variable-length integer will consume.
    /// </summary>
    /// <param name="length">The length value.</param>
    /// <returns>The required buffer size.</returns>
    [[nodiscard]] static constexpr int get_variable_length_size(uint32_t length) noexcept;

    /// <summary>
    /// Writes a variable-length value to a buffer.
    /// </summary>
    /// <param name="buffer">The buffer to write to.</param>
    /// <param name="length">The length value to write.</param>
    /// <param name="offset">
    /// The offset into the buffer at which to write the value.
    /// The offset will be incremented by the amount of bytes used by the length value.
    /// </param>
    static void write_variable_length(
        std::span<uint8_t> buffer,
        uint32_t length,
        int& offset);
};

// Inline implementations

constexpr int64_t DataUtils::get_true_incoming_sequence(
    uint16_t packet_sequence,
    int64_t current_sequence,
    int16_t max_queued_reliable_data_packets) noexcept {
    // Note; this method makes the assumption that the amount of queued reliable data
    // can never be more than slightly less than the max value of a uint16_t

    // Zero-out the lower two bytes of our last known sequence and
    // insert the packet sequence in that space
    int64_t sequence = packet_sequence | (current_sequence & (INT64_MAX ^ UINT16_MAX));

    // If the sequence we obtain is larger than our possible window, we must have wrapped back
    // to the last 'packet sequence' 'block' (uint16_t), and hence need to decrement the true
    // sequence by an entire block
    if (sequence > current_sequence + max_queued_reliable_data_packets) {
        sequence -= static_cast<int64_t>(UINT16_MAX) + 1;
    }

    // If the sequence we obtain is smaller than our possible window, we must have wrapped
    // forward to the next 'packet sequence' block, and hence need to increment the true
    // sequence by an entire block
    if (sequence < current_sequence - max_queued_reliable_data_packets) {
        sequence += static_cast<int64_t>(UINT16_MAX) + 1;
    }

    return sequence;
}

constexpr bool DataUtils::check_for_multi_data(std::span<const uint8_t> buffer) noexcept {
    return buffer.size() > 2
        && buffer[0] == MULTI_DATA_INDICATOR[0]
        && buffer[1] == MULTI_DATA_INDICATOR[1];
}

constexpr void DataUtils::write_multi_data_indicator(
    std::span<uint8_t> buffer,
    int& offset) noexcept {
    buffer[offset] = MULTI_DATA_INDICATOR[0];
    buffer[offset + 1] = MULTI_DATA_INDICATOR[1];
    offset += 2;
}

constexpr int DataUtils::get_variable_length_size(int length) {
    if (length < 0) {
        throw std::invalid_argument("Length must not be negative");
    }
    return get_variable_length_size(static_cast<uint32_t>(length));
}

constexpr int DataUtils::get_variable_length_size(uint32_t length) noexcept {
    if (length < 0xFF) {
        return sizeof(uint8_t);
    } else if (length < 0xFFFF) {
        return sizeof(uint16_t) + 1;
    } else {
        return sizeof(uint32_t) + 3;
    }
}

} // namespace sanctuary::soe_protocol::util
