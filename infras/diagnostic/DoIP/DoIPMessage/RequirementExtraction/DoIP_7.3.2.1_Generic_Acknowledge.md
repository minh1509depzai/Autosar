# 7.3.2.1 Generic acknowledge

**Source:** `AUTOSAR_SWS_DiagnosticOverIP.pdf`  
**Document:** Specification of Diagnostic over IP — AUTOSAR CP R21-11  
**Source page:** 30 of 126 (Document ID 418)

This chapter contains the check of the DoIP header with the according negative acknowledge messages with payload type `0x0000` for an invalid DoIP header.

**[SWS_DoIP_00012]** If an invalid DoIP header was received, a DoIP message with payload type `0x0000` shall be transmitted with the payload described in [SWS_DoIP_00013] on the TxPdu which is related to the RxPdu the message was received on, if the according SocketConnection status has not changed since the reception of the DoIP message.  
*(SRS_Eth_00025)*

**[SWS_DoIP_00013]** The payload of the generic DoIP header shall contain the corresponding NACK code (1 Byte) as specified from [SWS_DoIP_00014] to [SWS_DoIP_00019].  
*(SRS_Eth_00025)*

**[SWS_DoIP_00014]** If the Protocol information is incorrect, (see [SWS_DoIP_00005], [SWS_DoIP_00006] and [SWS_DoIP_00015] for valid information) the NACK code `0x00` shall be sent and the according socket shall be closed (see [SWS_DoIP_00058]).  
*(SRS_Eth_00025)*

**[SWS_DoIP_00016]** If a payload type is not supported (see [SWS_DoIP_00008] for valid payload types) the DoIP module shall send the NACK code `0x01` to indicate that a unknown payload type was requested. The message shall be discarded for further processing.  
*(SRS_Eth_00025)*

**[SWS_DoIP_00017]** If the payload length exceeds the value configured by `DoIPMaxRequestBytes`, the DoIP module shall send the NACK code `0x02` to indicate that the message is too large. The message shall be discarded for further processing.  
*(SRS_Eth_00025)*

**[SWS_DoIP_00018]** If the DoIP module is called with `DoIP_SoAdTpStartOfReception()` and the indicated payload length exceeds the currently available buffer size, the function must return with `BUFREQ_E_OVFL` value (No buffer of the required length can be provided) and trigger a Negative Response (NACK) with value `0x03`. The currently available buffer size calculation shall be based on Payload Type. If the DoIP message is processed internally (see [SWS_DoIP_00008]) the locally available buffer, other case the upper layer (`PduR_DoIPTpStartOfReception`) provided buffer size shall be the base for the response.  
*(SRS_Eth_00025)*

**[SWS_DoIP_00019]** If the DoIP module is called with a payload length that is not valid for the specific payload type, the NACK code `0x04` shall be sent and the according socket shall be closed (see [SWS_DoIP_00058]).  
*(SRS_Eth_00025)*

**Note:** The single valid payload length ranges for the single payload types are described in the single subchapters of the payloads (see [SWS_DoIP_00008] for the list of all receive payload types and the according chapter references).

**[SWS_DoIP_00292]** If a DoIP message with payload Type `0x0000` is received on a configured `DoIPUdpConnection` or `DoIPTcpConnection`, the message shall be discarded.  
*(SRS_Eth_00025)*
