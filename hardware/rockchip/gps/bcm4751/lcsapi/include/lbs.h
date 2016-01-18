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
/// \file lbs.h     Main entry for Broadcom LBS Application API for Aquila
///============================================================================

#ifndef BRCM_LBS_H_
#define BRCM_LBS_H_

#include <os_types.h>
#include <brcm_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BRCM_LBS_USE_PROXY
    #define BRCM_LBS_USE_PROXY TRUE
#endif

typedef enum brcmlbs_fddirection
{
    BRCMLBS_TX,
    BRCMLBS_RX
} brcmlbs_direction;

typedef void (*BrcmLbs_OnError) (BrcmLbs_Result error, const char *reason, void *arg);

struct LbsHandle_t {
    OsHandle gpsHandle; /* handle for gps_api - no need to know what is inside */
    OsHandle suplHandle;
    OsHandle rilHandle;
    int ipc_fd;
    BrcmLbs_OnError onerror;
    void *onerror_arg;
};
typedef struct LbsHandle_t* LbsHandle;

//*****************************************************************************
/**
*   Initialize interface to GPS/LBS system overall.
*   Note that each subsystem requires a call to its corresponding init function
*   \param path (in) path for the local socket used for IPC to GPS(/LBS)
*******************************************************************************/
OsHandle BrcmLbs_init(const char *path);

//*****************************************************************************
/**
*   close and destroy all resources related to the interface
*   \param lbs   - handle returned from BrcmLbs_init() call
*******************************************************************************/
void BrcmLbs_deinit(OsHandle lbs);


//*****************************************************************************
/**
*   The gll messages will be handled asynchronously, we provide the file descriptor 
*   that is to be used on a poll/select loop (event loop)
* 
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \return file descriptor of the fd descriptor where the messages from the 
*           gll will be sent
*******************************************************************************/
int BrcmLbs_getIpcFileDescriptor(OsHandle lbs);

/******************************************************************************
**  The gps_api needs to be passed its own handle.
*   Pass to this function the handle returned from BrcmLbs_init() and get back
*   the handle for BrcmLbsGps_XXX calls
* 
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \return      - handle for BrcmLbsGps_XXX calls
*******************************************************************************/
OsHandle BrcmLbs_getGpsHandle(OsHandle lbs);

/******************************************************************************
**  Get supl handle.
*   Pass to this function the handle returned from BrcmLbs_init(),
*   and get back the handle for BrcmLbsSupl_XXX calls
* 
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \return      - handle for BrcmLbsSupl_XXX calls
*******************************************************************************/
OsHandle BrcmLbs_getSuplHandle(OsHandle lbs);

//*****************************************************************************
/**
*   When we have something of the file descriptor to be read this functions will 
*   decode messages and call the right callbacks.
*   This function doesn't block
*   \param lbs   - handle returned from BrcmLbs_init() call
*******************************************************************************/

ssize_t BrcmLbs_processMessages(OsHandle lbs, brcmlbs_direction flags);



//*****************************************************************************
/**
*   Register to callback in case an error is reported.
*   \param lbs   - handle returned from BrcmLbs_init() call
*   \param onerror_cb   - on error callback
*   \param arg          - argument to pass to the callback when called
*******************************************************************************/
void BrcmLbs_registerOnError(OsHandle lbs,BrcmLbs_OnError onerror_cb,void *arg);

void BrcmLbs_error(OsHandle lbs, BrcmLbs_Result res, const char *fmt, ...);

struct BrcmLbs_version {
    uint16_t major;
    uint16_t minor;
    uint32_t hash;
};

void BrcmLbs_getVersion(struct BrcmLbs_version *ver);

static inline void BRCMLBS_MAKE_VERSION(struct BrcmLbs_version *ver)
{
    ver->major = 0x01;
    ver->minor = 0x02;
    ver->hash = sizeof(BrcmLbs_PosInfo);
    ver->hash += sizeof(BrcmLbsSupl_Notification);
    ver->hash += sizeof(BrcmLbsSupl_SetID);
    ver->hash += sizeof(BrcmLbs_FactTestInfo);
    ver->hash += sizeof(BrcmLbsSupl_Notification);
}

static inline int BRCMLBS_VERSION_IS_EQUAL(struct BrcmLbs_version *a,
        struct BrcmLbs_version *b)
{
    if (a->major != b->major)
        return 0;
    if (a->minor != b->minor)
        return 0;
    if (a->hash != b->hash)
        return 0;
    return 1;
}

#ifdef __cplusplus
}
#endif

#endif

