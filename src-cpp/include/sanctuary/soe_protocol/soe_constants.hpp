#pragma once

#include <chrono>
#include <cstdint>

namespace sanctuary::soe_protocol {

/// <summary>
/// Contains ideal constants for this implementation of the SOE protocol.
/// </summary>
class SoeConstants {
public:
    /// <summary>
    /// Gets the implemented version of the SOE protocol.
    /// </summary>
    static constexpr uint32_t SOE_PROTOCOL_VERSION = 3;

    /// <summary>
    /// Gets the number of bytes used to store the CRC check value of a packet.
    /// </summary>
    static constexpr uint8_t CRC_LENGTH = 2;

    /// <summary>
    /// Gets the default maximum packet length.
    /// </summary>
    static constexpr uint32_t DEFAULT_UDP_LENGTH = 512;

    /// <summary>
    /// Gets the default timespan after which to send a heartbeat, if no contextual
    /// packets have been received within the interval.
    /// </summary>
    static constexpr std::chrono::seconds DEFAULT_SESSION_HEARTBEAT_AFTER{25};

    /// <summary>
    /// Gets the default timespan after which to consider a session inactive, if no
    /// contextual packets have been received within the interval.
    /// </summary>
    static constexpr std::chrono::seconds DEFAULT_SESSION_INACTIVITY_TIMEOUT{30};

private:
    SoeConstants() = delete;
    ~SoeConstants() = delete;
    SoeConstants(const SoeConstants&) = delete;
    SoeConstants& operator=(const SoeConstants&) = delete;
};

} // namespace sanctuary::soe_protocol
