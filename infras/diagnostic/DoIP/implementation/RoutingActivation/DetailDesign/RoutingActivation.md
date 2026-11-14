# AUTOSAR DoIP Routing Activation — Detailed Implementation

I have inspected the uploaded `AUTOSAR_SWS_DiagnosticOverIP.pdf`. It specifies AUTOSAR Classic Platform R21-11, Document ID 418, and the relevant requirements are in Chapter 7.3.2.3, Routing activation, on printed pages 35–38. The configuration parameters and callback definitions are in Chapter 10.2.23–10.2.25, printed pages 119–124.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



I will use that specification as the primary source and develop the implementation around a Linux TCP server, where the operating system provides the TCP/IP stack and the DoIP application implements protocol handling.

One important distinction: the uploaded PDF specifies the AUTOSAR DoIP Basic Software module and its interfaces. A standalone Linux socket implementation can implement the same routing-activation behavior, but it is not automatically a fully AUTOSAR-conformant BSW module because it does not implement the required AUTOSAR integration, configuration, and module interfaces.

## 1. What routing activation must accomplish

Routing activation is the process by which a diagnostic tester obtains permission to send diagnostic messages through a DoIP entity.

A TCP connection alone does not grant this permission.

### 1. TCP connection established

A tester connects to the DoIP entity's TCP diagnostic port.

### 2. Receive Routing Activation Request

Payload type 0x0005 · Validate the DoIP header and payload length.

### 3. Validate tester and connection

Check source address, duplicate connections, connection capacity and activation type.

### 4. Authentication and confirmation

Execute configured callbacks when present; handle pending and rejected results.

### 5. Send Routing Activation Response

Payload type 0x0006 · Response code 0x10 means successful activation.

### 6. Enable diagnostic routing

Allow only configured, authorized source/target address combinations on this connection.

The implementation must follow the ordering and exceptional cases in requirements `SWS_DoIP_00101` through `SWS_DoIP_00114`, together with `SWS_DoIP_00160` and `SWS_DoIP_00274`. In particular, address validation, duplicate-connection handling, authentication and confirmation are separate checks; they should not be collapsed into a single `if (valid_tester)` statement.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



## 2. Wire protocol: exact message layouts

Before implementing the state machine, establish the binary message formats.

### 2.1 Generic DoIP header

Every Routing Activation message begins with the standard eight-byte DoIP header.

| Offset | Length  | Field                    |
| ------ | ------- | ------------------------ |
| 0      | 1 byte  | Protocol version         |
| 1      | 1 byte  | Inverse protocol version |
| 2      | 2 bytes | Payload type             |
| 4      | 4 bytes | Payload length           |

The payload type is encoded in network byte order (big-endian). The payload length is a 32-bit big-endian integer that excludes the eight-byte header.

For Routing Activation:

- Request payload type: `0x0005`
- Response payload type: `0x0006`

For the standard DoIP protocol version 2 header, the version and inverse-version bytes are `0x02` and `0xFD`. Validate them according to the protocol version your implementation supports.

### 2.2 Routing Activation Request — `0x0005`

According to Table 7.7, printed pages 35–36:

| Payload offset | Length  | Field                             |
| -------------- | ------- | --------------------------------- |
| 0              | 2 bytes | Tester source address             |
| 2              | 1 byte  | Activation type                   |
| 3              | 4 bytes | ISO-reserved bytes, expected zero |
| 7              | 4 bytes | Optional OEM-specific data        |

The accepted payload length is exactly 7 or 11 bytes, per `SWS_DoIP_00117`.

The complete message is therefore:

- 15 bytes for a seven-byte payload.
- 19 bytes for an eleven-byte payload.

Example request, assuming protocol version `0x02`, tester source address `0x0E00`, and activation type `0x00`:

```
02 FD 00 05 00 00 00 07
0E 00
00
00 00 00 00
```

The first eight bytes form the header. The remaining seven bytes are the request payload.

The example assumes `0x0E00` is configured as a valid tester address and activation type `0x00` is configured for that tester. Neither value should be accepted merely because it appears in this example.

### 2.3 Routing Activation Response — `0x0006`

According to Table 7.8, printed pages 38–39:

| Payload offset | Length  | Field                            |
| -------------- | ------- | -------------------------------- |
| 0              | 2 bytes | Tester source address            |
| 2              | 2 bytes | DoIP entity logical address      |
| 4              | 1 byte  | Routing activation response code |
| 5              | 4 bytes | ISO-reserved bytes, set to zero  |
| 9              | 4 bytes | Optional OEM-specific data       |

The base response payload is nine bytes. When the optional four-byte OEM-specific field is used, the payload is thirteen bytes.

Consequently, the corresponding complete messages are 17 and 21 bytes long.

The response must echo the tester source address from the request and contain the configured DoIP entity logical address. See `SWS_DoIP_00116`, `SWS_DoIP_00118`, `SWS_DoIP_00119` and `SWS_DoIP_00120`.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



## 3. Routing Activation response codes

The response code is the result of the validation and callback sequence. The values below correspond to the AUTOSAR R21-11 requirements on printed pages 36–39.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



| Code   | Meaning                                                          | Required action                                                                                                   |
| ------ | ---------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------- |
| `0x00` | Unknown source address                                           | Send response and close the requesting connection.                                                                |
| `0x01` | All tester connections are occupied                              | Send response and close the requesting connection when all existing connections answer the required alive checks. |
| `0x02` | A connection is already registered to a different source address | Send response and close the requesting connection.                                                                |
| `0x03` | The source address is already registered on another connection   | Send response and close the requesting connection when the existing connection answers its alive check.           |
| `0x04` | Routing activation denied by authentication                      | Send response; retain the connection registration without activating routing, as specified.                       |
| `0x05` | Routing activation denied by confirmation                        | Send response and close the requesting connection.                                                                |
| `0x06` | Unsupported activation type                                      | Send response and close the requesting connection.                                                                |
| `0x10` | Routing successfully activated                                   | Register the tester and enable routing.                                                                           |
| `0x11` | Confirmation callback is pending                                 | Send one response with this code; do not treat routing as successfully activated.                                 |

Two details deserve attention.

First, an authentication rejection (`0x04`) is not the same as a confirmation rejection (`0x05`). The R21-11 requirements specify different connection handling for the two cases.

Second, `0x11` is associated with a pending confirmation callback. It is not a successful activation response. Your implementation must not enable diagnostic routing just because the callback was invoked.

## 4. Architecture for a Linux implementation

I recommend separating the code into four components. This keeps the routing-activation state machine independent of TCP details and makes the implementation easier to test.

### TCP transport layer

Owns sockets, connection IDs, `recv()`, `send()`, partial writes, disconnects and TCP stream buffering.

### DoIP framing and dispatch

Reconstructs complete messages from the byte stream, validates the generic header and dispatches payload type `0x0005` to the activation handler.

### Routing Activation manager

Validates source addresses, handles duplicate connections and capacity, invokes callbacks and manages the routing-activation state machine.

### Diagnostic router

Checks whether routing is active for the connection and the source/target address combination before forwarding diagnostic messages to UDS processing.

In an AUTOSAR Classic implementation, the equivalent transport integration normally uses SoAd and the diagnostic routing path uses PduR, with DCM handling local diagnostic services. A Linux prototype can replace the SoAd socket interface with POSIX sockets, but should preserve the separation between transport and DoIP logic.

### 4.1 Data structures

Each accepted TCP connection needs its own context.

```
enum class ConnectionState {
    Connected,
    Registered,        // Source address registered; routing inactive
    Active,            // Routing activation succeeded
    WaitingAliveCheck,
    WaitingAuthentication,
    WaitingConfirmation,
    Closing,
    Closed
};

struct Connection {
    int socket_fd;
    uint64_t connection_id;

    uint16_t tester_address;
    bool tester_address_registered;
    bool routing_active;
    bool secure_transport;

    ConnectionState state;

    uint8_t activation_type;
    uint8_t request_oem[4];
    bool has_request_oem;
};
```

The important distinction is between `tester_address_registered` and `routing_active`.

A connection may be registered to a tester without having permission to route diagnostic messages. That distinction is necessary for the authentication-pending and authentication-rejection behavior in `SWS_DoIP_00110` and `SWS_DoIP_00111`.

The activation configuration should be separate from the connection:

```
struct RoutingActivationConfig {
    uint8_t activation_type;

    // Target logical addresses permitted by this activation.
    std::vector<uint16_t> target_addresses;

    bool security_required;

    bool authentication_enabled;
    uint8_t authentication_request_length;
    uint8_t authentication_response_length;

    bool confirmation_enabled;
    uint8_t confirmation_request_length;
    uint8_t confirmation_response_length;
};

struct TesterConfig {
    uint16_t source_address;
    std::vector<RoutingActivationConfig> activations;
};
```

At startup, load or compile the configured tester source addresses, activation types, permitted target addresses, security requirements, callback configuration and maximum number of tester connections.

These represent the same concepts as the AUTOSAR `DoIPTesterSA`, `DoIPRoutingActivationNumber`, `DoIPTargetAddressRef`, `DoIPRoutingActivationSecurityRequired`, and the authentication and confirmation callback containers described in Chapter 10.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



## 5. Implementation sequence

The following order is important. In particular, do not validate the activation type before checking the tester source address and existing connection registrations.

\#chatgpt-mermaid-\_r_5i1\_{font-family:-apple-system-body,ui-sans-serif,-apple-system,system-ui,"Segoe UI",Helvetica,"Apple Color Emoji",Arial,sans-serif,"Segoe UI Emoji","Segoe UI Symbol";font-size:16px;fill:rgb(237, 237, 237);}@keyframes edge-animation-frame{from{stroke-dashoffset:0;}}@keyframes dash{to{stroke-dashoffset:0;}}#chatgpt-mermaid-\_r_5i1\_ .edge-animation-slow{stroke-dasharray:9,5!important;stroke-dashoffset:900;animation:dash 50s linear infinite;stroke-linecap:round;}#chatgpt-mermaid-\_r_5i1\_ .edge-animation-fast{stroke-dasharray:9,5!important;stroke-dashoffset:900;animation:dash 20s linear infinite;stroke-linecap:round;}#chatgpt-mermaid-\_r_5i1\_ .error-icon{fill:rgb(48, 48, 48);}#chatgpt-mermaid-\_r_5i1\_ .error-text{fill:rgb(237, 237, 237);stroke:rgb(237, 237, 237);}#chatgpt-mermaid-\_r_5i1\_ .edge-thickness-normal{stroke-width:1px;}#chatgpt-mermaid-\_r_5i1\_ .edge-thickness-thick{stroke-width:3.5px;}#chatgpt-mermaid-\_r_5i1\_ .edge-pattern-solid{stroke-dasharray:0;}#chatgpt-mermaid-\_r_5i1\_ .edge-thickness-invisible{stroke-width:0;fill:none;}#chatgpt-mermaid-\_r_5i1\_ .edge-pattern-dashed{stroke-dasharray:3;}#chatgpt-mermaid-\_r_5i1\_ .edge-pattern-dotted{stroke-dasharray:2;}#chatgpt-mermaid-\_r_5i1\_ .marker{fill:rgb(175, 175, 175);stroke:rgb(175, 175, 175);}#chatgpt-mermaid-\_r_5i1\_ .marker.cross{stroke:rgb(175, 175, 175);}#chatgpt-mermaid-\_r_5i1\_ svg{font-family:-apple-system-body,ui-sans-serif,-apple-system,system-ui,"Segoe UI",Helvetica,"Apple Color Emoji",Arial,sans-serif,"Segoe UI Emoji","Segoe UI Symbol";font-size:16px;}#chatgpt-mermaid-\_r_5i1\_ p{margin:0;}#chatgpt-mermaid-\_r_5i1\_ .label{font-family:-apple-system-body,ui-sans-serif,-apple-system,system-ui,"Segoe UI",Helvetica,"Apple Color Emoji",Arial,sans-serif,"Segoe UI Emoji","Segoe UI Symbol";color:rgb(237, 237, 237);}#chatgpt-mermaid-\_r_5i1\_ .cluster-label text{fill:rgb(237, 237, 237);}#chatgpt-mermaid-\_r_5i1\_ .cluster-label span{color:rgb(237, 237, 237);}#chatgpt-mermaid-\_r_5i1\_ .cluster-label span p{background-color:transparent;}#chatgpt-mermaid-\_r_5i1\_ .label text,#chatgpt-mermaid-\_r_5i1\_ span{fill:rgb(237, 237, 237);color:rgb(237, 237, 237);}#chatgpt-mermaid-\_r_5i1\_ .node rect,#chatgpt-mermaid-\_r_5i1\_ .node circle,#chatgpt-mermaid-\_r_5i1\_ .node ellipse,#chatgpt-mermaid-\_r_5i1\_ .node polygon,#chatgpt-mermaid-\_r_5i1\_ .node path{fill:rgb(9, 23, 44);stroke:rgb(31, 78, 148);stroke-width:1px;}#chatgpt-mermaid-\_r_5i1\_ .rough-node .label text,#chatgpt-mermaid-\_r_5i1\_ .node .label text,#chatgpt-mermaid-\_r_5i1\_ .image-shape .label,#chatgpt-mermaid-\_r_5i1\_ .icon-shape .label{text-anchor:middle;}#chatgpt-mermaid-\_r_5i1\_ .node .katex path{fill:#000;stroke:#000;stroke-width:1px;}#chatgpt-mermaid-\_r_5i1\_ .rough-node .label,#chatgpt-mermaid-\_r_5i1\_ .node .label,#chatgpt-mermaid-\_r_5i1\_ .image-shape .label,#chatgpt-mermaid-\_r_5i1\_ .icon-shape .label{text-align:center;}#chatgpt-mermaid-\_r_5i1\_ .node.clickable{cursor:pointer;}#chatgpt-mermaid-\_r_5i1\_ .root .anchor path{fill:rgb(175, 175, 175)!important;stroke-width:0;stroke:rgb(175, 175, 175);}#chatgpt-mermaid-\_r_5i1\_ .arrowheadPath{fill:rgb(175, 175, 175);}#chatgpt-mermaid-\_r_5i1\_ .edgePath .path{stroke:rgb(175, 175, 175);stroke-width:1px;}#chatgpt-mermaid-\_r_5i1\_ .flowchart-link{stroke:rgb(175, 175, 175);fill:none;}#chatgpt-mermaid-\_r_5i1\_ .edgeLabel{background-color:rgb(0, 0, 0);text-align:center;}#chatgpt-mermaid-\_r_5i1\_ .edgeLabel p{background-color:rgb(0, 0, 0);}#chatgpt-mermaid-\_r_5i1\_ .edgeLabel rect{opacity:0.5;background-color:rgb(0, 0, 0);fill:rgb(0, 0, 0);}#chatgpt-mermaid-\_r_5i1\_ .labelBkg{background-color:rgba(0, 0, 0, 0.5);}#chatgpt-mermaid-\_r_5i1\_ .cluster rect{fill:rgb(48, 48, 48);stroke:rgba(255, 255, 255, 0.15);stroke-width:1px;}#chatgpt-mermaid-\_r_5i1\_ .cluster text{fill:rgb(237, 237, 237);}#chatgpt-mermaid-\_r_5i1\_ .cluster span{color:rgb(237, 237, 237);}#chatgpt-mermaid-\_r_5i1\_ div.mermaidTooltip{position:absolute;text-align:center;max-width:200px;padding:2px;font-family:-apple-system-body,ui-sans-serif,-apple-system,system-ui,"Segoe UI",Helvetica,"Apple Color Emoji",Arial,sans-serif,"Segoe UI Emoji","Segoe UI Symbol";font-size:12px;background:rgb(48, 48, 48);border:1px solid rgba(255, 255, 255, 0.15);border-radius:2px;pointer-events:none;z-index:100;}#chatgpt-mermaid-\_r_5i1\_ .flowchartTitleText{text-anchor:middle;font-size:18px;fill:rgb(237, 237, 237);}#chatgpt-mermaid-\_r_5i1\_ rect.text{fill:none;stroke-width:0;}#chatgpt-mermaid-\_r_5i1\_ .icon-shape,#chatgpt-mermaid-\_r_5i1\_ .image-shape{background-color:rgb(0, 0, 0);text-align:center;}#chatgpt-mermaid-\_r_5i1\_ .icon-shape p,#chatgpt-mermaid-\_r_5i1\_ .image-shape p{background-color:rgb(0, 0, 0);padding:2px;}#chatgpt-mermaid-\_r_5i1\_ .icon-shape .label rect,#chatgpt-mermaid-\_r_5i1\_ .image-shape .label rect{opacity:0.5;background-color:rgb(0, 0, 0);fill:rgb(0, 0, 0);}#chatgpt-mermaid-\_r_5i1\_ .label-icon{display:inline-block;height:1em;overflow:visible;vertical-align:-0.125em;}#chatgpt-mermaid-\_r_5i1\_ .node .label-icon path{fill:currentColor;stroke:revert;stroke-width:revert;}#chatgpt-mermaid-\_r_5i1\_ .node .neo-node{stroke:rgb(31, 78, 148);}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].node rect,#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].cluster rect,#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].node polygon{stroke:url(#chatgpt-mermaid-\_r_5i1\_-gradient);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].swimlane.cluster rect{filter:none;}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].node path{stroke:url(#chatgpt-mermaid-\_r_5i1\_-gradient);stroke-width:1px;}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].node .outer-path{filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].node .neo-line path{stroke:rgb(31, 78, 148);filter:none;}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].node circle{stroke:url(#chatgpt-mermaid-\_r_5i1\_-gradient);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].node circle .state-start{fill:#000000;}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].icon-shape .icon{fill:url(#chatgpt-mermaid-\_r_5i1\_-gradient);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-\_r_5i1\_ [data-look="neo"].icon-shape .icon-neo path{stroke:url(#chatgpt-mermaid-\_r_5i1\_-gradient);filter:drop-shadow( 1px 2px 2px rgba(185,185,185,1));}#chatgpt-mermaid-\_r_5i1\_ .node text{font-size:14px;font-weight:600;letter-spacing:normal;fill:rgb(153, 206, 255);}#chatgpt-mermaid-\_r_5i1\_ .edgeLabels text{font-size:13px;font-weight:600;letter-spacing:-0.08px;fill:rgb(153, 206, 255);}#chatgpt-mermaid-\_r_5i1\_ .node tspan[font-weight="normal"],#chatgpt-mermaid-\_r_5i1\_ .edgeLabels tspan[font-weight="normal"]{font-weight:600;}#chatgpt-mermaid-\_r_5i1\_ .edgeLabel .label rect{opacity:1;rx:13px;ry:13px;fill:rgb(0, 14, 26);stroke:rgb(26, 62, 95);stroke-width:1px;}#chatgpt-mermaid-\_r_5i1\_ .node rect,#chatgpt-mermaid-\_r_5i1\_ .node circle,#chatgpt-mermaid-\_r_5i1\_ .node ellipse,#chatgpt-mermaid-\_r_5i1\_ .node polygon,#chatgpt-mermaid-\_r_5i1\_ .node path{fill:rgb(0, 40, 77);stroke:rgba(255, 255, 255, 0.1);stroke-width:1px;}#chatgpt-mermaid-\_r_5i1\_ .node rect{rx:16px;ry:16px;}#chatgpt-mermaid-\_r_5i1\_ .node.mermaid-decision .label-container{fill:rgb(0, 14, 26);stroke:rgb(26, 62, 95);stroke-dasharray:2,2;}#chatgpt-mermaid-\_r_5i1\_ .edgePaths .flowchart-link{stroke:rgb(175, 175, 175);stroke-width:1px;stroke-linecap:round;stroke-linejoin:round;}#chatgpt-mermaid-\_r_5i1\_ .marker{fill:rgb(175, 175, 175);stroke:rgb(175, 175, 175);}#chatgpt-mermaid-\_r_5i1\_ :root{--mermaid-font-family:-apple-system-body,ui-sans-serif,-apple-system,system-ui,"Segoe UI",Helvetica,"Apple Color Emoji",Arial,sans-serif,"Segoe UI Emoji","Segoe UI Symbol";}Complete TCP DoIP messagereceivedValid header, TCP connectionand payload length?Apply relevant framing/errorhandlingSource address configured?Response 0x00, close newconnectionConnection registered toanother source?Response 0x02, close newconnectionSame tester address onanother connection?Alive-check existingconnectionResponse 0x03, close newconnectionRemove stale registration andcontinueAt maximum connectioncapacity?Activation type configured fortester?Alive-check registeredconnectionsResponse 0x01, close newconnectionResponse 0x06, close newconnectionAuthentication configured?Invoke authentication callbackRegister tester, routinginactive; retry callbackResponse 0x04, registerwithout routingConfirmation configured?Invoke confirmation callbackResponse 0x11 onceResponse 0x05, closeconnectionResponse 0x10; register andenable routingNoYesNoYesYesNoYesRespondsTimes outNoNoYesAll respondOne or more time outNoYesYesPendingRejectedAcceptedNoYesPendingRejectedAcceptedNo

This diagram expresses the normal processing sequence. The connection-capacity and alive-check steps must be implemented asynchronously in a real server, rather than blocking the entire TCP event loop while waiting for responses.

### Step 1 — Validate the received message

The message dispatcher should pass the routing-activation handler only a complete, validated DoIP message.

For a request with payload type `0x0005`, verify:

1. It was received on a configured TCP diagnostic connection, not UDP (`SWS_DoIP_00101`).
2. The generic DoIP header is valid.
3. The payload length is exactly seven or eleven bytes (`SWS_DoIP_00117`).
4. The complete declared payload has been received.
5. The reserved bytes are handled according to the protocol requirements.
6. The source address, activation type and optional OEM-specific bytes are extracted using the specified offsets.

TCP is a byte stream. A single `recv()` call is not guaranteed to return the complete message, and one call may contain multiple messages. Your framing layer must therefore accumulate bytes until it has a complete header and the full payload.

Do not use the size of a single `recv()` result as the message length.

### Step 2 — Validate the source address

Read the two-byte tester source address in big-endian order and compare it with the configured `DoIPTesterSA` entries.

- If the address is unknown, send response code `0x00` and close the requesting connection.
- If it is valid, continue to the connection-registration checks.

This is specified by `SWS_DoIP_00102` and `SWS_DoIP_00104`.

### Step 3 — Check existing registrations

There are two distinct conditions.

Condition A: The incoming connection is already registered to a different source address.

Send response code `0x02` on that connection and close it, as required by `SWS_DoIP_00106`.

Condition B: The requested tester source address is already registered on another connection.

Send an Alive Check Request (`0x0007`) to the existing connection and wait for a valid response or the configured `DoIPAliveCheckResponseTimeout`.

- If the existing connection answers, respond to the new request with `0x03` and close the new connection.
- If the existing connection times out, remove the stale registration and continue processing the new connection.

The second condition is governed by `SWS_DoIP_00103` and `SWS_DoIP_00105`.

Do not use only an IP address or TCP port as the identity of a tester. The logical tester source address is the registration key; the socket connection is the resource to which that registration belongs.

### Step 4 — Enforce the maximum number of tester connections

Compare the number of registered tester connections with `DoIPMaxTesterConnections`.

If capacity remains, continue.

If the limit has been reached, send Alive Check Requests to the existing registered connections and wait for the configured timeout:

- If every existing connection responds validly, reject the new request with `0x01` and close the new connection.
- If at least one existing connection times out, clean up the stale connection or connections and continue processing the new request, subject to the configured capacity.

This is `SWS_DoIP_00107`. Be careful to distinguish the new requesting connection from stale existing connections: closing the wrong socket here can accidentally disconnect a legitimate tester.

### Step 5 — Validate the activation type

For the configured tester, search its associated routing-activation configurations for an entry whose `DoIPRoutingActivationNumber` matches the request's activation-type byte.

If no matching entry exists, send `0x06` and close the connection, as required by `SWS_DoIP_00108` and `SWS_DoIP_00160`.

If it matches, use that configuration to determine the allowed diagnostic target addresses, security requirements and configured callbacks.

The activation type is not itself an authentication credential. It selects a configured activation profile.

## 6. Authentication and confirmation callbacks

This is the part of the implementation that is easiest to get wrong if you implement only the common success path.

AUTOSAR defines two independent optional callbacks:

- `DoIPRoutingActivationAuthenticationCallback`
- `DoIPRoutingActivationConfirmationCallback`

They are configured per routing-activation profile. Their configuration and data-length parameters are described in printed pages 121–124.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



### 6.1 Authentication callback

If authentication is configured, pass the configured number of bytes from the beginning of the request's OEM-specific data to the authentication function.

The parameter `DoIPRoutingActivationAuthenticationReqLength` is in the range 0–4. The corresponding response length, `DoIPRoutingActivationAuthenticationResLength`, is also in the range 0–4.

The callback outcomes are:

| Callback result                      | Required behavior                                                                                                                                                          |
| ------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `E_OK`                               | Authentication succeeds; continue to confirmation if configured, otherwise activate routing.                                                                               |
| `DOIP_E_PENDING`                     | Register the connection to the tester without activating routing, then invoke the callback again at the next `DoIP_MainFunction` call until it returns a different result. |
| Any other result, such as `E_NOT_OK` | Send response `0x04`; retain tester registration without enabling routing.                                                                                                 |

These behaviors are defined in `SWS_DoIP_00109`, `SWS_DoIP_00161`, `SWS_DoIP_00110` and `SWS_DoIP_00111`.

One implementation detail matters: do not repeatedly send routing-activation responses while the authentication callback remains pending. The requirement says to repeat the callback, not to transmit a response on every main-function cycle.

When authentication succeeds and its response length is nonzero, attach the configured response bytes at the beginning of the response's OEM-specific field.

### 6.2 Confirmation callback

If confirmation is configured, call it after authentication has succeeded, or directly when authentication is not configured.

The confirmation callback has its own request and response lengths, each ranging from 0–4 bytes.

| Callback result  | Required behavior                                                      |
| ---------------- | ---------------------------------------------------------------------- |
| `E_OK`           | Confirmation succeeds; activate routing if no other rejection applies. |
| `DOIP_E_PENDING` | Send one routing-activation response with code `0x11`.                 |
| `E_NOT_OK`       | Send response `0x05` and close the connection.                         |

The requirements are `SWS_DoIP_00112`, `SWS_DoIP_00114` and `SWS_DoIP_00274`.

Unlike authentication pending, confirmation pending is explicitly associated with sending a response. Do not treat it as permission to activate routing.

### 6.3 Callback state machine

A useful implementation model is:

```
enum class ActivationPhase {
    None,
    AuthenticationPending,
    ConfirmationPending,
    Complete,
    Rejected
};

enum class CallbackResult {
    Ok,
    Pending,
    NotOk
};
```

Store the activation phase, the selected activation configuration, and the callback input data in the connection context. The main-function processing should resume only connections whose callback is pending.

```
void process_pending_authentication(Connection& conn)
{
    if (conn.activation_phase !=
        ActivationPhase::AuthenticationPending) {
        return;
    }

    CallbackResult result = run_authentication_callback(conn);

    if (result == CallbackResult::Pending) {
        // Retry at the next DoIP_MainFunction cycle.
        // Keep diagnostic routing disabled.
        return;
    }

    if (result == CallbackResult::NotOk) {
        send_routing_activation_response(conn, 0x04);
        conn.routing_active = false;
        conn.activation_phase = ActivationPhase::Rejected;

        // Preserve tester registration as required by R21-11.
        return;
    }

    // Authentication succeeded. Continue to confirmation
    // if configured, otherwise finish activation.
    process_confirmation_or_activate(conn);
}
```

This is an illustrative core, not a complete drop-in implementation: `run_authentication_callback()`, response transmission, and the state transitions must be implemented against your actual callback interface and configuration.

For AUTOSAR compatibility, distinguish the AUTOSAR result values (`E_OK`, `E_NOT_OK`, `DOIP_E_PENDING`) from any internal result enum you use in a Linux prototype.

## 7. Response construction

Centralize response construction so every branch uses the same byte order, reserved fields and address handling.

```
#include <array>
#include <cstdint>
#include <vector>

static void write_u16_be(uint8_t* dst, uint16_t value)
{
    dst[0] = static_cast<uint8_t>(value >> 8);
    dst[1] = static_cast<uint8_t>(value);
}

static void write_u32_be(uint8_t* dst, uint32_t value)
{
    dst[0] = static_cast<uint8_t>(value >> 24);
    dst[1] = static_cast<uint8_t>(value >> 16);
    dst[2] = static_cast<uint8_t>(value >> 8);
    dst[3] = static_cast<uint8_t>(value);
}

std::vector<uint8_t> build_routing_activation_response(
    uint8_t protocol_version,
    uint16_t tester_address,
    uint16_t entity_address,
    uint8_t response_code,
    const std::vector<uint8_t>& oem_data)
{
    // This helper supports the base response and the
    // optional four-byte OEM-specific response field.
    if (!oem_data.empty() && oem_data.size() != 4) {
        return {};
    }

    const uint32_t payload_length =
        static_cast<uint32_t>(9 + oem_data.size());

    std::vector<uint8_t> message(8 + payload_length, 0);

    // Generic DoIP header.
    message[0] = protocol_version;
    message[1] = static_cast<uint8_t>(~protocol_version);

    write_u16_be(&message[2], 0x0006);
    write_u32_be(&message[4], payload_length);

    // Routing Activation Response payload.
    write_u16_be(&message[8], tester_address);
    write_u16_be(&message[10], entity_address);
    message[12] = response_code;

    // Bytes 13..16 are zero-initialized reserved bytes.

    if (!oem_data.empty()) {
        for (size_t i = 0; i < oem_data.size(); ++i) {
            message[17 + i] = oem_data[i];
        }
    }

    return message;
}
```

This helper uses the generic header layout and response payload defined in Tables 7.1 and 7.8. It deliberately leaves reserved bytes zero. The optional OEM-specific response data must be prepared by the configured callback logic; the helper must not invent authentication or confirmation data.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



Important implementation constraints:

- The caller must supply a validated protocol version.
- The response must be sent on the same TCP connection that received the request.
- Use a `send_all()`-style routine or a queued nonblocking write mechanism. A single POSIX `send()` call is not guaranteed to transmit every byte.
- Do not close a socket before the response has been queued or transmitted as appropriate for your transport design.
- Ensure that the TCP connection's state prevents diagnostic routing until the successful activation transition has occurred.

## 8. Final activation and diagnostic routing

The successful activation path is defined by `SWS_DoIP_00113`.

After all required checks have succeeded:

1. Construct a response with code `0x10`.
2. Include the request's tester source address.
3. Include the configured DoIP entity logical address.
4. Include OEM-specific response data where configured.
5. Register the connection to the tester.
6. Enable routing for the configured `DoIPTargetAddressRef` entries.
7. Mark the connection as active.

The connection should store the permitted target addresses, rather than just a Boolean `routing_active` flag.

For example, suppose the configuration associates tester `0x0E00` with activation type `0x00` and targets `0x1001` and `0x1002`.

After successful activation, the connection may route diagnostic messages for those configured targets. A diagnostic message for `0x1003` must not be accepted merely because the connection has activated routing.

The diagnostic dispatch layer should enforce the source/target combination before forwarding a `0x8001` Diagnostic Message to the appropriate PduR/UDS path. AUTOSAR's diagnostic-message reception requirements explicitly connect routing activation to the configured source/target combinations.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



A useful authorization check is:

```
bool may_route_diagnostic(
    const Connection& conn,
    uint16_t source_address,
    uint16_t target_address)
{
    if (!conn.routing_active) {
        return false;
    }

    if (!conn.tester_address_registered) {
        return false;
    }

    if (source_address != conn.tester_address) {
        return false;
    }

    return configured_target_allowed(conn, target_address);
}
```

`configured_target_allowed()` should consult the targets granted by the selected activation profile. It should not simply check whether the target address is globally known.

## 9. Connection lifecycle and concurrency

The implementation should use a single connection manager as the authoritative owner of tester registrations.

For a small Linux prototype, a `poll()` or `epoll()` event loop is sufficient. Each connection needs:

- Its socket and a unique connection identifier.
- TCP receive and transmit buffers.
- Its tester source address, if registered.
- Its routing-activation state and permitted target addresses.
- The selected activation configuration.
- Any pending callback state.
- Any outstanding alive-check state and deadline.

Do not hold a global mutex while waiting for an alive-check response. Instead, record which existing connections are being checked, send the Alive Check Requests, and continue processing other connections. When the response arrives or the deadline expires, resume the pending routing-activation operation.

A practical connection manager interface might look like this:

```
class RoutingActivationManager {
public:
    void on_activation_request(
        uint64_t connection_id,
        const uint8_t* payload,
        size_t payload_length);

    void on_alive_check_response(
        uint64_t connection_id,
        uint16_t tester_address);

    void on_connection_closed(uint64_t connection_id);

    void main_function();

private:
    void validate_and_start_activation(uint64_t connection_id);
    void start_duplicate_address_check(uint64_t connection_id);
    void start_capacity_check(uint64_t connection_id);
    void process_pending_authentication(uint64_t connection_id);
    void process_confirmation(uint64_t connection_id);
    void activate_routing(uint64_t connection_id);
};
```

The methods above describe the intended design, not a complete class implementation. Each asynchronous operation should have an explicit state and timeout handling.

A few lifecycle rules are essential:

1. One source address must not silently become registered to two live connections. Check registration atomically when committing a new registration.
2. A disconnected tester must not leave a stale registration behind. Remove its registration and routing permissions on connection teardown.
3. Pending authentication must not authorize diagnostics. Its retry state must survive until the callback completes or the connection is terminated.
4. A connection must not retain permissions from a previous activation. If a connection is closed or reset, remove its active routing state.
5. Alive-check responses must be matched to the correct connection and registered source address. A response from the wrong source address is not a valid confirmation of the connection's liveness.

The PDF specifies the alive-check timeout behavior in `SWS_DoIP_00105`, `SWS_DoIP_00107`, `SWS_DoIP_00139` and `SWS_DoIP_00140`.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



## 10. Configuration requirements

Before implementing the state machine, define a small configuration and validate it at startup.

| AUTOSAR parameter                              | Purpose                                          | Implementation requirement                                                           |
| ---------------------------------------------- | ------------------------------------------------ | ------------------------------------------------------------------------------------ |
| `DoIPLogicalAddress`                           | Logical address of the DoIP entity               | Use it in every Routing Activation Response.                                         |
| `DoIPTesterSA`                                 | Configured tester source address                 | Reject unknown source addresses.                                                     |
| `DoIPRoutingActivationNumber`                  | Supported activation type                        | Match it against the selected tester's activation configurations.                    |
| `DoIPTargetAddressRef`                         | Targets permitted for an activation              | Store the allowed target set after successful activation.                            |
| `DoIPMaxTesterConnections`                     | Maximum concurrent tester connections            | Enforce capacity and perform the required alive checks.                              |
| `DoIPAliveCheckResponseTimeout`                | Deadline for an alive-check response             | Use it for duplicate-address and capacity checks.                                    |
| `DoIPRoutingActivationSecurityRequired`        | Whether secure TCP is required for an activation | Verify the actual connection's security status before granting routing.              |
| `DoIPRoutingActivationAuthenticationReqLength` | Authentication input length                      | Validate it is within 0–4 bytes and pass the configured number of request OEM bytes. |
| `DoIPRoutingActivationAuthenticationResLength` | Authentication output length                     | Validate it is within 0–4 bytes and construct the response OEM field correctly.      |
| `DoIPRoutingActivationConfirmationReqLength`   | Confirmation input length                        | Validate it is within 0–4 bytes and pass the configured number of request OEM bytes. |
| `DoIPRoutingActivationConfirmationResLength`   | Confirmation output length                       | Validate it is within 0–4 bytes and construct the response OEM field correctly.      |

The parameter definitions are in Chapter 10, especially sections 10.2.23–10.2.26.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



Two further configuration checks are advisable:

- If both callback response fields are enabled, verify that their combined lengths fit in the four-byte OEM-specific response field. Authentication response bytes occupy the beginning of the field; confirmation response bytes occupy the end.
- If `DoIPRoutingActivationSecurityRequired` is true, a plain TCP connection must not be treated as secure merely because it uses the expected diagnostic port. The prototype needs an actual secure-transport mechanism and a trustworthy indication that the connection meets the configured requirement.

The PDF's parameter definitions establish that secure transport may be required, but this routing-activation implementation alone does not implement TLS or a complete secure-connection management mechanism.

## 11. Verification and test plan

Do not consider routing activation complete after testing only the successful request. The error paths and connection lifecycle are part of the implementation.

| Test  | Input or condition                                                             | Expected behavior                                                                                        |
| ----- | ------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------- |
| RA-01 | Valid tester, supported activation type, no callbacks                          | Response `0x10`; routing becomes active.                                                                 |
| RA-02 | Unknown tester source address                                                  | Response `0x00`; requesting connection closes.                                                           |
| RA-03 | Same connection already registered to a different source address               | Response `0x02`; requesting connection closes.                                                           |
| RA-04 | Same tester address on another connection, existing tester answers alive check | Response `0x03`; new connection closes.                                                                  |
| RA-05 | Duplicate tester address, existing connection times out                        | Remove stale registration and continue activation.                                                       |
| RA-06 | Connection limit reached; every existing connection answers                    | Response `0x01`; new connection closes.                                                                  |
| RA-07 | Connection limit reached; at least one existing connection times out           | Clean up stale connection(s), then continue activation.                                                  |
| RA-08 | Activation type not configured for the tester                                  | Response `0x06`; connection closes.                                                                      |
| RA-09 | Authentication callback returns `E_NOT_OK`                                     | Response `0x04`; registration retained without routing.                                                  |
| RA-10 | Authentication callback returns `DOIP_E_PENDING`                               | No immediate activation; callback retried by `DoIP_MainFunction`.                                        |
| RA-11 | Confirmation callback returns `E_NOT_OK`                                       | Response `0x05`; connection closes.                                                                      |
| RA-12 | Confirmation callback returns `DOIP_E_PENDING`                                 | One response with code `0x11`; routing remains inactive.                                                 |
| RA-13 | Wrong request payload length, such as 8 or 12 bytes                            | Reject according to the invalid-message handling policy; never process it as a valid activation request. |
| RA-14 | Activation request arrives over UDP                                            | Discard the message.                                                                                     |
| RA-15 | Diagnostic message arrives before successful activation                        | Do not route the diagnostic message.                                                                     |
| RA-16 | Diagnostic target is not permitted by the selected activation                  | Do not forward the message.                                                                              |
| RA-17 | TCP request arrives in multiple fragments                                      | Reassemble and process exactly once.                                                                     |
| RA-18 | Multiple complete DoIP messages arrive in one TCP read                         | Process each complete message separately.                                                                |
| RA-19 | Authentication and confirmation response data are both configured              | Verify OEM-specific data placement, field lengths and payload length.                                    |
| RA-20 | TCP connection closes after activation                                         | Remove its registration and all routing permissions.                                                     |

### Example integration test

For the basic success case, assume:

- DoIP entity logical address: `0x1000`
- Configured tester source address: `0x0E00`
- Configured activation type: `0x00`
- No authentication or confirmation callbacks
- Available tester-connection capacity

Send the request:

```
02 FD 00 05 00 00 00 07
0E 00 00 00 00 00 00
```

Expected response:

```
02 FD 00 06 00 00 00 09
0E 00 10 00 10 00 00 00 00
```

Interpretation:

- `02 FD`: protocol version and inverse.
- `00 06`: Routing Activation Response.
- `00 00 00 09`: nine-byte payload.
- `0E 00`: tester source address.
- `10 00`: DoIP entity logical address.
- `10`: successful routing activation.
- `00 00 00 00`: reserved bytes.

Then send a diagnostic request with payload type `0x8001` and a target address permitted by the activation configuration. Verify that the routing layer accepts it and forwards the diagnostic data to the configured destination.

Repeat with an unconfigured target address and with routing activation omitted. Neither should be forwarded.

## 12. Requirement traceability

The following is a focused traceability map for the implementation described above. It is not an exhaustive extraction of the entire 126-page specification.

| Requirement ID                    | Description                                                               | Conditions                                   | Expected behavior                                                             | Source            |
| --------------------------------- | ------------------------------------------------------------------------- | -------------------------------------------- | ----------------------------------------------------------------------------- | ----------------- |
| `SWS_DoIP_00101`                  | Routing activation request is accepted only on configured TCP connections | Payload type `0x0005`                        | Discard when received on an inappropriate connection                          | Printed p. 36     |
| `SWS_DoIP_00117`                  | Request payload length validation                                         | Payload type `0x0005`                        | Accept exactly 7 or 11 bytes                                                  | Printed p. 36     |
| `SWS_DoIP_00102`                  | Validate configured tester source address                                 | Valid DoIP header                            | Continue processing only for a configured tester                              | Printed p. 36     |
| `SWS_DoIP_00104`                  | Unknown tester address handling                                           | Source address not configured                | Response `0x00`, close connection                                             | Printed p. 36     |
| `SWS_DoIP_00106`                  | Conflicting registration on the same connection                           | Connection registered to another source      | Response `0x02`, close connection                                             | Printed p. 36     |
| `SWS_DoIP_00105`                  | Duplicate source-address handling                                         | Source address belongs to another connection | Alive check; response `0x03` if it responds; otherwise continue after timeout | Printed pp. 36–37 |
| `SWS_DoIP_00107`                  | Tester connection capacity                                                | Maximum reached                              | Alive-check registered connections; response `0x01` if all respond            | Printed p. 37     |
| `SWS_DoIP_00108`                  | Activation type validation                                                | Configured tester activation profiles        | Continue on a match                                                           | Printed p. 37     |
| `SWS_DoIP_00160`                  | Unsupported activation type                                               | No matching activation profile               | Response `0x06`, close connection                                             | Printed p. 37     |
| `SWS_DoIP_00109`–`00111`          | Authentication processing                                                 | Authentication callback configured           | Handle success, pending and rejection distinctly                              | Printed pp. 37–38 |
| `SWS_DoIP_00112`, `00114`         | Confirmation processing                                                   | Confirmation callback configured             | Handle success, pending and rejection distinctly                              | Printed p. 38     |
| `SWS_DoIP_00274`                  | Confirmation rejection                                                    | Callback returns `E_NOT_OK`                  | Response `0x05`, close connection                                             | Printed p. 38     |
| `SWS_DoIP_00113`                  | Successful activation                                                     | All applicable checks succeed                | Response `0x10`, register tester and enable routing                           | Printed p. 38     |
| `SWS_DoIP_00116`, `00118`–`00120` | Response payload construction                                             | Routing activation response                  | Correct addresses, response code and optional OEM data                        | Printed pp. 38–39 |

Source: `AUTOSAR_SWS_DiagnosticOverIP.pdf`, AUTOSAR Classic Platform R21-11.&#x20;

AUTOSAR_SWS_DiagnosticOverIP.pdf



## 13. Implementation scope and remaining compliance work

`RoutingActivation.hpp` and `RoutingActivation.cpp` implement a transport-independent routing-activation manager. The manager validates complete Routing Activation Request messages, constructs responses, tracks tester registrations, performs asynchronous alive checks, manages authentication and confirmation callback results, and authorizes diagnostic source/target pairs. The transport and callback interfaces are abstract so the same state machine can be integrated with a platform-specific DoIP stack.

This component is not a complete DoIP server or a claim of AUTOSAR conformance. The following integration and verification work remains:

- Provide TCP stream framing and dispatch complete DoIP messages to the manager. The transport must queue full responses, handle connection closure, and dispatch Alive Check Responses.
- Implement concrete authentication and confirmation callbacks and supply validated configuration.
- When an activation requires secure transport, provide a real secure transport and a trustworthy security indication; this manager does not implement TLS.
- Integrate the authorized target set with the diagnostic routing path, such as PduR/UDS in an AUTOSAR Classic system.
- Review requirements outside the routing-activation scope, including connection establishment and diagnostic-message processing, and execute the test matrix against the integrated system.
- For AUTOSAR Classic BSW conformance, map this manager to the required SoAd, PduR and other module interfaces.

Keep TCP framing tests separate from routing-activation state-machine tests so fragmentation and message-dispatch defects do not obscure failures in the activation flow.