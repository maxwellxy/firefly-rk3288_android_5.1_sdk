#ifndef _FIREFLY_VGA_H
#define _FIREFLY_VGA_H

#include <linux/fb.h>
#include <linux/rk_fb.h>
#include <linux/display-sys.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <dt-bindings/rkfb/rk_fb.h>
#endif
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif

#define GPIO_HIGH 1
#define GPIO_LOW 0

/* Event */
#define LT8618_SYSFS_SRC (1 << 6)
#define LT8618_ENABLE_CTL		(LT8618_SYSFS_SRC	| 0)
#define LT8618_DISABLE_CTL		(LT8618_SYSFS_SRC	| 1)
#define LT8618_SUSPEND_CTL		(LT8618_SYSFS_SRC	| 2)
#define LT8618_RESUME_CTL		(LT8618_SYSFS_SRC	| 3)
#define LT8618_TIMER_CHECK		(LT8618_SYSFS_SRC	| 4)
#define LT8618_TIMER_DELAY		(LT8618_SYSFS_SRC	| 5)

struct lt8618_monspecs {
	struct rk_display_device	*ddev;
	unsigned int				enable;
	unsigned int				suspend;
	struct fb_videomode			*mode;
//	struct list_head			modelist;
	struct mutex lock;
	unsigned int 				mode_set;
	unsigned int mode_change;
    struct workqueue_struct *workqueue;	
    
};

struct rgb2hdmi_dev
{
	struct fb_monspecs   specs;
	struct list_head     modelist;        //monitor mode list
	int    modelen;                       //monitor mode list len
//	const struct fb_videomode  *current_mode;     //current mode
	int						video_source;
	int						property;
	int						modeNum;
	struct lt8618_monspecs *vga;
	int     gpio_lcdc;
	int     gpio_lcdc_enable;
	int     gpio_pwn;
	int     gpio_pwn_enable;
    int     gpio_hpd;
    int     gpio_hpd_active;
	int  hpd_status;
	int hpd_timer_start;
	int first_start;
	int set_mode;
	struct delayed_work work;
	#ifdef CONFIG_SWITCH
	struct switch_dev	switchdev;		//Registered switch device
	int id;
	#endif
};

struct vga_delayed_work {
	struct delayed_work work;
	struct lt8618_monspecs *vga;
	int event;
	void *data;
};

extern struct lt8618_monspecs rgb_monspecs;
#define VGA_SOURCE_EXTERN 1
#define VGA_SOURCE_INTERNAL 0

extern int firefly_rgb2hdmi_set_mode(struct rk_display_device *device, struct fb_videomode *mode);
extern int firefly_rgb2hdmi_set_enable(struct rk_display_device *device, int enable);
extern int firefly_rgb2hdmi_enable(struct rgb2hdmi_dev* ddev);
extern int firefly_rgb2hdmi_resume(struct rgb2hdmi_dev* ddev, int check_ddc);
extern int firefly_rgb2hdmi_standby(struct rgb2hdmi_dev* ddev);
extern struct delayed_work *rgb_submit_work(struct lt8618_monspecs *vga, int event, int delay, void *data);
extern void rgb2hdmi_out_power(int enable);
#endif
