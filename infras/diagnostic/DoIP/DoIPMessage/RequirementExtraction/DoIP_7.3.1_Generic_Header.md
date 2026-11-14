# 7.3.1 Generic DoIP header

**Source:** `AUTOSAR_SWS_DiagnosticOverIP.pdf`  
**Document:** Specification of Diagnostic over IP — AUTOSAR CP R21-11  
**Source pages:** 28–29 of 126 (Document ID 418)

All Pdus received or sent via the SoAd shall support the the DoIP header structure as defined in the ISO 13400-2 [7] table 11. The DoIP header is described in this chapter.

**[SWS_DoIP_00004]** The first 8 Bytes of a DoIP message shall contain the DoIP Header followed by the actual payload data.  
*(SRS_Eth_00025)*

| Item | Position (Byte) | Length (Byte) |
|---|---:|---:|
| **Generic DoIP header synchronization pattern** |  |  |
| Protocol version | 0 | 1 |
| Inverse protocol version | 1 | 1 |
| **Generic DoIP payload type and payload length** |  |  |
| Payload type | 2 | 2 |
| Payload length | 4 | 4 |
| Payload type specific message content | 8 | ... |

*Table 7.1: DoIP message Generic header Layout*

**[SWS_DoIP_00005]** Byte 0 of the DoIP header has to contain the protocol version e.g. `0x02`.  
*(SRS_Eth_00025)*

**[SWS_DoIP_00006]** The Byte 1 of the DoIP header shall contain the inverse protocol version e.g. `0xFD` value shall be added if the protocol version is `0x02`.  
*(SRS_Eth_00025)*

**[SWS_DoIP_00007]** Byte 2 and Byte 3 shall contain the PayloadType.  
*(SRS_Eth_00025)*

**[SWS_DoIP_00008]** The following PayloadTypes shall be supported for reception of DoIP messages:  
*(SRS_Eth_00025)*

| Payload Type value | Payload type name | Chapter in DoIP SWS | Connection Kind |
|---|---|---|---|
| `0x0000` | Generic DoIP header negative acknowledge | 7.3.2.1 | UDP/TCP |
| `0x0001` | Vehicle Identification request message | 7.3.2.2.1 | UDP |
| `0x0002` | Vehicle identification request message with EID | 7.3.2.2.2 | UDP |
| `0x0003` | Vehicle identification request message with VIN | 7.3.2.2.3 | UDP |
| `0x0004` | Vehicle announcement message/vehicle identification response message | 7.3.2.2.1 | UDP |
| `0x0005` | Routing activation request | 7.3.2.3.1 | TCP |
| `0x0008` | Alive Check response | 7.3.2.4.2 | TCP |
| `0x4001` | DoIP entity status request | 7.3.2.5.3 | UDP |
| `0x4003` | Diagnostic power mode information request | 7.3.2.5.1 | UDP |
| `0x8001` | Diagnostic message | 7.3.2.6.1 | TCP |

*Table 7.2: DoIP payload types received by a DoIP entity, chapter reference and the connection type they are received on*

**[SWS_DoIP_00009]** The following PayloadTypes shall be supported for sending of DoIP messages:  
*(SRS_Eth_00025)*

| Payload Type value | Payload type name | Chapter in DoIP SWS | Connection Kind |
|---|---|---|---|
| `0x0000` | Generic DoIP header negative acknowledge | 7.3.2.1 | UDP/TCP |
| `0x0004` | Vehicle announcement message/vehicle identification response | 7.3.2.2.4 | UDP |
| `0x0006` | Routing activation response | 7.3.2.3.2 | TCP |
| `0x0007` | Alive Check request | 7.3.2.4.1 | TCP |
| `0x4002` | DoIP entity status response | 7.3.2.5.4 | UDP |
| `0x4004` | Diagnostic power mode information response | 7.3.2.5.2 | UDP |
| `0x8002` | Diagnostic message positive acknowledgement | 7.3.2.6.2 | TCP |
| `0x8003` | Diagnostic message negative acknowledgement | 7.3.2.6.3 | TCP |

*Table 7.3: DoIP payload types transmitted by a DoIP entity, chapter reference and the connection type they are transmitted on*

**[SWS_DoIP_00010]** Bytes 4 to 7 shall contain the payload length in Bytes not including the length of the DoIP header information (i.e. if a DoIP message is received with Payload length set to 2 it means that 10 Bytes in total were received).  
*(SRS_Eth_00025)*
