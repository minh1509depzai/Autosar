#ifndef DOIP_ROUTING_ACTIVATION_HPP
#define DOIP_ROUTING_ACTIVATION_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "../../../DoIPMessage/Implementation/ExportHeaderInterface/DoIPMessage.hpp"

namespace as {
namespace doip {
namespace routing_activation {

using ConnectionId = std::uint64_t;

class RoutingActivationMessage final : public DoIPMessage 
{
public:
    // Applies the generic DoIP header checks first, then the Routing
    // Activation payload checks. A valid message of another payload type is
    // Accepted but is_routing_activation_request() is false.
    AcknowledgeCode parse(  const std::vector<std::uint8_t>& message,
                            std::uint8_t expected_protocol_version,
                            const ParseLimits& limits ) override;

    bool is_routing_activation_request() const;
    std::uint16_t tester_address() const;
    std::uint8_t activation_type() const;
    const std::vector<std::uint8_t>& oem_data() const;

private:
    bool is_routing_activation_request_ = false;
    std::uint16_t tester_address_ = 0;
    std::uint8_t activation_type_ = 0;
    std::vector<std::uint8_t> oem_data_;
};

/*
* @SWS_DoIP_00102
* @SWS_DoIP_00104
* @SWS_DoIP_00106
* @SWS_DoIP_00108
*/
enum class RoutingActivationResponseCode : std::uint8_t 
{
  UnknownSourceAddress = 0x00,
  AllConnectionsOccupied = 0x01,
  ConnectionAlreadyRegistered = 0x02,
  SourceAddressAlreadyRegistered = 0x03,
  AuthenticationDenied = 0x04,
  ConfirmationDenied = 0x05,
  UnsupportedActivationType = 0x06,
  SuccessfullyActivated = 0x10,
  ConfirmationPending = 0x11
};

enum class CallbackStatus {
  Success,
  Pending,
  Rejected
};

struct CallbackResult {
  CallbackStatus status;
  std::vector<std::uint8_t> response_data;
};

class ActivationCallback {
public:
  virtual ~ActivationCallback() = default;

  virtual CallbackResult invoke(ConnectionId connection_id,
                                std::uint16_t tester_address,
                                std::uint8_t activation_type,
                                const std::vector<std::uint8_t>& request_data) = 0;
};

class AuthenticationCallback : public ActivationCallback {
public:
  ~AuthenticationCallback() override = default;
};

class ConfirmationCallback : public ActivationCallback {
public:
  ~ConfirmationCallback() override = default;
};

class RoutingActivationHandler;

class RoutingActivationCommunication {
public:
    virtual ~RoutingActivationCommunication() = default;

    // The SoAd adapter must queue the complete DoIP message for this connection.
    virtual bool send_message(ConnectionId connection_id,
                                const std::vector<std::uint8_t>& message) = 0;

    // @SWS_DoIP_00105: Check on that connection and wait for its response or DoIPAliveCheckResponseTimeout
    //                     
    virtual bool send_alive_check(ConnectionId connection_id) = 0;
    virtual void close_connection(ConnectionId connection_id) = 0;

    // The adapter forwards SoAd connection, receive, and close callbacks to this
    // handler. It must only store the handler here, not invoke it synchronously.
    // A null handler detaches the receiver.
    virtual void set_receiver(RoutingActivationHandler* receiver) = 0;
};

using RoutingActivationTransport = RoutingActivationCommunication;

struct RoutingActivationProfile {
    std::uint8_t activation_type = 0;
    std::vector<std::uint16_t> target_addresses;
    bool security_required = false;

    std::uint8_t authentication_request_length = 0;
    std::uint8_t authentication_response_length = 0;
    std::shared_ptr<AuthenticationCallback> authentication_callback;

    std::uint8_t confirmation_request_length = 0;
    std::uint8_t confirmation_response_length = 0;
    std::shared_ptr<ConfirmationCallback> confirmation_callback;
};

struct TesterConfiguration {
    std::uint16_t source_address = 0;
    std::vector<RoutingActivationProfile> activations;
};

struct RoutingActivationSettings {
    std::uint8_t protocol_version = 0x02;
    std::uint16_t entity_address = 0;
    std::size_t maximum_tester_connections = 1;
    std::chrono::milliseconds alive_check_response_timeout{1000};
    std::vector<TesterConfiguration> testers;
};

class RoutingActivationHandler {
public:
    virtual ~RoutingActivationHandler() = default;

    // These methods are the inbound event boundary from the communication
    // adapter; the implementation contains no SoAd-specific types or calls.
    // SWS_DoIP_00101: The caller must register only configured DoIPTcpConnection
    // instances; Routing Activation is not valid on UDP.
    virtual bool on_connection_opened(ConnectionId connection_id,
                                        bool secure_transport) = 0;
    // The transport/framer passes one complete DoIP message per call.
    // Messages other than Routing Activation Requests are ignored.
    virtual void on_message_received(ConnectionId connection_id,
                                    const std::vector<std::uint8_t>& message) = 0;
    virtual void on_alive_check_response(ConnectionId connection_id,
                                        std::uint16_t tester_address) = 0;
    virtual void on_connection_closed(ConnectionId connection_id) = 0;
    virtual void main_function() = 0;
    virtual bool may_route_diagnostic(ConnectionId connection_id,
                                        std::uint16_t source_address,
                                        std::uint16_t target_address) const = 0;
};

class RoutingActivationManager final : public RoutingActivationHandler {
public:
    RoutingActivationManager(
        const RoutingActivationSettings& settings,
        const std::shared_ptr<RoutingActivationCommunication>& communication);
    ~RoutingActivationManager() override;

    RoutingActivationManager(const RoutingActivationManager&) = delete;
    RoutingActivationManager& operator=(const RoutingActivationManager&) = delete;
    RoutingActivationManager(RoutingActivationManager&&) = delete;
    RoutingActivationManager& operator=(RoutingActivationManager&&) = delete;

    bool on_connection_opened(ConnectionId connection_id,
                                bool secure_transport) override;
    void on_message_received(ConnectionId connection_id,
                            const std::vector<std::uint8_t>& message) override;
    void on_alive_check_response(ConnectionId connection_id,
                                std::uint16_t tester_address) override;
    void on_connection_closed(ConnectionId connection_id) override;
    void main_function() override;
    bool may_route_diagnostic(ConnectionId connection_id,
                                std::uint16_t source_address,
                                std::uint16_t target_address) const override;

private:
    enum class ConnectionState {
        Connected,
        CheckingAlive,
        AuthenticationPending,
        ConfirmationPending,
        Registered,
        Active
    };

    enum class AliveCheckPurpose {
        DuplicateAddress,
        Capacity
    };

    struct ActivationRequest {
        std::uint16_t tester_address = 0;
        std::uint8_t activation_type = 0;
        std::vector<std::uint8_t> oem_data;
    };

    struct Connection {
        bool secure_transport = false;
        bool tester_registered = false;
        std::uint16_t tester_address = 0;
        ConnectionState state = ConnectionState::Connected;
        ActivationRequest request;
        const RoutingActivationProfile* profile = nullptr;
        std::vector<std::uint8_t> authentication_response;
        std::vector<std::uint8_t> confirmation_response;
        bool confirmation_pending_response_sent = false;
        std::vector<std::uint16_t> permitted_targets;
    };

    struct PendingAliveCheck {
        AliveCheckPurpose purpose;
        std::vector<ConnectionId> probed_connections;
        std::set<ConnectionId> responding_connections;
        std::set<ConnectionId> failed_connections;
        std::chrono::steady_clock::time_point deadline;
    };

    void validate_settings() const;
    void validate_and_start_activation(ConnectionId connection_id);
    void start_alive_check(ConnectionId requester_id,
                            AliveCheckPurpose purpose,
                            const std::vector<ConnectionId>& connections);
    void finish_alive_check(ConnectionId requester_id);
    void continue_activation(ConnectionId connection_id);
    void process_authentication(ConnectionId connection_id);
    void process_confirmation(ConnectionId connection_id);
    void activate_routing(ConnectionId connection_id);

    const TesterConfiguration* find_tester(std::uint16_t source_address) const;
    const RoutingActivationProfile* find_profile(
        const TesterConfiguration& tester,
        std::uint8_t activation_type) const;
    std::vector<ConnectionId> registered_connections(
        ConnectionId excluded_connection) const;
    std::size_t registered_connection_count() const;

    bool send_response(ConnectionId connection_id,
                        RoutingActivationResponseCode response_code);
    void reject_and_close(ConnectionId connection_id,
                            RoutingActivationResponseCode response_code);
    void close_connection(ConnectionId connection_id);
    void erase_connection(ConnectionId connection_id);
    std::vector<std::uint8_t> build_response(
        const Connection& connection,
        RoutingActivationResponseCode response_code) const;
    
    RoutingActivationSettings settings_;
    std::shared_ptr<RoutingActivationCommunication> communication_;
    std::unordered_map<ConnectionId, Connection> connections_;
    std::unordered_map<ConnectionId, PendingAliveCheck> pending_alive_checks_;
};

} // namespace routing_activation
} // namespace doip
} // namespace as

#endif