#ifndef DOIP_DIAGNOSTIC_MESSAGE_IMPORT_HPP
#define DOIP_DIAGNOSTIC_MESSAGE_IMPORT_HPP

extern "C" {
#include "PduR_DoIP.h"
}

#include <cstdint>
#include <vector>

namespace as {
namespace doip {

using ConnectionId = std::uint64_t;

bool DiagnosticMessageCommunication_send( ConnectionId connectionId, const std::vector<std::uint8_t>& message );
void DiagnosticMessageCommunication_closeConnection( ConnectionId connectionId );
bool DiagnosticMessageCommunication_isConfiguredTcpConnection(ConnectionId connectionId );
bool DiagnosticMessageCommunication_isConnectionOpen( ConnectionId connectionId );
bool DiagnosticMessageRoutingActivation_isTesterRegistered( ConnectionId connectionId, std::uint16_t testerSourceAddress );
bool DiagnosticMessageRoutingActivation_isTargetActivated(  ConnectionId connectionId,
                                                            std::uint16_t testerSourceAddress,
                                                            std::uint16_t targetAddress);

} // namespace doip
} // namespace as

#define DiagnosticMessage_send \
    ::as::doip::DiagnosticMessageCommunication_send

#define DiagnosticMessage_closeConnection \
    ::as::doip::DiagnosticMessageCommunication_closeConnection

#define DiagnosticMessage_isConfiguredTcpConnection \
    ::as::doip::DiagnosticMessageCommunication_isConfiguredTcpConnection

#define DiagnosticMessage_isConnectionOpen \
    ::as::doip::DiagnosticMessageCommunication_isConnectionOpen

#define DiagnosticMessage_isTesterRegistered \
    ::as::doip::DiagnosticMessageRoutingActivation_isTesterRegistered
    
#define DiagnosticMessage_isTargetActivated \
    ::as::doip::DiagnosticMessageRoutingActivation_isTargetActivated

#endif
