/**
 * @file    Configuration.hpp
 * @brief   Pre-compile configuration of the DoIP module.
 *
 * Configuration examples for the parameter definitions extracted in
 * infras/diagnostic/DoIP/Configuration/RequirementExtraction/DoIP_Configurations.arxml.
 * Parameter identity is the combination of parent container and parameter
 * short name; ECUC item IDs alone are not unique in the extraction.
 *
 * Conventions used in this file
 *  - Example values are not AUTOSAR defaults or a production ECUC instance.
 *  - Optional references and callbacks without an established representation
 *    are documented as unresolved rather than assigned invented symbols.
 *  - ECUC_DoIP_00095 occurs under three different connection parent containers;
 *    their configuration macros are parent-qualified below.
 *  - Boolean parameters use STD_ON / STD_OFF style values 1 / 0.
 */

#ifndef DOIP_CONFIGURATION_HPP
#define DOIP_CONFIGURATION_HPP

#include <cstddef>
#include <cstdint>

/* =========================================================================
 * Generic definitions
 * ========================================================================= */

/** Value of a boolean ECUC parameter that is enabled. */
#define DOIP_CFG_TRUE   1
/** Value of a boolean ECUC parameter that is disabled. */
#define DOIP_CFG_FALSE  0

#ifndef DOIP_GATEWAY
#define DOIP_GATEWAY  0x00U
#endif

#ifndef DOIP_NODE
#define DOIP_NODE  0x01U
#endif

/* =========================================================================
 * DoIPGeneral
 * ========================================================================= */

/**
 * DoIPMaxRequestBytes
 *  - ECUC item     : ECUC_DoIP_00019
 *  - Parent        : DoIPGeneral
 *  - Type          : EcucIntegerParamDef
 *  - Range         : 1 .. 4294967295
 *  - Multiplicity  : 1
 *  - Default       : no usable scalar default in the extraction
 *  - Post-build    : false (extracted field; value-configuration meaning is
 *                    unresolved)
 *  - PROJECT EXAMPLE: 4096UL; existing value retained for compatibility.
 *  - Description   : Maximum allowed number of bytes of a DoIP request
 *                    message, NOT counting the 8-byte DoIP generic header.
 *  - Requirement   : SWS_DoIP_00017 - if the received payload length is larger
 *                    than this value the module answers with generic NACK
 *                    0x02 ("Message too large") and discards the message.
 *  - Source        : 10.2.3, PDF p.89; 7.3.2.1, PDF p.30
 *  - Used by       : ParseLimits::max_request_bytes (DoIPMessage.hpp).
 */
#define DoIPMaxRequestBytes  4096UL

/**
 * DoIPLogicalAddress
 *  - ECUC item     : ECUC_DoIP_00020
 *  - Parent        : DoIPConfigSet
 *  - Type / Range  : EcucIntegerParamDef, 0 .. 65535
 *  - Multiplicity  : 1
 *  - Post-build    : true
 *  - PROJECT EXAMPLE: 0x0E00U; existing value retained for compatibility.
 *  - Description   : Logical address of this DoIP entity.
 *  - Requirement   : SWS_DoIP_00118 - sent as "Logical Address DoIP entity"
 *                    in the Routing Activation Response.
 *  - Source        : 7.3.2.3.2, PDF p.39
 */
#define DoIPLogicalAddress  0x0E00U

/**
 * DoIPMaxTesterConnections
 *  - ECUC item     : ECUC_DoIP_00012
 *  - Parent        : DoIPInterface
 *  - Type / Range  : EcucIntegerParamDef, 1 .. 255
 *  - Multiplicity  : 1
 *  - Post-build    : false
 *  - Default       : not specified in the extraction
 *  - PROJECT EXAMPLE: 2U; existing value retained for compatibility.
 *  - Description   : Maximum number of simultaneously registered tester
 *                    connections considered during Routing Activation.
 *  - Requirement   : SWS_DoIP_00107 - the number of registered connections is
 *                    compared with this value; when it is reached and no
 *                    registered socket can be replaced the request is rejected
 *                    (response code 0x01).
 *  - Source        : 7.3.2.3.1, PDF p.36
 */
#define DoIPMaxTesterConnections  2U

/**
 * DoIPAliveCheckResponseTimeout
 *  - ECUC item     : ECUC_DoIP_00009
 *  - Parent        : DoIPInterface
 *  - Type / Range  : EcucFloatParamDef, [0 .. INF] seconds
 *  - Multiplicity  : 1
 *  - Post-build    : false
 *  - Default       : not specified in the extraction
 *  - PROJECT EXAMPLE / compatibility: preserve the existing scalar 500U;
 *                    the extraction specifies seconds, but downstream unit
 *                    handling is unresolved; no unit conversion is performed.
 *  - Description   : Time the DoIP entity waits for an Alive Check Response
 *                    after sending an Alive Check Request.
 *  - Requirement   : SWS_DoIP_00105, SWS_DoIP_00140 - on expiry the socket
 *                    that did not answer is closed.
 *  - Source        : 7.3.2.3.1, PDF p.36; 7.3.2.4.1, PDF p.39
 */
#define DoIPAliveCheckResponseTimeout  500U

/* =========================================================================
 * DoIPTcpConnection
 * ========================================================================= */

/**
 * DoIPTcpConnectionSecurityRequired
 *  - ECUC item     : ECUC_DoIP_00097
 *  - Parent        : DoIPTcpConnection
 *  - Type          : EcucBooleanParamDef
 *  - Multiplicity  : 0 .. 1
 *  - Default       : false (if not set)
 *  - Description   : Indicates that the TCP connection requires a secure
 *                    (e.g. TLS) transport.
 *  - Requirement   : SWS_DoIP_00294 / SWS_DoIP_00295 - evaluated together with
 *                    DoIPRoutingActivationSecurityRequired: an activation that
 *                    requires security on a non-secure connection is rejected
 *                    (response code 0x06).
 *  - Source        : 10.2.15, PDF p.113; 7.2, PDF p.27
 */
#define DoIPTcpConnectionSecurityRequired  DOIP_CFG_FALSE

/* =========================================================================
 * DoIPTester
 * ========================================================================= */

/*
 * ECUC_DoIP_00031; DoIPInterface child container; multiplicity 1 .. 255.
 * The configuration tool enforces this multiplicity.
 */

/**
 * DoIPTesterSA
 *  - ECUC item     : ECUC_DoIP_00043
 *  - Parent        : DoIPTester
 *  - Type          : EcucIntegerParamDef
 *  - Range         : 0 .. 65535 (16-bit logical address)
 *  - Multiplicity  : 1
 *  - Default       : not specified
 *  - Post-build    : true
 *  - PROJECT EXAMPLE: 0x0E80U; existing value retained for compatibility.
 *  - Description   : Source address of the tester used in Routing Activation
 *                    and diagnostic messages.
 *  - Requirement   : SWS_DoIP_00102 - the source address of the request is
 *                    matched against this value (unknown -> response 0x00);
 *                    SWS_DoIP_00116 - echoed as "Logical Address Tester".
 *  - Source        : 10.2.26, PDF p.125; 7.3.2.3.1-7.3.2.3.2, PDF pp.36, 39
 */
#define DoIPTesterSA  0x0E80U

/**
 * DoIPRoutingActivationRef
 *  - ECUC item     : ECUC_DoIP_00062
 *  - Parent        : DoIPTester
 *  - Type          : Reference to DoIPRoutingActivation
 *  - Multiplicity  : 1 .. 255
 *  - Post-build    : true
 *  - Description   : Routing Activations that are possible for this tester.
 *  - Requirement   : SWS_DoIP_00108 - the request "Activation Type" must equal
 *                    DoIPRoutingActivationNumber of one referenced activation
 *                    (otherwise response 0x06 "unsupported activation type").
 *  - Source        : 10.2.26, PDF p.125; 7.3.2.3.1, PDF p.37
 *  - PROJECT EXAMPLE: count 1U and the existing activation-number list are
 *                    retained; these do not resolve an AUTOSAR reference.
 *  - Realisation   : number of referenced activations (a reference itself is
 *                    modelled as the DoIPRoutingActivationNumber list below).
 */
#define DoIPRoutingActivationRefCount  1U
/** Activation numbers (DoIPRoutingActivationNumber) referenced by the tester. */
#define DoIPRoutingActivationRefList   { DoIPRoutingActivationNumber }

/* =========================================================================
 * DoIPRoutingActivation
 * ========================================================================= */

/**
 * DoIPRoutingActivationNumber
 *  - ECUC item     : ECUC_DoIP_00033
 *  - Parent        : DoIPRoutingActivation
 *  - Type          : EcucIntegerParamDef
 *  - Range         : 0 .. 255
 *  - Multiplicity  : 1
 *  - Default       : not specified
 *  - Post-build    : true
 *  - PROJECT EXAMPLE: 0x00U; the extraction has no usable scalar default.
 *  - Description   : Routing Activation Number expected in a Routing
 *                    Activation request ("Activation Type" field).
 *  - Requirement   : SWS_DoIP_00108.
 *  - Source        : 10.2.23, PDF p.119; 7.3.2.3.1, PDF p.37
 *  - Note          : ISO 13400-2 reserves 0x00 = default, 0x01 =
 *                    WWH-OBD, 0xE0 = central security (not from the ARXML).
 */
#define DoIPRoutingActivationNumber  0x00U

/**
 * DoIPTargetAddressRef
 *  - ECUC item     : ECUC_DoIP_00034
 *  - Parent        : DoIPRoutingActivation
 *  - Type          : Reference to DoIPTargetAddress
 *  - Multiplicity  : 1 .. 65535
 *  - Post-build    : true
 *  - Description   : All DoIPTargetAddress entries activated by this Routing
 *                    Activation.
 *  - Requirement   : SWS_DoIP_00113 - after a successful activation routing
 *                    to these target addresses is enabled.
 *  - Source        : 10.2.23, PDF p.120; 7.3.2.3.1, PDF p.38
 *  - PROJECT EXAMPLE: count 1U and the existing target list are retained; they
 *                    provide logical-address values, not resolved ECUC refs.
 *  - Realisation   : count + initializer list of target logical addresses.
 */
#define DoIPTargetAddressRefCount  1U
/** Target logical addresses enabled by this Routing Activation. */
#define DoIPTargetAddressRefList   { 0x0E00U }

/**
 * DoIPRoutingActivationSecurityRequired
 *  - ECUC item     : ECUC_DoIP_00096
 *  - Parent        : DoIPRoutingActivation
 *  - Type          : EcucBooleanParamDef
 *  - Multiplicity  : 0 .. 1
 *  - Default       : false
 *  - Post-build    : not stated in the extract
 *  - Description   : This Routing Activation requires a secure TCP connection.
 *  - Requirement   : SWS_DoIP_00294 / SWS_DoIP_00295 together with
 *                    DoIPTcpConnectionSecurityRequired.
 *  - Source        : 10.2.23, PDF p.120; 7.2, PDF p.27
 */
#define DoIPRoutingActivationSecurityRequired  DOIP_CFG_FALSE

/* =========================================================================
 * DoIPRoutingActivationAuthenticationCallback
 * ========================================================================= */

/**
 * DoIPRoutingActivationAuthenticationFunc
 *  - ECUC item     : ECUC_DoIP_00039
 *  - Parent        : DoIPRoutingActivationAuthenticationCallback
 *  - Type          : EcucFunctionNameDef
 *  - Multiplicity  : 0 .. 1
 *  - Default       : not specified
 *  - Post-build    : not specified
 *  - Configuration : preserve the existing project function symbol; the
 *                    extracted "Regular Expression" default is not a usable
 *                    function-name value.
 *  - Description   : Name of the authentication callback function.
 *  - Requirement   : SWS_DoIP_00109 - 00111, API SWS_DoIP_00049. If omitted
 *                    while the container is configured, the RPort
 *                    CB<RoutingActivation>RoutingActivation of service
 *                    interface <RoutingActivation>_RoutingActivation is used.
 *  - Source        : 10.2.24, PDF pp.120-121; 8.6.3.3, PDF p.73
 */
#define DoIPRoutingActivationAuthenticationFunc  DoIP_AuthenticationCallback

/**
 * DoIPRoutingActivationAuthenticationReqLength
 *  - ECUC item     : ECUC_DoIP_00040
 *  - Parent        : DoIPRoutingActivationAuthenticationCallback
 *  - Type          : EcucIntegerParamDef
 *  - Range         : 0 .. 4 (bytes)
 *  - Multiplicity  : 1
 *  - Default       : not specified
 *  - Post-build    : false
 *  - PROJECT EXAMPLE: 0U; existing value retained for compatibility.
 *  - Description   : Number of OEM-specific request bytes used for
 *                    authentication; if non-zero they are passed to the
 *                    authentication callback.
 *  - Source        : 10.2.24, PDF p.121; 8.6.3.3, PDF p.73
 */
#define DoIPRoutingActivationAuthenticationReqLength  0U

/**
 * DoIPRoutingActivationAuthenticationResLength
 *  - ECUC item     : ECUC_DoIP_00041
 *  - Parent        : DoIPRoutingActivationAuthenticationCallback
 *  - Type          : EcucIntegerParamDef
 *  - Range         : 0 .. 4 (bytes)
 *  - Multiplicity  : 1
 *  - Default       : not specified
 *  - Post-build    : false
 *  - PROJECT EXAMPLE: 0U; existing value retained for compatibility.
 *  - Description   : Number of authentication response bytes; if 0 the
 *                    AuthenticationResData is not fetched through the API,
 *                    otherwise it is appended to the OEM-specific field of the
 *                    Routing Activation Response.
 *  - Requirement   : SWS_DoIP_00120.
 *  - Source        : 10.2.24, PDF p.121; 7.3.2.3.1-7.3.2.3.2, PDF pp.37-39
 */
#define DoIPRoutingActivationAuthenticationResLength  0U

/* =========================================================================
 * DoIPRoutingActivationConfirmationCallback
 * ========================================================================= */

/**
 * DoIPRoutingActivationConfirmationFunc
 *  - ECUC item     : ECUC_DoIP_00036
 *  - Parent        : DoIPRoutingActivationConfirmationCallback
 *  - Type          : EcucFunctionNameDef
 *  - Multiplicity  : 0 .. 1
 *  - Default       : not specified
 *  - Post-build    : not specified
 *  - Configuration : preserve the existing project function symbol; the
 *                    extracted "Regular Expression" default is not a usable
 *                    function-name value.
 *  - Description   : Name of the confirmation callback function.
 *  - Requirement   : SWS_DoIP_00112, SWS_DoIP_00114, SWS_DoIP_00274. If
 *                    omitted while the container is configured, the RPort
 *                    CB<RoutingActivation>RoutingActivation is used.
 *  - Source        : 10.2.25, PDF pp.121-122; 8.6.3.2, PDF p.72
 */
#define DoIPRoutingActivationConfirmationFunc  DoIP_ConfirmationCallback

/**
 * DoIPRoutingActivationConfirmationReqLength
 *  - ECUC item     : ECUC_DoIP_00037
 *  - Parent        : DoIPRoutingActivationConfirmationCallback
 *  - Type          : EcucIntegerParamDef
 *  - Range         : 0 .. 4 (bytes)
 *  - Multiplicity  : 1
 *  - Default       : not specified
 *  - Post-build    : false
 *  - PROJECT EXAMPLE: 0U; existing value retained for compatibility.
 *  - Description   : Number of trailing OEM-specific request bytes handed to
 *                    the confirmation function.
 *  - Source        : 10.2.25, PDF p.122; 8.6.3.2, PDF p.72
 */
#define DoIPRoutingActivationConfirmationReqLength  0U

/**
 * DoIPRoutingActivationConfirmationResLength
 *  - ECUC item     : ECUC_DoIP_00038
 *  - Parent        : DoIPRoutingActivationConfirmationCallback
 *  - Type          : EcucIntegerParamDef
 *  - Range         : 0 .. 4 (bytes)
 *  - Multiplicity  : 1
 *  - Default       : not specified
 *  - Post-build    : false
 *  - PROJECT EXAMPLE: 0U; existing value retained for compatibility.
 *  - Description   : Number of OEM-specific response bytes returned by the
 *                    confirmation function; when non-zero and the function
 *                    returns E_OK they are attached to the OEM-specific field
 *                    of the Routing Activation Response.
 *  - Requirement   : SWS_DoIP_00112, SWS_DoIP_00120.
 *  - Source        : 10.2.25, PDF p.122; 7.3.2.3.1-7.3.2.3.2, PDF pp.38-39
 */
#define DoIPRoutingActivationConfirmationResLength  0U

/* =========================================================================
 * Additional extracted parameters
 * =========================================================================
 * Scalar values marked PROJECT EXAMPLE fill gaps where the extraction has no
 * usable scalar default. They are illustrative only. Optional reference and
 * callback parameters without an existing representation are listed as
 * unresolved and intentionally have no macro or fabricated target/symbol.
 */

/* DoIPGeneral */
/**
 * ECUC_DoIP_00004; DoIPGeneral; Boolean, multiplicity 1.
 * PROJECT EXAMPLE: disabled; the extraction has no scalar default.
 */
#define DoIPDevelopmentErrorDetect  DOIP_CFG_FALSE
/**
 * ECUC_DoIP_00067; DoIPGeneral; Boolean, multiplicity 1.
 * PROJECT EXAMPLE: disabled; the extraction has no scalar default.
 */
#define DoIPDhcpOptionVinUse  DOIP_CFG_FALSE
/**
 * ECUC_DoIP_00064; DoIPGeneral; Boolean, multiplicity 1.
 * PROJECT EXAMPLE: disabled; the extraction has no scalar default.
 */
#define DoIPEntityStatusMaxByteFieldUse  DOIP_CFG_FALSE
/**
 * ECUC_DoIP_00065; DoIPGeneral; Integer, 0 .. 255, multiplicity 1.
 * PROJECT EXAMPLE: 255 (the description permits only 0 or 255).
 */
#define DoIPGIDInvalidityPattern  255U
/**
 * ECUC_DoIP_00073; DoIPGeneral; Integer, 5 .. 255, multiplicity 1.
 * PROJECT EXAMPLE: minimum valid size.
 */
#define DoIPHostNameSizeMax  5U
/**
 * ECUC_DoIP_00006; DoIPGeneral; Float, ]0 .. INF[ seconds, multiplicity 1.
 * PROJECT EXAMPLE: positive 10 ms period, represented in seconds.
 */
#define DoIPMainFunctionPeriod  0.01F
/**
 * ECUC_DoIP_00074; DoIPGeneral; Integer, 1 .. 255, multiplicity 1.
 * PROJECT EXAMPLE: minimum valid request count.
 */
#define DoIPMaxUDPRequestPerConnection  1U
/**
 * ECUC_DoIP_00021; DoIPGeneral; Enumeration, multiplicity 1.
 * PROJECT EXAMPLE: DOIP_NODE is the only listed usable literal; no other
 * value is inferred. This is not an extracted default.
 */
#define DoIPNodeType  DOIP_NODE
/**
 * ECUC_DoIP_00018; DoIPGeneral; Boolean, multiplicity 1.
 * PROJECT EXAMPLE: disabled; the extraction has no scalar default.
 */
#define DoIPUseEIDasGID  DOIP_CFG_FALSE

/** ECUC_DoIP_00005; DoIPGeneral; Boolean, multiplicity 1; extracted default false. */
#define DoIPVersionInfoApi  DOIP_CFG_FALSE
/**
 * ECUC_DoIP_00017; DoIPGeneral; Boolean, multiplicity 1.
 * PROJECT EXAMPLE: disabled; the extraction has no scalar default.
 */
#define DoIPVinGidMaster  DOIP_CFG_FALSE
/**
 * ECUC_DoIP_00066; DoIPGeneral; Integer, 0 .. 255, multiplicity 1.
 * PROJECT EXAMPLE: 0 (the description permits only 0 or 255).
 */
#define DoIPVinInvalidityPattern  0U

/* DoIPConfigSet */
/**
 * ECUC_DoIP_00014; DoIPConfigSet; Integer, 0 .. 281474976710655,
 * multiplicity 0..1. The extracted default is page noise, not a value.
 * PROJECT EXAMPLE: zero; not an AUTOSAR default.
 */
#define DoIPEid  0ULL
/**
 * ECUC_DoIP_00015; DoIPConfigSet; Integer, 0 .. 281474976710655,
 * multiplicity 0..1.
 * PROJECT EXAMPLE: zero; not an AUTOSAR default.
 */
#define DoIPGid  0ULL

/* DoIPInterface */
/**
 * ECUC_DoIP_00068; DoIPInterface; Float, ]0 .. INF[ seconds, multiplicity 1.
 * PROJECT EXAMPLE: one second.
 */
#define DoIPGeneralInactivityTime  1.0F
/**
 * ECUC_DoIP_00010; DoIPInterface; Float, ]0 .. INF[ seconds, multiplicity 1.
 * PROJECT EXAMPLE: one second.
 */
#define DoIPInitialInactivityTime  1.0F
/**
 * ECUC_DoIP_00008; DoIPInterface; Float, [0 .. INF] seconds, multiplicity 1.
 * PROJECT EXAMPLE: zero seconds.
 */
#define DoIPInitialVehicleAnnouncementTime  0.0F
/**
 * ECUC_DoIP_00101; DoIPInterface; Boolean, multiplicity 1.
 * PROJECT EXAMPLE: false (network interface always available).
 */
#define DoIPInterfaceActLineCtrl  DOIP_CFG_FALSE
/** ECUC_DoIP_00099; DoIPInterface; Enumeration, multiplicity 1; extracted default. */
#define DoIPInterfaceAnnouncementStart  DOIP_AUTOMATIC_ANNOUNCE
/**
 * ECUC_DoIP_00098; DoIPInterface; generated symbolic integer, 0 .. 255,
 * multiplicity 1.
 * PROJECT EXAMPLE: 0; confirm generated handle assignment in ECUC integration.
 */
#define DoIPInterfaceId  0U
/**
 * ECUC_DoIP_00013; DoIPInterface; Boolean, multiplicity 1.
 * PROJECT EXAMPLE: false; the extraction does not establish a dependency
 * resolution/default for the referenced DoIPEID.
 */
#define DoIPUseMacAddressForIdentification  DOIP_CFG_FALSE
/**
 * ECUC_DoIP_00016; DoIPInterface; Boolean, multiplicity 1.
 * PROJECT EXAMPLE: disabled; the extraction has no scalar default.
 */
#define DoIPUseVehicleIdentificationSyncStatus  DOIP_CFG_FALSE
/**
 * ECUC_DoIP_00094; DoIPInterface; Integer, 1 .. 255, multiplicity 0..1.
 * PROJECT EXAMPLE: minimum valid count.
 */
#define DoIPVehicleAnnouncementCount  1U
/**
 * ECUC_DoIP_00007; DoIPInterface; Float, [0 .. INF] seconds,
 * multiplicity 0..1.
 * PROJECT EXAMPLE: one second.
 */
#define DoIPVehicleAnnouncementInterval  1.0F

/* DoIPChannel references: unresolved because no reference-handle representation
 * or actual DoIPTester/DoIPTargetAddress instance is defined by this source.
 * ECUC_DoIP_00070: DoIPChannel.DoIPChannelSARef -> DoIPTester.
 * ECUC_DoIP_00071: DoIPChannel.DoIPChannelTARef -> DoIPTargetAddress.
 */

/* DoIPPduRRxPdu and DoIPPduRTxPdu */
/**
 * ECUC_DoIP_00057; DoIPPduRRxPdu; generated symbolic integer, 0 .. 65535.
 * PROJECT EXAMPLE: 0; use the integration's generated PduR handle in production.
 */
#define DoIPPduRRxPduId  0U
/**
 * ECUC_DoIP_00060; DoIPPduRTxPdu; generated symbolic integer, 0 .. 65535.
 * PROJECT EXAMPLE: 0; use the integration's generated PduR handle in production.
 */
#define DoIPPduRTxPduId  0U

/** ECUC_DoIP_00075; DoIPPduRTxPdu; extracted default DOIP_TPPDU. */
#define DoIPPduType  DOIP_TPPDU
/*
 * Unresolved references: ECUC_DoIP_00058 (DoIPPduRRxPduRef) and
 * ECUC_DoIP_00059 (DoIPPduRTxPduRef), both target Pdu. No target symbols or
 * reference-handle convention are supplied by the extraction/current header.
 */

/* DoIPTargetAddress */
/**
 * ECUC_DoIP_00054; DoIPTargetAddress; Integer, 0 .. 65535, multiplicity 1.
 * PROJECT EXAMPLE: retain the existing target address used by the list below.
 */
#define DoIPTargetAddressValue  0x0E00U

/* Connection request-address-assignment values. ECUC_DoIP_00095 is qualified
 * by its distinct parent container to avoid conflating the three definitions.
 * Each value is the extracted default true.
 */
/** ECUC_DoIP_00095; DoIPTcpConnection.DoIPRequestAddressAssignment. */
#define DoIPTcpConnectionRequestAddressAssignment  DOIP_CFG_TRUE
/** ECUC_DoIP_00095; DoIPUdpConnection.DoIPRequestAddressAssignment. */
#define DoIPUdpConnectionRequestAddressAssignment  DOIP_CFG_TRUE
/** ECUC_DoIP_00095; DoIPUdpVehicleAnnouncementConnection.DoIPRequestAddressAssignment. */
#define DoIPUdpVehicleAnnouncementConnectionRequestAddressAssignment  DOIP_CFG_TRUE

/* DoIPSoAdTcpRxPdu and DoIPSoAdTcpTxPdu */
/**
 * ECUC_DoIP_00082; DoIPSoAdTcpRxPdu; generated symbolic integer, 0 .. 65535.
 * PROJECT EXAMPLE: 0; replace with the integration's generated SoAd handle.
 */
#define DoIPSoAdTcpRxPduId  0U
/**
 * ECUC_DoIP_00085; DoIPSoAdTcpTxPdu; generated symbolic integer, 0 .. 65535.
 * PROJECT EXAMPLE: 0; replace with the integration's generated SoAd handle.
 */
#define DoIPSoAdTcpTxPduId  0U
/**
 * Second TCP connection instance (ECUC_DoIP_00082 and ECUC_DoIP_00085 under
 * ECUC_DoIP_00045, multiplicity 2 .. 255). PROJECT EXAMPLE: 1.
 */
#define DoIPSecondSoAdTcpRxPduId  1U
#define DoIPSecondSoAdTcpTxPduId  1U
/*
 * Unresolved references:
 * ECUC_DoIP_00083 DoIPSoAdTcpRxPdu.DoIPSoAdTcpRxPduRef -> Pdu.
 * ECUC_DoIP_00084 DoIPSoAdTcpTxPdu.DoIPSoAdTcpTxPduRef -> Pdu.
 * No external PDU symbols are invented.
 */

/* DoIPSoAdUdpRxPdu and DoIPSoAdUdpTxPdu */
/**
 * ECUC_DoIP_00048; DoIPSoAdUdpRxPdu; generated symbolic integer, 0 .. 65535.
 * PROJECT EXAMPLE: 0; replace with the integration's generated SoAd handle.
 */
#define DoIPSoAdUdpRxPduId  0U
/**
 * ECUC_DoIP_00051; DoIPSoAdUdpTxPdu; generated symbolic integer, 0 .. 65535.
 * PROJECT EXAMPLE: 0; replace with the integration's generated SoAd handle.
 */
#define DoIPSoAdUdpTxPduId  0U
/*
 * Unresolved references:
 * ECUC_DoIP_00049 DoIPSoAdUdpRxPdu.DoIPSoAdUdpRxPduRef -> Pdu.
 * ECUC_DoIP_00050 DoIPSoAdUdpTxPdu.DoIPSoAdUdpTxPduRef -> Pdu.
 * No external PDU symbols are invented.
 */

/* DoIPSoAdUdpVehicleAnnouncementTxPdu */
/**
 * ECUC_DoIP_00078; DoIPSoAdUdpVehicleAnnouncementTxPdu; generated symbolic
 * integer, 0 .. 4294967295. PROJECT EXAMPLE: 0.
 */
#define DoIPSoAdUdpVehicleAnnouncementTxPduId  0UL
/*
 * Unresolved Pdu reference ECUC_DoIP_00079:
 * DoIPSoAdUdpVehicleAnnouncementTxPduRef -> Pdu.
 */

/* DoIPRoutingActivation callback and tester parameters */
/**
 * ECUC_DoIP_00042; DoIPTester; Integer, 0 .. 4294967295, multiplicity 1.
 * PROJECT EXAMPLE: zero.
 */
#define DoIPNumByteDiagAckNack  0UL

/*
 * Optional callback parameters with no established empty/absence
 * representation remain unresolved; no callback names are fabricated:
 * ECUC_DoIP_00093 DoIPFurtherActionByteCallback.DoIPFurtherActionByteDirect.
 * ECUC_DoIP_00028 DoIPGetGidCallback.DoIPGetGidDirect.
 * ECUC_DoIP_00027 DoIPPowerModeCallback.DoIPPowerModeDirect.
 * ECUC_DoIP_00029 DoIPTriggerGidSyncCallback.DoIPTriggerGidSyncDirect.
 *
 * Existing authentication and confirmation callback macros above retain their
 * configured function symbols. Their trace IDs are ECUC_DoIP_00039 and
 * ECUC_DoIP_00036 respectively; callback signatures/ABI remain unspecified.
 *
 * References already represented by the existing count/list convention:
 * ECUC_DoIP_00062 DoIPTester.DoIPRoutingActivationRef is represented by the
 * existing DoIPRoutingActivationRefCount/DoIPRoutingActivationRefList macros.
 * ECUC_DoIP_00034 DoIPRoutingActivation.DoIPTargetAddressRef is represented
 * by the existing DoIPTargetAddressRefCount/DoIPTargetAddressRefList macros.
 */

namespace as {
namespace doip {
namespace configuration {

enum class DoIPNodeTypeValue : std::uint8_t
{
    Gateway = DOIP_GATEWAY,
    Node = DOIP_NODE
};

enum class DoIPAnnouncementStart : std::uint8_t
{
    Automatic,
    OnTrigger
};

enum class DoIPPduTypeValue : std::uint8_t
{
    InterfacePdu,
    TransportProtocolPdu
};

struct DoIPGeneralConfig
{
    std::uint32_t maxRequestBytes;
    float mainFunctionPeriodSeconds;
    std::uint16_t hostNameSizeMax;
    std::uint8_t gidInvalidityPattern;
    std::uint8_t vinInvalidityPattern;
    std::uint8_t maxUdpRequestsPerConnection;
    bool developmentErrorDetect;
    bool dhcpOptionVinUse;
    bool entityStatusMaxByteFieldUse;
    bool useEidAsGid;
    bool versionInfoApi;
    bool vinGidMaster;
    DoIPNodeTypeValue nodeType;
};

struct DoIPInterfaceConfig;

struct DoIPConfigSet
{
    std::uint64_t eid;
    std::uint64_t gid;
    std::uint16_t logicalAddress;
    bool eidConfigured;
    bool gidConfigured;
    /* @ECUC_DoIP_00100 */
    const DoIPInterfaceConfig* interfaces;
    /* @ECUC_DoIP_00100 */
    std::size_t interfaceCount;
};

struct DoIPTesterConfig;
struct DoIPRoutingActivationConfig;
struct DoIPChannelConfig;
struct DoIPConnectionsConfig;

struct DoIPInterfaceConfig
{
    std::uint8_t interfaceId;
    float aliveCheckResponseTimeoutSeconds;
    float generalInactivityTimeSeconds;
    float initialInactivityTimeSeconds;
    float initialVehicleAnnouncementTimeSeconds;
    float vehicleAnnouncementIntervalSeconds;
    std::uint8_t maxTesterConnections;
    std::uint8_t vehicleAnnouncementCount;
    bool vehicleAnnouncementCountConfigured;
    bool vehicleAnnouncementIntervalConfigured;
    bool activationLineControl;
    bool useMacAddressForIdentification;
    bool useVehicleIdentificationSyncStatus;
    DoIPAnnouncementStart announcementStart;
    /* @ECUC_DoIP_00031 */
    const DoIPTesterConfig* testers;
    std::size_t testerCount;
    /* @ECUC_DoIP_00030 */
    const DoIPRoutingActivationConfig* routingActivations;
    /* @ECUC_DoIP_00030 */
    std::size_t routingActivationCount;
    /* @ECUC_DoIP_00069 */
    const DoIPChannelConfig* channels;
    /* @ECUC_DoIP_00069 */
    std::size_t channelCount;
    /* @ECUC_DoIP_00032 */
    const DoIPConnectionsConfig* connections;
};

struct DoIPSoAdTcpRxPduConfig
{
    std::uint16_t pduId;
};

struct DoIPSoAdTcpTxPduConfig
{
    std::uint16_t pduId;
};

struct DoIPTcpConnectionConfig
{
    bool requestAddressAssignment;
    bool securityRequired;
    DoIPSoAdTcpRxPduConfig soAdTcpRxPdu;
    DoIPSoAdTcpTxPduConfig soAdTcpTxPdu;
};

struct DoIPSoAdUdpRxPduConfig
{
    std::uint16_t pduId;
};

struct DoIPSoAdUdpTxPduConfig
{
    std::uint16_t pduId;
};

struct DoIPUdpConnectionConfig
{
    bool requestAddressAssignment;
    DoIPSoAdUdpRxPduConfig soAdUdpRxPdu;
    DoIPSoAdUdpTxPduConfig soAdUdpTxPdu;
};

struct DoIPSoAdUdpVehicleAnnouncementTxPduConfig
{
    /* @ECUC_DoIP_00078 */
    std::uint32_t pduId;
};

struct DoIPUdpVehicleAnnouncementConnectionConfig
{
    bool requestAddressAssignment;
    /* @ECUC_DoIP_00077 */
    DoIPSoAdUdpVehicleAnnouncementTxPduConfig soAdUdpVehicleAnnouncementTxPdu;
};

/* @ECUC_DoIP_00058 */
/* @ECUC_DoIP_00059 */
struct PduConfig
{
};

/* @ECUC_DoIP_00055 */
struct DoIPPduRRxPduConfig
{
    std::uint16_t pduId;
    const PduConfig* pduRef;
};

struct DoIPPduRTxPduConfig
{
    std::uint16_t pduId;
    DoIPPduTypeValue pduType;
    bool pduTypeConfigured;
    const PduConfig* pduRef;
};

struct DoIPTargetAddressConfig
{
    std::uint16_t value;
};

/* @ECUC_DoIP_00032 */
struct DoIPConnectionsConfig
{
    /* @ECUC_DoIP_00045 */
    const DoIPTcpConnectionConfig* tcpConnections;
    std::size_t tcpConnectionCount;
    /* @ECUC_DoIP_00052 */
    const DoIPUdpConnectionConfig* udpConnections;
    std::size_t udpConnectionCount;
    /* @ECUC_DoIP_00076 */
    const DoIPUdpVehicleAnnouncementConnectionConfig* announcementConnections;
    std::size_t announcementConnectionCount;
    /* @ECUC_DoIP_00054 */
    const DoIPTargetAddressConfig* targets;
    std::size_t targetCount;
};

struct DoIPCallbackDataConfig
{
    std::uint8_t authenticationRequestLength;
    std::uint8_t authenticationResponseLength;
    std::uint8_t confirmationRequestLength;
    std::uint8_t confirmationResponseLength;
    bool authenticationCallbackConfigured;
    bool confirmationCallbackConfigured;
};

struct DoIPRoutingActivationConfig
{
    std::uint8_t number;
    bool securityRequired;
    const DoIPTargetAddressConfig* targets;
    std::size_t targetCount;
    DoIPCallbackDataConfig callbackData;
};

/* @ECUC_DoIP_00031 */
struct DoIPTesterConfig
{
    /* @ECUC_DoIP_00043 */
    std::uint16_t sourceAddress;
    /* @ECUC_DoIP_00042 */
    std::uint32_t diagnosticAckNackBytes;
    /* @ECUC_DoIP_00062 */
    const DoIPRoutingActivationConfig* activations;
    /* @ECUC_DoIP_00062 */
    std::size_t activationCount;
};

struct DoIPChannelConfig
{
    const DoIPTesterConfig* sourceTester;
    const DoIPTargetAddressConfig* target;
    const DoIPPduRRxPduConfig* pduRRxPdu;
    const DoIPPduRTxPduConfig* pduRTxPdu;
};

struct DoIPConfiguration
{
    DoIPGeneralConfig general;
    DoIPConfigSet configSet;
};

static const DoIPTargetAddressConfig DOIP_DEFAULT_TARGETS[] = {
    {DoIPTargetAddressValue}
};

static const DoIPTcpConnectionConfig DOIP_DEFAULT_TCP_CONNECTIONS[] = {
    {
        DoIPTcpConnectionRequestAddressAssignment != DOIP_CFG_FALSE,
        DoIPTcpConnectionSecurityRequired != DOIP_CFG_FALSE,
        {DoIPSoAdTcpRxPduId},
        {DoIPSoAdTcpTxPduId}
    },
    {
        DoIPTcpConnectionRequestAddressAssignment != DOIP_CFG_FALSE,
        DoIPTcpConnectionSecurityRequired != DOIP_CFG_FALSE,
        {DoIPSecondSoAdTcpRxPduId},
        {DoIPSecondSoAdTcpTxPduId}
    }
};

static const DoIPUdpConnectionConfig DOIP_DEFAULT_UDP_CONNECTIONS[] = {
    {
        DoIPUdpConnectionRequestAddressAssignment != DOIP_CFG_FALSE,
        {DoIPSoAdUdpRxPduId},
        {DoIPSoAdUdpTxPduId}
    }
};

static const DoIPUdpVehicleAnnouncementConnectionConfig
    DOIP_DEFAULT_ANNOUNCEMENT_CONNECTIONS[] = {
        {
            DoIPUdpVehicleAnnouncementConnectionRequestAddressAssignment != DOIP_CFG_FALSE,
            {DoIPSoAdUdpVehicleAnnouncementTxPduId}
        }
    };

static const DoIPConnectionsConfig DOIP_DEFAULT_CONNECTIONS = {
    DOIP_DEFAULT_TCP_CONNECTIONS,
    sizeof(DOIP_DEFAULT_TCP_CONNECTIONS) / sizeof(DOIP_DEFAULT_TCP_CONNECTIONS[0]),
    DOIP_DEFAULT_UDP_CONNECTIONS,
    sizeof(DOIP_DEFAULT_UDP_CONNECTIONS) / sizeof(DOIP_DEFAULT_UDP_CONNECTIONS[0]),
    DOIP_DEFAULT_ANNOUNCEMENT_CONNECTIONS,
    sizeof(DOIP_DEFAULT_ANNOUNCEMENT_CONNECTIONS) / sizeof(DOIP_DEFAULT_ANNOUNCEMENT_CONNECTIONS[0]),
    DOIP_DEFAULT_TARGETS,
    sizeof(DOIP_DEFAULT_TARGETS) / sizeof(DOIP_DEFAULT_TARGETS[0])
};

static const DoIPRoutingActivationConfig DOIP_DEFAULT_ROUTING_ACTIVATIONS[] = {
    {
        DoIPRoutingActivationNumber,
        DoIPRoutingActivationSecurityRequired != DOIP_CFG_FALSE,
        DOIP_DEFAULT_TARGETS,
        DoIPTargetAddressRefCount,
        {
            DoIPRoutingActivationAuthenticationReqLength,
            DoIPRoutingActivationAuthenticationResLength,
            DoIPRoutingActivationConfirmationReqLength,
            DoIPRoutingActivationConfirmationResLength,
            true,
            true
        }
    }
};

static const DoIPTesterConfig DOIP_DEFAULT_TESTERS[] = {
    {
        DoIPTesterSA,
        DoIPNumByteDiagAckNack,
        DOIP_DEFAULT_ROUTING_ACTIVATIONS,
        DoIPRoutingActivationRefCount
    }
};

static const PduConfig DOIP_DUMMY_RX_PDU = {};
static const PduConfig DOIP_DUMMY_TX_PDU = {};

static const DoIPPduRRxPduConfig DOIP_DEFAULT_PDU_R_RX = {
    DoIPPduRRxPduId,
    &DOIP_DUMMY_RX_PDU
};

static const DoIPPduRTxPduConfig DOIP_DEFAULT_PDU_R_TX = {
    DoIPPduRTxPduId,
    DoIPPduTypeValue::TransportProtocolPdu,
    true,
    &DOIP_DUMMY_TX_PDU
};

static const DoIPChannelConfig DOIP_DEFAULT_CHANNELS[] = {
    {
        DOIP_DEFAULT_TESTERS,
        DOIP_DEFAULT_TARGETS,
        &DOIP_DEFAULT_PDU_R_RX,
        &DOIP_DEFAULT_PDU_R_TX
    }
};

static const DoIPInterfaceConfig DOIP_DEFAULT_INTERFACES[] = {
    {
        DoIPInterfaceId,
        static_cast<float>(DoIPAliveCheckResponseTimeout),
        DoIPGeneralInactivityTime,
        DoIPInitialInactivityTime,
        DoIPInitialVehicleAnnouncementTime,
        DoIPVehicleAnnouncementInterval,
        DoIPMaxTesterConnections,
        DoIPVehicleAnnouncementCount,
        true,
        true,
        DoIPInterfaceActLineCtrl != DOIP_CFG_FALSE,
        DoIPUseMacAddressForIdentification != DOIP_CFG_FALSE,
        DoIPUseVehicleIdentificationSyncStatus != DOIP_CFG_FALSE,
        DoIPAnnouncementStart::Automatic,
        DOIP_DEFAULT_TESTERS,
        sizeof(DOIP_DEFAULT_TESTERS) / sizeof(DOIP_DEFAULT_TESTERS[0]),
        DOIP_DEFAULT_ROUTING_ACTIVATIONS,
        sizeof(DOIP_DEFAULT_ROUTING_ACTIVATIONS) / sizeof(DOIP_DEFAULT_ROUTING_ACTIVATIONS[0]),
        DOIP_DEFAULT_CHANNELS,
        sizeof(DOIP_DEFAULT_CHANNELS) / sizeof(DOIP_DEFAULT_CHANNELS[0]),
        &DOIP_DEFAULT_CONNECTIONS
    }
};

static const DoIPConfiguration DOIP_DEFAULT_CONFIGURATION = {
    {
        DoIPMaxRequestBytes,
        DoIPMainFunctionPeriod,
        DoIPHostNameSizeMax,
        DoIPGIDInvalidityPattern,
        DoIPVinInvalidityPattern,
        DoIPMaxUDPRequestPerConnection,
        DoIPDevelopmentErrorDetect != DOIP_CFG_FALSE,
        DoIPDhcpOptionVinUse != DOIP_CFG_FALSE,
        DoIPEntityStatusMaxByteFieldUse != DOIP_CFG_FALSE,
        DoIPUseEIDasGID != DOIP_CFG_FALSE,
        DoIPVersionInfoApi != DOIP_CFG_FALSE,
        DoIPVinGidMaster != DOIP_CFG_FALSE,
        static_cast<DoIPNodeTypeValue>(DoIPNodeType)
    },
    {
        DoIPEid,
        DoIPGid,
        DoIPLogicalAddress,
        true,
        true,
        DOIP_DEFAULT_INTERFACES,
        sizeof(DOIP_DEFAULT_INTERFACES) / sizeof(DOIP_DEFAULT_INTERFACES[0])
    }
};

} // namespace configuration
} // namespace doip
} // namespace as

#endif /* DOIP_CONFIGURATION_HPP */
