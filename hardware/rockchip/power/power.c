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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "RKPowerHAL"
#include <utils/Log.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define INTERACTIVE_PATH "/sys/devices/system/cpu/cpufreq/interactive/"
#define BOOSTPULSE_PATH  INTERACTIVE_PATH "boostpulse"

//TODO stay the same with PowerManager.java
#define PERFORMANCE_MODE_NORMAL         0
#define PERFORMANCE_MODE_PERFORMANCE    1

static int boostpulse_fd = -1;

char property[PROPERTY_VALUE_MAX];
static int support_turn_pm_policy = 0;

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static void rk_power_init(struct power_module *module)
{
    ALOGD("version 3.0\n");

    boostpulse_fd = open(BOOSTPULSE_PATH, O_WRONLY);
    if (boostpulse_fd < 0) {
        char buf[80];
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", BOOSTPULSE_PATH, buf);
    }

    property_get("ro.board.platform", property, "unknown");
    if (!strcmp(property, "rk312x") || !strcmp(property, "rk3288")) {
        support_turn_pm_policy = 1;
    }
}

static void rk_power_set_interactive(struct power_module *module, int on)
{
    /*
     * Lower maximum frequency when screen is off. CPU 0 and 1 share a
     * cpufreq policy.
     */

    sysfs_write("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", on ? "10000000" : "816000");
    sysfs_write(INTERACTIVE_PATH "input_boost", on ? "1" : "0");
    sysfs_write("/sys/devices/system/cpu/cpu7/online", on ? "1" : "0");
    sysfs_write("/sys/devices/system/cpu/cpu6/online", on ? "1" : "0");
    sysfs_write("/sys/devices/system/cpu/cpu5/online", on ? "1" : "0");
    sysfs_write("/sys/devices/system/cpu/cpu4/online", on ? "1" : "0");
    sysfs_write("/sys/devices/system/cpu/cpu3/online", on ? "1" : "0");
    sysfs_write("/sys/devices/system/cpu/cpu2/online", on ? "1" : "0");
#ifdef POWER_POLICY_BOX
    /*box use cpu1 for response remotectrl pwm's irq.*/
#else
    sysfs_write("/sys/devices/system/cpu/cpu1/online", on ? "1" : "0");
#endif
}

static void rk_power_hint(struct power_module *module, power_hint_t hint, void *data)
{
    int mode = 0;
    switch (hint) {
    case POWER_HINT_INTERACTION:
        if (boostpulse_fd >= 0) {
            write(boostpulse_fd, "1", 1);
        }
        break;

    case POWER_HINT_VSYNC:
        break;

    case POWER_HINT_PERFORMANCE_MODE:
        mode = *(int*)data;
        //ALOGD("POWER_HINT_PERFORMANCE_MODE: %d\n", mode);
        if (PERFORMANCE_MODE_PERFORMANCE == mode) {
            if (support_turn_pm_policy) {
                ALOGD("Try to turn pm policy\n");
                sysfs_write("/sys/module/rockchip_pm/parameters/policy", "0");
            }
            sysfs_write("/dev/video_state", "p");
        } else if(PERFORMANCE_MODE_NORMAL == mode) {
            if (support_turn_pm_policy) {
                sysfs_write("/sys/module/rockchip_pm/parameters/policy", "1");
            }
            sysfs_write("/dev/video_state", "n");
        } else {
            if (support_turn_pm_policy) {
                sysfs_write("/sys/module/rockchip_pm/parameters/policy", "1");
            }
            sysfs_write("/dev/video_state", "n");
        }
        break;
    default:
        break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version: POWER_MODULE_API_VERSION_0_2,
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: POWER_HARDWARE_MODULE_ID,
        name: TARGET_BOARD_PLATFORM " Power HAL",
        author: "Rockchip",
        methods: &power_module_methods,
    },

    init: rk_power_init,
    setInteractive: rk_power_set_interactive,
    powerHint: rk_power_hint,
};
