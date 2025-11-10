#include "sanctuary/soe_protocol/objects/rc4_key_state.hpp"
#include "sanctuary/soe_protocol/services/rc4_cipher.hpp"

namespace sanctuary::soe_protocol::objects {

Rc4KeyState::Rc4KeyState(std::span<const uint8_t> key_bytes)
    : index1(0)
    , index2(0)
    , state_() {
    services::Rc4Cipher::schedule_key(key_bytes, mutable_key_state());
}

Rc4KeyState::Rc4KeyState(const Rc4KeyState& existing_state)
    : index1(existing_state.index1)
    , index2(existing_state.index2)
    , state_() {
    std::copy(existing_state.state_.begin(), existing_state.state_.end(), state_.begin());
}

Rc4KeyState& Rc4KeyState::operator=(const Rc4KeyState& other) {
    if (this != &other) {
        index1 = other.index1;
        index2 = other.index2;
        std::copy(other.state_.begin(), other.state_.end(), state_.begin());
    }
    return *this;
}

} // namespace sanctuary::soe_protocol::objects
