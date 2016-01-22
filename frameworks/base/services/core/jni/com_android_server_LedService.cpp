/*
 * Copyright (C) 2009 The Android Open Source Project
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
* File name   : com_android_server_LedService.cpp
* Description : for firefly-rk3288
* Author      : qiyei2015(1273482124@qq.com) in chengdu
*
* History     : Version     Date         Author         modefy
*               1.0         2016/1/19    qiyei2015      create
*
**************************************************************************/

#define LOG_TAG "LedService"

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/led_hal.h>

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

namespace android
{

static led_device_t* led_device;

jint led_open(JNIEnv *env, jobject cls)
{
	jint err;

	hw_module_t* module;
    hw_device_t* device;

	/* 1. hw_get_module */
	err = hw_get_module("led", (hw_module_t const**)&module);
	if (err == 0) {
		/* 2. get device : module->methods->open */
	    err = module->methods->open(module, NULL, &device);
	    if (err == 0) {
			/* 3. call led_open */
	        led_device = (led_device_t *)device;
	        ALOGI("hal led_open");
			return led_device->led_open(led_device);
	    } else {
	        return -1;
    	}
    }

	return -1;
}


void led_close(JNIEnv *env, jobject cls)
{

	ALOGI("hal led_close");

}


jint led_ctrl(JNIEnv *env, jobject cls,jint which,jint status)
{

	ALOGI("led led_ctrl: which = %d,status = %d",which,status);
	return led_device->led_ctrl(led_device,which,status);
}


static JNINativeMethod method_table[] = {
	{"native_ledOpen", "()I", (void *)led_open},
	{"native_ledClose", "()V", (void *)led_close},
	{"native_ledCtrl", "(II)I", (void *)led_ctrl},
};


int register_android_server_LedService(JNIEnv *env)
{
    return jniRegisterNativeMethods(env, "com/android/server/LedService",
            method_table, NELEM(method_table));
}


};
