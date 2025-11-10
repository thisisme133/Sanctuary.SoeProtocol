#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <functional>

namespace sanctuary::soe_protocol {
class application_protocol_handler;
} // namespace sanctuary::soe_protocol

namespace single_session_peers {

/// <summary>
/// A background worker for the server session manager.
/// Listens for incoming connections and manages the server-side session.
/// </summary>
class server_worker {
public:
    /// <summary>
    /// Constructs a new server worker.
    /// </summary>
    /// <param name="port">The port to listen on.</param>
    /// <param name="app_factory">Factory function to create application protocol handlers.</param>
    explicit server_worker(
        int port,
        std::function<std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler>()> app_factory
    );

    /// <summary>
    /// Destructor - ensures the worker is stopped.
    /// </summary>
    ~server_worker();

    // Delete copy and move constructors/assignments
    server_worker(const server_worker&) = delete;
    server_worker& operator=(const server_worker&) = delete;
    server_worker(server_worker&&) = delete;
    server_worker& operator=(server_worker&&) = delete;

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
    std::function<std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler>()> app_factory_;
    std::unique_ptr<std::thread> worker_thread_;
    std::atomic<bool> stop_requested_{false};

    /// <summary>
    /// The main worker thread execution function.
    /// </summary>
    auto execute() -> void;
};

} // namespace single_session_peers
