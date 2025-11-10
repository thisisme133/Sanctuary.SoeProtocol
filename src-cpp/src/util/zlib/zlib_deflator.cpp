#include "sanctuary/soe_protocol/util/zlib/zlib_deflator.hpp"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace sanctuary::soe_protocol::util::zlib
{
    zlib_deflator::zlib_deflator(zlib_compression_level level, bool include_zlib_header)
        : stream_(new z_stream{})
        , selected_level_(level)
        , include_zlib_header_(include_zlib_header)
        , selected_window_bits_(include_zlib_header
            ? zlib_constants::zlib_default_window_bits
            : zlib_constants::deflate_default_window_bits)
    {
        // Initialize the stream to zero
        std::memset(stream_, 0, sizeof(z_stream));

        // Initialize the deflate stream
        int result = deflateInit2(
            stream_,
            static_cast<int>(level),
            static_cast<int>(zlib_compression_method::deflated),
            selected_window_bits_,
            zlib_constants::deflate_default_mem_level,
            static_cast<int>(zlib_compression_strategy::default_strategy)
        );

        if (result != Z_OK)
        {
            delete stream_;
            stream_ = nullptr;
            generate_compression_error(result, "Failed to initialize deflator");
        }
    }

    zlib_deflator::~zlib_deflator()
    {
        if (stream_ != nullptr)
        {
            deflateEnd(stream_);
            delete stream_;
            stream_ = nullptr;
        }
    }

    zlib_deflator::zlib_deflator(zlib_deflator&& other) noexcept
        : stream_(other.stream_)
        , selected_level_(other.selected_level_)
        , include_zlib_header_(other.include_zlib_header_)
        , selected_window_bits_(other.selected_window_bits_)
    {
        other.stream_ = nullptr;
    }

    zlib_deflator& zlib_deflator::operator=(zlib_deflator&& other) noexcept
    {
        if (this != &other)
        {
            if (stream_ != nullptr)
            {
                deflateEnd(stream_);
                delete stream_;
            }

            stream_ = other.stream_;
            selected_level_ = other.selected_level_;
            include_zlib_header_ = other.include_zlib_header_;
            selected_window_bits_ = other.selected_window_bits_;

            other.stream_ = nullptr;
        }
        return *this;
    }

    std::size_t zlib_deflator::deflate(
        std::span<const std::byte> input,
        std::span<std::byte> output,
        zlib_flush_code flush_method
    )
    {
        check_not_disposed();

        // Set up input
        stream_->next_in = reinterpret_cast<Bytef*>(const_cast<std::byte*>(input.data()));
        stream_->avail_in = static_cast<uInt>(input.size());

        // Set up output
        stream_->next_out = reinterpret_cast<Bytef*>(output.data());
        stream_->avail_out = static_cast<uInt>(output.size());

        // Perform deflation
        int result = ::deflate(stream_, static_cast<int>(flush_method));

        if (result != Z_STREAM_END)
        {
            generate_compression_error(result, "Failed to deflate");
        }

        // Calculate bytes written
        std::size_t bytes_written = output.size() - stream_->avail_out;

        // Add header length if we're including the zlib header
        if (include_zlib_header_)
        {
            bytes_written += zlib_constants::header_length;
        }

        return bytes_written;
    }

    void zlib_deflator::reset()
    {
        check_not_disposed();

        // Reset stream pointers
        stream_->next_in = nullptr;
        stream_->avail_in = 0;
        stream_->next_out = nullptr;
        stream_->avail_out = 0;

        // End the current deflate stream
        int result = deflateEnd(stream_);
        if (result != Z_OK)
        {
            generate_compression_error(result, "Failed to end deflate");
        }

        // Re-initialize the stream to zero
        std::memset(stream_, 0, sizeof(z_stream));

        // Re-initialize the deflate stream
        result = deflateInit2(
            stream_,
            static_cast<int>(selected_level_),
            static_cast<int>(zlib_compression_method::deflated),
            selected_window_bits_,
            zlib_constants::deflate_default_mem_level,
            static_cast<int>(zlib_compression_strategy::default_strategy)
        );

        if (result != Z_OK)
        {
            generate_compression_error(result, "Failed to re-init the deflator");
        }
    }

    void zlib_deflator::check_not_disposed() const
    {
        if (is_disposed())
        {
            throw std::runtime_error("zlib_deflator has been disposed");
        }
    }

    void zlib_deflator::generate_compression_error(int result, const char* generic_message) const
    {
        std::string message;

        if (stream_ != nullptr && stream_->msg != nullptr)
        {
            message = stream_->msg;
        }
        else
        {
            message = generic_message;
        }

        throw zlib_exception(static_cast<zlib_error_code>(result), message);
    }

} // namespace sanctuary::soe_protocol::util::zlib
