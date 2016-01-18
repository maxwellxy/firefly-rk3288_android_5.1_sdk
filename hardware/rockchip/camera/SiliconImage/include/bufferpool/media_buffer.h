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
 * @file media_buffer.h
 *
 * @brief
 *          Media Buffer interface
 *
 * <pre>
 *
 *   Principal Author: Joerg Detert
 *   Creation date:    Feb 28, 2008
 *
 * </pre>
 *
 *****************************************************************************/
#ifndef MEDIA_BUFFER_H_
#define MEDIA_BUFFER_H_


#include <ebase/types.h>
#include <ebase/dct_assert.h>

#include <common/return_codes.h>

#if defined (__cplusplus)
extern "C" {
#endif


/**
 *  @brief Enumeration of buffer flags used by @ref ScmiBuffer
 *
 */
enum /* BUFFER_FLAGS */
{
    BUFFER_LAST             = 0x00000001,   /**< Indicates the last buffer of a sequence of buffers
                                             (e.g. last input buffer in stream, last output buffer in
                                             sequence, or last buffer of a unit of application data). */
    BUFFER_END_OF_SPLICE    = 0x00000002    /**< Indicates the last buffer of a splice. */
};

/**
 * @brief The MediaBufferPool holds elements from type MediaBuffer_t.
 */
typedef struct MediaBuffer_s
{
    uint8_t*      pBaseAddress; /**< Base address of system memory buffer (can differ from
                                     actual buffer start address, set in ScmiBuffer). */
    uint32_t      baseSize;     /**< Base size of buffer (can differ from actual buffer
                                     size, set in ScmiBuffer). */
    uint32_t      lockCount;    /**< Counting how many times buffer is used. 0 means
                                     buffer belongs to pool and is free. */
    void*         pOwner;
    uint32_t      duration;     /**< Used to calculate duration */
    bool_t        syncPoint;    /**< Indicates if buffer contains a reference frame. */
    bool_t        last;         /**< Media source module signals delivery of last buffer. */
    bool_t        isFull;       /**< Flag set to TRUE when buffer is put in queue as a full buffer */
    void*         pNext;        /**< Common next pointer that can be used to create linked lists of media buffers. */
    void*         pMetaData;    /**< Pointer to optional meta data structure. */
} MediaBuffer_t;


/*****************************************************************************/
/**
 * @brief   Initialize a @ref MediaBuffer_t.
 *
 * @param   pBuf    Buffer to initialize.
 *
 *****************************************************************************/
extern void MediaBufInit(MediaBuffer_t* pBuf);


/*****************************************************************************/
/**
 * @brief   Resize a @ref MediaBuffer_t.
 *
 * @param   pBuf         Buffer to adjust in size.
 * @param   frontAdjust  Relative front adjust to resize used buffer space.
 * @param   backAdjust   Relative back adjust to resize used buffer space.
 *
 *****************************************************************************/
extern void MediaBufResize(MediaBuffer_t* pBuf, uint32_t frontAdjust, uint32_t backAdjust);


/*****************************************************************************/
/**
 *
 *   Functions to get/read members of media buffer structure.
 *
 *****************************************************************************/


/*****************************************************************************/
/**
 * @brief   Returns indication if buffer contains a frame suitable as sync point.
 *
 * @param   pBuf    Buffer of interest.
 *
 * @return  Buffer contains sync point frame or not.
 *
 *****************************************************************************/
INLINE bool_t MediaBufIsSyncPoint(const MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBuf != NULL);

    return  pBuf->syncPoint;
}


/*****************************************************************************/
/**
 * @brief   Returns indication if buffer is last one delivered by a source
 *          module.
 *
 * @param   pBuf    Buffer of interest.
 *
 * @return  Buffer is last or not.
 *
 *****************************************************************************/
INLINE bool_t MediaBufIsLast(const MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBuf != NULL);

    return pBuf->last;
}


/*****************************************************************************/
/**
 * @brief   Returns the base (original) size of media buffer.
 *
 * @param   pBuf    Buffer of interest.
 *
 * @return  Original (unadjusted) size of buffer (i.e. size of underlying physical memory chunk).
 *
 *****************************************************************************/
INLINE uint32_t MediaBufGetBaseSize(const MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBuf != NULL);

    return pBuf->baseSize;
}


/*****************************************************************************/
/**
 * @brief   Returns the base (original) start address of the selected media
 *          buffer. Note that any resizing is _NOT_ considered.
 *
 * @param   pBuf    Buffer of interest.
 *
 * @return  Pointer to start of buffer.
 *

 *****************************************************************************/
INLINE uint8_t* MediaBufGetBaseAddress(const MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBuf != NULL);

    return pBuf->pBaseAddress;
}


/*****************************************************************************/
/**
 * @brief   Returns the indication if metadata is available for this media
 *          buffer.
 *
 * @param   pBuf    Buffer of interest.
 *
 * @return  Metadata is available or not.
 *
 *****************************************************************************/
INLINE bool_t MediaBufHasMetaData(const MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBuf != NULL);

    return ( (bool_t)(pBuf->pMetaData != NULL) );
}


/*****************************************************************************/
/**
 * @brief   Returns the metadata for this media buffer.
 *
 * @param   pBuf    Buffer of interest.
 *
 * @return  Pointer to meta data.
 *
 *****************************************************************************/
INLINE void* MediaBufGetMetaData( const MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBuf != NULL);

    return pBuf->pMetaData;
}


/*****************************************************************************/
/**
 *   Functions to set/write members of media buffer structure
 *****************************************************************************/


/*****************************************************************************/
/**
 * @brief   Set the sync point indication flag for this media buffer.
 *
 * @param   pBuf        Target buffer.
 * @param   syncPoint   This buffer contains a sync point frame or not.
 *
 *****************************************************************************/
INLINE void MediaBufSetIsSyncPoint(MediaBuffer_t *pBuf, bool_t syncPoint)
{
    DCT_ASSERT(pBuf != NULL);

    pBuf->syncPoint = syncPoint;
}


/*****************************************************************************/
/**
 * @brief   Set the last buffer in stream indication flag for this media buffer.
 *
 * @param   pBuf    Target buffer.
 * @param   last    Buffer is last in stream or not.
 *
 *****************************************************************************/
INLINE void MediaBufSetIsLast(MediaBuffer_t *pBuf, bool_t last)
{
    DCT_ASSERT(pBuf != NULL);

    pBuf->last = last;
}


/*****************************************************************************/
/**
 * @brief Lock a buffer of a owning buffer pool. Buffer will not be available as
 *        empty buffer until unlocked as many times as locked before
 *        and released.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufLockBuffer(MediaBuffer_t*   pBuf);


/*****************************************************************************/
/**
 * @brief Unlock a buffer of a owning buffer pool which has previously been locked.
 *
 * @param   pBufQueue   Pointer to Media Buffer Queue object.
 * @param   pBuf        Pointer to media buffer.
 *
 * @return  Status of operation.
 *****************************************************************************/
extern RESULT MediaBufUnlockBuffer(MediaBuffer_t*   pBuf);


#if defined (__cplusplus)
}
#endif


#endif /*MEDIA_BUFFER_H_*/
