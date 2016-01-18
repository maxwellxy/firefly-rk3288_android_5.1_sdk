/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * Module    : Operating System Abstraction Layer
 *
 * Hierarchy :
 *
 * Purpose   : Encapsulates and abstracts services from different operating
 *             system, including user-mode as well as kernel-mode services.
 ******************************************************************************/
#ifndef _OSLAYER_MFS_H
#define _OSLAYER_MFS_H

#if defined (MFS)

#include <kernel/ctTypes.h>
#include <kernel/mutexTypes.h>
#include <kernel/mailboxTypes.h>
#include <kernel/timerTypes.h>
#include <kernel/activityTypes.h>

#include <ebase/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define OSLAYER_ASSERT(x) DCT_ASSERT(x)


//typedef int32_t (*osThreadFunc)(void *);
typedef aid_t osThreadFunc;
typedef int32_t (*osIsrFunc)(void *);
typedef int32_t (*osDpcFunc)(void *);

#ifdef OSLAYER_EVENT

#define OS_EVENT_INITIALIZED    0x6469

/*****************************************************************************/
/*  @brief  Event object (Linux Version) of OS Abstraction Layer */
typedef struct _osEvent
{
    int32_t   state;
    mbId_t    m_id;
    aid_t     act_id;
    void      *cond;
    int32_t   status;
    int32_t   automatic_reset;
    int32_t   initialized;
} osEvent;
#endif /* OSLAYER_EVENT */


#ifdef OSLAYER_MUTEX

#define OS_MUTEX_INITIALIZED    0x646a

/*****************************************************************************/
/*  @brief  Mutex object (Linux Version) of OS Abstraction Layer */
typedef struct _osMutex
{
    mutexId_t mutex;
    int32_t   initialized;
} osMutex;
#endif /* OSLAYER_MUTEX */


#ifdef OSLAYER_SEMAPHORE

#define OS_SEMAPHORE_INITIALIZED    0x646b

/*****************************************************************************/
/*  @brief  Semaphore object (Linux Version) of OS Abstraction Layer */
typedef struct _osSemaphore
{
    mutexId_t mutex;
    aid_t     act_id;
    mbId_t    m_id;
    int32_t   count;
    void      *cond;
    int32_t   status;
    int32_t   initialized;
} osSemaphore;
#endif /* OSLAYER_SEMAPHORE */


#ifdef OSLAYER_THREAD
/*****************************************************************************/
/*  @brief  Thread object (Linux Version) of OS Abstraction Layer */
typedef struct _osThread
{
    aid_t   act_id;     /* id of activity assigned to this thread object */
} osThread;
#endif /* OSLAYER_THREAD */


/*****************************************************************************/
/*  @brief  Instance records for callers (activities) of OS Abstraction Layer */
typedef struct _osInstanceRecord
{
    aid_t       act_id;           /**< id of activity assigned to this record */
    ctState_t   ct_state;         /**< state variable used by os layer functions */
    ctState_t   ct_state_gm;      /**< state variable used by getMailbox function */
    timerid_t   timer_id;         /**< used for all timer operations of the activity */
} osInstanceRecord;


/*****************************************************************************/
/*  @brief get result from MFS OS layer function call */
extern int32_t osGetLastResult (void);

/*****************************************************************************/
/*  @brief Assign a Mailbox Activity to be woken up upon Receiving (MFS Only) */
extern void  osAssignMailboxRxWakeUp (mbId_t  mid);

#ifdef __cplusplus
}
#endif

#endif /* MFS */
#endif /* OSLAYER_MFS_H */
