Question #2:
If i implement DoIP module on Linux, there's no need to take care Eth Stack as it already implemented ?

Answer #2:
Yes — if you implement a DoIP module on Linux, you generally do not need to implement the Ethernet driver, IP stack, or TCP/UDP protocols yourself. Linux already provides these capabilities.

However, DoIP is not simply an application that sends diagnostic data over a socket. You still need to implement the DoIP protocol and connect it to the diagnostic application.

## 1. Linux vs. AUTOSAR Classic stack

### Typical implementation on Linux

Diagnostic application / UDS server

Processes diagnostic services

Your DoIP implementation

ISO 13400 framing, routing activation, logical addressing, diagnostic messages

Linux socket API

TCP / UDP sockets

Linux networking stack

TCP, UDP, IP, routing, ARP and network interfaces

Ethernet driver and hardware

On Linux, you normally use the BSD socket API, such as `socket()`, `bind()`, `listen()`, `accept()`, `recv()` and `send()`. The kernel handles TCP/IP and communicates with the Ethernet driver.

In AUTOSAR Classic, by contrast, the corresponding functionality is typically split across modules such as `Eth`, `EthIf`, `TcpIp` and `SoAd`. You integrate with those modules instead of directly using Linux sockets.

## 2. What you still need to implement

| Component                           | Your responsibility                                                       |
| ----------------------------------- | ------------------------------------------------------------------------- |
| Ethernet driver                     | No, normally provided by Linux and the hardware driver                    |
| TCP/IP stack                        | No, provided by the Linux kernel                                          |
| TCP/UDP sockets                     | Use the Linux socket API                                                  |
| DoIP protocol                       | Yes — unless you reuse an existing implementation                         |
| UDS server / diagnostic application | Yes, if your goal includes processing UDS requests                        |
| Gateway routing to CAN              | Only if required; integrate with CAN transport and routing                |
| TLS                                 | Required if your target configuration mandates secured DoIP communication |

The DoIP layer must still implement protocol behavior such as message framing, vehicle discovery, routing activation, diagnostic message handling, connection management and appropriate error responses. See the [AUTOSAR DoIP specification, R24-11](https://www.autosar.org/fileadmin/standards/R24-11/CP/AUTOSAR_CP_SWS_DiagnosticOverIP.pdf?utm_source=chatgpt.com).&#x20;

[image](https://www.google.com/s2/favicons?domain=https://www.autosar.org\&sz=32)

autosar.org

+1



## 3. A minimal Linux implementation

For a learning project, I would start with this scope:

1. Create a UDP socket for vehicle identification and discovery messages.
2. Create a TCP server socket and listen on port `13400`.
3. Accept a tester connection.
4. Parse the 8-byte DoIP generic header and the declared payload length.
5. Handle routing activation before accepting diagnostic messages.
6. Receive DoIP diagnostic messages, extract the UDS payload, and pass it to a simple UDS handler.
7. Encode and send the diagnostic response.

One important implementation detail: TCP is a byte stream, not a packet-preserving protocol. A single `recv()` may return only part of a DoIP message, or bytes from multiple messages. Your parser must buffer data and process complete DoIP messages according to their header lengths.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://eclipse-opensovd.github.io\&sz=32)

CDA documentation

+1



My recommendation is to implement this first in ordinary C or C++ on Linux using sockets. Once you understand the protocol and state machine, you can map the same responsibilities onto AUTOSAR Classic's `SoAd`, `PduR` and `Dcm` interfaces.