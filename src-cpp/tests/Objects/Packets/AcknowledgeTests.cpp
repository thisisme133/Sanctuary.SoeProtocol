#include <gtest/gtest.h>
#include "Objects/Packets/Acknowledge.h"
#include <vector>
#include <cstdint>

using namespace Sanctuary::SoeProtocol::Objects::Packets;

TEST(AcknowledgeTests, RoundTrip_Succeeds)
{
    Acknowledge ack(2);

    std::vector<uint8_t> buffer(Acknowledge::SIZE);
    ack.Serialize(buffer);

    Acknowledge deserialized = Acknowledge::Deserialize(buffer);
    EXPECT_EQ(ack, deserialized);
}
