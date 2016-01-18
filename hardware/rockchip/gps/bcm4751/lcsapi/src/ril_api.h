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
/// \file ril_api.h     Broadcom RIL Application API
///============================================================================

#ifndef _BRCM_LBS_RIL_API_H_
#define _BRCM_LBS_RIL_API_H_

#include "brcm_types.h"
#include "lbs.h"
#include "ril_asn1.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************/
/** \name API functions                                                        */
/*******************************************************************************/

/********************************************************************************
* CALLBACK REGISTRATION AND PROCESSING
*********************************************************************************/

/********************************************************************************/
/** Callback: BrcmLbs_OnSendToNetwork
*   \param protocol - protocol
*   \param msg      - message to send
*   \param size     - size of the message
*   \param userData - user data sent with BrcmLbs_registerOnSendToNetwork
*********************************************************************************/
typedef void (*BrcmLbs_OnSendToNetwork)(BrcmLbsRilAsn1_Protocol protocol, const unsigned char *msg, size_t size, BrcmLbs_UserData userData);


/********************************************************************************/
/** Callback: BrcmLbs_OnCalibrationStart
*   \param userData - user data sent with BrcmLbs_registerOnCalibrationStart
*********************************************************************************/
typedef void (*BrcmLbs_OnCalibrationStart)(BrcmLbs_UserData userData);


/********************************************************************************/
/** Callback: BrcmLbs_OnCalibrationEnd
*   \param userData - user data sent with BrcmLbs_registerOnCalibrationEnd
*********************************************************************************/
typedef void (*BrcmLbs_OnCalibrationEnd)(BrcmLbs_UserData userData);

/********************************************************************************/
/** Callback: BrcmLbs_OnSetCertPath
*   \param path     - path to the cert file
*   \param userData - user data sent with BrcmLbs_registerOnSetCertPath
*********************************************************************************/
typedef void (*BrcmLbs_OnSetCertPath)(const char *path, BrcmLbs_UserData userData);

/********************************************************************************/
/** Callback registrations
*   Handle storing callback to reduce need for visibility of local structures.
*********************************************************************************/
void BrcmLbs_registerOnSendToNetwork(OsHandle rilHandle, BrcmLbs_OnSendToNetwork cb, BrcmLbs_UserData d);
void BrcmLbs_registerOnCalibrationStart(OsHandle rilHandle, BrcmLbs_OnCalibrationStart cb, BrcmLbs_UserData d);
void BrcmLbs_registerOnCalibrationEnd(OsHandle rilHandle, BrcmLbs_OnCalibrationEnd cb, BrcmLbs_UserData d);
void BrcmLbs_registerOnSetCertPath(OsHandle rilHandle, BrcmLbs_OnSetCertPath cb, BrcmLbs_UserData d);


/********************************************************************************/
/** Structure for Application layer callbacks
*********************************************************************************/
typedef struct BrcmLbsRil_Callbacks
{
    BrcmLbs_OnSendToNetwork     onSendToNetwork;   
    BrcmLbs_UserData            onSendToNetworkData;
    BrcmLbs_OnCalibrationStart  onCalibrationStart;   
    BrcmLbs_UserData            onCalibrationStartData;
    BrcmLbs_OnCalibrationEnd    onCalibrationEnd;   
    BrcmLbs_UserData            onCalibrationEndData;
    BrcmLbs_OnSetCertPath       onSetCertPath;   
    BrcmLbs_UserData            onSetCertPathData;
} BrcmLbsRil_Callbacks;


/********************************************************************************/
/** Initialize RIL API
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \return      - OS_HANDLE_INVALID if failure, some other value otherwise
*********************************************************************************/
OsHandle BrcmLbsRil_init(OsHandle lbs);


/********************************************************************************/
/** Deinitialize RIL API
*   \param ril - handle returned from BrcmLbsGps_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_deinit(OsHandle ril);


/********************************************************************************/
/** A message is incoming from the RIL to the GPS
*   \param ril      - handle returned from BrcmLbsGps_init() call
*   \param protocol - BRCM_LBS_RIL_RRLP or BRCM_LBS_RIL_RRC
*   \param msg      - message from the ril
*   \param size     - size of the message (in characters)
*   \return         - BrcmLbs_Result
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_sendToGps(OsHandle ril,
    BrcmLbsRilAsn1_Protocol protocol,
    unsigned char* msg,
    size_t size);

    
/********************************************************************************/
/** Send a the slp fqdn from the RIL to the GPS
*   \param ril      - handle returned from BrcmLbsGps_init() call
*   \param fqdn     - fqdn to send
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_setSlpFqdn(OsHandle ril,
    char* fqdn);
    
    
/********************************************************************************/
/** Sets the UE state
*   \param ril      - handle returned from BrcmLbsGps_init() call
*   \param protocol - BRCM_LBS_RIL_RRLP or BRCM_LBS_RIL_RRC
*   \param state    - the state to set
*   \return         - BrcmLbs_Result 
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_setUeState(OsHandle ril,
    BrcmLbsRilAsn1_Protocol protocol,
    BrcmLbsRilAsn1_UeState state);


/********************************************************************************/
/** Reset assistance data
*   \param ril      - handle returned from BrcmLbsGps_init() call
*   \param protocol - BRCM_LBS_RIL_RRLP or BRCM_LBS_RIL_RRC
*   \return         - BrcmLbs_Result 
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_resetAssistanceData(OsHandle ril, 
    BrcmLbsRilAsn1_Protocol protocol);

    
/********************************************************************************/
/** Send the calibration status to the GPS
*   \param ril      - handle returned from BrcmLbsGps_init() call
*   \param status   - status
*   \return         - BrcmLbs_Result 
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_calibrationStatus(OsHandle ril, 
    int status);
    

/********************************************************************************/
/** Structure for handle
*   Keeps all the static data that is necessary to keep the link working
*********************************************************************************/
struct RilHandle_t {
    OsHandle lbs;
    BrcmLbsRil_Callbacks callbacks;
    int (*processFunc) (OsHandle lbs, uint8_t *data, size_t datalen);
};

typedef struct RilHandle_t* RilHandle;

#ifdef __cplusplus
}
#endif

#endif //_BRCM_LBS_RIL_API_H_
