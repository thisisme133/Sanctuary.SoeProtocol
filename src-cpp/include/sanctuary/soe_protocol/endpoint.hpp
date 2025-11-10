#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <array>
#include <optional>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

namespace sanctuary::soe_protocol {

/// <summary>
/// Represents a network endpoint consisting of an IP address and port.
/// Supports both IPv4 and IPv6 addresses.
/// </summary>
class EndPoint {
public:
    /// <summary>
    /// Address family type.
    /// </summary>
    enum class AddressFamily : uint16_t {
        InterNetwork = AF_INET,      // IPv4
        InterNetworkV6 = AF_INET6    // IPv6
    };

    /// <summary>
    /// Creates an empty endpoint (unspecified address family).
    /// </summary>
    EndPoint();

    /// <summary>
    /// Creates an endpoint with the specified address family and port.
    /// Address will be INADDR_ANY/IN6ADDR_ANY_INIT.
    /// </summary>
    /// <param name="family">The address family.</param>
    /// <param name="port">The port number.</param>
    EndPoint(AddressFamily family, uint16_t port);

    /// <summary>
    /// Creates an IPv4 endpoint from the specified address and port.
    /// </summary>
    /// <param name="address">The IPv4 address string (e.g., "127.0.0.1").</param>
    /// <param name="port">The port number.</param>
    /// <returns>An EndPoint if parsing succeeded, std::nullopt otherwise.</returns>
    [[nodiscard]] static std::optional<EndPoint> from_ipv4(std::string_view address, uint16_t port);

    /// <summary>
    /// Creates an IPv6 endpoint from the specified address and port.
    /// </summary>
    /// <param name="address">The IPv6 address string (e.g., "::1").</param>
    /// <param name="port">The port number.</param>
    /// <returns>An EndPoint if parsing succeeded, std::nullopt otherwise.</returns>
    [[nodiscard]] static std::optional<EndPoint> from_ipv6(std::string_view address, uint16_t port);

    /// <summary>
    /// Creates an endpoint from the specified address string and port.
    /// Automatically detects IPv4 or IPv6.
    /// </summary>
    /// <param name="address">The IP address string.</param>
    /// <param name="port">The port number.</param>
    /// <returns>An EndPoint if parsing succeeded, std::nullopt otherwise.</returns>
    [[nodiscard]] static std::optional<EndPoint> parse(std::string_view address, uint16_t port);

    /// <summary>
    /// Creates an endpoint from a sockaddr structure.
    /// </summary>
    /// <param name="addr">The sockaddr structure.</param>
    /// <param name="addr_len">The length of the sockaddr structure.</param>
    [[nodiscard]] static EndPoint from_sockaddr(const sockaddr* addr, socklen_t addr_len);

    /// <summary>
    /// Gets the address family of this endpoint.
    /// </summary>
    [[nodiscard]] AddressFamily get_address_family() const;

    /// <summary>
    /// Gets the port number of this endpoint.
    /// </summary>
    [[nodiscard]] uint16_t get_port() const;

    /// <summary>
    /// Gets the address as a string.
    /// </summary>
    [[nodiscard]] std::string get_address_string() const;

    /// <summary>
    /// Gets a string representation of this endpoint (address:port).
    /// </summary>
    [[nodiscard]] std::string to_string() const;

    /// <summary>
    /// Gets a pointer to the underlying sockaddr structure.
    /// </summary>
    [[nodiscard]] const sockaddr* get_sockaddr() const;

    /// <summary>
    /// Gets a mutable pointer to the underlying sockaddr structure.
    /// </summary>
    [[nodiscard]] sockaddr* get_sockaddr_mutable();

    /// <summary>
    /// Gets the size of the underlying sockaddr structure.
    /// </summary>
    [[nodiscard]] socklen_t get_sockaddr_size() const;

    /// <summary>
    /// Sets the port number.
    /// </summary>
    void set_port(uint16_t port);

private:
    sockaddr_storage storage_{};
    socklen_t size_{};

    explicit EndPoint(const sockaddr* addr, socklen_t addr_len);
};

} // namespace sanctuary::soe_protocol

namespace sanctuary::soe_protocol::abstractions::services {
    // Forward declaration compatibility
    using EndPoint = sanctuary::soe_protocol::EndPoint;
}
