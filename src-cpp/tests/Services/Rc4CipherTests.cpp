#include <gtest/gtest.h>
#include "Objects/Rc4KeyState.h"
#include "Services/Rc4Cipher.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace Sanctuary::SoeProtocol::Objects;
using namespace Sanctuary::SoeProtocol::Services;

struct TestVector
{
    std::string Key;
    std::string PlainText;
    std::vector<uint8_t> CipherText;

    TestVector(std::string key, std::string plainText, std::vector<uint8_t> cipherText)
        : Key(std::move(key))
        , PlainText(std::move(plainText))
        , CipherText(std::move(cipherText))
    {
    }
};

class Rc4CipherTests : public ::testing::Test
{
protected:
    // Test vectors from https://en.wikipedia.org/wiki/RC4#Test_vectors
    static std::vector<TestVector> GetDefaultTestVectors()
    {
        return {
            TestVector(
                "Key",
                "Plaintext",
                {0xBB, 0xF3, 0x16, 0xE8, 0xD9, 0x40, 0xAF, 0x0A, 0xD3}
            ),
            TestVector(
                "Wiki",
                "pedia",
                {0x10, 0x21, 0xBF, 0x04, 0x20}
            ),
            TestVector(
                "Secret",
                "Attack at dawn",
                {0x45, 0xA0, 0x1F, 0x64, 0x5F, 0xC3, 0x5B, 0x38, 0x35, 0x52, 0x54, 0x4B, 0x9B, 0xF5}
            )
        };
    }

    static Rc4KeyState GetRc4KeyState(const TestVector& testVector)
    {
        std::vector<uint8_t> keyBytes(testVector.Key.begin(), testVector.Key.end());
        return Rc4KeyState(keyBytes);
    }
};

TEST_F(Rc4CipherTests, TestEncryption)
{
    for (const auto& testVector : GetDefaultTestVectors())
    {
        Rc4KeyState state = GetRc4KeyState(testVector);
        std::vector<uint8_t> plaintextBytes(testVector.PlainText.begin(), testVector.PlainText.end());
        std::vector<uint8_t> cipherBytes(plaintextBytes.size());

        Rc4Cipher::Transform(plaintextBytes, cipherBytes, state);
        EXPECT_EQ(testVector.CipherText, cipherBytes);
    }
}

TEST_F(Rc4CipherTests, TestRoundTrip)
{
    for (const auto& testVector : GetDefaultTestVectors())
    {
        Rc4KeyState encryptState = GetRc4KeyState(testVector);
        Rc4KeyState decryptState = GetRc4KeyState(testVector);

        std::vector<uint8_t> plaintextBytes(testVector.PlainText.begin(), testVector.PlainText.end());
        std::vector<uint8_t> encryptedBytes(plaintextBytes.size());
        std::vector<uint8_t> decryptedBytes(plaintextBytes.size());

        Rc4Cipher::Transform(plaintextBytes, encryptedBytes, encryptState);
        Rc4Cipher::Transform(encryptedBytes, decryptedBytes, decryptState);

        EXPECT_EQ(plaintextBytes, decryptedBytes);
    }
}

/**
 * Tests that the state is correctly advanced after
 * a transform is completed.
 */
TEST_F(Rc4CipherTests, TestExistingKeyState)
{
    for (const auto& testVector : GetDefaultTestVectors())
    {
        size_t half = testVector.CipherText.size() / 2;
        std::vector<uint8_t> decrypted(testVector.CipherText.size());

        Rc4KeyState state = GetRc4KeyState(testVector);

        Rc4Cipher::Transform(
            std::span<const uint8_t>(testVector.CipherText.data(), half),
            std::span<uint8_t>(decrypted.data(), half),
            state
        );
        Rc4Cipher::Transform(
            std::span<const uint8_t>(testVector.CipherText.data() + half, testVector.CipherText.size() - half),
            std::span<uint8_t>(decrypted.data() + half, decrypted.size() - half),
            state
        );

        std::string result(decrypted.begin(), decrypted.end());
        EXPECT_EQ(testVector.PlainText, result);
    }
}

/**
 * Tests that the Rc4Cipher::Advance function works correctly.
 */
TEST_F(Rc4CipherTests, TestAdvance)
{
    std::array<uint8_t, 3> testValues1 = {1, 2, 3};
    std::array<uint8_t, 3> testValues2 = {1, 2, 3};

    auto testVectors = GetDefaultTestVectors();
    Rc4KeyState state1 = GetRc4KeyState(testVectors[0]);
    Rc4KeyState state2 = GetRc4KeyState(testVectors[0]);

    Rc4Cipher::Transform(testValues1, testValues1, state1);

    Rc4Cipher::Advance(2, state2);
    Rc4Cipher::Transform(
        std::span<const uint8_t>(testValues2.data() + 2, 1),
        std::span<uint8_t>(testValues2.data() + 2, 1),
        state2
    );

    EXPECT_EQ(testValues1[2], testValues2[2]);
}
