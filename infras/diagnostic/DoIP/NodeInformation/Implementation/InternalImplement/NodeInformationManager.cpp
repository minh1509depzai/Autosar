#include "NodeInformationManager.hpp"

#include "Configuration.hpp"
#include "NodeInformationMessage.hpp"

namespace as {
namespace doip {

NodeInformationManager& NodeInformationManager::instance()
{
    static NodeInformationManager manager;
    return manager;
}

void NodeInformationManager::onMessageReceived( ConnectionId connectionId,
                                                std::uint16_t payloadType,
                                                const std::vector<std::uint8_t>& payload )
{
    if ( payloadType != static_cast<std::uint16_t>( PayloadType::EntityStatusRequest ) &&
         payloadType != static_cast<std::uint16_t>( PayloadType::PowerModeInformationRequest ) )
    {
        return;
    }

    const ConnectionId udpConnectionId = NodeInformation_getUdpConnectionId( );

    /* @SWS_DoIP_00090 */
    /* @SWS_DoIP_00094 */
    if ( connectionId != udpConnectionId )
    {
        return;
    }

    /* @SWS_DoIP_00091 */
    /* @SWS_DoIP_00095 */
    if ( !payload.empty() )
    {
        return;
    }

    if ( payloadType == static_cast<std::uint16_t>( PayloadType::EntityStatusRequest ) )
    {
        handleEntityStatusRequest(udpConnectionId);
        return;
    }

    if ( payloadType == static_cast<std::uint16_t>( PayloadType::PowerModeInformationRequest ) )
    {
        handlePowerModeRequest(udpConnectionId);
        return;
    }
}

void NodeInformationManager::handleEntityStatusRequest(ConnectionId udpConnectionId)
{
    const configuration::DoIPConfiguration& config = configuration::DOIP_DEFAULT_CONFIGURATION;
    
    if (config.containers.interfaces == nullptr || config.containers.interfaceCount == 0U)
    {
        return;
    }

    EntityStatusResponseData responseData = {};

    /* @SWS_DoIP_00097 */
    switch ( config.general.nodeType )
    {
        case configuration::DoIPNodeTypeValue::Gateway:
            responseData.nodeType = 0x00U;
            break;
        case configuration::DoIPNodeTypeValue::Node:
            responseData.nodeType = 0x01U;
            break;
        default:
            return;
    }

    /* @SWS_DoIP_00098 */
    responseData.maxOpenSockets = config.containers.interfaces[0].maxTesterConnections;

    /* @SWS_DoIP_00099 */
    responseData.currentlyOpenSockets = NodeInformation_getActiveConnections();

    /* @SWS_DoIP_00100 */
    responseData.includeMaxDataSize = config.general.entityStatusMaxByteFieldUse;
    responseData.maxDataSize = config.general.maxRequestBytes;

    const std::vector<std::uint8_t> response = NodeInformationMessage::buildEntityStatusResponse( responseData );

    /* @SWS_DoIP_00096 */
    NodeInformation_send(udpConnectionId, response);
}

void NodeInformationManager::handlePowerModeRequest(ConnectionId udpConnectionId)
{
    std::uint8_t callbackPowerMode = 0U;

    /* @SWS_DoIP_00093 */
    const Std_ReturnType callbackResult = NodeInformation_getPowerMode( &callbackPowerMode );
    const std::uint8_t responsePowerMode = ( callbackResult == E_OK ) ? callbackPowerMode : 0x00U;
    const std::vector<std::uint8_t> response = NodeInformationMessage::buildPowerModeResponse(responsePowerMode);

    /* @SWS_DoIP_00092 */
    NodeInformation_send(udpConnectionId, response);
}

} // namespace doip
} // namespace as
