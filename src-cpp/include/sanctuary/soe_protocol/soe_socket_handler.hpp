#pragma once

#include "sanctuary/soe_protocol/endpoint.hpp"
#include "sanctuary/soe_protocol/objects/session_mode.hpp"
#include "sanctuary/soe_protocol/objects/socket_handler_params.hpp"
#include "sanctuary/soe_protocol/soe_protocol_handler.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stop_token>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    using socket_t = int;
    constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

// Forward declarations
namespace sanctuary::soe_protocol::util {
    class NativeSpanPool;
}

namespace sanctuary::soe_protocol {

// Custom comparator for EndPoint to use as map key
struct EndPointCompare {
    bool operator()(const EndPoint& lhs, const EndPoint& rhs) const {
        // Compare the underlying sockaddr structures
        const auto* lhs_addr = lhs.get_sockaddr();
        const auto* rhs_addr = rhs.get_sockaddr();
        auto lhs_size = lhs.get_sockaddr_size();
        auto rhs_size = rhs.get_sockaddr_size();

        if (lhs_size != rhs_size) {
            return lhs_size < rhs_size;
        }

        return std::memcmp(lhs_addr, rhs_addr, lhs_size) < 0;
    }
};

/// <summary>
/// Manages a UDP socket and any SOE connections on that socket.
/// </summary>
class SoeSocketHandler {
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SoeSocketHandler"/> class.
    /// </summary>
    /// <param name="parameters">The control parameters for this instance.</param>
    explicit SoeSocketHandler(std::shared_ptr<objects::SocketHandlerParams> parameters);

    ~SoeSocketHandler();
    SoeSocketHandler(const SoeSocketHandler&) = delete;
    SoeSocketHandler(SoeSocketHandler&&) = delete;
    SoeSocketHandler& operator=(const SoeSocketHandler&) = delete;
    SoeSocketHandler& operator=(SoeSocketHandler&&) = delete;

    /// <summary>
    /// Binds this socket handler to an endpoint, readying it to act as a server.
    /// </summary>
    /// <param name="endpoint">The endpoint to listen on.</param>
    void bind(const EndPoint& endpoint);

    /// <summary>
    /// Creates a session and connects it to the given remote address.
    /// </summary>
    /// <param name="remote">The address of the remote to connect to.</param>
    /// <returns>A reference to the created session.</returns>
    SoeProtocolHandler& connect(const EndPoint& remote);

    /// <summary>
    /// Asynchronously runs the <see cref="SoeSocketHandler"/>. This method will not return until cancelled.
    /// Do not use this method in tandem with <see cref="run_tick"/>.
    /// </summary>
    /// <param name="stop_token">A <see cref="stop_token"/> that can be used to cancel this operation.</param>
    void run_async(std::stop_token stop_token);

    /// <summary>
    /// Runs a tick of operations. Do not use this method in tandem with <see cref="run_async"/>.
    /// </summary>
    /// <param name="read_from_socket">
    /// Whether the socket should be read during this tick. Set to <c>false</c> if using an external socket read loop.
    /// </param>
    /// <param name="stop_token">A <see cref="stop_token"/> that can be used to cancel this operation.</param>
    /// <returns><c>True</c> if another tick must be run as soon as possible.</returns>
    [[nodiscard]] bool run_tick(bool read_from_socket, std::stop_token stop_token);

private:
    std::shared_ptr<objects::SocketHandlerParams> parameters_;
    std::map<EndPoint, std::unique_ptr<SoeProtocolHandler>, EndPointCompare> sessions_;
    std::unique_ptr<util::NativeSpanPool> pool_;
    socket_t socket_;
    std::vector<uint8_t> receive_buffer_;
    bool is_running_{false};
    std::stop_source internal_stop_source_;

    void run_socket_receive_loop(std::stop_token stop_token);
    [[nodiscard]] bool process_one_from_socket(const EndPoint& remote_address, std::span<uint8_t> received_data);
    SoeProtocolHandler& create_session(EndPoint address, objects::SessionMode mode);
    void destroy_session(SoeProtocolHandler& session);
    void remap_session(const EndPoint& address, const objects::packets::RemapConnection& remap_request);
    void close_socket();
};

} // namespace sanctuary::soe_protocol
