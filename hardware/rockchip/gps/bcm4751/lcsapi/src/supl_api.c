///============================================================================
/// Copyright 2010 Broadcom Corporation -- http://www.broadcom.com
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
/// \file supl_api.cpp     Broadcom LBS Application API
///============================================================================



#include <unistd.h>
#include "lbs.h"
#include "supl.h"
#include "handles.h"
#include "brcmipc_unixsocket.h"
#include <string.h>
#include <stdlib.h>




static inline BrcmLbs_Result TX(SuplHandle supl, uint8_t * data, size_t datalen)
{
    ssize_t sz;
    LbsHandle hlbs = (LbsHandle)((SuplHandle) supl)->lbs;
    if ((sz=brcm_marshall_get_len(data,datalen))<0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return brcmipc_send(hlbs, data, sz);
}

static void call_OnRefLocation(SuplHandle supl, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int session_id;
    if (!supl->callbacks.onSuplReqRefLocation)
        return;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    session_id = (int32_t) brcm_marshall_get_value(arg,arg_len);
    (supl->callbacks.onSuplReqRefLocation)(supl,session_id,supl->userData);
} 

static void call_OnNotifVerif(SuplHandle supl, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int session_id;
    BrcmLbsSupl_Notification notif;
    if (!supl->callbacks.onSuplReqNotifVerif)
         return;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    session_id = (int32_t) brcm_marshall_get_value(arg,arg_len);
    if (!(arg=brcm_marshall_func_next_arg(data,datalen,arg,&arg_len)))
        return;
    if (brcm_marshall_get_payload(arg,arg_len,(uint8_t *) &notif, sizeof(notif)) < 0)
        return;
    (supl->callbacks.onSuplReqNotifVerif)(supl,session_id, &notif, supl->userData);
}

static void call_OnRequestConnection(SuplHandle supl, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int session_id;
    if (!supl->callbacks.onSuplReqConnection)
        return;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    session_id = (int32_t) brcm_marshall_get_value(arg,arg_len);
    (supl->callbacks.onSuplReqConnection)(supl,session_id, supl->userData);
}

static void call_OnSessionEnded(SuplHandle supl, uint8_t *data, size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int session_id;
    if (!supl->callbacks.onSuplSessionEnded)
        return;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    session_id = (int32_t) brcm_marshall_get_value(arg,arg_len);
    (supl->callbacks.onSuplSessionEnded)(supl,session_id, supl->userData);
}

static void call_OnDnsRequest(SuplHandle hsupl,uint8_t *data ,size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int session_id;
    int domain_len;
    char domain[128];
    if (!hsupl->callbacks.onSuplDnsQuery)
        return;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    session_id = (int32_t) brcm_marshall_get_value(arg,arg_len);
    if (!(arg=brcm_marshall_func_next_arg(data,datalen,arg,&arg_len)))
        return;
    if ((domain_len = brcm_marshall_get_payload_len(arg,arg_len)) < 0)
        return;
    if (domain_len >= sizeof(domain))
        return;
    if (brcm_marshall_get_payload(arg,arg_len,(uint8_t *)domain,domain_len) < 0)
        return;
    domain[domain_len++]='\0';
    (hsupl->callbacks.onSuplDnsQuery)(hsupl,session_id,domain, domain_len,
            hsupl->userData);

}

static void call_RequestSetId(SuplHandle hsupl,uint8_t *data ,size_t datalen)
{
    uint8_t *arg;
    size_t arg_len;
    int setid_type;
    if (!hsupl->callbacks.onSuplReqSetId)
        return;
    if (!(arg=brcm_marshall_func_first_arg(data,datalen,&arg_len)))
        return;
    setid_type = (int32_t) brcm_marshall_get_value(arg,arg_len);
    (hsupl->callbacks.onSuplReqSetId)(hsupl,setid_type, hsupl->userData);
}


static int BrcmLbsSupl_processFunc(OsHandle lbs, uint8_t *data, size_t datalen)
{

    LbsHandle hlbs = (LbsHandle) lbs;
    SuplHandle hsupl = (SuplHandle) hlbs->suplHandle;
    switch (brcm_marshall_func_id(data, datalen)) 
    {
        case BRCMLBSSUPL_ONREQLOCATION:
            call_OnRefLocation(hsupl, data, datalen);
            break;
        case BRCMLBSSUPL_ONNOTIFICATION:
            call_OnNotifVerif(hsupl, data, datalen);
            break;
        case BRCMLBSSUPL_ONREQCONNECTION:
            call_OnRequestConnection(hsupl, data, datalen);    
            break;
        case BRCMLBSSUPL_ONSESSIONENDED:
            call_OnSessionEnded(hsupl, data, datalen);
            break;
        case BRCMLBSSUPL_ONDNSREQ:
            call_OnDnsRequest(hsupl, data , datalen);
            break;
        case BRCMLBSSUPL_ONREQUESTSETID:
            call_RequestSetId(hsupl, data , datalen);
            break;
        default:
            return 0;
    }
    return 1;
}

OsHandle BrcmLbsSupl_init(OsHandle lbs)
{
    SuplHandle supl;
    if (!(supl = (SuplHandle) malloc(sizeof(struct SuplHandle_t))))
        return NULL;
    supl->lbs = lbs;
    ((LbsHandle)lbs)->suplHandle = supl;
    supl->processFunc = BrcmLbsSupl_processFunc;
    return (OsHandle)supl;
}

void BrcmLbsSupl_registerCallbacks(OsHandle supl, const BrcmLbsSupl_Callbacks *callbacks,
        BrcmLbs_UserData userData)
{
    ((SuplHandle)supl)->callbacks.onSuplReqNotifVerif = callbacks->onSuplReqNotifVerif;
    ((SuplHandle)supl)->callbacks.onSuplReqRefLocation = callbacks->onSuplReqRefLocation;
    ((SuplHandle)supl)->callbacks.onSuplReqConnection = callbacks->onSuplReqConnection;
    ((SuplHandle)supl)->callbacks.onSuplSessionEnded = callbacks->onSuplSessionEnded;
    ((SuplHandle)supl)->callbacks.onSuplDnsQuery = callbacks->onSuplDnsQuery;
    ((SuplHandle)supl)->callbacks.onSuplReqSetId = callbacks->onSuplReqSetId;
    ((SuplHandle)supl)->userData = userData;
}
/********************************************************************************/
/** Deinitialize SUPL API
*   \param gps - handle returned from BrcmLbsSupl_init() call
*********************************************************************************/
BrcmLbs_Result BrcmLbsSupl_deinit(OsHandle supl)
{
    LbsHandle lbs;
    if (!supl)
        return BRCM_LBS_ERROR_LBS_INVALID; 
    lbs=((SuplHandle)supl)->lbs;
    lbs->suplHandle=NULL;
    free(supl);
    /* Close link to server side */
    return BRCM_LBS_OK;
}

BrcmLbs_Result BrcmLbsSupl_networkInitiatedRequest(OsHandle supl, const uint8_t *data,
        uint32_t size, const char *key)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSSUPL_NI);
    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)data,size) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)key, strlen(key)) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_setServerInfo(OsHandle supl, const char* path, uint32_t port)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETSERVER);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)path, strlen(path)) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UINT32, port,
                NULL,0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_setTLS(OsHandle supl, BrcmLbsSupl_ConnectionType connectiontype)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETTLS);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UINT32,(uint32_t)connectiontype,
            NULL,0) < 0 )
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
       
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_setDeviceName(OsHandle supl, const char* name)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETDEVNAME);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)name, strlen(name)) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_setCertiPath(OsHandle supl, const char* path)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETCERTIPATH);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)path, strlen(path)) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_setCapabilities(OsHandle supl, const BRCM_LBS_SETCapabilities* cap, uint32_t enableTls)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETCAPABILITIES);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)cap, sizeof(BRCM_LBS_SETCapabilities)) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UINT32,enableTls,
                NULL, 0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_setTls(OsHandle supl, uint32_t enableTls, const uint8_t* data, uint32_t size)
{
    uint8_t buf[2048];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETTLS);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UINT32,enableTls,
                NULL, 0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)data, size) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_verificationResponse(OsHandle supl,
       int supl_sessionid, uint32_t allow)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETVERIF);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,
                (uint32_t) supl_sessionid, NULL, 0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UINT32,allow,
                NULL, 0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_RefLocationResponse(OsHandle supl,
        int supl_sessionid, struct BrcmLbsSupl_LocationId *refloc)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETREFLOCATION);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_INT32,
                (uint32_t) supl_sessionid, NULL, 0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)refloc, sizeof(struct BrcmLbsSupl_LocationId)) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}


BrcmLbs_Result BrcmLbsSupl_setID(OsHandle supl,
        struct BrcmLbsSupl_SetID *setid)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_SETID);
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UNDEF,0,
                (uint8_t *)setid, sizeof(struct BrcmLbsSupl_SetID)) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_enable(OsHandle supl)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSSUPL_ENABLE);
    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_INT32,1,
                NULL,0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_disable(OsHandle supl)
{
    uint8_t buf[1024];
    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSSUPL_ENABLE);
    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_INT32,0,
                NULL,0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_connectionEstablished(OsHandle supl, int session_id)
{
    uint8_t buf[256];
    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSSUPL_CONNESTABLISHED);
    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_INT32,
                session_id, NULL,0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_resolveResponse(OsHandle supl, int session_id, uint32_t saddr)
{
    uint8_t buf[256];
    brcm_marshall_func_init(buf, sizeof(buf), BRCMLBSSUPL_RESOLVRSP); 
    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_INT32,
                session_id,NULL,0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    if (brcm_marshall_func_add_arg(buf,sizeof(buf), BRCM_MARSHALL_ARG_UINT32,
                saddr, NULL, 0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}

BrcmLbs_Result BrcmLbsSupl_requestSetId(OsHandle supl)
{
    uint8_t buf[1024]; 
    brcm_marshall_func_init(buf, sizeof(buf),BRCMLBSSUPL_REQUESTSETID);
    if (brcm_marshall_func_add_arg(buf, sizeof(buf), BRCM_MARSHALL_ARG_INT32,1,
                NULL,0) < 0)
        return BRCM_LBS_ERROR_PARAMETER_INVALID;
    return TX((SuplHandle)supl,buf,sizeof(buf));
}