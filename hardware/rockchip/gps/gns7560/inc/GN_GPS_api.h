
//****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_GPS_api.h
//
// $Header: X:/MKS Projects/GN_API_doxygen/doxygen/rcs/GN_GPS_api.h 1.25 2010/07/09 16:53:28Z geraintf Rel $
// $Locker: $
//****************************************************************************

#ifndef GN_GPS_API_H
#define GN_GPS_API_H

#ifdef __cplusplus
   extern "C" {
#endif

//****************************************************************************
// File level documentation
/// \file
/// \brief
///      GN GPS Core Library - GPS External Interface API header file.
//
//-----------------------------------------------------------------------------
/// \ingroup    group_GN_GPS
//
/// \defgroup   GN_GPS_api_I  GN GPS Core Library - Internally Implemented GPS API.
//
/// \brief
///     GN GPS Core Library - GPS API definitions for functions implemented internally.
//
/// \details
///     GN GPS Core Library - GPS API definitions for the enumerated data types
///     structures and functions provided in the library and can be called by
///     the host software (ie inward called).
//
//-----------------------------------------------------------------------------
/// \ingroup    group_GN_GPS
//
/// \defgroup   GN_GPS_api_H  GN GPS Core Library - Host Implemented API
//
/// \brief
///     GN GPS Core Library - GPS API definitions for functions implemented by the host.
//
/// \details
///     GN GPS Core Library - GPS API definitions for the enumerated data types
///     structures and functions called by the library and must be implemented
///     by the host software to suit the target platform OS and hardware
///     configuration (ie outward called).
///     <p> All of these functions must be implemented in order to link a final
///     solution, even if only with a stub "{ return( 0 ); }" so say that the
///     particular action requested by the library is not supported.
//
//-----------------------------------------------------------------------------
/// \ingroup    group_GN_GPS
//
/// \defgroup   GN_GPS_api_D  GN GPS Core Library - Debug Output API
//
/// \brief
///     GN GPS Core Library - GPS API definitions for functions implemented by
///     the host to handle the library debug outputs.
//
/// \details
///     GN GPS Core Library - GPS API definitions for the functions called by
///     the library requesting the host software to stream or save GPS diagnostic
///     data to a suitable medium (eg a file or UART) as dictated by the target
///     platform's available resources.
///     <p> All these interfaces should be fully implemented during the early
///     stages of the integration as copies of this data will be requested for
///     any reported problems, or requested for integration assistance.
//
//*****************************************************************************

#include "gps_ptypes.h"

//*****************************************************************************
/// \addtogroup  GN_GPS_api_I
/// \{

//*****************************************************************************
/// \brief
///      GN GPS Patch Upload Status.
/// \details
///      Status of the GNS???? chip Baseband ROM software Patch upload process
///      being managed by the host software interface.
typedef enum // GN_Patch_Status
{
   GN_HL_SLEEP,               ///< The High-Level software has sent a Sleep command to the baseband.
   GN_HL_WAKING,              ///< The High-Level software has sent a WakeUp command to the baseband, and is now waiting to receive data.
   GN_PATCH_IN_PROCESS,       ///< The Baseband software Patch upload is in process.
   GN_PATCH_COMPLETE          ///< The Baseband software Patch upload has been completed.

}  e_GN_Patch_Status;         // GN GPS Patch Upload Status


//*****************************************************************************
/// \brief
///      GN GPS $PGNV Command Type Received.
/// \details
///      Details of the most recent GN GPS Proprietary NMEA $PGNV command type
///      received, if any, on the $PGNV NMEA input channel during the previous
///      #GN_GPS_Update() call.
typedef enum // GN_PGNV_Cmd
{
   GN_PGNV_NONE,              ///< No Command received.
   GN_PGNV_GSLP,              ///< $PGNVS,GSLP - "Go To Sleep" command received.
   GN_PGNV_WAKE,              ///< $PGNVS,WAKE - "Wake-up" command received.
   GN_PGNV_SDWN,              ///< $PGNVS,SDWN - "Shutdown" command received.
   GN_PGNV_RSET               ///< $PGNVS,RSET - "Re-Set" command received.

}  e_GN_PGNV_Cmd;             // GN GPS PGNV Command Type Received


//*****************************************************************************
/// \brief
///      GN GPS Core Library - Version Details.
typedef struct // GN_GPS_Version
{
   CH Name[32];               ///< Pointer to a GN GPS Nav Lib Version name string.
   U2 MajorVersNo;            ///< Major Version Number (ie y in yy.yy.xx).
   U2 MinorVersNo;            ///< Minor Version Number (ie y in xx.xx.yy).
   CH VerDate[8];             ///< GN GPS Library "Version" YYMMDD Date string.
   CH Date[16];               ///< GN GPS Library "compiled on" Date string.
   CH Time[16];               ///< GN GPS Library "compiled at" Time string.
   U2 BB_SW_Ver_ROM;          ///< GN GPS Baseband ROM software version number (0=Unknown).
   U2 BB_SW_Patch_Cs;         ///< GN_GPS Baseband ROM software patch checksum (0=patch not loaded)
   CH SysVersion[32];         ///< \brief   GN GPS System version string
                              ///  \details GN GPS System version string which is a combination of the
                              ///           Library version number & date and the Baseband ROM software
                              ///           version number and its current patch checksum (0000 = patch is
                              ///           not loaded yet) in the following format: \n
                              ///           #MajorVersNo.#MinorVersNo - #VerDate - #BB_SW_Ver_ROM - #BB_SW_Patch_Cs
}  s_GN_GPS_Version;          // GN_GPS Configuration Data Structure


//*****************************************************************************
/// \brief
///      GN GPS Sensitivity Mode states.
/// \details
///      GN GPS Sensitivity Mode states which can be set in the Configuration Data Structure.
///      The default mode is to use "dynamic" sensitivity mode (DynaTrack) which attempts to
///      optimally switch between a "high" and "normal" sensitivity mode state based on the
///      sensed environment.  "High" sensitivity mode is the best for indoor environments
///      but because it takes longer to search for weaker signals this can lead to longer
///      TTFF and signal re-acquisition times.  "Normal" sensitivity mode spends less time
///      looking for the signal and so gives faster TTFFs and re-acquisition times, but only
///      to a reduced sensitivity level.
typedef enum // GN_GPS_Sens_Mode
{
   GN_GPS_SENS_MODE_HIGH,     ///< High Sensitivity Mode, optimised for mainly indoor / handset usage.
   GN_GPS_SENS_MODE_NORMAL,   ///< Normal Sensitivity Mode, optimised for mainly automotive and outdoor usage.
   GN_GPS_SENS_MODE_DYNAMIC   ///< Dynamic Sensitivity Mode, optimally switching between the above High & Normal modes.

}  e_GN_GPS_Sens_Mode;        // GN GPS Sensitivity Mode states


//*****************************************************************************
/// \brief
///      GN GPS Cold Start TTFF Mode states.
/// \details
///      GN GPS Cold Start TTFF Mode states which can be set in the Configuration
///      Data Structure.
///      <p> This mode is provided because there is a trade-off between Cold
///      Start acquisition sensitivity and the time it takes to perform the
///      the satellite searches.  The weaker the signals searched for, the
///      longer each search step takes, and hence the TTFF will increase.
///      Using shorter less sensitive searches may reduce the TTFF significantly
///      in real world conditions because several time separated repeat search
///      attempts can be made.  This increases the likelihood that a signal
///      is no longer obscured behind a blockage, or a destructive multipath
///      signal fade has changed to constructive multipath signal gain.
///      <p> The default is a balanced approach between TTFF and Sensitivity.
///      Alternatively users may specify their preference for faster TTFF
///      or for increased sensitivity.
typedef enum // GN_GPS_Cold_TTFF
{
   GN_GPS_COLD_TTFF_BALANCE,  ///< The default balanced approach between speed and sensitivity.
   GN_GPS_COLD_TTFF_FASTER,   ///< Potentially faster cold start TTFF, but reduced cold start sensitivity.
   GN_GPS_COLD_TTFF_SENSITIVE ///< Increased sensitivity cold starts, but at potentially a slower cold start TTFF.

} e_GN_GPS_Cold_TTFF;         // GN GPS Cold Start TTFF Mode states


//*****************************************************************************
/// \brief
///      GN GPS Power verses Performance Mode.
/// \details
///      GN GPS Power verses Performance Mode states which can be set in
///      the Configuration Data Structure.
///      <p> This mode is provided because there is a trade-off between Power
///      Usage and Performance.  Efforts made to gain the highest performance
///      will invariably involve some extra power usage.  Whereas efforts made
///      to reduce power usage will involve making some assumptions, omitting
///      certain activities, etc, which could potentially lead to reduced
///      performance.
///      <p> The default is a balanced approach between performance and power
///      usage.  Alternatively users may specify their preference for
///      higher performance or reduced power usage.
typedef enum // GN_GPS_PowPerf
{
   GN_GPS_POW_PERF_DEFAULT,   ///< The default balanced approach between Power Usage and Performance.
   GN_GPS_POW_PERF_HI_PERF,   ///< Higher Performance, but at potentially increased power usage.
   GN_GPS_POW_PERF_LO_POW     ///< Lower power usage, but at potentially reduced performance.

} e_GN_GPS_PowPerf;           // GN GPS Power versus Performance Mode states


//-----------------------------------------------------------------------------
/// \brief
///      GN GPS Core Library Configuration Data.
/// \details
///      GN GPS Core Library Configuration Data for items that can be changed
///      at GPS run-time.
/// \anchor Output_Rate
/// <H3>NMEA Sentence Output Rate</H3>
///      The output period of the NMEA sentences are determined by the
///      \link #s_GN_GPS_Config::FixInterval FixInterval \endlink
///      multiplied by the respective output rates (i.e.
///      \link #s_GN_GPS_Config::GPGLL_Rate GPGLL_Rate, \endlink
///      \link #s_GN_GPS_Config::GPGGA_Rate GPGGA_Rate, \endlink
///      \link #s_GN_GPS_Config::GPGSA_Rate GPGSA_Rate, \endlink
///      \link #s_GN_GPS_Config::GPGST_Rate GPGST_Rate, \endlink
///      \link #s_GN_GPS_Config::GPGSV_Rate GPGSV_Rate, \endlink
///      \link #s_GN_GPS_Config::GPRMC_Rate GPRMC_Rate, \endlink
///      \link #s_GN_GPS_Config::GPVTG_Rate GPVTG_Rate, \endlink
///      \link #s_GN_GPS_Config::GPZDA_Rate GPZDA_Rate \endlink and
///      \link #s_GN_GPS_Config::PGNVD_Rate PGNVD_Rate \endlink).
///      <p> Setting the output rate to 0 suppresses the output of that NMEA sentence.
///
typedef struct // GN_GPS_Config
{
   e_GN_GPS_Sens_Mode SensMode;   ///< User's preferred Sensitivity Mode

   e_GN_GPS_Cold_TTFF ColdTTFF;  ///< \brief  Cold Start TTFF mode.  This setting allows the GPS to be
                                 ///          configured to either:
                                 ///          (a) The default mode for the GNS???? chip, which is a balanced approach.
                                 ///          (b) To be biased more towards a faster TTFF, but at a reduced acquisition sensitivity.
                                 ///          (c) To be biased more towards higher sensitivity, but at potentially slower TTFF.

   e_GN_GPS_PowPerf PowerPerf;   ///< \brief  Power Usage verses Performance mode.   This setting allows the GPS to be
                                 ///          configured to either:
                                 ///          (a) The default mode for the GNS???? chip,  which is a balanced approach.
                                 ///          (b) To be biased more towards better performance, but at potentially higher power usage.
                                 ///          (c) To be biased more towards lower power usage, but at potentially reduced performance.

   BL BGA_Chip;               ///< \brief   The GNS???? chip is in a BGA (Ball Grid Array) package, rather than CSP (Chip Scale Package).
                              ///           TRUE:  The GNS???? chip is BGA package (default).
                              ///           FALSE: The GNS???? chip is CSP package.

   BL c32KHz_NotFitted;       ///< \brief  Is the GNS???? chip 32KHz input clock "Not Fitted" ?.
                              ///          TRUE: The 32KHz clock input is definitely "Not Fitted".
                              ///          FALSE:  The GNS???? chip will auto detect whether the 32KHz
                              ///          input clock is present or not (default).

   BL SBAS_Enabled;           ///< \brief  Enable tracking and use of SBAS satellites (PRNs 120..138) and data.
                              ///          Currently only GNS7560 chip version ROM 510 (patched) is SBAS capable.
                              ///          Enabling SBAS reduces the number of tracking channels available for GPS satellites.
                              ///          Note that the EGNOS SBAS satellites over Europe are currently not enabled for ranging
                              ///          and, therefore, cannot be used for Navigation, but may consume tracking channels.

   U1 PosFiltMode;            ///< \brief Navigation Output Positon Filtering Mode
                              ///         0 : Raw Navigation Output
                              ///       >=1 : Navigation Output Filtering Mode enabled (default is mode 4).
                              ///       >=5 : Reserved for future enhancements.

   U2 FixInterval;            ///< Desired interval [milliseconds] between adjacent position fixes.  This should not be set less than 500ms.

   U2 H_AccEst_Mask[2];       ///< \brief  Horizontal position RMS Accuracy Estimate Masks [m] at which point a a suitable fix
                              ///          is deemed to have been obtained for the current for the current Operating Mode of
                              ///          [0]: Single "Push To Fix" required, and [1]: "Continuous Navigation" required.

   U2 V_AccEst_Mask[2];       ///< \brief  Vertical position RMS Accuracy Estimate Masks [m] at which point a a suitable fix
                              ///          is deemed to have been obtained for the current for the current Operating Mode of
                              ///          [0]: Single "Push To Fix" required, and [1]: "Continuous Navigation" required.

   U2 SleepModeTimeOut;       ///< Sleep Mode Time-Out [seconds], when the Baseband will automatically transition to Coma Mode.
   U4 ComaModeTimeOut;        ///< Coma Mode Time-Out [seconds],  when the Baseband will automatically transition to Coma Mode

   U1 GPGLL_Rate;             ///< NMEA $GPGLL (Geographic Position - Latitude, Longitude) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 GPGGA_Rate;             ///< NMEA $GPGGA (GPS Fix Data) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 GPGSA_Rate;             ///< NMEA $GPGSA (GNSS DOPS and Active Satellites) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 GPGST_Rate;             ///< NMEA $GPGST (GNSS Pseudorange Error Statistics) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 GPGSV_Rate;             ///< NMEA $GPGSV (GNSS Satellites in View) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 GPRMC_Rate;             ///< NMEA $GPRMC (Recommended Minimum GNSS Sentence) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 GPVTG_Rate;             ///< NMEA $GPVTG (Course Over Ground and Ground Speed) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 GPZCD_Rate;             ///< NMEA $GPZCD (output rate - OS Timestamp for the Fix [seconds]) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 GPZDA_Rate;             ///< NMEA $GPZDA (Time & Date) \ref Output_Rate "NMEA Sentence Output Rate".
   U1 PGNVD_Rate;             ///< NMEA $PGNVD (GN GPS Proprietary Diagnostics data) \ref Output_Rate "NMEA Sentence Output Rate".

   U1 ForceCold_Timeout;      ///< \brief    Force a Cold Re-Start if a valid fix has not been obtained within the defined time-out [minutes]
                              ///            ( 0      = Use the library default time-out,
                              ///              1..254 = Use this time-out [1..254 minutes],
                              ///              255    = Invalid - reserved for future use ).

   U2 NV_Write_Interval;      ///< \brief    Interval to make periodic calls to #GN_GPS_Write_NV_Store()
                              ///            while running [minutes].      0 = Never.
                              ///            (Eg, In-case power is lost before Sleep or Shutdown.)

   U2 Enable_Nav_Debug;       ///< GN Navigation Solution Debug Enabled level.    (0 = Disabled, >=1 = Enabled).
   U2 Enable_GNB_Debug;       ///< GN Baseband I/O Comms Debug Enabled level.     (0 = Disabled, >=1 = Enabled).
   U2 Enable_Event_Log;       ///< GN Navigation Library Event Log Enabled level. (0 = Disabled, >=1 = Enabled).

   R8 Reference_LLH[3];       ///< A Reference WGS84 Latitude, Longitude [degrees] and Ellipsoidal Height [metres] only used for debug outputs.

   void* Reserved;            ///< Reserved for future used.

} s_GN_GPS_Config;            // GN GPS Core Library Configuration Data


//*****************************************************************************
/// \brief
///      GN GPS UTC Date & Time Data used for the host Real-Time-Clock interface.
/// \details
///      The OS Time tick [milliseconds] must correspond as closely as possible
///      to the UTC Date & Time.
///      The UTC Time Accuracy RMS Estimate [milliseconds] must truly reflect the
///      accuracy of the given time with respect to the given OS Time tick.
///      Thus UTC Time must not contain any local time zone adjustments.
typedef struct // GN_GPS_UTC_Data
{
   U2 Year;                   ///< UTC Year A.D.                    [eg 2007].
   U2 Month;                  ///< UTC Month into year              [range 1..12].
   U2 Day;                    ///< UTC Days into month              [range 1..31].
   U2 Hours;                  ///< UTC Hours into day               [range 0..23].
   U2 Minutes;                ///< UTC Minutes into the hour        [range 0..59].
   U2 Seconds;                ///< UTC Seconds into minute          [range 0..59].
   U2 Milliseconds;           ///< UTC Milliseconds into the second [range 0..999].
   U4 Acc_Est;                ///< RTC Time Accuracy RMS Estimate   [milliseconds].
   U4 OS_Time_ms;             ///< Corresponding OS Time tick       [milliseconds].

}  s_GN_GPS_UTC_Data;         // GN GPS UTC Date & Time Data



//*****************************************************************************

#define NMEA_SV  16           ///< Number of entries in the #s_GN_GPS_Nav_Data structure satellite arrays

typedef struct GN_EE_GPS_Eph_El  // GN Extended Ephemeris - GPS Ephemeris Elements
{
   U1 SatID;            // Satellite ID (PRN) Number                 :  6 bits [1..32]
   U1 CodeOnL2;         // C/A or P on L2                            :  2 bits [0..3]
   U1 URA;              // User Range Accuracy Index                 :  4 bits [0..15]
   U1 SVHealth;         // Satellite Health Bits                     :  6 bits [0..63]
   U1 FitIntFlag;       // Fit Interval Flag                         :  1 bit  [0=4hrs, 1=6hrs]
   U1 AODA;             // Age Of Data Offset                        :  5 bits [x 900 sec]
   I1 L2Pflag;          // L2 P Data Flag                            :  1 bit  [0..1]
   I1 TGD;              // Total Group Delay                         :  8 bits [x 2^-31 sec]
   I1 af2;              // SV Clock Drift Rate                       :  8 bits [x 2^-55 sec/sec2]
   U2 Week;             // GPS Reference Week Number (inc roll over) : 12 bits [0..4095]
   U2 toc;              // Clock Reference Time of Week              : 16 bits [x 2^4 sec]
   U2 toe;              // Ephemeris Reference Time of Week          : 16 bits [x 2^4 sec]
   U2 IODC;             // Issue Of Data Clock                       : 10 bits [0..1023]
   I2 af1;              // SV Clock Drift                            : 16 bits [x 2^-43 sec/sec]
   I2 dn;               // Delta n                                   : 16 bits [x 2^-43 semi-circles/sec]
   I2 IDot;             // Rate of Inclination Angle                 : 14 bits [x 2^-43 semi-circles/sec]
   I2 Crs;              // Coefficient-Radius-sine                   : 16 bits [x 2^-5 meters]
   I2 Crc;              // Coefficient-Radius-cosine                 : 16 bits [x 2^-5 meters]
   I2 Cus;              // Coefficient-Argument_of_Latitude-sine     : 16 bits [x 2^-29 radians]
   I2 Cuc;              // Coefficient-Argument_of_Latitude-cosine   : 16 bits [x 2^-29 radians]
   I2 Cis;              // Coefficient-Inclination-sine              : 16 bits [x 2^-29 radians]
   I2 Cic;              // Coefficient-Inclination-cosine            : 16 bits [x 2^-29 radians]
   I4 af0;              // SV Clock Bias                             : 22 bits [x 2^-31 sec]
   I4 M0;               // Mean Anomaly                              : 32 bits [x 2^-31 semi-circles]
   U4 e;                // Eccentricity                              : 32 bits [x 2^-33]
   U4 APowerHalf;       // (Semi-Major Axis)^1/2                     : 32 bits [x 2^-19 metres^1/2]
   I4 Omega0;           // Longitude of the Ascending Node           : 32 bits [x 2^-31 semi-circles]
   I4 i0;               // Inclination angle                         : 32 bits [x 2^-31 semi-circles]
   I4 w;                // Argument of Perigee                       : 32 bits [x 2^-31 meters]
   I4 OmegaDot;         // Rate of Right Ascension                   : 24 bits [x 2^-43 semi-circles/sec]

}  s_GN_EE_GPS_Eph_El;        // Extended Ephemeris - GPS Ephemeris Elements

//-----------------------------------------------------------------------------
/// \brief
///      GN GPS Navigation Position Fix Type.
typedef enum // GN_GPS_FIX_TYPE
{
   GN_GPS_FIX_NONE,           ///< No Position Fix
   GN_GPS_FIX_ESTIMATED,      ///< Estimated (ie forward predicted) Position Fix
   GN_GPS_FIX_2D,             ///< Autonomous   2-Dimensional Position Fix
   GN_GPS_FIX_DIFF_2D,        ///< Differential 3-Dimensional Position Fix
   GN_GPS_FIX_3D,             ///< Autonomous   2-Dimensional Position Fix
   GN_GPS_FIX_DIFF_3D         ///< Differential 3-Dimensional Position Fix

}  e_GN_GPS_FIX_TYPE;         // GN GPS Navigation Position Fix Type

//-----------------------------------------------------------------------------
/// \brief
///      GN GPS Navigation Solution data.
/// \details
///      This data is sufficient to generate all the commonly used NMEA GPS
///      sentences of $GPGLL, $GPGGA, $GPGSA, $GPGST, $GPGSV, $GPRMC, $GPVTG
///      & $GPZDA.
typedef struct // GN_GPS_Nav_Data
{
   U4 Local_TTag;             ///< Local baseband millisecond time-tag [msec].
   U4 OS_Time_ms;             ///< Local Operating System Time [msec]
   U2 Year;                   ///< UTC Year A.D.                     [eg 2007].
   U2 Month;                  ///< UTC Month into the year           [range 1..12].
   U2 Day;                    ///< UTC Days into the month           [range 1..31].
   U2 Hours;                  ///< UTC Hours into the day            [range 0..23].
   U2 Minutes;                ///< UTC Minutes into the hour         [range 0..59].
   U2 Seconds;                ///< UTC Seconds into the hour         [range 0..59].
   U2 Milliseconds;           ///< UTC Milliseconds into the second  [range 0..999].
   I2 Gps_WeekNo;             ///< GPS Week Number.
   R8 Gps_TOW;                ///< Corrected GPS Time of Week [seconds].
   R8 UTC_Correction;         ///< Current (GPS-UTC) time zone difference [seconds].
   R8 X;                      ///< WGS84 ECEF X Cartesian coordinate [m].
   R8 Y;                      ///< WGS84 ECEF Y Cartesian coordinate [m].
   R8 Z;                      ///< WGS84 ECEF Z Cartesian coordinate [m].
   R8 Latitude;               ///< WGS84 Latitude  [degrees, positive North].
   R8 Longitude;              ///< WGS84 Longitude [degrees, positive East].
   R4 Altitude_Ell;           ///< Altitude above the WGS84 Ellipsoid [m].
   R4 Altitude_MSL;           ///< Altitude above Mean Sea Level [m].
   R4 SpeedOverGround;        ///< 2-dimensional Speed Over Ground [m/s].
   R4 CourseOverGround;       ///< 2-dimensional Course Over Ground [degrees].
   R4 VerticalVelocity;       ///< Vertical velocity [m/s, positive Up].
   R4 N_AccEst;               ///< Northing RMS 1-sigma (67%) Accuracy estimate [m].
   R4 E_AccEst;               ///< Easting  RMS 1-sigma (67%) Accuracy estimate [m].
   R4 V_AccEst;               ///< Vertical RMS 1-sigma (67%) Accuracy estimate [m].
   R4 H_AccMaj;               ///< Horizontal RMS 1-sigma (67%) Error ellipse semi-major axis [m].
   R4 H_AccMin;               ///< Horizontal RMS 1-sigma (67%) Error ellipse semi-minor axis [m].
   R4 H_AccMajBrg;            ///< Bearing of the Horizontal Error ellipse semi-major axis [degrees].
   R4 HVel_AccEst;            ///< Horizontal Velocity RMS 1-sigma (67%) Accuracy estimate [m/s].
   R4 VVel_AccEst;            ///< Vertical Velocity RMS 1-sigma (67%) Accuracy estimate [m/s].
   R4 PR_ResRMS;              ///< Standard deviation of the Pseudorange a posteriori residuals [m].
   R4 H_DOP;                  ///< HDOP (Horizontal Dilution of Precision).
   R4 V_DOP;                  ///< VDOP (Vertical Dilution of Precision).
   R4 P_DOP;                  ///< PDOP (3-D Position Dilution of Precision).

   e_GN_GPS_FIX_TYPE FixType; ///< Position Fix Type.
   BL Valid_2D_Fix;           ///< Is the published 2D position fix "Valid" relative to the required Horizontal Accuracy Masks ?
   BL Valid_3D_Fix;           ///< Is the published 3D position fix "Valid" relative to both the required Horizontal and Vertical Accuracy Masks ?
   U1 FixMode;                ///< Solution Fixing Mode (1 = Forced 2-D at Mean-Sea-Level, 2 = 3-D with automatic fall back to 2-D mode).

   U1 SatsInView;             ///< Satellites in View count.
   U1 SatsUsed;               ///< Satellites in Used for Navigation count.

   U1 SatsInViewSVid[NMEA_SV];///< Satellites in View SV id number [PRN].
   U1 SatsInViewSNR[NMEA_SV]; ///< Satellites in View Signal To Noise Ratio [dBHz].
   U2 SatsInViewAzim[NMEA_SV];///< Satellites in View Azimuth [degrees].
   I1 SatsInViewElev[NMEA_SV];///< Satellites in View Elevation [degrees].  If = -99 then Azimuth & Elevation angles are currently unknown.
   BL SatsInViewUsed[NMEA_SV];///< Satellites in View Used for Navigation ?

} s_GN_GPS_Nav_Data;          // GN GPS Navigation solution data


//*****************************************************************************
/// \brief
///      GN GPS Time Solution data.
/// \details
///      This data is primarily intended for Time & Frequency applications
typedef struct // GN_GPS_Time_Data
{
   U4 Local_TTag;             ///< Local baseband millisecond time-tag [msec].
   U4 OS_Time_ms;             ///< Local Operating System Time [msec]
   U2 Year;                   ///< UTC Year A.D.                     [eg 2007].
   U2 Month;                  ///< UTC Month into the year           [range 1..12].
   U2 Day;                    ///< UTC Days into the month           [range 1..31].
   U2 Hours;                  ///< UTC Hours into the day            [range 0..23].
   U2 Minutes;                ///< UTC Minutes into the hour         [range 0..59].
   U2 Seconds;                ///< UTC Seconds into the hour         [range 0..59].
   U2 Milliseconds;           ///< UTC Milliseconds into the second  [range 0..999].
   I2 Gps_WeekNo;             ///< GPS Week Number.
   R8 Gps_TOW;                ///< Corrected GPS Time of Week [seconds].
   R8 Loc_TOW;                ///< Local GPS Time of Week [seconds].
   R8 Clock_Bias;             ///< Receiver Clock Bias [seconds].
   R8 Clock_Drift;            ///< Receiver Clock Drift [seconds/second.]
   R8 UTC_Correction;         ///< Current (GPS-UTC) time difference [seconds].
   R4 T_AccEst;               ///< Time       RMS  1-sigma (67%) Accuracy estimate [s].
   R4 B_AccEst;               ///< Clock Bias RMS  1-sigma (67%) Accuracy estimate [s].
   R4 F_AccEst;               ///< Frequency  RMS  1-sigma (67%) Accuracy estimate [s/s].
   R4 T_DOP;                  ///< TDOP (Time Dilution of Precision)

   e_GN_GPS_FIX_TYPE FixType; ///< Position Fix Type.

} s_GN_GPS_Time_Data;         // GN GPS Time solution data


//*****************************************************************************
/// \brief
///      GN GPS Power Usage Allowed States.
/// \details
///      These states are used to tell the GN Baseband software how aggressively
///      it should be trying to conserve power.  This is because power saving is
///      obtained by reducing functionality or time spent looking for or at the
///      GPS signal which can result in some reduced performance (eg sensitivity,
///      accuracy and re-acquisition times).
typedef enum // GN_GPS_Power_Mode
{
   GN_GPS_POWER_ALLOWED_HIGH,    ///< High / Unlimited level of Power use is Allowed.
   GN_GPS_POWER_ALLOWED_MEDIUM,  ///< Only a Medium level of Power use is Allowed.
   GN_GPS_POWER_ALLOWED_LOW      ///< Only a Low level of Power use is Allowed.
}  e_GN_GPS_Power_Mode;


/// \}

//*****************************************************************************
/// \addtogroup  GN_GPS_api_H
/// \{

/// \brief
///      Memory size allocation required [bytes] for the storage area to save
///      the GN GPS Non-Volatile Store data passed to and from the host
///      implemented functions #GN_GPS_Read_NV_Store and #GN_GPS_Write_NV_Store.
#define SIZE_NV_STORE   (5248)

/// \}


//*****************************************************************************
//
// GN GPS External Interface API/Callback related function prototypes
//
//*****************************************************************************

//*****************************************************************************
// API Functions that may be called by the Host platform software.
//
/// \addtogroup  GN_GPS_api_I
/// \{

//*****************************************************************************
/// \brief
///      GN GPS API Function to return the current baseband software patch upload
///      status.
/// \returns
///      The current status of the patch upload process.
e_GN_Patch_Status GN_GPS_Get_Patch_Status( void );


//*****************************************************************************
/// \brief
///      GN GPS API Function to Get the Version Details of the GN GPS Library.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API functions #GN_GPS_Initialise(),
///      #GN_GPS_Update() and #GN_GPS_Shutdown().
/// \returns
///      Flag to indicate success or failure of the call.
/// \retval #TRUE if successful.
/// \retval #FALSE if unsuccessful.
BL GN_GPS_Get_Version(
   s_GN_GPS_Version* p_Version      ///< [in] Pointer to where the GPS Core Library should write the Version Details to.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to Set the Version Details of the GN GPS Library.
/// \details
///      GN GPS API Function to Set the Version Details of the GN GPS Software
///      with parameters from the host system wrapper software (eg the Version
///      Number & Date, or the Compiled Date & Time).
///      <p> This feature is provided so that the host software version
///      details will can be made to appear in the NMEA $PGNVR, VERS message and
///      the debug outputs.
///      Only non-zero or non-NULL inputs are applied.
///      <p> Structure fields:
///      <ul>
///         <li>#s_GN_GPS_Version::Name \copydoc s_GN_GPS_Version::Name</li>
///         <li>#s_GN_GPS_Version::MajorVersNo \copydoc s_GN_GPS_Version::MajorVersNo</li>
///         <li>#s_GN_GPS_Version::MinorVersNo \copydoc s_GN_GPS_Version::MinorVersNo</li>
///         <li>#s_GN_GPS_Version::VerDate \copydoc s_GN_GPS_Version::VerDate</li>
///         <li>#s_GN_GPS_Version::Date \copydoc s_GN_GPS_Version::Date</li>
///         <li>#s_GN_GPS_Version::Time \copydoc s_GN_GPS_Version::Time</li>
///      </ul>
///      can be changed.
///      <p> Structure fields
///      <ul>
///         <li>#s_GN_GPS_Version::BB_SW_Ver_ROM \copydoc s_GN_GPS_Version::BB_SW_Ver_ROM</li>
///         <li>#s_GN_GPS_Version::BB_SW_Patch_Cs \copydoc s_GN_GPS_Version::BB_SW_Patch_Cs</li>
///      </ul>
///      can not be changed.
///      <p> Since #s_GN_GPS_Version::SysVersion is automatically created from the
///      other fields, it will also reflect the changes, but cannot be set
///      directly itself.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API functions #GN_GPS_Initialise(),
///      #GN_GPS_Update() and #GN_GPS_Shutdown().
/// \returns
///      Flag to indicate success or failure of the call.
/// \retval #TRUE if successful.
/// \retval #FALSE if unsuccessful.
BL GN_GPS_Set_Version(
   s_GN_GPS_Version* p_Version      ///< [in] Pointer to where the GPS Core Library should get the Version Details from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function that performs the GN GPS Core Library Initialisation
///      at system start-up.
/// \details
///      This includes initialising any all memory it uses
///      to a zero state in case the host software OS has not done this.
///      <p> This API call must be made before any other GN GPS Library call.
///      Ideally the GPS hardware should have been powered up immediately beforehand.
///      <p> This API call sends the baseband a Wake-Up followed some initial
///      configuration data.
///      It then calls the host implemented API function #GN_GPS_Read_NV_Store() to
///      request that the host software return the previously saved GN GPS
///      Library's Non-Volatile Store data, and does some initial preparatory
///      processing of this data.
/// \attention
///      Unless otherwise specified, #GN_GPS_Initialise() must be called before all
///      other API's because it initialises memory and sets up pointers they use.
/// \returns
///      Nothing.
void GN_GPS_Initialise( void );


//*****************************************************************************
/// \brief
///     GN GPS API Function that performs all the GPS Position Fix related
///     Update activities.
/// \details
///     This is the main body of the GN GPS software run on the host.  It is
///     this function that makes all the API calls on the host software as
///     and when it needs any further interaction with the host.
///     <p> This function must be called from within the main loop in the host
///     implemented GPS task with a typical repeat period of not less than
///     100ms while the GPS is running.  This update period can be slowed down
///     or even suspended by the OS during periods of Sleep, provided it is
///     returned to the running rate on a WakeUp.
///     A faster loop update rate will help minimise the latency between
///     detecting the arrival of a new block of data from the baseband and
///     limits latency errors involved with the #s_GN_GPS_UTC_Data provided to the
///     host to set or calibrate the Real-Time Clock.
///     <p> GN_GPS_Update looks at all the data received from the baseband so far
///     and parses it for a complete raw measurement block.  Only once a
///     complete block is found is there any noticeable CPU load while it is
///     computing the new GPS navigation solution update, which will be during
///     one call every #s_GN_GPS_Config::FixInterval milliseconds.
/// \returns
///      Nothing.
void GN_GPS_Update( void );


//*****************************************************************************
/// \brief
///     GN GPS API Function that parses a $PGNV GN GPS Proprietary NMEA input
///     stream for command messages.
/// \details
///     For example, commands to issue Hot, Warm
///     and Cold Starts for TTFF (Time-To-First-Fix) testing.
///     <p> This function only needs be called on host platforms that require the
///     ability to handle these $PGNV messages.
///     <p> It should be called in the same loop context as #GN_GPS_Update().
///     The function makes a #GN_GPS_Read_PGNV() call on the host software to
///     request any new NMEA input characters received.
///     <p> It does very little else if there is no new data to handle, so there is
///     little to be gained by adding extra code to the main processing loop to
///     determine whether or not it should be called.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API function #GN_GPS_Update().
/// \returns
///      The type of command seen if it involves a change of operating mode
///      (eg Reset, Sleep, WakeUp, Shutdown).
e_GN_PGNV_Cmd GN_GPS_Parse_PGNV( void );


//*****************************************************************************
/// \brief
///      GN GPS API Function that performs all the GN GPS Library Shutdown
///      activities at the GPS system shutdown.
/// \details
///      This function should only be called after exiting the main processing
///      loop containing the #GN_GPS_Update API() call.
///      <p> This functions send a command to put the baseband into a Coma State
///      and calls the host implemented API function #GN_GPS_Write_NV_Store() to
///      request that the host software save the GN GPS Library's Non-Volatile
///      Store data.
/// \returns
///      Nothing.
void GN_GPS_Shutdown( void );


//*****************************************************************************
/// \brief
///      GN GPS API Function to Wake Up the GPS from a Sleep or Coma state.
/// \details
///      GN GPS API Function to Wake Up the GPS to start a period of navigation.
///      This assumes that the GPS has already been initialised and is waiting
///      in a minimum power Sleep, Coma but not a DeepComa state.
///      The complement to this function is #GN_GPS_Sleep().
///      <p> The Wake-Up is achieved by sending messages to the baseband and
///      therefore cannot wake up the baseband from a DeepComa state which will
///      require some hardware intervention (eg toggling the RST_N line or a
///      full power cycle).
///      <p> Once woken up, it is up to the host software to monitor the GPS output
///      and quality metrics, and the end user application(s) requirements to
///      decide when the current period of Navigation is no longer required.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API function #GN_GPS_Update().
/// \returns
///      Nothing.
void GN_GPS_WakeUp( void );


//*****************************************************************************
/// \brief
///      GN GPS API Function to put the GPS into Sleep, Coma or DeepComa state.
/// \details
///      GN GPS API Function to stop a period of GPS navigation and put the
///      software and GPS Baseband into a low power Sleep, Coma or DeepComa
///      state pending being woken up for the next period of navigation.
///      The complement to this function is #GN_GPS_WakeUp().
///      <p> A GPS Baseband Sleep state is a low power mode but the millisecond
///      time counter is kept running so that sub-millisecond time can be
///      maintained throughout, which allows for a very fast TTFF on Wake-Up.
///      Currently the maximum Sleep Mode Time out is 60 seconds.
///      <p> A GPS Baseband Coma state is a lower power mode where the millisecond
///      time counter has been stopped but a crude time is maintained via a 32
///      KHz crystal.  Millisecond quality time can be maintained in this mode
///      within the stability of the 32KHz clock and can give a starting time
///      on Wake-Up which is substantially better than that obtained from the
///      host Real-Time-Clock.
///      <p> A GPS Baseband Deep-Coma state is the lowest power mode where effectively
///      everything is off and time is not maintained.  Hardware intervention is
///      required to Wake_ip from a Deep-Coma state.
///      <p> Setting the #GN_GPS_Sleep()::SleepModeTimeOut to zero puts the GPS Baseband directly into Coma.
///      <p> Setting the #GN_GPS_Sleep()::ComaModeTimeOut to zero transitions the GPS baseband directly
///      from a Sleep to a Deep-Coma states.
///      <p> Setting both time-outs to zero puts the GPS Baseband directly into Deep-Coma.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API function #GN_GPS_Update().
void GN_GPS_Sleep(
   U2 SleepModeTimeOut,       ///< [in] Sleep Mode Time-Out [seconds], ie when an automatic transition is made to Coma Mode.
   U4 ComaModeTimeOut         ///< [in] Coma Mode Time-Out [seconds],  ie when an automatic transition is made to a Deep-Coma Mode.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to Get the current GPS Library Configuration data.
/// \details
///      Typically this function is called to get the Library defaults for all
///      configurable data items prior to calling #GN_GPS_Set_Config() to change
///      a particular item.
///      <p> Typically this function is called the once after #GN_GPS_Initialise()
///      and before entering the main #GN_GPS_Update() processing loop.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API functions #GN_GPS_Initialise() or
///      #GN_GPS_Update().
/// \returns
///      Flag to indicate whether the configuration settings are valid.
/// \retval #TRUE if valid.
/// \retval #FALSE if invalid.
BL GN_GPS_Get_Config(
   s_GN_GPS_Config* p_Config        ///< [in] Pointer to where the Host Software can get the Configuration data from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to Set or change the GPS Library Configuration data.
/// \details
///      API function #GN_GPS_Get_Config() should be called to get the GPS Library
///      defaults for all  configurable data items prior to changing the items
///      of interest.
///      <p> Typically this function is called the once after #GN_GPS_Initialise()
///      and before entering the main #GN_GPS_Update() processing loop.
///      <p> Items should not be explicitly set to the Library default values as
///      these may be changed in subsequent releases of the library.
///      <p> New configuration data entered via this API is double buffered internally
///      so it is safe to call this function at any time.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API functions #GN_GPS_Initialise() or
///      #GN_GPS_Update().
/// \returns
///      Flag to indicate whether the new configuration settings were accepted.
/// \retval #TRUE if accepted.
/// \retval #FALSE if not accepted.
BL GN_GPS_Set_Config(
   s_GN_GPS_Config* p_Config  ///< [in] Pointer to where the GPS Library can get the Configuration data from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to Set the current Power Usage Allowed state.
/// \details
///      GN GPS API Functions to set the current Power Usage Allowed State,
///      between the levels High, Medium and Low.
///      <p> The Power Usage Allowed Mode only effects the behaviour of the GN GPS
///      Baseband  & RF components of the system and not the host processor
///      software behaviour as this is not the significant power consumer.
///      <p> #GN_GPS_POWER_ALLOWED_HIGH is the state that should be set when
///      external power (eg AC or Car battery) is provided to the mobile device
///      and therefore the level of power usage is not a concern.
///      <p> #GN_GPS_POWER_ALLOWED_MEDIUM is the state that should be set for mobile
///      devices running on normal battery power.  All efforts are made to minimise
///      power usage, while maintaining a highest level of positioning performance.
///      <p> #GN_GPS_POWER_ALLOWED_LOW is the state that should be set when running on
///      battery power, but conservation of the battery life is important enough that
///      some degradation in the positioning performance will be accepted as a trade-off.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API functions #GN_GPS_Initialise() or
///      #GN_GPS_Update().
/// \returns
///      Flag to indicate whether the new configuration settings were accepted.
/// \retval #TRUE if accepted.
/// \retval #FALSE if not accepted.
BL GN_GPS_Set_Power_Mode(
   e_GN_GPS_Power_Mode Power_Mode   ///< [in] Current Power Usage Allowed Mode State
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to Get the GPS Library's internal Non-Volatile
///      Store Data pointer and size.
/// \details
///      This API function may be used if the Host Software wishes to control
///      when the Non-Volatile Store data is written to the chosen Non-Volatile
///      storage media.
///      <p> This API function may be used to provide #GN_GPS_Clear_NV_Data()
///      with the pointer of the GPS Library's internal NV Store image.
/// \returns
///      Size of the Non-Volatile Store data image in bytes.
U2 GN_GPS_Get_NV_Store(
   U1 **p_NV_Store                  ///< [out] Pointer to where the host software can get the NV Store Data from.
 );


//*****************************************************************************
/// \brief
///      GN GPS API Function to Clear specified data items from a Non-Volatile
///      Store RAM image.
/// \details
///      GN GPS API Function to Clear specified data items from a Non-Volatile
///      Store RAM image located at the given pointer.
///      <p> The character string describing the #GN_GPS_Clear_NV_Data:Items to
///      be cleared matches the one used in the $PGNVS,RSET, input PGNV command.
///      <p> "WARM" or "COLD" will clear all the data required prior to a
///      Warm or Cold Start test.
///      <p> Any other input string will be read a character at a time for
///      a data category to be cleared as follows:
///         -  'P' = Position
///         -  'T' = Time
///         -  'F' = Reference TCXO Frequency calibration,
///         -  'E' = Ephemeredes for all satellites
///         -  'A' = Almanacs for all satellites
///         -  'I' = Ionospheric model parameters
///         -  'U' = UTC model correction parameters
///         -  'H' = Satellite Health data
///      <p> If Time is to be cleared then #GN_GPS_Write_UTC() will be called
///      to clear the host platform Real-Time Clock information.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API functions #GN_GPS_Initialise() or
///      #GN_GPS_Update(), otherwise the data just cleared may get re-instated.
/// \retval #TRUE if accepted and the specified Non-Volatile Data items were cleared.
/// \retval #FALSE if not accepted.
BL GN_GPS_Clear_NV_Data(
   U1 *p_NV_Ram,                    ///< [in] Pointer to the NV Store Data RAM image.
   CH Items[16]                     ///< [in] String describing the NV Store Data items to clear.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the latest GPS Navigation solution data.
/// \details
///      The data provided includes everything normally made available in the
///      the commonly used NMEA 183 format GPS sentences.
///      <p> This function can be used to poll for a new navigation update as
///      it will only return valid data for a new position fix just the once.
///      If the host software has multiple users of the navigation solution
///      data then the host is responsible for distributing it to those users.
///      <p> The data returned is double buffered so it is safe to call this
///      function from any OS task or thread.
/// \returns
///      Flag to indicate if there is valid GPS Navigation solution data that
///      has not already been requested via this API call.
/// \retval #TRUE if a new GPS Navigation solution data is available.
/// \retval #FALSE if a new GPS Navigation solution data is not available.
BL GN_GPS_Get_Nav_Data(
   s_GN_GPS_Nav_Data* p_Nav_Data    ///< [in] Pointer to where the GPS Library should write the Nav Data to.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the latest GPS Time solution data.
/// \details
///      The data provided via this API call includes all the data items
///      normally required by host applications targeting time and frequency
///      calibration application.
///      <p> This function can be used to poll for a new Time & Frequency solution
///      update as it will only return valid data for a new update just the once.
///      If the host software has multiple users of the time & frequency solution
///      data then the host is responsible for distributing it to those users.
///      <p> This data can be considered as supplemental to the data returned via
///      the #GN_GPS_Get_Nav_Data() API call.   Both of these API calls will flag that
///      new data as valid at the same time, but this should be confirmed by
///      ensuring that the #s_GN_GPS_Nav_Data::Local_TTag variable is the same in both the
///      #s_GN_GPS_Time_Data and the #s_GN_GPS_Nav_Data structures.
///      <p> The data returned is double buffered so it is safe to call this
///      function from any OS task or thread.
/// \returns
///      Flag to indicate if there is valid GPS Time solution data that
///      has not already been requested via this API call.
/// \retval #TRUE if a new GPS Time solution data is available.
/// \retval #FALSE if a new GPS Time solution data is not available.
BL GN_GPS_Get_Time_Data(
   s_GN_GPS_Time_Data* p_Time_Data  ///< [out] Pointer to where the GPS Library should write the Time Data to.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request NMEA 0183 compatible GPS sentences.
/// \details
///      This function can be used to poll for new NMEA data as it will
///      only return the new NMEA data bytes just the once.  If the host
///      software has multiple users of the navigation solution data then
///      the host is responsible for distributing it to those users.
///      <p> There is no latency associated the library implemented
///      #GN_GPS_Write_NMEA() API call method as it is made as soon as new
///      NMEA data is ready, but this has to be handled by the host in the
///      same OS task / thread that GN_GPS_Update is running in.
///      <p> GN_GPS_Get_NMEA can be called from any RTOS thread/task, but the
///      availability of new data will be a function of how frequently the
///      function is polled.
/// \attention
///      The host software must choose whether to obtain its NMEA data via
///      via either this GN_GPS_Get_NMEA() call, or whether to use the
///      #GN_GPS_Write_NMEA host implemented API, but not both.
/// \attention
///      This GPS API function is not re-entrant and must only be called the
///      same OS task/thread context as API function #GN_GPS_Update().
/// \returns
///      The number of bytes of NMEA data actually written to the host software.
///      This will be zero bytes if there is no new NMEA data available.
U2 GN_GPS_Get_NMEA(
   U2  max_size,              ///< [in] Maximum number of bytes of NMEA data the GPS Library can return in this call.
   CH* p_NMEA                 ///< [in] Pointer to where GPS Library should write the NMEA data to.
);


//*****************************************************************************
/// \brief
///      GN GPS Utility Function to Re-Qualify the contents of a GPS Navigation
///      Solution data structure.
/// \details
///      GN GPS Utility Function to Re-Qualify the contents of a GPS Navigation
///      Solution data structure #s_GN_GPS_Nav_Data against new Horizontal and
///      Vertical position Accuracy Requirements, #GN_GPS_ReQual_Nav_Data:H_AccReq
///      & #GN_GPS_ReQual_Nav_Data:V_AccReq.
///      <p>The #s_GN_GPS_Nav_Data::Valid_2D_Fix and #s_GN_GPS_Nav_Data::Valid_3D_Fix
///      flags are modified accordingly.
///      <p> This function may be used to set different validity states for
///      different end users in a multi-user system.
///      <p>It may also be used prior to passing the Navigation Solution Data
///      structure #s_GN_GPS_Nav_Data to the utility function #GN_GPS_Encode_NMEA
///      so that the validity flags in the NMEA sentences are set according to
///      the new Horizontal and Vertical position Accuracy Requirements.
/// \retval #TRUE  if the GPS Navigation solution Data is Qualified to at least a "Valid_2D_Fix" state.
/// \retval #FALSE if the GPS Navigation solution Data is not Qualified to at least a "Valid_2D_Fix" state.
BL GN_GPS_ReQual_Nav_Data(
   s_GN_GPS_Nav_Data *p_Nav_Data,   ///< [in/out] Pointer to where the Nav Data to Re-Qualify should write the Nav Data to
   U2 H_AccReq,                     ///< [in]     Horizontal position RMS Accuracy Requirement [metres],  0 = Use the Library default.
   U2 V_AccReq                      ///< [in]     Vertical   position RMS Accuracy Requirement [metres],  0 = Use the Library default.
);

//*****************************************************************************
/// \brief
///      GN GPS Utility Function to Encode NMEA 0183 compatible GPS sentences.
/// \details
///      GN GPS Utility Function to Encode NMEA 0183 compatible GPS sentences
///      to a given string buffer based on a given GPS Navigation Solution data
///      structure #s_GN_GPS_Nav_Data and a bit mask #GN_GPS_Encode_NMEA:Encode_Mask
///      specifying which NMEA sentances are required.
///      <p> This function may be used to generate different combinations of NMEA
///      sentences for different end users in a multi-user system.
///      <p> Adjusting the #s_GN_GPS_Nav_Data::Valid_2D_Fix and
///      #s_GN_GPS_Nav_Data::Valid_3D_Fix flags in the input
///      #GN_GPS_Encode_NMEA:p_Nav_Data structure and will correspondingly adjust
///      the GPS Fix status flags in the NMEA sentence outputs.
/// \attention
///      If the destination NMEA string buffer is not sufficiently large (as
///      defined by #GN_GPS_Encode_NMEA:max_size) to hold the requested NMEA
///      sentances (as defined by #GN_GPS_Encode_NMEA:Encode_Mask), then the
///      writing will wrap back to the begining of the string buffer and effectively
///      delete the #GN_GPS_Encode_NMEA:max_size bytes just written.
/// \returns
///      The number of bytes of NMEA data actually written to the string buffer.
U2 GN_GPS_Encode_NMEA(
   U2 max_size,                     ///< [in]  Maximum size of the NMEA string buffer
   CH *p_NMEA,                      ///< [out] Pointer to where the NMEA sentences should be written to
   U2 Encode_Mask,                  ///< [in]  Mask stating which NMEA messages are to be Encoded (see below)
   s_GN_GPS_Nav_Data *p_Nav_Data    ///< [in]  Pointer to where the Nav Solution Data should be read from
);
#define GN_GPGLL_ENC_MASK  (1<<0)   ///< NMEA $GPGLL sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_GPGGA_ENC_MASK  (1<<1)   ///< NMEA $GPGGA sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_GPGSA_ENC_MASK  (1<<2)   ///< NMEA $GPGSA sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_GPGST_ENC_MASK  (1<<3)   ///< NMEA $GPGST sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_GPGSV_ENC_MASK  (1<<4)   ///< NMEA $GPGSV sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_GPRMC_ENC_MASK  (1<<5)   ///< NMEA $GPRMC sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_GPVTG_ENC_MASK  (1<<6)   ///< NMEA $GPVTG sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_GPZCD_ENC_MASK  (1<<7)   ///< NMEA $GPZDA sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_GPZDA_ENC_MASK  (1<<8)   ///< NMEA $GPZDA sentence Encode Mask for use with function #GN_GPS_Encode_NMEA
#define GN_PGNVD_ENC_MASK  (1<<9)   ///< NMEA $PGNVD sentence Encode Mask for use with function #GN_GPS_Encode_NMEA

/// \}


//*****************************************************************************
// API Functions called by the GN GPS Core Library which need to be implemented
// by the Host platform software.
//
/// \addtogroup GN_GPS_api_H
/// \{

//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to start uploading
///      the GN Baseband Patch.
/// \details
///      GN GPS API function called by #GN_GPS_Update() to request that the Host
///      software start uploading GN Baseband Patch data for the given ROM
///      code version.  This call will only be made once the library has
///      identified the GNS chip ROM software version number.
///      <p> If the currently reported patch checksum #Patch_CkSum is already
///      the same as the known true checksum then the patch is already loaded
///      correctly, and therefore does not need to be re-loaded.
///      <p> During the patch upload, the baseband continues to operate normally.
///      However, for the majority of the uploaded period only the ROM mask software
///      will be running and is, therefore, going to suffer from any issues that
///      would normally be fixed by the patches.  Therefore, best efforts should
///      be made by the host software not to re-upload the patch if this API
///      reports a patch checksum that is already correct.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \attention
///      The host must ensure that if the data in this file is split amongst
///      several writes then this is done at a "<CR><LF>" boundary so as to
///      avoid message corruption by the Ctrl Data also being sent to the GN
///      Baseband.
/// \returns
///      The Truth Patch 16-bit checksum, or zero if there is no Patch.
///      Once the patch has been successfully uploaded the Reported Patch
///      Checksum should be equal to this Truth Checksum.
U2 GN_GPS_Write_GNB_Patch(
   U2 ROM_version,            ///< [in] The current GN Baseband ROM software version number.
                              ///       This is used by the Host software to identify which patch to send.
   U2 Patch_CkSum             ///< [in] The current GN Baseband Reported Patch Checksum.
                              ///       This can be used by the Host software to identify that the correct
                              ///       patch is already loaded and, therefore, does not need uploading again.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Read back the GPS
///      Library's Non-Volatile Store Data from the Host's chosen Non-Volatile
///      Store media.
/// \details
///      GN GPS API Function to request the Host software to Read back the GPS
///      Non-Volatile Store data image which it had previously saved to the
///      host's chosen Non-Volatile Store media (eg File system, battery
///      backed RAM, Flash, EEPROM etc), following a previous
///      #GN_GPS_Write_NV_Store() call.
///      <p> This function will be called from the #GN_GPS_Initialise() and
///      #GN_GPS_WakeUp() library API functions.
///      <p> NV_size is available at compile time via the \#define
///      #SIZE_NV_STORE, and is typically under 5Kb.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      The number of bytes of Non-Volatile Store data actually read back.
///      If this is not equal to 'NV_size' then it is assumed that the
///      Non-Volatile Store Data read back is invalid and will be ignored.
///      This will lead to a Cold Start and consequently a significantly
///      degraded TTFF.
U2 GN_GPS_Read_NV_Store(
   U2  NV_size,               ///< [in] The size of the Non-Volatile Store Data to Read [bytes].
   U1* p_NV_Store             ///< [in] Pointer to where the Host software should put the NV Store Data.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Write the GPS Library's
///      Non-Volatile Store Data to the Host's chosen Non-Volatile Store media.
/// \details
///      GN GPS API Function to request the Host software to Write the GPS Library's
///      Non-Volatile Store Data to the Host's chosen Non-Volatile Store media
///      (eg File system, battery backed RAM, Flash, EEPROM etc).
///      <p> This function will be called from the #GN_GPS_Shutdown() and
///      #GN_GPS_Sleep() library API function.
///      It may also be called periodically from #GN_GPS_Update() based on
///      #s_GN_GPS_Config::NV_Write_Interval.
///      <p> NV_size is typically under 5Kb.
///      <p> In some platform library implementations this function will also be
///      called periodically while the GPS is running,  eg once every 10 minutes.
///      This is for target platforms which can have their power suddenly removed,
///      so #GN_GPS_Shutdown() does not always to run before stopping.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \attention
///      The time spent in this function should not be longer than, say, 100 ms.
///      If a significantly longer time is required, because the chosen Non-Volatile
///      Storage medium is a slow device such as an EEPROM, then this data should
///      be double buffered and saved in a separate off-line OS task/thread.
/// \returns
///      The number of bytes of Non-Volatile Store data actually written.
void GN_GPS_Write_NV_Store(
   U2  NV_size,               ///< [in] The size of the Non-Volatile Store Data to Write [bytes].
   U1* p_NV_Store             ///< [in] Pointer to where the Host Software can get the NV Store Data from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Get the current OS
///      Time tick in integer millisecond units.
/// \attention
///      The returned value must be capable of going all the way up to and
///      correctly wrapping a 32-bit unsigned integer boundary.
///      <p> It is critical that this call counts in milliseconds units within an
///      accuracy of better than, say, 200 ppm, and not at some other scale that
///      is significantly different.  It is OK for the granularity to be worse
///      than 1 millisecond, but should be no worse than 10 milliseconds.
///      <p> This OS Time must be from the same source as the OS Time returned
///      with the #GN_GPS_Read_UTC() host implemented API call.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      The current OS Time tick in integer millisecond units.
U4 GN_GPS_Get_OS_Time_ms( void );


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host Software, if possible, to Get
///      the OS Time tick, in integer millisecond units, corresponding to when
///      a burst of Measurement data received from GPS Baseband started.
/// \details
///      This data helps in determining the exact latencies in the communications
///      between the host based GPS Library software and the GN Baseband software.
///      It is not possible to determine this on many host platforms, in
///      which case the host should simply {return( 0 );}.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      The OS Time tick in integer millisecond units of the start of the last
///      baseband raw measurement data block.  If it is unknown, return zero.
U4 GN_GPS_Get_Meas_OS_Time_ms( void );


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Read back the
///      current UTC Date & Time data (eg from the Host's Real-Time Clock).
/// \details
///      This function is called function is called just the once at the
///      start of every period of Navigation, including wake-up from Sleep.
///      <p> This OS Time must be from the same source as the OS Time returned
///      with the host implemented #GN_GPS_Get_OS_Time_ms() call.
/// \attention
///      The host should aim to maintain knowledge of time as accurately as it can.
///      This may be done via a Real-Time Clock, or by calibrating a Real-Time Counter,
///      or OS Time Tick counter relative to the UTC time frame, or from cellular
///      network time, etc.
///      <p> The UTC time corresponds to the local #s_GN_GPS_UTC_Data::OS_Time_ms
///      within the accuracy quoted by #s_GN_GPS_UTC_Data::Acc_Est.  The host is
///      responsible for providing a time estimated accuracy that is reliable.
///      This includes ensuring that nothing
///      else changes the Real-Time Clock without the knowledge of the GPS.
///      The host should have saved details of when the UTC Time was last saved
///      and how accurate it was saved to, plus some information that can be used
///      to identify that the GPS was the last thing to have saved this time.
///      On reading back the current UTC Time its estimated accuracy should de
///      degraded by the granularity of the read, and by how much the clock would
///      have degraded since it was last set (eg based on a 20-50 ppm crystal).
///      <p> The more accurate time can be maintained the faster the Time-To First
///      Fix (TTFF) will be and hence less power will be used per fix.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      Flag to indicate success or failure of the call.
/// \retval #TRUE if successful.
/// \retval #FALSE if unsuccessful.
BL GN_GPS_Read_UTC(
   s_GN_GPS_UTC_Data* p_UTC         ///< [in] Pointer to where the Host software should put the UTC Date & Time.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Write the GPS Library's
///      current UTC Date & Time data (eg offset relative to the Host's Real-Time Clock).
/// \details
///      This API function is called at the end of every #GN_GPS_Update() call
///      which produced a new valid Time solution.
///      <p> The UTC time corresponds to the local #s_GN_GPS_UTC_Data::OS_Time_ms within the accuracy
///      quoted by #s_GN_GPS_UTC_Data::Acc_Est.
///      <p> If the UTC Date & Time Fields are all set to zero and the RMS Time
///      Accuracy is set to a very large value (eg 0x7FFFFFFF ms) then this should
///      be interpreted as a request to the Host software to delete it's existing
///      UTC time information.
/// \attention
///      <p> The host should use this data in the best possible manner to maintain
///      knowledge of time.   This may involve writing to a real physical Real-Time
///      Clock, or by calibrating a Real-Time Counter and saving the offset, or the
///      calibrating OS Time Tick counter relative to the UTC time frame.
///      <p> The host should save details of when the UTC Time was saved, how
///      accurate it was saved to, plus some information that can be used by
///      #GN_GPS_Read_UTC() to identify that the GPS was the last thing to
///      have saved this time,  ie it is safe to use it.
///      <p> The more accurate time can be maintained, the faster the Time-To First
///      Fix will be, and hence less overall power will be used per fix.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      Nothing.
void GN_GPS_Write_UTC(
   s_GN_GPS_UTC_Data* p_UTC   ///< [in] Pointer to where the Host software can get the UTC Date & Time from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Read GPS Measurement
///      Data from the Host's chosen GPS Baseband communications interface.
/// \details
///      GN GPS API Function to request the Host software to Read GPS Measurement
///      Data from the Host's chosen GPS Baseband communications interface (eg UART).
///      <p> This function is called at the begriming of every call to #GN_GPS_Update().
///      <p> Internally the GN GPS Library uses a circular buffer to store this
///      data.  Therefore, #GN_GPS_Read_GNB_Meas()::max_bytes is dynamically set to prevent a single
///      Read operation from straddling the internal circular buffer's end wrap
///      point, or from over writing data that has not been processed yet.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      The number of bytes actually read by the host software.  If this is equal
///      to #GN_GPS_Read_GNB_Meas()::max_bytes then this function may be immediately called again if
///      #GN_GPS_Read_GNB_Meas()::max_bytes was limited due to the GN GPS Library's circular buffer end
///      wrap point.
U2 GN_GPS_Read_GNB_Meas(
   U2  max_bytes,             ///< [in] Maximum number of bytes to Read in this API call.
   CH* p_GNB_Meas             ///< [in] Pointer to where the Host software should put the GPS Baseband Raw Measurement data.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Write GPS Control
///      Data to the Host's chosen GPS Baseband communications interface.
/// \details
///      GN GPS API Function to request the Host software to Write GPS Control
///      Data to the Host's chosen GPS Baseband communications interface (eg UART).
///      <p> This function is called at the end of the #GN_GPS_Update() function
///      calls which produced a new GPS Navigation Solution update, ie once every
///      #s_GN_GPS_Config::FixInterval milliseconds.
///      <p> Internally the GN GPS Library uses a circular buffer to store this
///      data.  Therefore, this function may be called twice in succession if the
///      data to be written straddles the internal circular buffer's end wrap point.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      The number of bytes actually written by the host software.  If this is
///      less than the number of bytes requested to be written, then it is assumed
///      that this is because the host software cannot physically handle any more
///      data at this time, and will be buffered internally.
U2 GN_GPS_Write_GNB_Ctrl(
   U2  num_bytes,             ///< [in] Available number of bytes to Write in this API call.
   CH* p_GNB_Ctrl             ///< [in] Pointer to where the Host software can get the GPS Baseband Control data from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request that the Host Software if should
///      perform a Hard Reset of the GN Baseband Chip.
/// \details
///      Typically this can be achieved by either:
///        .  Holding the RST_N line low for a minimum of 0.5 seconds.
///        .  A full power cycle ensuring the power is off for a minimum of
///           0.5 seconds to ensure a full baseband and RF reset occurs.
///      <p> This Hard Reset request can triggered near start-up if an excessive
///      number of checksum errors occurs on the communication channel (eg UART)
///      between the GN Baseband chip and the host platform within the first
///      few seconds of starting, indicating that something might not have
///      been set-up correctly.
///      <p> This Hard Reset request can also trigger if 5 adjacent Raw Measurement
///      data blocks which were expected from the GN Baseband chip have not
///      been received.  Prior to getting to this stage many software “Wake-Up”
///      messages would have been sent to the GN Baseband chip and clearly not
///      had the desired effect.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      Flag to indicate if this is Hard-Reset was possible in this host implementation.
/// \retval #TRUE if Hard-Reset was possible.
/// \retval #FALSE if Hard-Reset was not possible.
BL GN_GPS_Hard_Reset_GNB( void );


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Write GPS NMEA 183
///      Output Sentences to the Host's chosen NMEA interface(s).
/// \details
///      For example, shared memory, virtual serial port, named pipe, UART, log
///      file, etc.
///      <p> There is no latency associated with NMEA data obtained via this API
///      function is called by #GN_GPS_Update() as soon as there is any new NMEA
///      data is ready, but this has to be handled by the host software in the
///      same OS task / thread that GN_GPS_Update  is running in.
///      <p> Internally the GN GPS Core library uses a circular buffer to store
///      this data.  Therefore, this function may be called twice if the data
///      to be written straddles the internal circular buffer's end wrap point.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \attention
///      The host software must choose whether to obtain its NMEA data via
///      either this #GN_GPS_Write_NMEA() API,  or whether to poll the GPS Library
///      via the #GN_GPS_Get_NMEA() call, but not both.
/// \returns
///      The number of bytes actually written by the host software.  If this is
///      less than the number of bytes requested to be written, then it is
///      assumed that this is because the host side cannot physically handle
///      any more data at this time, and will be buffered internally.
U2 GN_GPS_Write_NMEA(
   U2  num_bytes,             ///< [in] Available number of bytes to Write in this API call.
   CH* p_NMEA                 ///< [in] Pointer to where the Host software can get the GPS NMEA data from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Read $PGNV GN GPS
///      Proprietary NMEA Input Messages from the Host's chosen $PGNV
///      communications interface.
/// \details
///      For example, shared memory, virtual serial port, named pipe, UART, log
///      file, etc.
///      <p> Internally the GN GPS Library uses a circular buffer to store this
///      data.  Therefore, #GN_GPS_Read_PGNV()::max_bytes is dynamically set to
///      prevent a single Read operation from straddling the internal circular
///      buffer's end wrap point, or from over writing data that has not been
///      processed yet.
/// \note
///      This function must be implemented by the Host software if the library
///      function #GN_GPS_Parse_PGNV() is called.
/// \returns
///      The number of bytes actually read by the host software.  If this is
///      equal to #GN_GPS_Read_PGNV()::max_bytes then this function may be
///      immediately called again if #GN_GPS_Read_PGNV()::max_bytes was limited
///      due to the GN GPS Library's circular buffer end wrap point.
U2 GN_GPS_Read_PGNV(
   U2  max_bytes,             ///< [in] Maximum number of bytes to read
   CH* p_PGNV                 ///< [in] Pointer to where the Host software should put the $PGNV data.
);


//*****************************************************************************

/// \}

//*****************************************************************************
// Debug API Functions called by the GN GPS Core Library which must be
// implemented by the Host software to stream debug data to an appropriate
// interface (eg UART, File, both etc).

/// \addtogroup GN_GPS_api_D
/// \{

//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Write GPS Baseband
///      I/O Communications Debug data to the Host's chosen debug interface.
/// \details
///      <p> Internally the GN GPS Library uses a circular buffer to store this
///      data.  Therefore, this function may be called twice in succession if the
///      data to be written straddles the internal circular buffer's end wrap point.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      The number of bytes actually written by the host software.  If this is
///      less than the number of bytes requested to be written, then it is
///      assumed that this is because the host side cannot physically handle
///      any more data at this time, and will be buffered internally.
U2 GN_GPS_Write_GNB_Debug(
   U2  num_bytes,             ///< [in] Available number of bytes to Write in this API call.
   CH* p_GNB_Debug            ///< [in] Pointer to where the Host software can get the GNB Debug data from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Write the GPS Library's
///      Navigation Solution Debug data to the Host's chosen debug interface.
/// \details
///      Internally the GN GPS Library uses a circular buffer to store this
///      data.  Therefore, this function may be called twice in succession if the
///      data to be written straddles the internal circular buffer's end wrap point.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      The number of bytes actually written by the host software.  If this is
///      less than the number of bytes requested to be written, then it is
///      assumed that this is because the host side cannot physically handle
///      any more data at this time, and will be buffered internally.
U2 GN_GPS_Write_Nav_Debug(
   U2  num_bytes,             ///< [in] Available number of bytes to Write in this API call.
   CH* p_Nav_Debug            ///< [in] Pointer to where the Host software can get the Nav Debug data from.
);


//*****************************************************************************
/// \brief
///      GN GPS API Function to request the Host software to Write the GPS Library's
///      Navigation Library Event Log data to the Host's chosen debug interface.
/// \details
///      Internally the GN GPS Library uses a circular buffer to store this
///      data.  Therefore, this function may be called twice in succession if the
///      data to be written straddles the internal circular buffer's end wrap point.
/// \note
///      This function must be implemented by the Host software and will be
///      called by functions from within the GN GPS Library.
/// \returns
///      The number of bytes actually written by the host software.  If this is
///      less than the number of bytes requested to be written, then it is
///      assumed that this is because the host side cannot physically handle
///      any more data at this time, and will be buffered internally.
U2 GN_GPS_Write_Event_Log(
   U2  num_bytes,             ///< [in] Available number of bytes to Write in this API call.
   CH* p_Event_Log            ///< [in] Pointer to where the Host software can get the Event Log data from.
);


//*****************************************************************************

/// \}

#ifdef __cplusplus
   }     // extern "C"
#endif

#endif   // GN_GPS_API_H
