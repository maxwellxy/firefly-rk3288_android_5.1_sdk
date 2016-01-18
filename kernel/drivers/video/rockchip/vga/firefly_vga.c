
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/rtc.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/irqdomain.h>
#include <linux/rk_fb.h>
#include "firefly_vga.h"
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#define DDC_I2C_RATE		50*1000
#define EDID_LENGTH 128

#define DEFAULT_MODE      10

extern  struct fb_videomode sda7123_vga_mode[];
extern int get_vga_mode_len(void);

struct fb_videomode *default_modedb = sda7123_vga_mode;

struct vga_ddc_dev *ddev = NULL;
extern int firefly_register_display_vga(struct device *parent);

static struct i2c_client *gClient = NULL;


struct timer_list timer_vga_ddc;

static int i2c_master_reg8_send(const struct i2c_client *client, const char reg, const char *buf, int count, int scl_rate)
{
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;
	int ret;
	char *tx_buf = (char *)kzalloc(count + 1, GFP_KERNEL);
	if(!tx_buf)
		return -ENOMEM;
	tx_buf[0] = reg;
	memcpy(tx_buf+1, buf, count); 

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = count + 1;
	msg.buf = (char *)tx_buf;
	msg.scl_rate = scl_rate;

	ret = i2c_transfer(adap, &msg, 1);
	kfree(tx_buf);
	return (ret == 1) ? count : ret;

}

static int i2c_master_reg8_recv(const struct i2c_client *client, const char reg, char *buf, int count, int scl_rate)
{
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msgs[2];
	int ret;
	char reg_buf = reg;
	
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 1;
	msgs[0].buf = &reg_buf;
	msgs[0].scl_rate = scl_rate;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].len = count;
	msgs[1].buf = (char *)buf;
	msgs[1].scl_rate = scl_rate;

	ret = i2c_transfer(adap, msgs, 2);

	return (ret == 2)? count : ret;
}


static int vga_edid_i2c_read_regs(struct i2c_client *client, u8 reg, u8 buf[], unsigned len)
{
	int ret; 
	ret = i2c_master_reg8_recv(client, reg, buf, len, DDC_I2C_RATE);
	return ret; 
}

static int vga_edid_i2c_set_regs(struct i2c_client *client, u8 reg, u8 const buf[], __u16 len)
{
	int ret; 
	ret = i2c_master_reg8_send(client, reg, buf, (int)len, DDC_I2C_RATE);
	return ret;
}


int vga_ddc_is_ok(void)
{
    int rc = -1, i;
	char buf[8];
	if (ddev != NULL) {
	    for(i = 0; i < 3; i++) {
		    rc = vga_edid_i2c_read_regs(ddev->client, 0, buf, 8);
		    if(rc == 8) {
			    if (buf[0] == 0x00 && buf[1] == 0xff && buf[2] == 0xff && buf[3] == 0xff &&
					    buf[4] == 0xff && buf[5] == 0xff && buf[6] == 0xff && buf[7] == 0x00) {
				    //printk("vga-ddc:  is ok\n");
				    return 1;
			    } else {
			       //	printk("vga-ddc: io error");
			    }
		    }else {
			    //printk("vga-ddc: i2c  error\n");
		    }
		    mdelay(30);
	    }
	} else {
		//printk("vga-ddc:  unknown error\n");
	}
	return 0;
}

static int vga_edid_read(char *buf, int len)
{
	int rc;

	if (ddev == NULL || ddev->client == NULL)
		return -ENODEV;

	if (buf == NULL)
		return -ENOMEM;

	// Check ddc i2c communication is available or not.
	rc = vga_edid_i2c_read_regs(ddev->client, 0, buf, 6);
	if(rc == 6) {
		memset(buf, 0, len);
		// Read EDID.
		rc = vga_edid_i2c_read_regs(ddev->client, 0, buf, len);
		if(rc == len)
			return 0;
	}

	printk("unable to read EDID block.\n");
	return -EIO;
}


static int vga_parse_edid(void)
{
	struct fb_monspecs *specs = NULL;
	if (ddev == NULL) {
		return -ENODEV;
	}else {
		specs = &ddev->specs;
		//free old edid
		if (ddev->edid) {
			kfree(ddev->edid);
			ddev->edid = NULL;
		}
		ddev->edid = kzalloc(EDID_LENGTH, GFP_KERNEL);
		if (!ddev->edid)
			return -ENOMEM;

		//read edid
		if (!vga_edid_read(ddev->edid, EDID_LENGTH)) {
			//free old fb_monspecs
			if(specs->modedb)
				kfree(specs->modedb);
			memset(specs, 0, sizeof(struct fb_monspecs));

			//parse edid to fb_monspecs
			fb_edid_to_monspecs(ddev->edid, specs);
		}else {
			return -EIO;
		}
	}
	printk("vga-ddc: read and parse vga edid success.\n");
	return 0;
}


static void vga_set_modelist(void)
{
	int i, j = 0, modelen = 0;
	struct fb_videomode *mode = NULL;
	struct list_head	*modelist  = &ddev->modelist;
	struct fb_monspecs	*specs = &ddev->specs;
	int pixclock;

	fb_destroy_modelist(modelist);

	for(i = 1; i <= specs->modedb_len; i++) {
		mode = &specs->modedb[i % specs->modedb_len];	
		//printk("%d %d %d %d %d %d %d %d %d %d %d %d %d\n",mode->refresh,mode->xres,mode->yres,mode->pixclock,mode->left_margin,mode->right_margin,mode->upper_margin, \
		 //  mode->lower_margin,mode->hsync_len,mode->vsync_len, mode->sync,mode->vmode,mode->flag);
		pixclock = PICOS2KHZ(mode->pixclock);
		if (pixclock < (specs->dclkmax / 1000)) {
			for (j = 0; j < get_vga_mode_len(); j++) {
				if (default_modedb[j].xres  == mode->xres &&
						default_modedb[j].yres == mode->yres &&
						    (default_modedb[j].refresh == mode->refresh ||
							  default_modedb[j].refresh == mode->refresh + 1 ||
							    default_modedb[j].refresh == mode->refresh -1 )) {
					fb_add_videomode(&default_modedb[j], modelist);
					modelen++;
					break;
				}
			}
		}
	}
	
	ddev->modelen = modelen;
}

static void vga_set_default_modelist(void)
{
	int i, j = 0, modelen = 0;
	struct fb_videomode *mode = NULL;
	struct list_head	*modelist  = &ddev->modelist;
	struct fb_monspecs	*specs = &ddev->specs;
	int pixclock;

	fb_destroy_modelist(modelist);
    modelen = get_vga_mode_len();
	for(i = 0; i < modelen; i++) {
		fb_add_videomode(&default_modedb[i], modelist);
	}
	
	ddev->modelen = modelen;
}

struct fb_videomode *vga_find_max_mode(void)
{
	struct fb_videomode *mode = NULL/*, *nearest_mode = NULL*/;
	struct fb_monspecs *specs = NULL;
	int i, pixclock;
	
	if (ddev == NULL)
		return NULL;

	specs = &ddev->specs;
	if(specs->modedb_len) {

		/* Get max resolution timing */
		mode = &specs->modedb[0];
		
		for (i = 0; i < specs->modedb_len; i++) {
			if(specs->modedb[i].xres > mode->xres)
				mode = &specs->modedb[i];
			else if( (specs->modedb[i].xres == mode->xres) && (specs->modedb[i].yres > mode->yres) )
				mode = &specs->modedb[i];
		}

		// For some monitor, the max pixclock read from EDID is smaller
		// than the clock of max resolution mode supported. We fix it.
		pixclock = PICOS2KHZ(mode->pixclock);
		pixclock /= 250;
		pixclock *= 250;
		pixclock *= 1000;
		if(pixclock == 148250000)
			pixclock = 148500000;
		if(pixclock > specs->dclkmax)
			specs->dclkmax = pixclock;


		printk("vga-ddc: max mode %dx%d@%d[pixclock-%ld KHZ]\n", mode->xres, mode->yres,
				mode->refresh, PICOS2KHZ(mode->pixclock));
	}

	return mode;
}


static struct fb_videomode *vga_find_best_mode(void)
{
	int res = -1,i;
	struct fb_videomode *mode = NULL, *best = NULL;

	res = vga_parse_edid();
    for(i = 0; i < 2 && res != 0; i++) {
	    mdelay(30);
	    res = vga_parse_edid();
    }
    
	if (res == 0) {
		mode = vga_find_max_mode();
		if (mode) {
			 vga_set_modelist();
			 best = (struct fb_videomode *)fb_find_nearest_mode(mode, &ddev->modelist);
		}
	} else {
	    vga_set_default_modelist();
		printk("vga-ddc: read and parse edid failed errno:%d.\n", res);
	}
	
	best  = vga_find_max_mode();
	return best;
}

int vga_switch_default_screen(void)
{
	int i, mode_num = DEFAULT_MODE;
	const struct fb_videomode *mode = NULL;
	static int init_flag = 0;
	
	if (ddev == NULL) {
		printk("vga-ddc: No DDC Dev.\n");
		return -ENODEV;
	}
	
	mode = vga_find_best_mode();
	if (mode) {
		printk("vga-ddc: best mode %dx%d@%d[pixclock-%ld KHZ]\n", mode->xres, mode->yres,
				mode->refresh, PICOS2KHZ(mode->pixclock));
	    for(i = 0; i < get_vga_mode_len(); i++)
	    {
		    if(fb_mode_is_equal(&sda7123_vga_mode[i], mode))
		    {	
			   mode_num = i + 1;
			   break;
		    }
	    }
	}

	return mode_num;
}

EXPORT_SYMBOL(vga_switch_default_screen);



static int firefly_fb_event_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct fb_event *event = data;
	int blank_mode = *((int *)event->data);
	struct delayed_work *delay_work;

	if (action == FB_EARLY_EVENT_BLANK) {
		switch (blank_mode) {
			case FB_BLANK_UNBLANK:
				break;
			default:
				if(!ddev->vga->suspend) {
					delay_work = vga_submit_work(ddev->vga, VGA_SUSPEND_CTL, 0, NULL);
					if(delay_work)
						flush_delayed_work(delay_work);
				}
				break;
		}
	}
	else if (action == FB_EVENT_BLANK) {
		switch (blank_mode) {
			case FB_BLANK_UNBLANK:
				if(ddev->vga->suspend) {
					vga_submit_work(ddev->vga, VGA_RESUME_CTL, 0, NULL);
				}
				break;
			default:
				break;
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block firefly_fb_notifier = {
        .notifier_call = firefly_fb_event_notify,
};

void vga_out_power(int enable) 
{
#ifdef CONFIG_FIREFLY_VGA_OUT_ONLY  	
   if(enable) {
        gpio_set_value(ddev->gpio_pwn,ddev->gpio_pwn_enable);
   } else {
        gpio_set_value(ddev->gpio_pwn, !(ddev->gpio_pwn_enable));
   }
#else
   if(enable) {
        gpio_set_value(ddev->gpio_sel,ddev->gpio_sel_enable);
   } else {
        gpio_set_value(ddev->gpio_sel, !(ddev->gpio_sel_enable));
   }
#endif
}

void vga_switch_source(int source) 
{
   gpio_direction_output(ddev->gpio_sel, (source==VGA_SOURCE_INTERNAL) ? ddev->gpio_sel_enable:(!ddev->gpio_sel_enable));
}


static void vga_work_queue(struct work_struct *work)
{
	struct vga_delayed_work *vga_w =
		container_of(work, struct vga_delayed_work, work.work);
	struct sda7123_monspecs *vga = vga_w->vga;
	int event = vga_w->event;
	int modeNum;
	
	//printk("%s event %04x\n", __FUNCTION__, event);
	
	mutex_lock(&vga->lock);

	switch(event) {
		case VGA_ENABLE_CTL:
		  //  printk("%s VGA_ENABLE_CTL %d %d\n",__FUNCTION__,vga->enable,vga->suspend);
			if(!vga->enable || vga->mode_change == 1 ) {
				vga->enable = 1;
				vga->mode_change = 0;
				if(!vga->suspend) {
					firefly_vga_enable();
				    printk("VGA ENABLE\n");
				}
			}
			break;
		case VGA_DISABLE_CTL:
			if(vga->enable) {
				if(!vga->suspend) {
					firefly_vga_standby();
					printk("VGA DISABLE\n");
				}
				vga->enable = 0;
			}
			break;
		case VGA_RESUME_CTL:
			if(vga->suspend) {
	            if(vga->enable) {
		            vga->suspend = 0;
		            printk("VGA RESUME\n");
		            //rk_display_device_enable(vga->ddev);
		            firefly_vga_resume(vga_ddc_is_ok());
	            }
			}
			break;
		case VGA_SUSPEND_CTL:
			if(!vga->suspend) {
			   if(vga->enable) {
	                if(vga->ddev->ops->setenable) {
		                firefly_vga_standby();
		                printk("VGA SUSPEND\n");
		                vga->suspend = 1;
	                }
			   }
			}
			break;
		case VGA_TIMER_CHECK:
            if( vga->enable && vga_ddc_is_ok()) {
                if(ddev->ddc_check_ok == 0 && ddev->ddc_timer_start == 1) {
                    modeNum = vga_switch_default_screen();
                    ddev->ddc_check_ok = 1;
                    printk("VGA Devie connected %d\n",modeNum);
                    ddev->set_mode = 1;
                    firefly_vga_set_mode(NULL, &default_modedb[modeNum - 1]);
                    firefly_vga_enable();
                    ddev->set_mode = 0;
                }
            } else if(ddev->ddc_check_ok == 1) {
                if(vga_ddc_is_ok() == 0 || vga->enable != 1) {
                    ddev->ddc_check_ok = 0;
	                #ifdef CONFIG_SWITCH
	                if (ddev->switchdev.state) {
	                    switch_set_state(&(ddev->switchdev), 0);
	                }
	                #endif
	                vga_set_default_modelist();
                    printk("VGA Devie disconnect\n");
                } 
            }
            
            
            if(ddev->ddc_timer_start == 1) {
              vga_submit_work(ddev->vga, VGA_TIMER_CHECK, 800, NULL);
            }
			break;
		case VGA_TIMER_DELAY:
		    ddev->first_start = 0;
		    break;
		default:
			printk(KERN_ERR "HDMI: hdmi_work_queue() unkown event\n");
			break;
	}
	
	if(vga_w->data)
		kfree(vga_w->data);
	kfree(vga_w);
	
	//printk("hdmi_work_queue() - exit\n");
	mutex_unlock(&vga->lock);
}



struct delayed_work *vga_submit_work(struct sda7123_monspecs *vga, int event, int delay, void *data)
{
	struct vga_delayed_work *work;

	//printk("%s event %04x delay %d\n", __FUNCTION__, event, delay);
	
	work = kmalloc(sizeof(struct vga_delayed_work), GFP_ATOMIC);

	if (work) {
		INIT_DELAYED_WORK(&work->work, vga_work_queue);
		work->vga = vga;
		work->event = event;
		work->data = data;
		queue_delayed_work(vga->workqueue,
				   &work->work,
				   msecs_to_jiffies(delay));
	} else {
		printk(KERN_WARNING "VGA: Cannot allocate memory to "
				    "create work\n");
		return 0;
	}
	
	return &work->work;
}


static int  vga_edid_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    char buf[EDID_LENGTH];
    struct fb_monspecs specs;
    struct fb_videomode *moded;
    int ret = -1;
    int gpio, rc;
    unsigned long data;
    struct regulator * ldo;
    struct device_node *vga_node = client->dev.of_node;
	enum of_gpio_flags flag;
    
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
		return -ENODEV;

    ldo = regulator_get(NULL, "act_ldo3");
		if (ldo == NULL) {
				pr_err("\n%s get ldo3 failed\n", __func__);
		} else{
				regulator_set_voltage(ldo, 1800000, 1800000);
				ret = regulator_enable(ldo);
				if(ret != 0){
						pr_err("%s: faild to enable ldo3\n", __func__);
				} else {
						pr_info("%s: turn on ldo3 done.\n", __func__);
				}
	}
    ldo = regulator_get(NULL, "act_ldo4");
	if (ldo == NULL) {
			pr_err("\n%s get ldo failed\n", __func__);
		} else{
			regulator_set_voltage(ldo, 3300000, 3300000);
			ret = regulator_enable(ldo);
			if(ret != 0){
					pr_err("%s: faild to enable ldo4.\n", __func__);
			} else {
					pr_info("%s: turn on ldo done.\n", __func__);
	        }
	}
	    ldo = regulator_get(NULL, "act_ldo2");
	if (ldo == NULL) {
		pr_err("\n%s get ldo2 failed\n", __func__);
	} else{
			regulator_set_voltage(ldo, 1000000, 1000000);
			ret = regulator_enable(ldo);
			if(ret != 0){
					pr_err("%s: faild to enable ldo2\n", __func__);
			} else {
					pr_info("%s: turn on ldo2 done.\n", __func__);
			}
	}
	ldo = regulator_get(NULL, "act_ldo8");
	if (ldo == NULL) {
			pr_err("\n%s get ldo8 failed\n", __func__);
	} else{
			regulator_set_voltage(ldo, 1800000, 1800000);
			ret = regulator_enable(ldo);
			if(ret != 0){
				pr_err("%s: faild to enable ldo8.\n", __func__);
			} else {
				pr_info("%s: turn on ldo done.\n", __func__);
			}
	}

	ddev = kzalloc(sizeof(struct vga_ddc_dev), GFP_KERNEL);
	if (ddev == NULL) 
		return -ENOMEM;
	
    INIT_LIST_HEAD(&ddev->modelist);
	ddev->client = client;

    gpio = of_get_named_gpio_flags(vga_node,"gpio-pwn", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid gpio-pwn: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "vga_pwn");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	ddev->gpio_pwn = gpio;
	ddev->gpio_pwn_enable = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
#ifdef CONFIG_FIREFLY_VGA_OUT_ONLY
	gpio_direction_output(ddev->gpio_pwn, !(ddev->gpio_pwn_enable));
#else	
    gpio_direction_output(ddev->gpio_pwn, ddev->gpio_pwn_enable);
    
	gpio = of_get_named_gpio_flags(vga_node,"gpio-sel", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid gpio-sel: %d\n",gpio);
		return -1;
	} 
   ret = gpio_request(gpio, "vga_sel");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	ddev->gpio_sel = gpio;
	ddev->gpio_sel_enable = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	gpio_direction_output(ddev->gpio_sel, ddev->gpio_sel_enable);
#endif

	of_property_read_u32(vga_node, "rockchip,source", &(rc));
	ddev->video_source = rc;
	of_property_read_u32(vga_node, "rockchip,prop", &(rc));
	ddev->property = rc - 1;
	
    ddev->modeNum = vga_switch_default_screen();
    
#ifndef CONFIG_FIREFLY_VGA_OUT_ONLY    
    vga_switch_source(VGA_SOURCE_EXTERN); 
#endif    

    printk("%s: success. %d \n", __func__,ddev->modeNum);
    

    
    //setup_timer(&timer_vga_ddc, vga_ddc_timer, data);
    //INIT_DELAYED_WORK(&ddev->work, vga_ddc_timer);
    //schedule_work(&chip->work);
    memset(&vga_monspecs, 0, sizeof(struct sda7123_monspecs));
    
    ddev->vga = &vga_monspecs;
    
    mutex_init(&ddev->vga->lock);
    
	ddev->vga->workqueue = create_singlethread_workqueue("vga");
	if (ddev->vga->workqueue == NULL) {
		printk(KERN_ERR "vga,: create workqueue failed.\n");
		goto failed_1;
	}

    ddev->first_start = 1;
    ddev->set_mode = 0;

	firefly_register_display_vga(&client->dev);
	
	fb_register_client(&firefly_fb_notifier);
	
	#ifdef CONFIG_SWITCH
	ddev->switchdev.name="vga";
	switch_dev_register(&(ddev->switchdev));
	#endif
	
	vga_submit_work(ddev->vga, VGA_TIMER_DELAY, 8000, NULL);
	
	return 0;
failed_1:
	return ret;
}

static int  vga_edid_remove(struct i2c_client *client)
{
	if(ddev->edid)
		kfree(ddev->edid);
	if (ddev->specs.modedb)
		kfree(ddev->specs.modedb);
	kfree(ddev);
	#ifdef CONFIG_SWITCH
	switch_dev_unregister(&(ddev->switchdev));
	//kfree(ddev->switchdev.name);
	#endif
	return 0;
}



#ifdef CONFIG_PM
static int vga_control_suspend(struct device *dev)
{
	int ret;
	struct regulator *ldo;
    mutex_lock(&ddev->vga->lock);
	ldo = regulator_get(NULL, "act_ldo3");
	if (ldo == NULL) {
		pr_err("%s get ldo3 failed\n", __func__);
	} else {
		if(regulator_is_enabled(ldo)) {
			ret = regulator_disable(ldo);
			if(ret != 0)
				pr_err("%s: faild to disableldo3\n", __func__);
			else
				pr_info("turn off ldo3 done.\n");
		} else 
			pr_warn("is disabled before disable ldo3");
		regulator_put(ldo);
	}
	ldo = regulator_get(NULL, "act_ldo4");
	if (ldo == NULL)
		pr_err("\n%s get ldo4 failed \n", __func__);
	else {
		if(regulator_is_enabled(ldo)) {
			ret = regulator_disable(ldo);
			if(ret != 0)
				pr_err("%s: faild to disable ldo4.\n", __func__);
			else
				pr_info("turn off ldo4 done.\n");
		} else
			pr_warn("is disabled before disable ldo4");
		regulator_put(ldo);
	}
	ldo = regulator_get(NULL, "act_ldo2");
	if (ldo == NULL)
		pr_err("\n%s get ldo2 failed \n", __func__);
    else {
		if(regulator_is_enabled(ldo)) {
			ret = regulator_disable(ldo);
			if(ret != 0)
				pr_err("%s: faild to disable  ldo2.\n", __func__);
			else
				pr_info("turn off ldo2 done.\n");
		}else	
			pr_warn("is disabled before disable ldo2");
	regulator_put(ldo);
	}
	ldo = regulator_get(NULL, "act_ldo8");
	if (ldo == NULL)
		pr_err("\n%s get ldo8 failed \n", __func__);
	else {
	if(regulator_is_enabled(ldo)) {
			ret = regulator_disable(ldo);
			if(ret != 0)
				pr_err("%s: faild to disable  ldo8.\n", __func__);
			else
				pr_info("turn off ldo8 done.\n");
		} else 
			pr_warn("is disabled before disable ldo8");
		regulator_put(ldo);
	}
	mutex_unlock(&ddev->vga->lock);
	return 0;
}

static int vga_control_resume(struct device *dev)
{
	int ret;
    struct regulator * ldo;
    mutex_lock(&ddev->vga->lock);
		ldo = regulator_get(NULL, "act_ldo3");
		if (ldo == NULL) {
				pr_err("\n%s get ldo3 failed\n", __func__);
		} else{
				if(!regulator_is_enabled(ldo)) {
						regulator_set_voltage(ldo, 1800000, 1800000);
						ret = regulator_enable(ldo);
						if(ret != 0){
								pr_err("%s: faild to enable ldo3\n", __func__);
						} else {
								pr_info("turn on ldo3 done.\n");
						}
				} else {
						pr_warn("ldo3 is enabled before enable ");
				}
		}
		ldo = regulator_get(NULL, "act_ldo4");
		if (ldo == NULL) {
				pr_err("\n%s get ldo failed\n", __func__);
		} else{
				if(!regulator_is_enabled(ldo)) {
						regulator_set_voltage(ldo, 3300000, 3300000);
						ret = regulator_enable(ldo);
						if(ret != 0){
								pr_err("%s: faild to enable ldo4.\n", __func__);
						} else {
								pr_info("turn on ldo done.\n");
						}
				} else {
						pr_warn("ldo4 is enabled before enable\n");
				}
		}
	   ldo = regulator_get(NULL, "act_ldo2");
		if (ldo == NULL) {
				pr_err("\n%s get ldo2 failed\n", __func__);
		} else{
				if(!regulator_is_enabled(ldo)) {
						regulator_set_voltage(ldo, 1000000, 1000000);
						ret = regulator_enable(ldo);
						if(ret != 0){
								pr_err("%s: faild to enable ldo2.\n", __func__);
						} else {
								pr_info("turn on ldo done.\n");
						}
				} else {
						pr_warn("ldo2 is enabled before enable\n");
				}
		}
		ldo = regulator_get(NULL, "act_ldo8");
		if (ldo == NULL) {
				pr_err("\n%s get ldo failed\n", __func__);
		} else{
				if(!regulator_is_enabled(ldo)) {
						regulator_set_voltage(ldo, 1800000, 1800000);
						ret = regulator_enable(ldo);
						if(ret != 0){
								pr_err("%s: faild to enable ldo8.\n", __func__);
						} else {
								pr_info("turn on ldo done.\n");
						}
				} else {
						pr_warn("ldo8 is enabled before enable\n");
				}
		}
	mutex_unlock(&ddev->vga->lock);
return 0;
}
#endif





static const struct dev_pm_ops firefly_vba_pm_ops = {
    //SET_RUNTIME_PM_OPS(rockchip_i2s_suspend_noirq,
    //                        rockchip_i2s_resume_noirq, NULL)
    .suspend_late = vga_control_suspend,
    .resume_early = vga_control_resume,
};


static const struct i2c_device_id vga_edid_id[] = {
	{ "vga_edid", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, vga_edid_id);

static struct of_device_id rtc_dt_ids[] = {
	{ .compatible = "firefly,vga_ddc" },
	{},
};

struct i2c_driver vga_edid_driver = {
	.driver		= {
		.name	= "vga_edid",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(rtc_dt_ids),
#ifdef CONFIG_PM		
	    .pm	= &firefly_vba_pm_ops,
#endif	
	},
	.probe		= vga_edid_probe,
	.remove		= vga_edid_remove,
	.id_table	= vga_edid_id,
};

static int __init vga_edid_init(void)
{
	return i2c_add_driver(&vga_edid_driver);
}

static void __exit vga_edid_exit(void)
{
	i2c_del_driver(&vga_edid_driver);
}


late_initcall(vga_edid_init);
module_exit(vga_edid_exit);

MODULE_AUTHOR("teefirefly@gmail.com");
MODULE_DESCRIPTION("Firefly vga edid driver");
MODULE_LICENSE("GPL");


