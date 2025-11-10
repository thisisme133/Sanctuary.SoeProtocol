#pragma once

#include "sanctuary/soe_protocol/objects/native_span.hpp"
#include <cstdint>
#include <memory>
#include <mutex>
#include <stack>
#include <stdexcept>

namespace sanctuary::soe_protocol::util {

/// <summary>
/// Represents a fixed-limit pool of fixed-size NativeSpan objects.
/// </summary>
class NativeSpanPool {
public:
    /// <summary>
    /// Initializes a new instance of the NativeSpanPool class.
    /// </summary>
    /// <param name="memory_size">
    /// The size in bytes of each NativeSpan object to allocate within the pool.
    /// </param>
    /// <param name="pool_size">
    /// The maximum number of NativeSpan objects that may be held within the pool.
    /// </param>
    NativeSpanPool(size_t memory_size, size_t pool_size);

    ~NativeSpanPool() = default;
    NativeSpanPool(const NativeSpanPool&) = delete;
    NativeSpanPool(NativeSpanPool&&) = delete;
    NativeSpanPool& operator=(const NativeSpanPool&) = delete;
    NativeSpanPool& operator=(NativeSpanPool&&) = delete;

    /// <summary>
    /// Rents a NativeSpan object from the pool.
    /// Rented objects should be returned to the pool.
    /// </summary>
    /// <remarks>
    /// This method will allocate a new NativeSpan object if the pool is empty.
    /// </remarks>
    /// <returns>The rented object.</returns>
    [[nodiscard]] std::unique_ptr<sanctuary::soe_protocol::objects::NativeSpan> rent();

    /// <summary>
    /// Returns a rented NativeSpan object to the pool.
    /// </summary>
    /// <remarks>
    /// The underlying memory of the NativeSpan will be freed if the pool is full.
    /// </remarks>
    /// <param name="span">The NativeSpan to return.</param>
    /// <exception cref="std::invalid_argument">
    /// Thrown if the given NativeSpan is a different length to the memory size of the pool.
    /// </exception>
    void return_span(std::unique_ptr<sanctuary::soe_protocol::objects::NativeSpan> span);

private:
    size_t memory_size_;
    size_t pool_size_;
    std::stack<std::unique_ptr<sanctuary::soe_protocol::objects::NativeSpan>> pool_;
    std::mutex mutex_;
};

} // namespace sanctuary::soe_protocol::util
