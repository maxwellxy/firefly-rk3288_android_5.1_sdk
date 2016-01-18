#include <time.h>
#include <string.h>

#include "../inc/gps_ptypes.h"
#include "../inc/GN_GPS_api.h"
#include "../inc/GN_GPS_DataLogs.h"
#include "../inc/GN_GPS_Example.h"

#include <utils/Atomic.h>
#include <linux/android_alarm.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <fcntl.h>
#include <utils/Log.h>

//#include <time.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <stdlib.h>

#ifndef Android_OS
#define	Android_OS
#endif
//****************************************************************************
// External data
extern s_CircBuff  UARTrx_cb;     // UART Rx circular buffer
extern s_CircBuff  UARTtx_cb;     // UART Tx circular buffer

#define NONVOL_FNAME   "/data/misc/gps/GN_NonVol_Example.txt"  // Non-volatile data file name
#define RTC_CALIB_FILE "/data/misc/gps/GN_RTC_Cal_Example.txt" // RTC calibration file name

#define     RTC_DRIFT       10  // RTC drift uncertainty = 10 ppm
#define 		DIFF_REF_C_TIME ((365*10+2+5)*(24*60*60))
 
static const U2 Days_to_Month[12] =
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

// Number of Days in a Month look up table.
static const U1 Days_in_Month[] =
{
    31,   // Jan
    28,   // Feb
    31,   // Mar
    30,   // Apr
    31,   // May
    30,   // Jun
    31,   // Jul
    31,   // Aug
    30,   // Sep
    31,   // Oct
    30,   // Nov
    31    // Dec
};

/************************************************************
 * The following is 4 functions for file system on MT6225   *
 ************************************************************/
extern FS_HANDLE FS_Open(const WCHAR * FileName, UINT Flags);
extern int FS_Read(FS_HANDLE File, void * DataPtr, UINT Length, UINT * Read);
extern int FS_Close (FS_HANDLE File);
extern int FS_Write(FS_HANDLE File, void * DataPtr, UINT Length, UINT * Written);
extern U2 Gps7560_write_nmea(CH* ptr, U2 bytes);

/************************************************************
 * The following is a function for RTC on MT6225			*
 ************************************************************/
extern  U4 get_tick_count();

#ifdef Android_OS
void getTimeFromRtc(struct tm *t)
{
    int rtc;
    if (( rtc = open ( "/dev/rtc", O_RDONLY )) < 0 ) {
        printf( "Could not access RTC\n" );
        return;
    }

    memset ( &t, 0, sizeof( struct tm ));

    if ( ioctl ( rtc, RTC_RD_TIME, &t ) < 0 ) {
        printf( "Could not read time from RTC\n" );
        return;
    }
	
    t->tm_isdst = -1; // not known

    close ( rtc );
    printf("get time-----year:%d",t->tm_year);
    printf("get time-----month:%d",t->tm_mon);
    printf("get time-----day:%d",t->tm_mday);
    printf("get time-----hour:%d",t->tm_hour);
    printf("get time-----minute:%d",t->tm_min);
    printf("get time-----second:%d",t->tm_sec);
}

//*****************************************************************************
// Converts a UTC Time in Date & Time format to a GPS seconds format.
U4 Convert_UTC_GPSsecs(
   U2 Year,                // i  - UTC Year         [eg 2006]
   U2 Month,               // i  - UTC Month        [range 1..12]
   U2 Day,                 // i  - UTC Days         [range 1..31]
   I4 Hours,               // i  - UTC Hours        [range -8..23]
   U2 Minutes,             // i  - UTC Minutes      [range 0..59]
   U2 Seconds )            // i  - UTC Seconds      [range 0..59]
 {
   // Local Definitions
   U4 ret_val;             // Return value
   I4 dayOfYear;           // Completed days into current year
   I4 noYears;             // Completed years since 1900
   I4 noLeapYears;         // Number of leap years between 1900 and present
   I4 noDays;              // Number of days since midnight, Dec31/Jan 1, 1900.
   I4 noGPSdays;           // Number of days since start of GPS time

   // Compute the day number into the year
   dayOfYear = (I4)( Days_to_Month[Month-1] + Day );

   // Leap year check.  (Note that this algorithm fails at 2100!)
   if ( Month > 2  &&  (Year%4) == 0 )  dayOfYear++;

   // The number of days between midnight, Dec31/Jan 1, 1900 and
   // midnight, Jan5/6, 1980 (ie start of GPS) is 28860.

   noYears     = (I4)Year - 1901;
   noLeapYears = noYears / 4;
   noDays      = (noYears*365) + noLeapYears + dayOfYear;
   noGPSdays   = noDays - 28860;                    // Number of GPS days
   ret_val     = noGPSdays*86400 + Hours*3600 + Minutes*60  + Seconds;

   ret_val = ret_val + DIFF_REF_C_TIME;

   return(ret_val);
}

//*****************************************************************************
// Convert GPS seconds format to UTC and set the return structure
void Convert_GPSsecs_UTC(
   U4* TimeSec,            // i  - GPS seconds
   U2*   Year,               //  o - Year         [eg 2006]
   U2*   Month,              //  o - Month        [range 1..12]
   U2*   Day,                //  o - Days         [range 1..31]
   U2*   Hours,              //  o - Hours        [range 0..23]
   U2*   Minutes,            //  o - Minutes      [range 0..59]
   U2*   Seconds )           //  o - Seconds      [range 0..59]
 {
   I4 GPS_secs;               // GPS Seconds since 5/6 Jan 1980.

   I2 gpsWeekNo;              // GPS Week Number
   I4 gpsTOW;                 // GPS Time of Week [seconds]
   I4 loSecOfD;               // Local Second of Day    (range 0-86399)
   I4 loYear;                 // Local Year             (range 1980-2100)
   I4 loDayOfW;               // Local Day of Week      (range 1-7)
   I4 loDayOfY;               // Local Day of Year      (range 1-366)
   I4 loSecOfH;               // Local Second of Hour   (range 0-3599)
   I4 i;                      // Loop index
   I4 tempI4;                 // Temporary I4 value

   // Convert from 'C' Time to GPS Time.
   GPS_secs = (*TimeSec) - DIFF_REF_C_TIME; 

   // Convert UTC Time of Week to Day of Week, Hours, Minutes and Seconds
   gpsWeekNo = GPS_secs /604800;
   gpsTOW    = GPS_secs - 604800*gpsWeekNo;

   loDayOfW  = gpsTOW / 86400;          // Calculate completed Days into Week
   loSecOfD  = gpsTOW - 86400*loDayOfW; // Calculate current Second of Day
   tempI4    = loSecOfD / 3600;           // Calculate current Hour of Day
   *Hours    = (U2)tempI4;                // Store current Hour of Day
   loSecOfH  = loSecOfD - 3600*tempI4;    // Calculate current Second of Hour
   tempI4    = loSecOfH / 60;             // Calculate current Minute of Hour
   *Minutes  = (U2)tempI4;                // Store current Minute of Hours
   *Seconds  = (U2)(loSecOfH - 60*tempI4);// Calc & Store current Minute of Second

   // Convert day of week and week number to day of year (tempI4) and year

   tempI4  = loDayOfW + (I4)gpsWeekNo*7;  // Calculate GPS day number
   tempI4  = tempI4 + 6;                  // Offset for start of GPS time 6/1/1980
   loYear  = 1980;

   // Advance completed 4 years periods,  which includes one leap year.
   // (Note that this algorithm fails at 2100, which is not a leap year.)
   while ( tempI4 > ((365*4) + 1) )
   {
      tempI4 = tempI4 - ((365*4) + 1);
      loYear = loYear + 4;
   };
   // Advance remaining completed years, which don't include any leap years.
   // (Note that this algorithm fails at 2100, which is not a leap year.)
   while ( tempI4 > 366 )
   {
      tempI4 = tempI4 - 365;
      if ( (loYear & 0x3) == 0 ) tempI4--;   // TRUE for leap years (fails at 2100)
      loYear++;
   };
   // Check for one too many days in a non-leap year.
   // (Note that this algorithm fails at 2100, which is not a leap year.)
   if ( tempI4 == 366  &&  (loYear & 0x3) != 0 )
   {
      loYear++;
      tempI4 = 1;
   }

   loDayOfY = tempI4;
   *Year    = (U2)loYear;
   // Convert Day of Year to Day of Month and Month of Year
   for ( i=0 ; i<12 ; i++ )
   {
      if ( loDayOfY <= Days_in_Month[i] )
      {
         *Day   = (U2)loDayOfY;
         *Month = (U2)i+1;
         break;
      }
       else
      {
         loDayOfY = loDayOfY - Days_in_Month[i];

         // Check for the extra day in February in Leap years
         if ( i == 1  &&  (loYear & 0x3) == 0 )   // Only Works up to 2100
         {
            if ( loDayOfY > (29 - 28) )    // After Feb 29th in a Leap year
            {
               loDayOfY--;             // Take off the 29th Feb
            }
            else                    // Must be the 29th of Feb on a Leap year
            {
               *Day     = (U2)29;
               *Month   = (U2)2;
               break;
            }
         }
      }
   }

   return;
}
#endif

//*****************************************************************************
// GN GPS Callback Function to request that the host start uploading a Patch
// File to the GN GPS Baseband for the given ROM code version. The host must
// ensure that if the data in this file is split amongst several writes then
// this is done at a <CR><LF> boundary so as to avoid message corruption by the
// Ctrl Data also being sent to the Baseband.
// Returns the Patch Checksum, or zero if there is no Patch.
U2 GN_GPS_Write_GNB_Patch(
   U2 ROM_version,                  // i - Current GN Baseband ROM version
   U2 Patch_CkSum )                 // i - Current GN Baseband Reported Patch
{
   char  temp_buffer[80];
   int   nWritten;
   U4    mstime;
   U2    ret_val;

   // Record some details in the event log - for diagnostics purposes only
   mstime = GN_GPS_Get_OS_Time_ms();
   nWritten = sprintf(temp_buffer,
                      "\n\rGN_GPS_Write_GNB_Patch, V %3d, OS ms time %7d\n\r",
                      ROM_version,
                      mstime );
   GN_GPS_Write_Event_Log((U2)nWritten, (CH*)temp_buffer);

   // Setup the patch code upload process
   ret_val = Setup_GNB_Patch( ROM_version, Patch_CkSum );

   return(ret_val);
}



//*****************************************************************************
// GN GPS Callback Function to Read back the GPS Non-Volatile Store Data from
// the Host's chosen Non-Volatile Store media.
// Returns the number of bytes actually read back.  If this is not equal to
// 'NV_size' then it is assumed that the Non-Volatile Store Data read back is
// invalid and Time-To_First_fix will be significantly degraded.
U2 GN_GPS_Read_NV_Store(
   U2 NV_size,                      // i - Size of the NV Store Data [bytes]
   U1 *p_NV_Store )                 // i - Pointer to the NV Store Data
{
   // In this example implementation the non-volatile data is stored in a
   // file on the local disk.
   // Note that, in a real product, it would probably not be desirable to
   // read data from a file at this point, because it would be relatively slow.
   // A better approach might be to read the file before the GPS was started,
   // and to store the data in memory.
   FS_HANDLE  f_NVol;

   UINT    num_read;        // Number of bytes read

   memset( p_NV_Store, 0, NV_size );

   f_NVol = FS_Open( NONVOL_FNAME, O_RDONLY);
   if ( f_NVol < 0 )
   {

      GN_GPS_Write_Event_Log(41, "WARNING: Non-volatile data not available\n");
	    return(0);
   }

   num_read =  FS_Read(f_NVol, p_NV_Store, NV_size, NULL);

   FS_Close(f_NVol);
   // Check that the correct number of bytes were read
   if ( num_read != NV_size )
   {
        int  n;
        char str[128];
        n = sprintf(str,"WARNING: Incorrect size of non-volatile data. Expected size = %d, actual = %d\n",
                    NV_size, num_read);

        GN_GPS_Write_Event_Log( (U2)n, (CH*)str );

        memset( p_NV_Store, 0, NV_size );
        num_read = 0;
   }
   else
   {
        int  n;
        char str[128];

        n = sprintf(str,"================>Non-volatile data obtained OK\n");
        GN_GPS_Write_Event_Log(n, str);
   }
   return(num_read);	
}


//*****************************************************************************
// GN GPS Callback Function to Write the GPS Non-Volatile Store Data to the
// Host's chosen Non-Volatile Store media.
// Returns the number of bytes actually written.
void GN_GPS_Write_NV_Store(
   U2 NV_size,                      // i - Size of the NV Store Data [bytes]
   U1 *p_NV_Store )                 // i - Pointer to the NV Store Data
{
   // In this example implementation, the non-volatile data is stored in a
   // file on the local disk.
   // Note that, in a real product, it would probably not be desirable to
   // write data to a file at this point, because it would be relatively slow.
   // A better approach might be to write the data to memory, and to output
   // this to the file after the GPS has been stopped.
   FS_HANDLE  f_NVol;
   int  num_write;       // Number of bytes written

   f_NVol = FS_Open(NONVOL_FNAME, O_CREAT | O_RDWR);
   if ( f_NVol < 0 )
   {
        GN_GPS_Write_Event_Log(46, "WARNING: Could not open file in Write_NV_Data\n");
        return;
   }

   num_write  = FS_Write(f_NVol, p_NV_Store, NV_size, NULL);
   if ( num_write != NV_size )
   {
        int  n;
        char str[128];
        n = sprintf(str,"WARNING: Could not write all the non-volatile data. size = %d, written = %d\n",
                    NV_size, num_write);

        GN_GPS_Write_Event_Log( (U2)n, (CH*)str );
   }
   else
   {
        GN_GPS_Write_Event_Log(29, "Non-volatile data written OK\n");
        
   }
   FS_Close(f_NVol);
	 return;
}

//*****************************************************************************

// GN GPS Callback Function to Get the current OS Time tick in integer
// millisecond units.  The returned 'OS_Time_ms' must be capable of going all
// the way up to and correctly wrapping a 32-bit unsigned integer boundary.
U4 GN_GPS_Get_OS_Time_ms( void )
{
		U4 OS_time_ms;
		
		OS_time_ms = get_tick_count();
		return OS_time_ms;
}

//*****************************************************************************
// GN GPS Callback Function to Get the OS Time tick, in integer millisecond
// units, corresponding to when a burst of Measurement data received from
// GPS Baseband started.  This helps in determining the system latency.
// If it is not possible to determine this value then, return( 0 );
// The returned 'OS_Time_ms' must be capable of going all the way up to and
// correctly wrapping a 32-bit unsigned integer boundary.
U4 GN_GPS_Get_Meas_OS_Time_ms( void )
{
   // This is not implemented.
   return(0);
}

//*****************************************************************************
// GN GPS Callback Function to Read back the current UTC Date & Time data
// (eg from the Host's Real-Time Clock).
// Returns TRUE if successful.
BL GN_GPS_Read_UTC(
   s_GN_GPS_UTC_Data *p_UTC )       // i - Pointer to the UTC Date & Time
{
   // In this example implementation, the PC's RTC is calibrated by the GPS
   // That is, the offset between the GPS time and the host's RTC time is
   // computed and stored in a file on the SD card. So, when the GPS
   // core makes this function call (i.e. calls GN_GPS_Read_UTC), we read
   // the content of the calibration file. If the file does not exist,
   // (i.e. the host's clock has not been calibrated) then we assume that the
   // uncalibrated RTC is accurate to 3 minutes RMS error (i.e. the worst-
   // case error is about 10 minutes). If the file does exist, then we
   // assume that the frequency error of the RTC may be up to 10 ppm RMS
   // (i.e. a worst-case error of about 30 ppm).
   //
   // Note that, if the error in the time returned by this function is
   // significantly greater than is indicated by the associated
   // accuracy estimate (i.e. 'Acc_Est' in the s_GN_GPS_UTC_Data structure)
   // then the performance of the GPS might be degraded..
   
   struct timeval tv_RTC;              // Local OS Real-Time-Clock Time in timeval format
   struct tm      tm_RTC;              // Local OS Real-Time-Clock Time in tm format    
	 BL   ret_val;        // Return value
	 U2   num_read ;
	 FS_HANDLE    f_RTC;        // RTC calibration file


   U4  currTimeSec;  // Current time in seconds since GPS week 0
   long        RTC_acc;      // Accuracy of the PC's RTC time (ms)

   U4       offset;       // Offset between RTC and UTC time (s)
   U4       time_set;     // Time when RTC was set
   U4       acc_set;      // Accuracy when RTC was set (ms)
   
//   RTC_acc_set    = 999999;      // Accuracy when RTC was set (ms)
//   RTC_OStime_set = 0;           // OS time when RTC was set (ms)

   ret_val = TRUE;

   memset( p_UTC, 0, sizeof(s_GN_GPS_UTC_Data) );
   p_UTC->Acc_Est  =  0x7FFFFF00;

   // Get the current OS time [ms]
   p_UTC->OS_Time_ms = GN_GPS_Get_OS_Time_ms();

   if ( gettimeofday( &tv_RTC, NULL ) < 0 )
   {
      LOGD( "GN_GPS_Read_UTC: gettimeofday() failed" );
      return( FALSE );
   }

   //LOGD( "GN_GPS_Read_UTC: gmtime_r() beginning" );
   if ( gmtime_r( &tv_RTC.tv_sec, &tm_RTC ) == NULL )
   {
      LOGD( "GN_GPS_Read_UTC: gmtime_r() failed." );
      return( FALSE );
   }

  // LOGD( "GN_GPS_Read_UTC: gmtime_r:%d-%d-%d, %d:%02d:%02d", tm_RTC.tm_year, tm_RTC.tm_mon
    //            ,tm_RTC.tm_mday,tm_RTC.tm_hour,tm_RTC.tm_min,tm_RTC.tm_sec);

   p_UTC->Year         = (U2)tm_RTC.tm_year + 1900;
   p_UTC->Month        = (U2)tm_RTC.tm_mon + 1;
   p_UTC->Day          = (U2)tm_RTC.tm_mday;
   p_UTC->Hours        = (U2)tm_RTC.tm_hour;
   p_UTC->Minutes      = (U2)tm_RTC.tm_min;
   p_UTC->Seconds      = (U2)tm_RTC.tm_sec;
   p_UTC->Milliseconds = (U2)tv_RTC.tv_usec / 1000;
   
   if ( p_UTC->Year < 2001)
   {
      // Real-Time Clock Time invlaid,  quit now without a UTC time.
      LOGD( "GN_GPS_Read_UTC: Date before 2001 (%d-%d-%d), Assuming its invalid.",
              p_UTC->Day, p_UTC->Month, p_UTC->Year );
      return( FALSE );
   }
#if 0
   // If there is no calibration data available, quit now without a valid starting UTC Time.
   if ( gn_RTC_Calib.CTime_Set == 0 )
   {
      LOGD( "GN_GPS_Read_UTC: No calibration data available" );
      return( FALSE );
   }
#endif   
   // Get the current time from the host clock, and convert to GPS seconds format
   currTimeSec = Convert_UTC_GPSsecs(p_UTC->Year,
                                     p_UTC->Month,
                                     p_UTC->Day,
                                     p_UTC->Hours,
                                     p_UTC->Minutes,
                                     p_UTC->Seconds );



    // Assume the 'raw' PC's clock is accurate to 3 minutes RMS.
    RTC_acc = (3*60*1000);     // 3 minutes, in ms

    num_read = 0;

	  f_RTC = FS_Open((const WCHAR *)RTC_CALIB_FILE, (UINT) O_RDONLY);
	  
    if(f_RTC < 0 )
    {
      		ret_val = FALSE;//p_UTC->Acc_Est= 60000;//get_tick_count() -RTC_OStime;
      		//Fwl_Printf(" RTC_CALIB_FILE fail\n");
    }
	  else
	  {

	      // Read the data.
	      // First read the time when RTC was set (i.e. the time when the PC's RTC was calibrated)

         FS_Read(f_RTC, &time_set, sizeof(U4), (UINT *) &num_read);
         if ( num_read == sizeof(time_set) )
         {
            // Now read the calibration offset
            FS_Read(f_RTC, &offset, sizeof(U4), (UINT *)&num_read);
            if ( num_read == sizeof(offset) )
            {
               // Now read the accuracy of the offset at the time
               // it was set
               FS_Read(f_RTC, &acc_set, sizeof(U4),(UINT *) &num_read);
            }
         }
	   }
	   
	   FS_Close( f_RTC );

      // If the calibration file was read correctly, apply the calibration
      // offset, and compute the expected degradation in accuracy due to the
      // RTC frequency error.
      // Allow up to 10 ppm frequency error RMS.
      if ( num_read == sizeof(U4) )
      {
         U4 DT;

         // Compute the elapsed time since the offset was determined
         DT = (currTimeSec - time_set);  // seconds

         if ( (acc_set >= 10000)   ||     // 10 seconds, in ms
              (DT >= (100*24*3600))  )    // 100 days, in seconds
         {
            // The accuracy when the calibration offset was determined was
            // 10 s (i.e. 10000 ms) or worse (i.e. not very good), or the
            // calibration is 100 or more days old. This means that the
            // calibration offset is not very reliable, so set the current
            // accuracy to 3 minutes RMS.


         }
         else if ( DT < 0 )
         {
            // Time has gone backwards! This probably means that the
            // PC's RTC has been reset. So, set the current
            // accuracy to 1 hour RMS, and reset the offset to 0 !!!
            int n;
            char str[64];
            n = sprintf(str, "RTC time has gone backwards: %ld\n", DT);
            GN_GPS_Write_Event_Log((U2)n, str);

            RTC_acc = (1*60*60*1000);  // 1 hour, in ms
            offset  = 0;

            ret_val = FALSE;
         }
         else
         {
            // The calibration looks to be valid, so compute the accuracy
            // of the corrected RTC, allowing 10 ppm degradation in accuracy
            // over time (RMS).
            // Note that DT is in seconds, but RTC_acc and acc are ms.

      //      RTC_acc = acc_set + DT / (1000000/(10*1000));
            RTC_acc = acc_set + DT / (1000000/(RTC_DRIFT*1000));

            // Limit the accuracy to 300 ms RMS (i.e. a worst-case error
            // of about 1 second). We do this because the resolution of the
            // time is only 1 seconds.
            if ( RTC_acc < 300 ) RTC_acc = 300;  // ms
         }

         // Apply the calibration correction
         currTimeSec = currTimeSec - offset;

         {
            int n;
            char str[128];
            n = sprintf(str, "Set initial RTC: %ld %ld %d %ld %d\n", time_set, offset, acc_set, DT, RTC_acc);
            GN_GPS_Write_Event_Log((U2)n,str);
         }

      }
      else
      {
         // The calibration file was not read correctly, so the PC's
         // RTC will be uncalibrated.
         GN_GPS_Write_Event_Log(40,"RTC calibration file not read correctly\n");
      }
  

   // Convert GPS seconds format to UTC and set the return structure
   Convert_GPSsecs_UTC( &currTimeSec,
                        &p_UTC->Year,       // [eg 2006]
                        &p_UTC->Month,      // [range 1..12]
                        &p_UTC->Day,        // [range 1..31]
                        &p_UTC->Hours,      // [range 0..23]
                        &p_UTC->Minutes,    // [range 0..59]
                        &p_UTC->Seconds );  // [range 0..59]

   // Set the remaining parameters in the return structure
   p_UTC->Milliseconds= GN_GPS_Get_OS_Time_ms();                // Milliseconds into second [0..999]
   p_UTC->Acc_Est     = (U4)RTC_acc;      // Time RMS Accuracy Estimate   [ms]

   return(ret_val);
}


//*****************************************************************************
// GN GPS Callback Function to Write UTC Date & Time data to the Host platform
// software area,  which can be used by the Host to update its Real-Time Clock.
void GN_GPS_Write_UTC(
   s_GN_GPS_UTC_Data *p_UTC )       // i - Pointer to the UTC Date & Time
{
   // Compute the difference between the time from the PC's RTC and that
   // provided by the GPS. This difference will be written to a data file,
   // to be applied subsequently as a calibration constant. Note that, for
   // simplicitly, we do not do this unless the year, month and day are the
   // same.
   struct timeval tv_RTC;              // Local OS Real-Time-Clock Time in timeval format
   struct tm      tm_RTC;              // Local OS Real-Time-Clock Time in tm format
	 FS_HANDLE    f_RTC;        // RTC calibration file
	 U4            currTimeSec;    // Current time (host clock) in seconds since GPS week 0
	 U4            currTimeSecGPS; // Current time (GPS clock) in seconds since GPS week 0

   // Only set the calibration offset if the accuracy of the UTC is
   // better than 1 second (i.e. 1000 ms).
   if ( p_UTC->Acc_Est > 1000 )      // 1 second, in ms
   {
      int n;
      char str[64];
      n = sprintf(str, "UTC accuracy is too poor to save: %d\n", p_UTC->Acc_Est);
      GN_GPS_Write_Event_Log((U2)n,str);

      return;
   }

#if 0
   if ( RTC_acc_set < 1000 ) // Current setting was accurate to better than 1 s
   {
      // Compute the current accuracy
      U4   currAcc;  // Current (degraded) accuracy (ms)
      U4   DT;       // ms

      DT = p_UTC->OS_Time_ms - RTC_OStime_set;

      // Allow RTC_DRIFT ppm degradation in accuracy over time (RMS).
      // Note that DT, RTC_acc and RTC_acc_set are all in ms.
      currAcc = RTC_acc_set + (DT /(1000000/RTC_DRIFT));

      if ( (currAcc < 50)            ||  // Better than 50 ms
           (currAcc < p_UTC->Acc_Est)  ) // Better than the new setting
      {
         // The current accuracy, even after degrading to allow for the
         // drift uncertainty, is better than the new one, or it is
         // better than 50 ms, so there is no need to reset the RTC
         // (i.e. the current setting is already sufficiently accurate)
       //  Fwl_Printf("GN_GPS_Write_UTC Skip set\n");
         GN_GPS_Write_Event_Log(9,"Skip RTC\n");

         return;
      }
   }
#endif

   if ( gettimeofday( &tv_RTC, NULL ) < 0 )
   {
         LOGD( "GN_GPS_Write_UTC:  gettimeofday() failed." );
         return;
   }
   
   if ( gmtime_r( &tv_RTC.tv_sec, &tm_RTC ) == NULL )
   {
         LOGD( "GN_GPS_Write_UTC:  gmtime_r() failed." );
         return;
   }

  // LOGD("GN_GPS_Write_UTC: gmtime_r():%d-%d-%d, %d:%02d:%02d", tm_RTC.tm_year, tm_RTC.tm_mon
    //            ,tm_RTC.tm_mday,tm_RTC.tm_hour,tm_RTC.tm_min,tm_RTC.tm_sec);

   // Get the current time from the host clock, and convert to GPS seconds format

   currTimeSec = Convert_UTC_GPSsecs((U2)tm_RTC.tm_year + 1900,
   																			(U2)tm_RTC.tm_mon + 1,
                                         (U2)tm_RTC.tm_mday,
                                         (I4)tm_RTC.tm_hour,
                                         (U2)tm_RTC.tm_min,
                                         (U2)tm_RTC.tm_sec );

   // Convert the GPS time to GPS seconds format
   currTimeSecGPS = Convert_UTC_GPSsecs( p_UTC->Year,        // [eg 2006]
                                         p_UTC->Month,       // [range 1..12]
                                         p_UTC->Day,         // [range 1..31]
                                         p_UTC->Hours,      // [range 0..23]
                                         p_UTC->Minutes,      // [range 0..59]
                                         p_UTC->Seconds);     // [range 0..59]

   {
      // Compute the difference between the two times
      U4  offset;            // seconds
      U4 time_set;     // seconds
      U4  acc_set;      // ms

      offset = currTimeSec - currTimeSecGPS;
      if ( p_UTC->Milliseconds >= 500 )  // i.e. if >= 0.5 seconds
      {
         offset = offset - 1;
      }

      f_RTC  = FS_Open( (const WCHAR *)RTC_CALIB_FILE, (UINT) O_CREAT | O_RDWR );
      if(f_RTC < 0 )
      {
         f_RTC = FS_Open( (const WCHAR *)RTC_CALIB_FILE, (UINT) O_CREAT | O_RDWR);
      }

      // Write the current PC time, offset and accuracy

      time_set = currTimeSec;
      acc_set  = p_UTC->Acc_Est;

      FS_Write( f_RTC , (void *)&time_set, sizeof(U4), NULL);
      FS_Write( f_RTC , (void *)&offset, sizeof(U4), NULL);
      FS_Write( f_RTC , (void *)&acc_set, sizeof(U4), NULL);

      FS_Close(f_RTC);

      {
         int n;
         char temp[256];
         n = sprintf(temp,"Write UTC:  %ld  %ld  %d\n",time_set, offset, acc_set );
         GN_GPS_Write_Event_Log(n,temp);
      }
	}
	return;
}


//*****************************************************************************
// GN GPS Callback Function to Read GPS Measurement Data from the Host's
// chosen GPS Baseband communications interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, 'max_bytes' is dynamically set to prevent a single Read
// operation from straddling the internal circular buffer's end wrap point, or
// from over writing data that has not been processed yet.
// Returns the number of bytes actually read.  If this is equal to 'max_bytes'
// then this callback function may be called again if 'max_bytes' was limited
// due to the circular buffer's end wrap point.
U2 GN_GPS_Read_GNB_Meas(
   U2  max_bytes,                   // i - Maximum number of bytes to read
   CH *p_GNB_Meas )                 // i - Pointer to the Measurement data.
{
   U2 num;

   num = 0;

   // Copy the data from the input circular buffer to the
   // Measurement Data
   while ( (UARTrx_cb.write != UARTrx_cb.read) && (num < max_bytes) )
   {
      p_GNB_Meas[num] = *UARTrx_cb.read;
      UARTrx_cb.read++;
      num++;
      if ( UARTrx_cb.read >= UARTrx_cb.end_buf)
      {
         UARTrx_cb.read = UARTrx_cb.start_buf;
      }
   }

   return(num);
}


//*****************************************************************************
// GN GPS Callback Function to Write GPS Control Data to the Host's chosen
// GPS Baseband communications interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.
U2 GN_GPS_Write_GNB_Ctrl(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_GNB_Ctrl )                 // i - Pointer to the Ctrl data.
{
   // Copy the data from the Ctrl Data to the output curcular buffer

   int num;               // number of (partial) bytes to write
   CH  *p_GNB_Ctrl_copy;  // Copy of the address of the start of the data

   p_GNB_Ctrl_copy = p_GNB_Ctrl;

   // First, compute the space to the wrap-point of the .
   // output circular buffer
   num = UARTtx_cb.end_buf - UARTtx_cb.write;
   if ( num < num_bytes )
   {
      // There is not enough space to the wrap-point for all
      // the data, so first just write data up to the wrap-point
      memcpy(UARTtx_cb.write, p_GNB_Ctrl_copy, num);

      // Reset the circular buffer
      UARTtx_cb.write = UARTtx_cb.start_buf;

      // Advance the pointer to the start address of the remaining data
      p_GNB_Ctrl_copy = p_GNB_Ctrl_copy + num;

      // Set the number of bytes remaining to be written
      num = num_bytes - num;
   }
   else
   {
      // All the data can be written in one go.
      num = num_bytes;
   }

   // Write the (remaining) data to the buffer
   memcpy(UARTtx_cb.write, p_GNB_Ctrl_copy, num);

   // Advance the circular buffer
   UARTtx_cb.write = UARTtx_cb.write + num;

   // Are we at the end of the buffer?
   if ( UARTtx_cb.write >= UARTtx_cb.end_buf )
   {
      UARTtx_cb.write = UARTtx_cb.start_buf;
   }

   return(num_bytes);
}

//*****************************************************************************
// GN GPS Callback Function to request that if possible the host should
// perform a Hard Power-Down Reset of the GN Baseband Chips.
// Returns TRUE is this is possible.
BL GN_GPS_Hard_Reset_GNB( void )
{
   // This is not implemented.
   return(FALSE);
}


//*****************************************************************************
// GN GPS Callback Function to Write GPS NMEA 183 Output Sentences to the
// the Host's chosen NMEA interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.
U2 GN_GPS_Write_NMEA(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_NMEA )                     // i - Pointer to the NMEA data.
{
   // Write the data to the log file
   Write_Data_To_Log( NMEA_LOG, num_bytes, p_NMEA );
	 Gps7560_write_nmea(p_NMEA, num_bytes);
   return(num_bytes);
}


//*****************************************************************************
// GN GPS Callback Function to Read $PGNV GloNav Propriatary NMEA Input
// Messages from the Host's chosen $PGNV communications interface.
// Internally the GN GPS Core library a circular buffer to store this data.
// Therefore, 'max_bytes' is dynamically set to prevent a single Read operation
// from straddling the internal circular buffer's end wrap point, or from
// over-writing data that has not yet been processed.
// Returns the number of bytes actually read.  If this is equal to 'max_bytes'
// then this callback function may be called again if 'max_bytes' was limited
// due to the circular buffer's end wrap point.
U2 GN_GPS_Read_PGNV(
   U2  max_bytes,                   // i - Maximum number of bytes to read
   CH *p_PGNV )                     // i - Pointer to the $PGNV data.
{
   // This is not implemented.
   return(0);
}


//*****************************************************************************
// Debug Callback Functions called by the GN GPS High Level Software library
// that need to be implemented by the Host platform software to capture debug
// data to an appropriate interface (eg UART, File, both etc).

// GN GPS Callback Function to Write GPS Baseband I/O communications Debug data
// to the the Host's chosen debug interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.
U2 GN_GPS_Write_GNB_Debug(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_GNB_Debug )                // i - Pointer to the GNB Debug data.
{
   // Write the data to the log file
   Write_Data_To_Log( ME_LOG, num_bytes, p_GNB_Debug );

   return(num_bytes);
}


//*****************************************************************************
// GN GPS Callback Function to Write GPS Navigation Solution Debug data to the
// Host's chosen debug interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.
U2 GN_GPS_Write_Nav_Debug(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_Nav_Debug )                // i - Pointer to the Nav Debug data.
{
   // Write the data to the log file
   Write_Data_To_Log( NAV_LOG, num_bytes, p_Nav_Debug );

   return(num_bytes);
}


//*****************************************************************************

// GN GPS Callback Function to Write GPS Navigation Library Event Log data
// to the Host's chosen debug interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.
U2 GN_GPS_Write_Event_Log(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_Event_Log )                // i - Pointer to the Event Log data.
{
   // Write the data to the log file
   Write_Data_To_Log( EVENT_LOG, num_bytes, p_Event_Log );

   return(num_bytes);
}

void GN_EE_Shutdown( void )
{
   // Nothing to do herefor the P-GPS Shutdown
  // EvLog_v( "GN_EE_Shutdown:  " );

   // De-Initialise the RXN_Mem_???? OS specific interface.
  // RXN_Mem_DeInitialise();

   return;
}

void GN_EE_Initialise( void )
{
	 return;
}

BL GN_EE_Set_GPS_Eph_El(
   s_GN_EE_GPS_Eph_El *p_Eph_El )
{
	 return 0;
}

   BL GN_EE_Get_GPS_Eph_El(
      U1 SV,                           // i  - Satellite identification (PRN)
                                       //         number [range 1..32].
      U2 WeekNo,                       // i  - Current full GPS Week No (including
                                       //         1024 roll overs) [range 0..4095].
      U4 TOW_s,                        // i  - Current GPS Time of Week
                                       //         [range 0..604799].
      s_GN_EE_GPS_Eph_El *p_Eph_El )  //  o - Pointer to where the EE module can
                                       //         write the GPS Ephemeris Elements to.
{
	 return 0;
}
//*****************************************************************************


