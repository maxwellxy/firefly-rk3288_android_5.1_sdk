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
 * @file audio_hw_hdmi.c
 * @brief 
 * @author  RkAudio
 * @version 1.0.8
 * @date 2015-08-24
 */

#include "audio_hw_hdmi.h"

#define LOG_TAG "audio_hdmi_monitor"


/**
 * @brief rk_check_hdmi_uevents 
 *
 * @param buf
 * @param len
 */
void rk_check_hdmi_uevents(const char *buf,int len)
{
    if (!strcmp(buf, "change@/devices/virtual/switch/hdmi"))
	{   
	    ALOGD("audio hardware hdmi hotplug event");
	    usleep(2 * 1000 * 1000);
	    property_set("media.audio.reset", "1");
	} else if(strstr(buf, "change@/devices/virtual/display/HDMI") != NULL) {
	    ALOGD("audio hardware hdmi changed event");
	    usleep(2 * 1000 * 1000);
	    property_set("media.audio.reset", "1");
    }
}

/**
 * @brief rk_handle_uevents 
 *
 * @param buff
 * @param len
 */
void rk_handle_uevents(const char *buff,int len)
{
    rk_check_hdmi_uevents(buff,len);
}

/**
 * @brief audio_hdmi_thread 
 *
 * @param arg
 *
 * @returns 
 */
void  *audio_hdmi_thread(void *arg)
{
    static char uevent_desc[4096];
    struct pollfd fds[1];
    int timeout;
    int err;
    uevent_init();
    fds[0].fd = uevent_get_fd();
    fds[0].events = POLLIN;
    timeout = 200;//ms
    memset(uevent_desc, 0, sizeof(uevent_desc));
    do {
        err = poll(fds, 1, timeout);
        if (err == -1) {
            if (errno != EINTR)
                ALOGE("event error: %m");
            continue;
        }

        if (fds[0].revents & POLLIN) {
            int len = uevent_next_event(uevent_desc, sizeof(uevent_desc) - 2);
            rk_handle_uevents(uevent_desc,len);
        }
    } while (1);

    pthread_exit(NULL);

    return NULL;
}
