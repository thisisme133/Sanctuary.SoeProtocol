#pragma once

#include "sanctuary/soe_protocol/objects/rc4_key_state.hpp"
#include <memory>
#include <optional>
#include <stdexcept>

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Contains parameters used by an application to control the underlying SOE session.
/// </summary>
class ApplicationParameters {
public:
    /// <summary>
    /// Initializes a new instance of the ApplicationParameters class.
    /// </summary>
    /// <param name="encryption_key_state">The initial encryption key state to use.</param>
    explicit ApplicationParameters(std::shared_ptr<Rc4KeyState> encryption_key_state = nullptr);

    ~ApplicationParameters() = default;
    ApplicationParameters(const ApplicationParameters&) = delete;
    ApplicationParameters(ApplicationParameters&&) noexcept = default;
    ApplicationParameters& operator=(const ApplicationParameters&) = delete;
    ApplicationParameters& operator=(ApplicationParameters&&) noexcept = default;

    /// <summary>
    /// Gets a value indicating whether encryption is enabled
    /// for the session.
    /// </summary>
    [[nodiscard]] bool is_encryption_enabled() const noexcept {
        return is_encryption_enabled_;
    }

    /// <summary>
    /// Sets a value indicating whether encryption is enabled
    /// for the session.
    /// </summary>
    /// <exception cref="std::invalid_argument">
    /// Thrown when attempting to enable encryption without a key state.
    /// </exception>
    void set_encryption_enabled(bool value);

    /// <summary>
    /// Gets the encryption key state to use with this session.
    /// </summary>
    [[nodiscard]] const std::shared_ptr<Rc4KeyState>& encryption_key_state() const noexcept {
        return encryption_key_state_;
    }

private:
    bool is_encryption_enabled_{false};
    std::shared_ptr<Rc4KeyState> encryption_key_state_;
};

} // namespace sanctuary::soe_protocol::objects
