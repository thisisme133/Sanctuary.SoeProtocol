#pragma once

#include <cstdint>

namespace sanctuary::soe_protocol::objects {

/// <summary>
/// Contains statistic values related to data input.
/// </summary>
class DataInputStats {
public:
    /// <summary>
    /// The total number of reliable data packets received, including duplicates.
    /// </summary>
    int32_t total_received{0};

    /// <summary>
    /// The number of duplicate reliable data packets received.
    /// </summary>
    int32_t duplicate_count{0};

    /// <summary>
    /// The number of reliable data packets that were received out of order.
    /// </summary>
    int32_t out_of_order_count{0};

    /// <summary>
    /// The total number of bytes received.
    /// </summary>
    /// <remarks>
    /// This value only includes the raw data count (i.e. not multi-data indicators, encryption padding, etc.).
    /// </remarks>
    int64_t total_received_bytes{0};

    /// <summary>
    /// The number of reliable data packets that were acknowledged.
    /// </summary>
    int32_t acknowledge_count{0};

    DataInputStats() = default;
    ~DataInputStats() = default;
    DataInputStats(const DataInputStats&) = default;
    DataInputStats(DataInputStats&&) noexcept = default;
    DataInputStats& operator=(const DataInputStats&) = default;
    DataInputStats& operator=(DataInputStats&&) noexcept = default;
};

} // namespace sanctuary::soe_protocol::objects
