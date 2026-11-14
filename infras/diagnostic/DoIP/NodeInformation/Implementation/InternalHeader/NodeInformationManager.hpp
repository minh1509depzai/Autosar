#ifndef DOIP_NODE_INFORMATION_MANAGER_HPP
#define DOIP_NODE_INFORMATION_MANAGER_HPP

#include "NodeInformationImport.hpp"

#include <cstdint>
#include <vector>

namespace as {
namespace doip {

class NodeInformationManager
{
public:
    static NodeInformationManager& instance( );

    NodeInformationManager( const NodeInformationManager&) = delete;
    NodeInformationManager& operator=( const NodeInformationManager& ) = delete;

    void onMessageReceived( ConnectionId connectionId,
                            std::uint16_t payloadType,
                            const std::vector<std::uint8_t>& payload );

private:
    NodeInformationManager( ) = default;

    void handleEntityStatusRequest( ConnectionId udpConnectionId );

    void handlePowerModeRequest( ConnectionId udpConnectionId );
};

} // namespace doip
} // namespace as

#endif
