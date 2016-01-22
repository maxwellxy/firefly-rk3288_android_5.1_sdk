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
* File name   : 
* Description : for firefly-rk3288
* Author      : qiyei2015(1273482124@qq.com) in chengdu
*
* Version       Date       Author         modefy
* 1.0           16-1-22    qiyei2015      create
*
**************************************************************************/


#ifndef ANDROID_LED_HAL_H
#define ANDROID_LED_HAL_H


#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>



struct led_device_t {
    struct hw_device_t common;

    /**
     * Set the provided leds to the provided values.
     *
     * Returns: 0 on succes, error code on failure.
     */

	int (*led_open)(struct led_device_t* dev);
	int (*led_ctrl)(struct led_device_t* dev,int which,int status);

};


#endif //ANDROID_LED_HAL_H
