#pragma once

#include "sanctuary/soe_protocol/abstractions/services/network_interface.hpp"
#include "sanctuary/soe_protocol/endpoint.hpp"
#include <cstdint>
#include <future>
#include <span>
#include <stop_token>
#include <memory>
#include <optional>

#ifdef _WIN32
    #include <winsock2.h>
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
    using socket_t = int;
    constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

namespace sanctuary::soe_protocol::services {

/// <summary>
/// Provides an implementation of the NetworkInterface
/// that sends/receives UDP data using a socket.
/// </summary>
class UdpSocketNetworkInterface : public abstractions::services::NetworkInterface {
public:
    /// <summary>
    /// Initializes a new instance of the UdpSocketNetworkInterface class.
    /// </summary>
    /// <param name="max_data_length">The maximum length of data that may be sent.</param>
    /// <param name="connect_on_receive">True to connect the socket upon receiving remote data.</param>
    explicit UdpSocketNetworkInterface(
        int max_data_length,
        bool connect_on_receive = false);

    /// <summary>
    /// Destroys the UdpSocketNetworkInterface and releases the underlying socket.
    /// </summary>
    ~UdpSocketNetworkInterface() override;

    // Disable copy
    UdpSocketNetworkInterface(const UdpSocketNetworkInterface&) = delete;
    UdpSocketNetworkInterface& operator=(const UdpSocketNetworkInterface&) = delete;

    // Enable move
    UdpSocketNetworkInterface(UdpSocketNetworkInterface&& other) noexcept;
    UdpSocketNetworkInterface& operator=(UdpSocketNetworkInterface&& other) noexcept;

    // NetworkReader interface

    /// <inheritdoc />
    [[nodiscard]] int available() const override;

    /// <inheritdoc />
    [[nodiscard]] std::future<int> receive_async(
        std::span<uint8_t> receive_to,
        std::stop_token stop_token = std::stop_token()) override;

    /// <inheritdoc />
    void bind(const EndPoint& local_end_point) override;

    // NetworkWriter interface

    /// <inheritdoc />
    [[nodiscard]] int send(std::span<const uint8_t> data) override;

    /// <inheritdoc />
    [[nodiscard]] std::future<int> send_async(
        std::span<const uint8_t> data,
        std::stop_token stop_token = std::stop_token()) override;

    // Additional methods

    /// <summary>
    /// Connects the interface to a remote endpoint.
    /// </summary>
    /// <param name="remote_end_point">The remote endpoint to connect to.</param>
    void connect(const EndPoint& remote_end_point);

    /// <summary>
    /// Gets the underlying socket handle.
    /// </summary>
    [[nodiscard]] socket_t get_socket() const { return socket_; }

    /// <summary>
    /// Checks if the socket is bound to a local endpoint.
    /// </summary>
    [[nodiscard]] bool is_bound() const;

private:
    socket_t socket_;
    bool connect_on_receive_;
    std::optional<EndPoint> remote_end_point_;

    void close_socket();
    void connect_internal(const EndPoint& remote_end_point);

#ifdef _WIN32
    static bool initialize_winsock();
    static bool winsock_initialized_;
#endif
};

} // namespace sanctuary::soe_protocol::services
