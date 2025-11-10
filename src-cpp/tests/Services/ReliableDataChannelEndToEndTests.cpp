#include <gtest/gtest.h>
#include "Objects/SessionParameters.h"
#include "Objects/SoeConstants.h"
#include "Objects/SoeOpCode.h"
#include "Objects/Packets/Acknowledge.h"
#include "Objects/Packets/AcknowledgeAll.h"
#include "Services/ReliableDataInputChannel.h"
#include "Services/ReliableDataOutputChannel.h"
#include "Services/SoeProtocolHandler.h"
#include "Util/NativeSpanPool.h"
#include "mocks/MockApplicationProtocolHandler.h"
#include "mocks/MockNetworkInterface.h"
#include <queue>
#include <vector>
#include <cstdint>
#include <random>
#include <iostream>

using namespace Sanctuary::SoeProtocol::Objects;
using namespace Sanctuary::SoeProtocol::Objects::Packets;
using namespace Sanctuary::SoeProtocol::Services;
using namespace Sanctuary::SoeProtocol::Util;
using namespace Sanctuary::SoeProtocol::Tests::Mocks;

class ReliableDataChannelEndToEndTests : public ::testing::Test
{
protected:
    static constexpr int MAX_DATA_LENGTH = static_cast<int>(SoeConstants::DEFAULT_UDP_LENGTH)
        - sizeof(uint16_t) // SoeOpCode
        - sizeof(uint16_t) // Sequence
        - SoeConstants::CRC_LENGTH;

    static inline NativeSpanPool SpanPool = NativeSpanPool(512, 16);

    static std::vector<uint8_t> GeneratePacket(int size)
    {
        std::vector<uint8_t> packet(size);
        std::mt19937 gen(23445); // Fixed seed for reproducibility
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : packet)
            byte = static_cast<uint8_t>(dis(gen));
        return packet;
    }

    static void GetHandlers(
        std::unique_ptr<ReliableDataOutputChannel>& outputChannel,
        std::unique_ptr<ReliableDataInputChannel>& inputChannel,
        std::unique_ptr<MockNetworkInterface>& networkInterface,
        std::queue<std::vector<uint8_t>>& receiveQueue
    )
    {
        networkInterface = std::make_unique<MockNetworkInterface>();
        constexpr int fragmentWindowSize = 32;

        SessionParameters params;
        params.ApplicationProtocol = "TestProtocol";
        params.RemoteUdpLength = SoeConstants::DEFAULT_UDP_LENGTH;
        params.IsCompressionEnabled = false;
        params.CrcLength = SoeConstants::CRC_LENGTH;
        params.MaxQueuedIncomingReliableDataPackets = fragmentWindowSize;
        params.MaxQueuedOutgoingReliableDataPackets = fragmentWindowSize;

        auto appHandler = std::make_unique<MockApplicationProtocolHandler>();

        auto handler = std::make_unique<SoeProtocolHandler>(
            nullptr,
            SessionMode::Client,
            params,
            SpanPool,
            *networkInterface,
            *appHandler
        );

        outputChannel = std::make_unique<ReliableDataOutputChannel>(
            *handler,
            SpanPool,
            MAX_DATA_LENGTH + sizeof(uint16_t)
        );

        inputChannel = std::make_unique<ReliableDataInputChannel>(
            *handler,
            handler->GetSessionParams(),
            handler->GetApplicationParams(),
            SpanPool,
            [&receiveQueue](std::span<const uint8_t> data) {
                receiveQueue.push(std::vector<uint8_t>(data.begin(), data.end()));
            }
        );
    }

    void AssertOnPackets(
        const std::vector<std::vector<uint8_t>>& packets,
        int numberOfPacketsToMulti
    )
    {
        std::queue<std::vector<uint8_t>> receiveQueue;
        std::unique_ptr<ReliableDataOutputChannel> outputChannel;
        std::unique_ptr<ReliableDataInputChannel> inputChannel;
        std::unique_ptr<MockNetworkInterface> networkInterface;

        GetHandlers(outputChannel, inputChannel, networkInterface, receiveQueue);

        for (size_t i = 0; i < packets.size(); i++)
        {
            outputChannel->EnqueueData(packets[i]);

            // If multi-batching is disabled, or we've submitted enough packets to do a multi-dispatch
            if (numberOfPacketsToMulti > 0 && (i + 1) % numberOfPacketsToMulti != 0)
                continue;

            size_t lastCount;
            do
            {
                lastCount = networkInterface->SentData.size();
                outputChannel->RunTick();

                if (networkInterface->SentData.empty())
                    continue;

                auto lastPacket = networkInterface->SentData.back();

                // Read sequence (big-endian)
                uint16_t sequence = (static_cast<uint16_t>(lastPacket[2]) << 8)
                    | static_cast<uint16_t>(lastPacket[3]);

                std::cout << "Acknowledging sequence " << sequence << std::endl;
                outputChannel->NotifyOfAcknowledgeAll(AcknowledgeAll(sequence));
            }
            while (networkInterface->SentData.size() > lastCount);
        }
        outputChannel->RunTick();

        while (!networkInterface->SentData.empty())
        {
            auto packet = networkInterface->SentData.front();
            networkInterface->SentData.pop();

            // Read OpCode (big-endian)
            SoeOpCode op = static_cast<SoeOpCode>(
                (static_cast<uint16_t>(packet[0]) << 8) | static_cast<uint16_t>(packet[1])
            );

            std::span<uint8_t> outputData(
                packet.data() + sizeof(uint16_t),
                packet.size() - sizeof(uint16_t) - SoeConstants::CRC_LENGTH
            );

            uint16_t sequence = 0;
            if (op == SoeOpCode::ReliableData || op == SoeOpCode::ReliableDataFragment)
            {
                sequence = (static_cast<uint16_t>(outputData[0]) << 8)
                    | static_cast<uint16_t>(outputData[1]);
            }

            std::cout << "Handling " << static_cast<int>(op) << " packet of length "
                      << packet.size() << " with seq " << sequence << std::endl;

            if (op == SoeOpCode::ReliableData)
                inputChannel->HandleReliableData(outputData);
            else if (op == SoeOpCode::ReliableDataFragment)
                inputChannel->HandleReliableDataFragment(outputData);
            else if (op == SoeOpCode::Acknowledge)
                outputChannel->NotifyOfAcknowledge(Acknowledge::Deserialize(
                    std::span<const uint8_t>(packet.data() + sizeof(uint16_t), packet.size() - sizeof(uint16_t))
                ));
            else if (op == SoeOpCode::AcknowledgeAll)
                outputChannel->NotifyOfAcknowledgeAll(AcknowledgeAll::Deserialize(
                    std::span<const uint8_t>(packet.data() + sizeof(uint16_t), packet.size() - sizeof(uint16_t))
                ));
        }

        ASSERT_EQ(packets.size(), receiveQueue.size());
        for (size_t i = 0; i < packets.size(); i++)
        {
            std::cout << "Checking recomposed packet " << i << std::endl;
            auto recomposed = receiveQueue.front();
            receiveQueue.pop();
            const auto& expected = packets[i];

            ASSERT_EQ(expected.size(), recomposed.size());
            EXPECT_EQ(expected, recomposed);
        }
    }
};

TEST_F(ReliableDataChannelEndToEndTests, TestSingleSmallPacket)
{
    AssertOnPackets({ GeneratePacket(5) }, 0);
}

class MultipleSmallPacketsTest : public ReliableDataChannelEndToEndTests, public ::testing::WithParamInterface<int> {};

TEST_P(MultipleSmallPacketsTest, TestMultipleSmallPackets)
{
    int numberOfPacketsToMulti = GetParam();
    AssertOnPackets(
        {
            GeneratePacket(3),
            GeneratePacket(45),
            GeneratePacket(1),
            GeneratePacket(214)
        },
        numberOfPacketsToMulti
    );
}

INSTANTIATE_TEST_SUITE_P(MultiCounts, MultipleSmallPacketsTest, ::testing::Values(0, 2));

class FragmentationTest : public ReliableDataChannelEndToEndTests, public ::testing::WithParamInterface<int> {};

TEST_P(FragmentationTest, TestMultipleSmallPacketsRequiringFragmentation)
{
    int numberOfPacketsToMulti = GetParam();
    AssertOnPackets(
        {
            GeneratePacket(3),
            GeneratePacket(45),
            GeneratePacket(1),
            GeneratePacket(214),
            GeneratePacket(214),
            GeneratePacket(214)
        },
        numberOfPacketsToMulti
    );
}

INSTANTIATE_TEST_SUITE_P(MultiCounts, FragmentationTest, ::testing::Values(0, 1, 2, 3, 4, 5, 6));

TEST_F(ReliableDataChannelEndToEndTests, TestLargestDataPacket)
{
    AssertOnPackets({ GeneratePacket(MAX_DATA_LENGTH) }, 0);
}

TEST_F(ReliableDataChannelEndToEndTests, TestSingleLargePacket)
{
    AssertOnPackets({ GeneratePacket(MAX_DATA_LENGTH + 1) }, 0);
}

class MultipleLargePacketsTest : public ReliableDataChannelEndToEndTests, public ::testing::WithParamInterface<int> {};

TEST_P(MultipleLargePacketsTest, TestMultipleLargePackets)
{
    int numberOfPacketsToMulti = GetParam();
    AssertOnPackets(
        {
            GeneratePacket(static_cast<int>(SoeConstants::DEFAULT_UDP_LENGTH)),
            GeneratePacket(static_cast<int>(SoeConstants::DEFAULT_UDP_LENGTH) + 7),
            GeneratePacket(static_cast<int>(SoeConstants::DEFAULT_UDP_LENGTH) + 54),
            GeneratePacket(static_cast<int>(SoeConstants::DEFAULT_UDP_LENGTH) * 2)
        },
        numberOfPacketsToMulti
    );
}

INSTANTIATE_TEST_SUITE_P(MultiCounts, MultipleLargePacketsTest, ::testing::Values(0, 5));

class AllPacketsTest : public ReliableDataChannelEndToEndTests, public ::testing::WithParamInterface<int> {};

TEST_P(AllPacketsTest, TestAllThePackets)
{
    int multiCount = GetParam();
    std::vector<std::vector<uint8_t>> packets;
    packets.reserve(256);

    for (int i = 1; i <= 256; i++)
        packets.push_back(GeneratePacket(i * 256));

    AssertOnPackets(packets, multiCount);
}

INSTANTIATE_TEST_SUITE_P(MultiCounts, AllPacketsTest, ::testing::Values(0, 8));
