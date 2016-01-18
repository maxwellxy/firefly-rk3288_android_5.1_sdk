/*

* rockchip hwcomposer( 2D graphic acceleration unit) .

*

* Copyright (C) 2015 Rockchip Electronics Co., Ltd.

*/
#include <hardware/rga.h>
#include "rga_angle.h"
#include "stdio.h"
int
RGA_set_src_act_info(
		struct rga_req *req,
		unsigned int   width,       /* act width  */
		unsigned int   height,      /* act height */
		unsigned int   x_off,       /* x_off      */
		unsigned int   y_off        /* y_off      */
		)
{
    req->src.act_w = width;
    req->src.act_h = height;
    req->src.x_offset = x_off;
    req->src.y_offset = y_off;
    
    return 1;
}

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_set_src_vir_info(
		struct rga_req *req,
		unsigned long   yrgb_addr,       /* yrgb_addr  */
		unsigned long   uv_addr,         /* uv_addr    */
		unsigned long   v_addr,          /* v_addr     */
		unsigned int   vir_w,           /* vir width  */
		unsigned int   vir_h,           /* vir height */
		unsigned char  format,          /* format     */
		unsigned char  a_swap_en        /* only for 32bit RGB888 format */
		)
#else
int
RGA_set_src_vir_info(
		struct rga_req *req,
		unsigned int   yrgb_addr,       /* yrgb_addr  */
		unsigned int   uv_addr,         /* uv_addr    */
		unsigned int   v_addr,          /* v_addr     */
		unsigned int   vir_w,           /* vir width  */
		unsigned int   vir_h,           /* vir height */
		unsigned char  format,          /* format     */
		unsigned char  a_swap_en        /* only for 32bit RGB888 format */
		)
#endif
{
    req->src.yrgb_addr = yrgb_addr;
    req->src.uv_addr  = uv_addr;
    req->src.v_addr   = v_addr;
    req->src.vir_w = vir_w;
    req->src.vir_h = vir_h;
    req->src.format = format;
    req->src.alpha_swap |= (a_swap_en & 1);

    return 1;
}

int
RGA_set_dst_act_info(
		struct rga_req *req,
		unsigned int   width,       /* act width  */
		unsigned int   height,      /* act height */
		unsigned int   x_off,       /* x_off      */
		unsigned int   y_off        /* y_off      */
		)
{
    req->dst.act_w = width;
    req->dst.act_h = height;
    req->dst.x_offset = x_off;
    req->dst.y_offset = y_off;
    
    return 1;
}

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_set_dst_vir_info(
		struct rga_req *msg,
		unsigned long   yrgb_addr,   /* yrgb_addr   */
		unsigned long   uv_addr,     /* uv_addr     */
		unsigned long   v_addr,      /* v_addr      */
		unsigned int   vir_w,       /* vir width   */
		unsigned int   vir_h,       /* vir height  */
		RECT           *clip,        /* clip window */
		unsigned char  format,      /* format      */
		unsigned char  a_swap_en
		)
#else
int
RGA_set_dst_vir_info(
		struct rga_req *msg,
		unsigned int   yrgb_addr,   /* yrgb_addr   */
		unsigned int   uv_addr,     /* uv_addr     */
		unsigned int   v_addr,      /* v_addr      */
		unsigned int   vir_w,       /* vir width   */
		unsigned int   vir_h,       /* vir height  */
		RECT           *clip,        /* clip window */
		unsigned char  format,      /* format      */
		unsigned char  a_swap_en
		)
#endif
{
    msg->dst.yrgb_addr = yrgb_addr;
    msg->dst.uv_addr  = uv_addr;
    msg->dst.v_addr   = v_addr;
    msg->dst.vir_w = vir_w;
    msg->dst.vir_h = vir_h;
    msg->dst.format = format;

    msg->clip.xmin = clip->xmin;
    msg->clip.xmax = clip->xmax;
    msg->clip.ymin = clip->ymin;
    msg->clip.ymax = clip->ymax;
    
    msg->dst.alpha_swap |= (a_swap_en & 1);
    
    return 1;
}

int 
RGA_set_pat_info(
    struct rga_req *msg,
    unsigned int width,
    unsigned int height,
    unsigned int x_off,
    unsigned int y_off,
    unsigned int pat_format    
    )
{    
    msg->pat.act_w = width;
    msg->pat.act_h = height;
    msg->pat.x_offset = x_off;
    msg->pat.y_offset = y_off;

    msg->pat.format = pat_format;

    return 1;
}


#if defined(__arm64__) || defined(__aarch64__)
int
RGA_set_rop_mask_info(
		struct rga_req *msg,
		unsigned long rop_mask_addr,
		unsigned int rop_mask_endian_mode
		)
#else
int
RGA_set_rop_mask_info(
		struct rga_req *msg,
		unsigned int rop_mask_addr,
		unsigned int rop_mask_endian_mode
		)
#endif
{
    msg->rop_mask_addr = rop_mask_addr;
    msg->endian_mode = rop_mask_endian_mode;
    return 1;
}

   
int RGA_set_alpha_en_info(
		struct rga_req *msg,
		unsigned int  alpha_cal_mode,    /* 0:alpha' = alpha + (alpha>>7) | alpha' = alpha */
		unsigned int  alpha_mode,        /* 0 global alpha / 1 per pixel alpha / 2 mix mode */
		unsigned int  global_a_value,
		unsigned int  PD_en,             /* porter duff alpha mode en */ 
		unsigned int  PD_mode, 
		unsigned int  dst_alpha_en )     /* use dst alpha  */
{
	
    msg->alpha_rop_flag |= 1;
    msg->alpha_rop_flag |= ((PD_en & 1) << 3);
    msg->alpha_rop_flag |= ((alpha_cal_mode & 1) << 4);

    msg->alpha_global_value = global_a_value;
    msg->alpha_rop_mode |= (alpha_mode & 3);    
    msg->alpha_rop_mode |= (dst_alpha_en << 5);

    msg->PD_mode = PD_mode;
    
    
    return 1;
}


int
RGA_set_rop_en_info(
		struct rga_req *msg,
		unsigned int ROP_mode,
		unsigned int ROP_code,
		unsigned int color_mode,
		unsigned int solid_color
		)
{
    msg->alpha_rop_flag |= (0x3);
    msg->alpha_rop_mode |= ((ROP_mode & 3) << 2);
      
    msg->rop_code = ROP_code;
    msg->color_fill_mode = color_mode;
    msg->fg_color = solid_color;
    return 1;
}

int RGA_set_fading_en_info(
		struct rga_req *msg,
		unsigned char r,
		unsigned char g,
		unsigned char b
		)
{
    msg->alpha_rop_flag |= (0x1 << 2);

    msg->fading.b = b;
    msg->fading.g = g;
    msg->fading.r = r;
    return 1;
}

int 
RGA_set_src_trans_mode_info(
		struct rga_req *msg,
		unsigned char trans_mode,
		unsigned char a_en,
		unsigned char b_en,
		unsigned char g_en,
		unsigned char r_en,
		unsigned char color_key_min,
		unsigned char color_key_max,
		unsigned char zero_mode_en
		)
{
    msg->src_trans_mode = ((a_en & 1) << 4) | ((b_en & 1) << 3) | ((g_en & 1) << 2) | ((r_en & 1) << 1) | (trans_mode & 1);
    
    msg->color_key_min = color_key_min;
    msg->color_key_max = color_key_max;
    msg->alpha_rop_mode |= (zero_mode_en << 4);
    return 1;
}


int
RGA_set_bitblt_mode(
		struct rga_req *msg,
		unsigned char scale_mode,    // 0/near  1/bilnear  2/bicubic  
		unsigned char rotate_mode,   // 0/copy 1/rotate_scale 2/x_mirror 3/y_mirror 
		unsigned int  angle,         // rotate angle     
		unsigned int  dither_en,     // dither en flag   
		unsigned int  AA_en,         // AA flag          
		unsigned int  yuv2rgb_mode
		)
{
    unsigned int alpha_mode;
    msg->render_mode = bitblt_mode;

    if(((msg->src.act_w >> 1) > msg->dst.act_w) || ((msg->src.act_h >> 1) > msg->dst.act_h))
        return -1;
    
    msg->scale_mode = scale_mode;
    msg->rotate_mode = rotate_mode;
    
    msg->sina = sina_table[angle];
    msg->cosa = cosa_table[angle];

    msg->yuv2rgb_mode = yuv2rgb_mode;

    msg->alpha_rop_flag |= ((AA_en << 7) & 0x80);

    alpha_mode = msg->alpha_rop_mode & 3;
    if(rotate_mode == BB_ROTATE)
    {
        if (AA_en == ENABLE) 
        {   
            if ((msg->alpha_rop_flag & 0x3) == 0x1)
            {
                if (alpha_mode == 0)
                {
                msg->alpha_rop_mode = 0x2;            
                }
                else if (alpha_mode == 1)
                {
                    msg->alpha_rop_mode = 0x1;
                }
            }
            else
            {
                msg->alpha_rop_flag |= 1;
                msg->alpha_rop_mode = 1;
            }                        
        }        
    }
   
    if (msg->src_trans_mode)
        msg->scale_mode = 0;

    msg->alpha_rop_flag |= (dither_en << 5);
    
    return 0;
}


int
RGA_set_color_palette_mode(
		struct rga_req *msg,
		unsigned char  palette_mode,    /* 1bpp/2bpp/4bpp/8bpp */
		unsigned char  endian_mode,     /* src endian mode sel */
		unsigned int  bpp1_0_color,    /* BPP1 = 0 */
		unsigned int  bpp1_1_color     /* BPP1 = 1 */
		)
{
    msg->render_mode = color_palette_mode;
    
    msg->palette_mode = palette_mode;
    msg->endian_mode = endian_mode;
    msg->fg_color = bpp1_0_color;
    msg->bg_color = bpp1_1_color;
    
    return 1;
}



int
RGA_set_color_fill_mode(
		struct rga_req *msg,
		COLOR_FILL  *gr_color,              /* gradient color part         */
		unsigned char  gr_satur_mode,            /* saturation mode             */
    	unsigned char  cf_mode,                  /* patten fill or solid fill   */
		unsigned int color,                    /* solid color                 */
		unsigned short pat_width,                /* pattern width               */
		unsigned short pat_height,               /* pattern height              */   
		unsigned char pat_x_off,                 /* pattern x offset            */
		unsigned char pat_y_off,                 /* pattern y offset            */
		unsigned char aa_en                      /* alpha en                    */
		)
{
    msg->render_mode = color_fill_mode;

    msg->gr_color.gr_x_a = ((int)(gr_color->gr_x_a * 256.0))& 0xffff;
    msg->gr_color.gr_x_b = ((int)(gr_color->gr_x_b * 256.0))& 0xffff;
    msg->gr_color.gr_x_g = ((int)(gr_color->gr_x_g * 256.0))& 0xffff;
    msg->gr_color.gr_x_r = ((int)(gr_color->gr_x_r * 256.0))& 0xffff;

    msg->gr_color.gr_y_a = ((int)(gr_color->gr_y_a * 256.0))& 0xffff;
    msg->gr_color.gr_y_b = ((int)(gr_color->gr_y_b * 256.0))& 0xffff;
    msg->gr_color.gr_y_g = ((int)(gr_color->gr_y_g * 256.0))& 0xffff;
    msg->gr_color.gr_y_r = ((int)(gr_color->gr_y_r * 256.0))& 0xffff;

    msg->color_fill_mode = cf_mode;
    
    msg->pat.act_w = pat_width;
    msg->pat.act_h = pat_height;

    msg->pat.x_offset = pat_x_off;
    msg->pat.y_offset = pat_y_off;

    msg->fg_color = color;

    msg->alpha_rop_flag |= ((gr_satur_mode & 1) << 6);
    
    if(aa_en)
    {
    	msg->alpha_rop_flag |= 0x1;
    	msg->alpha_rop_mode  = 1;
    }
    return 1;
}


int
RGA_set_line_point_drawing_mode(
		struct rga_req *msg,
		POINT sp,                     /* start point              */
		POINT ep,                     /* end   point              */
		unsigned int color,           /* line point drawing color */
		unsigned int line_width,      /* line width               */
		unsigned char AA_en,          /* AA en                    */
		unsigned char last_point_en   /* last point en            */
		)

{
    msg->render_mode = line_point_drawing_mode;

    msg->line_draw_info.start_point.x = sp.x;
    msg->line_draw_info.start_point.y = sp.y;
    msg->line_draw_info.end_point.x = ep.x;
    msg->line_draw_info.end_point.y = ep.y;

    msg->line_draw_info.color = color;
    msg->line_draw_info.line_width = line_width;
    msg->line_draw_info.flag |= (AA_en & 1);
    msg->line_draw_info.flag |= ((last_point_en & 1) << 1);
    
    if (AA_en == 1)
    {
        msg->alpha_rop_flag = 1;
        msg->alpha_rop_mode = 0x1;
    }
    
    return 1;
}


int
RGA_set_blur_sharp_filter_mode(
		struct rga_req *msg,
		unsigned char filter_mode,   /* blur/sharpness   */
		unsigned char filter_type,   /* filter intensity */
		unsigned char dither_en      /* dither_en flag   */
		)
{
    msg->render_mode = blur_sharp_filter_mode;
    
    msg->bsfilter_flag |= (filter_type & 3);
    msg->bsfilter_flag |= ((filter_mode & 1) << 2);
    msg->alpha_rop_flag |= ((dither_en & 1) << 5);
    return 1;
}

int
RGA_set_pre_scaling_mode(
		struct rga_req *msg,
		unsigned char dither_en
		)
{
    msg->render_mode = pre_scaling_mode;
    
    msg->alpha_rop_flag |= ((dither_en & 1) << 5);
    return 1;
}

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_update_palette_table_mode(
		struct rga_req *msg,
		unsigned long LUT_addr,      /* LUT table addr      */
		unsigned int palette_mode   /* 1bpp/2bpp/4bpp/8bpp */
		)
#else
int
RGA_update_palette_table_mode(
		struct rga_req *msg,
		unsigned int LUT_addr,      /* LUT table addr      */
		unsigned int palette_mode   /* 1bpp/2bpp/4bpp/8bpp */
		)
#endif
{
    msg->render_mode = update_palette_table_mode;
    
    msg->LUT_addr = LUT_addr;
    msg->palette_mode = palette_mode;
    return 1;
}


int
RGA_set_update_patten_buff_mode(
		struct rga_req *msg,
		unsigned int pat_addr, /* patten addr    */
		unsigned int w,        /* patten width   */
		unsigned int h,        /* patten height  */
		unsigned int format    /* patten format  */
		)
{
    msg->render_mode = update_patten_buff_mode;
    
    msg->pat.yrgb_addr   = pat_addr;
    msg->pat.act_w  = w*h;   // hxx
    msg->pat.act_h  = 1;     // hxx
    msg->pat.format = format;    
    return 1;
}

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_set_mmu_info(
		struct rga_req *msg,
		unsigned char  mmu_en,
		unsigned char  src_flush,
		unsigned char  dst_flush,
		unsigned char  cmd_flush,
		unsigned long base_addr,
		unsigned char  page_size
		)
#else
int
RGA_set_mmu_info(
		struct rga_req *msg,
		unsigned char  mmu_en,
		unsigned char  src_flush,
		unsigned char  dst_flush,
		unsigned char  cmd_flush,
		unsigned int base_addr,
		unsigned char  page_size
		)
#endif
{
    msg->mmu_info.mmu_en    = mmu_en;
    msg->mmu_info.base_addr = base_addr;
    msg->mmu_info.mmu_flag  = ((page_size & 0x3) << 4) |
                              ((cmd_flush & 0x1) << 3) |
                              ((dst_flush & 0x1) << 2) | 
                              ((src_flush & 0x1) << 1) | mmu_en;
    return 1;
}


#if 0
struct fb_info * get_fb(int fb_id)
{
    struct rk_fb_inf *inf =  platform_get_drvdata(g_fb_pdev);
    struct fb_info *fb = inf->fb[fb_id];
    return fb;
}
EXPORT_SYMBOL(get_fb);

void direct_fb_show(struct fb_info * fbi)
{
    rk_fb_set_par(fbi);
}
EXPORT_SYMBOL(direct_fb_show);
#endif

/* copy */
void rga_test_0()
{
    struct rga_req req;

    unsigned int  *src_addr = NULL;
    unsigned int  *dst_addr = NULL;

    RECT clip;

    clip.xmin = 0;
    clip.xmax = 799;
    clip.ymin = 0;
    clip.ymax = 479;
    
    RGA_set_src_act_info(&req, 320, 240, 0, 0);
#if defined(__arm64__) || defined(__aarch64__)
    RGA_set_src_vir_info(&req, (unsigned long)src_addr, 0, 0, 320, 240, RK_FORMAT_RGBA_8888, 0);
#else
    RGA_set_src_vir_info(&req, (unsigned int)src_addr, 0, 0, 320, 240, RK_FORMAT_RGBA_8888, 0);
#endif
    
    RGA_set_dst_act_info(&req, 320, 240, 200, 100);
#if defined(__arm64__) || defined(__aarch64__)
    RGA_set_dst_vir_info(&req, (unsigned long)dst_addr, 0, 0, 800, 480, &clip, RK_FORMAT_RGBA_8888, 0);
#else
    RGA_set_dst_vir_info(&req, (unsigned int)dst_addr, 0, 0, 800, 480, &clip, RK_FORMAT_RGBA_8888, 0);
#endif

    RGA_set_bitblt_mode(&req, 0, 0, 0, 0, 0, 0);

    RGA_set_mmu_info(&req, 1, 0, 0, 0, 0, 2);
	
#if 0
    {
    fb_info *fb = get_fb(0);

    fb->fix.smem_start = dst_buf;

    fb->var.xres = 800;
    fb->var.yres = 480;

    fb->var.red.offset = 16;
    fb->var.red.length = 8;
    fb->var.red.msb_right = 0;

    fb->var.green.offset = 8;
    fb->var.green.length = 8;
    fb->var.red.msb_right = 0;

    fb->var.blue.offset = 8;
    fb->var.blue.length = 8;
    fb->var.blue.msb_right = 0;

    fb->var.bits_per_pixel = 32;

    direct_fb_show(fb);    
    }
#endif
}



/* rotate 30??*/
void rga_test_rotate()
{
    struct rga_req req;

    unsigned int  *src_addr = NULL;
    unsigned int  *dst_addr = NULL;

    RECT clip;

    clip.xmin = 0;
    clip.xmax = 799;
    clip.ymin = 0;
    clip.ymax = 479;
    
    RGA_set_src_act_info(&req, 320, 240, 0, 0);
#if defined(__arm64__) || defined(__aarch64__)
    RGA_set_src_vir_info(&req, (unsigned long)src_addr, 0, 0, 320, 240, RK_FORMAT_RGBA_8888, 0);
#else
    RGA_set_src_vir_info(&req, (unsigned int)src_addr, 0, 0, 320, 240, RK_FORMAT_RGBA_8888, 0);
#endif

    RGA_set_dst_act_info(&req, 320, 240, 200, 100);
#if defined(__arm64__) || defined(__aarch64__)
    RGA_set_dst_vir_info(&req, (unsigned long)dst_addr, 0, 0, 800, 480, &clip, RK_FORMAT_RGBA_8888, 0);
#else
    RGA_set_dst_vir_info(&req, (unsigned int)dst_addr, 0, 0, 800, 480, &clip, RK_FORMAT_RGBA_8888, 0);
#endif
    RGA_set_bitblt_mode(&req, bilinear, BB_ROTATE, 30, 0, ENABLE, 0);

    RGA_set_mmu_info(&req, 1, 0, 0, 0, 0, 2);

#if 0

    {
    fb_info *fb = get_fb(0);

    fb->fix.smem_start = dst_buf;

    fb->var.xres = 800;
    fb->var.yres = 480;

    fb->var.red.offset = 16;
    fb->var.red.length = 8;
    fb->var.red.msb_right = 0;

    fb->var.green.offset = 8;
    fb->var.green.length = 8;
    fb->var.red.msb_right = 0;

    fb->var.blue.offset = 8;
    fb->var.blue.length = 8;
    fb->var.blue.msb_right = 0;

    fb->var.bits_per_pixel = 32;

    direct_fb_show(fb);    
    }
#endif
}








