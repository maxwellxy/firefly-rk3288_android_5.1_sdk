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

#ifndef ANDROID_LIGHT_SENSOR_H
#define ANDROID_LIGHT_SENSOR_H

//cvt_zxl add
#include <linux/types.h>
#include <linux/ioctl.h>
//end of cvt_zxl add

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SamsungSensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/
//cvte_zxl add
#define LIGHTSENSOR_IOCTL_MAGIC 'l'
#define LIGHTSENSOR_IOCTL_GET_ENABLED    _IOR(LIGHTSENSOR_IOCTL_MAGIC, 1, int *)
#define LIGHTSENSOR_IOCTL_ENABLE         _IOW(LIGHTSENSOR_IOCTL_MAGIC, 2, int *)
#define LIGHTSENSOR_IOCTL_DISABLE        _IOW(LIGHTSENSOR_IOCTL_MAGIC, 3, int *)
//end of cvte_zxl add

struct input_event;

class LightSensor:public SamsungSensorBase {

    float mPreviousLight;
    virtual int handleEnable(int en);
    virtual bool handleEvent(input_event const * event);
    float indexToValue(size_t index) const;
public:
    LightSensor();
};

/*****************************************************************************/

#endif  /* ANDROID_LIGHT_SENSOR_H */
