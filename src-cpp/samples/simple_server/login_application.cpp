#include "login_application.hpp"
#include <sanctuary/soe_protocol/objects/rc4_key_state.hpp>
#include <array>
#include <cstddef>

namespace simple_server {

login_application::login_application()
    : session_handler_(nullptr) {
    // Base64 decode "MY_KEY" - this is just a placeholder key
    // In C#: Convert.FromBase64String("MY_KEY")
    // For this example, we'll use a simple key (in production, use proper base64 decoding)
    std::array<std::byte, 6> key_bytes = {
        std::byte{0x30}, std::byte{0x0C}, std::byte{0x28},
        std::byte{0xB2}, std::byte{0x4A}
    };

    auto key_state = sanctuary::soe_protocol::rc4_key_state(
        std::span<const std::byte>(key_bytes.data(), key_bytes.size())
    );

    session_params_ = sanctuary::soe_protocol::application_parameters(std::move(key_state));
    session_params_.is_encryption_enabled = true;
}

auto login_application::session_params() const -> const sanctuary::soe_protocol::application_parameters& {
    return session_params_;
}

auto login_application::initialise(std::shared_ptr<sanctuary::soe_protocol::session_handler> session_handler) -> void {
    session_handler_ = std::move(session_handler);
}

auto login_application::on_session_opened() -> void {
    if (session_handler_) {
        spdlog::debug("Login session opened - ID {}", session_handler_->session_id());
    }
}

auto login_application::handle_app_data(std::span<const std::byte> data) -> void {
    if (data.empty()) {
        return;
    }

    auto login_op_code = static_cast<uint8_t>(data[0]);
    spdlog::info("Received application packet with OP code {}", login_op_code);

    // Uncomment to terminate session on receiving data
    // terminate_login_session();
}

auto login_application::on_session_closed(sanctuary::soe_protocol::disconnect_reason disconnect_reason) -> void {
    spdlog::debug(
        "Login session closed with reason {} - ID {}",
        static_cast<uint16_t>(disconnect_reason),
        session_handler_ ? session_handler_->session_id() : 0
    );
}

auto login_application::terminate_login_session() -> void {
    if (session_handler_) {
        session_handler_->terminate_session();
    }
}

} // namespace simple_server
