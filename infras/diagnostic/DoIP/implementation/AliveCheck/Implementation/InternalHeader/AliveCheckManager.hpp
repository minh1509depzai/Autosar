#ifndef DOIP_ALIVE_CHECK_MANAGER_HPP
#define DOIP_ALIVE_CHECK_MANAGER_HPP

#include "AliveCheckImport.hpp"
#include "PendingAliveCheck.hpp"

#include <cstdint>
#include <map>
#include <vector>

namespace as {
namespace doip {

class AliveCheckManager
{
	public:
		static AliveCheckManager& instance();

		AliveCheckManager( const AliveCheckManager& ) = delete;

		AliveCheckManager& operator=( const AliveCheckManager& ) = delete;

		bool startAliveCheck( ConnectionId connectionId, std::uint16_t registeredSourceAddress );

		// Message is already dispatched as an Alive Check message.
		void onMessageReceived( ConnectionId connectionId, const std::vector<std::uint8_t>& message );

		// Must be called every 100 ms; loops through the pending connections
		// and resets the ones whose response deadline has expired.
		void main();

	private:
		AliveCheckManager() = default;

		void resetConnection( ConnectionId connectionId );

		std::map<ConnectionId, PendingAliveCheck> _pendingAliveCheck;
};

} // namespace doip
} // namespace as

#endif
