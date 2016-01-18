#include <linux/ctype.h>
#include <linux/string.h>
#include "firefly_rgb2hdmi.h"
#include <linux/fb.h>
#include <linux/rk_fb.h>
#include <linux/display-sys.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/rtc.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/irqdomain.h>


const struct fb_videomode lt8618_video_mode[] = {
	//name				refresh		xres	yres	pixclock			h_bp	h_fp	v_bp	v_fp	h_pw	v_pw	polariry	PorI	flag(used for vic)
	{"640x480p@60Hz",	60,			640,	480,	25000000,	48,		16,		33,	   10,		 96,	2,		0,			0,		1	},
	{"800x600p@60Hz",	60,			800,	600,	40000000,	88,	    40,	    23,		1,		128,	4,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		0	},	
	{"1024x768p@60Hz",	60,			1024,	768,	65000000,	160,	24,		29,		3,		136,	6,		0,			0,		0	},
	{"1280x720p@60Hz",	60,			1280,	720,	74250000,	220,   110,  	20,		5,		 40,	5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		0	},	
	{"1280x1024p@60Hz",	60,			1280,	1024,	108000000,	248,	48,		38,		1,		112,	3,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		6	},
	{"1366x768p@60Hz",	60,			1366,	768,	85500000,	213,	70,		24,		3,		143,	3,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		0	},
	{"1440x900p@60Hz",	60,			1440,	900,	106500000,	232,	80,		25,		3,		152,	6,		FB_SYNC_VERT_HIGH_ACT,			0,		0	},
	{"1600x900p@60Hz",	60,			1600,	900,	108000000,	 96,	24,	    96,		1,		 80,	3,		FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,			0,		0	},
	{"1680x1050p@60Hz",	60,			1680,	1050,	146200000,	280,	104,	30,		3,		176,	6,		FB_SYNC_VERT_HIGH_ACT,			0,		0	},
	{"1920x1080p@60Hz",	60,			1920,	1080,	148500000,	148,	88,		36,		4,		 44,	5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		0	},
};


#define MODE_LEN  (ARRAY_SIZE(lt8618_video_mode))


int get_video_mode_len(void)
{
    return MODE_LEN;
}


struct lt8618_monspecs rgb_monspecs;


int firefly_switch_to_rgb(struct lt8618_monspecs * monspecs, struct rgb2hdmi_dev *ddev)
{	
	struct rk_screen *screen;

    struct fb_videomode* modedb = monspecs->mode;
    int tv_mode = monspecs->mode_set;
	
	if(modedb == NULL)
		return -1;
	screen =  kzalloc(sizeof(struct rk_screen), GFP_KERNEL);
	if(screen == NULL)
		return -1;
		
	printk("%s %d \n",__FUNCTION__,__LINE__);
	
	memset(screen, 0, sizeof(struct rk_screen));	
	/* screen type & face */
	screen->type = SCREEN_RGB;
	screen->face = OUT_P888;
 
	screen->mode = *modedb;
	
	/* Pin polarity */
	if(FB_SYNC_HOR_HIGH_ACT & modedb->sync)
		screen->pin_hsync = 1;
	else
		screen->pin_hsync = 0;
	if(FB_SYNC_VERT_HIGH_ACT & modedb->sync)
		screen->pin_vsync = 1;
	else
		screen->pin_vsync = 0;	
		
	screen->pin_den = 0;
	screen->pin_dclk = 1;
	
	/* Swap rule */
	screen->swap_rb = 0;
	screen->swap_rg = 0;
	screen->swap_gb = 0;
	screen->swap_delta = 0;
	screen->swap_dumy = 0;
	screen->overscan.left = 100;
	screen->overscan.top = 100;
	screen->overscan.right = 100;
	screen->overscan.bottom = 100;
	
	/* Operation function*/
	screen->init = NULL;
	screen->standby = NULL;	
	
	rk_fb_switch_screen(screen, 1 , ddev->video_source);
	
	ddev->modeNum = tv_mode;

    rgb2hdmi_out_power(1);
	
	kfree(screen);
	
	return 0;
}


int firefly_rgb2hdmi_standby(struct rgb2hdmi_dev* ddev)
{
	struct rk_screen screen;
	
	//rgb_monspecs.enable = 0;
	
	rgb2hdmi_out_power(0);
	
	screen.type = SCREEN_RGB;
	
	rk_fb_switch_screen(&screen, 0 , ddev->video_source);
	
	ddev->hpd_timer_start = 0;
	ddev->hpd_status = 0;
    //#ifdef CONFIG_SWITCH
    #if 0
    if (ddev->switchdev.state){
        switch_set_state(&(ddev->switchdev), 0);
    }
    #endif

	return 0;
}

int firefly_rgb2hdmi_enable(struct rgb2hdmi_dev* ddev)
{
    printk("%s %d start\n",__FUNCTION__,__LINE__);
    rgb_monspecs.enable = 1;
    firefly_switch_to_rgb(&rgb_monspecs, ddev);
    
#if 0
    ddev->hpd_timer_start = 1;
    if(ddev->first_start == 1 || ddev->set_mode == 0){
        rgb_submit_work(ddev->vga, LT8618_TIMER_CHECK, 600, NULL);
    }
#endif
    //#ifdef CONFIG_SWITCH
    #if 0
    switch_set_state(&(ddev->switchdev), 1);
    #endif
    //printk("%s %d exit\n",__FUNCTION__,__LINE__);
}

int firefly_rgb2hdmi_resume(struct rgb2hdmi_dev* ddev, int check_ddc)
{
    if(check_ddc) {
        rgb_monspecs.enable = 1;
        ddev->hpd_timer_start = 1;
        if(ddev->first_start == 1 || ddev->set_mode == 0){
            rgb_submit_work(ddev->vga, LT8618_TIMER_CHECK, 600, NULL);
        }
    } else {
        firefly_rgb2hdmi_enable(ddev);
    }
}

int firefly_rgb2hdmi_set_enable(struct rk_display_device *device, int enable)
{
    struct rgb2hdmi_dev *ddev = device->priv_data;
    printk("%s %d enable:%d\n",__FUNCTION__,__LINE__,enable);
	if(rgb_monspecs.suspend)
		return 0;
    if(enable == 0 && rgb_monspecs.enable)
    {
        firefly_rgb2hdmi_standby(ddev);
    }
    else if(enable == 1)
    {
        firefly_rgb2hdmi_enable(ddev);
        printk("HDMI ENABLE\n");
        //rgb_submit_work(ddev->vga, LT8618_ENABLE_CTL, 0, NULL);
    }
	return 0;
}

static int firefly_rgb2hdmi_get_enable(struct rk_display_device *device)
{
    printk("%s %d, %d\n",__FUNCTION__,__LINE__,rgb_monspecs.enable);
	return rgb_monspecs.enable;
}

static int firefly_rgb2hdmi_get_status(struct rk_display_device *device)
{
    printk("%s %d\n",__FUNCTION__,__LINE__);
	return 1;
}

static int firefly_rgb2hdmi_get_modelist(struct rk_display_device *device, struct list_head **modelist)
{
   // printk("%s %d\n",__FUNCTION__,__LINE__);
    struct rgb2hdmi_dev *ddev = device->priv_data;
	mutex_lock(&rgb_monspecs.lock);
	*modelist = &(ddev->modelist);
	mutex_unlock(&rgb_monspecs.lock);
	return 0;
}

int firefly_rgb2hdmi_set_mode(struct rk_display_device *device, struct fb_videomode *mode)
{
    struct rgb2hdmi_dev *ddev = device->priv_data;
	int i;
	
	printk("%s %d\n",__FUNCTION__,__LINE__);
	
	if(ddev->first_start == 1 && ddev->set_mode == 0) return 0;
	
	//
    
	for(i = 0; i < MODE_LEN; i++)
	{
		if(fb_mode_is_equal(&lt8618_video_mode[i], mode))
		{	
			if( ((i + 1) != ddev->modeNum) )
			{
				rgb_monspecs.mode_set = i + 1;
				rgb_monspecs.mode_change = 1;
				rgb_monspecs.mode = (struct fb_videomode *)&lt8618_video_mode[i];
			}
			printk("%s %d %d %d %d %d \n",__FUNCTION__,__LINE__,rgb_monspecs.mode->xres,rgb_monspecs.mode->yres,rgb_monspecs.mode->refresh,rgb_monspecs.mode->pixclock);
			return 0;
		}
	}
	
	return -1;
}

static int firefly_rgb2hdmi_get_mode(struct rk_display_device *device, struct fb_videomode *mode)
{
    //printk("%s %d\n",__FUNCTION__,__LINE__);
	if(mode == NULL)
	    return -1;
	*mode = *(rgb_monspecs.mode);
	return 0;
}

static struct rk_display_ops firefly_rgb2hdmi_display_ops = {
	.setenable = firefly_rgb2hdmi_set_enable,
	.getenable = firefly_rgb2hdmi_get_enable,
	.getstatus = firefly_rgb2hdmi_get_status,
	.getmodelist = firefly_rgb2hdmi_get_modelist,
	.setmode = firefly_rgb2hdmi_set_mode,
	.getmode = firefly_rgb2hdmi_get_mode,
};

static int firefly_display_rgb2hdmi_probe(struct rk_display_device *device, void *devdata)
{
    struct rgb2hdmi_dev *ddev = devdata;

    device->parent;
	device->owner = THIS_MODULE;
	strcpy(device->type, "HDMI");
	device->name = "firefly_rgb2hdmi";
	device->priority = DISPLAY_PRIORITY_VGA;
	device->property = ddev->property;
	device->priv_data = devdata;
	device->ops = &firefly_rgb2hdmi_display_ops;
	return 1;
}

static struct rk_display_driver display_firefly_rgb2hdmi = {
	.probe = firefly_display_rgb2hdmi_probe,
};

int firefly_register_display_rgb2hdmi(struct rgb2hdmi_dev* ddev, struct device *parent)
{
	int i;
	
	rgb_monspecs.mode = (struct fb_videomode *)&(lt8618_video_mode[ddev->modeNum - 1]);
	rgb_monspecs.mode_set = ddev->modeNum;

	rgb_monspecs.ddev = rk_display_device_register(&display_firefly_rgb2hdmi, parent, ddev);
	
	rk_display_device_enable(rgb_monspecs.ddev);
	
	return 0;
}
