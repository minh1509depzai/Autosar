#ifndef DOIP_MESSAGE_HPP
#define DOIP_MESSAGE_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace as {
namespace doip {

// SWS_DoIP_00004: the header occupies the first 8 bytes.
#define DOIP_MESSAGE_HEADER_LENGTH 8
#define DOIP_MESSAGE_MAX_BUFFER_LENGTH 4096

#ifndef DoIPProtocolVersion
#define DoIPProtocolVersion 0x02
#endif

enum class BuildMessageErrorCode : std::uint8_t
{
	Success = 0x00,
	UnknownPayloadType = 0x01,
	InvalidPayloadLength = 0x02
};

/*
 * Result of checking a received DoIP header.
 * Values other than Accepted are the NACK codes of the generic DoIP header
 * negative acknowledge (payload type 0x0000), SWS_DoIP_00013..00019.
 */
enum class AcknowledgeCode : std::uint8_t 
{
	IncorrectPatternFormat = 0x00,
	UnknownPayloadType = 0x01,
	MessageTooLarge = 0x02,
	OutOfMemory = 0x03,
	InvalidPayloadLength = 0x04,
	Accepted = 0xFF
};

/*
 * DoIP payload types accepted on reception (SWS_DoIP_00008, Table 7.2).
 * The value is the 2-byte payload type of the generic header (bytes 2-3).
 */
enum class PayloadType : std::uint16_t 
{
    GenericNegativeAck = 0x0000,
    VehicleIdentificationRequest = 0x0001,
    VehicleIdentificationRequestWithEid = 0x0002,
    VehicleIdentificationRequestWithVin = 0x0003,
    VehicleAnnouncementOrIdentificationResponse = 0x0004,
    RoutingActivationRequest = 0x0005,
	AliveCheckRequest = 0x0007,
    AliveCheckResponse = 0x0008,
    EntityStatusRequest = 0x4001,
    EntityStatusResponse = 0x4002,
    PowerModeInformationRequest = 0x4003,
    PowerModeInformationResponse = 0x4004,
    DiagnosticMessage = 0x8001,
    DiagnosticPositiveAcknowledge = 0x8002,
    DiagnosticNegativeAcknowledge = 0x8003
};

class DoIPMessage 
{
    public:
		DoIPMessage( ) = default;

		virtual ~DoIPMessage() = default;

		AcknowledgeCode parse( const std::vector<std::uint8_t>& message ); 

		static DoIPMessage build( PayloadType payloadType,
								  const std::vector<std::uint8_t>& payload,
								  BuildMessageErrorCode& errorCode );

		std::vector<std::uint8_t> toBytes() const;

		std::uint8_t getProtocolVersion() const;

		std::uint16_t getPayloadType() const;

		const std::vector<std::uint8_t>& getPayload() const;

    protected:
		static bool isSupportedPayloadType(std::uint16_t getPayloadType);

		static bool isPayloadLengthValid(std::uint16_t getPayloadType,
											std::size_t payload_length);

	private:
		std::uint8_t mProtocolVersion = 0;
		
		std::uint16_t mPayloadType = 0;
		
		std::vector<std::uint8_t> mPayload;

		bool mIsValid = false;
};

} // namespace doip
} // namespace as

#endif
