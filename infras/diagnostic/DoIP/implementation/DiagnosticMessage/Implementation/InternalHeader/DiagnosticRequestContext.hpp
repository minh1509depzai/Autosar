#ifndef DOIP_DIAGNOSTIC_REQUEST_CONTEXT_HPP
#define DOIP_DIAGNOSTIC_REQUEST_CONTEXT_HPP

#include "DiagnosticMessage.hpp"
#include "DiagnosticMessageImport.hpp"

namespace as {
namespace doip {

struct DiagnosticRequestContext
{
    ConnectionId connectionId;
    PduIdType rxPduId;
    PduIdType txPduId;
    std::uint32_t ackNackBytes;
    DiagnosticMessage requestMessage;
};

} // namespace doip
} // namespace as

#endif
