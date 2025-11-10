#pragma once

#include "sanctuary/soe_protocol/objects/session_parameters.hpp"
#include <cstdint>
#include <functional>
#include <memory>

// Forward declarations
namespace sanctuary::soe_protocol::abstractions {
    class ApplicationProtocolHandler;
}

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Options used to configure and control the SoeSocketHandler.
/// </summary>
class SocketHandlerParams {
public:
    using AppCreationCallback = std::function<std::unique_ptr<sanctuary::soe_protocol::abstractions::ApplicationProtocolHandler>()>;

    SocketHandlerParams() = default;
    ~SocketHandlerParams() = default;
    SocketHandlerParams(const SocketHandlerParams&) = delete;
    SocketHandlerParams(SocketHandlerParams&&) noexcept = default;
    SocketHandlerParams& operator=(const SocketHandlerParams&) = delete;
    SocketHandlerParams& operator=(SocketHandlerParams&&) noexcept = default;

    /// <summary>
    /// Gets the default session parameters used when creating new session handlers.
    /// </summary>
    [[nodiscard]] const std::shared_ptr<SessionParameters>& default_session_params() const noexcept {
        return default_session_params_;
    }

    /// <summary>
    /// Sets the default session parameters used when creating new session handlers.
    /// </summary>
    void set_default_session_params(std::shared_ptr<SessionParameters> params) {
        default_session_params_ = std::move(params);
    }

    /// <summary>
    /// Gets the callback used to create a new application handler.
    /// </summary>
    [[nodiscard]] const AppCreationCallback& app_creation_callback() const noexcept {
        return app_creation_callback_;
    }

    /// <summary>
    /// Sets the callback used to create a new application handler.
    /// </summary>
    void set_app_creation_callback(AppCreationCallback callback) {
        app_creation_callback_ = std::move(callback);
    }

    /// <summary>
    /// Gets the size of the packet pool used by the socket handler (i.e. how many packets you expect to be waiting in queues
    /// at any one time). This value should scale with the number of expected connections.
    /// </summary>
    [[nodiscard]] int32_t packet_pool_size() const noexcept {
        return packet_pool_size_;
    }

    /// <summary>
    /// Sets the size of the packet pool used by the socket handler.
    /// </summary>
    void set_packet_pool_size(int32_t size) noexcept {
        packet_pool_size_ = size;
    }

    /// <summary>
    /// Gets whether sessions are allowed to remap their ports.
    /// </summary>
    [[nodiscard]] bool allow_port_remaps() const noexcept {
        return allow_port_remaps_;
    }

    /// <summary>
    /// Sets whether sessions are allowed to remap their ports.
    /// </summary>
    void set_allow_port_remaps(bool allow) noexcept {
        allow_port_remaps_ = allow;
    }

    /// <summary>
    /// Gets whether the socket handler should stop when the active session is terminated.
    /// Useful when running in client mode.
    /// This flag only affects the run_async method.
    /// </summary>
    [[nodiscard]] bool stop_on_last_session_terminated() const noexcept {
        return stop_on_last_session_terminated_;
    }

    /// <summary>
    /// Sets whether the socket handler should stop when the active session is terminated.
    /// </summary>
    void set_stop_on_last_session_terminated(bool stop) noexcept {
        stop_on_last_session_terminated_ = stop;
    }

private:
    std::shared_ptr<SessionParameters> default_session_params_;
    AppCreationCallback app_creation_callback_;
    int32_t packet_pool_size_{5192};
    bool allow_port_remaps_{false};
    bool stop_on_last_session_terminated_{false};
};

} // namespace sanctuary::soe_protocol::objects
