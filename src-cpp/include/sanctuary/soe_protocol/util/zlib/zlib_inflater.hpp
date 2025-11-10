#pragma once

#include "zlib_types.hpp"
#include "zlib_exception.hpp"
#include <cstddef>
#include <span>
#include <zlib.h>

namespace sanctuary::soe_protocol::util::zlib
{
    /// <summary>
    /// A RAII wrapper around the zlib inflate API.
    /// </summary>
    class zlib_inflater
    {
    public:
        /// <summary>
        /// Initializes a new instance of the zlib_inflater class.
        /// </summary>
        /// <param name="zlib_header_present">Whether the zlib header is present in the input.</param>
        /// <exception cref="zlib_exception">Thrown if initialization fails.</exception>
        explicit zlib_inflater(bool zlib_header_present = true);

        /// <summary>
        /// Destructor. Cleans up the zlib stream.
        /// </summary>
        ~zlib_inflater();

        // Disable copying
        zlib_inflater(const zlib_inflater&) = delete;
        zlib_inflater& operator=(const zlib_inflater&) = delete;

        // Enable moving
        zlib_inflater(zlib_inflater&& other) noexcept;
        zlib_inflater& operator=(zlib_inflater&& other) noexcept;

        /// <summary>
        /// Inflates a buffer.
        /// </summary>
        /// <param name="input">The input buffer containing deflated data.</param>
        /// <param name="output">The buffer to write the inflated data to.</param>
        /// <returns>The number of bytes that were written to the output.</returns>
        /// <exception cref="zlib_exception">Thrown if inflation fails.</exception>
        [[nodiscard]] std::size_t inflate(
            std::span<const std::byte> input,
            std::span<std::byte> output
        );

        /// <summary>
        /// Resets the internal state of the inflater.
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
        int selected_window_bits_;
    };

} // namespace sanctuary::soe_protocol::util::zlib
