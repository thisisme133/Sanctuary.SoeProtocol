#pragma once

#include <cstdint>

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Contains statistical values related to data output.
/// </summary>
class DataOutputStats {
public:
    /// <summary>
    /// The total number of reliable data packets sent, including re-sent packets.
    /// </summary>
    int32_t total_sent_reliable_packets{0};

    /// <summary>
    /// The total number of reliable data packets that were re-sent.
    /// </summary>
    int32_t total_resent_reliable_packets{0};

    /// <summary>
    /// The total number of received acknowledgement packets (incl. ack all).
    /// </summary>
    int32_t incoming_acknowledge_count{0};

    /// <summary>
    /// The total number of reliable data packets that were acknowledged (i.e. including packets ack'ed by an ack-all).
    /// </summary>
    int32_t actual_acknowledge_count{0};

    DataOutputStats() = default;
    ~DataOutputStats() = default;
    DataOutputStats(const DataOutputStats&) = default;
    DataOutputStats(DataOutputStats&&) noexcept = default;
    DataOutputStats& operator=(const DataOutputStats&) = default;
    DataOutputStats& operator=(DataOutputStats&&) noexcept = default;
};

} // namespace sanctuary::soe_protocol::objects
