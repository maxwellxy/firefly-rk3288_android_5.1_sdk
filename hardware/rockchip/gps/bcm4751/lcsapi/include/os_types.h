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
///  THIS SOFTWARE IS PROVIDED “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
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
/// \file os_types.h    OS-dependent generic type definitions
///============================================================================

#ifndef _BRCM_LBS_OS_TYPES_H_
#define _BRCM_LBS_OS_TYPES_H_

#if __cplusplus
extern "C" {
#endif

#ifndef FALSE
    #define FALSE   0
    #define TRUE    1
#endif

#ifndef NULL
    #ifdef __cplusplus
        #define NULL    0
    #else
        #define NULL    ((void *)0)
    #endif
#endif

#if defined(__arm)
#define CONFIG_PLATFORM_ARM
#endif 

#if defined(__IAR_SYSTEMS_ICC__) && defined(__ICCARM__)
#define CONFIG_PLATFORM_IAR
#endif

#if defined(__TMS470__)
#define CONFIG_PLATFORM_TI_TSM470
#endif

#if defined(WIN32)
#define CONFIG_PLATFORM_WIN32
#endif

#if defined(_WIN32_WCE)
#define CONFIG_PLATFORM_WCE
#endif

#if defined(__GNUC__) && (defined(__linux__) || defined(__linux) || defined(__linux))
#define CONFIG_PLATFORM_LINUX
#endif

#if defined(__SYMBIAN32__)
#define CONFIG_PLATFORM_SYMBIAN32
#endif

#ifdef CONFIG_PLATFORM_LINUX

#include <stdint.h>

typedef uint8_t             OsBool;
typedef int8_t              OsInt8;
typedef uint8_t             OsUint8;
typedef int16_t             OsInt16;
typedef uint16_t            OsUint16;
typedef int32_t             OsInt32;
typedef uint32_t            OsUint32;
typedef int64_t             OsInt64;
typedef uint64_t            OsUint64;
typedef unsigned int        OsUint;
typedef uint8_t             OsOctet;
typedef void*               OsHandle;

#else

typedef unsigned char       OsBool;
typedef signed char         OsInt8;
typedef unsigned char       OsUint8;
typedef short               OsInt16;
typedef unsigned short      OsUint16;
typedef long                OsInt32;
typedef unsigned long       OsUint32;
typedef long long           OsInt64;
typedef unsigned long long  OsUint64;
typedef unsigned int        OsUint;
typedef OsUint8             OsOctet;
typedef void*               OsHandle;

#endif

#define OS_HANDLE_INVALID   (NULL)

#if __cplusplus
}  // extern "C"
#endif

#endif //_BRCM_LBS_OS_TYPES_H_
