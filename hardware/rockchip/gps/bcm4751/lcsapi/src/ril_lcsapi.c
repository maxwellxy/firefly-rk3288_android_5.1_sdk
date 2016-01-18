//==============================================================================
// Copyright 2011 Broadcom Corporation -- http://www.broadcom.com
// This program is the proprietary software of Broadcom Corporation and/or
// its licensors, and may only be used, duplicated, modified or distributed
// pursuant to the terms and conditions of a separate, written license
// agreement executed between you and Broadcom (an "Authorized License").
// Except as set forth in an Authorized License, Broadcom grants no license
// (express or implied), right to use, or waiver of any kind with respect to
// the Software, and Broadcom expressly reserves all rights in and to the
// Software and all intellectual property rights therein. IF YOU HAVE NO
// AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
// WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
// THE SOFTWARE.
//------------------------------------------------------------------------------
/// \file ril_asn1.c
//==============================================================================

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "ril_asn1.h"
#define  LOG_TAG  "gps_ril_BRCM"
#include <cutils/log.h>
#include <cutils/sockets.h>
#include <lbs.h>
#include "ril_api.h"


#define  RIL_DEBUG   1
#if RIL_DEBUG
#define  D(...)   LOGD(__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif


#ifdef ANDROID_23
#define BRCM_LOCAL_PATH ANDROID_SOCKET_DIR"/rilgps.socket"
#else
#define BRCM_LOCAL_PATH "/data/rilgps.socket"
#endif

#define _SIZE(x) (sizeof(x)/sizeof(x[0]))
#define _MIN(a,b) (a>b?b:a)

/// The global LCS socket connection trial number.
static int iLcsConnectTrial = 0;

// The state of our connection to the gpsd daemon 
typedef struct
{
    int init;
    BrcmLbsRilAsn1_Callbacks rilCallbacks;
    
    BrcmLbsRilCntin_Callbacks cntinCallbacks;

    // user data from BrcmLbsRilAsn1_init
    BrcmLbs_UserData userData;

    // user data from BrcmLbsRilCntin_init
	BrcmLbs_UserData cntinUserData;

    // Control file descriptors
    int control[2];

    // File descriptor for LCS socket
    int fd;

    // Handler for LCS socket GPS driver.
    OsHandle lbs;

    // socket path
    char *local_path;
    
    // thread handle
    pthread_t thread;

    // ril handle
    RilHandle ril;

	// flag to track cntin interface initialization
	int cntinInitialized;

	// flag to track asn1 interface initialization
	int asn1Initialized;

} RilState;


// Static global instance of the RilState
static RilState  _ril_state[1] =
{
    {
        .init =  0
    }

};


// Commands sent to the ril thread 
typedef enum
{
    CMD_SEND_TO_GPS = 0,
    CMD_SET_UE_STATE,
    CMD_RESET_ASSISTANCE_DATA,
    CMD_DEINIT,
    CMD_CALIBRATION_STATUS,
    CMD_SET_SLP_FQDN
} cmd_ids;



struct ril_cmds
{
    cmd_ids id;
    union
    {
        struct
        {
            BrcmLbsRilAsn1_Protocol protocol;
            unsigned char* msg;
            size_t size;
        } sendToGps;
        struct
        {
            BrcmLbsRilAsn1_Protocol protocol;
            BrcmLbsRilAsn1_UeState state;
        } setUeState;
        struct
        {
            BrcmLbsRilAsn1_Protocol protocol;
        } resetAssistanceData;
        struct
        {
            int status;
        } calibrationStatus;
        struct
        {
            char* fqdn;
        } setSlpFqdn;

    } u;
};


// Forward declarations
static void  ril_state_init(RilState* rilState);
static void* ril_state_thread(void* arg);
static void  ril_state_done(RilState* rilState);
static int   ril_thread_init(RilState *rilState);
static int   ril_lcsapi_init(RilState* rilState);
static void  connect_init_cb(int id, void *arg);
static void  on_SendToNetwork(BrcmLbsRilAsn1_Protocol protocol, const unsigned char* msg, size_t size, BrcmLbs_UserData userData);
static void  on_CalibrationStart(BrcmLbs_UserData userData);
static void  on_CalibrationEnd(BrcmLbs_UserData userData);
static void  on_SetCertPath(const char* path, BrcmLbs_UserData userData);



static int send_cmd(RilState *rilState, struct ril_cmds *cmd)
{
    ssize_t nw;

    if (!rilState)
    {
        return -1;
    }

    if (!cmd)
    {
        return -1;
    }

    if ((nw = write(rilState->control[0], cmd, sizeof(*cmd))) < 0)
    {
        return -1;
    }

    return 0;
}



/** Send an message(AT-command) to the GPS daemon.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \param protocol [input] protocol type of encoded message
 *  \param msg [input] ASN.1 encoded message
 *  \param size [input]  The size of message
 *  \return BRCM_LBS_OK if message properly sent
 */
BrcmLbs_Result BrcmLbsRilAsn1_sendToGps(
    OsHandle ril, 
    BrcmLbsRilAsn1_Protocol protocol, 
    const unsigned char *msg, 
    size_t size)
{
    RilState *rilState = _ril_state;
    struct ril_cmds cmd = {.id = CMD_SEND_TO_GPS};
    static unsigned char buf[1024];

    D("%s, protocol=%d, size=%d", __FUNCTION__, protocol, size);

    if (rilState->fd < 0)
    {
        D("%s called before LCS is ready, ignoring", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    if (!rilState->init)
    {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    if (size > 1024)
    {
        D("%s: unexpected message size: %d", __FUNCTION__, size);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    memcpy(buf, msg, size);
    cmd.u.sendToGps.protocol = protocol;
    cmd.u.sendToGps.msg = (unsigned char*)buf; 
    cmd.u.sendToGps.size = size;

    return (send_cmd(rilState, &cmd) ? BRCM_LBS_ERROR_UNEXPECTED : BRCM_LBS_OK);
}


/** Sends the SLP FQDN from the SIM card to the GPS daemon
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \param fqdn [input] fqdn 
 *  \return BRCM_LBS_OK if message properly sent
 */
BrcmLbs_Result BrcmLbsRilAsn1_setSlpFqdn(
    OsHandle ril, 
    const char *fqdn)
{
    RilState *rilState = _ril_state;
    struct ril_cmds cmd = {.id = CMD_SET_SLP_FQDN};
    static unsigned char buf[1024];
    int size = strlen(fqdn);

    D("%s, fqdn=%s", __FUNCTION__, fqdn);

    if (rilState->fd < 0)
    {
        D("%s called before LCS is ready, ignoring", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    if (!rilState->init)
    {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    if (size > 1024)
    {
        D("%s: unexpected message size: %d", __FUNCTION__, size);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    memcpy(buf, fqdn, size);
    cmd.u.setSlpFqdn.fqdn = (char*)buf; 

    return (send_cmd(rilState, &cmd) ? BRCM_LBS_ERROR_UNEXPECTED : BRCM_LBS_OK);
}



/** Sets the current UE State.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \param protocol [input] protocol type of encoded message
 *  \param state [input] (RRC) UE State
 *  \return BRCM_LBS_OK if state properly set
 *  \note Currently the protocol value has to be set to BRCM_LBS_RIL_RRC, this function is of no use for RRLP
 */
BrcmLbs_Result BrcmLbsRilAsn1_setUeState(
    OsHandle ril, 
    BrcmLbsRilAsn1_Protocol protocol, 
    BrcmLbsRilAsn1_UeState state)
{
    RilState *rilState = _ril_state;
    struct ril_cmds cmd = {.id = CMD_SET_UE_STATE};

    D("%s, protocol=%d, state=%d", __FUNCTION__, protocol, state);

    if (rilState->fd < 0)
    {
        D("%s called before LCS is ready, ignoring", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }
    
    if (!rilState->init)
    {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    cmd.u.setUeState.protocol = protocol;
    cmd.u.setUeState.state = state; 

    return (send_cmd(rilState, &cmd) ? BRCM_LBS_ERROR_UNEXPECTED : BRCM_LBS_OK);
}



/** Reset all assistance data prior to next positioning/measurement request.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \param protocol [input] RRLP or RRC
 *  \return BRCM_LBS_OK if assiatance data properly reset
 */
BrcmLbs_Result BrcmLbsRilAsn1_resetAssistanceData(OsHandle ril, BrcmLbsRilAsn1_Protocol protocol)
{
    RilState *rilState = _ril_state;
    struct ril_cmds cmd = {.id = CMD_RESET_ASSISTANCE_DATA};
    
    D("%s", __FUNCTION__);

    if (rilState->fd < 0)
    {
        D("%s called before LCS is ready, ignoring", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    if (!rilState->init)
    {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    cmd.u.resetAssistanceData.protocol = protocol;

    return (send_cmd(rilState, &cmd) ? BRCM_LBS_ERROR_UNEXPECTED : BRCM_LBS_OK);
}



/** 
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 *  \param status [input] 
 *  \return BRCM_LBS_OK if message properly sent
 */
BrcmLbs_Result BrcmLbsRilCntin_CalibrationStatus(OsHandle ril, int status)
{
    RilState *rilState = _ril_state;
    struct ril_cmds cmd = {.id = CMD_CALIBRATION_STATUS};
    
    D("%s", __FUNCTION__);

    if (rilState->fd < 0)
    {
        D("%s called before LCS is ready, ignoring", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    if (!rilState->init)
    {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    cmd.u.calibrationStatus.status = status;

    return (send_cmd(rilState, &cmd) ? BRCM_LBS_ERROR_UNEXPECTED : BRCM_LBS_OK);
}



BrcmLbs_Result StartIPC()
{
    RilState *rilState = _ril_state;

    ril_state_init(rilState);

    if (ril_thread_init(rilState))
    {
        LOGE("%s, ril_thread_init failed", __FUNCTION__);
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

	return BRCM_LBS_OK;
}



BrcmLbs_Result StopIPC()
{
    RilState *rilState = _ril_state;
    void*  dummy;
    struct ril_cmds cmd = {.id = CMD_DEINIT};

    D("%s", __FUNCTION__);

    if (rilState->control[0] < 0)
    {
        return BRCM_LBS_ERROR_UNEXPECTED;
    }

    send_cmd(rilState, &cmd);
    pthread_join(rilState->thread, &dummy);
    D("thread exit:%p", dummy);

    // close the control socket pair
    close(rilState->control[0]);
    rilState->control[0] = -1;
    close(rilState->control[1]);
    rilState->control[1] = -1;

    rilState->thread = (pthread_t)NULL;
    rilState->init = 0;

    return BRCM_LBS_OK;
}


/** Initialize the RIL/control-plane interface module inside the GPS daemon.
 *  \param callbacks pointer to the structure for RIL HAL callbacks.
 *  \param userData user-specific data that will be passed back as parameter in all callbacks
 *  \return OS_HANDLE_INVALID if failure, some other value otherwise
 */
OsHandle BrcmLbsRilAsn1_init(
    BrcmLbsRilAsn1_Callbacks* asn1Callbacks,
	BrcmLbs_UserData userData)
{
	RilState *rilState = _ril_state;

	D("%s", __FUNCTION__);

    rilState->rilCallbacks = *asn1Callbacks;
    rilState->userData = userData;

	if (!rilState->cntinInitialized)
	{
		if (BRCM_LBS_OK != StartIPC())
		{
			return OS_HANDLE_INVALID;
		}
	}

	rilState->asn1Initialized = 1;

    return (OsHandle)rilState;
}


/** Destroy the AT-command module inside the GPS daemon.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 */
BrcmLbs_Result BrcmLbsRilAsn1_deinit(OsHandle ril)
{
    RilState *rilState = _ril_state;

    D("%s", __FUNCTION__);

	if (!rilState->cntinInitialized)
	{
		StopIPC();
	}

	rilState->asn1Initialized = 0;

    return BRCM_LBS_OK;
}


struct timeout_actions_elem
{
    int inuse;
    int id;
    time_t tout;
    void (*timeout_cb)(int, void *data);
    void *data;
};


static struct timeout_actions_elem ta_conn_retry[1];


static void timeout_action_init(struct timeout_actions_elem *elems, size_t max_n)
{
    memset(elems, 0, sizeof(struct timeout_actions_elem)*max_n);
}


static int timeout_action_add(struct timeout_actions_elem *ta, size_t max_n,
                              int id, time_t tout, void (*timeout_cb)(int, void *data), void *data)
{
    size_t n;
    struct timespec ts;

    for (n = 0; n < max_n; n++)
    {
        if (ta[n].inuse)
        {
            continue;
        }

        clock_gettime(CLOCK_MONOTONIC, &ts);
        ta[n].inuse = 1;
        ta[n].id = id;
        ta[n].tout = ts.tv_sec + tout;
        ta[n].timeout_cb = timeout_cb;
        ta[n].data = data;
        return 0;
    }

    return -1;
}



static void timeout_action_del(struct timeout_actions_elem *ta, size_t max_n,
                               int id)
{
    size_t n;

    for (n = 0; n < max_n; n++)
    {
        if (!ta[n].inuse)
        {
            continue;
        }

        if (ta[n].id != id)
        {
            continue;
        }

        ta[n].inuse = 0;
    }
}


static void timeout_action_update_timeouts(struct timeout_actions_elem *ta,
        size_t max_n)
{
    size_t n;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    for (n = 0; n < max_n; n++)
    {
        if (!ta[n].inuse)
        {
            continue;
        }

        if (ta[n].tout > ts.tv_sec)
        {
            continue;
        }

        if (ta[n].timeout_cb)
            ta[n].timeout_cb(ta[n].id,
                             ta[n].data);

        ta[n].inuse = 0;
    }
}



static int timeout_action_any_inuse(struct timeout_actions_elem *ta, size_t max_n)
{
    size_t n;

    for (n = 0; n < max_n; n++)
    {
        if (ta[n].inuse)
        {
            return 1;
        }
    }

    return 0;
}



static void ril_state_init(RilState* rilState)
{
    D("%s", __FUNCTION__);

    rilState->init       = 0;
    rilState->control[0] = -1;
    rilState->control[1] = -1;
    rilState->fd         = -1;
    rilState->lbs       = NULL;
    rilState->local_path = BRCM_LOCAL_PATH;

    timeout_action_init(ta_conn_retry, _SIZE(ta_conn_retry));

    return;
}



static int ril_thread_init(RilState *rilState)
{
    D("%s", __FUNCTION__);

    rilState->control[0] = -1;
    rilState->control[1] = -1;

    if (socketpair( AF_LOCAL, SOCK_SEQPACKET, 0, rilState->control) < 0)
    {
        LOGE("could not create thread control socket pair: %s", strerror(errno));
        goto Fail;
    }

    if (pthread_create(&rilState->thread, NULL, ril_state_thread, rilState) != 0)
    {
        LOGE("Could not create ril-gps thread: %s", strerror(errno));
        goto Fail;
    }
    
    return 0;

Fail:
    if (rilState->control[0] >= 0)
    {
        close(rilState->control[0]);
    }

    if (rilState->control[1] >= 0)
    {
        close(rilState->control[1]);
    }

    rilState->control[0] = -1;
    rilState->control[1] = -1;

    return -1;
}


static int wait_time_get()
{
/// Until STAGE A wait for very short constant time.
/// After STAGE A, wait for exponentially increasing time,
/// because there won't be a possible way to fix this socket mismatch dynamically.
#define WAIT_STAGE_A_THRESHOLD 	10
#define WAIT_STAGE_A_TIME       1
    ++iLcsConnectTrial;

    if (iLcsConnectTrial == 1)   // don't wait the first time
    {
        return 0;
    }

    if (iLcsConnectTrial <= WAIT_STAGE_A_THRESHOLD)
    {
        return WAIT_STAGE_A_TIME;
    }

    // Now it's stage B.
    int iTemp = (iLcsConnectTrial - WAIT_STAGE_A_THRESHOLD);
    // About an year of waiting.
    return 1 << _MIN(25, iTemp);
#undef WAIT_STAGE_A_THRESHOLD
#undef WAIT_STAGE_A_TIME
}



static void wait_time_reset()
{
    iLcsConnectTrial = 0;
}



static void process_cmds(RilState *rilState, struct ril_cmds *cmd)
{
    if (rilState->fd < 0)
    {
        D("%s, fd < 0, socket not ready, ignoring.", __FUNCTION__);
        return;
    }

    switch (cmd->id)
    {
    case CMD_SEND_TO_GPS:
        D("%s, CMD_SEND_TO_GPS", __FUNCTION__);
        BrcmLbsRil_sendToGps(rilState->ril,
            cmd->u.sendToGps.protocol,
            cmd->u.sendToGps.msg,
            cmd->u.sendToGps.size);
        break;

    case CMD_SET_UE_STATE:
        D("%s, CMD_SET_UE_STATE", __FUNCTION__);
        BrcmLbsRil_setUeState(rilState->ril,
            cmd->u.setUeState.protocol,
            cmd->u.setUeState.state);
        break;

    case CMD_RESET_ASSISTANCE_DATA:
        D("%s, CMD_RESET_ASSISTANCE_DATA", __FUNCTION__);
        BrcmLbsRil_resetAssistanceData(rilState->ril,
            cmd->u.resetAssistanceData.protocol);
        break;

    case CMD_DEINIT:
        D("%s, CMD_DEINIT", __FUNCTION__);
        // already handled.
        break;
        
    case CMD_CALIBRATION_STATUS:
        D("%s, CMD_CALIBRATION_STATUS", __FUNCTION__);
        BrcmLbsRil_calibrationStatus(rilState->ril,
            cmd->u.calibrationStatus.status);            
        break;

    case CMD_SET_SLP_FQDN:
        D("%s, CMD_SET_SLP_FQDN", __FUNCTION__);
        BrcmLbsRil_setSlpFqdn(rilState->ril,
            cmd->u.setSlpFqdn.fqdn);
        break;
        
    default:
        D("%s: Unhandled command: %d", __FUNCTION__, cmd->id);
        break;
    }

}


static int poll_register_in(struct pollfd pfd[], int nmax, int fd)
{
    int i;

    for (i = 0; i < nmax; i++)
    {
        int flags;

        if (pfd[i].events)
        {
            continue;
        }

        /* important: make the fd non-blocking */
        flags = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        pfd[i].fd = fd;
        pfd[i].events = POLLIN;
        break;
    }

    return i + 1;
}


static void* ril_state_thread(void* arg)
{
    int         started    = 0;
    RilState*   rilState   = (RilState*) arg;
    int         ril_fd     = rilState->fd;
    int         control_fd = rilState->control[1];

    D("RIL LCSAPI thread running");
    ril_lcsapi_init(rilState);

    for (;;)
    {
        struct      pollfd pfd[2];
        int         nfds;
        int         nevents;
        unsigned int ne;
        int timeoutms = -1;

        memset(pfd, 0, sizeof(pfd));

        timeout_action_update_timeouts(ta_conn_retry, _SIZE(ta_conn_retry));

        if (rilState->fd < 0)
        {
            // Waiting a constant time isn't the best solution because
            // 1. In the case where LCS socket is matching, less waiting is better.
            //    The booting time is smaller.
            // 2. In the case where LCS socket is mismatching due to integration failure,
            //    it causes unexpected flood of warnings.
            D("ril_state_thread, fd < 0, calling timeout_action_add");
            timeout_action_add(ta_conn_retry, _SIZE(ta_conn_retry), 0,
                               wait_time_get(), connect_init_cb, rilState);
        }
        else
        {
            D("ril_state_thread, fd = %d", rilState->fd);
            nfds = poll_register_in(pfd, _SIZE(pfd), rilState->fd);
            // Resets the LCS socket waiting time so next connection loss won't wait for long.
            wait_time_reset();
        }

        if (timeout_action_any_inuse(ta_conn_retry, _SIZE(ta_conn_retry)))
        {
            timeoutms = 1000;
        }

        // register control file descriptors for polling
        nfds = poll_register_in(pfd, _SIZE(pfd), control_fd );

        nevents = poll(pfd, nfds, timeoutms);

        if (nevents < 0)
        {
            if (errno != EINTR)
            {
                LOGE("poll unexpected error: %s", strerror(errno));
            }

            continue;
        }

        D("ril thread received %d events", nevents);

        if (nevents == 0)
        {
            continue;
        }

        for (ne=0; ne<_SIZE(pfd); ne++)
        {
            if ((pfd[ne].fd == rilState->fd) && (pfd[ne].revents & (POLLERR | POLLHUP)) != 0)
            {
                LOGE("EPOLLERR or EPOLLHUP after poll() !?");
                ril_state_done(rilState);
                continue;
            }

            if (pfd[ne].revents & POLLIN)
            {
                int  fd = pfd[ne].fd;

                if (fd == control_fd)
                {
                    struct ril_cmds cmd;
                    int   ret;
                    D("ril control fd event");

                    do
                    {
                        ret = read( fd, &cmd, sizeof(cmd));
                    }
                    while (ret < 0 && errno == EINTR);

                    if (ret == sizeof(cmd))
                    {
                        if (cmd.id == CMD_DEINIT)
                        {
                            goto Exit;
                        }
                        else
                        {
                            process_cmds(rilState, &cmd);
                        }
                    }
                    else if (ret < 0)
                    {
                        D("Error reading command %d", errno);
                    }
                    else
                    {
                        D("Not proper size");
                    }
                }
                else if (fd == rilState->fd)
                {
                    D("gps fd event");
                    BrcmLbs_processMessages(rilState->lbs, BRCMLBS_RX);
                }
                else
                {
                    LOGE("poll() returned unkown fd %d ?", fd);
                }
            }
        }
    }

Exit:
    D("Exiting ril thread");
    ril_state_done(rilState);
    return NULL;
}



static void connect_init_cb(int id, void *arg)
{
    RilState *rilState = (RilState *) arg;

    D("%s", __FUNCTION__);

    if (!rilState)
    {
        D("%s: called with NULL arg", __FUNCTION__);
        return;
    }

    ril_lcsapi_init(rilState);
}



static void ril_lcsapi_onerror(BrcmLbs_Result res, const char *reason, void *arg)
{
    D("%s", __FUNCTION__);
    LOGE("Error on RIL LCSAPI glgps connection: %d (%s)", res, reason ? reason : "");
}



static int ril_lcsapi_init(RilState* rilState)
{
    D("%s", __FUNCTION__);

    if (rilState->init)
    {
        D("%s Already Initialized", __FUNCTION__);
        return 0;
    }

    if (!(rilState->lbs = BrcmLbs_init(rilState->local_path)))
    {
        LOGE("RIL LCSPAI Failed to connect to the gps driver (%s) errno(%d)", 
             rilState->local_path, errno);
        ril_state_done(rilState);
        goto Fail;
    }

    D("%s, socket path=%s ", __FUNCTION__, rilState->local_path);

    if (!(rilState->ril = BrcmLbsRil_init(rilState->lbs)))
    {
        LOGE("Failed to initialize the ril handler");
        goto Fail;
    }

    rilState->fd = BrcmLbs_getIpcFileDescriptor(rilState->lbs);

    BrcmLbs_registerOnSendToNetwork(rilState->ril, on_SendToNetwork, rilState);
    BrcmLbs_registerOnCalibrationStart(rilState->ril, on_CalibrationStart, rilState);
    BrcmLbs_registerOnCalibrationEnd(rilState->ril, on_CalibrationEnd, rilState);
    BrcmLbs_registerOnSetCertPath(rilState->ril, on_SetCertPath, rilState);

    rilState->init = 1;
    return 0;

Fail:
    return -1;
}



static void ril_state_done(RilState* rilState)
{
    D("%s", __FUNCTION__);

    if (rilState->lbs)
    {
        BrcmLbsRil_deinit(rilState->lbs);
    }

    rilState->lbs = NULL;
    rilState->fd = -1;
    rilState->init = 0;
}



static void on_SendToNetwork(
    BrcmLbsRilAsn1_Protocol protocol, 
    const unsigned char* msg, 
    size_t size, 
    BrcmLbs_UserData userData)
{
    RilState *rilState = (RilState *) userData;

    D("%s", __FUNCTION__);

    if (rilState->rilCallbacks.onRilSendToNetwork)
    {
        rilState->rilCallbacks.onRilSendToNetwork(protocol, msg, size, rilState->userData);
    }
    else
    {
        LOGE("%s onRilSendToNetwork callback is NULL", __FUNCTION__);
    }
}



static void on_CalibrationStart(BrcmLbs_UserData userData)
{
    RilState *rilState = (RilState *) userData;

    D("%s", __FUNCTION__);

    if (rilState->cntinCallbacks.onCalibrationStart)
    {
        rilState->cntinCallbacks.onCalibrationStart(rilState->userData);
    }
    else
    {
        LOGE("%s on_CalibrationStart callback is NULL", __FUNCTION__);
    }
}



static void on_CalibrationEnd(BrcmLbs_UserData userData)
{
    RilState *rilState = (RilState *) userData;

    D("%s", __FUNCTION__);

    if (rilState->cntinCallbacks.onCalibrationEnd)
    {
        rilState->cntinCallbacks.onCalibrationEnd(rilState->userData);
    }
    else
    {
        LOGE("%s on_CalibrationEnd callback is NULL", __FUNCTION__);
    }
}


static void on_SetCertPath(
    const char* path, 
    BrcmLbs_UserData userData)
{
    RilState *rilState = (RilState *) userData;

    D("%s", __FUNCTION__);

    if (rilState->rilCallbacks.onSetCertPath)
    {
        rilState->rilCallbacks.onSetCertPath(path, rilState->userData);
    }
    else
    {
        LOGE("%s onSetCertPath callback is NULL", __FUNCTION__);
    }
}


/** Initialize the cntin interface module inside the GPS daemon.
 *  \param callbacks pointer to the structure for RIL HAL callbacks.
 *  \param userData user-specific data that will be passed back as parameter in all callbacks
 *  \return OS_HANDLE_INVALID if failure, some other value otherwise
 */
OsHandle BrcmLbsRilCntin_init(
    BrcmLbsRilCntin_Callbacks* cntinCallbacks,
	BrcmLbs_UserData userData)
{
    RilState *rilState = _ril_state;

    D("%s", __FUNCTION__);

    rilState->cntinCallbacks = *cntinCallbacks;
    rilState->cntinUserData = userData;

	if (!rilState->asn1Initialized)
	{
		if (BRCM_LBS_OK != StartIPC())
		{
			return OS_HANDLE_INVALID;
		}
	}

	rilState->cntinInitialized = 1;

	return (OsHandle)rilState;
}



/** Destroy the AT-command module inside the GPS daemon.
 *  \param ril [input] The handle for the RIL interface module, returned from BrcmLbsRilAsn1_init() call
 */
BrcmLbs_Result BrcmLbsRilCntin_deinit(OsHandle ril)
{
    RilState *rilState = _ril_state;

    D("%s", __FUNCTION__);

	if (!rilState->asn1Initialized)
	{
		StopIPC();
	}

	rilState->cntinInitialized = 0;

    return BRCM_LBS_OK;
}
