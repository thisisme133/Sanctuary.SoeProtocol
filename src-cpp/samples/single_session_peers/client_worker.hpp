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
/// A background worker for the client session manager.
/// Connects to the server and manages the client-side session.
/// </summary>
class client_worker {
public:
    /// <summary>
    /// Constructs a new client worker.
    /// </summary>
    /// <param name="port">The port to connect to on the server.</param>
    /// <param name="app_factory">Factory function to create application protocol handlers.</param>
    explicit client_worker(
        int port,
        std::function<std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler>()> app_factory
    );

    /// <summary>
    /// Destructor - ensures the worker is stopped.
    /// </summary>
    ~client_worker();

    // Delete copy and move constructors/assignments
    client_worker(const client_worker&) = delete;
    client_worker& operator=(const client_worker&) = delete;
    client_worker(client_worker&&) = delete;
    client_worker& operator=(client_worker&&) = delete;

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
