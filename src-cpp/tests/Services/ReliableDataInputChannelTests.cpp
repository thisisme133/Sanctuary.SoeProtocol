#include <gtest/gtest.h>
#include "Objects/SessionParameters.h"
#include "Objects/SoeConstants.h"
#include "Objects/Packets/Acknowledge.h"
#include "Services/ReliableDataInputChannel.h"
#include "Services/SoeProtocolHandler.h"
#include "Util/DataUtils.h"
#include "Util/MultiPacketUtils.h"
#include "Util/NativeSpanPool.h"
#include "Util/SoePacketUtils.h"
#include "mocks/MockApplicationProtocolHandler.h"
#include "mocks/MockNetworkInterface.h"
#include <queue>
#include <vector>
#include <cstdint>
#include <random>

using namespace Sanctuary::SoeProtocol::Objects;
using namespace Sanctuary::SoeProtocol::Objects::Packets;
using namespace Sanctuary::SoeProtocol::Services;
using namespace Sanctuary::SoeProtocol::Util;
using namespace Sanctuary::SoeProtocol::Tests::Mocks;

class ReliableDataInputChannelTests : public ::testing::Test
{
protected:
    static constexpr int DATA_LENGTH = 16;
    static inline NativeSpanPool SpanPool = NativeSpanPool(DATA_LENGTH + 6, 8);

    std::queue<std::vector<uint8_t>> dataOutputQueue_;
    std::unique_ptr<MockNetworkInterface> netInterface_;
    SessionParameters sessionParams_;
    std::unique_ptr<ReliableDataInputChannel> channel_;
    std::unique_ptr<SoeProtocolHandler> handler_;

    void SetUp() override
    {
        dataOutputQueue_ = std::queue<std::vector<uint8_t>>();
        netInterface_ = std::make_unique<MockNetworkInterface>();

        sessionParams_.ApplicationProtocol = "TestProtocol";
        sessionParams_.RemoteUdpLength = 512;
        sessionParams_.AcknowledgeAllData = true;
        sessionParams_.MaximumAcknowledgeDelay = std::chrono::milliseconds(0);

        auto appHandler = std::make_unique<MockApplicationProtocolHandler>();

        handler_ = std::make_unique<SoeProtocolHandler>(
            nullptr,
            SessionMode::Client,
            sessionParams_,
            SpanPool,
            *netInterface_,
            *appHandler
        );

        channel_ = std::make_unique<ReliableDataInputChannel>(
            *handler_,
            handler_->GetSessionParams(),
            handler_->GetApplicationParams(),
            SpanPool,
            [this](std::span<const uint8_t> data) {
                dataOutputQueue_.push(std::vector<uint8_t>(data.begin(), data.end()));
            }
        );
    }

    void TearDown() override
    {
        channel_.reset();
        handler_.reset();
        netInterface_.reset();
    }

    static std::vector<uint8_t> GetDataFragment(
        uint16_t sequence,
        std::optional<uint32_t> completeDataLength,
        std::vector<uint8_t>& outData,
        int dataLength = DATA_LENGTH
    )
    {
        size_t length = dataLength
            + sizeof(uint16_t) // Sequence
            + (completeDataLength.has_value() ? sizeof(uint32_t) : 0);

        outData.resize(dataLength);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : outData)
            byte = static_cast<uint8_t>(dis(gen));

        std::vector<uint8_t> buffer(length);
        size_t offset = 0;

        // Write sequence (big-endian)
        buffer[offset++] = static_cast<uint8_t>((sequence >> 8) & 0xFF);
        buffer[offset++] = static_cast<uint8_t>(sequence & 0xFF);

        if (completeDataLength.has_value())
        {
            uint32_t cdl = completeDataLength.value();
            buffer[offset++] = static_cast<uint8_t>((cdl >> 24) & 0xFF);
            buffer[offset++] = static_cast<uint8_t>((cdl >> 16) & 0xFF);
            buffer[offset++] = static_cast<uint8_t>((cdl >> 8) & 0xFF);
            buffer[offset++] = static_cast<uint8_t>(cdl & 0xFF);
        }

        std::copy(outData.begin(), outData.end(), buffer.begin() + offset);

        return buffer;
    }

    void AssertCanPopAck(int expectedSequence, bool expectAll)
    {
        // Run a tick to force ack-alls
        channel_->RunTick();

        ASSERT_FALSE(netInterface_->SentData.empty());
        auto ack = netInterface_->SentData.front();
        netInterface_->SentData.pop();

        SoeOpCode expectedCode = expectAll ? SoeOpCode::AcknowledgeAll : SoeOpCode::Acknowledge;
        SoeOpCode actualCode;
        SoePacketUtils::ReadSoeOpCode(ack, actualCode);
        EXPECT_EQ(expectedCode, actualCode);

        std::span<const uint8_t> ackSpan(ack.data() + sizeof(uint16_t), ack.size() - sizeof(uint16_t));
        Acknowledge deserialized = Acknowledge::Deserialize(ackSpan);
        EXPECT_EQ(expectedSequence, deserialized.GetSequence());
    }
};

class SequentialFragmentTest : public ReliableDataInputChannelTests, public ::testing::WithParamInterface<bool> {};

TEST_P(SequentialFragmentTest, TestSequentialFragmentInsert)
{
    bool ackAll = GetParam();
    sessionParams_.AcknowledgeAllData = ackAll;

    std::vector<uint8_t> data0, data1, data2;
    auto fragment0 = GetDataFragment(0, DATA_LENGTH * 3, data0);
    auto fragment1 = GetDataFragment(1, std::nullopt, data1);
    auto fragment2 = GetDataFragment(2, std::nullopt, data2);

    channel_->HandleReliableDataFragment(fragment0);
    AssertCanPopAck(0, !ackAll);
    EXPECT_TRUE(dataOutputQueue_.empty());

    channel_->HandleReliableDataFragment(fragment1);
    AssertCanPopAck(1, !ackAll);
    EXPECT_TRUE(dataOutputQueue_.empty());

    channel_->HandleReliableDataFragment(fragment2);
    AssertCanPopAck(2, !ackAll);
    EXPECT_EQ(1, dataOutputQueue_.size());

    EXPECT_TRUE(netInterface_->SentData.empty()); // Check no superfluous acknowledgements

    size_t offset = 0;
    auto stitchedData = dataOutputQueue_.front();
    dataOutputQueue_.pop();

    EXPECT_EQ(data0, std::vector<uint8_t>(stitchedData.begin() + offset, stitchedData.begin() + offset + DATA_LENGTH));
    offset += DATA_LENGTH;
    EXPECT_EQ(data1, std::vector<uint8_t>(stitchedData.begin() + offset, stitchedData.begin() + offset + DATA_LENGTH));
    offset += DATA_LENGTH;
    EXPECT_EQ(data2, std::vector<uint8_t>(stitchedData.begin() + offset, stitchedData.end()));
}

INSTANTIATE_TEST_SUITE_P(AckModes, SequentialFragmentTest, ::testing::Values(true, false));

class NonSequentialFragmentTest : public ReliableDataInputChannelTests, public ::testing::WithParamInterface<bool> {};

TEST_P(NonSequentialFragmentTest, TestNonSequentialFragmentInsert)
{
    bool ackAll = GetParam();
    sessionParams_.AcknowledgeAllData = ackAll;

    std::vector<uint8_t> data0, data1, data2;
    auto fragment0 = GetDataFragment(0, DATA_LENGTH * 3, data0);
    auto fragment1 = GetDataFragment(1, std::nullopt, data1);
    auto fragment2 = GetDataFragment(2, std::nullopt, data2);

    channel_->HandleReliableDataFragment(fragment2);
    AssertCanPopAck(2, false);

    channel_->HandleReliableDataFragment(fragment0);
    AssertCanPopAck(0, !ackAll);
    EXPECT_TRUE(dataOutputQueue_.empty());

    channel_->HandleReliableDataFragment(fragment1);
    AssertCanPopAck(ackAll ? 1 : 2, !ackAll);
    EXPECT_EQ(1, dataOutputQueue_.size());

    EXPECT_TRUE(netInterface_->SentData.empty()); // Check no superfluous acknowledgements

    size_t offset = 0;
    auto stitchedData = dataOutputQueue_.front();
    dataOutputQueue_.pop();

    EXPECT_EQ(data0, std::vector<uint8_t>(stitchedData.begin() + offset, stitchedData.begin() + offset + DATA_LENGTH));
    offset += DATA_LENGTH;
    EXPECT_EQ(data1, std::vector<uint8_t>(stitchedData.begin() + offset, stitchedData.begin() + offset + DATA_LENGTH));
    offset += DATA_LENGTH;
    EXPECT_EQ(data2, std::vector<uint8_t>(stitchedData.begin() + offset, stitchedData.end()));
}

INSTANTIATE_TEST_SUITE_P(AckModes, NonSequentialFragmentTest, ::testing::Values(true, false));

class NonFragmentTest : public ReliableDataInputChannelTests, public ::testing::WithParamInterface<bool> {};

TEST_P(NonFragmentTest, TestNonFragmentInsert)
{
    bool ackAll = GetParam();
    sessionParams_.AcknowledgeAllData = ackAll;

    std::vector<uint8_t> data0, data1, data2;
    auto packet0 = GetDataFragment(0, std::nullopt, data0);
    auto packet1 = GetDataFragment(1, std::nullopt, data1);
    auto packet2 = GetDataFragment(2, std::nullopt, data2);

    channel_->HandleReliableData(packet0);
    AssertCanPopAck(0, !ackAll);
    EXPECT_EQ(data0, dataOutputQueue_.front());
    dataOutputQueue_.pop();

    channel_->HandleReliableData(packet2);
    AssertCanPopAck(2, false);

    channel_->HandleReliableData(packet1);
    AssertCanPopAck(ackAll ? 1 : 2, !ackAll);
    EXPECT_EQ(data1, dataOutputQueue_.front());
    dataOutputQueue_.pop();
    EXPECT_EQ(data2, dataOutputQueue_.front());
    dataOutputQueue_.pop();

    EXPECT_TRUE(netInterface_->SentData.empty()); // Check no superfluous acknowledgements
}

INSTANTIATE_TEST_SUITE_P(AckModes, NonFragmentTest, ::testing::Values(true, false));
