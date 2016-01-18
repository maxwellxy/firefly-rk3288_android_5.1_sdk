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
 * @file mapbuffer.h
 *
 * @brief
 *   Mapping of buffers.
 *
 *****************************************************************************/
/**
 *
 * @mainpage Module Documentation
 *
 *
 * Doc-Id: xx-xxx-xxx-xxx (NAME Implementation Specification)\n
 * Author: NAME
 *
 * DESCRIBE_HERE
 *
 *
 * The manual is divided into the following sections:
 *
 * -@subpage module_name_api_page \n
 * -@subpage module_name_page \n
 *
 * @page module_name_api_page Module Name API
 * This module is the API for the NAME. DESCRIBE IN DETAIL / ADD USECASES...
 *
 * for a detailed list of api functions refer to:
 * - @ref module_name_api
 *
 * @defgroup module_name_api Module Name API
 * @{
 */

#ifndef __MAPBUFFER_H__
#define __MAPBUFFER_H__

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <hal/hal_api.h>
#include <common/picture_buffer.h>


/******************************************************************************
 * mapRawBuffer
 *****************************************************************************/
static bool mapRawBuffer( HalHandle_t hal, const PicBufMetaData_t *pSrcBuffer, PicBufMetaData_t *pDstBuffer )
{
    DCT_ASSERT( NULL != hal );

    DCT_ASSERT( NULL != pDstBuffer );
    DCT_ASSERT( NULL != pSrcBuffer );

    if ( ( NULL == pSrcBuffer->Data.raw.pBuffer        ) ||
         ( 0    == pSrcBuffer->Data.raw.PicWidthPixel  ) ||
         ( 0    == pSrcBuffer->Data.raw.PicWidthBytes  ) ||
         ( 0    == pSrcBuffer->Data.raw.PicHeightPixel ) )
    {
        MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
        return false;
    }

    // copy meta data
    *pDstBuffer = *pSrcBuffer;

    pDstBuffer->Data.raw.pBuffer = NULL;

    // get sizes & base addresses of plane
    uint32_t PlaneSize = pSrcBuffer->Data.raw.PicWidthBytes * pSrcBuffer->Data.raw.PicHeightPixel;
    ulong_t  BaseAddr  = (ulong_t) (pSrcBuffer->Data.raw.pBuffer);

    if ( RET_SUCCESS != HalMapMemory( hal, BaseAddr, PlaneSize, HAL_MAPMEM_READONLY,
                             (void**)&(pDstBuffer->Data.raw.pBuffer) ) )
    {
        MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
        return false;
    }

    return true;
}


/******************************************************************************
 * mapYCbCrBuffer
 *****************************************************************************/
static bool mapYCbCrBuffer( HalHandle_t hal, const PicBufMetaData_t *pSrcBuffer, PicBufMetaData_t *pDstBuffer )
{
    DCT_ASSERT( NULL != hal );

    DCT_ASSERT( NULL != pDstBuffer );
    DCT_ASSERT( NULL != pSrcBuffer );

    if ( ( NULL == pSrcBuffer->Data.raw.pBuffer        ) ||
         ( 0    == pSrcBuffer->Data.raw.PicWidthPixel  ) ||
         ( 0    == pSrcBuffer->Data.raw.PicWidthBytes  ) ||
         ( 0    == pSrcBuffer->Data.raw.PicHeightPixel ) )
    {
        MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
        return false;
    }

    // copy meta data
    *pDstBuffer = *pSrcBuffer;

    if ( PIC_BUF_LAYOUT_COMBINED == pSrcBuffer->Layout )
    {
        pDstBuffer->Data.YCbCr.combined.pBuffer = NULL;

        // get sizes & base addresses of plane
        uint32_t YCPlaneSize = pSrcBuffer->Data.YCbCr.combined.PicWidthBytes * pSrcBuffer->Data.YCbCr.combined.PicHeightPixel;
        ulong_t  YCBaseAddr  = (ulong_t) (pSrcBuffer->Data.YCbCr.combined.pBuffer);

        // map combined plane
        if ( RET_SUCCESS != HalMapMemory( hal, YCBaseAddr, YCPlaneSize, HAL_MAPMEM_READONLY,
                             (void**)&(pDstBuffer->Data.YCbCr.combined.pBuffer) ) )
        {
            MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
            return false;
        }
    }
    else if ( PIC_BUF_LAYOUT_SEMIPLANAR == pSrcBuffer->Layout )
    {
        pDstBuffer->Data.YCbCr.semiplanar.Y.pBuffer    = NULL;
        pDstBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer = NULL;

        // get sizes & base addresses of planes
        uint32_t YPlaneSize     = pSrcBuffer->Data.YCbCr.semiplanar.Y.PicWidthBytes * pSrcBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel;
        uint32_t CbCrPlaneSize  = pSrcBuffer->Data.YCbCr.semiplanar.CbCr.PicWidthBytes * pSrcBuffer->Data.YCbCr.semiplanar.CbCr.PicHeightPixel;
        ulong_t  YBaseAddr      = (ulong_t) (pSrcBuffer->Data.YCbCr.semiplanar.Y.pBuffer);
        ulong_t  CbCrBaseAddr   = (ulong_t) (pSrcBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer);

        // map luma plane
        if ( RET_SUCCESS != HalMapMemory( hal, YBaseAddr, YPlaneSize, HAL_MAPMEM_READONLY,
                             (void**)&(pDstBuffer->Data.YCbCr.semiplanar.Y.pBuffer) ) )
        {
            MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
            return false;
        }

        // map combined chroma plane
        if ( RET_SUCCESS != HalMapMemory( hal, CbCrBaseAddr, CbCrPlaneSize, HAL_MAPMEM_READONLY,
                             (void**)&(pDstBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer) ) )
        {
            (void)HalUnMapMemory( hal, pDstBuffer->Data.YCbCr.semiplanar.Y.pBuffer );
            MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
            return false;
        }
    }
    else if ( PIC_BUF_LAYOUT_PLANAR == pSrcBuffer->Layout )
    {
        pDstBuffer->Data.YCbCr.planar.Y.pBuffer  = NULL;
        pDstBuffer->Data.YCbCr.planar.Cb.pBuffer = NULL;
        pDstBuffer->Data.YCbCr.planar.Cr.pBuffer = NULL;

        // get sizes & base addresses of planes
        uint32_t YPlaneSize  = pSrcBuffer->Data.YCbCr.planar.Y.PicWidthBytes * pSrcBuffer->Data.YCbCr.planar.Y.PicHeightPixel;
        uint32_t CbPlaneSize = pSrcBuffer->Data.YCbCr.planar.Cb.PicWidthBytes * pSrcBuffer->Data.YCbCr.planar.Cb.PicHeightPixel;
        uint32_t CrPlaneSize = pSrcBuffer->Data.YCbCr.planar.Cr.PicWidthBytes * pSrcBuffer->Data.YCbCr.planar.Cr.PicHeightPixel;
        ulong_t  YBaseAddr   = (ulong_t) (pSrcBuffer->Data.YCbCr.planar.Y.pBuffer);
        ulong_t  CbBaseAddr  = (ulong_t) (pSrcBuffer->Data.YCbCr.planar.Cb.pBuffer);
        ulong_t  CrBaseAddr  = (ulong_t) (pSrcBuffer->Data.YCbCr.planar.Cr.pBuffer);

        // map luma plane
        if ( RET_SUCCESS != HalMapMemory( hal, YBaseAddr, YPlaneSize, HAL_MAPMEM_READONLY,
                             (void**)&(pDstBuffer->Data.YCbCr.planar.Y.pBuffer) ) )
        {
            MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
            return false;
        }

        // map Cb plane
        if ( RET_SUCCESS != HalMapMemory( hal, CbBaseAddr, CbPlaneSize, HAL_MAPMEM_READONLY,
                             (void**)&(pDstBuffer->Data.YCbCr.planar.Cb.pBuffer) ) )
        {
            (void)HalUnMapMemory( hal, pDstBuffer->Data.YCbCr.semiplanar.Y.pBuffer );
            MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
            return false;
        }

        // map Cr plane
        if ( RET_SUCCESS != HalMapMemory( hal, CrBaseAddr, CrPlaneSize, HAL_MAPMEM_READONLY,
                             (void**)&(pDstBuffer->Data.YCbCr.planar.Cr.pBuffer) ) )
        {
            (void)HalUnMapMemory( hal, pDstBuffer->Data.YCbCr.planar.Cb.pBuffer );
            (void)HalUnMapMemory( hal, pDstBuffer->Data.YCbCr.planar.Y.pBuffer );
            MEMSET( pDstBuffer, 0, sizeof( PicBufMetaData_t ) );
            return false;
        }
    }

    return true;
}


/******************************************************************************
 * unmapRawBuffer
 *****************************************************************************/
static bool unmapRawBuffer( HalHandle_t hal, PicBufMetaData_t *pBuffer )
{
    DCT_ASSERT( NULL != hal );

    DCT_ASSERT( NULL != pBuffer );

    if ( RET_SUCCESS != HalUnMapMemory( hal, (void*)(pBuffer->Data.raw.pBuffer) ) )
    {
        return false;
    }

    MEMSET( pBuffer, 0, sizeof( PicBufMetaData_t ) );
    return true;
}


/******************************************************************************
 * unmapYCbCrBuffer
 *****************************************************************************/
static bool unmapYCbCrBuffer( HalHandle_t hal, PicBufMetaData_t *pBuffer )
{
    DCT_ASSERT( NULL != hal );

    DCT_ASSERT( NULL != pBuffer );

    if ( PIC_BUF_LAYOUT_COMBINED == pBuffer->Layout )
    {
        if ( RET_SUCCESS != HalUnMapMemory( hal, (void*)(pBuffer->Data.YCbCr.combined.pBuffer) ) )
        {
            return false;
        }
    }
    else if ( PIC_BUF_LAYOUT_SEMIPLANAR == pBuffer->Layout )
    {
        if ( RET_SUCCESS != HalUnMapMemory( hal, (void*)(pBuffer->Data.YCbCr.semiplanar.Y.pBuffer) ) )
        {
            return false;
        }
        pBuffer->Data.YCbCr.semiplanar.Y.pBuffer = NULL;

        if ( RET_SUCCESS != HalUnMapMemory( hal, (void*)(pBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer) ) )
        {
            return false;
        }
    }
    else if ( PIC_BUF_LAYOUT_PLANAR == pBuffer->Layout )
    {
        if ( RET_SUCCESS != HalUnMapMemory( hal, (void*)(pBuffer->Data.YCbCr.planar.Y.pBuffer) ) )
        {
            return false;
        }
        pBuffer->Data.YCbCr.planar.Y.pBuffer = NULL;

        if ( RET_SUCCESS != HalUnMapMemory( hal, (void*)(pBuffer->Data.YCbCr.planar.Cb.pBuffer) ) )
        {
            return false;
        }
        pBuffer->Data.YCbCr.planar.Cb.pBuffer = NULL;

        if ( RET_SUCCESS != HalUnMapMemory( hal, (void*)(pBuffer->Data.YCbCr.planar.Cr.pBuffer) ) )
        {
            return false;
        }
    }

    MEMSET( pBuffer, 0, sizeof( PicBufMetaData_t ) );
    return true;
}

/* @} module_name_api*/

#endif /*__MAPBUFFER_H__*/

