#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Represents a wrapper around native memory.
/// </summary>
class NativeSpan {
public:
    /// <summary>
    /// Initializes a new instance of the NativeSpan class.
    /// </summary>
    /// <param name="length">The length of the array to allocate.</param>
    explicit NativeSpan(size_t length);

    ~NativeSpan() = default;
    NativeSpan(const NativeSpan&) = delete;
    NativeSpan(NativeSpan&&) noexcept = default;
    NativeSpan& operator=(const NativeSpan&) = delete;
    NativeSpan& operator=(NativeSpan&&) noexcept = default;

    /// <summary>
    /// Gets or sets the index into the full_span at which data begins.
    /// </summary>
    [[nodiscard]] size_t start_offset() const noexcept { return start_offset_; }
    void set_start_offset(size_t offset) noexcept { start_offset_ = offset; }

    /// <summary>
    /// Gets or sets the length of the full_span that is currently in use.
    /// </summary>
    [[nodiscard]] size_t used_length() const noexcept { return used_length_; }
    void set_used_length(size_t length) noexcept { used_length_ = length; }

    /// <summary>
    /// Gets a span around the underlying native memory.
    /// </summary>
    [[nodiscard]] std::span<uint8_t> full_span() noexcept {
        return array_;
    }

    /// <summary>
    /// Gets a const span around the underlying native memory.
    /// </summary>
    [[nodiscard]] std::span<const uint8_t> full_span() const noexcept {
        return array_;
    }

    /// <summary>
    /// Gets a span around the underlying native memory that
    /// is actually being used.
    /// </summary>
    [[nodiscard]] std::span<uint8_t> used_span() noexcept {
        return std::span<uint8_t>(array_.data() + start_offset_, used_length_);
    }

    /// <summary>
    /// Gets a const span around the underlying native memory that
    /// is actually being used.
    /// </summary>
    [[nodiscard]] std::span<const uint8_t> used_span() const noexcept {
        return std::span<const uint8_t>(array_.data() + start_offset_, used_length_);
    }

    /// <summary>
    /// Copies data into the underlying memory of the native span,
    /// and sets used_length correspondingly.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <exception cref="std::invalid_argument">
    /// Thrown if the data is too long to be stored in the underlying allocated memory.
    /// </exception>
    void copy_data_into(std::span<const uint8_t> data);

    /// <summary>
    /// Creates a stringstream over the used_span.
    /// </summary>
    /// <returns>A stringstream instance.</returns>
    [[nodiscard]] std::unique_ptr<std::stringstream> to_stream() const;

private:
    std::vector<uint8_t> array_;
    size_t start_offset_{0};
    size_t used_length_{0};
};

} // namespace sanctuary::soe_protocol::objects
