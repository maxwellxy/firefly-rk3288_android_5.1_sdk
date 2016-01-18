//*****************************************************************************
// GloNav GPS Technology
// Copyright (C) 2007 GloNav Ltd.
// March House, London Rd, Daventry, Northants, UK.
// All rights reserved
//
// Filename  GN_GPS_DataLogs.h
//
//*****************************************************************************


//*****************************************************************************
//
// This is example code to illustrate how the Glonav GPS High-Level software
// can be integrated into the host platform. Note that, although this
// constitutes a fully-working GPS receiver, it is simplified relative to
// what would be expected in a real product. The emphasis is in trying to
// provide clarity of understanding of what is going on in the software, and
// how the various API function calls relate to each other, rather than in
// providing an efficient software implementation.
//
// The results from the GPS software are written to data log files. This is
// not what would be expected for a real system.
//
// The functions in this file constitute the data logging functionality.
//
// There are 4 log files:
//
//    1. The NMEA data log (NMEA_LOG).
//    2. The Measurement Engine (i.e. baseband chip) Debug data log (ME_LOG).
//    3. The Navigation Debug data log (NAV_LOG).
//    4. The Event Log (EVENT_LOG).
//
//*****************************************************************************

#ifndef GN_GPS_DATALOGS_H
#define GN_GPS_DATALOGS_H


#include "gps_ptypes.h"



//*****************************************************************************

typedef enum            // Data Log Type
{
   NMEA_LOG,            // NMEA data log
   ME_LOG,              // Measurement Engine Debug data log
   NAV_LOG,             // Nav Debug data log
   EVENT_LOG,           // Event Log
   NUM_LOGS             // Total number of log files

} e_Data_Log;           // Data Log Type



//*****************************************************************************
// Function prototypes

// Open the GPS log files
void GN_Open_Logs(void);

// Close the GPS log files
void GN_Close_Logs(void);

// Write data to the specified log file
void Write_Data_To_Log(
       e_Data_Log  log,          // Data log type
       U2          num_bytes,    // Number of bytes to Write
       CH          *p_data );    // Pointer to the data


#endif