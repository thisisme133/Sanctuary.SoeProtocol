#include <gtest/gtest.h>
#include "Objects/Packets/SessionRequest.h"
#include <vector>
#include <cstdint>

using namespace Sanctuary::SoeProtocol::Objects::Packets;

TEST(SessionRequestTests, RoundTrip_Succeeds)
{
    SessionRequest sessionRequest(3, 5467392, 512, "TestProtocol");

    std::vector<uint8_t> buffer(sessionRequest.GetSize());
    sessionRequest.Serialize(buffer);

    SessionRequest deserialized = SessionRequest::Deserialize(buffer, true);
    EXPECT_EQ(sessionRequest, deserialized);
}
