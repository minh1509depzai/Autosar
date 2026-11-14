#ifndef DOIP_ALIVE_CHECK_IMPORT_HPP
#define DOIP_ALIVE_CHECK_IMPORT_HPP

#include <cstdint>
#include <vector>

namespace as {
namespace doip {

using ConnectionId = std::uint64_t;

// Functions provided by AliveCheckCommunication (external module).
bool AliveCheckCommunication_sendAliveCheck( ConnectionId connectionId,
											 const std::vector<std::uint8_t>& message );

void AliveCheckCommunication_resetSocketConnection( ConnectionId connectionId );

} // namespace doip
} // namespace as

/*
 * Name mapping only (no wrapper functions): the names used inside the module
 * are aliases that resolve directly to the real functions.
 */
#define AliveCheck_send              ::as::doip::AliveCheckCommunication_sendAliveCheck
#define AliveCheck_resetConnection   ::as::doip::AliveCheckCommunication_resetSocketConnection

#endif
