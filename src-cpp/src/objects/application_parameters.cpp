#include "sanctuary/soe_protocol/objects/application_parameters.hpp"

namespace sanctuary::soe_protocol::objects {

ApplicationParameters::ApplicationParameters(std::shared_ptr<Rc4KeyState> encryption_key_state)
    : is_encryption_enabled_(false)
    , encryption_key_state_(std::move(encryption_key_state)) {
}

void ApplicationParameters::set_encryption_enabled(bool value) {
    if (value && !encryption_key_state_) {
        throw std::invalid_argument(
            "Cannot enable encryption when the key state is null");
    }
    is_encryption_enabled_ = value;
}

} // namespace sanctuary::soe_protocol::objects
