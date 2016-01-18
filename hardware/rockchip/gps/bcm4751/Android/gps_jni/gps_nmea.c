///============================================================================
/// Copyright 2007-2009 Broadcom Corporation -- http://www.broadcom.com
/// This program be used, duplicated, modified or distributed
/// pursuant to the terms and conditions of the Apache 2 License.
/// ---------------------------------------------------------------------------
/// \file gps_android.c
/// \for converting NMEA to Android like APIs
///============================================================================

#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <math.h>
#include <time.h>

#define  LOG_TAG  "gps_BRCM"
#include <cutils/log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#ifdef ANDROID23
#include <hardware/gps.h>
#else
#include <hardware_legacy/gps.h>
#endif

#define  GPS_DEBUG   1

#if GPS_DEBUG
#  define  D(...)   LOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

#define BRCM_CONTROL_PIPE "/data/glgpsctrl"
#define BRCM_NMEA_PIPE "/data/gpspipe"


static time_t utc_mktime(struct tm *_tm);

static int irm_send_command(const char *pipe_path, const char *cmd);
static int irm_start_periodic(const char *pipe_path,int period,int fixcount, int validfix, int duration_sec);
static int irm_start_single(const char *pipe_path, int accuracy, int timeout);
static int irm_startup(const char *pipe_path, GpsAidingData flags);
static int irm_stop(const char *pipe_path);

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   T O K E N I Z E R                     *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

typedef struct {
    const char*  p;
    const char*  end;
} Token;


#define TOKEN_LEN(tok)  (tok.end>tok.p?tok.end-tok.p:0)
//#define  MAX_NMEA_TOKENS  16
#define  MAX_NMEA_TOKENS  32

typedef struct {
    int     count;
    Token   tokens[ MAX_NMEA_TOKENS ];
} NmeaTokenizer;

static int
nmea_tokenizer_init( NmeaTokenizer*  t, const char*  p, const char*  end )
{
    int    count = 0;
    char*  q;

    // the initial '$' is optional
    if (p < end && p[0] == '$')
        p += 1;

    // remove trailing newline
    if (end > p && end[-1] == '\n') {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }

    // get rid of checksum at the end of the sentecne
    if (end >= p+3 && end[-3] == '*') {
        end -= 3;
    }

    while (p < end) {
        const char*  q = p;

        q = memchr(p, ',', end-p);
        if (q == NULL)
            q = end;

        if (q > p) {
            if (count < MAX_NMEA_TOKENS) {
                t->tokens[count].p   = p;
                t->tokens[count].end = q;
                count += 1;
            }
        }
        if (q < end)
            q += 1;

        p = q;
    }

    t->count = count;
    return count;
}

static Token
nmea_tokenizer_get( NmeaTokenizer*  t, int  index )
{
    Token  tok;
    static const char*  dummy = "";

    if (index < 0 || index >= t->count) {
        tok.p = tok.end = dummy;
    } else
        tok = t->tokens[index];

    return tok;
}


static int
str2int( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result*10 + c;
    }
    return  result;

Fail:
    return -1;
}

static int
strhex2int( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;
        if ((*p >= 'a') && (*p <= 'f'))
            c = *p - 'a' + 10;
        else if ((*p >= 'A') && (*p <= 'F'))
            c = *p - 'A' + 10;
        else if ((*p >= '0') && (*p <= '9'))
            c = *p - '0';
        else
            goto Fail;

        result = result*0x10 + c;
    }
    return  result;

Fail:
    return -1;
}

static double
str2float( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;
    char  temp[16];

    if (len >= (int)sizeof(temp))
        return 0.;

    memcpy( temp, p, len );
    temp[len] = 0;
    return strtod( temp, NULL );
}

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   P A R S E R                           *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

//#define  NMEA_MAX_SIZE  83
#define  NMEA_MAX_SIZE  160


#define SVINFO_GOT_EPH_INFO_FLAG   0x0001
#define SVINFO_GOT_SV_INFO_FLAG    0x0002
#define SVINFO_GOT_SV_USED_FLAG    0x0002

typedef struct {
    int     pos;
    int     overflow;
    int     utc_year;
    int     utc_mon;
    int     utc_day;
    GpsLocation  fix;
    GpsSvStatus  svstatus;
    int svinfo_flags;
    gps_location_callback  location_callback;
    gps_sv_status_callback  sv_status_callback;
    char    in[ NMEA_MAX_SIZE+1 ];
} NmeaReader;



static void
nmea_reader_init( NmeaReader*  r )
{
    memset( r, 0, sizeof(*r) );

    r->pos      = 0;
    r->overflow = 0;
    r->utc_year = -1;
    r->utc_mon  = -1;
    r->utc_day  = -1;
    r->location_callback = NULL;
    r->sv_status_callback = NULL;
    r->svinfo_flags = 0;
#ifdef ANDROID23
    r->fix.size = sizeof(GpsLocation);
    r->svstatus.size = sizeof(GpsSvStatus);
#endif
}


static void
nmea_reader_set_callback( NmeaReader*  r, GpsCallbacks  *callbacks)
{
    r->location_callback = callbacks->location_cb;
    r->sv_status_callback = callbacks->sv_status_cb;
    if (callbacks->location_cb != NULL && r->fix.flags != 0) {
        D("%s: sending latest fix to new callback", __FUNCTION__);
        r->location_callback( &r->fix );
        r->fix.flags = 0;
    }
}


static int
nmea_reader_update_time( NmeaReader*  r, Token  tok )
{
    int        hour, minute;
    double     seconds;
    struct tm  tm;
    time_t     fix_time;

    if (tok.p + 6 > tok.end)
        return -1;

    if (r->utc_year < 0) {
        // no date yet, get current one
        time_t  now = time(NULL);
        gmtime_r( &now, &tm );
        r->utc_year = tm.tm_year + 1900;
        r->utc_mon  = tm.tm_mon + 1;
        r->utc_day  = tm.tm_mday;
    }

    hour    = str2int(tok.p,   tok.p+2);
    minute  = str2int(tok.p+2, tok.p+4);
    seconds = str2float(tok.p+4, tok.end);

    tm.tm_hour = hour;
    tm.tm_min  = minute;
    tm.tm_sec  = (int) seconds;
    tm.tm_year = r->utc_year - 1900;
    tm.tm_mon  = r->utc_mon - 1;
    tm.tm_mday = r->utc_day;

    fix_time = utc_mktime( &tm );
    r->fix.timestamp = (long long)fix_time * 1000;
    return 0;
}

static int
nmea_reader_handle_gsv(NmeaReader *r, NmeaTokenizer *tzer)
{
    int lines;
    int nline;
    int nsat;
    Token tok;
    
    
    tok=nmea_tokenizer_get(tzer,1);
    lines = str2int(tok.p, tok.end);
    D("Lines str(%.*s) d(%d)",tok.end-tok.p,tok.p,lines);
    tok=nmea_tokenizer_get(tzer,2);
    nline = str2int(tok.p, tok.end);
    D("n line str(%.*s) d(%d)",tok.end-tok.p,tok.p,nline);
    tok=nmea_tokenizer_get(tzer,3);
    if (nline == 1)
        r->svstatus.num_svs = 0;
    for (nsat=1; nsat <= 4;nsat++) {
            Token tok;
            GpsSvInfo  *svinfo;
            tok=nmea_tokenizer_get(tzer,4*nsat);
            if (!TOKEN_LEN(tok))
                continue;
            if (str2int(tok.p,tok.end)>32)
                continue;
            svinfo=&r->svstatus.sv_list[r->svstatus.num_svs++];
            svinfo->prn=str2int(tok.p,tok.end);
            D("prn str(%.*s) d(%d)",tok.end-tok.p,tok.p,svinfo->prn);
            tok=nmea_tokenizer_get(tzer,4*nsat+1);
            svinfo->elevation=str2float(tok.p,tok.end);
            D("elv str(%.*s) d(%f)",tok.end-tok.p,tok.p,svinfo->elevation);
            tok=nmea_tokenizer_get(tzer,4*nsat+2);
            svinfo->azimuth=str2float(tok.p,tok.end);
            D("az str(%.*s) d(%f)",tok.end-tok.p,tok.p,svinfo->azimuth);
            tok=nmea_tokenizer_get(tzer,4*nsat+3);
            svinfo->snr=str2float(tok.p,tok.end);
            D("snr str(%.*s) d(%f)",tok.end-tok.p,tok.p,svinfo->snr);
    }
    if (lines == nline)
        r->svinfo_flags |= SVINFO_GOT_SV_INFO_FLAG;
    return 0;
}


#define PGLOR_SAT_SV_USED_IN_POSITION 0x4

#define PGLOR_SAT_SV_EPH_NOT_VALID 0x00
#define PGLOR_SAT_SV_EPH_SRC_BE 0x10
#define PGLOR_SAT_SV_EPH_SRC_CBEE 0x20
#define PGLOR_SAT_SV_EPH_SRC_SBEE 0x30

static int
nmea_reader_handle_pglor_sat(NmeaReader *r, NmeaTokenizer *tzer)
{
    Token tok;
    int indx;
    r->svstatus.used_in_fix_mask = 0;
    r->svstatus.ephemeris_mask   = 0;
    r->svstatus.almanac_mask     = 0;
    for (indx=2;indx<tzer->count;indx+=3) {
        int prn;
        uint32_t sv_flags;
        uint32_t sv_mask;
        tok=nmea_tokenizer_get(tzer,indx);
        if (!TOKEN_LEN(tok))
            continue;
        prn = str2int(tok.p,tok.end);
        D("prn str(%.*s) d(%d)",tok.end-tok.p,tok.p,prn);
        if (prn > 32)
            continue;
        sv_mask = 1 << (prn-1);
        tok=nmea_tokenizer_get(tzer,indx+2);
        sv_flags = strhex2int(tok.p,tok.end);
        D("svflags str(%.*s) d(%X)",tok.end-tok.p,tok.p,sv_flags);
        if (sv_flags&PGLOR_SAT_SV_USED_IN_POSITION)
            r->svstatus.used_in_fix_mask |= sv_mask;
        D("used d(%X)", r->svstatus.used_in_fix_mask);
        switch(sv_flags&0x30) {
            case PGLOR_SAT_SV_EPH_SRC_BE:
            case PGLOR_SAT_SV_EPH_SRC_CBEE:
            case PGLOR_SAT_SV_EPH_SRC_SBEE:
                r->svstatus.ephemeris_mask |= sv_mask;
                break;
            default:
                break;
        }
        D("eph d(%X)", r->svstatus.ephemeris_mask);

    }
    r->svinfo_flags |= SVINFO_GOT_SV_USED_FLAG;
    r->svinfo_flags |= SVINFO_GOT_EPH_INFO_FLAG;
    return 0;
}

static int
nmea_reader_update_date( NmeaReader*  r, Token  date, Token  time )
{
    Token  tok = date;
    int    day, mon, year;

    if (tok.p + 6 != tok.end) {
        D("date not properly formatted: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }
    day  = str2int(tok.p, tok.p+2);
    mon  = str2int(tok.p+2, tok.p+4);
    year = str2int(tok.p+4, tok.p+6) + 2000;

    if ((day|mon|year) < 0) {
        D("date not properly formatted: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }

    r->utc_year  = year;
    r->utc_mon   = mon;
    r->utc_day   = day;

    return nmea_reader_update_time( r, time );
}


static double
convert_from_hhmm( Token  tok )
{
    double  val     = str2float(tok.p, tok.end);
    int     degrees = (int)(floor(val) / 100);
    double  minutes = val - degrees*100.;
    double  dcoord  = degrees + minutes / 60.0;
    return dcoord;
}


static int
nmea_reader_update_latlong( NmeaReader*  r,
                            Token        latitude,
                            char         latitudeHemi,
                            Token        longitude,
                            char         longitudeHemi )
{
    double   lat, lon;
    Token    tok;

    tok = latitude;
    if (tok.p + 6 > tok.end) {
        D("latitude is too short: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }
    lat = convert_from_hhmm(tok);
    if (latitudeHemi == 'S')
        lat = -lat;

    tok = longitude;
    if (tok.p + 6 > tok.end) {
        D("longitude is too short: '%.*s'", tok.end-tok.p, tok.p);
        return -1;
    }
    lon = convert_from_hhmm(tok);
    if (longitudeHemi == 'W')
        lon = -lon;

    r->fix.flags    |= GPS_LOCATION_HAS_LAT_LONG;
    r->fix.latitude  = lat;
    r->fix.longitude = lon;
    return 0;
}


static int
nmea_reader_update_altitude( NmeaReader*  r,
                             Token        altitude,
                             Token        units )
{
    double  alt;
    Token   tok = altitude;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_ALTITUDE;
    r->fix.altitude = str2float(tok.p, tok.end);
    return 0;
}


static int
nmea_reader_update_accuracy ( NmeaReader*  r,
                             Token        hdop)
{
    double  alt;
    Token   tok = hdop;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_ACCURACY;
    r->fix.accuracy = str2float(tok.p, tok.end)*10.0;
    return 0;
}

static int
nmea_reader_update_bearing( NmeaReader*  r,
                            Token        bearing )
{
    double  alt;
    Token   tok = bearing;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_BEARING;
    r->fix.bearing  = str2float(tok.p, tok.end);
    return 0;
}


static int
nmea_reader_update_speed( NmeaReader*  r,
                          Token        speed )
{
    double  alt;
    Token   tok = speed;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->fix.speed    = str2float(tok.p, tok.end);
    return 0;
}


static void
nmea_reader_parse( NmeaReader*  r )
{
   /* we received a complete sentence, now parse it to generate
    * a new GPS fix...
    */
    NmeaTokenizer  tzer[1];
    Token          tok;

    D("Received: '%.*s'", r->pos, r->in);
    if (r->pos < 9) {
        D("Too short. discarded.");
        return;
    }

    nmea_tokenizer_init(tzer, r->in, r->in + r->pos);
#if GPS_DEBUG
    {
        int  n;
        D("Found %d tokens", tzer->count);
        for (n = 0; n < tzer->count; n++) {
            Token  tok = nmea_tokenizer_get(tzer,n);
            D("%2d: '%.*s'", n, tok.end-tok.p, tok.p);
        }
    }
#endif

    tok = nmea_tokenizer_get(tzer, 0);
    if (tok.p + 5 > tok.end) {
        D("sentence id '%.*s' too short, ignored.", tok.end-tok.p, tok.p);
        return;
    }

    // ignore first two characters.
    tok.p += 2;
    if ( !memcmp(tok.p, "GGA", 3) ) {
        // GPS fix
        Token  tok_time          = nmea_tokenizer_get(tzer,1);
        Token  tok_latitude      = nmea_tokenizer_get(tzer,2);
        Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,3);
        Token  tok_longitude     = nmea_tokenizer_get(tzer,4);
        Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,5);
        Token  tok_accuracy      = nmea_tokenizer_get(tzer,8);
        Token  tok_altitude      = nmea_tokenizer_get(tzer,9);
        Token  tok_altitudeUnits = nmea_tokenizer_get(tzer,10);

        nmea_reader_update_time(r, tok_time);
        nmea_reader_update_latlong(r, tok_latitude,
                                      tok_latitudeHemi.p[0],
                                      tok_longitude,
                                      tok_longitudeHemi.p[0]);
        nmea_reader_update_altitude(r, tok_altitude, tok_altitudeUnits);
        nmea_reader_update_accuracy(r, tok_accuracy);

    } else if ( !memcmp(tok.p, "GSA", 3) ) {
        // do something ?
    } else if ( !memcmp(tok.p, "GSV", 3) ) {
        nmea_reader_handle_gsv(r, tzer);

    } else if ( !memcmp(tok.p, "RMC", 3) ) {
        Token  tok_time          = nmea_tokenizer_get(tzer,1);
        Token  tok_fixStatus     = nmea_tokenizer_get(tzer,2);
        Token  tok_latitude      = nmea_tokenizer_get(tzer,3);
        Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,4);
        Token  tok_longitude     = nmea_tokenizer_get(tzer,5);
        Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,6);
        Token  tok_speed         = nmea_tokenizer_get(tzer,7);
        Token  tok_bearing       = nmea_tokenizer_get(tzer,8);
        Token  tok_date          = nmea_tokenizer_get(tzer,9);

        D("in RMC, fixStatus=%c", tok_fixStatus.p[0]);
        if (tok_fixStatus.p[0] == 'A')
        {
            nmea_reader_update_date( r, tok_date, tok_time );

            nmea_reader_update_latlong( r, tok_latitude,
                                           tok_latitudeHemi.p[0],
                                           tok_longitude,
                                           tok_longitudeHemi.p[0] );

            nmea_reader_update_bearing( r, tok_bearing );
            nmea_reader_update_speed  ( r, tok_speed );
        }
    }
    else if ( !memcmp(tok.p,"LOR",3) ) {
        Token tok;
        tok = nmea_tokenizer_get(tzer,1);
        D("sentence id $PGLOR,%.*s ", tok.end-tok.p, tok.p);
        if (TOKEN_LEN(tok) >= 3) {
            if (!memcmp(tok.p,"SAT",3) )
                nmea_reader_handle_pglor_sat(r, tzer);
        }

    }
    else {
        tok.p -= 2;
        D("unknown sentence '%.*s", tok.end-tok.p, tok.p);
    }
    if ((r->fix.flags & GPS_LOCATION_HAS_LAT_LONG) &&
           (r->fix.flags & GPS_LOCATION_HAS_ALTITUDE) &&
           (r->fix.flags & GPS_LOCATION_HAS_SPEED) &&
           (r->fix.flags & GPS_LOCATION_HAS_BEARING)) {

#if GPS_DEBUG
        char   temp[256];
        char*  p   = temp;
        char*  end = p + sizeof(temp);
        struct tm   utc;

        p += snprintf( p, end-p, "sending fix" );
        if (r->fix.flags & GPS_LOCATION_HAS_LAT_LONG) {
            p += snprintf(p, end-p, " lat=%g lon=%g", r->fix.latitude, r->fix.longitude);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_ALTITUDE) {
            p += snprintf(p, end-p, " altitude=%g", r->fix.altitude);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_SPEED) {
            p += snprintf(p, end-p, " speed=%g", r->fix.speed);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_BEARING) {
            p += snprintf(p, end-p, " bearing=%g", r->fix.bearing);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_ACCURACY) {
            p += snprintf(p,end-p, " accuracy=%g", r->fix.accuracy);
        }
        gmtime_r( (time_t*) &r->fix.timestamp, &utc );
        p += snprintf(p, end-p, " time=%s", asctime( &utc ) );
        D(temp);
#endif
        if (r->location_callback) {
            r->location_callback( &r->fix );
            r->fix.flags = 0;
        }
        else {
            D("no callback, keeping data until needed !");
        }
    }

    if ((r->svinfo_flags & SVINFO_GOT_EPH_INFO_FLAG) &&
            (r->svinfo_flags & SVINFO_GOT_SV_INFO_FLAG) &&
            (r->svinfo_flags & SVINFO_GOT_SV_USED_FLAG)) {
        if (r->sv_status_callback) {
            r->sv_status_callback( &r->svstatus );
            r->svinfo_flags = 0;
        }
    }
}


static void
nmea_reader_addc( NmeaReader*  r, int  c )
{
    if (r->overflow) {
        r->overflow = (c != '\n');
        return;
    }

    if (r->pos >= (int) sizeof(r->in)-1 ) {
        r->overflow = 1;
        r->pos      = 0;
        return;
    }

    r->in[r->pos] = (char)c;
    r->pos       += 1;

    if (c == '\n') {
        nmea_reader_parse( r );
        r->pos = 0;
    }
}


/*****************************************************************/
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


/* this is the state of our connection to the qemu_gpsd daemon */
typedef struct {
    int                     init;
    int                     fd;
    GpsCallbacks            callbacks;
    pthread_t               thread;
    int                     control[2];
#ifdef ANDROID23
    GpsPositionRecurrence   recurrence;
    uint32_t                min_interval;
    uint32_t                preferred_accuracy;
    uint32_t                preferred_time;
#endif
    int                     fix_frequency; 
    char                    *control_pipe;
    char                    *nmea_pipe;

} GpsState;

static GpsState  _gps_state[1];



static void
gps_state_done( GpsState*  s )
{
    // tell the thread to quit, and wait for it
    char   cmd = CMD_QUIT;
    void*  dummy;
    write( s->control[0], &cmd, 1 );
    pthread_join(s->thread, &dummy);

    // close the control socket pair
    close( s->control[0] ); s->control[0] = -1;
    close( s->control[1] ); s->control[1] = -1;

    // close connection to the QEMU GPS daemon
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
#ifdef ANDROID23
static void
#else
static void*
#endif
gps_state_thread( void*  arg )
{
    GpsState*   state = (GpsState*) arg;
    NmeaReader  reader[1];
    int         epoll_fd   = epoll_create(2);
    int         started    = 0;
    int         gps_fd     = -1;
    int         control_fd = state->control[1];

    nmea_reader_init( reader );

    // register control file descriptors for polling
    epoll_register( epoll_fd, control_fd );

    D("gps thread running");

    // now loop
    for (;;) {
        struct epoll_event   events[2];
        int                  ne, nevents;

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
                goto Exit;
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
                    else if (cmd == CMD_START) {
                        if (!started) {
                            D("gps thread starting  location_cb=%p", &state->callbacks);
                            started = 1;
                            gps_fd = state->fd;
                            epoll_register(epoll_fd, gps_fd);
                            nmea_reader_set_callback(reader,&state->callbacks);
                        }
                    }
                    else if (cmd == CMD_STOP) {
                        if (started) {
                            D("gps thread stopping");
                            epoll_deregister(epoll_fd, gps_fd);
                            gps_fd = -1;
                            started = 0;
                            nmea_reader_set_callback( reader, NULL );
                        }
                    }
                }
                else if (fd == gps_fd)
                {
                    char  buff[32];
                    D("gps fd event");
                    for (;;) {
                        int  nn, ret;

                        ret = read( fd, buff, sizeof(buff) );
                        if (ret < 0) {
                            if (errno == EINTR)
                                continue;
                            if (errno != EWOULDBLOCK)
                                LOGE("error while reading from gps daemon socket: %s:", strerror(errno));
                            break;
                        }
                        D("received %d bytes: %.*s", ret, ret, buff);
                        for (nn = 0; nn < ret; nn++)
                            nmea_reader_addc( reader, buff[nn] );
                    }
                    D("gps fd event end");
                }
                else
                {
                    LOGE("epoll_wait() returned unkown fd %d ?", fd);
                }
            }
        }
    }
Exit:
#ifdef ANDROID23
    return;
#else
    return NULL;
#endif
}


static void
gps_state_init( GpsState*  state )
{
    int    ret;

    state->init       = 1;
    state->control[0] = -1;
    state->control[1] = -1;
    state->fd         = -1;
#ifdef ANDROID23
    state->recurrence = GPS_POSITION_RECURRENCE_PERIODIC;
    state->min_interval = 1000;
    state->preferred_accuracy = 50;
    state->preferred_time = 1;  
#else
    state->fix_frequency = 1;
#endif
    state->control_pipe = BRCM_CONTROL_PIPE;
    state->nmea_pipe = BRCM_NMEA_PIPE;





    if ( socketpair( AF_LOCAL, SOCK_STREAM, 0, state->control ) < 0 ) {
        LOGE("could not create thread control socket pair: %s", strerror(errno));
        goto Fail;
    }
#ifdef ANDROID23
    state->thread = state->callbacks.create_thread_cb( "libgps", gps_state_thread, state );
    if ( !state->thread ) {
#else
    if ( pthread_create( &state->thread, NULL, gps_state_thread, state ) != 0 ) {
#endif
        LOGE("could not create gps thread: %s", strerror(errno));
        goto Fail;
    }

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
gps_init(GpsCallbacks* callbacks)
{
    GpsState*  s = _gps_state;

    if (!s->init)
        gps_state_init(s);

    s->callbacks = *callbacks;

    return 0;
}

static void
gps_cleanup(void)
{
    GpsState*  s = _gps_state;

    if (s->init)
        gps_state_done(s);
}


static int
gps_start()
{
    GpsState*  s = _gps_state;
    int retries=3;
    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

#ifdef ANDROID23
    if (s->recurrence == GPS_POSITION_RECURRENCE_PERIODIC) {
        D("%s(%d): calling irm periodic (%s, %d)",__FUNCTION__,__LINE__,s->control_pipe, s->min_interval / 1000);
        if (irm_start_periodic(s->control_pipe, s->min_interval / 1000,-1,-1,-1) < 0)
#else
    if (s->fix_frequency) {
        D("%s(%d): calling irm periodic (%s, %d)",__FUNCTION__,__LINE__,s->control_pipe, s->fix_frequency);
        if (irm_start_periodic(s->control_pipe,s->fix_frequency,-1,-1,-1) < 0)
#endif
            D("%s(%d): irm_start_periodic failed errno(%d)",__FUNCTION__,__LINE__,errno);
        else
            D("%s(%d): irm_start_periodic success",__FUNCTION__,__LINE__);
    }
    else {
        D("%s(%d): calling irm periodic (%s)",__FUNCTION__,__LINE__,s->control_pipe);
        if (irm_start_periodic(s->control_pipe,1,-1,1,60) < 0)
            D("%s(%d): irm_start_periodic failed errno(%d)",__FUNCTION__,__LINE__,errno);
        else
            D("%s(%d): irm_start_periodic success",__FUNCTION__,__LINE__);
    }
    do {
            if ((s->fd = open(s->nmea_pipe, O_RDONLY | O_NONBLOCK)) > 0) {
                D("gps emulation will read from %s",s->nmea_pipe);
                break;
            }
            sleep(1);
    } while (retries--);
    if (s->fd < 0)
        D("no '%s' pipe available: %s",s->nmea_pipe, strerror(errno));
    D("%s: called", __FUNCTION__);
    gps_state_start(s);
    return 0;
}


static int
gps_stop()
{
    GpsState*  s = _gps_state;

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }
    if (s->fd >= 0)
        close( s->fd ); s->fd = -1;

    if (irm_stop(s->control_pipe) < 0)
        D("%s(%d): irm_stop failed errno(%d)",__FUNCTION__,__LINE__,errno);
    else
        D("%s(%d): irm_stop success",__FUNCTION__,__LINE__);
    D("%s: called", __FUNCTION__);
    gps_state_stop(s);
    return 0;
}

static int
gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    return 0;
}

static int 
gps_inject_location(double latitude, double longitude, float accuracy)
{
	return 0;
}

static void
gps_delete_aiding_data(GpsAidingData flags)
{
    GpsState*  s = _gps_state;

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
    }
    if (irm_startup(s->control_pipe,flags) < 0)
        D("%s(%d): irm_startup(%X) failed errno(%d)",__FUNCTION__,__LINE__,flags,errno);
    else
        D("%s(%d): irm_startup(%X) success",__FUNCTION__,__LINE__,flags);
	
    D("%s: called", __FUNCTION__);
}

#ifdef ANDROID23
static int gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
            uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
#else
static int gps_set_position_mode(GpsPositionMode mode, int fix_frequency)
#endif
{
    // FIXME - support fix_frequency
    // only standalone supported for now.
    if (mode != GPS_POSITION_MODE_STANDALONE) {
        D("%s: only standalone supported by now!",__FUNCTION__);
        return -1;
    }
    GpsState*  s = _gps_state;

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }
#ifdef ANDROID23
    s->recurrence = recurrence;
    s->min_interval = min_interval ? min_interval : 1000;
    s->preferred_accuracy = preferred_accuracy ? preferred_accuracy : 50;
    s->preferred_time = preferred_time;
#else
    s->fix_frequency = fix_frequency;
#endif

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

static time_t utc_mktime(struct tm *_tm)
{
    time_t t_epoch=0;
    int m; 
    int *days_per_month;
    if (is_leap_year(_tm->tm_year+1900))
        days_per_month = days_per_month_leap;
    else
        days_per_month = days_per_month_no_leap;
    t_epoch += (_tm->tm_year - 70)*SECONDS_PER_NORMAL_YEAR; 
    t_epoch += number_of_leap_years_in_between(1970,_tm->tm_year+1900) *
        SECONDS_PER_DAY;
    for (m=0; m<_tm->tm_mon; m++) {
        t_epoch += days_per_month[m]*SECONDS_PER_DAY;
    }
    t_epoch += (_tm->tm_mday-1)*SECONDS_PER_DAY;
    t_epoch += _tm->tm_hour*SECONDS_PER_HOUR;
    t_epoch += _tm->tm_min*SECONDS_PER_MIN;
    t_epoch += _tm->tm_sec;
    return t_epoch;

}



void assist_gps_init( AGpsCallbacks* callbacks )
{

}

int assist_gps_data_conn_open( const char* apn )
{
	return 0;
}

int assist_gps_data_conn_closed(void)
{
	return 0;
}

int assist_gps_data_conn_failed(void)
{
	return 0;
}

int assist_gps_set_server( AGpsType type, const char* hostname, int port )
{
	return 0;
}

int gps_plus_init( GpsXtraCallbacks* callbacks )
{
	return 0;
}

int  gps_plus_inject_data( char* data, int length )
{
	return 0;
}

static const GpsXtraInterface sGpsPlusInterface = 
{ 
#ifdef ANDROID23
	size:						sizeof(GpsXtraInterface), 
#endif
	init:						gps_plus_init, 
	inject_xtra_data:			gps_plus_inject_data,
};

static const AGpsInterface sAssistGpsInterface = 
{ 
#ifdef ANDROID23
	size:						sizeof(AGpsInterface), 
#endif
	init:						assist_gps_init, 
	data_conn_open:				assist_gps_data_conn_open,
	data_conn_closed:			assist_gps_data_conn_closed,
	data_conn_failed:			assist_gps_data_conn_failed,
	set_server:					assist_gps_set_server,
};

const void* gps_get_extension(const char* name)
{
	if (strcmp(name, GPS_XTRA_INTERFACE) == 0) {
		return NULL;
	}
	
	if (strcmp(name, AGPS_INTERFACE) == 0) {
		return &sAssistGpsInterface;
	}

	return NULL;
}

static const GpsInterface  _GpsInterface = {
#ifdef ANDROID23
	size:                   sizeof(GpsInterface), 
#endif
    init: 					gps_init,
    start:					gps_start,
    stop:					gps_stop,
    cleanup:				gps_cleanup,
    inject_time:			gps_inject_time,
    inject_location:		gps_inject_location,
    delete_aiding_data:		gps_delete_aiding_data,
    set_position_mode:		gps_set_position_mode,
    get_extension: 			gps_get_extension,
};

#ifdef ANDROID23
const GpsInterface* gps_get_hardware_interface(struct gps_device_t* dev)
#else
const GpsInterface* gps_get_hardware_interface(void)
#endif
{
	return &_GpsInterface;
}

static int irm_send_command(const char *pipe_path, const char *cmd)
{
        FILE *fp;
        if (!(fp=fopen(pipe_path,"w")))
                return -1;
        fprintf(fp,cmd);
        fclose(fp);
        return 0;
}

static int irm_start_periodic(const char *pipe_path,int period,int fixcount, int validfix, int duration_sec)
{
        char cmd[256];
        snprintf(cmd,sizeof(cmd)-1,"$pglirm,req_pos,fix,period,%d,fixcount,%d,validfix,%d,duration_sec,%d\n",
                period,fixcount,validfix,duration_sec);
        return irm_send_command(pipe_path,cmd);
}

static int irm_start_single(const char *pipe_path, int accuracy, int timeout)
{
        char cmd[256];
        snprintf(cmd,sizeof(cmd)-1,"$pglirm,req_pos_single,fix,acc,%d,timeout,%d\n",
                accuracy,timeout);
        return irm_send_command(pipe_path,cmd);
}
static int irm_startup(const char *pipe_path, GpsAidingData flags)
{
        char cmd[256];

        snprintf(cmd,sizeof(cmd)-1,"$pglirm,startup");
        if (flags&GPS_DELETE_EPHEMERIS)
                strncat(cmd,",ignore_nav",sizeof(cmd)-1);
        if (flags&GPS_DELETE_ALMANAC)
                strncat(cmd,",ignore_ram_alm,ignore_rom_alm",sizeof(cmd)-1);
        if (flags&GPS_DELETE_POSITION)
                strncat(cmd,",ignore_pos",sizeof(cmd)-1);
        if (flags&GPS_DELETE_TIME)
                strncat(cmd,",ignore_time",sizeof(cmd)-1);
        strncat(cmd,"\n",sizeof(cmd)-1);
        return irm_send_command(pipe_path,cmd);
}

static int irm_stop(const char *pipe_path)
{
        char cmd[256];
        snprintf(cmd,sizeof(cmd)-1,"$pglirm,stop,all\n");
        return irm_send_command(pipe_path,cmd);
}

#ifdef ANDROID23
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

const struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = GPS_HARDWARE_MODULE_ID,
    .name = "lcsapi GPS Module",
    .author = "Broadcom Corporation",
    .methods = &hw_module_methods,
};
#endif
