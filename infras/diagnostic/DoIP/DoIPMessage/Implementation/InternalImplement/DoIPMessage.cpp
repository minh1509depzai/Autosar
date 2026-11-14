#include "DoIPMessage.hpp"
#include "Configuration.hpp"

#include <algorithm>
#include <stdexcept>

namespace as {
namespace doip {
namespace {
	std::uint16_t read_u16_be(const std::uint8_t* bytes)
	{
		return static_cast<std::uint16_t>( ( static_cast<std::uint16_t>(bytes[0]) << 8) |
										     static_cast<std::uint16_t>(bytes[1]) );
	}

	std::uint32_t read_u32_be(const std::uint8_t* bytes)
	{
	return (static_cast<std::uint32_t>(bytes[0]) << 24) |
			(static_cast<std::uint32_t>(bytes[1]) << 16) |
			(static_cast<std::uint32_t>(bytes[2]) << 8) |
			static_cast<std::uint32_t>(bytes[3]);
	}

	} // namespace

	bool DoIPMessage::isSupportedPayloadType(std::uint16_t getPayloadType)
	{
		// SWS_DoIP_00008, Table 7.2.
		switch (static_cast<PayloadType>(getPayloadType)) 
		{
			case PayloadType::GenericNegativeAck:
			case PayloadType::VehicleIdentificationRequest:
			case PayloadType::VehicleIdentificationRequestWithEid:
			case PayloadType::VehicleIdentificationRequestWithVin:
			case PayloadType::VehicleAnnouncementOrIdentificationResponse:
			case PayloadType::RoutingActivationRequest:
			case PayloadType::AliveCheckRequest:
			case PayloadType::AliveCheckResponse:
			case PayloadType::EntityStatusRequest:
			/* @SWS_DoIP_00096 */
			case PayloadType::EntityStatusResponse:
			case PayloadType::PowerModeInformationRequest:
			/* @SWS_DoIP_00092 */
			case PayloadType::PowerModeInformationResponse:
			case PayloadType::DiagnosticMessage:
				return true;
			default:
				return false;
		}
	}

	bool DoIPMessage::isPayloadLengthValid( std::uint16_t getPayloadType,
											std::size_t payload_length)
	{
		// Payload lengths per payload type as defined by ISO 13400-2.
		switch (static_cast<PayloadType>(getPayloadType)) 
		{
			case PayloadType::GenericNegativeAck:
				return payload_length == 1;
			case PayloadType::VehicleIdentificationRequest:
			case PayloadType::EntityStatusRequest:
			case PayloadType::PowerModeInformationRequest:
				return payload_length == 0;
			/* @SWS_DoIP_00096 */
			case PayloadType::EntityStatusResponse:
				return payload_length == 3 || payload_length == 7;
			/* @SWS_DoIP_00092 */
			case PayloadType::PowerModeInformationResponse:
				return payload_length == 1;
			case PayloadType::VehicleIdentificationRequestWithEid:
				return payload_length == 6;
			case PayloadType::VehicleIdentificationRequestWithVin:
				return payload_length == 17;
			case PayloadType::VehicleAnnouncementOrIdentificationResponse:
				return payload_length == 32 || payload_length == 33;
			case PayloadType::RoutingActivationRequest:
				return payload_length == 7 || payload_length == 11;
			case PayloadType::AliveCheckResponse:
				return payload_length == 2;
			case PayloadType::DiagnosticMessage:
				return payload_length >= 5;
			default:
				return false;
		}
	}

	AcknowledgeCode DoIPMessage::parse( const std::vector<std::uint8_t>& message )
	{

		// SWS_DoIP_00004: the header occupies the first 8 bytes.
		if ( message.size() < DOIP_MESSAGE_HEADER_LENGTH ) 
		{
			return AcknowledgeCode::IncorrectPatternFormat;
		}

		// SWS_DoIP_00005 / SWS_DoIP_00006 / SWS_DoIP_00014.
		mProtocolVersion = message[0];

		if ( message[0] != mProtocolVersion ||
			 message[1] != static_cast<std::uint8_t>(~mProtocolVersion)) 
		{
			return AcknowledgeCode::IncorrectPatternFormat;
		}
		
		// SWS_DoIP_00007 / SWS_DoIP_00016.
		mPayloadType = read_u16_be(&message[2]);

		if ( !isSupportedPayloadType(mPayloadType) ) 
		{
			return AcknowledgeCode::UnknownPayloadType;
		}

		// SWS_DoIP_00010: the length excludes the header.
		const std::uint32_t _payloadLength = read_u32_be(&message[4]);

		// SWS_DoIP_00017.
		if ( _payloadLength > DoIPMaxRequestBytes ) 
		{
			return AcknowledgeCode::MessageTooLarge;
		}

		// SWS_DoIP_00018.
		if ( _payloadLength > DOIP_MESSAGE_MAX_BUFFER_LENGTH ) 
		{
			return AcknowledgeCode::OutOfMemory;
		}

		// SWS_DoIP_00019: the length must fit the type and the received bytes.
		if ( !isPayloadLengthValid(mPayloadType, _payloadLength) ) 
		{
			return AcknowledgeCode::InvalidPayloadLength;
		}

		// All check passed and the DoIP message from now on is valid.
		mIsValid = true;

		return AcknowledgeCode::Accepted;
	}

	DoIPMessage DoIPMessage::build( PayloadType payloadType,
									const std::vector<std::uint8_t>& payload,
									BuildMessageErrorCode& errorCode )
	{
		errorCode = BuildMessageErrorCode::Success;

		/* @SWS_DoIP_00008 */
		/* @SWS_DoIP_00129 */
		/* @SWS_DoIP_00137 */
		if ( !isSupportedPayloadType( static_cast<std::uint16_t>( payloadType ) ) &&
		     payloadType != PayloadType::DiagnosticPositiveAcknowledge &&
		     payloadType != PayloadType::DiagnosticNegativeAcknowledge )
		{
			errorCode = BuildMessageErrorCode::UnknownPayloadType;
			return DoIPMessage();
		}

		if ( payload.size() > DOIP_MESSAGE_MAX_BUFFER_LENGTH )
		{
			errorCode = BuildMessageErrorCode::InvalidPayloadLength;
			return DoIPMessage();
		}

		DoIPMessage message;
		message.mProtocolVersion = DoIPProtocolVersion;
		message.mPayloadType = static_cast<std::uint16_t>(payloadType);
		message.mPayload = payload;
		message.mIsValid = true;
		return message;
	}

	std::vector<std::uint8_t> DoIPMessage::toBytes() const
	{
		std::vector<std::uint8_t> bytes(DOIP_MESSAGE_HEADER_LENGTH + mPayload.size(), 0);

		/* @SWS_DoIP_00005 */
		bytes[0] = mProtocolVersion;
		bytes[1] = static_cast<std::uint8_t>(~mProtocolVersion);

		/* @SWS_DoIP_00007 */
		bytes[2] = static_cast<std::uint8_t>((mPayloadType >> 8) & 0xFF);
		bytes[3] = static_cast<std::uint8_t>(mPayloadType & 0xFF);

		/* @SWS_DoIP_00010 */
		const std::uint32_t payloadLength = static_cast<std::uint32_t>(mPayload.size());
		bytes[4] = static_cast<std::uint8_t>((payloadLength >> 24) & 0xFF);
		bytes[5] = static_cast<std::uint8_t>((payloadLength >> 16) & 0xFF);
		bytes[6] = static_cast<std::uint8_t>((payloadLength >> 8) & 0xFF);
		bytes[7] = static_cast<std::uint8_t>(payloadLength & 0xFF);

		std::copy(mPayload.begin(), mPayload.end(), bytes.begin() + DOIP_MESSAGE_HEADER_LENGTH);
		return bytes;
	}

	std::uint8_t DoIPMessage::getProtocolVersion() const
	{
		return mProtocolVersion;
	}

	std::uint16_t DoIPMessage::getPayloadType() const
	{
		return mPayloadType;
	}

	const std::vector<std::uint8_t>& DoIPMessage::getPayload() const
	{
		return mPayload;
	}

} // namespace doip
} // namespace as
