#define RK30_MAX_LCDC_SUPPORT	2
#define RK30_MAX_LAYER_SUPPORT	5
#define RK_MAX_FB_SUPPORT       5
#define RK_WIN_MAX_AREA		    4
#define RK_MAX_BUF_NUM       	11

#define u32 unsigned int
#define u8  unsigned char
#define u16 unsigned  short

/*ALPHA BLENDING MODE*/
enum alpha_mode {               /*  Fs       Fd */
	AB_USER_DEFINE     = 0x0,
	AB_CLEAR    	   = 0x1,/*  0          0*/
	AB_SRC      	   = 0x2,/*  1          0*/
	AB_DST    	       = 0x3,/*  0          1  */
	AB_SRC_OVER   	   = 0x4,/*  1   	    1-As''*/
	AB_DST_OVER    	   = 0x5,/*  1-Ad''   1*/
	AB_SRC_IN    	   = 0x6,
	AB_DST_IN    	   = 0x7,
	AB_SRC_OUT    	   = 0x8,
	AB_DST_OUT    	   = 0x9,
	AB_SRC_ATOP        = 0xa,
	AB_DST_ATOP    	   = 0xb,
	XOR                = 0xc,
	AB_SRC_OVER_GLOBAL = 0xd
}; /*alpha_blending_mode*/

struct rk_lcdc_post_cfg{
	u32 xpos;
	u32 ypos;
	u32 xsize;
	u32 ysize;
};

struct rk_fb_area_par {
	u8  data_format;        /*layer data fmt*/
	short ion_fd;
	unsigned int phy_addr;
	short acq_fence_fd;
	u16  x_offset;
	u16  y_offset;
	u16 xpos;		/*start point in panel  --->LCDC_WINx_DSP_ST*/
	u16 ypos;
	u16 xsize;		/* display window width/height  -->LCDC_WINx_DSP_INFO*/
	u16 ysize;
	u16 xact;		/*origin display window size -->LCDC_WINx_ACT_INFO*/
	u16 yact;
	u16 xvir;		/*virtual width/height     -->LCDC_WINx_VIR*/
	u16 yvir;
	u8  fbdc_en;
	u8  fbdc_cor_en;
	u8  fbdc_data_format;
	u16 reserved0;
	u32 reserved1;
};

struct rk_fb_win_par {
	u8  win_id;
	u8  z_order;		/*win sel layer*/
	u8  alpha_mode;
	u16 g_alpha_val;
	u8  mirror_en;
	struct rk_fb_area_par area_par[RK_WIN_MAX_AREA];
	u32 reserved0;
};

struct rk_fb_win_cfg_data {
	u8  wait_fs;
	short ret_fence_fd;
	short rel_fence_fd[RK_MAX_BUF_NUM];
	struct  rk_fb_win_par win_par[RK30_MAX_LAYER_SUPPORT];
	struct  rk_lcdc_post_cfg post_cfg;
};

