/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <errno.h>

#include <pixelflinger/pixelflinger.h>

#include "minui.h"
//#include "unicode_map.h"
#ifdef BOARD_USE_CUSTOM_RECOVERY_FONT
#include BOARD_USE_CUSTOM_RECOVERY_FONT
#else
#ifndef RK3288_PCBA_FONT
//#include "font/font_cn_18x18.h"
#include "font/font_cn_20x20.h"
//#include "font/font_cn_24x24.h"
//#include "font/font_cn_28x28.h"
//#include "font/font_cn_32x32.h"
#else
#include "font/font_cn_36x36.h"
#endif
#endif

#ifdef RECOVERY_BGRA
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_BGRA_8888
#define PIXEL_SIZE 4
#endif
#ifdef RECOVERY_RGBX
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_RGBX_8888
#define PIXEL_SIZE 4
#endif
#ifndef PIXEL_FORMAT
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_RGB_565
#define PIXEL_SIZE 2
#endif

const unsigned cw_en=10;
// #define PRINT_SCREENINFO 1 // Enables printing of screen info to log

 
#define SCREEN_FILE  "/sys/class/graphics/fb0/screen_info"
unsigned int xres,yres;


typedef struct {
    GGLSurface texture;
    unsigned char *cwidth;
    unsigned char *cheight;
    unsigned ascent;
	unsigned count;
	unsigned offset[97];
	void **fontdata;
	unsigned *unicodemap;
} GRFont;

static GGLSurface font_ftex;
static GRFont *gr_font = 0;
static GGLContext *gr_context = 0;
static GGLSurface gr_font_texture;
static GGLSurface gr_framebuffer[2];
static GGLSurface gr_mem_surface;
static unsigned gr_active_fb = 0;

static int gr_fb_fd = -1;
static int gr_vt_fd = -1;

static struct fb_var_screeninfo vi;
static struct fb_fix_screeninfo fi;

#ifdef PRINT_SCREENINFO
static void print_fb_var_screeninfo()
{
	LOGI("vi.xres: %d\n", vi.xres);
	LOGI("vi.yres: %d\n", vi.yres);
	LOGI("vi.xres_virtual: %d\n", vi.xres_virtual);
	LOGI("vi.yres_virtual: %d\n", vi.yres_virtual);
	LOGI("vi.xoffset: %d\n", vi.xoffset);
	LOGI("vi.yoffset: %d\n", vi.yoffset);
	LOGI("vi.bits_per_pixel: %d\n", vi.bits_per_pixel);
	LOGI("vi.grayscale: %d\n", vi.grayscale);
}
#endif



int get_screen_info (struct fb_var_screeninfo* lvi)
{
	FILE * pFile;
	char mystring [32];
	pFile = fopen (SCREEN_FILE, "r");
	if (pFile == NULL)
	{
		printf("Error opening file");
		return -1;
	}
	else 
	{
		if ( fgets (mystring , 32 , pFile) != NULL )
		{
			puts (mystring);
			sscanf(mystring,"xres:%d",&xres);
		}
		else
		{
			close(pFile);
			return -1;
		}

		if ( fgets (mystring , 32 , pFile) != NULL )
		{
			puts (mystring);
			sscanf(mystring,"yres:%d",&yres); 
			fclose (pFile);
		}
		else
		{
			close(pFile);
			return -1;
		}
		lvi->xres = xres;
		lvi->yres =yres;
		lvi->xres_virtual = xres;
		lvi->yres_virtual =2 * yres;
		lvi->xoffset =0;
		lvi->yoffset =yres;
		lvi->bits_per_pixel =16;
		//vi.grayscale = 755251200;
		lvi->grayscale = yres * 1024 * 1024 + xres * 256;
		return 0;
	}
	return -1;
}


static int get_framebuffer(GGLSurface *fb)
{
	int fd;
	void *bits;
	int fd_map;
	int ret;

	fd_map = open("/sys/class/graphics/fb0/map", O_RDWR);
	if (fd_map < 0) {
		perror("cannot open  map of fb0");
	}
	else
	{
		ret = write(fd_map,"201",4); // set map for fb,fb0-win1-fb1-win0
		if(ret < 0)
		{
			printf("set fb map fail:%s\n",strerror(errno));
		}
		close(fd_map);
	}
	
	fd = open("/dev/graphics/fb0", O_RDWR); //we use overy layer,fb0 for camera
	if (fd < 0) {
	perror("cannot open fb0");
	return -1;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
	perror("failed to get fb0 info");
	close(fd);
	return -1;
	}

#ifdef PRINT_SCREENINFO
	print_fb_var_screeninfo();
#endif
	get_screen_info (&vi);

	fprintf(stderr, "Pixel format: %dx%d @ %dbpp\n", vi.xres, vi.yres, vi.bits_per_pixel);

	vi.bits_per_pixel = PIXEL_SIZE * 8;
	if (PIXEL_FORMAT == GGL_PIXEL_FORMAT_BGRA_8888) {
	fprintf(stderr, "Pixel format: BGRA_8888\n");
	if (PIXEL_SIZE != 4)    fprintf(stderr, "E: Pixel Size mismatch!\n");
	vi.red.offset     = 8;
	vi.red.length     = 8;
	vi.green.offset   = 16;
	vi.green.length   = 8;
	vi.blue.offset    = 24;
	vi.blue.length    = 8;
	vi.transp.offset  = 0;
	vi.transp.length  = 8;
	} else if (PIXEL_FORMAT == GGL_PIXEL_FORMAT_RGBX_8888) {
	fprintf(stderr, "Pixel format: RGBX_8888\n");
	if (PIXEL_SIZE != 4)    fprintf(stderr, "E: Pixel Size mismatch!\n");
	vi.red.offset     = 24;
	vi.red.length     = 8;
	vi.green.offset   = 16;
	vi.green.length   = 8;
	vi.blue.offset    = 8;
	vi.blue.length    = 8;
	vi.transp.offset  = 0;
	vi.transp.length  = 8;
	} else if (PIXEL_FORMAT == GGL_PIXEL_FORMAT_RGB_565) {
#ifdef RECOVERY_RGB_565
		fprintf(stderr, "Pixel format: RGB_565\n");
		vi.blue.offset    = 0;
		vi.green.offset   = 5;
		vi.red.offset     = 11;
		vi.nonstd &= 0xffffff00;
			vi.nonstd |= GGL_PIXEL_FORMAT_RGB_565;
#else
	fprintf(stderr, "Pixel format: BGR_565\n");
		vi.blue.offset    = 11;
		vi.green.offset   = 5;
		vi.red.offset     = 0;
#endif
		if (PIXEL_SIZE != 2)    fprintf(stderr, "E: Pixel Size mismatch!\n");
		vi.blue.length    = 5;
		vi.green.length   = 6;
		vi.red.length     = 5;
	vi.blue.msb_right = 0;
	vi.green.msb_right = 0;
	vi.red.msb_right = 0;
	vi.transp.offset  = 0;
	vi.transp.length  = 0;
	}
	else
	{
	perror("unknown pixel format");
	close(fd);
	return -1;
	}

	vi.vmode = FB_VMODE_NONINTERLACED;
	vi.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

	if (ioctl(fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
	perror("failed to put fb0 info");
	close(fd);
	return -1;
	}

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
	perror("failed to get fb0 info");
	close(fd);
	return -1;
	}

	bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (bits == MAP_FAILED) {
	perror("failed to mmap framebuffer");
	close(fd);
	return -1;
	}

#ifdef RECOVERY_GRAPHICS_USE_LINELENGTH
	vi.xres_virtual = fi.line_length / PIXEL_SIZE;
#endif

	fb->version = sizeof(*fb);
	fb->width = vi.xres;
	fb->height = vi.yres;
#ifdef BOARD_HAS_JANKY_BACKBUFFER
	LOGI("setting JANKY BACKBUFFER\n");
	fb->stride = fi.line_length/2;
#else
	fb->stride = vi.xres_virtual;
#endif
	fb->data = bits;
	fb->format = PIXEL_FORMAT;
	memset(fb->data, 0, vi.yres * fb->stride * PIXEL_SIZE);

	fb++;

	fb->version = sizeof(*fb);
	fb->width = vi.xres;
	fb->height = vi.yres;
#ifdef BOARD_HAS_JANKY_BACKBUFFER
	fb->stride = fi.line_length/2;
	fb->data = (void*) (((unsigned) bits) + vi.yres * fi.line_length);
#else
	fb->stride = vi.xres_virtual;
	fb->data = (void*) (((unsigned) bits) + vi.yres * fb->stride * PIXEL_SIZE);
#endif
	fb->format = PIXEL_FORMAT;
	memset(fb->data, 0, vi.yres * fb->stride * PIXEL_SIZE);

#ifdef PRINT_SCREENINFO
	print_fb_var_screeninfo();
#endif

	return fd;
}

static void get_memory_surface(GGLSurface* ms) {
  ms->version = sizeof(*ms);
  ms->width = vi.xres;
  ms->height = vi.yres;
  ms->stride = vi.xres_virtual;
  ms->data = malloc(vi.xres_virtual * vi.yres * PIXEL_SIZE);
  ms->format = PIXEL_FORMAT;
}

static void set_active_framebuffer(unsigned n)
{
    if (n > 1) return;
    vi.yres_virtual = vi.yres * 2;
    vi.yoffset = n * vi.yres;
//    vi.bits_per_pixel = PIXEL_SIZE * 8;
    if (ioctl(gr_fb_fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("active fb swap failed");
    }
    if (ioctl(gr_fb_fd,RK_FBIOSET_CONFIG_DONE, NULL) < 0) {
        perror("set config done failed");
    }

}

void gr_flip(void)
{
    GGLContext *gl = gr_context;

    /* swap front and back buffers */
    gr_active_fb = (gr_active_fb + 1) & 1;

#ifdef BOARD_HAS_FLIPPED_SCREEN
    /* flip buffer 180 degrees for devices with physicaly inverted screens */
    unsigned int i;
    for (i = 1; i < (vi.xres * vi.yres); i++) {
        unsigned short tmp = gr_mem_surface.data[i];
        gr_mem_surface.data[i] = gr_mem_surface.data[(vi.xres * vi.yres * 2) - i];
        gr_mem_surface.data[(vi.xres * vi.yres * 2) - i] = tmp;
    }
#endif

    /* copy data from the in-memory surface to the buffer we're about
     * to make active. */
    memcpy(gr_framebuffer[gr_active_fb].data, gr_mem_surface.data,
           vi.xres_virtual * vi.yres * PIXEL_SIZE);

    /* inform the display driver */
    set_active_framebuffer(gr_active_fb);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    GGLContext *gl = gr_context;
    GGLint color[4];
    color[0] = ((r << 8) | r) + 1;
    color[1] = ((g << 8) | g) + 1;
    color[2] = ((b << 8) | b) + 1;
    color[3] = ((a << 8) | a) + 1;
    gl->color4xv(gl, color);
}

int gr_measureEx(const char *s, void* font)
{
    GRFont* fnt = (GRFont*) font;
    int total = 0;
    unsigned pos;
    unsigned off;

    if (!fnt)   fnt = gr_font;

    while ((off = *s++))
    {
        off -= 32;
        if (off < 96)
            //total += (fnt->offset[off+1] - fnt->offset[off]);
            total += (fnt->cwidth);
    }
    return total;
}

unsigned character_width(const char *s, void* pFont)
{
	GRFont *font = (GRFont*) pFont;
	unsigned off;

	/* Handle default font */
    if (!font)  font = gr_font;

	off = *s - 32;
	if (off == 0)
		return 0;

	return font->cwidth;
}

int getGBCharID(unsigned c1, unsigned c2)
{
	if (c1 >= 0xB0 && c1 <=0xF7 && c2>=0xA1 && c2<=0xFE)
	{
		return (c1-0xB0)*94+c2-0xA1;
	}
	return -1;
}
/*
int getUNICharID(unsigned short unicode)
{
	int i;
	for (i = 0; i < UNICODE_NUM; i++) {
		if (unicode == unicodemap[i]) return i;
	}
	return -1;
}
*/

struct utf8_table {
	int     cmask;
	int     cval;
	int     shift;
	long    lmask;
	long    lval;
};

static struct utf8_table utf8_table[] =
{
    {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
    {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
    {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
    {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
    {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
    {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
    {0,						       /* end of table    */}
};

int utf8_mbtowc(wchar_t *p, const char *s, int n)
{
	wchar_t l;
	int c0, c, nc;
	struct utf8_table *t;

	nc = 0;
	c0 = *s;
	l = c0;
	for (t = utf8_table; t->cmask; t++) {
		nc++;
		if ((c0 & t->cmask) == t->cval) {
			l &= t->lmask;
			if (l < t->lval)
			{	
				printf("aa\r\n");
				return -nc;
			}
			*p = l;
			return nc;
		}
		if (n <= nc)
		{
	
			return 0;
		}
		s++;
		c = (*s ^ 0x80) & 0xFF;
		if (c & 0xC0)
		{
			return -nc;}
		l = (l << 6) | c;
	}
	return -nc;

}

int getCharID(const char* s, void* pFont)
{
	unsigned i, unicode;
	GRFont *gfont = (GRFont*) pFont;
	if (!gfont)  gfont = gr_font;
	utf8_mbtowc(&unicode, s, strlen(s));
	for (i = 0; i < gfont->count; i++)
	{
		if (unicode == gfont->unicodemap[i])
		return i;
	}
	return 0;
}


int gr_textEx(int x, int y, const char *s, void* pFont)
{	
    GGLContext *gl = gr_context;
    GRFont *gfont = NULL;
    unsigned off, width, height, n;
    wchar_t ch;

    /* Handle default font */
    if (!gfont)  gfont = gr_font;
    //x += overscan_offset_x;
    //x += 0;
    //y += overscan_offset_y;
    //y += 0;
    //y -= gfont->ascent;
    // fprintf(stderr, "gr_text: x=%d,y=%d,w=%s\n", x, y, s);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

    while(*s) {
        if(*((unsigned char*)(s)) < 0x20) {
            s++;
            continue;
        }
		off = getCharID(s,NULL);
        n = utf8_mbtowc(&ch, s, strlen(s));
        if(n <= 0)
            break;
        s += n;
	
		width = gfont->cwidth[off];
		height = gfont->cheight[off];
        memcpy(&font_ftex, &gfont->texture, sizeof(font_ftex));
        font_ftex.width = width;
        font_ftex.height = height;
        font_ftex.stride = width;
        font_ftex.data = gfont->fontdata[off];
        gl->bindTexture(gl, &font_ftex);
	    gl->texCoord2i(gl, 0 - x, 0 - y);
        gl->recti(gl, x, y, x + width, y + height);
        x += width;
	}
/*
    GGLContext *gl = gr_context;
    GRFont *font = (GRFont*) pFont;
	unsigned off;
	unsigned off2;
	unsigned off3;
	int id;
	unsigned short unicode;
    if (!font)  font = gr_font;

    gl->bindTexture(gl, &font->texture);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);
    while((off = *s++)) {
	if (off < 0x80)
	{
		off -= 32;
		if (off < 96){
			if ((x + cw_en) >= gr_fb_width())
				return x;

		        gl->texCoord2i(gl, (off * font->cwidth) - x, 0 - y);
		        gl->recti(gl, x, y, x + cw_en, y + font->cheight);
		    }
			x += cw_en;
		}
		else
		{
			off2 = *s++;
			id = getGBCharID(off,off2);
			//printf("%X %X %X  %d", off, off2, off3, id);
			if (id >= 0) {
				if ((x + font->cwidth) >= gr_fb_width())
					return x;

				gl->texCoord2i(gl, ((id % 96) * font->cwidth) - x, (id / 96 + 1) * font->cheight - y);
				gl->recti(gl, x, y, x + font->cwidth, y + font->cheight);
			}
			x += font->cwidth;
		}
	}
*/
    return x;
}

int gr_textExW(int x, int y, const char *s, void* pFont, int max_width)
{
    return x;
}

int gr_textExWH(int x, int y, const char *s, void* pFont, int max_width, int max_height)
{
    return x;
}

void gr_fill(int x, int y, int w, int h)
{
    GGLContext *gl = gr_context;
    gl->disable(gl, GGL_TEXTURE_2D);
    gl->recti(gl, x, y, x + w, y + h);
}

void gr_blit(gr_surface source, int sx, int sy, int w, int h, int dx, int dy) {
    if (gr_context == NULL) {
        return;
    }

    GGLContext *gl = gr_context;
    gl->bindTexture(gl, (GGLSurface*) source);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);
    gl->texCoord2i(gl, sx - dx, sy - dy);
    gl->recti(gl, dx, dy, dx + w, dy + h);
}

unsigned int gr_get_width(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->width;
}

unsigned int gr_get_height(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->height;
}

void* gr_loadFont(const char* fontName)
{
    int fd;
    GRFont *font = 0;
    GGLSurface *ftex;
    unsigned char *bits, *rle;
    unsigned char *in, data;
    unsigned width, height;
    unsigned element;

    fd = open(fontName, O_RDONLY);
    if (fd == -1)
    {
        char tmp[128];

		#ifdef SOFIA3GR_PCBA
		sprintf(tmp, "/system/etc/fonts/%s.dat", fontName);
		#else
		sprintf(tmp, "/res/fonts/%s.dat", fontName);
		#endif
        fd = open(tmp, O_RDONLY);
        if (fd == -1)
            return NULL;
    }

    font = calloc(sizeof(*font), 1);
    ftex = &font->texture;

    read(fd, &width, sizeof(unsigned));
    read(fd, &height, sizeof(unsigned));
   // read(fd, font->offset, sizeof(unsigned) * 96);
    //font->offset[96] = width;

    bits = malloc(width * height);
    memset(bits, 0, width * height);

    unsigned pos = 0;
    while (pos < width * height)
    {
        int bit;

        read(fd, &data, 1);
        for (bit = 0; bit < 8; bit++)
        {
            if (data & (1 << (7-bit)))  bits[pos++] = 255;
            else                        bits[pos++] = 0;

            if (pos == width * height)  break;
        }
    }
    close(fd);

    ftex->version = sizeof(*ftex);
    ftex->width = width;
    ftex->height = height;
    ftex->stride = width;
    ftex->data = (void*) bits;
    ftex->format = GGL_PIXEL_FORMAT_A_8;
    font->cheight = height;
    font->ascent = height - 2;
    return (void*) font;
}

int gr_getFontDetails(void* font, unsigned* cheight, unsigned* maxwidth)
{
    GRFont *fnt = (GRFont*) font;

    if (!fnt)   fnt = gr_font;
    if (!fnt)   return -1;

    if (cheight)    *cheight = fnt->cheight;
    if (maxwidth)
    {
        int pos;
        *maxwidth = 0;
        for (pos = 0; pos < 96; pos++)
        {
            unsigned int width = fnt->cwidth;
            if (width > *maxwidth)
            {
                *maxwidth = width;
            }
        }
    }
    return 0;
}

static void gr_init_font(void)
{
    GGLSurface *ftex;
    unsigned char *bits;
    unsigned char *in, data;
    int bmp, pos;
    unsigned i, d, n;
    void** font_data;
    unsigned char *width, *height;

    gr_font = calloc(sizeof(*gr_font), 1);
    ftex = &gr_font->texture;

    //width = font.width;
    //height = font.height;
    width = malloc(font.count);
    height = malloc(font.count);
    font_data = malloc(font.count * sizeof(void*));

    for(n = 0; n < font.count; n++) {
		if (n<95) {
			font_data[n] = malloc(font.width*font.height);
			memset(font_data[n], 0, font.width*font.height);
			width[n] = font.width;
			height[n] = font.height;
		}
		else {
			font_data[n] = malloc(font.cwidth*font.cheight);
			memset(font_data[n], 0, font.cwidth * font.cheight);
			width[n] = font.cwidth;
			height[n] = font.cheight;
		}
	}


    d = 0;
    in = font.rundata;
    while((data = *in++)) {
        n = data & 0x7f;
        for(i = 0; i < n; i++, d++) {
			if (d<95*font.width*font.height) {
				bmp = d/(font.width*font.height);
				pos = d%(font.width*font.height);
			}
			else {
				bmp = (d-95*font.width*font.height)/(font.cwidth*font.cheight)+95;
				pos = (d-95*font.width*font.height)%(font.cwidth*font.cheight);
			}
            ((unsigned char*)(font_data[bmp]))[pos] = (data & 0x80) ? 0xff : 0;
        }

    }


    ftex->version = sizeof(*ftex);
    ftex->format = GGL_PIXEL_FORMAT_A_8;
    gr_font->count = font.count;
    gr_font->unicodemap = font.unicodemap;
    gr_font->cwidth = width;
    gr_font->cheight = height;
    gr_font->fontdata = font_data;
    gr_font->ascent = font.cheight - 2;

/*
    gr_font = calloc(sizeof(*gr_font), 1);
    ftex = &gr_font->texture;

    width = font.width;
    height = font.height;
    bits = malloc(width * height);
    rle = bits;

    in = font.rundata;
    while((data = *in++))
    {
        memset(rle, (data & 0x80) ? 255 : 0, data & 0x7f);
        rle += (data & 0x7f);
    }
    ftex->version = sizeof(*ftex);
    ftex->width = width;
    ftex->height = height;
    ftex->stride = width;
    ftex->data = (void*) bits;
    ftex->format = GGL_PIXEL_FORMAT_A_8;

    gr_font->cwidth = font.cwidth;
    gr_font->cheight = font.cheight;
    gr_font->ascent = font.cheight - 2;
*/
    return;
}

int gr_init(void)
{
    gglInit(&gr_context);
    GGLContext *gl = gr_context;

    gr_init_font();
    gr_vt_fd = open("/dev/tty0", O_RDWR | O_SYNC);
    if (gr_vt_fd < 0) {
        // This is non-fatal; post-Cupcake kernels don't have tty0.
    } else if (ioctl(gr_vt_fd, KDSETMODE, (void*) KD_GRAPHICS)) {
        // However, if we do open tty0, we expect the ioctl to work.
        perror("failed KDSETMODE to KD_GRAPHICS on tty0");
        gr_exit();
        return -1;
    }

    gr_fb_fd = get_framebuffer(gr_framebuffer);
    if (gr_fb_fd < 0) {
        perror("Unable to get framebuffer.\n");
        gr_exit();
        return -1;
    }

    get_memory_surface(&gr_mem_surface);

    fprintf(stderr, "framebuffer: fd %d (%d x %d)\n",
            gr_fb_fd, gr_framebuffer[0].width, gr_framebuffer[0].height);

    /* start with 0 as front (displayed) and 1 as back (drawing) */
    gr_active_fb = 0;
    set_active_framebuffer(0);
    gl->colorBuffer(gl, &gr_mem_surface);

    gl->activeTexture(gl, 0);
    gl->enable(gl, GGL_BLEND);
    gl->blendFunc(gl, GGL_SRC_ALPHA, GGL_ONE_MINUS_SRC_ALPHA);

//    gr_fb_blank(true);
//    gr_fb_blank(false);

    return 0;
}

void gr_exit(void)
{
    close(gr_fb_fd);
    gr_fb_fd = -1;

    free(gr_mem_surface.data);

    ioctl(gr_vt_fd, KDSETMODE, (void*) KD_TEXT);
    close(gr_vt_fd);
    gr_vt_fd = -1;
}

int gr_fb_width(void)
{
    return gr_framebuffer[0].width;
}

int gr_fb_height(void)
{
    return gr_framebuffer[0].height;
}

gr_pixel *gr_fb_data(void)
{
    return (unsigned short *) gr_mem_surface.data;
}

void gr_fb_blank(int blank)
{
    int ret;

    ret = ioctl(gr_fb_fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
    if (ret < 0)
        perror("ioctl(): blank");
}

int gr_get_surface(gr_surface* surface)
{
    GGLSurface* ms = malloc(sizeof(GGLSurface));
    if (!ms)    return -1;

    // Allocate the data
    get_memory_surface(ms);

    // Now, copy the data
    memcpy(ms->data, gr_mem_surface.data, vi.xres * vi.yres * vi.bits_per_pixel / 8);

    *surface = (gr_surface*) ms;
    return 0;
}

int gr_free_surface(gr_surface surface)
{
    if (!surface)
        return -1;

    GGLSurface* ms = (GGLSurface*) surface;
    free(ms->data);
    free(ms);
    return 0;
}

void gr_write_frame_to_file(int fd)
{
    write(fd, gr_mem_surface.data, vi.xres * vi.yres * vi.bits_per_pixel / 8);
}
