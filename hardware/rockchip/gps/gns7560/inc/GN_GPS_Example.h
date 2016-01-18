//*****************************************************************************
// GloNav GPS Technology
// Copyright (C) 2007 GloNav Ltd.
// March House, London Rd, Daventry, Northants, UK.
// All rights reserved
//
// Filename  GN_GPS_Example.h
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
// The application creates three separate OS tasks:
//
//    1. The 'UART Input' task is responsible for reading data from the
//       UART (i.e. from the GPS baseband) and putting it into the
//       'input circular buffer'.
//
//    2. The 'UART Output' task is responsible for reading data from the
//       'output circular buffer' and writing it to the UART (i.e. sending
//       it to the GPS baseband). It is also responsible for sending the
//       patch code to the GPS baseband.
//
//    3. The GPS task simply runs the GPS software in continuous
//       navigation mode until the application is terminated.
//
// Each of these three tasks is implemented in a separate source file.
//
// The results from the GPS software are written to data log files.
//
// The application is terminated automatically after a specified period of
// time, defined by the RUN_DURATION #define parameter.
//
//*****************************************************************************

#ifndef GN_GPS_EXAMPLE_H
#define GN_GPS_EXAMPLE_H


#include "gps_ptypes.h"




//*****************************************************************************
// Define the operational characteristics of the three tasks
//#define UART_INP_TASK_SLEEP      10     // Sleep period of UART Input task (ms)
//#define UART_OUT_TASK_SLEEP      10     // Sleep period of UART Output task (ms)
//#define GPS_TASK_SLEEP          100     // Sleep period of GPS task (ms)

// Parameters relating to data received from and sent to the GPS Measurement Engine
#define UART_RX_BUF_SIZE       4096     // Input buffer size
#define UART_TX_BUF_SIZE       2048     // Output buffer size

// Duration of the run
#define RUN_DURATION            180     // seconds
//#define RUN_DURATION             30     // seconds


//*****************************************************************************
// Circular buffer structure
typedef struct
{
    U1 *write;       // next position to write to
    U1 *read;        // next position to read from
    U1 *start_buf;   // start of buffer
    U1 *end_buf;     // end of buffer + 1

} s_CircBuff;


//*****************************************************************************
// Function prototypes
U2 Setup_GNB_Patch( U2 ROM_version, U2 Patch_CkSum );



#endif