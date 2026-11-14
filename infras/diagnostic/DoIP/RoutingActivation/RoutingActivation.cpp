#include "RoutingActivation.hpp"

#include <algorithm>
#include <stdexcept>

namespace as {
namespace doip {
namespace routing_activation {
namespace {

const std::uint16_t kRoutingActivationResponseType = 0x0006;
const std::size_t kDoipHeaderLength = 8;
const std::size_t kRoutingActivationRequestBaseLength = 7;
const std::size_t kRoutingActivationRequestOemLength = 11;
const std::size_t kRoutingActivationResponseBaseLength = 9;

std::uint16_t read_u16_be(const std::uint8_t *bytes) {
  return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[0]) << 8) |
                                    static_cast<std::uint16_t>(bytes[1]));
}

void write_u16_be(std::uint8_t *bytes, std::uint16_t value) {
  bytes[0] = static_cast<std::uint8_t>(value >> 8);
  bytes[1] = static_cast<std::uint8_t>(value);
}

void write_u32_be(std::uint8_t *bytes, std::uint32_t value) {
  bytes[0] = static_cast<std::uint8_t>(value >> 24);
  bytes[1] = static_cast<std::uint8_t>(value >> 16);
  bytes[2] = static_cast<std::uint8_t>(value >> 8);
  bytes[3] = static_cast<std::uint8_t>(value);
}

bool has_target(const std::vector<std::uint16_t> &targets, std::uint16_t target_address) {
  return std::find(targets.begin(), targets.end(), target_address) != targets.end();
}

} // namespace

AcknowledgeCode RoutingActivationMessage::parse( const std::vector<std::uint8_t> &message,
                                                 std::uint8_t expected_protocol_version,
                                                 const ParseLimits &limits )
{
    is_routing_activation_request_ = false;
    tester_address_ = 0;
    activation_type_ = 0;
    oem_data_.clear();

    const AcknowledgeCode header_result =
        DoIPMessage::parse(message, expected_protocol_version, limits);
    if (header_result != AcknowledgeCode::Accepted) {
        return header_result;
    }
    if (mPayloadType != static_cast<std::uint16_t>(PayloadType::RoutingActivationRequest)) {
        return AcknowledgeCode::Accepted;
    }

    // SWS_DoIP_00117: Routing Activation Request payload length is 7 or 11.
    if (payload_.size() != kRoutingActivationRequestBaseLength &&
        payload_.size() != kRoutingActivationRequestOemLength) {
        return AcknowledgeCode::InvalidPayloadLength;
    }

    tester_address_ = read_u16_be(payload_.data());
    activation_type_ = payload_[2];
    for (std::size_t index = 3; index < kRoutingActivationRequestBaseLength; ++index) {
        if (payload_[index] != 0) {
            return AcknowledgeCode::InvalidPayloadLength;
        }
    }

    if (payload_.size() == kRoutingActivationRequestOemLength) {
        oem_data_.assign(payload_.begin() + kRoutingActivationRequestBaseLength,
                        payload_.end());
    }
    is_routing_activation_request_ = true;
    return AcknowledgeCode::Accepted;
}

bool RoutingActivationMessage::is_routing_activation_request() const {
  return is_routing_activation_request_;
}

std::uint16_t RoutingActivationMessage::tester_address() const {
  return tester_address_;
}

std::uint8_t RoutingActivationMessage::activation_type() const {
  return activation_type_;
}

const std::vector<std::uint8_t> &RoutingActivationMessage::oem_data() const {
  return oem_data_;
}

RoutingActivationManager::RoutingActivationManager(
  const RoutingActivationSettings &settings,
  const std::shared_ptr<RoutingActivationCommunication> &communication)
  : settings_(settings), communication_(communication) {
  if (!communication_) {
    throw std::invalid_argument("Routing Activation requires communication");
  }

  validate_settings();
  communication_->set_receiver(this);
}

RoutingActivationManager::~RoutingActivationManager() {
  communication_->set_receiver(nullptr);
}

void RoutingActivationManager::validate_settings() const {
  if (settings_.protocol_version == 0xFF) {
    throw std::invalid_argument("Invalid DoIP protocol version");
  }
  if (settings_.maximum_tester_connections == 0) {
    throw std::invalid_argument("Maximum tester connections must be nonzero");
  }
  if (settings_.alive_check_response_timeout.count() <= 0) {
    throw std::invalid_argument("Alive-check timeout must be positive");
  }

  std::set<std::uint16_t> tester_addresses;
  for (const TesterConfiguration &tester : settings_.testers) {
    if (!tester_addresses.insert(tester.source_address).second) {
      throw std::invalid_argument("Duplicate tester source address");
    }

    std::set<std::uint8_t> activation_types;
    for (const RoutingActivationProfile &profile : tester.activations) {
      if (!activation_types.insert(profile.activation_type).second) {
        throw std::invalid_argument("Duplicate routing-activation type");
      }

      if (profile.authentication_request_length > 4 || profile.authentication_response_length > 4 ||
          profile.confirmation_request_length > 4 || profile.confirmation_response_length > 4) {
        throw std::invalid_argument("Callback data lengths must be between 0 and 4");
      }
      if ((profile.authentication_callback == nullptr &&
           (profile.authentication_request_length != 0 ||
            profile.authentication_response_length != 0)) ||
          (profile.confirmation_callback == nullptr &&
           (profile.confirmation_request_length != 0 ||
            profile.confirmation_response_length != 0))) {
        throw std::invalid_argument("Callback lengths require the corresponding callback");
      }
      if (static_cast<unsigned int>(profile.authentication_response_length) +
            static_cast<unsigned int>(profile.confirmation_response_length) >
          4) {
        throw std::invalid_argument(
          "Authentication and confirmation response data exceed four bytes");
      }

      std::set<std::uint16_t> targets;
      for (std::uint16_t target : profile.target_addresses) {
        if (!targets.insert(target).second) {
          throw std::invalid_argument("Duplicate target address in activation profile");
        }
      }
    }
  }
}

bool RoutingActivationManager::on_connection_opened(ConnectionId connection_id,
                                                    bool secure_transport) {
  Connection connection;
  connection.secure_transport = secure_transport;
  return connections_.insert(std::make_pair(connection_id, connection)).second;
}

void RoutingActivationManager::on_message_received( ConnectionId connection_id,
                                                    const std::vector<std::uint8_t> &message ) {
    auto connection_it = connections_.find(connection_id);
    if (connection_it == connections_.end()) {
        return;
    }

    // SWS_DoIP_00101: TCP eligibility is guaranteed by the registration
    // contract; non-Routing-Activation payloads are discarded here.
    RoutingActivationMessage parsed_message;
    const AcknowledgeCode parse_result =
        parsed_message.parse(message, settings_.protocol_version, ParseLimits());
    if (parse_result != AcknowledgeCode::Accepted) {
        // SWS_DoIP_00014/00019 close the socket; other NACK causes discard.
        if (DoIPMessage::requires_socket_close(parse_result)) {
            close_connection(connection_id);
        }
        return;
    }
    if (!parsed_message.is_routing_activation_request()) {
        return;
    }

    ActivationRequest request;
    request.tester_address = parsed_message.tester_address();
    request.activation_type = parsed_message.activation_type();
    request.oem_data = parsed_message.oem_data();

    const ConnectionState state = connection_it->second.state;
    if (state != ConnectionState::Connected) {
        if (!connection_it->second.tester_registered ||
            (state != ConnectionState::Registered && state != ConnectionState::Active)) {
        close_connection(connection_id);
        return;
        }
    }

    connection_it->second.request = request;
    connection_it->second.profile = nullptr;
    connection_it->second.authentication_response.clear();
    connection_it->second.confirmation_response.clear();
    connection_it->second.confirmation_pending_response_sent = false;
    validate_and_start_activation(connection_id);
}

void RoutingActivationManager::on_alive_check_response(ConnectionId connection_id,
                                                       std::uint16_t tester_address) {
  const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::vector<ConnectionId> expired_checks;
  for (const auto &pending : pending_alive_checks_) {
    if (now >= pending.second.deadline) {
      expired_checks.push_back(pending.first);
    }
  }
  for (ConnectionId requester_id : expired_checks) {
    finish_alive_check(requester_id);
  }

  auto connection_it = connections_.find(connection_id);
  if (connection_it == connections_.end() || !connection_it->second.tester_registered ||
      connection_it->second.tester_address != tester_address) {
    return;
  }

  std::vector<ConnectionId> completed_checks;
  for (auto &pending : pending_alive_checks_) {
    PendingAliveCheck &check = pending.second;
    if (std::find(check.probed_connections.begin(), check.probed_connections.end(),
                  connection_id) == check.probed_connections.end()) {
      continue;
    }

    if (check.failed_connections.count(connection_id) != 0) {
      continue;
    }
    check.responding_connections.insert(connection_id);
    if (check.responding_connections.size() + check.failed_connections.size() ==
        check.probed_connections.size()) {
      completed_checks.push_back(pending.first);
    }
  }

  for (ConnectionId requester_id : completed_checks) {
    finish_alive_check(requester_id);
  }
}

void RoutingActivationManager::on_connection_closed(ConnectionId connection_id) {
  erase_connection(connection_id);
}

void RoutingActivationManager::main_function() {
  const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::vector<ConnectionId> expired_checks;
  for (const auto &pending : pending_alive_checks_) {
    if (now >= pending.second.deadline) {
      expired_checks.push_back(pending.first);
    }
  }
  for (ConnectionId requester_id : expired_checks) {
    finish_alive_check(requester_id);
  }

  std::vector<ConnectionId> pending_authentication;
  std::vector<ConnectionId> pending_confirmation;
  for (const auto &connection : connections_) {
    if (connection.second.state == ConnectionState::AuthenticationPending) {
      pending_authentication.push_back(connection.first);
    } else if (connection.second.state == ConnectionState::ConfirmationPending) {
      pending_confirmation.push_back(connection.first);
    }
  }
  for (ConnectionId connection_id : pending_authentication) {
    process_authentication(connection_id);
  }
  for (ConnectionId connection_id : pending_confirmation) {
    process_confirmation(connection_id);
  }
}

bool RoutingActivationManager::may_route_diagnostic(ConnectionId connection_id,
                                                    std::uint16_t source_address,
                                                    std::uint16_t target_address) const {
  auto connection_it = connections_.find(connection_id);
  if (connection_it == connections_.end()) {
    return false;
  }

  const Connection &connection = connection_it->second;
  return connection.state == ConnectionState::Active && connection.tester_registered &&
         connection.tester_address == source_address &&
         has_target(connection.permitted_targets, target_address);
}

/*
* @SWS_DoIP_00102
* 
* Finds the tester configuration for the given source address.
* Returns a pointer to the configuration if found, nullptr otherwise.
*/
const TesterConfiguration *
RoutingActivationManager::find_tester(std::uint16_t source_address) const 
{
    for (const TesterConfiguration &tester : settings_.testers) 
    {
        if (tester.source_address == source_address) {
            return &tester;
        }
    }
    return nullptr;
}

const RoutingActivationProfile *
RoutingActivationManager::find_profile(const TesterConfiguration &tester,
                                       std::uint8_t activation_type) const {
  for (const RoutingActivationProfile &profile : tester.activations) {
    if (profile.activation_type == activation_type) {
      return &profile;
    }
  }
  return nullptr;
}

std::vector<ConnectionId>
RoutingActivationManager::registered_connections(ConnectionId excluded_connection) const {
  std::vector<ConnectionId> registered;
  for (const auto &connection : connections_) {
    if (connection.first != excluded_connection && connection.second.tester_registered) {
      registered.push_back(connection.first);
    }
  }
  return registered;
}

std::size_t RoutingActivationManager::registered_connection_count() const {
  std::size_t count = 0;
  for (const auto &connection : connections_) {
    if (connection.second.tester_registered) {
      ++count;
    }
  }
  return count;
}

void RoutingActivationManager::validate_and_start_activation(ConnectionId connection_id) {
    auto connection_it = connections_.find(connection_id);
    if (connection_it == connections_.end()) {
        return;
    }
    Connection &connection = connection_it->second;
    const ActivationRequest &request = connection.request;

    // SWS_DoIP_00102: A valid request continues only for a configured tester source address.
    const TesterConfiguration *tester = find_tester(request.tester_address);
    
    if (tester == nullptr) 
    {
        // SWS_DoIP_00104: Reject an unconfigured tester address and close.
        reject_and_close(connection_id, RoutingActivationResponseCode::UnknownSourceAddress);
        return;
    }

    const bool already_registered = connection.tester_registered;
    if (connection.tester_registered) 
    {
        if (connection.tester_address != request.tester_address) 
        {
            // SWS_DoIP_00106: A registered socket cannot change its tester address.
            reject_and_close(connection_id, RoutingActivationResponseCode::ConnectionAlreadyRegistered);
            return;
        }
    }

    // SWS_DoIP_00103 / SWS_DoIP_00105: A source already registered on this socket may continue; a registration on another socket is alive-checked.
    std::vector<ConnectionId> duplicates;
    for (ConnectionId registered_id : registered_connections(connection_id)) {
        const Connection &registered = connections_.find(registered_id)->second;
        if (registered.tester_address == request.tester_address) {
        duplicates.push_back(registered_id);
        }
    }
    if (!duplicates.empty()) {
        start_alive_check(connection_id, AliveCheckPurpose::DuplicateAddress, duplicates);
        return;
    }

    // SWS_DoIP_00107: Existing registrations do not consume an additional slot
    // when they repeat activation on the same socket.
    if (!already_registered &&
        registered_connection_count() >= settings_.maximum_tester_connections) {
        start_alive_check(connection_id, AliveCheckPurpose::Capacity,
                        registered_connections(connection_id));
        return;
    }

    const RoutingActivationProfile *profile = find_profile(*tester, request.activation_type);
    if (profile == nullptr) {
        // SWS_DoIP_00108 / SWS_DoIP_00160: Reject unsupported activation types.
        reject_and_close(connection_id, RoutingActivationResponseCode::UnsupportedActivationType);
        return;
    }

    if (profile->security_required && !connection.secure_transport) {
        close_connection(connection_id);
        return;
    }

    if (request.oem_data.size() < profile->authentication_request_length ||
        request.oem_data.size() < profile->confirmation_request_length) {
        close_connection(connection_id);
        return;
    }

    connection.profile = profile;
    connection.tester_registered = true;
    connection.tester_address = request.tester_address;
    connection.state = ConnectionState::Registered;
    continue_activation(connection_id);
}

void RoutingActivationManager::start_alive_check( ConnectionId requester_id, 
                                                  AliveCheckPurpose purpose,
                                                  const std::vector<ConnectionId> &checked_connections ) 
{
    auto requester = connections_.find(requester_id);

    if (requester == connections_.end()) 
    {
        return;
    }

    if (checked_connections.empty()) 
    {
        validate_and_start_activation(requester_id);
        return;
    }

    // @SWS_DoIP_00105 / @SWS_DoIP_00107: Wait for alive-check replies or the
    // configured response timeout before deciding whether connections are stale.
    PendingAliveCheck check;
    check.purpose = purpose;
    check.probed_connections = checked_connections;
    check.deadline = std::chrono::steady_clock::now() + settings_.alive_check_response_timeout;
    requester->second.state = ConnectionState::CheckingAlive;
    pending_alive_checks_[requester_id] = check;

    for (ConnectionId checked_id : checked_connections) 
    {
        // For every connection that has been connected to the DoIP network, send an alive check.
        if (connections_.find(checked_id) == connections_.end() ||
            !communication_->send_alive_check(checked_id)) 
        {
            pending_alive_checks_[requester_id].failed_connections.insert(checked_id);
        }
    }

    const PendingAliveCheck &pending = pending_alive_checks_.find(requester_id)->second;

    if ( ( pending.responding_connections.size() + pending.failed_connections.size() ) ==
           pending.probed_connections.size() ) 
    {
        finish_alive_check(requester_id);
    }
}

void RoutingActivationManager::finish_alive_check(ConnectionId requester_id) {
    auto pending_it = pending_alive_checks_.find(requester_id);
    if (pending_it == pending_alive_checks_.end()) {
        return;
    }

    PendingAliveCheck check = pending_it->second;
    pending_alive_checks_.erase(pending_it);
    auto requester = connections_.find(requester_id);
    if (requester == connections_.end()) {
        return;
    }

    std::vector<ConnectionId> nonresponding;
    for (ConnectionId checked_id : check.probed_connections) {
        if (check.responding_connections.count(checked_id) == 0) {
        nonresponding.push_back(checked_id);
        }
    }

    if (check.purpose == AliveCheckPurpose::DuplicateAddress && !nonresponding.empty()) {
        for (ConnectionId stale_id : nonresponding) {
        close_connection(stale_id);
        }
        auto still_registered = connections_.find(requester_id);
        if (still_registered != connections_.end()) {
        still_registered->second.state = ConnectionState::Connected;
        validate_and_start_activation(requester_id);
        }
        return;
    }

    if (check.purpose == AliveCheckPurpose::DuplicateAddress) {
        // SWS_DoIP_00105: A live duplicate source address rejects this request.
        reject_and_close(requester_id, RoutingActivationResponseCode::SourceAddressAlreadyRegistered);
        return;
    }

    if (nonresponding.empty()) {
        // SWS_DoIP_00107: All registered connections answered, so capacity remains occupied.
        reject_and_close(requester_id, RoutingActivationResponseCode::AllConnectionsOccupied);
        return;
    }

    for (ConnectionId stale_id : nonresponding) {
        close_connection(stale_id);
    }

    auto still_registered = connections_.find(requester_id);
    if (still_registered == connections_.end()) {
        return;
    }
    still_registered->second.state = ConnectionState::Connected;
    if (registered_connection_count() >= settings_.maximum_tester_connections) {
        start_alive_check(requester_id, AliveCheckPurpose::Capacity,
                        registered_connections(requester_id));
    } else {
        validate_and_start_activation(requester_id);
    }
}

void RoutingActivationManager::continue_activation(ConnectionId connection_id) {
  auto connection_it = connections_.find(connection_id);
  if (connection_it == connections_.end() || connection_it->second.profile == nullptr) {
    return;
  }

  Connection &connection = connection_it->second;
  if (connection.profile->authentication_callback) {
    // SWS_DoIP_00109: Invoke the configured authentication callback.
    connection.state = ConnectionState::AuthenticationPending;
    process_authentication(connection_id);
  } else {
    process_confirmation(connection_id);
  }
}

void RoutingActivationManager::process_authentication(ConnectionId connection_id) {
  auto connection_it = connections_.find(connection_id);
  if (connection_it == connections_.end() || connection_it->second.profile == nullptr ||
      !connection_it->second.profile->authentication_callback) {
    return;
  }

  Connection &connection = connection_it->second;
  const RoutingActivationProfile &profile = *connection.profile;
  const std::vector<std::uint8_t> request_data(connection.request.oem_data.begin(),
                                               connection.request.oem_data.begin() +
                                                 profile.authentication_request_length);
  CallbackResult result = profile.authentication_callback->invoke(
    connection_id, connection.tester_address, connection.request.activation_type, request_data);

  if (result.status == CallbackStatus::Pending) {
    // SWS_DoIP_00110: Retry pending authentication on the next main cycle.
    connection.state = ConnectionState::AuthenticationPending;
    return;
  }
  if (result.status == CallbackStatus::Rejected) {
    // SWS_DoIP_00111: Denial keeps the tester registered but does not activate
    // routing.
    connection.state = ConnectionState::Registered;
    if (!send_response(connection_id, RoutingActivationResponseCode::AuthenticationDenied)) {
      close_connection(connection_id);
    }
    return;
  }

  if (result.status != CallbackStatus::Success ||
      result.response_data.size() != profile.authentication_response_length) {
    close_connection(connection_id);
    return;
  }

  // SWS_DoIP_00161: Include the configured authentication response bytes.
  connection.authentication_response = result.response_data;
  process_confirmation(connection_id);
}

void RoutingActivationManager::process_confirmation(ConnectionId connection_id) {
  auto connection_it = connections_.find(connection_id);
  if (connection_it == connections_.end() || connection_it->second.profile == nullptr) {
    return;
  }

  Connection &connection = connection_it->second;
  const RoutingActivationProfile &profile = *connection.profile;
  if (!profile.confirmation_callback) {
    activate_routing(connection_id);
    return;
  }

  // SWS_DoIP_00112: Confirmation consumes the configured trailing OEM bytes.
  const auto confirmation_data_begin =
    connection.request.oem_data.end() - profile.confirmation_request_length;
  const std::vector<std::uint8_t> request_data(confirmation_data_begin,
                                               connection.request.oem_data.end());
  CallbackResult result = profile.confirmation_callback->invoke(
    connection_id, connection.tester_address, connection.request.activation_type, request_data);

  if (result.status == CallbackStatus::Pending) {
    connection.state = ConnectionState::ConfirmationPending;
    // SWS_DoIP_00114: Send the pending response once, even if polling returns
    // pending on subsequent main-function cycles.
    if (!connection.confirmation_pending_response_sent) {
      if (!send_response(connection_id, RoutingActivationResponseCode::ConfirmationPending)) {
        close_connection(connection_id);
        return;
      }
      connection.confirmation_pending_response_sent = true;
    }
    return;
  }
  if (result.status == CallbackStatus::Rejected) {
    // SWS_DoIP_00274: Denial sends 0x05 and closes the socket.
    reject_and_close(connection_id, RoutingActivationResponseCode::ConfirmationDenied);
    return;
  }

  if (result.status != CallbackStatus::Success ||
      result.response_data.size() != profile.confirmation_response_length) {
    close_connection(connection_id);
    return;
  }

  connection.confirmation_response = result.response_data;
  activate_routing(connection_id);
}

void RoutingActivationManager::activate_routing(ConnectionId connection_id) {
  auto connection_it = connections_.find(connection_id);
  if (connection_it == connections_.end() || connection_it->second.profile == nullptr) {
    return;
  }

  // SWS_DoIP_00113: Send success before enabling diagnostic routing.
  if (!send_response(connection_id, RoutingActivationResponseCode::SuccessfullyActivated)) {
    close_connection(connection_id);
    return;
  }

  connection_it = connections_.find(connection_id);
  if (connection_it == connections_.end()) {
    return;
  }
  connection_it->second.permitted_targets = connection_it->second.profile->target_addresses;
  connection_it->second.state = ConnectionState::Active;
}

bool RoutingActivationManager::send_response(ConnectionId connection_id,
                                             RoutingActivationResponseCode response_code) {
  auto connection_it = connections_.find(connection_id);
  if (connection_it == connections_.end()) {
    return false;
  }
  return communication_->send_message(connection_id,
                                      build_response(connection_it->second, response_code));
}

void RoutingActivationManager::reject_and_close(ConnectionId connection_id,
                                                RoutingActivationResponseCode response_code) {
  if (connections_.find(connection_id) == connections_.end()) {
    return;
  }
  send_response(connection_id, response_code);
  close_connection(connection_id);
}

void RoutingActivationManager::close_connection(ConnectionId connection_id) {
  if (connections_.find(connection_id) == connections_.end()) {
    return;
  }
  communication_->close_connection(connection_id);
  erase_connection(connection_id);
}

void RoutingActivationManager::erase_connection(ConnectionId connection_id) {
  connections_.erase(connection_id);
  pending_alive_checks_.erase(connection_id);

  std::vector<ConnectionId> completed_checks;
  for (auto &pending : pending_alive_checks_) {
    PendingAliveCheck &check = pending.second;
    if (std::find(check.probed_connections.begin(), check.probed_connections.end(),
                  connection_id) == check.probed_connections.end()) {
      continue;
    }
    if (check.responding_connections.count(connection_id) == 0) {
      check.failed_connections.insert(connection_id);
    }
    if (check.responding_connections.size() + check.failed_connections.size() ==
        check.probed_connections.size()) {
      completed_checks.push_back(pending.first);
    }
  }

  for (ConnectionId requester_id : completed_checks) {
    finish_alive_check(requester_id);
  }
}

std::vector<std::uint8_t>
RoutingActivationManager::build_response(const Connection &connection,
                                         RoutingActivationResponseCode response_code) const {
  // SWS_DoIP_00116 / SWS_DoIP_00118 / SWS_DoIP_00119 / SWS_DoIP_00120:
  // Populate the tester, entity, response code, and optional callback data.
  bool include_oem_data = false;
  std::vector<std::uint8_t> oem_data(4, 0);
  if (connection.profile != nullptr) {
    const RoutingActivationProfile &profile = *connection.profile;
    const bool has_configured_response_data =
      profile.authentication_response_length != 0 || profile.confirmation_response_length != 0;
    const bool all_callback_data_available =
      connection.authentication_response.size() == profile.authentication_response_length &&
      connection.confirmation_response.size() == profile.confirmation_response_length;
    include_oem_data = has_configured_response_data && all_callback_data_available;
    std::copy(connection.authentication_response.begin(), connection.authentication_response.end(),
              oem_data.begin());

    const std::size_t confirmation_offset = oem_data.size() - profile.confirmation_response_length;
    std::copy(connection.confirmation_response.begin(), connection.confirmation_response.end(),
              oem_data.begin() + confirmation_offset);
  }

  const std::uint32_t payload_length = static_cast<std::uint32_t>(
    kRoutingActivationResponseBaseLength + (include_oem_data ? oem_data.size() : 0));
  std::vector<std::uint8_t> message(kDoipHeaderLength + payload_length, 0);

  message[0] = settings_.protocol_version;
  message[1] = static_cast<std::uint8_t>(~settings_.protocol_version);
  write_u16_be(&message[2], kRoutingActivationResponseType);
  write_u32_be(&message[4], payload_length);
  write_u16_be(&message[8], connection.request.tester_address);
  write_u16_be(&message[10], settings_.entity_address);
  message[12] = static_cast<std::uint8_t>(response_code);

  if (include_oem_data) {
    std::copy(oem_data.begin(), oem_data.end(), message.begin() + 17);
  }
  return message;
}

} // namespace routing_activation
} // namespace doip
} // namespace as
