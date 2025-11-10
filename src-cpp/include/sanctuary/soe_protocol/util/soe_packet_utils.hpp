#pragma once

#include "sanctuary/soe_protocol/objects/session_parameters.hpp"
#include "sanctuary/soe_protocol/objects/soe_packet_validation_result.hpp"
#include "sanctuary/soe_protocol/soe_op_code.hpp"
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

// Forward declarations
namespace sanctuary::soe_protocol::services {
    class Crc32;
}

namespace sanctuary::soe_protocol::util {

/// <summary>
/// Contains utility methods for working with SOE protocol packets.
/// </summary>
class SoePacketUtils {
public:
    /// <summary>
    /// Reads a SoeOpCode from a buffer.
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    /// <returns>The protocol OP code.</returns>
    [[nodiscard]] static SoeOpCode read_soe_op_code(std::span<const uint8_t> buffer);

    /// <summary>
    /// Gets a value indicating whether the given OP code represents a
    /// packet that is used outside the context of a session.
    /// </summary>
    /// <param name="op_code">The OP code.</param>
    /// <returns>True if the packet is session-less.</returns>
    [[nodiscard]] static constexpr bool is_contextless_packet(SoeOpCode op_code) noexcept;

    /// <summary>
    /// Gets a valid indicating whether the given OP code represents a
    /// packet that must be used within the context of a session.
    /// </summary>
    /// <param name="op_code">The OP code.</param>
    /// <returns>True if the packet requires a session.</returns>
    [[nodiscard]] static constexpr bool is_contextual_packet(SoeOpCode op_code) noexcept;

    /// <summary>
    /// Appends a CRC check value to the given buffer.
    /// </summary>
    /// <param name="buffer">The buffer to append to.</param>
    /// <param name="crc_state">The CRC state to use.</param>
    /// <param name="crc_length">The number of bytes to store the CRC check value in.</param>
    /// <returns>The new size of the buffer after appending CRC.</returns>
    static size_t append_crc(
        std::vector<uint8_t>& buffer,
        const std::shared_ptr<sanctuary::soe_protocol::services::Crc32>& crc_state,
        uint8_t crc_length);

    /// <summary>
    /// Validates that a buffer 'most likely' contains an SOE protocol packet.
    /// </summary>
    /// <param name="packet_data">The buffer to validate.</param>
    /// <param name="session_params">The current session parameters.</param>
    /// <param name="op_code">The OP code of the packet, if valid.</param>
    /// <returns>The result of the validation.</returns>
    [[nodiscard]] static sanctuary::soe_protocol::objects::SoePacketValidationResult validate_packet(
        std::span<const uint8_t> packet_data,
        const sanctuary::soe_protocol::objects::SessionParameters& session_params,
        SoeOpCode& op_code);

    /// <summary>
    /// Gets the minimum length that a packet may be, given its OP code.
    /// </summary>
    /// <param name="op_code">The OP code.</param>
    /// <param name="is_compression_enabled">Whether compression is enabled.</param>
    /// <param name="crc_length">The CRC length of the session.</param>
    /// <returns>The minimum length.</returns>
    [[nodiscard]] static size_t get_packet_minimum_length(
        SoeOpCode op_code,
        bool is_compression_enabled,
        uint8_t crc_length);

    /// <summary>
    /// Decompresses a ZLIB-compressed buffer.
    /// </summary>
    /// <param name="input">The compressed input buffer.</param>
    /// <returns>A vector containing the decompressed data.</returns>
    [[nodiscard]] static std::vector<uint8_t> decompress(std::span<const uint8_t> input);

private:
    [[nodiscard]] static constexpr size_t get_contextual_packet_padding(
        bool is_compression_enabled,
        uint8_t crc_length) noexcept;
};

// Inline implementations

constexpr bool SoePacketUtils::is_contextless_packet(SoeOpCode op_code) noexcept {
    return op_code == SoeOpCode::SessionRequest
        || op_code == SoeOpCode::SessionResponse
        || op_code == SoeOpCode::UnknownSender
        || op_code == SoeOpCode::RemapConnection;
}

constexpr bool SoePacketUtils::is_contextual_packet(SoeOpCode op_code) noexcept {
    return op_code == SoeOpCode::MultiPacket
        || op_code == SoeOpCode::Disconnect
        || op_code == SoeOpCode::Heartbeat
        || op_code == SoeOpCode::NetStatusRequest
        || op_code == SoeOpCode::NetStatusResponse
        || op_code == SoeOpCode::ReliableData
        || op_code == SoeOpCode::ReliableDataFragment
        || op_code == SoeOpCode::Acknowledge
        || op_code == SoeOpCode::AcknowledgeAll;
}

constexpr size_t SoePacketUtils::get_contextual_packet_padding(
    bool is_compression_enabled,
    uint8_t crc_length) noexcept {
    return sizeof(uint16_t)  // SoeOpCode
        + (is_compression_enabled ? 1 : 0)
        + crc_length;
}

} // namespace sanctuary::soe_protocol::util
