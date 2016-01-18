/*

* rockchip hwcomposer( 2D graphic acceleration unit) .

*

* Copyright (C) 2015 Rockchip Electronics Co., Ltd.

*/

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "rk_hwcomposer.h"
#include <hardware/hardware.h>

#include <sys/prctl.h>
#include <stdlib.h>
#include <errno.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <sync/sync.h>
#include <time.h>
#include <poll.h>
#include "rk_hwcomposer_hdmi.h"
#include <ui/PixelFormat.h>
#include <sys/stat.h>
//#include "hwc_ipp.h" 
#include "hwc_rga.h"
#include <linux/ion.h>
#include <ion/ion.h>
#include <linux/rockchip_ion.h>
#include <utils/Trace.h>

//primary,hotplug and virtual device context
static hwcContext * _contextAnchor = NULL;
static hwcContext * _contextAnchor1 = NULL;
static hwcContext * _contextAnchor2 = NULL;
//#define ENABLE_HDMI_APP_LANDSCAP_TO_PORTRAIT
#undef LOGV
#define LOGV(...)
static int mLogL = 0;
static int crcTable[256];
bool hdmi_noready = true;
static mix_info gmixinfo[2];
static int SkipFrameCount = 0;
//#if  (ENABLE_TRANSFORM_BY_RGA | ENABLE_LCDC_IN_NV12_TRANSFORM | USE_SPECIAL_COMPOSER)
static hwbkupmanage bkupmanage;
//#endif
#ifdef ENABLE_HDMI_APP_LANDSCAP_TO_PORTRAIT
static int bootanimFinish = 0;
#endif

static PFNEGLRENDERBUFFERMODIFYEDANDROIDPROC _eglRenderBufferModifiedANDROID;
int gwin_tab[MaxZones] = {win0,win1,win2_0,win2_1,win2_2,win2_3,win3_0,win3_1,win3_2,win3_3};


static int
hwc_blank(
            struct hwc_composer_device_1 *dev,
            int dpy,
            int blank);
static int
hwc_query(
        struct hwc_composer_device_1* dev,
        int what,
        int* value);

static int hwc_event_control(
                struct hwc_composer_device_1* dev,
                int dpy,
                int event,
                int enabled);

static int
hwc_prepare(
    hwc_composer_device_1_t * dev,
    size_t numDisplays,
    hwc_display_contents_1_t** displays
    );


static int
hwc_set(
    hwc_composer_device_1_t * dev,
    size_t numDisplays,
    hwc_display_contents_1_t  ** displays
    );

static int
hwc_device_close(
    struct hw_device_t * dev
    );

void
*hotplug_try_register(
    void *arg
    );

void
hotplug_get_resolution(
    int* w,
    int* h
    );

int 
hotplug_set_config();

int
hotplug_parse_mode(
    int *outX,
    int *outY
    );

int
hotplug_get_config(int flag);

int
hotplug_set_overscan(
    int flag
    );

int
hotplug_reset_dstposition(
    struct rk_fb_win_cfg_data * fb_info,
    int flag);

int
hotplug_set_frame(
    hwcContext * context,
    int flag);

bool
hotplug_free_dimbuffer();

int
hwc_sprite_replace(
    hwcContext * Context,
    hwc_display_contents_1_t * list);

int
hwc_repet_last();

bool
hwcPrimaryToExternalCheckConfig(
    hwcContext * ctx,
    struct rk_fb_win_cfg_data fb_info);

static unsigned int 
createCrc32(
    unsigned int crc,
    unsigned const char *buffer,
    unsigned int size);

static void 
initCrcTable(void);


int hwChangeFormatandroidL(IN int fmt)
{
	switch (fmt) 
	{
		case HAL_PIXEL_FORMAT_YCrCb_NV12:	/* YUV420---uvuvuv */
			return 0x20;                   /*android4.4 HAL_PIXEL_FORMAT_YCrCb_NV12 is 0x20*/    
			
		case HAL_PIXEL_FORMAT_YCrCb_NV12_10:	/* yuv444 */
			return 0x22;				        /*android4.4 HAL_PIXEL_FORMAT_YCrCb_NV12_10 is 0x20*/
#ifdef GPU_G6110
		case HAL_PIXEL_FORMAT_BGRX_8888:
		    return HAL_PIXEL_FORMAT_RGBA_8888;
#endif
		default:
			return fmt;
	}
}

static int
hwc_device_open(
    const struct hw_module_t * module,
    const char * name,
    struct hw_device_t ** device
    );

static struct hw_module_methods_t hwc_module_methods =
{
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM =
{
    common:
    {
        tag:           HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 2,
        id:            HWC_HARDWARE_MODULE_ID,
        name:          "Hardware Composer Module",
        author:        "Rockchip Corporation",
        methods:       &hwc_module_methods,
        dso:           NULL,
        reserved:      {0, }
    }
};

int
_HasAlpha(RgaSURF_FORMAT Format)
{
    return (Format == RK_FORMAT_RGB_565) ? false
          : (
                (Format == RK_FORMAT_RGBA_8888) ||
                (Format == RK_FORMAT_BGRA_8888)
            );
}

#if G6110_SUPPORT_FBDC
int HALPixelFormatGetCompression(int iFormat)
{
	/* Extension format. Return only the compression bits. */
	if (iFormat >= 0x100 && iFormat <= 0x1FF)
		return (iFormat & 0x70) >> 4;

	/* Upstream formats are not compressible unless they are redefined as
	 * extension formats (e.g. RGB_565, BGRA_8888).
	 */
	return HAL_FB_COMPRESSION_NONE;
}

int HALPixelFormatGetRawFormat(int iFormat)
{
	/* If the format is within the "vendor format" range, just mask out the
	 * compression bits.
	 */
	if (iFormat >= 0x100 && iFormat <= 0x1FF)
	{
		switch (iFormat & 0xF)
		{
			/* These formats will be *rendered* by the GPU and we want to
			 * support compression, but they are "upstream" formats too, so
			 * remap them.
			 */
			case HAL_PIXEL_FORMAT_RGB_565:
				return HAL_PIXEL_FORMAT_RGB_565;
			case HAL_PIXEL_FORMAT_BGRA_8888:
				return HAL_PIXEL_FORMAT_BGRA_8888;
		    case HAL_PIXEL_FORMAT_RGBA_8888:
				return HAL_PIXEL_FORMAT_RGBA_8888;
			/* Vendor format */
			default:
				return iFormat & ~0xF0;
		}
	}

	/* Upstream format */
	return iFormat;
}

int HALPixelFormatSetCompression(int iFormat, int iCompression)
{
	/* We can support compressing some "upstream" formats. If the compression
	 * is not disabled, convert the formats to our extension formats.
	 */
	if (iCompression != HAL_FB_COMPRESSION_NONE)
	{
		switch (iFormat)
		{
			case HAL_PIXEL_FORMAT_RGB_565:
			case HAL_PIXEL_FORMAT_BGRA_8888:
			case HAL_PIXEL_FORMAT_RGBA_8888:
				iFormat |= 0x100;
				break;
		}
	}

	/* Can only set compression on extension formats */
	if (iFormat < 0x100 || iFormat > 0x1FF)
		return iFormat;

	/* Clear any existing compression bits */
	iFormat &= ~0x70;

	/* Mask out invalid compression formats */
	switch (iCompression)
	{
		case HAL_FB_COMPRESSION_NONE:
		case HAL_FB_COMPRESSION_DIRECT_8x8:
		case HAL_FB_COMPRESSION_DIRECT_16x4:
		case HAL_FB_COMPRESSION_DIRECT_32x2:
		case HAL_FB_COMPRESSION_INDIRECT_8x8:
		case HAL_FB_COMPRESSION_INDIRECT_16x4:
		case HAL_FB_COMPRESSION_INDIRECT_4TILE_8x8:
		case HAL_FB_COMPRESSION_INDIRECT_4TILE_16x4:
			return iFormat | (iCompression << 4);
		default:
			return iFormat;
	}
}

#endif


//return property value of pcProperty
static int hwc_get_int_property(const char* pcProperty,const char* default_value)
{
    char value[PROPERTY_VALUE_MAX];
    int new_value = 0;

    if(pcProperty == NULL || default_value == NULL)
    {
        ALOGE("hwc_get_int_property: invalid param");
        return -1;
    }

    property_get(pcProperty, value, default_value);
    new_value = atoi(value);

    return new_value;
}

static int hwc_get_string_property(const char* pcProperty,const char* default_value,char* retult)
{
    if(pcProperty == NULL || default_value == NULL || retult == NULL)
    {
        ALOGE("hwc_get_string_property: invalid param");
        return -1;
    }

    property_get(pcProperty, retult, default_value);

    return 0;
}

static int LayerZoneCheck( hwc_layer_1_t * Layer , int disp)
{
	hwcContext * context = NULL;
#if HWC_EXTERNAL
	switch (disp){
		case HWC_DISPLAY_PRIMARY: 
			context = _contextAnchor;
			break;
		case HWC_DISPLAY_EXTERNAL:
			context = _contextAnchor1;
			break;
		default:
			context = _contextAnchor;
	}
#else
	context = _contextAnchor;
#endif
    hwc_region_t * Region = &(Layer->visibleRegionScreen);
    hwc_rect_t const * rects = Region->rects;
    int i;
    for (i = 0; i < (int) Region->numRects ;i++)
    {
        LOGV("checkzone=%s,[%d,%d,%d,%d]", \
            Layer->LayerName,rects[i].left,rects[i].top,rects[i].right,rects[i].bottom );
        if(rects[i].left < 0 || rects[i].top < 0
           || rects[i].right > context->fbhandle.width
           || rects[i].bottom > context->fbhandle.height)
        {
        	ALOGE("LayerZoneCheck ERROR line at %d",__LINE__);
            return -1;
        }
    }

    return 0;
}

void hwc_sync(hwc_display_contents_1_t  *list)
{
	if (list == NULL){
		return ;
	}

	for (int i=0; i< (int)list->numHwLayers; i++){
		if (list->hwLayers[i].acquireFenceFd>0){
			sync_wait(list->hwLayers[i].acquireFenceFd,3001);  // add 40ms timeout
		}
    	ALOGV("acquireFenceFd=%d,name=%s",list->hwLayers[i].acquireFenceFd,list->hwLayers[i].LayerName);
	}

}

#if 0
int rga_video_reset()
{
    if (_contextAnchor->video_hd || _contextAnchor->video_base)
    {
        ALOGV(" rga_video_reset,%x",_contextAnchor->video_hd);
        _contextAnchor->video_hd = 0;
        _contextAnchor->video_base =0;
    }

    return 0;
}
#endif

void hwc_sync_release(hwc_display_contents_1_t  *list)
{
	for (int i=0; i< (int)list->numHwLayers; i++){
		hwc_layer_1_t* layer = &list->hwLayers[i];
		if (layer == NULL){
			return ;
		}
		if (layer->acquireFenceFd>0){
			ALOGV(">>>close acquireFenceFd:%d,layername=%s",layer->acquireFenceFd,layer->LayerName);
			close(layer->acquireFenceFd);
			list->hwLayers[i].acquireFenceFd = -1;
		}
	}

	if (list->outbufAcquireFenceFd>0){
		ALOGV(">>>close outbufAcquireFenceFd:%d",list->outbufAcquireFenceFd);
		close(list->outbufAcquireFenceFd);
		list->outbufAcquireFenceFd = -1;
	}
   
}
int rga_video_copybit(struct private_handle_t *handle,int tranform,int w_valid,int h_valid,int fd_dst, int Dstfmt,int specialwin,int dpyID)
{
    struct rga_req  Rga_Request;
    RECT clip;
    unsigned char RotateMode = 0;
    int Rotation = 0;
    int SrcVirW,SrcVirH,SrcActW,SrcActH;
    int DstVirW,DstVirH,DstActW,DstActH;
    int xoffset = 0;
    int yoffset = 0;
    int   rga_fd = _contextAnchor->engine_fd;
    hwcContext * context = _contextAnchor;
    if(dpyID == HWCE){
        context = _contextAnchor1;
    }
    int index_v = context->mCurVideoIndex%MaxVideoBackBuffers;
    if (!rga_fd || !handle){
       return -1; 
    }
    if(_contextAnchor->video_fmt != HAL_PIXEL_FORMAT_YCrCb_NV12 && !specialwin){
        return -1;
    }

    if(specialwin && context->base_video_bk[index_v]==0){
        ALOGE("It hasn't direct_base for dst buffer");
    //    return -1;
    }

    if((handle->video_width <= 0 || handle->video_height <= 0 ||
       handle->video_width >= 8192 || handle->video_height >= 4096 )&&!specialwin){
        ALOGE("rga invalid w_h[%d,%d]",handle->video_width , handle->video_height);
        return -1;
    }

    //pthread_mutex_lock(&_contextAnchor->lock);
    memset(&Rga_Request, 0x0, sizeof(Rga_Request));
    clip.xmin = 0;
    clip.xmax = handle->height - 1;
    clip.ymin = 0;
    clip.ymax = handle->width - 1;
    switch (tranform){
        case HWC_TRANSFORM_ROT_90:
            RotateMode      = 1;
            Rotation    = 90;          
            SrcVirW = specialwin ? handle->stride:handle->video_width;
            SrcVirH = specialwin ? handle->height:handle->video_height;
            SrcActW = w_valid;
            SrcActH = h_valid;
            DstVirW = rkmALIGN(h_valid,8);
            DstVirH = w_valid;
            DstActW = w_valid;
            DstActH = h_valid;
            xoffset = h_valid -1;
            yoffset = 0;
            
            break;

        case HWC_TRANSFORM_ROT_180:
            RotateMode      = 1;
            Rotation    = 180;    
            SrcVirW = specialwin ? handle->stride:handle->video_width;
            SrcVirH = specialwin ? handle->height:handle->video_height;
            SrcActW = w_valid;
            SrcActH = h_valid;
            DstVirW = w_valid;
            DstVirH = h_valid;
            DstActW = w_valid;
            DstActH = h_valid;
            clip.xmin = 0;
            clip.xmax =  handle->width - 1;
            clip.ymin = 0;
            clip.ymax = handle->height - 1;
            xoffset = w_valid -1;
            yoffset = h_valid -1;
            
            break;

        case HWC_TRANSFORM_ROT_270:
            RotateMode      = 1;
            Rotation        = 270;   
            SrcVirW = specialwin ? handle->stride:handle->video_width;
            SrcVirH = specialwin ? handle->height:handle->video_height;
            SrcActW = w_valid;
            SrcActH = h_valid;
            DstVirW = rkmALIGN(h_valid,8);
            DstVirH = w_valid;
            DstActW = w_valid;
            DstActH = h_valid;
            xoffset = 0;
            yoffset = w_valid -1;
            break;
        case 0:                      
        default:
        {
            char property[PROPERTY_VALUE_MAX];
            int fmtflag = 0;
			if (property_get("sys.yuv.rgb.format", property, NULL) > 0) {
				fmtflag = atoi(property);
			}
			//if(fmtflag == 1)
               // Dstfmt = RK_FORMAT_RGB_565;
			//else 
               // Dstfmt = RK_FORMAT_RGBA_8888;
            SrcVirW = handle->video_width;
            SrcVirH = handle->video_height;
            SrcActW = handle->video_disp_width;
            SrcActH = handle->video_disp_height;
            DstVirW = handle->width;
            DstVirH = handle->height;
            DstActW = handle->video_disp_width;
            DstActH = handle->video_disp_height;
            clip.xmin = 0;
            clip.xmax =  handle->width - 1;
            clip.ymin = 0;
            clip.ymax = handle->height - 1;
            Dstfmt = RK_FORMAT_YCbCr_420_SP;
            break;
        }
    }
    int size = hwcGetBufferSizeForRga(DstActW,DstActH,Dstfmt) ;
    if(size > RLAGESIZE){
        ALOGD_IF(mLogL&HLLSEV,"now size=%d,largesize=%d,w_h_f[%d,%d,%d]",size,RLAGESIZE,DstActW,DstActH,Dstfmt);
        return -1;
    }
    ALOGD_IF(mLogL&HLLSEV,"src addr=[%x],w-h[%d,%d],act[%d,%d][f=%d]",
        specialwin ? handle->share_fd:handle->video_addr, SrcVirW, SrcVirH,SrcActW,SrcActH,specialwin ?  hwChangeRgaFormat(handle->format):RK_FORMAT_YCbCr_420_SP);
    ALOGD_IF(mLogL&HLLSEV,"dst fd=[%x],w-h[%d,%d],act[%d,%d][f=%d],rot=%d,rot_mod=%d",
        fd_dst, DstVirW, DstVirH,DstActW,DstActH,Dstfmt,Rotation,RotateMode);
    if(specialwin)  
        RGA_set_src_vir_info(&Rga_Request, handle->share_fd, 0, 0,SrcVirW, SrcVirH, hwChangeRgaFormat(handle->format), 0);    
    else
        RGA_set_src_vir_info(&Rga_Request, 0, handle->video_addr, 0,SrcVirW, SrcVirH, RK_FORMAT_YCbCr_420_SP, 0);
    RGA_set_dst_vir_info(&Rga_Request, fd_dst, 0, 0,DstVirW,DstVirH,&clip, Dstfmt, 0);    
    RGA_set_bitblt_mode(&Rga_Request, 0, RotateMode,Rotation,0,0,0);    
    RGA_set_src_act_info(&Rga_Request,SrcActW,SrcActH, 0,0);
    RGA_set_dst_act_info(&Rga_Request,DstActW,DstActH, xoffset,yoffset);

//debug: clear source data
#if 0
    memset((void*)handle->base,0x0,SrcVirW*SrcVirH*3/2);
#endif

    if( handle->type == 1 )
    {
        if( !specialwin)
        {
#if defined(__arm64__) || defined(__aarch64__)
            RGA_set_dst_vir_info(&Rga_Request, fd_dst,(unsigned long)(GPU_BASE), 0,DstVirW,DstVirH,&clip, Dstfmt, 0);
#else
            RGA_set_dst_vir_info(&Rga_Request, fd_dst,(unsigned int)(GPU_BASE), 0,DstVirW,DstVirH,&clip, Dstfmt, 0);
#endif
            ALOGW("Debugmem mmu_en fd=%d in vmalloc ,base=%p,[%dX%d],fmt=%d,src_addr=%x", fd_dst,GPU_BASE,DstVirW,DstVirH,handle->video_addr);
        }
        else
        {
            RGA_set_dst_vir_info(&Rga_Request, fd_dst,context->base_video_bk[index_v], 0,DstVirW,DstVirH,&clip, Dstfmt, 0);
            ALOGD_IF(mLogL&HLLSEV,"rga_video_copybit fd_dst=%d,base=%x,index_v=%d",fd_dst,context->base_video_bk[index_v],index_v);
        }
        RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);
        Rga_Request.mmu_info.mmu_flag |= (1<<31) | (1<<10) | (1<<8);

    }

    if(ioctl(rga_fd, RGA_BLIT_SYNC, &Rga_Request) != 0) {
        LOGE(" %s(%d) RGA_BLIT fail",__FUNCTION__, __LINE__);
        ALOGE("err src addr=[%x],w-h[%d,%d],act[%d,%d][f=%d],x_y_offset[%d,%d]",
            specialwin ? handle->share_fd:handle->video_addr, SrcVirW, SrcVirH,SrcActW,SrcActH,specialwin ?  hwChangeRgaFormat(handle->format):RK_FORMAT_YCbCr_420_SP,xoffset,yoffset);
        ALOGE("err dst fd=[%x],w-h[%d,%d],act[%d,%d][f=%d],rot=%d,rot_mod=%d",
            fd_dst, DstVirW, DstVirH,DstActW,DstActH,Dstfmt,Rotation,RotateMode);
    }


  //  pthread_mutex_unlock(&_contextAnchor->lock);

#if DUMP_AFTER_RGA_COPY_IN_GPU_CASE
    FILE * pfile = NULL;
    int srcStride = android::bytesPerPixel(handle->format);
    char layername[100];

    if(hwc_get_int_property("sys.hwc.dump_after_rga_copy","0"))
    {
        memset(layername,0,sizeof(layername));
        system("mkdir /data/dumplayer/ && chmod /data/dumplayer/ 777 ");
        sprintf(layername,"/data/dumplayer/dmlayer%d_%d_%d.bin",\
               handle->stride,handle->height,srcStride);

        pfile = fopen(layername,"wb");
        if(pfile)
        {
            fwrite((const void *)(GPU_BASE),(size_t)(3 * handle->stride*handle->height /2),1,pfile);
            fclose(pfile);
        }
    }
#endif
   
    return 0;
}



static int is_out_log( void )
{
    return hwc_get_int_property("sys.hwc.log","0");
}

int is_x_intersect(hwc_rect_t * rec,hwc_rect_t * rec2)
{
    if(rec2->top == rec->top)
        return 1;
    else if(rec2->top < rec->top)
    {
        if(rec2->bottom > rec->top)
            return 1;
        else
            return 0;
    }
    else
    {
        if(rec->bottom > rec2->top  )
            return 1;
        else
            return 0;        
    }
    return 0;
}
int is_zone_combine(ZoneInfo * zf,ZoneInfo * zf2)
{
    if(zf->format != zf2->format)
    {
        ALOGV("line=%d",__LINE__);
        ALOGV("format:%x=>%x",zf->format,zf2->format);
        return 0;
    }    
    if(zf->zone_alpha!= zf2->zone_alpha)
    {
        ALOGV("line=%d",__LINE__);
        ALOGV("zone_alpha:%x=>%x",zf->zone_alpha,zf2->zone_alpha);

        return 0;
    }    
    if(zf->is_stretch || zf2->is_stretch )    
    {
        ALOGV("line=%d",__LINE__);
        ALOGV("is_stretch:%x=>%x",zf->is_stretch,zf2->is_stretch);

        return 0;
    }    
    if(is_x_intersect(&(zf->disp_rect),&(zf2->disp_rect)))  
    {
        ALOGV("line=%d",__LINE__);
        ALOGV("is_x_intersect rec(%d,%d,%d,%d)=rec2(%d,%d,%d,%d)",zf->disp_rect.left,zf->disp_rect.top,zf->disp_rect.right,\
        zf->disp_rect.bottom,zf2->disp_rect.left,zf2->disp_rect.top,zf2->disp_rect.right,zf2->disp_rect.bottom);
        return 0;
    }    
    else
        return 1;
}

int is_yuv(int format)
{
    int ret = 0;
    switch(format){
        case HAL_PIXEL_FORMAT_YCrCb_NV12:
        case HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO:
        case HAL_PIXEL_FORMAT_YCrCb_NV12_10:
            ret = 1;
            break;

        default:
            break;
    }
    return ret;
}

int is_special_wins(hwcContext * Context)
{
    return 0;
    ZoneManager* pzone_mag = &(Context->zone_manager);
    if(pzone_mag->zone_cnt == 6
        &&strstr(pzone_mag->zone_info[0].LayerName,"com.android.systemui.ImageWallpaper")
        &&strstr(pzone_mag->zone_info[3].LayerName,"Starting ")
        )
    {
        return 1;
    }
    return 0;
}

bool is_same_rect(hwc_rect_t rect1,hwc_rect_t rect2)
{
    if(rect1.left == rect2.left && rect1.top == rect2.top
        && rect1.right == rect2.right && rect1.bottom == rect2.bottom)
        return true;
    else
        return false;
}

bool is_need_post(hwc_display_contents_1_t *list,int dpyID,int flag)
{
#ifdef RK3288_BOX
    hwcContext * context = _contextAnchor;
    if(context->mLcdcNum == 2){
        return true;
    }
#endif
    switch(flag){
        case 0://hotplug device not realdy,so we not post:from set_screen
#if (defined(GPU_G6110) || defined(RK3288_BOX))
            if(hdmi_noready && dpyID == HWCE){
#if (defined(RK3368_BOX) || defined(RK3288_BOX))
                if((!hdmi_noready && (getHdmiMode() == 1 || _contextAnchor->mHdmiSI.CvbsOn))){
                    hotplug_set_frame(_contextAnchor,0);
                }
#endif
                return false;
            }
#endif
            break;

        case 1://hotplug is realdy so primary not post:from fb_post
#if (defined(GPU_G6110) || defined(RK3288_BOX))
            if((!hdmi_noready && (getHdmiMode() == 1 || _contextAnchor->mHdmiSI.CvbsOn)) && dpyID==0){
                return false;
            }
#endif
            break;
        case 2://hotplug is realdy so primary not post:from set_lcdc
#if (defined(GPU_G6110) || defined(RK3288_BOX))
            if(!(hdmi_noready || dpyID == HWCE)){
                return false;
            }
#endif
            break;
        default:
            break;
    }
    return true;
}

bool is_gpu_or_nodraw(hwc_display_contents_1_t *list,int dpyID)
{
#ifdef RK3288_BOX
    hwcContext * context = _contextAnchor;
    if(context->mLcdcNum == 2){
        return false;
    }
#endif
#if (defined(GPU_G6110) || defined(RK3288_BOX))
    if((!hdmi_noready  && dpyID == HWCP
        && (getHdmiMode() == 1 || _contextAnchor->mHdmiSI.CvbsOn))){
        for (unsigned int i = 0; i < (list->numHwLayers - 1); i++){
            hwc_layer_1_t * layer = &list->hwLayers[i];
            layer->compositionType = HWC_NODRAW;
        }
        ALOGD_IF(mLogL&HLLSIX,"Primary nodraw %s,%d",__FUNCTION__,__LINE__);
        return true;
    }
    if(hdmi_noready && dpyID == HWCE){
        ALOGD_IF(mLogL&HLLSIX,"Hotplug nodraw %s,%d",__FUNCTION__,__LINE__);
        return true;
    }
#endif
    return false;
}

static int ZoneDispatchedCheck(hwcContext* ctx,ZoneManager* pzone_mag,int flag)
{
    int ret = 0;
    hwcContext* context = _contextAnchor;
    bool Is4K = context->mHdmiSI.NeedReDst;
    for(int i=0;i<pzone_mag->zone_cnt;i++){
        int disptched = pzone_mag->zone_info[i].dispatched;
        /*win2 win3 not support YUV*/
        if(disptched > win1 && is_yuv(pzone_mag->zone_info[i].format))
            return -1;
        /*scal not support whoes source bigger than 2560 to dst 4k*/
        if(pzone_mag->zone_info[i].width > 2160 && Is4K)
            return -1;
    }
    return ret;
}

int collect_all_zones( hwcContext * Context,hwc_display_contents_1_t * list)
{
    size_t i,j;
    int tsize = 0;
    int factor =1;
    Context->mMultiwindow = false;
    for (i = 0,j=0; i < (list->numHwLayers - 1) ; i++,j++){
        hwc_layer_1_t * layer = &list->hwLayers[i];
        hwc_region_t * Region = &layer->visibleRegionScreen;
        hwc_rect_t * SrcRect = &layer->sourceCrop;
        hwc_rect_t * DstRect = &layer->displayFrame;
        bool IsBottom = !strcmp(BOTTOM_LAYER_NAME,layer->LayerName);
        bool IsTop = !strcmp(TOP_LAYER_NAME,layer->LayerName);
        struct private_handle_t* SrcHnd = (struct private_handle_t *) layer->handle;
        float hfactor;
        float vfactor;
        hwcRECT dstRects[16];
        unsigned int m = 0;
        bool is_stretch = 0;
        hwc_rect_t const * rects = Region->rects;
        hwc_rect_t  rect_merge;
        bool haveStartwin = false;
        bool trsfrmbyrga = false;
        int glesPixels = 0;
        int overlayPixels = 0;
#if (defined(RK3368_BOX) || defined(RK3288_BOX))
        int d_w = 0;  //external weight & height
        int d_h = 0;
        int s_w = 0;
        int s_h = 0;
        float v_scale = 0.0;  //source v_scale & h_scale
        float h_scale = 0.0;
        bool NeedScale = false;
        hwcRECT DstRectScale;
        char value[PROPERTY_VALUE_MAX];
        property_get("persist.sys.video.cvrs", value, "false");
        NeedScale = !strcmp(value,"true");
        hotplug_get_resolution(&d_w,&d_h);
        DstRectScale.left  = 0; 
        DstRectScale.top   = 0; 
        DstRectScale.right = d_w; 
        DstRectScale.bottom= d_h;
#endif

        if(strstr(layer->LayerName,"Starting@# ")){
            haveStartwin = true;
        }
#if ENABLE_TRANSFORM_BY_RGA
        if(layer->transform && Context->mtrsformcnt == 1
            && SrcHnd->format != HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO){
            trsfrmbyrga = true;
        }
#endif
#if !ENABLE_LCDC_IN_NV12_TRANSFORM
        if(Context->mGtsStatus)
#endif
        {
            ALOGV("In gts status,go into lcdc when rotate video");
            if(layer->transform && SrcHnd->format == HAL_PIXEL_FORMAT_YCrCb_NV12){
                trsfrmbyrga = true;
            }
        }
        if(j>=MaxZones){
            ALOGD("Overflow [%d] >max=%d",m+j,MaxZones);
            return -1;
        }
        if((layer->transform == HWC_TRANSFORM_ROT_90)
            ||(layer->transform == HWC_TRANSFORM_ROT_270)){
            hfactor = (float) (SrcRect->bottom - SrcRect->top)
                    / (DstRect->right - DstRect->left);
            vfactor = (float) (SrcRect->right - SrcRect->left)
                    / (DstRect->bottom - DstRect->top);
        }else{
            hfactor = (float) (SrcRect->right - SrcRect->left)
                    / (DstRect->right - DstRect->left);

            vfactor = (float) (SrcRect->bottom - SrcRect->top)
                    / (DstRect->bottom - DstRect->top);
        }
        if(hfactor >= 8.0 || vfactor >= 8.0 || hfactor <= 0.125 || vfactor <= 0.125  ){
            Context->zone_manager.zone_info[j].scale_err = true;
            ALOGD_IF(mLogL&HLLSIX,"stretch[%f,%f]not support!",hfactor,vfactor);
        } 
        is_stretch = (hfactor != 1.0) || (vfactor != 1.0);
        if(Context == _contextAnchor1){
            is_stretch = is_stretch || _contextAnchor->mHdmiSI.NeedReDst;
        }
#if ONLY_USE_ONE_VOP
#ifdef RK3288_BOX
        if(_contextAnchor->mLcdcNum == 1)
#endif
        {
            is_stretch = is_stretch || _contextAnchor->mHdmiSI.NeedReDst;
        }
#endif
#ifdef RK3288_BOX
        if(Context==_contextAnchor && Context->mResolutionChanged && Context->mLcdcNum == 2){
            is_stretch = true;
        }
#endif
        int left_min=0 ;
        int top_min=0;
        int right_max=0;
        int bottom_max=0;
        int isLarge = 0;
        int srcw,srch;    
        if(rects){
             left_min = rects[0].left; 
             top_min  = rects[0].top;
             right_max  = rects[0].right;
             bottom_max = rects[0].bottom;
        }
        for (int r = 0; r < (int) Region->numRects ; r++){
            int r_left;
            int r_top;
            int r_right;
            int r_bottom;
           
            r_left   = hwcMAX(DstRect->left,   rects[r].left);
            left_min = hwcMIN(r_left,left_min);
            r_top    = hwcMAX(DstRect->top,    rects[r].top);
            top_min  = hwcMIN(r_top,top_min);
            r_right    = hwcMIN(DstRect->right,  rects[r].right);
            right_max  = hwcMAX(r_right,right_max);
            r_bottom = hwcMIN(DstRect->bottom, rects[r].bottom);
            bottom_max  = hwcMAX(r_bottom,bottom_max);
            glesPixels += (r_right-r_left)*(r_bottom-r_top);
        }
        rect_merge.left = left_min;
        rect_merge.top = top_min;
        rect_merge.right = right_max;
        rect_merge.bottom = bottom_max;

        if(Region->numRects > 1 && i == 0 && !(mLogL & 65536)) {
            Context->mMultiwindow = true;
        }
        overlayPixels = (DstRect->right-DstRect->left)*(DstRect->bottom-DstRect->top);
        Context->zone_manager.zone_info[j].glesPixels = glesPixels;
        Context->zone_manager.zone_info[j].overlayPixels = overlayPixels;
        overlayPixels = (SrcRect->right-SrcRect->left)*(SrcRect->bottom-SrcRect->top);
        glesPixels = int(1.0 * glesPixels /Context->zone_manager.zone_info[j].overlayPixels * overlayPixels);
        Context->zone_manager.zone_info[j].glesPixels += glesPixels;
        Context->zone_manager.zone_info[j].overlayPixels += overlayPixels;

        unsigned const char* pBuffer = (unsigned const char*)DstRect;
        unsigned int crc32 = createCrc32(0xFFFFFFFF,pBuffer,sizeof(hwc_rect_t));
        if(false) {
            pBuffer = (unsigned const char*)rects;
            crc32 = createCrc32(crc32,pBuffer,sizeof(hwc_rect_t)*Region->numRects);
        }
        Context->zone_manager.zone_info[j].zoneCrc = crc32;

        //zxl:If in video mode,then use all area.
        if(SrcHnd->format == HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO 
            || SrcHnd->format == HAL_PIXEL_FORMAT_YCrCb_NV12){
            dstRects[0].left   = DstRect->left;
            dstRects[0].top    = DstRect->top;
            dstRects[0].right  = DstRect->right;
            dstRects[0].bottom = DstRect->bottom;
#if (defined(RK3368_BOX) || defined(RK3288_BOX))
            if(Context == _contextAnchor1 && NeedScale){
                s_w = SrcRect->right - SrcRect->left;
                s_h = SrcRect->bottom - SrcRect->top;
                if(s_w*d_h-s_h*d_w > 0){ //d_w standard
                    ALOGV("%s,%d,[%d,%d][%d,%d]",__FUNCTION__,__LINE__,d_w,d_h,s_w,s_h);
                    DstRectScale.left   = 0;
                    DstRectScale.top    = ((d_h-s_h*d_w/s_w)%2==0)?((d_h-s_h*d_w/s_w)/2):((d_h-s_h*d_w/s_w)/2);
                    DstRectScale.right  = d_w;
                    DstRectScale.bottom = d_h - DstRectScale.top;
                }else{
                    ALOGV("%s,%d,[%d,%d][%d,%d]",__FUNCTION__,__LINE__,d_w,d_h,s_w,s_h);
                    DstRectScale.left   = ((d_w-s_w*d_h/s_h)%2==0)?((d_w-s_w*d_h/s_h)/2):((d_w-s_w*d_h/s_h+1)/2);;
                    DstRectScale.top    = 0;
                    DstRectScale.right  = d_w - DstRectScale.left;
                    DstRectScale.bottom = d_h;
                }
            }
#endif
        }else{
            dstRects[0].left   = hwcMAX(DstRect->left,   rect_merge.left);
            dstRects[0].top    = hwcMAX(DstRect->top,    rect_merge.top);
            dstRects[0].right  = hwcMIN(DstRect->right,  rect_merge.right);
            dstRects[0].bottom = hwcMIN(DstRect->bottom, rect_merge.bottom);
        }        
        /* Check dest area. */
        if ((dstRects[m].right <= dstRects[m].left) 
            ||  (dstRects[m].bottom <= dstRects[m].top)){
			Context->zone_manager.zone_info[j].zone_err = true;
			LOGI("%s(%d):  skip empty rectangle [%d,%d,%d,%d]",__FUNCTION__,__LINE__,
			dstRects[m].left,dstRects[m].top,dstRects[m].right,dstRects[m].bottom);
        }
        if((dstRects[m].right - dstRects[m].left) < 16 
            || (dstRects[m].bottom - dstRects[m].top) < 16){
            Context->zone_manager.zone_info[j].toosmall = true;
        }
        
        LOGV("%s(%d): Region rect[%d]:  [%d,%d,%d,%d]",__FUNCTION__,__LINE__,
            m,rects[m].left,rects[m].top,rects[m].right,rects[m].bottom);

        Context->zone_manager.zone_info[j].zone_alpha = (layer->blending) >> 16;
        Context->zone_manager.zone_info[j].is_stretch = is_stretch;
    	Context->zone_manager.zone_info[j].hfactor = hfactor;;
        Context->zone_manager.zone_info[j].zone_index = j;
        Context->zone_manager.zone_info[j].layer_index = i;
        Context->zone_manager.zone_info[j].dispatched = 0;
        Context->zone_manager.zone_info[j].direct_fd = 0;
        Context->zone_manager.zone_info[j].sort = 0;
        Context->zone_manager.zone_info[j].addr = 0;
        Context->zone_manager.zone_info[j].handle = (struct private_handle_t *)layer->handle;
        Context->zone_manager.zone_info[j].transform = layer->transform;
        Context->zone_manager.zone_info[j].realtransform = layer->realtransform;
#ifdef SUPPORT_STEREO
        Context->zone_manager.zone_info[j].alreadyStereo = layer->alreadyStereo;
        Context->zone_manager.zone_info[j].displayStereo = layer->displayStereo;
#endif
        strcpy(Context->zone_manager.zone_info[j].LayerName,layer->LayerName);
        Context->zone_manager.zone_info[j].disp_rect.left = dstRects[0].left;
        Context->zone_manager.zone_info[j].disp_rect.top = dstRects[0].top;

        //zxl:Temporary solution to fix blank bar bug when wake up.
        if(i==1 && Context == _contextAnchor && 
            !strcmp(layer->LayerName,VIDEO_PLAY_ACTIVITY_LAYER_NAME) ){
            Context->zone_manager.zone_info[j].disp_rect.right = SrcHnd->width;
            Context->zone_manager.zone_info[j].disp_rect.bottom = SrcHnd->height;

            if(Context->zone_manager.zone_info[j].disp_rect.left)
                Context->zone_manager.zone_info[j].disp_rect.left=0;

            if(Context->zone_manager.zone_info[j].disp_rect.top)
                Context->zone_manager.zone_info[j].disp_rect.top=0;
        }else{
            Context->zone_manager.zone_info[j].disp_rect.right = dstRects[0].right;
            Context->zone_manager.zone_info[j].disp_rect.bottom = dstRects[0].bottom;
        }

#if USE_HWC_FENCE
        Context->zone_manager.zone_info[j].acq_fence_fd =layer->acquireFenceFd;
#endif
        if(SrcHnd->format == HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO || (trsfrmbyrga)){
            int w_valid = 0 ,h_valid = 0;
#if USE_VIDEO_BACK_BUFFERS
            int index_v = Context->mCurVideoIndex%MaxVideoBackBuffers;
            int video_fd = Context->fd_video_bk[index_v];
#else
            int video_fd;
            int index_v;
            if(trsfrmbyrga){
                index_v = Context->mCurVideoIndex%MaxVideoBackBuffers;
                video_fd = Context->fd_video_bk[index_v];
            }else{
                video_fd= SrcHnd->share_fd;
            }
#endif
		    hwc_rect_t * psrc_rect = &(Context->zone_manager.zone_info[j].src_rect);
            //HAL_PIXEL_FORMAT_YCrCb_NV12;
            Context->zone_manager.zone_info[j].format = trsfrmbyrga ? SrcHnd->format:Context->video_fmt;
            ALOGV("HAL_PIXEL_FORMAT_YCrCb_NV12 transform=%d, addr[%x][%dx%d],ori_fd[%d][%dx%d]",
                    layer->transform,SrcHnd->video_addr,SrcHnd->video_width,SrcHnd->video_height,
                    SrcHnd->share_fd,SrcHnd->width,SrcHnd->height);
            switch (layer->transform){
                case 0:
                    psrc_rect->left   = SrcRect->left
                    - (int) ((DstRect->left   - dstRects[0].left)   * hfactor);
                    psrc_rect->top    = SrcRect->top
                    - (int) ((DstRect->top    - dstRects[0].top)    * vfactor);
                    psrc_rect->right  = SrcRect->right
                    - (int) ((DstRect->right  - dstRects[0].right)  * hfactor);
                    psrc_rect->bottom = SrcRect->bottom
                    - (int) ((DstRect->bottom - dstRects[0].bottom) * vfactor);
                    Context->zone_manager.zone_info[j].layer_fd = 0;    
                    Context->zone_manager.zone_info[j].addr = SrcHnd->video_addr; 
                    Context->zone_manager.zone_info[j].width = SrcHnd->video_width;
                    Context->zone_manager.zone_info[j].height = SrcHnd->video_height;
                    Context->zone_manager.zone_info[j].stride = SrcHnd->video_width; 
                    //Context->zone_manager.zone_info[j].format = SrcHnd->format;                        
                    break;
                    
        		 case HWC_TRANSFORM_ROT_270:
                    if(trsfrmbyrga){
                        psrc_rect->left = SrcRect->top;
                        psrc_rect->top  = SrcHnd->width -  SrcRect->right;//SrcRect->top;
                        psrc_rect->right = SrcRect->bottom;//SrcRect->right;
                        psrc_rect->bottom = SrcHnd->width - SrcRect->left;//SrcRect->bottom; 
                    }else{
                        psrc_rect->left   = SrcRect->top +  (SrcRect->right - SrcRect->left)
                        - ((dstRects[0].bottom - DstRect->top)    * vfactor);

                        psrc_rect->top    =  SrcRect->left
                        - (int) ((DstRect->left   - dstRects[0].left)   * hfactor);

                        psrc_rect->right  = psrc_rect->left
                        + (int) ((dstRects[0].bottom - dstRects[0].top) * vfactor);

                        psrc_rect->bottom = psrc_rect->top
                        + (int) ((dstRects[0].right  - dstRects[0].left) * hfactor);
                    }    
                    h_valid = trsfrmbyrga ? SrcHnd->height : (psrc_rect->bottom - psrc_rect->top);
                    w_valid = trsfrmbyrga ? SrcHnd->width : (psrc_rect->right - psrc_rect->left);
                    Context->zone_manager.zone_info[j].layer_fd = video_fd;
                    Context->zone_manager.zone_info[j].width = w_valid;
                    Context->zone_manager.zone_info[j].height = rkmALIGN(h_valid,8);
                    Context->zone_manager.zone_info[j].stride = w_valid;                                                    
                    //Context->zone_manager.zone_info[j].format = HAL_PIXEL_FORMAT_RGB_565;
                    break;

                case HWC_TRANSFORM_ROT_90:
                    if(trsfrmbyrga){
                        psrc_rect->left = SrcHnd->height - SrcRect->bottom;
                        psrc_rect->top  = SrcRect->left;//SrcRect->top;
                        psrc_rect->right = SrcHnd->height - SrcRect->top;//SrcRect->right;
                        psrc_rect->bottom = SrcRect->right;//SrcRect->bottom; 
                    }else{
                        psrc_rect->left   = SrcRect->top
                            - (int) ((DstRect->top    - dstRects[0].top)    * vfactor);

                        psrc_rect->top    =  SrcRect->left
                            - (int) ((DstRect->left   - dstRects[0].left)   * hfactor);

                        psrc_rect->right  = psrc_rect->left
                            + (int) ((dstRects[0].bottom - dstRects[0].top) * vfactor);

                        psrc_rect->bottom = psrc_rect->top
                            + (int) ((dstRects[0].right  - dstRects[0].left) * hfactor);                        
                    }
                    h_valid = trsfrmbyrga ? SrcHnd->height : (psrc_rect->bottom - psrc_rect->top);
                    w_valid = trsfrmbyrga ? SrcHnd->width : (psrc_rect->right - psrc_rect->left);
                   
                    Context->zone_manager.zone_info[j].layer_fd = video_fd;
                    Context->zone_manager.zone_info[j].width = w_valid;
                    Context->zone_manager.zone_info[j].height = rkmALIGN(h_valid,8); ;
                    Context->zone_manager.zone_info[j].stride = w_valid;   
                    //Context->zone_manager.zone_info[j].format = HAL_PIXEL_FORMAT_RGB_565;
                    break;

        		case HWC_TRANSFORM_ROT_180:
                    if(trsfrmbyrga){
                        psrc_rect->left = SrcHnd->width - SrcRect->right;
                        psrc_rect->top  = SrcHnd->height - SrcRect->bottom;//SrcRect->top;
                        psrc_rect->right = SrcHnd->width - SrcRect->left;//SrcRect->right;
                        psrc_rect->bottom = SrcHnd->height - SrcRect->top;//SrcRect->bottom; 
                    }else{            		
                        psrc_rect->left   = SrcRect->left +  (SrcRect->right - SrcRect->left)
                        - ((dstRects[0].right - DstRect->left)   * hfactor);

                        psrc_rect->top    = SrcRect->top
                        - (int) ((DstRect->top    - dstRects[0].top)    * vfactor);

                        psrc_rect->right  = psrc_rect->left
                        + (int) ((dstRects[0].right  - dstRects[0].left) * hfactor);

                        psrc_rect->bottom = psrc_rect->top
                        + (int) ((dstRects[0].bottom - dstRects[0].top) * vfactor);
                    }
                    // w_valid = psrc_rect->right - psrc_rect->left;
                    //h_valid = psrc_rect->bottom - psrc_rect->top;
                    w_valid = trsfrmbyrga ? SrcHnd->width : (psrc_rect->right - psrc_rect->left);
                    h_valid = trsfrmbyrga ? SrcHnd->height : (psrc_rect->bottom - psrc_rect->top);
                   
                    Context->zone_manager.zone_info[j].layer_fd = video_fd;
                    Context->zone_manager.zone_info[j].width = w_valid;
                    Context->zone_manager.zone_info[j].height = h_valid;
                    Context->zone_manager.zone_info[j].stride = w_valid;                                
                                                                
                    //Context->zone_manager.zone_info[j].format = HAL_PIXEL_FORMAT_RGB_565;                          
                    break;
                    
                default:
                    //ALOGD("Unsupport transform=0x%x",layer->transform);
                    return -1;
            }   
            ALOGV("layer->transform=%d",layer->transform);
            if(layer->transform){
                int lastfd = -1;
                bool fd_update = true;
                lastfd = Context->mRgaTBI.lastfd;
                if(trsfrmbyrga && lastfd == SrcHnd->share_fd && 
                    SrcHnd->format != HAL_PIXEL_FORMAT_YCrCb_NV12){
                    fd_update = true;
                }
                if(fd_update){
                    ALOGV("Zone[%d]->layer[%d],"
                        "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
                        "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],acq_fence_fd=%d,"
                        "layname=%s,fd=%d",
                        Context->zone_manager.zone_info[j].zone_index,
                        Context->zone_manager.zone_info[j].layer_index,
                        Context->zone_manager.zone_info[j].src_rect.left,
                        Context->zone_manager.zone_info[j].src_rect.top,
                        Context->zone_manager.zone_info[j].src_rect.right,
                        Context->zone_manager.zone_info[j].src_rect.bottom,
                        Context->zone_manager.zone_info[j].disp_rect.left,
                        Context->zone_manager.zone_info[j].disp_rect.top,
                        Context->zone_manager.zone_info[j].disp_rect.right,
                        Context->zone_manager.zone_info[j].disp_rect.bottom,
                        Context->zone_manager.zone_info[j].width,
                        Context->zone_manager.zone_info[j].height,
                        Context->zone_manager.zone_info[j].stride,
                        Context->zone_manager.zone_info[j].format,
                        Context->zone_manager.zone_info[j].transform,
                        Context->zone_manager.zone_info[j].realtransform,
                        Context->zone_manager.zone_info[j].blend,
                        Context->zone_manager.zone_info[j].acq_fence_fd,
                        Context->zone_manager.zone_info[j].LayerName,
                        Context->zone_manager.zone_info[j].layer_fd);
                    Context->mRgaTBI.hdl = SrcHnd;
                    Context->mRgaTBI.index = i;
                    Context->mRgaTBI.w_valid = w_valid;
                    Context->mRgaTBI.h_valid = h_valid;
                    Context->mRgaTBI.transform = layer->transform;
                    Context->mRgaTBI.trsfrmbyrga = trsfrmbyrga;
                    Context->mRgaTBI.layer_fd = Context->zone_manager.zone_info[j].layer_fd;
					Context->mRgaTBI.lastfd = SrcHnd->share_fd;
                    Context->mNeedRgaTransform = true;
                }
            }
			psrc_rect->left = psrc_rect->left - psrc_rect->left%2;
			psrc_rect->top = psrc_rect->top - psrc_rect->top%2;
			psrc_rect->right = psrc_rect->right - psrc_rect->right%2;
			psrc_rect->bottom = psrc_rect->bottom - psrc_rect->bottom%2;  
		}
        else{
            Context->zone_manager.zone_info[j].src_rect.left   = hwcMAX ((SrcRect->left \
            - (int) ((DstRect->left   - dstRects[0].left)   * hfactor)),0);
            Context->zone_manager.zone_info[j].src_rect.top    = hwcMAX ((SrcRect->top \
            - (int) ((DstRect->top    - dstRects[0].top)    * vfactor)),0);

            //zxl:Temporary solution to fix blank bar bug when wake up.
            if(i==1 && Context != _contextAnchor1 && 
                !strcmp(layer->LayerName,VIDEO_PLAY_ACTIVITY_LAYER_NAME)){
                Context->zone_manager.zone_info[j].src_rect.right = SrcHnd->width;
                Context->zone_manager.zone_info[j].src_rect.bottom = SrcHnd->height;

                if(Context->zone_manager.zone_info[j].src_rect.left)
                    Context->zone_manager.zone_info[j].src_rect.left=0;

                if(Context->zone_manager.zone_info[j].src_rect.top)
                    Context->zone_manager.zone_info[j].src_rect.top=0;
            }else{
                Context->zone_manager.zone_info[j].src_rect.right  = SrcRect->right \
                - (int) ((DstRect->right  - dstRects[0].right)  * hfactor);
                Context->zone_manager.zone_info[j].src_rect.bottom = SrcRect->bottom \
                - (int) ((DstRect->bottom - dstRects[0].bottom) * vfactor);
            }
            Context->zone_manager.zone_info[j].format = SrcHnd->format;
            Context->zone_manager.zone_info[j].width = SrcHnd->width;
            Context->zone_manager.zone_info[j].height = SrcHnd->height;
            Context->zone_manager.zone_info[j].stride = SrcHnd->stride;
            Context->zone_manager.zone_info[j].layer_fd = SrcHnd->share_fd;

            //odd number will lead to lcdc composer fail with error display.
            if(SrcHnd->format == HAL_PIXEL_FORMAT_YCrCb_NV12){
                Context->zone_manager.zone_info[j].src_rect.left = \
                    Context->zone_manager.zone_info[j].src_rect.left - Context->zone_manager.zone_info[j].src_rect.left%2;
                Context->zone_manager.zone_info[j].src_rect.top = \
                    Context->zone_manager.zone_info[j].src_rect.top - Context->zone_manager.zone_info[j].src_rect.top%2;
                Context->zone_manager.zone_info[j].src_rect.right = \
                    Context->zone_manager.zone_info[j].src_rect.right - Context->zone_manager.zone_info[j].src_rect.right%2;
                Context->zone_manager.zone_info[j].src_rect.bottom = \
                    Context->zone_manager.zone_info[j].src_rect.bottom - Context->zone_manager.zone_info[j].src_rect.bottom%2;                
            }
        }    
        srcw = Context->zone_manager.zone_info[j].src_rect.right - \
                Context->zone_manager.zone_info[j].src_rect.left;
        srch = Context->zone_manager.zone_info[j].src_rect.bottom -  \
                Context->zone_manager.zone_info[j].src_rect.top;
        int bpp = android::bytesPerPixel(Context->zone_manager.zone_info[j].format);
        if(Context->zone_manager.zone_info[j].format == HAL_PIXEL_FORMAT_YCrCb_NV12
            || Context->zone_manager.zone_info[j].format == HAL_PIXEL_FORMAT_YCrCb_NV12_10
            || Context->zone_manager.zone_info[j].format == HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO
            || haveStartwin)
            bpp = 2;
#ifdef GPU_G6110
        else if(Context->zone_manager.zone_info[j].format == HAL_PIXEL_FORMAT_BGRX_8888)
            bpp = 4;
#endif
        else
            bpp = 4;

        // ALOGD("haveStartwin=%d,bpp=%d",haveStartwin,bpp);
        Context->zone_manager.zone_info[j].size = srcw*srch*bpp;
        if(Context->zone_manager.zone_info[j].hfactor > 1.0 || Context->mIsMediaView)
            factor = 2;
        else
            factor = 1;
        tsize += (Context->zone_manager.zone_info[j].size *factor);
        if(Context->zone_manager.zone_info[j].size > \
            (Context->fbhandle.width * Context->fbhandle.height*3) ){  // w*h*4*3/4
            Context->zone_manager.zone_info[j].is_large = 1; 
        }else
            Context->zone_manager.zone_info[j].is_large = 0; 
#if (defined(RK3368_BOX) || defined(RK3288_BOX))
        if((SrcHnd->format == HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO ||
            SrcHnd->format == HAL_PIXEL_FORMAT_YCrCb_NV12) &&(Context == _contextAnchor1 && NeedScale)){
            Context->zone_manager.zone_info[j].disp_rect.left  = DstRectScale.left;  
            Context->zone_manager.zone_info[j].disp_rect.top   = DstRectScale.top;   
            Context->zone_manager.zone_info[j].disp_rect.right = DstRectScale.right; 
            Context->zone_manager.zone_info[j].disp_rect.bottom = DstRectScale.bottom;
        }
#endif
    }
    Context->zone_manager.zone_cnt = j;
    if(tsize)
        Context->zone_manager.bp_size = tsize / (1024 *1024) * 60 ;// MB
    // query ddr is enough ,if dont enough back to gpu composer
    ALOGV("tsize=%dMB,Context->ddrFd=%d,RK_QUEDDR_FREQ",tsize,Context->ddrFd);
    for(i=0;i<j;i++){
        ALOGD_IF(mLogL&HLLONE,"Zone[%d]->layer[%d],"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],acq_fence_fd=%d,"
            "s_g_o[%d,%d,%d],layname=%s",
            Context->zone_manager.zone_info[i].zone_index,
            Context->zone_manager.zone_info[i].layer_index,
            Context->zone_manager.zone_info[i].src_rect.left,
            Context->zone_manager.zone_info[i].src_rect.top,
            Context->zone_manager.zone_info[i].src_rect.right,
            Context->zone_manager.zone_info[i].src_rect.bottom,
            Context->zone_manager.zone_info[i].disp_rect.left,
            Context->zone_manager.zone_info[i].disp_rect.top,
            Context->zone_manager.zone_info[i].disp_rect.right,
            Context->zone_manager.zone_info[i].disp_rect.bottom,
            Context->zone_manager.zone_info[i].width,
            Context->zone_manager.zone_info[i].height,
            Context->zone_manager.zone_info[i].stride,
            Context->zone_manager.zone_info[i].format,
            Context->zone_manager.zone_info[i].transform,
            Context->zone_manager.zone_info[i].realtransform,
            Context->zone_manager.zone_info[i].blend,
            Context->zone_manager.zone_info[i].acq_fence_fd,
            Context->zone_manager.zone_info[i].is_stretch,
            Context->zone_manager.zone_info[i].glesPixels,
            Context->zone_manager.zone_info[i].overlayPixels,
            Context->zone_manager.zone_info[i].LayerName);
    }
    return 0;
}

// return 0: suess
// return -1: fail
int try_wins_dispatch_hor(void * ctx,hwc_display_contents_1_t * list)
{
    int win_disphed_flag[4] = {0,}; // win0, win1, win2, win3 flag which is dispatched
    int win_disphed[4] = {win0,win1,win2_0,win3_0};
    int i,j;
    int sort = 1;
    int cnt = 0;
    int srot_tal[4][2] = {0,};
    int sort_stretch[4] = {0}; 
    int sort_pre;
    float hfactor_max = 1.0;
    int large_cnt = 0;
    int bw = 0;
    bool isyuv = false;
    BpVopInfo  bpvinfo;    
    int same_cnt = 0;

    hwcContext * Context = (hwcContext *)ctx;
    hwcContext * contextAh = _contextAnchor;
    memset(&bpvinfo,0,sizeof(BpVopInfo));
    ZoneManager zone_m;
    memcpy(&zone_m,&Context->zone_manager,sizeof(ZoneManager));
    ZoneManager* pzone_mag = &zone_m;
    // try dispatch stretch wins
    char const* compositionTypeName[] = {
            "win0",
            "win1",
            "win2_0",
            "win2_1",
            "win2_2",
            "win2_3",
            "win3_0",
            "win3_1",
            "win3_2",
            "win3_3",
            };
    
#if OPTIMIZATION_FOR_TRANSFORM_UI
    //ignore transform ui layer case.
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
        if((pzone_mag->zone_info[i].transform != 0)&&
            (pzone_mag->zone_info[i].format != HAL_PIXEL_FORMAT_YCrCb_NV12))
            return -1;
    }
#endif

    if(Context->Is3D){
        return -1;
    }

    if(Context->mAlphaError){
        return -1;
    }

    for(int k=0;k<pzone_mag->zone_cnt;k++)
    {
        if(pzone_mag->zone_info[k].scale_err || pzone_mag->zone_info[k].toosmall
            || pzone_mag->zone_info[k].zone_err)
            return -1;
    }

    pzone_mag->zone_info[0].sort = sort;
    for(i=0;i<(pzone_mag->zone_cnt-1);)
    {
        bool is_winfull = false;
        pzone_mag->zone_info[i].sort = sort;
        sort_pre  = sort;
        cnt = 0;
        //means 4: win2 or win3 most has 4 zones 
        for(j=1;j<MOST_WIN_ZONES && (i+j) < pzone_mag->zone_cnt;j++)
        {
            ZoneInfo * next_zf = &(pzone_mag->zone_info[i+j]);
            bool is_combine = false;
            int k;
            for(k=0;k<=cnt;k++)  // compare all sorted_zone info
            {
                ZoneInfo * sorted_zf = &(pzone_mag->zone_info[i+j-1-k]);
                if(is_zone_combine(sorted_zf,next_zf)
                    #if ENBALE_WIN_ANY_ZONES
                    && same_cnt < 1
                    #endif
                   )
                {
                    is_combine = true;
                    same_cnt ++;
                }
                else
                {
                    is_combine = false;
                    #if ENBALE_WIN_ANY_ZONES
                    if(same_cnt >= 1)
                    {
                        is_winfull = true;
                        same_cnt = 0; 
                    }   
                    #endif
                    break;
                }
            }
            if(is_combine)
            {
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;
                ALOGV("combine [%d]=%d,cnt=%d",i+j,sort,cnt);
            }
            else
            {
                if(!is_winfull)
                sort++;
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;
                ALOGV("Not combine [%d]=%d,cnt=%d",i+j,sort,cnt);                
                break;
            }
        }
        if( sort_pre == sort && (i+cnt) < (pzone_mag->zone_cnt-1) )  // win2 ,4zones ,win3 4zones,so sort ++,but exit not ++
        {
            if(!is_winfull)
            sort ++;
            ALOGV("sort++ =%d,[%d,%d,%d]",sort,i,cnt,pzone_mag->zone_cnt);
        }    
        i += cnt;      
    }
    if(sort >4)  // lcdc dont support 5 wins
    {
        ALOGD_IF(mLogL&HLLFOU,"try %s lcdc<5wins sort=%d,%d",__FUNCTION__,sort,__LINE__);
        return -1;
    }    
	//pzone_mag->zone_info[i].sort: win type
	// srot_tal[i][0] : tatal same wins
	// srot_tal[0][i] : dispatched lcdc win
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
        ALOGV("sort[%d].type=%d",i,pzone_mag->zone_info[i].sort);
        if( pzone_mag->zone_info[i].sort == 1){
            srot_tal[0][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[0] = 1;
        }    
        else if(pzone_mag->zone_info[i].sort == 2){
            srot_tal[1][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[1] = 1;
        }    
        else if(pzone_mag->zone_info[i].sort == 3){
            srot_tal[2][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[2] = 1;
            
        }    
        else if(pzone_mag->zone_info[i].sort == 4){
            srot_tal[3][0]++;   
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[3] = 1;            
        }    
        if(pzone_mag->zone_info[i].hfactor > hfactor_max)
        {
            hfactor_max = pzone_mag->zone_info[i].hfactor;
        }
        if(pzone_mag->zone_info[i].is_large )
        {
            large_cnt ++;
        }
        if(pzone_mag->zone_info[i].format== HAL_PIXEL_FORMAT_YCrCb_NV12)
        {
            isyuv = true;
        }

    }
    if(hfactor_max >=1.4)
        bw ++;
    if(isyuv)    
    {
        if(pzone_mag->zone_cnt <5)
        bw += 2;
        else 
            bw += 4;
    }    
    // first dispatch more zones win
    j = 0;
    for(i=0;i<4;i++)    
    {        
        if( srot_tal[i][0] >=2)  // > twice zones
        {
            srot_tal[i][1] = win_disphed[j+2]; 
            win_disphed_flag[j+2] = 1; // win2 ,win3 is dispatch flag
            ALOGV("more twice zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 2)  // lcdc only has win2 and win3 supprot more zones
            {
                ALOGD_IF(mLogL&HLLFOU,"lcdc only has win2 and win3 supprot more zones");
                return -1;  
            }
        }
    }
    // second dispatch stretch win
    j = 0;
    for(i=0;i<4;i++)    
    {        
        if( sort_stretch[i] == 1)  // strech
        {
            srot_tal[i][1] = win_disphed[j];  // win 0 and win 1 suporot stretch
            win_disphed_flag[j] = 1; // win2 ,win3 is dispatch flag
            ALOGV("stretch zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 2)  // lcdc only has win2 and win3 supprot more zones
            {
                ALOGD_IF(mLogL&HLLFOU,"lcdc only has win0 and win1 supprot stretch");
                return -1;  
            }
        }
    }  
    // third dispatch common zones win
    for(i=0;i<4;i++)    
    {        
        if( srot_tal[i][1] == 0)  // had not dispatched
        {
            for(j=0;j<4;j++)
            {
                if(win_disphed_flag[j] == 0) // find the win had not dispatched
                    break;
            }  
            if(j>=4)
            {
                ALOGE("4 wins had beed dispatched ");
                return -1;
            }    
            srot_tal[i][1] = win_disphed[j];
            win_disphed_flag[j] = 1;
            ALOGV("srot_tal[%d][1].dispatched=%d",i,srot_tal[i][1]);
        }
    }

    for(i=0;i<pzone_mag->zone_cnt;i++)
    {        
         switch(pzone_mag->zone_info[i].sort) {
            case 1:
                pzone_mag->zone_info[i].dispatched = srot_tal[0][1]++;
                break;
            case 2:
                pzone_mag->zone_info[i].dispatched = srot_tal[1][1]++;            
                break;
            case 3:
                pzone_mag->zone_info[i].dispatched = srot_tal[2][1]++;            
                break;
            case 4:
                pzone_mag->zone_info[i].dispatched = srot_tal[3][1]++;            
                break;                
            default:
                ALOGE("try_wins_dispatch_hor sort err!");
                return -1;
        }
        ALOGD_IF(mLogL&HLLFIV,"zone[%d].dispatched[%d]=%s,sort=%d", \
        i,pzone_mag->zone_info[i].dispatched,
        compositionTypeName[pzone_mag->zone_info[i].dispatched -1],
        pzone_mag->zone_info[i].sort);

    }

    for(i=0;i<pzone_mag->zone_cnt;i++){
        int disptched = pzone_mag->zone_info[i].dispatched;
        int sct_width = pzone_mag->zone_info[i].width;
        int sct_height = pzone_mag->zone_info[i].height;
        /*win2 win3 not support YUV*/
        if(disptched > win1 && is_yuv(pzone_mag->zone_info[i].format))
            return -1;
        /*scal not support whoes source bigger than 2560 to dst 4k*/
        if(disptched <= win1 &&(sct_width > 2160 || sct_height > 2160) &&
            !is_yuv(pzone_mag->zone_info[i].format) && contextAh->mHdmiSI.NeedReDst)
            return -1;
    }

#if USE_QUEUE_DDRFREQ
    if(Context->ddrFd > 0)
    {
        for(i=0;i<pzone_mag->zone_cnt;i++)
        {
            int area_no = 0;
            int win_id = 0;
            ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],dispatched=%d,"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],"
            "layer_fd[%d],addr=%x,acq_fence_fd=%d"
            "layname=%s",    
            pzone_mag->zone_info[i].zone_index,
            pzone_mag->zone_info[i].layer_index,
            pzone_mag->zone_info[i].dispatched,
            pzone_mag->zone_info[i].src_rect.left,
            pzone_mag->zone_info[i].src_rect.top,
            pzone_mag->zone_info[i].src_rect.right,
            pzone_mag->zone_info[i].src_rect.bottom,
            pzone_mag->zone_info[i].disp_rect.left,
            pzone_mag->zone_info[i].disp_rect.top,
            pzone_mag->zone_info[i].disp_rect.right,
            pzone_mag->zone_info[i].disp_rect.bottom,
            pzone_mag->zone_info[i].width,
            pzone_mag->zone_info[i].height,
            pzone_mag->zone_info[i].stride,
            pzone_mag->zone_info[i].format,
            pzone_mag->zone_info[i].transform,
            pzone_mag->zone_info[i].realtransform,
            pzone_mag->zone_info[i].blend,
            pzone_mag->zone_info[i].layer_fd,
            pzone_mag->zone_info[i].addr,
            pzone_mag->zone_info[i].acq_fence_fd,
            pzone_mag->zone_info[i].LayerName);
            switch(pzone_mag->zone_info[i].dispatched) {
                case win0:
                    bpvinfo.vopinfo[0].state = 1;
                    bpvinfo.vopinfo[0].zone_num ++;                
                   break;
                case win1:
                    bpvinfo.vopinfo[1].state = 1;
                    bpvinfo.vopinfo[1].zone_num ++;                            
                    break;
                case win2_0:   
                    bpvinfo.vopinfo[2].state = 1;
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_1:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;  
                case win2_2:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_3:
                    bpvinfo.vopinfo[2].zone_num ++;                                                    
                    break;
                case win3_0:
                    bpvinfo.vopinfo[3].state = 1;
                    bpvinfo.vopinfo[3].zone_num ++;                                                    
                    break;
                case win3_1:
                    bpvinfo.vopinfo[3].zone_num ++;                                                                
                    break;   
                case win3_2:
                    bpvinfo.vopinfo[3].zone_num ++;                                                                
                    break;
                case win3_3:
                    bpvinfo.vopinfo[3].zone_num ++;                                                                
                    break;                 
                 case win_ext:
                    break;
                default:
                    ALOGE("hwc_dispatch  err!");
                    return -1;
             }    
        }     
        bpvinfo.bp_size = Context->zone_manager.bp_size;
        bpvinfo.bp_vop_size = Context->zone_manager.bp_size;    
        for(i= 0;i<4;i++)
        {
            ALOGD_IF(mLogL&HLLFIV,"RK_QUEDDR_FREQ info win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
        }    
        if(ioctl(Context->ddrFd, RK_QUEDDR_FREQ, &bpvinfo))
        {
            if(mLogL&HLLTHR)
            {
                for(i= 0;i<4;i++)
                {
                    ALOGD("RK_QUEDDR_FREQ info win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                        i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
                }    
            }    
            return -1;    
        }
    }
#endif    
    if((large_cnt + bw ) > 5 )
    {
        ALOGD_IF(mLogL&HLLTHR,"data too large ,lcdc not support");
        return -1;
    }
    memcpy(&Context->zone_manager,&zone_m,sizeof(ZoneManager));
    Context->zone_manager.mCmpType = HWC_HOR;
    Context->zone_manager.composter_mode = HWC_LCDC;
    return 0;
}

int try_wins_dispatch_mix_cross(void * ctx,hwc_display_contents_1_t * list)
{
    int win_disphed_flag[3] = {0,}; // win0, win1, win2, win3 flag which is dispatched
    int win_disphed[3] = {win0,win1,win2_0};
    int i,j;
    int cntfb = 0;
    hwcContext * Context = (hwcContext *)ctx;
    ZoneManager zone_m;
    memcpy(&zone_m,&Context->zone_manager,sizeof(ZoneManager));
    ZoneManager* pzone_mag = &zone_m;
    ZoneInfo    zone_info_ty[MaxZones];
    int sort = 1;
    int cnt = 0;
    int srot_tal[3][2] = {0,};
    int sort_stretch[3] = {0};
    int sort_pre;
    int gpu_draw = 0;
    float hfactor_max = 1.0;
    int large_cnt = 0;
    bool isyuv = false;
    int bw = 0;
    BpVopInfo  bpvinfo;
    int tsize = 0;
    int mix_index = 0;
    int iFirstTransformLayer=-1;
    int foundLayer = 0;
    bool intersect = false;
    bool bTransform=false;
    mix_info gMixInfo;

    return -1;
    memset(&bpvinfo,0,sizeof(BpVopInfo));
    char const* compositionTypeName[] = {
            "win0",
            "win1",
            "win2_0",
            "win2_1",
            "win2_2",
            "win2_3",
            "win3_0",
            "win3_1",
            "win3_2",
            "win3_3",
            };
    hwcContext * contextAh = _contextAnchor;
    memset(&zone_info_ty,0,sizeof(zone_info_ty));
    if(Context == _contextAnchor1){
        mix_index = 1;
    }else if(Context == _contextAnchor){
        mix_index = 0;
    }
    if(list->numHwLayers - 1 < 5){
        return -1;
    }

    if(Context->mAlphaError){
        return -1;
    }

    if(contextAh->mHdmiSI.NeedReDst){
        return -1;
    }

#ifdef RK3288_BOX
    if(Context==_contextAnchor && Context->mResolutionChanged && Context->mLcdcNum==2){
        return -1;
    }
#endif

    if(Context->Is3D){
        return -1;
    }

    for(int k=1;k<pzone_mag->zone_cnt;k++){
        if(pzone_mag->zone_info[foundLayer].glesPixels <= pzone_mag->zone_info[k].glesPixels){
            foundLayer = k;
        }
    }

    for(int k=foundLayer+1;k<pzone_mag->zone_cnt;k++){
        if(is_x_intersect(&(pzone_mag->zone_info[foundLayer].disp_rect),&(pzone_mag->zone_info[k].disp_rect))){
            intersect = true;
            return -1;
        }
    }

    memcpy((void*)&gMixInfo,(void*)&gmixinfo[mix_index],sizeof(gMixInfo));
    for(i=0,j=0;i<pzone_mag->zone_cnt;i++){
        //Set the layer which it's layer_index bigger than the first transform layer index to HWC_FRAMEBUFFER or HWC_NODRAW
        if(pzone_mag->zone_info[i].layer_index > 1 && pzone_mag->zone_info[i].layer_index != foundLayer){
            hwc_layer_1_t * layer = &list->hwLayers[pzone_mag->zone_info[i].layer_index];
            if(pzone_mag->zone_info[i].layer_index > 1 && pzone_mag->zone_info[i].layer_index != foundLayer){
                for(int j=2;j<pzone_mag->zone_cnt;j++){
                    layer = &list->hwLayers[j];
                    layer->compositionType = HWC_FRAMEBUFFER;
                }
            }
            cntfb ++;
        }else{
            memcpy(&zone_info_ty[j], &pzone_mag->zone_info[i],sizeof(ZoneInfo));
            zone_info_ty[j].sort = 0;
            j++;
        }
    }
    memcpy(pzone_mag,zone_info_ty,sizeof(zone_info_ty));
    pzone_mag->zone_cnt -= cntfb;
    for(i=0;i< pzone_mag->zone_cnt;i++)
    {
        ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],acq_fence_fd=%d,"
            "layname=%s",
            Context->zone_manager.zone_info[i].zone_index,
            Context->zone_manager.zone_info[i].layer_index,
            Context->zone_manager.zone_info[i].src_rect.left,
            Context->zone_manager.zone_info[i].src_rect.top,
            Context->zone_manager.zone_info[i].src_rect.right,
            Context->zone_manager.zone_info[i].src_rect.bottom,
            Context->zone_manager.zone_info[i].disp_rect.left,
            Context->zone_manager.zone_info[i].disp_rect.top,
            Context->zone_manager.zone_info[i].disp_rect.right,
            Context->zone_manager.zone_info[i].disp_rect.bottom,
            Context->zone_manager.zone_info[i].width,
            Context->zone_manager.zone_info[i].height,
            Context->zone_manager.zone_info[i].stride,
            Context->zone_manager.zone_info[i].format,
            Context->zone_manager.zone_info[i].transform,
            Context->zone_manager.zone_info[i].realtransform,
            Context->zone_manager.zone_info[i].blend,
            Context->zone_manager.zone_info[i].acq_fence_fd,
            Context->zone_manager.zone_info[i].LayerName);
    }
    pzone_mag->zone_info[0].sort = sort;
    for(i=0;i<(pzone_mag->zone_cnt-1);)
    {
        pzone_mag->zone_info[i].sort = sort;
        sort_pre  = sort;
        cnt = 0;
        for(j=1;j<4 && (i+j) < pzone_mag->zone_cnt;j++)
        {
            ZoneInfo * next_zf = &(pzone_mag->zone_info[i+j]);
            bool is_combine = false;
            int k;
            for(k=0;k<=cnt;k++)  // compare all sorted_zone info
            {
                ZoneInfo * sorted_zf = &(pzone_mag->zone_info[i+j-1-k]);
                if(is_zone_combine(sorted_zf,next_zf))
                {
                    is_combine = true;
                }
                else
                {
                    is_combine = false;
                    break;
                }
            }
            if(is_combine)
            {
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;
            }
            else
            {
                sort++;
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;
                break;
            }
        }
        if( sort_pre == sort && (i+cnt) < (pzone_mag->zone_cnt-1) )  // win2 ,4zones ,win3 4zones,so sort ++,but exit not ++
            sort ++;
        i += cnt;
    }
    if(sort >3)  // lcdc dont support 5 wins
    {
        ALOGD_IF(mLogL&HLLTHR,"lcdc dont support 5 wins sort=%d",sort);
        return -1;
    }
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
        int factor =1;
        ALOGV("sort[%d].type=%d",i,pzone_mag->zone_info[i].sort);
        if( pzone_mag->zone_info[i].sort == 1){
            srot_tal[0][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[0] = 1;
        }
        else if(pzone_mag->zone_info[i].sort == 2){
            srot_tal[1][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[1] = 1;
        }
        else if(pzone_mag->zone_info[i].sort == 3){
            srot_tal[2][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[2] = 1;
        }
        if(pzone_mag->zone_info[i].hfactor > hfactor_max)
        {
            hfactor_max = pzone_mag->zone_info[i].hfactor;
        }
        if(pzone_mag->zone_info[i].is_large )
        {
            large_cnt ++;
        }
        if(pzone_mag->zone_info[i].format== HAL_PIXEL_FORMAT_YCrCb_NV12)
        {
            isyuv = true;
        }
        if(Context->zone_manager.zone_info[i].hfactor > 1.0)
            factor = 2;
        else
            factor = 1;
        tsize += (Context->zone_manager.zone_info[i].size *factor);
    }
    j = 0;
    for(i=0;i<3;i++)
    {
        if( srot_tal[i][0] >=2)  // > twice zones
        {
            srot_tal[i][1] = win_disphed[j+2];
            win_disphed_flag[j+2] = 1; // win2 ,win3 is dispatch flag
            ALOGV("more twice zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 1)  // lcdc only has win2 and win3 supprot more zones
            {
                ALOGD("lcdc only has win2 and win3 supprot more zones");
                return -1;
            }
        }
    }
    j = 0;
    for(i=0;i<3;i++)
    {
        if( sort_stretch[i] == 1)  // strech
        {
            srot_tal[i][1] = win_disphed[j];  // win 0 and win 1 suporot stretch
            win_disphed_flag[j] = 1; // win0 ,win1 is dispatch flag
            ALOGV("stretch zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 2)  // lcdc only has win0 and win1 supprot stretch
            {
                ALOGD_IF(mLogL&HLLFIV,"lcdc only has win0 and win1 supprot stretch");
                return -1;
            }
        }
    }
    if(hfactor_max >=1.4)
    {
        bw += (j + 1);
    }
    if(isyuv)
    {
        bw +=5;
    }
    ALOGV("large_cnt =%d,bw=%d",large_cnt , bw);

    for(i=0;i<3;i++)
    {
        if( srot_tal[i][1] == 0)  // had not dispatched
        {
            for(j=0;j<3;j++)
            {
                if(win_disphed_flag[j] == 0) // find the win had not dispatched
                    break;
            }
            if(j>=3)
            {
                ALOGE("3 wins had beed dispatched ");
                return -1;
            }
            srot_tal[i][1] = win_disphed[j];
            win_disphed_flag[j] = 1;
            ALOGV("srot_tal[%d][1].dispatched=%d",i,srot_tal[i][1]);
        }
    }

    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
         switch(pzone_mag->zone_info[i].sort) {
            case 1:
                pzone_mag->zone_info[i].dispatched = srot_tal[0][1]++;
                break;
            case 2:
                pzone_mag->zone_info[i].dispatched = srot_tal[1][1]++;
                break;
            case 3:
                pzone_mag->zone_info[i].dispatched = srot_tal[2][1]++;
                break;
            default:
                ALOGE("try_wins_dispatch_mix_vh sort err!");
                return -1;
        }
        ALOGV("zone[%d].dispatched[%d]=%s,sort=%d", \
        i,pzone_mag->zone_info[i].dispatched,
        compositionTypeName[pzone_mag->zone_info[i].dispatched -1],
        pzone_mag->zone_info[i].sort);
    }

    for(i=0;i<pzone_mag->zone_cnt;i++){
        int disptched = pzone_mag->zone_info[i].dispatched;
        int sct_width = pzone_mag->zone_info[i].width;
        int sct_height = pzone_mag->zone_info[i].height;
        /*scal not support whoes source bigger than 2560 to dst 4k*/
        if(disptched <= win1 &&(sct_width > 2160 || sct_height > 2160) &&
            !is_yuv(pzone_mag->zone_info[i].format) && contextAh->mHdmiSI.NeedReDst)
            return -1;
    }

#if USE_QUEUE_DDRFREQ
    if(Context->ddrFd > 0)
    {
        for(i=0;i<pzone_mag->zone_cnt;i++)
        {
            int area_no = 0;
            int win_id = 0;
            ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],dispatched=%d,"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],"
            "layer_fd[%d],addr=%x,acq_fence_fd=%d"
            "layname=%s",
            pzone_mag->zone_info[i].zone_index,
            pzone_mag->zone_info[i].layer_index,
            pzone_mag->zone_info[i].dispatched,
            pzone_mag->zone_info[i].src_rect.left,
            pzone_mag->zone_info[i].src_rect.top,
            pzone_mag->zone_info[i].src_rect.right,
            pzone_mag->zone_info[i].src_rect.bottom,
            pzone_mag->zone_info[i].disp_rect.left,
            pzone_mag->zone_info[i].disp_rect.top,
            pzone_mag->zone_info[i].disp_rect.right,
            pzone_mag->zone_info[i].disp_rect.bottom,
            pzone_mag->zone_info[i].width,
            pzone_mag->zone_info[i].height,
            pzone_mag->zone_info[i].stride,
            pzone_mag->zone_info[i].format,
            pzone_mag->zone_info[i].transform,
            pzone_mag->zone_info[i].realtransform,
            pzone_mag->zone_info[i].blend,
            pzone_mag->zone_info[i].layer_fd,
            pzone_mag->zone_info[i].addr,
            pzone_mag->zone_info[i].acq_fence_fd,
            pzone_mag->zone_info[i].LayerName);
            switch(pzone_mag->zone_info[i].dispatched) {
                case win0:
                    bpvinfo.vopinfo[0].state = 1;
                    bpvinfo.vopinfo[0].zone_num ++;
                   break;
                case win1:
                    bpvinfo.vopinfo[1].state = 1;
                    bpvinfo.vopinfo[1].zone_num ++;
                    break;
                case win2_0:
                    bpvinfo.vopinfo[2].state = 1;
                    bpvinfo.vopinfo[2].zone_num ++;
                    break;
                case win2_1:
                    bpvinfo.vopinfo[2].zone_num ++;
                    break;
                case win2_2:
                    bpvinfo.vopinfo[2].zone_num ++;
                    break;
                case win2_3:
                    bpvinfo.vopinfo[2].zone_num ++;
                    break;
                default:
                    ALOGE("hwc_dispatch_mix  err!");
                    return -1;
             }
        }
        bpvinfo.vopinfo[3].state = 1;
        bpvinfo.vopinfo[3].zone_num ++;
        bpvinfo.bp_size = Context->zone_manager.bp_size;
        tsize += Context->fbhandle.width * Context->fbhandle.height*4;
        if(tsize)
            tsize = tsize / (1024 *1024) * 60 ;// MB
        bpvinfo.bp_vop_size = tsize ;
        for(i= 0;i<4;i++)
        {
            ALOGD_IF(mLogL&HLLTHR,"RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
        }
        if(ioctl(Context->ddrFd, RK_QUEDDR_FREQ, &bpvinfo))
        {
            if(mLogL&HLLTHR)
            {
                for(i= 0;i<4;i++)
                {
                    ALOGD("RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                        i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
                }
            }
            return -1;
        }
    }
#endif
    //Mark the composer mode to HWC_MIX
    if(list){
        list->hwLayers[0].compositionType = HWC_MIX_V2;
        list->hwLayers[1].compositionType = HWC_MIX_V2;
        list->hwLayers[foundLayer].compositionType = HWC_MIX_V2;
    }
    memcpy(&Context->zone_manager,&zone_m,sizeof(ZoneManager));
    Context->zone_manager.mCmpType = HWC_MIX_CROSS;
    Context->zone_manager.composter_mode = HWC_MIX_V2;
    memcpy((void*)&gmixinfo[mix_index],(void*)&gMixInfo,sizeof(gMixInfo));
    return 0;
}


int try_wins_dispatch_mix_up(void * ctx,hwc_display_contents_1_t * list)
{
    int win_disphed_flag[3] = {0,}; // win0, win1, win2, win3 flag which is dispatched
    int win_disphed[3] = {win0,win1,win2_0};
    int i,j;
    int cntfb = 0;
    hwcContext * Context = (hwcContext *)ctx;
    ZoneManager zone_m;
    memcpy(&zone_m,&Context->zone_manager,sizeof(ZoneManager));
    ZoneManager* pzone_mag = &zone_m;
    ZoneInfo    zone_info_ty[MaxZones];
    int sort = 1;
    int cnt = 0;
    int srot_tal[3][2] = {0,};
    int sort_stretch[3] = {0}; 
    int sort_pre;
    int gpu_draw = 0;
    float hfactor_max = 1.0;
    int large_cnt = 0;
    bool isyuv = false;
    int bw = 0;
    BpVopInfo  bpvinfo;    
    int tsize = 0;
    int mix_index = 0;
    int iFirstTransformLayer=-1;
    bool bTransform=false;
    mix_info gMixInfo;
    
    memset(&bpvinfo,0,sizeof(BpVopInfo));
    char const* compositionTypeName[] = {
            "win0",
            "win1",
            "win2_0",
            "win2_1",
            "win2_2",
            "win2_3",
            "win3_0",
            "win3_1",
            "win3_2",
            "win3_3",
            };
    hwcContext * contextAh = _contextAnchor;
    memset(&zone_info_ty,0,sizeof(zone_info_ty));
    if(Context == _contextAnchor1){
        mix_index = 1;
    }else if(Context == _contextAnchor){
        mix_index = 0;
    }
    if(list->numHwLayers - 1 < 3){
    	return -1;
    }

    if(Context->mAlphaError){
        return -1;
    }

    if(contextAh->mHdmiSI.NeedReDst){
        return -1;
    }
    
#ifdef RK3288_BOX
    if(Context==_contextAnchor && Context->mResolutionChanged && Context->mLcdcNum==2){
        return -1;
    }
#endif

    if(Context->Is3D && 
    ((!pzone_mag->zone_info[0].alreadyStereo && pzone_mag->zone_info[0].displayStereo)||
    (!pzone_mag->zone_info[1].alreadyStereo && pzone_mag->zone_info[1].displayStereo))){
        return -1;
    }

    for(int k=0;k<2;k++)
    {
        if(pzone_mag->zone_info[k].scale_err || pzone_mag->zone_info[k].toosmall
            || pzone_mag->zone_info[k].zone_err || (pzone_mag->zone_info[k].transform
                && pzone_mag->zone_info[k].format != HAL_PIXEL_FORMAT_YCrCb_NV12 && 0==k)
                    || (pzone_mag->zone_info[k].transform && 1 == k))
            return -1;
    }

    memcpy((void*)&gMixInfo,(void*)&gmixinfo[mix_index],sizeof(gMixInfo));
    for(i=0,j=0;i<pzone_mag->zone_cnt;i++)
    {
        //Set the layer which it's layer_index bigger than the first transform layer index to HWC_FRAMEBUFFER or HWC_NODRAW
        if(pzone_mag->zone_info[i].layer_index > 1)
        {
            hwc_layer_1_t * layer = &list->hwLayers[pzone_mag->zone_info[i].layer_index];
            //Judge the current layer whether backup in gmixinfo[mix_index] or not.
            if(Context->mLastCompType != HWC_MIX_UP
                || gMixInfo.lastZoneCrc[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].zoneCrc
                || gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].layer_fd
                || gMixInfo.alpha[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].zone_alpha) {
                gpu_draw = 1;
                layer->compositionType = HWC_FRAMEBUFFER;
                gMixInfo.lastZoneCrc[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].zoneCrc;
                gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].layer_fd;  
                gMixInfo.alpha[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].zone_alpha;
            }
            else
            {
                layer->compositionType = HWC_NODRAW;
            }
            if(gpu_draw && i == pzone_mag->zone_cnt-1)
            {
                for(int j=1;j<pzone_mag->zone_cnt;j++)
                {
                    layer = &list->hwLayers[j];
                    layer->compositionType = HWC_FRAMEBUFFER;
                }               
                ALOGV(" need draw by gpu");
            }
            cntfb ++;
        }
        else
        {
            memcpy(&zone_info_ty[j], &pzone_mag->zone_info[i],sizeof(ZoneInfo));
            zone_info_ty[j].sort = 0;
            j++;
        }
    }
    memcpy(pzone_mag, &zone_info_ty,sizeof(zone_info_ty));
    pzone_mag->zone_cnt -= cntfb;
    for(i=0;i< pzone_mag->zone_cnt;i++)
    {
        ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],acq_fence_fd=%d,"
            "layname=%s",                        
            Context->zone_manager.zone_info[i].zone_index,
            Context->zone_manager.zone_info[i].layer_index,
            Context->zone_manager.zone_info[i].src_rect.left,
            Context->zone_manager.zone_info[i].src_rect.top,
            Context->zone_manager.zone_info[i].src_rect.right,
            Context->zone_manager.zone_info[i].src_rect.bottom,
            Context->zone_manager.zone_info[i].disp_rect.left,
            Context->zone_manager.zone_info[i].disp_rect.top,
            Context->zone_manager.zone_info[i].disp_rect.right,
            Context->zone_manager.zone_info[i].disp_rect.bottom,
            Context->zone_manager.zone_info[i].width,
            Context->zone_manager.zone_info[i].height,
            Context->zone_manager.zone_info[i].stride,
            Context->zone_manager.zone_info[i].format,
            Context->zone_manager.zone_info[i].transform,
            Context->zone_manager.zone_info[i].realtransform,
            Context->zone_manager.zone_info[i].blend,
            Context->zone_manager.zone_info[i].acq_fence_fd,
            Context->zone_manager.zone_info[i].LayerName);
    }
    pzone_mag->zone_info[0].sort = sort;
    for(i=0;i<(pzone_mag->zone_cnt-1);)
    {
        pzone_mag->zone_info[i].sort = sort;
        sort_pre  = sort;
        cnt = 0;
        for(j=1;j<4 && (i+j) < pzone_mag->zone_cnt;j++)
        {
            ZoneInfo * next_zf = &(pzone_mag->zone_info[i+j]);
            bool is_combine = false;
            int k;
            for(k=0;k<=cnt;k++)  // compare all sorted_zone info
            {
                ZoneInfo * sorted_zf = &(pzone_mag->zone_info[i+j-1-k]);
                if(is_zone_combine(sorted_zf,next_zf))
                {
                    is_combine = true;
                }
                else
                {
                    is_combine = false;
                    break;
                }
            }
            if(is_combine)
            {
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;                
            }
            else
            {
                sort++;
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;                
                break;
            }
        }
        if( sort_pre == sort && (i+cnt) < (pzone_mag->zone_cnt-1) )  // win2 ,4zones ,win3 4zones,so sort ++,but exit not ++
            sort ++;
        i += cnt;  
    }
    if(sort >3)  // lcdc dont support 5 wins
    {
        ALOGD_IF(mLogL&HLLTHR,"lcdc dont support 5 wins sort=%d",sort);
        return -1;
    }    
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
        int factor =1;
        ALOGV("sort[%d].type=%d",i,pzone_mag->zone_info[i].sort);
        if( pzone_mag->zone_info[i].sort == 1){
            srot_tal[0][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[0] = 1;
        }    
        else if(pzone_mag->zone_info[i].sort == 2){
            srot_tal[1][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[1] = 1;
        }    
        else if(pzone_mag->zone_info[i].sort == 3){
            srot_tal[2][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[2] = 1;
        }    
        if(pzone_mag->zone_info[i].hfactor > hfactor_max)
        {
            hfactor_max = pzone_mag->zone_info[i].hfactor;
        }
        if(pzone_mag->zone_info[i].is_large )
        {
            large_cnt ++;
        }
        if(pzone_mag->zone_info[i].format== HAL_PIXEL_FORMAT_YCrCb_NV12)
        {
            isyuv = true;
        }
        if(Context->zone_manager.zone_info[i].hfactor > 1.0)
            factor = 2;
        else
            factor = 1;        
        tsize += (Context->zone_manager.zone_info[i].size *factor);
    }
    j = 0;
    for(i=0;i<3;i++)    
    {        
        if( srot_tal[i][0] >=2)  // > twice zones
        {
            srot_tal[i][1] = win_disphed[j+2]; 
            win_disphed_flag[j+2] = 1; // win2 ,win3 is dispatch flag
            ALOGV("more twice zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 1)  // lcdc only has win2 and win3 supprot more zones
            {
                ALOGD("lcdc only has win2 and win3 supprot more zones");
                return -1;  
            }
        }
    }
    j = 0;
    for(i=0;i<3;i++)    
    {        
        if( sort_stretch[i] == 1)  // strech
        {
            srot_tal[i][1] = win_disphed[j];  // win 0 and win 1 suporot stretch
            win_disphed_flag[j] = 1; // win0 ,win1 is dispatch flag
            ALOGV("stretch zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 2)  // lcdc only has win0 and win1 supprot stretch
            {
                ALOGD_IF(mLogL&HLLFIV,"lcdc only has win0 and win1 supprot stretch");
                return -1;  
            }
        }
    }
    if(hfactor_max >=1.4)
    {
        bw += (j + 1);
        
    }
    if(isyuv)
    {
        bw +=5;
    }
    ALOGV("large_cnt =%d,bw=%d",large_cnt , bw);
  
    for(i=0;i<3;i++)    
    {        
        if( srot_tal[i][1] == 0)  // had not dispatched
        {
            for(j=0;j<3;j++)
            {
                if(win_disphed_flag[j] == 0) // find the win had not dispatched
                    break;
            }  
            if(j>=3)
            {
                ALOGE("3 wins had beed dispatched ");
                return -1;
            }    
            srot_tal[i][1] = win_disphed[j];
            win_disphed_flag[j] = 1;
            ALOGV("srot_tal[%d][1].dispatched=%d",i,srot_tal[i][1]);
        }
    }
       
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {        
         switch(pzone_mag->zone_info[i].sort) {
            case 1:
                pzone_mag->zone_info[i].dispatched = srot_tal[0][1]++;
                break;
            case 2:
                pzone_mag->zone_info[i].dispatched = srot_tal[1][1]++;            
                break;
            case 3:
                pzone_mag->zone_info[i].dispatched = srot_tal[2][1]++;            
                break;
            default:
                ALOGE("try_wins_dispatch_mix_vh sort err!");
                return -1;
        }
        ALOGV("zone[%d].dispatched[%d]=%s,sort=%d", \
        i,pzone_mag->zone_info[i].dispatched,
        compositionTypeName[pzone_mag->zone_info[i].dispatched -1],
        pzone_mag->zone_info[i].sort);
    }
    
    for(i=0;i<pzone_mag->zone_cnt;i++){
        int disptched = pzone_mag->zone_info[i].dispatched;
        int sct_width = pzone_mag->zone_info[i].width;
        int sct_height = pzone_mag->zone_info[i].height;
        /*scal not support whoes source bigger than 2560 to dst 4k*/
        if(disptched <= win1 &&(sct_width > 2160 || sct_height > 2160) &&
            !is_yuv(pzone_mag->zone_info[i].format) && contextAh->mHdmiSI.NeedReDst)
            return -1;
    }
        
#if USE_QUEUE_DDRFREQ
    if(Context->ddrFd > 0)
    {
        for(i=0;i<pzone_mag->zone_cnt;i++)
        {
            int area_no = 0;
            int win_id = 0;
            ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],dispatched=%d,"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],"
            "layer_fd[%d],addr=%x,acq_fence_fd=%d"
            "layname=%s",    
            pzone_mag->zone_info[i].zone_index,
            pzone_mag->zone_info[i].layer_index,
            pzone_mag->zone_info[i].dispatched,
            pzone_mag->zone_info[i].src_rect.left,
            pzone_mag->zone_info[i].src_rect.top,
            pzone_mag->zone_info[i].src_rect.right,
            pzone_mag->zone_info[i].src_rect.bottom,
            pzone_mag->zone_info[i].disp_rect.left,
            pzone_mag->zone_info[i].disp_rect.top,
            pzone_mag->zone_info[i].disp_rect.right,
            pzone_mag->zone_info[i].disp_rect.bottom,
            pzone_mag->zone_info[i].width,
            pzone_mag->zone_info[i].height,
            pzone_mag->zone_info[i].stride,
            pzone_mag->zone_info[i].format,
            pzone_mag->zone_info[i].transform,
            pzone_mag->zone_info[i].realtransform,
            pzone_mag->zone_info[i].blend,
            pzone_mag->zone_info[i].layer_fd,
            pzone_mag->zone_info[i].addr,
            pzone_mag->zone_info[i].acq_fence_fd,
            pzone_mag->zone_info[i].LayerName);
            switch(pzone_mag->zone_info[i].dispatched) {
                case win0:
                    bpvinfo.vopinfo[0].state = 1;
                    bpvinfo.vopinfo[0].zone_num ++;                
                   break;
                case win1:
                    bpvinfo.vopinfo[1].state = 1;
                    bpvinfo.vopinfo[1].zone_num ++;                            
                    break;
                case win2_0:   
                    bpvinfo.vopinfo[2].state = 1;
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_1:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;  
                case win2_2:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_3:
                    bpvinfo.vopinfo[2].zone_num ++;                                                    
                    break;           
                default:
                    ALOGE("hwc_dispatch_mix  err!");
                    return -1;
             }    
        }  
        bpvinfo.vopinfo[3].state = 1;
        bpvinfo.vopinfo[3].zone_num ++;                                        
        bpvinfo.bp_size = Context->zone_manager.bp_size;
        tsize += Context->fbhandle.width * Context->fbhandle.height*4;
        if(tsize)
            tsize = tsize / (1024 *1024) * 60 ;// MB
        bpvinfo.bp_vop_size = tsize ;  
        for(i= 0;i<4;i++)
        {
            ALOGD_IF(mLogL&HLLTHR,"RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
        }    
        if(ioctl(Context->ddrFd, RK_QUEDDR_FREQ, &bpvinfo))
        {
            if(mLogL&HLLTHR)
            {
                for(i= 0;i<4;i++)
                {
                    ALOGD("RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                        i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
                }    
            }    
            return -1;    
        }
    }
#endif   
    //Mark the composer mode to HWC_MIX
    if(list){
        list->hwLayers[0].compositionType = HWC_MIX_V2;
        list->hwLayers[1].compositionType = HWC_MIX_V2;
    }
    memcpy(&Context->zone_manager,&zone_m,sizeof(ZoneManager));
    Context->mHdmiSI.mix_up = true;
    Context->zone_manager.mCmpType = HWC_MIX_UP;
    Context->zone_manager.composter_mode = HWC_MIX;
    memcpy((void*)&gmixinfo[mix_index],(void*)&gMixInfo,sizeof(gMixInfo));
    return 0;    
}

int try_wins_dispatch_mix_down(void * ctx,hwc_display_contents_1_t * list)
{
    int win_disphed_flag[3] = {0,}; // win0, win1, win2, win3 flag which is dispatched
    int win_disphed[3] = {win0,win1,win2_0};
    int i,j;
    int cntfb = 0;
    int foundLayer = 1;
    hwcContext * Context = (hwcContext *)ctx;
    ZoneManager zone_m;
    memcpy(&zone_m,&Context->zone_manager,sizeof(ZoneManager));
    ZoneManager* pzone_mag = &zone_m;

    ZoneInfo    zone_info_ty[MaxZones];
    int sort = 1;
    int cnt = 0;
    int srot_tal[3][2] = {0,};
    int sort_stretch[3] = {0}; 
    int sort_pre;
    int gpu_draw = 0;
    float hfactor_max = 1.0;
    int large_cnt = 0;
    bool isyuv = false;
    int bw = 0;
    BpVopInfo  bpvinfo;    
    int tsize = 0; 
    int mix_index = 0;
    mix_info gMixInfo;
    memset(&bpvinfo,0,sizeof(BpVopInfo));
    char const* compositionTypeName[] = {
            "win0",
            "win1",
            "win2_0",
            "win2_1",
            "win2_2",
            "win2_3",
            "win3_0",
            "win3_1",
            "win3_2",
            "win3_3",
            };
    hwcContext * contextAh = _contextAnchor;
    memset(&zone_info_ty,0,sizeof(zone_info_ty));
    if(pzone_mag->zone_cnt < 5 && !Context->mMultiwindow) {
        return -1;
    }
    if(Context == _contextAnchor1) {
        mix_index = 1;
    } else if(Context == _contextAnchor) {
        mix_index = 0;
    }
#if OPTIMIZATION_FOR_TRANSFORM_UI
    //ignore transform ui layer case.
    for(i=0;i<pzone_mag->zone_cnt;i++) {
        if((pzone_mag->zone_info[i].transform != 0)&&
            (pzone_mag->zone_info[i].format != HAL_PIXEL_FORMAT_YCrCb_NV12)
#if 1
            && (Context->mtrsformcnt!=1 || (Context->mtrsformcnt==1 && pzone_mag->zone_cnt>2))
#else //ENABLE_TRANSFORM_BY_RGA
            && ((Context->mtrsformcnt!=1)
            || !strstr(pzone_mag->zone_info[i].LayerName,"Starting@# "))
#endif
            ) {
                ALOGD_IF(mLogL&HLLFOU,"Policy out %s,%d ",__FUNCTION__,__LINE__);
                return -1;
            }
    }
#endif

    if(Context->Is3D){
        return -1;
    }

    if(contextAh->mHdmiSI.NeedReDst){
        return -1;
    }

TryAgain:
    sort = 1;
    cntfb = 0;
    foundLayer++;
    if(!Context->mMultiwindow && foundLayer>2) {
        ALOGD_IF(mLogL&HLLFOU,"Policy out %s,%d",__FUNCTION__,__LINE__);
        return -1;
    } else if(Context->mMultiwindow) {
        bw = 0;
        tsize = 0;
        isyuv = false;
        large_cnt = 0;
        memset((void*)srot_tal,0,sizeof(srot_tal));
        memset((void*)sort_stretch,0,sizeof(sort_stretch));
        memset((void*)win_disphed_flag,0,sizeof(win_disphed_flag));
        memcpy(&zone_m,&Context->zone_manager,sizeof(ZoneManager));
    }
    if(foundLayer > pzone_mag->zone_cnt - 1) {
        ALOGD_IF(mLogL&HLLFOU,"Policy out %s,%d",__FUNCTION__,__LINE__);
        return -1;
    }

    for(int k=foundLayer;k<pzone_mag->zone_cnt;k++) {
        if(pzone_mag->zone_info[k].scale_err || pzone_mag->zone_info[k].toosmall
            || pzone_mag->zone_info[k].zone_err || pzone_mag->zone_info[k].transform) {
            ALOGD_IF(mLogL&HLLFOU,"Policy out %s,%d ",__FUNCTION__,__LINE__);
            return -1;
        }
    }

    memcpy((void*)&gMixInfo,(void*)&gmixinfo[mix_index],sizeof(gMixInfo));
    for(i=0,j=0;i<pzone_mag->zone_cnt;i++) {
        if(pzone_mag->zone_info[i].layer_index < foundLayer) {
            hwc_layer_1_t * layer = &list->hwLayers[pzone_mag->zone_info[i].layer_index];
            if(pzone_mag->zone_info[i].format == HAL_PIXEL_FORMAT_YCrCb_NV12) {
                ALOGD_IF(mLogL&HLLFOU,"Policy out Donot support video ");
                return -1;
            }    
            if(Context->mLastCompType != HWC_MIX_DOWN
                || gMixInfo.lastZoneCrc[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].zoneCrc
                || gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].layer_fd
                || gMixInfo.alpha[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].zone_alpha) {
            	ALOGV("bk fd=%d,cur fd=%d;bk alpha=%x,cur alpha=%x,i=%d,layer_index=%d",gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index], \
            	pzone_mag->zone_info[i].layer_fd,gMixInfo.alpha[pzone_mag->zone_info[i].layer_index],\
            	pzone_mag->zone_info[i].zone_alpha, i,pzone_mag->zone_info[i].layer_index);
                gpu_draw = 1;
                layer->compositionType = HWC_FRAMEBUFFER;
                gMixInfo.lastZoneCrc[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].zoneCrc;
                gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].layer_fd;  
                gMixInfo.alpha[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].zone_alpha;
            } else {
                layer->compositionType = HWC_NODRAW;
            }

            if(gpu_draw && pzone_mag->zone_info[i].layer_index == foundLayer - 1){
                for(int i = 0;i < foundLayer ;i++){
                    layer = &list->hwLayers[i];
                    layer->compositionType = HWC_FRAMEBUFFER;
                    ALOGV(" need draw by gpu");
                }
            }
            cntfb ++;
        } else {
           memcpy(&zone_info_ty[j], &pzone_mag->zone_info[i],sizeof(ZoneInfo));
           zone_info_ty[j].sort = 0;
           j++;
        }
    }
    memcpy(pzone_mag, &zone_info_ty,sizeof(zone_info_ty));
    pzone_mag->zone_cnt -= cntfb;
    for(i=0;i< pzone_mag->zone_cnt;i++) {
        ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],acq_fence_fd=%d,"
            "layname=%s",                        
            pzone_mag->zone_info[i].zone_index,
            pzone_mag->zone_info[i].layer_index,
            pzone_mag->zone_info[i].src_rect.left,
            pzone_mag->zone_info[i].src_rect.top,
            pzone_mag->zone_info[i].src_rect.right,
            pzone_mag->zone_info[i].src_rect.bottom,
            pzone_mag->zone_info[i].disp_rect.left,
            pzone_mag->zone_info[i].disp_rect.top,
            pzone_mag->zone_info[i].disp_rect.right,
            pzone_mag->zone_info[i].disp_rect.bottom,
            pzone_mag->zone_info[i].width,
            pzone_mag->zone_info[i].height,
            pzone_mag->zone_info[i].stride,
            pzone_mag->zone_info[i].format,
            pzone_mag->zone_info[i].transform,
            pzone_mag->zone_info[i].realtransform,
            pzone_mag->zone_info[i].blend,
            pzone_mag->zone_info[i].acq_fence_fd,
            pzone_mag->zone_info[i].LayerName);
    }
    pzone_mag->zone_info[0].sort = sort;
    for(i=0;i<(pzone_mag->zone_cnt-1);) {
        pzone_mag->zone_info[i].sort = sort;
        sort_pre  = sort;
        cnt = 0;
        for(j=1;j<4 && (i+j) < pzone_mag->zone_cnt;j++) {
            ZoneInfo * next_zf = &(pzone_mag->zone_info[i+j]);
            bool is_combine = false;
            int k;
            for(k=0;k<=cnt;k++) {
                ZoneInfo * sorted_zf = &(pzone_mag->zone_info[i+j-1-k]);
                if(is_zone_combine(sorted_zf,next_zf)) {
                    is_combine = true;
                } else {
                    is_combine = false;
                    break;
                }
            }
            if(is_combine) {
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;
            } else {
                sort++;
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;                
                break;
            }
        }
        if( sort_pre == sort && (i+cnt) < (pzone_mag->zone_cnt-1) ) {
            sort ++;
        }
        i += cnt;  
    }
    if(sort >3) {
        ALOGD_IF(mLogL&HLLTHR,"lcdc dont support 5 wins sort=%d",sort);
        goto TryAgain;
    }
    int count = sort;
    for(i=0;i<pzone_mag->zone_cnt;i++) {
        int factor =1;
        ALOGV("sort[%d].type=%d",i,pzone_mag->zone_info[i].sort);
        if( pzone_mag->zone_info[i].sort == 1) {
            srot_tal[0][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[0] = 1;
        } else if (pzone_mag->zone_info[i].sort == 2) {
            srot_tal[1][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[1] = 1;
        } else if (pzone_mag->zone_info[i].sort == 3) {
            srot_tal[2][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[2] = 1;
        }    
        if(pzone_mag->zone_info[i].hfactor > hfactor_max) {
            hfactor_max = pzone_mag->zone_info[i].hfactor;
        }
        if(pzone_mag->zone_info[i].is_large ) {
            large_cnt ++;
        }
        if(pzone_mag->zone_info[i].format== HAL_PIXEL_FORMAT_YCrCb_NV12) {
            isyuv = true;
        }
        if(Context->zone_manager.zone_info[i].hfactor > 1.0){
            factor = 2;
        } else {
            factor = 1;        
        }
        tsize += (Context->zone_manager.zone_info[i].size *factor);
    }
    j = 0;
    for(i=0;i<count;i++) {        
        if( srot_tal[i][0] >=2) {
            srot_tal[i][1] = win_disphed[j+2]; 
            win_disphed_flag[j+2] = 1; // win2 ,win3 is dispatch flag
            ALOGV("more twice zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 1) {
                ALOGD_IF(mLogL&HLLFOU,"Policy try again %s,%d ",__FUNCTION__,__LINE__);
                goto TryAgain; 
            }
        }
    }
    j = 0;
    for(i=0;i<count;i++) {        
        if( sort_stretch[i] == 1) {
            srot_tal[i][1] = win_disphed[j];  // win 0 and win 1 suporot stretch
            win_disphed_flag[j] = 1; // win0 ,win1 is dispatch flag
            ALOGV("stretch zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 2) {
                ALOGD_IF(mLogL&HLLFOU,"Policy try again %s,%d ",__FUNCTION__,__LINE__);
                goto TryAgain;
            }
        }
    }
    if(hfactor_max >=1.4) {
        bw += (j + 1);
        
    }
    if(isyuv) {
        bw +=5;
    }
    //ALOGD("large_cnt =%d,bw=%d",large_cnt , bw);
  
    for(i=0;i<count;i++) {
        if( srot_tal[i][1] == 0) {
            for(j=0;j<count;j++) {
                if(win_disphed_flag[j] == 0) // find the win had not dispatched
                    break;
            }  
            if(j>=count) {
                ALOGD_IF(mLogL&HLLFOU,"Policy try again %s,%d ",__FUNCTION__,__LINE__);
                goto TryAgain;
            }    
            srot_tal[i][1] = win_disphed[j];
            win_disphed_flag[j] = 1;
            ALOGV("srot_tal[%d][1].dispatched=%d",i,srot_tal[i][1]);
        }
    }
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {        
         switch(pzone_mag->zone_info[i].sort) {
            case 1:
                pzone_mag->zone_info[i].dispatched = srot_tal[0][1]++;
                break;
            case 2:
                pzone_mag->zone_info[i].dispatched = srot_tal[1][1]++;            
                break;
            case 3:
                pzone_mag->zone_info[i].dispatched = srot_tal[2][1]++;            
                break;
            default:
                ALOGE("try_wins_dispatch_mix sort err!");
                return -1;
        }
        ALOGV("zone[%d].dispatched[%d]=%s,sort=%d", \
        i,pzone_mag->zone_info[i].dispatched,
        compositionTypeName[pzone_mag->zone_info[i].dispatched -1],
        pzone_mag->zone_info[i].sort);
    }

    for(i=0;i<pzone_mag->zone_cnt;i++){
        int disptched = pzone_mag->zone_info[i].dispatched;
        /*win2 win3 not support YUV*/
        if(disptched > win1 && is_yuv(pzone_mag->zone_info[i].format)){
            ALOGD_IF(mLogL&HLLFOU,"Policy try again %s,%d ",__FUNCTION__,__LINE__);
            goto TryAgain;
        }
    }
    
#if USE_QUEUE_DDRFREQ
    if(Context->ddrFd > 0)
    {
        for(i=0;i<pzone_mag->zone_cnt;i++)
        {
            int area_no = 0;
            int win_id = 0;
            ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],dispatched=%d,"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],"
            "layer_fd[%d],addr=%x,acq_fence_fd=%d"
            "layname=%s",    
            pzone_mag->zone_info[i].zone_index,
            pzone_mag->zone_info[i].layer_index,
            pzone_mag->zone_info[i].dispatched,
            pzone_mag->zone_info[i].src_rect.left,
            pzone_mag->zone_info[i].src_rect.top,
            pzone_mag->zone_info[i].src_rect.right,
            pzone_mag->zone_info[i].src_rect.bottom,
            pzone_mag->zone_info[i].disp_rect.left,
            pzone_mag->zone_info[i].disp_rect.top,
            pzone_mag->zone_info[i].disp_rect.right,
            pzone_mag->zone_info[i].disp_rect.bottom,
            pzone_mag->zone_info[i].width,
            pzone_mag->zone_info[i].height,
            pzone_mag->zone_info[i].stride,
            pzone_mag->zone_info[i].format,
            pzone_mag->zone_info[i].transform,
            pzone_mag->zone_info[i].realtransform,
            pzone_mag->zone_info[i].blend,
            pzone_mag->zone_info[i].layer_fd,
            pzone_mag->zone_info[i].addr,
            pzone_mag->zone_info[i].acq_fence_fd,
            pzone_mag->zone_info[i].LayerName);
            switch(pzone_mag->zone_info[i].dispatched) {
                case win0:
                    bpvinfo.vopinfo[0].state = 1;
                    bpvinfo.vopinfo[0].zone_num ++;                
                   break;
                case win1:
                    bpvinfo.vopinfo[1].state = 1;
                    bpvinfo.vopinfo[1].zone_num ++;                            
                    break;
                case win2_0:   
                    bpvinfo.vopinfo[2].state = 1;
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_1:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;  
                case win2_2:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_3:
                    bpvinfo.vopinfo[2].zone_num ++;                                                    
                    break;           
                default:
                    ALOGE("hwc_dispatch_mix  err!");
                    return -1;
             }    
        }  
        bpvinfo.vopinfo[3].state = 1;
        bpvinfo.vopinfo[3].zone_num ++;                                        
        bpvinfo.bp_size = Context->zone_manager.bp_size;
        tsize += Context->fbhandle.width * Context->fbhandle.height*4;
        if(tsize)
            tsize = tsize / (1024 *1024) * 60 ;// MB
        bpvinfo.bp_vop_size = tsize ;  
        for(i= 0;i<4;i++) {
            ALOGD_IF(mLogL&HLLFIV,"RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
        }
        if(ioctl(Context->ddrFd, RK_QUEDDR_FREQ, &bpvinfo)) {
            if(mLogL&HLLTHR) {
                for(i= 0;i<4;i++) {
                    ALOGW("RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                        i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
                }
            }
            ALOGD_IF(mLogL&HLLFOU,"Policy try again %s,%d ",__FUNCTION__,__LINE__);
            memset(&bpvinfo,0,sizeof(BpVopInfo));
            goto TryAgain;
        }
    }
#endif   
    if((large_cnt + bw) >= 5) { 
        ALOGD_IF(mLogL&HLLFOU,"Policy try again %s,%d ",__FUNCTION__,__LINE__);
        goto TryAgain;
    }

    if(gMixInfo.gpu_draw_fd[foundLayer] != 0) {
        for(i=0;i<foundLayer;i++) {
            list->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
        }
        for(i=foundLayer;i<GPUDRAWCNT;i++) {
            gMixInfo.gpu_draw_fd[i] = 0;
        }
    }
    memcpy(&Context->zone_manager,&zone_m,sizeof(ZoneManager));
    Context->zone_manager.mCmpType = HWC_MIX_DOWN;
    Context->zone_manager.composter_mode = HWC_MIX;
    memcpy((void*)&gmixinfo[mix_index],(void*)&gMixInfo,sizeof(gMixInfo));
    return 0;
}

//Refer to try_wins_dispatch_mix to deal with the case which exist ui transform layers.
//Unter the first transform layer,use lcdc to compose,equal or on the top of the transform layer,use gpu to compose
int try_wins_dispatch_mix_v2 (void * ctx,hwc_display_contents_1_t * list)
{
#if OPTIMIZATION_FOR_TRANSFORM_UI
    int win_disphed_flag[3] = {0,}; // win0, win1, win2, win3 flag which is dispatched
    int win_disphed[3] = {win0,win1,win2_0};
    int i,j;
    int cntfb = 0;
    hwcContext * Context = (hwcContext *)ctx;
    ZoneManager zone_m;
    memcpy(&zone_m,&Context->zone_manager,sizeof(ZoneManager));
    ZoneManager* pzone_mag = &zone_m;
    ZoneInfo    zone_info_ty[MaxZones];
    int sort = 1;
    int cnt = 0;
    int srot_tal[3][2] = {0,};
    int sort_stretch[3] = {0}; 
    int sort_pre;
    int gpu_draw = 0;
    float hfactor_max = 1.0;
    int large_cnt = 0;
    bool isyuv = false;
    int bw = 0;
    BpVopInfo  bpvinfo;    
    int tsize = 0;
    int mix_index = 0;
    int mFtrfl = 0;
    int iFirstTransformLayer=-1;
    bool bTransform=false;
    mix_info gMixInfo;

    memset(&bpvinfo,0,sizeof(BpVopInfo));
    char const* compositionTypeName[] = {
            "win0",
            "win1",
            "win2_0",
            "win2_1",
            "win2_2",
            "win2_3",
            "win3_0",
            "win3_1",
            "win3_2",
            "win3_3",
            };
    hwcContext * contextAh = _contextAnchor;
    memset(&zone_info_ty,0,sizeof(zone_info_ty));
    if(Context == _contextAnchor1) {
        mix_index = 1;
    }else if(Context == _contextAnchor) {
        mix_index = 0;
    }
    if(pzone_mag->zone_cnt < 5) {
		ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
    	return -1;
    }
    //Find out which layer start transform.
    for(i=0;i<pzone_mag->zone_cnt;i++) {
        if(pzone_mag->zone_info[i].transform != 0) {
            mFtrfl = i;
            iFirstTransformLayer = pzone_mag->zone_info[i].layer_index;
            bTransform = true;
            break;
        }
    }

    if(Context->mAlphaError){
        return -1;
    }

    if(contextAh->mHdmiSI.NeedReDst){
        return -1;
    }

    if(Context->Is3D){
        return -1;
    }

#ifdef RK3288_BOX
    if(Context==_contextAnchor && Context->mResolutionChanged && Context->mLcdcNum==2){
        return -1;
    }
#endif

    for(int k=0;k<mFtrfl;k++) {
        if(pzone_mag->zone_info[k].scale_err || pzone_mag->zone_info[k].toosmall
            || pzone_mag->zone_info[k].zone_err || pzone_mag->zone_info[k].transform)
            return -1;
    }

    //If not exist transform layers,then return.
    if(!bTransform) {
		ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
		return -1;
	}

    memcpy((void*)&gMixInfo,(void*)&gmixinfo[mix_index],sizeof(gMixInfo));
    for(i=0,j=0;i<pzone_mag->zone_cnt;i++) {
        //Set the layer which it's layer_index bigger than the first transform layer index to HWC_FRAMEBUFFER or HWC_NODRAW
        if(pzone_mag->zone_info[i].layer_index >= iFirstTransformLayer) {
            hwc_layer_1_t * layer = &list->hwLayers[pzone_mag->zone_info[i].layer_index];
            if(pzone_mag->zone_info[i].format == HAL_PIXEL_FORMAT_YCrCb_NV12) {
                ALOGD_IF(mLogL&HLLTHR,"Donot support video[%d][%s]",__LINE__,__FUNCTION__);
                return -1;
            }
            //Judge the current layer whether backup in gmixinfo[mix_index] or not.
            if(Context->mLastCompType != HWC_MIX_VTWO
                || gMixInfo.lastZoneCrc[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].zoneCrc
                || gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].layer_fd
                || gMixInfo.alpha[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].zone_alpha) {
                gpu_draw = 1;
                layer->compositionType = HWC_FRAMEBUFFER;
                gMixInfo.lastZoneCrc[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].zoneCrc;
                gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].layer_fd;  
                gMixInfo.alpha[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].zone_alpha;
            } else {
                layer->compositionType = HWC_NODRAW;
            }
            if(gpu_draw && pzone_mag->zone_info[i].layer_index > iFirstTransformLayer) {
                for(int j=iFirstTransformLayer;j<pzone_mag->zone_info[i].layer_index;j++) {
                    layer = &list->hwLayers[j];
                    layer->compositionType = HWC_FRAMEBUFFER;
                }               
                ALOGV(" need draw by gpu");
            }
            cntfb ++;
        } else {
            //hwc_layer_1_t * layer = &list->hwLayers[pzone_mag->zone_info[i].layer_index];
            //layer->compositionType = HWC_MIX_V2;
            memcpy(&zone_info_ty[j], &pzone_mag->zone_info[i],sizeof(ZoneInfo));
            zone_info_ty[j].sort = 0;
            j++;
        }
    }
    memcpy(pzone_mag, &zone_info_ty,sizeof(zone_info_ty));
    pzone_mag->zone_cnt -= cntfb;
    for(i=0;i< pzone_mag->zone_cnt;i++)
    {
        ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],acq_fence_fd=%d,"
            "layname=%s",                        
            Context->zone_manager.zone_info[i].zone_index,
            Context->zone_manager.zone_info[i].layer_index,
            Context->zone_manager.zone_info[i].src_rect.left,
            Context->zone_manager.zone_info[i].src_rect.top,
            Context->zone_manager.zone_info[i].src_rect.right,
            Context->zone_manager.zone_info[i].src_rect.bottom,
            Context->zone_manager.zone_info[i].disp_rect.left,
            Context->zone_manager.zone_info[i].disp_rect.top,
            Context->zone_manager.zone_info[i].disp_rect.right,
            Context->zone_manager.zone_info[i].disp_rect.bottom,
            Context->zone_manager.zone_info[i].width,
            Context->zone_manager.zone_info[i].height,
            Context->zone_manager.zone_info[i].stride,
            Context->zone_manager.zone_info[i].format,
            Context->zone_manager.zone_info[i].transform,
            Context->zone_manager.zone_info[i].realtransform,
            Context->zone_manager.zone_info[i].blend,
            Context->zone_manager.zone_info[i].acq_fence_fd,
            Context->zone_manager.zone_info[i].LayerName);
    }
    pzone_mag->zone_info[0].sort = sort;
    for(i=0;i<(pzone_mag->zone_cnt-1);)
    {
        pzone_mag->zone_info[i].sort = sort;
        sort_pre  = sort;
        cnt = 0;
        for(j=1;j<4 && (i+j) < pzone_mag->zone_cnt;j++)
        {
            ZoneInfo * next_zf = &(pzone_mag->zone_info[i+j]);
            bool is_combine = false;
            int k;
            for(k=0;k<=cnt;k++)  // compare all sorted_zone info
            {
                ZoneInfo * sorted_zf = &(pzone_mag->zone_info[i+j-1-k]);
                if(is_zone_combine(sorted_zf,next_zf))
                {
                    is_combine = true;
                }
                else
                {
                    is_combine = false;
                    break;
                }
            }
            if(is_combine)
            {
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;                
            }
            else
            {
                sort++;
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;                
                break;
            }
        }
        if( sort_pre == sort && (i+cnt) < (pzone_mag->zone_cnt-1) )  // win2 ,4zones ,win3 4zones,so sort ++,but exit not ++
            sort ++;
        i += cnt;  
    }
    if(sort >3)  // lcdc dont support 5 wins
    {
        ALOGD_IF(mLogL&HLLTHR,"lcdc dont support 5 wins");
        return -1;
    }    
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
        int factor =1;
        ALOGV("sort[%d].type=%d",i,pzone_mag->zone_info[i].sort);
        if( pzone_mag->zone_info[i].sort == 1){
            srot_tal[0][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[0] = 1;
        }    
        else if(pzone_mag->zone_info[i].sort == 2){
            srot_tal[1][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[1] = 1;
        }    
        else if(pzone_mag->zone_info[i].sort == 3){
            srot_tal[2][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[2] = 1;
        }    
        if(pzone_mag->zone_info[i].hfactor > hfactor_max)
        {
            hfactor_max = pzone_mag->zone_info[i].hfactor;
        }
        if(pzone_mag->zone_info[i].is_large )
        {
            large_cnt ++;
        }
        if(pzone_mag->zone_info[i].format== HAL_PIXEL_FORMAT_YCrCb_NV12)
        {
            isyuv = true;
        }
        if(Context->zone_manager.zone_info[i].hfactor > 1.0)
            factor = 2;
        else
            factor = 1;        
        tsize += (Context->zone_manager.zone_info[i].size *factor);
    }
    j = 0;
    for(i=0;i<3;i++)    
    {        
        if( srot_tal[i][0] >=2)  // > twice zones
        {
            srot_tal[i][1] = win_disphed[j+2]; 
            win_disphed_flag[j+2] = 1; // win2 ,win3 is dispatch flag
            ALOGV("more twice zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 1)  // lcdc only has win2 and win3 supprot more zones
            {
                ALOGD("lcdc only has win2 and win3 supprot more zones");
                return -1;  
            }
        }
    }
    j = 0;
    for(i=0;i<3;i++)    
    {        
        if( sort_stretch[i] == 1)  // strech
        {
            srot_tal[i][1] = win_disphed[j];  // win 0 and win 1 suporot stretch
            win_disphed_flag[j] = 1; // win0 ,win1 is dispatch flag
            ALOGV("stretch zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 2)  // lcdc only has win0 and win1 supprot stretch
            {
                //ALOGD("lcdc only has win0 and win1 supprot stretch");
                return -1;  
            }
        }
    }
    if(hfactor_max >=1.4)
    {
        bw += (j + 1);
        
    }
    if(isyuv)
    {
        bw +=5;
    }
    //ALOGD("large_cnt =%d,bw=%d",large_cnt , bw);
  
    for(i=0;i<3;i++)    
    {        
        if( srot_tal[i][1] == 0)  // had not dispatched
        {
            for(j=0;j<3;j++)
            {
                if(win_disphed_flag[j] == 0) // find the win had not dispatched
                    break;
            }  
            if(j>=3)
            {
                ALOGE("3 wins had beed dispatched ");
                return -1;
            }    
            srot_tal[i][1] = win_disphed[j];
            win_disphed_flag[j] = 1;
            ALOGV("srot_tal[%d][1].dispatched=%d",i,srot_tal[i][1]);
        }
    }
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {        
         switch(pzone_mag->zone_info[i].sort) {
            case 1:
                pzone_mag->zone_info[i].dispatched = srot_tal[0][1]++;
                break;
            case 2:
                pzone_mag->zone_info[i].dispatched = srot_tal[1][1]++;            
                break;
            case 3:
                pzone_mag->zone_info[i].dispatched = srot_tal[2][1]++;            
                break;
            default:
                ALOGE("try_wins_dispatch_mix_v2 sort err!");
                return -1;
        }
        ALOGV("zone[%d].dispatched[%d]=%s,sort=%d", \
        i,pzone_mag->zone_info[i].dispatched,
        compositionTypeName[pzone_mag->zone_info[i].dispatched -1],
        pzone_mag->zone_info[i].sort);
    }

    for(i=0;i<pzone_mag->zone_cnt;i++){
        int disptched = pzone_mag->zone_info[i].dispatched;
        int sct_width = pzone_mag->zone_info[i].width;
        int sct_height = pzone_mag->zone_info[i].height;
        /*win2 win3 not support YUV*/
        if(disptched > win1 && is_yuv(pzone_mag->zone_info[i].format))
            return -1;
        /*scal not support whoes source bigger than 2560 to dst 4k*/
        if(disptched <= win1 &&(sct_width > 2160 || sct_height > 2160) &&
            !is_yuv(pzone_mag->zone_info[i].format) && contextAh->mHdmiSI.NeedReDst)
            return -1;
    }

#if USE_QUEUE_DDRFREQ
    if(Context->ddrFd > 0)
    {
        for(i=0;i<pzone_mag->zone_cnt;i++)
        {
            int area_no = 0;
            int win_id = 0;
            ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],dispatched=%d,"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],"
            "layer_fd[%d],addr=%x,acq_fence_fd=%d"
            "layname=%s",    
            pzone_mag->zone_info[i].zone_index,
            pzone_mag->zone_info[i].layer_index,
            pzone_mag->zone_info[i].dispatched,
            pzone_mag->zone_info[i].src_rect.left,
            pzone_mag->zone_info[i].src_rect.top,
            pzone_mag->zone_info[i].src_rect.right,
            pzone_mag->zone_info[i].src_rect.bottom,
            pzone_mag->zone_info[i].disp_rect.left,
            pzone_mag->zone_info[i].disp_rect.top,
            pzone_mag->zone_info[i].disp_rect.right,
            pzone_mag->zone_info[i].disp_rect.bottom,
            pzone_mag->zone_info[i].width,
            pzone_mag->zone_info[i].height,
            pzone_mag->zone_info[i].stride,
            pzone_mag->zone_info[i].format,
            pzone_mag->zone_info[i].transform,
            pzone_mag->zone_info[i].realtransform,
            pzone_mag->zone_info[i].blend,
            pzone_mag->zone_info[i].layer_fd,
            pzone_mag->zone_info[i].addr,
            pzone_mag->zone_info[i].acq_fence_fd,
            pzone_mag->zone_info[i].LayerName);
            switch(pzone_mag->zone_info[i].dispatched) {
                case win0:
                    bpvinfo.vopinfo[0].state = 1;
                    bpvinfo.vopinfo[0].zone_num ++;                
                   break;
                case win1:
                    bpvinfo.vopinfo[1].state = 1;
                    bpvinfo.vopinfo[1].zone_num ++;                            
                    break;
                case win2_0:   
                    bpvinfo.vopinfo[2].state = 1;
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_1:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;  
                case win2_2:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_3:
                    bpvinfo.vopinfo[2].zone_num ++;                                                    
                    break;           
                default:
                    ALOGE("hwc_dispatch_mix  err!");
                    return -1;
             }    
        }  
        bpvinfo.vopinfo[3].state = 1;
        bpvinfo.vopinfo[3].zone_num ++;                                        
        bpvinfo.bp_size = Context->zone_manager.bp_size;
        tsize += Context->fbhandle.width * Context->fbhandle.height*4;
        if(tsize)
            tsize = tsize / (1024 *1024) * 60 ;// MB
        bpvinfo.bp_vop_size = tsize ;  
        for(i= 0;i<4;i++)
        {
            ALOGD_IF(mLogL&HLLFIV,"RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
        }    
        if(ioctl(Context->ddrFd, RK_QUEDDR_FREQ, &bpvinfo))
        {
            if(mLogL&HLLTHR)
            {
                for(i= 0;i<4;i++)
                {
                    ALOGD("RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                        i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
                }    
            }    
            return -1;    
        }
    }
#endif   
    if((large_cnt + bw) >= 5) {       
        ALOGD_IF(mLogL&HLLTHR,"lagre win > 2,and Scale-down 1.5 multiple,lcdc no support");
        return -1;
    }
    if(list) {
        list->hwLayers[0].compositionType = HWC_MIX_V2;
        list->hwLayers[1].compositionType = HWC_MIX_V2;
    }
    //Mark the composer mode to HWC_MIX_V2
    memcpy(&Context->zone_manager,&zone_m,sizeof(ZoneManager));
    Context->zone_manager.mCmpType = HWC_MIX_VTWO;
    Context->zone_manager.composter_mode = HWC_MIX_V2;
    memcpy((void*)&gmixinfo[mix_index],(void*)&gMixInfo,sizeof(gMixInfo));
    return 0;
#else
    return -1;
#endif
}



int try_wins_dispatch_mix_vh (void * ctx,hwc_display_contents_1_t * list)
{
    int win_disphed_flag[3] = {0,}; // win0, win1, win2, win3 flag which is dispatched
    int win_disphed[3] = {win0,win1,win2_0};
    int i,j;
    int cntfb = 0;
    hwcContext * Context = (hwcContext *)ctx;
    ZoneManager zone_m;
    memcpy(&zone_m,&Context->zone_manager,sizeof(ZoneManager));
    ZoneManager* pzone_mag = &zone_m;
    ZoneInfo    zone_info_ty[MaxZones];
    int sort = 1;
    int cnt = 0;
    int srot_tal[3][2] = {0,};
    int sort_stretch[3] = {0}; 
    int sort_pre;
    int gpu_draw = 0;
    float hfactor_max = 1.0;
    int large_cnt = 0;
    bool isyuv = false;
    int bw = 0;
    BpVopInfo  bpvinfo;    
    int tsize = 0;
    int mix_index = 0;
    int iFirstTransformLayer=-1;
    bool bTransform=false;
    mix_info gMixInfo;

    memset(&bpvinfo,0,sizeof(BpVopInfo));
    char const* compositionTypeName[] = {
            "win0",
            "win1",
            "win2_0",
            "win2_1",
            "win2_2",
            "win2_3",
            "win3_0",
            "win3_1",
            "win3_2",
            "win3_3",
            };
    hwcContext * contextAh = _contextAnchor;
    memset(&zone_info_ty,0,sizeof(zone_info_ty));
    if(Context == _contextAnchor1){
        mix_index = 1;
    }else if(Context == _contextAnchor){
        mix_index = 0;
    }
    if(list->numHwLayers - 1 < 2)
    {
    	return -1;
    }

    if(Context->mAlphaError){
        return -1;
    }

    for(int k=0;k<1;k++)
    {
        if(pzone_mag->zone_info[k].scale_err || pzone_mag->zone_info[k].toosmall
            || pzone_mag->zone_info[k].zone_err || (pzone_mag->zone_info[k].transform
                && (pzone_mag->zone_info[k].format != HAL_PIXEL_FORMAT_YCrCb_NV12)))
            return -1;
    }

    if(Context->Is3D && (!pzone_mag->zone_info[0].alreadyStereo && pzone_mag->zone_info[0].displayStereo)){
        return -1;
    }

    memcpy((void*)&gMixInfo,(void*)&gmixinfo[mix_index],sizeof(gMixInfo));
    for(i=0,j=0;i<pzone_mag->zone_cnt;i++)
    {
        //Set the layer which it's layer_index bigger than the first transform layer index to HWC_FRAMEBUFFER or HWC_NODRAW
        if(pzone_mag->zone_info[i].layer_index > 0)
        {
            hwc_layer_1_t * layer = &list->hwLayers[pzone_mag->zone_info[i].layer_index];
            //Judge the current layer whether backup in gmixinfo[mix_index] or not.
            if(Context->mLastCompType != HWC_MIX_VH
                || gMixInfo.lastZoneCrc[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].zoneCrc
                || gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].layer_fd
                || gMixInfo.alpha[pzone_mag->zone_info[i].layer_index] != pzone_mag->zone_info[i].zone_alpha) {
                gpu_draw = 1;
                layer->compositionType = HWC_FRAMEBUFFER;
                gMixInfo.lastZoneCrc[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].zoneCrc;
                gMixInfo.gpu_draw_fd[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].layer_fd;  
                gMixInfo.alpha[pzone_mag->zone_info[i].layer_index] = pzone_mag->zone_info[i].zone_alpha;
            }
            else
            {
                layer->compositionType = HWC_FRAMEBUFFER;
            }
            if(gpu_draw && pzone_mag->zone_info[i].layer_index > 0)
            {
                for(int j=1;j<pzone_mag->zone_info[i].layer_index;j++)
                {
                    layer = &list->hwLayers[j];
                    layer->compositionType = HWC_FRAMEBUFFER;
                }               
                ALOGV(" need draw by gpu");
            }
            cntfb ++;
        }
        else
        {
            memcpy(&zone_info_ty[j], &pzone_mag->zone_info[i],sizeof(ZoneInfo));
            zone_info_ty[j].sort = 0;
            j++;
        }
    }
    memcpy(pzone_mag, &zone_info_ty,sizeof(zone_info_ty));
    pzone_mag->zone_cnt -= cntfb;
    for(i=0;i< pzone_mag->zone_cnt;i++)
    {
        ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],acq_fence_fd=%d,"
            "layname=%s",                        
            Context->zone_manager.zone_info[i].zone_index,
            Context->zone_manager.zone_info[i].layer_index,
            Context->zone_manager.zone_info[i].src_rect.left,
            Context->zone_manager.zone_info[i].src_rect.top,
            Context->zone_manager.zone_info[i].src_rect.right,
            Context->zone_manager.zone_info[i].src_rect.bottom,
            Context->zone_manager.zone_info[i].disp_rect.left,
            Context->zone_manager.zone_info[i].disp_rect.top,
            Context->zone_manager.zone_info[i].disp_rect.right,
            Context->zone_manager.zone_info[i].disp_rect.bottom,
            Context->zone_manager.zone_info[i].width,
            Context->zone_manager.zone_info[i].height,
            Context->zone_manager.zone_info[i].stride,
            Context->zone_manager.zone_info[i].format,
            Context->zone_manager.zone_info[i].transform,
            Context->zone_manager.zone_info[i].realtransform,
            Context->zone_manager.zone_info[i].blend,
            Context->zone_manager.zone_info[i].acq_fence_fd,
            Context->zone_manager.zone_info[i].LayerName);
    }
    pzone_mag->zone_info[0].sort = sort;
    for(i=0;i<(pzone_mag->zone_cnt-1);)
    {
        pzone_mag->zone_info[i].sort = sort;
        sort_pre  = sort;
        cnt = 0;
        for(j=1;j<4 && (i+j) < pzone_mag->zone_cnt;j++)
        {
            ZoneInfo * next_zf = &(pzone_mag->zone_info[i+j]);
            bool is_combine = false;
            int k;
            for(k=0;k<=cnt;k++)  // compare all sorted_zone info
            {
                ZoneInfo * sorted_zf = &(pzone_mag->zone_info[i+j-1-k]);
                if(is_zone_combine(sorted_zf,next_zf))
                {
                    is_combine = true;
                }
                else
                {
                    is_combine = false;
                    break;
                }
            }
            if(is_combine)
            {
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;                
            }
            else
            {
                sort++;
                pzone_mag->zone_info[i+j].sort = sort;
                cnt++;                
                break;
            }
        }
        if( sort_pre == sort && (i+cnt) < (pzone_mag->zone_cnt-1) )  // win2 ,4zones ,win3 4zones,so sort ++,but exit not ++
            sort ++;
        i += cnt;  
    }
    if(sort >3)  // lcdc dont support 5 wins
    {
        ALOGD_IF(mLogL&HLLTHR,"lcdc dont support 5 wins [%d]",__LINE__);
        return -1;
    }    
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
        int factor =1;
        ALOGV("sort[%d].type=%d",i,pzone_mag->zone_info[i].sort);
        if( pzone_mag->zone_info[i].sort == 1){
            srot_tal[0][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[0] = 1;
        }    
        else if(pzone_mag->zone_info[i].sort == 2){
            srot_tal[1][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[1] = 1;
        }    
        else if(pzone_mag->zone_info[i].sort == 3){
            srot_tal[2][0]++;
            if(pzone_mag->zone_info[i].is_stretch)
                sort_stretch[2] = 1;
        }    
        if(pzone_mag->zone_info[i].hfactor > hfactor_max)
        {
            hfactor_max = pzone_mag->zone_info[i].hfactor;
        }
        if(pzone_mag->zone_info[i].is_large )
        {
            large_cnt ++;
        }
        if(pzone_mag->zone_info[i].format== HAL_PIXEL_FORMAT_YCrCb_NV12)
        {
            isyuv = true;
        }
        if(Context->zone_manager.zone_info[i].hfactor > 1.0)
            factor = 2;
        else
            factor = 1;        
        tsize += (Context->zone_manager.zone_info[i].size *factor);
    }
    j = 0;
    for(i=0;i<3;i++)    
    {        
        if( srot_tal[i][0] >=2)  // > twice zones
        {
            srot_tal[i][1] = win_disphed[j+2]; 
            win_disphed_flag[j+2] = 1; // win2 ,win3 is dispatch flag
            ALOGV("more twice zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 1)  // lcdc only has win2 and win3 supprot more zones
            {
                ALOGD("lcdc only has win2 and win3 supprot more zones");
                return -1;  
            }
        }
    }
    j = 0;
    for(i=0;i<3;i++)    
    {        
        if( sort_stretch[i] == 1)  // strech
        {
            srot_tal[i][1] = win_disphed[j];  // win 0 and win 1 suporot stretch
            win_disphed_flag[j] = 1; // win0 ,win1 is dispatch flag
            ALOGV("stretch zones srot_tal[%d][1]=%d",i,srot_tal[i][1]);
            j++;
            if(j > 2)  // lcdc only has win0 and win1 supprot stretch
            {
                ALOGD("lcdc only has win0 and win1 supprot stretch");
                return -1;  
            }
        }
    }
    if(hfactor_max >=1.4)
    {
        bw += (j + 1);
        
    }
    if(isyuv)
    {
        bw +=5;
    }
    ALOGV("large_cnt =%d,bw=%d",large_cnt , bw);
  
    for(i=0;i<3;i++)    
    {        
        if( srot_tal[i][1] == 0)  // had not dispatched
        {
            for(j=0;j<3;j++)
            {
                if(win_disphed_flag[j] == 0) // find the win had not dispatched
                    break;
            }  
            if(j>=3)
            {
                ALOGE("3 wins had beed dispatched ");
                return -1;
            }    
            srot_tal[i][1] = win_disphed[j];
            win_disphed_flag[j] = 1;
            ALOGV("srot_tal[%d][1].dispatched=%d",i,srot_tal[i][1]);
        }
    }
       
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {        
         switch(pzone_mag->zone_info[i].sort) {
            case 1:
                pzone_mag->zone_info[i].dispatched = srot_tal[0][1]++;
                break;
            case 2:
                pzone_mag->zone_info[i].dispatched = srot_tal[1][1]++;            
                break;
            case 3:
                pzone_mag->zone_info[i].dispatched = srot_tal[2][1]++;            
                break;
            default:
                ALOGE("try_wins_dispatch_mix_vh sort err!");
                return -1;
        }
        ALOGV("zone[%d].dispatched[%d]=%s,sort=%d", \
        i,pzone_mag->zone_info[i].dispatched,
        compositionTypeName[pzone_mag->zone_info[i].dispatched -1],
        pzone_mag->zone_info[i].sort);
    }

    for(i=0;i<pzone_mag->zone_cnt;i++){
        int disptched = pzone_mag->zone_info[i].dispatched;
        int sct_width = pzone_mag->zone_info[i].width;
        int sct_height = pzone_mag->zone_info[i].height;
        /*scal not support whoes source bigger than 2560 to dst 4k*/
        if(disptched <= win1 &&(sct_width > 2160 || sct_height > 2160) &&
            !is_yuv(pzone_mag->zone_info[i].format) && contextAh->mHdmiSI.NeedReDst)
            return -1;
    }

#if USE_QUEUE_DDRFREQ
    if(Context->ddrFd > 0)
    {
        for(i=0;i<pzone_mag->zone_cnt;i++)
        {
            int area_no = 0;
            int win_id = 0;
            ALOGD_IF(mLogL&HLLFIV,"Zone[%d]->layer[%d],dispatched=%d,"
            "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
            "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],"
            "layer_fd[%d],addr=%x,acq_fence_fd=%d"
            "layname=%s",    
            pzone_mag->zone_info[i].zone_index,
            pzone_mag->zone_info[i].layer_index,
            pzone_mag->zone_info[i].dispatched,
            pzone_mag->zone_info[i].src_rect.left,
            pzone_mag->zone_info[i].src_rect.top,
            pzone_mag->zone_info[i].src_rect.right,
            pzone_mag->zone_info[i].src_rect.bottom,
            pzone_mag->zone_info[i].disp_rect.left,
            pzone_mag->zone_info[i].disp_rect.top,
            pzone_mag->zone_info[i].disp_rect.right,
            pzone_mag->zone_info[i].disp_rect.bottom,
            pzone_mag->zone_info[i].width,
            pzone_mag->zone_info[i].height,
            pzone_mag->zone_info[i].stride,
            pzone_mag->zone_info[i].format,
            pzone_mag->zone_info[i].transform,
            pzone_mag->zone_info[i].realtransform,
            pzone_mag->zone_info[i].blend,
            pzone_mag->zone_info[i].layer_fd,
            pzone_mag->zone_info[i].addr,
            pzone_mag->zone_info[i].acq_fence_fd,
            pzone_mag->zone_info[i].LayerName);
            switch(pzone_mag->zone_info[i].dispatched) {
                case win0:
                    bpvinfo.vopinfo[0].state = 1;
                    bpvinfo.vopinfo[0].zone_num ++;                
                   break;
                case win1:
                    bpvinfo.vopinfo[1].state = 1;
                    bpvinfo.vopinfo[1].zone_num ++;                            
                    break;
                case win2_0:   
                    bpvinfo.vopinfo[2].state = 1;
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_1:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;  
                case win2_2:
                    bpvinfo.vopinfo[2].zone_num ++;                                        
                    break;
                case win2_3:
                    bpvinfo.vopinfo[2].zone_num ++;                                                    
                    break;           
                default:
                    ALOGE("hwc_dispatch_mix  err!");
                    return -1;
             }    
        }  
        bpvinfo.vopinfo[3].state = 1;
        bpvinfo.vopinfo[3].zone_num ++;                                        
        bpvinfo.bp_size = Context->zone_manager.bp_size;
        tsize += Context->fbhandle.width * Context->fbhandle.height*4;
        if(tsize)
            tsize = tsize / (1024 *1024) * 60 ;// MB
        bpvinfo.bp_vop_size = tsize ;  
        for(i= 0;i<4;i++)
        {
            ALOGD_IF(mLogL&HLLFIV,"RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
        }    
        if(ioctl(Context->ddrFd, RK_QUEDDR_FREQ, &bpvinfo))
        {
            if(mLogL&HLLTHR)
            {
                for(i= 0;i<4;i++)
                {
                    ALOGD("RK_QUEDDR_FREQ mixinfo win[%d] bo_size=%dMB,bp_vop_size=%dMB,state=%d,num=%d",
                        i,bpvinfo.bp_size,bpvinfo.bp_vop_size,bpvinfo.vopinfo[i].state,bpvinfo.vopinfo[i].zone_num);
                }    
            }    
            return -1;    
        }
    }
#endif   
    //Mark the composer mode to HWC_MIX_V2
    memcpy(&Context->zone_manager,&zone_m,sizeof(ZoneManager));
    if(list){
        list->hwLayers[0].compositionType = HWC_MIX_V2;
    }
    Context->mHdmiSI.mix_vh = true;
    Context->zone_manager.mCmpType = HWC_MIX_VH;
    Context->zone_manager.composter_mode = HWC_MIX_V2;
    memcpy((void*)&gmixinfo[mix_index],(void*)&gMixInfo,sizeof(gMixInfo));
    return 0;
}


// return 0: suess
// return -1: fail
int try_wins_dispatch_ver(void * ctx,hwc_display_contents_1_t * list)
{
    int win_disphed_flag[4] = {0,}; // win0, win1, win2, win3 flag which is dispatched
    int win_disphed[4] = {win0,win1,win2_0,win3_0};
    int i,j;
    hwcContext * Context = (hwcContext *)ctx;
    ZoneManager zone_m;
    memcpy(&zone_m,&Context->zone_manager,sizeof(ZoneManager));
    ZoneManager* pzone_mag = &zone_m;
    // try dispatch stretch wins
    char const* compositionTypeName[] = {
            "win0",
            "win1",
            "win2_0",
            "win2_1",
            "win2_2",
            "win2_3",
            "win3_0",
            "win3_1",
            "win3_2",
            "win3_3",
            "win_ext"
            };
            
    // first dispatch stretch win         
    if(pzone_mag->zone_cnt <=4)
    {
        for(i=0,j=0;i<pzone_mag->zone_cnt;i++)
        {
            if(pzone_mag->zone_info[i].is_stretch == true
                && pzone_mag->zone_info[i].dispatched == 0) 
            {
                pzone_mag->zone_info[i].dispatched = win_disphed[j];  // win 0 and win 1 suporot stretch
                win_disphed_flag[j] = 1; // win2 ,win3 is dispatch flag
                ALOGV("stretch zones [%d]=%d",i,pzone_mag->zone_info[i].dispatched);
                j++;
                if(j > 2)  // lcdc only has win2 and win3 supprot more zones
                {
                    //ALOGD("lcdc only has win0 and win1 supprot stretch");
                    return -1;  
                }     
            }
        }
        // second dispatch common zones win  
        for(i=0,j=0;i<pzone_mag->zone_cnt;i++)    
        {        
            if( pzone_mag->zone_info[i].dispatched == 0)  // had not dispatched
            {
                for(j=0;j<4;j++)
                {
                    if(win_disphed_flag[j] == 0) // find the win had not dispatched
                        break;
                }  
                if(j>=4)
                {
                    ALOGE("4 wins had beed dispatched ");
                    return -1;
                }    
                pzone_mag->zone_info[i].dispatched  = win_disphed[j];
                win_disphed_flag[j] = 1;
                ALOGV("zone[%d][1].dispatched=%d",i,pzone_mag->zone_info[i].dispatched);
            }
        }
    
    }
    else
    {
        // first dispatch Bottom and  Top      

        for(i=0,j=3;i<pzone_mag->zone_cnt;i++)
        {
            bool IsBottom = !strcmp(BOTTOM_LAYER_NAME, pzone_mag->zone_info[i].LayerName);
            bool IsTop = !strcmp(TOP_LAYER_NAME,pzone_mag->zone_info[i].LayerName);
            if(IsTop || IsBottom)
            {
              pzone_mag->zone_info[i].dispatched  =  win_disphed[j];
              win_disphed_flag[j] = 1;
              j--;
            }  
        }
        for(i=0,j=0;i<pzone_mag->zone_cnt;i++)    
        {        
            if( pzone_mag->zone_info[i].dispatched == 0)  // had not dispatched
            {
                for(j=0;j<4;j++)
                {
                    if(win_disphed_flag[j] == 0) // find the win had not dispatched
                        break;
                }  
                if(j>=4)
                {
                    bool isLandScape = ( (0==pzone_mag->zone_info[i].realtransform) \
                                || (HWC_TRANSFORM_ROT_180==pzone_mag->zone_info[i].realtransform) );
                    bool isSmallRect = (isLandScape && (pzone_mag->zone_info[i].height< (unsigned int)Context->fbhandle.height/4))  \
                                ||(!isLandScape && (pzone_mag->zone_info[i].width < (unsigned int)Context->fbhandle.width/4)) ;
                    if(isSmallRect)
                        pzone_mag->zone_info[i].dispatched  = win_ext;
                    else
                        return -1;  // too large
                }   
                else
                {
                    pzone_mag->zone_info[i].dispatched  = win_disphed[j];
                    win_disphed_flag[j] = 1;
                    ALOGV("zone[%d].dispatched=%d",i,pzone_mag->zone_info[i].dispatched);
                }    
            }
        }
    }
       
    Context->zone_manager.composter_mode = HWC_LCDC;
    return 0;
}

static int check_zone(hwcContext * Context)
{
    ZoneManager* pzone_mag = &(Context->zone_manager);
    int iCountFBDC = 0;

    if(Context == NULL)
    {
        LOGE("Context is null");
        return -1;
    }

#if G6110_SUPPORT_FBDC
    for(int i=0;i<pzone_mag->zone_cnt;i++)
    {
        //Count layers which used fbdc
        if(HALPixelFormatGetCompression(pzone_mag->zone_info[i].format) != HAL_FB_COMPRESSION_NONE)
        {
            iCountFBDC++;
        }
    }

    //If FBDC layers bigger than one,then go into GPU composition.
    if(iCountFBDC > 1)
    {
        LOGGPUCOP("%s:line=%d,iCountFBDC=%d",__func__,__LINE__,iCountFBDC);
        return -1;
    }
#endif
    return 0;
}

static hwcSTATUS
hwcCheckFormat(
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
        case HAL_PIXEL_FORMAT_RGB_888:
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_YCrCb_NV12:
        case HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO:
#if G6110_SUPPORT_FBDC
        case FBDC_BGRA_8888:
        case FBDC_RGBA_8888:
#endif
             return hwcSTATUS_OK;
        default:
            ALOGE("%s:line=%d invalid format=0x%x",__func__,__LINE__,GPU_FORMAT);
            return hwcSTATUS_INVALID_ARGUMENT;
        }
    }


    return hwcSTATUS_OK;
}

static int skip_count = 0;
static uint32_t
check_layer(
    hwcContext * Context,
    uint32_t Count,
    uint32_t Index,
    bool videomode,
    hwc_layer_1_t * Layer
    )
{
    struct private_handle_t * handle =
        (struct private_handle_t *) Layer->handle;
    //(void) Context;

    (void) Count;
    (void) Index;

    if(0 == Index && (Layer->blending >> 16) < 250
        && ((Layer->blending & 0xffff) == HWC_BLENDING_PREMULT)){
        Context->mAlphaError = true;
    }else{
        Context->mAlphaError = false;
    }

#if OPTIMIZATION_FOR_DIMLAYER
    if(!strcmp(Layer->LayerName,"DimLayer"))
    {
        Layer->handle= Context->mDimHandle;
        handle = (struct private_handle_t*) Layer->handle;
        Layer->sourceCrop.left = Layer->displayFrame.left;
        Layer->sourceCrop.top = Layer->displayFrame.top;
        Layer->sourceCrop.right = Layer->displayFrame.right;
        Layer->sourceCrop.bottom = Layer->displayFrame.bottom;

        Layer->flags &= ~HWC_SKIP_LAYER;

        Context->bHasDimLayer = true;
    }
#endif
    
#if 0    
    float hfactor = 1;
    float vfactor = 1;

    hfactor = (float) (Layer->sourceCrop.right - Layer->sourceCrop.left)
            / (Layer->displayFrame.right - Layer->displayFrame.left);

    vfactor = (float) (Layer->sourceCrop.bottom - Layer->sourceCrop.top)
            / (Layer->displayFrame.bottom - Layer->displayFrame.top);

    /* Check whether this layer is forced skipped. */

    if(hfactor<1.0 ||vfactor<1.0 )
    {
        ALOGE("[%d,%d,%d,%d],[%d,%d,%d,%d]",Layer->sourceCrop.right , Layer->sourceCrop.left,Layer->displayFrame.right , Layer->displayFrame.left,
            Layer->sourceCrop.bottom ,Layer->sourceCrop.top,Layer->displayFrame.bottom ,Layer->displayFrame.top);
    }
    ALOGD("[%f,%f],name[%d]=%s",hfactor,vfactor,Index,Layer->LayerName);
#endif
    if(handle != NULL && Context->mVideoMode && Layer->transform != 0)
        Context->mVideoRotate=true;
    else
        Context->mVideoRotate=false;
    if ((Layer->flags & HWC_SKIP_LAYER)
       // ||(hfactor != 1.0f)  // because rga scale down too slowly,so return to opengl  ,huangds modify
       // ||(vfactor != 1.0f)  // because rga scale down too slowly,so return to opengl ,huangds modify
        || (handle == NULL)
#if !OPTIMIZATION_FOR_TRANSFORM_UI
        ||((Layer->transform != 0)&& (!videomode)
#if ENABLE_TRANSFORM_BY_RGA
          &&!strstr(Layer->LayerName,"Starting@# ")
#endif
          )
#endif
#if !(defined(GPU_G6110) || defined(RK3288_BOX))
        || skip_count<10 
#endif
        || (handle->type == 1 && !_contextAnchor->iommuEn)
        || (Context->mGtsStatus && !strcmp(Layer->LayerName,"SurfaceView")
        && (handle && GPU_FORMAT == HAL_PIXEL_FORMAT_RGBA_8888))
    )
    {
        /* We are forbidden to handle this layer. */
        if(mLogL&HLLTHR)
        {
            LOGD("%s(%d):Will not handle layer %s: SKIP_LAYER,Layer->transform=%d,Layer->flags=%d",
                __FUNCTION__, __LINE__, Layer->LayerName,Layer->transform,Layer->flags);
            if(handle)   
            {
                LOGD("Will not handle format=%x,handle_type=%d",GPU_FORMAT,handle->type);  
            }    
        }        
        Layer->compositionType = HWC_FRAMEBUFFER;
        if (skip_count<10)
        {
        	skip_count++;
        }
		ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        return HWC_FRAMEBUFFER;
    }
    // Force 4K transform video go into GPU
#if 0
    int w=0,h=0;
    w =  Layer->sourceCrop.right - Layer->sourceCrop.left;
    h =  Layer->sourceCrop.bottom - Layer->sourceCrop.top;

    if(Context->mVideoMode && (w>=3840 || h>=2160) && Layer->transform)
    {
        ALOGV("4K video transform=%d,w=%d,h=%d go into GPU",Layer->transform,w,h);
        return HWC_FRAMEBUFFER;
    }
#endif

#if ENABLE_TRANSFORM_BY_RGA
        if(Layer->transform
            && handle->format != HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO
            &&Context->mtrsformcnt == 1)
        {
            Context->mTrsfrmbyrga = true;
            ALOGV("zxl:layer->transform=%d",Layer->transform);
        }
#endif

#if !ENABLE_LCDC_IN_NV12_TRANSFORM
        if(Context->mGtsStatus)
#endif
        {
            ALOGV("In gts status,go into lcdc when rotate video");
            if(Layer->transform && handle->format == HAL_PIXEL_FORMAT_YCrCb_NV12)
            {
                Context->mTrsfrmbyrga = true;
                LOGV("zxl:layer->transform=%d",Layer->transform );
            }
        }
    
    do
    {
        RgaSURF_FORMAT format = RK_FORMAT_UNKNOWN;

#if 0
        /* Check for dim layer. */
        if ((Layer->blending & 0xFFFF) == HWC_BLENDING_DIM )
        {
            Layer->compositionType = HWC_DIM;
            Layer->flags           = 0;
            ALOGV("DIM Layer");
            break;
        }
#endif
        /* TODO: I BELIEVE YOU CAN HANDLE SUCH LAYER!. */
        /* At least surfaceflinger can handle this layer. */
        Layer->compositionType = HWC_FRAMEBUFFER;

        /* Get format. */
		//zxl: remove hwcGetFormat,or it will let fbdc format return gpu.
        if(  /*hwcCheckFormat(handle, &format) != hwcSTATUS_OK
            ||*/ (LayerZoneCheck(Layer,Context == _contextAnchor ? HWC_DISPLAY_PRIMARY : HWC_DISPLAY_EXTERNAL) != 0))
        {
             return HWC_FRAMEBUFFER;
        }

        LOGV("name[%d]=%s,cnt=%d,vodemod=%d",Index,Layer->LayerName,Count ,videomode);
             
        Layer->compositionType = HWC_LCDC;
        Layer->flags           = 0;
        //ALOGD("win 0");
        break;                 
       

    }while (0);
    /* Return last composition type. */
    return Layer->compositionType;
}


/*****************************************************************************/

#if hwcDEBUG
static void
_Dump(
    hwc_display_contents_1_t* list
    )
{
    size_t i, j;

    for (i = 0; list && (i < (list->numHwLayers - 1)); i++)
    {
        hwc_layer_1_t const * l = &list->hwLayers[i];
        struct private_handle_t * handle = (struct private_handle_t *) (l->handle);
        if(l->flags & HWC_SKIP_LAYER)
        {
            LOGD("layer %p skipped", l);
        }
        else
        {
            if(handle)
            LOGD("layer=%p, "
                 "name=%s "
                 "type=%d, "
                 "hints=%08x, "
                 "flags=%08x, "
                 "handle=%p, "
                 "format=0x%x, "
                 "tr=%02x, "
                 "blend=%04x, "
                 "{%d,%d,%d,%d}, "
                 "{%d,%d,%d,%d}",
                 l,
                 l->LayerName,
                 l->compositionType,
                 l->hints,
                 l->flags,
                 l->handle,
                 handle->format,
                 l->transform,
                 l->blending,
                 l->sourceCrop.left,
                 l->sourceCrop.top,
                 l->sourceCrop.right,
                 l->sourceCrop.bottom,
                 l->displayFrame.left,
                 l->displayFrame.top,
                 l->displayFrame.right,
                 l->displayFrame.bottom);

            for (j = 0; j < l->visibleRegionScreen.numRects; j++)
            {
                LOGD("\trect%d: {%d,%d,%d,%d}", j,
                     l->visibleRegionScreen.rects[j].left,
                     l->visibleRegionScreen.rects[j].top,
                     l->visibleRegionScreen.rects[j].right,
                     l->visibleRegionScreen.rects[j].bottom);
            }
        }
    }
}
#endif

#if hwcDumpSurface
static void
_DumpSurface(
    hwc_display_contents_1_t* list
    )
{
    size_t i;
    static int DumpSurfaceCount = 0;

    char pro_value[PROPERTY_VALUE_MAX];
    property_get("sys.dump",pro_value,0);
    //LOGI(" sys.dump value :%s",pro_value);
    if(!strcmp(pro_value,"true"))
    {
        for (i = 0; list && (i < (list->numHwLayers - 1)); i++)
        {
            hwc_layer_1_t const * l = &list->hwLayers[i];

            if(l->flags & HWC_SKIP_LAYER)
            {
                LOGI("layer %p skipped", l);
            }
            else
            {
                struct private_handle_t * handle_pre = (struct private_handle_t *) l->handle;
                int32_t SrcStride ;
                FILE * pfile = NULL;
                char layername[100] ;


                if( handle_pre == NULL || handle_pre->format == HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO)
                    continue;

                SrcStride = android::bytesPerPixel(handle_pre->format);
                memset(layername,0,sizeof(layername));
                system("mkdir /data/dump/ && chmod /data/dump/ 777 ");
                //mkdir( "/data/dump/",777);
                sprintf(layername,"/data/dump/dmlayer%d_%d_%d_%d.bin",DumpSurfaceCount,handle_pre->stride,handle_pre->height,SrcStride);
                DumpSurfaceCount ++;
                pfile = fopen(layername,"wb");
                if(pfile)
                {
#ifdef GPU_G6110
                    fwrite((const void *)(handle_pre->pvBase),(size_t)(SrcStride * handle_pre->stride*handle_pre->height),1,pfile);

#else
                    fwrite((const void *)(handle_pre->base),(size_t)(SrcStride * handle_pre->stride*handle_pre->height),1,pfile);
#endif
                    fclose(pfile);
                    LOGI(" dump surface layername %s,w:%d,h:%d,formatsize :%d",layername,handle_pre->width,handle_pre->height,SrcStride);
                }
            }
        }

    }
    property_set("sys.dump","false");


}
#endif

void
hwcDumpArea(
    IN hwcArea * Area
    )
{
    hwcArea * area = Area;

    while (area != NULL)
    {
        char buf[128];
        char digit[8];
        bool first = true;

        sprintf(buf,
                "Area[%d,%d,%d,%d] owners=%08x:",
                area->rect.left,
                area->rect.top,
                area->rect.right,
                area->rect.bottom,
                area->owners);

        for (int i = 0; i < 32; i++)
        {
            /* Build decimal layer indices. */
            if (area->owners & (1U << i))
            {
                if (first)
                {
                    sprintf(digit, " %d", i);
                    strcat(buf, digit);
                    first = false;
                }
                else
                {
                    sprintf(digit, ",%d", i);
                    strcat(buf, digit);
                }
            }

            if (area->owners < (1 << i))
            {
                break;
            }
        }

        LOGD("%s", buf);

        /* Advance to next area. */
        area = area->next;
    }    
}
static int CompareLines(int *da,int w)
{
    int i,j;
    for(i = 0;i<1;i++) // compare 4 lins
    {
        for(j= 0;j<w;j+=8)  
        {
            if((unsigned int)*da != 0xff000000 && (unsigned int)*da != 0x0)
            {
                return 1;
            }
            da +=8;

        }
    }    
    return 0;
}
static int CompareVers(int *da,int w,int h)
{
    int i,j;
    int *data ;
    for(i = 0;i<1;i++) // compare 4 lins
    {
        data = da + i;
        for(j= 0;j<h;j+=4)  
        {
            if((unsigned int)*data != 0xff000000 && (unsigned int)*data != 0x0 )
            {
                return 1;
            }
            data +=4*w;    
        }
    }    
    return 0;
}

static int DetectValidData(int *data,int w,int h)
{
    int i,j;
    int *da;
    int ret;
    /*  detect model
    -------------------------
    |   |   |    |    |      |       
    |   |   |    |    |      |
    |------------------------|       
    |   |   |    |    |      |
    |   |   |    |    |      |       
    |   |   |    |    |      |
    |------------------------|       
    |   |   |    |    |      |
    |   |   |    |    |      |       
    |------------------------|
    |   |   |    |    |      |       
    |   |   |    |    |      |       
    |------------------------|       
    |   |   |    |    |      |
    --------------------------
       
    */
    if(data == NULL)
        return 1;
    for(i = 2;i<h;i+= 8)
    {
        da = data +  i *w;
        if(CompareLines(da,w))
            return 1;
    }    
    //for(i = 8;i<w;i+= 8)
    //{
    //    da = data +  i ;
    //    if(CompareVers(da,w,h))
    //        return 1;
    //}
   
    return 0; 
    
}

/**
 * @brief Sort by pos (positive-order)
 *
 * @param pos           0:ypos  1:xpos
 * @param win_id 		Win index
 * @param p_fb_info 	Win config data
 * @return 				Errno no
 */

static int  sort_area_by_pos(int pos,int win_id,struct rk_fb_win_cfg_data* p_fb_info)
{
    int i,j,k;
    bool bSwitch;
	if((win_id !=2 && win_id !=3) || p_fb_info==NULL || (pos != 0 && pos != 1))
	{
		ALOGW("%s(%d):invalid param",__FUNCTION__,__LINE__);
		return -1;
	}

	struct rk_fb_area_par tmp_fb_area;
	for(i=0;i<4;i++)
	{
		if(p_fb_info->win_par[i].win_id == win_id)
		{
		    for(j=0;j<3;j++)
		    {
		        bSwitch=false;
                for(k=RK_WIN_MAX_AREA-1;k>j;k--)
                {
                    if((p_fb_info->win_par[i].area_par[k].ion_fd || p_fb_info->win_par[i].area_par[k].phy_addr)  &&
                        (p_fb_info->win_par[i].area_par[k-1].ion_fd || p_fb_info->win_par[i].area_par[k-1].phy_addr) )
                        {
                            if(((pos == 0) && (p_fb_info->win_par[i].area_par[k].ypos < p_fb_info->win_par[i].area_par[k-1].ypos)) ||
                                ((pos == 1) && (p_fb_info->win_par[i].area_par[k].xpos < p_fb_info->win_par[i].area_par[k-1].xpos)) )
                            {
                                //switch
                                memcpy(&tmp_fb_area,&(p_fb_info->win_par[i].area_par[k-1]),sizeof(struct rk_fb_area_par));
                                memcpy(&(p_fb_info->win_par[i].area_par[k-1]),&(p_fb_info->win_par[i].area_par[k]),sizeof(struct rk_fb_area_par));
                                memcpy(&(p_fb_info->win_par[i].area_par[k]),&tmp_fb_area,sizeof(struct rk_fb_area_par));
                                bSwitch=true;
                            }
                        }
                }
                if(!bSwitch)    //end advance
                    return 0;
            }
            break;
        }
    }
	return 0;
}

#if USE_SPECIAL_COMPOSER
static int backupbuffer(hwbkupinfo *pbkupinfo)
{
    struct rga_req  Rga_Request;
    RECT clip;
    int bpp;

    ALOGV("backupbuffer addr=[%x,%x],bkmem=[%x,%x],w-h[%d,%d][%d,%d,%d,%d][f=%d]",
        pbkupinfo->buf_fd, pbkupinfo->buf_addr_log, pbkupinfo->membk_fd, pbkupinfo->pmem_bk_log,pbkupinfo->w_vir,
        pbkupinfo->h_vir,pbkupinfo->xoffset,pbkupinfo->yoffset,pbkupinfo->w_act,pbkupinfo->h_act,pbkupinfo->format);

    bpp = pbkupinfo->format == RK_FORMAT_RGB_565 ? 2:4;

    clip.xmin = 0;
    clip.xmax = pbkupinfo->w_act - 1;
    clip.ymin = 0;
    clip.ymax = pbkupinfo->h_act - 1;

    memset(&Rga_Request, 0x0, sizeof(Rga_Request));

    
    RGA_set_src_vir_info(&Rga_Request, pbkupinfo->buf_fd, 0, 0,pbkupinfo->w_vir, pbkupinfo->h_vir, pbkupinfo->format, 0);
    RGA_set_dst_vir_info(&Rga_Request, pbkupinfo->membk_fd, 0, 0,pbkupinfo->w_act, pbkupinfo->h_act, &clip, pbkupinfo->format, 0);
    //RGA_set_src_vir_info(&Rga_Request, (int)pbkupinfo->buf_addr_log, 0, 0,pbkupinfo->w_vir, pbkupinfo->h_vir, pbkupinfo->format, 0);
    //RGA_set_dst_vir_info(&Rga_Request, (int)pbkupinfo->pmem_bk_log, 0, 0,pbkupinfo->w_act, pbkupinfo->h_act, &clip, pbkupinfo->format, 0);
    //RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);

    RGA_set_bitblt_mode(&Rga_Request, 0, 0,0,0,0,0);
    RGA_set_src_act_info(&Rga_Request,pbkupinfo->w_act,  pbkupinfo->h_act,  pbkupinfo->xoffset, pbkupinfo->yoffset);
    RGA_set_dst_act_info(&Rga_Request, pbkupinfo->w_act,  pbkupinfo->h_act, 0, 0);

   // uint32_t RgaFlag = (i==(RgaCnt-1)) ? RGA_BLIT_SYNC : RGA_BLIT_ASYNC;
    if(ioctl(_contextAnchor->engine_fd, RGA_BLIT_ASYNC, &Rga_Request) != 0) {
        LOGE(" %s(%d) RGA_BLIT fail",__FUNCTION__, __LINE__);
    }
// #endif       
    return 0; 
}
static int restorebuffer(hwbkupinfo *pbkupinfo, int direct_fd)
{
    struct rga_req  Rga_Request;
    RECT clip;
    memset(&Rga_Request, 0x0, sizeof(Rga_Request));

    clip.xmin = 0;
    clip.xmax = pbkupinfo->w_vir - 1;
    clip.ymin = 0;
    clip.ymax = pbkupinfo->h_vir - 1;


    ALOGV("restorebuffer addr=[%x,%x],bkmem=[%x,%x],direct_fd=%x,w-h[%d,%d][%d,%d,%d,%d][f=%d]",
        pbkupinfo->buf_fd, pbkupinfo->buf_addr_log, pbkupinfo->membk_fd, pbkupinfo->pmem_bk_log,direct_fd,pbkupinfo->w_vir,
        pbkupinfo->h_vir,pbkupinfo->xoffset,pbkupinfo->yoffset,pbkupinfo->w_act,pbkupinfo->h_act,pbkupinfo->format);

    //RGA_set_src_vir_info(&Rga_Request, (int)pbkupinfo->pmem_bk_log, 0, 0,pbkupinfo->w_act, pbkupinfo->h_act, pbkupinfo->format, 0);
   // RGA_set_dst_vir_info(&Rga_Request, (int)pbkupinfo->buf_addr_log, 0, 0,pbkupinfo->w_vir, pbkupinfo->h_vir, &clip, pbkupinfo->format, 0);
   // RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);
      
    RGA_set_src_vir_info(&Rga_Request,  pbkupinfo->membk_fd, 0, 0,pbkupinfo->w_act, pbkupinfo->h_act, pbkupinfo->format, 0);
    if(direct_fd)
        RGA_set_dst_vir_info(&Rga_Request, direct_fd, 0, 0,pbkupinfo->w_vir, pbkupinfo->h_vir, &clip, pbkupinfo->format, 0);    
    else
    RGA_set_dst_vir_info(&Rga_Request, pbkupinfo->buf_fd, 0, 0,pbkupinfo->w_vir, pbkupinfo->h_vir, &clip, pbkupinfo->format, 0);
    RGA_set_bitblt_mode(&Rga_Request, 0, 0,0,0,0,0);
    RGA_set_src_act_info(&Rga_Request,pbkupinfo->w_act,  pbkupinfo->h_act, 0, 0);
    RGA_set_dst_act_info(&Rga_Request,pbkupinfo->w_act,  pbkupinfo->h_act,  pbkupinfo->xoffset, pbkupinfo->yoffset);
    if(ioctl(_contextAnchor->engine_fd, RGA_BLIT_ASYNC, &Rga_Request) != 0) {
        LOGE(" %s(%d) RGA_BLIT fail",__FUNCTION__, __LINE__);
    }
    return 0; 
}
static int  CopyBuffByRGA (hwbkupinfo *pcpyinfo)
{
    struct rga_req  Rga_Request;
    RECT clip;
    memset(&Rga_Request, 0x0, sizeof(Rga_Request));
    clip.xmin = 0;
    clip.xmax = pcpyinfo->w_vir - 1;
    clip.ymin = 0;
    clip.ymax = pcpyinfo->h_vir - 1;
    ALOGV("CopyBuffByRGA addr=[%x,%x],bkmem=[%x,%x],w-h[%d,%d][%d,%d,%d,%d][f=%d]",
        pcpyinfo->buf_fd, pcpyinfo->buf_addr_log, pcpyinfo->membk_fd, pcpyinfo->pmem_bk_log,pcpyinfo->w_vir,
        pcpyinfo->h_vir,pcpyinfo->xoffset,pcpyinfo->yoffset,pcpyinfo->w_act,pcpyinfo->h_act,pcpyinfo->format);
    RGA_set_src_vir_info(&Rga_Request,  pcpyinfo->membk_fd, 0, 0,pcpyinfo->w_vir, pcpyinfo->h_vir, pcpyinfo->format, 0);
    RGA_set_dst_vir_info(&Rga_Request, pcpyinfo->buf_fd, 0, 0,pcpyinfo->w_vir, pcpyinfo->h_vir, &clip, pcpyinfo->format, 0);    
    RGA_set_bitblt_mode(&Rga_Request, 0, 0,0,0,0,0);
    RGA_set_src_act_info(&Rga_Request,pcpyinfo->w_act,  pcpyinfo->h_act, pcpyinfo->xoffset,pcpyinfo->yoffset);
    RGA_set_dst_act_info(&Rga_Request,pcpyinfo->w_act,  pcpyinfo->h_act,  pcpyinfo->xoffset, pcpyinfo->yoffset);

   // uint32_t RgaFlag = (i==(RgaCnt-1)) ? RGA_BLIT_SYNC : RGA_BLIT_ASYNC;
    if(ioctl(_contextAnchor->engine_fd, RGA_BLIT_ASYNC, &Rga_Request) != 0) {
        LOGE(" %s(%d) RGA_BLIT fail",__FUNCTION__, __LINE__);
    }
        
    return 0; 
}
static int Is_lcdc_using( int fd)
{
    int i;
    int dsp_fd[4];
    hwcContext * context = _contextAnchor;
    // ioctl 
    // ioctl 
	int sync = 0;
    int count = 0;
    while(!sync)
    {
        count++;
        usleep(1000);
        ioctl(context->fbFd, RK_FBIOGET_LIST_STAT, &sync);
    }
    ioctl(context->fbFd, RK_FBIOGET_DSP_FD, dsp_fd);     
    for(i= 0;i<4;i++)
    {
        if(fd == dsp_fd[i])
            return 1;
    }
    return 0;
}

static int
hwc_buff_recover(        
    int gpuflag
    )
{
    int LcdCont;
    hwbkupinfo cpyinfo;
    int i;
    hwcContext * context = _contextAnchor;
    ZoneManager* pzone_mag = &(context->zone_manager);    
    //bool IsDispDirect = bkupmanage.crrent_dis_fd == bkupmanage.direct_fd;//Is_lcdc_using(bkupmanage.direct_fd);//
    bool IsDispDirect = Is_lcdc_using(bkupmanage.direct_fd);//Is_lcdc_using(bkupmanage.direct_fd);//
    
    int needrev = 0;
    ALOGV("bkupmanage.dstwinNo=%d",bkupmanage.dstwinNo);
    if (context == NULL )
    {
        LOGE("%s(%d):Invalid device!", __FUNCTION__, __LINE__);
        return HWC_EGL_ERROR;
    }
    if(!gpuflag)
    {
        if( pzone_mag->zone_cnt <= 2 )
        {
            return 0;
        }
        LcdCont = 0;
        for(  i= 0; i < pzone_mag->zone_cnt ; i++)
        {
            if(pzone_mag->zone_info[i].dispatched == win0 || \
                pzone_mag->zone_info[i].dispatched == win1
               )
            {
                LcdCont ++;
            }
        }
        if( LcdCont != 2)
        {
            return 0;   // dont need recover
        }
    }
    for (i = 0; i < pzone_mag->zone_cnt && i< 2; i++)
    {
        struct private_handle_t * handle = pzone_mag->zone_info[i].handle;
        if( handle == NULL)
            continue;
        if( handle == bkupmanage.handle_bk && \
            handle->share_fd == bkupmanage.bkupinfo[0].buf_fd )
        {
            ALOGV(" handle->phy_addr==bkupmanage ,name=%s",pzone_mag->zone_info[i].LayerName);
            needrev = 1;
            break;
        }
    }
    if(!needrev)  
        return 0;
    if(!IsDispDirect)
    {
        struct rk_fb_win_cfg_data fb_info ;
        memset(&fb_info,0,sizeof(fb_info));
        cpyinfo.membk_fd = bkupmanage.bkupinfo[bkupmanage.count -1].buf_fd;
        cpyinfo.buf_fd = bkupmanage.direct_fd;
        cpyinfo.xoffset = 0;
        cpyinfo.yoffset = 0;
        cpyinfo.w_vir = bkupmanage.bkupinfo[0].w_vir;        
        cpyinfo.h_vir = bkupmanage.bkupinfo[0].h_vir;
        cpyinfo.w_act = bkupmanage.bkupinfo[0].w_vir;        
        cpyinfo.h_act = bkupmanage.bkupinfo[0].h_vir;
        cpyinfo.format = bkupmanage.bkupinfo[0].format;
        CopyBuffByRGA(&cpyinfo);
        if(ioctl(context->engine_fd, RGA_FLUSH, NULL) != 0)
        {
            LOGE("%s(%d):RGA_FLUSH Failed!", __FUNCTION__, __LINE__);
        }        
        fb_info.win_par[0].area_par[0].data_format = bkupmanage.bkupinfo[0].format;
        fb_info.win_par[0].win_id = 0;
        fb_info.win_par[0].z_order = 0;
        fb_info.win_par[0].area_par[0].ion_fd = bkupmanage.direct_fd;
        fb_info.win_par[0].area_par[0].acq_fence_fd = -1;
        fb_info.win_par[0].area_par[0].x_offset = 0;
        fb_info.win_par[0].area_par[0].y_offset = 0;
        fb_info.win_par[0].area_par[0].xpos = 0;
        fb_info.win_par[0].area_par[0].ypos = 0;
        fb_info.win_par[0].area_par[0].xsize = bkupmanage.bkupinfo[0].w_vir;
        fb_info.win_par[0].area_par[0].ysize = bkupmanage.bkupinfo[0].h_vir;
        fb_info.win_par[0].area_par[0].xact = bkupmanage.bkupinfo[0].w_vir;
        fb_info.win_par[0].area_par[0].yact = bkupmanage.bkupinfo[0].h_vir;
        fb_info.win_par[0].area_par[0].xvir = bkupmanage.bkupinfo[0].w_vir;
        fb_info.win_par[0].area_par[0].yvir = bkupmanage.bkupinfo[0].h_vir;
#if USE_HWC_FENCE
        fb_info.wait_fs = 1;
#endif
        ioctl(context->fbFd, RK_FBIOSET_CONFIG_DONE, &fb_info);

#if USE_HWC_FENCE
	for(int k=0;k<RK_MAX_BUF_NUM;k++)
	{
	    if(fb_info.rel_fence_fd[k]!= -1)
            close(fb_info.rel_fence_fd[k]);
	}

    if(fb_info.ret_fence_fd != -1)
        close(fb_info.ret_fence_fd);
#endif

        bkupmanage.crrent_dis_fd =  bkupmanage.direct_fd;            
    }
    for(i = 0; i < bkupmanage.count;i++)
    {    
        restorebuffer(&bkupmanage.bkupinfo[i],0);
    }
    if(ioctl(context->engine_fd, RGA_FLUSH, NULL) != 0)
    {
        LOGE("%s(%d):RGA_FLUSH Failed!", __FUNCTION__, __LINE__);
    }
    return 0;
}
int
hwc_layer_recover(
    hwc_composer_device_1_t * dev,
    size_t numDisplays,
    hwc_display_contents_1_t** displays
    )
{
    hwc_buff_recover(0);  
    return 0;
}


static int
hwc_LcdcToGpu(  
    hwc_display_contents_1_t* list
    )
{
    if(!bkupmanage.needrev)
        return 0;    
    hwc_buff_recover(1); 
    bkupmanage.needrev = 0;
    return 0; 
}


int hwc_do_special_composer( hwcContext * Context)
{
    int                 srcFd;
    unsigned int        srcWidth;
    unsigned int        srcHeight;
    RgaSURF_FORMAT      srcFormat;
    unsigned int        srcStride;

    int                 dstFd ;
    unsigned int        dstStride;
    unsigned int        dstWidth;
    unsigned int        dstHeight ;
    RgaSURF_FORMAT      dstFormat;
    int                 x_off;
    int                 y_off;
    unsigned int        act_dstwidth;
    unsigned int        act_dstheight;

    RECT clip;
    int DstBuferIndex = -1,ComposerIndex = 0;
    int LcdCont;
    unsigned char       planeAlpha;
    int                 perpixelAlpha;
    int                 curFd = 0;

    struct rga_req  Rga_Request[MAX_DO_SPECIAL_COUNT];
    int             RgaCnt = 0;
    int     dst_indexfid = 0;
    struct private_handle_t *handle_cur = NULL;  
    static int backcout = 0;
    bool IsDiff = 0;
    int dst_bk_fd = 0;
    ZoneManager* pzone_mag = &(Context->zone_manager);
    int i;
    int ext_cnt = 0;
    int ext_st = 0;

    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
        if(pzone_mag->zone_info[i].dispatched  == win_ext)
           ext_cnt ++; 
    }
    if( ext_cnt == 0)
    {
        return 0;
    }
    for(i=0;i<pzone_mag->zone_cnt;i++)
    {
        if(pzone_mag->zone_info[i].dispatched  == win_ext)
        {
            ext_st = i;
            break; 
        }   
    }
    if(ext_st == 0)
    {
       goto BackToGPU;  
    }
    LcdCont = 0;
  
    memset(&Rga_Request, 0x0, sizeof(Rga_Request));
    ALOGD("ext_st=%d,ext_cnt=%d,zone_cnt=%d",ext_st,ext_cnt,pzone_mag->zone_cnt);
    for(ComposerIndex = ext_st ;ComposerIndex < (ext_st + ext_cnt); ComposerIndex++)
    {
        bool NeedBlit = true;
        ZoneInfo *psrcZeInfo = &pzone_mag->zone_info[ComposerIndex];
        srcWidth = psrcZeInfo->width;
        srcHeight = psrcZeInfo->height;
        srcStride = psrcZeInfo->stride;
        srcFd = psrcZeInfo->layer_fd;
        hwcGetFormat( psrcZeInfo->handle,
                 &srcFormat
                 );        
        srcFd = psrcZeInfo->layer_fd;
        for(DstBuferIndex = ext_st -1; DstBuferIndex >=0; DstBuferIndex--)
        {
            ZoneInfo *pdstZeInfo = &pzone_mag->zone_info[DstBuferIndex];

            bool IsWp = strstr(pdstZeInfo->LayerName,WALLPAPER);            
            if( IsWp) {               
                DstBuferIndex = -1;
                break;
            }
                      
            dstWidth = pdstZeInfo->width;
            dstHeight = pdstZeInfo->height;
            dstStride = pdstZeInfo->stride;
            dstFd = pdstZeInfo->layer_fd;            
            hwcGetFormat( pdstZeInfo->handle,
                 &dstFormat
                 );        
                 
            if(dstHeight > 4096) {
                LOGV("  %d->%d: dstHeight=%d > 2048", ComposerIndex, DstBuferIndex, dstHeight);
                continue;   // RGA donot support destination vir_h > 2048
            }
            
            hwc_rect_t const * srcVR = &(psrcZeInfo->disp_rect);
            hwc_rect_t const * dstVR = &(pdstZeInfo->disp_rect);

            LOGD("  %d->%d:  src= rot[%d] fmt[%d] wh[%d(%d),%d] src[%d,%d,%d,%d] vis[%d,%d,%d,%d]",
                ComposerIndex, DstBuferIndex,
                psrcZeInfo->realtransform, srcFormat, srcWidth, srcStride, srcHeight,
                psrcZeInfo->src_rect.left,psrcZeInfo->src_rect.top,
                psrcZeInfo->src_rect.right,psrcZeInfo->src_rect.bottom,
                srcVR->left, srcVR->top, srcVR->right, srcVR->bottom
                );
            LOGD("           dst= rot[%d] fmt[%d] wh[%d(%d),%d] src[%d,%d,%d,%d] vis[%d,%d,%d,%d] ",
                pdstZeInfo->realtransform, dstFormat, dstWidth, dstStride, dstHeight,
                pdstZeInfo->src_rect.left,pdstZeInfo->src_rect.top,
                pdstZeInfo->src_rect.right,pdstZeInfo->src_rect.bottom,
                dstVR->left, dstVR->top, dstVR->right, dstVR->bottom
                );

            // lcdc need address aligned to 128 byte when win1 area is too large.
            // (win0 area consider is large)
            // display width must smaller than dst stride.
            if( dstStride < (unsigned int)(dstVR->right - dstVR->left )) {
                LOGE("  dstStride[%d] < [%d]", dstStride, dstVR->right - dstVR->left);
                DstBuferIndex = -1;
                break;
            }

            // incoming param error, need to debug!
            if(dstVR->right > 4096) {
                LOGE("  dstLayer's VR right (%d) is too big!!!", dstVR->right);
                DstBuferIndex = -1;
                break;
            }

            act_dstwidth = srcWidth;
            act_dstheight = srcHeight;
            x_off = psrcZeInfo->disp_rect.left;
            y_off = psrcZeInfo->disp_rect.top;


          // if the srcLayer inside the dstLayer, then get DstBuferIndex and break.
            if( (srcVR->left >= dstVR->left )
             && (srcVR->top >= dstVR->top )
             && (srcVR->right <= dstVR->right )
             && (srcVR->bottom <= dstVR->bottom )
            )
            {
                handle_cur = pdstZeInfo->handle;
                break;
            }
        }

      //  if(ComposerIndex == 2) // first find ,store
          //  dst_indexfid = DstBuferIndex;
       // else if( DstBuferIndex != dst_indexfid )
         //   DstBuferIndex = -1;
        // there isn't suitable dstLayer to copy, use gpu compose.
        if (DstBuferIndex < 0)      goto BackToGPU;

        // Remove the duplicate copies of bottom bar.
        if(!(bkupmanage.dstwinNo == 0xff || bkupmanage.dstwinNo == DstBuferIndex) )
        {
            ALOGW(" last and current frame is not the win,[%d - %d]",bkupmanage.dstwinNo,DstBuferIndex);
            goto BackToGPU;
        }    
        if(srcFormat == RK_FORMAT_YCbCr_420_SP)
            goto BackToGPU;

        if (NeedBlit)
        {
            curFd = dstFd ;
            clip.xmin = 0;
            clip.xmax = dstStride - 1;
            clip.ymin = 0;
            clip.ymax = dstHeight - 1;
            //x_off  = x_off < 0 ? 0:x_off;


            LOGD("    src[%d]=%s,  dst[%d]=%s",ComposerIndex,pzone_mag->zone_info[ComposerIndex].LayerName,DstBuferIndex,pzone_mag->zone_info[DstBuferIndex].LayerName);
            LOGD("    src info f[%d] w_h[%d(%d),%d]",srcFormat,srcWidth,srcStride,srcHeight);
            LOGD("    dst info f[%d] w_h[%d(%d),%d] rect[%d,%d,%d,%d]",dstFormat,dstWidth,dstStride,dstHeight,x_off,y_off,act_dstwidth,act_dstheight);
            RGA_set_src_vir_info(&Rga_Request[RgaCnt], srcFd, (int)0, 0,srcStride, srcHeight, srcFormat, 0);
            RGA_set_dst_vir_info(&Rga_Request[RgaCnt], dstFd, (int)0, 0,dstStride, dstHeight, &clip, dstFormat, 0);
            /* Get plane alpha. */
            planeAlpha = pzone_mag->zone_info[ComposerIndex].zone_alpha;
            /* Setup blending. */
            {
               
                perpixelAlpha = _HasAlpha(srcFormat);
                LOGV("perpixelAlpha=%d,planeAlpha=%d,line=%d ",perpixelAlpha,planeAlpha,__LINE__);
                /* Setup alpha blending. */
                if (perpixelAlpha && planeAlpha < 255 && planeAlpha != 0)
                {

                   RGA_set_alpha_en_info(&Rga_Request[RgaCnt],1,2, planeAlpha ,1, 9,0);
                }
                else if (perpixelAlpha)
                {
                    /* Perpixel alpha only. */
                   RGA_set_alpha_en_info(&Rga_Request[RgaCnt],1,1, 0, 1, 3,0);

                }
                else /* if (planeAlpha < 255) */
                {
                    /* Plane alpha only. */
                   RGA_set_alpha_en_info(&Rga_Request[RgaCnt],1, 0, planeAlpha ,0,0,0);

                }
            }

            RGA_set_bitblt_mode(&Rga_Request[RgaCnt], 0, 0,0,0,0,0);
            RGA_set_src_act_info(&Rga_Request[RgaCnt],srcWidth, srcHeight,  0, 0);
            RGA_set_dst_act_info(&Rga_Request[RgaCnt], act_dstwidth, act_dstheight, x_off, y_off);

            RgaCnt ++;
        }
    }

#if 0
    // Check Aligned
    if(_contextAnchor->IsRk3188)
    {
        int TotalSize = 0;
        int32_t bpp ;
        bool  IsLarge = false;
        int DstLayerIndex;
        for(int i = 0; i < 2; i++)
        {
            hwc_layer_1_t *dstLayer = &(list->hwLayers[i]);
            hwc_region_t * Region = &(dstLayer->visibleRegionScreen);
            hwc_rect_t const * rects = Region->rects;
            struct private_handle_t * handle_pre = (struct private_handle_t *) dstLayer->handle;
            bpp = android::bytesPerPixel(handle_pre->format);

            TotalSize += (rects[0].right - rects[0].left) \
                            *  (rects[0].bottom - rects[0].top) * 4;
        }
        // fb regard as RGBX , datasize is width * height * 4, so 0.75 multiple is width * height * 4 * 3/4
        if ( TotalSize >= (_contextAnchor->fbWidth * _contextAnchor->fbHeight * 3))
        {
            IsLarge = true;
        }
        for(DstLayerIndex = 1; DstLayerIndex >=0; DstLayerIndex--)
        {
            hwc_layer_1_t *dstLayer = &(list->hwLayers[DstLayerIndex]);

            hwc_rect_t * DstRect = &(dstLayer->displayFrame);
            hwc_rect_t * SrcRect = &(dstLayer->sourceCrop);
            hwc_region_t * Region = &(dstLayer->visibleRegionScreen);
            hwc_rect_t const * rects = Region->rects;
            struct private_handle_t * handle_pre = (struct private_handle_t *) dstLayer->handle;
            hwcRECT dstRects;
            hwcRECT srcRects;
            int xoffset;
            bpp = android::bytesPerPixel(handle_pre->format);

            hwcLockBuffer(_contextAnchor,
                  (struct private_handle_t *) dstLayer->handle,
                  &dstLogical,
                  &dstPhysical,
                  &dstWidth,
                  &dstHeight,
                  &dstStride,
                  &dstInfo);


            dstRects.left   = hwcMAX(DstRect->left,   rects[0].left);

            srcRects.left   = SrcRect->left
                - (int) (DstRect->left   - dstRects.left);

            xoffset = hwcMAX(srcRects.left - dstLayer->exLeft, 0);


            LOGV("[%d]=%s,IsLarge=%d,dstStride=%d,xoffset=%d,exAddrOffset=%d,bpp=%d,dstPhysical=%x",
                DstLayerIndex,list->hwLayers[DstLayerIndex].LayerName,
                IsLarge, dstStride,xoffset,dstLayer->exAddrOffset,bpp,dstPhysical);
            if( IsLarge &&
                ((dstStride * bpp) % 128 || (xoffset * bpp + dstLayer->exAddrOffset) % 128)
            )
            {
                LOGV("  Not 128 aligned && win is too large!") ;
                break;
            }


        }
        if (DstLayerIndex >= 0)     goto BackToGPU;        
    }
    // there isn't suitable dstLayer to copy, use gpu compose.
#endif

	/*
    if(!strcmp("Keyguard",list->hwLayers[DstBuferIndex].LayerName))
    {       
         bkupmanage.skipcnt = 10;
    }
    else if( bkupmanage.skipcnt > 0)
    {
        bkupmanage.skipcnt --;
        if(bkupmanage.skipcnt > 0)
          goto BackToGPU; 
    }
	*/
#if 0   
    if(strcmp(bkupmanage.LayerName,list->hwLayers[DstBuferIndex].LayerName))
    {
        ALOGD("[%s],[%s]",bkupmanage.LayerName,list->hwLayers[DstBuferIndex].LayerName);
        strcpy( bkupmanage.LayerName,list->hwLayers[DstBuferIndex].LayerName);        
        goto BackToGPU; 
    }
#endif    
    // Realy Blit
   // ALOGD("RgaCnt=%d",RgaCnt);
    IsDiff = handle_cur != bkupmanage.handle_bk \
                || (curFd != bkupmanage.bkupinfo[0].buf_fd &&
                    curFd != bkupmanage.bkupinfo[bkupmanage.count -1].buf_fd ); 

    if(!IsDiff )  // restore from current display buffer         
    {
        //if(bkupmanage.crrent_dis_fd != bkupmanage.direct_fd)
        if(!Is_lcdc_using(bkupmanage.direct_fd))
        {
            hwbkupinfo cpyinfo;
            ALOGV("bkupmanage.invalid=%d",bkupmanage.invalid);
            if(bkupmanage.invalid)
            {
                cpyinfo.membk_fd = bkupmanage.bkupinfo[bkupmanage.count -1].buf_fd;
                cpyinfo.buf_fd = bkupmanage.direct_fd;
                cpyinfo.xoffset = 0;
                cpyinfo.yoffset = 0;
                cpyinfo.w_vir = bkupmanage.bkupinfo[0].w_vir;        
                cpyinfo.h_vir = bkupmanage.bkupinfo[0].h_vir;
                cpyinfo.w_act = bkupmanage.bkupinfo[0].w_vir;        
                cpyinfo.h_act = bkupmanage.bkupinfo[0].h_vir;
                cpyinfo.format = bkupmanage.bkupinfo[0].format;
                CopyBuffByRGA(&cpyinfo);
                bkupmanage.invalid = 0;
            }    
            pzone_mag->zone_info[DstBuferIndex].direct_fd = bkupmanage.direct_fd;
            dst_bk_fd = bkupmanage.crrent_dis_fd = bkupmanage.direct_fd;
            for(int i=0; i<RgaCnt; i++)
            {
                Rga_Request[i].dst.yrgb_addr = bkupmanage.direct_fd;
            }
            
        }
        for(int i=0; i<bkupmanage.count; i++) 
        {
            restorebuffer(&bkupmanage.bkupinfo[i],dst_bk_fd);
        }   
        if(!dst_bk_fd  )
            bkupmanage.crrent_dis_fd =  bkupmanage.bkupinfo[bkupmanage.count -1].buf_fd;
    }
    for(int i=0; i<RgaCnt; i++) {

        if(IsDiff) // backup the dstbuff        
        {
            bkupmanage.bkupinfo[i].format = Rga_Request[i].dst.format;
            bkupmanage.bkupinfo[i].buf_fd = Rga_Request[i].dst.yrgb_addr;
            bkupmanage.bkupinfo[i].buf_addr_log = (void*)Rga_Request[i].dst.uv_addr;            
            bkupmanage.bkupinfo[i].xoffset = Rga_Request[i].dst.x_offset;
            bkupmanage.bkupinfo[i].yoffset = Rga_Request[i].dst.y_offset;
            bkupmanage.bkupinfo[i].w_vir = Rga_Request[i].dst.vir_w;
            bkupmanage.bkupinfo[i].h_vir = Rga_Request[i].dst.vir_h;            
            bkupmanage.bkupinfo[i].w_act = Rga_Request[i].dst.act_w;
            bkupmanage.bkupinfo[i].h_act = Rga_Request[i].dst.act_h;  
            if(!i)
            {
                backcout = 0;
                bkupmanage.invalid = 1;
            }    
            bkupmanage.crrent_dis_fd =  bkupmanage.bkupinfo[i].buf_fd;
            if(Rga_Request[i].src.format == RK_FORMAT_RGBA_8888)
                bkupmanage.needrev = 1;
            else
                bkupmanage.needrev = 0;
            backcout ++;
            backupbuffer(&bkupmanage.bkupinfo[i]);

        }
        uint32_t RgaFlag = (i==(RgaCnt-1)) ? RGA_BLIT_SYNC : RGA_BLIT_ASYNC;
        if(ioctl(_contextAnchor->engine_fd, RgaFlag, &Rga_Request[i]) != 0) {
            LOGE(" %s(%d) RGA_BLIT fail",__FUNCTION__, __LINE__);
        }
        bkupmanage.dstwinNo = DstBuferIndex;

    }
    bkupmanage.handle_bk = handle_cur;
    bkupmanage.count = backcout;
    return 0;

BackToGPU:
   // ALOGD(" go brack to GPU");
    return -1;
}
#endif

//extern "C" void *blend(uint8_t *dst, uint8_t *src, int dst_w, int src_w, int src_h);
int hwc_control_3dmode(int num,int flag)
{
    hwcContext * context = _contextAnchor1;
    if(context->fd_3d < 0 || !_contextAnchor1)
        return -1;

    int ret = 0;
    ssize_t err;
    char buf[200];
    int fd = context->fd_3d;
    switch(flag){
    case 0:
        memset(buf,0,sizeof(buf));
        lseek(fd,0,SEEK_SET);
        err = read(fd, buf, sizeof(buf));
        if(err <= 0)
            ALOGW("read hdmi 3dmode err=%d",err);

        int mode,hdmi3dmode;
        //ALOGI("line %d,buf[%s]",__LINE__,buf);
        sscanf(buf,"3dmodes=%d cur3dmode=%d",&mode,&hdmi3dmode);
        //ALOGI("hdmi3dmode=%d,mode=%d",hdmi3dmode,mode);

        if(8==hdmi3dmode)
            ret = 1;
        else if(6==hdmi3dmode)
            ret = 2;
        else if(0==hdmi3dmode)
            ret = 8;
        else 
            ret = 0;
        break;

    case 1:
        lseek(fd,0,SEEK_SET);
        if(1==num)
            ret = write(fd,"8",2);
        else if(2==num)
            ret = write(fd,"6",2);
        else if(8==num)
            ret = write(fd,"0",2);
        else if(0==num)
            ret = write(fd,"-1",3);
        if(ret < 0)
            ALOGW("change 3dmode to %d err is %s",num,strerror(errno));
        break;

    default:
        break;
    }
    return ret;
}

int dump_prepare_info(hwc_display_contents_1_t** displays, int flag)
{
    if(!(mLogL&HLLONE))
        return 0;

    for(int i=0;i<2;i++)
    {
        if(displays[i] != NULL)
        {
            unsigned int num = displays[i]->numHwLayers;
            for(unsigned int j=0;j<num - 1;j++)
            {
                hwc_layer_1_t* layer = &displays[i]->hwLayers[j];
                if (layer != NULL){
#ifdef SUPPORT_STEREO
                    ALOGD("dID=%d,num=%d,layername=%s,ardStereo=%d,dspStereo=%d,cmpType=%d",
                        i,num,layer->LayerName,layer->alreadyStereo,layer->displayStereo,layer->compositionType);
#else
                    ALOGD("dID=%d,num=%d,layername=%s",i,num,layer->LayerName);
#endif          
                }
            }
        }
    }
    return 0;
}

int dump_config_info(struct rk_fb_win_cfg_data fb_info ,hwcContext * context, int flag)
{
    char poutbuf[20];
    char eoutbuf[20];
    bool isLogOut = flag == 3;
    isLogOut = isLogOut || (mLogL&HLLONE);

    if(!isLogOut){
        return 0;
    }

    switch(flag){
    case 0:
        strcpy(poutbuf,"Primary set:");
        strcpy(eoutbuf,"External set:");
        break;

    case 1:
        strcpy(poutbuf,"MIX Primary set:");
        strcpy(eoutbuf,"MIX External set:");
        break;
        
    case 2:
        strcpy(poutbuf,"Primary post:");
        strcpy(eoutbuf,"External post:");
        break;

    case 3:
        strcpy(poutbuf,"PCfg error:");
        strcpy(eoutbuf,"ECfg error:");
        break;

    case 4:
        strcpy(poutbuf,"last config:");
        strcpy(eoutbuf,"last config:");
        break;

    default:
        strcpy(poutbuf,"default:");
        strcpy(eoutbuf,"default:");
        break;
    }
    for(int i = 0;i<4;i++)
    {
        for(int j=0;j<4;j++)
        {
            if(fb_info.win_par[i].area_par[j].ion_fd || fb_info.win_par[i].area_par[j].phy_addr)
                ALOGD("%s win[%d],area[%d],z_win[%d,%d],[%d,%d,%d,%d]=>[%d,%d,%d,%d],w_h_f[%d,%d,%d],fd=%d,addr=%x,fbFd=%d",
                    context==_contextAnchor ? poutbuf : eoutbuf,
                    i,j,
                    fb_info.win_par[i].z_order,
                    fb_info.win_par[i].win_id,
                    fb_info.win_par[i].area_par[j].x_offset,
                    fb_info.win_par[i].area_par[j].y_offset,
                    fb_info.win_par[i].area_par[j].xact,
                    fb_info.win_par[i].area_par[j].yact,
                    fb_info.win_par[i].area_par[j].xpos,
                    fb_info.win_par[i].area_par[j].ypos,
                    fb_info.win_par[i].area_par[j].xsize,
                    fb_info.win_par[i].area_par[j].ysize,
                    fb_info.win_par[i].area_par[j].xvir,
                    fb_info.win_par[i].area_par[j].yvir,
                    fb_info.win_par[i].area_par[j].data_format,
                    fb_info.win_par[i].area_par[j].ion_fd,
                    fb_info.win_par[i].area_par[j].phy_addr,
                    context->fbFd);
        }
    }
    return 0;
}

bool hwc_check_cfg(hwcContext * ctx,struct rk_fb_win_cfg_data fb_info)
{
    bool ret = true;
    bool z_ret = false;
    if(!ctx) {
        ret = false;
        return ret;
    }
    int width,height;
    hwcContext * context = _contextAnchor;
    if(ctx==context) {
        width  = context->dpyAttr[HWCP].xres;
        height = context->dpyAttr[HWCP].yres;
    } else {
        width  = context->dpyAttr[HWCE].xres;
        height = context->dpyAttr[HWCE].yres;
    }
    if(ctx->zone_manager.mCmpType == HWC_MIX_DOWN) {
        z_ret = true;
    }
    for(int i = 0;i<4;i++){
        for(int j=0;j<4;j++){
            if(fb_info.win_par[i].area_par[j].ion_fd || fb_info.win_par[i].area_par[j].phy_addr){
                int z_order = fb_info.win_par[i].z_order;
                int win_id = fb_info.win_par[i].win_id;
                int x_offset = fb_info.win_par[i].area_par[j].x_offset;
                int y_offset = fb_info.win_par[i].area_par[j].y_offset;
                int xact = fb_info.win_par[i].area_par[j].xact;
                int yact = fb_info.win_par[i].area_par[j].yact;
                int xpos = fb_info.win_par[i].area_par[j].xpos;
                int ypos = fb_info.win_par[i].area_par[j].ypos;
                int xsize = fb_info.win_par[i].area_par[j].xsize;
                int ysize = fb_info.win_par[i].area_par[j].ysize;
                int xvir = fb_info.win_par[i].area_par[j].xvir;
                int yvir = fb_info.win_par[i].area_par[j].yvir;
                int data_format = fb_info.win_par[i].area_par[j].data_format;

                z_ret = z_ret || (z_order == 0);//z_order At least once to 0
                ret = ret && (x_offset + xact <= xvir);
                ret = ret && (y_offset + yact <= yvir);
                ret = ret && (xpos + xsize <= width);
                ret = ret && (ypos + ysize <= height);
                if(win_id >= 2){
                    ret = ret && (xact == xsize);
                    ret = ret && (yact == ysize);
                    ret = ret && (data_format <= 7);
                }
                if(!ret){
                    ALOGW("%s[%d,%d]w[%d],a[%d],z_win[%d,%d],[%d,%d,%d,%d]=>[%d,%d,%d,%d],w_h_f[%d,%d,%d]",
                          ctx==_contextAnchor ? "PCfg err" : "ECfg err",width,height,i,j,z_order,win_id,
                          x_offset,y_offset,xact,yact,xpos,ypos,xsize,ysize,xvir,yvir,data_format);
                    break;
                }
            }
        }
        if(!ret){
            break;
        }
    }
    ret = ret && z_ret;
    return ret;
}

int hwc_collect_cfg(hwcContext * context, hwc_display_contents_1_t *list,struct rk_fb_win_cfg_data *fbinfo,int mix_flag,bool mix_prepare)
{
    ZoneManager* pzone_mag = &(context->zone_manager);
    int i,j;
    int z_order = 0;
    int win_no = 0;
    int is_spewin = is_special_wins(context);
    struct rk_fb_win_cfg_data fb_info;

    memset(&fb_info,0,sizeof(fb_info));
    fb_info.ret_fence_fd = -1;
    for(i=0;i<RK_MAX_BUF_NUM;i++) {
        fb_info.rel_fence_fd[i] = -1;
    }

    if(mix_flag == 1 && !context->mHdmiSI.mix_up){
        z_order ++;
    }
    for(i=0;i<pzone_mag->zone_cnt;i++){
        hwc_rect_t * psrc_rect = &(pzone_mag->zone_info[i].src_rect);
        hwc_rect_t * pdisp_rect = &(pzone_mag->zone_info[i].disp_rect);
        int area_no = 0;
        int win_id = 0;
        int raw_format=hwChangeFormatandroidL(pzone_mag->zone_info[i].format);
        ALOGV("hwc_set_lcdc Zone[%d]->layer[%d],dispatched=%d,"
        "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
        "w_h_s_f[%d,%d,%d,0x%x],tr_rtr_bled[%d,%d,%d],"
        "layer_fd[%d],addr=%x,acq_fence_fd=%d"
        "layname=%s",    
        pzone_mag->zone_info[i].zone_index,
        pzone_mag->zone_info[i].layer_index,
        pzone_mag->zone_info[i].dispatched,
        pzone_mag->zone_info[i].src_rect.left,
        pzone_mag->zone_info[i].src_rect.top,
        pzone_mag->zone_info[i].src_rect.right,
        pzone_mag->zone_info[i].src_rect.bottom,
        pzone_mag->zone_info[i].disp_rect.left,
        pzone_mag->zone_info[i].disp_rect.top,
        pzone_mag->zone_info[i].disp_rect.right,
        pzone_mag->zone_info[i].disp_rect.bottom,
        pzone_mag->zone_info[i].width,
        pzone_mag->zone_info[i].height,
        pzone_mag->zone_info[i].stride,
        pzone_mag->zone_info[i].format,
        pzone_mag->zone_info[i].transform,
        pzone_mag->zone_info[i].realtransform,
        pzone_mag->zone_info[i].blend,
        pzone_mag->zone_info[i].layer_fd,
        pzone_mag->zone_info[i].addr,
        pzone_mag->zone_info[i].acq_fence_fd,
        pzone_mag->zone_info[i].LayerName);


        switch(pzone_mag->zone_info[i].dispatched) {
            case win0:
                win_no ++;
                win_id = 0;
                area_no = 0;
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                z_order++;
                break;
            case win1:
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                win_no ++;
                win_id = 1;
                area_no = 0;
                z_order++;
                break;
            case win2_0:
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                win_no ++;
                win_id = 2;
                area_no = 0;
                z_order++;
                break;
            case win2_1:
                win_id = 2;
                area_no = 1;
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                break;
            case win2_2:
                win_id = 2;
                area_no = 2;
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                break;
            case win2_3:
                win_id = 2;
                area_no = 3;
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                break;
            case win3_0:
                win_no ++;
                win_id = 3;
                area_no = 0;
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                z_order++;
                break;
            case win3_1:
                win_id = 3;
                area_no = 1;
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                break;
            case win3_2:
                win_id = 3;
                area_no = 2;
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                break;
            case win3_3:
                win_id = 3;
                area_no = 3;
                ALOGV("[%d]dispatched=%d,z_order=%d",i,pzone_mag->zone_info[i].dispatched,z_order);
                break;
             case win_ext:
                break;
            default:
                ALOGE("hwc_set_lcdc  err!");
                return -1;
         }    

#if G6110_SUPPORT_FBDC
    if(HALPixelFormatGetCompression(pzone_mag->zone_info[i].format) != HAL_FB_COMPRESSION_NONE){
        raw_format = HALPixelFormatGetRawFormat(pzone_mag->zone_info[i].format);
        switch(raw_format){
            case HAL_PIXEL_FORMAT_RGB_565:
                raw_format = FBDC_RGB_565;
                fb_info.win_par[win_no-1].area_par[area_no].fbdc_data_format = FBDC_RGB_565;
                fb_info.win_par[win_no-1].area_par[area_no].fbdc_en= 1;
                fb_info.win_par[win_no-1].area_par[area_no].fbdc_cor_en = 0;
                break;
            case HAL_PIXEL_FORMAT_BGRA_8888:
            case HAL_PIXEL_FORMAT_RGBA_8888:
                raw_format = FBDC_ABGR_888;
                fb_info.win_par[win_no-1].area_par[area_no].fbdc_data_format = FBDC_ABGR_888;
                fb_info.win_par[win_no-1].area_par[area_no].fbdc_en= 1;
                fb_info.win_par[win_no-1].area_par[area_no].fbdc_cor_en = 0;
                break;
            default:
                ALOGE("Unsupport format 0x%x",raw_format);
                break;
        }
    }
#endif

        if(win_no ==1 && !mix_flag)         {
            if(raw_format ==  HAL_PIXEL_FORMAT_RGBA_8888){
                fb_info.win_par[win_no-1].area_par[area_no].data_format = HAL_PIXEL_FORMAT_RGBX_8888;
            }else{
                fb_info.win_par[win_no-1].area_par[area_no].data_format =  raw_format;
            }
                
        }else{
            fb_info.win_par[win_no-1].area_par[area_no].data_format =  raw_format;
        }

        fb_info.win_par[win_no-1].win_id = win_id;
        fb_info.win_par[win_no-1].alpha_mode = AB_SRC_OVER;
        fb_info.win_par[win_no-1].g_alpha_val =  pzone_mag->zone_info[i].zone_alpha;
        fb_info.win_par[win_no-1].z_order = z_order-1;
        fb_info.win_par[win_no-1].area_par[area_no].ion_fd = \
                        pzone_mag->zone_info[i].direct_fd ? \
                        pzone_mag->zone_info[i].direct_fd: pzone_mag->zone_info[i].layer_fd;     
        fb_info.win_par[win_no-1].area_par[area_no].phy_addr = pzone_mag->zone_info[i].addr;
#if USE_HWC_FENCE
        fb_info.win_par[win_no-1].area_par[area_no].acq_fence_fd = -1;//pzone_mag->zone_info[i].acq_fence_fd;
#else
        fb_info.win_par[win_no-1].area_par[area_no].acq_fence_fd = -1;
#endif
        fb_info.win_par[win_no-1].area_par[area_no].x_offset = hwcMAX(psrc_rect->left, 0);
        fb_info.win_par[win_no-1].area_par[area_no].y_offset = hwcMAX(psrc_rect->top, 0);
        fb_info.win_par[win_no-1].area_par[area_no].xpos =  hwcMAX(pdisp_rect->left, 0);
        fb_info.win_par[win_no-1].area_par[area_no].ypos = hwcMAX(pdisp_rect->top , 0);
        fb_info.win_par[win_no-1].area_par[area_no].xsize = pdisp_rect->right - pdisp_rect->left;
        fb_info.win_par[win_no-1].area_par[area_no].ysize = pdisp_rect->bottom - pdisp_rect->top;
        if(pzone_mag->zone_info[i].transform == HWC_TRANSFORM_ROT_90
            || pzone_mag->zone_info[i].transform == HWC_TRANSFORM_ROT_270){
           
            if(!context->mNV12_VIDEO_VideoMode){
                fb_info.win_par[win_no-1].area_par[area_no].xact = psrc_rect->right- psrc_rect->left;
                fb_info.win_par[win_no-1].area_par[area_no].yact = psrc_rect->bottom - psrc_rect->top;
            }else{
                //Only for NV12_VIDEO
                fb_info.win_par[win_no-1].area_par[area_no].xact = psrc_rect->bottom - psrc_rect->top;
                fb_info.win_par[win_no-1].area_par[area_no].yact = psrc_rect->right- psrc_rect->left;
            }

            fb_info.win_par[win_no-1].area_par[area_no].xvir = pzone_mag->zone_info[i].height ;
            fb_info.win_par[win_no-1].area_par[area_no].yvir = pzone_mag->zone_info[i].stride;  
        }else{
            fb_info.win_par[win_no-1].area_par[area_no].xact = psrc_rect->right- psrc_rect->left;
            fb_info.win_par[win_no-1].area_par[area_no].yact = psrc_rect->bottom - psrc_rect->top;
            fb_info.win_par[win_no-1].area_par[area_no].xvir = pzone_mag->zone_info[i].stride;
            fb_info.win_par[win_no-1].area_par[area_no].yvir = pzone_mag->zone_info[i].height;
        }    
    }    

#ifndef GPU_G6110
    //win2 & win3 need sort by ypos (positive-order)
    sort_area_by_pos(0,2,&fb_info);
    sort_area_by_pos(0,3,&fb_info);
#else
    //win2 & win3 need sort by xpos (positive-order)
    sort_area_by_pos(1,2,&fb_info);
    sort_area_by_pos(1,3,&fb_info);
#endif

#if VIDEO_UI_OPTIMATION
    if(fb_info.win_par[0].area_par[0].data_format == HAL_PIXEL_FORMAT_YCrCb_NV12_OLD
        && list->numHwLayers == 3)  // @ video & 2 layers
    {
        bool IsDiff = true;
        int ret;
        hwc_layer_1_t * layer = &list->hwLayers[1];
        if(layer){
            struct private_handle_t* handle = (struct private_handle_t *) layer->handle;
            if(handle && (handle->format == HAL_PIXEL_FORMAT_RGBA_8888 ||
                    handle->format == HAL_PIXEL_FORMAT_RGBX_8888 ||
                    handle->format == HAL_PIXEL_FORMAT_BGRA_8888)){
                IsDiff = handle->share_fd != context->vui_fd;
            }
            if(IsDiff){
                context->vui_hide = 0;
            }else if(!context->vui_hide){
                ret = DetectValidData((int *)(GPU_BASE),handle->width,handle->height);
                if(!ret){
                    context->vui_hide = 1;
                    ALOGD(" @video UI close");
                }
            }
            // close UI win:external always do it
            if(context->vui_hide == 1){
                for(i = 1;i<4;i++){
                    for(j=0;j<4;j++){
                        fb_info.win_par[i].area_par[j].ion_fd = 0;
                        fb_info.win_par[i].area_par[j].phy_addr = 0;
                    }
                }
            }
            context->vui_fd = handle->share_fd;
        }
    }
#endif

#if DEBUG_CHECK_WIN_CFG_DATA
    for(i = 0;i<4;i++){
        for(j=0;j<4;j++){
            if(fb_info.win_par[i].area_par[j].ion_fd || fb_info.win_par[i].area_par[j].phy_addr){
                #if 1
                if(fb_info.win_par[i].z_order<0 ||
                fb_info.win_par[i].win_id < 0 || fb_info.win_par[i].win_id > 4 ||
                fb_info.win_par[i].g_alpha_val < 0 || fb_info.win_par[i].g_alpha_val > 0xFF ||
                fb_info.win_par[i].area_par[j].x_offset < 0 || fb_info.win_par[i].area_par[j].y_offset < 0 ||
                fb_info.win_par[i].area_par[j].x_offset > 4096 || fb_info.win_par[i].area_par[j].y_offset > 4096 ||
                fb_info.win_par[i].area_par[j].xact < 0 || fb_info.win_par[i].area_par[j].yact < 0 ||
                fb_info.win_par[i].area_par[j].xact > 4096 || fb_info.win_par[i].area_par[j].yact > 4096 ||
                fb_info.win_par[i].area_par[j].xpos < 0 || fb_info.win_par[i].area_par[j].ypos < 0 ||
                fb_info.win_par[i].area_par[j].xpos >4096 || fb_info.win_par[i].area_par[j].ypos > 4096 ||
                fb_info.win_par[i].area_par[j].xsize < 0 || fb_info.win_par[i].area_par[j].ysize < 0 ||
                fb_info.win_par[i].area_par[j].xsize > 4096 || fb_info.win_par[i].area_par[j].ysize > 4096 ||
                fb_info.win_par[i].area_par[j].xvir < 0 ||  fb_info.win_par[i].area_par[j].yvir < 0 ||
                fb_info.win_par[i].area_par[j].xvir > 4096 || fb_info.win_par[i].area_par[j].yvir > 4096 ||
                fb_info.win_par[i].area_par[j].ion_fd < 0)
                #endif
                ALOGE("par[%d],area[%d],z_win_galp[%d,%d,%x],[%d,%d,%d,%d]=>[%d,%d,%d,%d],w_h_f[%d,%d,%d],acq_fence_fd=%d,fd=%d,addr=%x",
                        i,j,
                        fb_info.win_par[i].z_order,
                        fb_info.win_par[i].win_id,
                        fb_info.win_par[i].g_alpha_val,
                        fb_info.win_par[i].area_par[j].x_offset,
                        fb_info.win_par[i].area_par[j].y_offset,
                        fb_info.win_par[i].area_par[j].xact,
                        fb_info.win_par[i].area_par[j].yact,
                        fb_info.win_par[i].area_par[j].xpos,
                        fb_info.win_par[i].area_par[j].ypos,
                        fb_info.win_par[i].area_par[j].xsize,
                        fb_info.win_par[i].area_par[j].ysize,
                        fb_info.win_par[i].area_par[j].xvir,
                        fb_info.win_par[i].area_par[j].yvir,
                        fb_info.win_par[i].area_par[j].data_format,
                        fb_info.win_par[i].area_par[j].acq_fence_fd,
                        fb_info.win_par[i].area_par[j].ion_fd,
                        fb_info.win_par[i].area_par[j].phy_addr);
              }
        }
        
    }    
#endif

#if USE_HWC_FENCE
#if SYNC_IN_VIDEO
    if(context->mVideoMode && !context->mIsMediaView && !g_hdmi_mode)
        fb_info.wait_fs=1;
    else
#endif
        fb_info.wait_fs=0;  //not wait acquire fence temp(wait in hwc)
#endif
    //if primary the y_offset will be n times of height
    if(mix_flag && !mix_prepare){
        int numLayers = list->numHwLayers;
        int format = -1;
        hwc_layer_1_t *fbLayer = &list->hwLayers[numLayers - 1];
        if (!fbLayer){
            ALOGE("fbLayer=NULL");
            return -1;
        }
        struct private_handle_t*  handle = (struct private_handle_t*)fbLayer->handle;
        if (!handle){
            ALOGD_IF(mLogL&HLLFOU,"hanndle=NULL at line %d",__LINE__);
            return -1;
        }

        win_no ++;
        if(mix_flag == 1 && !context->mHdmiSI.mix_up){
            format = context->fbhandle.format;
            z_order=1;
        }else if(mix_flag == 2 || (mix_flag == 1 && context->mHdmiSI.mix_up)){
            format = HAL_PIXEL_FORMAT_RGBA_8888;
            z_order++;
        }

        ALOGV("mix_flag=%d,win_no =%d,z_order = %d",mix_flag,win_no,z_order);
        unsigned int offset = handle->offset;
        fb_info.win_par[win_no-1].area_par[0].data_format = format;
        if(context->mHdmiSI.mix_vh && mix_flag == 2){
            fb_info.win_par[win_no-1].win_id = 1;
        }else{
            fb_info.win_par[win_no-1].win_id = 3;
        }
        fb_info.win_par[win_no-1].z_order = z_order-1;
        if(pzone_mag->mCmpType == HWC_MIX_CROSS){
            fb_info.win_par[win_no-1].z_order = z_order-2;
            fb_info.win_par[win_no-2].z_order = z_order-1;
        }
        fb_info.win_par[win_no-1].area_par[0].ion_fd = handle->share_fd;
#if USE_HWC_FENCE
        fb_info.win_par[win_no-1].area_par[0].acq_fence_fd = -1;//fbLayer->acquireFenceFd;
#else
        fb_info.win_par[win_no-1].area_par[0].acq_fence_fd = -1;
#endif
        fb_info.win_par[win_no-1].area_par[0].x_offset = 0;
        fb_info.win_par[win_no-1].area_par[0].y_offset = offset/context->fbStride;
        fb_info.win_par[win_no-1].area_par[0].xpos = 0;
        fb_info.win_par[win_no-1].area_par[0].ypos = 0;
        fb_info.win_par[win_no-1].area_par[0].xsize = handle->width;
        fb_info.win_par[win_no-1].area_par[0].ysize = handle->height;
        fb_info.win_par[win_no-1].area_par[0].xact = handle->width;
        fb_info.win_par[win_no-1].area_par[0].yact = handle->height;
        fb_info.win_par[win_no-1].area_par[0].xvir = handle->stride;
        fb_info.win_par[win_no-1].area_par[0].yvir = handle->height;
#if USE_HWC_FENCE
#if SYNC_IN_VIDEO
    if(context->mVideoMode && !context->mIsMediaView && !g_hdmi_mode)
        fb_info.wait_fs=1;
    else
#endif
        fb_info.wait_fs=0;
#endif
    }
    memcpy((void*)fbinfo,(void*)&fb_info,sizeof(rk_fb_win_cfg_data));
    return 0;
}

int hwc_pre_prepare(hwc_display_contents_1_t** displays, int flag)
{
    mLogL = is_out_log();
    int forceStereop = 0;
    int forceStereoe = 0;
    hwcContext * contextp = _contextAnchor;
    hwcContext * contexte = _contextAnchor1;
    contextp->Is3D = false;
    if(contexte!=NULL){
        contexte->Is3D = false;
    }
#ifdef RK3288_BOX
    if(contextp->mLcdcNum == 2){
        int xres = contextp->dpyAttr[HWC_DISPLAY_PRIMARY].xres;
        int yres = contextp->dpyAttr[HWC_DISPLAY_PRIMARY].yres;
        int relxres = contextp->dpyAttr[HWC_DISPLAY_PRIMARY].relxres;
        int relyres = contextp->dpyAttr[HWC_DISPLAY_PRIMARY].relyres;
        if(xres != relxres || yres != relyres){
            contextp->mResolutionChanged = true;
        }else{
            contextp->mResolutionChanged = false;
        }
    }
#endif
#ifdef SUPPORT_STEREO
#if SUPPORTFORCE3D
    char pro_valuep[PROPERTY_VALUE_MAX];
    char pro_valuee[PROPERTY_VALUE_MAX];
    property_get("sys.hwc.force3d.primary",pro_valuep,0);
    property_get("sys.hwc.force3d.external",pro_valuee,0);
    int force3dp = atoi(pro_valuep);
    int force3de = atoi(pro_valuee);
    if(1==force3dp || 2==force3dp){
        forceStereoe = forceStereop = force3dp;
    }else{
        forceStereoe = forceStereop = 0;
    }
    //if(1==force3de || 2==force3de){
    //    forceStereoe = force3de;
    //}else{
    //    forceStereoe = 0;
    //}
#endif
    for(int i=0;i<2;i++){
        if(displays[i] != NULL){
            unsigned int numlayer = displays[i]->numHwLayers;
            int needStereo = 0;
            for (unsigned int j = 0; j <(numlayer - 1); j++) {
                if(displays[i]->hwLayers[j].alreadyStereo) {
                    needStereo = displays[i]->hwLayers[j].alreadyStereo;
                    break;
                }
            }

            if(0==i && forceStereop){
                needStereo = forceStereop;
            }else if(1==i && forceStereoe){
                needStereo = forceStereoe;
            }

            if(needStereo) {
                if(0==i){
                    contextp->Is3D = true;
                }else if(1==i && contexte){
                    contexte->Is3D = true;
                }
                for (unsigned int j = 0; j <(numlayer - 1); j++) {
                    displays[i]->hwLayers[j].displayStereo = needStereo;
                }
            }else{
                for (unsigned int j = 0; j <(numlayer - 1); j++) {
                    displays[i]->hwLayers[j].displayStereo = needStereo;
                }
            }

            if(1==i && numlayer > 1 && needStereo != hwc_control_3dmode(2,0)){
                hwc_control_3dmode(needStereo,1);
            }
        }
    }
#endif

#if (defined(GPU_G6110) || defined(RK3288_BOX))
#ifdef RK3288_BOX
    if(contextp->mLcdcNum == 2){
        return 0;
    }
#endif
#if USE_WM_SIZE
    contextp->mHdmiSI.hdmi_anm = 0;
    contextp->mHdmiSI.anroidSt = false;

    for(int i=0;i<2;i++)
    {
        if(displays[i] != NULL)
        {
            unsigned int numlayer = displays[i]->numHwLayers;
            for(unsigned int j=0;j<numlayer - 1;j++)
            {
                hwc_layer_1_t* layer = &displays[i]->hwLayers[j];
                struct private_handle_t* SrcHnd = (struct private_handle_t *) layer->handle;
                if (layer == NULL){
                    ALOGW("%s,%d,layer is null");
                }else if(strstr(layer->LayerName,"BootAnimation") != NULL && (getHdmiMode()==1 
                    || contextp->mHdmiSI.CvbsOn)){
                    layer->sourceCrop.left = 0;
                    layer->sourceCrop.top = 0;
                    layer->sourceCrop.right = SrcHnd->stride;
                    layer->sourceCrop.bottom = SrcHnd->height;
                    contextp->mHdmiSI.hdmi_anm = 1;
                }else if(strstr(layer->LayerName,"Android ") == layer->LayerName && 
                (getHdmiMode() == 1|| contextp->mHdmiSI.CvbsOn)){
                    contextp->mHdmiSI.hdmi_anm = 1;
                    contextp->mHdmiSI.anroidSt = true;
                }
            }
        }
    }
#endif
#endif
    return 0;
}

int hwc_try_policy(hwcContext * context,hwc_display_contents_1_t * list,int dpyID)
{
    int ret;
    for(int i = 0;i < HWC_POLICY_NUM;i++){
        ret = context->fun_policy[i]((void*)context,list);
        if(!ret){
            break; // find the Policy
        }
    }
    return ret;
}
    
int hwc_prepare_virtual(hwc_composer_device_1_t * dev, hwc_display_contents_1_t  *list)
{
	if (list==NULL)
	{
		return -1;
	}

    hwcContext * context = _contextAnchor;

    context->wfdRgaBlit = false;
	for (size_t j = 0; j <(list->numHwLayers - 1); j++)
	{
		struct private_handle_t * handle = (struct private_handle_t *)list->hwLayers[j].handle;

		if (handle && GPU_FORMAT==HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO)
		{
			ALOGV("WFD rga_video_copybit,%x,w=%d,h=%d",\
				GPU_BASE,GPU_WIDTH,GPU_HEIGHT);
			if (context->wfdOptimize==0)
			{
				rga_video_copybit(handle,0,0,0,handle->share_fd,RK_FORMAT_YCbCr_420_SP,0,HWCV);
			}
		}
	}
#if VIRTUAL_RGA_BLIT
    unsigned int  i ;
    bool mBlit = true;
    int  pixelSize  = 0;

    /*if wfdOptimize,than return*/
    if(context->wfdOptimize > 0)
        return 0;
        
    for (  i = 0; i < (list->numHwLayers - 1); i++)
    {
        hwc_layer_1_t * layer = &list->hwLayers[i];
        struct private_handle_t * handle = (struct private_handle_t *)layer->handle;
        if ((layer->flags & HWC_SKIP_LAYER) || (handle == NULL))
        {
            return 0;
        }
    }

    for (i = 0; i < (list->numHwLayers - 1); i++)
    {
        hwc_layer_1_t * layer = &list->hwLayers[i];
        struct private_handle_t * handle = (struct private_handle_t *)layer->handle;

        pixelSize += ((layer->sourceCrop.bottom - layer->sourceCrop.top) * \
                        (layer->sourceCrop.right - layer->sourceCrop.left));
        if(pixelSize > 4718592)  // pixel too large,RGA done use more time
        {
            mBlit = false;
            break;
        }
        layer->compositionType = HWC_BLITTER;
    }

    if(!mBlit)
    {
        for (i = 0; i < (list->numHwLayers - 1); i++)
        {
            hwc_layer_1_t * layer = &list->hwLayers[i];
            layer->compositionType = HWC_FRAMEBUFFER;
        }
    }else
        context->wfdRgaBlit = true;
#endif
	return 0;
}

static int hwc_prepare_screen(hwc_composer_device_1 *dev, hwc_display_contents_1_t *list, int dpyID) 
{
    ATRACE_CALL();

	size_t i;
    size_t j;

    hwcContext * context = _contextAnchor;
    if(dpyID == HWCE){
        context = _contextAnchor1;
    }
 
    int ret;
    int err;    
    bool vertical = false;
    struct private_handle_t * handles[MAX_VIDEO_SOURCE];
    int index=0;
    int video_sources=0;
    int iVideoSources;
    int m,n;
    int vinfo_cnt = 0;
    int video_cnt = 0;
    int mix_index = dpyID;
    int transformcnt = 0;
    bool bIsMediaView=false;
    char gts_status[PROPERTY_VALUE_MAX];

    /* Check layer list. */
    if (list == NULL || (list->numHwLayers  == 0)/*||!(list->flags & HWC_GEOMETRY_CHANGED)*/){
        ALOGD_IF(mLogL&HLLTWO,"dpyID=%d list null",dpyID);
        return 0;
    }
    if(is_gpu_or_nodraw(list,dpyID)){
        return 0;
    }
#if (defined(GPU_G6110) || defined(RK3288_BOX))
#if USE_WM_SIZE
    if(_contextAnchor->mHdmiSI.anroidSt){
        goto GpuComP;
    }
#endif
#endif

    LOGV("%s(%d):>>> hwc_prepare_primary %d layers <<<",
         __FUNCTION__,
         __LINE__,
         list->numHwLayers -1);

    
#if GET_VPU_INTO_FROM_HEAD
    //init handles,reset bMatch
    for (i = 0; i < MAX_VIDEO_SOURCE; i++){
        handles[i]=NULL;
        context->video_info[i].bMatch=false;
    }
#endif

    for (i = 0; i < (list->numHwLayers - 1); i++){
        struct private_handle_t * handle = (struct private_handle_t *) list->hwLayers[i].handle;
        if(handle && GPU_FORMAT == HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO){
            video_cnt ++;
        }
        if(handle && GPU_FORMAT == HAL_PIXEL_FORMAT_YV12){
            ALOGD_IF(mLogL&HLLFOU,"HAL_PIXEL_FORMAT_YV12 out");
            goto GpuComP;
        }
    }
    context->mVideoMode=false;
    context->mVideoRotate=false;
    context->mIsMediaView=false;
    context->mHdmiSI.mix_up=false;
    context->mHdmiSI.mix_vh=false;
    context->mNeedRgaTransform = false;
    context->mNV12_VIDEO_VideoMode=false;
#if OPTIMIZATION_FOR_DIMLAYER
    context->bHasDimLayer = false;
#endif
    context->mtrsformcnt  = 0;
    for (i = 0; i < (list->numHwLayers - 1); i++){
        struct private_handle_t * handle = (struct private_handle_t *) list->hwLayers[i].handle;

#if 0
        if(handle)
            ALOGV("layer name=%s,format=%d",list->hwLayers[i].LayerName,GPU_FORMAT);
#endif
        if(!strcmp(list->hwLayers[i].LayerName,"MediaView"))
            context->mIsMediaView=true;

        if(list->hwLayers[i].transform != 0){
            context->mtrsformcnt ++;
        }

        if(handle && GPU_FORMAT == HAL_PIXEL_FORMAT_YCrCb_NV12){
            context->mVideoMode = true;
        }

        if(handle && GPU_FORMAT == HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO){
            tVPU_FRAME vpu_hd;

            context->mVideoMode = true;
            context->mNV12_VIDEO_VideoMode = true;

            ALOGV("video");
#if GET_VPU_INTO_FROM_HEAD
            for(m=0;m<MAX_VIDEO_SOURCE;m++){
                ALOGV("m=%d,[%p,%p],[%p,%p]",m,context->video_info[m].video_hd,handle,
                    context->video_info[m].video_base,(void*)handle->base);
                if( (context->video_info[m].video_hd == handle)&& handle->video_width != 0
                    && (context->video_info[m].video_base == (void*)handle->base)){
                    //match video,but handle info been update
                    context->video_info[m].bMatch=true;
                    break;
                }

            }

            //if can't find any match video in back video source,then update handle
            if(m == MAX_VIDEO_SOURCE )
#endif
            {
#if GET_VPU_INTO_FROM_HEAD
                memcpy(&vpu_hd,(void*)(GPU_BASE),sizeof(tVPU_FRAME));
#else
#if defined(__arm64__) || defined(__aarch64__)
                memcpy(&vpu_hd,(void*)((unsigned long)GPU_BASE+2*handle->stride*handle->height),sizeof(tVPU_FRAME));
#else
                memcpy(&vpu_hd,(void*)((unsigned int)GPU_BASE+2*handle->stride*handle->height),sizeof(tVPU_FRAME));
#endif
#endif
                //if find invalid params,then increase iVideoSources and try again.
                if(vpu_hd.FrameWidth>8192 || vpu_hd.FrameWidth <=0 || \
                    vpu_hd.FrameHeight>8192 || vpu_hd.FrameHeight<=0){
                    ALOGE("invalid video(w=%d,h=%d)",vpu_hd.FrameWidth,vpu_hd.FrameHeight);
                }

                handle->video_addr = vpu_hd.FrameBusAddr[0];
                handle->video_width = vpu_hd.FrameWidth;
                handle->video_height = vpu_hd.FrameHeight;
                handle->video_disp_width = vpu_hd.DisplayWidth;
                handle->video_disp_height = vpu_hd.DisplayHeight;

#if WRITE_VPU_FRAME_DATA
                if(hwc_get_int_property("sys.hwc.write_vpu_frame_data","0")){
                    static FILE* pOutFile = NULL;
                    VPUMemLink(&vpu_hd.vpumem);
                    pOutFile = fopen("/data/raw.yuv", "wb");
                    if (pOutFile) {
                        ALOGE("pOutFile open ok!");
                    } else {
                        ALOGE("pOutFile open fail!");
                    }
                    fwrite(vpu_hd.vpumem.vir_addr,1, vpu_hd.FrameWidth*vpu_hd.FrameHeight*3/2, pOutFile);
                 }
#endif

#if GET_VPU_INTO_FROM_HEAD
                //record handle in handles
                handles[index]=handle;
                index++;
#endif
                ALOGV("prepare [%x,%dx%d] active[%d,%d]",handle->video_addr,handle->video_width,\
                    handle->video_height,vpu_hd.DisplayWidth,vpu_hd.DisplayHeight);

                context->video_fmt = vpu_hd.OutputWidth;
                if(context->video_fmt !=HAL_PIXEL_FORMAT_YCrCb_NV12
                    && context->video_fmt !=HAL_PIXEL_FORMAT_YCrCb_NV12_10)
                    context->video_fmt = HAL_PIXEL_FORMAT_YCrCb_NV12;   // Compatible old sf lib 
                ALOGV("context->video_fmt =%d",context->video_fmt);
            }
        }
        if(list->hwLayers[i].realtransform == HAL_TRANSFORM_ROT_90
            || list->hwLayers[i].realtransform == HAL_TRANSFORM_ROT_270 ){
            vertical = true;
        }

    }

#if GET_VPU_INTO_FROM_HEAD
    for (i = 0; i < index; i++){
        struct private_handle_t * handle = handles[i];
        if(handle == NULL)
            continue;

        for(m=0;m<MAX_VIDEO_SOURCE;m++){
            //save handle into video_info which doesn't match before.
            if(!context->video_info[m].bMatch){
                ALOGV("save handle=%p,base=%p,w=%d,h=%d",handle,GPU_BASE,handle->video_width,handle->video_height);
                context->video_info[m].video_hd = handle ;
                context->video_info[m].video_base = (void*)(GPU_BASE);
                context->video_info[m].bMatch=true;
                vinfo_cnt++;
                break;
            }
         }
    }

    for(m=0;m<MAX_VIDEO_SOURCE;m++){
        //clear handle into video_info which doesn't match before.
        ALOGV("cancel m=%d,handle=%p,base=%p",m,context->video_info[m].video_hd,context->video_info[m].video_base);
        if(!context->video_info[m].bMatch)
        {
            context->video_info[m].video_hd = NULL;
            context->video_info[m].video_base = NULL;
        }
    }
#endif

    if(video_cnt >1){
        // more videos goto gpu cmp
        ALOGW("more2 video=%d goto GpuComP",video_cnt);
		ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        goto GpuComP;
    }          
    //Get gts staus,save in context->mGtsStatus
    hwc_get_string_property("sys.cts_gts.status","false",gts_status);
    if(!strcmp(gts_status,"true")){
        context->mGtsStatus = true;
    }else{
        context->mGtsStatus = false;
    }

    context->mTrsfrmbyrga = false;
    /* Check all layers: tag with different compositionType. */
    for (i = 0; i < (list->numHwLayers - 1); i++){
        hwc_layer_1_t * layer = &list->hwLayers[i];
        if(layer->transform){
            transformcnt ++;
        }
        uint32_t compositionType =
             check_layer(context, list->numHwLayers - 1, i,context->mVideoMode, layer);

        if (compositionType == HWC_FRAMEBUFFER){
            break;
        }
    }

    if(hwc_get_int_property("sys.hwc.disable","0")== 1){
		ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        goto GpuComP;
	}
    /* Roll back to FRAMEBUFFER if any layer can not be handled. */
    if (i != (list->numHwLayers - 1) || (list->numHwLayers==1) /*|| context->mtrsformcnt > 1*/){
		ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        goto GpuComP;
    }

#if !ENABLE_LCDC_IN_NV12_TRANSFORM
    if(!context->mGtsStatus){
        if(context->mVideoMode &&  !context->mNV12_VIDEO_VideoMode && context->mtrsformcnt>0){
            ALOGV("Go into GLES,in nv12 transform case");
			ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
            goto GpuComP;
        }
    }
#endif

    for (i = 0; i < (list->numHwLayers - 1); i++){
        struct private_handle_t * handle = (struct private_handle_t *) list->hwLayers[i].handle;
        int stride_gr;
        int video_w=0,video_h=0;

        if(GPU_FORMAT == HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO){
            video_w = handle->video_width;
            video_h = handle->video_height;
        }else if(GPU_FORMAT == HAL_PIXEL_FORMAT_YCrCb_NV12){
            video_w = handle->width;
            video_h = handle->height;
        }

        //alloc video gralloc buffer in video mode
        if(context->fd_video_bk[0] == -1 && context->mTrsfrmbyrga){
            ALOGD_IF(mLogL&HLLFOU,"mNV12_VIDEO_VideoMode=%d,mTrsfrmbyrga=%d,w=%d,h=%d",
                context->mNV12_VIDEO_VideoMode,context->mTrsfrmbyrga,video_w,video_h);
            for(j=0;j<MaxVideoBackBuffers;j++){
                err = context->mAllocDev->alloc(context->mAllocDev, RWIDTH,RHEIGHT,HAL_PIXEL_FORMAT_YCrCb_NV12, \
                    GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_RENDER|GRALLOC_USAGE_HW_VIDEO_ENCODER, \
                    (buffer_handle_t*)(&(context->pbvideo_bk[j])),&stride_gr);
                if(!err){
                    struct private_handle_t*handle = (struct private_handle_t*)context->pbvideo_bk[j];
                    context->fd_video_bk[j] = handle->share_fd;
#if defined(__arm64__) || defined(__aarch64__)
                    context->base_video_bk[j]= (long)(GPU_BASE);
#else
                    context->base_video_bk[j]= (int)(GPU_BASE);
#endif
                    ALOGD_IF(mLogL&HLLTWO,"video alloc fd [%dx%d,f=%d],fd=%d",
                        handle->width,handle->height,handle->format,handle->share_fd);

                }else {
                    ALOGE("video alloc faild video(w=%d,h=%d,format=0x%x,error=%s)",handle->video_width,
                        handle->video_height,context->fbhandle.format,strerror(errno));
					ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
					for(size_t k=0;k<j;k++){
					    if(context->fd_video_bk[k] != -1){
                            err = context->mAllocDev->free(context->mAllocDev,context->pbvideo_bk[k]);
                            if(err){
                                ALOGW("free back buff error %s,%d,%d",strerror(errno),j,k);
                            }
                            context->fd_video_bk[k] = -1;
                        }
					}
					context->fd_video_bk[j] != -1;
					goto GpuComP;
                }
            }
        }
    }


    // free video gralloc buffer in ui mode
    if(context->fd_video_bk[0] > 0 &&
        (/*!context->mVideoMode*/(
#if USE_VIDEO_BACK_BUFFERS
        !context->mNV12_VIDEO_VideoMode && 
#endif
        !context->mTrsfrmbyrga) || (video_cnt >1))){
        err = 0;
        for(i=0;i<MaxVideoBackBuffers;i++){
            ALOGD_IF(mLogL&HLLTWO,"dpyID=%d,free video fd=%d,base=%p,%p",
                dpyID,context->fd_video_bk[i],context->base_video_bk[i],context->pbvideo_bk[i]);
            if(context->pbvideo_bk[i] != NULL)
                err = context->mAllocDev->free(context->mAllocDev, context->pbvideo_bk[i]);
            if(!err){
                context->fd_video_bk[i] = -1;
                context->base_video_bk[i] = 0;
                context->pbvideo_bk[i] = NULL;
            }
            ALOGW_IF(err, "free(...) failed %d (%s)", err, strerror(-err));
        }
    }

    //G6110 FBDC: only let video case continue.
#if G6110_SUPPORT_FBDC
    if(!context->mVideoMode){
        goto GpuComP;
    }
#endif

    ret = collect_all_zones(context,list);
    hwc_sprite_replace(context,list);
    if(ret !=0 ){
		ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        goto GpuComP;
    }
   
    //if(vertical == true)
    ret = hwc_try_policy(context,list,dpyID);
    if(list->hwLayers[context->mRgaTBI.index].compositionType == HWC_FRAMEBUFFER){
        context->mNeedRgaTransform = false;
    }else if(context->mNeedRgaTransform){
        int w_valid = context->mRgaTBI.w_valid;
        int h_valid = context->mRgaTBI.h_valid;
        int layer_fd = context->mRgaTBI.layer_fd;
        int lastfd = context->mRgaTBI.transform;
        uint32_t transform = context->mRgaTBI.transform;
        bool trsfrmbyrga = context->mRgaTBI.trsfrmbyrga;
        struct private_handle_t* hdl = context->mRgaTBI.hdl;
        int Dstfmt = trsfrmbyrga ? hwChangeRgaFormat(hdl->format) : RK_FORMAT_YCbCr_420_SP;
        if(rga_video_copybit(hdl,transform,w_valid,h_valid,layer_fd,Dstfmt,trsfrmbyrga,dpyID)){
            ALOGD_IF(mLogL&HLLONE,"T:RGA copyblit fail");
            context->mRgaTBI.lastfd = 0;
            goto GpuComP;
        }
#if USE_VIDEO_BACK_BUFFERS
        context->mCurVideoIndex++;  //update video buffer index
#else
        if(trsfrmbyrga)
            context->mCurVideoIndex++;  //update video buffer index
#endif
    }

    if(ret !=0 ){
		ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        goto GpuComP;
    }

    if(context->zone_manager.composter_mode != HWC_MIX) {
        for(i = 0;i<GPUDRAWCNT;i++){
            gmixinfo[mix_index].gpu_draw_fd[i] = 0;
        }
    }

    if(check_zone(context) && dpyID == 0){
        ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        goto GpuComP;
    }
    //before composition:do overlay no error???
    struct rk_fb_win_cfg_data fbinfo;
    if(context->zone_manager.composter_mode == HWC_LCDC){
        err = hwc_collect_cfg(context,list,&fbinfo,0,true);
    }else if(context->zone_manager.composter_mode == HWC_MIX){
        err = hwc_collect_cfg(context,list,&fbinfo,1,true);
    }else if(context->zone_manager.composter_mode == HWC_MIX_V2){
        err = hwc_collect_cfg(context,list,&fbinfo,2,true);
    }
    if(err){
        ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        goto GpuComP;
    }else{
        if(!hwc_check_cfg(context,fbinfo)){
            ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
            dump_config_info(fbinfo,context,3);
            goto GpuComP;
        }
    }
#if (defined(RK3368_BOX) || defined(RK3288_BOX))
#ifdef RK3288_BOX
	if(_contextAnchor->mLcdcNum == 1)
#endif
    {
        if(!hwcPrimaryToExternalCheckConfig(context,fbinfo)){
            ALOGD_IF(mLogL&HLLONE,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
            goto GpuComP;
        }
    }
#endif
    context->mLastCompType = context->zone_manager.mCmpType;
    return 0;
GpuComP   :
    for (i = 0; i < (list->numHwLayers - 1); i++)
    {
        hwc_layer_1_t * layer = &list->hwLayers[i];

        layer->compositionType = HWC_FRAMEBUFFER;
    }
    for(i = 0;i<GPUDRAWCNT;i++)
    {
        gmixinfo[mix_index].gpu_draw_fd[i] = 0;
    }

#if USE_SPECIAL_COMPOSER
    hwc_LcdcToGpu(list);         //Dont remove
    bkupmanage.dstwinNo = 0xff;  // GPU handle
    bkupmanage.invalid = 1;
#endif
    for (j = 0; j <(list->numHwLayers - 1); j++)
    {
        struct private_handle_t * handle = (struct private_handle_t *)list->hwLayers[j].handle;

        list->hwLayers[j].compositionType = HWC_FRAMEBUFFER;                
     
        if (handle && GPU_FORMAT==HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO)
        {
            ALOGV("rga_video_copybit,handle=%x,base=%x,w=%d,h=%d,video(w=%d,h=%d)",\
                  handle,GPU_BASE,GPU_WIDTH,GPU_HEIGHT,handle->video_width,handle->video_height);
            rga_video_copybit(handle,0,0,0,handle->share_fd,RK_FORMAT_YCbCr_420_SP,0,dpyID);
        }
    }
    context->zone_manager.composter_mode = HWC_FRAMEBUFFER;
    context->mLastCompType = -1;
    return 0;

}

int
hwc_prepare(
    hwc_composer_device_1_t * dev,
    size_t numDisplays,
    hwc_display_contents_1_t** displays
    )
{
    ATRACE_CALL();
    hwcContext * context = _contextAnchor;
    int ret = 0;
    size_t i;
    hwc_display_contents_1_t* list = displays[0];  // ignore displays beyond the first  

    /* Check device handle. */
    if (context == NULL|| &context->device.common != (hw_device_t *)dev){
        LOGE("%s(%d):Invalid device!", __FUNCTION__, __LINE__);
        return HWC_EGL_ERROR;
    }

    hwc_pre_prepare(displays,0);
    dump_prepare_info(displays,0);

#if hwcDumpSurface
    _DumpSurface(list);
#endif

    void* zone_m = (void *)&context->zone_manager;
    memset(zone_m,0,sizeof(ZoneManager));
    context->zone_manager.composter_mode = HWC_FRAMEBUFFER;
    
    if(_contextAnchor1 != NULL){
        zone_m = (void *)&_contextAnchor1->zone_manager;
        memset(zone_m,0,sizeof(ZoneManager));
        _contextAnchor1->zone_manager.composter_mode = HWC_FRAMEBUFFER;
    }
    
    /* Roll back to FRAMEBUFFER if any layer can not be handled. */
    if(hwc_get_int_property("sys.hwc.compose_policy","0") <= 0 ){
        for (i = 0; i < (list->numHwLayers - 1); i++){
            list->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
        }
        hwc_display_contents_1_t* list_e = displays[1];
        if(list_e){
            for (i = 0; i < (list_e->numHwLayers - 1); i++){
                list_e->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
            }
        }    
#if (defined(GPU_G6110) || defined(RK3288_BOX))
#ifdef RK3288_BOX
        if(context->mLcdcNum == 2){
            return 0;
        }
#endif
        if(!hdmi_noready && getHdmiMode() == 1){
            for (unsigned int i = 0; i < (list->numHwLayers - 1); i++){
                hwc_layer_1_t * layer = &list->hwLayers[i];
                layer->compositionType = HWC_NODRAW;
            }
        }
#endif
        ALOGD_IF(mLogL&HLLFOU,"Policy out [%d][%s]",__LINE__,__FUNCTION__);
        return 0;
    }

#if hwcDEBUG
    if(mLogL&HLLEIG){
        LOGD("%s(%d):Layers to set:", __FUNCTION__, __LINE__);
        _Dump(list);
    }
#endif

    for (size_t i = 0; i < numDisplays; i++) {
        hwc_display_contents_1_t *list = displays[i];
        switch(i) {
            case HWC_DISPLAY_PRIMARY:
            case HWC_DISPLAY_EXTERNAL:
                ret = hwc_prepare_screen(dev, list, i);
                break;
            case HWC_DISPLAY_VIRTUAL:
                if(list){
                    ret = hwc_prepare_virtual(dev, list);
                }
                break;
            default:
                ret = -EINVAL;
        }
    }
    dump_prepare_info(displays,0);
    return ret;
}

int hwc_blank(struct hwc_composer_device_1 *dev, int dpy, int blank)
{
    // We're using an older method of screen blanking based on
    // early_suspend in the kernel.  No need to do anything here.

    ALOGI("hwc_blank dpy[%d],blank[%d]",dpy,blank);
    // return 0;
    switch (dpy) {
    case HWC_DISPLAY_PRIMARY: {
        int fb_blank = blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK;
        int err = ioctl(_contextAnchor->fbFd, FBIOBLANK, fb_blank);
        if (err < 0) {
            if (errno == EBUSY)
                ALOGD("%sblank ioctl failed (display already %sblanked)",
                        blank ? "" : "un", blank ? "" : "un");
            else
                ALOGE("%sblank ioctl failed: %s", blank ? "" : "un",
                        strerror(errno));
            return -errno;
        }
        else
        {
            _contextAnchor->fb_blanked = blank;
        }
        break;
    }

    case HWC_DISPLAY_EXTERNAL:{
#if HWC_EXTERNAL
		if(blank == 0)
		    hdmi_noready = false;
		_contextAnchor1->fb_blanked = blank;
        int fb_blank = blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK;
#endif
        break;
    }
	
    default:
        return -EINVAL;

    }

    return 0;
}

int hwc_query(struct hwc_composer_device_1* dev,int what, int* value)
{

    hwcContext * context = _contextAnchor;

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // we support the background layer
        value[0] = 1;
        break;
    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = 1e9 / context->fb_fps;
        break;
    default:
        // unsupported query
        return -EINVAL;
    }
    return 0;
}

static int display_commit( int dpy)
{
    return 0;
}

static int hwc_fbPost(hwc_composer_device_1_t * dev, size_t numDisplays, hwc_display_contents_1_t** list)
{
    return 0;
}

static int hwc_Post( hwcContext * context,hwc_display_contents_1_t* list)
{
    ATRACE_CALL();
    if (list == NULL){
        return -1;
    }

    int dpyID = 0;
    if(context!=_contextAnchor){
        dpyID = 1;
    }
    if(!is_need_post(list,dpyID,1)){
        return -1;
    }
    //if (context->fbFd>0 && !context->fb_blanked)
#if defined(RK3288_MID)
    if(dpyID == 0 || (dpyID == 1 && !context->fb_blanked)){
#else
#ifdef RK3288_BOX
    int lcdcNum = _contextAnchor->mLcdcNum;
    bool isPost = dpyID == 0 || (dpyID == 1 && !context->fb_blanked);
    if(lcdcNum==1 || (lcdcNum==2 && isPost)){
#else
    if(true){
#endif
#endif
        struct fb_var_screeninfo info;
        struct rk_fb_win_cfg_data fb_info;
        memset(&fb_info,0,sizeof(fb_info));
        fb_info.ret_fence_fd = -1;
        for(int i=0;i<RK_MAX_BUF_NUM;i++) {
            fb_info.rel_fence_fd[i] = -1;
        }
        int numLayers = list->numHwLayers;
        hwc_layer_1_t *fbLayer = &list->hwLayers[numLayers - 1];
        if (!fbLayer)
        {
            ALOGE("fbLayer=NULL");
            return -1;
        }
        info = context->info;
        struct private_handle_t*  handle = (struct private_handle_t*)fbLayer->handle;
        if (!handle)
        {
			ALOGE("hanndle=NULL at line %d",__LINE__);
            return -1;
        }

        ALOGV("hwc_primary_Post num=%d,ion=%d",numLayers,handle->share_fd);
        #if 0
        unsigned int videodata[2];

        videodata[0] = videodata[1] = context->fbPhysical;
	    if(ioctl(context->fbFd, RK_FBIOSET_DMABUF_FD, &(handle->share_fd))== -1)
	    {
	        ALOGE("RK_FBIOSET_DMABUF_FD err");
	        return -1;
	    }
        if (ioctl(context->fbFd, FB1_IOCTL_SET_YUV_ADDR, videodata) == -1)
        {
            ALOGE("FB1_IOCTL_SET_YUV_ADDR err");
            return -1;
        }

        unsigned int offset = handle->offset;
        info.yoffset = offset/context->fbStride;
        if (ioctl(context->fbFd, FBIOPUT_VSCREENINFO, &info) == -1)
        {
            ALOGE("FBIOPUT_VSCREENINFO err!");
        }
        else
        {
            int sync = 0;
            ioctl(context->fbFd, RK_FBIOSET_CONFIG_DONE, &sync);
        }
        #else

        unsigned int offset = handle->offset;        
        fb_info.win_par[0].area_par[0].data_format = context->fbhandle.format;
        fb_info.win_par[0].win_id = 0;
        fb_info.win_par[0].z_order = 0;
        fb_info.win_par[0].area_par[0].ion_fd = handle->share_fd;
#if USE_HWC_FENCE
        fb_info.win_par[0].area_par[0].acq_fence_fd = -1;//fbLayer->acquireFenceFd;
#else
        fb_info.win_par[0].area_par[0].acq_fence_fd = -1;
#endif
        fb_info.win_par[0].area_par[0].x_offset = 0;
        fb_info.win_par[0].area_par[0].y_offset = offset/context->fbStride;
        fb_info.win_par[0].area_par[0].xpos = 0;
        fb_info.win_par[0].area_par[0].ypos = 0;
        fb_info.win_par[0].area_par[0].xsize = handle->width;
        fb_info.win_par[0].area_par[0].ysize = handle->height;
        fb_info.win_par[0].area_par[0].xact = handle->width;
        fb_info.win_par[0].area_par[0].yact = handle->height;
        fb_info.win_par[0].area_par[0].xvir = handle->stride;
        fb_info.win_par[0].area_par[0].yvir = handle->height;
#if USE_HWC_FENCE
#if SYNC_IN_VIDEO
    if(context->mVideoMode && !context->mIsMediaView && !g_hdmi_mode)
        fb_info.wait_fs=1;
    else
#endif
	    fb_info.wait_fs=0;
#endif
        if(context == _contextAnchor1){
            if(_contextAnchor->mHdmiSI.NeedReDst){
                if(hotplug_reset_dstposition(&fb_info,0)){
                    ALOGW("reset_dst fail [%d]",__LINE__);
                }
            }
         }else{
#if (defined(GPU_G6110) || defined(RK3288_BOX))
#ifdef RK3288_BOX
            if(_contextAnchor->mLcdcNum==1)
#endif
#ifdef RK3368_MID
            if(context->mHdmiSI.CvbsOn || context->mHdmiSI.HdmiOn)
#endif
            {
                if(hotplug_reset_dstposition(&fb_info,1)){
                    ALOGW("reset_dst fail [%d]",__LINE__);
                }
            }
#endif
        }
#ifdef RK3288_BOX
        if(context==_contextAnchor && context->mResolutionChanged && context->mLcdcNum==2){
            hotplug_reset_dstposition(&fb_info,2);
        }
#endif
        if(ioctl(context->fbFd, RK_FBIOSET_CONFIG_DONE, &fb_info)){
            ALOGE("ID=%d:ioctl fail:%s",context!=_contextAnchor,strerror(errno));
            dump_config_info(fb_info,context,3);
        }else{
#if ONLY_USE_ONE_VOP
#ifdef RK3288_BOX
            if(_contextAnchor->mLcdcNum == 1)
#endif
            {
                memcpy(&_contextAnchor->fb_info,&fb_info,sizeof(rk_fb_win_cfg_data));
            }
#endif
            ALOGD_IF(mLogL&HLLONE,"ID=%d:",context!=_contextAnchor);
        }
        dump_config_info(fb_info,context,2);
#if USE_HWC_FENCE
        for(int k=0;k<RK_MAX_BUF_NUM;k++)
        {
            if(fb_info.rel_fence_fd[k] >= 0)
                fbLayer->releaseFenceFd = fb_info.rel_fence_fd[k];
        }
		if(fb_info.ret_fence_fd >= 0)
        	list->retireFenceFd = fb_info.ret_fence_fd;
#else
        for(int k=0;k<RK_MAX_BUF_NUM;k++)
        {
            if(fb_info.rel_fence_fd[k] >=0 )
                close(fb_info.rel_fence_fd[k]);
        }
        fbLayer->releaseFenceFd=-1;

        if(fb_info.ret_fence_fd >= 0)
        {
            close(fb_info.ret_fence_fd);
        }
        list->retireFenceFd=-1;
#endif

        #endif
    }
    return 0;
}

static int hwc_set_lcdc(hwcContext * context, hwc_display_contents_1_t *list,int mix_flag) 
{
    ATRACE_CALL();
    int dpyID = 0;
    unsigned int j = 0;
    if(context==_contextAnchor1){
        dpyID = 1;
    }
    if(!is_need_post(list,dpyID,2)){
        return -1;
    }
    struct rk_fb_win_cfg_data fb_info;
    hwc_collect_cfg(context,list,&fb_info,mix_flag,false);
    //if(!context->fb_blanked)
    if(true){
#ifndef GPU_G6110  //This will lead nenamark fps go down in rk3368.
        if(context != _contextAnchor1){
            hwc_display_t dpy = NULL;
            hwc_surface_t surf = NULL;
            dpy = eglGetCurrentDisplay();
            surf = eglGetCurrentSurface(EGL_DRAW);
            _eglRenderBufferModifiedANDROID((EGLDisplay) dpy, (EGLSurface) surf);
            eglSwapBuffers((EGLDisplay) dpy, (EGLSurface) surf);
        }
#endif
        if(context == _contextAnchor1){
            if(_contextAnchor->mHdmiSI.NeedReDst){
                if(hotplug_reset_dstposition(&fb_info,0)){
                    ALOGW("reset_dst fail [%d]",__LINE__);
                }
            }
         }else{
#if (defined(GPU_G6110) || defined(RK3288_BOX))
#ifdef RK3288_BOX
            if(_contextAnchor->mLcdcNum==1)
#endif
#if RK3368_MID
            if(context->mHdmiSI.CvbsOn || context->mHdmiSI.HdmiOn)
#endif
            {
                if(hotplug_reset_dstposition(&fb_info,1)){
                    ALOGW("reset_dst fail [%d]",__LINE__);
                }
            }
#endif
        }

#ifdef RK3288_BOX
        if(context==_contextAnchor && context->mResolutionChanged && context->mLcdcNum==2){
            hotplug_reset_dstposition(&fb_info,2);
        }
#endif

        if(ioctl(context->fbFd, RK_FBIOSET_CONFIG_DONE, &fb_info)){
            ALOGE("ID=%d:ioctl fail:%s",dpyID,strerror(errno));
            dump_config_info(fb_info,context,3);
        }else{
#if ONLY_USE_ONE_VOP
#ifdef RK3288_BOX
            if(_contextAnchor->mLcdcNum == 1)
#endif
            {
                memcpy(&_contextAnchor->fb_info,&fb_info,sizeof(rk_fb_win_cfg_data));
            }
#endif
            ALOGD_IF(mLogL&HLLONE,"ID=%d:",dpyID);
        }
        if(mix_flag){
            dump_config_info(fb_info,context,1);
        }else{
            dump_config_info(fb_info,context,0);
        }
#if USE_HWC_FENCE
#if 0
        for(unsigned int i=0;i<RK_MAX_BUF_NUM;i++){
            ALOGD_IF(mLogL&HLLSIX,"rel_fence_fd[%d] = %d", i, fb_info.rel_fence_fd[i]);
            if(fb_info.rel_fence_fd[i] >= 0){
                if(i< list->numHwLayers){
                    if(fb_info.win_par[0].area_par[0].data_format == HAL_PIXEL_FORMAT_YCrCb_NV12_OLD
                        &&list->hwLayers[0].transform != 0 && 0 == i){
                        list->hwLayers[i].releaseFenceFd = -1;
                        close(fb_info.rel_fence_fd[i]);
                    }else{
                        if(mix_flag == 1 && !context->mHdmiSI.mix_up){
                            list->hwLayers[i+2].releaseFenceFd = fb_info.rel_fence_fd[i];
                        }else if(mix_flag){
                            if(i<1){
                                list->hwLayers[list->numHwLayers -1].releaseFenceFd = fb_info.rel_fence_fd[i];
                            }else{
                                list->hwLayers[i-1].releaseFenceFd = fb_info.rel_fence_fd[i];
                            }
                        }else{
                            list->hwLayers[i].releaseFenceFd = fb_info.rel_fence_fd[i];
                        }
                    }
                }else{
                    close(fb_info.rel_fence_fd[i]);
                }
             }
    	}
#else
        for(unsigned int i=0;i<list->numHwLayers-1;i++) {
            hwc_layer_1_t * layer = &list->hwLayers[i];
            if(layer->compositionType != HWC_NODRAW && layer->compositionType != HWC_FRAMEBUFFER) {
                if(fb_info.win_par[0].area_par[0].data_format == HAL_PIXEL_FORMAT_YCrCb_NV12_OLD
                    &&list->hwLayers[0].transform != 0 && 0 == i){
                    list->hwLayers[0].releaseFenceFd = -1;
                    close(fb_info.rel_fence_fd[j]);
                    fb_info.rel_fence_fd[j++] = -1;
                }else{
                    layer->releaseFenceFd = fb_info.rel_fence_fd[j++];
                }
            }
        }

        if(mix_flag) {
            list->hwLayers[list->numHwLayers-1].releaseFenceFd = fb_info.rel_fence_fd[j++];
        }

        for(;j<RK_MAX_BUF_NUM;j++) {
            if(fb_info.rel_fence_fd[j] >= 0) {
                close(fb_info.rel_fence_fd[j]);
                fb_info.rel_fence_fd[j] = -1;
                ALOGW_IF(mLogL&HLLSIX,"fb_info.rel_fence_fd[%d]=%d",j,fb_info.rel_fence_fd[j]);
            }
        }
#endif
        for(unsigned int i=0;i< (list->numHwLayers);i++){
            ALOGD_IF(mLogL&HLLSIX,"Layer[%d].relFenceFd=%d",i,list->hwLayers[i].releaseFenceFd);
        }

        if(list->retireFenceFd > 0){
            close(list->retireFenceFd);
        }
		if(fb_info.ret_fence_fd >= 0){
        	list->retireFenceFd = fb_info.ret_fence_fd;
        }
#else
    	for(i=0;i<RK_MAX_BUF_NUM;i++){
            if(fb_info.rel_fence_fd[i] >= 0 ){
                if(i< (int)(list->numHwLayers -1)){
                    list->hwLayers[i].releaseFenceFd = -1;
                    close(fb_info.rel_fence_fd[i]);
                }else{
                    close(fb_info.rel_fence_fd[i]);
                }
             }            
    	}
        list->retireFenceFd = -1;
        if(fb_info.ret_fence_fd >= 0){
            close(fb_info.ret_fence_fd);
        }
        //list->retireFenceFd = fb_info.ret_fence_fd;        
#endif
    }else{
        for(unsigned int i=0;i< (list->numHwLayers -1);i++){
            list->hwLayers[i].releaseFenceFd = -1;    	   
    	}
        list->retireFenceFd = -1;
    }
    return 0;
}
int hwc_rga_blit( hwcContext * context ,hwc_display_contents_1_t *list)
{
#if VIRTUAL_RGA_BLIT
    hwcSTATUS status = hwcSTATUS_OK;
    unsigned int i;
    unsigned int index = 0;

#if hwcUseTime
    struct timeval tpend1, tpend2;
    long usec1 = 0;
#endif
#if hwcBlitUseTime
    struct timeval tpendblit1, tpendblit2;
    long usec2 = 0;
#endif

    hwc_layer_1_t *fbLayer = NULL;
    struct private_handle_t * fbhandle = NULL;
    bool bNeedFlush = false;
    FenceMangrRga RgaFenceMg;

#if hwcUseTime
    gettimeofday(&tpend1, NULL);
#endif
    memset(&RgaFenceMg,0,sizeof(FenceMangrRga));

    LOGV("%s(%d):>>> Set  %d layers <<<",
         __FUNCTION__,
         __LINE__,
         list->numHwLayers);
    /* Prepare. */
    for (i = 0; i < (list->numHwLayers - 1); i++)
    {
        /* Check whether this composition can be handled by hwcomposer. */
        if (list->hwLayers[i].compositionType >= HWC_BLITTER)
        {
#if FENCE_TIME_USE
            struct timeval tstart, tend;
            gettimeofday(&tstart, NULL);
#endif

            #if 0
            if(context->membk_fence_acqfd[context->membk_index] > 0)
            {
                sync_wait(context->membk_fence_acqfd[context->membk_index], 500);
                close(context->membk_fence_acqfd[context->membk_index]);
                context->membk_fence_acqfd[context->membk_index] = -1;
                //ALOGD("close0 rga acq_fd=%d",fb_info.win_par[0].area_par[0].acq_fence_fd);
            }
            #endif
#if FENCE_TIME_USE
            gettimeofday(&tend, NULL);
            if(((tend.tv_sec - tstart.tv_sec)*1000 + (tend.tv_usec - tstart.tv_usec)/1000) > 16)
            {
                ALOGW("wait for LCDC fence too long ,spent t = %ld ms",((tend.tv_sec - tstart.tv_sec)*1000 + (tend.tv_usec - tstart.tv_usec)/1000));
            }
#endif

#if ENABLE_HWC_WORMHOLE
            hwcRECT FbRect;
            hwcArea * area;
            hwc_region_t holeregion;
#endif
            bNeedFlush = true;

            fbLayer = &list->hwLayers[list->numHwLayers - 1];
            ALOGV("fbLyaer = %x,num=%d",fbLayer,list->numHwLayers);
            if (fbLayer == NULL)
            {
                ALOGE("fbLayer is null");
                hwcONERROR(hwcSTATUS_INVALID_ARGUMENT);
            }
            fbhandle = (struct private_handle_t*)fbLayer->handle;
            if (fbhandle == NULL)
            {
                ALOGE("fbhandle is null");
                hwcONERROR(hwcSTATUS_INVALID_ARGUMENT);
            }
            ALOGV("i=%d,tpye=%d,hanlde=%p",i,list->hwLayers[i].compositionType,fbhandle);
#if ENABLE_HWC_WORMHOLE
            /* Reset allocated areas. */
            if (context->compositionArea != NULL)
            {
                ZoneFree(context, context->compositionArea);

                context->compositionArea = NULL;
            }

            FbRect.left = 0;
            FbRect.top = 0;
            FbRect.right = fbhandle->width;
            FbRect.bottom = fbhandle->height;

            /* Generate new areas. */
            /* Put a no-owner area with screen size, this is for worm hole,
             * and is needed for clipping. */
            context->compositionArea = zone_alloc(context,
                                       NULL,
                                       &FbRect,
                                       0U);

            /* Split areas: go through all regions. */
            for (unsigned int k = 0; k < list->numHwLayers - 1; k++)
            {
                int owner = 1U << k;
                hwc_layer_1_t *  hwLayer = &list->hwLayers[k];
                hwc_region_t * region  = &hwLayer->visibleRegionScreen;
                //struct private_handle_t* srchnd = (struct private_handle_t *) hwLayer->handle;

                if((hwLayer->blending & 0xFFFF) != HWC_BLENDING_NONE)
                {
                    ALOGV("ignore alpha layer");
                    continue;
                }
                /* Now go through all rectangles to split areas. */
                for (int j = 0; j < region->numRects; j++)
                {
                    /* Assume the region will never go out of dest surface. */
                    DivArea(context,
                               context->compositionArea,
                               (hwcRECT *) &region->rects[j],
                               owner);

                }

            }
#if DUMP_SPLIT_AREA
            LOGV("SPLITED AREA:");
            hwcDumpArea(context->compositionArea);
#endif

            area = context->compositionArea;

            while (area != NULL)
            {
                /* Check worm hole first. */
                if (area->owners == 0U)
                {

                    holeregion.numRects = 1;
                    holeregion.rects = (hwc_rect_t const*) & area->rect;
                    /* Setup worm hole source. */
                    LOGV(" WormHole [%d,%d,%d,%d]",
                         area->rect.left,
                         area->rect.top,
                         area->rect.right,
                         area->rect.bottom
                        );

                    hwcClear(context,
                             0xFF000000,
                             &list->hwLayers[i],
                             fbhandle,
                             (hwc_rect_t *)&area->rect,
                             &holeregion
                            );

                    /* Advance to next area. */
                }
                area = area->next;
            }
#endif
            /* Done. */
            break;
        }
        else if (list->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
        {
            /* Previous swap rectangle is gone. */
            break;

        }
    }
    /* Go through the layer list one-by-one blitting each onto the FB */

#if RGA_USE_FENCE
    for(i = 0;i< RGA_REL_FENCE_NUM;i++)
    {
        context->rga_fence_relfd[i] = -1;
    }
    if(context->composer_mode == HWC_RGA)
        RgaFenceMg.use_fence = true;
#endif

    for (i = 0; i < list->numHwLayers -1; i++)
    {
        switch (list->hwLayers[i].compositionType)
        {
            case HWC_BLITTER:
                ALOGV("%s(%d):Layer %d ,name=%s,is BLIITER", __FUNCTION__, __LINE__, i,list->hwLayers[i].LayerName);
                /* Do the blit. */

#if hwcBlitUseTime
                gettimeofday(&tpendblit1, NULL);
#endif
                hwcONERROR(
                    hwcBlit(context,
                            &list->hwLayers[i],
                            fbhandle,
                            &list->hwLayers[i].sourceCrop,
                            &list->hwLayers[i].displayFrame,
                            &list->hwLayers[i].visibleRegionScreen,
                            &RgaFenceMg,index));

#if hwcBlitUseTime
                gettimeofday(&tpendblit2, NULL);
                usec2 = 1000 * (tpendblit2.tv_sec - tpendblit1.tv_sec) + (tpendblit2.tv_usec - tpendblit1.tv_usec) / 1000;
                LOGD("hwcBlit compositer %d layers=%s use time=%ld ms", i, list->hwLayers[i].LayerName, usec2);
#endif
                index++;
                break;

            case HWC_CLEAR_HOLE:
                LOGV("%s(%d):Layer %d is CLEAR_HOLE", __FUNCTION__, __LINE__, i);
                /* Do the clear, color = (0, 0, 0, 1). */
                /* TODO: Only clear holes on screen.
                 * See Layer::onDraw() of surfaceflinger. */
                if (i != 0) break;

                hwcONERROR(
                    hwcClear(context,
                             0xFF000000,
                             &list->hwLayers[i],
                             fbhandle,
                             &list->hwLayers[i].displayFrame,
                             &list->hwLayers[i].visibleRegionScreen));
                break;

            case HWC_DIM:
                LOGV("%s(%d):Layer %d is DIM", __FUNCTION__, __LINE__, i);
                if (i == 0)
                {
                    /* Use clear instead of dim for the first layer. */
                    hwcONERROR(
                        hwcClear(context,
                                 ((list->hwLayers[0].blending & 0xFF0000) << 8),
                                 &list->hwLayers[i],
                                 fbhandle,
                                 &list->hwLayers[i].displayFrame,
                                 &list->hwLayers[i].visibleRegionScreen));
                }
                else
                {
                    /* Do the dim. */
                    hwcONERROR(
                        hwcDim(context,
                               &list->hwLayers[i],
                               fbhandle,
                               &list->hwLayers[i].displayFrame,
                               &list->hwLayers[i].visibleRegionScreen));
                }
                break;

            case HWC_OVERLAY:
                /* TODO: HANDLE OVERLAY LAYERS HERE. */
                LOGV("%s(%d):Layer %d is OVERLAY", __FUNCTION__, __LINE__, i);
                break;
            }

    }

#if hwcUseTime
    gettimeofday(&tpend2, NULL);
    usec1 = 1000 * (tpend2.tv_sec - tpend1.tv_sec) + (tpend2.tv_usec - tpend1.tv_usec) / 1000;
    LOGV("hwcBlit compositer %d layers use time=%ld ms", list->numHwLayers -1, usec1);
#endif

    return 0; //? 0 : HWC_EGL_ERROR;
OnError:

    LOGE("%s(%d):Failed!", __FUNCTION__, __LINE__);
    return HWC_EGL_ERROR;
#else
    return 0;
#endif
}

int hwc_check_fencefd(size_t numDisplays,hwc_display_contents_1_t  ** displays)
{
    for (size_t i = 0; i < numDisplays; i++) {
        hwc_display_contents_1_t *list = displays[i];
        if(list){
            int numLayers = list->numHwLayers;
            for(int j = 0;j<numLayers;j++){
                hwc_layer_1_t *layer = &list->hwLayers[j];
                if(layer && layer->acquireFenceFd>0){
                    ALOGW("Foce to close aqFenceFd,%d,%d",i,j);
                    close(layer->acquireFenceFd);
                    layer->acquireFenceFd = -1;
                }
                if(layer){
                    ALOGD_IF(mLogL&HLLSIX,"%d,%d,%d",i,j,layer->releaseFenceFd);
                }
            }
        }
    }
    return 0;
}

static int hwc_set_screen(hwc_composer_device_1 *dev, hwc_display_contents_1_t *list,int dpyID) 
{
    ATRACE_CALL();
    if(!is_need_post(list,dpyID,0)){
        return -1;
    }
    hwcContext * context = _contextAnchor;
    if(dpyID == HWCE){
        context = _contextAnchor1;
    }

#if hwcUseTime
    struct timeval tpend1, tpend2;
    long usec1 = 0;
#endif

    hwc_display_t dpy = NULL;
    hwc_surface_t surf = NULL;

    hwc_sync(list);
    if (list != NULL) {
        dpy = list->dpy;
        surf = list->sur;        
    }

    /* Check device handle. */
    if (dpyID == 0 && (context == NULL || 
        &_contextAnchor->device.common != (hw_device_t *) dev)){
        LOGE("%s(%d): Invalid device!", __FUNCTION__, __LINE__);
        return HWC_EGL_ERROR;
    }

    /* Check layer list. */
    if ((list == NULL  || list->numHwLayers == 0) && dpyID == 0){
        LOGE("(%d):list=NULL,Layers =%d",__LINE__,list->numHwLayers);
        /* Reset swap rectangles. */
        return -1;
    }else if(list == NULL){
        return -1;
    }

    LOGV("%s(%d):>>> Set start %d layers <<<,mode=%d",
         __FUNCTION__,
         __LINE__,
         list->numHwLayers,context->zone_manager.composter_mode);

#if hwcDEBUG
    if(mLogL&HLLEIG){
        LOGD("%s(%d):Layers to set:", __FUNCTION__, __LINE__);
        _Dump(list);
    }
#endif
#if hwcUseTime
    gettimeofday(&tpend1,NULL);
#endif
    int ret = -1;
    if(context->zone_manager.composter_mode == HWC_LCDC) {
        ret = hwc_set_lcdc(context,list,0);
    }else if(context->zone_manager.composter_mode == HWC_FRAMEBUFFER){
        ret = hwc_Post(context,list);
    }else if(context->zone_manager.composter_mode == HWC_MIX){
        ret = hwc_set_lcdc(context,list,1);
    }else if(context->zone_manager.composter_mode == HWC_MIX_V2){
        ret = hwc_set_lcdc(context,list,2);
    }

#if !(defined(GPU_G6110) || defined(RK3288_BOX))
    if(dpyID == HWCP)
#endif
    {
        static int frame_cnt = 0;
        char value[PROPERTY_VALUE_MAX];
        property_get("sys.glib.state", value, "0");
        int skipcnt = atoi(value);
        if(skipcnt > 0) {
            if(((++frame_cnt)%skipcnt) == 0) {
                _eglRenderBufferModifiedANDROID((EGLDisplay) NULL, (EGLSurface) NULL);
                eglSwapBuffers((EGLDisplay) NULL, (EGLSurface) NULL); 
            }
        } else {
            frame_cnt = 0;
        }
    }
    
#if hwcUseTime
    gettimeofday(&tpend2,NULL);
    usec1 = 1000*(tpend2.tv_sec - tpend1.tv_sec) + (tpend2.tv_usec- tpend1.tv_usec)/1000;
    LOGD("hwcBlit compositer %d layers use time=%ld ms",list->numHwLayers,usec1);
#endif
    //close(Context->fbFd1);
#ifdef ENABLE_HDMI_APP_LANDSCAP_TO_PORTRAIT            
    if (list != NULL && getHdmiMode()>0){
        if (bootanimFinish==0){
        bootanimFinish = hwc_get_int_property("service.bootanim.exit","0")
            if (bootanimFinish > 0){
                usleep(1000000);
            }
        }else{
            if (strstr(list->hwLayers[list->numHwLayers-1].LayerName,"FreezeSurface")<=0){
                if (list->hwLayers[0].transform==HAL_TRANSFORM_ROT_90 || 
                    list->hwLayers[0].transform==HAL_TRANSFORM_ROT_270){
                        int rotation = list->hwLayers[0].transform;
                        if (ioctl(_contextAnchor->fbFd, RK_FBIOSET_ROTATE, &rotation)!=0){
                            LOGE("%s(%d):RK_FBIOSET_ROTATE error!", __FUNCTION__, __LINE__);
                        }
                    }else{
                        int rotation = 0;
                        if (ioctl(_contextAnchor->fbFd, RK_FBIOSET_ROTATE, &rotation)!=0){
                            LOGE("%s(%d):RK_FBIOSET_ROTATE error!", __FUNCTION__, __LINE__);
                        }
                    }
            }
        }
    }
#endif

    //ALOGD("set end");
    return ret; //? 0 : HWC_EGL_ERROR;
}        

int hwc_set_virtual(hwc_composer_device_1_t * dev, hwc_display_contents_1_t  **contents, unsigned int rga_fb_addr)
{
    ATRACE_CALL();
	hwc_display_contents_1_t* list_pri = contents[0];
	hwc_display_contents_1_t* list_wfd = contents[2];
	hwc_layer_1_t *  fbLayer = &list_pri->hwLayers[list_pri->numHwLayers - 1];
	hwc_layer_1_t *  wfdLayer = &list_wfd->hwLayers[list_wfd->numHwLayers - 1];
	hwcContext * context = _contextAnchor;
	struct timeval tpend1, tpend2;
	long usec1 = 0;
	gettimeofday(&tpend1,NULL);
	if (list_wfd)
	{
		hwc_sync(list_wfd);
	}
	if (fbLayer==NULL || wfdLayer==NULL)
	{
		return -1;
	}

	if ((context->wfdOptimize>0) && wfdLayer->handle)
	{
		hwc_cfg_t cfg;
		memset(&cfg, 0, sizeof(hwc_cfg_t));
		cfg.src_handle = (struct private_handle_t *)fbLayer->handle;
		cfg.transform = fbLayer->realtransform;
		ALOGD("++++transform=%d",cfg.transform);
		cfg.dst_handle = (struct private_handle_t *)wfdLayer->handle;
		cfg.src_rect.left = (int)fbLayer->displayFrame.left;
		cfg.src_rect.top = (int)fbLayer->displayFrame.top;
		cfg.src_rect.right = (int)fbLayer->displayFrame.right;
		cfg.src_rect.bottom = (int)fbLayer->displayFrame.bottom;
		//cfg.src_format = cfg.src_handle->format;

		cfg.rga_fbAddr = rga_fb_addr;
		cfg.dst_rect.left = (int)wfdLayer->displayFrame.left;
		cfg.dst_rect.top = (int)wfdLayer->displayFrame.top;
		cfg.dst_rect.right = (int)wfdLayer->displayFrame.right;
		cfg.dst_rect.bottom = (int)wfdLayer->displayFrame.bottom;
		//cfg.dst_format = cfg.dst_handle->format;
		set_rga_cfg(&cfg);
		do_rga_transform_and_scale();
	}
#if VIRTUAL_RGA_BLIT
	else if(context->wfdRgaBlit)
	{
	    hwcContext * ctx = _contextAnchor2;
	    hwc_rga_blit(ctx, list_wfd);
	}
#endif

	gettimeofday(&tpend2,NULL);
	usec1 = 1000*(tpend2.tv_sec - tpend1.tv_sec) + (tpend2.tv_usec- tpend1.tv_usec)/1000;
	ALOGV("hwc use time=%ld ms",usec1);
	return 0;
}

int
hwc_set(
    hwc_composer_device_1_t * dev,
    size_t numDisplays,
    hwc_display_contents_1_t  ** displays
    )
{
    ATRACE_CALL();
    int ret[4] = {0,0,0,0};
#if (defined(GPU_G6110) || defined(RK3288_BOX))
#ifdef RK3288_BOX
    if(_contextAnchor->mLcdcNum==1){
        if(getHdmiMode() == 1 || _contextAnchor->mHdmiSI.CvbsOn){
            hotplug_set_overscan(0);
        }
    }else{
        hotplug_set_overscan(0);
    }
#else
    if(getHdmiMode() == 1 || _contextAnchor->mHdmiSI.CvbsOn){
        hotplug_set_overscan(0);
    }
#endif
#endif
    for (uint32_t i = 0; i < numDisplays; i++) {
        hwc_display_contents_1_t* list = displays[i];
        switch(i) {
            case HWC_DISPLAY_PRIMARY:
            case HWC_DISPLAY_EXTERNAL:
                ret[i] = hwc_set_screen(dev, list, i);
                break;
            case HWC_DISPLAY_VIRTUAL:           
                if (list){
                    unsigned int fb_addr = 0;
                    // fb_addr = context->hwc_ion.pion->phys + context->hwc_ion.last_offset;
                    ret[2] = hwc_set_virtual(dev, displays,fb_addr);
                }
                break;
            default:
                ret[3] = -EINVAL;
        }
    }
    for (uint32_t i = 0; i < numDisplays; i++) {
        hwc_display_contents_1_t* list = displays[i];
        if(list){
            hwc_sync_release(list);
        }
    }
    hwc_check_fencefd(numDisplays,displays);
#if ONLY_USE_ONE_VOP
    if(ret[0] && ret[1]){
        ALOGW_IF(mLogL&HLLONE,"%d,ret[%d,%d]",numDisplays,ret[0],ret[1]);
    }
#endif
    return 0;
}

static void hwc_registerProcs(struct hwc_composer_device_1* dev,
                                    hwc_procs_t const* procs)
{
    hwcContext * context = _contextAnchor;

    context->procs =  (hwc_procs_t *)procs;
}


static int hwc_event_control(struct hwc_composer_device_1* dev,
        int dpy, int event, int enabled)
{

    hwcContext * context = _contextAnchor;
    bool log = mLogL & HLLFIV;
    ALOGD_IF(log,"D_EN[%d,%d]",dpy,enabled);
    if(dpy==1 && _contextAnchor1){
        context = _contextAnchor1;
        if(context->fbFd <= 0){
            ALOGW("D_EN[%d,%d] ERROR",dpy,enabled);
            return 0;
        }
    }
    switch (event) {
    case HWC_EVENT_VSYNC:
    {
        int val = !!enabled;
        int err;

        err = ioctl(context->fbFd, RK_FBIOSET_VSYNC_ENABLE, &val);
        if (err < 0)
        {
            LOGE(" RK_FBIOSET_VSYNC_ENABLE err=%d",err);
            return -1;
        }
        return 0;
    }
    default:
        return -1;
    }
}

static void handle_vsync_event(hwcContext * context )
{

    if (!context->procs)
        return;

    int err = lseek(context->vsync_fd, 0, SEEK_SET);
    if (err < 0) {
        ALOGE("error seeking to vsync timestamp: %s", strerror(errno));
        return;
    }

    char buf[4096];
    err = read(context->vsync_fd, buf, sizeof(buf));
    if (err < 0) {
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return;
    }
    buf[sizeof(buf) - 1] = '\0';

    //errno = 0;
    uint64_t timestamp = strtoull(buf, NULL, 0) ;/*+ (uint64_t)(1e9 / context->fb_fps)  ;*/
    if(context->timestamp != timestamp){
        context->timestamp = timestamp;
        context->procs->vsync(context->procs, 0, timestamp);
    }
/*
    uint64_t mNextFakeVSync = timestamp + (uint64_t)(1e9 / context->fb_fps);
    struct timespec spec;
    spec.tv_sec  = mNextFakeVSync / 1000000000;
    spec.tv_nsec = mNextFakeVSync % 1000000000;

    do {
        err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec, NULL);
    } while (err<0 && errno == EINTR);


    if (err==0)
    {
        context->procs->vsync(context->procs, 0, mNextFakeVSync );
        //ALOGD(" timestamp=%lld ms,preid=%lld us",mNextFakeVSync/1000000,(uint64_t)(1e6 / context->fb_fps) );
    }
    else
    {
        ALOGE(" clock_nanosleep ERR!!!");
    }
*/
}

void hwc_change_config(){
#ifdef RK3288_BOX
    hwcContext * context = _contextAnchor;
    if(context->mLcdcNum == 2){
        char buf[100];
        int width = 0;
        int height = 0;
        int fd = -1;
        fd = open("/sys/class/graphics/fb0/screen_info", O_RDONLY);
        if(fd < 0)
    	{
    	    ALOGE("hwc_change_config:open fb0 screen_info error,fd=%d",fd);
            return;
    	}
    	if(read(fd,buf,sizeof(buf)) < 0)
        {
            ALOGE("error reading fb0 screen_info: %s", strerror(errno));
            return;
        }
        close(fd);
    	sscanf(buf,"xres:%d yres:%d",&width,&height);
        ALOGD("hwc_change_config:width=%d,height=%d",width,height);
    	context->dpyAttr[HWC_DISPLAY_PRIMARY].relxres = width;
        context->dpyAttr[HWC_DISPLAY_PRIMARY].relyres = height;
    }
#endif
    return;
}

void handle_hotplug_event(int hdmi_mode ,int flag )
{
    hwcContext * context = _contextAnchor;
    if (!context->procs){
        return;
    }
    bool isNeedRemove = true;
#if (defined(GPU_G6110) || defined(RK3288_BOX))
#ifdef RK3288_BOX
    if(context->mLcdcNum == 1){
#endif
        if(!context->mIsBootanimExit){
            if(hdmi_mode){
                if(6 == flag){
                    context->mHdmiSI.HdmiOn = true;
                    context->mHdmiSI.CvbsOn = false;
                }else if(1 == flag){
                    context->mHdmiSI.CvbsOn = true;
                    context->mHdmiSI.HdmiOn = false;
                }
                hotplug_free_dimbuffer();
                hotplug_get_config(1);
                hotplug_set_config();
            }
            return;
        }
        if(context->mIsFirstCallbackToHotplug){
            isNeedRemove = false;
            context->mIsFirstCallbackToHotplug = false;
        }
#ifdef RK3288_BOX
    }
#endif
#endif
    if(isNeedRemove && (context->mHdmiSI.CvbsOn || context->mHdmiSI.HdmiOn)){
        int count = 0;
        if(context->mHdmiSI.NeedReDst){
            context->mHdmiSI.NeedReDst = false;
        }
        while(_contextAnchor1 && _contextAnchor1->fb_blanked){
            count++;
            usleep(10000);
            if(300==count){
                ALOGW("wait for unblank");
                break;
            }
        }
        hdmi_noready = true;
        hotplug_free_dimbuffer();
        if(context->mHdmiSI.CvbsOn){
            context->mHdmiSI.CvbsOn = false;
        }else{
            context->mHdmiSI.HdmiOn = false;
        }
        if(_contextAnchor1){
            _contextAnchor1->fb_blanked = 1;
        }
#if (defined(RK3288_MID) || defined(RK3288_BOX))
        if(context->mLcdcNum == 2){
            hotplug_set_frame(context,0);
        }
#endif
        context->dpyAttr[HWC_DISPLAY_EXTERNAL].connected = false;
        context->procs->hotplug(context->procs, HWC_DISPLAY_EXTERNAL, 0);
#if (defined(GPU_G6110) || defined(RK3288_BOX))
    if(context->mLcdcNum == 1){
        hotplug_set_overscan(1);
    }
#endif
        ALOGI("remove hotplug device [%d,%d,%d]",__LINE__,hdmi_mode,flag);
    }
    if(hdmi_mode){
        hotplug_free_dimbuffer();
        hotplug_get_config(1);
        hotplug_set_config();
        if(6 == flag){
            context->mHdmiSI.HdmiOn = true;
            context->mHdmiSI.CvbsOn = false;
        }else if(1 == flag){
            context->mHdmiSI.CvbsOn = true;
            context->mHdmiSI.HdmiOn = false;
        }
#if (defined(RK3288_MID) || defined(RK3288_BOX))
        if(context->mLcdcNum == 2){
            hotplug_set_frame(context,0);
        }
#endif
        char value[PROPERTY_VALUE_MAX];
        property_set("sys.hwc.htg","hotplug");
        context->procs->hotplug(context->procs, HWC_DISPLAY_EXTERNAL, 1);
        property_get("sys.hwc.htg",value,"hotplug");
        int count = 0;
        while(strcmp(value,"true")){
            count ++;
            if(count%3==0){
                context->procs->hotplug(context->procs, HWC_DISPLAY_EXTERNAL, 0);
            }
            context->procs->hotplug(context->procs, HWC_DISPLAY_EXTERNAL, 1);
            property_get("sys.hwc.htg",value,"hotplug");
            ALOGI("Trying to hotplug device[%d,%d,%d]",__LINE__,hdmi_mode,flag);
        }
        ALOGI("connet to hotplug device [%d,%d,%d]",__LINE__,hdmi_mode,flag);
#if (defined(GPU_G6110) || defined(RK3288_BOX))
    if(context->mLcdcNum == 1){
        hotplug_set_overscan(0);
    }
#endif
    }

    return;
}


static void *hwc_thread(void *data)
{
    prctl(PR_SET_NAME,"HWC_Vsync");
    hwcContext * context = _contextAnchor;

#if 0
    uint64_t timestamp = 0;
    nsecs_t now = 0;
    nsecs_t next_vsync = 0;
    nsecs_t sleep;
    const nsecs_t period = nsecs_t(1e9 / 50.0);
    struct timespec spec;
   // int err;
    do
    {

        now = systemTime(CLOCK_MONOTONIC);
        next_vsync = context->mNextFakeVSync;

        sleep = next_vsync - now;
        if (sleep < 0) {
            // we missed, find where the next vsync should be
            sleep = (period - ((now - next_vsync) % period));
            next_vsync = now + sleep;
        }
        context->mNextFakeVSync = next_vsync + period;

        spec.tv_sec  = next_vsync / 1000000000;
        spec.tv_nsec = next_vsync % 1000000000;

        do
        {
            err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec, NULL);
        } while (err<0 && errno == EINTR);

        if (err == 0)
        {
            if (context->procs && context->procs->vsync)
            {
                context->procs->vsync(context->procs, 0, next_vsync);

                ALOGD(" hwc_thread next_vsync=%lld ",next_vsync);
            }

        }

    } while (1);
#endif

  //    char uevent_desc[4096];
   // memset(uevent_desc, 0, sizeof(uevent_desc));



    char temp[4096];

    int err = read(context->vsync_fd, temp, sizeof(temp));
    if (err < 0) {
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return NULL;
    }

    struct pollfd fds[1];
    fds[0].fd = context->vsync_fd;
    fds[0].events = POLLPRI;
    //fds[1].fd = uevent_get_fd();
    //fds[1].events = POLLIN;

    while (true) {
        int err = poll(fds, 1, -1);
        if (err > 0) {
            if (fds[0].revents & POLLPRI) {
                handle_vsync_event(context);
            }

        }
        else if (err == -1) {
            if (errno == EINTR)
                break;
            ALOGE("error in vsync thread: %s", strerror(errno));
        }
    }

    return NULL;
}



int
hwc_device_close(
    struct hw_device_t *dev
    )
{
    int i;
    int err=0;
    hwcContext * context = _contextAnchor;

    LOGD("%s(%d):Close hwc device in thread=%d",
         __FUNCTION__, __LINE__, gettid());
    ALOGD("hwc_device_close ----------------------");
    /* Check device. */
    if (context == NULL
    || &context->device.common != (hw_device_t *) dev
    )
    {
        LOGE("%s(%d):Invalid device!", __FUNCTION__, __LINE__);

        return -EINVAL;
    }

    if (--context->reference > 0)
    {
        /* Dereferenced only. */
        return 0;
    }

    if(context->engine_fd)
        close(context->engine_fd);
    /* Clean context. */
    if(context->vsync_fd > 0)
        close(context->vsync_fd);
    if(context->fbFd > 0)
    {
        close(context->fbFd );

    }
    if(context->fbFd1 > 0)
    {
        close(context->fbFd1 );
    }

#if  USE_SPECIAL_COMPOSER
    for(i=0;i<bakupbufsize;i++)
    {
        if(bkupmanage.bkupinfo[i].phd_bk)
        {
            err = context->mAllocDev->free(context->mAllocDev, bkupmanage.bkupinfo[i].phd_bk);
            ALOGW_IF(err, "free bkupmanage.bkupinfo[%d].phd_bk (...) failed %d (%s)",i, err, strerror(-err));
        }      
    
    }
#endif

//#if  (ENABLE_TRANSFORM_BY_RGA | ENABLE_LCDC_IN_NV12_TRANSFORM | USE_SPECIAL_COMPOSER)
    if(bkupmanage.phd_drt)
    {
        err = context->mAllocDev->free(context->mAllocDev, bkupmanage.phd_drt);
        ALOGW_IF(err, "free bkupmanage.phd_drt (...) failed %d (%s)", err, strerror(-err));
    }
//#endif


    // free video gralloc buffer
    for(i=0;i<MaxVideoBackBuffers;i++)
    {
        if(context->pbvideo_bk[i] != NULL)
            err = context->mAllocDev->free(context->mAllocDev, context->pbvideo_bk[i]);
        if(!err)
        {
            context->fd_video_bk[i] = -1;
            context->base_video_bk[i] = 0;
            context->pbvideo_bk[i] = NULL;
        }
        ALOGW_IF(err, "free pbvideo_bk (...) failed %d (%s)", err, strerror(-err));
    }

#if OPTIMIZATION_FOR_DIMLAYER
    if(context->mDimHandle)
    {
        err = context->mAllocDev->free(context->mAllocDev, context->mDimHandle);
        ALOGW_IF(err, "free mDimHandle (...) failed %d (%s)", err, strerror(-err));
    }
#endif

    pthread_mutex_destroy(&context->lock);
    free(context);

    _contextAnchor = NULL;

    return 0;
}

static int hwc_getDisplayConfigs(struct hwc_composer_device_1* dev, int disp,
            			uint32_t* configs, size_t* numConfigs)
{
   int ret = 0;
   hwcContext * pdev = ( hwcContext  *)dev; 
    //in 1.1 there is no way to choose a config, report as config id # 0
    //This config is passed to getDisplayAttributes. Ignore for now.
    switch(disp) {
        
        case HWC_DISPLAY_PRIMARY:
            if(*numConfigs > 0) {
                configs[0] = 0;
                *numConfigs = 1;
            }
            ret = 0; //NO_ERROR
            break;
        case HWC_DISPLAY_EXTERNAL:
            ret = -1; //Not connected
            if(pdev->dpyAttr[HWC_DISPLAY_EXTERNAL].connected) {
                ret = 0; //NO_ERROR
                if(*numConfigs > 0) {
                    configs[0] = 0;
                    *numConfigs = 1;
                }
            }
            break;
    }
   return 0;
}

static int hwc_getDisplayAttributes(struct hwc_composer_device_1* dev, int disp,
            			 uint32_t config, const uint32_t* attributes, int32_t* values)
{

    hwcContext  *pdev = (hwcContext  *)dev; 
    //If hotpluggable displays are inactive return error
    if(disp == HWC_DISPLAY_EXTERNAL && !pdev->dpyAttr[disp].connected) {
        return -1;
    }
    static  uint32_t DISPLAY_ATTRIBUTES[] = {
        HWC_DISPLAY_VSYNC_PERIOD,
        HWC_DISPLAY_WIDTH,
        HWC_DISPLAY_HEIGHT,
        HWC_DISPLAY_DPI_X,
        HWC_DISPLAY_DPI_Y,
        HWC_DISPLAY_NO_ATTRIBUTE,
     };
    //From HWComposer

    const int NUM_DISPLAY_ATTRIBUTES = (sizeof(DISPLAY_ATTRIBUTES)/sizeof(DISPLAY_ATTRIBUTES)[0]);

    for (size_t i = 0; i < NUM_DISPLAY_ATTRIBUTES - 1; i++) {
        switch (attributes[i]) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            values[i] = pdev->dpyAttr[disp].vsync_period;
            break;
        case HWC_DISPLAY_WIDTH:
            values[i] = pdev->dpyAttr[disp].xres;
            ALOGD("%s disp = %d, width = %d",__FUNCTION__, disp,
                    pdev->dpyAttr[disp].xres);
            break;
        case HWC_DISPLAY_HEIGHT:
            values[i] = pdev->dpyAttr[disp].yres;
            ALOGD("%s disp = %d, height = %d",__FUNCTION__, disp,
                    pdev->dpyAttr[disp].yres);
            break;
        case HWC_DISPLAY_DPI_X:
            values[i] = (int32_t) (pdev->dpyAttr[disp].xdpi*1000.0);
            break;
        case HWC_DISPLAY_DPI_Y:
            values[i] = (int32_t) (pdev->dpyAttr[disp].ydpi*1000.0);
            break;
        case HWC_DISPLAY_NO_ATTRIBUTE:
            break;
        default:
            ALOGE("Unknown display attribute %d",
                    attributes[i]);
            return -EINVAL;
        }
    }

   return 0;
}

int is_surport_wfd_optimize()
{
   char value[PROPERTY_VALUE_MAX];
   memset(value,0,PROPERTY_VALUE_MAX);
   property_get("drm.service.enabled", value, "false");
   if (!strcmp(value,"false"))
   {
     return false;
   }
   else
   {
     return true;
   }
}

int hwc_copybit(struct hwc_composer_device_1 *dev,buffer_handle_t src_handle,
                    buffer_handle_t dst_handle,int flag)
{
    ALOGV("hwc_copybit");
    if (src_handle==0 || dst_handle==0)
    {
        return -1;
    }

    struct private_handle_t *srcHandle = (struct private_handle_t *)src_handle;
    struct private_handle_t *dstHandle = (struct private_handle_t *)dst_handle;

    if (srcHandle->format==HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO)
    {
        if (flag > 0)
        {
            ALOGV("============rga_video_copybit");

            //Debug:clear screenshots to 0x80
#if 0
            memset((void*)(srcHandle->base),0x80,4*srcHandle->height*srcHandle->stride);
#endif
            rga_video_copybit(srcHandle,0,0,0,srcHandle->share_fd,RK_FORMAT_YCbCr_420_SP,0,0);
        }

        //do nothing,or it will lead to clear srcHandle's base address.
        if (flag == 0)
        {
            ALOGV("============rga_video_reset");
          //  rga_video_reset();
        }

    }

    return 0;
}

static void hwc_dump(struct hwc_composer_device_1* dev, char *buff, int buff_len)
{
  // return 0;
}

int
hwc_device_open(
    const struct hw_module_t * module,
    const char * name,
    struct hw_device_t ** device
    )
{
    int  status    = 0;
    int rel;
    hwcContext * context = NULL;
    struct fb_fix_screeninfo fixInfo;
    struct fb_var_screeninfo info;
    int refreshRate = 0;
    float xdpi;
    float ydpi;
    uint32_t vsync_period; 
    hw_module_t const* module_gr;
    int err;
    int stride_gr;
    int i;

    
    LOGD("%s(%d):Open hwc device in thread=%d",
         __FUNCTION__, __LINE__, gettid());

    *device = NULL;

    if (strcmp(name, HWC_HARDWARE_COMPOSER) != 0)
    {
        LOGE("%s(%d):Invalid device name!", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    /* Get context. */
    context = _contextAnchor;

    /* Return if already initialized. */
    if (context != NULL)
    {
        /* Increament reference count. */
        context->reference++;

        *device = &context->device.common;
        return 0;
    }


    /* Allocate memory. */
    context = (hwcContext *) malloc(sizeof (hwcContext));
    
    if(context == NULL)
    {
        LOGE("%s(%d):malloc Failed!", __FUNCTION__, __LINE__);
        return -EINVAL;
    }
    memset(context, 0, sizeof (hwcContext));

    context->fbFd = open("/dev/graphics/fb0", O_RDWR, 0);
    if(context->fbFd < 0)
    {
         hwcONERROR(hwcSTATUS_IO_ERR);
    }
#if USE_QUEUE_DDRFREQ
    context->ddrFd = open("/dev/ddr_freq", O_RDWR, 0);
    if(context->ddrFd < 0)
    {
         ALOGE("/dev/ddr_freq open failed !!!!!");
        // hwcONERROR(hwcSTATUS_IO_ERR);
    }
    else
    {
        ALOGD("context->ddrFd ok");
    }
#endif
    rel = ioctl(context->fbFd, RK_FBIOGET_IOMMU_STA, &context->iommuEn);	    
    if (rel != 0)
    {
         hwcONERROR(hwcSTATUS_IO_ERR);
    }


    rel = ioctl(context->fbFd, FBIOGET_FSCREENINFO, &fixInfo);
    if (rel != 0)
    {
         hwcONERROR(hwcSTATUS_IO_ERR);
    }



    if (ioctl(context->fbFd, FBIOGET_VSCREENINFO, &info) == -1)
    {
         hwcONERROR(hwcSTATUS_IO_ERR);
    }
    if (int(info.width) <= 0 || int(info.height) <= 0)
	{
		// the driver doesn't return that information
		// default to 160 dpi
		info.width  = ((info.xres * 25.4f)/160.0f + 0.5f);
		info.height = ((info.yres * 25.4f)/160.0f + 0.5f);
	}
    xdpi =  (info.xres * 25.4f) / info.width;
    ydpi =  (info.yres * 25.4f) / info.height;

    refreshRate = 1000000000000LLU /
    (
       uint64_t( info.upper_margin + info.lower_margin + info.yres )
       * ( info.left_margin  + info.right_margin + info.xres )
       * info.pixclock
     );
    
    if (refreshRate == 0) {
        ALOGW("invalid refresh rate, assuming 60 Hz");
        refreshRate = 60*1000;
    }

    
    vsync_period  = 1000000000 / refreshRate;

    context->fb_blanked = 1;
    context->dpyAttr[HWC_DISPLAY_PRIMARY].fd = context->fbFd;
    //xres, yres may not be 32 aligned
    context->dpyAttr[HWC_DISPLAY_PRIMARY].stride = fixInfo.line_length /(info.xres/8);
    context->dpyAttr[HWC_DISPLAY_PRIMARY].xres = info.xres;
    context->dpyAttr[HWC_DISPLAY_PRIMARY].yres = info.yres;
    context->dpyAttr[HWC_DISPLAY_PRIMARY].relxres = info.xres;
    context->dpyAttr[HWC_DISPLAY_PRIMARY].relyres = info.yres;
    context->dpyAttr[HWC_DISPLAY_PRIMARY].xdpi = xdpi;
    context->dpyAttr[HWC_DISPLAY_PRIMARY].ydpi = ydpi;
    context->dpyAttr[HWC_DISPLAY_PRIMARY].vsync_period = vsync_period;
    context->dpyAttr[HWC_DISPLAY_PRIMARY].connected = true;
    context->info = info;

    /* Initialize variables. */
   
    context->device.common.tag = HARDWARE_DEVICE_TAG;
    context->device.common.version = HWC_DEVICE_API_VERSION_1_3;
    
    context->device.common.module  = (hw_module_t *) module;

    /* initialize the procs */
    context->device.common.close   = hwc_device_close;
    context->device.prepare        = hwc_prepare;
    context->device.set            = hwc_set;
   // context->device.common.version = HWC_DEVICE_API_VERSION_1_0;
    context->device.blank          = hwc_blank;
    context->device.query          = hwc_query;
    context->device.eventControl   = hwc_event_control;

    context->device.registerProcs  = hwc_registerProcs;

    context->device.getDisplayConfigs = hwc_getDisplayConfigs;
    context->device.getDisplayAttributes = hwc_getDisplayAttributes;
    context->device.rkCopybit = hwc_copybit;

    context->device.fbPost2 = hwc_fbPost;
    context->device.dump = hwc_dump;
#if USE_SPECIAL_COMPOSER
    context->device.layer_recover   = hwc_layer_recover;
#else
    context->device.layer_recover   = NULL;
#endif

    /* initialize params of video buffers*/
    for(i=0;i<MaxVideoBackBuffers;i++)
    {
        context->fd_video_bk[i] = -1;
        context->base_video_bk[i] = 0;
        context->pbvideo_bk[i] = NULL;
    }
    context->mCurVideoIndex= 0;

    context->mSkipFlag = 0;
    context->mVideoMode = false;
    context->mNV12_VIDEO_VideoMode = false;
    context->mIsMediaView = false;
    context->mVideoRotate = false;
    context->mGtsStatus   = false;
    context->mTrsfrmbyrga = false;

#if GET_VPU_INTO_FROM_HEAD
    /* initialize params of video source info*/
    for(i=0;i<MAX_VIDEO_SOURCE;i++)
    {
        context->video_info[i].video_base = NULL;
        context->video_info[i].video_hd = NULL;
        context->video_info[i].bMatch=false;
    }
#endif

    /* Get gco2D object pointer. */
    context->engine_fd = open("/dev/rga",O_RDWR,0);
    if( context->engine_fd < 0)
    {
        hwcONERROR(hwcRGA_OPEN_ERR);
        ALOGE("rga open err!");

    }

#if ENABLE_WFD_OPTIMIZE
	 property_set("sys.enable.wfd.optimize","1");
#endif
    {
        int type = hwc_get_int_property("sys.enable.wfd.optimize","0");
        context->wfdOptimize = type;
        init_rga_cfg(context->engine_fd);
        if (type>0 && !is_surport_wfd_optimize())
        {
           property_set("sys.enable.wfd.optimize","0");
        }
    }

    /* Initialize pmem and frameubffer stuff. */
   // context->fbFd         = 0;
   // context->fbPhysical   = ~0U;
   // context->fbStride     = 0;



    if ( info.pixclock > 0 )
    {
        refreshRate = 1000000000000000LLU /
        (
            uint64_t( info.vsync_len + info.upper_margin + info.lower_margin + info.yres )
            * ( info.hsync_len + info.left_margin  + info.right_margin + info.xres )
            * info.pixclock
        );
    }
    else
    {
        ALOGW("fbdev pixclock is zero");
    }

    if (refreshRate == 0)
    {
        refreshRate = 60*1000;  // 60 Hz
    }

    context->fb_fps = refreshRate / 1000.0f;

    context->fbPhysical = fixInfo.smem_start;
    context->fbStride   = fixInfo.line_length;
	context->fbhandle.width = info.xres;
	context->fbhandle.height = info.yres;
#ifdef GPU_G6110
    #if G6110_SUPPORT_FBDC
    context->fbhandle.format = FBDC_ABGR_888;
    #else
    context->fbhandle.format = HAL_PIXEL_FORMAT_RGBA_8888;
    #endif
#else
    context->fbhandle.format = info.nonstd & 0xff;
#endif //end of GPU_G6110
    context->fbhandle.stride = (info.xres+ 31) & (~31);
    context->pmemPhysical = ~0U;
    context->pmemLength   = 0;
	//hwc_get_int_property("ro.rk.soc", "0");
    context->fbSize = info.xres*info.yres*4*3;
    context->lcdSize = info.xres*info.yres*4; 

    mUsedVopNum = 1;
    context->mLcdcNum = 1;
    context->mHdmiSI.HdmiOn = false;
    context->mHdmiSI.NeedReDst = false;
    context->mHdmiSI.vh_flag = false;
    context->mIsBootanimExit = false;
    context->mIsFirstCallbackToHotplug = false;

#ifdef RK3288_BOX
    {
        int fd = -1;
        int ret = -1;
        char name[64];
        char value[10];
        const char node[] = "/sys/class/graphics/fb%u/lcdcid";
        for(unsigned int i = 0;i < 8 && context->mLcdcNum == 1;i++){
            snprintf(name, 64, node, i);
            fd = open(name,O_RDONLY,0);
            if(fd > 0){
                ret = read(fd,value,sizeof(value));
                if(ret < 0){
                    ALOGW("Get fb%d lcdcid fail:%s",i,strerror(errno));
                }else{
                    if(atoi(value)==1){
                        context->mLcdcNum = 2;
                        ALOGI("Get fb%d lcdcid=%d",i,atoi(value));
                    }
                }
                close(fd);
            }else{
                ALOGW("Open fb%d lcdcid fail:%s",i,strerror(errno));
            }
        }
    }
    if(context->mLcdcNum == 2){
        mUsedVopNum = 2;
    }
#endif
#if (defined(GPU_G6110) || defined(RK3288_BOX))
    if(context->mLcdcNum == 1){
        context->screenFd = open("/sys/class/graphics/fb0/screen_info", O_RDONLY);
        if(context->screenFd <= 0){
            ALOGW("fb0 screen_info open fail for:%s",strerror(errno));
        }
    }else{
        context->screenFd = -1;
    }
#else
    context->screenFd = -1;
#endif
    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module_gr);
    ALOGE_IF(err, "FATAL: can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
    if (err == 0) {
        gralloc_open(module_gr, &context->mAllocDev);

        memset(&bkupmanage,0,sizeof(hwbkupmanage));
        bkupmanage.dstwinNo = 0xff;
        bkupmanage.direct_fd=0;

#if USE_SPECIAL_COMPOSER
        for(i=0;i<bakupbufsize;i++)
        {
            err = context->mAllocDev->alloc(context->mAllocDev, context->fbhandle.width,\
                        context->fbhandle.height/4, context->fbhandle.format,\
                        GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_RENDER,\
                        (buffer_handle_t*)(&bkupmanage.bkupinfo[i].phd_bk),&stride_gr);
            if(!err){
                struct private_handle_t*phandle_gr = (struct private_handle_t*)bkupmanage.bkupinfo[i].phd_bk;
                bkupmanage.bkupinfo[i].membk_fd = phandle_gr->share_fd;
                ALOGD("@hwc alloc[%d] [%dx%d,f=%d],fd=%d ",i,phandle_gr->width,
                    phandle_gr->height,phandle_gr->format,phandle_gr->share_fd);
            }
            else{
                ALOGE("hwc alloc[%d] faild",i);
                goto OnError;
            }
        
        }
#endif

    }   
	else
	{
	    ALOGE(" GRALLOC_HARDWARE_MODULE_ID failed");
	}

#if OPTIMIZATION_FOR_DIMLAYER
    context->bHasDimLayer = false;
    err = context->mAllocDev->alloc(context->mAllocDev, context->fbhandle.width, \
                                    context->fbhandle.height,HAL_PIXEL_FORMAT_RGB_565, \
                                    GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_RENDER, \
                                    (buffer_handle_t*)(&(context->mDimHandle)),&stride_gr);
    if(!err){
        struct private_handle_t* handle = (struct private_handle_t*)context->mDimHandle;
        context->mDimFd = handle->share_fd;
#if defined(__arm64__) || defined(__aarch64__)
        context->mDimBase = (long)(GPU_BASE);
#else
        context->mDimBase = (int)(GPU_BASE);
#endif
        ALOGD("Dim buffer alloc fd [%dx%d,f=%d],fd=%d ",context->fbhandle.width,
            context->fbhandle.height,HAL_PIXEL_FORMAT_RGB_565,handle->share_fd);
    }
    else{
            ALOGE("Dim buffer alloc faild");
            goto OnError;
    }

    memset((void*)context->mDimBase,0x0,context->fbhandle.width*context->fbhandle.height*2);
#endif

    err = context->mAllocDev->alloc(context->mAllocDev, 32,32,HAL_PIXEL_FORMAT_RGB_565, \
                                    GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_RENDER, \
                                    (buffer_handle_t*)(&(context->mHdmiSI.FrameHandle)),&stride_gr);
    if(!err){
        struct private_handle_t* handle = (struct private_handle_t*)context->mHdmiSI.FrameHandle;
        context->mHdmiSI.FrameFd = handle->share_fd;
#if defined(__arm64__) || defined(__aarch64__)
        context->mHdmiSI.FrameBase = (long)(GPU_BASE);
#else
        context->mHdmiSI.FrameBase = (int)(GPU_BASE);
#endif
        ALOGD("Frame buffer alloc fd [32x32,f=%d],fd=%d ",HAL_PIXEL_FORMAT_RGB_565,handle->share_fd);
    }
    else{
        ALOGE("Frame buffer alloc faild");
        goto OnError;
    }
    memset((void*)context->mHdmiSI.FrameBase,0x00,32*32*2);

#if SPRITEOPTIMATION
    /*sprite*/
    for(i=0;i<3;i++)
    {
        err = context->mAllocDev->alloc(context->mAllocDev, BufferSize,BufferSize,HAL_PIXEL_FORMAT_RGBA_8888,\
                    GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_RENDER,\
                    (buffer_handle_t*)(&context->mSrBI.handle[i]),&stride_gr);
        if(!err){
            struct private_handle_t*handle = (struct private_handle_t*)context->mSrBI.handle[i];
            context->mSrBI.fd[i] = handle->share_fd;
#if defined(__arm64__) || defined(__aarch64__)
            context->mSrBI.hd_base[i] = (long)(GPU_BASE);
#else
            context->mSrBI.hd_base[i] = (int)(GPU_BASE);
#endif
            ALOGD("@hwc alloc[%d] [%dx%d,f=%d],[hande->type=%d],fd=%d",
                i,handle->width,handle->height,handle->format,handle->type,handle->share_fd);
        }else{
            ALOGE("hwc alloc[%d] faild",i);
            goto OnError;
        }
    }
#endif

    /* Increment reference count. */
    context->reference++;
    context->fun_policy[HWC_HOR] = try_wins_dispatch_hor;
    context->fun_policy[HWC_MIX_VTWO] = try_wins_dispatch_mix_v2;
    context->fun_policy[HWC_MIX_UP] = try_wins_dispatch_mix_up;
    context->fun_policy[HWC_MIX_DOWN] = try_wins_dispatch_mix_down;
    context->fun_policy[HWC_MIX_CROSS] = try_wins_dispatch_mix_cross;
    context->fun_policy[HWC_MIX_VH] = try_wins_dispatch_mix_vh;
    _contextAnchor = context;
#if VIRTUAL_RGA_BLIT
    _contextAnchor2 = (hwcContext *) malloc(sizeof (hwcContext));
    memcpy((void*)_contextAnchor2,(void*)context,sizeof(hwcContext));
#endif
    if (context->fbhandle.width > context->fbhandle.height)
    {
        property_set("sys.display.oritation","0");
    }
    else
    {
        property_set("sys.display.oritation","2");
    }
    _eglRenderBufferModifiedANDROID = (PFNEGLRENDERBUFFERMODIFYEDANDROIDPROC)
                                    eglGetProcAddress("eglRenderBufferModifiedANDROID");

    if(_eglRenderBufferModifiedANDROID == NULL)
    {
        LOGE("EGL_ANDROID_buffer_modifyed extension "
             "Not Found for hwcomposer");

        hwcONERROR(hwcTHREAD_ERR);
    }
    

#if USE_HW_VSYNC

    context->vsync_fd = open("/sys/class/graphics/fb0/vsync", O_RDONLY, 0);
    //context->vsync_fd = open("/sys/devices/platform/rk30-lcdc.0/vsync", O_RDONLY);
    if (context->vsync_fd < 0) {
        hwcONERROR(hwcSTATUS_IO_ERR);
    }

    if (pthread_mutex_init(&context->lock, NULL))
    {
        hwcONERROR(hwcMutex_ERR);
    }

    if (pthread_create(&context->hdmi_thread, NULL, hwc_thread, context))
    {
        hwcONERROR(hwcTHREAD_ERR);
    }
#endif

    /* Return device handle. */
    *device = &context->device.common;

    LOGD("RGA HWComposer verison%s\n"
         "Device:               %p\n"
         "fb_fps=%f",
         "1.0.0",
         context,
         context->fb_fps);

    property_set("sys.ghwc.version",GHWC_VERSION); 
    LOGD(HWC_VERSION);

    char Version[32];

    memset(Version,0,sizeof(Version));
    if(ioctl(context->engine_fd, RGA_GET_VERSION, Version) == 0)
    {
        property_set("sys.grga.version",Version);
        LOGD(" rga version =%s",Version);

    }
#ifdef TARGET_BOARD_PLATFORM_RK3368
    if(0 == hwc_get_int_property("ro.rk.soc", "0"))
        property_set("sys.rk.soc","rk3368");
#endif
    /*
    context->ippDev = new ipp_device_t();
    rel = ipp_open(context->ippDev);
    if (rel < 0)
    {
        delete context->ippDev;
        context->ippDev = NULL;
        ALOGE("Open ipp device fail.");
    }
    */
    init_hdmi_mode();
    pthread_t t;
    if (pthread_create(&t, NULL, rk_hwc_hdmi_thread, NULL))
    {
        LOGD("Create readHdmiMode thread error .");
    }
#if HWC_EXTERNAL
	pthread_t t0;
	if (pthread_create(&t0, NULL, hotplug_try_register, NULL))
    {
        LOGD("Create hotplug_try_register thread error .");
    }
#endif
#ifdef RK3288_BOX
    if(context->mLcdcNum == 2){
        hwc_change_config();
    }
#endif
    initCrcTable();
    return 0;

OnError:

    if (context->vsync_fd > 0)
    {
        close(context->vsync_fd);
    }
    if(context->fbFd > 0)
    {
        close(context->fbFd );

    }
    if(context->fbFd1 > 0)
    {
        close(context->fbFd1 );
    }

#if  USE_SPECIAL_COMPOSER
    for(i=0;i<bakupbufsize;i++)
    {
        if(bkupmanage.bkupinfo[i].phd_bk)
        {
            err = context->mAllocDev->free(context->mAllocDev, bkupmanage.bkupinfo[i].phd_bk);
            ALOGW_IF(err, "free(...) failed %d (%s)", err, strerror(-err));            
        }      
    
    }
    if(bkupmanage.phd_drt)
    {
        err = context->mAllocDev->free(context->mAllocDev, bkupmanage.phd_drt);
        ALOGW_IF(err, "free(...) failed %d (%s)", err, strerror(-err));            
    }      
#endif
    pthread_mutex_destroy(&context->lock);

    /* Error roll back. */ 
    if (context != NULL)
    {
        if (context->engine_fd != 0)
        {
            close(context->engine_fd);
        }
        free(context);

    }
    
    *device = NULL;
    LOGE("%s(%d):Failed!", __FUNCTION__, __LINE__);

    return -EINVAL;
}

int  getHdmiMode()
{
#if 0
    char pro_value[16];
    property_get("sys.hdmi.mode",pro_value,0);
    int mode = atoi(pro_value);
    return mode;
#else
    // LOGD("g_hdmi_mode=%d",g_hdmi_mode);
#endif
    // LOGD("g_hdmi_mode=%d",g_hdmi_mode);

    return g_hdmi_mode;
}

int hwc_read_node(const char *intValue,char *outValue,int flag)
{
    int fd = -1;
    int ret = -1;
    size_t size = sizeof(outValue);
    fd = open(intValue, O_RDONLY);
    memset(outValue, 0, size);
    int err = read(fd, outValue, sizeof(outValue));
    if (err < 0){
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return ret;
    }
    ret = atoi(outValue);
    close(fd);
    return ret;
}

void init_hdmi_mode()
{
#ifdef RK3288_BOX
    if(_contextAnchor->mLcdcNum == 2){
        int index = -1;
        int connect = -1;
        char outputValue[100];
        char inputValue[100] = "/sys/devices/virtual/display/HDMI/connect";
        connect = hwc_read_node(inputValue,outputValue,0);
        if(connect >= 0){
            memset(inputValue, 0, sizeof(inputValue));
            strcpy(inputValue,"/sys/devices/virtual/display/HDMI/property");
            index = hwc_read_node(inputValue,outputValue,0);
            ALOGD("%d,index=%d,connect=%d,hdmi=%d",__LINE__,index,connect,g_hdmi_mode);
            if(index == 1 && connect == 1){
                g_hdmi_mode = 1;
            }else if(index == 1){
                g_hdmi_mode = 0;
            }
        }
        index = -1;
        connect = -1;
        memset(inputValue, 0, sizeof(inputValue));
        strcpy(inputValue,"/sys/devices/virtual/display/HDMI1/connect");
        connect = hwc_read_node(inputValue,outputValue,0);
        if(connect >= 0){
            memset(inputValue, 0, sizeof(inputValue));
            strcpy(inputValue,"/sys/devices/virtual/display/HDMI1/property");
            index = hwc_read_node(inputValue,outputValue,0);
            ALOGD("%d,index=%d,connect=%d,hdmi=%d",__LINE__,index,connect,g_hdmi_mode);
            if(index == 1 && connect == 1){
                g_hdmi_mode = 1;
            }else if(index == 1){
                g_hdmi_mode = 0;
            }
        }
    }else {
        int fd = open("/sys/devices/virtual/switch/hdmi/state", O_RDONLY);	
        if (fd > 0){
            char statebuf[100];
            memset(statebuf, 0, sizeof(statebuf));
            int err = read(fd, statebuf, sizeof(statebuf));
            if (err < 0){
                ALOGE("error reading vsync timestamp: %s", strerror(errno));
                return;
            }
            close(fd);
            g_hdmi_mode = atoi(statebuf);
        }else{
            LOGE("Open hdmi mode error.");
        }
    }
#else
    int fd = open("/sys/devices/virtual/switch/hdmi/state", O_RDONLY);	
    if (fd > 0)
    {
        char statebuf[100];
        memset(statebuf, 0, sizeof(statebuf));
        int err = read(fd, statebuf, sizeof(statebuf));

        if (err < 0)
        {
            ALOGE("error reading vsync timestamp: %s", strerror(errno));
            return;
        }
        close(fd);
        g_hdmi_mode = atoi(statebuf);
        /* if (g_hdmi_mode==0)
        {
        property_set("sys.hdmi.mode", "0");
        }
        else
        {
        property_set("sys.hdmi.mode", "1");
        }*/
    }
    else
    {
        LOGE("Open hdmi mode error.");
    }

    if(g_hdmi_mode == 1)
    {
#ifdef GPU_G6110
        //hotplug_free_dimbuffer();
        //hotplug_get_config(0);
        //hotplug_set_config();
#endif
    }
#endif 
}
int closeFb(int fd)
{
    if (fd > 0)
    {
        int disable = 0;

        if (ioctl(fd, 0x5019, &disable) == -1)
        {
            LOGE("close fb[%d] fail.",fd);
            return -1;
        }
        ALOGD("fb1 realy close!");
        return (close(fd));
    }
    return -1;
}
int hotplug_get_config(int flag){
    /*flag:0 hdmi;1 cvbs*/
    ALOGD("enter %s", __FUNCTION__);
    //memset(values, 0, sizeof(values));
    int fd;
    int err;
    int stride_gr;
    hwcContext *context = NULL;
    context = _contextAnchor1;
    if(context == NULL ){
        context = (hwcContext *) malloc(sizeof (hwcContext));
        if(context==NULL){
            ALOGE("hotplug_get_config:Alloc context fail");
            return -1;
        }
        memset(context, 0, sizeof (hwcContext));
    }
	struct fb_var_screeninfo info = _contextAnchor->info;
	int outX = 0;
	int outY = 0;
	hotplug_parse_mode(&outX, &outY);
	info.xres = outX;
	info.yres = outY;
	info.yres_virtual = info.yres * 3;
    info.xres_virtual = info.xres;
	info.grayscale = 0;
	info.grayscale |= info.xres<< 8;
	info.grayscale |= info.yres<<20;
#if (defined(GPU_G6110) || defined(RK3288_BOX))
#ifdef RK3288_BOX
    if(_contextAnchor->mLcdcNum == 1){
        if(_contextAnchor->fbFd > 0){
            fd  =  _contextAnchor->fbFd;
        }else{
            fd  =  open("/dev/graphics/fb0", O_RDWR, 0);
        }
    }else{
    	if(context->fbFd > 0){
    	    fd  =  context->fbFd;
        }else{
            fd  =  open("/dev/graphics/fb4", O_RDWR, 0);
        }
    }
#else
    if(_contextAnchor->fbFd > 0){
        fd  =  _contextAnchor->fbFd;
    }else{
        fd  =  open("/dev/graphics/fb0", O_RDWR, 0);
    }
#endif
#else
	if(context->fbFd > 0){
	    fd  =  context->fbFd;
    }else{
        fd  =  open("/dev/graphics/fb4", O_RDWR, 0);
    }
#endif
	if (fd < 0){
	    ALOGE("hotplug_get_config:open /dev/graphics/fb4 fail");
        return -errno;
	}
#ifndef GPU_G6110
#ifdef RK3288_BOX
    if(_contextAnchor->mLcdcNum == 2){
        info.reserved[3] |= 1;
#endif
    	if (ioctl(fd, FBIOPUT_VSCREENINFO, &info)){
    	    ALOGE("hotplug_get_config:FBIOPUT_VSCREENINFO error,hdmifd=%d",fd);
            return -errno;
    	}
#ifdef RK3288_BOX
    }
#endif
#endif
    context->fd_3d = _contextAnchor->fd_3d;
    if(context->fd_3d<=0){
        context->fd_3d = open("/sys/class/display/HDMI/3dmode", O_RDWR, 0);
        if(context->fd_3d < 0){
            ALOGE("open /sys/class/display/HDMI/3dmode fail");
        }
        _contextAnchor->fd_3d = context->fd_3d;
    }

#if (defined(RK3368_BOX) || defined(RK3288_BOX))
    if(flag == 1){
        char buf[100];
        int width = 0;
        int height = 0;
        int fdExternal = -1;
#ifdef RK3288_BOX
        if(_contextAnchor->mLcdcNum == 2){
            fdExternal = open("/sys/class/graphics/fb4/screen_info", O_RDONLY);
        }else{
            fdExternal = open("/sys/class/graphics/fb0/screen_info", O_RDONLY);
        }
#else
        fdExternal = open("/sys/class/graphics/fb0/screen_info", O_RDONLY);
#endif
        if(fdExternal < 0){
            ALOGE("hotplug_get_config:open fb screen_info error,cvbsfd=%d",fdExternal);
            return -errno;
    	}
        if(read(fdExternal,buf,sizeof(buf)) < 0){
            ALOGE("error reading fb screen_info: %s", strerror(errno));
            return -1;
        }
        close(fdExternal);
		sscanf(buf,"xres:%d yres:%d",&width,&height);
        ALOGD("hotplug_get_config:width=%d,height=%d",width,height);
    	info.xres = width;
    	info.yres = height;
    }
#endif
#if USE_QUEUE_DDRFREQ
    context->ddrFd = _contextAnchor->fbFd;
#endif
    int refreshRate = 0;
	if ( info.pixclock > 0 ){
		refreshRate = 1000000000000000LLU /
		(
			uint64_t( info.vsync_len + info.upper_margin + info.lower_margin + info.yres )
			* ( info.hsync_len + info.left_margin  + info.right_margin + info.xres )
			* info.pixclock
		);
	}else{
		ALOGD( "fbdev pixclock is zero for fd: %d", fd );
	}

	if (refreshRate == 0){
		refreshRate = 60*1000;  // 60 Hz
	}
	struct fb_fix_screeninfo finfo;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1){
	    ALOGE("FBIOGET_FSCREENINFO,hdmifd=%d",fd);
		return -errno;
	}
	if (int(info.width) <= 0 || int(info.height) <= 0){
		// the driver doesn't return that information
		// default to 160 dpi
		info.width  = ((info.xres * 25.4f)/160.0f + 0.5f);
		info.height = ((info.yres * 25.4f)/160.0f + 0.5f);
	}

	float xdpi = (info.xres * 25.4f) / info.width;
	float ydpi = (info.yres * 25.4f) / info.height;
	unsigned int vsync_period  = 1000000000 / refreshRate;
	
	context->dpyAttr[HWC_DISPLAY_EXTERNAL].fd = context->fbFd = fd;
    //xres, yres may not be 32 aligned
    context->dpyAttr[HWC_DISPLAY_EXTERNAL].stride = finfo.line_length /(info.xres/8);
    context->dpyAttr[HWC_DISPLAY_EXTERNAL].xres = info.xres;
    context->dpyAttr[HWC_DISPLAY_EXTERNAL].yres = info.yres;
    context->dpyAttr[HWC_DISPLAY_EXTERNAL].xdpi = xdpi;
    context->dpyAttr[HWC_DISPLAY_EXTERNAL].ydpi = ydpi;
    context->dpyAttr[HWC_DISPLAY_EXTERNAL].vsync_period = vsync_period;
    context->dpyAttr[HWC_DISPLAY_EXTERNAL].connected = true;
    context->info = info;
    context->mAllocDev = _contextAnchor->mAllocDev;

    /* initialize params of video buffers*/
    for(int i=0;i<MaxVideoBackBuffers;i++){
        context->fd_video_bk[i] = -1;
        context->base_video_bk[i] = 0;
        context->pbvideo_bk[i] = NULL;
    }
    context->mCurVideoIndex= 0;

	context->fb_blanked = 1;
    context->mSkipFlag = 0;
    context->mVideoMode = false;
    context->mNV12_VIDEO_VideoMode = false;
    context->mIsMediaView = false;
    context->mVideoRotate = false;
    context->mGtsStatus   = false;
    context->mTrsfrmbyrga = false;

    context->fb_fps = refreshRate / 1000.0f;

    context->fbPhysical = finfo.smem_start;
    context->fbStride   = finfo.line_length;
	context->fbhandle.width = info.xres;
	context->fbhandle.height = info.yres;
#ifdef GPU_G6110
    #if G6110_SUPPORT_FBDC
    context->fbhandle.format = FBDC_ABGR_888;
    #else
    context->fbhandle.format = HAL_PIXEL_FORMAT_RGBA_8888;
    #endif
#else
    context->fbhandle.format = info.nonstd & 0xff;
#endif
    context->fbhandle.stride = (info.xres+ 31) & (~31);
    context->pmemPhysical = ~0U;
    context->pmemLength   = 0;
	//hwc_get_int_property("ro.rk.soc", "0");
    context->fbSize = info.xres*info.yres*4*3;
    context->lcdSize = info.xres*info.yres*4; 

#if OPTIMIZATION_FOR_DIMLAYER
    err = _contextAnchor->mAllocDev->alloc(_contextAnchor->mAllocDev, context->fbhandle.width, \
                                    context->fbhandle.height,HAL_PIXEL_FORMAT_RGB_565, \
                                    GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_RENDER, \
                                    (buffer_handle_t*)(&(context->mDimHandle)),&stride_gr);
    if(!err){
        struct private_handle_t*handle = (struct private_handle_t*)context->mDimHandle;
        context->mDimFd = handle->share_fd;
#if defined(__arm64__) || defined(__aarch64__)
        context->mDimBase = (long)(GPU_BASE);
#else
        context->mDimBase = (int)(GPU_BASE);
#endif
        ALOGD("Dim buffer alloc fd [%dx%d,f=%d],fd=%d ",context->fbhandle.width,context->fbhandle.height,HAL_PIXEL_FORMAT_RGB_565,handle->share_fd);                                

    }else{
        ALOGE("Dim buffer alloc faild");
        goto OnError;
    }

    memset((void*)context->mDimBase,0x0,context->fbhandle.width*context->fbhandle.height*2);
#endif
#if SPRITEOPTIMATION
    /*sprite*/
    for(int i=0;i<MaxSpriteBNUM;i++){
        if(_contextAnchor->mSrBI.handle[i]){
            context->mSrBI.fd[i]      = _contextAnchor->mSrBI.fd[i];
            context->mSrBI.hd_base[i] = _contextAnchor->mSrBI.hd_base[i];
            context->mSrBI.handle[i]  = _contextAnchor->mSrBI.handle[i];
        }else{
            err = context->mAllocDev->alloc(context->mAllocDev, BufferSize,BufferSize,HAL_PIXEL_FORMAT_RGBA_8888,\
                GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_RENDER,\
                (buffer_handle_t*)(&context->mSrBI.handle[i]),&stride_gr);
            if(!err){
                struct private_handle_t*handle = (struct private_handle_t*)context->mSrBI.handle[i];
                context->mSrBI.fd[i] = handle->share_fd;
#if defined(__arm64__) || defined(__aarch64__)
                context->mSrBI.hd_base[i] = (long)(GPU_BASE);
#else
                context->mSrBI.hd_base[i] = (int)(GPU_BASE);
#endif
                _contextAnchor->mSrBI.fd[i]      = context->mSrBI.fd[i];
                _contextAnchor->mSrBI.hd_base[i] = context->mSrBI.hd_base[i];
                _contextAnchor->mSrBI.handle[i]  = context->mSrBI.handle[i];
                ALOGD("@hwc alloc[%d] [%dx%d,f=%d],fd=%d ",
                    i,handle->width,handle->height,handle->format,handle->share_fd);
            }else{
                ALOGE("hwc alloc[%d] faild",i);
                goto OnError;
            }
        }
    }
    context->mSrBI.mCurIndex = 0;
#endif
    context->fun_policy[HWC_HOR] = try_wins_dispatch_hor;
    context->fun_policy[HWC_MIX_DOWN] = try_wins_dispatch_mix_down;
    context->fun_policy[HWC_MIX_CROSS] = try_wins_dispatch_mix_cross;
    context->fun_policy[HWC_MIX_VTWO] = try_wins_dispatch_mix_v2;
    context->fun_policy[HWC_MIX_UP] = try_wins_dispatch_mix_up;
    context->fun_policy[HWC_MIX_VH] = try_wins_dispatch_mix_vh;
    _contextAnchor1 = context;
#ifndef GPU_G6110
#ifdef RK3288_BOX
    if(_contextAnchor->mLcdcNum == 2)
#endif
    {
        hotplug_set_frame(_contextAnchor,0);
    }
#endif

    return 1;

OnError:

    if (context->vsync_fd > 0)
    {
        close(context->vsync_fd);
    }
    if(context->fbFd > 0)
    {
        close(context->fbFd );
    }
    if(context->fbFd1 > 0)
    {
        close(context->fbFd1 );
    }

#if OPTIMIZATION_FOR_DIMLAYER
    if(context->mDimHandle)
    {
        err = _contextAnchor->mAllocDev->free(_contextAnchor->mAllocDev, context->mDimHandle);
        ALOGW_IF(err, "free mDimHandle (...) failed %d (%s)", err, strerror(-err));
    }
#endif
    pthread_mutex_destroy(&context->lock);

    /* Error roll back. */ 
    if (context != NULL)
    {
        if (context->engine_fd != 0)
        {
            close(context->engine_fd);
        }
        free(context);
    }
    _contextAnchor1 = NULL;
    LOGE("%s(%d):Failed!", __FUNCTION__, __LINE__);

    return -EINVAL;

}

int hotplug_parse_mode(int *outX, int *outY)
{
   int fd = open("/sys/class/display/HDMI/mode", O_RDONLY);
   ALOGD("enter %s", __FUNCTION__);

   if (fd > 0)
   {
        char statebuf[100];
        memset(statebuf, 0, sizeof(statebuf));
        int err = read(fd, statebuf, sizeof(statebuf));
        if (err < 0)
        {
            ALOGE("error reading hdmi mode: %s", strerror(errno));
            return -1;
        }
        //ALOGD("statebuf=%s",statebuf);
        close(fd);
        char xres[10];
        char yres[10];
        int temp = 0;
        memset(xres, 0, sizeof(xres));
        memset(yres, 0, sizeof(yres));
        for (unsigned int i=0; i<strlen(statebuf); i++)
        {
            if (statebuf[i] >= '0' && statebuf[i] <= '9')
            {
                xres[i] = statebuf[i];
            }
            else
            {
                temp = i;
                break;
            }
        }
        int m = 0;
        for (unsigned int j=temp+1; j<strlen(statebuf);j++)
        {
            if (statebuf[j] >= '0' && statebuf[j] <= '9')
            {
                yres[m] = statebuf[j];
                m++;
            }
            else
            {
                break;
            }
        }
        *outX = atoi(xres);
        *outY = atoi(yres);
        close(fd);
        return 0;
    }
    else
    {
        close(fd);
        ALOGE("Get HDMI mode fail");
        return -1;
    }
}

int hotplug_set_config(){
    int dType = HWC_DISPLAY_EXTERNAL;
    hwcContext * context = _contextAnchor;
    hwcContext * context1 = _contextAnchor1;
    if(context1 != NULL){
        context->dpyAttr[dType].fd = context1->dpyAttr[dType].fd;
        context->dpyAttr[dType].stride = context1->dpyAttr[dType].stride;
        context->dpyAttr[dType].xres = context1->dpyAttr[dType].xres;
        context->dpyAttr[dType].yres = context1->dpyAttr[dType].yres;
        context->dpyAttr[dType].xdpi = context1->dpyAttr[dType].xdpi;
        context->dpyAttr[dType].ydpi = context1->dpyAttr[dType].ydpi;
        context->dpyAttr[dType].vsync_period = context1->dpyAttr[dType].vsync_period;
        context->dpyAttr[dType].connected = true;
#if 1//(defined(GPU_G6110) || defined(RK3288_BOX))
        //box source can not be bigger than 1080p
        if(context->dpyAttr[dType].yres > 1080){
            context->dpyAttr[dType].xres = 1920;
            context->dpyAttr[dType].yres = 1080;
            context->mHdmiSI.NeedReDst = true;
            LOGV("w_s,h_s,w_d,h_d = [%d,%d,%d,%d]",
                context->dpyAttr[dType].xres,
                context->dpyAttr[dType].yres,
                context1->dpyAttr[dType].xres,
                context1->dpyAttr[dType].yres);
        }else{
            context->mHdmiSI.NeedReDst = false;
        }
#endif
        return 1;
    }else{
        context->dpyAttr[dType].connected = false;
        ALOGE("hotplug_set_config fail");
        return -1;
    }
}

void hotplug_get_resolution(int* w,int* h)
{
    *w = int(_contextAnchor->dpyAttr[HWC_DISPLAY_EXTERNAL].xres);
    *h = int(_contextAnchor->dpyAttr[HWC_DISPLAY_EXTERNAL].yres);
}

int hotplug_close_device()
{
    int i;
    int err=0;
    hwcContext * context = _contextAnchor1;

    if(context->engine_fd)
        close(context->engine_fd);
    /* Clean context. */
    if(context->vsync_fd > 0)
        close(context->vsync_fd);
    if(context->fbFd > 0)
    {
        close(context->fbFd );
    }
    if(context->fbFd1 > 0)
    {
        close(context->fbFd1 );
    }
    
    // free video gralloc buffer
    for(i=0;i<MaxVideoBackBuffers;i++)
    {
        if(context->pbvideo_bk[i] != NULL)
            err = context->mAllocDev->free(context->mAllocDev, context->pbvideo_bk[i]);
        if(!err)
        {
            context->fd_video_bk[i] = -1;
            context->base_video_bk[i] = 0;
            context->pbvideo_bk[i] = NULL;
        }
        ALOGW_IF(err, "free pbvideo_bk (...) failed %d (%s)", err, strerror(-err));
    }

#if OPTIMIZATION_FOR_DIMLAYER
    if(context->mDimHandle)
    {
        err = context->mAllocDev->free(context->mAllocDev, context->mDimHandle);
        ALOGW_IF(err, "free mDimHandle (...) failed %d (%s)", err, strerror(-err));
    }
#endif

    pthread_mutex_destroy(&context->lock);
    free(context);
    _contextAnchor1 = NULL;
    return 0;
}

void *hotplug_try_register(void *arg)
{
    prctl(PR_SET_NAME,"HWC_htg1");
    HWC_UNREFERENCED_PARAMETER(arg);
    hwcContext * context = _contextAnchor;
    int count = 0;
#ifndef RK3368_BOX
#if RK3288_BOX
    if(context->mLcdcNum == 2)
#endif
    {
        if(getHdmiMode() == 1){
            hotplug_free_dimbuffer();
            hotplug_get_config(0);
        }
    }
#endif
    while(context->fb_blanked){
        count++;
        usleep(10000);
        if(300==count){
            ALOGW("wait for primary unblank");
            break;
        }
    }
    if(getHdmiMode() == 1){
        handle_hotplug_event(1, 6);
		ALOGI("hotplug_try_register at line = %d",__LINE__);
    }else{
#if (defined(RK3368_BOX) || defined(RK3288_BOX))
#if RK3288_BOX
        if(context->mLcdcNum == 1){
            handle_hotplug_event(1, 1);
            ALOGI("hotplug_try_register at line = %d",__LINE__);
        }
#else
        handle_hotplug_event(1, 1);
        ALOGI("hotplug_try_register at line = %d",__LINE__);
#endif
#endif
    }
#if (defined(GPU_G6110) || defined(RK3288_BOX))
#if RK3288_BOX
    if(context->mLcdcNum == 2){
        goto READY;
    }
#endif
    while(!context->mIsBootanimExit){
        int i = 0;
        char value[PROPERTY_VALUE_MAX];
        property_get("service.bootanim.exit",value,"0");
        i = atoi(value);
        if(1==i){
            context->mIsBootanimExit = true;
            context->mIsFirstCallbackToHotplug = true;
        }else{
            usleep(30000);
        }
    }
    if(getHdmiMode() == 1){
        handle_hotplug_event(1, 6);
		ALOGI("hotplug_try_register at line = %d",__LINE__);
#ifndef RK3368_MID
    }else{
        handle_hotplug_event(1, 1);
		ALOGI("hotplug_try_register at line = %d",__LINE__);
#endif
    }
#endif

READY:
    pthread_exit(NULL);
    return NULL;
}

int hotplug_set_overscan(int flag)
{
    char new_valuep[PROPERTY_VALUE_MAX];
    char new_valuee[PROPERTY_VALUE_MAX];

    switch(flag){
    case 0:
        property_get("persist.sys.overscan.main", new_valuep, "false");
        property_get("persist.sys.overscan.aux",  new_valuee, "false");
        break;

    case 1:
        strcpy(new_valuep,"overscan 100,100,100,100");
        strcpy(new_valuee,"overscan 100,100,100,100");
        break;

    default:
        break;
    }

    int fdp = open("/sys/class/graphics/fb0/scale",O_RDWR);
    if(fdp > 0){
        int ret = write(fdp,new_valuep,sizeof(new_valuep));
        if(ret != sizeof(new_valuep)){
            ALOGE("write /sys/class/graphics/fb0/scale fail");
            close(fdp);
            return -1;
        }
        ALOGV("new_valuep=[%s]",new_valuep);
        close(fdp);
    }
#ifdef RK3288_BOX
    if(_contextAnchor->mLcdcNum == 2){
        int fde = open("/sys/class/graphics/fb4/scale",O_RDWR);
        if(fde > 0){
            int ret = write(fde,new_valuee,sizeof(new_valuee));
            if(ret != sizeof(new_valuee)){
                ALOGE("write /sys/class/graphics/fb4/scale fail");
                close(fde);
                return -1;
            }
            ALOGV("new_valuep=[%s]",new_valuee);
            close(fde);
        }
    }
#endif
    return 0;
}

int hotplug_reset_dstposition(struct rk_fb_win_cfg_data * fb_info,int flag)
{
    /*flag:HDMI hotplug has two situation
    *1:
    *0:mHdmiSI.NeedReDst case hotplug 1080p when 4k
    */
    hwcContext *context = _contextAnchor;
    char buf[100];
    int fd = context->screenFd;
    unsigned int w_source = 0;
    unsigned int h_source = 0;
    unsigned int w_dst = 0;
    unsigned int h_dst = 0;
    unsigned int w_hotplug = 0;
    unsigned int h_hotplug = 0;
    if(fb_info == NULL){
        return -1;
    }

    if(flag != 2 && _contextAnchor1 == NULL){
        return -1;
    }

    //ALOGD("%s,%d",__FUNCTION__,__LINE__);
    switch(flag){
    case 0:
        w_source = context->dpyAttr[HWC_DISPLAY_EXTERNAL].xres;
        h_source = context->dpyAttr[HWC_DISPLAY_EXTERNAL].yres;
        w_dst    = _contextAnchor1->dpyAttr[HWC_DISPLAY_EXTERNAL].xres;
        h_dst    = _contextAnchor1->dpyAttr[HWC_DISPLAY_EXTERNAL].yres;
        break;

    case 1:
        w_source = context->dpyAttr[HWC_DISPLAY_PRIMARY].xres;
        h_source = context->dpyAttr[HWC_DISPLAY_PRIMARY].yres;
        w_hotplug = context->dpyAttr[HWC_DISPLAY_EXTERNAL].xres;
        h_hotplug = context->dpyAttr[HWC_DISPLAY_EXTERNAL].yres;
        lseek(fd,0,SEEK_SET);
        if(read(fd,buf,sizeof(buf)) < 0){
            ALOGE("error reading fb screen_info:%d,%s",fd,strerror(errno));
            return -1;
        }
		sscanf(buf,"xres:%d yres:%d",&w_dst,&h_dst);
        ALOGD_IF(mLogL&HLLONE,"width=%d,height=%d",w_dst,h_dst);
        break;

    case 2:
        w_source = context->dpyAttr[HWC_DISPLAY_PRIMARY].xres;
        h_source = context->dpyAttr[HWC_DISPLAY_PRIMARY].yres;
        w_dst    = context->dpyAttr[HWC_DISPLAY_PRIMARY].relxres;
        h_dst    = context->dpyAttr[HWC_DISPLAY_PRIMARY].relyres;
        break;

    default:
        break;
    }
    
    float w_scale = (float)w_dst / w_source; 
    float h_scale = (float)h_dst / h_source;
    
    if(h_source != h_dst)
    {   
        for(int i = 0;i<4;i++)
        {
            for(int j=0;j<4;j++)
            {
                if(fb_info->win_par[i].area_par[j].ion_fd || fb_info->win_par[i].area_par[j].phy_addr)
                {
                    fb_info->win_par[i].area_par[j].xpos  =
                        (unsigned short)(fb_info->win_par[i].area_par[j].xpos * w_scale);
                    fb_info->win_par[i].area_par[j].ypos  =
                        (unsigned short)(fb_info->win_par[i].area_par[j].ypos * h_scale);
                    fb_info->win_par[i].area_par[j].xsize =
                        (unsigned short)(fb_info->win_par[i].area_par[j].xsize * w_scale);
                    fb_info->win_par[i].area_par[j].ysize =
                        (unsigned short)(fb_info->win_par[i].area_par[j].ysize * h_scale);
                    ALOGD_IF(mLogL&HLLONE,"Adjust dst to => [%d,%d,%d,%d]",
                        fb_info->win_par[i].area_par[j].xpos,fb_info->win_par[i].area_par[j].ypos,
                        fb_info->win_par[i].area_par[j].xsize,fb_info->win_par[i].area_par[j].ysize);
                }
            }
        }
    }
    return 0;
}

int hotplug_set_frame(hwcContext* context,int flag)
{
    int ret = 0;
    struct rk_fb_win_cfg_data fb_info;
    memset(&fb_info,0,sizeof(fb_info));
    fb_info.ret_fence_fd = -1;
    for(int i=0;i<RK_MAX_BUF_NUM;i++) {
        fb_info.rel_fence_fd[i] = -1;
    }

    fb_info.win_par[0].area_par[0].data_format = HAL_PIXEL_FORMAT_RGB_565;
    fb_info.win_par[0].win_id = 0;
    fb_info.win_par[0].z_order = 0;
    fb_info.win_par[0].area_par[0].ion_fd = context->mHdmiSI.FrameFd;
    fb_info.win_par[0].area_par[0].acq_fence_fd = -1;
    fb_info.win_par[0].area_par[0].x_offset = 0;
    fb_info.win_par[0].area_par[0].y_offset = 0;
    fb_info.win_par[0].area_par[0].xpos = 0;
    fb_info.win_par[0].area_par[0].ypos = 0;
    fb_info.win_par[0].area_par[0].xsize = 32;
    fb_info.win_par[0].area_par[0].ysize = 32;
    fb_info.win_par[0].area_par[0].xact = 32;
    fb_info.win_par[0].area_par[0].yact = 32;
    fb_info.win_par[0].area_par[0].xvir = 32;
    fb_info.win_par[0].area_par[0].yvir = 32;
    fb_info.wait_fs = 0;

    if(ioctl(_contextAnchor1->fbFd, RK_FBIOSET_CONFIG_DONE, &fb_info) == -1){
        ALOGE("%s,%d,RK_FBIOSET_CONFIG_DONE fail",__FUNCTION__,__LINE__);
    }else{
        ALOGD_IF(mLogL&HLLONE,"hotplug_set_frame");
    }

    for(int k=0;k<RK_MAX_BUF_NUM;k++){
        //ALOGD("%s,%d,fb_info.rel_fence_fd[%d]=%d",
            //__FUNCTION__,__LINE__,k,fb_info.rel_fence_fd[k]);
        if(fb_info.rel_fence_fd[k] >=0 ){
            ret = 1;
            close(fb_info.rel_fence_fd[k]);
        }    
    }
    //ALOGD("%s,%d,fb_info.ret_fence_fd=%d",
        //__FUNCTION__,__LINE__,fb_info.ret_fence_fd);
    if(fb_info.ret_fence_fd >= 0){
        ret = 1;
        close(fb_info.ret_fence_fd);
    }
    return ret;
}
int hwc_sprite_replace(hwcContext * Context,hwc_display_contents_1_t * list)
{
#if SPRITEOPTIMATION
#if (defined(RK3368_BOX))// || defined(RK3288_BOX))
    ATRACE_CALL();
    if(Context == _contextAnchor)
        return 0;

    ZoneInfo mZoneInfo;
    ZoneManager* pzone_mag = &Context->zone_manager;
    int i = pzone_mag->zone_cnt-1;//Must be Sprite if has
    memcpy(&mZoneInfo,&pzone_mag->zone_info[i],sizeof(ZoneInfo));

    if(pzone_mag->zone_info[i].zone_err || pzone_mag->zone_info[i].transform)
        return -1;

    int mSize = 64;
    if(_contextAnchor->mHdmiSI.NeedReDst){
        mSize = 128;
        return -1;
    }

    if(!strcmp(pzone_mag->zone_info[i].LayerName,"Sprite") &&
        (pzone_mag->zone_info[i].toosmall || pzone_mag->zone_info[i].is_stretch)){
        mZoneInfo.toosmall = false;
        mZoneInfo.is_stretch = false;
    }else{
        return 0;
    }

    int width=0,height=0;
	int xpos,ypos;
	int x_offset,y_offset;
    RECT clip;
    int Rotation = 0;
    unsigned char RotateMode = 1;
    struct rga_req  Rga_Request;
    int SrcVirW,SrcVirH,SrcActW,SrcActH;
    int DstVirW,DstVirH,DstActW,DstActH;
    int xoffset;
    int yoffset;
    int fd_dst = Context->mSrBI.fd[Context->mSrBI.mCurIndex];
    int Dstfmt = hwChangeRgaFormat(HAL_PIXEL_FORMAT_RGBA_8888);
    int rga_fd = _contextAnchor->engine_fd;
    hwcContext * context = _contextAnchor;

    if (!rga_fd)
        return -1;

    DstActW = mZoneInfo.disp_rect.right  - mZoneInfo.disp_rect.left;
    DstActH = mZoneInfo.disp_rect.bottom - mZoneInfo.disp_rect.top;

    if(mSize < DstActW || mSize < DstActH)
        mSize = 128;

    if(mSize < DstActW || mSize < DstActH)
        return -1;

    DstVirW = mSize;
    DstVirH = mSize;

    if(Context==_contextAnchor){
        width  = Context->dpyAttr[0].xres;
        height = Context->dpyAttr[0].yres;
    }else if(Context==_contextAnchor1){
		width  = Context->dpyAttr[1].xres;
        height = Context->dpyAttr[1].yres;
	}
	
    struct private_handle_t *handle = mZoneInfo.handle;
    if (!handle)
        return -1;
        
    if(mZoneInfo.disp_rect.left <= width - mZoneInfo.disp_rect.right)
        xpos = mZoneInfo.disp_rect.left;
    else
		xpos = mZoneInfo.disp_rect.right - mSize;
	if(mZoneInfo.disp_rect.top <= height - mZoneInfo.disp_rect.bottom)
        ypos = mZoneInfo.disp_rect.top;
    else
		ypos = mZoneInfo.disp_rect.bottom - mSize;

	xoffset = mZoneInfo.disp_rect.left - xpos;
	yoffset = mZoneInfo.disp_rect.top  - ypos;
    
    x_offset = mZoneInfo.src_rect.left;
    y_offset = mZoneInfo.src_rect.top;
    
    SrcVirW = handle->stride;
    SrcVirH = handle->height;
    SrcActW = mZoneInfo.src_rect.right - mZoneInfo.src_rect.left;
    SrcActH = mZoneInfo.src_rect.bottom - mZoneInfo.src_rect.top;
    SrcActW = SrcActW<16?(SrcActW+SrcActW%2):(SrcActW);
    SrcActH = SrcActH<16?(SrcActH+SrcActH%2):(SrcActH);

    mZoneInfo.stride = (mSize + 31)&(~31);
    mZoneInfo.width = mSize;
    mZoneInfo.height = mSize;
    mZoneInfo.disp_rect.left   = xpos;
    mZoneInfo.disp_rect.right  = xpos + mSize;
    mZoneInfo.disp_rect.top    = ypos;
    mZoneInfo.disp_rect.bottom = ypos + mSize;
    mZoneInfo.src_rect.left    = 0;
    mZoneInfo.src_rect.right   = mSize;
    mZoneInfo.src_rect.top     = 0;
    mZoneInfo.src_rect.bottom  = mSize;
    mZoneInfo.layer_fd = Context->mSrBI.fd[Context->mSrBI.mCurIndex];

    if(SrcVirW<=0 || SrcVirH<=0 || SrcActW<=0 || SrcActH<=0)
        return -1;

    if(DstVirW<=0 || DstVirH<=0 || DstActW<=0 || DstActH<=0)
        return -1;
    memcpy(&pzone_mag->zone_info[i],&mZoneInfo,sizeof(ZoneInfo));
    memset((void*)(Context->mSrBI.hd_base[Context->mSrBI.mCurIndex]),0x0,mSize*mSize*4);
    ALOGD_IF(mLogL&HLLTWO,"Sprite Zone[%d]->layer[%d],"
        "[%d,%d,%d,%d] =>[%d,%d,%d,%d],"
        "w_h_s_f[%d,%d,%d,%d],tr_rtr_bled[%d,%d,%d],acq_fence_fd=%d,"
        "layname=%s",
        Context->zone_manager.zone_info[i].zone_index,
        Context->zone_manager.zone_info[i].layer_index,
        Context->zone_manager.zone_info[i].src_rect.left,
        Context->zone_manager.zone_info[i].src_rect.top,
        Context->zone_manager.zone_info[i].src_rect.right,
        Context->zone_manager.zone_info[i].src_rect.bottom,
        Context->zone_manager.zone_info[i].disp_rect.left,
        Context->zone_manager.zone_info[i].disp_rect.top,
        Context->zone_manager.zone_info[i].disp_rect.right,
        Context->zone_manager.zone_info[i].disp_rect.bottom,
        Context->zone_manager.zone_info[i].width,
        Context->zone_manager.zone_info[i].height,
        Context->zone_manager.zone_info[i].stride,
        Context->zone_manager.zone_info[i].format,
        Context->zone_manager.zone_info[i].transform,
        Context->zone_manager.zone_info[i].realtransform,
        Context->zone_manager.zone_info[i].blend,
        Context->zone_manager.zone_info[i].acq_fence_fd,
        Context->zone_manager.zone_info[i].LayerName);

    memset(&Rga_Request, 0x0, sizeof(Rga_Request));

    clip.xmin = 0;
    clip.xmax = mSize-1;
    clip.ymin = 0;
    clip.ymax = mSize-1;

    ALOGD_IF(mLogL&HLLTWO,"src addr=[%x],handle type=[%d],w-h[%d,%d],act[%d,%d],off[%d,%d][f=%d]",
        handle->share_fd,handle->type,SrcVirW, SrcVirH,SrcActW,SrcActH,x_offset,y_offset,hwChangeRgaFormat(handle->format));
    ALOGD_IF(mLogL&HLLTWO,"dst fd=[%x],w-h[%d,%d],act[%d,%d],off[%d,%d][f=%d],rot=%d,rot_mod=%d",
        fd_dst, DstVirW, DstVirH,DstActW,DstActH,xoffset,yoffset,Dstfmt,Rotation,RotateMode);

    RGA_set_src_vir_info(&Rga_Request, handle->share_fd, 0, 0,SrcVirW, SrcVirH, hwChangeRgaFormat(handle->format), 0);    
    RGA_set_dst_vir_info(&Rga_Request, fd_dst, 0, 0,DstVirW,DstVirH,&clip, Dstfmt, 0);
    RGA_set_bitblt_mode(&Rga_Request, 0, RotateMode,Rotation,0,0,0);
    RGA_set_src_act_info(&Rga_Request,SrcActW,SrcActH, x_offset,y_offset);
    RGA_set_dst_act_info(&Rga_Request,DstActW,DstActH, xoffset,yoffset);

    if( handle->type == 1 )
    {
#if defined(__arm64__) || defined(__aarch64__)
        RGA_set_dst_vir_info(&Rga_Request, fd_dst,(unsigned long)(GPU_BASE), 0,DstVirW,DstVirH,&clip, Dstfmt, 0);
#else
        RGA_set_dst_vir_info(&Rga_Request, fd_dst,(unsigned int)(GPU_BASE), 0,DstVirW,DstVirH,&clip, Dstfmt, 0);
#endif
        RGA_set_mmu_info(&Rga_Request, 1, 0, 0, 0, 0, 2);
        Rga_Request.mmu_info.mmu_flag |= (1<<31) | (1<<10) | (1<<8);
    }

    if(ioctl(rga_fd, RGA_BLIT_SYNC, &Rga_Request) != 0) {
        LOGE(" %s(%d) RGA_BLIT fail",__FUNCTION__, __LINE__);
    }
    
    Context->mSrBI.mCurIndex = (Context->mSrBI.mCurIndex + 1)%MaxSpriteBNUM;
    //gettimeofday(&te,NULL);
    //ALOGD("SPRITE USE TIME T = %ld",(te.tv_sec-ts.tv_sec)*1000000+te.tv_usec-ts.tv_usec);
#endif
    return 0;
#else
    return 0;
#endif
}


bool hotplug_free_dimbuffer()
{
#if OPTIMIZATION_FOR_DIMLAYER
    hwcContext * context = _contextAnchor;
	if(_contextAnchor1 && _contextAnchor1->mDimHandle){
	    buffer_handle_t mhandle = _contextAnchor1->mDimHandle;
        int err = context->mAllocDev->free(context->mAllocDev, mhandle);
        ALOGW_IF(err,"free mDimHandle failed %d (%s)", err, strerror(-err));
        _contextAnchor1->mDimHandle = 0;
	}
#endif
    return true;
}

int hwc_repet_last()
{
#if ONLY_USE_ONE_VOP
    int ret = 0;
    hwcContext * context = _contextAnchor;
#ifdef RK3288_BOX
    if(context->mLcdcNum == 2){
        return 0;
    }
#endif
    struct rk_fb_win_cfg_data fb_info;
    memcpy(&fb_info,&context->fb_info,sizeof(rk_fb_win_cfg_data));
    fb_info.ret_fence_fd = -1;
    for(int i=0;i<RK_MAX_BUF_NUM;i++) {
        fb_info.rel_fence_fd[i] = -1;
    }
    if(ioctl(context->fbFd, RK_FBIOSET_CONFIG_DONE, &fb_info) == -1){
        ALOGE("%s,%d,RK_FBIOSET_CONFIG_DONE fail",__FUNCTION__,__LINE__);
    }else{
        ALOGD_IF(mLogL&HLLONE,"hwc_repet_last");
    }
    dump_config_info(fb_info,context,4);
    for(int k=0;k<RK_MAX_BUF_NUM;k++){
        if(fb_info.rel_fence_fd[k] >=0 ){
            ret = 1;
            close(fb_info.rel_fence_fd[k]);
        }
    }
    if(fb_info.ret_fence_fd >= 0){
        ret = 1;
        close(fb_info.ret_fence_fd);
    }
    return ret;
#endif
    return 0;
}

bool hwcPrimaryToExternalCheckConfig(hwcContext * ctx,struct rk_fb_win_cfg_data fb_info)
{
    hwcContext * context = _contextAnchor;
#ifdef RK3288_BOX
    if(context->mLcdcNum == 2){
        return true;
    }
#endif
    if(ctx != _contextAnchor){
        return true;
    }

    int compostMode = ctx->zone_manager.composter_mode;
    if(ctx->mHdmiSI.mix_vh){
        return true;
    }else if(compostMode != HWC_LCDC){
        return false;
    }

    bool ret = true;
    bool isSameResolution = false;
    bool isLargeHdmi = context->mHdmiSI.NeedReDst;
    int widthPrimary  = context->dpyAttr[HWCP].xres;
    int heightPrimary = context->dpyAttr[HWCP].yres;
    int widthExternal  = context->dpyAttr[HWCE].xres;
    int heightExternal = context->dpyAttr[HWCE].yres;

    isSameResolution = (widthPrimary == widthExternal && heightPrimary == heightExternal);
    for(int i = 0;i<4;i++){
        for(int j=0;j<4;j++){
            if(fb_info.win_par[i].area_par[j].ion_fd || fb_info.win_par[i].area_par[j].phy_addr){
                int win_id = fb_info.win_par[i].win_id;
                if(win_id >= 2){
                    ret = ret && isSameResolution && !isLargeHdmi;
                }
            }
        }
        if(!ret){
            break;
        }
    }
    return ret;
}

static unsigned int createCrc32(unsigned int crc,unsigned const char *buffer, unsigned int size)  
{
	unsigned int i;
	for (i = 0; i < size; i++) {
		crc = crcTable[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
	}
	return crc ;
}

static void initCrcTable(void)  
{
	unsigned int c;
	unsigned int i, j;

	for (i = 0; i < 256; i++) {
		c = (unsigned int)i;
		for (j = 0; j < 8; j++) {
			if (c & 1) {
				c = 0xedb88320L ^ (c >> 1);
			} else {
			    c = c >> 1;
			}
		}
		crcTable[i] = c;
	}
}
