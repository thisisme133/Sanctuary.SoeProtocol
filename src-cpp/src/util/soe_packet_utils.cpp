#include "sanctuary/soe_protocol/util/soe_packet_utils.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge_all.hpp"
#include "sanctuary/soe_protocol/objects/packets/disconnect.hpp"
#include "sanctuary/soe_protocol/objects/packets/remap_connection.hpp"
#include "sanctuary/soe_protocol/objects/packets/session_request.hpp"
#include "sanctuary/soe_protocol/objects/packets/session_response.hpp"
#include "sanctuary/soe_protocol/objects/packets/unknown_sender.hpp"
#include <stdexcept>
#include <zlib.h>

// Forward declaration of Crc32 hash method (to be implemented in services)
namespace sanctuary::soe_protocol::services {
    class Crc32 {
    public:
        virtual ~Crc32() = default;
        [[nodiscard]] virtual uint32_t hash(std::span<const uint8_t> data) const = 0;
    };
}

namespace sanctuary::soe_protocol::util {

namespace {
    // Helper function to read uint16 in big endian
    [[nodiscard]] uint16_t read_uint16_be(std::span<const uint8_t> buffer) {
        return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
    }

    // Helper function to read uint32 in big endian
    [[nodiscard]] uint32_t read_uint32_be(std::span<const uint8_t> buffer) {
        return (static_cast<uint32_t>(buffer[0]) << 24) |
               (static_cast<uint32_t>(buffer[1]) << 16) |
               (static_cast<uint32_t>(buffer[2]) << 8) |
               buffer[3];
    }

    // Helper function to read uint24 in big endian
    [[nodiscard]] uint32_t read_uint24_be(std::span<const uint8_t> buffer) {
        return (static_cast<uint32_t>(buffer[0]) << 16) |
               (static_cast<uint32_t>(buffer[1]) << 8) |
               buffer[2];
    }
}

SoeOpCode SoePacketUtils::read_soe_op_code(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(uint16_t)) {
        return SoeOpCode::Invalid;
    }

    return static_cast<SoeOpCode>(read_uint16_be(buffer));
}

size_t SoePacketUtils::append_crc(
    std::vector<uint8_t>& buffer,
    const std::shared_ptr<sanctuary::soe_protocol::services::Crc32>& crc_state,
    uint8_t crc_length) {

    if (crc_length == 0 || !crc_state) {
        return buffer.size();
    }

    // Calculate CRC over the entire buffer
    uint32_t crc_value = crc_state->hash(buffer);

    // Append the CRC bytes
    size_t original_size = buffer.size();
    buffer.resize(original_size + crc_length);

    switch (crc_length) {
        case 1:
            buffer[original_size] = static_cast<uint8_t>(crc_value);
            break;
        case 2:
            buffer[original_size] = static_cast<uint8_t>((crc_value >> 8) & 0xFF);
            buffer[original_size + 1] = static_cast<uint8_t>(crc_value & 0xFF);
            break;
        case 3:
            buffer[original_size] = static_cast<uint8_t>((crc_value >> 16) & 0xFF);
            buffer[original_size + 1] = static_cast<uint8_t>((crc_value >> 8) & 0xFF);
            buffer[original_size + 2] = static_cast<uint8_t>(crc_value & 0xFF);
            break;
        default: // 4 or more
            buffer[original_size] = static_cast<uint8_t>((crc_value >> 24) & 0xFF);
            buffer[original_size + 1] = static_cast<uint8_t>((crc_value >> 16) & 0xFF);
            buffer[original_size + 2] = static_cast<uint8_t>((crc_value >> 8) & 0xFF);
            buffer[original_size + 3] = static_cast<uint8_t>(crc_value & 0xFF);
            break;
    }

    return buffer.size();
}

sanctuary::soe_protocol::objects::SoePacketValidationResult SoePacketUtils::validate_packet(
    std::span<const uint8_t> packet_data,
    const sanctuary::soe_protocol::objects::SessionParameters& session_params,
    SoeOpCode& op_code) {

    using namespace sanctuary::soe_protocol::objects;

    op_code = SoeOpCode::Invalid;

    if (packet_data.size() < sizeof(uint16_t)) {
        return SoePacketValidationResult::TooShort;
    }

    op_code = read_soe_op_code(packet_data);
    if (!is_contextless_packet(op_code) && !is_contextual_packet(op_code)) {
        return SoePacketValidationResult::InvalidOpCode;
    }

    size_t minimum_length = get_packet_minimum_length(
        op_code,
        session_params.is_compression_enabled(),
        session_params.crc_length());

    if (minimum_length > packet_data.size()) {
        return SoePacketValidationResult::TooShort;
    }

    if (is_contextless_packet(op_code) || session_params.crc_length() == 0) {
        return SoePacketValidationResult::Valid;
    }

    // Validate CRC
    const uint8_t crc_length = session_params.crc_length();
    auto data_without_crc = packet_data.subspan(0, packet_data.size() - crc_length);
    uint32_t actual_crc = session_params.crc_state()->hash(data_without_crc);
    bool crc_match = false;

    switch (crc_length) {
        case 1: {
            uint8_t crc = packet_data[packet_data.size() - 1];
            crc_match = static_cast<uint8_t>(actual_crc) == crc;
            break;
        }
        case 2: {
            uint16_t crc = read_uint16_be(packet_data.subspan(packet_data.size() - 2));
            crc_match = static_cast<uint16_t>(actual_crc) == crc;
            break;
        }
        case 3: {
            uint32_t crc = read_uint24_be(packet_data.subspan(packet_data.size() - 3));
            crc_match = (actual_crc & 0x00FFFFFF) == crc;
            break;
        }
        case 4: {
            uint32_t crc = read_uint32_be(packet_data.subspan(packet_data.size() - 4));
            crc_match = actual_crc == crc;
            break;
        }
    }

    return crc_match
        ? SoePacketValidationResult::Valid
        : SoePacketValidationResult::CrcMismatch;
}

size_t SoePacketUtils::get_packet_minimum_length(
    SoeOpCode op_code,
    bool is_compression_enabled,
    uint8_t crc_length) {

    using namespace sanctuary::soe_protocol::objects::packets;

    switch (op_code) {
        case SoeOpCode::SessionRequest:
            return SessionRequest::MIN_SIZE;
        case SoeOpCode::SessionResponse:
            return SessionResponse::SIZE;
        case SoeOpCode::MultiPacket:
            // Data length + first byte of data
            return get_contextual_packet_padding(is_compression_enabled, crc_length) + 2;
        case SoeOpCode::Disconnect:
            return get_contextual_packet_padding(is_compression_enabled, crc_length) + Disconnect::SIZE;
        case SoeOpCode::Heartbeat:
            return get_contextual_packet_padding(is_compression_enabled, crc_length);
        case SoeOpCode::NetStatusRequest:
            return get_contextual_packet_padding(is_compression_enabled, crc_length);
        case SoeOpCode::NetStatusResponse:
            return get_contextual_packet_padding(is_compression_enabled, crc_length);
        case SoeOpCode::ReliableData:
        case SoeOpCode::ReliableDataFragment:
            // Sequence + first byte of data
            return get_contextual_packet_padding(is_compression_enabled, crc_length)
                + sizeof(uint16_t) + 1;
        case SoeOpCode::Acknowledge:
            return get_contextual_packet_padding(is_compression_enabled, crc_length) + Acknowledge::SIZE;
        case SoeOpCode::AcknowledgeAll:
            return get_contextual_packet_padding(is_compression_enabled, crc_length) + AcknowledgeAll::SIZE;
        case SoeOpCode::UnknownSender:
            return UnknownSender::SIZE;
        case SoeOpCode::RemapConnection:
            return RemapConnection::SIZE;
        default:
            throw std::invalid_argument("Invalid OP code");
    }
}

std::vector<uint8_t> SoePacketUtils::decompress(std::span<const uint8_t> input) {
    std::vector<uint8_t> output;

    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(input.data());
    stream.avail_in = static_cast<uInt>(input.size());

    // Initialize zlib for decompression (using zlib header)
    if (inflateInit(&stream) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib decompression");
    }

    // Decompress in chunks
    constexpr size_t CHUNK_SIZE = 16384; // 16KB chunks
    std::vector<uint8_t> temp_buffer(CHUNK_SIZE);

    int ret;
    do {
        stream.next_out = temp_buffer.data();
        stream.avail_out = static_cast<uInt>(temp_buffer.size());

        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&stream);
            throw std::runtime_error("Zlib decompression error");
        }

        size_t have = temp_buffer.size() - stream.avail_out;
        output.insert(output.end(), temp_buffer.begin(), temp_buffer.begin() + have);

    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    return output;
}

} // namespace sanctuary::soe_protocol::util
