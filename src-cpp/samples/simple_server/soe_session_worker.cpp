#include "soe_session_worker.hpp"
#include <sanctuary/soe_protocol/soe_socket_handler.hpp>
#include <sanctuary/soe_protocol/objects/socket_handler_params.hpp>
#include <sanctuary/soe_protocol/objects/session_parameters.hpp>
#include <sanctuary/soe_protocol/endpoint.hpp>
#include <spdlog/spdlog.h>
#include <asio.hpp>
#include <stdexcept>

namespace simple_server {

soe_session_worker::soe_session_worker(
    int port,
    std::string app_protocol,
    std::function<std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler>()> app_factory
)
    : port_(port)
    , app_protocol_(std::move(app_protocol))
    , app_factory_(std::move(app_factory))
    , worker_thread_(nullptr) {
}

soe_session_worker::~soe_session_worker() {
    stop();
}

auto soe_session_worker::start() -> void {
    if (worker_thread_) {
        spdlog::warn("Worker thread already started");
        return;
    }

    stop_requested_ = false;
    worker_thread_ = std::make_unique<std::thread>([this] { execute(); });
}

auto soe_session_worker::stop() -> void {
    if (!worker_thread_) {
        return;
    }

    stop_requested_ = true;

    if (worker_thread_->joinable()) {
        worker_thread_->join();
    }

    worker_thread_.reset();
}

auto soe_session_worker::wait() -> void {
    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
}

auto soe_session_worker::execute() -> void {
    try {
        spdlog::info("Starting server on port {}", port_);

        // Create session parameters
        auto session_params = sanctuary::soe_protocol::session_parameters{};
        session_params.application_protocol = app_protocol_;
        session_params.is_compression_enabled = true;

        // Create socket handler parameters
        auto handler_params = sanctuary::soe_protocol::socket_handler_params{};
        handler_params.default_session_params = session_params;
        handler_params.app_creation_callback = app_factory_;

        // Create and configure the socket handler
        auto socket_handler = sanctuary::soe_protocol::soe_socket_handler(handler_params);

        // Bind to localhost on the specified port
        auto endpoint = sanctuary::soe_protocol::endpoint{
            asio::ip::address_v4::loopback(),
            static_cast<uint16_t>(port_)
        };
        socket_handler.bind(endpoint);

        // Run the socket handler until stop is requested
        while (!stop_requested_) {
            socket_handler.tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

    } catch (const std::exception& e) {
        spdlog::error("Error in session worker: {}", e.what());
        throw;
    }
}

} // namespace simple_server
