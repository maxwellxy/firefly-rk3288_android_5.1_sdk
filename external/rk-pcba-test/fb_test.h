#ifndef __FB_TEST_H_
#define __FB_TEST_H_

typedef struct fb_dev fb_dev;
typedef struct point point ;
struct fb_dev
{
	int width ;
	int height;
	void * fb_mem;
	int bpp;
};

struct point
{
	int x;
	int y;
};
struct fb_dev *rk_fb;

unsigned short circle_buf[35*2*35*2];
unsigned short * get_fb_mem();
int get_fb_height();
int get_fb_width();
int fb_graphic_init();
int fb_test_init();
void draw_square(int x,int y);
int draw_fill_circle(unsigned short * fb,point center);
#endif
