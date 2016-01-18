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
/// \file gps_api_test.cpp     Broadcom LBS Application API test
///============================================================================
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "brcm_types.h"
#include "lbs.h"
#include "gps_api.h"
#include "supl.h"

#include <stdint.h>
static int inloop;
static void stop_thread(void);
static void *event_loop(OsHandle lbs);

static void test_OnPosition(BrcmLbs_ReqHandle rh, const BrcmLbs_PosInfo* data, BrcmLbs_UserData userData);


const uint8_t ni_msg1[] = {
        0x00,0x35,0x01,0x00,0x00,0x40,0x9F,0x43,
        0x2F,0x85,0x60,0xC5,0x32,0x80,0x28,0x17,
        0x0D,0x41,0xC7,0x05,0x49,0xC7,0x07,0x03,
        0xC7,0x03,0x55,0xC7,0x03,0x67,0xCD,0x49,
        0x39,0xCF,0x69,0x45,0xD1,0x4D,0x03,0xCD,
        0x63,0x05,0xC7,0x03,0x4F,0xC7,0x03,0x33,
        0xC7,0x01,0x04,0x00,0xC0
};

const uint8_t ni_msg2[] = {
        0x00,0x70,0x01,0x00,0x00,0x40,0x00,0x00,
        0x00,0x46,0x05,0x40,0x01,0x18,0x20,0x03,
        0xD2,0xDF,0xFF,0xE5,0x30,0x07,0x00,0x06,
        0x90,0x07,0x20,0x06,0x50,0x06,0xE0,0x07,
        0x40,0x02,0x00,0x04,0x30,0x06,0xF0,0x06,
        0xD0,0x06,0xD0,0x07,0x50,0x06,0xE0,0x06,
        0x90,0x06,0x30,0x06,0x10,0x07,0x40,0x06,
        0x90,0x06,0xF0,0x06,0xE0,0x07,0x30,0x00,
        0xB7,0xFF,0xF9,0x4C,0x01,0xC0,0x01,0xA4,
        0x01,0xC8,0x01,0x94,0x01,0xB8,0x01,0xD0,
        0x00,0x80,0x01,0x0C,0x01,0xBC,0x01,0xB4,
        0x01,0xB4,0x01,0xD4,0x01,0xB8,0x01,0xA4,
        0x01,0x8C,0x01,0x84,0x01,0xD0,0x01,0xA4,
        0x01,0xBC,0x01,0xB8,0x01,0xCC,0x00,0x00
};

const uint8_t ni_msg3[] = {
        0x00,0x21,0x01,0x00,0x00,0x40,0x00,0x00,
        0x00,0x43,0x3B,0xA0,0xA8,0x50,0x20,0x17,
        0xC8,0x89,0xDD,0xE7,0x77,0x98,0x0C,0x23,
        0xF7,0x79,0xDD,0xE6,0x37,0x18,0xC2,0xA9,
        0x80
};

// SMS: 09017C010007804180551512F20070440B804180
//      551512F27DF53090211203332B5D060504EA061C
//      6B00560100004000000046054001182017D040A6
//      0E00D20E40CA0DC0E80400860DE0DA0DA0EA0DC0
//      D20C60C20E80D20DE0DC0E601029838034839032
//      83703A01002183783683683A83703483183083A0
//      34837837039800
const uint8_t ni_high_priority[] = {
        0x00,0x56,0x01,0x00,0x00,0x40,0x00,0x00,
        0x00,0x46,0x05,0x40,0x01,0x18,0x20,0x17,
        0xD0,0x40,0xA6,0x0E,0x00,0xD2,0x0E,0x40,
        0xCA,0x0D,0xC0,0xE8,0x04,0x00,0x86,0x0D,
        0xE0,0xDA,0x0D,0xA0,0xEA,0x0D,0xC0,0xD2,
        0x0C,0x60,0xC2,0x0E,0x80,0xD2,0x0D,0xE0,
        0xDC,0x0E,0x60,0x10,0x29,0x83,0x80,0x34,
        0x83,0x90,0x32,0x83,0x70,0x3A,0x01,0x00,
        0x21,0x83,0x78,0x36,0x83,0x68,0x3A,0x83,
        0x70,0x34,0x83,0x18,0x30,0x83,0xA0,0x34,
        0x83,0x78,0x37,0x03,0x98,0x00
};


// SMS: 09017C010007804180551512F20070440B804180
//      551512F27DF53090211203332B5D060504EA061C
//      6B00560100004000000046054001182017C440A6
//      0E00D20E40CA0DC0E80400860DE0DA0DA0EA0DC0
//      D20C60C20E80D20DE0DC0E601029838034839032
//      83703A01002183783683683A83703483183083A0
//      34837837039800
const uint8_t ni_low_priority[] = {
        0x00,0x56,0x01,0x00,0x00,0x40,0x00,0x00,
        0x00,0x46,0x05,0x40,0x01,0x18,0x20,0x17,
        0xC4,0x40,0xA6,0x0E,0x00,0xD2,0x0E,0x40,
        0xCA,0x0D,0xC0,0xE8,0x04,0x00,0x86,0x0D,
        0xE0,0xDA,0x0D,0xA0,0xEA,0x0D,0xC0,0xD2,
        0x0C,0x60,0xC2,0x0E,0x80,0xD2,0x0D,0xE0,
        0xDC,0x0E,0x60,0x10,0x29,0x83,0x80,0x34,
        0x83,0x90,0x32,0x83,0x70,0x3A,0x01,0x00,
        0x21,0x83,0x78,0x36,0x83,0x68,0x3A,0x83,
        0x70,0x34,0x83,0x18,0x30,0x83,0xA0,0x34,
        0x83,0x78,0x37,0x03,0x98,0x00
};


int fix_started=0;
int fix_stopped=0;
void test_serviceControl(OsHandle gpsh)
{
	/** Setup the location service functions. When the BRCM_LBS_SERVICE_OFF parameter is specified, 
	*	the location service is disabled. All the ongoing position requests will be terminated.
	*	Incoming position requests will be rejected until another service type is set.
	*   \param gps          - handle returned from BrcmLbsGps_init() call
	*	\param  serviceType - a set of LBS services to be set. This is a bitmask of BrcmLbs_Service flags. 			
	*	\return             - BRCM_LBS_OK if success, error - otherwise
	*/
	OsUint16 mode = BRCM_LBS_SERVICE_SUPL;
	BrcmLbs_Result result = BrcmLbsGps_serviceControl(gpsh, mode);
	if (result == BRCM_LBS_OK) {
		printf("gpsapi_tst >> BrcmLbsGps_serviceControl(): TX OK\n");
	}
	else {
		printf("gpsapi_tst >> BrcmLbsGps_serviceControl(): FAIL (error in tx)\n");
	}
}

void test_setNmeaMode(OsHandle gpsh)
{
	/** Set GPS NMEA output mode
	*   \param gps  - handle returned from BrcmLbsGps_init() call
	*   \param mode - GPS NMEA mode (composite of BrcmLbs_NmeaMode bitmasks)
	*/
	OsUint32 mode = BRCM_LBS_OUTPUT_NMEA_RMC_MASK | BRCM_LBS_OUTPUT_NMEA_GSV_MASK;
	BrcmLbs_Result result = BrcmLbsGps_setNmeaMode(gpsh, mode);
	if (result == BRCM_LBS_OK) {
		printf("gpsapi_tst >> BrcmLbsGps_setNmeaMode(): TX OK\n");
	}
	else {
		printf("gpsapi_tst >> BrcmLbsGps_setNmeaMode(): FAIL (error in tx)\n");
	}
}

void test_setPowerMode(OsHandle gpsh)
{
	/**	Change periodic GPS positioning power configuration.
	*   \param gps          - handle returned from BrcmLbsGps_init() call
	*   \param powerState   - the power state value to be configured. It can be one of the following value:
	*						  BRCM_LBS_POWER_SAVE_ON Turn on power save mode
	*						  BRCM_LBS_POWER_SAVE_OFF Turn off power save mode
	*/
	BrcmLbs_PowerState mode = BRCM_LBS_POWER_SAVE_ON;
	BrcmLbs_Result result = BrcmLbsGps_setPowerMode(gpsh, mode);
	if (result == BRCM_LBS_OK) {
		printf("gpsapi_tst >> BrcmLbsGps_setPowerMode(): TX OK\n");
	}
	else {
		printf("gpsapi_tst >> BrcmLbsGps_setPowerMode(): FAIL (error in tx)\n");
	}
}

BrcmLbs_ReqHandle test_requestSingleLocation(OsHandle gpsh)
{
	/** Request a single location data from LBS
	*   \param gps              - handle returned from BrcmLbsGps_init() call
	*   \param timeoutSec       - location receive time in sec.
	*   \param accuracyMeters   - location accuracy in meters.
	*   \param userData         - user data specified creating request
	*   \return                 - the handle of created request
	*/
	OsInt32 timeoutSec = 30;
	OsInt32 accuracyMeters = 50;
	BrcmLbs_ReqHandle rqHandle = BrcmLbsGps_requestSingleLocation(gpsh, timeoutSec, accuracyMeters, test_OnPosition, NULL);
	if (rqHandle) {
		printf("gpsapi_tst >> BrcmLbsGps_requestSingleLocation(): Tx request #%i OK\n", *((int*)rqHandle));
	}
	else {
		printf("gpsapi_tst >> BrcmLbsGps_requestSingleLocation(): FAIL (error in tx)\n");
	}
        while(!fix_started)
            sleep(1);
        while(!fix_stopped)
            sleep(1);
	return rqHandle;
}

BrcmLbs_ReqHandle test_requestPeriodicLocation(OsHandle gpsh)
{
	/** Request a single location data from LBS
	*   \param gps              - handle returned from BrcmLbsGps_init() call
	*   \param timeoutSec       - location receive time in sec.
	*   \param accuracyMeters   - location accuracy in meters.
	*   \param userData         - user data specified creating request
	*   \return                 - the handle of created request
	*/
	OsInt32 periodSec = 5;
	OsInt32 accuracyMeters = 25;
	BrcmLbs_ReqHandle rqHandle = BrcmLbsGps_requestPeriodicLocation(gpsh, periodSec, accuracyMeters, test_OnPosition, (BrcmLbs_UserData)0xaa55);
	if (rqHandle) {
		printf("gpsapi_tst >> BrcmLbsGps_requestPeriodicLocation(): Tx request #%i OK\n", *((int*)rqHandle));
	}
	else {
		printf("gpsapi_tst >> BrcmLbsGps_requestPeriodicLocation(): FAIL (error in tx)\n");
	}
	return rqHandle;
}

void test_stopRequest(OsHandle gpsh, BrcmLbs_ReqHandle rqHandle)
{
	/** Stop request a location data from LBS
	*   \param gps  - handle returned from BrcmLbsGps_init() call
	*   \param rh   - handle for the request that should be stopped
	*	\return     - BRCM_LBS_OK if success, error - otherwise
	*/

	BrcmLbs_Result result = BrcmLbsGps_stopRequest(gpsh, rqHandle);
	if (result == BRCM_LBS_OK) {
		printf("gpsapi_tst >> BrcmLbsGps_stopRequest(#%i): TX OK\n", *((int*)rqHandle));
	}
	else {
		printf("gpsapi_tst >> BrcmLbsGps_stopRequest(#%i): FAIL (error in tx)\n", *((int*)rqHandle));
	}
}

void test_stopAllRequests(OsHandle gpsh)
{
	/** Stop all ongoing requests in LBS
	*   \param gps  - handle returned from BrcmLbsGps_init() call
	*   \param rh   - handle for the request that should be stopped
	*	\return     - BRCM_LBS_OK if success, error - otherwise
	*/

	BrcmLbs_Result result = BrcmLbsGps_stopAllRequests(gpsh);
	if (result == BRCM_LBS_OK) {
		printf("gpsapi_tst >> BrcmLbsGps_stopAllRequests(): TX OK\n");
	}
	else {
		printf("gpsapi_tst >> BrcmLbsGps_stopAllRequests(): FAIL (error in tx)\n");
	}
}

void test_deleteAidingData(OsHandle gpsh)
{
	/** Delete assistance data from LBS
	*   \param gps  - handle returned from BrcmLbsGps_init() call
	*   \mask       - mask of AD codes(see BrcmLbs_GpsAdCode)
	*	\return     - BRCM_LBS_OK if success, error - otherwise
	*/
	OsUint16 mode = BRCMLBSGPS_AIDING_POS|BRCMLBSGPS_AIDING_TIM;
	BrcmLbs_Result result = BrcmLbsGps_deleteAidingData(gpsh, mode);
	if (result == BRCM_LBS_OK) {
		printf("gpsapi_tst >> BrcmLbsGsp_deleteAidingData(): TX OK\n");
	}
	else {
		printf("gpsapi_tst >> BrcmLbsGps_deleteAidingData(): FAIL (error in tx)\n");
	}
}

void test_setServerInfo(OsHandle gpsh)
{
	/** Provide configuration info for LBS server
	*   \param gps handle returned from BrcmLbsGps_init() call
	*   \param path FQDN of LBS server
	*   \param port port number
	*/
	char path[] = "PATH_TO_LBS_SERVER";
	OsUint32 port = 31416;
	BrcmLbs_Result result = BrcmLbsClient_setServerInfo(gpsh, path, port);
	if (result == BRCM_LBS_OK) {
		printf("gpsapi_tst >> BrcmLbsClient_setServerInfo(): TX OK\n");
	}
	else {
		printf("gpsapi_tst >> BrcmLbsClient_setServerInfo(): FAIL (error in tx)\n");
	}
}

#if 0
void test_sync(OsHandle gpsh)
{
	/** Request for LBS server synchronization
	*   \param gps          - handle returned from BrcmLbsGps_init() call
	*   \param syncLTO	    - LTO download requested.
	*   \param syncCells    - area Cells download requested. 
	*   \return             - BRCM_LBS_OK if success, error - otherwise
	*/
	OsBool syncLTO = true;
	OsBool syncCells = false;
	BrcmLbs_Result result = BrcmLbs_sync(gpsh, syncLTO, syncCells);
	if (result == BRCM_LBS_OK) {
		printf("gpsapi_tst >> BrcmLbs_sync(): TX OK\n");
	}
	else {
		printf("gpsapi_tst >> BrcmLbs_sync(): FAIL (error in tx)\n");
	}
}
#endif

/********************************************************************************
* CALLBACK PROCESSING
*********************************************************************************/

/********************************************************************************
** Notify callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   \param rh       - handle to identify request
*   \param data     - requested position.
*   \param gps      - the handle of request
*   \param userData - user data specified when creating request
*********************************************************************************/
void test_OnPosition(BrcmLbs_ReqHandle rh, const BrcmLbs_PosInfo* data, BrcmLbs_UserData userData)
{
    if (data->location.posValid)
	fprintf(stderr,"gpsapi_tst >> OnPosition has been called with rq#0x%X ud:0x%X\nlat:%f long:%f alt:%d\n",
                (unsigned int)rh, (unsigned int)userData, data->location.latitude,data->location.longitude,data->location.altitude);
}

/********************************************************************************
** Notify callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   \param rh       - handle to identify request
*   \param data     - requested position.
*   \param gps      - the handle of request
*   \param userData - user data specified when creating request
*********************************************************************************/
void test_OnPosition2(BrcmLbs_ReqHandle rh, const BrcmLbs_PosInfo* data, BrcmLbs_UserData userData)
{
    if (data->location.posValid)
	fprintf(stderr,"gpsapi_tst >> OnPosition has been called with rq#0x%X ud:0x%X\nlat:%f long:%f alt:%d\n",
                (unsigned int)rh, (unsigned int)userData, data->location.latitude,data->location.longitude,data->location.altitude);
}

/********************************************************************************
** Notify when new NMEA sentence is available.
*   Callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   \param gps        - the handle of request
*   \param nmea       - NMEA string
*   \param size       - number of characters in the NMEA string
*   \param userData   - user data specified when creating request
*********************************************************************************/
void test_OnNmea(BrcmLbs_ReqHandle rh, const char* nmea, OsUint16 size, BrcmLbs_UserData userData)
{
            fprintf(stdout,"0x%X:%s\n",(unsigned int)rh,(char*) nmea);
}

/********************************************************************************
** Notify when request processing is started by LBS core.
*   Callback for requests: BrcmLbs_LocationSingle and BrcmLbs_LocationPeriodic
*   \param rh         - the handle of request
*   \param result     - result of the start: BRCM_LBS_OK, when started. An error, otherwise
*   \param userData   - user data specified when creating request
*********************************************************************************/
void test_OnStart(BrcmLbs_ReqHandle rh, BrcmLbs_Result result, BrcmLbs_UserData userData)
{
	printf("gpsapi_tst >> OnStart has been called with rq 0x%x\n", (unsigned int)rh);
        if (result == BRCM_LBS_OK)
		printf("... OK\n");
        fix_started=1;
        fix_stopped=0;
}

/********************************************************************************
** Notify when request processing is completed or aborted by LBS core.
*   Callback for requests: BrcmLbs_requestSingleLocation and 
*   BrcmLbs_requestPeriodicLocation
*   \param gps        - the handle of request
*   \param userData   - user data specified creating request
*********************************************************************************/
void test_OnStop(BrcmLbs_ReqHandle rh, BrcmLbs_UserData userData)
{
	printf("gpsapi_tst >> OnStop has been called with rq 0x%x\n", (unsigned int)rh);
        fix_stopped=1;
        fix_started=0;
}

/********************************************************************************/
/** Notify about the end of the database synchronization with LBS server.
*   Callback for request: BrcmLbs_Sync
*   \param result       - BRCM_LBS_OK in case of successful synchronization, 
*                         or an error otherwise.
*   \param userData     - user data specified in BrcmLbs_sync() call 
*********************************************************************************/
void test_OnSyncDone(BrcmLbs_Result result, BrcmLbs_UserData userData)
{
	printf("gpsapi_tst >> OnSyncDone has been called");
	if (result != BRCM_LBS_ERROR_OUT_OF_MEMORY) {
		printf("FAIL\n\t(error in rx : result received '%u' not as transmitted '%u')\n", result, BRCM_LBS_ERROR_OUT_OF_MEMORY);
	}
	else
		printf("... OK\n");
}

/********************************************************************************/
/** Supl stack is requesting for cell id info 
*   Callback for request: BrcmLbs_CellInfo 
*   \param supl_session_id  - The Supl Session ID we have to send info back 
*   \param userData     - user data specified in BrcmLbsSupl_init() call 
*********************************************************************************/
void test_onRequestRefLocation(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
    BrcmLbsSupl_LocationId refLoc;
    printf("gpsapi_tst >> OnRequestRefLocation has been called");
    refLoc.status = BRCM_LBS_STATUS_CURRENT;
    refLoc.cellInfo.network_type = BRCM_LBS_CELL_INFO_WCDMA;
    refLoc.cellInfo.u.wcdmaCell.m.frequencyInfoPresent = 0;
    refLoc.cellInfo.u.wcdmaCell.m.cellMeasuredResultPresent = 0;
    refLoc.cellInfo.u.wcdmaCell.refMCC = 214;
    refLoc.cellInfo.u.wcdmaCell.refMNC = 1;
    refLoc.cellInfo.u.wcdmaCell.refUC  = (128<<16)+1144;
    BrcmLbsSupl_RefLocationResponse(supl,supl_session_id,&refLoc);
}



void test_onDnsQuery(OsHandle supl, int session_id, char *domain, size_t len, BrcmLbs_UserData userData)
{
    printf("Request to resolve %.*s\n",len,domain);
    BrcmLbsSupl_resolveResponse(supl, session_id,
            ntohl(inet_addr("66.35.208.226")));
    
}
void test_onRequestConnection(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
    printf("Request for connection (requested by session n: %d)\n",
            supl_session_id);
    BrcmLbsSupl_connectionEstablished(supl,supl_session_id);
}

void test_onSuplSessionEnded(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
    printf("Supl Session ended (session id: %d)\n",
            supl_session_id);
}

/********************************************************************************/
/** Callback registrations
*********************************************************************************/
void register_callbacks(OsHandle gpsHandle)
{
	BrcmLbs_registerOnNmea(gpsHandle, test_OnNmea, NULL);
	BrcmLbs_registerOnStart(gpsHandle, test_OnStart, NULL);
	BrcmLbs_registerOnStop(gpsHandle, test_OnStop, NULL);
	BrcmLbs_registerOnSyncDone(gpsHandle, test_OnSyncDone, NULL);
}

static void test_onNotificationRef(OsHandle supl, int supl_session_id,
        BrcmLbsSupl_Notification *notif, BrcmLbs_UserData userData)
{
    char c_accept;
    fprintf(stderr,"%s called \n", __FUNCTION__);
    if (!notif)
        return;
    fprintf(stderr,"notif: %d\n",notif->notificationType);
    fprintf(stderr,"notif reqID %.*s\n",notif->requestorId.len,
                            notif->requestorId.data);
    fprintf(stderr,"notif client name %.*s\n",notif->clientName.len,
                            notif->clientName.data);
    printf("Do you accept request?\n");
    scanf("%c",&c_accept);
    if (c_accept == 'y')
        BrcmLbsSupl_verificationResponse(supl, supl_session_id, 1);
    else
        BrcmLbsSupl_verificationResponse(supl, supl_session_id, 0);
}

void do_tests(OsHandle lbsHandle)
{
	OsHandle gpsHandle = BrcmLbs_getGpsHandle(lbsHandle);
        OsHandle suplHandle = BrcmLbs_getSuplHandle(lbsHandle);
	BrcmLbs_ReqHandle rh1,rh2,rh3;
        BRCM_LBS_SETCapabilities brcm_cap;
        BrcmLbsSupl_SetID  setid;
        BrcmLbsSupl_Callbacks supl_cbs = {
            .onSuplReqNotifVerif = test_onNotificationRef,
            .onSuplReqRefLocation = test_onRequestRefLocation,
            .onSuplReqConnection = test_onRequestConnection,
            .onSuplSessionEnded = test_onSuplSessionEnded,
            .onSuplDnsQuery = test_onDnsQuery
        };
        BrcmLbs_Position pos = {
            .m = { .altitudePresent = 0,
                   .pos_errorPresent = 1
                },
            .latitude = 40.4,
            .longitude = -3.5,
            .pos_error = 100 
        };
        pthread_t thread;
        void *status;

        BrcmLbsSupl_registerCallbacks(suplHandle, &supl_cbs,NULL);
        BrcmLbsSupl_disable(suplHandle);
	register_callbacks(gpsHandle);
        if (pthread_create(&thread,NULL,event_loop,lbsHandle)!=0) {
            perror("pthread_create");
            return;
        }
        test_deleteAidingData(gpsHandle);
#if 0
	rh1 = test_requestPeriodicLocation(gpsHandle);
        sleep(100);
	rh2 = test_requestPeriodicLocation(gpsHandle);
        test_stopRequest(gpsHandle, rh2);
        sleep(100);
        test_stopRequest(gpsHandle, rh1);

	rh3 = test_requestSingleLocation(gpsHandle);
        sleep(100);
        fprintf(stderr,"stopping single location: \n");
#endif
        brcm_cap.posTechnology.agpsSETassisted=0;
        brcm_cap.posTechnology.agpsSETBased=1;
        brcm_cap.posTechnology.autonomousGPS=1;

        setid.eType = BRCM_LBS_ID_MSISDN;
        setid.u.msisdn[0]=0x41;
        setid.u.msisdn[1]=0x80;
        setid.u.msisdn[2]=0x11;
        setid.u.msisdn[3]=0x21;
        setid.u.msisdn[4]=0x22;
        setid.u.msisdn[5]=0xf2;
        setid.u.msisdn[6]=0xff;
        setid.u.msisdn[7]=0xff;
		
		// any one set from 3 choises
		BrcmLbsGps_setMode(gpsHandle, BRCM_LBS_UEBASED);
		//BrcmLbsGps_setMode(gpsHandle, BRCM_LBS_UEASSISTED);
		//BrcmLbsGps_setMode(gpsHandle, BRCM_LBS_AUTONOMOUS);
		

        BrcmLbsSupl_enable(suplHandle);
        BrcmLbsSupl_setServerInfo(suplHandle,"66.35.208.226",7275);
		BrcmLbsSupl_setTLS(suplHandle, BRCM_LBS_TLS);
		BrcmLbsSupl_setCertiPath(suplHandle, "/system/supl/certi/");
		BrcmLbsSupl_setDeviceName(suplHandle, "pdp0");
        BrcmLbsSupl_setID(suplHandle,&setid);

        fprintf(stderr, "Enabling supl\n");
        BrcmLbsGps_injectUtcTime(gpsHandle,1269389324000ULL,10);
        BrcmLbsGps_injectPosition(gpsHandle,&pos);
		BrcmLbsGps_setLbsMode(gpsHandle, BRCM_LBS_ON);

		BrcmLbs_sync(gpsHandle);
#if 0 
        rh1 = test_requestPeriodicLocation(gpsHandle);
        sleep(100);
        test_stopRequest(gpsHandle, rh1);
        sleep(1);
        BrcmLbsSupl_disable(suplHandle);
//        rh1 = test_requestSingleLocation(gpsHandle);
//        sleep(60);
#endif
#if 1 
        fprintf(stderr, "NI\n");
        BrcmLbsSupl_networkInitiatedRequest(suplHandle,ni_msg1,
                sizeof(ni_msg1),"");
        sleep(60);
#endif
#if 0 
        fprintf(stderr, "3rd\n");
        test_deleteAidingData(gpsHandle);
        rh1 = test_requestSingleLocation(gpsHandle);
//        sleep(60);
        sleep(1);
        BrcmLbsSupl_enable(suplHandle);
        rh1 = test_requestPeriodicLocation(gpsHandle);
        sleep(100);
        test_stopRequest(gpsHandle, rh1);
#endif 
        fprintf(stderr, "about to exit\n");
        stop_thread(); 
        fprintf(stderr,"stopped: \n");
        pthread_join(thread,&status);
}

static void stop_thread(void)
{
        inloop=0;
}

static void *event_loop(OsHandle lbs)
{
    int fd = BrcmLbs_getIpcFileDescriptor(lbs);
    fd_set rfds;
    int rxfd = fd;
    int nevent;
    struct timeval tv;
    inloop=1;
    while (inloop) {
        FD_ZERO(&rfds);
        FD_SET(rxfd, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if ((nevent = select(fd +1,&rfds, NULL,NULL,&tv))>0) {
            if (FD_ISSET(rxfd, &rfds))
               BrcmLbs_processMessages(lbs,BRCMLBS_RX);
        }
        else if (nevent < 0)  {
            if (errno != EINTR)
                perror("select\n");
        }
    }
    return NULL;
}

int main(int argc, char** argv)
{
	OsHandle lbsHandle = NULL;

	if (!(lbsHandle = BrcmLbs_init("/tmp/lcs.socket"))) {
		fprintf(stderr,"gpsapi_cli >> ERROR: unable to connect with gpsapi_srv\n");
		return 0;
	}

	printf("gpsapi_cli >> connected\n");
	do_tests(lbsHandle);
        return 0;
}
