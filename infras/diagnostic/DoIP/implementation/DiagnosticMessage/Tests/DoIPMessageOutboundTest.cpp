#include "DoIPMessage.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

int main()
{
    using namespace as::doip;

    const std::vector<std::uint8_t> payload = {0x0e, 0x00, 0x0e, 0x80, 0x00};
    const PayloadType types[] = {
        PayloadType::DiagnosticPositiveAcknowledge,
        PayloadType::DiagnosticNegativeAcknowledge
    };

    for (const PayloadType type : types)
    {
        BuildMessageErrorCode error = BuildMessageErrorCode::UnknownPayloadType;
        const DoIPMessage outbound = DoIPMessage::build(type, payload, error);
        assert(error == BuildMessageErrorCode::Success);
        const std::vector<std::uint8_t> bytes = outbound.toBytes();
        assert(bytes.size() == payload.size() + DOIP_MESSAGE_HEADER_LENGTH);
        assert(bytes[2] == 0x80);
        assert(bytes[3] == static_cast<std::uint8_t>(type));
        assert(bytes[7] == payload.size());
        assert(outbound.getPayload() == payload);

        DoIPMessage inbound;
        assert(inbound.parse(bytes) == AcknowledgeCode::UnknownPayloadType);
    }

    BuildMessageErrorCode error = BuildMessageErrorCode::Success;
    DoIPMessage::build(static_cast<PayloadType>(0xffff), payload, error);
    assert(error == BuildMessageErrorCode::UnknownPayloadType);
    return 0;
}
