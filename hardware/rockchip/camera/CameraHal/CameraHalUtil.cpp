#include <stdio.h>
#include <stdlib.h> 
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <linux/videodev2.h> 
#include <binder/IPCThreadState.h>
#include "CameraHal.h"


#include <camera/CameraParameters.h>

#if defined(TARGET_RK30) && (defined(TARGET_BOARD_PLATFORM_RK30XX) || (defined(TARGET_BOARD_PLATFORM_RK2928)))
#include "../libgralloc/gralloc_priv.h"
#if (CONFIG_CAMERA_INVALIDATE_RGA==0)
#include <hardware/rga.h>
#endif
#elif defined(TARGET_RK30) && defined(TARGET_BOARD_PLATFORM_RK30XXB)
#include <hardware/hal_public.h>
#include <hardware/rga.h>
#elif defined(TARGET_RK3368)
#include <hardware/img_gralloc_public.h>
#include <hardware/rga.h>
#elif defined(TARGET_RK29)
#include "../libgralloc/gralloc_priv.h"
#endif

#include "../jpeghw/release/encode_release/rk29-ipp.h"
#include <utils/CallStack.h> 

#define MIN(x,y)   ((x<y) ? x: y)
#define MAX(x,y)    ((x>y) ? x: y)

#ifdef ALOGD
#define LOGD      ALOGD
#endif
#ifdef ALOGV
#define LOGV      ALOGV
#endif
#ifdef ALOGE
#define LOGE      ALOGE
#endif
#ifdef ALOGI
#define LOGI      ALOGI
#endif

#define CAMERA_DISPLAY_FORMAT_NV12       "nv12"


extern "C" int cameraFormatConvert(int v4l2_fmt_src, int v4l2_fmt_dst, const char *android_fmt_dst, 
						 char *srcbuf, char *dstbuf,long srcphy,long dstphy,int src_size,
						 int src_w, int src_h, int srcbuf_w,
						 int dst_w, int dst_h, int dstbuf_w,
						 bool mirror);


static char cameraCallProcess[30];
extern "C" int getCallingPid() {
    return android::IPCThreadState::self()->getCallingPid();
}


extern "C" void callStack(){
    android::CallStack stack;  
    stack.update();  
    stack.log("CameraHal");

}
extern "C" char* getCallingProcess()
{
    int fp = -1;
	cameraCallProcess[0] = 0x00; 
	sprintf(cameraCallProcess,"/proc/%d/cmdline",getCallingPid());

	fp = open(cameraCallProcess, O_RDONLY);

	if (fp < 0) {
		memset(cameraCallProcess,0x00,sizeof(cameraCallProcess));
		LOGE("Obtain calling process info failed");
	} 
	else {
		memset(cameraCallProcess,0x00,sizeof(cameraCallProcess));
		read(fp, cameraCallProcess, 29);
		close(fp);
		fp = -1;
		LOGD("Calling process is: %s",cameraCallProcess);
	}
    return cameraCallProcess;

}

extern "C" int cameraPixFmt2HalPixFmt(const char *fmt)
{
    int hal_pixel_format=HAL_PIXEL_FORMAT_YCrCb_NV12;
    
    if (strcmp(fmt,android::CameraParameters::PIXEL_FORMAT_RGB565) == 0) {
        hal_pixel_format = HAL_PIXEL_FORMAT_RGB_565;        
    } else if (strcmp(fmt,android::CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
        hal_pixel_format = HAL_PIXEL_FORMAT_YCrCb_NV12;
    }else if(strcmp(fmt,android::CameraParameters::PIXEL_FORMAT_YUV420P) == 0){
        hal_pixel_format = HAL_PIXEL_FORMAT_YV12;
    }else if (strcmp(fmt,CAMERA_DISPLAY_FORMAT_NV12) == 0) {
        hal_pixel_format = HAL_PIXEL_FORMAT_YCrCb_NV12;
    } else if (strcmp(fmt,android::CameraParameters::PIXEL_FORMAT_YUV422SP) == 0) {
        hal_pixel_format = HAL_PIXEL_FORMAT_YCbCr_422_SP;
    } else {
        hal_pixel_format = -EINVAL;
        LOGE("%s(%d): pixel format %s is unknow!",__FUNCTION__,__LINE__,fmt);        
    }

    return hal_pixel_format;
}
extern "C" void arm__scale_crop_nv12torgb565(int srcW, int srcH,int dstW,int dstH, char *srcbuf, short int *dstbuf)
{
#if 0
    int line, col;
    int uv_src_w = srcW/2;
    int uv_src_h = srcH/2;
    int src_w = srcW,src_h = srcH;
    int dst_w = dstW,dst_h = dstH;
    int ratio,cropW,cropH,left_offset,top_offset;
    char* src,*psY,*psUV,*dst,*pdUV,*pdY;
	long zoomindstxIntInv,zoomindstyIntInv;
	long sX,sY,sY_UV;
    
	long yCoeff00_y,yCoeff01_y,xCoeff00_y,xCoeff01_y;
	long yCoeff00_uv,yCoeff01_uv,xCoeff00_uv,xCoeff01_uv;
	long r0,r1,a,b,c,d,ry,ru,rv;

    int y, u, v, yy, vr, ug, vg, ub,r,g,b1;

    

	src = psY = (unsigned char*)(srcbuf)+top_offset*src_w+left_offset;
	//psUV = psY +src_w*src_h+top_offset*src_w/2+left_offset;
	psUV = (unsigned char*)(srcbuf) +src_w*src_h+top_offset*src_w/2+left_offset;

	

	
	dst = pdY = (unsigned char*)dstbuf; 
	pdUV = pdY + dst_w*dst_h;

    	//need crop ?
	if((src_w*100/src_h) != (dst_w*100/dst_h)){
		ratio = ((src_w*100/dst_w) >= (src_h*100/dst_h))?(src_h*100/dst_h):(src_w*100/dst_w);
		cropW = ratio*dst_w/100;
		cropH = ratio*dst_h/100;
		
		left_offset=((src_w-cropW)>>1) & (~0x01);
		top_offset=((src_h-cropH)>>1) & (~0x01);
	}else{
		cropW = src_w;
		cropH = src_h;
		top_offset=0;
		left_offset=0;
	}

	zoomindstxIntInv = ((unsigned long)(cropW)<<16)/dstW + 1;
	zoomindstyIntInv = ((unsigned long)(cropH)<<16)/dstH + 1;

    for (line = 0; line < dstH; line++) {
		yCoeff00_y = (line*zoomindstyIntInv)&0xffff;
		yCoeff01_y = 0xffff - yCoeff00_y; 
		sY = (line*zoomindstyIntInv >> 16);
		sY = (sY >= srcH - 1)? (srcH - 2) : sY; 
        
        sY_UV = = ((line/2)*zoomindstyIntInv >> 16);
	    sY_UV = (sY_UV >= uv_src_h - 1)? (uv_src_h - 2) : sY_UV; 
        for (col = 0; col < dstW; col++) {


            //get y
			xCoeff00_y = (col*zoomindstxIntInv)&0xffff;
			xCoeff01_y = 0xffff - xCoeff00_y;	
			sX = (col*zoomindstxIntInv >> 16);
			sX = (sX >= srcW -1)?(srcW- 2) : sX;
			a = psY[sY*srcW + sX];
			b = psY[sY*srcW + sX + 1];
			c = psY[(sY+1)*srcW + sX];
			d = psY[(sY+1)*srcW + sX + 1];

			r0 = (a * xCoeff01_y + b * xCoeff00_y)>>16 ;
			r1 = (c * xCoeff01_y + d * xCoeff00_y)>>16 ;
			ry = (r0 * yCoeff01_y + r1 * yCoeff00_y)>>16;

            //get u & v
			xCoeff00_uv = ((col/2)*zoomindstxIntInv)&0xffff;
			xCoeff01_uv = 0xffff - xCoeff00_uv;	
			sX = ((col/2)*zoomindstxIntInv >> 16);
			sX = (sX >= uv_src_w -1)?(uv_src_w- 2) : sX;
			//U
			a = psUV[(sY*uv_src_w + sX)*2];
			b = psUV[(sY*uv_src_w + sX + 1)*2];
			c = psUV[((sY+1)*uv_src_w + sX)*2];
			d = psUV[((sY+1)*uv_src_w + sX + 1)*2];

			r0 = (a * xCoeff01_uv + b * xCoeff00_uv)>>16 ;
			r1 = (c * xCoeff01_uv + d * xCoeff00_uv)>>16 ;
			ru = (r0 * yCoeff01_uv + r1 * yCoeff00_uv)>>16;

            //v

			a = psUV[(sY*uv_src_w + sX)*2 + 1];
			b = psUV[(sY*uv_src_w + sX + 1)*2 + 1];
			c = psUV[((sY+1)*uv_src_w + sX)*2 + 1];
			d = psUV[((sY+1)*uv_src_w + sX + 1)*2 + 1];

			r0 = (a * xCoeff01_uv + b * xCoeff00_uv)>>16 ;
			r1 = (c * xCoeff01_uv + d * xCoeff00_uv)>>16 ;
			rv = (r0 * yCoeff01_uv + r1 * yCoeff00_uv)>>16;



            yy = ry << 8;
            u =  ru - 128;
            ug = 88 * u;
            ub = 454 * u;
            v =  rv - 128;
            vg = 183 * v;
            vr = 359 * v;

            r = (yy +      vr) >> 8;
            g = (yy - ug - vg) >> 8;
            b = (yy + ub     ) >> 8;
            if (r < 0)   r = 0;
            if (r > 255) r = 255;
            if (g < 0)   g = 0;
            if (g > 255) g = 255;
            if (b < 0)   b = 0;
            if (b > 255) b = 255;

            
        }
    }
#endif
}


extern "C" void arm_nv12torgb565(int width, int height, char *src, short int *dst,int dstbuf_w)
{
    int line, col;
    int y, u, v, yy, vr, ug, vg, ub;
    int r, g, b;
    char *py, *pu, *pv;    

    py = src;
    pu = py + (width * height);
    pv = pu + 1;
    y = *py++;
    yy = y << 8;
    u = *pu - 128;
    ug = 88 * u;
    ub = 454 * u;
    v = *pv - 128;
    vg = 183 * v;
    vr = 359 * v;
    
    for (line = 0; line < height; line++) {
        for (col = 0; col < width; col++) {
            r = (yy +      vr) >> 8;
            g = (yy - ug - vg) >> 8;
            b = (yy + ub     ) >> 8;
            if (r < 0)   r = 0;
            if (r > 255) r = 255;
            if (g < 0)   g = 0;
            if (g > 255) g = 255;
            if (b < 0)   b = 0;
            if (b > 255) b = 255;
            
            *dst++ = (((__u16)r>>3)<<11) | (((__u16)g>>2)<<5) | (((__u16)b>>3)<<0);

            y = *py++;
            yy = y << 8;
            if (col & 1) {
                pu += 2;
                pv = pu+1;
                u = *pu - 128;
                ug =   88 * u;
                ub = 454 * u;
                v = *pv - 128;
                vg = 183 * v;
                vr = 359 * v;
            }
        }
        dst += dstbuf_w - width;
        if ((line & 1) == 0) { 
            //even line: rewind
            pu -= width;
            pv = pu+1;
        }
    }
}




extern "C" int rga_nv12torgb565(int src_width, int src_height, char *src, short int *dst, int dstbuf_width,int dst_width,int dst_height)
{
    int rgafd = -1,ret = -1;

    //LOGD("rga_nv12torgb565: (%dx%d)->(%dx%d),src_buf = 0x%x,dst_buf = 0x%x",src_width,src_height,dst_width,dst_height,src,dst);

#ifdef TARGET_RK30

    if((rgafd = open("/dev/rga",O_RDWR)) < 0) {
    	LOGE("%s(%d):open rga device failed!!",__FUNCTION__,__LINE__);
        ret = -1;
    	return ret;
	}

#if (CONFIG_CAMERA_INVALIDATE_RGA==0)
    struct rga_req  Rga_Request;
    int err = 0;
    
    memset(&Rga_Request,0x0,sizeof(Rga_Request));

	unsigned char *psY, *psUV;
	int srcW,srcH,cropW,cropH;
	int ratio = 0;
	int top_offset=0,left_offset=0;
//need crop ?
	if((src_width*100/src_height) != (dst_width*100/dst_height)){
		ratio = ((src_width*100/dst_width) >= (src_height*100/dst_height))?(src_height*100/dst_height):(src_width*100/dst_width);
		cropW = ratio*dst_width/100;
		cropH = ratio*dst_height/100;
		
		left_offset=((src_width-cropW)>>1) & (~0x01);
		top_offset=((src_height-cropH)>>1) & (~0x01);
	}else{
		cropW = src_width;
		cropH = src_height;
		top_offset=0;
		left_offset=0;
	}

	psY = (unsigned char*)(src)+top_offset*src_width+left_offset;
	psUV = (unsigned char*)(src) +src_width*src_height+top_offset*src_width/2+left_offset;
	
	Rga_Request.src.yrgb_addr =  (long)psY;
    Rga_Request.src.uv_addr  = (long)psUV;
    Rga_Request.src.v_addr   =  Rga_Request.src.uv_addr;
    Rga_Request.src.vir_w =  src_width;
    Rga_Request.src.vir_h = src_height;
    Rga_Request.src.format = RK_FORMAT_YCbCr_420_SP;
    Rga_Request.src.act_w = cropW;
    Rga_Request.src.act_h = cropH;
    Rga_Request.src.x_offset = 0;
    Rga_Request.src.y_offset = 0;

    Rga_Request.dst.yrgb_addr = (long)dst;
    Rga_Request.dst.uv_addr  = 0;
    Rga_Request.dst.v_addr   = 0;
    Rga_Request.dst.vir_w = dstbuf_width;
    Rga_Request.dst.vir_h = dst_height;
    Rga_Request.dst.format = RK_FORMAT_RGB_565;
    Rga_Request.clip.xmin = 0;
    Rga_Request.clip.xmax = dst_width - 1;
    Rga_Request.clip.ymin = 0;
    Rga_Request.clip.ymax = dst_height - 1;
    Rga_Request.dst.act_w = dst_width;
    Rga_Request.dst.act_h = dst_height;
    Rga_Request.dst.x_offset = 0;
    Rga_Request.dst.y_offset = 0;
    Rga_Request.mmu_info.mmu_en    = 1;
    Rga_Request.mmu_info.mmu_flag  = ((2 & 0x3) << 4) | 1;
    Rga_Request.alpha_rop_flag |= (1 << 5);             /* ddl@rock-chips.com: v0.4.3 */

	if((src_width!=dst_width) || ( src_height!=dst_height)){
		Rga_Request.sina = 0;
		Rga_Request.cosa = 0x10000;
		Rga_Request.rotate_mode = 1;
		Rga_Request.scale_mode = 1;
	}else{
		Rga_Request.sina = 0;
		Rga_Request.cosa = 0;
		Rga_Request.rotate_mode = 0;
		Rga_Request.scale_mode = 0;
	}

    if(ioctl(rgafd, RGA_BLIT_SYNC, &Rga_Request) != 0) {
        LOGE("%s(%d):  RGA_BLIT_ASYNC Failed", __FUNCTION__, __LINE__);
        err = -1;
    }    
    return err;
#else
    LOGE("%s(%d): RGA is invalidate!",__FUNCTION__, __LINE__);
    return 0;
#endif
#else
    LOGE("%s(%d): rk29 havn't RGA device in chips!!",__FUNCTION__, __LINE__);
    return -1;
#endif
}

extern "C"  int arm_camera_yuv420_scale_arm(int v4l2_fmt_src, int v4l2_fmt_dst, 
									char *srcbuf, char *dstbuf,int src_w, int src_h,int dst_w, int dst_h,bool mirror,int zoom_val);

extern "C" int rga_nv12_scale_crop(int src_width, int src_height, char *src, short int *dst, 
										int dst_width,int dst_height,int zoom_val,bool mirror,bool isNeedCrop,bool isDstNV21)
{
    int rgafd = -1,ret = -1;
	int scale_times_w = 0,scale_times_h = 0,h = 0,w = 0;
	struct rga_req	Rga_Request;
	int err = 0;
	unsigned char *psY, *psUV;
	int src_cropW,src_cropH,dst_cropW,dst_cropH,zoom_cropW,zoom_cropH;
	int ratio = 0;
	int src_top_offset=0,src_left_offset=0,dst_top_offset=0,dst_left_offset=0,zoom_top_offset=0,zoom_left_offset=0;

	/*has something wrong with rga of rk312x mirror operation*/
	#if defined(TARGET_RK312x)
		if(mirror){
			return arm_camera_yuv420_scale_arm(V4L2_PIX_FMT_NV12, (isDstNV21 ? V4L2_PIX_FMT_NV21:V4L2_PIX_FMT_NV12), 
				src, (char *)dst,src_width, src_height,dst_width, dst_height,true,zoom_val);
		}
	#endif 
	/*rk3188 do not support yuv to yuv scale by rga*/
	#if defined(TARGET_RK3188)
		return arm_camera_yuv420_scale_arm(V4L2_PIX_FMT_NV12, (isDstNV21 ? V4L2_PIX_FMT_NV21:V4L2_PIX_FMT_NV12), 
			src, (char *)dst,src_width, src_height,dst_width, dst_height,true,zoom_val);
	#endif 
	
	if((dst_width > RGA_VIRTUAL_W) || (dst_height > RGA_VIRTUAL_H)){
		LOGE("%s(%d):(dst_width > RGA_VIRTUAL_W) || (dst_height > RGA_VIRTUAL_H), switch to arm ",__FUNCTION__,__LINE__);
		
		return arm_camera_yuv420_scale_arm(V4L2_PIX_FMT_NV12, (isDstNV21 ? V4L2_PIX_FMT_NV21:V4L2_PIX_FMT_NV12), 
			src, (char *)dst,src_width, src_height,dst_width, dst_height,mirror,zoom_val);
	}

	//need crop ? when cts FOV,don't crop
	if(isNeedCrop && (src_width*100/src_height) != (dst_width*100/dst_height)){
		ratio = ((src_width*100/dst_width) >= (src_height*100/dst_height))?(src_height*100/dst_height):(src_width*100/dst_width);
		zoom_cropW = ratio*dst_width/100;
		zoom_cropH = ratio*dst_height/100;
		
		zoom_left_offset=((src_width-zoom_cropW)>>1) & (~0x01);
		zoom_top_offset=((src_height-zoom_cropH)>>1) & (~0x01);
	}else{
		zoom_cropW = src_width;
		zoom_cropH = src_height;
		zoom_left_offset=0;
		zoom_top_offset=0;
	}

	if(zoom_val > 100){
		zoom_cropW = zoom_cropW*100/zoom_val;
		zoom_cropH = zoom_cropH*100/zoom_val;
		zoom_left_offset = ((src_width-zoom_cropW)>>1) & (~0x01);
		zoom_top_offset= ((src_height-zoom_cropH)>>1) & (~0x01);
	}

		
    if(dst_width > RGA_ACTIVE_W){
	        scale_times_w = (dst_width/RGA_ACTIVE_W);
			scale_times_w++;
    }else{
        scale_times_w = 1;
	}
	if(dst_height > RGA_ACTIVE_H){
		scale_times_h = (dst_height/RGA_ACTIVE_H);   
		scale_times_h++;
    } else {
		scale_times_h = 1;
    }
    if((rgafd = open("/dev/rga",O_RDWR)) < 0) {
    	LOGE("%s(%d):open rga device failed!!",__FUNCTION__,__LINE__);
        ret = -1;
    	return ret;
	}
	
	src_cropW = zoom_cropW/scale_times_w;
	src_cropH = zoom_cropH/scale_times_h;
	
	dst_cropW = dst_width/scale_times_w;
	dst_cropH = dst_height/scale_times_h;
	
	for(h = 0; h< scale_times_h; h++){
		for(w = 0; w< scale_times_w; w++){
		    memset(&Rga_Request,0x0,sizeof(Rga_Request));

			src_left_offset = zoom_left_offset + w*src_cropW;
			src_top_offset  = zoom_top_offset  + h*src_cropH;

			dst_left_offset = w*dst_cropW;
			dst_top_offset  = h*dst_cropH;

			psY = (unsigned char*)(src);

		#if defined(TARGET_RK3188)
			Rga_Request.src.yrgb_addr =  (long)psY;
		    Rga_Request.src.uv_addr  = (long)psY + src_width * src_height;
		#else
			Rga_Request.src.yrgb_addr =  0;
            Rga_Request.src.uv_addr  = (long)psY;
		#endif
		    Rga_Request.src.v_addr   =  0;
		    Rga_Request.src.vir_w =  src_width;
		    Rga_Request.src.vir_h = src_height;
		    Rga_Request.src.format = RK_FORMAT_YCbCr_420_SP;
		    Rga_Request.src.act_w = src_cropW & (~0x01);
		    Rga_Request.src.act_h = src_cropH & (~0x01);
#if defined(TARGET_RK3368)
		    Rga_Request.src.x_offset = src_left_offset & (~0x1f);//32 alignment,rga's bug
		    Rga_Request.src.y_offset = src_top_offset & (~0xf);
#else
		    Rga_Request.src.x_offset = src_left_offset & (~0x01);
		    Rga_Request.src.y_offset = src_top_offset & (~0x01);
#endif
		#if defined(TARGET_RK3188)
		    Rga_Request.dst.yrgb_addr = (long)dst;
		    Rga_Request.dst.uv_addr  = (long)dst + dst_width*dst_height;
		#else
			Rga_Request.dst.yrgb_addr = 0;
            Rga_Request.dst.uv_addr  = (long)dst;
		#endif
		    Rga_Request.dst.v_addr   = 0;
		    Rga_Request.dst.vir_w = dst_width;
		    Rga_Request.dst.vir_h = dst_height;
		    if(isDstNV21) 
		        Rga_Request.dst.format = RK_FORMAT_YCrCb_420_SP;
		    else 
		        Rga_Request.dst.format = RK_FORMAT_YCbCr_420_SP;
		    Rga_Request.clip.xmin = 0;
		    Rga_Request.clip.xmax = dst_width - 1;
		    Rga_Request.clip.ymin = 0;
		    Rga_Request.clip.ymax = dst_height - 1;
		    Rga_Request.dst.act_w = dst_cropW;
		    Rga_Request.dst.act_h = dst_cropH;
		    Rga_Request.dst.x_offset = dst_left_offset;
		    Rga_Request.dst.y_offset = dst_top_offset;    

		    Rga_Request.mmu_info.mmu_en    = 1;
		    Rga_Request.mmu_info.mmu_flag  = ((2 & 0x3) << 4) | 1 | (1 << 8) | (1 << 10);
		    Rga_Request.alpha_rop_flag |= (1 << 5);             /* ddl@rock-chips.com: v0.4.3 */
		    
			#if defined(TARGET_RK312x)
				/* wrong operation of nv12 to nv21 ,not scale */
				if(1/*(cropW != dst_width) || ( cropH != dst_height)*/){
			#else
				if((src_cropW != dst_width) || ( src_cropH != dst_height)){
			#endif
				Rga_Request.sina = 0;
				Rga_Request.cosa = 0x10000;
				Rga_Request.scale_mode = 1;
		    	Rga_Request.rotate_mode = mirror ? 2:1;
			}else{
				Rga_Request.sina = 0;
				Rga_Request.cosa =  0;
				Rga_Request.scale_mode = 0;
		    	Rga_Request.rotate_mode = mirror ? 2:0;
				Rga_Request.render_mode = pre_scaling_mode;
			}
		    

		    if(ioctl(rgafd, RGA_BLIT_SYNC, &Rga_Request) != 0) {
		        LOGE("%s(%d):  RGA_BLIT_ASYNC Failed", __FUNCTION__, __LINE__);
		        err = -1;
	    	}
		}
	}
	close(rgafd);
	
    return err;
}

extern "C"  int arm_camera_yuv420_scale_arm(int v4l2_fmt_src, int v4l2_fmt_dst, 
									char *srcbuf, char *dstbuf,int src_w, int src_h,int dst_w, int dst_h,bool mirror,int zoom_val)
{
	unsigned char *psY,*pdY,*psUV,*pdUV; 
	unsigned char *src,*dst;
	int srcW,srcH,cropW,cropH,dstW,dstH;
	long zoomindstxIntInv,zoomindstyIntInv;
	long x,y;
	long yCoeff00,yCoeff01,xCoeff00,xCoeff01;
	long sX,sY;
	long r0,r1,a,b,c,d;
	int ret = 0;
	bool nv21DstFmt = false;
	int ratio = 0;
	int top_offset=0,left_offset=0;
	if((v4l2_fmt_src != V4L2_PIX_FMT_NV12) ||
		((v4l2_fmt_dst != V4L2_PIX_FMT_NV12) && (v4l2_fmt_dst != V4L2_PIX_FMT_NV21) )){
		LOGE("%s:%d,not suppport this format ",__FUNCTION__,__LINE__);
		return -1;
	}

    //just copy ?
    if((v4l2_fmt_src == v4l2_fmt_dst) && (mirror == false)
        &&(src_w == dst_w) && (src_h == dst_h) && (zoom_val == 100)){
        memcpy(dstbuf,srcbuf,src_w*src_h*3/2);
        return 0;
    }else if((v4l2_fmt_dst == V4L2_PIX_FMT_NV21) 
            && (src_w == dst_w) && (src_h == dst_h) 
            && (mirror == false) && (zoom_val == 100)){
    //just convert fmt

        cameraFormatConvert(V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV21, NULL, 
    					    srcbuf, dstbuf,0,0,src_w*src_h*3/2,
    					    src_w, src_h,src_w,
    					    dst_w, dst_h,dst_w,
    						mirror);
        return 0;

    }

	if ((v4l2_fmt_dst == V4L2_PIX_FMT_NV21)){
		nv21DstFmt = true;
		
	}

	//need crop ?
	if((src_w*100/src_h) != (dst_w*100/dst_h)){
		ratio = ((src_w*100/dst_w) >= (src_h*100/dst_h))?(src_h*100/dst_h):(src_w*100/dst_w);
		cropW = ratio*dst_w/100;
		cropH = ratio*dst_h/100;
		
		left_offset=((src_w-cropW)>>1) & (~0x01);
		top_offset=((src_h-cropH)>>1) & (~0x01);
	}else{
		cropW = src_w;
		cropH = src_h;
		top_offset=0;
		left_offset=0;
	}

    //zoom ?
    if(zoom_val > 100){
        cropW = cropW*100/zoom_val;
        cropH = cropH*100/zoom_val;
		left_offset=((src_w-cropW)>>1) & (~0x01);
		top_offset=((src_h-cropH)>>1) & (~0x01);
    }

	src = psY = (unsigned char*)(srcbuf)+top_offset*src_w+left_offset;
	//psUV = psY +src_w*src_h+top_offset*src_w/2+left_offset;
	psUV = (unsigned char*)(srcbuf) +src_w*src_h+top_offset*src_w/2+left_offset;

	
	srcW =src_w;
	srcH = src_h;
//	cropW = src_w;
//	cropH = src_h;

	
	dst = pdY = (unsigned char*)dstbuf; 
	pdUV = pdY + dst_w*dst_h;
	dstW = dst_w;
	dstH = dst_h;

	zoomindstxIntInv = ((unsigned long)(cropW)<<16)/dstW + 1;
	zoomindstyIntInv = ((unsigned long)(cropH)<<16)/dstH + 1;
	//y
	//for(y = 0; y<dstH - 1 ; y++ ) {	
	for(y = 0; y<dstH; y++ ) {	 
		yCoeff00 = (y*zoomindstyIntInv)&0xffff;
		yCoeff01 = 0xffff - yCoeff00; 
		sY = (y*zoomindstyIntInv >> 16);
		sY = (sY >= srcH - 1)? (srcH - 2) : sY; 	 
		for(x = 0; x<dstW; x++ ) {
			xCoeff00 = (x*zoomindstxIntInv)&0xffff;
			xCoeff01 = 0xffff - xCoeff00;	
			sX = (x*zoomindstxIntInv >> 16);
			sX = (sX >= srcW -1)?(srcW- 2) : sX;
			a = psY[sY*srcW + sX];
			b = psY[sY*srcW + sX + 1];
			c = psY[(sY+1)*srcW + sX];
			d = psY[(sY+1)*srcW + sX + 1];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;
			
			if(mirror)
				pdY[dstW -1 - x] = r0;
			else
				pdY[x] = r0;
		}
		pdY += dstW;
	}

	dstW /= 2;
	dstH /= 2;
	srcW /= 2;
	srcH /= 2;

	//UV
	//for(y = 0; y<dstH - 1 ; y++ ) {
	for(y = 0; y<dstH; y++ ) {
		yCoeff00 = (y*zoomindstyIntInv)&0xffff;
		yCoeff01 = 0xffff - yCoeff00; 
		sY = (y*zoomindstyIntInv >> 16);
		sY = (sY >= srcH -1)? (srcH - 2) : sY;		
		for(x = 0; x<dstW; x++ ) {
			xCoeff00 = (x*zoomindstxIntInv)&0xffff;
			xCoeff01 = 0xffff - xCoeff00;	
			sX = (x*zoomindstxIntInv >> 16);
			sX = (sX >= srcW -1)?(srcW- 2) : sX;
			//U
			a = psUV[(sY*srcW + sX)*2];
			b = psUV[(sY*srcW + sX + 1)*2];
			c = psUV[((sY+1)*srcW + sX)*2];
			d = psUV[((sY+1)*srcW + sX + 1)*2];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;
		
			if(mirror && nv21DstFmt)
				pdUV[dstW*2-1- (x*2)] = r0;
			else if(mirror)
				pdUV[dstW*2-1-(x*2+1)] = r0;
			else if(nv21DstFmt)
				pdUV[x*2 + 1] = r0;
			else
				pdUV[x*2] = r0;
			//V
			a = psUV[(sY*srcW + sX)*2 + 1];
			b = psUV[(sY*srcW + sX + 1)*2 + 1];
			c = psUV[((sY+1)*srcW + sX)*2 + 1];
			d = psUV[((sY+1)*srcW + sX + 1)*2 + 1];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;

			if(mirror && nv21DstFmt)
				pdUV[dstW*2-1- (x*2+1) ] = r0;
			else if(mirror)
				pdUV[dstW*2-1-(x*2)] = r0;
			else if(nv21DstFmt)
				pdUV[x*2] = r0;
			else
				pdUV[x*2 + 1] = r0;
		}
		pdUV += dstW*2;
	}
	return 0;
}	

extern "C" int rk_camera_zoom_ipp(int v4l2_fmt_src, int srcbuf, int src_w, int src_h,int dstbuf,int zoom_value)
{
	int vipdata_base;

	struct rk29_ipp_req ipp_req;
	int src_y_offset,src_uv_offset,dst_y_offset,dst_uv_offset,src_y_size,dst_y_size;
	int scale_w_times =0,scale_h_times = 0,w,h;
	int ret = 0;
    int ippFd = -1;
    int ratio = 0;
    int top_offset=0,left_offset=0;
    int cropW,cropH;


    if((ippFd = open("/dev/rk29-ipp",O_RDWR)) < 0) {
    	LOGE("%s(%d):open rga device failed!!",__FUNCTION__,__LINE__);
        ret = -1;
    	goto do_ipp_err;
	}

    /*
    *ddl@rock-chips.com: 
    * IPP Dest image resolution is 2047x1088, so scale operation break up some times
    */
    if ((src_w > 0x7f0) || (src_h > 0x430)) {
        scale_w_times = ((src_w/0x7f0)>(src_h/0x430))?(src_w/0x7f0):(src_h/0x430); 
        scale_h_times = scale_w_times;
        scale_w_times++;
        scale_h_times++;
    } else {
        scale_w_times = 1;
        scale_h_times = 1;
    }
    memset(&ipp_req, 0, sizeof(struct rk29_ipp_req));


    //compute zoom 
	cropW = (src_w*100/zoom_value)& (~0x03);
	cropH = (src_h*100/zoom_value)& (~0x03);
	left_offset=MAX((((src_w-cropW)>>1)-1),0);
	top_offset=MAX((((src_h-cropH)>>1)-1),0);
    left_offset &= ~0x01; 
    top_offset &=~0x01;

    ipp_req.timeout = 3000;
    ipp_req.flag = IPP_ROT_0; 
    ipp_req.store_clip_mode =1;
    ipp_req.src0.w = cropW/scale_w_times;
    ipp_req.src0.h = cropH/scale_h_times;
    ipp_req.src_vir_w = src_w;
    ipp_req.src0.fmt = IPP_Y_CBCR_H2V2;
    ipp_req.dst0.w = src_w/scale_w_times;
    ipp_req.dst0.h = src_h/scale_h_times;
    ipp_req.dst_vir_w = src_w;   
    ipp_req.dst0.fmt = IPP_Y_CBCR_H2V2;
    vipdata_base = srcbuf;
    src_y_size = src_w*src_h;
    dst_y_size = src_w*src_h;

    for (h=0; h<scale_h_times; h++) {
        for (w=0; w<scale_w_times; w++) {
            int ipp_times = 3;
            src_y_offset = (top_offset + h*cropH/scale_h_times)* src_w 
                        + left_offset + w*cropW/scale_w_times;
		    src_uv_offset = (top_offset + h*cropH/scale_h_times)* src_w/2
                        + left_offset + w*cropW/scale_w_times;

            dst_y_offset = src_w*src_h*h/scale_h_times + src_w*w/scale_w_times;
            dst_uv_offset = src_w*src_h*h/scale_h_times/2 + src_w*w/scale_w_times;

    		ipp_req.src0.YrgbMst = vipdata_base + src_y_offset;
    		ipp_req.src0.CbrMst = vipdata_base + src_y_size + src_uv_offset;
    		ipp_req.dst0.YrgbMst = dstbuf + dst_y_offset;
    		ipp_req.dst0.CbrMst = dstbuf + dst_y_size + dst_uv_offset;
    		while(ipp_times-- > 0) {
                if (ioctl(ippFd,IPP_BLIT_SYNC,&ipp_req)){
                    LOGE("ipp do erro,do again,ipp_times = %d!\n",ipp_times);
                 } else {
                    break;
                 }
            }
            if (ipp_times <= 0) {
                ret = -1;
    			goto do_ipp_err;
    		}
        }
    }

do_ipp_err:
    if(ippFd > 0)
       close(ippFd);
    
	return ret;    
}

extern "C" int rk_camera_yuv_scale_crop_ipp(int v4l2_fmt_src, int v4l2_fmt_dst, 
			long srcbuf, long dstbuf,int src_w, int src_h,int dst_w, int dst_h,bool rotation_180)
{
	long vipdata_base;

	struct rk29_ipp_req ipp_req;
	int src_y_offset,src_uv_offset,dst_y_offset,dst_uv_offset,src_y_size,dst_y_size;
	int scale_w_times =0,scale_h_times = 0,w,h;
	int ret = 0;
    int ippFd = -1;
    int ratio = 0;
    int top_offset=0,left_offset=0;
    int cropW,cropH;


    if((ippFd = open("/dev/rk29-ipp",O_RDWR)) < 0) {
    	LOGE("%s(%d):open /dev/rk29-ipp device failed!!",__FUNCTION__,__LINE__);
        ret = -1;
    	goto do_ipp_err;
	}


    /*
    *ddl@rock-chips.com: 
    * IPP Dest image resolution is 2047x1088, so scale operation break up some times
    */
    if ((dst_w > 0x7f0) || (dst_h > 0x430)) {
        scale_w_times = ((dst_w/0x7f0)>(dst_h/0x430))?(dst_w/0x7f0):(dst_h/0x430); 
        scale_h_times = scale_w_times;
        scale_w_times++;
        scale_h_times++;
    } else {
        scale_w_times = 1;
        scale_h_times = 1;
    }
    memset(&ipp_req, 0, sizeof(struct rk29_ipp_req));

    	//need crop ?
	if((src_w*100/src_h) != (dst_w*100/dst_h)){
		ratio = ((src_w*100/dst_w) >= (src_h*100/dst_h))?(src_h*100/dst_h):(src_w*100/dst_w);
		cropW = (ratio*dst_w/100)& (~0x03);
		cropH = (ratio*dst_h/100)& (~0x03);
		
		left_offset=MAX((((src_w-cropW)>>1)-1),0);
		top_offset=MAX((((src_h-cropH)>>1)-1),0);
        left_offset &= ~0x01; 
        top_offset &=~0x01;
	}else{
		cropW = src_w;
		cropH = src_h;
		top_offset=0;
		left_offset=0;
	}
#if 1
    if((src_w == 2592) && (src_h == 1944) && (dst_w == 2592) && (dst_h == 1458)){
        scale_w_times= 2;
        scale_h_times = 3;
		cropW = dst_w;
		cropH = dst_h;
		
		left_offset=0;
		top_offset=242;
    }
#endif
    ipp_req.timeout = 3000;
    if(rotation_180)
        ipp_req.flag = IPP_ROT_180; 
    else
        ipp_req.flag = IPP_ROT_0; 
    ipp_req.store_clip_mode =1;
    ipp_req.src0.w = cropW/scale_w_times;
    ipp_req.src0.h = cropH/scale_h_times;
    ipp_req.src_vir_w = src_w;
    if(v4l2_fmt_src == V4L2_PIX_FMT_NV12)
        ipp_req.src0.fmt = IPP_Y_CBCR_H2V2;
    else if(v4l2_fmt_src == V4L2_PIX_FMT_NV21)
        ipp_req.src0.fmt = IPP_Y_CBCR_H2V1;
    ipp_req.dst0.w = dst_w/scale_w_times;
    ipp_req.dst0.h = dst_h/scale_h_times;
    ipp_req.dst_vir_w = dst_w;   
    if(v4l2_fmt_dst == V4L2_PIX_FMT_NV12)
        ipp_req.dst0.fmt = IPP_Y_CBCR_H2V2;
    else if(v4l2_fmt_dst == V4L2_PIX_FMT_NV21)
        ipp_req.dst0.fmt = IPP_Y_CBCR_H2V1;
    vipdata_base = srcbuf;
    src_y_size = src_w*src_h;
    dst_y_size = dst_w*dst_h;
    for (h=0; h<scale_h_times; h++) {
        for (w=0; w<scale_w_times; w++) {
            int ipp_times = 3;
            src_y_offset = (top_offset + h*cropH/scale_h_times)* src_w 
                        + left_offset + w*cropW/scale_w_times;
		    src_uv_offset = (top_offset + h*cropH/scale_h_times)* src_w/2
                        + left_offset + w*cropW/scale_w_times;

            if(rotation_180){
                dst_y_offset = dst_w*dst_h*(scale_h_times-1-h)/scale_h_times + dst_w*(scale_w_times-1-w)/scale_w_times;
                dst_uv_offset = dst_w*dst_h*(scale_h_times-1-h)/scale_h_times/2 + dst_w*(scale_w_times-1-w)/scale_w_times;
            }
            else{
                dst_y_offset = dst_w*dst_h*h/scale_h_times + dst_w*w/scale_w_times;
                dst_uv_offset = dst_w*dst_h*h/scale_h_times/2 + dst_w*w/scale_w_times;
            }

    		ipp_req.src0.YrgbMst = vipdata_base + src_y_offset;
    		ipp_req.src0.CbrMst = vipdata_base + src_y_size + src_uv_offset;
    		ipp_req.dst0.YrgbMst = dstbuf + dst_y_offset;
    		ipp_req.dst0.CbrMst = dstbuf + dst_y_size + dst_uv_offset;
    		while(ipp_times-- > 0) {
                if (ioctl(ippFd,IPP_BLIT_SYNC,&ipp_req)){
                    LOGE("ipp do erro,do again,ipp_times = %d!\n",ipp_times);
                 } else {
                    break;
                 }
            }
            if (ipp_times <= 0) {
                ret = -1;
    			goto do_ipp_err;
    		}
        }
    }

do_ipp_err:
    if(ippFd > 0)
       close(ippFd);
	return ret;    
}

extern "C"  int YData_Mirror_Line(int v4l2_fmt_src, int *psrc, int *pdst, int w)
{
    int i;

    for (i=0; i<(w>>2); i++) {
        *pdst = ((*psrc>>24)&0x000000ff) | ((*psrc>>8)&0x0000ff00)
                | ((*psrc<<8)&0x00ff0000) | ((*psrc<<24)&0xff000000);
        psrc++;
        pdst--;
    }

    return 0;
}
extern "C"  int UVData_Mirror_Line(int v4l2_fmt_src, int *psrc, int *pdst, int w)
{
    int i;

    for (i=0; i<(w>>2); i++) {
        *pdst = ((*psrc>>16)&0x0000ffff) | ((*psrc<<16)&0xffff0000);                
        psrc++;
        pdst--;
    }

    return 0;
}
extern "C"  int YuvData_Mirror_Flip(int v4l2_fmt_src, char *pdata, char *pline_tmp, int w, int h)
{
    int *pdata_tmp = NULL;
    int *ptop, *pbottom;
    int err = 0,i,j;

    pdata_tmp = (int*)pline_tmp;
    
    // Y mirror and flip
    ptop = (int*)pdata;
    pbottom = (int*)(pdata+w*(h-1));    
    for (j=0; j<(h>>1); j++) {
        YData_Mirror_Line(v4l2_fmt_src, ptop, pdata_tmp+((w>>2)-1),w);
        YData_Mirror_Line(v4l2_fmt_src, pbottom, ptop+((w>>2)-1), w);
        memcpy(pbottom, pdata_tmp, w);
        ptop += (w>>2);
        pbottom -= (w>>2);
    }
    // UV mirror and flip
    ptop = (int*)(pdata+w*h);
    pbottom = (int*)(pdata+w*(h*3/2-1));    
    for (j=0; j<(h>>2); j++) {
        UVData_Mirror_Line(v4l2_fmt_src, ptop, pdata_tmp+((w>>2)-1),w);
        UVData_Mirror_Line(v4l2_fmt_src, pbottom, ptop+((w>>2)-1), w);
        memcpy(pbottom, pdata_tmp, w);
        ptop += (w>>2);
        pbottom -= (w>>2);
    }
YuvData_Mirror_Flip_end:
    return err;
}
extern "C" int YUV420_rotate(const unsigned char* srcy, int src_stride,  unsigned char* srcuv,
                   unsigned char* dsty, int dst_stride, unsigned char* dstuv,
                   int width, int height,int rotate_angle){
   int i = 0,j = 0;
	// 90 , y plane
  if(rotate_angle == 90){
      srcy += src_stride * (height - 1);
  	  srcuv += src_stride * ((height >> 1)- 1); 
  	  src_stride = -src_stride;
	}else if(rotate_angle == 270){
      dsty += dst_stride * (width - 1);
      dstuv += dst_stride * ((width>>1) - 1);
	  dst_stride = -dst_stride;
  }

  for (i = 0; i < width; ++i)
    for (j = 0; j < height; ++j)
      *(dsty+i * dst_stride + j) = *(srcy+j * src_stride + i); 
  
  //uv 
  unsigned char av_u0,av_v0;
  for (i = 0; i < width; i += 2)
    for (j = 0; j < (height>>1); ++j) {
		av_u0 = *(srcuv+i + (j * src_stride));
		av_v0 = *(srcuv+i + (j * src_stride)+1);
      *(dstuv+((j<<1) + ((i >> 1) * dst_stride)))= av_u0;
      *(dstuv+((j<<1) + ((i >> 1) * dst_stride)+1)) = av_v0;
	}
   
  return 0;
 }

 extern "C" int cameraFormatConvert(int v4l2_fmt_src, int v4l2_fmt_dst, const char *android_fmt_dst, 
							 char *srcbuf, char *dstbuf,long srcphy,long dstphy,int src_size,
							 int src_w, int src_h, int srcbuf_w,
							 int dst_w, int dst_h, int dstbuf_w,
							 bool mirror)
 {
	 int y_size,i,j;
	 int ret = -1;
	 /*
	 if (v4l2_fmt_dst) { 
		 LOGD("cameraFormatConvert '%c%c%c%c'@(0x%x,0x%x,%dx%d)->'%c%c%c%c'@(0x%x,0x%x,%dx%d) ",
					 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
					 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF,
					 (int)srcbuf, srcphy,src_w,src_h,
					 v4l2_fmt_dst & 0xFF, (v4l2_fmt_dst >> 8) & 0xFF,
					 (v4l2_fmt_dst >> 16) & 0xFF, (v4l2_fmt_dst >> 24) & 0xFF,
					  (int)dstbuf,dstphy,dst_w,dst_h);
	 } else if (android_fmt_dst) {
		 LOGD("cameraFormatConvert '%c%c%c%c'@(0x%x,0x%x,%dx%d)->%s@(0x%x,0x%x,%dx%d)",
					 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
					 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF
					 , (int)srcbuf, srcphy,src_w,src_h,android_fmt_dst, (int)dstbuf,dstphy,
					  dst_w,dst_h);
	 }
	 */  
	 
	 y_size = src_w*src_h;
	 switch (v4l2_fmt_src)
	 {
		 case V4L2_PIX_FMT_YUV420:
		 {
//			 if (CAMERA_IS_UVC_CAMERA() 
//				 || (CAMERA_IS_RKSOC_CAMERA() && (mCamDriverCapability.version != KERNEL_VERSION(0, 0, 1)))) {
//				 goto cameraFormatConvert_default;
//			 }
		 }
		 case V4L2_PIX_FMT_NV12:
		 {
			 int *dst_vu, *src_uv;
 
			 if ((v4l2_fmt_dst == V4L2_PIX_FMT_NV12) || 
				 (android_fmt_dst && (strcmp(android_fmt_dst,CAMERA_DISPLAY_FORMAT_NV12)==0))) {
				 if (dstbuf && (dstbuf != srcbuf)) {
					 if (dstbuf_w == dst_w) {
						 memcpy(dstbuf,srcbuf, y_size*3/2);
					 } else {	   /* ddl@rock-chips.com: v0.4.1 */
						 for (i=0;i<(dst_h*3/2);i++) {
							 memcpy(dstbuf,srcbuf, dst_w);
							 dstbuf += dstbuf_w;
							 srcbuf += srcbuf_w;
						 }
					 }
					 ret = 0;
				 }
			 } else if ((v4l2_fmt_dst == V4L2_PIX_FMT_NV21) || 
				 (android_fmt_dst && (strcmp(android_fmt_dst,android::CameraParameters::PIXEL_FORMAT_YUV420SP)==0))) {
				 if ((src_w == dst_w) && (src_h == dst_h)) {
					 if (mirror == false) {
						 if (dstbuf != srcbuf)
							 memcpy(dstbuf,srcbuf, y_size);
						 src_uv = (int*)(srcbuf + y_size); 
						 dst_vu = (int*)(dstbuf+y_size);
						 for (i=0; i<(y_size>>3); i++) {
							 *dst_vu = ((*src_uv&0x00ff00ff)<<8) | ((*src_uv&0xff00ff00)>>8);
							 dst_vu++;
							 src_uv++;
						 }
					 } else {						 
						 char *psrc,*pdst;
						 psrc = srcbuf;
						 pdst = dstbuf + dst_w-1;
						 for (i=0; i<src_h; i++) {							  
							 for (j=0; j<src_w; j++) {
								 *pdst-- = *psrc++;
							 }
							 pdst += 2*dst_w;
						 }
 
						 psrc = srcbuf + y_size; 
						 pdst = dstbuf + y_size + dst_w-1;
						 for (i=0; i<src_h/2; i++) {							
							 for (j=0; j<src_w; j++) {
								 *pdst-- = *psrc++;
							 }
							 pdst += 2*dst_w;
						 }
					 }
					 ret = 0;
				 } else {
					 if ((v4l2_fmt_dst == V4L2_PIX_FMT_NV21) || 
						 (android_fmt_dst && (strcmp(android_fmt_dst,android::CameraParameters::PIXEL_FORMAT_YUV420SP)==0))) {
						 int *dst_uv,*src_uv; 
						 unsigned *dst_y,*src_y,*src_y1;
						 int a, b, c, d;
						 if ((src_w == dst_w*4) && (src_h == dst_h*4)) {
							 dst_y = (unsigned int*)dstbuf;
							 src_y = (unsigned int*)srcbuf; 	 
							 src_y1= src_y + (src_w*3)/4;
							 for (i=0; i<dst_h; i++) {
								 for(j=0; j<dst_w/4; j++) {
									 a = (*src_y>>24) + (*src_y&0xff) + (*src_y1>>24) + (*src_y1&0xff);
									 a >>= 2;
									 src_y++;
									 src_y1++;
									 b = (*src_y>>24) + (*src_y&0xff) + (*src_y1>>24) + (*src_y1&0xff);
									 b >>= 2;
									 src_y++;
									 src_y1++;
									 c = (*src_y>>24) + (*src_y&0xff) + (*src_y1>>24) + (*src_y1&0xff);
									 c >>= 2;
									 src_y++;
									 src_y1++;
									 d = (*src_y>>24) + (*src_y&0xff) + (*src_y1>>24) + (*src_y1&0xff);
									 d >>= 2;
									 src_y++;
									 src_y1++;
									 *dst_y++ = a | (b<<8) | (c<<16) | (d<<24);
								 }
								 //dst_y = (int*)(srcbuf+src_w*(i+1));
								 src_y += (src_w*3)/4;
								 src_y1= src_y + (src_w*3)/4;
							 }
							 dst_uv = (int*)(dstbuf+dst_w*dst_h);
							 //dst_uv = (int*)(srcbuf+y_size);
							 src_uv = (int*)(srcbuf+y_size);
							 for (i=0; i<dst_h/2; i++) {
								 for(j=0; j<dst_w/4; j++) {
									 *dst_uv = (*src_uv&0xffff0000)|((*(src_uv+2)&0xffff0000)>>16);
									 *dst_uv = ((*dst_uv&0x00ff00ff)<<8)|((*dst_uv&0xff00ff00)>>8);
									 dst_uv++;
									 src_uv += 4;
								 }
								 //dst_uv = (int*)(srcbuf+y_size+src_w*(i+1));
								 src_uv += src_w*3/4;
							 }
						 }
						 ret = 0;
					 } else {
						 if (v4l2_fmt_dst) {	
							 LOGE("cameraFormatConvert '%c%c%c%c'@(0x%x,0x%x)->'%c%c%c%c'@(0x%x,0x%x), %dx%d->%dx%d "
								  "scale isn't support",
										 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
										 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF,
										 v4l2_fmt_dst & 0xFF, (v4l2_fmt_dst >> 8) & 0xFF,
										 (v4l2_fmt_dst >> 16) & 0xFF, (v4l2_fmt_dst >> 24) & 0xFF,
										 (long)srcbuf, srcphy, (long)dstbuf,dstphy,src_w,src_h,dst_w,dst_h);
						 } else if (android_fmt_dst) {
							 LOGD("cameraFormatConvert '%c%c%c%c'@(0x%x,0x%x)->%s@(0x%x,0x%x) %dx%d->%dx%d "
								  "scale isn't support",
										 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
										 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF
										 , (long)srcbuf, srcphy,android_fmt_dst, (long)dstbuf,dstphy,
										  src_w,src_h,dst_w,dst_h);
						 }
					 }			
				 }
			 } else if (android_fmt_dst && (strcmp(android_fmt_dst,android::CameraParameters::PIXEL_FORMAT_RGB565)==0)) {
/*				 
				 if (srcphy && dstphy) {
                #ifdef TARGET_RK29 
					 YUV2RGBParams	para;
 
					 memset(&para, 0x00, sizeof(YUV2RGBParams));
					 para.yuvAddr = srcphy;
					 para.outAddr = dstphy;
					 para.inwidth  = (src_w + 15)&(~15);
					 para.inheight = (src_h + 15)&(~15);
					 para.outwidth	= (dst_w + 15)&(~15);
					 para.outheight = (dst_h + 15)&(~15);
					 para.inColor  = PP_IN_YUV420sp;
					 para.outColor	= PP_OUT_RGB565;
 
					 ret = doYuvToRgb(&para);	 
                #else
					 LOGE("%s(%d): Convert nv12 to rgb565 isn't support physical address in current paltform",__FUNCTION__,__LINE__);
                #endif
				 } else if (srcbuf && dstbuf) {
					 if(mRGAFd > 0) {
						 ret = rga_nv12torgb565(mRGAFd,src_w,src_h,srcbuf, (short int*)dstbuf,dstbuf_w);					   
					 } else {
						 ret = arm_nv12torgb565(src_w,src_h,srcbuf, (short int*)dstbuf,dstbuf_w);				  
					 }
				 }
*/			 } else if (android_fmt_dst && (strcmp(android_fmt_dst,android::CameraParameters::PIXEL_FORMAT_YUV420P)==0)) {
				 char *dst_u,*dst_v,*src_y,*dst_y,*srcuv;
				 int dstc_size,dsty_size, align_dstw,align_dsthalfw;
				 
				 if ((src_w == dst_w) && (src_h == dst_h)) { 
					 align_dstw = ((dst_w+15)&0xfffffff0);
					 align_dsthalfw = ((dst_w/2+15)&0xfffffff0);
					 dsty_size = align_dstw*dst_h;
					 dstc_size = align_dsthalfw*dst_h/2;
					 src_y = srcbuf;
					 dst_y = dstbuf;
					 
					 if (mirror == false) {
						 for (j=0; j<src_h; j++) {
							 for (i=0; i<src_w; i++) {
								 *dst_y++ = *src_y++;
							 }
							 dst_y += align_dstw-src_w;
						 }
						 
						 srcuv = (char*)(srcbuf + y_size); 
						 dst_u = (char*)(dstbuf+dsty_size);
						 dst_v = dst_u + dstc_size;
 
						 for (j=0; j<src_h/2; j++) {
							 for (i=0; i<src_w/2; i++) {						
								 *dst_v++ = *srcuv++;
								 *dst_u++ = *srcuv++;
							 }
							 dst_u += align_dsthalfw-src_w/2;
							 dst_v += align_dsthalfw-src_w/2;
						 }
						 
					 } else {						 
						 char *psrc,*pdst;
						 psrc = srcbuf;
						 pdst = dstbuf + dst_w-1;
						 for (i=0; i<src_h; i++) {							  
							 for (j=0; j<src_w; j++) {
								 *pdst-- = *psrc++;
							 }
							 pdst += 2*align_dstw - (align_dstw - dst_w);
						 }
 
						 psrc = srcbuf + y_size; 
						 dst_u = dstbuf + dsty_size + dst_w/2-1;
						 dst_v = dst_u + dstc_size;
						 for (i=0; i<src_h/2; i++) {							
							 for (j=0; j<src_w/2; j++) {
								 *dst_v-- = *psrc++;
								 *dst_u-- = *psrc++;
							 }
							 dst_u += align_dsthalfw*2 - (align_dsthalfw - dst_w/2);
							 dst_v += align_dsthalfw*2 - (align_dsthalfw - dst_w/2);
						 }
					 }
 
					 ret = 0;
				 }
			 }
			 break;
		 }
		 case V4L2_PIX_FMT_YUV422P:
		 {
//			 if (CAMERA_IS_UVC_CAMERA() 
//				 || (mCamDriverCapability.version != KERNEL_VERSION(0, 0, 1))) {
//				 goto cameraFormatConvert_default;
//			 }
		 }		  
		 case V4L2_PIX_FMT_NV16:
		 {
			 break;
		 }
		 case V4L2_PIX_FMT_YUYV:
		 {
			 char *srcbuf_begin;
			 int *dstint_y, *dstint_uv, *srcint;
			 
			 if ((v4l2_fmt_dst == V4L2_PIX_FMT_NV12) || 
				 ((v4l2_fmt_dst == V4L2_PIX_FMT_YUV420)/* && CAMERA_IS_RKSOC_CAMERA() 
				 && (mCamDriverCapability.version == KERNEL_VERSION(0, 0, 1))*/)) { 
				 if ((src_w == dst_w) && (src_h == dst_h)) {
					 dstint_y = (int*)dstbuf;				 
					 srcint = (int*)srcbuf;
					 dstint_uv =  (int*)(dstbuf + y_size);
					 /* 
					  * author :zyh
					  * neon code for YUYV to YUV420
					  */
#ifdef HAVE_ARM_NEON
					 for(i=0;i<src_h;i++) {
						 int n = src_w;
						 char tmp = i%2;//get uv only when in even row
						 asm volatile (
							"   pld [%[src], %[src_stride], lsl #2]                         \n\t"
							"   cmp %[n], #16                                               \n\t"
							"   blt 5f                                                      \n\t"
							"0: @ 16 pixel swap                                             \n\t"
							"   vld2.8  {q0,q1} , [%[src]]!  @ q0 = y q1 = uv               \n\t"
							"   vst1.16 {q0},[%[dst_y]]!     @ now q0  -> dst               \n\t"
							"   cmp %[tmp], #1                                              \n\t"
							"   bge 1f                                                      \n\t"
							"   vst1.16 {q1},[%[dst_uv]]!    @ now q1  -> dst   	    	\n\t"
							"1: @ don't need get uv in odd row                              \n\t"
							"   sub %[n], %[n], #16                                         \n\t"
							"   cmp %[n], #16                                               \n\t"
							"   bge 0b                                                      \n\t"
							"5: @ end                                                       \n\t"
							: [dst_y] "+r" (dstint_y), [dst_uv] "+r" (dstint_uv),[src] "+r" (srcint), [n] "+r" (n),[tmp] "+r" (tmp)
							: [src_stride] "r" (src_w)
							: "cc", "memory", "q0", "q1", "q2"
							);
					 }
					 //LOGE("---------------neon code YUY to YUV420-----------------------------");
					 /*
					  * C code YUYV to YUV420
					  */
#else
					 for(i=0;i<src_h; i++) {
						 for (j=0; j<(src_w>>2); j++) {
							 if(i%2 == 0){
								*dstint_uv++ = (*(srcint+1)&0xff000000)|((*(srcint+1)&0x0000ff00)<<8)
										 |((*srcint&0xff000000)>>16)|((*srcint&0x0000ff00)>>8); 
							 }
							 *dstint_y++ = ((*(srcint+1)&0x00ff0000)<<8)|((*(srcint+1)&0x000000ff)<<16)
							 				 |((*srcint&0x00ff0000)>>8)|(*srcint&0x000000ff);
							 srcint += 2;
						 }
					 }
					 //LOGE("---------------c code YUY to YUV420-----------------------------");
#endif
					 ret = 0;
				 } else {
					 if (v4l2_fmt_dst) {	
						 LOGE("cameraFormatConvert '%c%c%c%c'@(0x%x,0x%x)->'%c%c%c%c'@(0x%x,0x%x), %dx%d->%dx%d "
							  "scale isn't support",
									 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
									 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF,
									 v4l2_fmt_dst & 0xFF, (v4l2_fmt_dst >> 8) & 0xFF,
									 (v4l2_fmt_dst >> 16) & 0xFF, (v4l2_fmt_dst >> 24) & 0xFF,
									 (long)srcbuf, srcphy, (long)dstbuf,dstphy,src_w,src_h,dst_w,dst_h);
					 } else if (android_fmt_dst) {
						 LOGD("cameraFormatConvert '%c%c%c%c'@(0x%x,0x%x)->%s@(0x%x,0x%x) %dx%d->%dx%d "
							  "scale isn't support",
									 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
									 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF
									 , (long)srcbuf, srcphy,android_fmt_dst, (long)dstbuf,dstphy,
									  src_w,src_h,dst_w,dst_h);
					 }
				 }
 
			 } else if ((v4l2_fmt_dst == V4L2_PIX_FMT_NV21)|| 
						(android_fmt_dst && (strcmp(android_fmt_dst,android::CameraParameters::PIXEL_FORMAT_YUV420SP)==0))) {
				 if ((src_w==dst_w) && (src_h==dst_h)) {
					 dstint_y = (int*)dstbuf;				 
					 srcint = (int*)srcbuf;
					 for(i=0;i<(y_size>>2);i++) {
						 *dstint_y++ = ((*(srcint+1)&0x00ff0000)<<8)|((*(srcint+1)&0x000000ff)<<16)
									 |((*srcint&0x00ff0000)>>8)|(*srcint&0x000000ff);
						 srcint += 2;
					 }
					 dstint_uv =  (int*)(dstbuf + y_size);
					 srcint = (int*)srcbuf;
					 for(i=0;i<src_h/2; i++) {
						 for (j=0; j<(src_w>>2); j++) {
							 *dstint_uv++ = ((*(srcint+1)&0xff000000)>>8)|((*(srcint+1)&0x0000ff00)<<16)
										 |((*srcint&0xff000000)>>24)|(*srcint&0x0000ff00); 
							 srcint += 2;
						 }
						 srcint += (src_w>>1);	
					 } 
					 ret = 0;
				 } else {
					 if (v4l2_fmt_dst) {	
						 LOGE("cameraFormatConvert '%c%c%c%c'@(0x%x,0x%x)->'%c%c%c%c'@(0x%x,0x%x), %dx%d->%dx%d "
							  "scale isn't support",
									 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
									 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF,
									 v4l2_fmt_dst & 0xFF, (v4l2_fmt_dst >> 8) & 0xFF,
									 (v4l2_fmt_dst >> 16) & 0xFF, (v4l2_fmt_dst >> 24) & 0xFF,
									 (long)srcbuf, srcphy, (long)dstbuf,dstphy,src_w,src_h,dst_w,dst_h);
					 } else if (android_fmt_dst) {
						 LOGD("cameraFormatConvert '%c%c%c%c'@(0x%x,0x%x)->%s@(0x%x,0x%x) %dx%d->%dx%d "
							  "scale isn't support",
									 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
									 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF
									 , (long)srcbuf, srcphy,android_fmt_dst, (long)dstbuf,dstphy,
									  src_w,src_h,dst_w,dst_h);
					 }
				 }
			 }			  
			 break;
		 }
		 case V4L2_PIX_FMT_RGB565:
		 {
			 if (android_fmt_dst && (strcmp(android_fmt_dst,android::CameraParameters::PIXEL_FORMAT_RGB565)==0)){
				 if (srcbuf && dstbuf && (srcbuf != dstbuf)){
					/* if(mRGAFd > 0) {
						 ret = rga_rgb565_cp(mRGAFd,src_w,src_h,srcbuf, (short int*)dstbuf);					   
					 } else {
						 memcpy(dstbuf,srcbuf,src_w*src_h*2);
						 ret = 0;
					 }*/
				 }
			 }
			 break;
		 }
 
		 case V4L2_PIX_FMT_MJPEG:
		 {
		 	#if 0
			 VPU_FRAME outbuf; 
			 unsigned int output_len;
			 unsigned int input_len,i,j,w,h;
			 FILE *fp;
			 char filename[50];
			 unsigned int *psrc,*pdst;
			 
			 output_len = 0;
			 input_len = src_size;
			 if (v4l2_fmt_dst == V4L2_PIX_FMT_NV12) {				 
				 
				 ret = mMjpegDecoder.decode(mMjpegDecoder.decoder,(unsigned char*)&outbuf, &output_len, (unsigned char*)srcbuf, &input_len);
				 if ((ret >= 0) && (output_len == sizeof(VPU_FRAME))) {
					 VPUMemLink(&outbuf.vpumem);
					 /* following codes are used to get yuv data after decoder */
					 VPUMemInvalidate(&outbuf.vpumem);
					 w = ((outbuf.DisplayWidth+15)&(~15));
					 h = ((outbuf.DisplayHeight+15)&(~15));
					 if (mirror == false) {
						 memcpy(dstbuf, outbuf.vpumem.vir_addr,w*h*3/2);
					 } else {
						 pdst = (unsigned int*)(dstbuf); 
						 psrc = (unsigned int*)outbuf.vpumem.vir_addr;
						 pdst += ((w>>2)-1);
						 for (j=0; j<h; j++) {							  
							 for (i=0; i<(w>>2); i++) {
								 *pdst = ((*psrc>>24)&0x000000ff) | ((*psrc>>8)&0x0000ff00)
										 | ((*psrc<<8)&0x00ff0000) | ((*psrc<<24)&0xff000000);
								 psrc++;
								 pdst--;
							 }
							 pdst += (w>>1);
						 }
 
						 pdst = (unsigned int*)dstbuf; 
						 psrc = (unsigned int*)outbuf.vpumem.vir_addr;
 
						 pdst += (w*h/4);
						 psrc += (w*h/4);
						 pdst += ((w>>2)-1);
						 for (j=0; j<(h/2); j++) {							  
							 for (i=0; i<(w>>2); i++) {
								 *pdst = ((*psrc>>16)&0x0000ffff) | ((*psrc<<16)&0xffff0000);
								 psrc++;
								 pdst--;
							 }
							 pdst += (w>>1);
						 }
					 }
					 VPUFreeLinear(&outbuf.vpumem);
				 } else {
					 LOGE("%s(%d): mjpeg decode failed! ret:%d	output_len: 0x%x, src_buf: %p, input_len: %d",__FUNCTION__,__LINE__,
						 ret,output_len,srcbuf, input_len);
				 }
			 } 
			 #endif
			 break;			
		 }
		 
 cameraFormatConvert_default:		 
		 default:
			 if (android_fmt_dst) {
				 LOGE("%s(%d): CameraHal is not support (%c%c%c%c -> %s)",__FUNCTION__,__LINE__,
					 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
					 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF, android_fmt_dst);
			 } else if (v4l2_fmt_dst) {
				 LOGE("%s(%d): CameraHal is not support (%c%c%c%c -> %c%c%c%c)",__FUNCTION__,__LINE__,
					 v4l2_fmt_src & 0xFF, (v4l2_fmt_src >> 8) & 0xFF,
					 (v4l2_fmt_src >> 16) & 0xFF, (v4l2_fmt_src >> 24) & 0xFF,
					 v4l2_fmt_dst & 0xFF, (v4l2_fmt_dst >> 8) & 0xFF,
					 (v4l2_fmt_dst >> 16) & 0xFF, (v4l2_fmt_dst >> 24) & 0xFF);
			 }
			 break;
	 }
	 return ret;
 
 }


