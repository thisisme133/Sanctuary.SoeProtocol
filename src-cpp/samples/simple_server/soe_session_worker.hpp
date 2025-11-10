#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <functional>

namespace sanctuary::soe_protocol {
class soe_socket_handler;
class application_protocol_handler;
} // namespace sanctuary::soe_protocol

namespace simple_server {

/// <summary>
/// Background worker that manages the SOE session server.
/// Runs the socket handler in a separate thread and manages its lifecycle.
/// </summary>
class soe_session_worker {
public:
    /// <summary>
    /// Constructs a new SOE session worker.
    /// </summary>
    /// <param name="port">The port to listen on.</param>
    /// <param name="app_protocol">The application protocol name.</param>
    /// <param name="app_factory">Factory function to create application protocol handlers.</param>
    explicit soe_session_worker(
        int port,
        std::string app_protocol,
        std::function<std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler>()> app_factory
    );

    /// <summary>
    /// Destructor - ensures the worker is stopped.
    /// </summary>
    ~soe_session_worker();

    // Delete copy and move constructors/assignments
    soe_session_worker(const soe_session_worker&) = delete;
    soe_session_worker& operator=(const soe_session_worker&) = delete;
    soe_session_worker(soe_session_worker&&) = delete;
    soe_session_worker& operator=(soe_session_worker&&) = delete;

    /// <summary>
    /// Starts the worker thread.
    /// </summary>
    auto start() -> void;

    /// <summary>
    /// Stops the worker thread and waits for it to complete.
    /// </summary>
    auto stop() -> void;

    /// <summary>
    /// Waits for the worker thread to complete.
    /// </summary>
    auto wait() -> void;

private:
    int port_;
    std::string app_protocol_;
    std::function<std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler>()> app_factory_;
    std::unique_ptr<std::thread> worker_thread_;
    std::atomic<bool> stop_requested_{false};

    /// <summary>
    /// The main worker thread execution function.
    /// </summary>
    auto execute() -> void;
};

} // namespace simple_server
