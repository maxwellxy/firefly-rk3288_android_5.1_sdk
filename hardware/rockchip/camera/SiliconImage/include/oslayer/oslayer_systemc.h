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
 * @file oslayer_systemc.h
 *
 * Encapsulates and abstracts services of SystemC
 *
 *****************************************************************************/
#ifdef LINUX

#ifdef MSVD_COSIM

#include <ebase/types.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef int32_t (*osThreadFunc)(void *);
  typedef int32_t (*osIsrFunc)(void *);
  typedef int32_t (*osDpcFunc)(void *);

  typedef struct _osScEvent osScEvent;
  typedef struct _osScMutex osScMutex;
  typedef struct _osScSemaphore osScSemaphore;
  typedef struct _osScThreadHandle osScThreadHandle;


#ifdef OSLAYER_EVENT
/*****************************************************************************/
/*  @brief  Event object (Linux Version) of OS Abstraction Layer */
typedef struct _osEvent
{
  osScEvent* p_event;
} osEvent;
#endif /* OSLAYER_EVENT */


#ifdef OSLAYER_MUTEX
/*****************************************************************************/
/*  @brief  Mutex object (Linux Version) of OS Abstraction Layer */
typedef struct _osMutex
{
  osScMutex* p_mutex;
} osMutex;
#endif /* OSLAYER_MUTEX */


#ifdef OSLAYER_SEMAPHORE
/*****************************************************************************/
/*  @brief  Semaphore object (Linux Version) of OS Abstraction Layer */
typedef struct _osSemaphore
{
  osScSemaphore* p_semaphore;
} osSemaphore;
#endif /* OSLAYER_SEMAPHORE */


#ifdef OSLAYER_THREAD
/*****************************************************************************/
/*  @brief  Thread object (Linux Version) of OS Abstraction Layer */

typedef struct _osThread
{
  osScThreadHandle* p_handle;
  int32_t (*pThreadFunc)(void *);
  void *p_arg;
} osThread;
#endif /* OSLAYER_THREAD */

#ifdef __cplusplus
}
#endif


#endif /*! MSVD_COSIM */

#endif /* LINUX */
