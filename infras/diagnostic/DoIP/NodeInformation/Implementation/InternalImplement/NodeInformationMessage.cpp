#include "NodeInformationMessage.hpp"

#include <stdexcept>

namespace as {
namespace doip {
namespace {

std::vector<std::uint8_t> buildFrame( PayloadType payloadType,
                                      const std::vector<std::uint8_t>& payload )
{
    BuildMessageErrorCode errorCode = BuildMessageErrorCode::Success;
    const DoIPMessage message = DoIPMessage::build( payloadType, payload, errorCode );

    if (errorCode != BuildMessageErrorCode::Success)
    {
        throw std::logic_error("Unable to build a valid Node Information response");
    }

    return message.toBytes();
}

} // namespace

std::vector<std::uint8_t> NodeInformationMessage::buildEntityStatusResponse( const EntityStatusResponseData& fields )
{
    std::vector<std::uint8_t> payload;

    payload.reserve( fields.includeMaxDataSize ? 7U : 3U );

    /* @SWS_DoIP_00097 */
    payload.push_back( fields.nodeType );

    /* @SWS_DoIP_00098 */
    payload.push_back( fields.maxOpenSockets);

    /* @SWS_DoIP_00099 */
    payload.push_back( fields.currentlyOpenSockets );

    /* @SWS_DoIP_00100 */
    if ( fields.includeMaxDataSize )
    {
        payload.push_back(static_cast<std::uint8_t>((fields.maxDataSize >> 24) & 0xFFU));
        payload.push_back(static_cast<std::uint8_t>((fields.maxDataSize >> 16) & 0xFFU));
        payload.push_back(static_cast<std::uint8_t>((fields.maxDataSize >> 8) & 0xFFU));
        payload.push_back(static_cast<std::uint8_t>(fields.maxDataSize & 0xFFU));
    }

    /* @SWS_DoIP_00096 */
    return buildFrame( PayloadType::EntityStatusResponse, payload );
}

std::vector<std::uint8_t> NodeInformationMessage::buildPowerModeResponse( std::uint8_t powerMode )
{
    const std::vector<std::uint8_t> payload( 1U, powerMode );

    /* @SWS_DoIP_00092 */
    return buildFrame( PayloadType::PowerModeInformationResponse, payload );
}

} // namespace doip
} // namespace as
