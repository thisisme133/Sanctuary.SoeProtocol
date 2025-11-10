#include "sanctuary/soe_protocol/objects/native_span.hpp"
#include <algorithm>
#include <stdexcept>

namespace sanctuary::soe_protocol::objects {

NativeSpan::NativeSpan(size_t length)
    : array_(length)
    , start_offset_(0)
    , used_length_(0) {
}

void NativeSpan::copy_data_into(std::span<const uint8_t> data) {
    if (data.size() > array_.size()) {
        throw std::invalid_argument(
            "The provided data is too long to fit in the underlying native memory");
    }

    std::copy(data.begin(), data.end(), array_.begin());
    used_length_ = data.size();
}

std::unique_ptr<std::stringstream> NativeSpan::to_stream() const {
    auto stream = std::make_unique<std::stringstream>(
        std::ios_base::in | std::ios_base::out | std::ios_base::binary);

    auto used = used_span();
    stream->write(reinterpret_cast<const char*>(used.data()), used.size());
    stream->seekg(0);

    return stream;
}

} // namespace sanctuary::soe_protocol::objects
