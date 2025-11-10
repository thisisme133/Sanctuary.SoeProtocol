#include "sanctuary/soe_protocol/soe_protocol_handler.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge.hpp"
#include "sanctuary/soe_protocol/objects/packets/acknowledge_all.hpp"
#include "sanctuary/soe_protocol/objects/packets/disconnect.hpp"
#include "sanctuary/soe_protocol/objects/packets/remap_connection.hpp"
#include "sanctuary/soe_protocol/objects/packets/session_request.hpp"
#include "sanctuary/soe_protocol/objects/packets/session_response.hpp"
#include "sanctuary/soe_protocol/soe_constants.hpp"
#include "sanctuary/soe_protocol/util/data_utils.hpp"
#include "sanctuary/soe_protocol/util/multi_packet_utils.hpp"
#include "sanctuary/soe_protocol/util/native_span_pool.hpp"
#include "sanctuary/soe_protocol/util/soe_packet_utils.hpp"
#include <algorithm>
#include <cstring>
#include <random>
#include <stdexcept>

namespace sanctuary::soe_protocol {

SoeProtocolHandler::SoeProtocolHandler(
    EndPoint remote,
    objects::SessionMode mode,
    std::unique_ptr<objects::SessionParameters> session_parameters,
    util::NativeSpanPool& span_pool,
    std::unique_ptr<abstractions::services::NetworkWriter> network_writer,
    std::unique_ptr<abstractions::ApplicationProtocolHandler> application
)
    : span_pool_(span_pool)
    , network_writer_(std::move(network_writer))
    , application_(std::move(application))
    , remote_(std::move(remote))
    , session_params_(std::move(session_parameters))
    , application_params_(application_->session_params())
    , mode_(mode)
    , state_(objects::SessionState::Negotiating)
    , last_received_packet_time_(std::chrono::steady_clock::now())
    , contextual_send_buffer_(session_params_->udp_length())
{
    // Create the data input channel
    data_input_channel_ = std::make_unique<services::ReliableDataInputChannel>(
        *this,
        *session_params_,
        application_params_,
        span_pool_,
        [this](std::span<const uint8_t> data) {
            application_->handle_app_data(data);
        }
    );

    // Create the data output channel
    data_output_channel_ = std::make_unique<services::ReliableDataOutputChannel>(
        *this,
        span_pool_,
        calculate_max_data_length()
    );

    // Initialize the application
    application_->initialise(*this);
}

SoeProtocolHandler::~SoeProtocolHandler() {
    // Clean up the packet queue
    std::lock_guard<std::mutex> lock(packet_queue_mutex_);
    while (!packet_queue_.empty()) {
        span_pool_.return_span(std::move(packet_queue_.front()));
        packet_queue_.pop();
    }
}

bool SoeProtocolHandler::enqueue_packet(std::unique_ptr<objects::NativeSpan> packet_data) {
    std::lock_guard<std::mutex> lock(packet_queue_mutex_);

    if (packet_queue_.size() >= session_params_->max_queued_raw_packets()) {
        return false;
    }

    packet_queue_.push(std::move(packet_data));
    return true;
}

void SoeProtocolHandler::initialize() {
    last_received_packet_time_ = std::chrono::steady_clock::now();
}

bool SoeProtocolHandler::run_tick(bool& needs_more_time, std::stop_token stop_token) {
    needs_more_time = false;

    if (state_ == objects::SessionState::Terminated) {
        return false;
    }

    needs_more_time = process_one_from_packet_queue();
    send_heartbeat_if_required();

    auto elapsed = std::chrono::steady_clock::now() - last_received_packet_time_;
    if (elapsed > session_params_->inactivity_timeout()) {
        terminate_session(objects::DisconnectReason::Timeout, false);
        return false;
    }

    data_input_channel_->run_tick();
    data_output_channel_->run_tick(stop_token);

    return true;
}

bool SoeProtocolHandler::enqueue_data(std::span<const uint8_t> data) {
    if (state_ != objects::SessionState::Running) {
        return false;
    }

    data_output_channel_->enqueue_data(data);
    return true;
}

void SoeProtocolHandler::terminate_session() {
    terminate_session(objects::DisconnectReason::Application, true);
}

void SoeProtocolHandler::terminate_session(
    objects::DisconnectReason reason,
    bool notify_remote,
    bool terminated_by_remote
) {
    if (state_ == objects::SessionState::Terminated) {
        return;
    }

    try {
        // Naive flush of the output channel
        data_output_channel_->run_tick(std::stop_token());
        termination_reason_ = reason;

        if (notify_remote && state_ == objects::SessionState::Running) {
            objects::packets::Disconnect disconnect{session_id_, reason};
            std::vector<uint8_t> buffer(objects::packets::Disconnect::SIZE);
            disconnect.serialize(buffer);
            send_contextual_packet(SoeOpCode::Disconnect, buffer);
        }
    } catch (...) {
        // Ensure state is set even if something goes wrong
    }

    state_ = objects::SessionState::Terminated;
    terminated_by_remote_ = terminated_by_remote;
    application_->on_session_closed(reason);
}

bool SoeProtocolHandler::process_one_from_packet_queue() {
    std::unique_ptr<objects::NativeSpan> packet;

    {
        std::lock_guard<std::mutex> lock(packet_queue_mutex_);
        if (packet_queue_.empty()) {
            return false;
        }
        packet = std::move(packet_queue_.front());
        packet_queue_.pop();
    }

    auto used = packet->used_span();
    std::vector<uint8_t> buffer(used.begin(), used.end());
    process_packet_core(buffer, true);
    span_pool_.return_span(std::move(packet));
    return true;
}

void SoeProtocolHandler::process_packet_core(
    std::span<uint8_t> packet_data,
    bool validate,
    SoeOpCode op_code
) {
    if (validate) {
        SoeOpCode validated_op_code = SoeOpCode::Invalid;
        auto validation_result = util::SoePacketUtils::validate_packet(
            packet_data,
            *session_params_,
            validated_op_code
        );

        if (validation_result != objects::SoePacketValidationResult::Valid) {
            terminate_session(objects::DisconnectReason::CorruptPacket, true);
            return;
        }
        op_code = validated_op_code;
    }

    if (open_session_on_next_client_packet_) {
        application_->on_session_opened();
        open_session_on_next_client_packet_ = false;
    }

    // We set this after packet validation as a primitive method of stopping the connection
    // if all we've received is multiple corrupt packets in a row
    last_received_packet_time_ = std::chrono::steady_clock::now();
    packet_data = packet_data.subspan(sizeof(uint16_t)); // Skip opcode
    bool is_sessionless = util::SoePacketUtils::is_contextless_packet(op_code);

    if (is_sessionless) {
        handle_contextless_packet(op_code, packet_data);
    } else {
        // Remove CRC from the end
        auto crc_len = session_params_->crc_length();
        if (packet_data.size() >= crc_len) {
            packet_data = packet_data.subspan(0, packet_data.size() - crc_len);
        }
        handle_contextual_packet(op_code, packet_data);
    }
}

int32_t SoeProtocolHandler::calculate_max_data_length() const {
    return static_cast<int32_t>(session_params_->udp_length())
        - static_cast<int32_t>(sizeof(uint16_t)) // SoeOpCode
        - (session_params_->is_compression_enabled() ? 1 : 0)
        - static_cast<int32_t>(session_params_->crc_length());
}

// Contextless packet handling

void SoeProtocolHandler::send_session_request() {
    if (state_ != objects::SessionState::Negotiating) {
        throw std::runtime_error("Can only send a session request while in the Negotiating state");
    }

    if (mode_ != objects::SessionMode::Client) {
        throw std::runtime_error("Can only send a session request while in the Client mode");
    }

    // Generate random session ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;
    uint32_t id = dist(gen);

    objects::packets::SessionRequest request{
        SoeConstants::SOE_PROTOCOL_VERSION,
        id,
        session_params_->udp_length(),
        session_params_->application_protocol()
    };

    size_t packet_size = request.get_size();
    // Unfortunately we can only guarantee our UDP length here, and not the remote's
    if (packet_size > session_params_->udp_length()) {
        throw std::runtime_error("The application_protocol string is too long");
    }

    std::vector<uint8_t> buffer(packet_size);
    request.serialize(buffer);
    network_writer_->send(buffer);
}

void SoeProtocolHandler::handle_contextless_packet(
    SoeOpCode op_code,
    std::span<const uint8_t> packet_data
) {
    switch (op_code) {
        case SoeOpCode::SessionRequest:
            handle_session_request(packet_data);
            break;
        case SoeOpCode::SessionResponse:
            handle_session_response(packet_data);
            break;
        case SoeOpCode::UnknownSender:
            // TODO: Request a remap here
            terminate_session(objects::DisconnectReason::UnreachableConnection, false);
            break;
        case SoeOpCode::RemapConnection:
            throw std::runtime_error("Remap requests must be handled by the connection manager");
        default:
            throw std::runtime_error("handle_contextless_packet cannot handle this packet type");
    }
}

void SoeProtocolHandler::handle_session_request(std::span<const uint8_t> packet_data) {
    if (mode_ == objects::SessionMode::Client) {
        terminate_session(objects::DisconnectReason::ConnectingToSelf, false);
        return;
    }

    auto request = objects::packets::SessionRequest::deserialize(packet_data, false);
    session_params_->set_remote_udp_length(request.udp_length);
    session_id_ = request.session_id;

    if (state_ != objects::SessionState::Negotiating) {
        terminate_session(objects::DisconnectReason::ConnectError, true);
        return;
    }

    bool protocols_match = request.soe_protocol_version == SoeConstants::SOE_PROTOCOL_VERSION
        && request.application_protocol == session_params_->application_protocol();

    if (!protocols_match) {
        terminate_session(objects::DisconnectReason::ProtocolMismatch, true);
        return;
    }

    // Generate random CRC seed
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;

    session_params_->set_crc_length(SoeConstants::CRC_LENGTH);
    session_params_->set_crc_seed(dist(gen));
    data_output_channel_->set_max_data_length(calculate_max_data_length());

    objects::packets::SessionResponse response{
        session_id_,
        session_params_->crc_seed(),
        session_params_->crc_length(),
        session_params_->is_compression_enabled(),
        0,
        session_params_->udp_length(),
        SoeConstants::SOE_PROTOCOL_VERSION
    };

    std::vector<uint8_t> buffer(objects::packets::SessionResponse::SIZE);
    response.serialize(buffer);
    network_writer_->send(buffer);

    state_ = objects::SessionState::Running;
    open_session_on_next_client_packet_ = true;
}

void SoeProtocolHandler::handle_session_response(std::span<const uint8_t> packet_data) {
    if (mode_ == objects::SessionMode::Server) {
        terminate_session(objects::DisconnectReason::ConnectingToSelf, false);
        return;
    }

    auto response = objects::packets::SessionResponse::deserialize(packet_data, false);
    session_params_->set_remote_udp_length(response.udp_length);
    session_params_->set_crc_length(response.crc_length);
    session_params_->set_crc_seed(response.crc_seed);
    session_params_->set_is_compression_enabled(response.is_compression_enabled);
    session_id_ = response.session_id;
    data_output_channel_->set_max_data_length(calculate_max_data_length());

    if (state_ != objects::SessionState::Negotiating) {
        terminate_session(objects::DisconnectReason::ConnectError, true);
        return;
    }

    if (response.soe_protocol_version != SoeConstants::SOE_PROTOCOL_VERSION) {
        terminate_session(objects::DisconnectReason::ProtocolMismatch, true);
        return;
    }

    state_ = objects::SessionState::Running;
    application_->on_session_opened();
}

// Contextual packet handling

void SoeProtocolHandler::send_contextual_packet(
    SoeOpCode op_code,
    std::span<const uint8_t> packet_data
) {
    size_t extra_bytes = sizeof(uint16_t)
        + (session_params_->is_compression_enabled() ? 1 : 0)
        + session_params_->crc_length();

    if (packet_data.size() + extra_bytes > session_params_->remote_udp_length()) {
        throw std::runtime_error("Cannot send a packet larger than the remote UDP length");
    }

    // Build the packet
    std::vector<uint8_t> buffer;
    buffer.reserve(packet_data.size() + extra_bytes);

    // Write opcode
    uint16_t op_code_value = static_cast<uint16_t>(op_code);
    buffer.push_back(static_cast<uint8_t>(op_code_value >> 8));
    buffer.push_back(static_cast<uint8_t>(op_code_value & 0xFF));

    // Write compression flag if enabled
    if (session_params_->is_compression_enabled()) {
        buffer.push_back(0); // Compression is not implemented at the moment
    }

    // Write packet data
    buffer.insert(buffer.end(), packet_data.begin(), packet_data.end());

    // Append CRC
    util::SoePacketUtils::append_crc(buffer, session_params_->crc_state(), session_params_->crc_length());

    network_writer_->send(buffer);
}

void SoeProtocolHandler::handle_contextual_packet(
    SoeOpCode op_code,
    std::span<uint8_t> packet_data
) {
    std::vector<uint8_t> decompressed_data;
    std::span<uint8_t> working_data = packet_data;

    if (session_params_->is_compression_enabled()) {
        bool is_compressed = packet_data[0] > 0;
        working_data = packet_data.subspan(1);

        if (is_compressed) {
            decompressed_data = util::SoePacketUtils::decompress(working_data);
            working_data = decompressed_data;
        }
    }

    handle_contextual_packet_internal(op_code, working_data);
}

void SoeProtocolHandler::handle_contextual_packet_internal(
    SoeOpCode op_code,
    std::span<uint8_t> packet_data
) {
    switch (op_code) {
        case SoeOpCode::MultiPacket: {
            size_t offset = 0;
            while (offset < packet_data.size()) {
                uint32_t sub_packet_length = util::MultiPacketUtils::read_variable_length(
                    packet_data,
                    offset
                );

                if (sub_packet_length < sizeof(uint16_t) || sub_packet_length > packet_data.size() - offset) {
                    terminate_session(objects::DisconnectReason::CorruptPacket, true);
                    return;
                }

                SoeOpCode sub_packet_op_code = util::SoePacketUtils::read_soe_op_code(
                    packet_data.subspan(offset)
                );

                auto sub_data = packet_data.subspan(
                    offset + sizeof(uint16_t),
                    sub_packet_length - sizeof(uint16_t)
                );

                handle_contextual_packet_internal(sub_packet_op_code, sub_data);
                offset += sub_packet_length;
            }
            break;
        }
        case SoeOpCode::Disconnect: {
            auto disconnect = objects::packets::Disconnect::deserialize(packet_data);
            terminate_session(disconnect.reason, false, true);
            break;
        }
        case SoeOpCode::Heartbeat: {
            if (mode_ == objects::SessionMode::Server) {
                send_contextual_packet(SoeOpCode::Heartbeat, {});
            }
            // Otherwise ignore, no need to hit the default clause
            break;
        }
        case SoeOpCode::ReliableData: {
            data_input_channel_->handle_reliable_data(packet_data);
            break;
        }
        case SoeOpCode::ReliableDataFragment: {
            data_input_channel_->handle_reliable_data_fragment(packet_data);
            break;
        }
        case SoeOpCode::Acknowledge: {
            auto ack = objects::packets::Acknowledge::deserialize(packet_data);
            data_output_channel_->notify_of_acknowledge(ack);
            break;
        }
        case SoeOpCode::AcknowledgeAll: {
            auto ack_all = objects::packets::AcknowledgeAll::deserialize(packet_data);
            data_output_channel_->notify_of_acknowledge_all(ack_all);
            break;
        }
        default:
            throw std::runtime_error("The contextual handler does not support this packet type");
    }
}

void SoeProtocolHandler::send_heartbeat_if_required() {
    bool may_send_heartbeat = mode_ == objects::SessionMode::Client
        && state_ == objects::SessionState::Running
        && session_params_->heartbeat_after() != std::chrono::milliseconds::zero()
        && (std::chrono::steady_clock::now() - last_received_packet_time_) > session_params_->heartbeat_after();

    if (may_send_heartbeat) {
        send_contextual_packet(SoeOpCode::Heartbeat, {});
    }
}

} // namespace sanctuary::soe_protocol
