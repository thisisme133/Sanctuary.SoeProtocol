#pragma once

#include "zlib_types.hpp"
#include <exception>
#include <string>

namespace sanctuary::soe_protocol::util::zlib
{
    /// <summary>
    /// Thrown when a zlib function returns a non-recoverable error code.
    /// </summary>
    class zlib_exception : public std::exception
    {
    public:
        /// <summary>
        /// Initializes a new instance of the zlib_exception class.
        /// </summary>
        /// <param name="error">The error code returned by zlib.</param>
        /// <param name="message">The message associated with the error.</param>
        explicit zlib_exception(zlib_error_code error, const std::string& message = "");

        /// <summary>
        /// Gets the error code returned by zlib.
        /// </summary>
        [[nodiscard]] zlib_error_code error_code() const noexcept { return error_code_; }

        /// <summary>
        /// Gets the error message.
        /// </summary>
        [[nodiscard]] const char* what() const noexcept override { return message_.c_str(); }

    private:
        zlib_error_code error_code_;
        std::string message_;
    };

} // namespace sanctuary::soe_protocol::util::zlib
