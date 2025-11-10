#include "sanctuary/soe_protocol/objects/session_parameters.hpp"

// Forward declaration - to be implemented
// #include "sanctuary/soe_protocol/services/crc32.hpp"

namespace sanctuary::soe_protocol::objects {

void SessionParameters::set_crc_seed(uint32_t seed) {
    crc_seed_ = seed;
    // TODO: Uncomment when Crc32 is implemented
    // crc_state_ = std::make_shared<sanctuary::soe_protocol::services::Crc32>(seed);
}

std::unique_ptr<SessionParameters> SessionParameters::clone() const {
    auto cloned = std::make_unique<SessionParameters>();

    cloned->application_protocol_ = application_protocol_;
    cloned->udp_length_ = udp_length_;
    cloned->remote_udp_length_ = remote_udp_length_;
    cloned->crc_seed_ = crc_seed_;
    cloned->crc_state_ = crc_state_;  // Shallow copy
    cloned->crc_length_ = crc_length_;
    cloned->is_compression_enabled_ = is_compression_enabled_;
    cloned->max_queued_raw_packets_ = max_queued_raw_packets_;
    cloned->max_queued_incoming_reliable_data_packets_ = max_queued_incoming_reliable_data_packets_;
    cloned->max_queued_outgoing_reliable_data_packets_ = max_queued_outgoing_reliable_data_packets_;
    cloned->data_ack_window_ = data_ack_window_;
    cloned->heartbeat_after_ = heartbeat_after_;
    cloned->inactivity_timeout_ = inactivity_timeout_;
    cloned->acknowledge_all_data_ = acknowledge_all_data_;
    cloned->maximum_acknowledge_delay_ = maximum_acknowledge_delay_;

    return cloned;
}

} // namespace sanctuary::soe_protocol::objects
