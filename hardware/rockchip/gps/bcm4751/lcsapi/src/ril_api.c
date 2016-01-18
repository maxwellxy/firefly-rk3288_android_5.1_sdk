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
#include "ril_api.h"
#include "brcmipc_unixsocket.h"
#include <string.h>
#include <stdlib.h>

#define _SIZE(x) (sizeof(x)/sizeof(x[0]))

#include <stdio.h>

#define  LOG_TAG  "gps_ril_BRCM"
#include <cutils/log.h>
#define  RIL_DEBUG   0
#if RIL_DEBUG
#define  D(...)   LOGD(__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif


static int BrcmLbsRil_processFunc(OsHandle lbs, uint8_t *data, size_t datalen);


static BrcmLbs_Result tx(OsHandle ril, uint8_t *data, int32_t datalen)
{
#if RIL_DEBUG
    int i;
#endif

    if(datalen<0 || data==NULL)
    return BRCM_LBS_ERROR_PARAMETER_INVALID;

#if RIL_DEBUG
    for (i=0; i<datalen; i++)
    {
        D("%s, data[%d] = %X\n", __FUNCTION__, i, data[i]);
    }
#endif

    LbsHandle hlbs = (LbsHandle)((RilHandle) ril)->lbs;
    return brcmipc_send(hlbs, data, datalen);
}


/********************************************************************************/
/** Initialize RIL API
*   \param ril   - handle returned from BrcmLbs_init() call
*   \return      - OS_HANDLE_INVALID if failure, some other value otherwise
*********************************************************************************/
OsHandle BrcmLbsRil_init(OsHandle lbs)
{
    RilHandle ril;

    D("%s\n", __FUNCTION__);

    if (!(ril = (RilHandle)malloc(sizeof(struct RilHandle_t))))
    {
        return NULL;
    }

    // Initialize handle
    ril->lbs = lbs;
    ((LbsHandle)lbs)->rilHandle = ril;
    ril->processFunc = BrcmLbsRil_processFunc;
    ril->callbacks.onSendToNetwork = NULL;
    ril->callbacks.onSendToNetworkData = NULL;
    ril->callbacks.onCalibrationStart = NULL;
    ril->callbacks.onCalibrationStartData = NULL;
    ril->callbacks.onCalibrationEnd = NULL;
    ril->callbacks.onCalibrationEndData = NULL;
    ril->callbacks.onSetCertPath = NULL;
    ril->callbacks.onSetCertPathData = NULL;

    return ril;
}



/********************************************************************************/
/** Callback registrations
*   Handle storing callback to reduce need for visibility of local structures.
*********************************************************************************/
void BrcmLbs_registerOnSendToNetwork(OsHandle ril, BrcmLbs_OnSendToNetwork cb, BrcmLbs_UserData d)
{
    D("%s\n", __FUNCTION__);

    ((RilHandle) ril)->callbacks.onSendToNetwork = cb;
    ((RilHandle) ril)->callbacks.onSendToNetworkData = d;
}


void BrcmLbs_registerOnCalibrationStart(OsHandle ril, BrcmLbs_OnCalibrationStart cb, BrcmLbs_UserData d)
{
    D("%s\n", __FUNCTION__);

    ((RilHandle) ril)->callbacks.onCalibrationStart = cb;
    ((RilHandle) ril)->callbacks.onCalibrationStartData = d;
}


void BrcmLbs_registerOnCalibrationEnd(OsHandle ril, BrcmLbs_OnCalibrationEnd cb, BrcmLbs_UserData d)
{
    D("%s\n", __FUNCTION__);

    ((RilHandle) ril)->callbacks.onCalibrationEnd = cb;
    ((RilHandle) ril)->callbacks.onCalibrationEndData = d;
}

void BrcmLbs_registerOnSetCertPath(OsHandle ril, BrcmLbs_OnSetCertPath cb, BrcmLbs_UserData d)
{
    D("%s\n", __FUNCTION__);

    ((RilHandle) ril)->callbacks.onSetCertPath = cb;
    ((RilHandle) ril)->callbacks.onSetCertPathData = d;
}

/********************************************************************************/
/** Deinitialize LBSRIL API
*   \param ril - handle returned from BrcmLbsGps_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_deinit(OsHandle ril)
{
    D("%s\n", __FUNCTION__);

    if (!ril)
    {
        return BRCM_LBS_ERROR_LBS_INVALID;
    }

    free(ril);

    /* Close link to server side */
    return BRCM_LBS_OK;
}


/********************************************************************************/
/** Send a buffer from the RIL to the GPS
*   \param    ril - handle returned from BrcmLbsRil_init
*   \param    protocol - BrcmCpHalAsn1_Protocol
*   \param    msg - message to send
*   \param    size - size of message
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_sendToGps(OsHandle ril, 
    BrcmLbsRilAsn1_Protocol protocol,
    unsigned char* msg,
    size_t size)
{
    uint8_t buf[1024];

    D("%s\n", __FUNCTION__);

    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSRIL_SENDTOGPS);

    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UINT32, 
            (uint32_t)protocol, NULL, 0) < 0)
    {
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    }

    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UNDEF, 0,
        (uint8_t *)msg, size) < 0)
    {
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    }

    return tx(ril, buf, brcm_marshall_get_len(buf, sizeof(buf)));
}



/********************************************************************************/
/** Send a the slp fqdn from the RIL to the GPS
*   \param    fqdn - message to send
*   \param    size - size of fqdn
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_setSlpFqdn(OsHandle ril, 
    char* fqdn)
{
    uint8_t buf[1024];
    int size = strlen(fqdn);

    D("%s\n", __FUNCTION__);

    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSRIL_SETSLPFQDN);

    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UNDEF, 0,
        (uint8_t *)fqdn, size) < 0)
    {
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    }

    return tx(ril, buf, brcm_marshall_get_len(buf, sizeof(buf)));
}



/********************************************************************************/
/** Send the UeState to the GPS
*   \param    ril - handle returned from BrcmLbsRil_init 
*   \param    protocol - BrcmCpHalAsn1_Protocol
*   \param    state - BrcmLbsRilAsn1_UeState
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_setUeState(OsHandle ril, 
    BrcmLbsRilAsn1_Protocol protocol,
    BrcmLbsRilAsn1_UeState state)
{
    uint8_t buf[1024];

    D("%s\n", __FUNCTION__);

    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSRIL_SETUESTATE);

    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UINT32, 
        (uint32_t)protocol, NULL, 0) < 0)
    {
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    }

    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UINT32, 
        (uint32_t)state, NULL, 0) < 0)
    {
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    }

    return tx(ril, buf, brcm_marshall_get_len(buf, sizeof(buf)));
}



/********************************************************************************/
/** Send Reset assistance data message to GPS
*   \param    ril - handle returned from BrcmLbsRil_init  
*   \param    protocol - BrcmCpHalAsn1_Protocol
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_resetAssistanceData(OsHandle ril, 
    BrcmLbsRilAsn1_Protocol protocol)
{
    uint8_t buf[128];

    D("%s\n", __FUNCTION__);

    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSRIL_RESETASSISTANCEDATA);

    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UINT32, 
        (uint32_t)protocol, NULL, 0) < 0)
    {
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    }

    return tx(ril, buf, brcm_marshall_get_len(buf, sizeof(buf)));
}


/********************************************************************************/
/** Send calibration status to GPS
*   \param    ril - handle returned from BrcmLbsRil_init  
*   \param    status - status
*********************************************************************************/
BrcmLbs_Result BrcmLbsRil_calibrationStatus(OsHandle ril, 
    int status)
{
    uint8_t buf[128];

    D("%s\n", __FUNCTION__);

    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSRIL_CALIBRATIONSTATUS);

    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UINT32, 
        (uint32_t)status, NULL, 0) < 0)
    {
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    }

    return tx(ril, buf, brcm_marshall_get_len(buf, sizeof(buf)));
}



/*********************************************************************************/
/*                              C A L L B A C K S                                */
/*********************************************************************************/
/********************************************************************************/
/** 
*********************************************************************************/
void call_OnSendToNetwork(RilHandle ril, uint8_t *data, size_t datalen)
{
    BrcmLbsRilAsn1_Protocol protocol = BRCM_LBS_RIL_RRLP;
    static unsigned char msg[1024];
    ssize_t size = 0;
    uint8_t *arg;
    size_t arg_len;

    D("%s\n", __FUNCTION__);

    if (!ril->callbacks.onSendToNetwork)
    {
        return;
    }

    if (!(arg = brcm_marshall_func_first_arg(data, datalen, &arg_len)))
    {
        return;
    }
    protocol = (int32_t) brcm_marshall_get_value(arg, arg_len);

    if (!(arg = brcm_marshall_func_next_arg(data, datalen, arg, &arg_len)))
    {
        return;
    }

    // Extract message
    if ((size = brcm_marshall_get_payload_len(arg, arg_len)) < 0)
    {
        return;
    }

    if (size > 1024)
    {
        LOGE("GPS message to RIL larger than buffer: %ld", size);
        return;
    }

    if (brcm_marshall_get_payload(arg, arg_len, msg, size) < 0)
    {
        return;
    }

    //callback
    (ril->callbacks.onSendToNetwork)(protocol, msg, size, ril->callbacks.onSendToNetworkData);
}


void call_OnSetCertPath(RilHandle ril, uint8_t *data, size_t datalen)
{
    static char path[256];
    ssize_t path_len = 0;
    uint8_t *arg;
    size_t arg_len;

    D("%s\n", __FUNCTION__);

    if (!ril->callbacks.onSetCertPath)
    {
        return;
    }

    if (!(arg = brcm_marshall_func_first_arg(data, datalen, &arg_len)))
    {
        return;
    }

    // Extract message
    if ((path_len = brcm_marshall_get_payload_len(arg, arg_len)) < 0)
    {
        return;
    }

    if (path_len > 256)
    {
        LOGE("OnSetCertPath to RIL larger than buffer: %ld", path_len);
        return;
    }

    if (brcm_marshall_get_payload(arg, arg_len, (unsigned char*)path, path_len) < 0)
    {
        return;
    }

    // null terminate the string
    path[path_len++]='\0';

    //callback
    (ril->callbacks.onSetCertPath)(path, ril->callbacks.onSetCertPathData);
}


void call_OnCalibrationStart(RilHandle ril, uint8_t *data, size_t datalen)
{
    //callback
    if (ril->callbacks.onCalibrationStart)
    {
        (ril->callbacks.onCalibrationStart)(ril->callbacks.onCalibrationStartData);
    }
}


void call_OnCalibrationEnd(RilHandle ril, uint8_t *data, size_t datalen)
{
    //callback
    if (ril->callbacks.onCalibrationEnd)
    {
        (ril->callbacks.onCalibrationEnd)(ril->callbacks.onCalibrationEndData);
    }
}



/********************************************************************************
** Deserialize received call message and call appropriate callbacks
*
*   \param ril      - handle returned from BrcmLbsGps_init() call
*   \param data     - byte array containing received serialized call
*   \param datalen  - length in bytes of the received serialized call
********************************************************************************/
static int BrcmLbsRil_processFunc(OsHandle lbs, uint8_t *data, size_t datalen)
{
    LbsHandle hlbs = (LbsHandle) lbs;
    RilHandle hril = (RilHandle) hlbs->rilHandle;

    switch (brcm_marshall_func_id(data, datalen))
    {
        case BRCMLBSRIL_SENDTONETWORK:
            call_OnSendToNetwork(hril, data, datalen);
            break;

        case BRCMLBSRIL_CALIBRATIONSTART:
            call_OnCalibrationStart(hril, data, datalen);
            break;

        case BRCMLBSRIL_CALIBRATIONEND:
            call_OnCalibrationEnd(hril, data, datalen);
            break;

        case BRCMLBSRIL_SETCERTPATH:
            call_OnSetCertPath(hril, data, datalen);
            break;

        default:
            break;
    }

    return 0;
}
