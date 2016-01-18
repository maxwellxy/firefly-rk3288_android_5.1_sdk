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
///  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
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

#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#define  LOG_TAG  "gps_BRCM"
#include <cutils/log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#ifdef GPS_H_IN_HARDWARE 
#include <hardware/gps.h>
#else
#include <hardware_legacy/gps.h>
#include <hardware_legacy/gps_ni.h>
#endif
#include <lbs.h>
#include <gps_api.h>
#include <supl.h>

#include "gps_fact_test.h"
#define  GPS_DEBUG   0

#if GPS_DEBUG
#  define  D(...)   LOGD(__VA_ARGS__)
#  define  DE(...)  LOGE(__VA_ARGS__)
#  define  DI(...)  LOGI(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#  define  DE(...)  LOGE(__VA_ARGS__)
#  define  DI(...)  ((void)0)
#endif

#define _SIZE(x) (sizeof(x)/sizeof(x[0]))
#define _MIN(a,b) (a>b?b:a)


//to detect whether it's using the new module way, or the old library.
#ifdef GPS_HARDWARE_MODULE_ID
#define GPS_AS_MODULE
#endif

#ifdef GPS_AS_MODULE
#define BRCM_LOCAL_PATH ANDROID_SOCKET_DIR"/gps"
#else
#define BRCM_LOCAL_PATH "/data/lcs.socket"
#endif

#define GPS_TEST_SENSITIVITY    100
#define GPS_TEST_CNO            101 
//#define GLONASS_GPS_CW_SUPPORT 1

/*****************************************************************/
/*****                                                       *****/
/*****       C O N N E C T I O N   S T A T E                 *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

/* commands sent to the gps thread */
typedef enum {
    CMD_QUIT  = 0,
    CMD_START,
    CMD_STOP,
    CMD_INJECT_TIME,
    CMD_INJECT_LOCATION,
    CMD_DELETE_AIDING,
    CMD_SET_POSITION,
} cmd_ids;

struct gps_cmds {
    cmd_ids id;
    union {
        struct {
            GpsUtcTime time;
            int64_t timeReference;
            int uncertainty;
        } inject_time;
        struct {
            double latitude;
            double longitude;
            float accuracy;
        } inject_location;
        struct {
            GpsAidingData flags;
        } delete_aiding_data;
        struct {
            GpsPositionMode mode;
#ifdef GPS_AS_MODULE
            GpsPositionRecurrence recurrence;
            uint32_t min_interval; 
            uint32_t preferred_accuracy;
            uint32_t preferred_time;
#else
            int fix_frequency;
#endif
        } set_position_mode;
    }u;
};

struct request_pending {
    size_t n;
    int requests_id[10];
};

/// The global LCS socket connection trial number.
static int iLcsConnectTrial = 0;

int put_request_session(struct request_pending *pend,int session_nr)
{
    unsigned int i;
    if (pend->n >= _SIZE(pend->requests_id)) {
        return -1;
    }
    for (i=0; i<pend->n;i++) {
        if (session_nr == pend->requests_id[i])
            return i;
    }
    pend->requests_id[pend->n++] = session_nr;
    return pend->n;
}

int get_request_session(struct request_pending *pend)
{
    if (pend->n == 0)
        return -1;
    if (pend->n > _SIZE(pend->requests_id))
        return -1;
    return pend->requests_id[--pend->n];
}

struct timeout_actions_elem {
    int inuse;
    int id;
    time_t tout;
    void (*timeout_cb)(int, void *data);
    void *data;
}; 

static struct timeout_actions_elem ta_list[10];
static struct timeout_actions_elem ta_conn_retry[1];

static void timeout_action_init(struct timeout_actions_elem *elems, size_t max_n)
{
    memset(elems,0,sizeof(struct timeout_actions_elem)*max_n);
}

static int timeout_action_add(struct timeout_actions_elem *ta, size_t max_n,
        int id, time_t tout, void (*timeout_cb)(int, void *data), void *data)
{
    size_t n; 
    struct timespec ts;
    for (n=0;n<max_n;n++) {
        if (ta[n].inuse)
            continue;
        clock_gettime(CLOCK_MONOTONIC,&ts);
        ta[n].inuse=1;
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
    for (n=0;n<max_n;n++) {
        if (!ta[n].inuse)
            continue;
        if (ta[n].id != id)
            continue;
        ta[n].inuse=0;
    }
}

static void timeout_action_update_timeouts(struct timeout_actions_elem *ta,
        size_t max_n)
{
    size_t n;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    for (n=0;n<max_n;n++) {
        if (!ta[n].inuse)
            continue;
        if (ta[n].tout > ts.tv_sec)
            continue;
        if (ta[n].timeout_cb)
            ta[n].timeout_cb(ta[n].id,
                    ta[n].data);
        ta[n].inuse=0;
    }
}

static int timeout_action_any_inuse(struct timeout_actions_elem *ta, size_t max_n)
{
    size_t n;
    for (n=0;n<max_n;n++) {
        if (ta[n].inuse)
            return 1;
    }
    return 0;
}

/* this is the state of our connection to the qemu_gpsd daemon */
typedef struct {
    int                     init;
    int                     fd;
    GpsCallbacks            callbacks;
    GpsFactTestCallbacks    ft_callbacks;
    pthread_t               thread;
    int                     control[2];
#ifdef GPS_AS_MODULE
    GpsPositionRecurrence   recurrence;
    uint32_t                min_interval;
    uint32_t                preferred_accuracy;
    uint32_t                preferred_time;
#else
    int                     fix_frequency; 
#endif
    char                    *local_path;
	/// LCS handler. This is the handler for LCS socket GPS driver.
    OsHandle                lcsh;
    OsHandle                gpsh;
    OsHandle                suplh;
    BrcmLbs_ReqHandle       rqh;
    GpsUtcTime              last_fix_system_time;
    GpsUtcTime              last_fix_gps_time;
    GpsPositionMode         mode;
    AGpsCallbacks           agps_callbacks;
    GpsNiCallbacks          ni_callbacks;
#ifdef AGPS_RIL_INTERFACE
    AGpsRilCallbacks        agps_ril_callbacks;
#endif
    struct request_pending  conn_pending;
    struct request_pending  refloc_pending;
    struct request_pending  notif_pending;
    char                    suplhost[80];
    int                     suplport;
#ifdef AGPS_RIL_INTERFACE
    AGpsSetIDType           setid_type;
    unsigned char           setid[8];
#endif
    int                     glonass;
    BrcmLbs_GpsStatus       engine_status;
    int                     notification_id;
    int                     gps_request_in_progress;
	int                     posValid;
    int                     rcv_ni_msg;
	
} GpsState;

static GpsState  _gps_state[1] = {
    {
        .init = 0,
        .rcv_ni_msg = 0	
    }
};





static GpsUtcTime utc_to_epoch_ms(const BrcmLbs_UtcTime *utc);
static GpsUtcTime estimate_gpstime(GpsState *s);
static void update_last_fix_time(GpsState *s,const  BrcmLbs_UtcTime *fix_time);
static void on_position(BrcmLbs_ReqHandle rh, const BrcmLbs_PosInfo *posinfo, BrcmLbs_UserData userData);
static void on_nmea (BrcmLbs_ReqHandle rh, const char* nmea, OsUint16 size, BrcmLbs_UserData userData);
static void on_start(BrcmLbs_ReqHandle rh, BrcmLbs_Result result,
                        BrcmLbs_UserData userData);
static void on_stop(BrcmLbs_ReqHandle rh, BrcmLbs_UserData userData);
static void on_status(BrcmLbs_GpsStatus status, BrcmLbs_UserData userData);
static void on_facttest_result(BrcmLbs_ReqHandle rh,
                 const BrcmLbs_FactTestInfo *result, BrcmLbs_UserData userData);
static void on_facttest_result_r(BrcmLbs_ReqHandle rh,
                 const BrcmLbs_FactTestInfo *result, BrcmLbs_UserData userData);
static void gps_engine_status_update(GpsStatusValue update);
static void ascii2bcd(const char *asciinum, unsigned char *bcd, size_t bcd_sz);
static int gps_lcsapi_init(GpsState* state);
static void gps_state_init( GpsState*  state );
static int  assist_gps_init_internal();
#ifdef AGPS_RIL_INTERFACE
static void check_and_request_setid(GpsState *s);
#endif // AGPS_RIL_INTERFACE

static int send_cmd(GpsState *s, struct gps_cmds *cmd);
static int gps_start();
static void gps_start_sh(GpsState *s);
static void gps_stop_sh(GpsState *s);
static void gps_inject_time_sh(GpsState *s,GpsUtcTime time,
        int64_t timeReference, int uncertainty);
static void gps_inject_location_sh(GpsState *s, double latitude,
        double longitude, float accuracy);
static void gps_delete_aiding_data_sh(GpsState *s, GpsAidingData flags);
static void gps_set_position_mode_sh(GpsState *s, GpsPositionMode mode,
#ifdef GPS_AS_MODULE
        GpsPositionRecurrence recurrence, uint32_t min_interval, 
        uint32_t preferred_accuracy, uint32_t preferred_time);        
#else
        int fix_frequency);
#endif
static void gps_ni_init(GpsNiCallbacks *callbacks);
static void gps_ni_respond(int notif_id, GpsUserResponseType user_response);
static void gps_ni_timeout_response(int id, void *data);



#ifdef GPS_AS_MODULE
static void send_terminate_ack(int control_fd)
{
    char ack = 0;
    write(control_fd, &ack, sizeof(char));
}

static void wait_terminate_ack(GpsState *s)
{
    int control_fd = s->control[0];
    
    struct pollfd pfd[1];
    int flags;
    flags = fcntl(control_fd, F_GETFL);
    fcntl(control_fd, F_SETFL, flags| O_NONBLOCK);
    pfd[0].fd = control_fd;
    pfd[0].events = POLLIN;
    
    int nfd = poll(pfd, 1, 10000);
    D("wait_ack() leave nfd=%d\n", nfd);
}
#endif //GPS_AS_MODULE


static void gps_control_thread_done(GpsState *s)
{
    struct gps_cmds cmd = {
       .id = CMD_QUIT
    };
    void*  dummy;

    if (s->control[0]<0)
        return;

    send_cmd(_gps_state,&cmd);

#ifdef GPS_AS_MODULE
    wait_terminate_ack(s);
#else
    pthread_join(s->thread, &dummy);
#endif

    D("thread exit:%p",dummy);

    // close the control socket pair
    close( s->control[0] ); s->control[0] = -1;
    close( s->control[1] ); s->control[1] = -1;
    
    s->thread = (pthread_t)NULL;
}

static void
gps_state_done( GpsState*  s )
{
    if (s->suplh)
        BrcmLbsSupl_deinit(s->suplh);
    if (s->gpsh)
        BrcmLbsGps_deinit(s->gpsh);
    if (s->lcsh)
        BrcmLbs_deinit(s->lcsh);
    s->suplh=NULL;
    s->gpsh=NULL;
    s->lcsh=NULL;
    s->fd = -1;
}


static int send_cmd(GpsState *s, struct gps_cmds *cmd)
{
    ssize_t nw;
    if (!s)
        return -1;
    if (!cmd)
        return -1;
    if ((nw=write(s->control[0],cmd,sizeof(*cmd))) < 0)
        return -1;
    return 0;
}

static int poll_register_in(struct pollfd pfd[], int nmax, int fd)
{
    int i;
    for (i=0;i<nmax;i++) {
        int flags;
        if (pfd[i].events)
            continue;
        /* important: make the fd non-blocking */
        flags = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        pfd[i].fd = fd;
        pfd[i].events = POLLIN;
        break;
    }
    return i+1;
}



static void set_close_on_exec(int fd)
{
    int flags;
    if (fd < 0)
        return;
    flags = fcntl(fd,F_GETFD);
    fcntl(fd,F_SETFD,flags | FD_CLOEXEC);
}

static int gps_wait_time_get()
{
/// Until STAGE A wait for very short constant time.
/// After STAGE A, wait for exponentially increasing time,
/// because there won't be a possible way to fix this socket mismatch dynamically.
#define WAIT_STAGE_A_THRESHOLD 	10
#define WAIT_STAGE_A_TIME       1
	++iLcsConnectTrial;
    if (iLcsConnectTrial == 1) // don't wait the first time
        return 0;
	if (iLcsConnectTrial <= WAIT_STAGE_A_THRESHOLD)
		return WAIT_STAGE_A_TIME;

	// Now it's stage B.
	int iTemp = (iLcsConnectTrial - WAIT_STAGE_A_THRESHOLD);
	// About an year of waiting.
	return 1 << _MIN(25, iTemp);
#undef WAIT_STAGE_A_THRESHOLD
#undef WAIT_STAGE_A_TIME
}

static void gps_wait_time_reset()
{
	iLcsConnectTrial = 0;
}

static void process_cmds(GpsState *s, struct gps_cmds *cmd)
{
    switch(cmd->id) {
        case CMD_QUIT:
            // Option already checked on the thread loop
            break;
        case CMD_START:
            gps_start_sh(s);
            break;
        case CMD_STOP:
            gps_stop_sh(s);
            break;
        case CMD_INJECT_TIME:
            gps_inject_time_sh(s,
                    cmd->u.inject_time.time,
                    cmd->u.inject_time.timeReference,
                    cmd->u.inject_time.uncertainty);
            break;
        case CMD_INJECT_LOCATION:
            gps_inject_location_sh(s,
                    cmd->u.inject_location.latitude,
                    cmd->u.inject_location.longitude,
                    cmd->u.inject_location.accuracy);
            break;
        case CMD_DELETE_AIDING:
            gps_delete_aiding_data_sh(s,
                    cmd->u.delete_aiding_data.flags);
            break;
        case CMD_SET_POSITION:
            gps_set_position_mode_sh(s,
                    cmd->u.set_position_mode.mode,
#ifdef GPS_AS_MODULE
                    cmd->u.set_position_mode.recurrence,
                    cmd->u.set_position_mode.min_interval,
                    cmd->u.set_position_mode.preferred_accuracy,
                    cmd->u.set_position_mode.preferred_time);
#else
                    cmd->u.set_position_mode.fix_frequency);
#endif
            break;
    }

}


static void connect_init_cb(int id, void *arg)
{
    GpsState *s = (GpsState *) arg;
    if (!s) {
        D("%s: with NULL arg",__FUNCTION__);
        return;
    }
    gps_lcsapi_init(s);

}
/* this is the main thread, it waits for commands from gps_state_start/stop and,
 * when started, messages from the QEMU GPS daemon. these are simple NMEA sentences
 * that must be parsed to be converted into GPS fixes sent to the framework
 */
#ifdef GPS_AS_MODULE
static void
#else
static void*
#endif
gps_state_thread( void*  arg )
{
    GpsState*   state = (GpsState*) arg;
    int         started    = 0;
    int         gps_fd     = state->fd;
    int         control_fd = state->control[1];


    D("gps thread running");
    // now loop
    gps_lcsapi_init(state);
    for (;;) {
        struct      pollfd pfd[2];
        int         nfds;
        int         nevents;
        unsigned int ne;
        int timeoutms = -1;
        memset(pfd,0,sizeof(pfd));
        timeout_action_update_timeouts(ta_conn_retry,_SIZE(ta_conn_retry));
        timeout_action_update_timeouts(ta_list,_SIZE(ta_list));
        if (state->fd < 0) {
            // Waiting a constant time isn't the best solution because
            // 1. In the case where LCS socket is matching, less waiting is better.
            //    The booting time is smaller.
            // 2. In the case where LCS socket is mismatching due to integration failure,
            //    it causes unexpected flood of warnings.
            timeout_action_add(ta_conn_retry,_SIZE(ta_conn_retry),0,
                        gps_wait_time_get(),connect_init_cb,state);
        }
        else{
            nfds = poll_register_in(pfd, _SIZE(pfd), state->fd);
            // Resets the LCS socket waiting time so next connection loss won't wait for long.
            gps_wait_time_reset();
        }
        if (timeout_action_any_inuse(ta_list,_SIZE(ta_list))||
                timeout_action_any_inuse(ta_conn_retry, _SIZE(ta_conn_retry)))
            timeoutms = 1000;
        // register control file descriptors for polling
        nfds = poll_register_in(pfd, _SIZE(pfd), control_fd );

        nevents = poll(pfd, nfds, timeoutms);
        if (nevents < 0) {
            if (errno != EINTR)
                LOGE("poll unexpected error: %s", strerror(errno));
            continue;
        }
        D("gps thread received %d events", nevents);
        if (nevents == 0)
            continue;
        for (ne = 0; ne < _SIZE(pfd); ne++) {
            if ((pfd[ne].fd == state->fd) && (pfd[ne].revents & (POLLERR|POLLHUP)) != 0) {
                LOGE("EPOLLERR or EPOLLHUP after poll() !");
                gps_state_done(state);
                gps_engine_status_update(GPS_STATUS_ENGINE_OFF);
                continue;
            }
            if (pfd[ne].revents & POLLIN) {
                int  fd = pfd[ne].fd;

                if (fd == control_fd)
                {
                    struct gps_cmds cmd;
                    int   ret;
                    D("gps control fd event");
                    do {
                        ret = read( fd, &cmd, sizeof(cmd));
                    } while (ret < 0 && errno == EINTR);
                    if (ret == sizeof(cmd)) {
                        if (cmd.id == CMD_QUIT)
                            goto Exit;
                        else
                            process_cmds(state,&cmd);
                    }
                    else if (ret < 0) {
                        D("Error reading command %d",errno);
                    }
                    else {
                        D("Not proper size");
                    }
                }
                else if (fd == state->fd)
                {
                    D("gps fd event");
                    BrcmLbs_processMessages(state->lcsh, BRCMLBS_RX);
                }
                else
                {
                    LOGE("poll() returned unkown fd %d ?", fd);
                }
            }
        }
    }
Exit:
    gps_state_done(state);
#ifdef GPS_AS_MODULE
    send_terminate_ack(control_fd);
    return;
#else
    return NULL;
#endif
}


static int 
gps_control_thread_init(GpsState *state)
{
    state->control[0] = -1;
    state->control[1] = -1;

    if ( socketpair( AF_LOCAL, SOCK_SEQPACKET, 0, state->control ) < 0 ) {
        LOGE("could not create thread control socket pair: %s", strerror(errno));
        goto Fail;
    }

#ifdef GPS_AS_MODULE
    state->thread = state->callbacks.create_thread_cb( "libgps", gps_state_thread, state );
    if ( !state->thread ) {
#else
    if ( pthread_create( &state->thread, NULL, gps_state_thread, state ) != 0 ) {
#endif
        LOGE("could not create gps thread: %s", strerror(errno));
        goto Fail;
    }
    return 0;
Fail:
    if (state->control[0] >= 0)
        close(state->control[0]);
    if (state->control[1] >= 0)
        close(state->control[1]);
    state->control[0]=-1;
    state->control[1]=-1;
    return -1;
}

static void lcsapi_onerror(BrcmLbs_Result res, const char *reason, void *arg)
{
    LOGE("Error on glgps connection: %d (%s)",res, reason?reason:"");
}

static int gps_lcsapi_init(GpsState* state)
{

    if (!(state->lcsh=BrcmLbs_init(state->local_path))) {
        LOGE("Failed to connect to the gps driver (%s) errno(%d)",
                state->local_path,errno);
        goto Fail;
    }

	if (assist_gps_init_internal()) 
	{
        LOGE("%s:%d: Failed to initialize SUPL handler.", __FUNCTION__, __LINE__);
        goto Fail;
	}

    if (!(state->gpsh=BrcmLbsGps_init(state->lcsh))) {
        LOGE("Failed to initialize the gps driver");
        goto Fail;
    }

    state->fd         = BrcmLbs_getIpcFileDescriptor(state->lcsh);

    BrcmLbs_registerOnError(state->lcsh, lcsapi_onerror, state);

    BrcmLbs_registerOnNmea(state->gpsh,on_nmea, state);
    BrcmLbs_registerOnStart(state->gpsh,on_start, state);
    BrcmLbs_registerOnStop(state->gpsh,on_stop, state);
    BrcmLbs_registerOnGpsStatus(state->gpsh, on_status, state);
    
    if (state->gps_request_in_progress)
        gps_start();

		return 0;
Fail:
    gps_state_done( state );
    return -1;
}

static void
gps_state_init( GpsState*  state )
{
    struct timeval tv;
    int    ret;

    state->init       = 1;
    state->control[0] = -1;
    state->control[1] = -1;
#ifdef GPS_AS_MODULE
    state->recurrence = GPS_POSITION_RECURRENCE_PERIODIC;
    state->min_interval = 1000;
    state->preferred_accuracy = 50;
    state->preferred_time = 1;  
#else
    state->fix_frequency = 1;
#endif
    state->fd         = -1;
    state->lcsh       = NULL;
    state->gpsh       = NULL;
    state->suplh      = NULL;
    state->local_path = BRCM_LOCAL_PATH;
#ifdef AGPS_RIL_INTERFACE
    state->setid_type = AGPS_SETID_TYPE_NONE;
#endif
    state->engine_status = BRCM_LBS_GPS_ENGINE_OFF;
    
    gettimeofday(&tv,NULL);
    state->last_fix_system_time = (GpsUtcTime) tv.tv_sec*1000+tv.tv_usec/1000;
    state->last_fix_gps_time = (GpsUtcTime) tv.tv_sec*1000+tv.tv_usec/1000;
    timeout_action_init(ta_list,_SIZE(ta_list));
    timeout_action_init(ta_conn_retry,_SIZE(ta_conn_retry));
    
    state->gps_request_in_progress=0;

    D("gps state initialized");
    return;

}


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       I N T E R F A C E                               *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/


static int
gps_init(GpsCallbacks* callbacks)
{
    GpsState*  s = _gps_state;
    static int isInitialized = 0;

    D("%s: called.(%d)", __FUNCTION__, isInitialized);
    if (isInitialized == 0)
    {
        s->callbacks = *callbacks;
        gps_state_init(s);

#ifdef GPS_AS_MODULE
        s->callbacks.set_capabilities_cb(GPS_CAPABILITY_SCHEDULING | GPS_CAPABILITY_MSA 
                | GPS_CAPABILITY_MSB | GPS_CAPABILITY_SINGLE_SHOT);
#endif

        gps_control_thread_init(s);
        isInitialized = 1;
    }

    return 0;
}

static int check_thread_status(GpsState *s)
{
    if (!s->thread)
    {
        return gps_control_thread_init(s);
    }
        
    return 0;
}

static void
gps_cleanup(void)
{
//    GpsState*  s = _gps_state;

//    gps_control_thread_done(s);
   
    D("%s: called.", __FUNCTION__);
}

static int gps_start()
{
    GpsState *s = _gps_state;
    struct gps_cmds cmd = {
        .id = CMD_START
    };
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }
    return send_cmd(s, &cmd);
}

static void 
gps_start_sh(GpsState *s)
{
    BRCM_LBS_SETCapabilities brcm_cap;
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return;
    }
    if (!s->gpsh) {
        D("%s: not gps handle associated", __FUNCTION__);
        return;
    }
#ifdef AGPS_RIL_INTERFACE
    check_and_request_setid(s);
#endif
    switch(s->mode) {
        case GPS_POSITION_MODE_STANDALONE:
            if (s->suplh)
                BrcmLbsSupl_disable(s->suplh);
            brcm_cap.posTechnology.agpsSETassisted=0;
            brcm_cap.posTechnology.agpsSETBased=0;
            brcm_cap.posTechnology.autonomousGPS=1;
            BrcmLbsSupl_setCapabilities(s->suplh, &brcm_cap, 1);
            break;
        case GPS_POSITION_MODE_MS_BASED:
            if (!s->suplh)
                break;
            brcm_cap.posTechnology.agpsSETassisted=0;
            brcm_cap.posTechnology.agpsSETBased=1;
            brcm_cap.posTechnology.autonomousGPS=1;
            BrcmLbsSupl_enable(s->suplh);
            BrcmLbsSupl_setCapabilities(s->suplh, &brcm_cap, 1);
            break;
        case GPS_POSITION_MODE_MS_ASSISTED:
            if (!s->suplh)
                break;
            brcm_cap.posTechnology.agpsSETassisted=1;
            brcm_cap.posTechnology.agpsSETBased=0;
            brcm_cap.posTechnology.autonomousGPS=1;
            BrcmLbsSupl_enable(s->suplh);
            BrcmLbsSupl_setCapabilities(s->suplh, &brcm_cap, 1);
            break;
        case GPS_TEST_SENSITIVITY:   // Sensitivity Test , Factory_Low_SNR'
        
            if (BrcmLbsGps_deleteAidingData(s->gpsh,
                        BRCMLBSGPS_AIDING_POS
                        | BRCMLBSGPS_AIDING_EPH
                        | BRCMLBSGPS_AIDING_TIM
                        | BRCMLBSGPS_AIDING_ALM
                        | BRCMLBSGPS_AIDING_ROM_ALM) != BRCM_LBS_OK)
                break;

#ifdef GLONASS_GPS_CW_SUPPORT
            s->glonass = 1;
            D("[%s]: GPS_FACT_TEST_CN0(glonass = %d) FRQ fact test launched",__FUNCTION__, s->glonass);
            s->rqh= BrcmLbsClient_requestFactTestCombo(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_CN0,
                             (!s->glonass)?1:BRCM_UNASSIGNED_PRN,
                             (s->glonass)?0:BRCM_UNASSIGNED_FCN,
							 10,-1, on_facttest_result_r,(void *)s);

#else
            s->glonass = 0;
            D("[%s]: GPS_FACT_TEST_CN0(glonass = %d) FRQ fact test launched",__FUNCTION__, s->glonass);
          
            /*
            // deprecated use combox instead now
            D("%s(%d): FRQ fact test launched",__FUNCTION__,__LINE__);
            s->rqh=BrcmLbsClient_requestFactTest(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_CN0,
                            1,10,-1, on_facttest_result_r,(void *)s);
            */    
            
            s->rqh= BrcmLbsClient_requestFactTestCombo(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_CN0,
                             (!s->glonass)?1:BRCM_UNASSIGNED_PRN,
                             (s->glonass)?0:BRCM_UNASSIGNED_FCN,
                             10,-1, on_facttest_result_r,(void *)s);
#endif
           
 
            break;
        case GPS_TEST_CNO:   //CN0 Test , Factory_High_SNR
        
            if (BrcmLbsGps_deleteAidingData(s->gpsh,
                        BRCMLBSGPS_AIDING_POS
                        | BRCMLBSGPS_AIDING_EPH
                        | BRCMLBSGPS_AIDING_TIM
                        | BRCMLBSGPS_AIDING_ALM
                        | BRCMLBSGPS_AIDING_OSC
                        | BRCMLBSGPS_AIDING_ROM_ALM) != BRCM_LBS_OK)
                break;
        
#ifdef GLONASS_GPS_CW_SUPPORT
            s->glonass = 1;
            D("[%s]: GPS_FACT_TEST_FRQ(glonass = %d) fact test launched",__FUNCTION__, s->glonass);
            s->rqh= BrcmLbsClient_requestFactTestCombo(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_FRQ,
                           (!s->glonass)?1:BRCM_UNASSIGNED_PRN,
                           (s->glonass)?0:BRCM_UNASSIGNED_FCN,
                           10,-1, on_facttest_result_r,(void *)s);
#else
          s->glonass = 0;
          D("[%s]: GPS_FACT_TEST_FRQ(glonass = %d) fact test launched",__FUNCTION__, s->glonass);
/*
            // deprecated use combox instead now
            s->rqh=BrcmLbsClient_requestFactTest(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_FRQ,
                            1,10,-1, on_facttest_result_r,(void *)s);
*/
            s->rqh= BrcmLbsClient_requestFactTestCombo(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_FRQ,
                           (!s->glonass)?1:BRCM_UNASSIGNED_PRN,
                           (s->glonass)?0:BRCM_UNASSIGNED_FCN,
                           10,-1, on_facttest_result_r,(void *)s);
#endif        
            D("%s(%d): CN0 fact test launched",__FUNCTION__,__LINE__);
            break;
        default:
            D("Not a valid mode selected");
            return;
    }
#ifdef GPS_AS_MODULE
    if (s->mode == GPS_POSITION_MODE_MS_ASSISTED) {
        // Framework will always set s->recurrence to GPS_POSITION_RECURRENCE_PERIODIC
        // We need to force single shot for MSA.
        D("%s(%d): calling irm periodic (%s)",__FUNCTION__,__LINE__,s->local_path);
        s->rqh = BrcmLbsGps_requestSingleLocation(s->gpsh,180,s->preferred_accuracy,
                        on_position,s); 
    }
    else {
        // periodic
        D("%s(%d): periodic request (%s, %d)",__FUNCTION__,__LINE__,s->local_path,
                s->min_interval);
        s->rqh = BrcmLbsGps_requestPeriodicLocation(s->gpsh,s->min_interval,
                    s->preferred_accuracy, on_position, s);
    }
#else    
    if ((s->mode == GPS_POSITION_MODE_MS_BASED) || (s->mode == GPS_POSITION_MODE_MS_ASSISTED) ||
        (s->mode == GPS_POSITION_MODE_STANDALONE)) {
            if (s->fix_frequency) {
                D("%s(%d): periodic request (%s, %d)",__FUNCTION__,__LINE__,s->local_path,
                        s->fix_frequency);
                s->rqh=BrcmLbsGps_requestPeriodicLocation(s->gpsh,s->fix_frequency*1000,
                            50, on_position, s);
            }
            else {
                D("%s(%d): calling irm periodic (%s)",__FUNCTION__,__LINE__,s->local_path);
                s->rqh=BrcmLbsGps_requestSingleLocation(s->gpsh,180,50,
                                on_position,s);
            }
    }
#endif
        
    if (s->rqh) {
        s->gps_request_in_progress=1;
        D("%s(%d): start request success",__FUNCTION__,__LINE__);
    }
    else
        D("%s(%d): start request failed errno(%d)",__FUNCTION__,__LINE__,errno);
}

static int gps_stop()
{
    GpsState *s = _gps_state;
    struct gps_cmds cmd = {
        .id = CMD_STOP
    };
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }
    return send_cmd(s,&cmd);
}

static void
gps_stop_sh(GpsState *s)
{
    BrcmLbs_Result result;

    s->gps_request_in_progress=0;

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return;
    }
    if (!s->gpsh) {
        D("%s: not gps handle associated", __FUNCTION__);
        return;
    }

    if ((result = BrcmLbsGps_stopAllRequests(s->gpsh)) != BRCM_LBS_OK)
        D("%s(%d): stop failed error(%d)",__FUNCTION__,__LINE__,result);
    else
        D("%s(%d): stop success",__FUNCTION__,__LINE__);
    D("%s: called", __FUNCTION__);
}

static int gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    GpsState*  s = _gps_state;
    struct gps_cmds cmd = {
        .id = CMD_INJECT_TIME
    };
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }
    cmd.u.inject_time.time=time;
    cmd.u.inject_time.timeReference=timeReference;
    cmd.u.inject_time.uncertainty=uncertainty;
    return send_cmd(s, &cmd);
}

static void 
gps_inject_time_sh(GpsState *s,
        GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    if (!s->gpsh) {
        D("%s: not gps handle associated", __FUNCTION__);
        return;
    }
    BrcmLbsGps_injectUtcTime(s->gpsh,time,uncertainty);
}

static int gps_inject_location(double latitude, double longitude, float accuracy)
{
    GpsState*  s = _gps_state;
    struct gps_cmds cmd = {
        .id = CMD_INJECT_LOCATION
    };

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }
    cmd.u.inject_location.latitude=latitude;
    cmd.u.inject_location.longitude=longitude;
    cmd.u.inject_location.accuracy=accuracy;
    return send_cmd(s,&cmd);
}

static void
gps_inject_location_sh(GpsState *s, double latitude, double longitude,
        float accuracy)
{
    BrcmLbs_Position pos = {
        .m = { .altitudePresent = 0,
               .pos_errorPresent = 1
            },
        .latitude = latitude,
        .longitude = longitude,
        .pos_error = accuracy 
    };
    if (!s->gpsh) {
        D("%s: not gps handle associated", __FUNCTION__);
        return;
    }
    BrcmLbsGps_injectPosition(s->gpsh,&pos);
}

static void gps_delete_aiding_data(GpsAidingData flags)
{
    GpsState *s =_gps_state;
    struct gps_cmds cmd = {
        .id = CMD_DELETE_AIDING
    };
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return;
    }
    cmd.u.delete_aiding_data.flags=flags;
    send_cmd(s,&cmd);
}

static int wait_for_stop(GpsState *s, int tout_ms)
{
#define SLEEP_STEP_MS (200)
#define SLEEP_STEP_US (SLEEP_STEP_MS*1000)
    int e_time; 
    if (BRCM_LBS_GPS_ENGINE_ON != s->engine_status)
        return 0;
    gps_stop();
    for (e_time = 0; e_time < tout_ms; e_time += SLEEP_STEP_MS) {
        if (BRCM_LBS_GPS_ENGINE_OFF != s->engine_status)
            usleep(SLEEP_STEP_US);
        else
            return 0;
    }
    return -1;
}

static void
gps_delete_aiding_data_sh(GpsState *s, GpsAidingData flags)
{
    uint16_t mode=0;
    int      req_in_prog = 0;
    BrcmLbs_Result result = BRCM_LBS_OK;
    
    if (NULL == s)
    {   D("%s: invalid state pointer !!", __FUNCTION__);
        return;
    }
    
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
    }

#if defined(CONFIG_LIBGPS_IGNORE_DELETING_AIDING_OK)    
    LOGD("%s: check request progress", __FUNCTION__);
    if (s->gps_request_in_progress)
    {
        LOGD("%s: request in progress, ignore deleting aiding", __FUNCTION__);
        return;
    }
#endif
    if (BRCM_LBS_GPS_ENGINE_ON == s->engine_status) {
        req_in_prog = s->gps_request_in_progress;
        wait_for_stop(s, 1000);
        wait_for_stop(s, 1000);
    }
    if (!s->gpsh) {
        D("%s: not gps handle associated", __FUNCTION__);
        return;
    }
    
    if (flags & GPS_DELETE_EPHEMERIS)
        mode |= BRCMLBSGPS_AIDING_EPH;
    if (flags & GPS_DELETE_ALMANAC)
        mode |= BRCMLBSGPS_AIDING_ALM;
    if (flags & GPS_DELETE_POSITION)
        mode |= BRCMLBSGPS_AIDING_POS;
    if (flags & GPS_DELETE_TIME)
        mode |= BRCMLBSGPS_AIDING_TIM;
    if ((flags & GPS_DELETE_ALL) == GPS_DELETE_ALL)
        mode = BRCMLBSGPS_AIDING_POS | BRCMLBSGPS_AIDING_EPH | BRCMLBSGPS_AIDING_TIM |
            BRCMLBSGPS_AIDING_ALM;
            
    if ((result = BrcmLbsGps_deleteAidingData(s->gpsh, mode)) != BRCM_LBS_OK)
        D("%s(%d): delete aiding(%X) failed error(%d)",__FUNCTION__,__LINE__,
                mode,result);
    else
        D("%s(%d): delete aiding(%X) success",__FUNCTION__,__LINE__,mode);

#if !defined(CONFIG_LIBGPS_IGNORE_DELETING_AIDING_OK)    
    if (req_in_prog)
    {
        D("%s: restart request", __FUNCTION__);
        usleep(200000);    // wait for 200ms
        gps_start();
    }
#endif
    D("%s: called", __FUNCTION__);
}


#ifdef GPS_AS_MODULE
static int gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
            uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
#else
static int gps_set_position_mode(GpsPositionMode mode, int fix_frequency)
#endif
{
    GpsState *s= _gps_state;
    struct gps_cmds cmd = {
        .id = CMD_SET_POSITION
    };
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

    if (check_thread_status(s))
    {
        LOGE("%s: could not start control thread !!", __FUNCTION__);
    }
    
    cmd.u.set_position_mode.mode = mode;
#ifdef GPS_AS_MODULE
    cmd.u.set_position_mode.recurrence = recurrence;
    cmd.u.set_position_mode.min_interval = min_interval ? min_interval : 1000;
    cmd.u.set_position_mode.preferred_accuracy = preferred_accuracy ? preferred_accuracy : 50;
    cmd.u.set_position_mode.preferred_time = preferred_time;
#else
    cmd.u.set_position_mode.fix_frequency = fix_frequency;
#endif
    return  send_cmd(s,&cmd);
}

#ifdef GPS_AS_MODULE
static void gps_set_position_mode_sh(GpsState *s,
        GpsPositionMode mode, GpsPositionRecurrence recurrence,
        uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
#else
static void gps_set_position_mode_sh(GpsState *s,
        GpsPositionMode mode, int fix_frequency)
#endif
{
    // FIXME - support fix_frequency
    // only standalone supported for now.
    switch (mode) {
        case GPS_POSITION_MODE_STANDALONE:
        case GPS_POSITION_MODE_MS_BASED:
        case GPS_POSITION_MODE_MS_ASSISTED: 
        case GPS_TEST_SENSITIVITY:   // Sensitivity Test 
        case GPS_TEST_CNO:   // CN0 Test
            s->mode = mode;
            break;
        default:
            D("%s: mode not supported, assuming standaland!",__FUNCTION__);
            s->mode = GPS_POSITION_MODE_STANDALONE;
    }

#ifdef GPS_AS_MODULE
    s->recurrence = recurrence;
    s->min_interval = min_interval;
    s->preferred_accuracy = preferred_accuracy;
    s->preferred_time = preferred_time;
#else
   s->fix_frequency = fix_frequency;
#endif

    D("%s: called", __FUNCTION__);
}

#define SECONDS_PER_MIN (60)
#define SECONDS_PER_HOUR (60*SECONDS_PER_MIN)
#define SECONDS_PER_DAY  (24*SECONDS_PER_HOUR)
#define SECONDS_PER_NORMAL_YEAR (365*SECONDS_PER_DAY)
#define SECONDS_PER_LEAP_YEAR (SECONDS_PER_NORMAL_YEAR+SECONDS_PER_DAY)


static int days_per_month_no_leap[] =
    {31,28,31,30,31,30,31,31,30,31,30,31};
static int days_per_month_leap[] =
    {31,29,31,30,31,30,31,31,30,31,30,31};

static int is_leap_year(int year)
{
    if ((year%400) == 0)
        return 1;
    if ((year%100) == 0)
        return 0;
    if ((year%4) == 0)
        return 1;
    return 0;
}
static int number_of_leap_years_in_between(int from, int to)
{
    int n_400y, n_100y, n_4y;
    n_400y = (to-1)/400 - from/400;
    n_100y = (to-1)/100 - from/100;
    n_4y = (to-1)/4 - from/4;
    return (n_4y - n_100y + n_400y);
}

static GpsUtcTime utc_to_epoch_ms(const BrcmLbs_UtcTime *utc)
{
    GpsUtcTime t_epoch_ms=0;
    int m;
    int *days_per_month;
    if (is_leap_year(utc->year))
        days_per_month = days_per_month_leap;
    else
        days_per_month = days_per_month_no_leap;
    t_epoch_ms += (GpsUtcTime) (utc->year - 1970)*SECONDS_PER_NORMAL_YEAR*1000;
    t_epoch_ms += (GpsUtcTime) number_of_leap_years_in_between(1970,utc->year) *
        SECONDS_PER_DAY*1000;
    for (m=1; m<utc->month; m++) {
        t_epoch_ms += (GpsUtcTime) days_per_month[m-1]*SECONDS_PER_DAY*1000;
    }
    t_epoch_ms += (GpsUtcTime) (utc->day-1)*SECONDS_PER_DAY*1000;
    t_epoch_ms += (GpsUtcTime) utc->hour*SECONDS_PER_HOUR*1000;
    t_epoch_ms += (GpsUtcTime) utc->min*SECONDS_PER_MIN*1000;
    t_epoch_ms += (GpsUtcTime) utc->sec*1000;
    t_epoch_ms += (GpsUtcTime) utc->milliSec;
    return t_epoch_ms;

}

#ifdef AGPS_RIL_INTERFACE
static void check_and_request_setid(GpsState *s)
{
    if (s->setid_type != AGPS_SETID_TYPE_NONE)
        return;
    if (!s->agps_ril_callbacks.request_setid)
        return;
    // order is important always ask for AGPS_RIL_REQUEST_SETID_MSISDN first     
    LOGD("%s: called request_setid AGPS_RIL_REQUEST_SETID_MSISDN", __FUNCTION__);
    s->agps_ril_callbacks.request_setid(AGPS_RIL_REQUEST_SETID_MSISDN);
            
 	LOGD("%s: called request_setid AGPS_RIL_REQUEST_SETID_IMSI", __FUNCTION__);
    s->agps_ril_callbacks.request_setid(AGPS_RIL_REQUEST_SETID_IMSI);
}
#endif

static char inttohex[] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

static size_t encodehex(char *dest, size_t d_size,
        const unsigned char *source,
        size_t s_size)
{
    size_t i;
    for (i=0;((i<s_size) && (i< (d_size/2))); i++) {
        dest[2*i] = inttohex [(source[i]>>4)&0x0F];
        dest[2*i+1] = inttohex [(source[i])&0x0F];
    }
    return i*2;
}


static void on_notification_ref(OsHandle supl, int supl_session_id,
        const BrcmLbsSupl_Notification *notif, BrcmLbs_UserData userData)
{
    GpsState *s = _gps_state;
    GpsNiNotification notification;

    D("%s: entry", __FUNCTION__);

    if (!supl) {
        LOGE("Error initializing supl");
        return;
    }
	
    if (!s->ni_callbacks.notify_cb ) {
        LOGW("no supl ni notify cb");
        return;
    }

    memset(&notification, 0x00, sizeof(notification));

    if ( supl_session_id == 0 ) {
        s->notification_id++;
        D("%s: notification_id = %d", __FUNCTION__, s->notification_id);
    }

    if (put_request_session(&s->notif_pending, supl_session_id) < 0) {
        LOGW("No room for incoming supl session");
		return;
    }
    
#ifdef GPS_AS_MODULE
    notification.size = sizeof(GpsNiNotification);
#endif
    notification.notification_id = s->notification_id;
    notification.ni_type = GPS_NI_TYPE_UMTS_SUPL;
    notification.default_response = GPS_NI_RESPONSE_NORESP;
    
    switch( notif->notificationType )
    {
        case BRCM_LBS_noNotificationNoVerification :
        case BRCM_LBS_notificationOnly :
            notification.notify_flags = GPS_NI_NEED_NOTIFY;
            break;
        case BRCM_LBS_notificationAndVerficationAllowedNA :
            notification.notify_flags = GPS_NI_NEED_NOTIFY | GPS_NI_NEED_VERIFY;
            notification.default_response = GPS_NI_RESPONSE_ACCEPT;
            break;
        case BRCM_LBS_notificationAndVerficationDeniedNA :
            notification.notify_flags = GPS_NI_NEED_NOTIFY | GPS_NI_NEED_VERIFY;
            notification.default_response = GPS_NI_RESPONSE_DENY;
            break;
        case BRCM_LBS_privacyOverride :
            notification.notify_flags = GPS_NI_PRIVACY_OVERRIDE;
            break;
        default :
            return;
    }

    switch ( notif->encodingType )
    {
        case BRCMLBSSUPL_ENC_UCS2 :
            notification.requestor_id_encoding = GPS_ENC_SUPL_UCS2;
            notification.text_encoding = GPS_ENC_SUPL_UCS2;
            break;
        case BRCMLBSSUPL_ENC_GSMDEFAULT :
            notification.requestor_id_encoding = GPS_ENC_SUPL_GSM_DEFAULT;
            notification.text_encoding = GPS_ENC_SUPL_GSM_DEFAULT;            
            break;
        case BRCMLBSSUPL_ENC_UTF8 :
            notification.requestor_id_encoding = GPS_ENC_SUPL_UTF8;
            notification.text_encoding = GPS_ENC_SUPL_UTF8;
            break;
        case BRCMLBSSUPL_ENC_UNKNOWN :
            notification.requestor_id_encoding = GPS_ENC_UNKNOWN;
            notification.text_encoding = GPS_ENC_UNKNOWN;
            break;
        default :
            return;
    }
    encodehex(notification.requestor_id,sizeof(notification.requestor_id),
                notif->requestorId.data, notif->requestorId.len);
    encodehex(notification.text,sizeof(notification.text),
            notif->clientName.data, notif->clientName.len);

    notification.timeout = 6;

    s->ni_callbacks.notify_cb(&notification);
    if (notification.default_response != GPS_NI_RESPONSE_NORESP)
        timeout_action_add(ta_list,_SIZE(ta_list), notification.notification_id,
            notification.timeout,gps_ni_timeout_response,
            (void *)notification.default_response);
}


#ifdef AGPS_RIL_INTERFACE
static void on_request_ref_location(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
    GpsState *s = _gps_state;
    if (!supl) {
        LOGE("Supl handler not valid");
        return;
    }
    if (put_request_session(&s->refloc_pending,supl_session_id) < 0) {
        LOGW("No room for incoming supl session");
    }

    if (s->agps_ril_callbacks.request_refloc)
    {
        s->agps_ril_callbacks.request_refloc(AGPS_RIL_REQUEST_REFLOC_CELLID); 
    }
}
#else
static void on_request_ref_location(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
}
#endif

void on_request_connection(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
    GpsState *s = _gps_state;

#ifdef GPS_AS_MODULE
    AGpsStatus status = {sizeof(AGpsStatus),AGPS_TYPE_SUPL,GPS_REQUEST_AGPS_DATA_CONN};
#else
    AGpsStatus status = {AGPS_TYPE_SUPL,GPS_REQUEST_AGPS_DATA_CONN};
#endif

#ifdef AGPS_RIL_INTERFACE
    check_and_request_setid(s);
#endif
    if (!supl) {
        LOGE("Error initializing supl");
        return;
    }
    if (put_request_session(&s->conn_pending,supl_session_id) < 0) {
        LOGW("No room for incoming supl sessions");
        return;
    }
    if (!s->agps_callbacks.status_cb)
        return;
    s->agps_callbacks.status_cb(&status);
}

void on_supl_session_ended(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
    GpsState *s = _gps_state;

#ifdef GPS_AS_MODULE
    AGpsStatus status = {sizeof(AGpsStatus),AGPS_TYPE_SUPL,GPS_RELEASE_AGPS_DATA_CONN};
#else
    AGpsStatus status = {AGPS_TYPE_SUPL,GPS_RELEASE_AGPS_DATA_CONN};
#endif

    if (!supl) {
        LOGE("Error initializing supl");
        return;
    }
    if (!s->agps_callbacks.status_cb)
        return;
    s->agps_callbacks.status_cb(&status);

#ifdef CONFIG_LCS_NI_SESSION_NOTIF
    if (s->rcv_ni_msg)
    {
        s->rcv_ni_msg = 0;
        gps_engine_status_update(GPS_STATUS_SESSION_END);	
    }
#endif
	
    DI("%s: successfully called",__FUNCTION__);
}

/// Registers the callback functions given from the framework.
static void assist_gps_init( AGpsCallbacks* callbacks )
{
    GpsState *s = _gps_state;
    s->agps_callbacks = *callbacks;
}

/// This function internally initializes a SUPL handler only after making sure that LCS socket is connected.
static int assist_gps_init_internal()
{
    GpsState *s = _gps_state;
    BrcmLbsSupl_Callbacks supl_cbs = {
        .onSuplReqNotifVerif = on_notification_ref,
        .onSuplReqRefLocation = on_request_ref_location,
        .onSuplReqConnection = on_request_connection,
        .onSuplSessionEnded = on_supl_session_ended
    };
    if (!s->lcsh) {
        LOGW("LCS socket GPS driver is not yet initialized by GPS daemon. %s() will be retried later.", __FUNCTION__);
        return -1;
    }
    if (!(s->suplh = BrcmLbs_getSuplHandle(s->lcsh))) {
        LOGE("Error initializing supl");
        return -1;
    }
    BrcmLbsSupl_registerCallbacks(s->suplh, &supl_cbs, NULL);
    if (s->suplhost[0]) {
        if (BrcmLbsSupl_setServerInfo(s->suplh,
                    s->suplhost,s->suplport) != BRCM_LBS_OK)
            LOGE("Error setting the supl server");
    }
	return 0;
}

static int assist_gps_data_conn_open( const char* apn )
{
    GpsState *s = _gps_state;
    int session_nr;
    if (!(s->suplh)) {
        LOGE("Supl handler was not initialized");
        return -1;
    }
    if (apn != NULL) {
        LOGD("DeviceName or APN : (%s)", apn);
        if (BrcmLbsSupl_setDeviceName(s->suplh, apn) != BRCM_LBS_OK) {
            LOGW("Error setDeviceName : (%s)", apn);
        }
    }
    else {
        LOGD("DeviceName or APN is NULL");
        return -1;
    }
    if ((session_nr=get_request_session(&s->conn_pending)) >= 0) {
        if (BrcmLbsSupl_connectionEstablished(s->suplh, session_nr) != BRCM_LBS_OK) {
            LOGW("Error reporting connection established");
        }
    }
    return 0;
}

static int assist_gps_data_conn_closed(void)
{
	return 0;
}

static int assist_gps_data_conn_failed(void)
{
	return 0;
}

static int assist_gps_set_server( AGpsType type, const char* hostname, int port )
{
    GpsState *s = _gps_state;
    strncpy(s->suplhost,hostname,sizeof(s->suplhost));
    s->suplhost[sizeof(s->suplhost)-1]='\0';
    s->suplport=port;
    if (!s->suplh)
        return -1;
    if (BrcmLbsSupl_setServerInfo(s->suplh,hostname,port) != BRCM_LBS_OK) {
        LOGE("Error setting the supl server");
        return -1;
    }
    return 0;
}

static int gps_plus_init( GpsXtraCallbacks* callbacks )
{
	return 0;
}

static int gps_plus_inject_data( char* data, int length )
{
	return 0;
}


static void fact_test_init (GpsFactTestCallbacks *callbacks)
{
    GpsState *s = _gps_state;
    if (callbacks)
        s->ft_callbacks = *callbacks;
}

static void fact_test_start (GpsFactTestParams *params, int duration_sec)
{
    GpsState *s = _gps_state;
    /*
    // deprecated use combox instead now
    if (!(s->rqh=BrcmLbsClient_requestFactTest(s->gpsh,params->TestMode,params->TestItems,
        params->prn,params->AvgIntrvlSec,duration_sec,
       on_facttest_result,(void *)s)))
    */
#ifdef GLONASS_GPS_CW_SUPPORT
    s->glonass = 1;
#else
    s->glonass = 0;
#endif   
     // in order to provide custom FCN changes need to be made in GpsFactTestParams 
     if (!(s->rqh= BrcmLbsClient_requestFactTestCombo(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_CN0,
                             (!s->glonass)?params->prn:BRCM_UNASSIGNED_PRN,
                             (s->glonass)?0:BRCM_UNASSIGNED_FCN,
                             params->AvgIntrvlSec,duration_sec, on_facttest_result_r,(void *)s)))
            D("%s(%d): fact test failed errno(%d)",__FUNCTION__,__LINE__,errno);
    else
            D("%s(%d): fact test success",__FUNCTION__,__LINE__);
}

#ifdef AGPS_RIL_INTERFACE
static void assist_ril_init(AGpsRilCallbacks *callbacks)
{
    GpsState *s = _gps_state;
    s->agps_ril_callbacks = *callbacks;
}

static void assist_ril_ref_location(const AGpsRefLocation *agps_reflocation, size_t sz)
{
    GpsState *s = _gps_state;
    BrcmLbsSupl_LocationId refLoc;
    int session_nr;
#ifdef GPS_AS_MODULE
    AGpsRefLocationCellID cellid = (AGpsRefLocationCellID)(agps_reflocation->u.cellID);
#else
    AGpsRefLocationCellID *cellid = (AGpsRefLocationCellID *)agps_reflocation;
#endif
    if (!s->suplh) {
        LOGW("Supl was not initialized");
        return;
    }
    if (!agps_reflocation) {
        LOGE("no ref. location");
        return;
    }
    if ((session_nr = get_request_session(&s->refloc_pending)) < 0) {
        LOGW("There is no supl session waiting for ref. location");
        return;
    }

    refLoc.status = BRCM_LBS_STATUS_CURRENT;
    switch (agps_reflocation->type) {
        case AGPS_REF_LOCATION_TYPE_GSM_CELLID:
#ifdef GPS_AS_MODULE
            if (sz != sizeof(AGpsRefLocation)) {
#else
            if (sz != sizeof(AGpsRefLocationCellID)) {
#endif
                LOGE("size mismatch on cell id argument");
                return;
            }
            refLoc.cellInfo.network_type = BRCM_LBS_CELL_INFO_GSM;
            refLoc.cellInfo.u.gsmCell.m.aRFCNPresent = 0;
            refLoc.cellInfo.u.gsmCell.m.bSICPresent = 0;
            refLoc.cellInfo.u.gsmCell.m.rxLevPresent = 0;
#ifdef GPS_AS_MODULE
            refLoc.cellInfo.u.gsmCell.refMCC = cellid.mcc;
            refLoc.cellInfo.u.gsmCell.refMNC = cellid.mnc;
            refLoc.cellInfo.u.gsmCell.refLAC = cellid.lac;
            refLoc.cellInfo.u.gsmCell.refCI  = cellid.cid;
#else
            refLoc.cellInfo.u.gsmCell.refMCC = cellid->mcc;
            refLoc.cellInfo.u.gsmCell.refMNC = cellid->mnc;
            refLoc.cellInfo.u.gsmCell.refLAC = cellid->lac;
            refLoc.cellInfo.u.gsmCell.refCI  = cellid->cid;
#endif           
            break;
        case AGPS_REF_LOCATION_TYPE_UMTS_CELLID:
#ifdef GPS_AS_MODULE
            if (sz != sizeof(AGpsRefLocation)) {
#else
            if (sz != sizeof(AGpsRefLocationCellID)) {
#endif
                LOGE("size mismatch on cell id argument");
                return;
            }
            refLoc.cellInfo.network_type = BRCM_LBS_CELL_INFO_WCDMA;
            refLoc.cellInfo.u.wcdmaCell.m.frequencyInfoPresent = 0;
            refLoc.cellInfo.u.wcdmaCell.m.cellMeasuredResultPresent = 0;
#ifdef GPS_AS_MODULE
            refLoc.cellInfo.u.wcdmaCell.refMCC = cellid.mcc;
            refLoc.cellInfo.u.wcdmaCell.refMNC = cellid.mnc;
            refLoc.cellInfo.u.wcdmaCell.refUC  = cellid.cid;
#else
            refLoc.cellInfo.u.wcdmaCell.refMCC = cellid->mcc;
            refLoc.cellInfo.u.wcdmaCell.refMNC = cellid->mnc;
            refLoc.cellInfo.u.wcdmaCell.refUC  = cellid->cid;
#endif
            break;
        default:
            LOGW("Ref location: %d not supported",agps_reflocation->type);
            return;
    }
    
    if (BrcmLbsSupl_RefLocationResponse(s->suplh,session_nr,&refLoc) != BRCM_LBS_OK) {
        LOGE("Error sending ref. location for session %d",session_nr);
    }
}

static void assist_ril_set_id(AGpsSetIDType type, const char *setid)
{
    GpsState *s = _gps_state;
    BrcmLbsSupl_SetID sid;
    if (!s->suplh) {
        LOGW("Supl was not initialized");
        return;
    }
    if (!setid) {
        LOGE("Not a valid setid");
        return;
    }
    switch(type) {
        case AGPS_SETID_TYPE_IMSI:
            sid.eType = BRCM_LBS_ID_IMSI;
            ascii2bcd(setid,sid.u.imsi,sizeof(sid.u.imsi));
            break;
        case AGPS_SETID_TYPE_MSISDN:
            sid.eType = BRCM_LBS_ID_MSISDN;
            ascii2bcd(setid,sid.u.msisdn,sizeof(sid.u.msisdn));
            break;
        case AGPS_SETID_TYPE_NONE:
            LOGW("The phone may not have a SIM card.");
			return;
        default:
            LOGE("Not a valid type as a set id");
            return;
    }
    if (BrcmLbsSupl_setID(s->suplh,&sid) != BRCM_LBS_OK) {
        LOGE("Error sending set id");
    }
    s->setid_type = type;

}
#endif

static void assist_ril_send_ni_message(uint8_t *msg, size_t sz)
{
    GpsState *s = _gps_state;
    if (!s->suplh) {
        LOGW("Supl was not initialized");
        return;
    }
#ifdef AGPS_RIL_INTERFACE
    LOGD("%s: NI msg, request_setid", __FUNCTION__);
    check_and_request_setid(s);
#endif
    if (BrcmLbsSupl_networkInitiatedRequest(s->suplh,msg,sz,"") != BRCM_LBS_OK)
    {
        LOGE("Error sending ni message");
    }
#ifdef CONFIG_LCS_NI_SESSION_NOTIF
    else
    {
        s->rcv_ni_msg = 1;
        gps_engine_status_update(GPS_STATUS_SESSION_BEGIN);
    }
#endif
}

#ifdef GPS_AS_MODULE
static void assist_ril_update_network_state(int connected, int type, int roaming, const char* extra_info)
{
    return;
}
#endif

static const GpsXtraInterface sGpsPlusInterface = 
{ 
#ifdef GPS_AS_MODULE
    .size =                   sizeof(GpsXtraInterface), 
#endif
	.init =						gps_plus_init, 
	.inject_xtra_data =			gps_plus_inject_data,
};

static const AGpsInterface sAssistGpsInterface =
{
#ifdef GPS_AS_MODULE
    .size =                     sizeof(AGpsInterface),
#endif
    .init =                     assist_gps_init,
    .data_conn_open =           assist_gps_data_conn_open,
    .data_conn_closed =         assist_gps_data_conn_closed,
    .data_conn_failed =         assist_gps_data_conn_failed,
    .set_server =               assist_gps_set_server,
};

static const GpsNiInterface  sGpsNiInterface = {
#ifdef GPS_AS_MODULE
    .size =                     sizeof(GpsNiInterface),
#endif
    .init =                     gps_ni_init,
    .respond =                	gps_ni_respond,
};

#ifdef AGPS_RIL_INTERFACE
static const AGpsRilInterface sAssistGpsRilInterface =
{
#ifdef GPS_AS_MODULE
    .size =                 sizeof(AGpsRilInterface),
#endif
    .init =   assist_ril_init,
    .set_ref_location = assist_ril_ref_location,
    .set_set_id =     assist_ril_set_id,
    .ni_message =     assist_ril_send_ni_message,
#ifdef GPS_AS_MODULE
    .update_network_state = assist_ril_update_network_state,
#endif
};
#endif

static const GpsFactTestInterface sGpsFactTestInterface =
{
        .init =                 fact_test_init,
        .test_start =           fact_test_start
};

static void gps_ni_init(GpsNiCallbacks *callbacks)
{
    GpsState*  s = _gps_state;
    s->ni_callbacks = *callbacks;
}

static void gps_ni_send_respond(int notif_id, GpsUserResponseType user_response)
{
    GpsState*  s = _gps_state;
	int session_nr = 0;
    // add BRCM interface here.
    if (!s->suplh) {
        D("No supl handler");
        return;
    }

    D("%s: entry (user_response = %d)", __FUNCTION__, user_response);

	if ((session_nr = get_request_session(&s->notif_pending)) >= 0) {
	    switch(user_response) {
			case GPS_NI_RESPONSE_ACCEPT:
				BrcmLbsSupl_verificationResponse(s->suplh, session_nr, 1);
				break;
			case GPS_NI_RESPONSE_DENY:
				BrcmLbsSupl_verificationResponse(s->suplh, session_nr, 0);
				break;
			case GPS_NI_RESPONSE_NORESP:
				D("No response from User");
				break;
			default:
				break;
	    }
	} else {
		D("%s: no session id (session_nr = %d)", __FUNCTION__, session_nr);
	}
}

static void gps_ni_respond(int notif_id, GpsUserResponseType user_response)
{
    timeout_action_del(ta_list,_SIZE(ta_list),notif_id);
    gps_ni_send_respond(notif_id, user_response);
}

static void gps_ni_timeout_response(int id, void *data)
{
    GpsUserResponseType user_response = (GpsUserResponseType) data;
    gps_ni_send_respond(id, user_response);
}

const void* gps_get_extension(const char* name)
{
	if (strcmp(name, GPS_XTRA_INTERFACE) == 0) {
		return NULL;
	}
	
	if (strcmp(name, AGPS_INTERFACE) == 0) {
		return &sAssistGpsInterface;
	}
        if (strcmp(name,GPS_FACT_TEST_INTERFACE) == 0) {
                return &sGpsFactTestInterface;
        }
    if (strcmp(name, GPS_NI_INTERFACE) == 0) {
        return &sGpsNiInterface;
    }
#ifdef AGPS_RIL_INTERFACE
        if (strcmp(name,AGPS_RIL_INTERFACE) == 0) {
            return &sAssistGpsRilInterface;
        }
#endif

	return NULL;
}

static const GpsInterface  _GpsInterface = {
#ifdef GPS_AS_MODULE
    .size =                     sizeof(GpsInterface),
#endif
    .init = 					gps_init,
    .start =					gps_start,
    .stop =					    gps_stop,
    .cleanup =				    gps_cleanup,
    .inject_time =			    gps_inject_time,
    .inject_location =		    gps_inject_location,
    .delete_aiding_data =		gps_delete_aiding_data,
    .set_position_mode =		gps_set_position_mode,
    .get_extension = 			gps_get_extension,
};

static void on_position(BrcmLbs_ReqHandle rh, const BrcmLbs_PosInfo *posinfo,
        BrcmLbs_UserData  arg)
{
    GpsState *s = (GpsState *) arg;
    GpsLocation loc;
    GpsSvStatus svstatus;
    int brcm_sv;
    int num_svs;
#ifdef GPS_AS_MODULE
    loc.size = sizeof(GpsLocation);
#endif
    loc.flags = 0;
	s->posValid = (int)posinfo->location.posValid;
    if (posinfo->location.posValid)
        loc.flags |= GPS_LOCATION_HAS_LAT_LONG;
    if (posinfo->location.m.altitudePresent)
        loc.flags |= GPS_LOCATION_HAS_ALTITUDE;
    if (posinfo->location.m.speedPresent)
        loc.flags |= GPS_LOCATION_HAS_SPEED;
    if (posinfo->location.m.bearingPresent)
        loc.flags |= GPS_LOCATION_HAS_BEARING;
    if (posinfo->location.m.horAccuracyPresent)
        loc.flags |= GPS_LOCATION_HAS_ACCURACY;
    loc.latitude = posinfo->location.latitude;
    loc.longitude = posinfo->location.longitude;
    loc.altitude = posinfo->location.altitude;
    loc.speed = posinfo->location.speed;
    loc.accuracy = posinfo->location.horAccuracy;
    loc.bearing = posinfo->location.bearing;
    loc.timestamp =utc_to_epoch_ms(&posinfo->location.utcTime);
    update_last_fix_time(s,&posinfo->location.utcTime);

    if ( (loc.flags & GPS_LOCATION_HAS_LAT_LONG) &&
                    (loc.flags & GPS_LOCATION_HAS_ALTITUDE) &&
//                  (loc.flags & GPS_LOCATION_HAS_SPEED) &&
//                   (loc.flags & GPS_LOCATION_HAS_BEARING) &&
                    (loc.flags & GPS_LOCATION_HAS_ACCURACY)) {
        if (s->callbacks.location_cb)
            s->callbacks.location_cb(&loc);
    }
    num_svs=_MIN(posinfo->svNum,
                          _MIN((int)_SIZE(svstatus.sv_list),(int)_SIZE(posinfo->svInfo)));
#ifdef GPS_AS_MODULE
    svstatus.size = sizeof(GpsSvStatus);
#endif
    svstatus.ephemeris_mask = 0;
    svstatus.almanac_mask = 0;
    svstatus.used_in_fix_mask = 0;
    for (brcm_sv=0,svstatus.num_svs=0;brcm_sv<num_svs;brcm_sv++) {
        GpsSvInfo *svinfo = &svstatus.sv_list[svstatus.num_svs];
        const BrcmLbs_SvInfo *brcm_svinfo = &posinfo->svInfo[brcm_sv];
        uint32_t sv_mask = 0;
#ifndef CONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS       
        if (brcm_svinfo->sSvId > 32)
            continue;
#endif            
        if (!brcm_svinfo->sCNo)
            continue;
        svstatus.num_svs ++;
#ifdef GPS_AS_MODULE
        svinfo->size = sizeof(GpsSvInfo);
#endif
        svinfo->prn = brcm_svinfo->sSvId;
        svinfo->snr = brcm_svinfo->sCNo;
        svinfo->elevation = brcm_svinfo->sElev;
        svinfo->azimuth = brcm_svinfo->sAz;
#ifdef CONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS
#ifdef CONFIG_LCS_ALL_CONSTELLATIONS_IN_USE_MASK
	if (brcm_sv <= 32)
        {
#else
        if (brcm_svinfo->sSvId <= 32)
        {
#endif
#endif        

#ifdef CONFIG_LCS_ALL_CONSTELLATIONS_IN_USE_MASK
        sv_mask = 1 << brcm_sv;
#else
        sv_mask = 1 << (brcm_svinfo->sSvId-1);
#endif
        if (brcm_svinfo->bHasEph)
            svstatus.ephemeris_mask |= sv_mask;
//        svstatus.almanac_mask |= sv_mask;
        if (brcm_svinfo->bUsed)
            svstatus.used_in_fix_mask |= sv_mask;
#ifdef CONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS
        }
#endif        

    }
    D("on_position used_in_fix_mask:%x", svstatus.used_in_fix_mask);
    D("on_position num_svs:%x", svstatus.num_svs);

    if (s->callbacks.sv_status_cb)
        s->callbacks.sv_status_cb(&svstatus);
}

static void on_nmea (BrcmLbs_ReqHandle rh, const char* nmea, OsUint16 size, BrcmLbs_UserData userData)
{
    GpsState *s = (GpsState *) userData;
    if (s->callbacks.nmea_cb)
    {		
        GpsUtcTime timestamp;
        timestamp = estimate_gpstime(s);
#ifdef CONFIG_HAL_SUPLLOG		
        if(!(s->posValid))
        {
            //some platforms might want to request to set timestamp to 0 in case of no position fix
            timestamp = 0;
        }
#endif	
        s->callbacks.nmea_cb(timestamp,nmea,size); // Same as Android
    }
}

static void update_last_fix_time(GpsState *s,const BrcmLbs_UtcTime *fix_time)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    s->last_fix_system_time = (GpsUtcTime) tv.tv_sec*1000+tv.tv_usec/1000;
    if (!fix_time)
        s->last_fix_gps_time = (GpsUtcTime) tv.tv_sec*1000+tv.tv_usec/1000;
    else
        s->last_fix_gps_time = utc_to_epoch_ms(fix_time);
}

static GpsUtcTime estimate_gpstime(GpsState *s)
{
    GpsUtcTime system_time;
    GpsUtcTime diff_time;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    system_time = (GpsUtcTime) tv.tv_sec*1000 + tv.tv_usec/1000;
    diff_time = system_time - s->last_fix_system_time;
    return s->last_fix_gps_time + diff_time;
}

static void on_stop(BrcmLbs_ReqHandle rh, BrcmLbs_UserData userData)
{
    GpsState *s = (GpsState *) userData;

    gps_engine_status_update(GPS_STATUS_SESSION_END);
}

static void on_status(BrcmLbs_GpsStatus status, BrcmLbs_UserData userData)
{
    GpsState *s = (GpsState *) userData;
    s->engine_status = status;
    
    switch(status) {
        case BRCM_LBS_GPS_ENGINE_ON:
            gps_engine_status_update(GPS_STATUS_ENGINE_ON);
            break;
        case BRCM_LBS_GPS_ENGINE_OFF:
            gps_engine_status_update(GPS_STATUS_ENGINE_OFF);
            break;
        default:
            break;
    }
}

static void on_start(BrcmLbs_ReqHandle rh, BrcmLbs_Result result,
                        BrcmLbs_UserData userData)
{
    GpsState *s = (GpsState *) userData;
    if (result != BRCM_LBS_OK) {
        D("Received error on start (%d)",result);
        return;
    }

    gps_engine_status_update(GPS_STATUS_SESSION_BEGIN);
}

static void on_facttest_result(BrcmLbs_ReqHandle rh,
                 const BrcmLbs_FactTestInfo *result, BrcmLbs_UserData userData)
{
    GpsState *s = (GpsState *) userData;
    GpsFactTestResponse ft_response;
    switch(result->TestItem) {
        case BRCM_LBS_FACT_TEST_CW:
            ft_response.TestItem = GPS_FACT_TEST_CW;
            break;
        case BRCM_LBS_FACT_TEST_CN0:
            ft_response.TestItem = GPS_FACT_TEST_CN0;
            break;
        case BRCM_LBS_FACT_TEST_FRQ:
            ft_response.TestItem = GPS_FACT_TEST_FRQ;
            break;
        case BRCM_LBS_FACT_TEST_WER:
            ft_response.TestItem = GPS_FACT_TEST_WER;
            break;
        case BRCM_LBS_FACT_TEST_ACQ:
            ft_response.TestItem = GPS_FACT_TEST_ACQ;
            break;
    }
    ft_response.SVid = result->SVid;
    ft_response.EnergyFound = result->EnergyFound;
    ft_response.AvgValid = result->AvgValid;
    ft_response.LastSSdBm= result->LastSSdBm;
    ft_response.AvgSSdBm = result->AvgSSdBm;

    ft_response.LastSNRdbHZ= result->LastSNRdbHZ;
    ft_response.AvgSNRdBHZ = result->AvgSNRdBHZ;


    ft_response.NoiseFigure = result->NoiseFigure;

    ft_response.FrequencyMeasured = result->FrequencyMeasured;
    ft_response.LastFreqPpu    = result->LastFreqPpu;
    ft_response.LastFreqUncPpu = result->LastFreqUncPpu;

    ft_response.WerMeasured = result->WerMeasured;
    ft_response.GoodWordCnt = result->GoodWordCnt;
    ft_response.TotalWordCnt= result->TotalWordCnt;

    switch(result->CntInStatus) {
        case BRCM_LBS_CNTIN_NOT_USED:
            ft_response.CntInStatus= GPS_CNTIN_NOT_USED;
            break;
        case BRCM_LBS_CNTIN_OK:
            ft_response.CntInStatus= GPS_CNTIN_OK;
            break;
        case BRCM_LBS_CNTIN_NOK:
            ft_response.CntInStatus= GPS_CNTIN_NOK;
            break;
        case BRCM_LBS_CNTIN_NO_INPUT:
            ft_response.CntInStatus= GPS_CNTIN_NO_INPUT;
            break;
        case BRCM_LBS_CNTIN_USER_DEAD:
            ft_response.CntInStatus= GPS_CNTIN_USER_DEAD;
            break;
        case BRCM_LBS_CNTIN_USER_CANCEL:
            ft_response.CntInStatus= GPS_CNTIN_USER_CANCEL;
            break;
    }
    ft_response.CntinMeasured= result->CntinMeasured;
    ft_response.CntinOffsetPpu= result->CntinOffsetPpu;


    ft_response.RtcMeasured = result->RtcMeasured;
    ft_response.RtcOffsetPpu= result->RtcOffsetPpu;

    ft_response.RfAgc = result->RfAgc;

    ft_response.SignalDropCount= result->SignalDropCount;

    ft_response.ValidCW= result->ValidCW;
    ft_response.DopplerPpuCW= result->DopplerPpuCW;
    ft_response.DopplerUncPpuCW= result->DopplerUncPpuCW;
    ft_response.SSdBmCW= result->SSdBmCW;
    if (s->ft_callbacks.result_cb)
        s->ft_callbacks.result_cb(&ft_response);
}

static void gps_engine_status_update(GpsStatusValue update)
{
    GpsState*  s = _gps_state;
    GpsStatus gpsstatus;
    D("gps_engine_status_update %d", update);
#ifdef GPS_AS_MODULE
    gpsstatus.size = sizeof(GpsStatus);
#endif
    gpsstatus.status = update;
    if (s->callbacks.status_cb)
        s->callbacks.status_cb(&gpsstatus);
#ifdef GPS_AS_MODULE
    if (update == GPS_STATUS_ENGINE_ON) 
    {
        s->callbacks.acquire_wakelock_cb();
        DI("%s: called acquire_wakelock_cb", __FUNCTION__);
    } else if (update == GPS_STATUS_ENGINE_OFF) 
    {
        s->callbacks.release_wakelock_cb();
        DI("%s: called release_wakelock_cb", __FUNCTION__);
    }
#endif
}


static void on_facttest_result_r(BrcmLbs_ReqHandle rh,
                 const BrcmLbs_FactTestInfo *result, BrcmLbs_UserData userData)
{
    GpsState *s = (GpsState *) userData;
    GpsSvStatus svstatus;
    GpsSvInfo *svinfo = &svstatus.sv_list[0];

#ifdef GPS_AS_MODULE  
    svstatus.size = sizeof(GpsSvStatus);
#endif  
    svstatus.num_svs=1;
    svstatus.ephemeris_mask = 0;
    svstatus.almanac_mask = 0;
    svstatus.used_in_fix_mask = 0;

#ifdef GPS_AS_MODULE  
    svinfo->size = sizeof(GpsSvInfo);
#endif
    svinfo->prn = result->SVid;
    if (result->EnergyFound)
        svinfo->snr = result->LastSNRdbHZ;
    else
        svinfo->snr = 0;

    svinfo->elevation = 0;
    svinfo->azimuth = 0;
    if (s->callbacks.sv_status_cb)
        s->callbacks.sv_status_cb(&svstatus);
}

static void ascii2bcd(const char *asciinum, unsigned char *bcd, size_t bcd_sz)
{
    unsigned int i;
    for (i=0; i < bcd_sz*2;i++) {
        unsigned char n=0;
        if (*asciinum) {
            n = *asciinum-'0';
            asciinum++;
        }
        else {
            n = 0xf;
        }

        if (i%2) {
            bcd[i/2] = (n<<4) | (bcd[i/2]&0xF);
        }
        else
            bcd[i/2] = n;
    }

}


#ifdef GPS_AS_MODULE
const GpsInterface* gps_get_hardware_interface(struct gps_device_t* dev)
#else
const GpsInterface* gps_get_hardware_interface(void)
#endif
{
	return &_GpsInterface;
}


#ifdef GPS_AS_MODULE
static int open_gps(const struct hw_module_t* module, char const* name,
                    struct hw_device_t** device)
{
    struct gps_device_t *gps_device = malloc(sizeof(struct gps_device_t));
    if (gps_device)
    {
        memset(gps_device, 0, sizeof(struct gps_device_t));
        gps_device->common.tag        = HARDWARE_DEVICE_TAG;
        gps_device->common.version    = 0;
        gps_device->common.module     = (struct hw_module_t*)module;
        gps_device->get_gps_interface = gps_get_hardware_interface;

        *device = (struct hw_device_t*)gps_device;

        return 0;
    }

    return 1; 
}

static struct hw_module_methods_t hw_module_methods = {
    .open = open_gps
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = GPS_HARDWARE_MODULE_ID,
    .name = "lcsapi GPS Module",
    .author = "Broadcom Corporation",
    .methods = &hw_module_methods,
};
#endif
