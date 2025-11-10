#include "sanctuary/soe_protocol/util/native_span_pool.hpp"
#include <stdexcept>

namespace sanctuary::soe_protocol::util {

NativeSpanPool::NativeSpanPool(size_t memory_size, size_t pool_size)
    : memory_size_(memory_size)
    , pool_size_(pool_size)
    , pool_()
    , mutex_() {
}

std::unique_ptr<sanctuary::soe_protocol::objects::NativeSpan> NativeSpanPool::rent() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::unique_ptr<sanctuary::soe_protocol::objects::NativeSpan> retrieved;

    if (!pool_.empty()) {
        retrieved = std::move(pool_.top());
        pool_.pop();
    } else {
        retrieved = std::make_unique<sanctuary::soe_protocol::objects::NativeSpan>(memory_size_);
    }

    if (retrieved->used_length() > 0 || retrieved->start_offset() > 0) {
        throw std::runtime_error("Span has been used after return to pool");
    }

    return retrieved;
}

void NativeSpanPool::return_span(std::unique_ptr<sanctuary::soe_protocol::objects::NativeSpan> span) {
    if (!span) {
        throw std::invalid_argument("span cannot be null");
    }

    if (span->full_span().size() != memory_size_) {
        throw std::invalid_argument("The NativeSpan was not rented from this pool");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (pool_.size() >= pool_size_) {
        // Let the span be destroyed (freed) when it goes out of scope
        return;
    }

    span->set_start_offset(0);
    span->set_used_length(0);
    pool_.push(std::move(span));
}

} // namespace sanctuary::soe_protocol::util
