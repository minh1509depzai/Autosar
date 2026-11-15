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

const configuration::DoIPChannelConfig* findConfiguredChannel(
    const configuration::DoIPConfigSet& configSet,
    std::uint16_t sourceAddress,
    std::uint16_t targetAddress )
{
    if (configSet.interfaces == nullptr)
    {
        return nullptr;
    }

    for (std::size_t interfaceIndex = 0U; interfaceIndex < configSet.interfaceCount; ++interfaceIndex)
    {
        const configuration::DoIPInterfaceConfig& interfaceConfig = configSet.interfaces[interfaceIndex];
        if (interfaceConfig.channels == nullptr)
        {
            continue;
        }

        for (std::size_t index = 0U; index < interfaceConfig.channelCount; ++index)
        {
            const configuration::DoIPChannelConfig& channel = interfaceConfig.channels[index];
            if (channel.sourceTester != nullptr && channel.target != nullptr &&
                channel.sourceTester->sourceAddress == sourceAddress &&
                channel.target->value == targetAddress)
            {
                return &channel;
            }
        }
    }
    return nullptr;
}

std::uint32_t getAckNackBytes(
    const configuration::DoIPConfigSet& configSet,
    std::uint16_t sourceAddress)
{
    if (configSet.interfaces == nullptr)
    {
        return 0U;
    }

    for (std::size_t interfaceIndex = 0U; interfaceIndex < configSet.interfaceCount; ++interfaceIndex)
    {
        const configuration::DoIPInterfaceConfig& interfaceConfig = configSet.interfaces[interfaceIndex];
        if (interfaceConfig.testers == nullptr)
        {
            continue;
        }

        for (std::size_t index = 0U; index < interfaceConfig.testerCount; ++index)
        {
            if (interfaceConfig.testers[index].sourceAddress == sourceAddress)
            {
                return interfaceConfig.testers[index].diagnosticAckNackBytes;
            }
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

    const configuration::DoIPConfigSet& configSet = configuration::DOIP_DEFAULT_CONFIGURATION.configSet;
    const std::uint16_t sourceAddress = requestMessage.sourceAddress();
    const std::uint16_t targetAddress = requestMessage.targetAddress();
    const std::uint32_t ackNackBytes = getAckNackBytes(configSet, sourceAddress);

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
    const configuration::DoIPChannelConfig* channel = findConfiguredChannel(configSet, sourceAddress, targetAddress);
    if (channel == nullptr)
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

    const configuration::DoIPPduRRxPduConfig* rxPdu = channel->pduRRxPdu;
    const configuration::DoIPPduRTxPduConfig* txPdu = channel->pduRTxPdu;
    
    if ( ( rxPdu == nullptr ) || ( txPdu == nullptr ) ||
         ( requestMessage.userData().size() > static_cast<std::size_t>(std::numeric_limits<PduLengthType>::max()) ))
    {
        return;
    }

    const PduIdType rxPduId = static_cast<PduIdType>(rxPdu->pduId);
    const PduIdType txPduId = static_cast<PduIdType>(txPdu->pduId);
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
