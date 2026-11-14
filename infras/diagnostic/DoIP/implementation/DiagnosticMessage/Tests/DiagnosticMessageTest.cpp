#include "DiagnosticMessage.hpp"
#include "DiagnosticMessageManager.hpp"
#include "DoIPMessage.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

std::vector<std::uint8_t> sentMessage;
std::vector<std::uint8_t> copiedUserData;
bool configuredConnection = true;
bool connectionOpen = true;
bool testerRegistered = true;
bool targetActivated = true;
bool receiveAccepted = true;
bool sendResponseDuringRxIndication = false;
Std_ReturnType synchronousResponseResult = E_NOT_OK;
PduLengthType reportedBufferSize = 32U;
std::uint32_t indicatedResult = 1U;
as::doip::ConnectionId closedConnection = 0U;

void assertNegativeAcknowledge(std::uint8_t code)
{
    assert(sentMessage.size() >= 14U);
    assert(sentMessage[2] == 0x80U && sentMessage[3] == 0x03U);
    assert(sentMessage[12] == code);
    assert(sentMessage[13] == 0U);
}

} // namespace

namespace as {
namespace doip {

bool DiagnosticMessageCommunication_send(
    ConnectionId,
    const std::vector<std::uint8_t>& message)
{
    sentMessage = message;
    return true;
}

void DiagnosticMessageCommunication_closeConnection(ConnectionId connectionId)
{
    closedConnection = connectionId;
}

bool DiagnosticMessageCommunication_isConfiguredTcpConnection(ConnectionId)
{
    return configuredConnection;
}

bool DiagnosticMessageCommunication_isConnectionOpen(ConnectionId)
{
    return connectionOpen;
}

bool DiagnosticMessageRoutingActivation_isTesterRegistered(
    ConnectionId,
    std::uint16_t)
{
    return testerRegistered;
}

bool DiagnosticMessageRoutingActivation_isTargetActivated(
    ConnectionId,
    std::uint16_t,
    std::uint16_t)
{
    return targetActivated;
}

} // namespace doip
} // namespace as

extern "C" {

BufReq_ReturnType PduR_DoIPStartOfReception(
    PduIdType,
    const PduInfoType*,
    PduLengthType,
    PduLengthType* bufferSizePtr)
{
    *bufferSizePtr = reportedBufferSize;
    return BUFREQ_OK;
}

BufReq_ReturnType PduR_DoIPCopyRxData(
    PduIdType,
    const PduInfoType* info,
    PduLengthType*)
{
    copiedUserData.assign(info->SduDataPtr, info->SduDataPtr + info->SduLength);
    return receiveAccepted ? BUFREQ_OK : BUFREQ_E_NOT_OK;
}

void PduR_DoIPRxIndication(PduIdType, Std_ReturnType result)
{
    indicatedResult = result;
    if (result == E_OK && sendResponseDuringRxIndication)
    {
        std::uint8_t responseBytes[] = {0x62U, 0xF1U, 0x90U};
        PduInfoType responseInfo = {};
        responseInfo.SduDataPtr = responseBytes;
        responseInfo.SduLength = sizeof(responseBytes);
        synchronousResponseResult =
            as::doip::DiagnosticMessageManager::instance().tpTransmit(
                0U, &responseInfo);
    }
}

}

int main()
{
    using namespace as::doip;

    DiagnosticMessage parsed;
    const std::vector<std::uint8_t> request = {
        0x0EU, 0x80U, 0x0EU, 0x00U, 0x22U, 0xF1U, 0x90U
    };
    assert(parsed.parse(request));
    assert(parsed.sourceAddress() == 0x0E80U);
    assert(parsed.targetAddress() == 0x0E00U);
    assert((parsed.userData() == std::vector<std::uint8_t>{0x22U, 0xF1U, 0x90U}));
    assert(!parsed.parse(std::vector<std::uint8_t>{0x0EU, 0x80U, 0x0EU, 0x00U}));

    DiagnosticMessageManager& manager = DiagnosticMessageManager::instance();
    configuredConnection = false;
    manager.onMessageReceived(4U, 0x8001U, request);
    assert(sentMessage.empty());
    configuredConnection = true;

    testerRegistered = false;
    manager.onMessageReceived(5U, 0x8001U, request);
    assertNegativeAcknowledge(0x02U);
    assert(closedConnection == 5U);

    testerRegistered = true;
    sentMessage.clear();
    std::vector<std::uint8_t> unknownTargetRequest = request;
    unknownTargetRequest[3] = 0x01U;
    manager.onMessageReceived(5U, 0x8001U, unknownTargetRequest);
    assertNegativeAcknowledge(0x03U);

    sentMessage.clear();
    std::vector<std::uint8_t> tooLargeRequest(4093U, 0U);
    tooLargeRequest[0] = 0x0EU;
    tooLargeRequest[1] = 0x80U;
    tooLargeRequest[2] = 0x0EU;
    tooLargeRequest[3] = 0x00U;
    manager.onMessageReceived(5U, 0x8001U, tooLargeRequest);
    assertNegativeAcknowledge(0x04U);

    sentMessage.clear();
    reportedBufferSize = 2U;
    manager.onMessageReceived(5U, 0x8001U, request);
    assertNegativeAcknowledge(0x05U);
    assert(indicatedResult == E_NOT_OK);
    reportedBufferSize = 32U;

    sentMessage.clear();
    targetActivated = false;
    manager.onMessageReceived(5U, 0x8001U, request);
    assertNegativeAcknowledge(0x06U);
    assert(indicatedResult == E_NOT_OK);
    targetActivated = true;

    sentMessage.clear();
    manager.onMessageReceived(6U, 0x8001U, request);
    assert(copiedUserData == parsed.userData());
    assert(indicatedResult == E_OK);
    assert(sentMessage[2] == 0x80U && sentMessage[3] == 0x02U);
    assert(sentMessage[8] == 0x0EU && sentMessage[9] == 0x00U);
    assert(sentMessage[10] == 0x0EU && sentMessage[11] == 0x80U);
    assert(sentMessage[12] == 0U);

    const std::vector<std::uint8_t> acknowledge = sentMessage;
    manager.onMessageReceived(8U, 0x8001U, request);
    assert(sentMessage == acknowledge);
    assert(copiedUserData == parsed.userData());

    std::uint8_t responseBytes[] = {0x62U, 0xF1U, 0x90U};
    PduInfoType responseInfo = {};
    responseInfo.SduDataPtr = responseBytes;
    responseInfo.SduLength = sizeof(responseBytes);
    assert(manager.tpTransmit(0U, &responseInfo) == E_OK);
    assert(sentMessage[2] == 0x80U && sentMessage[3] == 0x01U);
    assert(sentMessage[12] == 0x62U && sentMessage[13] == 0xF1U &&
           sentMessage[14] == 0x90U);

    sentMessage.clear();
    receiveAccepted = false;
    manager.onMessageReceived(9U, 0x8001U, request);
    assert(sentMessage.empty());
    assert(indicatedResult == E_NOT_OK);
    receiveAccepted = true;

    sendResponseDuringRxIndication = true;
    manager.onMessageReceived(11U, 0x8001U, request);
    sendResponseDuringRxIndication = false;
    assert(synchronousResponseResult == E_OK);
    assert(sentMessage[2] == 0x80U && sentMessage[3] == 0x02U);

    manager.onMessageReceived(7U, 0x8001U, request);
    connectionOpen = false;
    assert(manager.ifTransmit(0U, &responseInfo) == E_NOT_OK);
    connectionOpen = true;
    assert(manager.ifTransmit(0U, &responseInfo) == E_NOT_OK);
    return 0;
}
