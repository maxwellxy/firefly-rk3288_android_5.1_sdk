/*
 * LEDs driver for firefly-rk3288
 *
 * Created on: 2015/11/19
 * Author: qiyei2009	<1273482124@qq.com>
 * Copyright (C) 2015 CBPM-KX  chengdu,sichuan province in china
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/slab.h>

#define DEVICE_NAME "firefly-led"

/*定义led 设备相关数据结构体*/
struct gpio_led_info {
	struct platform_device	*pdev;
	int     power_gpio;
	int     power_enable_value;
	int     work_gpio;
	int     work_enable_value;
};

struct gpio_led_info *firefly_led;

static struct cdev led_cdev;

static int led_major = 0;
static int led_minor = 0;
static int num_led = 1;

static dev_t led_dev_num = 0;

static struct class *led_class;

static struct device *led_device;

struct device_node *led_node;


static int firefly_led_open(struct inode *inode, struct file *filp)
{

	//filp->private_data = firefly_led_info;
	
	printk(KERN_ERR"firefly_led_open\n");

	
	return 0;

}


static int firefly_led_release(struct inode *inode, struct file *filp)
{


	return 0;
}


static long firefly_led_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{

	//struct gpio_led_info *firefly_led = filp->private_data; //获取设备结构体

	switch(arg){
	case 0:
		if (command == 0)
			gpio_direction_output(firefly_led->power_gpio, !firefly_led->power_enable_value);//低电平亮
		else
			gpio_direction_output(firefly_led->power_gpio, firefly_led->power_enable_value);//低电平亮	
		break;

	case 1:
		if (command == 0)
			gpio_direction_output(firefly_led->work_gpio,!firefly_led->work_enable_value);//低电平亮
		else
			gpio_direction_output(firefly_led->work_gpio,firefly_led->work_enable_value);//低电平亮
		break;
		
	default:
		return -EINVAL;
		break;
	}

	return 0;
			

}



static struct file_operations led_fops = {
	.open	= firefly_led_open,
	.release = firefly_led_release,
	.unlocked_ioctl =firefly_led_ioctl,
	.owner		= THIS_MODULE,
};



static int gpio_led_probe(struct platform_device *pdev)
{
	int ret = -1;
	int gpio,flag;

	int result, i;
	
	
	//申请设备号
	/*
 	* Get a range of minor numbers to work with, asking for a dynamic
 	* major unless directed otherwise at load time.
 	*/
	if (led_major) {
		led_dev_num = MKDEV(led_major, led_minor);
		result = register_chrdev_region(led_dev_num, num_led,DEVICE_NAME);
	} else {
		result = alloc_chrdev_region(&led_dev_num, led_minor,num_led,DEVICE_NAME);
		led_major = MAJOR(led_dev_num);
	}
	if (result < 0) {
		printk(KERN_WARNING "firefly-led: can't get major %d\n", led_major);
		return result;
	}

	//初始化cdev 结构体
	cdev_init(&led_cdev, &led_fops);
	led_cdev.owner = THIS_MODULE;
	led_cdev.ops = & led_fops;

	//注册cdev
	result = cdev_add(&led_cdev, led_dev_num, 1);
	/* Fail gracefully if need be */
	if (result)
		printk(KERN_NOTICE "Error %d adding firefly-led",result);

		/* 创建led_drv类 */
	led_class = class_create(THIS_MODULE, "led_drv");

	/* 在led_drv类下创建/dev/LED设备，供应用程序打开设备*/
	led_device = device_create(led_class, NULL,led_dev_num, NULL, DEVICE_NAME);

	printk("LED driver init\n");

	//分配内存空间
	firefly_led = kzalloc(sizeof(struct gpio_led_info),GFP_KERNEL);
	if(!firefly_led){
		return -1;
	}

	led_node = pdev->dev.of_node;//获得LED的设备节点

	/*找到power led*/
	gpio = of_get_named_gpio_flags(led_node,"led_power", 0, (enum of_gpio_flags *)&flag);

	if(!gpio_is_valid(gpio)){
		printk(KERN_ERR"get gpio is error !%d\n",gpio);
		return -1;
	}

	ret = gpio_request(gpio,"led-power");
	
	if(ret){
		printk(KERN_ERR"gpio request is error !%d\n",ret);
		
		goto gpio_request_fail;
	}

	firefly_led->power_gpio = gpio;
	firefly_led->power_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	
	/*找到led-work*/
	gpio = of_get_named_gpio_flags(led_node, "led_work",0,(enum of_gpio_flags *)&flag);

	if(!gpio_is_valid(gpio)){
		printk(KERN_ERR"get gpio is error !%d\n",gpio);
		return -1;
	}

	ret = gpio_request(gpio,"led-work");
	
	if(ret){
		printk(KERN_ERR"gpio request is error !%d\n",ret);
		goto gpio_request_fail;
	}

	firefly_led->work_gpio = gpio;
	firefly_led->work_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	
	printk(KERN_ERR"gpio_led_probe!\n");

	return ret;
	
gpio_request_fail:
	
	gpio_free(gpio);
	ret = -EIO;
	return ret;
}


static int gpio_led_remove(struct platform_device *pdev)
{
	gpio_free(firefly_led->work_gpio);
	gpio_free(firefly_led->power_gpio);
	
	return 0;
}


static const struct of_device_id of_gpio_leds_match[] = {
	{ .compatible = "firefly,led" },
	{ /* Sentinel */ }
};


static struct platform_driver gpio_led_driver = {
	.probe		= gpio_led_probe,
	.remove		= gpio_led_remove,
	.driver		= {
		.name	= "firefly-led",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(of_gpio_leds_match),
	},
};


module_platform_driver(gpio_led_driver);

MODULE_AUTHOR("qiyei2009	<1273482124@qq.com>");
MODULE_DESCRIPTION(" LED driver for firefly-rk3288");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:leds-gpio");

