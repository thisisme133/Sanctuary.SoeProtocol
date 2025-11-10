#include <gtest/gtest.h>
#include "Objects/Packets/AcknowledgeAll.h"
#include <vector>
#include <cstdint>

using namespace Sanctuary::SoeProtocol::Objects::Packets;

TEST(AcknowledgeAllTests, RoundTrip_Succeeds)
{
    AcknowledgeAll ackAll(2);

    std::vector<uint8_t> buffer(AcknowledgeAll::SIZE);
    ackAll.Serialize(buffer);

    AcknowledgeAll deserialized = AcknowledgeAll::Deserialize(buffer);
    EXPECT_EQ(ackAll, deserialized);
}
