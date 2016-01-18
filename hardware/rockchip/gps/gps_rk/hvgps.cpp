///////////////////////////////////////////////////////////////////////////////////
//
// Filename:     hvgps.cpp
// Author:       sjchen
// Copyright: 
// Date: 	2012/08/30
// Description:
//		GPS interface of android4.0 or later
//
// Revision:
//		0.0.1
//
///////////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <math.h>
#include <time.h>
#include <android/log.h>  
#include "hardware/gps.h"

#include "hvgps.h"

#ifdef DEBUG_LEVEL
#  define  D(...)   LOGI(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

///////////////////////////////////////////////////////////////////////////////////
// 
// define global or static variables
//
///////////////////////////////////////////////////////////////////////////////////
bool   g_GPSInited = false;
static bool g_GPSStarted = false;

//hold reference to callbacks
static gps_location_callback g_location_cb;
static gps_status_callback g_status_cb;
static gps_sv_status_callback g_sv_status_cb;
static gps_nmea_callback g_nmea_cb;
static sem_t  s_hStopSem;
gps_create_thread g_create_thread_cb;
GpsStatus g_status;
 

///////////////////////////////////////////////////////////////////////////////////
// 
// Function Name: WaitSem
// Parameters: 
// Description: 
//        wait semaphore to signal
// Notes: sjchen 2012/09/04
//
///////////////////////////////////////////////////////////////////////////////////
unsigned long WaitSem	( sem_t* psem, unsigned long dwTimeout)
{
	struct timespec btime;	

	if(dwTimeout == (unsigned long)0xffffffff)
	{
		return sem_wait(psem);
	}

	clock_gettime(CLOCK_REALTIME, &btime);

	btime.tv_sec += dwTimeout/1000 ;
	btime.tv_nsec += (dwTimeout%1000)*1000000;
	if(btime.tv_nsec >= 1000000000)
	{
		btime.tv_nsec -= 1000000000;
		btime.tv_sec++;	
	}

	return sem_timedwait(psem, &(btime));
}

///////////////////////////////////////////////////////////////////////////////////
// 
// Function Name: GPSUpdate
// Parameters: 
// Description: 
//        called by hvgps.a every second
// Notes: sjchen 2012/09/04
//
///////////////////////////////////////////////////////////////////////////////////
void GPSUpdate(GpsLocation* loc, GpsSvStatus* sv_inf, GpsUtcTime timestamp, char strNMEA[40][512],int nNmeaNum)
{
	int  i;

	if(g_status_cb)
	{
		if(g_GPSInited)
		{
			if(g_GPSStarted)
			{
				g_status.status = GPS_STATUS_SESSION_BEGIN;
			}
			else 
			{
				g_status.status = GPS_STATUS_ENGINE_ON;

			}
		}
		else
		{
			g_status.status = GPS_STATUS_ENGINE_OFF;
		} 

		g_status_cb(&g_status);

	}

	if(g_GPSInited && g_GPSStarted)
	{

		if(loc && g_location_cb && loc->timestamp)
			//report lon,;lat, alt, speed, bearing---masked by flag
			(*g_location_cb)(loc);     

		if(sv_inf && g_sv_status_cb)
		{
			//report cn0, elevation, azimuth
			(*g_sv_status_cb)(sv_inf); 

		}
		if(strNMEA && g_nmea_cb && nNmeaNum > 0)
		{
			//output nmea sentence
			for(i = 0; i < nNmeaNum; i++)
			{
				g_nmea_cb(timestamp, strNMEA[i], strlen(strNMEA[i]));
			}

		}
	}
}


///////////////////////////////////////////////////////////////////////////////////
// 
// Function Name: PAL_CreateThread_JNI
// Parameters: 
// Description: 
//        It may be called by hvgps.a,so it's name could not be modifed.
// Notes: sjchen 2012/09/04
//
///////////////////////////////////////////////////////////////////////////////////
void * PAL_CreateThread_JNI ( void * entry, void * pData, unsigned long u32Flag )
{
    pthread_t  tid;
    typedef void (*THREAD_FUNC)(void*); 
    tid = g_create_thread_cb("hvgps trd", (THREAD_FUNC)entry ,pData);

    return (void *)tid; 
}


///////////////////////////////////////////////////////////////////////////////////
// 
// Function Name: hv_gps_init
// Parameters: 
// Description: 
//        Initialize GPS
// Notes: sjchen 2012/09/04
//
///////////////////////////////////////////////////////////////////////////////////
static int hv_gps_init(GpsCallbacks* callbacks)
{
	LOGE("GPSInit()----------Entering");

	if(sem_init(&s_hStopSem,0,1))
	{
		LOGE("Create stop sem error\n");
		return -1;
	}

	g_location_cb = callbacks->location_cb;
	g_status_cb = callbacks->status_cb;
	g_sv_status_cb = callbacks->sv_status_cb;
	g_nmea_cb = callbacks->nmea_cb;
	g_create_thread_cb = callbacks->create_thread_cb;

	gps_set_capabilities set_capabilities_cb;
	set_capabilities_cb = callbacks->set_capabilities_cb;
	set_capabilities_cb(0);	

	g_GPSInited = true;

	LOGE("GPSInit()----------done");

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////
// 
// Function Name: hv_gps_cleanup
// Parameters: 
// Description: 
//        clean GPS
// Notes: sjchen 2012/09/04
//
///////////////////////////////////////////////////////////////////////////////////
static void hv_gps_cleanup(void)
{
	LOGE("hv_gps_cleanup--------------------------");
	
	WaitSem(&s_hStopSem,180000); //3 minute
return;


	g_GPSInited = false;
	g_GPSStarted = false;
	g_location_cb = NULL;
	g_status_cb = NULL;
	g_sv_status_cb = NULL;
	g_nmea_cb = NULL;

	sem_post(&s_hStopSem);
	sem_destroy(&s_hStopSem);
}


///////////////////////////////////////////////////////////////////////////////////
// 
// Function Name: StopThread
// Parameters: 
// Description: 
//        Stop GPS thread
// Notes: sjchen 2012/09/04
//
///////////////////////////////////////////////////////////////////////////////////
void  StopThread(void *pData)
{
	
	GPS_Stop();

	DeinitGbc();

	D("GPS Stopped**********");

	g_GPSStarted = false;

  g_status.status = GPS_STATUS_ENGINE_OFF;

  g_status_cb(&g_status);

	sem_post(&s_hStopSem);
}

///////////////////////////////////////////////////////////////////////////////////
// 
// Function Name: hv_gps_start
// Parameters: 
// Description: 
//        Start GPS
// Notes: sjchen 2012/09/04
//
///////////////////////////////////////////////////////////////////////////////////
static int hv_gps_start()
{
	GpsSvStatus sv_inf;

	LOGE("GPS Starting=======================");

	//Clear the SV info 
	memset(&sv_inf,0,sizeof(GpsSvStatus));
	sv_inf.size = sizeof(GpsSvStatus);
	(*g_sv_status_cb)(&sv_inf);

	WaitSem(&s_hStopSem,180000); //3 minute

  if(g_GPSStarted)
  {
         sem_post(&s_hStopSem);
         return -1;
  }

	unsigned long  u32ParameterAge = 604800;

	//for save GPS temporary file
	SetOutputFolder((char *)"/data/gpsdata");

	if(!InitGbc())
	{
		LOGE("InitGbc failed \r\n");
		return -1;
	}

	if(LoadParameter())
	{
		u32ParameterAge = GetParameterAge();
	}	

	if(u32ParameterAge < 7000) //about 2 hours 
	{
		GPS_HotStart();
	}
	else if(u32ParameterAge < 86400) // one day
	{
		GPS_WarmStart();
	}
	else 
	{
		GPS_ColdStart();
	}

	D("GPS Started %d\r\n",u32ParameterAge);

	g_GPSStarted = true;

	D("%s: called", __FUNCTION__);

	LOGE("GPS Starting====================done");


	sem_post(&s_hStopSem);

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////
// 
// Function Name: hv_gps_stop
// Parameters: 
// Description: 
//        stop GPS 
// Notes: sjchen 2012/09/04
//
///////////////////////////////////////////////////////////////////////////////////
static int hv_gps_stop()
{
	D("About to deinitgbc");
	
	WaitSem(&s_hStopSem,180000); //3 minute

	PAL_CreateThread_JNI( (void *)StopThread, NULL, 0 );

	return 0;
}

void hv_gps_reset()
{
}

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       I N T E R F A C E                               *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/
static int hv_gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
	LOGI("Enter %s,%lld,%lld,%d \r\n",__FUNCTION__,time,timeReference,uncertainty);
	return 0;
}

static int hv_gps_inject_location(double latitude, double longitude, float accuracy)
{
	LOGI("Enter %s,%f,%f,%f \r\n",__FUNCTION__,latitude,longitude,accuracy);
	return 0;
}

static void hv_gps_delete_aiding_data(GpsAidingData flags)
{
	LOGI("Enter %s,%d \r\n",__FUNCTION__,flags);

}

static int hv_gps_set_position_mode23(GpsPositionMode mode, GpsPositionRecurrence recurrence,
                                    uint32_t min_interval, uint32_t preferred_accuracy, 
                                    uint32_t preferred_time)
{
  	LOGI("[%s]:%d,%d,%d,%d,%d \n",__FUNCTION__,
		recurrence,mode,(int)min_interval,(int)preferred_accuracy,(int)preferred_time);
	return 0;
}


static const void* hv_gps_get_extension(const char* name)
{
	LOGI("Enter %s,%s \r\n",__FUNCTION__,name);
	//TODO: AGPS_INTERFACE  or GPS_NI_INTERFACE
	return NULL;
}


static const GpsInterface  HvGpsInterface = {
	sizeof(GpsInterface), //added in 2.3
	hv_gps_init,
	hv_gps_start,
	hv_gps_stop,
	hv_gps_cleanup,
	hv_gps_inject_time,
	hv_gps_inject_location,
	hv_gps_delete_aiding_data,
	hv_gps_set_position_mode23,
	hv_gps_get_extension,
};

/*
__attribute__((visibility("default"))) void IAMCSJ()
{

}*/
#define EXPORT_VISIBLE __attribute__((visibility("default")))
EXPORT_VISIBLE const GpsInterface* gps_get_hardware_interface() 
{
	return &HvGpsInterface;
}



