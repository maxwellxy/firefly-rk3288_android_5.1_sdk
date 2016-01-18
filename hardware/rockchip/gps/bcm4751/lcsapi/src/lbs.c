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
/// \file lbs.h     Main entry for Broadcom LBS Application API for Aquila
///============================================================================

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <stdio.h>
#include <errno.h>

#define CUSTOMRILAPI 0

#include <lbs.h>
#include <gps_api.h>
#include <supl.h>
#if CUSTOMRILAPI
#else
#include "ril_api.h"
#endif
#include "handles.h"
#include "brcm_marshall.h"
#include "brcmipc_unixsocket.h"


static BrcmLbs_Result tx(OsHandle lbs, uint8_t *data, size_t datalen)
{
    LbsHandle hlbs = lbs;
    if(datalen==0 || data==NULL)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return brcmipc_send(hlbs, data, datalen);
}

/*****************************************************************************/
/** Initialize interface to GPS/LBS system overall.
*   Note that each subsystem requires a call to its corresponding init function
*   \param path (in) path for the local socket used for IPC to GPS(/LBS)
*******************************************************************************/
OsHandle BrcmLbs_init(const char *path)
{
    LbsHandle lbs;
    uint8_t buf[64];
    int fd=-1;
    struct BrcmLbs_version ver;
    BRCMLBS_MAKE_VERSION(&ver);
    
    if (!(lbs = (LbsHandle)malloc(sizeof(struct LbsHandle_t))))
        return NULL;
    memset(lbs, 0, sizeof(struct LbsHandle_t));
    /* Connect to socket for IPC */
    if ((fd = brcmipc_connect(path)) < 0) {
        goto err;
    }
    lbs->ipc_fd = fd;
    brcm_marshall_func_init(buf,sizeof(buf),BRCMLBS_INIT);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UNDEF,
                0, (uint8_t *)&ver, sizeof(ver))<0)
        goto err;
    if (tx(lbs, buf, brcm_marshall_get_len(buf,sizeof(buf))) != BRCM_LBS_OK)
        goto err;

    return lbs;
err:
    if (fd >=0)
        close(fd);
    if (lbs)
        free(lbs);
    return NULL;
}

/*****************************************************************************/
/** close and destroy all resources related to the interface
*   \param lbs   - handle returned from BrcmLbs_init() call
*******************************************************************************/
void BrcmLbs_deinit(OsHandle lbs)
{
    if (!lbs)
        return;
    close(((LbsHandle)lbs)->ipc_fd);
    free(lbs);
}

/*****************************************************************************
**  The gll messages will be handled asynchronously, we provide the file descriptor 
*   that is to be used on a poll/select loop (event loop)
* 
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \return file descriptor of the fd descriptor where the messages from the 
*           gll will be sent
*******************************************************************************/
int BrcmLbs_getIpcFileDescriptor(OsHandle lbs)
{
    return ((LbsHandle)lbs)->ipc_fd;
}

/******************************************************************************
**  The gps_api needs to be passed its own handle.
*   Pass to this function the handle returned from BrcmLbs_init(),
*   and get back the handle for BrcmLbsGps_XXX calls
* 
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \return      - handle for BrcmLbsGps_XXX calls
*******************************************************************************/
OsHandle BrcmLbs_getGpsHandle(OsHandle lbs)
{
    if (!((LbsHandle)lbs)->gpsHandle)
        ((LbsHandle)lbs)->gpsHandle = BrcmLbsGps_init(lbs);
    return ((LbsHandle)lbs)->gpsHandle;
}

/******************************************************************************
**  Get supl handle.
*   Pass to this function the handle returned from BrcmLbs_init(),
*   and get back the handle for BrcmLbsSupl_XXX calls
* 
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \return      - handle for BrcmLbsSupl_XXX calls
*******************************************************************************/
OsHandle BrcmLbs_getSuplHandle(OsHandle lbs)
{
    if (!((LbsHandle)lbs)->suplHandle)
        ((LbsHandle)lbs)->suplHandle = BrcmLbsSupl_init(lbs);
    return ((LbsHandle)lbs)->suplHandle;
}

void BrcmLbs_error(OsHandle lbs, BrcmLbs_Result res, const char *fmt, ...)
{
    LbsHandle hlbs = (LbsHandle) lbs;
    va_list ap;
    char str[256];
    if (!hlbs->onerror)
        return;
    if (!fmt) {
        hlbs->onerror(res, NULL, hlbs->onerror_arg);
        return;
    }
    va_start(ap, fmt);
    str[sizeof(str)-1]='\0';
    vsnprintf(str,sizeof(str)-1,fmt,ap);
    va_end(ap);
    hlbs->onerror(res, str, hlbs->onerror_arg);
}

static void process_serverhello(OsHandle lbs, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    struct BrcmLbs_version cli_ver;
    struct BrcmLbs_version ser_ver;
    BRCMLBS_MAKE_VERSION(&cli_ver);
    if (!(arg=brcm_marshall_func_first_arg(data, datalen, &arg_len))) {
        BrcmLbs_error(lbs, BRCM_LBS_ERROR_UNEXPECTED, "Missing argument on server hello");
        return;
    }
    if (brcm_marshall_get_payload(arg,arg_len,(uint8_t *)&ser_ver,sizeof(ser_ver)) < 0) {
        BrcmLbs_error(lbs, BRCM_LBS_ERROR_PARAMETER_INVALID, "Error getting arg1 of server hello");
        return;
    }
    if (!BRCMLBS_VERSION_IS_EQUAL(&cli_ver, &ser_ver)) {
        BrcmLbs_error(lbs, BRCM_LBS_ERROR_INVALID_VERSION,
                "Version mismatch cli:%u.%u.%u server:%u.%u.%u",
                cli_ver.major, cli_ver.minor, cli_ver.hash,
                ser_ver.major, ser_ver.minor, ser_ver.hash);
    }
}

static int proto_process(OsHandle lbs, uint8_t *data, size_t datalen)
{
    switch (brcm_marshall_func_id(data,datalen))
    {
    case BRCMLBS_SERVERHELLO:
        process_serverhello(lbs, data, datalen);
        break;
    default:
        return 0;
    }
    return 1;

}

/*****************************************************************************/
/** When we have something of the file descriptor to be read this functions will 
*   decode messages and call the right callbacks.
*   This function doesn't block
*   \param lbs   - handle returned from BrcmLbs_init() call
*******************************************************************************/
ssize_t BrcmLbs_processMessages(OsHandle lbs,brcmlbs_direction flag)
{
    LbsHandle hlbs = (LbsHandle) lbs;
    GpsHandle hgps = (GpsHandle) ((LbsHandle)lbs)->gpsHandle;
    SuplHandle hsupl = (SuplHandle) ((LbsHandle)lbs)->suplHandle;
#if CUSTOMRILAPI	
#else
    RilHandle hril = (RilHandle)((LbsHandle)lbs)->rilHandle;
#endif
    uint8_t buf[1024];
    ssize_t nread;

    if (flag == BRCMLBS_RX) {
        if ((nread = read(hlbs->ipc_fd, buf,sizeof(buf))) > 0) {
            proto_process(lbs,buf,nread); 
            if (hgps)
                hgps->processFunc(hlbs,buf,nread);
            if (hsupl)
                hsupl->processFunc(hlbs,buf,nread);
#if CUSTOMRILAPI
#else				
            if (hril)
                hril->processFunc(hlbs,buf,nread);
#endif				
        }
        return nread;
    }
    return 0;
}

void BrcmLbs_getVersion(struct BrcmLbs_version *ver)
{
    if (!ver)
        return;
    BRCMLBS_MAKE_VERSION(ver);
}

void BrcmLbs_registerOnError(OsHandle lbs, BrcmLbs_OnError errorcb, void *arg)
{
    LbsHandle hlbs=(LbsHandle *)lbs;
    hlbs->onerror=errorcb;
    hlbs->onerror_arg=arg;
}
