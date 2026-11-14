Question #1:
The relationship between DoIP and other modules ?

Answer #1:
# Relationship between DoIP and other AUTOSAR modules

In AUTOSAR Classic, DoIP (Diagnostics over Internet Protocol) connects vehicle diagnostics to an IP/Ethernet network. It does not replace the diagnostic application or the transport/network modules. It works with them to carry diagnostic requests from an external tester to the ECU's diagnostic services.

For example, when a diagnostic tester sends a UDS request such as `22 F1 90` (ReadDataByIdentifier), DoIP handles the diagnostic communication over IP, while the DCM processes the UDS request.

One important distinction: DoIP is the diagnostic transport path; DCM is the diagnostic service processor.

## 1. Overall AUTOSAR Classic architecture

The following is the logical relationship between the main modules involved in DoIP diagnostics. AUTOSAR's TCP/IP stack specification shows these modules in its communication-stack architecture.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://www.autosar.org\&sz=32)

AUTOSAR

+1



External diagnostic tester

UDS request: `22 F1 90`

Ethernet network

Ethernet frames, IP packets, TCP segments

Eth → EthIf → TcpIp → SoAd

Physical/network communication and socket interface

DoIP

ISO 13400 protocol, diagnostic addressing and message handling

PduR — PDU Router

Routes diagnostic data to its configured destination

DCM — Diagnostic Communication Manager

Processes UDS services and generates responses

This diagram represents the common local-diagnostics path. The response travels back through the configured routing path in the opposite direction.

## 2. What each module does

| Module       | Relationship with DoIP                                                                                                     |
| ------------ | -------------------------------------------------------------------------------------------------------------------------- |
| Eth          | Controls the Ethernet hardware through the driver interface.                                                               |
| EthIf        | Provides a standardized interface between Ethernet drivers and upper layers.                                               |
| TcpIp        | Implements IP and transport protocols such as TCP and UDP.                                                                 |
| SoAd         | Connects upper-layer AUTOSAR modules to TCP/IP sockets; handles socket data and connection notifications.                  |
| DoIP         | Implements the DoIP diagnostic protocol, including diagnostic message handling, logical addressing and routing activation. |
| PduR         | Routes diagnostic PDUs between DoIP and DCM or another configured transport module.                                        |
| DCM          | Processes UDS services, such as ReadDataByIdentifier, ECU Reset and Diagnostic Session Control.                            |
| DEM          | Manages diagnostic events and diagnostic trouble codes; DCM can access its services for relevant UDS requests.             |
| BswM / EthSM | Help manage communication modes and Ethernet-related state transitions, depending on configuration.                        |

The AUTOSAR Socket Adaptor specification explicitly describes SoAd as the lower-layer interface used by DoIP and PduR as the routing connection to the rest of the diagnostic stack.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://www.scribd.com\&sz=32)

Network Socket

+1



## 3. The three most important relationships

### A. DoIP ↔ SoAd: network communication

SoAd abstracts the socket interface. DoIP does not need to implement TCP itself.

For example, a typical DoIP ECU uses:

- TCP port `13400` for diagnostic communication.
- UDP port `13400` for certain discovery and identification messages.

A typical sequence is:

1. DoIP requests the necessary socket services through SoAd.
2. SoAd uses TcpIp to establish or manage the underlying connection.
3. Incoming data is delivered to DoIP through the configured callbacks.
4. DoIP interprets the bytes as DoIP protocol messages.

The exact socket configuration and connection handling depend on the implementation.

### B. DoIP ↔ PduR ↔ DCM: diagnostic processing

This is the most important relationship when implementing diagnostic functionality.

Suppose the tester sends:

`22 F1 90`

This is a UDS request to read a Data Identifier, commonly used for a VIN.

1. DoIP receives the diagnostic message and extracts its diagnostic payload.
2. PduR routes the payload to DCM.
3. DCM validates and processes the UDS service.
4. DCM produces a positive or negative UDS response.
5. PduR routes the response back to DoIP.
6. DoIP encapsulates the response in a DoIP diagnostic message and sends it through SoAd.

DoIP does not itself implement the business meaning of the UDS service. DCM does.

### C. DoIP ↔ EthSM / BswM: communication state management

These modules are related to communication availability, not to interpreting UDS services.

- EthSM manages Ethernet communication modes and state transitions.
- BswM evaluates configured mode conditions and coordinates actions across modules.
- DoIP can react to relevant communication-state changes and manage its own protocol behavior.

Their exact interactions depend on the AUTOSAR release, BSW configuration and vendor implementation.

## 4. What if the ECU is a diagnostic gateway?

DoIP does not always route requests to the local DCM.

External tester

DoIP → PduR

Local route

DCM

Diagnostics for this ECU

Gateway route

CanTp

Diagnostics forwarded to a CAN ECU

PduR's configured routing determines which destination receives the diagnostic payload. In a gateway scenario, the request can be forwarded to CanTp and then transmitted over CAN to another ECU. The response follows the reverse path.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://www.scribd.com\&sz=32)

Network Socket

+1



## 5. How DoIP differs from SOME/IP

Both use the Ethernet/IP communication stack, but they serve different purposes.

| Aspect                        | DoIP                                                                       | SOME/IP                                                                 |
| ----------------------------- | -------------------------------------------------------------------------- | ----------------------------------------------------------------------- |
| Primary purpose               | Vehicle diagnostics                                                        | Service-oriented application communication                              |
| Standard                      | ISO 13400                                                                  | AUTOSAR SOME/IP specifications                                          |
| Main upper-layer relationship | PduR and DCM                                                               | Service/application communication components                            |
| Protocol behavior             | Diagnostic addressing, routing activation and diagnostic message transport | Service methods, events, notifications and optionally service discovery |
| Typical user                  | Diagnostic tester                                                          | Vehicle application or ECU service consumer                             |

They can share Eth, EthIf, TcpIp and SoAd infrastructure while using separate protocol-specific modules and configurations.

Recommended learning order: understand SoAd and TcpIp first, then DoIP's protocol handling, then PduR routing, and finally DCM's UDS processing. This follows the actual data path and makes the interfaces much easier to understand.

Source note: The PDF currently attached to this project is Specification of Default Error Tracer (AUTOSAR CP R21-11), not the DoIP specification. The explanation above therefore uses AUTOSAR communication-stack references rather than claiming these relationships were extracted from that PDF.