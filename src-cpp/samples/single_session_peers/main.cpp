/// <summary>
/// SingleSessionPeers Sample Application
///
/// This sample demonstrates how to create client and server SOE protocol peers
/// that communicate with each other in a ping-pong pattern to test throughput.
///
/// The sample:
/// - Starts a server listening on the specified port
/// - Starts a client that connects to the server
/// - Exchanges "Ping!" and "Pong!" messages for a specified duration (10 seconds)
/// - Measures and reports throughput (messages per second)
/// - Automatically shuts down when the test completes
///
/// Configuration:
/// - Default Port: 12345 (configurable via command line argument)
/// - Application Protocol: "Ping_1"
/// - Test Duration: 10 seconds
/// - Encryption: Disabled (for simplicity)
/// - Compression: Disabled (for simplicity)
///
/// Usage:
///   single_session_peers [port]
///
/// Example:
///   single_session_peers 12345
///
/// The program will automatically terminate after the ping-pong test completes.
/// </summary>

#include "ping_application.hpp"
#include "client_worker.hpp"
#include "server_worker.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>
#include <cstdlib>

auto main(int argc, char* argv[]) -> int {
    try {
        // Configure logging
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("single_session_peers", console_sink);
        logger->set_level(spdlog::level::debug);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_default_logger(logger);

        // Parse configuration
        int port = 12345; // Default port
        if (argc > 1) {
            try {
                port = std::stoi(argv[1]);
            } catch (const std::exception&) {
                spdlog::error("Invalid port number provided");
                return 1;
            }
        }

        spdlog::info("SingleSessionPeers starting...");
        spdlog::info("Port: {}", port);

        // Create application factory - this will be called for both client and server
        auto app_factory = []() -> std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler> {
            return std::make_shared<single_session_peers::ping_application>();
        };

        // Create and start the server worker
        auto server = single_session_peers::server_worker(port, app_factory);
        server.start();
        spdlog::info("Server started on port {}", port);

        // Create and start the client worker
        auto client = single_session_peers::client_worker(port, app_factory);
        client.start();
        spdlog::info("Client started, connecting to server on port {}", port);

        // Wait for both workers to complete
        // The client will terminate after the ping-pong test duration (10 seconds)
        // and both workers are configured to stop when the last session terminates
        spdlog::info("Ping-pong test running...");

        server.wait();
        client.wait();

        spdlog::info("Ping-pong test completed");
        return 0;

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }
}
