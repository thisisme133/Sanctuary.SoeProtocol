#include <gtest/gtest.h>
#include "Objects/Packets/RemapConnection.h"
#include <vector>
#include <cstdint>

using namespace Sanctuary::SoeProtocol::Objects::Packets;

TEST(RemapConnectionTests, RoundTrip_Succeeds)
{
    RemapConnection remapConnection(16, 32);

    std::vector<uint8_t> buffer(RemapConnection::SIZE);
    remapConnection.Serialize(buffer);

    RemapConnection deserialized = RemapConnection::Deserialize(buffer, true);
    EXPECT_EQ(remapConnection, deserialized);
}
