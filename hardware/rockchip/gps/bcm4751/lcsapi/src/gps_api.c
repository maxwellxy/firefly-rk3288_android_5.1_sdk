///============================================================================
/// Copyright 2009 Broadcom Corporation -- http://www.broadcom.com
/// This program is the proprietary software of Broadcom Corporation and/or
/// its licensors, and may only be used, duplicated, modified or distributed
/// pursuant to the terms and conditions of a separate, written license
/// agreement executed between you and Broadcom (an "Authorized License").
/// Except as set forth in an Authorized License, Broadcom grants no license
/// (express or implied), right to use, or waiver of any kind with respect to
/// the Software, and Broadcom expressly reserves all rights in and to the
/// Software and all intellectual property rights therein. IF YOU HAVE NO
/// AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
/// WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
/// THE SOFTWARE.
/// ---------------------------------------------------------------------------
/// \file gps_api.cpp     Broadcom LBS Application API
///============================================================================

#include <unistd.h>
#include "lbs.h"
#include "gps_api.h"
#include "handles.h"
#include "brcmipc_unixsocket.h"
#include <string.h>
#include <stdlib.h>

#define _SIZE(x) (sizeof(x)/sizeof(x[0]))
#define NMEA_SZ 128

#include <stdio.h>

/********************************************************************************/
/** Structure for REQUEST handle
*   Keeps a serial number to identify original request when response comes
*********************************************************************************/

typedef struct {
   BrcmLbs_OnPosition onposition_cb;
   BrcmLbs_OnFactTestResult onfacttest_cb;
   BrcmLbs_UserData userData;
} ReqHandle_t;

static ReqHandle_t reqhandle_pool[10];
static int reqhandle_inuse[_SIZE(reqhandle_pool)];


static void reqhandle_init(void)
{
    memset(reqhandle_pool,0,sizeof(reqhandle_pool));
    memset(reqhandle_inuse,0,sizeof(reqhandle_inuse));
}

static int get_reqhandle_indx(ReqHandle_t  *rh)
{
    int indx;
    indx = rh-reqhandle_pool;
    if ((indx < (int)_SIZE(reqhandle_pool)) && (indx>=0))
        return indx;
    else
        return -1;
}

static ReqHandle_t *get_reqhandle_by_indx(int indx)
{
    if ((indx < (int)_SIZE(reqhandle_pool)) && (indx>=0))
        return &reqhandle_pool[indx];
    return NULL;
}

static ReqHandle_t *get_reqhandle()
{
    int i;
    for (i=0;i<(int)_SIZE(reqhandle_pool);i++) {
        if (!reqhandle_inuse[i]) {
            reqhandle_inuse[i]=1;
            return &reqhandle_pool[i];
        }
    }
    return NULL;
}

static void put_reqhandle(ReqHandle_t *rh)
{
    int indx;
    if ((indx = get_reqhandle_indx(rh))<0)
        return;
    reqhandle_inuse[indx] = 0;
}


static int BrcmLbsGps_processFunc(OsHandle lbs, uint8_t *data, size_t datalen);
static BrcmLbs_Result tx(OsHandle gps, uint8_t *data, int32_t datalen)
{
    if(datalen<0 || data==NULL)
    return BRCM_LBS_ERROR_PARAMETER_INVALID;

    LbsHandle hlbs = (LbsHandle)((GpsHandle) gps)->lbs;
    return brcmipc_send(hlbs, data, datalen);
}


/*******************************************************************************/
/** \name API functions                                                        */
/*******************************************************************************/

/********************************************************************************/
/** Initialize GPS API
*   \param gps   - handle returned from BrcmLbs_init() call
*   \return      - OS_HANDLE_INVALID if failure, some other value otherwise
*********************************************************************************/
OsHandle BrcmLbsGps_init(OsHandle lbs)
{
    GpsHandle gps;
    if (!(gps=(GpsHandle)malloc(sizeof(struct GpsHandle_t))))
        return NULL;
    /* Initialize handle */
    gps->lbs = lbs;
    ((LbsHandle)lbs)->gpsHandle = gps;
    gps->serviceType = 0;
    gps->positioningMode = BRCM_LBS_UNSPECIFIED;
    gps->powerState = BRCM_LBS_POWER_SAVE_OFF;
    gps->nmeaMode = 0;
    gps->processFunc = BrcmLbsGps_processFunc;
    /* Register callbacks */
    gps->callbacks.onLbsPosition = NULL;
    gps->callbacks.onLbsNmea = NULL;
    gps->callbacks.onLbsStart = NULL;
    gps->callbacks.onLbsStop = NULL;
    gps->callbacks.onLbsSyncDone = NULL;
    gps->callbacks.onGpsStatus = NULL;

    reqhandle_init();
    return gps;
}

/********************************************************************************/
/** Callback registrations
*   Handle storing callback to reduce need for visibility of local structures.
*********************************************************************************/

void BrcmLbs_registerOnNmea(OsHandle gpsHandle, BrcmLbs_OnNmea cb, BrcmLbs_UserData d)
{
    ((GpsHandle) gpsHandle)->callbacks.onLbsNmea = cb;
    ((GpsHandle) gpsHandle)->callbacks.onLbsNmeaData = d;
}

void BrcmLbs_registerOnStart(OsHandle gpsHandle, BrcmLbs_OnStart cb, BrcmLbs_UserData d)
{
    ((GpsHandle) gpsHandle)->callbacks.onLbsStart = cb;
    ((GpsHandle) gpsHandle)->callbacks.onLbsStartData = d;
}

void BrcmLbs_registerOnStop(OsHandle gpsHandle, BrcmLbs_OnStop cb, BrcmLbs_UserData d)
{
    ((GpsHandle) gpsHandle)->callbacks.onLbsStop = cb;
    ((GpsHandle) gpsHandle)->callbacks.onLbsStopData = d;
}

void BrcmLbs_registerOnSyncDone(OsHandle gpsHandle, BrcmLbsClient_OnSyncDone cb, BrcmLbs_UserData d)
{
    ((GpsHandle) gpsHandle)->callbacks.onLbsSyncDone = cb;
    ((GpsHandle) gpsHandle)->callbacks.onLbsSyncDoneData = d;
}


void BrcmLbs_registerOnGpsStatus(OsHandle gpsHandle, BrcmLbs_OnGpsStatus cb, BrcmLbs_UserData d)
{
    ((GpsHandle) gpsHandle)->callbacks.onGpsStatus = cb;
    ((GpsHandle) gpsHandle)->callbacks.onGpsStatusData = d;
}

/********************************************************************************/
/** Deinitialize GPS API
*   \param gps - handle returned from BrcmLbsGps_init() call
*   \return    - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_deinit(OsHandle gps)
{
    if (!gps)
        return BRCM_LBS_ERROR_LBS_INVALID;
    free(gps);
    /* Close link to server side */
    return BRCM_LBS_OK;
}


//*****************************************************************************
/** Setup the location service functions. When the BRCM_LBS_SERVICE_OFF parameter is specified,
*   the location service is disabled. All the ongoing position requests will be terminated.
*   Incoming position requests will be rejected until another service type is set.
*
*   \param gps          - handle returned from BrcmLbsGps_init() call
*   \param  serviceType - a set of LBS services to be set. This is a bitmask of BrcmLbs_Service flags.
*   \return             - BRCM_LBS_OK if success, error - otherwise
*******************************************************************************/
BrcmLbs_Result BrcmLbsGps_serviceControl(OsHandle gps, OsUint16 serviceType)
{
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_SERVICECONTROL);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT16,serviceType,
            NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;

    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK)
        ((GpsHandle)gps)->serviceType = serviceType;
    return retval;
}

//*****************************************************************************
/** Retrieve the current location services. This is a bitmask of BrcmLbs_Service flags.
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \return     - bitmask of BrcmLbs_Service flags
*******************************************************************************/
OsUint16 BrcmLbsGps_serviceQuery(OsHandle gps)
{
    return ((GpsHandle)gps)->serviceType;
}

/********************************************************************************/
/** Set GPS mode
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param mode - GPS Positioning mode
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setMode(OsHandle gps, BrcmLbs_PositioningMode mode)
{
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_SETMODE);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,mode,
            NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK)
        ((GpsHandle)gps)->positioningMode = mode;
    return retval;
}

/********************************************************************************/
/** Set GPS NMEA output mode
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param mode - GPS NMEA mode (composite of BrcmLbs_NmeaMode bitmasks)
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setNmeaMode(OsHandle gps, OsUint32 mode)
{
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_SETMODE);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,mode,
            NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK) {
        ((GpsHandle)gps)->nmeaMode = mode;
    }
    return retval;
}

/********************************************************************************/
/** Change periodic GPS positioning power configuration.
*   \param gps          - handle returned from BrcmLbsGps_init() call
*   \param powerState   - the power state value to be configured. It can be one of the following value:
*                         BRCM_LBS_POWER_SAVE_ON Turn on power save mode
*                         BRCM_LBS_POWER_SAVE_OFF Turn off power save mode
*   \return             - BRCM_LBS_OK if success, error - otherwise
*******************************************************************************/
BrcmLbs_Result BrcmLbsGps_setPowerMode(OsHandle gps, BrcmLbs_PowerState mode)
{
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_SETPOWERMODE);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,mode,
            NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK)
        ((GpsHandle)gps)->powerState = mode;
    return retval;
}

/********************************************************************************/
/** Set GL Log mode
*   \param gps   - handle returned from BrcmLbsGps_init() call
*   \param onoff        - the Log On / Off
*                         BRCM_LBS_LOG_OFF = 0       Turn off GL Log                       
*                         BRCM_LBS_LOG_ON  = 1       Turn on  GL Log  
*   \return      - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setLogMode(OsHandle gps, BrcmLbs_Logmode onoff)
{
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_LOGENABLE);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,onoff,
            NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    return retval;
}

/********************************************************************************/
/** Set HULA mode
*   \param gps   - handle returned from BrcmLbsGps_init() call
*   \param onoff        - HULA On / Off
*                         BRCM_LBS_HULA_OFF = 0       Turn off HULA                       
*                         BRCM_LBS_HULA_ON  = 1       Turn on  HULA  
*   \return      - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setHULAMode(OsHandle gps, BrcmLbs_HULAmode onoff)
{
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_HULAENABLE);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,onoff,
            NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    return retval;
}


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
                                                   BrcmLbs_UserData userData
                                                   )
{
    ReqHandle_t *rh;
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_REQUESTSINGLELOCATION);
    rh = get_reqhandle();
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            get_reqhandle_indx(rh), NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            timeoutSec, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            accuracyMeters, NULL,0) < 0 )
    return NULL;

    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK) {
        if (rh) {
            rh->onposition_cb = cb;
            rh->userData = userData;
        }
        return (BrcmLbs_ReqHandle) rh;
    }
    else
        return NULL;
}

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
                                           BrcmLbs_UserData userData)
{
    return BrcmLbsGps_requestPeriodicLocation_ex(gps, periodMs, accuracyMeters,
            0, cb, userData);
}

/********************************************************************************/
/** Request a periodic location data from LBS
*   \param gps              - handle returned from BrcmLbsGps_init() call
*   \param periodMs         - location update time in ms.
*   \param accuracyMeters   - location accuracy in meters.
*   \param accTimeOut       - timeout for the initial accuracy
*   \param userData         - user-specific data that will be passed back as parameter 
*                             in all callbacks
*   \return                 - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbsGps_requestPeriodicLocation_ex(OsHandle gps, 
                                           OsUint32 periodMs, 
                                           OsUint32  accuracyMeters,
                                           OsUint32 accTimeOut,
                                            const BrcmLbs_OnPosition cb,
                                           BrcmLbs_UserData userData)
{
    ReqHandle_t *rh;
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_REQUESTPERIODICLOCATION);
    rh = get_reqhandle();
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            get_reqhandle_indx(rh), NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            periodMs, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            accuracyMeters, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            accTimeOut, NULL,0) < 0 )
    return NULL;
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK){
        if (rh) {
            rh->onposition_cb = cb;
            rh->userData = userData;
        }
        return (BrcmLbs_ReqHandle) rh;
    }
    else
        return NULL;
}

/********************************************************************************/
/** Stop request a location data from LBS
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param rh   - handle for the request that should be stopped
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_stopRequest(OsHandle gps, BrcmLbs_ReqHandle rh)
{
    BrcmLbs_Result result;
    uint8_t buf[1024];
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_STOPREQUEST);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            get_reqhandle_indx((ReqHandle_t *)rh), NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    result = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (result == BRCM_LBS_OK)
        put_reqhandle((ReqHandle_t *)rh);
    return result;
}

/********************************************************************************/
/** Stop all ongoing requests in LBS
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \param rh   - handle for the request that should be stopped
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_stopAllRequests(OsHandle gps)
{
    BrcmLbs_Result result;
    uint8_t buf[1024];
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_STOPALLREQUESTS);
    result = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (result == BRCM_LBS_OK)
        reqhandle_init();
    return result;
}

/********************************************************************************/
/** Delete assistance data from LBS
*   \param gps  - handle returned from BrcmLbsGps_init() call
*   \mask       - mask of AD codes(see BrcmLbs_GpsAdCode)
*   \return     - BRCM_LBS_OK if success, error - otherwise
*********************************************************************************/
BrcmLbs_Result BrcmLbsGps_deleteAidingData(OsHandle gps, OsUint16 mask)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_DELETEAIDINGDATA);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT16,mask,
            NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
}

/********************************************************************************/
/** Provide configuration info for LBS server
\param gps handle returned from BrcmLbsGps_init() call
\param path FQDN of LBS server
\param port port number
*********************************************************************************/
BrcmLbs_Result BrcmLbsClient_setServerInfo(OsHandle gps, const char* path, OsUint32 port)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSCLIENT_SETSERVERINFO);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UNDEF,0,
            (uint8_t  *)path, strlen(path)) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,port,
            NULL,0) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
}

/********************************************************************************
** Inject Position
*
*   \param pos - Ref. position
*   \return    - BRCM_LBS_OK if success, error - otherwise
********************************************************************************/
BrcmLbs_Result BrcmLbsGps_injectPosition(OsHandle gps, BrcmLbs_Position *pos)
{
    uint8_t buf[256];
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_SETREFPOSITION);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UNDEF,0,
            (uint8_t  *)pos, sizeof(*pos)) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
}

/********************************************************************************
** Inject time (coarse time)
*
*   \param gps          - Handle returned from BrcmLbsGps_init() call
*   \param utcEpochMs   - UTC mileseconds from 1970-01-01
*   \param unc          - UTC uncertainty
*   \return             - BRCM_LBS_OK if success, error - otherwise
********************************************************************************/
BrcmLbs_Result BrcmLbsGps_injectUtcTime(OsHandle gps, OsUint64 utcEpochMs,unsigned int unc)
{
    uint8_t buf[256];
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_SETREFTIME);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UNDEF,0,
            &utcEpochMs, sizeof(utcEpochMs)) < 0 )
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,unc,
                NULL,0) < 0)
    return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
}

/********************************************************************************
** LBS enable / disable
*
*   \param gps          - Handle returned from BrcmLbsGps_init() call
*   \param mode         - LBS mode enable / disable
*   \return             - BRCM_LBS_OK if success, error - otherwise
********************************************************************************/
BrcmLbs_Result BrcmLbsGps_setLbsMode(OsHandle gps, BrcmLbs_LBSmode mode)
{
    uint8_t buf[128];
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_SETLBSMODE);
    if(brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UINT32,(uint32_t)mode,
            NULL,0) < 0 )
        return BRCM_LBS_ERROR_PARAMETER_INVALID;

    return tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
}

/********************************************************************************
** Provide Wifi AP List
*
*   \param gps          - Handle returned from BrcmLbsGps_init() call
*   \param wifilist     - Wifi List for getting location
*   \return             - BRCM_LBS_OK if success, error - otherwise
********************************************************************************/
BrcmLbs_Result BrcmLbs_assistWifiList(OsHandle gps, const char* wifiList)
{
    uint8_t buf[1024];

	brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSCLIENT_SETWIFISCANLIST);

	if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UNDEF,0,
            (uint8_t  *)wifiList, strlen(wifiList)) < 0 )
		return BRCM_LBS_ERROR_PARAMETER_INVALID;

    return tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
}

/********************************************************************************/
/** Request for LBS server synchronization
*   \param gps          - handle returned from BrcmLbsGps_init() call
*   \return             - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbs_sync(OsHandle gps)
{
    ReqHandle_t *rh;
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    rh = get_reqhandle();
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBS_SYNC);
    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_INT32,1,
                NULL,0) < 0)
        return NULL;    
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK){
        return (BrcmLbs_ReqHandle) rh;
    }
    else
        return NULL;
}

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
                     BrcmLbs_FactTestItem item, int prn, int AvgIntrvlSec, int duration_secs,
                     const BrcmLbs_OnFactTestResult cb, BrcmLbs_UserData userData )
{
    ReqHandle_t *rh;
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_REQUESTFACTTEST);
    rh = get_reqhandle();
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            get_reqhandle_indx(rh), NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            (uint32_t) mode, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            (uint32_t) item , NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
             prn, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
             AvgIntrvlSec, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
             duration_secs, NULL,0) < 0 )
    return NULL;
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK){
        if (rh) {
            rh->onfacttest_cb = cb;
            rh->userData = userData;
        }
        return (BrcmLbs_ReqHandle) rh;
    }
    else
        return NULL;
}


/********************************************************************************/
/** Send request to start factory test 
*   \param gps          - handle returned from BrcmLbsGps_init() call
*   \param mode         - Factory Test mode.
*   \param item         - Factory Test item. 
*   \param prn          - prn to test 
*   \param fcn          - fcn to test
*   \param AvgIntrvlSec - Average interval 
*   \param duration_secs- test duration 
*   \param userData     - user-specific data that will be passed back as parameter 
*                         in all callbacks
*   \return             - the handle of created request
*********************************************************************************/
BrcmLbs_ReqHandle BrcmLbsClient_requestFactTestCombo(OsHandle gps, BrcmLbs_FactTestMode mode,
                     BrcmLbs_FactTestItem item, int prn, int fcn, int AvgIntrvlSec, int duration_secs,
                     const BrcmLbs_OnFactTestResult cb, BrcmLbs_UserData userData )
{
    ReqHandle_t *rh;
    uint8_t buf[1024];
    BrcmLbs_Result retval;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBSGPS_REQUESTFACTTESTCOMBO);
    rh = get_reqhandle();
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            get_reqhandle_indx(rh), NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            (uint32_t) mode, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
            (uint32_t) item , NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
             prn, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
             fcn, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
             AvgIntrvlSec, NULL,0) < 0 )
    return NULL;
    if(brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT32,
             duration_secs, NULL,0) < 0 )
    return NULL;
    retval = tx(gps, buf, brcm_marshall_get_len(buf,sizeof(buf)));
    if (retval == BRCM_LBS_OK){
        if (rh) {
            rh->onfacttest_cb = cb;
            rh->userData = userData;
        }
        return (BrcmLbs_ReqHandle) rh;
    }
    else
        return NULL;
}

/*********************************************************************************/
/*                              C A L L B A C K S                                */
/*********************************************************************************/

/********************************************************************************/
/** Execute callback for rqs: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   Callback prototype is:
*   (*BrcmLbs_OnPosition)(  BrcmLbs_ReqHandle rh, // the handle of the request
*                           const BrcmLbs_PosInfo* data, // position data
*                           BrcmLbs_UserData userData // user data specified when creating rq
*                        )
*
*   \param gps      - handle returned from BrcmLbsGps_init() call
*   \param call     - packet received with marshaled data
*********************************************************************************/
void call_OnPosition(GpsHandle gps, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int rh_indx;
    ReqHandle_t *rh;
    BrcmLbs_PosInfo posinfo;
    /* Extract serial# of rq which this responds */
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    rh_indx = brcm_marshall_get_value(arg,arg_len);
    if (!(rh=get_reqhandle_by_indx(rh_indx)))
        return;
    /* Extract position data */
    if (!rh->onposition_cb)
        return;
    if (!(arg = brcm_marshall_func_next_arg(data,datalen,arg,&arg_len)))
        return;
    if (brcm_marshall_get_payload(arg,arg_len,(uint8_t *)&posinfo,sizeof(posinfo)) < 0)
        return;
    /* Execute call */
     rh->onposition_cb((BrcmLbs_ReqHandle) rh, &posinfo, rh->userData);
}

/********************************************************************************/
/** Notify when new NMEA sentence is available.
*   Execute callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   Callback prototype is:
*   (*BrcmLbs_OnNmea)( BrcmLbs_ReqHandle rh, // the handle of the request
*                      const char* nmea, // NMEA string
*                      OsUint16 size, // num of characters in string
*                      BrcmLbs_UserData userData // user data specified when creating rq
*                     )
*
*   \param gps      - handle returned from BrcmLbsGps_init() call
*   \param call     - packet received with marshaled data
*********************************************************************************/
void call_OnNmea(GpsHandle gps, uint8_t *data, size_t datalen)
{
    int rh_indx;
    ReqHandle_t *rh;
    uint8_t *arg;
    size_t arg_len;
    uint8_t nmea[NMEA_SZ+1];
    ssize_t nmea_len;
    /* Extract serial# of rq which this responds */
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    rh_indx = brcm_marshall_get_value(arg,arg_len);
    if (!(rh=get_reqhandle_by_indx(rh_indx)))
        return;

    if (!(arg=brcm_marshall_func_next_arg(data,datalen,arg,&arg_len)))
        return;
    /* Extract NMEA message */
    if ((nmea_len = brcm_marshall_get_payload_len(arg,arg_len)) < 0)
        return;
    if (!gps->callbacks.onLbsNmea)
        return;
    if (nmea_len >= NMEA_SZ)
        return;
     if (brcm_marshall_get_payload(arg,arg_len,nmea,nmea_len) < 0)
        return;
     nmea[nmea_len++]='\0';
    /* Execute call */
    (gps->callbacks.onLbsNmea)(rh, (const char*) nmea, nmea_len, gps->callbacks.onLbsNmeaData);
}

/********************************************************************************/
/** Notify when request processing is started by LBS core.
*   Execute callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   (*BrcmLbs_OnStart)(  BrcmLbs_ReqHandle rh, // the handle of the request
*                        BrcmLbs_Result result, // BRCM_LBS_OK when started; error, otherwise
*                        BrcmLbs_UserData userData // user data specified when creating rq
*                     )
*
*   \param gps      - handle returned from BrcmLbsGps_init() call
*   \param call     - packet received with marshaled data
*********************************************************************************/
void call_OnStart(GpsHandle gps, uint8_t *data, size_t datalen)
{
    int rh_indx;
    uint8_t *arg;
    size_t arg_len;
    ReqHandle_t *rh;
    BrcmLbs_Result result;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    rh_indx = brcm_marshall_get_value(arg,arg_len);
    /* Extract serial# of rq which this responds */
    if (!(rh=get_reqhandle_by_indx(rh_indx)))
        return;
    if (!(arg=brcm_marshall_func_next_arg(data,datalen,arg,&arg_len)))
        return;
    result = (BrcmLbs_Result) brcm_marshall_get_value(arg,arg_len);
    if (gps->callbacks.onLbsStart)
        (gps->callbacks.onLbsStart)((BrcmLbs_ReqHandle) rh, result,
                gps->callbacks.onLbsStartData);
    /* Execute call */
}

/********************************************************************************/
/** Notify when request processing is completed or aborted by LBS core.
*   Execute callback for rqs: BrcmLbs_requestSingleLocation, BrcmLbs_requestPeriodicLocation
*   (*BrcmLbs_OnStop)(  BrcmLbs_ReqHandle rh, // the handle of the request
*                       BrcmLbs_UserData userData // user data specified when creating rq
*                    )
*
*   \param gps      - handle returned from BrcmLbsGps_init() call
*   \param call     - packet received with marshaled data
*********************************************************************************/
void call_OnStop(GpsHandle gps, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int rh_indx;
    ReqHandle_t *rh;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    rh_indx = brcm_marshall_get_value(arg,arg_len);
    if (!(rh=get_reqhandle_by_indx(rh_indx)))
        return;
    if (gps->callbacks.onLbsStop)
        (gps->callbacks.onLbsStop)(rh, gps->callbacks.onLbsStopData);
    put_reqhandle(rh);
}

/********************************************************************************/
/** Notify about the end of the database synchronization with LBS server.
*   Execute callback for request: BrcmLbs_Sync
*   (*BrcmLbsClient_OnSyncDone)(  BrcmLbs_Result result, // BRCM_LBS_OK if synch OK, error otherwise.
*                                 BrcmLbs_UserData userData // user data specified in BrcmLbs_sync() call
*                              )
*
*   \param gps      - handle returned from BrcmLbsGps_init() call
*   \param call     - packet received with marshaled data
*********************************************************************************/
void call_OnSyncDone(GpsHandle gps, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    BrcmLbs_Result result;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    result = (BrcmLbs_Result)  brcm_marshall_get_value(arg,arg_len);
    (gps->callbacks.onLbsSyncDone)(result, gps->callbacks.onLbsSyncDoneData);
}

/********************************************************************************/
/** Execute callback for rqs: BrcmLbs_RequestFactTest
*   Callback prototype is:
*   (*BrcmLbs_OnFactTestResult)(  BrcmLbs_ReqHandle rh, // the handle of the request
*                           const BrcmLbs_FactTestInfo* data, // The Factory Test result
*                           BrcmLbs_UserData userData // user data specified when creating rq
*                        )
*
*   \param gps      - handle returned from BrcmLbsGps_init() call
*   \param data     - packet received with marshalled data
*   \param datalen  - length of the packet received with marshalled data
*********************************************************************************/
void call_OnFactTest(GpsHandle gps, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int rh_indx;
    ReqHandle_t *rh;
    BrcmLbs_FactTestInfo ft_info;
    /* Extract serial# of rq which this responds */
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    rh_indx = brcm_marshall_get_value(arg,arg_len);
    if (!(rh=get_reqhandle_by_indx(rh_indx)))
        return;
    /* Extract position data */
    if (!rh->onfacttest_cb)
        return;
    if (!(arg = brcm_marshall_func_next_arg(data,datalen,arg,&arg_len)))
        return;
    if (brcm_marshall_get_payload(arg,arg_len,(uint8_t *)&ft_info,sizeof(ft_info)) < 0)
        return;
    /* Execute call */
     rh->onfacttest_cb((BrcmLbs_ReqHandle) rh, &ft_info, rh->userData);
}

void call_OnGpsStatus(GpsHandle gps, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    uint32_t status;

    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    status = (uint32_t) brcm_marshall_get_value(arg,arg_len);
    if (!gps->callbacks.onGpsStatus)
        return;
    gps->callbacks.onGpsStatus(status, gps->callbacks.onGpsStatusData);
}

/********************************************************************************
** Deserialize received call message and call appropriate callbacks
*
*   \param gps      - handle returned from BrcmLbsGps_init() call
*   \param rxbytes  - byte array containing received serialized call
*   \param sz       - length in bytes of the received serialized call
********************************************************************************/
static int BrcmLbsGps_processFunc(OsHandle lbs, uint8_t *data, size_t datalen)
{
    LbsHandle hlbs = (LbsHandle) lbs;
    GpsHandle hgps = (GpsHandle) hlbs->gpsHandle;

    switch (brcm_marshall_func_id(data, datalen))
    {
        case BRCMLBS_ONPOSITION:
            call_OnPosition(hgps, data, datalen);
            break;
        case BRCMLBS_ONNMEA:
            call_OnNmea(hgps, data, datalen);
            break;
        case BRCMLBS_ONSTART:
            call_OnStart(hgps, data, datalen);
            break;
        case BRCMLBS_ONSTOP:
            call_OnStop(hgps, data, datalen);
            break;
        case BRCMLBSCLIENT_ONSYNCDONE:
            call_OnSyncDone(hgps, data, datalen);
            break;
        case BRCMLBS_FACTTESTRESPONSE:
            call_OnFactTest(hgps, data, datalen);
            break;
        case BRCMLBSGPS_STATUS:
            call_OnGpsStatus(hgps, data, datalen);
            break;
        default:
            return 0;
            break;
    }
    return 1;
}
