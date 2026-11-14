#include "AliveCheckManager.hpp"
#include "AliveCheckMessage.hpp"
#include "Configuration.hpp"
#include "DoIPMessage.hpp"

#include <chrono>

namespace as {
namespace doip {

AliveCheckManager& AliveCheckManager::instance()
{
	static AliveCheckManager manager;
	return manager;
}

bool AliveCheckManager::startAliveCheck( ConnectionId connectionId, std::uint16_t registeredSourceAddress )
{
	if ( _pendingAliveCheck.find( connectionId ) != _pendingAliveCheck.end() )
	{
		return false;
	}

	/* @SWS_DoIP_00139 */
	if ( !AliveCheck_send( connectionId, AliveCheckMessage::buildRequest( DoIPProtocolVersion ) ) )
	{
		return false;
	}

	/* @SWS_DoIP_00140 */
	PendingAliveCheck pending;
	pending.registeredSourceAddress = registeredSourceAddress;
	pending.deadline = std::chrono::steady_clock::now() +
					   std::chrono::milliseconds( static_cast<long long>( DoIPAliveCheckResponseTimeout ) );
	_pendingAliveCheck[connectionId] = pending;

	return true;
}

void AliveCheckManager::onMessageReceived( ConnectionId connectionId, const std::vector<std::uint8_t>& message )
{
	AliveCheckMessage aliveCheckMessage;

	if ( !aliveCheckMessage.parse( message ) )
	{
		return;
	}

	if ( aliveCheckMessage.isRequest() )
	{
		// Request from a tester: answer with this ECU's logical address.
		AliveCheck_send( connectionId,
						 AliveCheckMessage::buildResponse( DoIPProtocolVersion,
														   static_cast<std::uint16_t>( DoIPLogicalAddress ) ) );
		return;
	}

	std::map<ConnectionId, PendingAliveCheck>::iterator it = _pendingAliveCheck.find( connectionId );

	if ( it == _pendingAliveCheck.end() )
	{
		return;
	}

	/* @SWS_DoIP_00141 */
	if ( aliveCheckMessage.sourceAddress() == it->second.registeredSourceAddress )
	{
		_pendingAliveCheck.erase( it );
	}
	else
	{
		resetConnection( connectionId );
	}
}

void AliveCheckManager::main()
{
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::vector<ConnectionId> expired;

	for ( std::map<ConnectionId, PendingAliveCheck>::const_iterator it = _pendingAliveCheck.begin();
		  it != _pendingAliveCheck.end(); ++it )
	{
		/* @SWS_DoIP_00140 */
		if ( now >= it->second.deadline )
		{
			expired.push_back( it->first );
		}
	}

	for ( std::size_t i = 0; i < expired.size(); ++i )
	{
		resetConnection( expired[i] );
	}
}

void AliveCheckManager::resetConnection( ConnectionId connectionId )
{
	_pendingAliveCheck.erase( connectionId );

	/* @SWS_DoIP_00358 */
	AliveCheck_resetConnection( connectionId );
}

} // namespace doip
} // namespace as
