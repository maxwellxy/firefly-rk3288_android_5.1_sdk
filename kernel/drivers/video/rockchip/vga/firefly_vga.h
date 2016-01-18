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
#define VGA_SYSFS_SRC (1 << 6)
#define VGA_ENABLE_CTL		(VGA_SYSFS_SRC	| 0)
#define VGA_DISABLE_CTL		(VGA_SYSFS_SRC	| 1)
#define VGA_SUSPEND_CTL		(VGA_SYSFS_SRC	| 2)
#define VGA_RESUME_CTL		(VGA_SYSFS_SRC	| 3)
#define VGA_TIMER_CHECK		(VGA_SYSFS_SRC	| 4)
#define VGA_TIMER_DELAY		(VGA_SYSFS_SRC	| 5)

struct sda7123_monspecs {
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

struct vga_ddc_dev
{
	unsigned char        *edid;
	struct i2c_client    *client;
	struct fb_monspecs   specs;
	struct list_head     modelist;        //monitor mode list
	int    modelen;                       //monitor mode list len
//	const struct fb_videomode  *current_mode;     //current mode
	int						video_source;
	int						property;
	int						modeNum;
	struct sda7123_monspecs *vga;
	int     gpio_sel;
	int     gpio_sel_enable;
	int     gpio_pwn;
	int     gpio_pwn_enable;
	int  ddc_check_ok;
	int ddc_timer_start;
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
	struct sda7123_monspecs *vga;
	int event;
	void *data;
};

extern struct vga_ddc_dev *ddev;
extern struct timer_list timer_vga_ddc;
extern struct sda7123_monspecs vga_monspecs;
#define VGA_SOURCE_EXTERN 1
#define VGA_SOURCE_INTERNAL 0
extern void vga_switch_source(int source);
extern int firefly_vga_set_mode(struct rk_display_device *device, struct fb_videomode *mode);
extern int firefly_vga_set_enable(struct rk_display_device *device, int enable);
//int firefly_register_display_vga(struct device *parent);
extern int firefly_vga_enable(void);
extern firefly_vga_resume(int check_ddc);
extern int firefly_vga_standby(void);
extern struct delayed_work *vga_submit_work(struct sda7123_monspecs *vga, int event, int delay, void *data);
extern void vga_out_power(int enable) ;
#endif
