#include "sanctuary/soe_protocol/soe_socket_handler.hpp"
#include "sanctuary/soe_protocol/objects/packets/remap_connection.hpp"
#include "sanctuary/soe_protocol/objects/session_parameters.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include "sanctuary/soe_protocol/util/native_span_pool.hpp"
#include "sanctuary/soe_protocol/util/soe_packet_utils.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace sanctuary::soe_protocol {

namespace {

/// <summary>
/// Simple network writer implementation for UDP socket.
/// </summary>
class SocketNetworkWriter : public abstractions::services::NetworkWriter {
public:
    SocketNetworkWriter(EndPoint remote, socket_t socket)
        : remote_(std::move(remote))
        , socket_(socket)
    {}

    int send(std::span<const uint8_t> data) override {
#ifdef _WIN32
        return ::sendto(
            socket_,
            reinterpret_cast<const char*>(data.data()),
            static_cast<int>(data.size()),
            0,
            remote_.get_sockaddr(),
            remote_.get_sockaddr_size()
        );
#else
        return static_cast<int>(::sendto(
            socket_,
            data.data(),
            data.size(),
            0,
            remote_.get_sockaddr(),
            remote_.get_sockaddr_size()
        ));
#endif
    }

    std::future<int> send_async(std::span<const uint8_t> data, std::stop_token stop_token) override {
        // For now, just perform synchronous send
        // In a production implementation, this would use async I/O
        return std::async(std::launch::async, [this, vec = std::vector<uint8_t>(data.begin(), data.end())]() {
            return send(vec);
        });
    }

private:
    EndPoint remote_;
    socket_t socket_;
};

} // anonymous namespace

SoeSocketHandler::SoeSocketHandler(std::shared_ptr<objects::SocketHandlerParams> parameters)
    : parameters_(std::move(parameters))
    , socket_(INVALID_SOCKET_VALUE)
{
    const auto& ssn_params = parameters_->default_session_params();

    size_t max_data_len = std::max(
        ssn_params->udp_length(),
        ssn_params->remote_udp_length()
    );

    pool_ = std::make_unique<util::NativeSpanPool>(
        max_data_len,
        static_cast<size_t>(parameters_->packet_pool_size())
    );

    size_t max_socket_len = max_data_len * 64;

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("Failed to initialize Winsock");
    }

    socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }

    // Set non-blocking mode
    u_long mode = 1;
    ioctlsocket(socket_, FIONBIO, &mode);

    // Set buffer sizes
    int recv_buf_size = static_cast<int>(max_socket_len);
    int send_buf_size = static_cast<int>(max_socket_len);
    setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&recv_buf_size), sizeof(recv_buf_size));
    setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&send_buf_size), sizeof(send_buf_size));
#else
    socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set non-blocking mode
    int flags = fcntl(socket_, F_GETFL, 0);
    fcntl(socket_, F_SETFL, flags | O_NONBLOCK);

    // Set buffer sizes
    int recv_buf_size = static_cast<int>(max_socket_len);
    int send_buf_size = static_cast<int>(max_socket_len);
    setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(recv_buf_size));
    setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size));
#endif

    receive_buffer_.resize(ssn_params->udp_length() * 32);
}

SoeSocketHandler::~SoeSocketHandler() {
    close_socket();
}

void SoeSocketHandler::close_socket() {
    if (socket_ != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(socket_);
        WSACleanup();
#else
        ::close(socket_);
#endif
        socket_ = INVALID_SOCKET_VALUE;
    }
}

void SoeSocketHandler::bind(const EndPoint& endpoint) {
    if (::bind(socket_, endpoint.get_sockaddr(), endpoint.get_sockaddr_size()) != 0) {
        throw std::runtime_error("Failed to bind socket");
    }
}

SoeProtocolHandler& SoeSocketHandler::connect(const EndPoint& remote) {
    auto& session = create_session(remote, objects::SessionMode::Client);
    session.send_session_request();
    return session;
}

void SoeSocketHandler::run_async(std::stop_token stop_token) {
    is_running_ = true;
    internal_stop_source_ = std::stop_source();

    // Create a combined stop token
    std::stop_source combined_source;
    std::stop_callback external_cb(stop_token, [&combined_source]() {
        combined_source.request_stop();
    });
    std::stop_callback internal_cb(internal_stop_source_.get_token(), [&combined_source]() {
        combined_source.request_stop();
    });

    // Start the socket receive loop in a separate thread
    std::thread receive_thread([this, token = combined_source.get_token()]() {
        run_socket_receive_loop(token);
    });

    try {
        while (!combined_source.get_token().stop_requested()) {
            bool run_next_tick = run_tick(false, combined_source.get_token());
            if (!run_next_tick) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    } catch (...) {
        // Ensure cleanup happens
    }

    combined_source.request_stop();
    if (receive_thread.joinable()) {
        receive_thread.join();
    }

    is_running_ = false;
}

bool SoeSocketHandler::run_tick(bool read_from_socket, std::stop_token stop_token) {
    bool run_next_tick = false;

    if (read_from_socket) {
        // Check if data is available
#ifdef _WIN32
        u_long bytes_available = 0;
        ioctlsocket(socket_, FIONREAD, &bytes_available);
        if (bytes_available > 0)
#else
        // Use non-blocking receive
#endif
        {
            EndPoint remote_address;
            socklen_t addr_len = sizeof(sockaddr_storage);

#ifdef _WIN32
            int received_len = ::recvfrom(
                socket_,
                reinterpret_cast<char*>(receive_buffer_.data()),
                static_cast<int>(receive_buffer_.size()),
                0,
                remote_address.get_sockaddr_mutable(),
                &addr_len
            );
#else
            ssize_t received_len = ::recvfrom(
                socket_,
                receive_buffer_.data(),
                receive_buffer_.size(),
                0,
                remote_address.get_sockaddr_mutable(),
                &addr_len
            );
#endif

            if (received_len > 0) {
                run_next_tick = process_one_from_socket(
                    remote_address,
                    std::span<uint8_t>(receive_buffer_.data(), static_cast<size_t>(received_len))
                );
            }
        }
    }

    std::vector<SoeProtocolHandler*> to_remove;
    to_remove.reserve(16);

    for (auto& [endpoint, session] : sessions_) {
        if (session->termination_reason() != objects::DisconnectReason::None
            || session->state() == objects::SessionState::Terminated) {
            to_remove.push_back(session.get());
            continue;
        }

        bool needs_more_time = false;
        try {
            if (!session->run_tick(needs_more_time, stop_token)) {
                to_remove.push_back(session.get());
            }
        } catch (const std::exception&) {
            // Log error if logging is available
            to_remove.push_back(session.get());
        }

        if (needs_more_time) {
            run_next_tick = true;
        }
    }

    for (auto* session : to_remove) {
        try {
            destroy_session(*session);
        } catch (const std::exception&) {
            // Log error if logging is available
        }
    }

    return run_next_tick;
}

bool SoeSocketHandler::process_one_from_socket(
    const EndPoint& remote_address,
    std::span<uint8_t> received_data
) {
    auto it = sessions_.find(remote_address);

    if (it == sessions_.end()) {
        // No existing session for this endpoint
        SoeOpCode op_code = util::SoePacketUtils::read_soe_op_code(received_data);

        switch (op_code) {
            case SoeOpCode::SessionRequest: {
                auto& session = create_session(remote_address, objects::SessionMode::Server);
                auto span = pool_->rent();
                span->copy_data_into(received_data);
                if (!session.enqueue_packet(std::move(span))) {
                    pool_->return_span(std::move(span));
                }
                break;
            }
            case SoeOpCode::RemapConnection: {
                auto remap = objects::packets::RemapConnection::deserialize(received_data, true);
                remap_session(remote_address, remap);
                break;
            }
            default:
                // Unknown sender - could send UnknownSender packet here
                break;
        }
        return true;
    }

    // Existing session
    auto span = pool_->rent();
    span->copy_data_into(received_data);
    if (!it->second->enqueue_packet(std::move(span))) {
        pool_->return_span(std::move(span));
    }

    return true;
}

void SoeSocketHandler::run_socket_receive_loop(std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
        EndPoint remote_address;
        socklen_t addr_len = sizeof(sockaddr_storage);

#ifdef _WIN32
        int received_len = ::recvfrom(
            socket_,
            reinterpret_cast<char*>(receive_buffer_.data()),
            static_cast<int>(receive_buffer_.size()),
            0,
            remote_address.get_sockaddr_mutable(),
            &addr_len
        );

        if (received_len == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            break;
        }
#else
        ssize_t received_len = ::recvfrom(
            socket_,
            receive_buffer_.data(),
            receive_buffer_.size(),
            0,
            remote_address.get_sockaddr_mutable(),
            &addr_len
        );

        if (received_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            break;
        }
#endif

        if (received_len > 0) {
            process_one_from_socket(
                remote_address,
                std::span<uint8_t>(receive_buffer_.data(), static_cast<size_t>(received_len))
            );
        }
    }
}

SoeProtocolHandler& SoeSocketHandler::create_session(EndPoint address, objects::SessionMode mode) {
    auto handler = std::make_unique<SoeProtocolHandler>(
        address,
        mode,
        parameters_->default_session_params()->clone(),
        *pool_,
        std::make_unique<SocketNetworkWriter>(address, socket_),
        parameters_->app_creation_callback()()
    );

    handler->initialize();

    auto& ref = *handler;
    sessions_.emplace(std::move(address), std::move(handler));

    return ref;
}

void SoeSocketHandler::destroy_session(SoeProtocolHandler& session) {
    if (session.termination_reason() != objects::DisconnectReason::None) {
        // Log destruction if logging is available
    }

    session.terminate_session();

    // Find and remove the session
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it->second.get() == &session) {
            sessions_.erase(it);
            break;
        }
    }

    if (sessions_.empty() && parameters_->stop_on_last_session_terminated()) {
        internal_stop_source_.request_stop();
    }
}

void SoeSocketHandler::remap_session(
    const EndPoint& address,
    const objects::packets::RemapConnection& remap_request
) {
    if (!parameters_->allow_port_remaps()) {
        return;
    }

    SoeProtocolHandler* handler = nullptr;

    for (auto& [endpoint, session] : sessions_) {
        if (session->session_id() == remap_request.session_id
            && session->session_params().crc_seed() == remap_request.crc_seed) {
            handler = session.get();
            break;
        }
    }

    // If we couldn't find a matching handler then just blindly return. No need to notify the sender
    if (handler == nullptr) {
        return;
    }

    // Check that only the port has changed (same IP address)
    std::string new_addr = address.get_address_string();
    std::string old_addr = handler->remote().get_address_string();

    // We do NOT want to handle IP address remaps - this is probably someone trying to hijack a session.
    if (new_addr != old_addr) {
        return;
    }

    // At this point only the port has changed - probably due to NAT. We're happy to remap this
    // Extract the session and remove it from the old endpoint
    std::unique_ptr<SoeProtocolHandler> session_ptr;
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it->second.get() == handler) {
            session_ptr = std::move(it->second);
            sessions_.erase(it);
            break;
        }
    }

    // Update the remote endpoint
    handler->remote() = address;

    // Re-insert with new endpoint
    sessions_.emplace(address, std::move(session_ptr));
}

} // namespace sanctuary::soe_protocol
