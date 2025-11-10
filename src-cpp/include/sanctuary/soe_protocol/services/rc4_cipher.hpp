#pragma once

#include "sanctuary/soe_protocol/objects/rc4_key_state.hpp"
#include <cstdint>
#include <span>
#include <stdexcept>

namespace sanctuary::soe_protocol::services {

/// <summary>
/// Provides a means for transforming data using the RC4 algorithm.
/// </summary>
class Rc4Cipher {
public:
    /// <summary>
    /// Transforms a buffer using an existing key state.
    /// </summary>
    /// <param name="input_buffer">The buffer to encrypt.</param>
    /// <param name="output_buffer">The output buffer. Must be at least as long as the input_buffer.</param>
    /// <param name="key_state">The key state to use.</param>
    /// <exception cref="std::invalid_argument">
    /// Thrown if the output buffer is shorter than the input buffer.
    /// </exception>
    static void transform(
        std::span<const uint8_t> input_buffer,
        std::span<uint8_t> output_buffer,
        objects::Rc4KeyState& key_state
    );

    /// <summary>
    /// Advances the given key state.
    /// </summary>
    /// <param name="amount">The amount to advance the key state by.</param>
    /// <param name="key_state">The key state to advance.</param>
    static void advance(int32_t amount, objects::Rc4KeyState& key_state);

    /// <summary>
    /// Schedules the given key into the given state buffer.
    /// </summary>
    /// <param name="key_data_buffer">A buffer containing the key data.</param>
    /// <param name="key_state">A buffer to place the scheduled key data into.</param>
    /// <exception cref="std::out_of_range">
    /// Thrown if the length of the key_data_buffer is
    /// less than one, or greater than Rc4KeyState::LENGTH.
    /// </exception>
    /// <exception cref="std::invalid_argument">
    /// Thrown if the key_state buffer is shorter than Rc4KeyState::LENGTH.
    /// </exception>
    static void schedule_key(
        std::span<const uint8_t> key_data_buffer,
        std::span<uint8_t> key_state
    );

private:
    /// <summary>
    /// Increments the key state indices and performs the swap operation.
    /// </summary>
    /// <param name="key_state">The key state to increment.</param>
    [[gnu::always_inline]]
    static inline void increment_key_state(objects::Rc4KeyState& key_state);
};

} // namespace sanctuary::soe_protocol::services
