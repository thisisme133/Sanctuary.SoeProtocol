#include "sanctuary/soe_protocol/util/zlib/zlib_inflater.hpp"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace sanctuary::soe_protocol::util::zlib
{
    zlib_inflater::zlib_inflater(bool zlib_header_present)
        : stream_(new z_stream{})
        , selected_window_bits_(zlib_header_present
            ? zlib_constants::zlib_default_window_bits
            : zlib_constants::deflate_default_window_bits)
    {
        // Initialize the stream to zero
        std::memset(stream_, 0, sizeof(z_stream));

        // Initialize the inflate stream
        int result = inflateInit2(stream_, selected_window_bits_);

        if (result != Z_OK)
        {
            delete stream_;
            stream_ = nullptr;
            generate_compression_error(result, "Failed to initialize inflater");
        }
    }

    zlib_inflater::~zlib_inflater()
    {
        if (stream_ != nullptr)
        {
            inflateEnd(stream_);
            delete stream_;
            stream_ = nullptr;
        }
    }

    zlib_inflater::zlib_inflater(zlib_inflater&& other) noexcept
        : stream_(other.stream_)
        , selected_window_bits_(other.selected_window_bits_)
    {
        other.stream_ = nullptr;
    }

    zlib_inflater& zlib_inflater::operator=(zlib_inflater&& other) noexcept
    {
        if (this != &other)
        {
            if (stream_ != nullptr)
            {
                inflateEnd(stream_);
                delete stream_;
            }

            stream_ = other.stream_;
            selected_window_bits_ = other.selected_window_bits_;

            other.stream_ = nullptr;
        }
        return *this;
    }

    std::size_t zlib_inflater::inflate(
        std::span<const std::byte> input,
        std::span<std::byte> output
    )
    {
        check_not_disposed();

        // Set up input
        stream_->next_in = reinterpret_cast<Bytef*>(const_cast<std::byte*>(input.data()));
        stream_->avail_in = static_cast<uInt>(input.size());

        // Set up output
        stream_->next_out = reinterpret_cast<Bytef*>(output.data());
        stream_->avail_out = static_cast<uInt>(output.size());

        // Perform inflation
        int result = ::inflate(stream_, Z_FINISH);

        if (result != Z_STREAM_END)
        {
            generate_compression_error(result, "Failed to inflate");
        }

        // Calculate bytes written
        return output.size() - stream_->avail_out;
    }

    void zlib_inflater::reset()
    {
        check_not_disposed();

        // Reset stream pointers
        stream_->next_in = nullptr;
        stream_->avail_in = 0;
        stream_->next_out = nullptr;
        stream_->avail_out = 0;

        // End the current inflate stream
        int result = inflateEnd(stream_);
        if (result != Z_OK)
        {
            generate_compression_error(result, "Failed to end inflate");
        }

        // Re-initialize the stream to zero
        std::memset(stream_, 0, sizeof(z_stream));

        // Re-initialize the inflate stream
        result = inflateInit2(stream_, selected_window_bits_);

        if (result != Z_OK)
        {
            generate_compression_error(result, "Failed to re-init the inflater");
        }
    }

    void zlib_inflater::check_not_disposed() const
    {
        if (is_disposed())
        {
            throw std::runtime_error("zlib_inflater has been disposed");
        }
    }

    void zlib_inflater::generate_compression_error(int result, const char* generic_message) const
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
