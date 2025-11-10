#pragma once

#include <algorithm>
#include <concepts>
#include <span>
#include <utility>

namespace sanctuary::soe_protocol::extensions {

/// <summary>
/// Contains extension functions for std::span.
/// </summary>
class SpanExtensions {
public:
    /// <summary>
    /// Swaps the position of two elements in a span.
    /// </summary>
    /// <typeparam name="T">The type of elements in the span.</typeparam>
    /// <param name="data">The span to swap the elements in.</param>
    /// <param name="index1">The index of the first element.</param>
    /// <param name="index2">The index of the second element.</param>
    template<typename T>
    static constexpr void swap(std::span<T> data, size_t index1, size_t index2) noexcept {
        if (index1 >= data.size() || index2 >= data.size()) {
            return; // Out of bounds, do nothing
        }

        if (index1 == index2) {
            return; // Nothing to swap
        }

        T temp = std::move(data[index1]);
        data[index1] = std::move(data[index2]);
        data[index2] = std::move(temp);
    }

    /// <summary>
    /// Swaps the position of two elements in a span (using int indices for compatibility).
    /// </summary>
    /// <typeparam name="T">The type of elements in the span.</typeparam>
    /// <param name="data">The span to swap the elements in.</param>
    /// <param name="index1">The index of the first element.</param>
    /// <param name="index2">The index of the second element.</param>
    template<typename T>
    static constexpr void swap(std::span<T> data, int index1, int index2) noexcept {
        if (index1 < 0 || index2 < 0) {
            return; // Negative indices are invalid
        }
        swap(data, static_cast<size_t>(index1), static_cast<size_t>(index2));
    }

    /// <summary>
    /// Reverses the elements in a span.
    /// </summary>
    /// <typeparam name="T">The type of elements in the span.</typeparam>
    /// <param name="data">The span to reverse.</param>
    template<typename T>
    static constexpr void reverse(std::span<T> data) noexcept {
        std::reverse(data.begin(), data.end());
    }

    /// <summary>
    /// Fills a span with a specific value.
    /// </summary>
    /// <typeparam name="T">The type of elements in the span.</typeparam>
    /// <param name="data">The span to fill.</param>
    /// <param name="value">The value to fill with.</param>
    template<typename T>
    static constexpr void fill(std::span<T> data, const T& value) noexcept {
        std::fill(data.begin(), data.end(), value);
    }

    /// <summary>
    /// Copies elements from one span to another.
    /// </summary>
    /// <typeparam name="T">The type of elements in the span.</typeparam>
    /// <param name="source">The source span to copy from.</param>
    /// <param name="destination">The destination span to copy to.</param>
    /// <returns>The number of elements copied.</returns>
    template<typename T>
    static constexpr size_t copy(std::span<const T> source, std::span<T> destination) noexcept {
        size_t count = std::min(source.size(), destination.size());
        std::copy_n(source.begin(), count, destination.begin());
        return count;
    }
};

// Free function aliases for convenience
template<typename T>
inline constexpr void swap(std::span<T> data, size_t index1, size_t index2) noexcept {
    SpanExtensions::swap(data, index1, index2);
}

template<typename T>
inline constexpr void swap(std::span<T> data, int index1, int index2) noexcept {
    SpanExtensions::swap(data, index1, index2);
}

template<typename T>
inline constexpr void reverse(std::span<T> data) noexcept {
    SpanExtensions::reverse(data);
}

template<typename T>
inline constexpr void fill(std::span<T> data, const T& value) noexcept {
    SpanExtensions::fill(data, value);
}

template<typename T>
inline constexpr size_t copy(std::span<const T> source, std::span<T> destination) noexcept {
    return SpanExtensions::copy(source, destination);
}

} // namespace sanctuary::soe_protocol::extensions
