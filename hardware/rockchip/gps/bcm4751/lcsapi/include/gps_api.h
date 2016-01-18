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
/// \file gps_api.h     Broadcom LBS Application API
///============================================================================

#ifndef _BRCM_LBS_GPS_API_H_
#define _BRCM_LBS_GPS_API_H_

#include "brcm_types.h"
#include "lbs.h"

#ifndef LOGD
#define LOGD(...) ALOGD( __VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(...) ALOGE( __VA_ARGS__)
#endif

#ifndef LOGW
#define LOGW(...) ALOGW( __VA_ARGS__)
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
** LBS Services
*******************************************************************************/

typedef enum BrcmLbs_Service
{
    BRCM_LBS_SERVICE_OFF                = 0,        /**< No GPS services                            */
    BRCM_LBS_SERVICE_AUTONOMOUS         = 1 << 0,   /**< Autonomous service                         */
    BRCM_LBS_SERVICE_E911               = 1 << 1,   /**< E911 service                               */
    BRCM_LBS_SERVICE_LTO                = 1 << 2,   /**< LTO service                                */
    BRCM_LBS_SERVICE_SUPL_MS_BASED      = 1 << 3,   /**< SUPL service, with MS based capability     */
    BRCM_LBS_SERVICE_SUPL_MS_ASSISTED   = 1 << 4,   /**< SUPL service, with MS assisted capability  */
    BRCM_LBS_SERVICE_ECID               = 1 << 5,   /**< Enhanced CID for SUPL                      */
    BRCM_LBS_SERVICE_CELLS              = 1 << 6,   /**< Cell tower based positioning               */
    BRCM_LBS_SERVICE_WLAN               = 1 << 7,   /**< Wlan AP based positioning                  */

    /* Combinations of flags */

    /* SUPL service, with MS based and MS assisted */
    BRCM_LBS_SERVICE_SUPL               =   BRCM_LBS_SERVICE_SUPL_MS_BASED | 
                                            BRCM_LBS_SERVICE_SUPL_MS_ASSISTED,
    /* All services, without ECID.  */
    BRCM_LBS_SERVICE_ALL_NOECID         =   BRCM_LBS_SERVICE_AUTONOMOUS | 
                                            BRCM_LBS_SERVICE_E911 |
                                            BRCM_LBS_SERVICE_LTO |
                                            BRCM_LBS_SERVICE_SUPL,
    /* All services  */
    BRCM_LBS_SERVICE_ALL                =   ~(0)

} BrcmLbs_Service;


/*******************************************************************************
** NMEA output modes
*******************************************************************************/
typedef enum BrcmLbs_NmeaMode
{
    BRCM_LBS_OUTPUT_NMEA_GGA_MASK   = 1 << 0,   /**< Client will receive GGA NMEA   */
    BRCM_LBS_OUTPUT_NMEA_RMC_MASK   = 1 << 1,   /**< Client will receive RMC NMEA   */
    BRCM_LBS_OUTPUT_NMEA_GSV_MASK   = 1 << 2,   /**< Client will receive GSV NMEA   */
    BRCM_LBS_OUTPUT_NMEA_GSA_MASK   = 1 << 3,   /**< Client will receive GSA NMEA   */
    BRCM_LBS_OUTPUT_NMEA_PGLOR_MASK = 1 << 4,   /**< Client will receive PGLOR NMEA */
    BRCM_LBS_OUTPUT_NMEA_ALL_MASK   =   ~(0)
} BrcmLbs_NmeaMode;


/*******************************************************************************
** GPS positioning mode
*******************************************************************************/
typedef enum BrcmLbs_PositioningMode
{
    BRCM_LBS_UNSPECIFIED,
    BRCM_LBS_AUTONOMOUS,
    BRCM_LBS_UEBASED,
    BRCM_LBS_UEASSISTED
} BrcmLbs_PositioningMode;


/*******************************************************************************
** GPS power state
*******************************************************************************/
typedef enum BrcmLbs_PowerState
{
    BRCM_LBS_POWER_SAVE_OFF = 0, /**< Turn off power save mode */
    BRCM_LBS_POWER_SAVE_ON  = 1  /**< Turn on power save mode  */
} BrcmLbs_PowerState;


/*******************************************************************************
** GL LOG On/OFF
*******************************************************************************/
typedef enum BrcmLbs_Logmode
{
    BRCM_LBS_LOG_OFF = 0,     /**< Turn off GL Log */
    BRCM_LBS_LOG_ON  = 1      /**< Turn on GL Log  */
} BrcmLbs_Logmode;

/*******************************************************************************
** HULA On/OFF
*******************************************************************************/
typedef enum BrcmLbs_HULAmode
{
    BRCM_LBS_HULA_OFF = 0,     /**< Turn off HULA */
    BRCM_LBS_HULA_ON  = 1      /**< Turn on HULA  */
} BrcmLbs_HULAmode;

/*******************************************************************************
** LBS mode
*******************************************************************************/
typedef enum BrcmLbs_LBSmode
{
    BRCM_LBS_OFF = 0,  /**< Turn off LBS function(LTO) */
    BRCM_LBS_ON = 1    /**< Turn on LBS function(LTO)  */
} BrcmLbs_LBSmode;


typedef enum BrcmLbs_GpsStatus
{
    BRCM_LBS_GPS_ENGINE_ON = (1<<0),
    BRCM_LBS_GPS_ENGINE_OFF = (1<<1)
} BrcmLbs_GpsStatus;



/********************************************************************************/
/** ID numbers for each function in gps_api to identify marshaled calls
*********************************************************************************/
typedef enum gps_api_call_ids
{
	//CALLS
	BRCMLBS_INIT = 0,                    ///< INTERFACE
	BRCMLBSGPS_DEINIT,                      ///< INTERFACE, unused
	BRCMLBSGPS_SERVICECONTROL,              ///< INTERFACE, unused
	BRCMLBSGPS_SERVICEQUERY,                ///< INTERFACE, unused
	BRCMLBSGPS_SETMODE,                     ///< INTERFACE, unused
	BRCMLBSGPS_SETNMEAMODE,                 ///< INTERFACE, unused
	BRCMLBSGPS_SETPOWERMODE,                ///< INTERFACE
	BRCMLBSGPS_REQUESTSINGLELOCATION,       ///< INTERFACE
	BRCMLBSGPS_REQUESTPERIODICLOCATION,     ///< INTERFACE
	BRCMLBSGPS_STOPREQUEST,                 ///< INTERFACE
	BRCMLBSGPS_STOPALLREQUESTS,             ///< INTERFACE
	BRCMLBSGPS_DELETEAIDINGDATA,            ///< INTERFACE
    BRCMLBSGPS_SETREFPOSITION,              ///< INTERFACE
    BRCMLBSGPS_SETREFTIME,                  ///< INTERFACE
	BRCMLBSCLIENT_SETSERVERINFO,            ///< INTERFACE, unused
	BRCMLBS_SYNC,                           ///< INTERFACE
	BRCMLBSGPS_REQUESTFACTTEST,             ///< INTERFACE
	//CALLBACKS
	BRCMLBS_ONPOSITION,                     ///< CALLBACK
	BRCMLBS_ONNMEA,                         ///< CALLBACK
	BRCMLBS_ONSTART,                        ///< CALLBACK
	BRCMLBS_ONSTOP,                         ///< CALLBACK
	BRCMLBSCLIENT_ONSYNCDONE,               ///< CALLBACK, unused
	BRCMLBS_FACTTESTRESPONSE,               ///< CALLBACK
	//More CALLS
	BRCMLBSGPS_SETLBSMODE,                  ///< INTERFACE
    BRCMLBSGPS_STATUS,                      ///< INTERFACE, unused
    BRCMLBSGPS_LOGENABLE,                   ///< INTERFACE
    BRCMLBSCLIENT_SETWIFISCANLIST,          ///< INTERFACE
    BRCMLBS_SERVERHELLO,
    BRCMLBSRIL_SENDTONETWORK,
    BRCMLBSRIL_SENDTOGPS,
    BRCMLBSRIL_SETUESTATE,
    BRCMLBSRIL_RESETASSISTANCEDATA,
    BRCMLBSGPS_REQUESTFACTTESTCOMBO,
    BRCMLBSGPS_HULAENABLE,
    BRCMLBSRIL_CALIBRATIONSTART,
    BRCMLBSRIL_CALIBRATIONEND,
    BRCMLBSRIL_CALIBRATIONSTATUS,
    BRCMLBSRIL_SETSLPFQDN,
    BRCMLBSRIL_SETCERTPATH
} gps_api_calls;

typedef enum gps_aiding_data_ids
{
    BRCMLBSGPS_AIDING_POS = 1,
    BRCMLBSGPS_AIDING_EPH = 2,
    BRCMLBSGPS_AIDING_TIM = 4,
    BRCMLBSGPS_AIDING_ALM = 8,
    BRCMLBSGPS_AIDING_OSC = 16,
    BRCMLBSGPS_AIDING_ROM_ALM = 32,
    BRCMLBSGPS_AIDING_LTO = 64,
    BRCMLBSGPS_AIDING_IONO = 128,
    BRCMLBSGPS_AIDING_UTC = 256,
    BRCMLBSGPS_AIDING_UID = 512,
    BRCMLBSGPS_AIDING_EEIM = 1024,

} gps_aiding_data;



/*******************************************************************************
** GPS Request handle
*******************************************************************************/
typedef OsHandle BrcmLbs_ReqHandle;

/*******************************************************************************/
/** \name API functions                                                        */
/*******************************************************************************/

/********************************************************************************
* CALLBACK REGISTRATION AND PROCESSING
*********************************************************************************/

/********************************************************************************/
/** Notify callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   \param gps      - the handle of request
*   \param data     - requested position.
*   \param gps      - the handle of request
*   \param userData - user data specified when creating request
*********************************************************************************/
typedef void (*BrcmLbs_OnPosition)(BrcmLbs_ReqHandle rh, const BrcmLbs_PosInfo* data, BrcmLbs_UserData userData);


/********************************************************************************/
/** Notify when new NMEA sentence is available.
*   Callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   \param gps        - the handle of request
*   \param nmea       - NMEA string
*   \param size       - number of characters in the NMEA string
*   \param userData   - user data specified when creating request
*********************************************************************************/
typedef void (*BrcmLbs_OnNmea)(BrcmLbs_ReqHandle rh, const char* nmea, OsUint16 size, BrcmLbs_UserData userData);


/********************************************************************************/
/** Notify when request processing is started by LBS core.
*   Callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   \param rh         - the handle of request
*   \param result     - result of the start: BRCM_LBS_OK, when started. An error, otherwise
*   \param userData   - user data specified when creating request
*********************************************************************************/
typedef   void (*BrcmLbs_OnStart)(BrcmLbs_ReqHandle rh, BrcmLbs_Result result, BrcmLbs_UserData userData);


/********************************************************************************/
/** Notify when request processing is completed or aborted by LBS core.
*   Callback for requests: BrcmLbs_requestSingleLocation and
*   BrcmLbs_requestPeriodicLocation
*   \param gps        - the handle of request
*   \param userData   - user data specified creating request
*********************************************************************************/
typedef void (*BrcmLbs_OnStop)(BrcmLbs_ReqHandle rh, BrcmLbs_UserData userData);

/********************************************************************************/
/** Notify when request processing is completed or aborted by LBS core.
*   Callback for requests: BrcmLbs_requestSingleLocation and
*   BrcmLbs_requestPeriodicLocation
*   \param gps        - the handle of request
*   \param userData   - user data specified creating request
*********************************************************************************/
typedef void (*BrcmLbs_OnFactTestResult)(BrcmLbs_ReqHandle rh,const BrcmLbs_FactTestInfo *ft_info, BrcmLbs_UserData userData);

/********************************************************************************/
/** Notify about the end of the database synchronization with LBS server.
*   Callback for request: BrcmLbs_Sync
*   \param result       - BRCM_LBS_OK in case of successful synchronization,
*                         or an error otherwise.
*   \param userData     - user data specified in BrcmLbs_sync() call
*********************************************************************************/
typedef void (*BrcmLbsClient_OnSyncDone)(BrcmLbs_Result result, BrcmLbs_UserData userData);


typedef void (*BrcmLbs_OnGpsStatus) (BrcmLbs_GpsStatus status,BrcmLbs_UserData userData);
/********************************************************************************/
/** Callback registrations
*   Handle storing callback to reduce need for visibility of local structures.
*********************************************************************************/
void BrcmLbs_registerOnPosition(OsHandle gpsHandle, BrcmLbs_OnPosition, BrcmLbs_UserData d);
void BrcmLbs_registerOnNmea(OsHandle gpsHandle, BrcmLbs_OnNmea, BrcmLbs_UserData d);
void BrcmLbs_registerOnStart(OsHandle gpsHandle, BrcmLbs_OnStart, BrcmLbs_UserData d);
void BrcmLbs_registerOnStop(OsHandle gpsHandle, BrcmLbs_OnStop, BrcmLbs_UserData d);
void BrcmLbs_registerOnSyncDone(OsHandle gpsHandle, BrcmLbsClient_OnSyncDone, BrcmLbs_UserData d);

void BrcmLbs_registerOnGpsStatus(OsHandle gpsHandle, BrcmLbs_OnGpsStatus, BrcmLbs_UserData d);


/********************************************************************************/
/** Initialize GPS API
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \return      - OS_HANDLE_INVALID if failure, some other value otherwise
*********************************************************************************/
OsHandle BrcmLbsGps_init(OsHandle lbs);

/********************************************************************************/
/** Deinitialize GPS API
*   \param gps - handle returned from BrcmLbsGps_init() call
*   \return    - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_deinit(OsHandle gps);

//*****************************************************************************
/** Setup the location service functions. When the BRCM_LBS_SERVICE_OFF parameter is specified,
*   the location service is disabled. All the ongoing position requests will be terminated.
*   Incoming position requests will be rejected until another service type is set.
*
*   \param gps          - handle returned from BrcmLbsGps_init() call
*   \param  serviceType - a set of LBS services to be set. This is a bitmask of BrcmLbs_Service flags.
*   \return             - BRCM_LBS_OK if success, error - otherwise
*******************************************************************************/
BrcmLbs_Result BrcmLbsGps_serviceControl(OsHandle gps, OsUint16 serviceType);


//*****************************************************************************
/** Retrieve the current location services. This is a bitmask of BrcmLbs_Service flags.
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \return     - bitmask of BrcmLbs_Service flags
*******************************************************************************/
OsUint16 BrcmLbsGps_serviceQuery(OsHandle gps);


/********************************************************************************/
/** Set GPS mode
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param mode - GPS Positioning mode
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setMode(OsHandle gps, BrcmLbs_PositioningMode mode);

/********************************************************************************/
/** Set GL Log mode
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param onoff        - the Log On / Off
*                         BRCM_LBS_LOG_OFF = 0       Turn off GL Log                       
*                         BRCM_LBS_LOG_ON  = 1       Turn on  GL Log  
*   \return      - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
 BrcmLbs_Result BrcmLbsGps_setLogMode(OsHandle gps, BrcmLbs_Logmode onoff);

/********************************************************************************/
/** Set HULA mode
*   \param gps   - handle returned from BrcmLbsGps_init() call
*   \param onoff        - HULA On / Off
*                         BRCM_LBS_HULA_OFF = 0       Turn off HULA                       
*                         BRCM_LBS_HULA_ON  = 1       Turn on  HULA  
*   \return      - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setHULAMode(OsHandle gps, BrcmLbs_HULAmode onoff);

/********************************************************************************/
/** Set GPS NMEA output mode
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param mode - GPS NMEA mode (composite of BrcmLbs_NmeaMode bitmasks)
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setNmeaMode(OsHandle gps, OsUint32 mode);


/********************************************************************************/
/** Change periodic GPS positioning power configuration.
*   \param gps          - handle returned from BrcmLbsGps_init() call
*   \param powerState   - the power state value to be configured. It can be one of the following value:
*                         BRCM_LBS_POWER_SAVE_ON Turn on power save mode
*                         BRCM_LBS_POWER_SAVE_OFF Turn off power save mode
*   \return             - BRCM_LBS_OK if success, error - otherwise
*******************************************************************************/
BrcmLbs_Result BrcmLbsGps_setPowerMode(OsHandle gps, BrcmLbs_PowerState powerState);

/********************************************************************************/
/** Request a single location data from LBS
*   \param gps              - handle returned from BrcmLbsGps_init() call
*   \param timeoutSec       - location receive time in sec.
*   \param accuracyMeters   - location accuracy in meters.
*   \param userData         - user data specified creating request
*   \return                 - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbsGps_requestSingleLocation(OsHandle gps,
                                                   OsUint32 timeoutSec, 
                                                   OsUint32 accuracyMeters,
                                                   const BrcmLbs_OnPosition cb,
                                                   BrcmLbs_UserData userData);

/********************************************************************************/
/** Request a periodic location data from LBS
*   \param gps              - handle returned from BrcmLbsGps_init() call
*   \param periodMs         - location update time in ms.
*   \param accuracyMeters   - location accuracy in meters.
*   \param userData         - user-specific data that will be passed back as parameter
*                             in all callbacks
*   \return                 - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbsGps_requestPeriodicLocation(OsHandle gps,
                                                     OsUint32 periodMs, 
                                                     OsUint32  accuracyMeters,
                                                     const BrcmLbs_OnPosition cb,
                                                     BrcmLbs_UserData userData );


/********************************************************************************/
/** Request a periodic location data from LBS
*   \param gps              - handle returned from BrcmLbsGps_init() call
*   \param periodMs         - location update time in ms.
*   \param accuracyMeters   - location accuracy in meters.
*   \param accTimeOut       - relaxed initial accuracy for first fix.
*   \param userData         - user-specific data that will be passed back as parameter
*                             in all callbacks
*   \return                 - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbsGps_requestPeriodicLocation_ex(OsHandle gps,
                                                        OsUint32 periodMs, 
                                                        OsUint32  accuracyMeters,
                                                        OsUint32 accTimeOut,
                                                        const BrcmLbs_OnPosition cb,
                                                        BrcmLbs_UserData userData );

/********************************************************************************/
/** Stop request a location data from LBS
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param rh   - handle for the request that should be stopped
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_stopRequest(OsHandle gps, BrcmLbs_ReqHandle rh);

/********************************************************************************/
/** Stop all ongoing requests in LBS
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param rh   - handle for the request that should be stopped
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_stopAllRequests(OsHandle gps);

/********************************************************************************/
/** Delete assistance data from LBS
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \mask       - mask of AD codes(see BrcmLbs_GpsAdCode)
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_deleteAidingData(OsHandle gps, OsUint16 mask);

/********************************************************************************/
/** Provide configuration info for LBS server
\param gps handle returned from BrcmLbsGps_init() call
\param path FQDN of LBS server
\param port port number
*********************************************************************************/
BrcmLbs_Result BrcmLbsClient_setServerInfo(OsHandle gps, const char* path, OsUint32 port);

/********************************************************************************
** Inject Position
*
*   \param pos - Ref. position
*   \return    - BRCM_LBS_OK if success, error - otherwise
********************************************************************************/
BrcmLbs_Result BrcmLbsGps_injectPosition(OsHandle gps, BrcmLbs_Position *pos);

/********************************************************************************
** Inject time (coarse time)
*
*   \param gps          - Handle returned from BrcmLbsGps_init() call
*   \param utcEpochMs   - UTC mileseconds from 1970-01-01
*   \param unc          - UTC uncertainty
*   \return             - BRCM_LBS_OK if success, error - otherwise
********************************************************************************/
BrcmLbs_Result BrcmLbsGps_injectUtcTime(OsHandle gps, OsUint64 utcEpochMs, unsigned int unc);

/********************************************************************************
** LBS enable / disable
*
*   \param gps          - Handle returned from BrcmLbsGps_init() call
*   \param mode         - LBS mode enable / disable
*   \return             - BRCM_LBS_OK if success, error - otherwise
********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setLbsMode(OsHandle gps, BrcmLbs_LBSmode mode);

/********************************************************************************
** Provide Wifi AP List
*
*   \param gps          - Handle returned from BrcmLbsGps_init() call
*   \param wifilist     - Wifi List for getting location
*   \return             - BRCM_LBS_OK if success, error - otherwise
********************************************************************************/
BrcmLbs_Result BrcmLbs_assistWifiList(OsHandle gps, const char* wifiList);

/********************************************************************************/
/** Send request to start factory test 
*   \param gps          - handle returned from BrcmLbsGps_init() call
*   \param mode         - Factory Test mode.
*   \param item         - Factory Test item. 
*   \param prn          - prn to test 
*   \param AvgIntrvlSec - Average interval 
*   \param duration_secs- test duration 
*   \param userData     - user-specific data that will be passed back as parameter 
*                         in all callbacks
*   \return             - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbsClient_requestFactTest(OsHandle gps, BrcmLbs_FactTestMode mode,
                     BrcmLbs_FactTestItem, int prn, int AvgIntrvlSec, int duration_secs,
                     const BrcmLbs_OnFactTestResult cb, BrcmLbs_UserData userData );


#define BRCM_UNASSIGNED_PRN (255)
#define BRCM_UNASSIGNED_FCN (255)

/********************************************************************************/
/** Send request to start factory test 
*   \param gps          - handle returned from BrcmLbsGps_init() call
*   \param mode         - Factory Test mode.
*   \param item         - Factory Test item. 
*   \param prn          - prn to test  (BRCM_UNASSIGNED_PRN)
*   \param fcn		- fcn to test  (BRCM_UNASSIGNED_FCN)
*   \param AvgIntrvlSec - Average interval 
*   \param duration_secs- test duration 
*   \param userData     - user-specific data that will be passed back as parameter 
*                         in all callbacks
*   \return             - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbsClient_requestFactTestCombo(OsHandle gps, BrcmLbs_FactTestMode mode,
                     BrcmLbs_FactTestItem, int prn, int fcn, int AvgIntrvlSec, int duration_secs,
                     const BrcmLbs_OnFactTestResult cb, BrcmLbs_UserData userData );

/********************************************************************************/
/** Request for LBS server synchronization
*   \param gps          - handle returned from BrcmLbsGps_init() call  
*   \return             - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbs_sync(OsHandle gps);


/********************************************************************************/
/** Structure for Application layer callbacks
*********************************************************************************/
typedef struct BrcmLbs_Callbacks
{
    BrcmLbs_OnPosition     onLbsPosition;     /**< Notify about the requested position.                   */
    BrcmLbs_UserData       onLbsPositionData;
    BrcmLbs_OnNmea         onLbsNmea;         /**< Notify about the requested position in NMEA format.    */
    BrcmLbs_UserData       onLbsNmeaData;
    BrcmLbs_OnStart        onLbsStart;        /**< Notify that request processing is started by LBS core. */
    BrcmLbs_UserData       onLbsStartData;
    BrcmLbs_OnStop         onLbsStop;         /**< Notify that request processing is stopped by LBS core. */
    BrcmLbs_UserData       onLbsStopData;
    BrcmLbsClient_OnSyncDone onLbsSyncDone;   /**< Notify that request processing is stopped by LBS core. */
    BrcmLbs_UserData         onLbsSyncDoneData;

    BrcmLbs_OnGpsStatus        onGpsStatus;
    BrcmLbs_UserData        onGpsStatusData;
} BrcmLbs_Callbacks;

/********************************************************************************
** Deserialize received call message and call appropriate callbacks
*
*   \param gps      - handle returned from BrcmLbsGps_init() call
*   \param rxbytes  - byte array containing received serialized call
*   \param sz       - length in bytes of the received serialized call
********************************************************************************/
void BrcmLbsGps_processRx(OsHandle gps);



#ifdef __cplusplus
}
#endif

#endif //_BRCM_LBS_GPS_API_H_
