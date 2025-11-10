/// <summary>
/// SimpleServer Sample Application
///
/// This sample demonstrates how to create a simple SOE protocol server that:
/// - Listens for incoming connections on a specified port
/// - Handles login sessions with encryption enabled
/// - Processes application data packets
/// - Manages session lifecycle (open, data handling, close)
///
/// Configuration:
/// - Port: 20042 (configurable via command line argument)
/// - Application Protocol: "LoginUdp_18"
/// - Encryption: Enabled with RC4
/// - Compression: Enabled
///
/// Usage:
///   simple_server [port]
///
/// Example:
///   simple_server 20042
/// </summary>

#include "login_application.hpp"
#include "soe_session_worker.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <csignal>
#include <atomic>
#include <memory>
#include <string>
#include <cstdlib>

namespace {
    std::atomic<bool> shutdown_requested{false};

    void signal_handler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            spdlog::info("Shutdown signal received");
            shutdown_requested = true;
        }
    }
}

auto main(int argc, char* argv[]) -> int {
    try {
        // Configure logging
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("simple_server", console_sink);
        logger->set_level(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_default_logger(logger);

        // Parse configuration
        int port = 20042; // Default port from appsettings.json
        if (argc > 1) {
            try {
                port = std::stoi(argv[1]);
            } catch (const std::exception&) {
                spdlog::error("Invalid port number provided");
                return 1;
            }
        }

        std::string app_protocol = "LoginUdp_18"; // From appsettings.json

        spdlog::info("SimpleServer starting...");
        spdlog::info("Port: {}", port);
        spdlog::info("Application Protocol: {}", app_protocol);

        // Set up signal handlers for graceful shutdown
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // Create the session worker with a factory for login applications
        auto worker = simple_server::soe_session_worker(
            port,
            app_protocol,
            []() -> std::shared_ptr<sanctuary::soe_protocol::application_protocol_handler> {
                return std::make_shared<simple_server::login_application>();
            }
        );

        // Start the worker
        worker.start();
        spdlog::info("Server started successfully. Press Ctrl+C to stop.");

        // Wait for shutdown signal
        while (!shutdown_requested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        spdlog::info("Shutting down server...");
        worker.stop();
        spdlog::info("Server stopped successfully");

        return 0;

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }
}
