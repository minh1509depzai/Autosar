#ifndef DOIP_PENDING_ALIVE_CHECK_HPP
#define DOIP_PENDING_ALIVE_CHECK_HPP

#include <chrono>
#include <cstdint>

namespace as {
namespace doip {

struct PendingAliveCheck
{
	std::uint16_t registeredSourceAddress = 0;

	std::chrono::steady_clock::time_point deadline;
};

} // namespace doip
} // namespace as

#endif
