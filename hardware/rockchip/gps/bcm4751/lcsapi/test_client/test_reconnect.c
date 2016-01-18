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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>
#include <time.h>

#include <lbs.h>
#include <gps_api.h>
#include <supl.h>


#define LOGE(...) do {fprintf(stderr,__VA_ARGS__); fprintf(stderr,"\n"); } while(0)
#define LOGD(...) LOGE(__VA_ARGS__)
#define LOGW(...) LOGE(__VA_ARGS__)

#define  D(...)   LOGD(__VA_ARGS__)

#define _SIZE(x) (sizeof(x)/sizeof(x[0]))
#define _MIN(a,b) (a>b?b:a)


#define BRCM_LOCAL_PATH "/tmp/lcs.socket"


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))



typedef enum {
        GPS_STATUS_ENGINE_OFF,
        GPS_STATUS_ENGINE_ON,
        GPS_STATUS_SESSION_BEGIN,
        GPS_STATUS_SESSION_END
} GpsStatusValue;

typedef int AGpsType;

/*****************************************************************/
/*****                                                       *****/
/*****       C O N N E C T I O N   S T A T E                 *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

/* commands sent to the gps thread */
enum {
    CMD_QUIT  = 0,
    CMD_START = 1,
    CMD_STOP  = 2
};

struct request_pending {
    size_t n;
    int requests_id[10];
};

int put_request_session(struct request_pending *pend,int session_nr)
{
    unsigned int i;
    if (pend->n >= ARRAY_SIZE(pend->requests_id)) {
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
    if (pend->n > ARRAY_SIZE(pend->requests_id))
        return -1;
    return pend->requests_id[--pend->n];
}

typedef enum {
        GPS_POSITION_MODE_STANDALONE,
        GPS_POSITION_MODE_MS_BASED,
        GPS_POSITION_MODE_MS_ASSISTED
} GpsPositionMode;


/* this is the state of our connection to the qemu_gpsd daemon */
typedef struct {
    int                     init;
    int                     fd;
    pthread_t               thread;
    int                     control[2];
    int                     fix_frequency; 
    char                    *local_path;
	/// LCS handler. This is the handler for LCS socket GPS driver.
    OsHandle                lcsh;
    OsHandle                gpsh;
    OsHandle                suplh;
    BrcmLbs_ReqHandle       rqh;
    struct request_pending  conn_pending;
    struct request_pending  refloc_pending;
    struct request_pending  notif_pending;
    GpsPositionMode         mode;

} GpsState;

static GpsState  _gps_state[1] = {
    {
        init:   0
    }
};





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
static void gps_state_init( GpsState*  state );
static int  assist_gps_init_internal();
#ifdef AGPS_RIL_INTERFACE
static void check_and_request_setid(GpsState *s);
#endif // AGPS_RIL_INTERFACE

static void gps_control_thread_done(GpsState *s)
{
    char   cmd = CMD_QUIT;
    void*  dummy;

    if (s->control[0]<0)
        return;

    write( s->control[0], &cmd, 1 );
    pthread_join(s->thread, &dummy);

    // close the control socket pair
    close( s->control[0] ); s->control[0] = -1;
    close( s->control[1] ); s->control[1] = -1;
}

static void
gps_state_done( GpsState*  s )
{
    if (s->gpsh)
        BrcmLbsGps_deinit(s->gpsh);
    if (s->lcsh)
        BrcmLbs_deinit(s->lcsh);
    s->gpsh=NULL;
    s->lcsh=NULL;
    s->init = 0;
}


static void
gps_state_start( GpsState*  s )
{
    char  cmd = CMD_START;
    int   ret;

    do { ret=write( s->control[0], &cmd, 1 ); }
    while (ret < 0 && errno == EINTR);
    
    if (ret != 1)
        D("%s: could not send CMD_START command: ret=%d: %s",
          __FUNCTION__, ret, strerror(errno));
}


static void
gps_state_stop( GpsState*  s )
{
    char  cmd = CMD_STOP;
    int   ret;

    do { ret=write( s->control[0], &cmd, 1 ); }
    while (ret < 0 && errno == EINTR);

    if (ret != 1)
        D("%s: could not send CMD_STOP command: ret=%d: %s",
          __FUNCTION__, ret, strerror(errno));
}


static int
epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int           ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    } while (ret < 0 && errno == EINTR);
    return ret;
}


static int
epoll_deregister( int  epoll_fd, int  fd )
{
    int  ret;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd, NULL );
    } while (ret < 0 && errno == EINTR);
    return ret;
}

static void set_close_on_exec(int fd)
{
    int flags;
    if (fd < 0)
        return;
    flags = fcntl(fd,F_GETFD);
    fcntl(fd,F_SETFD,flags | FD_CLOEXEC);
}


/* this is the main thread, it waits for commands from gps_state_start/stop and,
 * when started, messages from the QEMU GPS daemon. these are simple NMEA sentences
 * that must be parsed to be converted into GPS fixes sent to the framework
 */
static void*
gps_state_thread( void*  arg )
{
    GpsState*   state = (GpsState*) arg;
    int         epoll_fd   = epoll_create(2);
    int         control_fd = state->control[1];

    // register control file descriptors for polling
    epoll_register( epoll_fd, control_fd );

    D("gps thread running");

    // now loop
    for (;;) {
        struct epoll_event   events[2];
        int                  ne, nevents;

        if (!state->init) {
            gps_state_init(state);
            if (state->fd < 0) {
                sleep(1);
                continue;
            }
            epoll_register(epoll_fd, state->fd);
        }

        nevents = epoll_wait( epoll_fd, events, 2, -1 );
        if (nevents < 0) {
            if (errno != EINTR)
                LOGE("epoll_wait() unexpected error: %s", strerror(errno));
            continue;
        }
        D("gps thread received %d events", nevents);
        for (ne = 0; ne < nevents; ne++) {
            if ((events[ne].events & (EPOLLERR|EPOLLHUP)) != 0) {
                LOGE("EPOLLERR or EPOLLHUP after epoll_wait() !?");
                epoll_deregister(epoll_fd, state->fd);
                gps_state_done(state);
                continue;
            }
            if ((events[ne].events & EPOLLIN) != 0) {
                int  fd = events[ne].data.fd;

                if (fd == control_fd)
                {
                    char  cmd = 255;
                    int   ret;
                    D("gps control fd event");
                    do {
                        ret = read( fd, &cmd, 1 );
                    } while (ret < 0 && errno == EINTR);

                    if (cmd == CMD_QUIT) {
                        D("gps thread quitting on demand");
                        goto Exit;
                    }
                }
                else if (fd == state->fd)
                {
                    D("gps fd event");
                    BrcmLbs_processMessages(state->lcsh, BRCMLBS_RX);
                }
                else
                {
                    LOGE("epoll_wait() returned unkown fd %d ?", fd);
                }
            }
        }
    }
Exit:
    gps_state_done(state);
    return NULL;
}


static void
gps_control_thread_init(GpsState *state)
{
    state->control[0] = -1;
    state->control[1] = -1;

    if ( socketpair( AF_LOCAL, SOCK_STREAM, 0, state->control ) < 0 ) {
        LOGE("could not create thread control socket pair: %s", strerror(errno));
        goto Fail;
    }

    if ( pthread_create( &state->thread, NULL, gps_state_thread, state ) != 0 ) {
        LOGE("could not create gps thread: %s", strerror(errno));
        goto Fail;
    }
    return;
Fail:
    if (state->control[0] >= 0)
        close(state->control[0]);
    if (state->control[1] >= 0)
        close(state->control[1]);
    state->control[0]=-1;
    state->control[1]=-1;
}

static void
gps_state_init( GpsState*  state )
{
    state->init       = 1;
    state->control[0] = -1;
    state->control[1] = -1;
    state->fd         = -1;
    state->fix_frequency = 1;
    state->lcsh       = NULL;
    state->gpsh       = NULL;
    state->suplh      = NULL;
    state->local_path = BRCM_LOCAL_PATH;
    

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

    BrcmLbs_registerOnNmea(state->gpsh,on_nmea, state);
    BrcmLbs_registerOnStart(state->gpsh,on_start, state);
    BrcmLbs_registerOnStop(state->gpsh,on_stop, state);
    BrcmLbs_registerOnGpsStatus(state->gpsh, on_status, state);


    D("gps state initialized");
    return;

Fail:
    gps_state_done( state );
}


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       I N T E R F A C E                               *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/


static int
gps_init(void)
{
    GpsState*  s = _gps_state;
    gps_control_thread_init(s);
    

    return 0;
}

static void
gps_cleanup(void)
{
    GpsState*  s = _gps_state;

    gps_control_thread_done(s);
   
}


static int
gps_start()
{
    GpsState*  s = _gps_state;
    BRCM_LBS_SETCapabilities brcm_cap;
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }
#ifdef AGPS_RIL_INTERFACE
    check_and_request_setid(s);
#endif
    switch(s->mode) {
//        case GPS_POSITION_MODE_STANDALONE:
//            if (s->suplh)
//                BrcmLbsSupl_disable(s->suplh);
//            break;
        case GPS_POSITION_MODE_STANDALONE:
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
#if 0
        case 100:   // Sensitivity Test , Factory_Low_SNR
            D("%s(%d): FRQ fact test launched",__FUNCTION__,__LINE__);
            if (BrcmLbsGps_deleteAidingData(s->gpsh,
                        BRCMLBSGPS_AIDING_POS
                        | BRCMLBSGPS_AIDING_EPH
                        | BRCMLBSGPS_AIDING_TIM
                        | BRCMLBSGPS_AIDING_ALM
                        | BRCMLBSGPS_AIDING_ROM_ALM) != BRCM_LBS_OK)
                break;
            s->rqh=BrcmLbsClient_requestFactTest(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_CN0,
                            1,10,-1, on_facttest_result_r,(void *)s);
            break;
        case 101:   //CN0 Test , Factory_High_SNR
            D("%s(%d): CN0 fact test launched",__FUNCTION__,__LINE__);
            if (BrcmLbsGps_deleteAidingData(s->gpsh,
                        BRCMLBSGPS_AIDING_POS
                        | BRCMLBSGPS_AIDING_EPH
                        | BRCMLBSGPS_AIDING_TIM
                        | BRCMLBSGPS_AIDING_ALM
                        | BRCMLBSGPS_AIDING_OSC
                        | BRCMLBSGPS_AIDING_ROM_ALM) != BRCM_LBS_OK)
                break;
            s->rqh=BrcmLbsClient_requestFactTest(s->gpsh,GPS_FACT_TEST_CONT,GPS_FACT_TEST_FRQ,
                            1,10,-1, on_facttest_result_r,(void *)s);
            break;
#endif
        default:
            D("Not a valid mode selected");
            return -1;
    }
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
                s->rqh=BrcmLbsGps_requestSingleLocation(s->gpsh,120,50,
                                on_position,s);
        }
    }
        
    if (s->rqh)
        D("%s(%d): start request success",__FUNCTION__,__LINE__);
    else
        D("%s(%d): start request failed errno(%d)",__FUNCTION__,__LINE__,errno);

    return 0;
}


static int
gps_stop()
{
    GpsState*  s = _gps_state;
    BrcmLbs_Result result;

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

    if ((result = BrcmLbsGps_stopAllRequests(s->gpsh)) != BRCM_LBS_OK)
        D("%s(%d): stop failed error(%d)",__FUNCTION__,__LINE__,result);
    else
        D("%s(%d): stop success",__FUNCTION__,__LINE__);
    D("%s: called", __FUNCTION__);
    return 0;
}

#if 0
static int
gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    GpsState*  s = _gps_state;
    BrcmLbsGps_injectUtcTime(s->gpsh,time,uncertainty);
    return 0;
}
#endif

static int
gps_inject_location(double latitude, double longitude, float accuracy)
{
    GpsState*  s = _gps_state;
    BrcmLbs_Position pos = {
        .m = { .altitudePresent = 0,
               .pos_errorPresent = 1
            },
        .latitude = latitude,
        .longitude = longitude,
        .pos_error = accuracy 
    };
    BrcmLbsGps_injectPosition(s->gpsh,&pos);
    return 0;
}
#if 0
static void
gps_delete_aiding_data(GpsAidingData flags)
{
    GpsState*  s = _gps_state;

    uint16_t mode=0;
    BrcmLbs_Result result;
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
    }
    if (flags&GPS_DELETE_EPHEMERIS)
        mode |= BRCMLBSGPS_AIDING_EPH | BRCMLBSGPS_AIDING_LTO;
    if (flags&GPS_DELETE_ALMANAC)
        mode |= BRCMLBSGPS_AIDING_ALM;
    if (flags&GPS_DELETE_POSITION)
        mode |= BRCMLBSGPS_AIDING_POS;
    if (flags&GPS_DELETE_TIME)
        mode |= BRCMLBSGPS_AIDING_TIM;
    if (flags&GPS_DELETE_ALL)
        mode = BRCMLBSGPS_AIDING_POS | BRCMLBSGPS_AIDING_EPH | BRCMLBSGPS_AIDING_TIM |
            BRCMLBSGPS_AIDING_ALM | BRCMLBSGPS_AIDING_OSC | BRCMLBSGPS_AIDING_ROM_ALM |
            BRCMLBSGPS_AIDING_LTO | BRCMLBSGPS_AIDING_IONO | BRCMLBSGPS_AIDING_UTC |
            BRCMLBSGPS_AIDING_UID | BRCMLBSGPS_AIDING_EEIM;
    if ((result = BrcmLbsGps_deleteAidingData(s->gpsh, mode)) != BRCM_LBS_OK)
        D("%s(%d): delete aiding(%X) failed error(%d)",__FUNCTION__,__LINE__,
                mode,result);
    else
        D("%s(%d): delete aiding(%X) success",__FUNCTION__,__LINE__,mode);

    D("%s: called", __FUNCTION__);
}
#endif

static int gps_set_position_mode(GpsPositionMode mode, int fix_frequency)
{
    // FIXME - support fix_frequency
    // only standalone supported for now.
    GpsState*  s = _gps_state;
    switch (mode) {
        case GPS_POSITION_MODE_STANDALONE:
        case GPS_POSITION_MODE_MS_BASED:
        case GPS_POSITION_MODE_MS_ASSISTED: 
        case 100:   // Sensitivity Test 
        case 101:   // CN0 Test
            s->mode = mode;
            break;
        default:
            D("%s: mode not supported, assuming standalano!",__FUNCTION__);
            s->mode = GPS_POSITION_MODE_STANDALONE;
    }

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }
    s->fix_frequency = fix_frequency;

    D("%s: called", __FUNCTION__);
    return 0;
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
    n_400y = to/400 - from/400;
    n_100y = to/100 - from/100;
    n_4y = to/4 - from/4;
    return (n_4y - n_100y + n_400y);
}


#if 0
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
#endif
#ifdef AGPS_RIL_INTERFACE
static void check_and_request_setid(GpsState *s)
{
    if (s->setid_type != AGPS_SETID_TYPE_NONE)
        return;
    if (!s->agps_ril_callbacks.request_setid)
        return;
     s->agps_ril_callbacks.request_setid(AGPS_SETID_TYPE_IMSI);
}
#endif

static void on_notification_ref(OsHandle supl, int supl_session_id,
        const BrcmLbsSupl_Notification *notif, BrcmLbs_UserData userData)
{
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
    s->agps_ril_callbacks.request_refloc(AGPS_RIL_REQUEST_REFLOC_CELLID); 
}
#else
static void on_request_ref_location(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
}
#endif

void on_request_connection(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
#if 0
    GpsState *s = _gps_state;
    AGpsStatus status = {AGPS_TYPE_SUPL,GPS_REQUEST_AGPS_DATA_CONN};
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
#endif
}

void on_supl_session_ended(OsHandle supl, int supl_session_id, BrcmLbs_UserData userData)
{
#if 0
    GpsState *s = _gps_state;
    AGpsStatus status = {AGPS_TYPE_SUPL,GPS_RELEASE_AGPS_DATA_CONN};
    if (!supl) {
        LOGE("Error initializing supl");
        return;
    }
    if (!s->agps_callbacks.status_cb)
        return;
    s->agps_callbacks.status_cb(&status);
#endif
}

#if 0
/// Registers the callback functions given from the framework.
static void assist_gps_init( AGpsCallbacks* callbacks )
{
    GpsState *s = _gps_state;
    s->agps_callbacks = *callbacks;
}
#endif

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
    if (!(s->suplh)) {
        LOGE("Supl handler was not initialized");
        return -1;
    }
    if (BrcmLbsSupl_setServerInfo(s->suplh,hostname,port) != BRCM_LBS_OK) {
        LOGE("Error setting the supl server");
        return -1;
    }
    return 0;
}

#if 0
static int gps_plus_init( GpsXtraCallbacks* callbacks )
{
	return 0;
}
#endif

static int gps_plus_inject_data( char* data, int length )
{
	return 0;
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
    AGpsRefLocationCellID *cellid = (AGpsRefLocationCellID *)agps_reflocation;
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
            if (sz != sizeof(AGpsRefLocationCellID)) {
                LOGE("size mismatch on cell id argument");
                return;
            }
            refLoc.cellInfo.network_type = BRCM_LBS_CELL_INFO_GSM;
            refLoc.cellInfo.u.gsmCell.m.aRFCNPresent = 0;
            refLoc.cellInfo.u.gsmCell.m.bSICPresent = 0;
            refLoc.cellInfo.u.gsmCell.m.rxLevPresent = 0;
            refLoc.cellInfo.u.gsmCell.refMCC = cellid->mcc;
            refLoc.cellInfo.u.gsmCell.refMNC = cellid->mnc;
            refLoc.cellInfo.u.gsmCell.refLAC = cellid->lac;
            refLoc.cellInfo.u.gsmCell.refCI  = cellid->cid;
            break;
        case AGPS_REF_LOCATION_TYPE_UMTS_CELLID:
            if (sz != sizeof(AGpsRefLocationCellID)) {
                LOGE("size mismatch on cell id argument");
                return;
            }
            refLoc.cellInfo.network_type = BRCM_LBS_CELL_INFO_WCDMA;
            refLoc.cellInfo.u.wcdmaCell.m.frequencyInfoPresent = 0;
            refLoc.cellInfo.u.wcdmaCell.m.cellMeasuredResultPresent = 0;
            refLoc.cellInfo.u.wcdmaCell.refMCC = cellid->mcc;
            refLoc.cellInfo.u.wcdmaCell.refMNC = cellid->mnc;
            refLoc.cellInfo.u.wcdmaCell.refUC  = cellid->cid;
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
    if (BrcmLbsSupl_networkInitiatedRequest(s->suplh,msg,sz,"") != BRCM_LBS_OK)
    {
        LOGE("Error sending ni message");
    }
}

static void on_position(BrcmLbs_ReqHandle rh, const BrcmLbs_PosInfo *posinfo,
        BrcmLbs_UserData  arg)
{
//    GpsState *s = (GpsState *) arg;
    int brcm_sv;
    int num_svs;
//    unsigned int ephemeris_mask = 0;
//    unsigned int almanac_mask = 0;
    unsigned int used_in_fix_mask = 0;
    D("pos: %flat - %flong - %dalt",posinfo->location.latitude,
            posinfo->location.longitude,
            posinfo->location.altitude);
    num_svs=posinfo->svNum;
    for (brcm_sv=0;brcm_sv<num_svs;brcm_sv++) {
        const BrcmLbs_SvInfo *brcm_svinfo = &posinfo->svInfo[brcm_sv];
        uint32_t sv_mask = 0;
        if (brcm_svinfo->sSvId > 32)
            continue;
        sv_mask = 1 << (brcm_svinfo->sSvId-1);
        //svinfo->prn = brcm_svinfo->sSvId;
        //svinfo->snr = brcm_svinfo->sCNo;
        //svinfo->elevation = brcm_svinfo->sElev;
        //svinfo->azimuth = brcm_svinfo->sAz;
//        if (brcm_svinfo->hasEph)
//        svstatus.ephemeris_mask |= sv_mask;
//        svstatus.almanac_mask |= sv_mask;
        if (brcm_svinfo->bUsed)
            used_in_fix_mask |= sv_mask;

    }
    D("on_position used_in_fix_mask:%x",used_in_fix_mask );
    D("on_position num_svs:%x", num_svs);
}

static void on_nmea (BrcmLbs_ReqHandle rh, const char* nmea, OsUint16 size, BrcmLbs_UserData userData)
{
    //GpsState *s = (GpsState *) userData;
    D("NMEA: %*s",size,nmea);
}

static void on_stop(BrcmLbs_ReqHandle rh, BrcmLbs_UserData userData)
{
    //GpsState *s = (GpsState *) userData;

    gps_engine_status_update(GPS_STATUS_SESSION_END);
}


static void on_status(BrcmLbs_GpsStatus status, BrcmLbs_UserData userData)
{
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
#if 0
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
#endif
}

static void gps_engine_status_update(GpsStatusValue update)
{
    D("gps_engine_status_update %d", update);
}


static void on_facttest_result_r(BrcmLbs_ReqHandle rh,
                 const BrcmLbs_FactTestInfo *result, BrcmLbs_UserData userData)
{
#if 0
    GpsState *s = (GpsState *) userData;
    GpsSvStatus svstatus;
    GpsSvInfo *svinfo = &svstatus.sv_list[0];

    svstatus.num_svs=1;
    svstatus.ephemeris_mask = 0;
    svstatus.almanac_mask = 0;
    svstatus.used_in_fix_mask = 0;

    svinfo->prn = result->SVid;
    if (result->EnergyFound)
        svinfo->snr = result->LastSNRdbHZ;
    else
        svinfo->snr = 0;

    svinfo->elevation = 0;
    svinfo->azimuth = 0;
    if (s->callbacks.sv_status_cb)
        s->callbacks.sv_status_cb(&svstatus);
#endif
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


#if 1
int main(int argc, char **argv)
{
    gps_init();
    gps_set_position_mode(GPS_POSITION_MODE_STANDALONE,1000);
    while(1) {
        gps_start();
        sleep(100);
        gps_stop();
        sleep(2);
    }
}
#endif

