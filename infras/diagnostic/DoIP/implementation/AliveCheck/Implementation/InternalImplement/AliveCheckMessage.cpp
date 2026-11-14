#include "AliveCheckMessage.hpp"
#include "DoIPMessage.hpp"

namespace as {
namespace doip {
namespace {
	const std::size_t RESPONSE_LENGTH = DOIP_MESSAGE_HEADER_LENGTH + 2;

	void appendHeader( std::vector<std::uint8_t>& bytes,
					   std::uint8_t protocolVersion,
					   PayloadType payloadType,
					   std::uint32_t payloadLength )
	{
		const std::uint16_t type = static_cast<std::uint16_t>( payloadType );

		bytes.push_back( protocolVersion );
		bytes.push_back( static_cast<std::uint8_t>( ~protocolVersion ) );
		bytes.push_back( static_cast<std::uint8_t>( ( type >> 8 ) & 0xFF ) );
		bytes.push_back( static_cast<std::uint8_t>( type & 0xFF ) );
		bytes.push_back( static_cast<std::uint8_t>( ( payloadLength >> 24 ) & 0xFF ) );
		bytes.push_back( static_cast<std::uint8_t>( ( payloadLength >> 16 ) & 0xFF ) );
		bytes.push_back( static_cast<std::uint8_t>( ( payloadLength >> 8 ) & 0xFF ) );
		bytes.push_back( static_cast<std::uint8_t>( payloadLength & 0xFF ) );
	}
} // namespace

std::vector<std::uint8_t> AliveCheckMessage::buildRequest( std::uint8_t protocolVersion )
{
	std::vector<std::uint8_t> bytes;

	/* @SWS_DoIP_00139 */
	appendHeader( bytes, protocolVersion, PayloadType::AliveCheckRequest, 0 );

	return bytes;
}

std::vector<std::uint8_t> AliveCheckMessage::buildResponse( std::uint8_t protocolVersion,
															std::uint16_t sourceAddress )
{
	std::vector<std::uint8_t> bytes;

	appendHeader( bytes, protocolVersion, PayloadType::AliveCheckResponse, 2 );
	bytes.push_back( static_cast<std::uint8_t>( ( sourceAddress >> 8 ) & 0xFF ) );
	bytes.push_back( static_cast<std::uint8_t>( sourceAddress & 0xFF ) );

	return bytes;
}

bool AliveCheckMessage::parse( const std::vector<std::uint8_t>& message )
{
	if ( message.size() < DOIP_MESSAGE_HEADER_LENGTH )
	{
		return false;
	}

	_payloadType = static_cast<std::uint16_t>( ( message[2] << 8 ) | message[3] );

	if ( isRequest() )
	{
		return true;
	}

	if ( isResponse() && message.size() >= RESPONSE_LENGTH )
	{
		/* @SWS_DoIP_00141 */
		_sourceAddress = static_cast<std::uint16_t>( ( message[DOIP_MESSAGE_HEADER_LENGTH] << 8 ) |
													 message[DOIP_MESSAGE_HEADER_LENGTH + 1] );
		return true;
	}

	return false;
}

bool AliveCheckMessage::isRequest() const
{
	return _payloadType == static_cast<std::uint16_t>( PayloadType::AliveCheckRequest );
}

bool AliveCheckMessage::isResponse() const
{
	return _payloadType == static_cast<std::uint16_t>( PayloadType::AliveCheckResponse );
}

std::uint16_t AliveCheckMessage::sourceAddress() const
{
	return _sourceAddress;
}

} // namespace doip
} // namespace as
