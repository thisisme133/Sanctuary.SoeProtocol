#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

// Forward declarations
namespace sanctuary::soe_protocol::services {
    class Crc32;
}

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Contains parameters used to control a session.
/// </summary>
class SessionParameters {
public:
    SessionParameters() = default;
    ~SessionParameters() = default;
    SessionParameters(const SessionParameters&) = delete;
    SessionParameters(SessionParameters&&) noexcept = default;
    SessionParameters& operator=(const SessionParameters&) = delete;
    SessionParameters& operator=(SessionParameters&&) noexcept = default;

    /// <summary>
    /// Gets the application protocol being proxied by this session.
    /// </summary>
    [[nodiscard]] const std::string& application_protocol() const noexcept {
        return application_protocol_;
    }

    /// <summary>
    /// Sets the application protocol being proxied by this session.
    /// </summary>
    void set_application_protocol(std::string protocol) {
        application_protocol_ = std::move(protocol);
    }

    /// <summary>
    /// Gets the maximum length of a UDP packet that this party
    /// can send or receive.
    /// </summary>
    [[nodiscard]] uint32_t udp_length() const noexcept {
        return udp_length_;
    }

    /// <summary>
    /// Sets the maximum length of a UDP packet that this party
    /// can send or receive.
    /// </summary>
    void set_udp_length(uint32_t length) noexcept {
        udp_length_ = length;
    }

    /// <summary>
    /// Gets the maximum length of a UDP packet that the other
    /// party in the session can receive.
    /// </summary>
    [[nodiscard]] uint32_t remote_udp_length() const noexcept {
        return remote_udp_length_;
    }

    /// <summary>
    /// Sets the maximum length of a UDP packet that the other
    /// party in the session can receive.
    /// </summary>
    void set_remote_udp_length(uint32_t length) noexcept {
        remote_udp_length_ = length;
    }

    /// <summary>
    /// Gets the seed used to calculate packet CRC hashes.
    /// </summary>
    [[nodiscard]] uint32_t crc_seed() const noexcept {
        return crc_seed_;
    }

    /// <summary>
    /// Sets the seed used to calculate packet CRC hashes.
    /// This also updates the crc_state.
    /// </summary>
    void set_crc_seed(uint32_t seed);

    /// <summary>
    /// Gets the Crc32 state used to calculate CRC hashes for this session.
    /// This property is auto-set when crc_seed is set.
    /// </summary>
    [[nodiscard]] const std::shared_ptr<sanctuary::soe_protocol::services::Crc32>& crc_state() const noexcept {
        return crc_state_;
    }

    /// <summary>
    /// Sets the Crc32 state used to calculate CRC hashes for this session.
    /// </summary>
    void set_crc_state(std::shared_ptr<sanctuary::soe_protocol::services::Crc32> state) noexcept {
        crc_state_ = std::move(state);
    }

    /// <summary>
    /// Gets the number of bytes used to store a packet CRC hash.
    /// Must be between 0 and 4, inclusive.
    /// </summary>
    [[nodiscard]] uint8_t crc_length() const noexcept {
        return crc_length_;
    }

    /// <summary>
    /// Sets the number of bytes used to store a packet CRC hash.
    /// Must be between 0 and 4, inclusive.
    /// </summary>
    void set_crc_length(uint8_t length) noexcept {
        crc_length_ = length;
    }

    /// <summary>
    /// Gets a value indicating whether compression is enabled
    /// for the session.
    /// </summary>
    [[nodiscard]] bool is_compression_enabled() const noexcept {
        return is_compression_enabled_;
    }

    /// <summary>
    /// Sets a value indicating whether compression is enabled
    /// for the session.
    /// </summary>
    void set_compression_enabled(bool enabled) noexcept {
        is_compression_enabled_ = enabled;
    }

    /// <summary>
    /// Gets the maximum number of raw packets that may be queued for either processing or sending.
    /// </summary>
    [[nodiscard]] int32_t max_queued_raw_packets() const noexcept {
        return max_queued_raw_packets_;
    }

    /// <summary>
    /// Sets the maximum number of raw packets that may be queued for either processing or sending.
    /// </summary>
    void set_max_queued_raw_packets(int32_t count) noexcept {
        max_queued_raw_packets_ = count;
    }

    /// <summary>
    /// Gets the maximum number of data fragments that may be queued for either stitching or dispatch.
    /// </summary>
    [[nodiscard]] int16_t max_queued_incoming_reliable_data_packets() const noexcept {
        return max_queued_incoming_reliable_data_packets_;
    }

    /// <summary>
    /// Sets the maximum number of data fragments that may be queued for either stitching or dispatch.
    /// </summary>
    void set_max_queued_incoming_reliable_data_packets(int16_t count) noexcept {
        max_queued_incoming_reliable_data_packets_ = count;
    }

    /// <summary>
    /// Gets the maximum number of reliable data fragments that may be queued for output.
    /// </summary>
    [[nodiscard]] int16_t max_queued_outgoing_reliable_data_packets() const noexcept {
        return max_queued_outgoing_reliable_data_packets_;
    }

    /// <summary>
    /// Sets the maximum number of reliable data fragments that may be queued for output.
    /// </summary>
    void set_max_queued_outgoing_reliable_data_packets(int16_t count) noexcept {
        max_queued_outgoing_reliable_data_packets_ = count;
    }

    /// <summary>
    /// Gets the data acknowledgement window.
    /// </summary>
    [[nodiscard]] int16_t data_ack_window() const noexcept {
        return data_ack_window_;
    }

    /// <summary>
    /// Sets the data acknowledgement window.
    /// </summary>
    void set_data_ack_window(int16_t window) noexcept {
        data_ack_window_ = window;
    }

    /// <summary>
    /// Gets the timespan after which to send a heartbeat, if no contextual packets have been received within the
    /// interval. Set to std::chrono::seconds::zero() to disable heart-beating.
    /// </summary>
    [[nodiscard]] std::chrono::milliseconds heartbeat_after() const noexcept {
        return heartbeat_after_;
    }

    /// <summary>
    /// Sets the timespan after which to send a heartbeat.
    /// </summary>
    void set_heartbeat_after(std::chrono::milliseconds duration) noexcept {
        heartbeat_after_ = duration;
    }

    /// <summary>
    /// Gets the default timespan after which to consider a session inactive, if no contextual packets have been
    /// received within the interval. Set to std::chrono::seconds::zero() to prevent a session from being terminated
    /// due to inactivity.
    /// </summary>
    [[nodiscard]] std::chrono::milliseconds inactivity_timeout() const noexcept {
        return inactivity_timeout_;
    }

    /// <summary>
    /// Sets the default timespan after which to consider a session inactive.
    /// </summary>
    void set_inactivity_timeout(std::chrono::milliseconds timeout) noexcept {
        inactivity_timeout_ = timeout;
    }

    /// <summary>
    /// Gets a value indicating whether all data packets should be acknowledged.
    /// </summary>
    [[nodiscard]] bool acknowledge_all_data() const noexcept {
        return acknowledge_all_data_;
    }

    /// <summary>
    /// Sets a value indicating whether all data packets should be acknowledged.
    /// </summary>
    void set_acknowledge_all_data(bool acknowledge) noexcept {
        acknowledge_all_data_ = acknowledge;
    }

    /// <summary>
    /// Gets the maximum amount of time that may elapse before acknowledging incoming reliable data sequences.
    /// </summary>
    [[nodiscard]] std::chrono::milliseconds maximum_acknowledge_delay() const noexcept {
        return maximum_acknowledge_delay_;
    }

    /// <summary>
    /// Sets the maximum amount of time that may elapse before acknowledging incoming reliable data sequences.
    /// </summary>
    void set_maximum_acknowledge_delay(std::chrono::milliseconds delay) noexcept {
        maximum_acknowledge_delay_ = delay;
    }

    /// <summary>
    /// Creates a shallow copy of this SessionParameters instance.
    /// </summary>
    /// <returns>The copied instance.</returns>
    [[nodiscard]] std::unique_ptr<SessionParameters> clone() const;

private:
    std::string application_protocol_;
    uint32_t udp_length_{512};  // SoeConstants::DEFAULT_UDP_LENGTH
    uint32_t remote_udp_length_{0};
    uint32_t crc_seed_{0};
    std::shared_ptr<sanctuary::soe_protocol::services::Crc32> crc_state_;
    uint8_t crc_length_{2};  // SoeConstants::CRC_LENGTH
    bool is_compression_enabled_{false};
    int32_t max_queued_raw_packets_{512};
    int16_t max_queued_incoming_reliable_data_packets_{256};
    int16_t max_queued_outgoing_reliable_data_packets_{196};
    int16_t data_ack_window_{32};
    std::chrono::milliseconds heartbeat_after_{std::chrono::seconds(25)};  // SoeConstants::DEFAULT_SESSION_HEARTBEAT_AFTER
    std::chrono::milliseconds inactivity_timeout_{std::chrono::seconds(30)};  // SoeConstants::DEFAULT_SESSION_INACTIVITY_TIMEOUT
    bool acknowledge_all_data_{false};
    std::chrono::milliseconds maximum_acknowledge_delay_{2};
};

} // namespace sanctuary::soe_protocol::objects
