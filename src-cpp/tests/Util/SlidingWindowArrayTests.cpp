#include <gtest/gtest.h>
#include "Util/SlidingWindowArray.h"
#include <vector>
#include <cstdint>

using namespace Sanctuary::SoeProtocol::Util;

TEST(SlidingWindowArrayTests, TestLength)
{
    SlidingWindowArray<uint8_t> slidingWindow(8);
    EXPECT_EQ(8, slidingWindow.Length());

    size_t offset;
    auto underlying = slidingWindow.GetUnderlyingArray(offset);
    EXPECT_EQ(8, underlying.size());
}

TEST(SlidingWindowArrayTests, TestCurrent)
{
    std::vector<uint8_t> existing = { 0, 1, 2 };
    SlidingWindowArray<uint8_t> slidingWindow(existing);

    slidingWindow.Slide();
    EXPECT_EQ(existing[1], slidingWindow.Current());
}

TEST(SlidingWindowArrayTests, TestCtorUsingExistingArray)
{
    std::vector<uint8_t> existing = { 0, 1, 2 };
    SlidingWindowArray<uint8_t> slidingWindow(existing);
    EXPECT_EQ(3, slidingWindow.Length());

    size_t offset;
    auto underlying = slidingWindow.GetUnderlyingArray(offset);
    EXPECT_EQ(existing, std::vector<uint8_t>(underlying.begin(), underlying.end()));
}

TEST(SlidingWindowArrayTests, TestIndexer)
{
    std::vector<uint8_t> existing = { 0, 1, 2 };
    SlidingWindowArray<uint8_t> slidingWindow(existing);

    for (size_t i = 0; i < existing.size(); i++)
        EXPECT_EQ(existing[i], slidingWindow[i]);

    slidingWindow[1] = 5;
    EXPECT_EQ(5, slidingWindow[1]);
}

TEST(SlidingWindowArrayTests, TestSlide)
{
    std::vector<uint8_t> existing = { 0, 1, 2, 4, 5 };
    SlidingWindowArray<uint8_t> slidingWindow(existing);

    slidingWindow.Slide(2);
    EXPECT_EQ(existing[2], slidingWindow[0]);

    slidingWindow.Slide();
    EXPECT_EQ(existing[3], slidingWindow[0]);
    EXPECT_EQ(existing[0], slidingWindow[2]);

    slidingWindow.Slide(9);
    EXPECT_EQ(existing[2], slidingWindow[0]);

    slidingWindow.Slide(-4);
    EXPECT_EQ(existing[3], slidingWindow[0]);
}
