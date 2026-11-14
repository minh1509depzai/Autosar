#include "DiagnosticMessage.hpp"

namespace as {
namespace doip {
namespace {

std::uint16_t readU16BigEndian(const std::uint8_t* bytes)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[0]) << 8U) |
        static_cast<std::uint16_t>(bytes[1]));
}

void appendU16BigEndian(std::vector<std::uint8_t>& bytes, std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

} // namespace

bool DiagnosticMessage::parse(const std::vector<std::uint8_t>& payload)
{
    /* @SWS_DoIP_00122 */
    if (payload.size() < 5U)
    {
        return false;
    }

    _sourceAddress = readU16BigEndian(payload.data());
    _targetAddress = readU16BigEndian(payload.data() + 2U);
    _userData.assign(payload.begin() + 4, payload.end());
    return true;
}

std::uint16_t DiagnosticMessage::sourceAddress() const
{
    return _sourceAddress;
}

std::uint16_t DiagnosticMessage::targetAddress() const
{
    return _targetAddress;
}

const std::vector<std::uint8_t>& DiagnosticMessage::userData() const
{
    return _userData;
}

std::vector<std::uint8_t> DiagnosticMessage::buildPositiveAcknowledge(
    std::uint32_t previousMessageBytes) const
{
    std::vector<std::uint8_t> payload;
    payload.reserve(5U);
    /* @SWS_DoIP_00132 */
    appendU16BigEndian(payload, _targetAddress);
    /* @SWS_DoIP_00133 */
    appendU16BigEndian(payload, _sourceAddress);
    /* @SWS_DoIP_00134 */
    payload.push_back(static_cast<std::uint8_t>(previousMessageBytes));
    return payload;
}

std::vector<std::uint8_t> DiagnosticMessage::buildNegativeAcknowledge(
    DiagnosticNackCode nackCode,
    std::uint32_t previousMessageBytes) const
{
    std::vector<std::uint8_t> payload;
    payload.reserve(6U);
    /* @SWS_DoIP_00135 */
    appendU16BigEndian(payload, _targetAddress);
    /* @SWS_DoIP_00136 */
    appendU16BigEndian(payload, _sourceAddress);
    /* @SWS_DoIP_00137 */
    payload.push_back(static_cast<std::uint8_t>(nackCode));
    /* @SWS_DoIP_00138 */
    payload.push_back(static_cast<std::uint8_t>(previousMessageBytes));
    return payload;
}

std::vector<std::uint8_t> DiagnosticMessage::buildResponse(
    const std::vector<std::uint8_t>& userData) const
{
    std::vector<std::uint8_t> payload;
    payload.reserve(4U + userData.size());
    /* @SWS_DoIP_00131 */
    appendU16BigEndian(payload, _targetAddress);
    appendU16BigEndian(payload, _sourceAddress);
    /* @SWS_DoIP_00173 */
    payload.insert(payload.end(), userData.begin(), userData.end());
    return payload;
}

} // namespace doip
} // namespace as
