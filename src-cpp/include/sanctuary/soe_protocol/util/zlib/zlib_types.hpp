#pragma once

#include <cstdint>

namespace sanctuary::soe_protocol::util::zlib
{
    /// <summary>
    /// Contains constant values used to configure zlib.
    /// </summary>
    namespace zlib_constants
    {
        /// <summary>
        /// The length in bytes of the zlib header.
        /// </summary>
        inline constexpr int header_length = 2;

        /// <summary>
        /// <p><strong>From the ZLib manual:</strong></p>
        /// <p>ZLib's <code>windowBits</code> parameter is the base two logarithm of the window size (the size of the
        /// history buffer). It should be in the range 8..15 for this version of the library. Larger values of this
        /// parameter result in better compression at the expense of memory usage. The default value is 15 if deflateInit is
        /// used instead.<br /></p>
        /// <strong>Note</strong>:
        /// <code>windowBits</code> can also be -8..-15 for raw deflate (i.e. no zlib header is written). In this case,
        /// -windowBits determines the window size. <code>Deflate</code> will then generate raw deflate data with no ZLib
        /// header or trailer, and will not compute an adler32 check value.<br />
        /// <p>See also: How to choose a compression level (in comments to <code>CompressionLevel</code>).</p>
        /// </summary>
        inline constexpr int deflate_default_window_bits = -15;

        /// <summary>
        /// <p><strong>From the ZLib manual:</strong></p>
        /// <p>ZLib's <code>windowBits</code> parameter is the base two logarithm of the window size (the size of the
        /// history buffer). It should be in the range 8..15 for this version of the library. Larger values of this
        /// parameter result in better compression at the expense of memory usage. The default value is 15 if deflateInit is
        /// used instead.<br /></p>
        /// </summary>
        inline constexpr int zlib_default_window_bits = 15;

        /// <summary>
        /// <p><strong>From the ZLib manual:</strong></p>
        /// <p>The <code>memLevel</code> parameter specifies how much memory should be allocated for the internal
        /// compression state. <code>memLevel</code> = 1 uses minimum memory but is slow and reduces compression ratio;
        /// <code>memLevel</code> = 9 uses maximum memory for optimal speed. The default value is 8.</p>
        /// <p>See also: How to choose a compression level (in comments to <code>CompressionLevel</code>.)</p>
        /// </summary>
        inline constexpr int deflate_default_mem_level = 8;
    }

    /// <summary>
    /// Enumerates the possible flush methods that can be used with a deflate operation.
    /// </summary>
    enum class zlib_flush_code : int
    {
        /// <summary>
        /// Allows the algorithm to decide how much data to accumulate before producing output.
        /// This maximizes the compression level that is achieved.
        /// </summary>
        no_flush = 0,

        /// <summary>
        /// All pending output is flushed to the output buffer and aligned on a byte boundary.
        /// </summary>
        sync_flush = 2,

        /// <summary>
        /// Indicates that the input buffer contains the entire sequence to be deflated, allowing optimisations to be applied.
        /// </summary>
        finish = 4,

        /// <summary>
        /// If flush is set to Z_BLOCK, a deflate block is completed and emitted, as for Z_SYNC_FLUSH, but the output is not
        /// aligned on a byte boundary, and up to seven bits of the current block are held to be written as the next byte
        /// after the next deflate block is completed.  In this case, the decompressor may not be provided enough bits at
        /// this point in order to complete decompression of the data provided so far to the compressor.  It may need to
        /// wait for the next block to be emitted.  This is for advanced applications that need to control the emission of
        /// deflate blocks.
        /// </summary>
        block = 5
    };

    /// <summary>
    /// Enumerates the possible error codes of a zlib operation.
    /// </summary>
    enum class zlib_error_code : int
    {
        /// <summary>
        /// Deflate/Inflate: Progress has been made. Supply more input, or more output space.
        /// Otherwise: The operation completed successfully.
        /// </summary>
        ok = 0,

        /// <summary>
        /// All input has been consumed and all output produced successfully.
        /// </summary>
        stream_end = 1,

        /// <summary>
        /// The stream structure is inconsistent (e.g. next_in or next_out are null).
        /// </summary>
        stream_error = -2,

        /// <summary>
        /// Inflate: The input stream is corrupt (doesn't conform to the zlib format,
        /// or incorrect check value).
        /// </summary>
        data_error = -3,

        /// <summary>
        /// Not enough memory to complete the operation.
        /// </summary>
        mem_error = -4,

        /// <summary>
        /// No progress is possible or there is not enough room in the
        /// output buffer when finish flush is used.
        /// This error is not fatal, and inflation can continue with more input and/or more output space.
        /// </summary>
        buf_error = -5,

        /// <summary>
        /// The version of the underlying library is not the same as what the caller is expecting.
        /// </summary>
        version_error = -6
    };

    /// <summary>
    /// Used to tune the compression algorithm.
    /// </summary>
    enum class zlib_compression_strategy : int
    {
        /// <summary>
        /// For normal data.
        /// </summary>
        default_strategy = 0,

        /// <summary>
        /// For data produced by a filter (or predictor). Filtered data consists mostly of small values with a somewhat
        /// random distribution. In this case, the compression algorithm is tuned to compress them better. The effect of
        /// filtered is to force more Huffman coding and less string matching; it is somewhat intermediate
        /// between default_strategy and huffman_only.
        /// </summary>
        filtered = 1,

        /// <summary>
        /// Force Huffman encoding only (no string match).
        /// </summary>
        huffman_only = 2,

        /// <summary>
        /// Limit match distances to one (run-length encoding). RLE is designed to be almost as fast as
        /// huffman_only, but give better compression for PNG image data. The strategy parameter only affects
        /// the compression ratio but not the correctness of the compressed output even if it is not set appropriately.
        /// </summary>
        run_length_encoding = 3,

        /// <summary>
        /// Prevents the use of dynamic Huffman codes, allowing for a simpler decoder for special applications.
        /// </summary>
        fixed = 4
    };

    /// <summary>
    /// The compression method to use.
    /// </summary>
    enum class zlib_compression_method : int
    {
        /// <summary>
        /// Use the deflated algorithm.
        /// </summary>
        deflated = 8
    };

    /// <summary>
    /// ZLib can accept any integer value between 0 and 9 (inclusive) as a valid compression level parameter:
    /// 1 gives best speed, 9 gives best compression, 0 gives no compression at all (the input data is simply copied a block
    /// at a time). default_compression = -1 requests a default compromise between speed
    /// and compression (currently equivalent to level 6).
    /// </summary>
    /// <remarks>
    /// <para><strong>How to choose a compression level:</strong><br />
    /// The names no_compression, best_speed, default_compression,
    /// best_compression are taken over from the corresponding ZLib definitions, which map to our public
    /// NoCompression, Fastest, Optimal, and SmallestSize respectively.</para>
    /// <em>Optimal Compression:</em>
    /// <code>
    /// zlib_compression_level compression_level = zlib_compression_level::default_compression;
    /// int window_bits = 15;  // or -15 if no headers required
    /// int mem_level = 8;
    /// zlib_compression_strategy strategy = zlib_compression_strategy::default_strategy;
    /// </code>
    ///
    /// <em>Fastest compression:</em>
    /// <code>
    /// zlib_compression_level compression_level = zlib_compression_level::best_speed;
    /// int window_bits = 15;  // or -15 if no headers required
    /// int mem_level = 8;
    /// zlib_compression_strategy strategy = zlib_compression_strategy::default_strategy;
    /// </code>
    ///
    /// <em>No compression (even faster, useful for data that cannot be compressed such some image formats):</em>
    /// <code>
    /// zlib_compression_level compression_level = zlib_compression_level::no_compression;
    /// int window_bits = 15;  // or -15 if no headers required
    /// int mem_level = 7;
    /// zlib_compression_strategy strategy = zlib_compression_strategy::default_strategy;
    /// </code>
    ///
    /// <em>Smallest Size Compression:</em>
    /// <code>
    /// zlib_compression_level compression_level = zlib_compression_level::best_compression;
    /// int window_bits = 15;  // or -15 if no headers required
    /// int mem_level = 8;
    /// zlib_compression_strategy strategy = zlib_compression_strategy::default_strategy;
    /// </code>
    /// </remarks>
    enum class zlib_compression_level : int
    {
        /// <summary>
        /// No compression is used.
        /// </summary>
        no_compression = 0,

        /// <summary>
        /// Optimized for time taken to compress the input.
        /// </summary>
        best_speed = 1,

        /// <summary>
        /// A default compromise between speed and compression.
        /// </summary>
        default_compression = -1,

        /// <summary>
        /// Optimized for size of the output.
        /// </summary>
        best_compression = 9
    };

} // namespace sanctuary::soe_protocol::util::zlib
