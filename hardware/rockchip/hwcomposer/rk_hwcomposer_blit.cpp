/*

* rockchip hwcomposer( 2D graphic acceleration unit) .

*

* Copyright (C) 2015 Rockchip Electronics Co., Ltd.

*/



#include "rk_hwcomposer.h"

#include <fcntl.h>
#ifdef TARGET_BOARD_PLATFORM_RK30XXB
#include <hardware/hal_public.h>
#include <linux/fb.h>
#else
#if GPU_G6110
#include  <hardware/img_gralloc_public.h>
#else
#include "gralloc_priv.h"
#endif
#endif
#include <ui/PixelFormat.h>

#include <cutils/properties.h>
#include "vpu.h"

#ifndef TARGET_SECVM
#include "common/include/ppOp.h"
#else
#include "ppOp.h"
#endif

//#include "struct.h"

#undef LOGV
#define LOGV(...)

#undef LOGI
#define LOGI(...)

extern int is_out_log( void );

#ifdef TARGET_SECVM
#ifdef __cplusplus
extern "C" {
extern int ppOpInit(PP_OP_HANDLE *hnd, PP_OPERATION *init);
extern int ppOpSet(PP_OP_HANDLE hnd, PP_SET_OPT opt, uint32_t val);
extern int ppOpPerform(PP_OP_HANDLE hnd);
extern int ppOpSync(PP_OP_HANDLE hnd);
extern int ppOpRelease(PP_OP_HANDLE hnd);
}
#endif
#endif

/*
int
_HasAlpha(RgaSURF_FORMAT Format)
{
    return (Format == RK_FORMAT_RGB_565) ? false
           : (
               (Format == RK_FORMAT_RGBA_8888) ||
               (Format == RK_FORMAT_BGRA_8888)
           );
}
*/
#if VIRTUAL_RGA_BLIT
static int _DumpFbInfo(struct fb_var_screeninfo *info, int win)
{
    LOGD("dump win%d: vir[%d,%d] [%d,%d,%d,%d] => [%d,%d,%d,%d]", win,
         info->xres_virtual, info->yres_virtual,
         info->xoffset,
         info->yoffset,
         info->xoffset + info->xres,
         info->yoffset + info->yres,
         (info->nonstd >> 8)&0xfff,
         (info->nonstd >> 20)&0xfff,
         ((info->grayscale >> 8)&0xfff) + ((info->nonstd >> 8)&0xfff),
         ((info->grayscale >> 20)&0xfff) + ((info->nonstd >> 20)&0xfff));

    return 0;
}
hwcSTATUS
_ComputeUVOffset(
    IN  RgaSURF_FORMAT Format,
    IN  unsigned int Logical ,
    IN  unsigned int Height,
    IN  unsigned int Stride,
    OUT unsigned int * ULogical,
    OUT unsigned int *  UStride,
    OUT unsigned int *  VLogical,
    OUT unsigned int *  VStride
)
{
    unsigned int uLogical;
    unsigned int vLogical;
    unsigned int uStride;
    unsigned int vStride;

    switch (Format)
    {
        case RK_FORMAT_YCbCr_420_SP:
            uStride   = vStride   = Stride;
            uLogical = vLogical = Logical + Stride * Height; //((Height + 15) & ~15) ;
            break;

        default:
            return hwcSTATUS_INVALID_ARGUMENT;
    }

    /* Return results. */
    if (ULogical != NULL)
    {
        *ULogical = uLogical;
    }

    if (VLogical != NULL)
    {
        *VLogical = vLogical;
    }

    if (UStride != NULL)
    {
        *UStride = uStride;
    }

    if (VStride != NULL)
    {
        *VStride = vStride;
    }

    return hwcSTATUS_OK;
}

#if VIDEO_USE_PPROT

// pp can do check 0 :ok ,-1 cannot support
int hwcppCheck(struct rga_req * rga_p,cmpType mode,int isyuv,int rot,hwcRECT *src,hwcRECT *dst)
{
    float hfactor = 1.0;
    float vfactor = 1.0; 

    if( !(mode == HWC_RGA_TRSM_GPU_VOP || mode == HWC_RGA_TRSM_VOP)
        ||  !isyuv 
        || !rot)
    {
        if(is_out_log())
            ALOGD("exit line=%d,[%d,%d,%d]",__LINE__,mode,isyuv ,rot);
        return -1;
    }
    if(src->left%8 /*|| dst->left%8*/) 
    {
        if(is_out_log())
            ALOGD("exit line=%d,[%d,%d]",__LINE__,src->left, dst->left);
        return -1;
    }         
    if((src->right- src->left)%8 
        || (dst->right - dst->left)%8
        || (src->bottom - src->top)%8) 
    {
        if(is_out_log())
            ALOGD("src w,h,dst w exit line=%d,[%d,%d,%d]",
                __LINE__,src->right- src->left, dst->right- dst->left,src->bottom - src->top);
        return -1;
    }

    hfactor = (float)(rkmALIGN(rga_p->dst.act_w,8))/(float) (rga_p->src.act_w);
    vfactor = (float)(rkmALIGN(rga_p->dst.act_h,2))/(float)(rga_p->src.act_h);
    if(!((hfactor >= 1.0 && vfactor >= 1.0) || (hfactor <= 1.0 && vfactor <= 1.0)))
    {
        if(is_out_log())
            ALOGD("exit line=%d,[%f,%f]",__LINE__,hfactor ,vfactor);
        return -1;
    }
    if(hfactor >= 3 || vfactor >= 3 )
    {
        if(is_out_log())
            ALOGD("exit line=%d,[%f,%f]",__LINE__,hfactor ,vfactor);
        return -1;
    }
    return 0;
}

//return property value of pcProperty
int hwcDobypp(struct rga_req * rga_p,int x,int y,int tra)
{
    int ret;
    static int vpuFd = -1;
    unsigned int src = rga_p->line_draw_info.color & 0xffff;
    unsigned int dst = (rga_p->line_draw_info.color & 0xffff0000)>> 16;
    
#ifndef TARGET_SECVM
	android::PP_OP_HANDLE hnd;
	android::PP_OPERATION opt;
    if(vpuFd < 0)
    {
        vpuFd= VPUClientInit(VPU_PP);
        ALOGV("vpu init fd=%d",vpuFd);
    }
#else
	PP_OP_HANDLE hnd;
    PP_OPERATION opt;
#endif
    memset(&opt, 0, sizeof(opt));
    opt.srcAddr     = src;
    opt.srcFormat   = PP_IN_FORMAT_YUV420SEMI;
    opt.srcHStride  = rga_p->src.vir_w;
    opt.srcVStride  = rga_p->src.vir_h;
    opt.srcWidth    = rga_p->src.act_w;
    opt.srcHeight   = rga_p->src.act_h;
    opt.srcX        = rga_p->src.x_offset;
    opt.srcY        = rga_p->src.y_offset;

    opt.dstAddr     = dst;
    opt.dstFormat   = PP_OUT_FORMAT_YUV420INTERLAVE;
    
    opt.dstHStride   = rga_p->dst.vir_w;
    opt.dstVStride   = rga_p->dst.vir_h;
    
    opt.dstWidth    = rkmALIGN(rga_p->dst.act_h,8);
    opt.dstHeight   = rkmALIGN(rga_p->dst.act_w,2);
    opt.dstX        = rkmALIGN(x,8);
    opt.dstY        = y;

    opt.deinterlace = 0;
    
   // if(wid_alig16 != SRC_WIDTH)
       // opt.srcCrop8R = 1;
  //  if(hei_alig16 != SRC_HEIGHT)
    //opt.srcCrop8D= 1;
        
       
    switch (tra)
    {
        case 0:
            break;
      
        case HWC_TRANSFORM_ROT_90:
            opt.rotation    = PP_ROTATION_RIGHT_90;
            break;

        case HWC_TRANSFORM_ROT_180:
            //opt.dstHStride   = rga_p->dst.vir_h ;
           // opt.dstVStride   = rga_p->dst.vir_w;
            
            opt.dstWidth    = rkmALIGN(rga_p->dst.act_w,8);
            opt.dstHeight   = rkmALIGN(rga_p->dst.act_h,2);
        
            opt.rotation    = PP_ROTATION_180;
            break;

        case HWC_TRANSFORM_ROT_270:
            opt.rotation =  PP_ROTATION_LEFT_90;
            break;
        default:
            break;
    }

#ifdef TARGET_SECVM
    if(!rga_p->src.yrgb_addr )
        VPUMemImport_phyaddr(src, &opt.srcAddr);
	else
        opt.srcAddr =   rga_p->src.yrgb_addr;
	VPUMemImport_phyaddr(dst, &opt.dstAddr);
#endif

    if(is_out_log())
	    ALOGD("src[addr=%x,%d,%d,%d,%d][%d,%d]=>dst[addr=%x,%d,%d,%d,%d][%d,%d],rot=%d,%d",
        opt.srcAddr,opt.srcX,opt.srcY,opt.srcWidth,opt.srcHeight,opt.srcHStride,opt.srcVStride,
        opt.dstAddr,opt.dstX,opt.dstY,opt.dstWidth,opt.dstHeight,opt.dstHStride,opt.dstVStride,tra,HWC_TRANSFORM_ROT_180);
    
    opt.vpuFd       = vpuFd;

#ifndef TARGET_SECVM
    ret |= android::ppOpInit(&hnd, &opt);
    if (ret) {
        ALOGE("ppOpInit failed,vpuFd=%d",vpuFd);
		hnd = NULL;
		goto vpuerr;
	}
#else 
    ret = ppOpInit(&hnd, &opt);
    if (ret < 0) {
        ALOGE("ppOpInit failed.");
        hnd = NULL;
        goto vpuerr;
    }else {
		ALOGV("ppOpInit success fd %d", ret);
		vpuFd = ret;
	}
#endif

#ifndef TARGET_SECVM
	ret = android::ppOpPerform(hnd);
#else
    ret = ppOpPerform(hnd);
#endif
    if (ret) {
        ALOGE("ppOpPerform failed");
        goto vpuerr;

    }

#ifndef TARGET_SECVM
	ret = android::ppOpSync(hnd);
#else
    ret = ppOpSync(hnd);
#endif
    if (ret) {
        ALOGE("ppOpSync failed");
        goto vpuerr;
        
    }
#ifndef TARGET_SECVM
	ret = android::ppOpRelease(hnd);
#else
    ret = ppOpRelease(hnd);
#endif
    if (ret) {    
        ALOGE("ppOpPerform failed");
        goto vpuerr;
        
    }
    
    return 0;
vpuerr:

    ALOGE("src[addr=%x,%d,%d,%d,%d][%d,%d]=>dst[addr=%x,%d,%d,%d,%d][%d,%d],rot=%d",
    opt.srcAddr,opt.srcX,opt.srcY,opt.srcWidth,opt.srcHeight,opt.srcHStride,opt.srcVStride,
    opt.dstAddr,opt.dstX,opt.dstY,opt.dstWidth,opt.dstHeight,opt.dstHStride,opt.dstVStride,tra);
    return -1;

}
#endif
//static int blitcount = 0;
extern int hwc_get_int_property(const char* pcProperty, const char* default_value);

hwcSTATUS
hwcBlit(
    IN hwcContext * Context,
    IN hwc_layer_1_t * Src,
    IN struct private_handle_t * DstHandle,
    IN hwc_rect_t * SrcRect,
    IN hwc_rect_t * DstRect,
    IN hwc_region_t * Region,
    IN FenceMangrRga *FceMrga,
    IN int index
)
{
    hwcSTATUS status = hwcSTATUS_OK;

    hwcRECT             srcRect;
    hwcRECT             dstRect;

    void *              srcLogical  = NULL;
    unsigned int        srcPhysical = ~0;
    void *              srcInfo     = NULL;
    unsigned int        srcStride;
    unsigned int        srcWidth;
    unsigned int        srcHeight;
    RgaSURF_FORMAT      srcFormat;

    void *              dstLogical  = NULL;
    unsigned int        dstPhysical = ~0;
    int                 dstFd, dstBase;
    void *              dstInfo     = NULL;
    unsigned int        dstStride;
    unsigned int        dstWidth;
    unsigned int        dstHeight;
    RgaSURF_FORMAT      dstFormat;

    int                 perpixelAlpha;
    unsigned char       planeAlpha;

    struct private_handle_t* srchnd = (struct private_handle_t *) Src->handle;

    unsigned int n = 0;
    int Rotation = 0;

    float hfactor;
    float vfactor;
    int  stretch;
    int  yuvFormat;
    struct rga_req  Rga_Request;
    RECT clip;
    unsigned char RotateMode = 0;
    unsigned int      Xoffset;
    unsigned int      Yoffset;
    unsigned int      WidthAct;
    unsigned int      HeightAct;
    unsigned char       mmu_en;
    unsigned char   scale_mode = 2;
    unsigned char   dither_en = 0;
    int retv  = 0;
    
    struct private_handle_t* handle = srchnd;
    dither_en = android::bytesPerPixel(GPU_FORMAT) == android::bytesPerPixel(GPU_DST_FORMAT) ? 0 : 1;
    LOGV(" hwcBlit start--->");

    LOGV("layer src w-h[%d,%d]", srchnd->width, srchnd->height);
    memset(&Rga_Request, 0x0, sizeof(Rga_Request));
    /* >>> Begin surface information. */
    hwcONERROR(
        hwcGetBufferInfo(Context,
                      srchnd,
                      &srcLogical,
                      &srcPhysical,
                      &srcWidth,
                      &srcHeight,
                      &srcStride,
                      &srcInfo));

    hwcONERROR(
        hwcGetBufferInfo(Context,
                      DstHandle,
                      &dstLogical,
                      &dstPhysical,
                      &dstWidth,
                      &dstHeight,
                      &dstStride,
                      &dstInfo));

    hwcONERROR(
        hwcGetBufFormat(srchnd,
                     &srcFormat
                    ));

    //fix bug of color change when running screenrecord
    if (DstHandle->format != Context->fbhandle.format)
    {
        ALOGV("Force dst format to fb format: %x => %x", DstHandle->format, Context->fbhandle.format);
        DstHandle->format = Context->fbhandle.format;
    }

    hwcONERROR(
        hwcGetBufFormat(DstHandle,
                     &dstFormat
                    ));

/*    if(Context->composer_mode  == HWC_RGA_TRSM_VOP
        || Context->composer_mode  == HWC_RGA_TRSM_GPU_VOP)
    {
        //ALOGD("force dst yuv");
        //dstFormat = RK_FORMAT_YCbCr_420_SP;
    }*/    


   // ALOGD("fmt = %d",dstFormat);        
    /* <<< End surface information. */

    /* Setup transform */
    //Src->transform = HWC_TRANSFORM_FLIP_H;

    {
        srcRect.left    = SrcRect->left;
        srcRect.top     = SrcRect->top;
        srcRect.right   = SrcRect->right;
        srcRect.bottom  = SrcRect->bottom;
        
        dstRect.left   = DstRect->left;
        dstRect.top    = DstRect->top;
        dstRect.right  = DstRect->right;
        dstRect.bottom = DstRect->bottom;
    }


    clip.xmin = 0;
    clip.xmax = dstWidth - 1;
    clip.ymin = 0;
    clip.ymax = dstHeight - 1;



#if !ONLY_USE_FB_BUFFERS
//        dstFd = (unsigned int)(Context->membk_fds[Context->membk_index]);
//#else
        dstFd = (unsigned int)(DstHandle->share_fd);
#endif
 	//ALOGD("mem_type=%d", srchnd->type);
    /*if (srchnd->type == 1)
    {
        RGA_set_src_vir_info(&Rga_Request, 0, 0, 0, srcStride, srcHeight, srcFormat, 0);
        RGA_set_dst_vir_info(&Rga_Request,  0, 0, 0, DstHandle->stride, dstHeight, &clip, dstFormat, 0);
        rga_set_fds_offsets(&Rga_Request, srchnd->share_fd, dstFd, 0, 0);
    }
    else
    if(handle->usage & GRALLOC_USAGE_PROTECTED)
    {
        RGA_set_src_vir_info(&Rga_Request, handle->phy_addr, handle->phy_addr +(handle->width * handle->height ),  \
                                 handle->phy_addr +(handle->width * handle->height ), srcStride, srcHeight, srcFormat, 0);
        RGA_set_dst_vir_info(&Rga_Request, 0, 0,  0, DstHandle->stride, dstHeight, &clip, dstFormat, 0);
        rga_set_fds_offsets(&Rga_Request, 0, dstFd, 0, 0);
    }
    else*/
    {
        RGA_set_src_vir_info(&Rga_Request, srchnd->share_fd, 0,  0, srcStride, srcHeight, srcFormat, 0);
        RGA_set_dst_vir_info(&Rga_Request, dstFd, 0,  0, DstHandle->stride, dstHeight, &clip, dstFormat, 0);
        //rga_set_fds_offsets(&Rga_Request, srchnd->share_fd, dstFd, 0, 0);        
    }
    LOGD("RGA src:fd=%d,base=%p,src_vir_w = %d, src_vir_h = %d,srcLogical=%x,srcFormat=%d", srchnd->share_fd, srchnd->base, \
         srcStride, srcHeight, srcLogical, srcFormat);
    LOGD("RGA dst:fd=%d,offset=%d,base=%p,dst_vir_w = %d, dst_vir_h = %d,dstLogical=%x,dstPhysical=%x,dstFormat=%d", dstFd, DstHandle->offset, DstHandle->base, \
         DstHandle->stride, dstHeight, dstLogical, dstPhysical, dstFormat);

    mmu_en = 0;

    LOGD("%s(%d):  "
         "source=%p srcPhysical=%x,[%dx%d](stride=%d,format=%d) "
         "dest=%p [%dx%d](stride=%d,format=%d) "
         "rects=%d",
         __FUNCTION__,
         __LINE__,
         srchnd,
         srcPhysical,
         srcWidth,
         srcHeight,
         srcStride,
         srcFormat,
         DstHandle,
         dstWidth,
         dstHeight,
         dstStride,
         dstFormat,
         Region->numRects);

    /* Get plane alpha. */
    planeAlpha = Src->blending >> 16;

     //  LOGD(" planeAlpha=%x,Src->blending=%x,name=%s", planeAlpha, Src->blending,Src->LayerName);

    if(index > 0)    // bottom layer donnt need alpah
    {
        /* Setup blending. */
        switch ((Src->blending & 0xFFFF))
        {
            case HWC_BLENDING_PREMULT:
                perpixelAlpha = _HasAlpha(srcFormat);  // only copy

                /* Setup alpha blending. */
                if (perpixelAlpha && planeAlpha < 255)
                {

                    RGA_set_alpha_en_info(&Rga_Request, 1, 2, planeAlpha , 1, 9, 0);
                }
                else if (perpixelAlpha)
                {
                    /* Perpixel alpha only. */
                    RGA_set_alpha_en_info(&Rga_Request, 1, 1, 0, 1, 3, 0);

                }
                else /* if (planeAlpha < 255) */
                {
                    /* Plane alpha only. */
                    RGA_set_alpha_en_info(&Rga_Request, 1, 0, planeAlpha , 0, 0, 0);

                }
                break;

            case HWC_BLENDING_COVERAGE:
                /* SRC_ALPHA / ONE_MINUS_SRC_ALPHA. */
                /* Cs' = Cs * As
                 * As' = As
                 * C = Cs' + Cd * (1 - As)
                 * A = As' + Ad * (1 - As) */
                perpixelAlpha = _HasAlpha(srcFormat);
                LOGI("perpixelAlpha=%d,planeAlpha=%d,line=%d ", perpixelAlpha, planeAlpha, __LINE__);
                /* Setup alpha blending. */
                if (perpixelAlpha && planeAlpha < 255)
                {

                    RGA_set_alpha_en_info(&Rga_Request, 1, 2, planeAlpha , 0, 0, 0);
                }
                else if (perpixelAlpha)
                {
                    /* Perpixel alpha only. */
                    RGA_set_alpha_en_info(&Rga_Request, 1, 1, 0, 0, 0, 0);

                }
                else /* if (planeAlpha < 255) */
                {
                    /* Plane alpha only. */
                    RGA_set_alpha_en_info(&Rga_Request, 1, 0, planeAlpha , 0, 0, 0);

                }
                break;

            case HWC_BLENDING_NONE:
            default:
                /* Tips: BLENDING_NONE is non-zero value, handle zero value as
                 * BLENDING_NONE. */
                /* C = Cs
                 * A = As */
                break;
        }
    }    
    /* Check yuv format. */
    yuvFormat = (srcFormat >= RK_FORMAT_YCbCr_422_SP && srcFormat <= RK_FORMAT_YCbCr_420_P);

    /* Check stretching. */

    if(yuvFormat)  
    {
        dstRect.left -= dstRect.left%2;
        dstRect.top -= dstRect.top%2;            
        dstRect.right -= dstRect.right%2;
        dstRect.bottom -= dstRect.bottom%2;
    }
    
    if ((Src->transform == HWC_TRANSFORM_ROT_90) || (Src->transform == HWC_TRANSFORM_ROT_270))
    {
        hfactor = (float)(srcRect.bottom - srcRect.top)
                  / (dstRect.right - dstRect.left);

        vfactor = (float)(srcRect.right - srcRect.left)
                  / (dstRect.bottom - dstRect.top);

    }
    else
    {
        hfactor = (float)(srcRect.right - srcRect.left)
                  / (dstRect.right - dstRect.left);

        vfactor = (float)(srcRect.bottom - srcRect.top)
                  / (dstRect.bottom - dstRect.top);

    }

    if (hfactor < 1 || vfactor < 1)  // scale up use bicubic
        scale_mode = 2;

    stretch = (hfactor != 1.0f) || (vfactor != 1.0f);

    if (stretch && (srcFormat == RK_FORMAT_RGBA_8888 || srcFormat == RK_FORMAT_BGRA_8888))
    {
        scale_mode = 0;     //  force change scale_mode to 0 ,for rga not support
    }


    LOGV("%s(%d):,SRCname=%s,VirSrcRect=[%d,%d,%d,%d] => VirDstRect=[%d,%d,%d,%d] "
         "stretch=%d hfactor=%f vfactor=%f",
         __FUNCTION__,
         __LINE__,
         Src->LayerName,
         srcRect.left,
         srcRect.top,
         srcRect.right,
         srcRect.bottom,
         dstRect.left,
         dstRect.top,
         dstRect.right,
         dstRect.bottom,
         stretch,
         hfactor,
         vfactor);


    /* Go through all visible regions (clip rectangles?). */
    do
    {
        /* 16 rectangles a batch. */
        unsigned int m;
        hwcRECT srcRects[16];
        hwcRECT dstRects[16];
        hwc_rect_t const * rects = Region->rects;

        for (m = 0; n < (unsigned int) Region->numRects && m < 16; n++)
        {
            /* Hardware will mirror in dest rect and blit area in clipped rect.
             * But we need mirror area in clippred rect.
             * NOTE: Now we always set dstRect to clip area. */

            /* Intersect clip with dest. */
            if(Src->dospecialflag)
            {
                dstRects[m].left   = dstRect.left;   
                dstRects[m].top    = dstRect.top;
                dstRects[m].right  = dstRect.right;
                dstRects[m].bottom = dstRect.bottom;    
                ALOGD("@spicial [%d,%d,%d,%d]",dstRects[m].left,dstRects[m].top,dstRects[m].right,dstRects[m].bottom);
            }
            else
            {
                dstRects[m].left   = hwcMAX(dstRect.left,   rects[n].left);
                dstRects[m].top    = hwcMAX(dstRect.top,    rects[n].top);
                dstRects[m].right  = hwcMIN(dstRect.right,  rects[n].right);
                dstRects[m].right  = hwcMIN(dstRects[m].right,(signed int)(dstWidth));
                dstRects[m].bottom = hwcMIN(dstRect.bottom, rects[n].bottom);
            }    
            if (dstRects[m].top < 0) // @ buyudaren grame dstRects[m].top < 0,bottom is height ,so do this
            {
                dstRects[m].top = dstRects[m].top + dstHeight;
                dstRects[m].bottom = dstRects[m].bottom + dstRects[m].top;
            }

            /* Check dest area. */
            if ((dstRects[m].right <= dstRects[m].left)
                    || (dstRects[m].bottom <= dstRects[m].top)
               )
            {
                /* Skip this empty rectangle. */
                LOGI("%s(%d):  skip empty rectangle [%d,%d,%d,%d]",
                     __FUNCTION__,
                     __LINE__,
                     dstRects[m].left,
                     dstRects[m].top,
                     dstRects[m].right,
                     dstRects[m].bottom);

                continue;
            }
            if(yuvFormat)  
            {
                dstRects[m].left -= dstRects[m].left%2;
                dstRects[m].top -= dstRects[m].top%2;            
                dstRects[m].right -= dstRects[m].right%2;
                dstRects[m].bottom -= dstRects[m].bottom%2;

            }
            
            LOGI("%s(%d): Region rect[%d]:  [%d,%d,%d,%d]",
                 __FUNCTION__,
                 __LINE__,
                 m,
                 rects[n].left,
                 rects[n].top,
                 rects[n].right,
                 rects[n].bottom);

            /* Advance to next rectangle. */
            m++;
            if(Src->dospecialflag)
            {
                 n = (unsigned int) Region->numRects;
            }     
        }

        /* Try next group if no rects. */
        if (m == 0)
        {
            hwcONERROR(hwcSTATUS_INVALID_ARGUMENT);
        }

        
        for (unsigned int i = 0; i < m; i++)
        {
            switch (Src->transform)
            {

                case 0:
                    RotateMode = stretch;
                    Xoffset = dstRects[i].left;
                    Yoffset = dstRects[i].top;
                    WidthAct = dstRects[i].right - dstRects[i].left ;
                    HeightAct = dstRects[i].bottom - dstRects[i].top ;
                    /* calculate srcRects,dstRect is virtual rect,dstRects[i] is actual rect,
                       hfactor and vfactor are scale factor.
                    */
                    srcRects[i].left   = srcRect.left
                                         - (int)((dstRect.left   - dstRects[i].left)   * hfactor);

                    srcRects[i].top    = srcRect.top
                                         - (int)((dstRect.top    - dstRects[i].top)    * vfactor);

                    srcRects[i].right  = srcRect.right
                                         - (int)((dstRect.right  - dstRects[i].right)  * hfactor);

                    srcRects[i].bottom = srcRect.bottom
                                         - (int)((dstRect.bottom - dstRects[i].bottom) * vfactor);

                    break;
                case HWC_TRANSFORM_FLIP_H:
                    RotateMode      = 2;
                    Xoffset = dstRects[i].left;
                    Yoffset = dstRects[i].top;
                    WidthAct = dstRects[i].right - dstRects[i].left ;
                    HeightAct = dstRects[i].bottom - dstRects[i].top ;
                    srcRects[i].left   = srcRect.left
                                         - (int)((dstRect.left   - dstRects[i].left)   * hfactor);

                    srcRects[i].top    = srcRect.top
                                         - (int)((dstRect.top    - dstRects[i].top)    * vfactor);

                    srcRects[i].right  = srcRect.right
                                         - (int)((dstRect.right  - dstRects[i].right)  * hfactor);

                    srcRects[i].bottom = srcRect.bottom
                                         - (int)((dstRect.bottom - dstRects[i].bottom) * vfactor);

                    break;
                case HWC_TRANSFORM_FLIP_V:
                    RotateMode      = 3;
                    Xoffset = dstRects[i].left;
                    Yoffset = dstRects[i].top;
                    WidthAct = dstRects[i].right - dstRects[i].left ;
                    HeightAct = dstRects[i].bottom - dstRects[i].top ;
                    srcRects[i].left   = srcRect.left
                                         - (int)((dstRect.left   - dstRects[i].left)   * hfactor);

                    srcRects[i].top    = srcRect.top
                                         - (int)((dstRect.top    - dstRects[i].top)    * vfactor);

                    srcRects[i].right  = srcRect.right
                                         - (int)((dstRect.right  - dstRects[i].right)  * hfactor);

                    srcRects[i].bottom = srcRect.bottom
                                         - (int)((dstRect.bottom - dstRects[i].bottom) * vfactor);

                    break;

                case HWC_TRANSFORM_ROT_90:
                    RotateMode      = 1;
                    Rotation    = 90;
                    Xoffset = dstRects[i].right - 1;
                    Yoffset = dstRects[i].top ;
                    WidthAct = dstRects[i].bottom - dstRects[i].top ;
                    HeightAct = dstRects[i].right - dstRects[i].left ;

                    srcRects[i].left   = srcRect.top
                                         - (int)((dstRect.top    - dstRects[i].top)    * vfactor);

                    srcRects[i].top    =  srcRect.left
                                          - (int)((dstRect.left   - dstRects[i].left)   * hfactor);

                    srcRects[i].right  = srcRects[i].left
                                         + (int)((dstRects[i].bottom - dstRects[i].top) * vfactor);

                    srcRects[i].bottom = srcRects[i].top
                                         + (int)((dstRects[i].right  - dstRects[i].left) * hfactor);

                    break;

                case HWC_TRANSFORM_ROT_180:

                    RotateMode      = 1;
                    Rotation    = 180;
                    Xoffset = dstRects[i].right - 1;
                    Yoffset = dstRects[i].bottom - 1;
                    WidthAct = dstRects[i].right - dstRects[i].left;
                    HeightAct = dstRects[i].bottom - dstRects[i].top;
                    //srcRects[i].left   = srcRect.left +  (srcRect.right - srcRect.left)
                    //- ((dstRects[i].right - dstRects[i].left) * hfactor)
                    //+ ((dstRect.left   - dstRects[i].left)   * hfactor);

                    srcRects[i].left   = srcRect.left + (srcRect.right - srcRect.left)
                                         - ((dstRects[i].right - dstRect.left)   * hfactor);

                    srcRects[i].top    = srcRect.top
                                         - (int)((dstRect.top    - dstRects[i].top)    * vfactor);

                    srcRects[i].right  = srcRects[i].left
                                         + (int)((dstRects[i].right  - dstRects[i].left) * hfactor);

                    srcRects[i].bottom = srcRects[i].top
                                         + (int)((dstRects[i].bottom - dstRects[i].top) * vfactor);

                    break;

                case HWC_TRANSFORM_ROT_270:
                
                    RotateMode      = 1;
                    Rotation        = 270;
                    Xoffset = dstRects[i].left;
                    Yoffset = dstRects[i].bottom - 1;
                    WidthAct = dstRects[i].bottom - dstRects[i].top ;
                    HeightAct = dstRects[i].right - dstRects[i].left ;

                    //srcRects[i].left   = srcRect.top +  (srcRect.right - srcRect.left)
                    //- ((dstRects[i].bottom - dstRects[i].top) * vfactor)
                    //+ ((dstRect.top    - dstRects[i].top)    * vfactor);

                    srcRects[i].left   = srcRect.top + (srcRect.right - srcRect.left)
                                         - ((dstRects[i].bottom - dstRect.top)    * vfactor);

                    srcRects[i].top    =  srcRect.left
                                          - (int)((dstRect.left   - dstRects[i].left)   * hfactor);

                    srcRects[i].right  = srcRects[i].left
                                         + (int)((dstRects[i].bottom - dstRects[i].top) * vfactor);

                    srcRects[i].bottom = srcRects[i].top
                                         + (int)((dstRects[i].right  - dstRects[i].left) * hfactor);

                    break;
                default:
                    hwcONERROR(hwcSTATUS_INVALID_ARGUMENT);
                    break;
            }

            srcRects[i].left = hwcMAX(srcRects[i].left,0);
            srcRects[i].top = hwcMAX(srcRects[i].top,0); 
            if (yuvFormat)
            {
                srcRects[i].left -=   srcRects[i].left % 2;
                srcRects[i].top  -=  srcRects[i].top % 2;
                srcRects[i].right  -=  srcRects[i].right % 2;
                srcRects[i].bottom -=  srcRects[i].bottom % 2;
            }

            LOGD("%s(%d): Adjust ActSrcRect[%d]=[%d,%d,%d,%d] => ActDstRect=[%d,%d,%d,%d]",
                 __FUNCTION__,
                 __LINE__,
                 i,
                 srcRects[i].left,
                 srcRects[i].top,
                 srcRects[i].right,
                 srcRects[i].bottom,
                 dstRects[i].left,
                 dstRects[i].top,
                 dstRects[i].right,
                 dstRects[i].bottom
                );

            LOGD("RGA src[%d] Xoffset=%d,Yoffset=%d,WidthAct=%d,HeightAct= %d",
                 i,
                 srcRects[i].left,
                 srcRects[i].top,
                 srcRects[i].right -  srcRects[i].left,
                 srcRects[i].bottom - srcRects[i].top);
            LOGD("RGA dst[%d] Xoffset=%d,Yoffset=%d,WidthAct=%d,HeightAct=%d,transform =%d,RotateMode=%d,Rotation=%d",
                 i,
                 Xoffset,
                 Yoffset,
                 WidthAct,
                 HeightAct,
                 Src->transform,
                 RotateMode,
                 Rotation);
            RGA_set_bitblt_mode(&Rga_Request, scale_mode, RotateMode, Rotation, dither_en, 0, 0);
            RGA_set_src_act_info(&Rga_Request, srcRects[i].right -  srcRects[i].left, srcRects[i].bottom - srcRects[i].top,  srcRects[i].left, srcRects[i].top);
            RGA_set_dst_act_info(&Rga_Request, WidthAct, HeightAct, Xoffset, Yoffset);

			//ALOGD("src_type=%d,dst_type=%d,index=%d",
			//	srchnd->type,Context->membk_type[Context->membk_index],Context->membk_index);
            if (srchnd->type == 1 || DstHandle->type/*|| Context->membk_type[Context->membk_index] == 1*/)
            {
                RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);
                Rga_Request.mmu_info.mmu_flag |= (1 << 31) | (DstHandle->type << 10) | (srchnd->type << 8);
				ALOGV("rga_flag=%x",Rga_Request.mmu_info.mmu_flag); 
            }
/*
            if(FceMrga->use_fence)
            {
                //RGA_set_src_fence_flag(&Rga_Request,Src->acquireFenceFd,true);
                RGA_set_src_fence_flag(&Rga_Request,0,false);
                ALOGV("set src_fd=%d,name=%s,is_last=%d",Src->acquireFenceFd,Src->LayerName,FceMrga->is_last);
                // ALOGD("[%d,%d,%d,%d]",n,Region->numRects,i,m);
                if((n >= (unsigned int) Region->numRects)
                    && (i== m-1))
                {
                    ALOGV("set dstoutfence flag=true");
                    RGA_set_dst_fence_flag(&Rga_Request,true);
                }
            }
            #if VIDEO_USE_PPROT
            if(!hwcppCheck(&Rga_Request,Context->composer_mode,yuvFormat,Src->transform, \
                &srcRects[i],&dstRects[i]) && !Context->wfddev)
            {
                Context->Is_bypp = true;
                Rga_Request.src.x_offset = srcRect.left;
                Rga_Request.src.y_offset = srcRect.top;
                retv = hwcDobypp(&Rga_Request,dstRects[i].left, dstRects[i].top,Src->transform);
                //memset((void*)(Context->membk_base[Context->membk_index]),0x80,1280*800*2);
                if( retv != 0)
                {
                    LOGE("%s: Adjust ActSrcRect[%d]=[%d,%d,%d,%d] => ActDstRect=[%d,%d,%d,%d]",
                         Src->LayerName,
                         i,
                         srcRects[i].left,
                         srcRects[i].top,
                         srcRects[i].right,
                         srcRects[i].bottom,
                         dstRects[i].left,
                         dstRects[i].top,
                         dstRects[i].right,
                         dstRects[i].bottom
                        );
                
                }
                return hwcSTATUS_OK;
            }
            #endif
*/
            retv = ioctl(Context->engine_fd, RGA_BLIT_ASYNC, &Rga_Request);
            if( retv != 0)
            {
                LOGE("RGA ASYNC err=%d,name=%s",retv, Src->LayerName);
                LOGE("%s(%d)[i=%d]:  RGA_BLIT_ASYNC Failed Region->numRects=%d ,n=%d,m=%d", __FUNCTION__, __LINE__, i, Region->numRects, n, m);
                LOGE("RGA src:fd=%d,base=%p,src_vir_w = %d, src_vir_h = %d,srcLogical=%x,srcFormat=%d", srchnd->share_fd, srchnd->base, \
                     srcStride, srcHeight, srcLogical, srcFormat);
                LOGE("RGA dst:fd=%d,offset=%d,base=%p,dst_vir_w = %d, dst_vir_h = %d,dstLogical=%x,dstPhysical=%x,dstFormat=%d", dstFd, DstHandle->offset, DstHandle->base, \
                     dstWidth, dstHeight, dstLogical, dstPhysical, dstFormat);
                
                LOGE("%s(%d): Adjust ActSrcRect[%d]=[%d,%d,%d,%d] => ActDstRect=[%d,%d,%d,%d]",
                     __FUNCTION__,
                     __LINE__,
                     i,
                     srcRects[i].left,
                     srcRects[i].top,
                     srcRects[i].right,
                     srcRects[i].bottom,
                     dstRects[i].left,
                     dstRects[i].top,
                     dstRects[i].right,
                     dstRects[i].bottom
                    );

                LOGE("RGA src[%d] Xoffset=%d,Yoffset=%d,WidthAct=%d,HeightAct= %d",
                     i,
                     srcRects[i].left,
                     srcRects[i].top,
                     srcRects[i].right -  srcRects[i].left,
                     srcRects[i].bottom - srcRects[i].top);
                LOGE("RGA dst[%d] Xoffset=%d,Yoffset=%d,WidthAct=%d,HeightAct=%d,transform =%d,RotateMode=%d,Rotation=%d",
                     i,
                     Xoffset,
                     Yoffset,
                     WidthAct,
                     HeightAct,
                     Src->transform,
                     RotateMode,
                     Rotation);
				ALOGE("mmu_rga_flag=%x",Rga_Request.mmu_info.mmu_flag);
/*
                if(FceMrga->use_fence)
                {
                    FceMrga->rel_fd = RGA_get_dst_fence(&Rga_Request);
                    if(FceMrga->rel_fd >= 0)
                    {
                        ALOGE("RGA err force close[%d]",FceMrga->rel_fd);
                        close( FceMrga->rel_fd);
                    }    
                    FceMrga->rel_fd = -1;			
                }
*/ 
            }
/*
            if(FceMrga->use_fence)
            {
                FceMrga->rel_fd = RGA_get_dst_fence(&Rga_Request);
                Context->engine_err_cnt = Rga_Request.line_draw_info.start_point.x; // RGA  err count,
            }
*/ 
        }
    }
    while (n < (unsigned int) Region->numRects);
    LOGI(" hwcBlit end---<");

    //debug: clear fb to 0xff
#if 0
    if (1 == hwc_get_int_property("sys.setfb", "0"))
    {
        memset((void*)dstBase, 0xff, dstWidth*dstWidth);
    }
#endif

    return hwcSTATUS_OK;

OnError:
    LOGE("%s(%d):  Failed", __FUNCTION__, __LINE__);
#ifdef HWC_Layer_DEBUG
    LOGE("composer err in layer=%s,rect.left=%d,rect.top=%d,rect.right=%d,rect.bottom=%d",
         Src->LayerName, Src->sourceCrop.left, Src->sourceCrop.top, Src->sourceCrop.right, Src->sourceCrop.bottom);
#endif
    /* Error roll back. */

    /* Unlock buffer. */

    return status;
}

hwcSTATUS
hwcDim(
    IN hwcContext * Context,
    IN hwc_layer_1_t * Src,
    IN struct private_handle_t * DstHandle,
    IN hwc_rect_t * DstRect,
    IN hwc_region_t * Region
)
{
    hwcSTATUS status = hwcSTATUS_OK;

#if 1
    void *     dstLogical;
    unsigned int      dstFd = ~0, dstBase = ~0;
    void *     dstInfo;
    unsigned int      dstStride;
    unsigned int      dstWidth;
    unsigned int      dstHeight;
    RgaSURF_FORMAT dstFormat;

    unsigned int        planeAlpha;
    unsigned int        n = 0;
    struct rga_req  Rga_Request;
    RECT clip;
    unsigned int      Xoffset;
    unsigned int      Yoffset;
    unsigned int      WidthAct;
    unsigned int      HeightAct;
    struct private_handle_t* srchnd = (struct private_handle_t *) Src->handle;
    memset(&Rga_Request, 0x0, sizeof(Rga_Request));

    /* >>> Begin dest surface information. */
    LOGV("hwcDim start--->");
    hwcONERROR(
        hwcGetBufferInfo(Context,
                      DstHandle,
                      &dstLogical,
                      &dstFd,
                      &dstWidth,
                      &dstHeight,
                      &dstStride,
                      &dstInfo));
    hwcONERROR(
        hwcGetBufFormat(DstHandle,
                     &dstFormat
                    ));
    /* <<< End surface information. */

    /* Get planeAlpha. */
    planeAlpha = Src->blending >> 16;

    LOGI("%s(%d):  planeAlpha=%d", __FUNCTION__, __LINE__, planeAlpha);

    clip.xmin = 0;
    clip.xmax = dstWidth - 1;
    clip.ymin = 0;
    clip.ymax = dstHeight - 1;

/*
#if !ONLY_USE_FB_BUFFERS
        dstFd = (unsigned int)(Context->membk_fds[Context->membk_index]);
#else
        dstFd = (unsigned int)(DstHandle->share_fd);
#endif
*/
    dstFd = (unsigned int)(DstHandle->share_fd);
    
 	//ALOGD("mem_type=%d", srchnd->type);
    /*if (srchnd->type == 1)
    {
        RGA_set_src_vir_info(&Rga_Request, 0, 0, 0, srcStride, srcHeight, srcFormat, 0);
        RGA_set_dst_vir_info(&Rga_Request,  0, 0, 0, DstHandle->stride, dstHeight, &clip, dstFormat, 0);
        rga_set_fds_offsets(&Rga_Request, srchnd->share_fd, dstFd, 0, 0);
    }
    else*/
    {
        RGA_set_dst_vir_info(&Rga_Request, 0, 0,  0, DstHandle->stride, dstHeight, &clip, dstFormat, 0);
        //rga_set_fds_offsets(&Rga_Request, srchnd->share_fd, dstFd, 0, 0);
    }

    /* Go through all visible regions (clip rectangles?). */
    do
    {
        /* 16 rectangles a batch. */
        unsigned int m;
        hwcRECT dstRects[16];
        COLOR_FILL FillColor ;
        memset(&FillColor , 0x0, sizeof(COLOR_FILL));
        hwc_rect_t const * rects = Region->rects;

        for (m = 0; n < (unsigned int) Region->numRects && m < 16; n++)
        {
            /* Intersect clip with dest. */
            dstRects[m].left   = hwcMAX(DstRect->left,   rects[n].left);
            dstRects[m].top    = hwcMAX(DstRect->top,    rects[n].top);
            dstRects[m].right  = hwcMIN(DstRect->right,  rects[n].right);
            dstRects[m].bottom = hwcMIN(DstRect->bottom, rects[n].bottom);


            /* Check dest area. */
            if ((dstRects[m].right <= dstRects[m].left)
                    || (dstRects[m].bottom <= dstRects[m].top)
               )
            {
                /* Skip this empty rectangle. */
                LOGV("%s(%d):  skip empty rectangle [%d,%d,%d,%d]",
                     __FUNCTION__,
                     __LINE__,
                     dstRects[m].left,
                     dstRects[m].top,
                     dstRects[m].right,
                     dstRects[m].bottom);

                continue;
            }

            LOGV("%s(%d):  rect[%d]: [%d,%d,%d,%d] by [%d,%d,%d,%d]",
                 __FUNCTION__,
                 __LINE__,
                 m,
                 dstRects[m].left,
                 dstRects[m].top,
                 dstRects[m].right,
                 dstRects[m].bottom,
                 rects[n].left,
                 rects[n].top,
                 rects[n].right,
                 rects[n].bottom);

            /* Advance to next rectangle. */
            m++;
        }

        /* Try next group if no rects. */
        if (m == 0)
        {
            hwcONERROR(hwcSTATUS_INVALID_ARGUMENT);
        }

        if (planeAlpha == 255)
        {

            /* Perform a Clear. */
            RGA_set_color_fill_mode(&Rga_Request, &FillColor, 0, 0, 0x00, 0, 0, 0, 0, 0);

        }
        else
        {

            RGA_set_alpha_en_info(&Rga_Request, 1, 0, planeAlpha , 0, 0, 0);
            RGA_set_color_fill_mode(&Rga_Request, &FillColor, 0, 0, 0x00, 0, 0, 0, 0, 0);


        }
        for (unsigned int i = 0; i < n; i++)
        {
            Xoffset = dstRects[i].left;
            Yoffset = dstRects[i].top;
            WidthAct = dstRects[i].right - dstRects[i].left ;
            HeightAct = dstRects[i].bottom - dstRects[i].top ;

            LOGI("RGA dst[%d] Xoffset=%d,Yoffset=%d,WidthAct=%d,HeightAct=%d",
                 i,
                 Xoffset,
                 Yoffset,
                 WidthAct,
                 HeightAct
                );
            RGA_set_dst_act_info(&Rga_Request, WidthAct, HeightAct, Xoffset, Yoffset);

            RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);

            if (srchnd->type == 1 /*|| Context->membk_type[Context->membk_index] == 1*/)
            {
                RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);
                Rga_Request.mmu_info.mmu_flag |= (1 << 31) | (DstHandle->type << 10) | (srchnd->type << 8);
				ALOGV("rga_flag=%x",Rga_Request.mmu_info.mmu_flag); 
            }
            if (ioctl(Context->engine_fd, RGA_BLIT_ASYNC, &Rga_Request) != 0)
            {
                LOGE("%s(%d)[i=%d]:  RGA_BLIT_ASYNC Failed", __FUNCTION__, __LINE__, i);
            }
        }
    }
    while (n < (unsigned int) Region->numRects);

    /* Unlock buffer. */

    LOGI("hwcDim end--->");

    return hwcSTATUS_OK;

OnError:
    LOGE("%s(%d):  Failed", __FUNCTION__, __LINE__);
    /* Error roll back. */

#endif
    return status;
}

hwcSTATUS
hwcClear(
    IN hwcContext * Context,
    IN unsigned int Color,
    IN hwc_layer_1_t * Src,
    IN struct private_handle_t * DstHandle,
    IN hwc_rect_t * DstRect,
    IN hwc_region_t * Region
)
{
    hwcSTATUS status = hwcSTATUS_OK;
#if 1
    void *     dstLogical;
    unsigned int      dstFd = ~0, dstBase = ~0;
    void *     dstInfo;
    unsigned int      dstStride;
    unsigned int      dstWidth;
    unsigned int      dstHeight;
    RgaSURF_FORMAT dstFormat;
    unsigned int        n = 0;

    struct rga_req  Rga_Request;
    RECT clip;
    unsigned int      Xoffset;
    unsigned int      Yoffset;
    unsigned int      WidthAct;
    unsigned int      HeightAct;
    COLOR_FILL FillColor ;
    struct private_handle_t* srchnd = (struct private_handle_t *) Src->handle;

    memset(&FillColor , 0x0, sizeof(COLOR_FILL));

    LOGV("%s(%d):  color=0x%.8x", __FUNCTION__, __LINE__, Color);
/*
    if(Context->composer_mode  == HWC_RGA_TRSM_VOP
        || Context->composer_mode  == HWC_RGA_TRSM_GPU_VOP)
    {
       return hwcSTATUS_OK;
       //ALOGD("force dst yuv");
    }
*/
    memset(&Rga_Request, 0x0, sizeof(Rga_Request));

    /* >>> Begin dest surface information. */

    hwcONERROR(
        hwcGetBufferInfo(Context,
                      DstHandle,
                      &dstLogical,
                      &dstFd,
                      &dstWidth,
                      &dstHeight,
                      &dstStride,
                      &dstInfo));
    hwcONERROR(
        hwcGetBufFormat(DstHandle,
                     &dstFormat
                    ));
    /* <<< End surface information. */

    /* Get planeAlpha. */


    clip.xmin = 0;
    clip.xmax = dstWidth - 1;
    clip.ymin = 0;
    clip.ymax = dstHeight - 1;

/*
#if !ONLY_USE_FB_BUFFERS
    dstFd = (unsigned int)(Context->membk_fds[Context->membk_index]);
#else
    dstFd = (unsigned int)(DstHandle->share_fd);
#endif
*/
    dstFd = (unsigned int)(DstHandle->share_fd);
    
 	//ALOGD("mem_type=%d", srchnd->type);
    /*if (srchnd->type == 1)
    {
        RGA_set_src_vir_info(&Rga_Request, 0, 0, 0, srcStride, srcHeight, srcFormat, 0);
        RGA_set_dst_vir_info(&Rga_Request,  0, 0, 0, DstHandle->stride, dstHeight, &clip, dstFormat, 0);
        rga_set_fds_offsets(&Rga_Request, srchnd->share_fd, dstFd, 0, 0);
    }
    else*/
    {
        RGA_set_dst_vir_info(&Rga_Request, 0, 0,  0, DstHandle->stride, dstHeight, &clip, dstFormat, 0);
        //rga_set_fds_offsets(&Rga_Request, srchnd->share_fd, dstFd, 0, 0);
    }

    /* Go through all visible regions (clip rectangles?). */
    do
    {
        /* 16 rectangles a batch. */
        unsigned int m;
        hwcRECT dstRects[16];

        hwc_rect_t const * rects = Region->rects;

        for (m = 0; n < (unsigned int) Region->numRects && m < 16; n++)
        {
            /* Intersect clip with dest. */
            dstRects[m].left   = hwcMAX(DstRect->left,   rects[n].left);
            dstRects[m].top    = hwcMAX(DstRect->top,    rects[n].top);
            dstRects[m].right  = hwcMIN(DstRect->right,  rects[n].right);
            dstRects[m].bottom = hwcMIN(DstRect->bottom, rects[n].bottom);


            /* Check dest area. */
            if ((dstRects[m].right <= dstRects[m].left)
                    || (dstRects[m].bottom <= dstRects[m].top)
               )
            {
                /* Skip this empty rectangle. */
                LOGV("%s(%d):  skip empty rectangle [%d,%d,%d,%d]",
                     __FUNCTION__,
                     __LINE__,
                     dstRects[m].left,
                     dstRects[m].top,
                     dstRects[m].right,
                     dstRects[m].bottom);

                continue;
            }

            LOGV("%s(%d):  rect[%d]: [%d,%d,%d,%d] by [%d,%d,%d,%d]",
                 __FUNCTION__,
                 __LINE__,
                 m,
                 dstRects[m].left,
                 dstRects[m].top,
                 dstRects[m].right,
                 dstRects[m].bottom,
                 rects[n].left,
                 rects[n].top,
                 rects[n].right,
                 rects[n].bottom);

            /* Advance to next rectangle. */
            m++;
        }

        /* Try next group if no rects. */
        if (m == 0)
        {
            hwcONERROR(hwcSTATUS_INVALID_ARGUMENT);
        }
        RGA_set_color_fill_mode(&Rga_Request, &FillColor, 0, 0, Color, 0, 0, 0, 0, 0);

        for (unsigned int i = 0; i < n; i++)
        {
            Xoffset = dstRects[i].left;
            Yoffset = dstRects[i].top;
            WidthAct = dstRects[i].right - dstRects[i].left ;
            HeightAct = dstRects[i].bottom - dstRects[i].top ;

            LOGI("RGA dst[%d] Xoffset=%d,Yoffset=%d,WidthAct=%d,HeightAct=%d",
                 i,
                 Xoffset,
                 Yoffset,
                 WidthAct,
                 HeightAct
                );
            RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);

            if (srchnd->type == 1 /*|| Context->membk_type[Context->membk_index] == 1*/)
            {
                RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);
                Rga_Request.mmu_info.mmu_flag |= (1 << 31) | (DstHandle->type << 10) | (srchnd->type << 8);
				ALOGV("rga_flag=%x",Rga_Request.mmu_info.mmu_flag); 
            }

            RGA_set_dst_act_info(&Rga_Request, WidthAct, HeightAct, Xoffset, Yoffset);
            if (ioctl(Context->engine_fd, RGA_BLIT_ASYNC, &Rga_Request) != 0)
            {
                LOGE("%s(%d)[i=%d]:  RGA_BLIT_ASYNC Failed", __FUNCTION__, __LINE__, i);
            }
        }

    }
    while (n < (unsigned int) Region->numRects);

    /* Unlock buffer. */

    return hwcSTATUS_OK;

OnError:
    LOGE("%s(%d):  Failed", __FUNCTION__, __LINE__);
    /* Error roll back. */

#endif
    return status;
}
#endif




