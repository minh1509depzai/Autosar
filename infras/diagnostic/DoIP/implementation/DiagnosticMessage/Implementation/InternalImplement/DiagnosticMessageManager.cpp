#include "DiagnosticMessageManager.hpp"

#include "Configuration.hpp"
#include "DiagnosticMessage.hpp"
#include "DiagnosticRequestContext.hpp"
#include "DoIPMessage.hpp"

#include <limits>

namespace as {
namespace doip {
namespace {

const std::uint16_t kDiagnosticMessagePayloadType = 0x8001U;

const configuration::DoIPPduConfig* findPduMapping(
    const configuration::DoIPConfigurationView& containers)
{
    if (containers.pduMappings == nullptr || containers.pduMappingCount != 1U)
    {
        return nullptr;
    }
    return &containers.pduMappings[0];
}

bool isConfiguredChannel( const configuration::DoIPConfigurationView& containers,
                          std::uint16_t sourceAddress,
                          std::uint16_t targetAddress )
{
    if (containers.channels == nullptr)
    {
        return false;
    }

    for (std::size_t index = 0U; index < containers.channelCount; ++index)
    {
        const configuration::DoIPChannelConfig& channel = containers.channels[index];
        if (channel.sourceTester != nullptr && channel.target != nullptr &&
            channel.sourceTester->sourceAddress == sourceAddress &&
            channel.target->value == targetAddress)
        {
            return true;
        }
    }
    return false;
}

std::uint32_t getAckNackBytes(
    const configuration::DoIPConfigurationView& containers,
    std::uint16_t sourceAddress)
{
    if (containers.testers == nullptr)
    {
        return 0U;
    }

    for (std::size_t index = 0U; index < containers.testerCount; ++index)
    {
        if (containers.testers[index].sourceAddress == sourceAddress)
        {
            return containers.testers[index].diagnosticAckNackBytes;
        }
    }
    return 0U;
}

bool sendDiagnosticFrame(
    ConnectionId connectionId,
    PayloadType payloadType,
    const std::vector<std::uint8_t>& payload)
{
    BuildMessageErrorCode errorCode = BuildMessageErrorCode::Success;
    const DoIPMessage message = DoIPMessage::build(payloadType, payload, errorCode);
    if (errorCode != BuildMessageErrorCode::Success)
    {
        return false;
    }
    return DiagnosticMessage_send(connectionId, message.toBytes());
}

} // namespace

DiagnosticMessageManager& DiagnosticMessageManager::instance()
{
    static DiagnosticMessageManager manager;
    return manager;
}

void DiagnosticMessageManager::onMessageReceived( ConnectionId connectionId,
                                                  std::uint16_t payloadType,
                                                  const std::vector<std::uint8_t>& payload )
{
    if (payloadType != PayloadType::DiagnosticMessage)
    {
        return;
    }

    /* @SWS_DoIP_00121 */
    if (!DiagnosticMessage_isConfiguredTcpConnection(connectionId))
    {
        return;
    }

    handleDiagnosticRequest(connectionId, payload);
}

void DiagnosticMessageManager::handleDiagnosticRequest( ConnectionId connectionId,
                                                        const std::vector<std::uint8_t>& payload )
{
    DiagnosticMessage requestMessage;

    if ( !requestMessage.parse(payload) )
    {
        return;
    }

    const configuration::DoIPConfigurationView& containers = configuration::DOIP_DEFAULT_CONFIGURATION.containers;
    const std::uint16_t sourceAddress = requestMessage.sourceAddress();
    const std::uint16_t targetAddress = requestMessage.targetAddress();
    const std::uint32_t ackNackBytes = getAckNackBytes(containers, sourceAddress);

    /* @SWS_DoIP_00123 */
    if (!DiagnosticMessage_isTesterRegistered(connectionId, sourceAddress))
    {
        sendNegativeAcknowledge( requestMessage, 
                                 connectionId, 
                                 ackNackBytes,
                                 DiagnosticNackCode::InvalidSourceAddress);

        DiagnosticMessage_closeConnection(connectionId);
        return;
    }

    /* @SWS_DoIP_00124 */
    if (!isConfiguredChannel(containers, sourceAddress, targetAddress))
    {
        sendNegativeAcknowledge( requestMessage, 
                                 connectionId, 
                                 ackNackBytes,
                                 DiagnosticNackCode::UnknownTargetAddress );

        return;
    }

    /* @SWS_DoIP_00125 */
    const std::uint32_t maxRequestBytes = configuration::DOIP_DEFAULT_CONFIGURATION.general.maxRequestBytes;
    
    if (maxRequestBytes < 4U || payload.size() > static_cast<std::size_t>(maxRequestBytes - 4U) )
    {
        sendNegativeAcknowledge( requestMessage, 
                                 connectionId, 
                                 ackNackBytes,
                                 DiagnosticNackCode::DiagnosticMessageTooLarge );

        return;
    }

    const configuration::DoIPPduConfig* pduMapping = findPduMapping(containers);
    
    if ( ( pduMapping == nullptr ) || 
         ( requestMessage.userData().size() > static_cast<std::size_t>(std::numeric_limits<PduLengthType>::max()) ))
    {
        return;
    }

    const PduIdType rxPduId = static_cast<PduIdType>(pduMapping->pduRRxPduId);
    const PduIdType txPduId = static_cast<PduIdType>(pduMapping->pduRTxPduId);
    if (_pendingResponsesByTxPdu.find(txPduId) != _pendingResponsesByTxPdu.end())
    {
        return;
    }

    std::vector<std::uint8_t> userData = requestMessage.userData();
    PduInfoType startInfo = {};
    startInfo.SduDataPtr = nullptr;
    startInfo.SduLength = 0U;
    PduLengthType bufferSize = 0U;

    /* @SWS_DoIP_00126 */
    const BufReq_ReturnType startResult = PduR_DoIPStartOfReception( rxPduId,
                                                                     &startInfo,
                                                                     static_cast<PduLengthType>( userData.size() ),
                                                                     &bufferSize );
    if ( startResult != BUFREQ_OK )
    {
        sendNegativeAcknowledge( requestMessage, 
                                 connectionId, 
                                 ackNackBytes,
                                 DiagnosticNackCode::OutOfMemory );
        return;
    }

    if ( bufferSize < userData.size() )
    {
        PduR_DoIPRxIndication( rxPduId, E_NOT_OK );
        sendNegativeAcknowledge( requestMessage, 
                                 connectionId, 
                                 ackNackBytes,
                                 DiagnosticNackCode::OutOfMemory );
        return;
    }

    /* @SWS_DoIP_00127 */
    if ( !DiagnosticMessage_isTargetActivated( connectionId, sourceAddress, targetAddress ) )
    {
        PduR_DoIPRxIndication(rxPduId, E_NOT_OK);

        sendNegativeAcknowledge( requestMessage, 
                                 connectionId, 
                                 ackNackBytes,
                                 DiagnosticNackCode::TargetNotActivated );
        return;
    }

    PduInfoType copyInfo = {};
    copyInfo.SduDataPtr = userData.empty() ? nullptr : userData.data();
    copyInfo.SduLength = static_cast<PduLengthType>(userData.size());

    /* @SWS_DoIP_00128 */
    const BufReq_ReturnType copyResult = PduR_DoIPCopyRxData(rxPduId, &copyInfo, &bufferSize);

    if ( copyResult != BUFREQ_OK )
    {
        PduR_DoIPRxIndication( rxPduId, E_NOT_OK );
        return;
    }

    DiagnosticRequestContext context = {
        connectionId,
        rxPduId,
        txPduId,
        ackNackBytes,
        requestMessage
    };
    _pendingResponsesByTxPdu[txPduId] = context;
    PduR_DoIPRxIndication(rxPduId, E_OK);

    /* @SWS_DoIP_00129 */
    sendPositiveAcknowledge(context);
}

bool DiagnosticMessageManager::sendPositiveAcknowledge( const DiagnosticRequestContext& context)
{
    const std::vector<std::uint8_t> payload = context.requestMessage.buildPositiveAcknowledge(context.ackNackBytes);
    
    return sendDiagnosticFrame( context.connectionId,
                                PayloadType::DiagnosticPositiveAcknowledge,
                                payload );
}

bool DiagnosticMessageManager::sendNegativeAcknowledge( const DiagnosticMessage& requestMessage,
                                                        ConnectionId connectionId,
                                                        std::uint32_t ackNackBytes,
                                                        DiagnosticNackCode nackCode )
{
    const std::vector<std::uint8_t> payload = requestMessage.buildNegativeAcknowledge(nackCode, ackNackBytes);
    
    return sendDiagnosticFrame( connectionId, 
                                PayloadType::DiagnosticNegativeAcknowledge, 
                                payload );
}

Std_ReturnType DiagnosticMessageManager::tpTransmit( PduIdType txPduId, const PduInfoType* pduInfo)
{
    return handleResponse( txPduId, pduInfo );
}

Std_ReturnType DiagnosticMessageManager::ifTransmit( PduIdType txPduId, const PduInfoType* pduInfo )
{
    return handleResponse(txPduId, pduInfo);
}

Std_ReturnType DiagnosticMessageManager::handleResponse( PduIdType txPduId, const PduInfoType* pduInfo )
{
    /* @SWS_DoIP_00130 */
    std::map<PduIdType, DiagnosticRequestContext>::iterator contextIterator = _pendingResponsesByTxPdu.find(txPduId);
    
    if ( ( contextIterator == _pendingResponsesByTxPdu.end() ) ||
         ( pduInfo == nullptr ) ||
         ( pduInfo->SduLength > 0U && pduInfo->SduDataPtr == nullptr ) )
    {
        return E_NOT_OK;
    }

    const DiagnosticRequestContext context = contextIterator->second;
    if ( !DiagnosticMessage_isConnectionOpen(context.connectionId) )
    {
        _pendingResponsesByTxPdu.erase(contextIterator);

        return E_NOT_OK;
    }

    std::vector<std::uint8_t> userData;
    
    if (pduInfo->SduLength > 0U)
    {
        userData.assign( pduInfo->SduDataPtr,
                         pduInfo->SduDataPtr + pduInfo->SduLength );
    }

    const std::vector<std::uint8_t> payload = context.requestMessage.buildResponse(userData);

    _pendingResponsesByTxPdu.erase(contextIterator);

    return sendDiagnosticFrame( context.connectionId,
                                PayloadType::DiagnosticMessage,
                                payload ) ? E_OK : E_NOT_OK;
}

void DiagnosticMessageManager::onConnectionClosed(ConnectionId connectionId)
{
    for ( std::map<PduIdType, DiagnosticRequestContext>::iterator iterator = _pendingResponsesByTxPdu.begin();
          iterator != _pendingResponsesByTxPdu.end(); )
    {
        if (iterator->second.connectionId == connectionId)
        {
            iterator = _pendingResponsesByTxPdu.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
}

} // namespace doip
} // namespace as
