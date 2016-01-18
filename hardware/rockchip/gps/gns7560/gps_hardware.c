
#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <math.h>
#include <time.h>
#include <errno.h>


#define  LOG_TAG  "gps_hardware"
#include <cutils/log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include "gps.h"
#include "inc/gps_ptypes.h"
#include "inc/GN_GPS_api.h"
extern void GN_GPS_Thread(void);
extern U1 GN_GPS_Init();
extern U1 GN_GPS_Start(void);
extern void GN_GPS_Stop(void);
extern void GN_GPS_Exit(void);
/* the name of the qemud-controlled socket */
//#define  QEMUD_SOCKET_NAME  "qemud_gps"

#define  GPS_DEBUG  0

#if GPS_DEBUG
#  define  D(...)   LOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

/* commands sent to the gps thread */
enum {
    CMD_QUIT  = 0,
    CMD_START = 1,
    CMD_STOP  = 2
};
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

        if (q >= p) {
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

#define  NMEA_MAX_SIZE  83

typedef struct {
    int     pos;
    int     overflow;
    int     utc_year;
    int     utc_mon;
    int     utc_day;
    int     utc_diff;
    GpsLocation  fix;
    GpsSvStatus  sat_status;
    GpsCallbacks  callback;
    char    in[ NMEA_MAX_SIZE+1 ];
} NmeaReader;


static void
nmea_reader_update_utc_diff( NmeaReader*  r )
{
    time_t         now = time(NULL);
    struct tm      tm_local;
    struct tm      tm_utc;
    long           time_local, time_utc;

    gmtime_r( &now, &tm_utc );
    localtime_r( &now, &tm_local );

    time_local = tm_local.tm_sec +
                 60*(tm_local.tm_min +
                 60*(tm_local.tm_hour +
                 24*(tm_local.tm_yday +
                 365*tm_local.tm_year)));

    time_utc = tm_utc.tm_sec +
               60*(tm_utc.tm_min +
               60*(tm_utc.tm_hour +
               24*(tm_utc.tm_yday +
               365*tm_utc.tm_year)));

    r->utc_diff = time_utc - time_local;
}


static void
nmea_reader_init( NmeaReader*  r )
{
    memset( r, 0, sizeof(*r) );

    r->pos      = 0;
    r->overflow = 0;
    r->utc_year = -1;
    r->utc_mon  = -1;
    r->utc_day  = -1;
    r->callback.location_cb = NULL;
    r->callback.status_cb = NULL;
    r->callback.sv_status_cb = NULL;

    nmea_reader_update_utc_diff( r );
}


static void
nmea_reader_set_callback( NmeaReader*  r, GpsCallbacks cb )
{
    r->callback.location_cb = cb.location_cb;
    if (cb.location_cb!= NULL && r->fix.flags != 0) {
        D("%s: sending latest fix to new location callback", __FUNCTION__);
        r->callback.location_cb( &r->fix );
        r->fix.flags = 0;
    }
/*	r->callback.status_cb = cb.status_cb;
    if (cb.status_cb != NULL && r->sat_status.num_svs != 0) {
        D("%s: sending latest fix to new gps satelite status callback", __FUNCTION__);
        r->callback.status_cb( &r->sat_status);
    }
*/
	r->callback.sv_status_cb = cb.sv_status_cb;
    if (cb.sv_status_cb != NULL && r->sat_status.num_svs != 0) {
        D("%s: sending latest fix to new gps satelite infomation callback", __FUNCTION__);
        r->callback.sv_status_cb( &r->sat_status);
		r->sat_status.num_svs = 0;
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

    fix_time = mktime( &tm ) + r->utc_diff;
    r->fix.timestamp = (long long)fix_time * 1000;
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
    r->fix.speed = (r->fix.speed*1.852)/3.6;
    return 0;
}

static int str2num(Token a_token)
{
	Token tok = a_token;
	if (tok.p >= tok.end)
        return -1;
	return (int)str2float(tok.p, tok.end);
}

//ldx add gpgsv sentence related funcitons
static int
nmea_reader_update_sat_num( NmeaReader*  r,
                          Token        tok_sat_num )
{
    double  alt;
    Token   tok = tok_sat_num;

    if (tok.p >= tok.end)
        return -1;

    r->sat_status.num_svs= str2float(tok.p, tok.end);
    return 0;
}

static int
nmea_reader_update_sat_pnr( NmeaReader*  r,
                          Token        tok_sat_pnr, int index )
{
    double  alt;
    Token   tok = tok_sat_pnr;

    if (tok.p >= tok.end)
        return -1;

//    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->sat_status.sv_list[index].prn= str2float(tok.p, tok.end);
    return 0;
}

static int
nmea_reader_update_sat_elevation( NmeaReader*  r,
                          Token        tok_sat_elevation , int index)
{
    double  alt;
    Token   tok = tok_sat_elevation;

    if (tok.p >= tok.end)
        return -1;

//    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->sat_status.sv_list[index].elevation= str2float(tok.p, tok.end);
    return 0;
}

static int
nmea_reader_update_sat_azimuth( NmeaReader*  r,
                          Token        tok_sat_azimuth, int index )
{
    double  alt;
    Token   tok = tok_sat_azimuth;

    if (tok.p >= tok.end)
        return -1;

//    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->sat_status.sv_list[index].azimuth= str2float(tok.p, tok.end);
    return 0;
}

static int
nmea_reader_update_sat_snr( NmeaReader*  r,
                          Token        tok_sat_snr, int index )
{
    double  alt;
    Token   tok = tok_sat_snr;

    if (tok.p >= tok.end)
        return -1;

    //r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->sat_status.sv_list[index].snr= str2float(tok.p, tok.end);
    return 0;
}


static void
nmea_reader_parse( NmeaReader*  r )
{
   /* we received a complete sentence, now parse it to generate
    * a new GPS fix...
    */ 
//	D("Received: '%s'", r->in); 
	D("\n###########################\n");  
    NmeaTokenizer  tzer[1];
    Token          tok;

    D("Received: '%.*s'", r->pos, r->in);
    if (r->pos < 9) {
        D("Too short. discarded.");
	r->fix.flags = 0;
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
        Token  tok_altitude      = nmea_tokenizer_get(tzer,9);
        Token  tok_altitudeUnits = nmea_tokenizer_get(tzer,10);

        nmea_reader_update_time(r, tok_time);
        nmea_reader_update_latlong(r, tok_latitude,
                                      tok_latitudeHemi.p[0],
                                      tok_longitude,
                                      tok_longitudeHemi.p[0]);
        nmea_reader_update_altitude(r, tok_altitude, tok_altitudeUnits);

	if (r->fix.flags != 0) {
	    if (r->callback.location_cb ) {
		D("%s: report GGA", __FUNCTION__);
	        r->callback.location_cb( &r->fix );
	        r->fix.flags = 0;
	    }
	}

    } else if ( !memcmp(tok.p, "GSA", 3) ) {
        // do something ?
        int i, j;
        Token tok_sel_mode = nmea_tokenizer_get(tzer,1);
        Token tok_mode = nmea_tokenizer_get(tzer,2);
        Token tok_PDOP = nmea_tokenizer_get(tzer,14);
        Token tok_HDOP = nmea_tokenizer_get(tzer,15);
        Token tok_VDOP  = nmea_tokenizer_get(tzer,16);
        Token tok_pnr[12];

	r->fix.flags |= GPS_LOCATION_HAS_ACCURACY;
        //r->fix.accuracy = g_nav_data.P_DOP;
        
        for(i = 0; i < 12; i ++)
                    tok_pnr[i] = nmea_tokenizer_get(tzer,3+i);

        r->sat_status.used_in_fix_mask = 0;

        for(i = 0; i < 12; i ++)
        {
            int pnr = str2int(tok_pnr[i].p, tok_pnr[i].end);
            D("used in fix sat %d pnr %d\n", i, pnr);
            //used_in_fix_mask is a mask for pnr
            if(pnr > 0)
                r->sat_status.used_in_fix_mask |= 1 << (pnr - 1);
        }
        D("used in fix mask %x\n", r->sat_status.used_in_fix_mask);
    } else if ( !memcmp(tok.p, "GSV", 3) ) {
    	//GPS Satelite view There are 4 satelite info in each GSV sentence
    Token tok_sat_pnr[4];
	Token tok_sat_elevation[4];
	Token tok_sat_azimuth[4];
	Token tok_sat_snr[4];

	Token tok_sat_gsv_total =nmea_tokenizer_get(tzer,1);
	Token tok_sat_gsv_index =nmea_tokenizer_get(tzer,2);
	
	int sat_index = str2num(tok_sat_gsv_index) - 1;
	int sat_total    = str2num(tok_sat_gsv_total) - 1;
	D("Found sat : %d \n", sat_total);
	D("sat_index: %d\n", sat_index);
    Token tok_sat_num 		= nmea_tokenizer_get(tzer,3);
	tok_sat_pnr[0] 		= nmea_tokenizer_get(tzer,4);
	tok_sat_elevation[0] 	= nmea_tokenizer_get(tzer,5);
	tok_sat_azimuth[0] 		= nmea_tokenizer_get(tzer,6);
	tok_sat_snr[0] 			= nmea_tokenizer_get(tzer,7);
	
	tok_sat_pnr[1] 		= nmea_tokenizer_get(tzer,4+4);
	tok_sat_elevation[1] 	= nmea_tokenizer_get(tzer,5+4);
	tok_sat_azimuth[1] 		= nmea_tokenizer_get(tzer,6+4);
	tok_sat_snr[1] 			= nmea_tokenizer_get(tzer,7+4);
	
	tok_sat_pnr[2] 		= nmea_tokenizer_get(tzer,4+8);
	tok_sat_elevation[2] 	= nmea_tokenizer_get(tzer,5+8);
	tok_sat_azimuth[2] 		= nmea_tokenizer_get(tzer,6+8);
	tok_sat_snr[2] 			= nmea_tokenizer_get(tzer,7+8);
	
	tok_sat_pnr[3] 		= nmea_tokenizer_get(tzer,4+12);
	tok_sat_elevation[3] 	= nmea_tokenizer_get(tzer,5+12);
	tok_sat_azimuth[3] 		= nmea_tokenizer_get(tzer,6+12);
	tok_sat_snr[3] 			= nmea_tokenizer_get(tzer,7+12);

	nmea_reader_update_sat_num(r, tok_sat_num);
	int index;
	for (index = 0; index < 4; index++)
	{
		nmea_reader_update_sat_pnr(r, tok_sat_pnr[index], index+4*sat_index);
		nmea_reader_update_sat_elevation(r, tok_sat_elevation[index], index+4*sat_index);
		nmea_reader_update_sat_azimuth(r, tok_sat_azimuth[index], index+4*sat_index);
		nmea_reader_update_sat_snr(r, tok_sat_snr[index], index+4*sat_index);

    	D("satellite infomation : pnr:%d snr:%f \n" ,r->sat_status.sv_list[index+4*sat_index].prn,r->sat_status.sv_list[index+4*sat_index].snr);
	}

	if ((r->sat_status.num_svs != 0) && (sat_total == sat_index)) {
      	 	 if (r->callback.sv_status_cb ) {
            	r->callback.sv_status_cb( &r->sat_status);
            	r->sat_status.num_svs = 0;
       	 	}
	}
    	//do something ?
    } else if ( !memcmp(tok.p, "VTG", 3) ) {
    	//do something ?
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

	    if (r->fix.flags != 0) {
		if (r->callback.location_cb ) {
		    D("%s: report RMC", __FUNCTION__); 
		    r->callback.location_cb( &r->fix );
		    r->fix.flags = 0;
		}
	    }
	}
    } else {
        tok.p -= 2;
        D("unknown sentence '%.*s", tok.end-tok.p, tok.p);
    }
/*
    if (r->fix.flags != 0) {
        if (r->callback.location_cb && r->callback.status_cb) {
            r->callback.location_cb( &r->fix );
	     	r->callback.status_cb( &r->sat_status);
			int j = 0;				
			for(j = 0; j < 32; j++)			
			r->callback.sv_status_cb( &r->sat_status.sv_list[j]);
            r->fix.flags = 0;
        }
        else {
            D("no callback, keeping data until needed !");
        }
    }
*/
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


/* this is the state of our connection to the qemu_gpsd daemon */
typedef struct {
    int                     init;
	int                     start;
    pthread_t               thread;
	pthread_mutex_t         mutex;
    GpsCallbacks            callbacks; // JNI GPS_Callbacks
} GpsState;

static GpsState    _gps_state[1];
static NmeaReader  _nmea_reader;
static GpsStatus	  g_status ;


U2 Gps7560_write_nmea(CH* ptr, U2 bytes)
{
	int i = 0;
	for(i = 0; i < bytes; i++)
	{
		nmea_reader_addc(&_nmea_reader, *(ptr + i));
	}
	return i;
}

static void gps_state_done( GpsState*  s )
{	
	LOGD("====%s: ............\n", __func__);	

	s->thread = -1;
    s->init = 0;
	GN_GPS_Exit();
		
	LOGD("====%s: done!\n", __func__);	
}


static void gps_state_start(GpsState*  s)
{
	//pthread_mutex_lock(&state->mutex);
	LOGD("====%s: ............\n", __func__);

	s->thread     = -1;
	s->start = GN_GPS_Start();
	
	//pthread_mutex_unlock(&state->mutex);
    LOGD("====%s: done!\n", __func__);
}


static void gps_state_stop(GpsState*  s)
{
	
	LOGD("====%s:............ \n", __func__);	
	//pthread_mutex_lock(&s->mutex);
	s->start = 0;
	pthread_join(s->thread, NULL);
	//s->callbacks.location_cb 	= NULL;
	//s->callbacks.sv_status_cb   = NULL;
	//nmea_reader_set_callback(&_nmea_reader, s->callbacks );
	GN_GPS_Stop();
	s->thread = -1;
	
	//pthread_mutex_unlock(&s->mutex);
	LOGD("====%s: done!\n", __func__);	
}

static int
epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int                 ret, flags;

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

static void gps_state_thread(void*  arg)
{
    GpsState*   s = (GpsState*) arg;
    NmeaReader  reader[1];
    //int         gps_fd     = state->fd;

	nmea_reader_init(&_nmea_reader);

    LOGD("gps thread running\n");

    while (1) {

		//pthread_mutex_lock(&state->mutex);
		if (s->start == 0)
		{	
			//s->callbacks.location_cb 	= NULL;
			//s->callbacks.sv_status_cb   = NULL;
			//nmea_reader_set_callback(&_nmea_reader, s->callbacks);
			LOGD("====%s: exit.\n", __func__);
			goto Exit;
		}
		else
		{
			GN_GPS_Thread();
			nmea_reader_set_callback(&_nmea_reader, s->callbacks);
		}
		//pthread_mutex_unlock(&state->mutex);
    }
Exit:
	LOGD("gps thread stop\n");
    return ;
}



static void gps_state_init(GpsState*  s)
{
	LOGD("====%s:............... \n", __func__);

	s->thread = -1;
	s->start = 0;
	s->init = GN_GPS_Init();
	
	LOGD("====%s: done!\n", __func__);
}


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       I N T E R F A C E                               *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/


static int hardware_gps_init(GpsCallbacks* callbacks)
{	
    GpsState*  s = _gps_state;

	gps_state_init(s);
	
	if (!s->init) {
		LOGD("====%s: hardware gps init error\n", __func__);
		return -1;
	}

	pthread_mutex_init(&s->mutex, NULL);
    s->callbacks = *callbacks;

	g_status.status = GPS_STATUS_NONE;
	s->callbacks.status_cb(&g_status);

	LOGD("====%s: done!\n", __func__);
    return 0;
}

static void hardware_gps_cleanup(void)
{
    GpsState*  s = _gps_state;
	if (s->init) {
		if (!s->start)
			gps_state_done(s);
		pthread_mutex_destroy(&s->mutex);
	}
	
	//s->callbacks.location_cb 	= NULL;
	//s->callbacks.status_cb   	= NULL;
	//s->callbacks.sv_status_cb   = NULL;
	//nmea_reader_set_callback(&_nmea_reader, s->callbacks );

	LOGD("====%s: done!\n", __func__);
}


static int hardware_gps_start()
{	
    GpsState*  s = _gps_state;

    if (!s->init) {
        LOGD("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

	pthread_mutex_lock(&s->mutex);
	if (!s->start) {
		gps_state_start(s);
		if (!s->start) {
			LOGD("====%s: init gps state err\n", __func__);
			pthread_mutex_unlock(&s->mutex);
			return -1;
		}

		s->thread = s->callbacks.create_thread_cb("ste_gps", gps_state_thread, (void *)s);

		g_status.status =GPS_STATUS_SESSION_BEGIN;		
		s->callbacks.status_cb(&g_status);
	}
	pthread_mutex_unlock(&s->mutex);

    LOGD("====%s: done!\n", __FUNCTION__);

    return 0;
}


static int hardware_gps_stop()
{
    GpsState*  s = _gps_state;

    if (!s->init) {
        LOGD("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

	pthread_mutex_lock(&s->mutex);
	if (s->start)
	{
		gps_state_stop(s);

		g_status.status = GPS_STATUS_ENGINE_OFF;
	   	s->callbacks.status_cb(&g_status);
	}
	pthread_mutex_unlock(&s->mutex);


    LOGD("====%s: done!\n", __FUNCTION__);
    return 0;
}


static void
hardware_gps_set_fix_frequency()
{
    GpsState*  s = _gps_state;

    if (!s->init) {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return ;
    }

    LOGD("%s: called", __FUNCTION__);
    // FIXME - support fix_frequency
}

static int
hardware_gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    return 0;
}

static int hardware_gps_inject_location (double latitude, double longitude, float accuracy)
{
    /* not yet implemented */
    return 0;
}

static void
hardware_gps_delete_aiding_data(GpsAidingData flags)
{
}

static int hardware_gps_set_position_mode(GpsPositionMode mode, int fix_frequency)
{
    // FIXME - support fix_frequency
    // only standalone supported for now.
    if (mode != GPS_POSITION_MODE_STANDALONE)
        return -1;
    return 0;
}

static const void*
hardware_gps_get_extension(const char* name)
{
    return NULL;
}

static const GpsInterface  hardwareGpsInterface = {
	.size = sizeof(GpsInterface),
    .init = hardware_gps_init,
    .start = hardware_gps_start,
    .stop = hardware_gps_stop,
    //hardware_gps_set_fix_frequency,
    .cleanup = hardware_gps_cleanup,
    .inject_time = hardware_gps_inject_time,
	.inject_location = hardware_gps_inject_location,
    .delete_aiding_data = hardware_gps_delete_aiding_data,
    .set_position_mode = hardware_gps_set_position_mode,
    .get_extension = hardware_gps_get_extension,
};
const GpsInterface* gps_get_hardware_interface()
{
    D("\nGPS is to get hardware interface!\n");
	return &hardwareGpsInterface;
}


