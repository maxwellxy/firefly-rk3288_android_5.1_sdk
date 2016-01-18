/*

* rockchip hwcomposer( 2D graphic acceleration unit) .

*

* Copyright (C) 2015 Rockchip Electronics Co., Ltd.

*/




#include "rk_hwcomposer.h"
#include <linux/fb.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
//#include <linux/android_pmem.h>
#include <ui/PixelFormat.h>
#include <fcntl.h>




/*******************************************************************************
**
**  YUV pixel formats of android hal.
**
**  Different android versions have different definitaions.
**  These are collected from hardware/libhardware/include/hardware/hardware.h
*/



hwcSTATUS
hwcGetFormat(
    IN  struct private_handle_t * Handle,
    OUT RgaSURF_FORMAT * Format
    
    )
{
    struct private_handle_t *handle = Handle;
    if (Format != NULL)
    {
    	
        switch (GPU_FORMAT)
        {
        case HAL_PIXEL_FORMAT_RGB_565:
            *Format = RK_FORMAT_RGB_565;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            *Format = RK_FORMAT_RGB_888;
            break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            *Format = RK_FORMAT_RGBA_8888;
            break;

        case HAL_PIXEL_FORMAT_RGBX_8888:
            *Format = RK_FORMAT_RGBX_8888;
            break;


        case HAL_PIXEL_FORMAT_BGRA_8888:
            *Format = RK_FORMAT_BGRA_8888;
            break;

        case HAL_PIXEL_FORMAT_YCrCb_NV12:
            /* YUV 420 semi planner: NV12 */
            *Format = RK_FORMAT_YCbCr_420_SP;
            break;
		case HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO:
		   *Format = RK_FORMAT_YCbCr_420_SP;
			 break; 
        default:
            return hwcSTATUS_INVALID_ARGUMENT;
        }
    }


    return hwcSTATUS_OK;
}


int hwChangeRgaFormat(IN int fmt )
{
    switch (fmt)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
        return RK_FORMAT_RGB_565;
    case HAL_PIXEL_FORMAT_RGB_888:
        return RK_FORMAT_RGB_888;
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return RK_FORMAT_RGBA_8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return RK_FORMAT_RGBX_8888;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return RK_FORMAT_BGRA_8888;
    case HAL_PIXEL_FORMAT_YCrCb_NV12:
        return RK_FORMAT_YCbCr_420_SP;
	case HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO:
	   return RK_FORMAT_YCbCr_420_SP;
    default:
        return hwcSTATUS_INVALID_ARGUMENT;
    }
}

int hwcGetBufferSizeForRga(IN int w,IN int h,IN int fmt)
{
    float bpp = 4;
    int size;
    switch(fmt){
        case RK_FORMAT_RGB_565:
            bpp = 2;
            break;
        case RK_FORMAT_RGB_888:
            bpp = 3;
            break;
        case RK_FORMAT_RGBA_8888:
        case RK_FORMAT_RGBX_8888:
        case RK_FORMAT_BGRA_8888:
            bpp = 4;
            break;
        case RK_FORMAT_YCbCr_420_SP:
            bpp = 1.5;
            break;
        default:
            break;
    }
    size = int(w * h * bpp) + 1;
    return size;
}

#if VIRTUAL_RGA_BLIT
hwcSTATUS
hwcGetBufferInfo(
      hwcContext  * Context,
      struct private_handle_t * Handle,
     void * * Logical,
     unsigned int  * Physical,
     unsigned int  * Width,
     unsigned int  * Height,
     unsigned int  * Stride,
     void * * Info
    )
{
    hwcSTATUS status = hwcSTATUS_OK;
    struct private_handle_t * handle = Handle;
    *Width  = GPU_WIDTH ;
    *Height = GPU_HEIGHT;
    *Stride = handle->stride;
    return status;
}
#endif


/*******************************************************************************
**
**  YUV pixel formats of android hal.
**
**  Different android versions have different definitaions.
**  These are collected from hardware/libhardware/include/hardware/hardware.h
*/

#if VIRTUAL_RGA_BLIT
hwcSTATUS
hwcGetBufFormat(
      struct private_handle_t * Handle,
     RgaSURF_FORMAT * Format)
{
    struct private_handle_t *handle = Handle;
    if (Format != NULL)
    {
        switch (GPU_FORMAT)
        {
        case HAL_PIXEL_FORMAT_RGB_565:
            *Format = RK_FORMAT_RGB_565;
            break;

        case HAL_PIXEL_FORMAT_RGBA_8888:
            *Format = RK_FORMAT_RGBA_8888;
            break;

        case HAL_PIXEL_FORMAT_RGBX_8888:
            *Format = RK_FORMAT_RGBX_8888;
            break;


        case HAL_PIXEL_FORMAT_BGRA_8888:
            *Format = RK_FORMAT_BGRA_8888;
            break;

        case HAL_PIXEL_FORMAT_YCrCb_NV12:
            /* YUV 420 semi planner: NV12 */
            *Format = RK_FORMAT_YCbCr_420_SP;
            break;
		case HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO:
		   *Format = RK_FORMAT_YCbCr_420_SP;
			 break;
        default:
            return hwcSTATUS_INVALID_ARGUMENT;
        }
    }

    return hwcSTATUS_OK;
}

#if ENABLE_HWC_WORMHOLE
/*
 * Area spliting feature depends on the following 3 functions:
 * '_AllocateArea', '_FreeArea' and '_SplitArea'.
 */
#define MEM_SIZE 512

hwcArea *
zone_alloc(
     hwcContext * Context,
     hwcArea * Slibing,
     hwcRECT * Rect,
     int Owner
    )
{
    hwcArea * area;
    hwcAreaPool * pool  = &Context->areaMem;

    while(true)
    {
        if (pool->areas == NULL)
        {

            /* No areas allocated, allocate now. */
            pool->areas = (hwcArea *) malloc(sizeof (hwcArea) * MEM_SIZE);

            /* Get area. */
            area = pool->areas;

            /* Update freeNodes. */
            pool->freeNodes = area + 1;

            break;
        }

        else if (pool->freeNodes - pool->areas >= MEM_SIZE)
        {
            /* This pool is full. */
            if (pool->next == NULL)
            {
                pool->next = (hwcAreaPool *) malloc(sizeof (hwcAreaPool));

                pool = pool->next;

                pool->areas     = NULL;
                pool->freeNodes = NULL;
                pool->next      = NULL;
            }

            else
            {
                pool = pool->next;
            }
        }

        else
        {
            area = pool->freeNodes++;

            break;
        }
    }

    area->rect   = *Rect;
    area->owners = Owner;

	//LOGD("area->rect.left=%d,top=%d,right=%d,bottom=%d,area->owners=%d",area->rect.left,area->rect.top,area->rect.right,area->rect.bottom,area->owners);
    if (Slibing == NULL)
    {
        area->next = NULL;
    }

    else if (Slibing->next == NULL)
    {
        area->next = NULL;
        Slibing->next = area;
    }

    else
    {
        area->next = Slibing->next;
        Slibing->next = area;
    }

    return area;
}


void
ZoneFree(
     hwcContext * Context,
     hwcArea* Head
    )
{
    hwcAreaPool * pool  = &Context->areaMem;

    while (pool != NULL)
    {
        if (Head >= pool->areas && Head < pool->areas + MEM_SIZE)
        {
            if (Head < pool->freeNodes)
            {
                pool->freeNodes = Head;

                while (pool->next != NULL)
                {
                    pool = pool->next;

                    pool->freeNodes = pool->areas;
                }
            }

            break;
        }

        else if (pool->freeNodes < pool->areas + MEM_SIZE)
        {
            break;
        }

        else
        {
            pool = pool->next;
        }
    }
}


void
DivArea(
     hwcContext * Context,
     hwcArea * Area,
     hwcRECT * Rect,
     int Owner
    )
{
    hwcRECT r0[4];
    hwcRECT r1[4];
    int i = 0;
    int j = 0;

    hwcRECT * rect;

    while (true)
    {
        rect = &Area->rect;

        if ((Rect->left   < rect->right)
        &&  (Rect->top    < rect->bottom)
        &&  (Rect->right  > rect->left)
        &&  (Rect->bottom > rect->top)
        )
        {
            /* Overlapped. */
            break;
        }

        if (Area->next == NULL)
        {
            /* This rectangle is not overlapped with any area. */
            zone_alloc(Context, Area, Rect, Owner);
            return;
        }

        Area = Area->next;
    }

    if ((Rect->left <= rect->left)
    &&  (Rect->right >= rect->right)
    )
    {

        if (Rect->left < rect->left)
        {
            r1[j].left   = Rect->left;
            r1[j].top    = Rect->top;
            r1[j].right  = rect->left;
            r1[j].bottom = Rect->bottom;

            j++;
        }

        if (Rect->top < rect->top)
        {
            r1[j].left   = rect->left;
            r1[j].top    = Rect->top;
            r1[j].right  = rect->right;
            r1[j].bottom = rect->top;

            j++;
        }

        else if (rect->top < Rect->top)
        {
            r0[i].left   = rect->left;
            r0[i].top    = rect->top;
            r0[i].right  = rect->right;
            r0[i].bottom = Rect->top;

            i++;
        }

        if (Rect->right > rect->right)
        {
            r1[j].left   = rect->right;
            r1[j].top    = Rect->top;
            r1[j].right  = Rect->right;
            r1[j].bottom = Rect->bottom;

            j++;
        }

        if (Rect->bottom > rect->bottom)
        {
            r1[j].left   = rect->left;
            r1[j].top    = rect->bottom;
            r1[j].right  = rect->right;
            r1[j].bottom = Rect->bottom;

            j++;
        }

        else if (rect->bottom > Rect->bottom)
        {
            r0[i].left   = rect->left;
            r0[i].top    = Rect->bottom;
            r0[i].right  = rect->right;
            r0[i].bottom = rect->bottom;

            i++;
        }
    }

    else if (Rect->left <= rect->left)
    {

        if (Rect->left < rect->left)
        {
            r1[j].left   = Rect->left;
            r1[j].top    = Rect->top;
            r1[j].right  = rect->left;
            r1[j].bottom = Rect->bottom;

            j++;
        }

        if (Rect->top < rect->top)
        {
            r1[j].left   = rect->left;
            r1[j].top    = Rect->top;
            r1[j].right  = Rect->right;
            r1[j].bottom = rect->top;

            j++;
        }

        else if (rect->top < Rect->top)
        {
            r0[i].left   = rect->left;
            r0[i].top    = rect->top;
            r0[i].right  = Rect->right;
            r0[i].bottom = Rect->top;

            i++;
        }

        r0[i].left   = Rect->right;
        r0[i].top    = rect->top;
        r0[i].right  = rect->right;
        r0[i].bottom = rect->bottom;

        i++;

        if (Rect->bottom > rect->bottom)
        {
            r1[j].left   = rect->left;
            r1[j].top    = rect->bottom;
            r1[j].right  = Rect->right;
            r1[j].bottom = Rect->bottom;

            j++;
        }

        else if (rect->bottom > Rect->bottom)
        {
            r0[i].left   = rect->left;
            r0[i].top    = Rect->bottom;
            r0[i].right  = Rect->right;
            r0[i].bottom = rect->bottom;

            i++;
        }
    }

    else if (Rect->right >= rect->right)
    {

        r0[i].left   = rect->left;
        r0[i].top    = rect->top;
        r0[i].right  = Rect->left;
        r0[i].bottom = rect->bottom;

        i++;

        if (Rect->top < rect->top)
        {
            r1[j].left   = Rect->left;
            r1[j].top    = Rect->top;
            r1[j].right  = rect->right;
            r1[j].bottom = rect->top;

            j++;
        }

        else if (rect->top < Rect->top)
        {
            r0[i].left   = Rect->left;
            r0[i].top    = rect->top;
            r0[i].right  = rect->right;
            r0[i].bottom = Rect->top;

            i++;
        }

        if (Rect->right > rect->right)
        {
            r1[j].left   = rect->right;
            r1[j].top    = Rect->top;
            r1[j].right  = Rect->right;
            r1[j].bottom = Rect->bottom;

            j++;
        }

        if (Rect->bottom > rect->bottom)
        {
            r1[j].left   = Rect->left;
            r1[j].top    = rect->bottom;
            r1[j].right  = rect->right;
            r1[j].bottom = Rect->bottom;

            j++;
        }

        else if (rect->bottom > Rect->bottom)
        {
            r0[i].left   = Rect->left;
            r0[i].top    = Rect->bottom;
            r0[i].right  = rect->right;
            r0[i].bottom = rect->bottom;

            i++;
        }
    }

    else
    {

        r0[i].left   = rect->left;
        r0[i].top    = rect->top;
        r0[i].right  = Rect->left;
        r0[i].bottom = rect->bottom;

        i++;

        if (Rect->top < rect->top)
        {
            r1[j].left   = Rect->left;
            r1[j].top    = Rect->top;
            r1[j].right  = Rect->right;
            r1[j].bottom = rect->top;

            j++;
        }

        else if (rect->top < Rect->top)
        {
            r0[i].left   = Rect->left;
            r0[i].top    = rect->top;
            r0[i].right  = Rect->right;
            r0[i].bottom = Rect->top;

            i++;
        }

        r0[i].left   = Rect->right;
        r0[i].top    = rect->top;
        r0[i].right  = rect->right;
        r0[i].bottom = rect->bottom;

        i++;

        if (Rect->bottom > rect->bottom)
        {
            r1[j].left   = Rect->left;
            r1[j].top    = rect->bottom;
            r1[j].right  = Rect->right;
            r1[j].bottom = Rect->bottom;

            j++;
        }

        else if (rect->bottom > Rect->bottom)
        {
            r0[i].left   = Rect->left;
            r0[i].top    = Rect->bottom;
            r0[i].right  = Rect->right;
            r0[i].bottom = rect->bottom;

            i++;
        }
    }

    if (j > 0)
    {
        if (Area->next == NULL)
        {
            for (int k = 0; k < j; k++)
            {
                zone_alloc(Context, Area, &r1[k], Owner);
            }
        }

        else
        {
            for (int k = 0; k < j; k++)
            {
                DivArea(Context, Area, &r1[k], Owner);
            }
        }
    }

    if (i > 0)
    {
        for (int k = 0; k < i; k++)
        {
            zone_alloc(Context, Area, &r0[k], Area->owners);
        }

        if (rect->left   < Rect->left)   { rect->left   = Rect->left;   }
        if (rect->top    < Rect->top)    { rect->top    = Rect->top;    }
        if (rect->right  > Rect->right)  { rect->right  = Rect->right;  }
        if (rect->bottom > Rect->bottom) { rect->bottom = Rect->bottom; }
    }

    Area->owners |= Owner;
}
#endif
#endif
