#ifndef DOIP_ENTITY_STATUS_RESPONSE_DATA_HPP
#define DOIP_ENTITY_STATUS_RESPONSE_DATA_HPP

#include <cstdint>

namespace as {
namespace doip {

struct EntityStatusResponseData
{
    std::uint8_t nodeType;
    std::uint8_t maxOpenSockets;
    std::uint8_t currentlyOpenSockets;
    std::uint32_t maxDataSize;
    bool includeMaxDataSize;
};

} // namespace doip
} // namespace as

#endif
