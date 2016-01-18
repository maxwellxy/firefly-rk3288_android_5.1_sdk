/*
 * Copyright (C) 2008 The Android Open Source Project
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

// #include <linux/akm8973.h>

#include <cutils/log.h>

#include "MmaSensor.h"

/*****************************************************************************/

MmaSensor::MmaSensor()
: SensorBase(MMA_DEVICE_NAME, "gsensor"),
      mEnabled(0),
      mPendingMask(0),
      mInputReader(32)
{
    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[Accelerometer].version = sizeof(sensors_event_t);
    mPendingEvents[Accelerometer].sensor = ID_A;
    mPendingEvents[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[Accelerometer].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    for (int i=0 ; i<numSensors ; i++)
        mDelays[i] = 200000000; // 200 ms by default

    // read the actual value of all sensors if they're enabled already
    struct input_absinfo absinfo;
    short flags = 0;

    open_device();

    if (!mEnabled) {
        close_device();
    }
}

MmaSensor::~MmaSensor() {
}

int MmaSensor::enable(int32_t handle, int en)
{
	D("Entered : handle = 0x%x, en = 0x%x.", handle, en);
    int what = -1;
    switch (handle) {
        case ID_A : 
            what = Accelerometer;
            break;
        default :
            E("invalie 'handle'.");
            return -EINVAL;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    int newState  = en ? 1 : 0;
    int err = 0;

	I("newState = 0x%x, what = 0x%x, mEnabled = 0x%x.", newState, what, mEnabled);
    if ((uint32_t(newState)<<what) != (mEnabled & (1<<what))) {
        if (!mEnabled) {
            open_device();
        }
       
		if ( 1 == newState ) {
            I("to call 'MMA_IOCTL_START'.");
			if ( 0 > (err = ioctl(dev_fd, MMA_IOCTL_START) ) ) {
				E("fail to perform MMA_IOCTL_START, err = %d, error is '%s'", err, strerror(errno));
				goto EXIT;
			}
            mEnabled |= (1 << what);
		}
		else {
            I("to call 'MMA_IOCTL_CLOSE'.");
			if ( 0 > (err = ioctl(dev_fd, MMA_IOCTL_CLOSE) ) ) {
				E("fail to perform MMA_IOCTL_CLOSE, err = %d, error is '%s'", err, strerror(errno));
				goto EXIT;
			}
            mEnabled &= ~(1 << what);
		}
    }

EXIT:
	if ( !mEnabled ) {
    	close_device();
	}
    D("to exit : mEnabled = 0x%x.", mEnabled);
    return err;
}

int MmaSensor::setDelay(int32_t handle, int64_t ns)
{
	D("Entered : handle = 0x%x, ns = %lld.", handle, ns);
#ifdef MMA_IOCTL_APP_SET_RATE
    int what = -1;
    switch (handle) {
        case ID_A: what = Accelerometer; break;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    if (ns < 0)
        return -EINVAL;

    mDelays[what] = ns;
    return update_delay();
#else
    return -1;
#endif
}

int MmaSensor::update_delay()
{
	D("Entered.");
    int result = 0;

    if (mEnabled) {
        uint64_t wanted = -1LLU;
        for (int i=0 ; i<numSensors ; i++) {
            if (mEnabled & (1<<i)) {
                uint64_t ns = mDelays[i];
                wanted = wanted < ns ? wanted : ns;
            }
        }
        short delay = int64_t(wanted) / 1000000;
		int sample_rate = (0 >= delay) ? 1000 : (1000 / delay);
		int acceptable_sample_rate = 0;
		if ( sample_rate <= 2 ) {
				acceptable_sample_rate = MMA8452_RATE_1P56;
		}
		else if ( sample_rate <= 7 ) {
				acceptable_sample_rate = MMA8452_RATE_6P25;
		}
		else if ( sample_rate <= 13 ) {
				acceptable_sample_rate = MMA8452_RATE_12P5;
		}
		else {   
				acceptable_sample_rate = MMA8452_RATE_50;
		}
		D("acceptable_sample_rate = %d", acceptable_sample_rate);

		if ( 0 > (result = ioctl(dev_fd, MMA_IOCTL_APP_SET_RATE, &acceptable_sample_rate) ) ) {
				E("fail to perform MMA_IOCTL_APP_SET_RATE, result = %d, error is '%s'", result, strerror(errno) );
		}
    }
    return result;
}

int MmaSensor::readEvents(sensors_event_t* data, int count)
{
	D("Entered : count = %d.", count);
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;       /* 已经接受的 event 的数量, 待返回. */
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        D("count = 0x%x, type = 0x%x.", count, type);
        if (type == EV_ABS) {           // #define EV_ABS 0x03
            processEvent(event->code, event->value);
            mInputReader.next();
        } else if (type == EV_SYN) {    // #define EV_SYN 0x00
            int64_t time = timevalToNano(event->time);
            for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {
                D("mPendingMask = 0x%x, j = %d; (mPendingMask & (1<<j)) = 0x%x", mPendingMask, j, (mPendingMask & (1<<j)) );
                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvents[j].timestamp = time;
                    D( "mEnabled = 0x%x, j = %d; mEnabled & (1<<j) = 0x%x.", mEnabled, j, (mEnabled & (1 << j) ) );
                    if (mEnabled & (1<<j)) {
                        *data++ = mPendingEvents[j];
                        count--;
                        numEventReceived++;
                    }
                }
            }
            if (!mPendingMask) {
                mInputReader.next();
            }
        } else {
            LOGE("MmaSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
            mInputReader.next();
        }
    }

    return numEventReceived;
}

void MmaSensor::processEvent(int code, int value)
{
	D("Entered : code = 0x%x, value = 0x%x.", code, value);
    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.x = value * ACCELERATION_RATIO_ANDROID_TO_HW;
            break;
        case EVENT_TYPE_ACCEL_Y:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.y = value * ACCELERATION_RATIO_ANDROID_TO_HW;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.z = value * ACCELERATION_RATIO_ANDROID_TO_HW;
            break;
    }
}

