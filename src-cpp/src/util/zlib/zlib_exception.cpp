#include "sanctuary/soe_protocol/util/zlib/zlib_exception.hpp"

namespace sanctuary::soe_protocol::util::zlib
{
    zlib_exception::zlib_exception(zlib_error_code error, const std::string& message)
        : error_code_(error)
        , message_(message.empty() ? "Zlib error" : message)
    {
    }

} // namespace sanctuary::soe_protocol::util::zlib
