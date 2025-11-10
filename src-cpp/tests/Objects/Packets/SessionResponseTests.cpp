#include <gtest/gtest.h>
#include "Objects/Packets/SessionResponse.h"
#include <vector>
#include <cstdint>

using namespace Sanctuary::SoeProtocol::Objects::Packets;

TEST(SessionResponseTests, RoundTrip_Succeeds)
{
    SessionResponse sessionResponse(531633, 34322, 2, true, 0, 512, 3);

    std::vector<uint8_t> buffer(SessionResponse::SIZE);
    sessionResponse.Serialize(buffer);

    SessionResponse deserialized = SessionResponse::Deserialize(buffer, true);
    EXPECT_EQ(sessionResponse, deserialized);
}
