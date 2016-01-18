/******************************************************************************
 *
 * Copyright 2010, Silicon Image, Inc.  All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of: Silicon Image, Inc., 1060
 * East Arques Avenue, Sunnyvale, California 94085
 *
 *****************************************************************************/
/**
 * @file media_buffer_queue_ex.h
 *
 * @brief
 *          Interface for Extended Extended Media Buffer Queue
 *          (supports External Buffer Pool for multi-drop setups).
 *
 * <pre>
 *
 *   Principal Author: Joerg Detert
 *   Creation date:    Feb 28, 2008
 *
 * </pre>
 *
 *****************************************************************************/
/** @defgroup media_buffer_queue_ex MediaBufferQueueEx interface description.
 *  @{
 *
 * @page media_buffer_queue_ex MediaBufferQueueEx interface description
 * Here you will find the interface documentation of the MediaBufferQueueEx.
 *
 */

#ifndef MEDIA_BUFFER_QUEUE_EX_H
#define MEDIA_BUFFER_QUEUE_EX_H

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
#define BUFQUEUE_RINGBUFFER BUFPOOL_RINGBUFFER /**< Buffer queue acts like a ring buffer, that means no random access. */


/*****************************************************************************/
/**
 * @brief Definition of Media Buffer Pool Callback Events.
 *
 * @note See media_buffer_pool for details
 *
 *****************************************************************************/


/*****************************************************************************/
/**
 * @brief Parameter id, used when calling @ref MediaBufQueueExSetParameter
 *        or EXMediaBufQueueExGetParameter.
 *
 *****************************************************************************/
typedef MediaBufPoolParamId_t MediaBufQueueExParamId_t;


/*****************************************************************************/
/**
 * @brief Configuration structure for Extended Media Buffer Queue.
          Currently identical to config structure for Media Buffer Pool.
 *
 *****************************************************************************/
typedef MediaBufPoolConfig_t MediaBufQueueExConfig_t;


/*****************************************************************************/
/**
 * @brief Structure used to pass memory to the Extended Media Buffer Queue.
 *        Currently identical to related structure for Media Buffer Pool.
 *        Comments relating to Media Buffer Pool are valid for this structure
 *        as well in an analogous manner.
 * @note  Only memory for administration of the queue state will be required
 *        if an external buffer pool is used.
 *
 *****************************************************************************/
typedef MediaBufPoolMemory_t MediaBufQueueExMemory_t;


/**
 * @brief
 */
/*****************************************************************************/
/**
 * @brief Used as parameter for @ref MediaBufQueueExCbNotify_t.
 *
 * @note: EMPTY_BUFFER_ADDED notification event is used as BUFFER_TO_BE_RELEASED notification event.
 *
 *****************************************************************************/
typedef MediaBufPoolEvent_t MediaBufQueueExEvent_t;


/*****************************************************************************/
/**
 * @brief Callback function that can be registered to receive information
 *        about buffer queue events.
 *
 *****************************************************************************/
typedef MediaBufPoolCbNotify_t MediaBufQueueExCbNotify_t;


/*****************************************************************************/
/**
 * @brief Configuration structure for Extended Media Buffer Queue.
          Currently identical to config structure for Media Buffer Pool.
 *
 *****************************************************************************/
typedef struct MediaBufQueueExStat_s
{
    uint16_t currFillLevel;      /**< Current fill level in units of buffers. */
    uint16_t maxFillLevel;       /**< Maximum fill level in units of buffers. */
    uint32_t avgFillLevel;       /**< Exponential moving average fill level in units of buffers
                                      with alpha = 1/16, expressed as a 16.16 fixed point number.
                                      Updates are done when a full buffer is added or a buffer is
                                      released to the pool, i.e. each time fillLevel changes. */
    uint32_t meanTargetArea;     /**< Half the way between low and high watermark in units of buffers,
                                      expressed as a 16.16 fixed point number. */
} MediaBufQueueExStat_t;


/*****************************************************************************/
/**
 * @brief Extended Media Buffer Queue object, associated with a MediaBufPool element.
 *        Used to manage buffer objects from the buffer pool in a first-in
 *        first-out manner.
 *        The pool has to be initialized via MediaBufQueueExCreate() and
 *        released via MediaBufQueueExDestroy().
 *
 *****************************************************************************/
typedef struct MediaBufQueueEx_s
{
    void*           p_next;          /**< pointer for appending this object to a list */

    MediaBufPool_t* pBufPool;        /**< Associated media buffer pool; may be created during queue creation or externally */
    MediaBuffer_t** pBufArray;       /**< Pointer to buffer array */
    uint32_t        head;            /**< Read position in buffer queue (internal use)*/
    uint32_t        tail;            /**< Write position in buffer queue (internal use)*/
    uint32_t        fullBufsAvail;   /**< How many full buffers are still available to get them */
    uint16_t        maxFillLevel;    /**< Maximum fill level of underlying buffer pool */
    uint32_t        avgFillLevel;    /**< Exponential moving average fill level of underlying buffer pool */
    void*           pUserContext;    /**< user context for buffer queue */

    bool_t               isExtPool;             /**< TRUE if external Buffer Pool is used for single producer, multiple consumer setups. */
    uint16_t             highWatermark;         /**< if value is reached high watermark callback is triggered; used if isExtPool. */
    uint16_t             lowWatermark;          /**< if value is reached low watermark callback is triggered; used if isExtPool. */
    MediaBufPoolNotify_t notify[MAX_NUM_REGISTERED_CB];   /**< Array with info about users registered for notification; used if isExtPool. */
} MediaBufQueueEx_t;


/*****************************************************************************/
/**
 * @brief Get the size of the buffer queue.
 *
 * @param   pConfig  Configuration settings for Extended Media Buffer Queue.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExGetSize(MediaBufQueueExConfig_t* pConfig);


/*****************************************************************************/
/**
 * @brief Get the size of the buffer queue if an external buffer pool is used.
 *
 * @param   pConfig  Configuration settings for Extended Media Buffer Queue.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExGetSizeExtPool(MediaBufQueueExConfig_t* pConfig);


/*****************************************************************************/
/**
 * @brief Create a buffer queue.
 *
 * @param   pBufQueue    Pointer to Extended Media Buffer Queue object.
 * @param   pConfig      Configuration settings for Extended Media Buffer Queue.
 * @param   queueMemory  Memory to create the Extended Media Buffer Queue in.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExCreate(MediaBufQueueEx_t*       pBufQueue,
                                    MediaBufQueueExConfig_t* pConfig,
                                    MediaBufQueueExMemory_t  queueMemory);


/*****************************************************************************/
/**
 * @brief Create a special buffer queue using an existing, external buffer pool.
 *
 *        This way single producer, multiple consumer setups can be achieved:<\n>
 *
 *        - The producer would normally register its callbacks directly with the
 *        external buffer pool, retrieve empty buffers from that buffer pool
 *        and put filled buffers into that buffer pool first and then into all
 *        output queues. Watermark levels are specific to the buffer pool, queues
 *        hold their own levels which are initialized to the levels of the buffer
 *        pool on queue creation.<\n>
 *
 *        - The consumers would normally register their callbacks with their
 *        individual queues, take the filled buffers and release them from those
 *        queues and may change queue specific watermark levels to their
 *        requirements.<\n>
 *
 *        - The controlling instance will have to register its callbacks with the
 *        buffer pool and the output queues to able to get informed and see the full
 *        picture of watermarks, fill levels and the like.
 *
 *        Buffers keep being locked until all consumers have released them. It is
 *        obvious that in this setup buffer size manipulation may not work as
 *        expected, so be warned!
 *
 *        Queue flushing and resetting will not be propergated to the underlying
 *        buffer pool, so this must be done manually for the buffer pool after
 *        all involved output queues have been flushed or reset.
 *
 * @param   pBufQueue    Pointer to Extended Media Buffer Queue object.
 * @param   pConfig      Configuration settings for Extended Media Buffer Queue.
 * @param   queueMemory  Memory to create the Extended Media Buffer Queue in.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExCreateExtPool(MediaBufQueueEx_t*       pBufQueue,
                                           MediaBufQueueExConfig_t* pConfig,
                                           MediaBufQueueExMemory_t  queueMemory,
                                           MediaBufPool_t*          pExtBufPool);


/*****************************************************************************/
/**
 * @brief Destroy a buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExDestroy(MediaBufQueueEx_t* pBufQueue);


/*****************************************************************************/
/**
 * @brief Get an empty buffer from a buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Pointer to the media buffer.
 *          Can be NULL if no such buffer is available.
 *****************************************************************************/
extern MediaBuffer_t* MediaBufQueueExGetEmptyBuffer(MediaBufQueueEx_t* pBufQueue);


/*****************************************************************************/
/**
 * @brief Insert a full buffer into a buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExPutFullBuffer(MediaBufQueueEx_t* pBufQueue,
                                           MediaBuffer_t*     pBuf);


/*****************************************************************************/
/**
 * @brief Finalize putting a buffer into queues with external buffer pool.
 *        For buffer queues with external buffer pools (e.g. multi consumer setup)
 *        it is necessary to lock a buffer from retrieval as empty buffer until
 *        it is put into all desired queues.
 *        Call this function once after the buffer was put into all those queues,
 *        passing in any one of those queues as parameter.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExPutFullBufferDone(MediaBufQueueEx_t* pBufQueue,
                                               MediaBuffer_t*     pBuf);


/*****************************************************************************/
/**
 * @brief Get a full buffer out of the buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Pointer to the media buffer.
 *          Can be NULL if no such buffer is available.
 *****************************************************************************/
extern MediaBuffer_t* MediaBufQueueExGetFullBuffer(MediaBufQueueEx_t* pBufQueue);


/*****************************************************************************/
/**
 * @brief Release a buffer to a buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExReleaseBuffer(MediaBufQueueEx_t* pBufQueue,
                                           MediaBuffer_t*     pBuf);


/*****************************************************************************/
/**
 * @brief Flush buffer queue, i.e. empty all full buffers which have not yet
 *        been retrieved. Also resets all statistics.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExFlush(MediaBufQueueEx_t* pBufQueue);


/*****************************************************************************/
/**
 * @brief Reset buffer queue, i.e. produce same state as after creation
 *        (configuration is not discarded).
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExReset(MediaBufQueueEx_t* pBufQueue);


/* ***************************************************************************/
/**
 * @brief   Set a parameter for a buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   paramId     Id of parameter to be set.
 * @param   paramValue  Value of parameter to be set.
 *
 * @return  RESULT
 *****************************************************************************/
extern RESULT MediaBufQueueExSetParameter(MediaBufQueueEx_t*       pBufQueue,
                                          MediaBufQueueExParamId_t paramId,
                                          int32_t                  paramValue);


/* ***************************************************************************/
/**
 * @brief   Get a parameter for a buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   paramId     Id of parameter to get.
 * @param   pParamValue Address to receive parameter value.
 *
 * @return  RESULT
 *****************************************************************************/
extern RESULT MediaBufQueueExGetParameter(MediaBufQueueEx_t*       pBufQueue,
                                          MediaBufQueueExParamId_t paramId,
                                          int32_t*                 pParamValue);


/*****************************************************************************/
/**
 * @brief Lock a buffer of a buffer queue. Buffer will not be available as
 *        empty buffer until unlocked as many times as locked before
 *        and released.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExLockBuffer(MediaBufQueueEx_t* pBufQueue,
                                        MediaBuffer_t*     pBuf);


/*****************************************************************************/
/**
 * @brief Unlock a buffer of a buffer queue which has previously been locked.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExUnlockBuffer(MediaBufQueueEx_t* pBufQueue,
                                          MediaBuffer_t*     pBuf);


/*****************************************************************************/
/**
 * @brief Reterieve statistics for a buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   pStat       Pointer to statistics structure to be filled.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExGetStats(MediaBufQueueEx_t*     pBufQueue,
                                      MediaBufQueueExStat_t* pStat);


/* ***************************************************************************/
/**
 * @brief   Register a callback to get informed about MediaBufQueueEx_t events.
 *
 * @param   pBufQueue       Pointer to Extended Media Buffer Queue object.
 * @param   fpCallback      Function to be triggered in case of event.
 * @param   pUserContext    Pointer to be passed to callback.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExRegisterCb(MediaBufQueueEx_t*         pBufQueue,
                                        MediaBufQueueExCbNotify_t  fpCallback,
                                        void*                      pUserContext);


/* ***************************************************************************/
/**
 * @brief   Deregister a callback from MediaBufQueueEx_t.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 * @param   fpCallback  Function to be deregistered.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufQueueExDeregisterCb(MediaBufQueueEx_t *       pBufQueue,
                                          MediaBufQueueExCbNotify_t fpCallback);



/*****************************************************************************/
/**
 * @brief Returns base address of buffer memory.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Base address of buffer memory.
 *****************************************************************************/
INLINE void* MediaBufQueueExGetBufMemBaseAddr(const MediaBufQueueEx_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    return pBufQueue->pBufPool->pBufArray[0].pBaseAddress;
}


/*****************************************************************************/
/**
 * @brief Returns size of total buffer memory.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Size of buffer memory.
 *****************************************************************************/
INLINE uint32_t MediaBufQueueExGetBufMemTotalSize(const MediaBufQueueEx_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    return pBufQueue->pBufPool->poolSize;
}


/*****************************************************************************/
/**
 * @brief Returns size of memory covered by a single buffer.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Size of memory covered by a single buffer.
 *****************************************************************************/
INLINE uint32_t MediaBufQueueExGetBufSize(const MediaBufQueueEx_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    return pBufQueue->pBufPool->bufSize;
}


/*****************************************************************************/
/**
 * @brief Returns number of buffers in a buffer queue.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Current number of buffers in a buffer queue.
 *****************************************************************************/
INLINE uint32_t MediaBufQueueExGetBufNum(const MediaBufQueueEx_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    return pBufQueue->pBufPool->bufNum;
}


/*****************************************************************************/
/**
 * @brief   Gets the number of empty buffers in a media buffer queue.
 *
 * @param   pBufQueue  Pointer to buffer queue object.
 *
 * @return  Number of empty buffers.
 *****************************************************************************/
INLINE uint16_t MediaBufQueueExGetEmptyBufNum(const MediaBufQueueEx_t* pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    return pBufQueue->pBufPool->freeBufNum;
}


/*****************************************************************************/
/**
 * @brief   Gets the fill level a media buffer queue in buffers.
 *
 * @param   pBufQueue  Pointer to buffer queue object.
 *
 * @return  Fill level in buffers.
 *****************************************************************************/
INLINE uint16_t MediaBufQueueExGetFillLevel(const MediaBufQueueEx_t* pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    if (!pBufQueue->isExtPool)
    {
        return pBufQueue->pBufPool->fillLevel;
    }
    else
    {
        return pBufQueue->fullBufsAvail;
    }
}


/*****************************************************************************/
/**
 * @brief   Returns indication if empty buffers are available in a buffer queue.
 *
 * @param   pBufQueue  Pointer to buffer queue object.
 *
 * @return  Empty buffers are available or not.
 *****************************************************************************/
INLINE bool_t MediaBufQueueExEmptyBuffersAvailable(const MediaBufQueueEx_t* pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    return ((bool_t)(pBufQueue->pBufPool->freeBufNum > 0U));
}


/*****************************************************************************/
/**
 * @brief Returns indication if buffer queue is empty.
 *        If so, no full buffers are contained in it,
 *        whether already retreived or not.
 *
 * @param   pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return  Buffer queue is empty or not.
 *****************************************************************************/
INLINE bool_t MediaBufQueueExIsEmpty(const MediaBufQueueEx_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    if (!pBufQueue->isExtPool)
    {
        return ((bool_t)(pBufQueue->pBufPool->fillLevel == 0U));
    }
    else
    {
        return ((bool_t)(pBufQueue->fullBufsAvail == 0U));
    }
}


/*****************************************************************************/
/**
 * @brief Returns indication if full buffers are available in a buffer queue
 *        which have not yet been retreived.
 *
 * @param pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return Full buffers are available to be retrieved or not.
 *****************************************************************************/
INLINE bool_t MediaBufQueueExFullBuffersAvailable(const MediaBufQueueEx_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);

    return ((bool_t)(pBufQueue->fullBufsAvail > 0U));
}


/*****************************************************************************/
/**
 * @brief Returns indication if buffer queue is full.
 *        If so, no full buffers can be added to it.
 *
 * @param pBufQueue   Pointer to Extended Media Buffer Queue object.
 *
 * @return Buffer queue is full or not.
 *****************************************************************************/
INLINE bool_t MediaBufQueueExIsFull(const MediaBufQueueEx_t *pBufQueue)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    if (!pBufQueue->isExtPool)
    {
        return ((bool_t)(pBufQueue->pBufPool->fillLevel == pBufQueue->pBufPool->bufNum));
    }
    else
    {
        return ((bool_t)(pBufQueue->fullBufsAvail == pBufQueue->pBufPool->bufNum));
    }
}


 /*@}*/

#if defined (__cplusplus)
}
#endif
#endif  /* MEDIA_BUFFER_QUEUE_EX_H */

