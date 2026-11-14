#ifndef DOIP_DIAGNOSTIC_MESSAGE_MANAGER_HPP
#define DOIP_DIAGNOSTIC_MESSAGE_MANAGER_HPP

#include "DiagnosticMessageImport.hpp"
#include "DiagnosticRequestContext.hpp"

#include <cstdint>
#include <map>
#include <vector>

namespace as {
namespace doip {

class DiagnosticMessageManager
{
public:
    static DiagnosticMessageManager& instance();

    DiagnosticMessageManager( const DiagnosticMessageManager& ) = delete;

    DiagnosticMessageManager& operator=( const DiagnosticMessageManager&) = delete;

    void onMessageReceived( ConnectionId connectionId,
                            std::uint16_t payloadType,
                            const std::vector<std::uint8_t>& payload );
    
    Std_ReturnType tpTransmit( PduIdType txPduId, const PduInfoType* pduInfo );
    
    Std_ReturnType ifTransmit( PduIdType txPduId, const PduInfoType* pduInfo );
    
    void onConnectionClosed(ConnectionId connectionId);

private:
    DiagnosticMessageManager() = default;

    void handleDiagnosticRequest( ConnectionId connectionId,
                                  const std::vector<std::uint8_t>& payload );
    bool sendPositiveAcknowledge( const DiagnosticRequestContext& context );

    bool sendNegativeAcknowledge( const DiagnosticMessage& requestMessage,
                                  ConnectionId connectionId,
                                  std::uint32_t ackNackBytes,
                                  DiagnosticNackCode nackCode );

    Std_ReturnType handleResponse( PduIdType txPduId, const PduInfoType* pduInfo );

    std::map<PduIdType, DiagnosticRequestContext> _pendingResponsesByTxPdu;
};

} // namespace doip
} // namespace as

#endif
