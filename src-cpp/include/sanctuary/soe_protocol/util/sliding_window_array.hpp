#pragma once

#include <cstdint>
#include <vector>

namespace sanctuary::soe_protocol::util {

/// <summary>
/// Implements a sliding-window array, which indexes its items relative
/// to the current position of the window on the underlying array.
/// </summary>
/// <typeparam name="T">The underlying type of the window array.</typeparam>
template<typename T>
class SlidingWindowArray {
public:
    /// <summary>
    /// Initializes a new instance of the SlidingWindowArray class.
    /// </summary>
    /// <param name="window_length">The length of the underlying array to use.</param>
    explicit SlidingWindowArray(size_t window_length)
        : array_(window_length)
        , window_start_(0) {
    }

    /// <summary>
    /// Initializes a new instance of the SlidingWindowArray class.
    /// </summary>
    /// <param name="array">The underlying array to wrap.</param>
    explicit SlidingWindowArray(std::vector<T> array)
        : array_(std::move(array))
        , window_start_(0) {
    }

    ~SlidingWindowArray() = default;
    SlidingWindowArray(const SlidingWindowArray&) = default;
    SlidingWindowArray(SlidingWindowArray&&) noexcept = default;
    SlidingWindowArray& operator=(const SlidingWindowArray&) = default;
    SlidingWindowArray& operator=(SlidingWindowArray&&) noexcept = default;

    /// <summary>
    /// Gets an item at the given index.
    /// </summary>
    /// <param name="index">The index, relative to the current window.</param>
    /// <returns>The element at the given index.</returns>
    [[nodiscard]] T& operator[](int index) {
        return array_[translate_window_offset_to_array_index(index)];
    }

    /// <summary>
    /// Gets an item at the given index.
    /// </summary>
    /// <param name="index">The index, relative to the current window.</param>
    /// <returns>The element at the given index.</returns>
    [[nodiscard]] const T& operator[](int index) const {
        return array_[translate_window_offset_to_array_index(index)];
    }

    /// <summary>
    /// Gets an item at the given index.
    /// </summary>
    /// <param name="index">The index, relative to the current window.</param>
    /// <returns>The element at the given index.</returns>
    [[nodiscard]] T& operator[](int64_t index) {
        return array_[translate_window_offset_to_array_index(index)];
    }

    /// <summary>
    /// Gets an item at the given index.
    /// </summary>
    /// <param name="index">The index, relative to the current window.</param>
    /// <returns>The element at the given index.</returns>
    [[nodiscard]] const T& operator[](int64_t index) const {
        return array_[translate_window_offset_to_array_index(index)];
    }

    /// <summary>
    /// Gets the length of the underlying array.
    /// </summary>
    [[nodiscard]] size_t length() const noexcept {
        return array_.size();
    }

    /// <summary>
    /// Gets the item currently exposed by the window.
    /// </summary>
    [[nodiscard]] T& current() {
        return (*this)[0];
    }

    /// <summary>
    /// Gets the item currently exposed by the window.
    /// </summary>
    [[nodiscard]] const T& current() const {
        return (*this)[0];
    }

    /// <summary>
    /// Shifts the start of the window.
    /// </summary>
    /// <param name="offset">The amount to shift the window by.</param>
    void slide(int offset = 1) {
        window_start_ = translate_window_offset_to_array_index(offset);
    }

    /// <summary>
    /// Gets a reference to the underlying array.
    /// </summary>
    /// <param name="current_window_start_index">The index within the array at which the current window starts.</param>
    /// <returns>The underlying array.</returns>
    [[nodiscard]] std::vector<T>& get_underlying_array(int64_t& current_window_start_index) {
        current_window_start_index = window_start_;
        return array_;
    }

    /// <summary>
    /// Gets a const reference to the underlying array.
    /// </summary>
    /// <param name="current_window_start_index">The index within the array at which the current window starts.</param>
    /// <returns>The underlying array.</returns>
    [[nodiscard]] const std::vector<T>& get_underlying_array(int64_t& current_window_start_index) const {
        current_window_start_index = window_start_;
        return array_;
    }

private:
    std::vector<T> array_;
    /// <summary>
    /// Gets the index into the array at which the current window starts.
    /// </summary>
    int64_t window_start_;

    /// <summary>
    /// Translates an offset from the current window position
    /// to an index in the underlying array.
    /// </summary>
    /// <param name="offset">The offset from the current window position.</param>
    /// <returns>The representative index in the underlying array.</returns>
    [[nodiscard]] int64_t translate_window_offset_to_array_index(int64_t offset) const {
        int64_t index = window_start_ + offset;
        const int64_t length = static_cast<int64_t>(array_.size());

        if (index < 0) {
            return length + (index % length);
        }

        if (index >= length) {
            return index % length;
        }

        return index;
    }
};

} // namespace sanctuary::soe_protocol::util
