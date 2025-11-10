#include "client_worker.hpp"
#include <sanctuary/soe_protocol/soe_socket_handler.hpp>
#include <sanctuary/soe_protocol/objects/socket_handler_params.hpp>
#include <sanctuary/soe_protocol/objects/session_parameters.hpp>
#include <sanctuary/soe_protocol/endpoint.hpp>
#include <spdlog/spdlog.h>
#include <asio.hpp>
#include <thread>

namespace single_session_peers {

client_worker::client_worker(
    int port,
    std::function<std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler>()> app_factory
)
    : port_(port)
    , app_factory_(std::move(app_factory))
    , worker_thread_(nullptr) {
}

client_worker::~client_worker() {
    stop();
}

auto client_worker::start() -> void {
    if (worker_thread_) {
        spdlog::warn("Client worker thread already started");
        return;
    }

    stop_requested_ = false;
    worker_thread_ = std::make_unique<std::thread>([this] { execute(); });
}

auto client_worker::stop() -> void {
    if (!worker_thread_) {
        return;
    }

    stop_requested_ = true;

    if (worker_thread_->joinable()) {
        worker_thread_->join();
    }

    worker_thread_.reset();
}

auto client_worker::wait() -> void {
    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
}

auto client_worker::execute() -> void {
    try {
        // Give the server manager some time to spool up
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Create session parameters
        auto session_params = sanctuary::soe_protocol::session_parameters{};
        session_params.application_protocol = "Ping_1";

        // Create socket handler parameters
        auto handler_params = sanctuary::soe_protocol::socket_handler_params{};
        handler_params.default_session_params = session_params;
        handler_params.app_creation_callback = app_factory_;
        handler_params.stop_on_last_session_terminated = true;

        // Create the socket handler
        auto socket_handler = sanctuary::soe_protocol::soe_socket_handler(handler_params);

        // Bind to any available port on localhost
        auto bind_endpoint = sanctuary::soe_protocol::endpoint{
            asio::ip::address_v4::loopback(),
            0 // Any available port
        };
        socket_handler.bind(bind_endpoint);

        // Connect to the server
        auto connect_endpoint = sanctuary::soe_protocol::endpoint{
            asio::ip::address_v4::loopback(),
            static_cast<uint16_t>(port_)
        };
        socket_handler.connect(connect_endpoint);

        // Run the socket handler
        while (!stop_requested_ && !socket_handler.should_stop()) {
            socket_handler.tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

    } catch (const std::exception& e) {
        spdlog::error("Error in client worker: {}", e.what());
        throw;
    }
}

} // namespace single_session_peers
