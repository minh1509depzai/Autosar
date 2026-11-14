#ifndef DOIP_ALIVE_CHECK_EXPORT_HPP
#define DOIP_ALIVE_CHECK_EXPORT_HPP

#include "AliveCheckManager.hpp"

/*
 * Name mapping only (no wrapper functions): public names resolve directly
 * to the AliveCheckManager singleton methods.
 */
#define AliveCheck_start ::as::doip::AliveCheckManager::instance().startAliveCheck
#define AliveCheck_main  ::as::doip::AliveCheckManager::instance().main
#define AliveCheck_onMessageReceived ::as::doip::AliveCheckManager::instance().onMessageReceived

#endif
