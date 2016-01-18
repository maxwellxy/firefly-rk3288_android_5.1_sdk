
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <utils/Log.h>  //add by yxj for debug

#include "fb_test.h"

#undef LOG_TAG
#define LOG_TAG "Fb"


#define FB_PATH "/dev/graphics/fb0"

#ifdef SOFIA3GR_PCBA
	#define HZK_PATH "/system/etc/images/HZK16"
#else
	#define HZK_PATH "/res/images/HZK16"
#endif

#define MIN(x,y)  (((x)<(y))?(x):(y))
#define MAX(x,y)  (((x)>(y))?(x):(y))
int open_fb(fb_dev *fb)
{
	struct fb_fix_screeninfo fix_info;
	struct fb_var_screeninfo var_info;
	int fb_fd = 0;
	void *start_mem;
	if(fb == NULL)
	{
		LOGE("Illegal argument!\n");
		return -1 ;
	}

	fb_fd = open(FB_PATH,O_RDWR);
    if (fb_fd < 0) {
        perror("cannot open fb0");
        return -1;
    }

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix_info) < 0) {
        perror("failed to get fb0 info");
        close(fb_fd);
        return -1;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var_info) < 0) {
        perror("failed to get fb0 info");
        close(fb_fd);
        return -1;
    }

    start_mem = mmap(0, fix_info.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (start_mem == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        close(fb_fd);
        return -1;
    }
	//LOGD("xres:%d>>>yres:%d>>>>bpp:%d>>>smen_len:%d",
	//	var_info.xres,var_info.yres,var_info.bits_per_pixel,fix_info.smem_len);
	fb->width = var_info.xres;
	fb->height = var_info.yres;
	fb->bpp = var_info.bits_per_pixel ;
	fb->fb_mem = start_mem;

return 0;
	
}

int get_fb_width()
{
	return rk_fb->width;
}

int get_fb_height()
{
	return rk_fb->height;
}

int get_fb_bpp()
{
	return rk_fb->bpp;
}
unsigned short * get_fb_mem()
{
	return (unsigned short *)rk_fb->fb_mem;
}
int set_back_ground(unsigned short *fb,int r,int g,int b)
{
	int w,h;
	int i ,j;
	if(fb == NULL)
	{
		LOGE("illegal pointer!\n");
		return -1 ;
	}
	w = get_fb_width();
	h = get_fb_hight();
	
	for(i=0;i<h;i++)
	{
		for(j=0;j < w; j++)
		{
			fb[i*w + j] = ((r>>3)<<11)|((g>>6)<<5)|(b>>3);
		}
	}
	return 0;
}
int draw_dot(unsigned short * fb,int x,int y)
{
	//LOGD("%s>>>>>\n",__func__);
	int w,h;
	if(fb == NULL)
	{
		LOGE("illegal pointer!\n");
		return -1 ;
	}

	
	w = get_fb_width();
	h = get_fb_hight();

	if(x < 0)
	{
		x = 0;
	}
	else if(x > w)
	{
		x = w;
	}

	if( y < 0)
	{
		y = 0;
	}
	else if(y > h)
	{
		y = h;
	}
	fb[y*w + x] = 0xf800; //red
	
	return 0;
}

int draw_line(unsigned short *fb, point point1,point point2)
{
	int x,y ;
	if( fb == NULL)
	{
		LOGE("malloc err");
		return -1 ;
	}

	if(point1.x < 0)
	{
		point1.x = 0;
	}
	else if(point1.x > get_fb_width())
	{
		point1.x = get_fb_width();
	}

	if(point1.y < 0)
	{
		point1.y = 0;
	}
	else if(point1.y > get_fb_hight())
	{
		point1.y = get_fb_hight();
	}

    if(point2.x < 0)
	{
		point2.x = 0;
	}
	else if(point2.x > get_fb_width())
	{
		point2.x = get_fb_width();
	}

	if(point2.y < 0)
	{
		point2.y = 0;
	}
	else if(point2.y > get_fb_hight())
	{
		point2.y = get_fb_hight();
	}
	
    for( x=MIN(point1.x,point2.x); x<=MAX(point1.x,point2.x); x++ )
    {
        y = (point2.y-point1.y)*(x-point1.x)/(point2.x-point1.x) + point1.y;
        
        draw_dot(fb, x,y);
    }
 return 0;
}

int draw_circle(unsigned short *fb,point center,unsigned int r)
{
	//LOGD("%s>>>>>>>>>\n",__func__);
	int x,y,tmp;
	if( fb == NULL)
	{
		LOGE("malloc err");
		return -1 ;
	}

	for(x=center.x - r;x<= center.x +r ; x++)
	{
		tmp  = sqrt(r*r -(x-center.x)*(x-center.x));
		y = center.y + tmp ;
		draw_dot(fb,x,y);
		y = center.y - tmp ;
		draw_dot(fb,x,y);
	}

	for(y=center.y-r; y<=center.y+r; y++)
    {
		
     	tmp = sqrt(r*r - (y-center.y)*(y-center.y));
     	x = center.x + tmp;
     	draw_dot(fb, x,y);
     	x = center.x - tmp;
     	draw_dot(fb,x,y);
    }
	
	return 0 ;
	
}

int calcula_fill_circle_data(unsigned short *fb,point center,unsigned r)
{
	//LOGD("%s>>>>>>>>>>>\n",__func__);
	int i ;
	if( fb == NULL)
	{
		LOGE("malloc err");
		return -1 ;
	}
	for( i = 0;i <= r; i++)
	{
		draw_circle(fb,center,i);
	}

	return 0 ;
}

int draw_fill_circle(unsigned short *fb,point center)
{
	unsigned short *dst ;
	 int y_offset;
	 int x_offset ;
	 int i;

	y_offset = center.y - 36;
	if(y_offset < 0)
	{
		y_offset = 0;
	}
	
	dst = fb + y_offset*get_fb_width();

	for(i = 0;i < 35*2;i++)
	{
		memcpy(dst +i*get_fb_width() + center.x,circle_buf+35*2*i,35*2*2);
	}
	
	return 0;
}

int fb_graphic_init()
{
	struct point circle_center;
	circle_center.x = 35;
	circle_center.y = 35;
	calcula_fill_circle_data(circle_buf,circle_center,35);
	return 0 ;
}

void draw_square(int x,int y)
{
	const int r = 10;
	int i,j;
	int screen_w,screen_h;
	unsigned short *fb_map;
	screen_w = get_fb_width();
	screen_h = get_fb_height();
	fb_map = (unsigned short *)get_fb_mem();
	if(x>=(screen_w-r/2))
	{
		x=screen_w -r/2;
	}
	else if(x<=r/2)
	{
		x = r/2;
	}
	 if(y>=(screen_h-r/2))
	{
		y=screen_h -r/2;
	}
	else if(y<=r/2)
	{
		y = r/2 + 1;
	}
	for(i = 0;i<r;i++)
	{
		for(j =0;j < r;j++)
			fb_map[(y-r/2-1+i)*screen_w +x-r/2+j] = 0xf800;
	}


}

int fb_test_init()
{
	int ret ;
	/*struct point point1,point2,o_point;
	point1.x = 0;
	point1.y = 0;
	point2.x = 800;
	point2.y = 480;
	o_point.x = 400;
	o_point.y = 240;*/
	unsigned short * fb_map;
	rk_fb = malloc(sizeof(fb_dev));
	if(rk_fb ==NULL )
	{
		LOGE("malloc err");
		return -1 ;
	}
	ret = open_fb(rk_fb);
	/*fb_map = get_fb_mem();
	printf("xres:%d>>>yres:%d>>>>bpp:%d>>>\n",
	rk_fb->width,rk_fb->height,rk_fb->bpp);
	set_back_ground(fb_map,0x00,0x00,0x00);
	draw_line(fb_map,point1,point2);
	sleep(3);
	draw_fill_circle(fb_map,o_point,35);*/
	return 0;
}

