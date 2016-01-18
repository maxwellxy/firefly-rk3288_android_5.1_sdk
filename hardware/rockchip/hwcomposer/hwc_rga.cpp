/*

* rockchip hwcomposer( 2D graphic acceleration unit) .

*

* Copyright (C) 2015 Rockchip Electronics Co., Ltd.

*/
#include "hwc_rga.h"
#include "rga_angle.h"

inline int WIDTH(const hwc_rect &rect) { return rect.right - rect.left; }
inline int HEIGHT(const hwc_rect &rect) { return rect.bottom - rect.top; }
template<typename T> inline T max(T a, T b) { return (a > b) ? a : b; }
template<typename T> inline T min(T a, T b) { return (a < b) ? a : b; }

typedef struct 
{
	int rga_fd;
	int scale_type;
	int model;
    struct rga_req  rga_cfg;
}rga_cfg_t;

static rga_cfg_t  g_rga_cfg;

int init_rga_cfg(int rga_fd)
{
   if (rga_fd <= 0)
   {
	   ALOGD("%s,%d,error,rga_fd:[%d]",__FUNCTION__,__LINE__,rga_fd);
	   return -1;
   }
   memset(&g_rga_cfg, 0, sizeof(rga_cfg_t));
   g_rga_cfg.rga_fd = rga_fd;
   g_rga_cfg.scale_type = NOT_TO_SCALE;
   return 0;
}
 
bool is_land_screen(hwc_rect_t rect)
{
   return  (WIDTH(rect) > HEIGHT(rect)) ? true:false;
}

double is_scrop_by_width(int src_w, int src_h, int dst_w, int dst_h)
{
   double t = 0.0;
   if (src_w > src_h)
   {
	   double ws = 1.0*src_w/dst_w;
	   double hs = 1.0*src_h/dst_h;
	   t = ws > hs ? hs:ws;
   }
   else
   {
	   double ws = 1.0*dst_w/src_w;
	   double hs = 1.0*dst_h/src_h;
	   t = ws > hs ? hs:ws;
   }
   return t;
}

hwc_rect_t calculate_src_crop_center(hwc_rect_t src_rect, hwc_rect_t dst_rect, int transform)
{
	hwc_rect_t crop;

	if (g_rga_cfg.scale_type == NOT_TO_SCALE)
	{
		crop.left = src_rect.left;
		crop.top = src_rect.top;
		crop.right = src_rect.right;
		crop.bottom = src_rect.bottom;
		ALOGD("[%d,%d,%d,%d]",crop.left,crop.top,crop.right,crop.bottom);
		return crop;
	}
	//·ÇµÈ±ÈÀý
	switch (transform)
	{
	case 0:

		break;
	case HWC_TRANSFORM_ROT_90:

		break;
	case HWC_TRANSFORM_ROT_180:
		break;
	case HWC_TRANSFORM_ROT_270:
		break;
	default:

		break;
	}
	return crop;
}

hwc_rect_t calculate_wfd_dst_crop_center(hwc_rect_t src_rect, hwc_rect_t dst_rect, int transform)
{
	hwc_rect_t crop;
        memset(&crop, 0, sizeof(hwc_rect_t));
	switch (transform)
	{
	case 0:
		if (is_land_screen(src_rect)) 
		{
			crop.left = dst_rect.left;
			crop.top = dst_rect.top;
			crop.right = dst_rect.right;
			crop.bottom = dst_rect.bottom;
		}
		else //port screen
		{
		   int sw = WIDTH(src_rect);
		   int sh = HEIGHT(src_rect);
		   int dw = WIDTH(dst_rect);
		   int dh = HEIGHT(dst_rect);
	           double aspec = is_scrop_by_width(sw,sh,dw,dh);
		   int w = sw*aspec;
		   int h = sh*aspec;
		   if (dw > dh)
		   {
			   crop.left = (dw-w)/2;
			   crop.top = (dh-h)/2;
			   crop.right = crop.left + w;
			   crop.bottom = crop.top + h;
		   }
		   else
		   {
			  
		   }
		}
		break;
	case HWC_TRANSFORM_ROT_90:
		if (is_land_screen(src_rect)) 
		{
			int sh = WIDTH(src_rect);
			int sw = HEIGHT(src_rect);
			int dw = WIDTH(dst_rect);
			int dh = HEIGHT(dst_rect);
			double aspec = is_scrop_by_width(sw,sh,dw,dh);
			int w = sw*aspec;
			int h = sh*aspec;
			if (dw > dh)
			{
				crop.left = (dw-w)/2;
				crop.top = (dh-h)/2;
				crop.right = crop.left + h;
				crop.bottom = crop.top + w;
			}
			else
			{
				crop.left = dst_rect.left;
				crop.top = dst_rect.top;
				crop.right = dst_rect.right;
				crop.bottom = dst_rect.bottom;
			}
		}
		else  //port screen
		{
			int dw = WIDTH(dst_rect);
			int dh = HEIGHT(dst_rect);
			/*
			int sh = WIDTH(src_rect);
			int sw = HEIGHT(src_rect);
			double aspec = is_scrop_by_width(sw,sh,dw,dh);
			int w = sw*aspec;
			int h = sh*aspec;*/
			if (dw > dh)
			{
				crop.left = dst_rect.left;
				crop.top = dst_rect.top;
				crop.right = dst_rect.bottom;
				crop.bottom = dst_rect.right;
			}
		}
		break;
	case HWC_TRANSFORM_ROT_180:
		if (is_land_screen(src_rect)) 
		{
			crop.left = dst_rect.left;
			crop.top = dst_rect.top;
			crop.right = dst_rect.right;
			crop.bottom = dst_rect.bottom;
		}
		else
		{
			int sw = WIDTH(src_rect);
			int sh = HEIGHT(src_rect);
			int dw = WIDTH(dst_rect);
			int dh = HEIGHT(dst_rect);
			double aspec = is_scrop_by_width(sw,sh,dw,dh);
			int w = sw*aspec;
			int h = sh*aspec;
			if (dw > dh)
			{
				crop.left = (dw-w)/2;
				crop.top = (dh-h)/2;
				crop.right = crop.left + w;
				crop.bottom = crop.top + h;
			}
			else
			{
				crop.left = dst_rect.left;
				crop.top = dst_rect.top;
				crop.right = dst_rect.right;
				crop.bottom = dst_rect.bottom;
			}
		}
		break;
	case HWC_TRANSFORM_ROT_270:
		if (is_land_screen(src_rect)) 
		{
			int sh = WIDTH(src_rect);
			int sw = HEIGHT(src_rect);
			int dw = WIDTH(dst_rect);
			int dh = HEIGHT(dst_rect);
			double aspec = is_scrop_by_width(sw,sh,dw,dh);
			int w = sw*aspec;
			int h = sh*aspec;
			if (dw > dh)
			{
				crop.left = (dw-w)/2;
				crop.top = (dh-h)/2;
				crop.right = crop.left + h;
				crop.bottom = crop.top + w;
			}
			else
			{
				crop.left = dst_rect.left;
				crop.top = dst_rect.top;
				crop.right = dst_rect.right;
				crop.bottom = dst_rect.bottom;
			}
		}
		else
		{
			int dw = WIDTH(dst_rect);
			int dh = HEIGHT(dst_rect);
			/*int sh = WIDTH(src_rect);
			int sw = HEIGHT(src_rect);
			double aspec = is_scrop_by_width(sw,sh,dw,dh);
			int w = sw*aspec;
			int h = sh*aspec;*/
			if (dw > dh)
			{
				crop.left = dst_rect.left;
				crop.top = dst_rect.top;
				crop.right = dst_rect.bottom;
				crop.bottom = dst_rect.right;
			}
		}
		break;
	default:

		break;
	}
	return crop;
}

void set_transform_cfg(int transform)
{
	switch (transform)
	{
	case 0:
		g_rga_cfg.rga_cfg.sina = 0;
		g_rga_cfg.rga_cfg.cosa = 0x10000;
		break;
	case HWC_TRANSFORM_ROT_90:
		g_rga_cfg.rga_cfg.sina = sina_table[90];
		g_rga_cfg.rga_cfg.cosa = cosa_table[90];
		g_rga_cfg.rga_cfg.rotate_mode = 1;
		break;
	case HWC_TRANSFORM_ROT_180:
		g_rga_cfg.rga_cfg.sina = sina_table[180];
		g_rga_cfg.rga_cfg.cosa = cosa_table[180];
		g_rga_cfg.rga_cfg.rotate_mode = 1;
		break;
	case HWC_TRANSFORM_ROT_270:
		ALOGD("set_transform_cfg:HWC_TRANSFORM_ROT_270");
		g_rga_cfg.rga_cfg.sina = sina_table[270];
		g_rga_cfg.rga_cfg.cosa = cosa_table[270];
		g_rga_cfg.rga_cfg.rotate_mode = 1;
		break;
	default:
		break;
	}
	return ;
}

int get_wfd_transform(int transform)
{
	switch (transform)
	{
	  case 0:
		  return 0;
		  break;
	  case HWC_TRANSFORM_ROT_90:
		  return HWC_TRANSFORM_ROT_270;
		  break;
	  case HWC_TRANSFORM_ROT_180:
		  return HWC_TRANSFORM_ROT_180;
		  break;
      case HWC_TRANSFORM_ROT_270:
		  return HWC_TRANSFORM_ROT_90;
		  break;
	  default:
		  return 0;
	}
}

void set_rga_offset(hwc_rect_t src_rect,hwc_rect_t dst_rect, int transform)
{
	switch (transform)
	{
		case 0:
		{
			g_rga_cfg.rga_cfg.dst.x_offset = dst_rect.left;
			g_rga_cfg.rga_cfg.dst.y_offset = dst_rect.top;
			break;
		}
		case HWC_TRANSFORM_ROT_90:
		{
			int x_offset = HEIGHT(dst_rect)+dst_rect.left-1;
			int y_offset = dst_rect.top;
			if (x_offset < 0)
			{
			   x_offset = 0;
			}
			if (y_offset < 0)
			{
				y_offset = 0;
			}
			g_rga_cfg.rga_cfg.dst.x_offset = x_offset;
			g_rga_cfg.rga_cfg.dst.y_offset = y_offset;
			break;
		}
		case HWC_TRANSFORM_ROT_180:
		{
			int x_offset = HEIGHT(dst_rect)+dst_rect.left-1;
			int y_offset = dst_rect.top;
			if (x_offset < 0)
			{
				x_offset = 0;
			}
			if (y_offset < 0)
			{
				y_offset = 0;
			}
			g_rga_cfg.rga_cfg.dst.x_offset = WIDTH(dst_rect)-1+dst_rect.left;
			g_rga_cfg.rga_cfg.dst.y_offset = HEIGHT(dst_rect)-1;
			break;
		}
		case HWC_TRANSFORM_ROT_270:
		{
			int x_offset = dst_rect.left-1;
			int y_offset = WIDTH(dst_rect)-1;
			if (x_offset < 0)
			{
				x_offset = 0;
			}
			if (y_offset < 0)
			{
				y_offset = 0;
			}
			g_rga_cfg.rga_cfg.dst.x_offset = x_offset;
			g_rga_cfg.rga_cfg.dst.y_offset = y_offset;
			break;
		}
		default:
			break;
	}
}

void set_scale_mode(hwc_rect_t src_rect,hwc_rect_t dst_rect, int transform)
{
	double ws = 1.0*WIDTH(src_rect)/WIDTH(dst_rect);
	double hs = 1.0*HEIGHT(src_rect)/HEIGHT(dst_rect);
	if (ws>1.0 || hs>1.0)
	{
		g_rga_cfg.rga_cfg.scale_mode = 2;
		g_rga_cfg.rga_cfg.rotate_mode = 2;
	}
	else if (ws==1.0 && hs==1.0)
	{
		g_rga_cfg.rga_cfg.scale_mode = 0;
		g_rga_cfg.rga_cfg.rotate_mode = 0;
	}
	else
	{
		g_rga_cfg.rga_cfg.scale_mode = 1;
		g_rga_cfg.rga_cfg.rotate_mode = 1;
	}
}

int set_rga_cfg(hwc_cfg_t  *cfg)
{
  if (cfg == NULL)
  {
	  ALOGD("%s,%d,error, cfg is NULL.",__FUNCTION__,__LINE__);
	  return -1;
  }
  hwc_rect_t src_crop;
  hwc_rect_t dst_crop;
  memset(&src_crop, 0, sizeof(hwc_rect_t));
  memset(&dst_crop, 0, sizeof(hwc_rect_t));
  int transform = get_wfd_transform(cfg->transform);
  ALOGD_IF(DEBUG_LOG,"transform=%d,%d",cfg->transform,HWC_TRANSFORM_ROT_270);
  src_crop = calculate_src_crop_center(cfg->src_rect,cfg->dst_rect,transform);
  dst_crop = calculate_wfd_dst_crop_center(cfg->src_rect,cfg->dst_rect,transform);
  ALOGD_IF(DEBUG_LOG,"src_crop:[%d,%d,%d,%d]",src_crop.left,src_crop.top,src_crop.right,src_crop.bottom);
  ALOGD_IF(DEBUG_LOG,"dst_crop:[%d,%d,%d,%d]",dst_crop.left,dst_crop.top,dst_crop.right,dst_crop.bottom);
  struct private_handle_t *src_handle = cfg->src_handle;
  struct private_handle_t *dst_handle = cfg->dst_handle;
  //src cfg
  if (cfg->rga_fbAddr==0)
  {
	   g_rga_cfg.rga_cfg.src.yrgb_addr = (unsigned long )(SRC_HANDLE_BASE);
  }
  else
  {
	   g_rga_cfg.rga_cfg.src.yrgb_addr = cfg->rga_fbAddr+0x60000000;
  }
  g_rga_cfg.rga_cfg.src.vir_w = WIDTH(cfg->src_rect);
  g_rga_cfg.rga_cfg.src.vir_h = HEIGHT(cfg->src_rect);
  g_rga_cfg.rga_cfg.src.format = SRC_HANDLE_FORMAT == \
	  HAL_PIXEL_FORMAT_RGB_565 ?  RK_FORMAT_RGB_565 : RK_FORMAT_RGBX_8888;
  g_rga_cfg.rga_cfg.src.act_w = WIDTH(src_crop);
  g_rga_cfg.rga_cfg.src.act_h = HEIGHT(src_crop);
  g_rga_cfg.rga_cfg.src.x_offset = src_crop.left;
  g_rga_cfg.rga_cfg.src.y_offset = src_crop.top;

  //dst cfg
  //memset((void*)cfg->dst_handle->base,0,cfg->dst_handle->width*cfg->dst_handle->height*4);

  g_rga_cfg.rga_cfg.dst.yrgb_addr = (unsigned long)(DST_HANDLE_BASE);

  g_rga_cfg.rga_cfg.dst.vir_w = WIDTH(cfg->dst_rect);
  g_rga_cfg.rga_cfg.dst.vir_h = HEIGHT(cfg->dst_rect);
  g_rga_cfg.rga_cfg.dst.format = DST_HANDLE_FORMAT == \
	  HAL_PIXEL_FORMAT_RGB_565 ?  RK_FORMAT_RGB_565 : RK_FORMAT_RGBX_8888;
  g_rga_cfg.rga_cfg.dst.act_w = WIDTH(dst_crop);
  g_rga_cfg.rga_cfg.dst.act_h = HEIGHT(dst_crop);
  ALOGD_IF(DEBUG_LOG, "src:vir_w[%d],vir_h[%d],act_w[%d],act_h[%d],src_addr=%x----,"\
	                  "dst:vir_w[%d],vir_h[%d],act_w[%d],act_h[%d]",\
	                    g_rga_cfg.rga_cfg.src.vir_w,\
						g_rga_cfg.rga_cfg.src.vir_h,\
						g_rga_cfg.rga_cfg.src.act_w,\
						g_rga_cfg.rga_cfg.src.act_h,\
						cfg->rga_fbAddr,\
						g_rga_cfg.rga_cfg.dst.vir_w,\
						g_rga_cfg.rga_cfg.dst.vir_h,\
						g_rga_cfg.rga_cfg.dst.act_w,\
						g_rga_cfg.rga_cfg.dst.act_h
						);

  set_transform_cfg(transform);
  set_rga_offset(src_crop, dst_crop, transform);
  ALOGD_IF(DEBUG_LOG, "x_offset=%d,y_offset=%d",g_rga_cfg.rga_cfg.dst.x_offset,g_rga_cfg.rga_cfg.dst.y_offset);

  g_rga_cfg.rga_cfg.clip.xmin = 0;
  g_rga_cfg.rga_cfg.clip.xmax = g_rga_cfg.rga_cfg.dst.vir_w - 1;//1023
  g_rga_cfg.rga_cfg.clip.ymin = 0;
  g_rga_cfg.rga_cfg.clip.ymax = g_rga_cfg.rga_cfg.dst.vir_h - 1;//767
  g_rga_cfg.rga_cfg.alpha_rop_flag |= (1 << 5);
  g_rga_cfg.rga_cfg.mmu_info.mmu_en = 1;
  g_rga_cfg.rga_cfg.mmu_info.mmu_flag  = ((2 & 0x3) << 4) | 1;

  ALOGD_IF(DEBUG_LOG,"src rect[%d,%d,%d,%d],src_addr=0x%x,dst rect[%d,%d,%d,%d],dst_addr=0x%x",\
	  cfg->src_rect.left,cfg->src_rect.top,cfg->src_rect.right,cfg->src_rect.bottom,SRC_HANDLE_BASE,\
	  cfg->dst_rect.left,cfg->dst_rect.top,cfg->dst_rect.right,cfg->dst_rect.bottom,DST_HANDLE_BASE);

  return 0;
}

int do_rga_transform_and_scale()
{
    if (g_rga_cfg.rga_fd > 0)
	{
		if(ioctl(g_rga_cfg.rga_fd, RGA_BLIT_SYNC, &g_rga_cfg.rga_cfg) != 0)
		{
			ALOGE("%s(%d):  RGA_BLIT_SYNC Failed ", __FUNCTION__, __LINE__);
		}
		else
		{
			ALOGD("RGA_BLIT_SYNC Succ.");
		}
	}
    return 0;
}






