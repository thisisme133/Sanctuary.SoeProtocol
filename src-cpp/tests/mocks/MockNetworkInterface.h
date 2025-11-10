#pragma once

#include "Abstractions/Services/INetworkInterface.h"
#include <queue>
#include <vector>
#include <cstdint>
#include <span>

namespace Sanctuary::SoeProtocol::Tests::Mocks
{
    /**
     * @brief Represents a mocked INetworkInterface.
     */
    class MockNetworkInterface : public Abstractions::Services::INetworkInterface
    {
    public:
        std::queue<std::vector<uint8_t>> SentData;
        std::queue<std::vector<uint8_t>> ReceiveData;

        int Available() const override
        {
            return ReceiveData.empty() ? 0 : static_cast<int>(ReceiveData.front().size());
        }

        void Bind(const std::string& localEndPoint) override
        {
            // No-op for mock
        }

        void Connect(const std::string& remoteEndPoint) override
        {
            // No-op for mock
        }

        int Receive(std::span<uint8_t> receiveTo) override
        {
            if (ReceiveData.empty())
                return 0;

            auto data = ReceiveData.front();
            ReceiveData.pop();
            std::copy(data.begin(), data.end(), receiveTo.begin());
            return static_cast<int>(data.size());
        }

        int Send(std::span<const uint8_t> data) override
        {
            SentData.push(std::vector<uint8_t>(data.begin(), data.end()));
            return static_cast<int>(data.size());
        }

        void Clear()
        {
            while (!SentData.empty())
                SentData.pop();
            while (!ReceiveData.empty())
                ReceiveData.pop();
        }
    };
}
