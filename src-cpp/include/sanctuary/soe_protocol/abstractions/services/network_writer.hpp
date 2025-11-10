#pragma once

#include <cstdint>
#include <future>
#include <span>
#include <stop_token>

namespace sanctuary::soe_protocol::abstractions::services {

/// <summary>
/// Represents a network IO abstraction for
/// writing data to a remote endpoint.
/// </summary>
class NetworkWriter {
public:
    virtual ~NetworkWriter() = default;

    /// <summary>
    /// Sends the given bytes on the network stream.
    /// </summary>
    /// <param name="data">The data to send.</param>
    /// <returns>The number of bytes written to the network stream.</returns>
    [[nodiscard]] virtual int send(std::span<const uint8_t> data) = 0;

    /// <summary>
    /// Sends the given bytes on the network stream.
    /// </summary>
    /// <param name="data">The data to send</param>
    /// <param name="stop_token">A stop token that can be used to stop the operation.</param>
    /// <returns>A future containing the number of bytes written to the network stream.</returns>
    [[nodiscard]] virtual std::future<int> send_async(
        std::span<const uint8_t> data,
        std::stop_token stop_token = std::stop_token()) = 0;
};

} // namespace sanctuary::soe_protocol::abstractions::services
