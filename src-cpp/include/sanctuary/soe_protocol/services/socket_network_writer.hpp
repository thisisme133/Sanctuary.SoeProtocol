#pragma once

#include "sanctuary/soe_protocol/abstractions/services/network_writer.hpp"
#include "sanctuary/soe_protocol/endpoint.hpp"
#include <cstdint>
#include <future>
#include <span>
#include <stop_token>

#ifdef _WIN32
    #include <winsock2.h>
    using socket_t = SOCKET;
#else
    using socket_t = int;
#endif

namespace sanctuary::soe_protocol::services {

/// <summary>
/// Provides an implementation of the NetworkWriter interface, which simply wraps a socket
/// and stores a remote address to which data will be sent.
/// </summary>
class SocketNetworkWriter : public abstractions::services::NetworkWriter {
public:
    /// <summary>
    /// Initializes a new instance of the SocketNetworkWriter.
    /// </summary>
    /// <param name="remote">The remote address to which data will be sent.</param>
    /// <param name="socket">The underlying socket to send data on.</param>
    SocketNetworkWriter(EndPoint remote, socket_t socket);

    /// <summary>
    /// Destroys the SocketNetworkWriter.
    /// Note: This does not close the underlying socket, as it does not own it.
    /// </summary>
    ~SocketNetworkWriter() override = default;

    // Disable copy
    SocketNetworkWriter(const SocketNetworkWriter&) = delete;
    SocketNetworkWriter& operator=(const SocketNetworkWriter&) = delete;

    // Enable move
    SocketNetworkWriter(SocketNetworkWriter&& other) noexcept = default;
    SocketNetworkWriter& operator=(SocketNetworkWriter&& other) noexcept = default;

    // NetworkWriter interface

    /// <inheritdoc />
    [[nodiscard]] int send(std::span<const uint8_t> data) override;

    /// <inheritdoc />
    [[nodiscard]] std::future<int> send_async(
        std::span<const uint8_t> data,
        std::stop_token stop_token = std::stop_token()) override;

    /// <summary>
    /// Gets the remote endpoint.
    /// </summary>
    [[nodiscard]] const EndPoint& get_remote() const { return remote_; }

    /// <summary>
    /// Gets the underlying socket handle.
    /// </summary>
    [[nodiscard]] socket_t get_socket() const { return socket_; }

private:
    EndPoint remote_;
    socket_t socket_;
};

} // namespace sanctuary::soe_protocol::services
