/*
 * Copyright (C) 2012 The Android Open Source Project
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
/**
 * @file audio_hw_hdmi.h
 * @brief 
 * @author  RKAudio
 * @version 1.0.8
 * @date 2015-08-24
 */

#ifndef AUIDO_HDMI_MONITOR_H
#define AUIDO_HDMI_MONITOR_H


#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <poll.h>
#include <linux/fb.h>
#include <sys/resource.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <hardware_legacy/uevent.h>

void rk_check_hdmi_uevents(const char *buf,int len);

void rk_handle_uevents(const char *buff,int len);

void *audio_hdmi_thread(void *arg);

#endif


