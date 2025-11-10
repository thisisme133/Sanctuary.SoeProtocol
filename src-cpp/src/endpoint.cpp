#include "sanctuary/soe_protocol/endpoint.hpp"
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
    #pragma comment(lib, "ws2_32.lib")
#endif

namespace sanctuary::soe_protocol {

EndPoint::EndPoint() : size_(0) {
    std::memset(&storage_, 0, sizeof(storage_));
}

EndPoint::EndPoint(AddressFamily family, uint16_t port) {
    std::memset(&storage_, 0, sizeof(storage_));

    if (family == AddressFamily::InterNetwork) {
        auto* addr = reinterpret_cast<sockaddr_in*>(&storage_);
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        addr->sin_addr.s_addr = INADDR_ANY;
        size_ = sizeof(sockaddr_in);
    } else {
        auto* addr = reinterpret_cast<sockaddr_in6*>(&storage_);
        addr->sin6_family = AF_INET6;
        addr->sin6_port = htons(port);
        addr->sin6_addr = IN6ADDR_ANY_INIT;
        size_ = sizeof(sockaddr_in6);
    }
}

EndPoint::EndPoint(const sockaddr* addr, socklen_t addr_len) : size_(addr_len) {
    std::memset(&storage_, 0, sizeof(storage_));
    if (addr_len <= sizeof(storage_)) {
        std::memcpy(&storage_, addr, addr_len);
    }
}

std::optional<EndPoint> EndPoint::from_ipv4(std::string_view address, uint16_t port) {
    EndPoint ep;
    auto* addr = reinterpret_cast<sockaddr_in*>(&ep.storage_);

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    std::string addr_str(address);
    if (inet_pton(AF_INET, addr_str.c_str(), &addr->sin_addr) != 1) {
        return std::nullopt;
    }

    ep.size_ = sizeof(sockaddr_in);
    return ep;
}

std::optional<EndPoint> EndPoint::from_ipv6(std::string_view address, uint16_t port) {
    EndPoint ep;
    auto* addr = reinterpret_cast<sockaddr_in6*>(&ep.storage_);

    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);

    std::string addr_str(address);
    if (inet_pton(AF_INET6, addr_str.c_str(), &addr->sin6_addr) != 1) {
        return std::nullopt;
    }

    ep.size_ = sizeof(sockaddr_in6);
    return ep;
}

std::optional<EndPoint> EndPoint::parse(std::string_view address, uint16_t port) {
    // Try IPv4 first
    if (auto ep = from_ipv4(address, port)) {
        return ep;
    }

    // Try IPv6
    return from_ipv6(address, port);
}

EndPoint EndPoint::from_sockaddr(const sockaddr* addr, socklen_t addr_len) {
    return EndPoint(addr, addr_len);
}

EndPoint::AddressFamily EndPoint::get_address_family() const {
    const auto* base = reinterpret_cast<const sockaddr*>(&storage_);
    return static_cast<AddressFamily>(base->sa_family);
}

uint16_t EndPoint::get_port() const {
    const auto* base = reinterpret_cast<const sockaddr*>(&storage_);

    if (base->sa_family == AF_INET) {
        const auto* addr = reinterpret_cast<const sockaddr_in*>(&storage_);
        return ntohs(addr->sin_port);
    } else if (base->sa_family == AF_INET6) {
        const auto* addr = reinterpret_cast<const sockaddr_in6*>(&storage_);
        return ntohs(addr->sin6_port);
    }

    return 0;
}

std::string EndPoint::get_address_string() const {
    const auto* base = reinterpret_cast<const sockaddr*>(&storage_);
    char buffer[INET6_ADDRSTRLEN];

    if (base->sa_family == AF_INET) {
        const auto* addr = reinterpret_cast<const sockaddr_in*>(&storage_);
        if (inet_ntop(AF_INET, &addr->sin_addr, buffer, sizeof(buffer)) != nullptr) {
            return std::string(buffer);
        }
    } else if (base->sa_family == AF_INET6) {
        const auto* addr = reinterpret_cast<const sockaddr_in6*>(&storage_);
        if (inet_ntop(AF_INET6, &addr->sin6_addr, buffer, sizeof(buffer)) != nullptr) {
            return std::string(buffer);
        }
    }

    return "";
}

std::string EndPoint::to_string() const {
    const auto* base = reinterpret_cast<const sockaddr*>(&storage_);

    if (base->sa_family == AF_INET) {
        return get_address_string() + ":" + std::to_string(get_port());
    } else if (base->sa_family == AF_INET6) {
        return "[" + get_address_string() + "]:" + std::to_string(get_port());
    }

    return "";
}

const sockaddr* EndPoint::get_sockaddr() const {
    return reinterpret_cast<const sockaddr*>(&storage_);
}

sockaddr* EndPoint::get_sockaddr_mutable() {
    return reinterpret_cast<sockaddr*>(&storage_);
}

socklen_t EndPoint::get_sockaddr_size() const {
    return size_;
}

void EndPoint::set_port(uint16_t port) {
    auto* base = reinterpret_cast<sockaddr*>(&storage_);

    if (base->sa_family == AF_INET) {
        auto* addr = reinterpret_cast<sockaddr_in*>(&storage_);
        addr->sin_port = htons(port);
    } else if (base->sa_family == AF_INET6) {
        auto* addr = reinterpret_cast<sockaddr_in6*>(&storage_);
        addr->sin6_port = htons(port);
    }
}

} // namespace sanctuary::soe_protocol
