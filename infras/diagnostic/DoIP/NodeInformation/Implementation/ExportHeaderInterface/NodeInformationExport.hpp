#ifndef DOIP_NODE_INFORMATION_EXPORT_HPP
#define DOIP_NODE_INFORMATION_EXPORT_HPP

#include "NodeInformationManager.hpp"

#define NodeInformation_onMessageReceived ::as::doip::NodeInformationManager::instance().onMessageReceived

#endif
