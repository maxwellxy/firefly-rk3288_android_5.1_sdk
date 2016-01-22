/*
 * Copyright (C) 2013 The Android Open Source Project
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
/**************************************************************************
* 
* File name   : led_hal
* Description : for firefly-rk3288
* Author      : qiyei2015(1273482124@qq.com) in chengdu
*
* Version       Date       Author         modefy
* 1.0           16-1-22    qiyei2015      create
*
**************************************************************************/

#include <hardware/led_hal.h>
#include <hardware/hardware.h>

#include <cutils/log.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>


#define LOG_TAG "Ledhal"

/* 1. 实现一个名为HMI的hw_module_t结构体 */

/* 2. 实现一个open函数, 它返回led_device_t结构体 */

/* 3. 实现led_device_t结构体 */

/* 参考 hardware\libhardware\modules\vibrator\vibrator.c*/

int fd;

static int led_close(struct hw_device_t* device){


	close(fd);
	return 0;
}


static int led_open(struct light_device_t* dev){

	fd = open("/dev/firefly-led",O_RDWR);
	if (fd < 0)
	{
		ALOGI("native add led_open fail,fd = %d",fd);
		return -1;
	} else
		ALOGI("native add led_open,fd = %d",fd);

	return 0;
}

static int led_ctrl(struct light_device_t* dev,int which,int status){

	int ret = -1;
	ret = ioctl(fd,status,which);
	ALOGI("led_ctrl: which = %d,status = %d,ret = %d",which,status,ret);
	return ret;
}




static struct led_device_t led_dev = {
	.common = {
	    .tag = HARDWARE_DEVICE_TAG,
		.close = led_close,
	},
	.led_open = led_open,
	.led_ctrl = led_ctrl,

};


static int led_device_open(const struct hw_module_t* module, const char* id,
		   struct hw_device_t** device){

	*device = &led_dev;
	return 0;

}



/*===========================================================================*/
/* Default Led HW module interface definition                           */
/*===========================================================================*/

static struct hw_module_methods_t led_module_methods = {
    .open = led_device_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .id = "led",
    .methods = &led_module_methods,
};

