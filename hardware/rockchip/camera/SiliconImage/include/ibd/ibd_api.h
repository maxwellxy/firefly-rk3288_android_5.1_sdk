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
 * @file ibd_api.h
 *
 * @brief
 *   Definition of ibd API.
 *
 *****************************************************************************/
/**
 * @page ibd_page IBD
 * The In-Buffer Display Module allows to perform some simple graphics stuff on image buffers.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref ibd_api
 * - @ref ibd_common
 * - @ref ibd
 *
 * @defgroup ibd_api IBD API
 * @{
 *
 */

#ifndef __IBD_API_H__
#define __IBD_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <ebase/types.h>
#include <common/return_codes.h>

#include <common/picture_buffer.h>
#include <bufferpool/media_buffer.h>

#include <hal/hal_api.h>

#include "ibd_common.h"

/*****************************************************************************/
/**
 * @brief   Open the IBD driver for the given picture buffer.
 *
 * @param   halHandle           Handle to HAL session to use.
 * @param   pBuffer             The media buffer to use for drawing.
 *
 * @return  Handle to use for drawing into this buffer; NULL on failure.
 *
 *****************************************************************************/
extern ibdHandle_t ibdOpenMapped
(
    HalHandle_t     halHandle,
    MediaBuffer_t   *pBuffer
);

/*****************************************************************************/
/**
 * @brief   Open the IBD driver for the given picture buffer.
 *
 * @param   pPicBufMetaData     The picture buffer to use for drawing.
 *
 * @return  Handle to use for drawing into this buffer; NULL on failure.
 *
 *****************************************************************************/
extern ibdHandle_t ibdOpenDirect
(
    PicBufMetaData_t    *pPicBufMetaData
);

/******************************************************************************
 * @brief   Close the IBD driver after drawing to buffer is done.
 *
 * @param   ibdHandle   Handle to IBD session as returned by @ref ibdOpen().
 *
 * @return  Result of operation.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
extern RESULT ibdClose
(
    ibdHandle_t     ibdHandle
);

/*****************************************************************************/
/**
 * @brief   Process given array of drawing commands, using the given picture buffer.
 *
 * @param   ibdHandle       Handle to IBD session as returned by @ref ibdOpen().
 * @param   numCmds         Number of commands in command array.
 * @param   pIbdCmds        Reference to command array.
 * @param   scaledCoords    Coords are given as scale factors in 0.32 fixed point
 *                          representation rather than as absolut pixel coords.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
extern RESULT ibdDraw
(
    ibdHandle_t     ibdHandle,
    uint32_t        numCmds,
    ibdCmd_t        *pIbdCmds,
    bool_t          scaledCoords
);

/******************************************************************************
 * @brief   Convert absolute to scaled coordinate representation.
 *
 * @param   curCoord    Coordinate to calc scale factor for.
 * @param   maxCoord    Max value of coordinate.
 *
 * @return  Scaled coordinate value.
 *
 *****************************************************************************/
INLINE int32_t ibdScaleCoord
(
    int32_t     curCoord,
    int32_t     maxCoord
)
{
    double scaled;

    //TODO: this is a quick hack, better try to avoid floating point arithmetic
    scaled  = ((double)curCoord) / ((double)maxCoord);
    scaled *= ((double)0x80000000ul);

    return ((int32_t)scaled);
}

/******************************************************************************
 * @brief   Convert scaled to absolute coordinate representation.
 *
 * @param   scaledCoord Coordinate to calc absolute value for.
 * @param   maxCoord    Max value of coordinate.
 *
 * @return  Absolute coordinate value.
 *
 *****************************************************************************/
INLINE int32_t ibdUnscaleCoord
(
    int32_t     scaledCoord,
    int32_t     maxCoord
)
{
    double unscaled;

    //TODO: this is a quick hack, better try to avoid floating point arithmetic
    unscaled  = ((double)scaledCoord) / ((double)0x80000000ul);
    unscaled *= ((double)maxCoord);
    unscaled += 0.5;

    return ((int32_t)unscaled);
}

/* @} ibd_api */

#ifdef __cplusplus
}
#endif

#endif /* __IBD_API_H__ */
