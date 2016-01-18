/******************************************************************************
 *
 * Copyright 2007, Silicon Image, Inc.  All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of: Silicon Image, Inc., 1060
 * East Arques Avenue, Sunnyvale, California 94085
 *
 *****************************************************************************/
/**
 * @file media_buffer_queue.h
 *
 * @brief
 *          Media Buffer Queue interface
 *
 * <pre>
 *
 *   Principal Author: Joerg Detert
 *   Creation date:    Feb 28, 2008
 *
 * </pre>
 *
 *****************************************************************************/
/** @defgroup media_buffer_queue MediaBufferQueue interface description.
 *  @{
 *
 * @page media_buffer_queue MediaBufferQueue interface description
 * Here you will find the interface documentation of the MediaBufferQueue.
 *
 */

#ifndef MEDIA_BUFFER_QUEUE_H
#define MEDIA_BUFFER_QUEUE_H

#if defined (__cplusplus)
extern "C" {
#endif

#include <ebase/types.h>
#include <ebase/dct_assert.h>

#include <common/return_codes.h>

#include "media_buffer.h"
#include "media_buffer_pool.h"


/*****************************************************************************/
/**
 * @brief Definition of Media Buffer Pool Flags.
 *
 *****************************************************************************/
#define BUFQUEUE_RINGBUFFER BUFPOOL_RINGBUFFER /**< Buffer queue acts like a ring buffer, that means no random access */


/*****************************************************************************/
/**
 * @brief Parameter id, used when calling @ref MediaBufQueueSetParameter
 *        or MediaBufQueueGetParameter.
 *
 *****************************************************************************/
typedef MediaBufPoolParamId_t   MediaBufQueueParamId_t;


/*****************************************************************************/
/**
 * @brief Configuration structure for Media Buffer Queue.
          Currently identical to config structure for Media Buffer Pool.
 *
 *****************************************************************************/
typedef MediaBufPoolConfig_t MediaBufQueueConfig_t;


/*****************************************************************************/
/**
 * @brief Structure used to pass memory to the Media Buffer Queue.
 *        Currently identical to related structure for Media Buffer Pool.
 *        Comments relating to Media Buffer Pool are valid for this structure
 *        as well in an analogous manner.
 *
 *****************************************************************************/
typedef MediaBufPoolMemory_t MediaBufQueueMemory_t;


/**
 * @brief
 */
/*****************************************************************************/
/**
 * @brief Used as parameter for @ref MediaBufQueueCbNotify_t.
 *
 *****************************************************************************/
typedef MediaBufPoolEvent_t MediaBufQueueEvent_t;


/*****************************************************************************/
/**
 * @brief Callback function that can be registered to receive information
 *        about buffer queue events.
 *
 *****************************************************************************/
typedef MediaBufPoolCbNotify_t MediaBufQueueCbNotify_t;


/*****************************************************************************/
/**
 * @brief Configuration structure for Media Buffer Queue.
          Currently identical to config structure for Media Buffer Pool.
 *
 *****************************************************************************/
typedef struct MediaBufQueueStat_s
{
    uint16_t currFillLevel;      /**< Current fill level in units of buffers. */
    uint16_t maxFillLevel;       /**< Maximum fill level in units of buffers. */
    uint32_t avgFillLevel;       /**< Exponential moving average fill level in units of buffers
                                      with alpha = 1/16, expressed as a 16.16 fixed point number.
                                      Updates are done when a full buffer is added or a buffer is
                                      released to the pool, i.e. each time fillLevel changes. */
    uint32_t meanTargetArea;     /**< Half the way between low and high watermark in units of buffers,
                                      expressed as a 16.16 fixed point number. */
} MediaBufQueueStat_t;


/*****************************************************************************/
/**
 * @brief Media Buffer Queue object, associated with a MediaBufPool element.
 *        Used to manage buffer objects from the buffer pool in a first-in
 *        first-out manner.
 *        The pool has to be initialized via MediaBufQueueCreate() and
 *        released via MediaBufQueueDestroy().
 *
 *****************************************************************************/
typedef struct MediaBufQueue_s
{
    MediaBufPool_t  bufPool;         /**< Associated media buffer pool*/
    MediaBuffer_t** pBufArray;       /**< Pointer to buffer array */
    uint32_t        head;            /**< Read position in buffer queue (internal use)*/
    uint32_t        tail;            /**< Write position in buffer queue (internal use)*/
    uint32_t        fullBufsAvail;   /**< How many full buffers are still available to get them */
    uint16_t        maxFillLevel;    /**< Maximum fill level of underlying buffer pool */
    uint32_t        avgFillLevel;    /**< Exponential moving average fill level of underlying buffer pool */
    void*           pUserContext;    /**< user context for buffer queue */
} MediaBufQueue_t;


/*****************************************************************************/
/**
 * @brief Get the size of the buffer queue.
 *
 * @param   pConfig  Configuration settings for Media Buffer Queue.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueGetSize(MediaBufQueueConfig_t* pConfig);


/*****************************************************************************/
/**
 * @brief Create a buffer queue.
 *
 * @param   pBufQueue    Pointer to Media Buffer Queue object.
 * @param   pConfig      Configuration settings for Media Buffer Queue.
 * @param   queueMemory  Memory to create the Media Buffer Queue in.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueCreate(MediaBufQueue_t*       pBufQueue,
                                  MediaBufQueueConfig_t* pConfig,
                                  MediaBufQueueMemory_t  queueMemory);


/*****************************************************************************/
/**
 * @brief Destroy a buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueDestroy(MediaBufQueue_t* pBufQueue);


/*****************************************************************************/
/**
 * @brief Get an empty buffer from a buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Pointer to the media buffer.
 *          Can be NULL if no such buffer is available.
 *****************************************************************************/
extern MediaBuffer_t* MediaBufQueueGetEmptyBuffer(MediaBufQueue_t* pBufQueue);


/*****************************************************************************/
/**
 * @brief Insert a full buffer into a buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueuePutFullBuffer(MediaBufQueue_t* pBufQueue,
                                         MediaBuffer_t*   pBuf);


/*****************************************************************************/
/**
 * @brief Get a full buffer out of the buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Pointer to the media buffer.
 *          Can be NULL if no such buffer is available.
 *****************************************************************************/
extern MediaBuffer_t* MediaBufQueueGetFullBuffer(MediaBufQueue_t* pBufQueue);


/*****************************************************************************/
/**
 * @brief Release a buffer to a buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueReleaseBuffer(MediaBufQueue_t* pBufQueue,
                                         MediaBuffer_t*   pBuf);


/*****************************************************************************/
/**
 * @brief Flush buffer queue, i.e. empty all full buffers which have not yet
 *        been retrieved. Also resets all statistics.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueFlush(MediaBufQueue_t* pBufQueue);


/*****************************************************************************/
/**
 * @brief Reset buffer queue, i.e. produce same state as after creation
 *        (configuration is not discarded).
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueReset(MediaBufQueue_t* pBufQueue);


/* ***************************************************************************/
/**
 * @brief   Set a parameter for a buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   paramId     Id of parameter to be set.
 * @param   paramValue  Value of parameter to be set.
 *
 * @return  RESULT
 *****************************************************************************/
extern RESULT MediaBufQueueSetParameter(MediaBufQueue_t*         pBufQueue,
                                        MediaBufQueueParamId_t paramId,
                                        int32_t                paramValue);


/* ***************************************************************************/
/**
 * @brief   Get a parameter for a buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   paramId     Id of parameter to get.
 * @param   pParamValue Address to receive parameter value.
 *
 * @return  RESULT
 *****************************************************************************/
extern RESULT MediaBufQueueGetParameter(MediaBufQueue_t*         pBufQueue,
                                        MediaBufQueueParamId_t paramId,
                                        int32_t*               pParamValue);


/*****************************************************************************/
/**
 * @brief Lock a buffer of a buffer queue. Buffer will not be available as
 *        empty buffer until unlocked as many times as locked before
 *        and released.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueLockBuffer(MediaBufQueue_t* pBufQueue,
                                      MediaBuffer_t*   pBuf);


/*****************************************************************************/
/**
 * @brief Unlock a buffer of a buffer queue which has previously been locked.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueUnlockBuffer(MediaBufQueue_t* pBufQueue,
                                        MediaBuffer_t*   pBuf);


/*****************************************************************************/
/**
 * @brief Reterieve statistics for a buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   pStat       Pointer to statistics structure to be filled.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueGetStats(MediaBufQueue_t*     pBufQueue,
                                    MediaBufQueueStat_t* pStat);


/* ***************************************************************************/
/**
 * @brief   Register a callback to get informed about MediaBufQueue_t events.
 *
 * @param   pBufQueue       Pointer to Media Buffer Queue object.
 * @param   fpCallback      Function to be triggered in case of event.
 * @param   pUserContext    Pointer to be passed to callback.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueRegisterCb(MediaBufQueue_t*         pBufQueue,
                                      MediaBufQueueCbNotify_t  fpCallback,
                                      void*                    pUserContext);


/* ***************************************************************************/
/**
 * @brief   Deregister a callback from MediaBufQueue_t.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   fpCallback  Function to be deregistered.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueDeregisterCb(MediaBufQueue_t *       pBufQueue,
                                        MediaBufQueueCbNotify_t fpCallback);



/*****************************************************************************/
/**
 * @brief Returns base address of buffer memory.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Base address of buffer memory.
 *****************************************************************************/
INLINE void* MediaBufQueueGetBufMemBaseAddr(const MediaBufQueue_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);

    return pBufQueue->bufPool.pBufArray[0].pBaseAddress;
}


/*****************************************************************************/
/**
 * @brief Returns size of total buffer memory.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Size of buffer memory.
 *****************************************************************************/
INLINE uint32_t MediaBufQueueGetBufMemTotalSize(const MediaBufQueue_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);

    return pBufQueue->bufPool.poolSize;
}


/*****************************************************************************/
/**
 * @brief Returns size of memory covered by a single buffer.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Size of memory covered by a single buffer.
 *****************************************************************************/
INLINE uint32_t MediaBufQueueGetBufSize(const MediaBufQueue_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);

    return pBufQueue->bufPool.bufSize;
}


/*****************************************************************************/
/**
 * @brief Returns number of buffers in a buffer queue.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Current number of buffers in a buffer queue.
 *****************************************************************************/
INLINE uint32_t MediaBufQueueGetBufNum(const MediaBufQueue_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);

    return pBufQueue->bufPool.bufNum;
}


/*****************************************************************************/
/**
 * @brief   Gets the number of empty buffers in a media buffer queue.
 *
 * @param   pBufQueue  Pointer to buffer queue object.
 *
 * @return  Number of empty buffers.
 *****************************************************************************/
INLINE uint16_t MediaBufQueueGetEmptyBufNum(const MediaBufQueue_t* pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    return pBufQueue->bufPool.freeBufNum;
}


/*****************************************************************************/
/**
 * @brief   Gets the fill level a media buffer queue in buffers.
 *
 * @param   pBufQueue  Pointer to buffer queue object.
 *
 * @return  Fill level in buffers.
 *****************************************************************************/
INLINE uint16_t MediaBufQueueGetFillLevel(const MediaBufQueue_t* pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    return pBufQueue->bufPool.fillLevel;
}


/*****************************************************************************/
/**
 * @brief   Returns indication if empty buffers are available in a buffer queue.
 *
 * @param   pBufQueue  Pointer to buffer queue object.
 *
 * @return  Empty buffers are available or not.
 *****************************************************************************/
INLINE bool_t MediaBufQueueEmptyBuffersAvailable(const MediaBufQueue_t* pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    return ((bool_t)(pBufQueue->bufPool.freeBufNum > 0U));
}


/*****************************************************************************/
/**
 * @brief Returns indication if buffer queue is empty.
 *        If so, no full buffers are contained in it,
 *        whether already retreived or not.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return  Buffer queue is empty or not.
 *****************************************************************************/
INLINE bool_t MediaBufQueueIsEmpty(const MediaBufQueue_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);

    return ((bool_t)(pBufQueue->bufPool.fillLevel == 0U));
}


/*****************************************************************************/
/**
 * @brief Returns indication if full buffers are available in a buffer queue
 *        which have not yet been retreived.
 *
 * @param pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return Full buffers are available to be retrieved or not.
 *****************************************************************************/
INLINE bool_t MediaBufQueueFullBuffersAvailable(const MediaBufQueue_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);

    return ((bool_t)(pBufQueue->fullBufsAvail > 0U));
}


/*****************************************************************************/
/**
 * @brief Returns indication if buffer queue is full.
 *        If so, no full buffers can be added to it.
 *
 * @param pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return Buffer queue is full or not.
 *****************************************************************************/
INLINE bool_t MediaBufQueueIsFull(const MediaBufQueue_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);

    return ((bool_t)(pBufQueue->bufPool.fillLevel == pBufQueue->bufPool.bufNum));
}


 /*@}*/

#if defined (__cplusplus)
}
#endif
#endif  /* MEDIA_BUFFER_QUEUE_H */

