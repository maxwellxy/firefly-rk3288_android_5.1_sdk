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
/// \file hal_wlan.h    Broadcom LBS API to WLAN(WiFi) level
///============================================================================
 
#ifndef _BRCM_LBS_HAL_WLAN_H_
#define _BRCM_LBS_HAL_WLAN_H_

#include "brcm_types.h"

#if __cplusplus
extern "C" {
#endif

/********************************************************************************/
/** WLAN AP Info
The WLAN AP Info parameter defines the parameters of a WLAN access point [IEEE 802.11v].
*********************************************************************************/
typedef struct BrcmLbsWlan_APInformation
{
    OsOctet macAddress[6];      /**< MAC Address                                */
    OsInt8  rssi;               /**< Signal strength at SET (0 - if not present)*/
    OsInt8  noise;              /**< Noice at SET (0 - if not present)          */
} BrcmLbsWlan_APInformation;

typedef struct BrcmLbsWlan_Scan
{
    OsInt8                      size;   /**< size of array  */
    BrcmLbsWlan_APInformation   ap[64]; /**< array of APs   */
} BrcmLbsWlan_Scan;

typedef struct BrcmLbsWlan_IE
{
    OsOctet     macAddress[6];  /**< MAC Address                                */
    OsInt8      size;           /**< Size of Information Elements binary        */
    OsOctet*    data;           /**< Information Elements binary                */
} BrcmLbsWlan_IE;

/** Request codes */
typedef enum BrcmLbsWlan_ReqCode
{
    LBS_WLAN_MAC,             /**< Request to get MAC address  */
    LBS_WLAN_EXT,             /**< Request to get extended WLAN source information (mac, rssi, noise)       */
    LBS_WLAN_SINGLESHOT,      /**< Request to get WLAN scan information in a single-shot                    */
    LBS_WLAN_PERIODIC,        /**< Request to get WLAN scan information periodically                        */
    LBS_WLAN_PERIODIC_STOP    /**< Stop periodic request after LBS_WLAN_PERIODIC                            */
} BrcmLbsWlan_ReqCode;

/** Notifications messages codes */
typedef enum BrcmLbsWlan_DataCode
{
    LBS_WLAN_DATA_AP,           /**< AP information     */
    LBS_WLAN_DATA_SCAN          /**< Scan information   */
} BrcmLbsWlan_DataCode;

/** Notifications data type */
typedef struct BrcmLbsWlan_NotifyData
{
    BrcmLbsWlan_DataCode        type;      /**< Data Type from cbLbsWlanDataReq.                                       */
    BrcmLbs_Result              code;      /**< Error code. LBS_DATA_END means end of the notification data.           */
    union
    {
        BrcmLbsWlan_APInformation   wlanAp;
        BrcmLbsWlan_Scan            wlanScan;   /**< WLAN information.                                                  */
    } u;
} BrcmLbsWlan_NotifyData;

/** WLAN geotag */
typedef struct BrcmLbsWlan_Geotag
{
    BrcmLbsWlan_APInformation wlanInfo;
    BrcmLbs_Location          location;
} BrcmLbsWlan_Geotag;


/********************************************************************************/
/** Request data from WLAN
*   \param dataCode       - type of the requested data.
*   \param userData       - user data specified in BrcmLbsWlan_init() call 
*********************************************************************************/
typedef void (*BrcmLbsWlan_OnReqData)(BrcmLbsWlan_ReqCode dataCode, BrcmLbs_UserData userData);


/********************************************************************************/
/** Structure for WLAN HAL callbacks
*********************************************************************************/
typedef struct BrcmLbsWlan_Callbacks
{
    BrcmLbsWlan_OnReqData            onLbsWlanDataReq;            /**< get WLAN data  */
} BrcmLbsWlan_Callbacks;

/********************************************************************************/
/** Initialize LBS interface to WLAN HAL
*   \param lbs      - handle returned from BrcmLbs_init() call
*   \param cb       - pointer to the structure for WLAN HAL callbacks.
*   \param userData - user-specific data that will be passed back as parameter 
*                     in all callbacks
*   \return         - OS_HANDLE_INVALID if failure, some other value otherwise
*********************************************************************************/
OsHandle BrcmLbsWlan_init(OsHandle lbs, const BrcmLbsWlan_Callbacks* cb, BrcmLbs_UserData userData);

/********************************************************************************/
/** Deinitialize WLAN API
*   \param wlan   - handle returned from BrcmLbsWlan_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsWlan_deinit(OsHandle wlan);

/********************************************************************************/
/** Notification with the data from WLAN. 
*   Called as the result of processing cbLbsWlanDataReq.
*   \param wlan   - handle returned from BrcmLbsWlan_init() call
*   \param data   - pointer to the data buffer.
*********************************************************************************/
void BrcmLbsWlan_notifyData(OsHandle wlan, const BrcmLbsWlan_NotifyData* data);


#if __cplusplus
}  // extern "C"
#endif

#endif //_BRCM_LBS_HAL_WLAN_H_
