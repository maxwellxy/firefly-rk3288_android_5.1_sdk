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
/// \file hal_os.h      Broadcom LBS API to OS
///============================================================================

#ifndef _BRCM_LBS_HAL_OS_H_
#define _BRCM_LBS_HAL_OS_H_

#if __cplusplus
extern "C" {
#endif

#include "brcm_types.h"

/********************************************************************************/
/** Connection handling
*********************************************************************************/

// Notably useful for platforms where we are able to take more control of 
// implementation because
// the platform offers rich enough (documented) APIs to take care of the network
// communication to servers

/* (Internet) Connection Manager */

/********************************************************************************/
/** Report successful completion of connection request initiated 
*   by BrcmLbsOs_OnRequestInternetConnection
*   \param os   - handle returned from BrcmLbsOs_init() call
*********************************************************************************/
void BrcmLbsOs_connectedToInternet(OsHandle os);

/********************************************************************************/
/** Inform the LBS library after connection is lost
*   \param os - handle returned from BrcmLbsOs_init() call
*********************************************************************************/
void BrcmLbsOs_disconnectedFromInternet(OsHandle os);


/********************************************************************************/
/** Request connection
*   \param userData - user data specified when calling BrcmLbsOs_init
*********************************************************************************/
typedef void (*BrcmLbsOs_OnRequestInternetConnection)(BrcmLbs_UserData userData);

/********************************************************************************/
/** It is now ok to disconnect from Internet, e.g. by closing GPRS session, 
*   disconnecting from WiFi etc.
*   \param userData - user data specified when calling BrcmLbsOs_init
*********************************************************************************/
typedef void (*BrcmLbsOs_OnCloseInternetConnection)(BrcmLbs_UserData userData);

// Not required for Aquila and friends
//// ---- Connections ----
//
//// This set of connection related functions are more encompassing, since
//// there the whole connection management and sending / receiving of data is hidden away
//// (aka "MobCom" style)
//
///********************************************************************************/
///** Report status of connection request initiated by BrcmLbsOs_OnRequestConnection
//*   \param os       - handle returned from BrcmLbsOs_init() call
//*   \param result   - result code
//*   \param ch       - This value will be provided by the LBS library as connection handle
//*                     in BrcmLbsOs_OnSendDataFromLbsToNetwork and BrcmLbsOs_OnConnectionClose
//*                     functions
//*********************************************************************************/
//void BrcmLbsOs_connected(OsHandle os, BrcmLbs_Result result, OsHandle ch);
//
///********************************************************************************/
///** Inform the LBS library after connection lost
//*   \param os   - handle returned from BrcmLbsOs_init() call
//*********************************************************************************/
//void BrcmLbsOs_disconnected(OsHandle os, OsHandle ch);
//
///********************************************************************************/
///** Provide data received from connection to the LBS library
//*   \param os       - handle returned from BrcmLbsOs_init call
//*   \param data     - address of buffer
//*   \param size     - length of the buffer
//*********************************************************************************/
//void BrcmLbsOs_sendDataFromNetworkToLbs(OsHandle os, const OsOctet* data, OsUint32 size);
//
///********************************************************************************/
///** Request connection
//*   \param connType - Connection type (LBS, SUPL)
//*   \param userData - user data specified when calling BrcmLbsOs_init
//*********************************************************************************/
//typedef void (*BrcmLbsOs_OnRequestConnection)(BrcmLbs_ConnType connType, BrcmLbs_UserData userData);
//
///********************************************************************************/
///** Send data buffer
//*   \param handle   - value provided in brcm_lbs_os_connected as "handle"
//*                     This value can be used by HAL identify connection
//*   \param data     - address of buffer to send
//*   \param size     - length of the buffer
//*   \param userData - user data specified when calling BrcmLbsOs_init
//*********************************************************************************/
//typedef void (*BrcmLbsOs_OnSendDataFromLbsToNetwork)(OsHandle ch, const OsOctet* data, OsUint32 size, BrcmLbs_UserData userData);
//
///********************************************************************************/
///** Close connection
//*   \param handle   - value provided in brcm_lbs_os_connected as "handle"
//*                     This value can be used by HAL identify connection
//*********************************************************************************/
//typedef void (*BrcmLbsOs_OnCloseConnection)(OsHandle ch, BrcmLbs_UserData userData);


/********************************************************************************/
/** Structure for OS HAL callbacks
*********************************************************************************/
typedef struct BrcmLbsOs_Callbacks
{
    /* (Internet) Connection Manager */
    BrcmLbsOs_OnRequestInternetConnection   onOsRequestInternetConnection;  /**< Request connection to internet so we can create IP session    */
                                                                            /**< to server                                                     */
    BrcmLbsOs_OnCloseInternetConnection     onOsCloseInternetConnection;    /**< Is is now ok to disconnect GPRS/WiFi, whatever was used.      */

    ///* Connections */
    //BrcmLbsOs_OnRequestConnection           onOsRequestConnection;          /**< Request connection             */
    //BrcmLbsOs_OnSendDataFromLbsToNetwork    onOsSendDataFromLbsToNetwork;   /**< Write data to the connection   */
    //BrcmLbsOs_OnCloseConnection             onOsCloseConnection;            /**< Close connection               */

} BrcmLbsOs_Callbacks;

/********************************************************************************/
/** Initialize OS callbacks
*   \param lbs      - handle returned from BrcmLbs_init() call
*   \param cb       - pointer to the structure for OS HAL callbacks.
*   \param userData - user-specific data that will be passed back as parameter 
*                     in all callbacks
*   \return         - OS_HANDLE_INVALID if failure, some other value otherwise
*********************************************************************************/
OsHandle BrcmLbsOs_init(OsHandle lbs, const BrcmLbsOs_Callbacks* cb, BrcmLbs_UserData userData);

/********************************************************************************/
/** Deinitialize LBS API
*   \param os   - handle returned from BrcmLbsOs_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsOs_deinit(OsHandle os);


#if __cplusplus
}  // extern "C"
#endif

#endif //_BRCM_LBS_HAL_OS_H_
