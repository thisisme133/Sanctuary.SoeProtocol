#pragma once

#include "zlib_types.hpp"
#include "zlib_exception.hpp"
#include <cstddef>
#include <span>
#include <zlib.h>

namespace sanctuary::soe_protocol::util::zlib
{
    /// <summary>
    /// A RAII wrapper around the zlib deflate API.
    /// </summary>
    class zlib_deflator
    {
    public:
        /// <summary>
        /// Initializes a new instance of the zlib_deflator class.
        /// </summary>
        /// <param name="level">The compression level to use.</param>
        /// <param name="include_zlib_header">Whether to write the zlib header to the output.</param>
        /// <exception cref="zlib_exception">Thrown if initialization fails.</exception>
        explicit zlib_deflator(zlib_compression_level level, bool include_zlib_header);

        /// <summary>
        /// Destructor. Cleans up the zlib stream.
        /// </summary>
        ~zlib_deflator();

        // Disable copying
        zlib_deflator(const zlib_deflator&) = delete;
        zlib_deflator& operator=(const zlib_deflator&) = delete;

        // Enable moving
        zlib_deflator(zlib_deflator&& other) noexcept;
        zlib_deflator& operator=(zlib_deflator&& other) noexcept;

        /// <summary>
        /// Deflates a buffer.
        /// </summary>
        /// <param name="input">The input buffer.</param>
        /// <param name="output">The buffer to output the deflated bytes to.</param>
        /// <param name="flush_method">The flush method to use.</param>
        /// <returns>The number of bytes that were written to the output.</returns>
        /// <exception cref="zlib_exception">Thrown if deflation fails.</exception>
        [[nodiscard]] std::size_t deflate(
            std::span<const std::byte> input,
            std::span<std::byte> output,
            zlib_flush_code flush_method = zlib_flush_code::finish
        );

        /// <summary>
        /// Resets the internal state of the deflator.
        /// </summary>
        /// <exception cref="zlib_exception">Thrown if reset fails.</exception>
        void reset();

        /// <summary>
        /// Gets a value indicating whether this instance has been disposed.
        /// </summary>
        [[nodiscard]] bool is_disposed() const noexcept { return stream_ == nullptr; }

    private:
        void check_not_disposed() const;
        [[noreturn]] void generate_compression_error(int result, const char* generic_message) const;

        z_stream* stream_;
        zlib_compression_level selected_level_;
        bool include_zlib_header_;
        int selected_window_bits_;
    };

} // namespace sanctuary::soe_protocol::util::zlib
