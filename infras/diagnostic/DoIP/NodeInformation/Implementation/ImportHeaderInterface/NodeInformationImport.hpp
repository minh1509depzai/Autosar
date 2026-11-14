#ifndef DOIP_NODE_INFORMATION_IMPORT_HPP
#define DOIP_NODE_INFORMATION_IMPORT_HPP

#include "Std_Types.h"

#include <cstdint>
#include <vector>

namespace as {
namespace doip {

using ConnectionId = std::uint64_t;

void NodeInformationCommunication_send( ConnectionId connectionId, const std::vector<std::uint8_t>& message );

ConnectionId NodeInformationConfiguration_getUdpConnectionId();

std::uint8_t NodeInformationConnectionManagement_getActiveConnections();

Std_ReturnType NodeInformationCallback_getPowerMode( std::uint8_t* powerMode );

} // namespace doip
} // namespace as

#define NodeInformation_send ::as::doip::NodeInformationCommunication_send

#define NodeInformation_getUdpConnectionId ::as::doip::NodeInformationConfiguration_getUdpConnectionId

#define NodeInformation_getActiveConnections ::as::doip::NodeInformationConnectionManagement_getActiveConnections

#define NodeInformation_getPowerMode ::as::doip::NodeInformationCallback_getPowerMode

#endif
