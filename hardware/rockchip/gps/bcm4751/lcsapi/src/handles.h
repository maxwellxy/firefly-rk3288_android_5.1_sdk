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
/// \file handles.h    
///============================================================================


#ifndef _BRCM_HANDLES_H_
#define _BRCM_HANDLES_H_

#include "brcm_types.h"
#include "lbs.h"
#include "supl.h"
#include "gps_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SuplHandle_t {
    LbsHandle lbs;
    BrcmLbsSupl_Callbacks callbacks;
    BrcmLbs_UserData userData;
    int (*processFunc) (OsHandle lbs, uint8_t *data, size_t datalen);
}SuplHandle_t;

typedef SuplHandle_t *SuplHandle;


/********************************************************************************/
/** Structure for GPS handle
*   Keeps all the static data that is necessary to keep the link working
*********************************************************************************/
struct GpsHandle_t {
    OsHandle lbs;
    OsUint16 serviceType;
    OsUint32 nmeaMode;
    BrcmLbs_PositioningMode positioningMode;
    BrcmLbs_PowerState powerState;
    BrcmLbs_Callbacks callbacks;
    OsUint32 nextRqNum;
    int (*processFunc) (OsHandle lbs, uint8_t *data, size_t datalen);
};

typedef struct GpsHandle_t* GpsHandle;


#ifdef __cplusplus
}
#endif

#endif  // _BRCM_HANDLES_H_
