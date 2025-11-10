#include <gtest/gtest.h>
#include "Objects/SessionParameters.h"
#include "Objects/SoeOpCode.h"
#include "Objects/Packets/AcknowledgeAll.h"
#include "Services/Crc32.h"
#include "Util/SoePacketUtils.h"
#include <vector>
#include <cstdint>
#include <span>

using namespace Sanctuary::SoeProtocol::Objects;
using namespace Sanctuary::SoeProtocol::Objects::Packets;
using namespace Sanctuary::SoeProtocol::Services;
using namespace Sanctuary::SoeProtocol::Util;

class SoePacketUtilsTests : public ::testing::Test
{
protected:
    static SessionParameters GetSessionParams(uint8_t crcLength = 0)
    {
        SessionParameters params;
        params.ApplicationProtocol = "TestProtocol";
        params.IsCompressionEnabled = false;
        params.CrcSeed = 5;
        params.CrcLength = crcLength;
        return params;
    }
};

class AppendCrcTest : public SoePacketUtilsTests, public ::testing::WithParamInterface<uint8_t> {};

TEST_P(AppendCrcTest, AppendCrc_Correct_ForAllValidLengths)
{
    uint8_t crcLength = GetParam();
    Crc32 crc(5);

    std::vector<uint8_t> buffer(4 + crcLength);
    size_t offset = 0;

    // Write test value
    buffer[0] = 0x1B;
    buffer[1] = 0x18;
    buffer[2] = 0x15;
    buffer[3] = 0x34;
    offset = 4;

    uint32_t expectedCrc = crc.Hash(std::span<const uint8_t>(buffer.data(), 4));
    std::vector<uint8_t> expectedBuffer(4);

    // Big-endian write of CRC
    expectedBuffer[0] = static_cast<uint8_t>((expectedCrc >> 24) & 0xFF);
    expectedBuffer[1] = static_cast<uint8_t>((expectedCrc >> 16) & 0xFF);
    expectedBuffer[2] = static_cast<uint8_t>((expectedCrc >> 8) & 0xFF);
    expectedBuffer[3] = static_cast<uint8_t>(expectedCrc & 0xFF);

    SoePacketUtils::AppendCrc(buffer, offset, crc, crcLength);

    for (int i = 0; i < crcLength; i++)
        EXPECT_EQ(expectedBuffer[4 - crcLength + i], buffer[4 + i]);
}

INSTANTIATE_TEST_SUITE_P(
    CrcLengths,
    AppendCrcTest,
    ::testing::Values(0, 1, 2, 3, 4)
);

TEST_F(SoePacketUtilsTests, ValidatePacket_InvalidatesPacket_WithShortOpCode)
{
    std::vector<uint8_t> packet = { static_cast<uint8_t>(SoeOpCode::SessionRequest) };
    SoeOpCode outOpCode;
    SoePacketValidationResult result = SoePacketUtils::ValidatePacket(packet, GetSessionParams(), outOpCode);
    EXPECT_EQ(SoePacketValidationResult::TooShort, result);
}

class ValidatePacketInvalidOpCodeTest : public SoePacketUtilsTests, public ::testing::WithParamInterface<uint8_t> {};

TEST_P(ValidatePacketInvalidOpCodeTest, ValidatePacket_InvalidatesPacket_WithInvalidOpCode)
{
    uint8_t opCode = GetParam();
    std::vector<uint8_t> packet = { 0, opCode };
    SoeOpCode outOpCode;
    SoePacketValidationResult result = SoePacketUtils::ValidatePacket(packet, GetSessionParams(), outOpCode);
    EXPECT_EQ(SoePacketValidationResult::InvalidOpCode, result);
}

INSTANTIATE_TEST_SUITE_P(
    InvalidOpCodes,
    ValidatePacketInvalidOpCodeTest,
    ::testing::Values(0, 4, 255)
);

TEST_F(SoePacketUtilsTests, ValidatePacket_Validates_OpOnlyContextlessPacket)
{
    std::vector<uint8_t> packet = { 0, static_cast<uint8_t>(SoeOpCode::UnknownSender) };
    SoeOpCode outOpCode;
    SoePacketValidationResult result = SoePacketUtils::ValidatePacket(packet, GetSessionParams(), outOpCode);
    EXPECT_EQ(SoePacketValidationResult::Valid, result);
}

class ValidatePacketCrcTest : public SoePacketUtilsTests, public ::testing::WithParamInterface<uint8_t> {};

TEST_P(ValidatePacketCrcTest, ValidatePacket_Validates_ValidContextualPacketForAllCrcLengths)
{
    uint8_t crcLength = GetParam();
    SessionParameters sessionParams = GetSessionParams(crcLength);

    std::vector<uint8_t> packet(sizeof(uint16_t) + AcknowledgeAll::SIZE + crcLength);
    size_t offset = 0;

    // Write OpCode (big-endian)
    uint16_t opCode = static_cast<uint16_t>(SoeOpCode::AcknowledgeAll);
    packet[offset++] = static_cast<uint8_t>((opCode >> 8) & 0xFF);
    packet[offset++] = static_cast<uint8_t>(opCode & 0xFF);

    // Serialize AcknowledgeAll
    AcknowledgeAll ackAll(10);
    std::span<uint8_t> ackSpan(packet.data() + offset, AcknowledgeAll::SIZE);
    ackAll.Serialize(ackSpan);
    offset += AcknowledgeAll::SIZE;

    // Append CRC
    SoePacketUtils::AppendCrc(packet, offset, sessionParams.CrcState, crcLength);

    SoeOpCode outOpCode;
    SoePacketValidationResult result = SoePacketUtils::ValidatePacket(packet, sessionParams, outOpCode);
    EXPECT_EQ(SoePacketValidationResult::Valid, result);
}

INSTANTIATE_TEST_SUITE_P(
    AllCrcLengths,
    ValidatePacketCrcTest,
    ::testing::Values(0, 1, 2, 3, 4)
);

TEST_F(SoePacketUtilsTests, ValidatePacket_Invalidates_ContextualPacketWithIncorrectCrc)
{
    constexpr uint8_t CRC_LENGTH = 2;

    SessionParameters sessionParams = GetSessionParams(CRC_LENGTH);
    std::vector<uint8_t> packet(sizeof(uint16_t) + AcknowledgeAll::SIZE + CRC_LENGTH);
    size_t offset = 0;

    // Write OpCode (big-endian)
    uint16_t opCode = static_cast<uint16_t>(SoeOpCode::AcknowledgeAll);
    packet[offset++] = static_cast<uint8_t>((opCode >> 8) & 0xFF);
    packet[offset++] = static_cast<uint8_t>(opCode & 0xFF);

    // Serialize AcknowledgeAll
    AcknowledgeAll ackAll(10);
    std::span<uint8_t> ackSpan(packet.data() + offset, AcknowledgeAll::SIZE);
    ackAll.Serialize(ackSpan);
    offset += AcknowledgeAll::SIZE;

    // Append CRC with wrong seed
    Crc32 wrongCrc(0);
    SoePacketUtils::AppendCrc(packet, offset, wrongCrc, CRC_LENGTH);

    SoeOpCode outOpCode;
    SoePacketValidationResult result = SoePacketUtils::ValidatePacket(packet, sessionParams, outOpCode);
    EXPECT_EQ(SoePacketValidationResult::CrcMismatch, result);
}

TEST_F(SoePacketUtilsTests, Decompress_Succeeds)
{
    std::vector<uint8_t> data = { 5, 5, 5, 5, 5, 10, 10, 10, 10, 10 };

    // Compress the data using zlib
    std::vector<uint8_t> compressed = SoePacketUtils::Compress(data);

    // Decompress it back
    std::vector<uint8_t> decompressed = SoePacketUtils::Decompress(compressed);

    EXPECT_EQ(data, decompressed);
}
