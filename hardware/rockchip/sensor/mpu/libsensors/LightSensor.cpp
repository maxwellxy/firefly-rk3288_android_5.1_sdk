/*
 * Copyright (C) 2011 Samsung
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <pthread.h>

#include "LightSensor.h"

LightSensor::LightSensor()
    : SamsungSensorBase("/dev/lightsensor", "lightsensor-level", ABS_MISC)
{
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
    mPreviousLight = -1;
}

int LightSensor::handleEnable(int en) {
    mPreviousLight = -1;
//cvte_zxl add
    int flags = en ? 1 : 0;
    int err = 0;
    if (flags != mEnabled) {
        if (!mEnabled) {
            open_device();
        }
        err = ioctl(dev_fd, LIGHTSENSOR_IOCTL_ENABLE, &flags);
        err = err<0 ? -errno : 0;
        ALOGE_IF(err, "LIGHTSENSOR_IOCTL_ENABLE failed (%s)", strerror(-err));
        if (!err) {
            mEnabled = en ? 1 : 0;
        }
        if (!mEnabled) {
            close_device();
        }
    }
    return err;
//end of cvte_zxl add
}

bool LightSensor::handleEvent(input_event const *event) {
    if (event->value == -1) {
        return false;
    }
    mPendingEvent.light = indexToValue(event->value);
   // mPendingEvent.light = event->value;
    if (mPendingEvent.light != mPreviousLight) {
        mPreviousLight = mPendingEvent.light;
        return true;
    }
    return true;
}

//cvt_zxl modify
float LightSensor::indexToValue(size_t index) const {
    /* Driver gives a rolling average adc value.  We convert it lux levels. */
    static const float lux_value[] = {
          33.0,
          66.0,
          108.0,
          192.0,
          365.0,
          720.0,
          1080.0,
          1440.0,
    };
    if (index < 0 || index > ARRAY_SIZE(lux_value) - 1) {
        index = 7;
    }
    return lux_value[index];
}
//end of cvt_zxl modify
