#ifndef DOIP_ALIVE_CHECK_MESSAGE_HPP
#define DOIP_ALIVE_CHECK_MESSAGE_HPP

#include <cstdint>
#include <vector>

namespace as {
namespace doip {

/*
 * Unified Alive Check message (request 0x0007 / response 0x0008).
 * The generic header must already be validated by DoIPMessage::parse.
 */
class AliveCheckMessage
{
	public:
		static std::vector<std::uint8_t> buildRequest( std::uint8_t protocolVersion );

		static std::vector<std::uint8_t> buildResponse( std::uint8_t protocolVersion,
														std::uint16_t sourceAddress );

		bool parse( const std::vector<std::uint8_t>& message );

		bool isRequest() const;

		bool isResponse() const;

		std::uint16_t sourceAddress() const;

	private:
		std::uint16_t _payloadType = 0;

		std::uint16_t _sourceAddress = 0;
};

} // namespace doip
} // namespace as

#endif
