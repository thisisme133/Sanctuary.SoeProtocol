#include "ping_application.hpp"
#include <sanctuary/soe_protocol/objects/session_mode.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <algorithm>

namespace single_session_peers {

ping_application::ping_application()
    : session_handler_(nullptr)
    , session_params_(nullptr) {
}

auto ping_application::session_params() const -> const sanctuary::soe_protocol::application_parameters& {
    return session_params_;
}

auto ping_application::initialise(std::shared_ptr<sanctuary::soe_protocol::session_handler> session_handler) -> void {
    session_handler_ = std::move(session_handler);
}

auto ping_application::on_session_opened() -> void {
    session_start_ = std::chrono::steady_clock::now();
    spdlog::info(
        "{} Session opened. Running ping throughput test for {}s...",
        get_mode_prefix(),
        ping_pong_duration_.count()
    );

    // Client sends the first ping
    if (session_handler_ && session_handler_->mode() == sanctuary::soe_protocol::session_mode::client) {
        std::string_view ping_msg = "Ping!";
        std::vector<std::byte> ping_data(ping_msg.size());
        std::transform(ping_msg.begin(), ping_msg.end(), ping_data.begin(),
            [](char c) { return static_cast<std::byte>(c); });
        session_handler_->enqueue_data(ping_data);
    }
}

auto ping_application::handle_app_data(std::span<const std::byte> data) -> void {
    receive_count_++;

    // Convert span to string for comparison
    std::string message;
    message.reserve(data.size());
    for (const auto& byte : data) {
        message.push_back(static_cast<char>(byte));
    }

    // Respond with opposite message
    std::string_view response = (message == "Ping!") ? "Pong!" : "Ping!";
    std::vector<std::byte> response_data(response.size());
    std::transform(response.begin(), response.end(), response_data.begin(),
        [](char c) { return static_cast<std::byte>(c); });

    if (session_handler_) {
        session_handler_->enqueue_data(response_data);

        // Client terminates the session after the duration
        if (session_handler_->mode() == sanctuary::soe_protocol::session_mode::client) {
            auto elapsed = std::chrono::steady_clock::now() - session_start_;
            if (elapsed >= ping_pong_duration_) {
                session_handler_->terminate_session();
            }
        }
    }
}

auto ping_application::on_session_closed(sanctuary::soe_protocol::disconnect_reason disconnect_reason) -> void {
    auto elapsed = std::chrono::steady_clock::now() - session_start_;
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

    if (elapsed_seconds == 0) {
        elapsed_seconds = 1; // Avoid division by zero
    }

    spdlog::info(
        "{} Session closed. Throughput: {}/s",
        get_mode_prefix(),
        receive_count_ / elapsed_seconds
    );
}

auto ping_application::get_mode_prefix() const -> std::string {
    if (!session_handler_) {
        return "<Unknown>";
    }

    switch (session_handler_->mode()) {
        case sanctuary::soe_protocol::session_mode::client:
            return "<Client>";
        case sanctuary::soe_protocol::session_mode::server:
            return "<Server>";
        default:
            return "<Unknown>";
    }
}

} // namespace single_session_peers
