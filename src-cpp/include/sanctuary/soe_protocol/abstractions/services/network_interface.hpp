#pragma once

#include "network_reader.hpp"
#include "network_writer.hpp"

namespace sanctuary::soe_protocol::abstractions::services {

/// <summary>
/// Represents a network interface.
/// </summary>
class NetworkInterface : public NetworkReader, public NetworkWriter {
public:
    virtual ~NetworkInterface() = default;
};

} // namespace sanctuary::soe_protocol::abstractions::services
