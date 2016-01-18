
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
#include "firefly_rgb2hdmi.h"
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#define DDC_I2C_RATE		50*1000
#define EDID_LENGTH 128

#define DEFAULT_MODE      10

extern  struct fb_videomode lt8618_video_mode[];
extern int get_video_mode_len(void);

struct fb_videomode *rgb_default_modedb = lt8618_video_mode;

struct rgb2hdmi_dev *rhdev = NULL;
extern int firefly_register_display_rgb2hdmi(struct rgb2hdmi_dev *rhdev, struct device *parent);

int hdmi_hpd_det(void)
{
#if 1
    int level = gpio_get_value(rhdev->gpio_hpd);
    printk("hdmi hpd level %d \n",level);
    return level == rhdev->gpio_hpd_active ? 1 : 0;
#endif
}

static void rgb_set_default_modelist(void)
{
	int i, j = 0, modelen = 0;
	struct list_head	*modelist  = &rhdev->modelist;

	fb_destroy_modelist(modelist);
    modelen = get_video_mode_len();
	for(i = 0; i < modelen; i++) {
		fb_add_videomode(&rgb_default_modedb[i], modelist);
	}
	
	rhdev->modelen = modelen;
}

int rgb_get_default_mode(void)
{
	int i, mode_num = DEFAULT_MODE;
	
	if (rhdev == NULL) {
		printk("vga-ddc: No DDC Dev.\n");
		return -ENODEV;
	}
    //msleep(2000);

    rgb_set_default_modelist();

	return mode_num;
}

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
				if(!rhdev->vga->suspend) {
                    firefly_rgb2hdmi_standby(rhdev);
		            rhdev->vga->suspend = 1;
					//delay_work = rgb_submit_work(rhdev->vga, LT8618_SUSPEND_CTL, 0, NULL);
					//if(delay_work)
						//flush_delayed_work(delay_work);
				}
				break;
		}
	}
	else if (action == FB_EVENT_BLANK) {
		switch (blank_mode) {
			case FB_BLANK_UNBLANK:
				if(rhdev->vga->suspend) {
		            rhdev->vga->suspend = 0;
                    firefly_rgb2hdmi_resume(rhdev,0);
					//rgb_submit_work(rhdev->vga, LT8618_RESUME_CTL, 0, NULL);
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

void rgb2hdmi_out_power(int enable) 
{
   if(enable) {
        gpio_set_value(rhdev->gpio_lcdc,rhdev->gpio_lcdc_enable);
        gpio_set_value(rhdev->gpio_pwn,rhdev->gpio_pwn_enable);
   } else {
        gpio_set_value(rhdev->gpio_pwn, !(rhdev->gpio_pwn_enable));
        gpio_set_value(rhdev->gpio_lcdc,!(rhdev->gpio_lcdc_enable));
   }
}

static void rgb_work_queue(struct work_struct *work)
{
	struct vga_delayed_work *vga_w =
		container_of(work, struct vga_delayed_work, work.work);
	struct lt8618_monspecs *vga = vga_w->vga;
	int event = vga_w->event;
	int modeNum;
	
	//printk("%s event %04x\n", __FUNCTION__, event);
	
	mutex_lock(&vga->lock);

	switch(event) {
		case LT8618_ENABLE_CTL:
			if(!vga->enable || vga->mode_change == 1 ) {
				vga->enable = 1;
				vga->mode_change = 0;
				if(!vga->suspend) {
					firefly_rgb2hdmi_enable(rhdev);
				    printk("HDMI ENABLE\n");
				}
			}
			break;
		case LT8618_DISABLE_CTL:
			if(vga->enable) {
				if(!vga->suspend) {
					firefly_rgb2hdmi_standby(rhdev);
					printk("HDMI DISABLE\n");
				}
				vga->enable = 0;
			}
			break;
		case LT8618_RESUME_CTL:
			if(vga->suspend) {
	            if(vga->enable) {
		            vga->suspend = 0;
		            printk("HDMI RESUME\n");
		            firefly_rgb2hdmi_resume(rhdev,hdmi_hpd_det());
	            }
			}
			break;
		case LT8618_SUSPEND_CTL:
			if(!vga->suspend) {
			   if(vga->enable) {
	                if(vga->ddev->ops->setenable) {
		                firefly_rgb2hdmi_standby(rhdev);
		                printk("HDMI SUSPEND\n");
		                vga->suspend = 1;
	                }
			   }
			}
			break;
		case LT8618_TIMER_CHECK:
            if( vga->enable && hdmi_hpd_det()) {
                if(rhdev->hpd_status == 0 && rhdev->hpd_timer_start == 1) {
                    modeNum = rgb_get_default_mode();
                    rhdev->hpd_status = 1;
                    printk("HDMI Devie connected %d\n",modeNum);
                    rhdev->set_mode = 1;
                    firefly_rgb2hdmi_set_mode(rhdev->vga->ddev, &rgb_default_modedb[modeNum - 1]);
                    firefly_rgb2hdmi_enable(rhdev);
                    rhdev->set_mode = 0;
                }
            } else if(rhdev->hpd_status == 1) {
                if(hdmi_hpd_det() == 0 || vga->enable != 1) {
                    rhdev->hpd_status = 0;
	                #if 0
	                if (rhdev->switchdev.state) {
	                    switch_set_state(&(rhdev->switchdev), 0);
	                }
	                #endif
	                rgb_set_default_modelist();
                    printk("HDMI Devie disconnect\n");
                } 
            }
            
            
            if(rhdev->hpd_timer_start == 1) {
              rgb_submit_work(rhdev->vga, LT8618_TIMER_CHECK, 800, NULL);
            }
			break;
		case LT8618_TIMER_DELAY:
		    rhdev->first_start = 0;
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



struct delayed_work *rgb_submit_work(struct lt8618_monspecs *vga, int event, int delay, void *data)
{
	struct vga_delayed_work *work;

	//printk("%s event %04x delay %d\n", __FUNCTION__, event, delay);
	
	work = kmalloc(sizeof(struct vga_delayed_work), GFP_ATOMIC);

	if (work) {
		INIT_DELAYED_WORK(&work->work, rgb_work_queue);
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


static int  rgb2hdmi_probe(struct platform_device *pdev)
{
    char buf[EDID_LENGTH];
    struct fb_monspecs specs;
    struct fb_videomode *moded;
    int ret = -1;
    int gpio, rc;
    unsigned long data;
    struct regulator * ldo;
    struct device_node *vga_node = pdev->dev.of_node;
	enum of_gpio_flags flag;
	int modeNum;
    
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

	rhdev = kzalloc(sizeof(struct rgb2hdmi_dev), GFP_KERNEL);
	if (rhdev == NULL) {
        pr_err("%s: alloc mem for rgb2hdmi is fail\n",__func__);
		return -ENOMEM;
    }

    //platform_set_drvdata(pdev,rhdev);
	
    INIT_LIST_HEAD(&rhdev->modelist);

    gpio = of_get_named_gpio_flags(vga_node,"power-gpio", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid power-gpio: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "rgb2hdmi_pwn");
	if (ret != 0) {
		gpio_free(gpio);
		pr_err("request power-gpio: %d fail \n",gpio);
		ret = -EIO;
		goto failed_1;
	}
	rhdev->gpio_pwn = gpio;
	rhdev->gpio_pwn_enable = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;

    gpio_direction_output(rhdev->gpio_pwn, !rhdev->gpio_pwn_enable);
    
	gpio = of_get_named_gpio_flags(vga_node,"lcdc-gpio", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid lcd-gpio: %d\n",gpio);
		return -1;
	} 
   ret = gpio_request(gpio, "lcdc_gpio");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		pr_err("request lcdc-gpio: %d fail \n",gpio);
		goto failed_1;
	}
	rhdev->gpio_lcdc = gpio;
	rhdev->gpio_lcdc_enable = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	gpio_direction_output(rhdev->gpio_lcdc, !rhdev->gpio_lcdc_enable);

#if 1
	gpio = of_get_named_gpio_flags(vga_node,"hpd-gpio", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid hpd-gpio: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "hpd_gpio");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		pr_err("request hpd-gpio: %d fail \n",gpio);
		goto failed_1;
	}
	rhdev->gpio_hpd = gpio;
	rhdev->gpio_hpd_active = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	gpio_direction_input(rhdev->gpio_hpd);
#endif


	of_property_read_u32(vga_node, "rockchip,source", &(rc));
	rhdev->video_source = rc;
	of_property_read_u32(vga_node, "rockchip,prop", &(rc));
	rhdev->property = rc - 1;
	
    rhdev->modeNum = rgb_get_default_mode();
    
    printk("%s: success. %d \n", __func__,rhdev->modeNum);
    
    memset(&rgb_monspecs, 0, sizeof(struct lt8618_monspecs));
    rhdev->vga = &rgb_monspecs;
    mutex_init(&rhdev->vga->lock);
    
#if 0
	rhdev->vga->workqueue = create_singlethread_workqueue("vga");
	if (rhdev->vga->workqueue == NULL) {
		printk(KERN_ERR "vga,: create workqueue failed.\n");
		goto failed_1;
	}
#endif

    rhdev->first_start = 1;
    rhdev->set_mode = 0;

	firefly_register_display_rgb2hdmi(rhdev, &pdev->dev);
	
	fb_register_client(&firefly_fb_notifier);
	
	//#ifdef CONFIG_SWITCH
	#if 0
	rhdev->switchdev.name="vga";
	switch_dev_register(&(rhdev->switchdev));
	#endif
	
	//rgb_submit_work(rhdev->vga, LT8618_TIMER_DELAY, 8000, NULL);
    {
        modeNum = rgb_get_default_mode();
        rhdev->hpd_status = 1;
        printk("HDMI Devie connected %d\n",modeNum);
        rhdev->set_mode = 1;
        firefly_rgb2hdmi_set_mode(rhdev->vga->ddev, &rgb_default_modedb[modeNum - 1]);
        firefly_rgb2hdmi_enable(rhdev);
        rhdev->set_mode = 0;
        rhdev->first_start = 0;
    }
 
	return 0;
failed_1:
	return ret;
}

static int  rgb2hdmi_remove(struct platform_device *pdev)
{
	if (rhdev->specs.modedb)
		kfree(rhdev->specs.modedb);
	kfree(rhdev);
	//#ifdef CONFIG_SWITCH
	#if 0
	switch_dev_unregister(&(rhdev->switchdev));
	//kfree(rhdev->switchdev.name);
	#endif
	return 0;
}



#ifdef CONFIG_PM
static int rgb_control_suspend(struct device *dev)
{
	int ret;
	struct regulator *ldo;
    mutex_lock(&rhdev->vga->lock);
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
	mutex_unlock(&rhdev->vga->lock);
	return 0;
}

static int rgb_control_resume(struct device *dev)
{
	int ret;
    struct regulator * ldo;
    mutex_lock(&rhdev->vga->lock);
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
	mutex_unlock(&rhdev->vga->lock);
return 0;
}
#endif





static const struct dev_pm_ops firefly_rgb_pm_ops = {
    //SET_RUNTIME_PM_OPS(rockchip_i2s_suspend_noirq,
    //                        rockchip_i2s_resume_noirq, NULL)
    .suspend_late = rgb_control_suspend,
    .resume_early = rgb_control_resume,
};

static struct of_device_id rgb2hdmi_dt_ids[] = {
	{ .compatible = "firefly,rgb2hdmi" },
	{},
};

struct platform_driver rgb2hdmi_driver = {
	.driver		= {
		.name	= "rgb2hdmi",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(rgb2hdmi_dt_ids),
#ifdef CONFIG_PM		
	    //.pm	= &firefly_rgb_pm_ops,
#endif	
	},
	.probe		= rgb2hdmi_probe,
	.remove		= rgb2hdmi_remove,
};

static int __init rgb2hdmi_init(void)
{
    return platform_driver_register(&rgb2hdmi_driver);
}

static void __exit rgb2hdmi_exit(void)
{
    return platform_driver_unregister(&rgb2hdmi_driver);
}


late_initcall(rgb2hdmi_init);
module_exit(rgb2hdmi_exit);

MODULE_AUTHOR("teefirefly@gmail.com");
MODULE_DESCRIPTION("Firefly rgb to hdmi driver");
MODULE_LICENSE("GPL");


