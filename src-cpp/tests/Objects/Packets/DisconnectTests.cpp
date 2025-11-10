#include <gtest/gtest.h>
#include "Objects/Packets/Disconnect.h"
#include <vector>
#include <cstdint>

using namespace Sanctuary::SoeProtocol::Objects::Packets;

TEST(DisconnectTests, RoundTrip_Succeeds)
{
    Disconnect disconnect(5, DisconnectReason::Application);

    std::vector<uint8_t> buffer(Disconnect::SIZE);
    disconnect.Serialize(buffer);

    Disconnect deserialized = Disconnect::Deserialize(buffer);
    EXPECT_EQ(disconnect, deserialized);
}
