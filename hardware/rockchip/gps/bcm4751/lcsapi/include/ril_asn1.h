///============================================================================
/// Copyright 2011 Broadcom Corporation
///
/// This program is free software; you can redistribute it and/or modify
/// it under the following terms: 
///
/// Redistribution and use in source and binary forms, with or without 
/// modification, are permitted provided that the following conditions are met:
///  Redistributions of source code must retain the above copyright notice, this
///  list of conditions and the following disclaimer.
///  Redistributions in binary form must reproduce the above copyright notice, 
///  this list of conditions and the following disclaimer in the documentation 
///  and/or other materials provided with the distribution.
///  Neither the name of Broadcom nor the names of its contributors may be used 
///  to endorse or promote products derived from this software without specific 
///  prior written permission.
///  
///  THIS SOFTWARE IS PROVIDED “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
///  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
///  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
///  BROADCOM BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
///  EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, 
///  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;LOSS OF USE, DATA, OR PROFITS; 
///  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
///  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR 
///  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
///  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/// ---------------------------------------------------------------------------
/// \file ril_asn1.h
/// \brief Interface function definitions used for interfacing GPS and RIL
///        (control-plane) using ASN.1 encoded messages
///============================================================================

// ASN1 messages handled by Broadcom GPS CP stack:
//
// 1.	RRC(3G):
// GPS CP stack accepts:
// T_RRC_6_12_DL_DCCH_MessageType_assistanceDataDelivery
// T_RRC_6_12_DL_DCCH_MessageType_measurementControl
//
// GPS CP stack sends:
// T_RRC_6_12_UL_DCCH_MessageType_measurementControlFailure
// T_RRC_6_12_UL_DCCH_MessageType_measurementReport
// T_RRC_6_12_UL_DCCH_MessageType_measurementReport
//
// 2.	RRLP(2G)
//
// GPS CP stack accepts:
// T_ASN1T_RRLP_RRLP_Component_msrPositionReq
// T_ASN1T_RRLP_RRLP_Component_assistanceData
// T_ASN1T_RRLP_RRLP_Component_protocolError
//
// GPS CP stack sends:
// T_ASN1T_RRLP_RRLP_Component_msrPositionRsp
// T_ASN1T_RRLP_RRLP_Component_assistanceDataAck
// T_ASN1T_RRLP_RRLP_Component_protocolError

// REVIEW: add versioning?

#ifndef _BRCM_LBS_HAL_RIL_ASN1_H
#define _BRCM_LBS_HAL_RIL_ASN1_H

#include "brcm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// RRC UE state types
typedef enum BrcmLbsRilAsn1_UeState
{
    BRCM_LBS_RIL_UE_STATE_CELL_DCH,  /**< Dedicated Channel*/
    BRCM_LBS_RIL_UE_STATE_CELL_FACH, /**< Forward Access Channel */
    BRCM_LBS_RIL_UE_STATE_CELL_PCH,  /**< Paging Channel */
    BRCM_LBS_RIL_UE_STATE_URA_PCH,   /**< UTRAN Registration Area Paging Channel */
    BRCM_LBS_RIL_UE_STATE_IDLE       /**< Idle */
} BrcmLbsRilAsn1_UeState;

typedef enum BrcmLbsRilAsn1_Protocol
{
	BRCM_LBS_RIL_RRLP,
	BRCM_LBS_RIL_RRC,
	BRCM_LBS_RIL_LTE
} BrcmLbsRilAsn1_Protocol;

/** Callback definition for sending ASN.1 encoded message to network.
 *  \param protocol [input] protocol type of encoded message
 *  \param msg [input] ASN.1 encoded message
 *  \param size [input]  The size of message
 *  \param userData [input] user data specified in BrcmLbsRilAsn1_init() call 
 *  \return BRCM_LBS_OK if message properly sent
 */
typedef BrcmLbs_Result (*BrcmLbsRilAsn1_OnSendToNetwork)(BrcmLbsRilAsn1_Protocol protocol, const unsigned char *msg, size_t size, BrcmLbs_UserData userData);

/** Callback definition for sending the SUPL Cert path to the RIL
 *  \param path [input] the path to the cert file
 *  \param userData [input] user data specified in BrcmLbsRilAsn1_init() call 
 *  \return void
 */
typedef void (*BrcmLbsRilAsn1_OnSetCertPath)(const char *path, BrcmLbs_UserData userData);

/** Send an message(AT-command) to the GPS daemon.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \param protocol [input] protocol type of encoded message
 *  \param msg [input] ASN.1 encoded message
 *  \param size [input]  The size of message
 *  \return BRCM_LBS_OK if message properly sent
 */
BrcmLbs_Result BrcmLbsRilAsn1_sendToGps(OsHandle ril, BrcmLbsRilAsn1_Protocol protocol, const unsigned char *msg, size_t size);


/** Setting SLP FQDN from the SIM card
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \param fqdn pointer to SLP FQDN string from the SIM card. 
 *              NULL when SIM does not have SLP FQDN information
 *  \return OS_HANDLE_INVALID if failure, some other value otherwise
 */
BrcmLbs_Result BrcmLbsRilAsn1_SetSlpFqdn(OsHandle ril, const char *fqdn);

/** Sets the current UE State.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \param protocol [input] protocol type of encoded message
 *  \param state [input] (RRC) UE State
 *  \return BRCM_LBS_OK if state properly set
 *  \note Currently the protocol value has to be set to BRCM_LBS_RIL_RRC, this function is of no use for RRLP
 */
BrcmLbs_Result BrcmLbsRilAsn1_setUeState(OsHandle ril, BrcmLbsRilAsn1_Protocol protocol, BrcmLbsRilAsn1_UeState state);

/** Reset all assistance data prior to next positioning/measurement request.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \return BRCM_LBS_OK if assiatance data properly reset
 */
BrcmLbs_Result BrcmLbsRilAsn1_resetAssistanceData(OsHandle ril, BrcmLbsRilAsn1_Protocol protocol);

/********************************************************************************/
/** Structure for c-plane HAL callbacks
*********************************************************************************/
typedef struct BrcmLbsRilAsn1_Callbacks
{
    BrcmLbsRilAsn1_OnSendToNetwork            onRilSendToNetwork; /**< Send message to cell baseband */
    BrcmLbsRilAsn1_OnSetCertPath              onSetCertPath;      /**< Set the SUPL cert path */
} BrcmLbsRilAsn1_Callbacks;


/** Initialize the RIL/control-plane interface module inside the GPS daemon.
 *  \param callbacks pointer to the structure for RIL HAL callbacks.
 *  \param userData user-specific data that will be passed back as parameter in all callbacks
 *  \return OS_HANDLE_INVALID if failure, some other value otherwise
 */
OsHandle BrcmLbsRilAsn1_init(
        BrcmLbsRilAsn1_Callbacks* asn1Callbacks,
		BrcmLbs_UserData userData);

/** Destroy the AT-command module inside the GPS daemon.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 */
BrcmLbs_Result BrcmLbsRilAsn1_deinit(OsHandle ril);

/** Reset all assistance data prior to next positioning/measurement request.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilCntin_init() call
 *  \return BRCM_LBS_OK if assiatance data properly reset
 */
BrcmLbs_Result BrcmLbsRilCntin_CalibrationStatus(OsHandle ril, int status);

/** Callback definition for 
 *  \param userData [input] user data specified in BrcmLbsRilCntin_init() call 
 *  \return BRCM_LBS_OK if message properly sent
 */
typedef BrcmLbs_Result (*BrcmLbsRilCntin_CalibrationStart)(BrcmLbs_UserData userData);

/** Callback definition for 
 *  \param userData [input] user data specified in BrcmLbsRilCntin_init() call 
 *  \return BRCM_LBS_OK if message properly sent
 */
typedef BrcmLbs_Result (*BrcmLbsRilCntin_CalibrationEnd)(BrcmLbs_UserData userData);

typedef struct BrcmLbsRilCntin_Callbacks
{
    BrcmLbsRilCntin_CalibrationStart             onCalibrationStart;
    BrcmLbsRilCntin_CalibrationEnd               onCalibrationEnd;
} BrcmLbsRilCntin_Callbacks;

/** Initialize the RIL/control-plane interface module inside the GPS daemon.
 *  \param callbacks pointer to the structure for RIL HAL callbacks.
 *  \param userData user-specific data that will be passed back as parameter in all callbacks
 *  \return OS_HANDLE_INVALID if failure, some other value otherwise
 */
OsHandle BrcmLbsRilCntin_init(
        BrcmLbsRilCntin_Callbacks* cntinCallbacks,
		BrcmLbs_UserData userData);

/** Destroy the AT-command module inside the GPS daemon.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 */
BrcmLbs_Result BrcmLbsRilCntin_deinit(OsHandle ril);

#ifdef __cplusplus
}
#endif

#endif // _BRCM_LBS_HAL_RIL_ASN1_H
