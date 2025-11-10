#include "sanctuary/soe_protocol/services/socket_network_writer.hpp"
#include <stdexcept>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <errno.h>
    #include <unistd.h>
#endif

namespace sanctuary::soe_protocol::services {

SocketNetworkWriter::SocketNetworkWriter(EndPoint remote, socket_t socket)
    : remote_(std::move(remote))
    , socket_(socket) {
}

int SocketNetworkWriter::send(std::span<const uint8_t> data) {
#ifdef _WIN32
    int result = sendto(
        socket_,
        reinterpret_cast<const char*>(data.data()),
        static_cast<int>(data.size()),
        0,
        remote_.get_sockaddr(),
        remote_.get_sockaddr_size());

    if (result == SOCKET_ERROR) {
        throw std::runtime_error("Failed to send data. Error: " + std::to_string(WSAGetLastError()));
    }
#else
    ssize_t result = sendto(
        socket_,
        data.data(),
        data.size(),
        0,
        remote_.get_sockaddr(),
        remote_.get_sockaddr_size());

    if (result < 0) {
        throw std::runtime_error("Failed to send data. Error: " + std::string(strerror(errno)));
    }
#endif

    return static_cast<int>(result);
}

std::future<int> SocketNetworkWriter::send_async(
    std::span<const uint8_t> data,
    std::stop_token stop_token) {

    return std::async(std::launch::async, [this, data, stop_token]() -> int {
        if (stop_token.stop_requested()) {
            return 0;
        }

#ifdef _WIN32
        int result = sendto(
            socket_,
            reinterpret_cast<const char*>(data.data()),
            static_cast<int>(data.size()),
            0,
            remote_.get_sockaddr(),
            remote_.get_sockaddr_size());

        if (result == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send data. Error: " + std::to_string(WSAGetLastError()));
        }
#else
        ssize_t result = sendto(
            socket_,
            data.data(),
            data.size(),
            0,
            remote_.get_sockaddr(),
            remote_.get_sockaddr_size());

        if (result < 0) {
            throw std::runtime_error("Failed to send data. Error: " + std::string(strerror(errno)));
        }
#endif

        return static_cast<int>(result);
    });
}

} // namespace sanctuary::soe_protocol::services
