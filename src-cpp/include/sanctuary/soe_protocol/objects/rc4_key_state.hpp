#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Contains a state that can be used with the Rc4Cipher class.
/// </summary>
class Rc4KeyState {
public:
    /// <summary>
    /// Gets the number of bytes to use for the RC4 key state.
    /// </summary>
    static constexpr size_t LENGTH = 256;

    /// <summary>
    /// Gets or sets the first RC4 transform index.
    /// </summary>
    int32_t index1{0};

    /// <summary>
    /// Gets or sets the second RC4 transform index.
    /// </summary>
    int32_t index2{0};

    /// <summary>
    /// Initializes a new instance of the Rc4KeyState class.
    /// </summary>
    /// <param name="key_bytes">The key bytes to initialize this state with.</param>
    explicit Rc4KeyState(std::span<const uint8_t> key_bytes);

    /// <summary>
    /// Initializes a new instance of the Rc4KeyState class
    /// by copying an existing state.
    /// </summary>
    /// <param name="existing_state">The state to copy.</param>
    Rc4KeyState(const Rc4KeyState& existing_state);

    Rc4KeyState(Rc4KeyState&&) noexcept = default;
    Rc4KeyState& operator=(const Rc4KeyState&);
    Rc4KeyState& operator=(Rc4KeyState&&) noexcept = default;
    ~Rc4KeyState() = default;

    /// <summary>
    /// Gets the RC4 key state as a mutable span.
    /// </summary>
    [[nodiscard]] std::span<uint8_t> mutable_key_state() noexcept {
        return state_;
    }

    /// <summary>
    /// Gets the RC4 key state as a const span.
    /// </summary>
    [[nodiscard]] std::span<const uint8_t> key_state() const noexcept {
        return state_;
    }

    /// <summary>
    /// Copies this Rc4KeyState instance.
    /// </summary>
    /// <returns>The copied object.</returns>
    [[nodiscard]] std::unique_ptr<Rc4KeyState> copy() const {
        return std::make_unique<Rc4KeyState>(*this);
    }

private:
    std::array<uint8_t, LENGTH> state_;
};

} // namespace sanctuary::soe_protocol::objects
