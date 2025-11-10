#include "sanctuary/soe_protocol/services/udp_socket_network_interface.hpp"
#include <stdexcept>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

namespace sanctuary::soe_protocol::services {

#ifdef _WIN32
bool UdpSocketNetworkInterface::winsock_initialized_ = false;

bool UdpSocketNetworkInterface::initialize_winsock() {
    if (!winsock_initialized_) {
        WSADATA wsa_data;
        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            return false;
        }
        winsock_initialized_ = true;
    }
    return true;
}
#endif

UdpSocketNetworkInterface::UdpSocketNetworkInterface(
    int max_data_length,
    bool connect_on_receive)
    : socket_(INVALID_SOCKET_VALUE)
    , connect_on_receive_(connect_on_receive)
    , remote_end_point_(std::nullopt) {

#ifdef _WIN32
    if (!initialize_winsock()) {
        throw std::runtime_error("Failed to initialize Winsock");
    }
#endif

    // Create IPv4 UDP socket
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        throw std::runtime_error("Failed to create socket. Error: " + std::to_string(WSAGetLastError()));
#else
        throw std::runtime_error("Failed to create socket. Error: " + std::string(strerror(errno)));
#endif
    }

    // Set send and receive buffer sizes
#ifdef _WIN32
    int buffer_size = max_data_length;
    if (setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&buffer_size), sizeof(buffer_size)) < 0) {
        close_socket();
        throw std::runtime_error("Failed to set send buffer size");
    }
    if (setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&buffer_size), sizeof(buffer_size)) < 0) {
        close_socket();
        throw std::runtime_error("Failed to set receive buffer size");
    }
#else
    int buffer_size = max_data_length;
    if (setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        close_socket();
        throw std::runtime_error("Failed to set send buffer size");
    }
    if (setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        close_socket();
        throw std::runtime_error("Failed to set receive buffer size");
    }
#endif
}

UdpSocketNetworkInterface::~UdpSocketNetworkInterface() {
    close_socket();
}

UdpSocketNetworkInterface::UdpSocketNetworkInterface(UdpSocketNetworkInterface&& other) noexcept
    : socket_(other.socket_)
    , connect_on_receive_(other.connect_on_receive_)
    , remote_end_point_(std::move(other.remote_end_point_)) {
    other.socket_ = INVALID_SOCKET_VALUE;
}

UdpSocketNetworkInterface& UdpSocketNetworkInterface::operator=(UdpSocketNetworkInterface&& other) noexcept {
    if (this != &other) {
        close_socket();
        socket_ = other.socket_;
        connect_on_receive_ = other.connect_on_receive_;
        remote_end_point_ = std::move(other.remote_end_point_);
        other.socket_ = INVALID_SOCKET_VALUE;
    }
    return *this;
}

void UdpSocketNetworkInterface::close_socket() {
    if (socket_ != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET_VALUE;
    }
}

int UdpSocketNetworkInterface::available() const {
    if (socket_ == INVALID_SOCKET_VALUE) {
        return 0;
    }

#ifdef _WIN32
    u_long bytes_available = 0;
    if (ioctlsocket(socket_, FIONREAD, &bytes_available) == 0) {
        return static_cast<int>(bytes_available);
    }
#else
    int bytes_available = 0;
    if (ioctl(socket_, FIONREAD, &bytes_available) == 0) {
        return bytes_available;
    }
#endif

    return 0;
}

std::future<int> UdpSocketNetworkInterface::receive_async(
    std::span<uint8_t> receive_to,
    std::stop_token stop_token) {

    return std::async(std::launch::async, [this, receive_to, stop_token]() -> int {
        if (!remote_end_point_.has_value()) {
            throw std::runtime_error("The remote endpoint has not been set. Either bind or connect the interface");
        }

        if (!is_bound()) {
            throw std::runtime_error("Must bind the interface before attempting to receive");
        }

        if (stop_token.stop_requested()) {
            return 0;
        }

        EndPoint from_endpoint = remote_end_point_.value();
        socklen_t from_len = from_endpoint.get_sockaddr_size();

#ifdef _WIN32
        int result = recvfrom(
            socket_,
            reinterpret_cast<char*>(receive_to.data()),
            static_cast<int>(receive_to.size()),
            0,
            from_endpoint.get_sockaddr_mutable(),
            &from_len);

        if (result == SOCKET_ERROR) {
            throw std::runtime_error("Failed to receive data. Error: " + std::to_string(WSAGetLastError()));
        }
#else
        ssize_t result = recvfrom(
            socket_,
            receive_to.data(),
            receive_to.size(),
            0,
            from_endpoint.get_sockaddr_mutable(),
            &from_len);

        if (result < 0) {
            throw std::runtime_error("Failed to receive data. Error: " + std::string(strerror(errno)));
        }
#endif

        if (connect_on_receive_) {
            connect_internal(from_endpoint);
        }

        return static_cast<int>(result);
    });
}

void UdpSocketNetworkInterface::bind(const EndPoint& local_end_point) {
    if (socket_ == INVALID_SOCKET_VALUE) {
        throw std::runtime_error("Socket is not initialized");
    }

    if (::bind(socket_, local_end_point.get_sockaddr(), local_end_point.get_sockaddr_size()) < 0) {
#ifdef _WIN32
        throw std::runtime_error("Failed to bind socket. Error: " + std::to_string(WSAGetLastError()));
#else
        throw std::runtime_error("Failed to bind socket. Error: " + std::string(strerror(errno)));
#endif
    }

    // Initialize remote endpoint with IPv6 address structure for recvfrom
    remote_end_point_ = EndPoint(EndPoint::AddressFamily::InterNetworkV6, 0);
}

int UdpSocketNetworkInterface::send(std::span<const uint8_t> data) {
    if (!remote_end_point_.has_value()) {
        throw std::runtime_error("The remote endpoint has not been set. Either bind or connect the interface");
    }

    const EndPoint& remote = remote_end_point_.value();

#ifdef _WIN32
    int result = sendto(
        socket_,
        reinterpret_cast<const char*>(data.data()),
        static_cast<int>(data.size()),
        0,
        remote.get_sockaddr(),
        remote.get_sockaddr_size());

    if (result == SOCKET_ERROR) {
        throw std::runtime_error("Failed to send data. Error: " + std::to_string(WSAGetLastError()));
    }
#else
    ssize_t result = sendto(
        socket_,
        data.data(),
        data.size(),
        0,
        remote.get_sockaddr(),
        remote.get_sockaddr_size());

    if (result < 0) {
        throw std::runtime_error("Failed to send data. Error: " + std::string(strerror(errno)));
    }
#endif

    return static_cast<int>(result);
}

std::future<int> UdpSocketNetworkInterface::send_async(
    std::span<const uint8_t> data,
    std::stop_token stop_token) {

    return std::async(std::launch::async, [this, data, stop_token]() -> int {
        if (!remote_end_point_.has_value()) {
            throw std::runtime_error("The remote endpoint has not been set. Either bind or connect the interface");
        }

        if (stop_token.stop_requested()) {
            return 0;
        }

        const EndPoint& remote = remote_end_point_.value();

#ifdef _WIN32
        int result = sendto(
            socket_,
            reinterpret_cast<const char*>(data.data()),
            static_cast<int>(data.size()),
            0,
            remote.get_sockaddr(),
            remote.get_sockaddr_size());

        if (result == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send data. Error: " + std::to_string(WSAGetLastError()));
        }
#else
        ssize_t result = sendto(
            socket_,
            data.data(),
            data.size(),
            0,
            remote.get_sockaddr(),
            remote.get_sockaddr_size());

        if (result < 0) {
            throw std::runtime_error("Failed to send data. Error: " + std::string(strerror(errno)));
        }
#endif

        return static_cast<int>(result);
    });
}

void UdpSocketNetworkInterface::connect(const EndPoint& remote_end_point) {
    if (socket_ == INVALID_SOCKET_VALUE) {
        throw std::runtime_error("Socket is not initialized");
    }

    if (::connect(socket_, remote_end_point.get_sockaddr(), remote_end_point.get_sockaddr_size()) < 0) {
#ifdef _WIN32
        throw std::runtime_error("Failed to connect socket. Error: " + std::to_string(WSAGetLastError()));
#else
        throw std::runtime_error("Failed to connect socket. Error: " + std::string(strerror(errno)));
#endif
    }

    connect_internal(remote_end_point);
}

void UdpSocketNetworkInterface::connect_internal(const EndPoint& remote_end_point) {
    remote_end_point_ = remote_end_point;
    connect_on_receive_ = false;
}

bool UdpSocketNetworkInterface::is_bound() const {
    if (socket_ == INVALID_SOCKET_VALUE) {
        return false;
    }

    sockaddr_storage addr{};
    socklen_t addr_len = sizeof(addr);

    if (getsockname(socket_, reinterpret_cast<sockaddr*>(&addr), &addr_len) < 0) {
        return false;
    }

    // Check if the socket is bound to a valid address (not unspecified)
    if (addr.ss_family == AF_INET) {
        const auto* addr_in = reinterpret_cast<const sockaddr_in*>(&addr);
        return addr_in->sin_port != 0;
    } else if (addr.ss_family == AF_INET6) {
        const auto* addr_in6 = reinterpret_cast<const sockaddr_in6*>(&addr);
        return addr_in6->sin6_port != 0;
    }

    return false;
}

} // namespace sanctuary::soe_protocol::services
