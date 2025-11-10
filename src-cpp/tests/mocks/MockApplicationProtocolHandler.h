#pragma once

#include "Abstractions/IApplicationProtocolHandler.h"
#include "Objects/ApplicationParameters.h"
#include "Objects/Packets/Disconnect.h"
#include "Objects/Rc4KeyState.h"
#include <array>
#include <cstdint>

namespace Sanctuary::SoeProtocol::Tests::Mocks
{
    /**
     * @brief Represents a mocked IApplicationProtocolHandler.
     */
    class MockApplicationProtocolHandler : public Abstractions::IApplicationProtocolHandler
    {
    private:
        Objects::ApplicationParameters sessionParams_;
        bool hasBeenInitialized_;
        bool hasSessionOpened_;
        bool hasSessionClosed_;

    public:
        MockApplicationProtocolHandler()
            : sessionParams_(Objects::Rc4KeyState({0, 1, 2, 3, 4}))
            , hasBeenInitialized_(false)
            , hasSessionOpened_(false)
            , hasSessionClosed_(false)
        {
        }

        Objects::ApplicationParameters& GetSessionParams() override
        {
            return sessionParams_;
        }

        void Initialise(Abstractions::ISessionHandler& sessionHandler) override
        {
            hasBeenInitialized_ = true;
        }

        void OnSessionOpened() override
        {
            hasSessionOpened_ = true;
        }

        void HandleAppData(const std::span<const uint8_t> data) override
        {
            // No-op for mock
        }

        void OnSessionClosed(Objects::Packets::DisconnectReason disconnectReason) override
        {
            hasSessionClosed_ = true;
        }

        bool HasBeenInitialized() const { return hasBeenInitialized_; }
        bool HasSessionOpened() const { return hasSessionOpened_; }
        bool HasSessionClosed() const { return hasSessionClosed_; }
    };
}
