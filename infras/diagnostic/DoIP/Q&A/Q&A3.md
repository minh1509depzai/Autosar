Question #3:
Tell me about routing activation and which API does the AUTOSAR spec use to perform this functionality

Answer #3:
Routing activation is a DoIP protocol procedure, not a TCP/IP API call. In AUTOSAR Classic, the DoIP module processes the request from the diagnostic tester, validates it against the configured routing-activation rules, and decides whether diagnostic communication can proceed over that TCP connection.

The important distinction is that the tester requests routing activation; the ECU's DoIP module handles it.

## 1. How routing activation works

1\. Diagnostic tester

Establishes a TCP connection to the ECU

2\. Sends Routing Activation Request

DoIP payload type `0x0005`

3\. ECU DoIP module

Checks tester logical address, activation type, configuration, and any configured authentication or confirmation callbacks

4\. Sends Routing Activation Response

Payload type `0x0006`, including a response code

5\. Diagnostic routing becomes available

If activation succeeds, diagnostic messages can be exchanged over the activated connection

The request is sent over TCP, not UDP. Routing activation is separate from establishing the TCP connection: a TCP connection alone does not mean the tester is authorized to exchange diagnostic messages. See the AUTOSAR Specification of Diagnostic over IP, section on routing activation.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://www.autosar.org\&sz=32)

autosar.org

+1



## 2. Which AUTOSAR APIs are involved?

There is no single standardized API named `DoIP_RoutingActivation()` that the tester calls to activate routing. The DoIP module handles the protocol message internally.

The relevant interfaces fall into three groups.

### A. SoAd APIs — socket communication

DoIP uses the AUTOSAR Socket Adaptor (`SoAd`) to receive and transmit data over TCP/UDP and to interact with configured socket connections.

Typical API families include:

- `SoAd_IfTransmit()` — transmit an I-PDU.
- `SoAd_TpTransmit()` — transmit a transport-protocol I-PDU.
- `SoAd_OpenSoCon()` / `SoAd_CloseSoCon()` — request opening or closing a socket connection.

The exact calls and sequence depend on the AUTOSAR release and implementation. These APIs provide socket communication; they do not themselves perform the routing-activation protocol.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://studylib.net\&sz=32)

studylib.net

+1



### B. DoIP routing-activation callbacks

The AUTOSAR DoIP specification defines configurable callbacks for applications that need to participate in routing activation.

| Configuration parameter                   | Purpose                                                                      |
| ----------------------------------------- | ---------------------------------------------------------------------------- |
| `DoIPRoutingActivationAuthenticationFunc` | Configures a direct C callback for authentication during routing activation. |
| `DoIPRoutingActivationConfirmationFunc`   | Configures a direct C callback for confirmation during routing activation.   |

These are configured under the corresponding `DoIPRoutingActivationAuthenticationCallback` and `DoIPRoutingActivationConfirmationCallback` containers.

If the callback containers are configured but the direct C callback functions are not specified, the specification describes using an R-Port with a generated service interface named `<RoutingActivation>_RoutingActivation`.

These callbacks are optional, configuration-dependent hooks. They are not replacements for the DoIP module's own routing-activation processing.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://www.autosar.org\&sz=32)

autosar.org

+1



### C. PduR and DCM — after activation

Once routing activation succeeds, the DoIP module can accept diagnostic messages on the activated connection.

For example, a tester sends UDS request `22 F1 90` to read a data identifier:

1. DoIP receives and parses the diagnostic message.
2. DoIP passes the diagnostic payload through the configured `PduR` routing path.
3. `DCM` processes the UDS request.
4. The response travels back through `PduR` and DoIP to the tester.

`PduR` and `DCM` are involved in diagnostic data processing; neither is the API that performs routing activation itself.

## 3. Where to look in the specification

The uploaded PDF in this project is `AUTOSAR_SWS_DefaultErrorTracer.pdf`, which does not specify DoIP routing activation. For this question, use the official [AUTOSAR Specification of Diagnostic over IP, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_DiagnosticOverIP.pdf?utm_source=chatgpt.com).

Focus on:

- Routing activation — request and response message formats and activation behavior.
- `DoIPRoutingActivationAuthenticationCallback` — authentication callback configuration.
- `DoIPRoutingActivationConfirmationCallback` — confirmation callback configuration.
- The DoIP–SoAd dependency and API definitions for socket communication.

The key architectural takeaway is: SoAd provides socket APIs; DoIP implements the routing-activation state machine and invokes optional application callbacks; PduR and DCM handle diagnostic requests after activation.