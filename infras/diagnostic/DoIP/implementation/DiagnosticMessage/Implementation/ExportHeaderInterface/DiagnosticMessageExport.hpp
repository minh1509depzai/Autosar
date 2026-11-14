#ifndef DOIP_DIAGNOSTIC_MESSAGE_EXPORT_HPP
#define DOIP_DIAGNOSTIC_MESSAGE_EXPORT_HPP

#include "DiagnosticMessageManager.hpp"

#define DiagnosticMessage_onMessageReceived \
    ::as::doip::DiagnosticMessageManager::instance().onMessageReceived
#define DiagnosticMessage_onConnectionClosed \
    ::as::doip::DiagnosticMessageManager::instance().onConnectionClosed
#define DiagnosticMessage_TpTransmit \
    ::as::doip::DiagnosticMessageManager::instance().tpTransmit
#define DiagnosticMessage_IfTransmit \
    ::as::doip::DiagnosticMessageManager::instance().ifTransmit

#endif
