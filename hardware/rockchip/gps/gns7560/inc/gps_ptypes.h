
//****************************************************************************
// GloNav GPS Technology
// Copyright (C) 2001-2006 GloNav Ltd.
// March House, London Rd, Daventry, Northants, UK.
//
// Filename gps_ptypes.h
//
// $Header: $
// $Locker: $
//****************************************************************************
//
// GPS platform primitive typedefs


#ifndef GPS_PTYPES_H
#define GPS_PTYPES_H

#ifdef __cplusplus
   #include <climits>   // C++ header
#else
   #include <limits.h>  // C header
#endif

//****************************************************************************
// GPS 4500 Build-specific option definitions

#define CPU_PROFILE


//****************************************************************************
// Include the following if all available debug screens are available

//#define ALL_DEBUG_SCREENS


//****************************************************************************
// If the Nav Debug data is expected to be viewed in real-time, for example
// in ProComm, the following ESC characters may best be defined.

#define _HOME_  ""
#define _CLEAR_ ""
#define _ERASE_ ""


//****************************************************************************
// In Generic_Lib\R4_math.h the R4abs macros is defined as being equal to
// the _fabsf macro under certain conditions. However, in at least one
// port the _fabsf macro seems not to be defined when it was expected
// to be. Hence we can disable the definition of R4abs to be _fabsf by
// defining the following option.

#define NO_FABSF

#define VA_ARG_NEEDS_I4_FOR_CH


//****************************************************************************
// If there is no sign of valid Comms data from the GPS ME, the High-Level
// software will repeatedly send a Wake-Up command. The following parameter
// defines the interval between these commands. If a value of 0 is defined,
// a wake-up command will be sent every time the GN_GPS_Update function is
// called by the host.

#define WAKEUP_TX_INT    0


//****************************************************************************
// Include the following to output different debug data

//#define  HOST_RTC_DEBUG     // Data relating to the Host RTC
//#define  SUBFRAME_DEBUG     // Data relating to the Nav Message data decode
//#define  EXT_TIME_DEBUG     // External time input
#define  SKIP_FASTER_FIRST_10_SEC


//****************************************************************************
// Define the interval (in seconds) at which the host will be requested to
// save the non-volatile data. If the interval is defined as 0, these
// regular requests will not be made.

#define NV_WRITE_INTERVAL     0  // Don't make regular requests to save data
//#define NV_WRITE_INTERVAL      60  // Save Non-Vol data every 60 s

//****************************************************************************

// Define the Default Debug Enabled flags
#define DEFAULT_NAV_DEBUG   0x0001        // Default basic debug level
#define DEFAULT_GNB_DEBUG   0x0001        // Default basic debug level
#define DEFAULT_EVENT_LOG   0x0001        // Default basic debug level

//****************************************************************************


//--------------------------------------------------------------------------

// Unsigned integer types

// Define U1 to be exactly 8 bits

#undef U1_DEFINED
#if UCHAR_MAX == 0xFF    // UCHAR_MAX is the maximum value of unsigned char
   #define U1_DEFINED
   typedef unsigned char   U1;
   typedef unsigned char   L1;      // 1 byte logical (TRUE or FALSE only)
#endif

// Define U2 to be at least 16 bits

#undef U2_DEFINED
#if USHRT_MAX >= 0xFFFF    // USHRT_MAX is the maximum value of unsigned short
   #define U2_DEFINED
   typedef  unsigned short U2;
   typedef  unsigned short L2;      // 2 byte logical (TRUE or FALSE only)
#elif UINT_MAX >= 0xFFFF   // UINT_MAX is the maximum value of unsigned int
   #define U2_DEFINED
   typedef  unsigned int   U2;
   typedef  unsigned int   L2;      // 2 byte logical (TRUE or FALSE only)
#endif

// Define U4 to be at least 32 bits

#undef U4_DEFINED
#if UINT_MAX >= 0xFFFFFFFF
   #define U4_DEFINED        // UINT_MAX is the maximum value of unsigned int
   typedef  unsigned int   U4;
   typedef  unsigned int   L4;      // 4 byte logical (TRUE or FALSE only)
#elif ULONG_MAX >= 0xFFFFFFFF // ULONG_MAX is the maximum value of unsigned long
   #define U4_DEFINED
   typedef  unsigned long  U4;
   typedef  unsigned long  L4;      // 4 byte logical (TRUE or FALSE only)
#endif

//-----------------------------------------------------------------------------

// Signed integer types

// Define I1 to be exactly 8 bits
#undef I1_DEFINED
#if SCHAR_MAX == 0x7F  // SCHAR_MAX is the maximum value of signed char
   #define I1_DEFINED
   typedef signed char  I1;
#endif

// Define I2 to be at least 16 bits
#undef I2_DEFINED
#if SHRT_MAX >= 0x7FFF  // SHRT_MAX is the maximum value of signed short
   #define I2_DEFINED
   typedef  signed short   I2;
#elif INT_MAX >= 0x7FFF // INT_MAX is the maximum value of signed int
   #define I2_DEFINED
   typedef  signed int  I2;
#endif

// Define S32 to be at least 32 bits
#undef I4_DEFINED
#if INT_MAX >= 0x7FFFFFFF    // INT_MAX is the maximum value of signed int
   #define I4_DEFINED
   typedef  signed int  I4;
#elif LONG_MAX >= 0x7FFFFFFF  // LONG_MAX is the maximum value of signed long
   #define I4_DEFINED
   typedef  signed long I4;
#endif

//-----------------------------------------------------------------------------

// Define real floating point number, single and double precision
typedef  float    R4;         // 4 byte floating point
typedef  double   R8;         // 8 byte floating point

// Define ASCII character type
typedef  char     CH;         // ASCII character

// Boolean type
typedef  U1       BL;         // Boolean logical (TRUE or FALSE only)

//-----------------------------------------------------------------------------
/********************************
 * add for test 2007.8.3		*
 ********************************/
typedef int FS_HANDLE;
typedef unsigned char WCHAR;
typedef unsigned int UINT;
typedef	int			INT;
typedef	float	kal_uint32;
typedef	int		module_type;	// device ID
typedef	char	kal_bool;
typedef	int		UART_PORT;
typedef	int		UART_baudrate;
typedef	char	UART_bitsPerCharacter;
typedef	char	UART_parity;
typedef	char	UART_flowCtrlMode;
typedef	char	UART_stopBits;
typedef	unsigned char uint8;
typedef	unsigned int  kal_uint16;
typedef	unsigned char kal_uint8;

typedef struct
{
UART_baudrate baud;
UART_bitsPerCharacter dataBits;
UART_stopBits stopBits;
UART_parity parity;
UART_flowCtrlMode flowControl;
uint8 xonChar;
uint8 xoffChar;
char	DSRCheck;	// can not support 'bool' type
} UARTDCBStruct;


#define	FS_READ_WRITE	1
#define	FS_CREATE_ALWAYS 	7
#define	uart_port2	3
#define	KAL_TRUE	1
#define	KAL_FALSE	0
//-----------------------------------------------------------------------------

// Define TRUE and FALSE
#ifndef TRUE
   #define TRUE  1
#endif

#ifndef FALSE
   #define FALSE 0
#endif

// Ensure NULL is defined
#ifdef NULL
   #undef NULL
#endif
#define NULL 0


//-----------------------------------------------------------------------------

// Define the use of inline function types as being possible
#define INLINE __inline


//*****************************************************************************

#endif   // GPS_PTYPES_H

// end of file
