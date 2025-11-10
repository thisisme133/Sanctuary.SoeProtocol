#include "sanctuary/soe_protocol/services/rc4_cipher.hpp"
#include <algorithm>
#include <format>

namespace sanctuary::soe_protocol::services {

void Rc4Cipher::transform(
    std::span<const uint8_t> input_buffer,
    std::span<uint8_t> output_buffer,
    objects::Rc4KeyState& key_state
) {
    if (output_buffer.size() < input_buffer.size()) {
        throw std::invalid_argument("The output buffer must be at least as long as the input buffer.");
    }

    auto my_key_state = key_state.mutable_key_state();

    for (size_t i = 0; i < input_buffer.size(); ++i) {
        increment_key_state(key_state);

        const auto xor_index = (my_key_state[key_state.index1] + my_key_state[key_state.index2])
                               % objects::Rc4KeyState::LENGTH;
        output_buffer[i] = input_buffer[i] ^ my_key_state[xor_index];
    }
}

void Rc4Cipher::advance(int32_t amount, objects::Rc4KeyState& key_state) {
    for (int32_t i = 0; i < amount; ++i) {
        increment_key_state(key_state);
    }
}

void Rc4Cipher::schedule_key(
    std::span<const uint8_t> key_data_buffer,
    std::span<uint8_t> key_state
) {
    if (key_data_buffer.size() < 1 || key_data_buffer.size() > objects::Rc4KeyState::LENGTH) {
        throw std::out_of_range(std::format(
            "Key length must be greater than zero and less than {}.",
            objects::Rc4KeyState::LENGTH
        ));
    }

    if (key_state.size() < objects::Rc4KeyState::LENGTH) {
        throw std::invalid_argument(std::format(
            "The key state buffer must be at least {} bytes long",
            objects::Rc4KeyState::LENGTH
        ));
    }

    // Initialize key state with sequential values
    for (size_t i = 0; i < objects::Rc4KeyState::LENGTH; ++i) {
        key_state[i] = static_cast<uint8_t>(i);
    }

    // Key scheduling algorithm
    uint8_t swap_index1 = 0;
    uint8_t swap_index2 = 0;

    for (size_t i = 0; i < objects::Rc4KeyState::LENGTH; ++i) {
        swap_index2 = static_cast<uint8_t>(
            (swap_index2 + key_state[i] + key_data_buffer[swap_index1]) % objects::Rc4KeyState::LENGTH
        );
        std::swap(key_state[i], key_state[swap_index2]);

        swap_index1 = static_cast<uint8_t>((swap_index1 + 1) % key_data_buffer.size());
    }
}

void Rc4Cipher::increment_key_state(objects::Rc4KeyState& key_state) {
    auto my_key_state = key_state.mutable_key_state();

    key_state.index1 = (key_state.index1 + 1) % objects::Rc4KeyState::LENGTH;
    key_state.index2 = (key_state.index2 + my_key_state[key_state.index1]) % objects::Rc4KeyState::LENGTH;
    std::swap(my_key_state[key_state.index1], my_key_state[key_state.index2]);
}

} // namespace sanctuary::soe_protocol::services
