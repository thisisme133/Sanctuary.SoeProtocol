#pragma once

#include <cstdint>
#include <future>
#include <span>
#include <stop_token>

namespace sanctuary::soe_protocol::abstractions::services {

// Forward declaration for endpoint abstraction
struct EndPoint;

/// <summary>
/// Represents a network IO abstraction for
/// reading data from a remote endpoint.
/// </summary>
class NetworkReader {
public:
    virtual ~NetworkReader() = default;

    /// <summary>
    /// Gets the amount of data that has been received from the
    /// network interface and is available to be read.
    /// </summary>
    [[nodiscard]] virtual int available() const = 0;

    /// <summary>
    /// Receives data from the network stream.
    /// </summary>
    /// <param name="receive_to">The buffer to receive the data to.</param>
    /// <param name="stop_token">A stop token that can be used to stop the operation.</param>
    /// <returns>A future containing the number of bytes read from the network stream.</returns>
    [[nodiscard]] virtual std::future<int> receive_async(
        std::span<uint8_t> receive_to,
        std::stop_token stop_token = std::stop_token()) = 0;

    /// <summary>
    /// Binds this <see cref="NetworkReader"/> to a local endpoint.
    /// </summary>
    /// <param name="local_end_point">The endpoint to bind to.</param>
    virtual void bind(const EndPoint& local_end_point) = 0;
};

} // namespace sanctuary::soe_protocol::abstractions::services
