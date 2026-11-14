#ifndef DOIP_DIAGNOSTIC_MESSAGE_HPP
#define DOIP_DIAGNOSTIC_MESSAGE_HPP

#include <cstdint>
#include <vector>

namespace as {
namespace doip {

enum class DiagnosticNackCode : std::uint8_t
{
    InvalidSourceAddress = 0x02U,
    UnknownTargetAddress = 0x03U,
    DiagnosticMessageTooLarge = 0x04U,
    OutOfMemory = 0x05U,
    TargetNotActivated = 0x06U
};

class DiagnosticMessage
{
public:
    bool parse(const std::vector<std::uint8_t>& payload);

    std::uint16_t sourceAddress() const;
    std::uint16_t targetAddress() const;
    const std::vector<std::uint8_t>& userData() const;

    std::vector<std::uint8_t> buildPositiveAcknowledge(
        std::uint32_t previousMessageBytes) const;
    std::vector<std::uint8_t> buildNegativeAcknowledge(
        DiagnosticNackCode nackCode,
        std::uint32_t previousMessageBytes) const;
    std::vector<std::uint8_t> buildResponse(
        const std::vector<std::uint8_t>& userData) const;

private:
    std::uint16_t _sourceAddress = 0U;
    std::uint16_t _targetAddress = 0U;
    std::vector<std::uint8_t> _userData;
};

} // namespace doip
} // namespace as

#endif
