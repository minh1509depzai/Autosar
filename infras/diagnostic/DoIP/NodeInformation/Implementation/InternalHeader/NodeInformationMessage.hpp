#ifndef DOIP_NODE_INFORMATION_MESSAGE_HPP
#define DOIP_NODE_INFORMATION_MESSAGE_HPP

#include "DoIPMessage.hpp"
#include "EntityStatusResponseData.hpp"

#include <cstdint>
#include <vector>

namespace as {
namespace doip {

class NodeInformationMessage : public DoIPMessage
{
public:
    static std::vector<std::uint8_t> buildEntityStatusResponse( const EntityStatusResponseData& fields );

    static std::vector<std::uint8_t> buildPowerModeResponse( std::uint8_t powerMode );
};

} // namespace doip
} // namespace as

#endif
