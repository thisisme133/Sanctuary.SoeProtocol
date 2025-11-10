#include <gtest/gtest.h>
#include "Objects/SessionParameters.h"
#include "Objects/SoeConstants.h"
#include "Objects/SoeOpCode.h"
#include "Objects/Packets/Acknowledge.h"
#include "Objects/Packets/AcknowledgeAll.h"
#include "Services/ReliableDataOutputChannel.h"
#include "Services/SoeProtocolHandler.h"
#include "Util/NativeSpanPool.h"
#include "mocks/MockApplicationProtocolHandler.h"
#include "mocks/MockNetworkInterface.h"
#include <vector>
#include <cstdint>
#include <random>
#include <thread>
#include <chrono>

using namespace Sanctuary::SoeProtocol::Objects;
using namespace Sanctuary::SoeProtocol::Objects::Packets;
using namespace Sanctuary::SoeProtocol::Services;
using namespace Sanctuary::SoeProtocol::Util;
using namespace Sanctuary::SoeProtocol::Tests::Mocks;

class ReliableDataOutputChannelTests : public ::testing::Test
{
protected:
    static constexpr int FRAGMENT_WINDOW_SIZE = 8;
    static constexpr int MAX_DATA_LENGTH = static_cast<int>(SoeConstants::DEFAULT_UDP_LENGTH)
        - sizeof(uint16_t) // SoeOpCode
        - sizeof(uint16_t) // Sequence
        - SoeConstants::CRC_LENGTH;

    static inline NativeSpanPool SpanPool = NativeSpanPool(512, 8);

    std::unique_ptr<MockNetworkInterface> netInterface_;
    std::unique_ptr<MockApplicationProtocolHandler> appHandler_;
    std::unique_ptr<ReliableDataOutputChannel> channel_;
    std::unique_ptr<SoeProtocolHandler> handler_;

    void SetUp() override
    {
        netInterface_ = std::make_unique<MockNetworkInterface>();
        appHandler_ = std::make_unique<MockApplicationProtocolHandler>();

        SessionParameters params;
        params.ApplicationProtocol = "TestProtocol";
        params.RemoteUdpLength = SoeConstants::DEFAULT_UDP_LENGTH;
        params.IsCompressionEnabled = false;
        params.CrcLength = SoeConstants::CRC_LENGTH;
        params.MaxQueuedOutgoingReliableDataPackets = FRAGMENT_WINDOW_SIZE;

        handler_ = std::make_unique<SoeProtocolHandler>(
            nullptr,
            SessionMode::Client,
            params,
            SpanPool,
            *netInterface_,
            *appHandler_
        );

        channel_ = std::make_unique<ReliableDataOutputChannel>(
            *handler_,
            SpanPool,
            MAX_DATA_LENGTH + sizeof(uint16_t)
        );
    }

    void TearDown() override
    {
        channel_.reset();
        handler_.reset();
        appHandler_.reset();
        netInterface_.reset();
    }

    static std::vector<uint8_t> GeneratePacket(int size)
    {
        std::vector<uint8_t> packet(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : packet)
            byte = static_cast<uint8_t>(dis(gen));
        return packet;
    }

    static void AssertReceivedPacketsEqualBuffer(
        MockNetworkInterface& networkInterface,
        std::span<const uint8_t> buffer,
        bool expectMasterFragment
    )
    {
        size_t position = 0;

        while (!networkInterface.SentData.empty())
        {
            auto receiveBuffer = networkInterface.SentData.front();
            networkInterface.SentData.pop();

            size_t dataOffset = sizeof(uint16_t) // SoeOpCode
                + sizeof(uint16_t) // Sequence
                + (expectMasterFragment ? sizeof(uint32_t) : 0);

            std::span<const uint8_t> data(
                receiveBuffer.data() + dataOffset,
                receiveBuffer.size() - dataOffset - SoeConstants::CRC_LENGTH
            );
            expectMasterFragment = false;

            for (uint8_t byte : data)
            {
                ASSERT_LT(position, buffer.size());
                EXPECT_EQ(buffer[position++], byte);
            }
        }

        EXPECT_EQ(buffer.size(), position);
    }
};

TEST_F(ReliableDataOutputChannelTests, TestRepeatsDataOnAckFailure)
{
    constexpr int fragmentCount = 4;
    constexpr int packetLength = MAX_DATA_LENGTH - 4 + MAX_DATA_LENGTH * (fragmentCount - 1);
    auto packet = GeneratePacket(packetLength);

    channel_->EnqueueData(packet);
    channel_->RunTick();
    AssertReceivedPacketsEqualBuffer(*netInterface_, packet, true);

    // Don't acknowledge
    std::this_thread::sleep_for(
        std::chrono::milliseconds(ReliableDataOutputChannel::ACK_WAIT_MILLISECONDS + 100)
    );
    channel_->RunTick();
    AssertReceivedPacketsEqualBuffer(*netInterface_, packet, true);
}

TEST_F(ReliableDataOutputChannelTests, HandlesAdvanceAcking)
{
    constexpr int fragmentCount = 4;
    constexpr int packetLength = MAX_DATA_LENGTH - 4 + MAX_DATA_LENGTH * (fragmentCount - 1);
    auto packet = GeneratePacket(packetLength);

    channel_->EnqueueData(packet);
    channel_->RunTick();

    AssertReceivedPacketsEqualBuffer(*netInterface_, packet, true);

    channel_->NotifyOfAcknowledge(Acknowledge(3));

    std::this_thread::sleep_for(
        std::chrono::milliseconds(ReliableDataOutputChannel::ACK_WAIT_MILLISECONDS + 100)
    );

    channel_->RunTick();

    std::span<const uint8_t> expectedSpan(packet.data(), packet.size() - MAX_DATA_LENGTH);
    AssertReceivedPacketsEqualBuffer(*netInterface_, expectedSpan, true);
}

TEST_F(ReliableDataOutputChannelTests, TestRepeatsDataFromArbitraryPositionOnAckDelay)
{
    constexpr int fragmentCount = 4;
    constexpr int packetLength = MAX_DATA_LENGTH - 4 + MAX_DATA_LENGTH * (fragmentCount - 1);
    auto packet = GeneratePacket(packetLength);

    channel_->EnqueueData(packet);
    channel_->RunTick();

    AssertReceivedPacketsEqualBuffer(*netInterface_, packet, true);
    channel_->NotifyOfAcknowledgeAll(AcknowledgeAll(1));

    std::this_thread::sleep_for(
        std::chrono::milliseconds(ReliableDataOutputChannel::ACK_WAIT_MILLISECONDS + 100)
    );
    channel_->RunTick();

    constexpr int expectedConsumed = MAX_DATA_LENGTH - 4 + MAX_DATA_LENGTH;
    std::span<const uint8_t> expectedSpan(packet.data() + expectedConsumed, packet.size() - expectedConsumed);
    AssertReceivedPacketsEqualBuffer(*netInterface_, expectedSpan, false);
}

TEST_F(ReliableDataOutputChannelTests, TestRepeatsFullWindowOfDataFromArbitraryPositionOnAckDelay)
{
    constexpr int fragmentCount = FRAGMENT_WINDOW_SIZE * 2;
    constexpr int packetLength = MAX_DATA_LENGTH - 4 + MAX_DATA_LENGTH * (fragmentCount - 1);
    auto packet = GeneratePacket(packetLength);

    channel_->EnqueueData(packet);
    channel_->RunTick();

    constexpr int expectedReceiveLength = MAX_DATA_LENGTH - 4 + MAX_DATA_LENGTH * (FRAGMENT_WINDOW_SIZE - 1);
    std::span<const uint8_t> firstExpected(packet.data(), expectedReceiveLength);
    AssertReceivedPacketsEqualBuffer(*netInterface_, firstExpected, true);

    channel_->NotifyOfAcknowledgeAll(AcknowledgeAll(FRAGMENT_WINDOW_SIZE - 2));
    std::this_thread::sleep_for(
        std::chrono::milliseconds(ReliableDataOutputChannel::ACK_WAIT_MILLISECONDS + 100)
    );
    channel_->RunTick();

    constexpr int expectedConsumed = MAX_DATA_LENGTH - 4 + MAX_DATA_LENGTH * (FRAGMENT_WINDOW_SIZE - 2);
    constexpr int expectedRepeatLength = MAX_DATA_LENGTH * FRAGMENT_WINDOW_SIZE;

    std::span<const uint8_t> secondExpected(packet.data() + expectedConsumed, expectedRepeatLength);
    AssertReceivedPacketsEqualBuffer(*netInterface_, secondExpected, false);
}

class AllDataTest : public ReliableDataOutputChannelTests, public ::testing::WithParamInterface<bool> {};

TEST_P(AllDataTest, TestAllTheData)
{
    bool useEncryption = GetParam();
    appHandler_->GetSessionParams().IsEncryptionEnabled = useEncryption;

    constexpr int maxPacketLength = 512;
    constexpr int maxNonFragmentDataLength = maxPacketLength - sizeof(uint16_t);
    uint16_t sequence = 0;
    std::queue<std::vector<uint8_t>> dataQueue;

    for (int i = 0; i < 256; i++)
    {
        std::vector<uint8_t> data = GeneratePacket(i * 16);

        if (data.size() < maxNonFragmentDataLength)
        {
            std::vector<uint8_t> fragment(data.size() + sizeof(uint16_t));
            fragment[0] = static_cast<uint8_t>((sequence >> 8) & 0xFF);
            fragment[1] = static_cast<uint8_t>(sequence & 0xFF);
            sequence++;
            std::copy(data.begin(), data.end(), fragment.begin() + sizeof(uint16_t));
            dataQueue.push(fragment);
        }
        else
        {
            size_t remaining = data.size();
            size_t offset = 0;

            // First fragment with complete data length
            std::vector<uint8_t> fragment(maxPacketLength);
            fragment[0] = static_cast<uint8_t>((sequence >> 8) & 0xFF);
            fragment[1] = static_cast<uint8_t>(sequence & 0xFF);
            sequence++;

            uint32_t cdl = static_cast<uint32_t>(data.size());
            fragment[2] = static_cast<uint8_t>((cdl >> 24) & 0xFF);
            fragment[3] = static_cast<uint8_t>((cdl >> 16) & 0xFF);
            fragment[4] = static_cast<uint8_t>((cdl >> 8) & 0xFF);
            fragment[5] = static_cast<uint8_t>(cdl & 0xFF);

            size_t firstFragmentDataSize = maxPacketLength - sizeof(uint16_t) - sizeof(uint32_t);
            std::copy(data.begin(), data.begin() + firstFragmentDataSize, fragment.begin() + 6);
            offset += firstFragmentDataSize;
            remaining -= firstFragmentDataSize;
            dataQueue.push(fragment);

            // Subsequent fragments
            while (remaining > 0)
            {
                size_t fragSize = std::min(static_cast<size_t>(maxPacketLength), remaining + sizeof(uint16_t));
                fragment.resize(fragSize);
                fragment[0] = static_cast<uint8_t>((sequence >> 8) & 0xFF);
                fragment[1] = static_cast<uint8_t>(sequence & 0xFF);
                sequence++;

                size_t dataSize = fragSize - sizeof(uint16_t);
                std::copy(data.begin() + offset, data.begin() + offset + dataSize, fragment.begin() + sizeof(uint16_t));
                offset += dataSize;
                remaining -= dataSize;
                dataQueue.push(fragment);
            }
        }

        channel_->EnqueueData(data);
        channel_->NotifyOfAcknowledgeAll(AcknowledgeAll(static_cast<uint16_t>(sequence - 1)));
    }

    while (!netInterface_->SentData.empty())
    {
        auto sent = netInterface_->SentData.front();
        netInterface_->SentData.pop();

        auto expected = dataQueue.front();
        dataQueue.pop();

        std::span<const uint8_t> sentData(sent.data() + sizeof(uint16_t), sent.size() - sizeof(uint16_t));
        EXPECT_EQ(expected, std::vector<uint8_t>(sentData.begin(), sentData.end()));
    }
}

INSTANTIATE_TEST_SUITE_P(EncryptionModes, AllDataTest, ::testing::Values(false, true));
