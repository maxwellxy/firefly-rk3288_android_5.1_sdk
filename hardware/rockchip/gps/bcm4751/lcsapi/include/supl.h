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
/// \file supl.h    Broadcom interface to SUPL module
///============================================================================


#ifndef _BRCM_LBS_SUPL_H_
#define _BRCM_LBS_SUPL_H_

#include "brcm_types.h"
#include "lbs.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef enum supl_api_call_ids
{
	//CALLS
	BRCMLBSSUPL_NI=100,             ///< INTERFACE
    BRCMLBSSUPL_SETSERVER,          ///< INTERFACE
	BRCMLBSSUPL_SETDEVNAME,         ///< INTERFACE
	BRCMLBSSUPL_SETCAPABILITIES,    ///< INTERFACE
	BRCMLBSSUPL_SETTLS,             ///< INTERFACE
	BRCMLBSSUPL_SETVERIF,           ///< INTERFACE
	BRCMLBSSUPL_SETREFLOCATION,     ///< INTERFACE
	BRCMLBSSUPL_SETID,              ///< INTERFACE
    BRCMLBSSUPL_ENABLE,             ///< INTERFACE
    BRCMLBSSUPL_CONNESTABLISHED,    ///< INTERFACE
    BRCMLBSSUPL_RESOLVRSP,          ///< INTERFACE

	//CALLBACKS
	BRCMLBSSUPL_ONNOTIFICATION,
	BRCMLBSSUPL_ONREQLOCATION,
    BRCMLBSSUPL_ONREQCONNECTION,
    BRCMLBSSUPL_ONSESSIONENDED,
    BRCMLBSSUPL_ONDNSREQ,

	//More CALLS
	BRCMLBSSUPL_SETCERTIPATH,
    
    //More CALLS
    BRCMLBSSUPL_REQUESTSETID,
    
    //More CALLBACKS
    BRCMLBSSUPL_ONREQUESTSETID
} supl_api_calls;

/********************************************************************************/
/** Structure for SUPL HAL callbacks
*********************************************************************************/

/********************************************************************************/
/** Called by the SUPL engine after receiving SUPL INIT message
\details  
  Inform that SLP requests for Notification & Verification
@param
  notif - Describes the notification/verification mechanism to be applied.
\note     
  Application should response with glsupl_verification_rsp() only if notification types are:
  BRCM_LBS_notificationAndVerficationAllowedNA or 
  BRCM_LBS_notificationAndVerficationDeniedNA 
*********************************************************************************/
typedef void (*BrcmLbsSupl_OnReqNotifVerif)(OsHandle hd, int supl_session_id, const BrcmLbsSupl_Notification* notif, BrcmLbs_UserData userData);


/********************************************************************************/
/** Called by the SUPL when it requires a ref location
\details
   SUPL stack needs some ref. location. The answer will be passed thru a call to   to BrcmLbsSupl_RefLocationResponse

*********************************************************************************/

typedef void (*BrcmLbsSupl_OnReqRefLocation)(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData);

typedef void (*BrcmLbsSupl_OnReqConnection)(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData);

typedef void (*BrcmLbsSupl_OnSessionEnded)(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData);

typedef void (*BrcmLbsSupl_OnDnsQuery)(OsHandle supl, int session_id, char *domain, size_t domain_len, BrcmLbs_UserData userData);

typedef void (*BrcmLbsSupl_OnRequestSetId)(OsHandle supl, int setIdType, BrcmLbs_UserData userData);

typedef struct BrcmLbsSupl_Callbacks
{
    BrcmLbsSupl_OnReqNotifVerif             onSuplReqNotifVerif;
    BrcmLbsSupl_OnReqRefLocation            onSuplReqRefLocation;
    BrcmLbsSupl_OnReqConnection             onSuplReqConnection;
    BrcmLbsSupl_OnSessionEnded              onSuplSessionEnded;
    BrcmLbsSupl_OnDnsQuery                  onSuplDnsQuery;
    BrcmLbsSupl_OnRequestSetId              onSuplReqSetId;
} BrcmLbsSupl_Callbacks;


/********************************************************************************/
/** \name   SUPL API functions                                                  */  
/********************************************************************************/


/********************************************************************************/
/** Init SUPL module.
\param lbs      - handle returned from BrcmLbs_init() call
\return handle to SUPL module, if successful, or NULL if fails
*********************************************************************************/
OsHandle BrcmLbsSupl_init(OsHandle lbs);

/********************************************************************************/
/** Init SUPL module.
\param supl      - handle returned from BrcmLbsSupl_init() call
\param cb       - callbacks to register
\param userData - user-specific data that will be passed back as parameter 
                  in all callbacks
*********************************************************************************/
void BrcmLbsSupl_registerCallbacks(OsHandle supl, const BrcmLbsSupl_Callbacks* cb, BrcmLbs_UserData userData);

/********************************************************************************/
/** Deinitialize SUPL API
\param supl handle returned from BrcmLbsSupl_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_deinit(OsHandle supl);

/********************************************************************************/
/** Network Initiated request
\param supl handle returned from BrcmLbsSupl_init() call
\param data - pointer to a buffer with binary coded Network initiated message.
\param size - size of this message.
\param key  - key string for calculation HMAC of SUPL_INIT (key is SLP FQDN)
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_networkInitiatedRequest(OsHandle supl, const uint8_t* data,  uint32_t size, const char* key);

/********************************************************************************/
/** Provide configuration info for SUPL server
\param supl handle returned from BrcmLbsSupl_init() call
\param path FQDN of SUPL server
\param port port number
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_setServerInfo(OsHandle supl, const char* path, OsUint32 port);

/********************************************************************************/
/** Enable or Disable TLS connection for SUPL server
\param supl handle returned from BrcmLbsSupl_init() call
\param connectiontype TLS enable or disable 
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_setTLS(OsHandle supl, BrcmLbsSupl_ConnectionType connectiontype);

/********************************************************************************/
/** Provide configuration info of Device Name for binding socket
\param supl handle returned from BrcmLbsSupl_init() call
\param name Device Name
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_setDeviceName(OsHandle supl, const char* name);

/********************************************************************************/
/** Set Certificate file PATH for supl TLS connection 
\param supl handle returned from BrcmLbsSupl_init() call
\param path Certificate file PATH
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_setCertiPath(OsHandle supl, const char* path);

/********************************************************************************/
/** Sets capabilities of SET for SUPL protocol stack
\param supl handle returned from BrcmLbsSupl_init() call
\param cap  capabilities that set should support
\param enableTls TRUE, if TLS connection should be made, false otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_setCapabilities(OsHandle supl, const BRCM_LBS_SETCapabilities* cap, uint32_t enableTls);

/********************************************************************************/
/** Tells SUPL stack whether user approved positioning request
\param supl handle returned from BrcmLbsSupl_init() call
\param  bAllow  1 - allow positioning info,
                0 - deny
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_verificationResponse(OsHandle supl,
        int supl_session_id, uint32_t allow);

/********************************************************************************/
/** Tells SUPL stack reference location 
\param supl handle returned from BrcmLbsSupl_init() call
\param  refloc  reference location
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_RefLocationResponse(OsHandle supl,
        int supl_session_id, struct BrcmLbsSupl_LocationId *refloc);

/********************************************************************************/
/** Tells SUPL stack SET ID
\param supl handle returned from BrcmLbsSupl_init() call
\param  setid  SET ID
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_setID(OsHandle supl,
        struct BrcmLbsSupl_SetID *setid);

/********************************************************************************/
/** Tells SUPL stack connection established
\param supl handle returned from BrcmLbsSupl_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_connectionEstablished(OsHandle supl,
        int supl_session_id);

/********************************************************************************/
/** Tells SUPL stack enable SUPL
\param supl handle returned from BrcmLbsSupl_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_enable(OsHandle supl);

/********************************************************************************/
/** Tells SUPL stack disable SUPL
\param supl handle returned from BrcmLbsSupl_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_disable(OsHandle supl);

// Note that at outer layers, there may well be API that provides e.g. server FQDN, port number 
// and whether TLS should be enabled/disabled. These details the inner SUPL layer doesn't really
// care about, but they are obviously necessary to establish actual connection to server.

/********************************************************************************/
/** Tells SUPL stack resolve DNS response
\param supl handle returned from BrcmLbsSupl_init() call
\param  saddr ip address of DNS
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_resolveResponse(OsHandle supl, int session_id, uint32_t saddr);


// NTT Docomo support
/********************************************************************************/
/** Tells SUPL stack to request setID 
\param supl handle returned from BrcmLbsSupl_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_requestSetId(OsHandle supl);


#ifdef __cplusplus
}
#endif

#endif  // _BRCM_LBS_SUPL_H_
