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
#include <stdio.h>
#include <math.h>
#include <cutils/log.h>

#include "MmaSensor.h"
#include "mma8452_kernel.h"

#if defined(ANGLE_SUPPORT)
static int sAngleFd = -1;
static int sAccFd = -1;
static int sCtrlFd = -1;
static int sCountAngle[2] = {0,0};
static int sKeyCtrl = -1;
#define DISABLE_KEY	0
#define ENABLE_KEY	1
#define GSENSOR_IOCTL_KEYBOARD		        _IOW(GSENSOR_IOCTL_MAGIC, 0x11, int[2] )
#define ANGLE_VALID_COUNT	5
static int angle_open_device(void)
{	
    if (sAngleFd < 0) 
    {
		sAngleFd = open("/dev/angle", O_RDWR);
		if(sAngleFd < 0)
	    {
			ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}
    }

	if (sAccFd < 0) 
    {
		sAccFd = open(MMA_DEVICE_NAME, O_RDWR);
		if(sAccFd < 0)
	    {
			ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}
    }
#if 0
	if (sCtrlFd < 0) 
    {
		sCtrlFd = open("/dev/ec", O_RDWR);
		if(sCtrlFd < 0)
	    {
			ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}
    }
#endif
	ALOGD("%s\n",__FUNCTION__);
	return 0;
}

static int angle_close_device(void)
{
    if(sAngleFd >= 0)
	{
        close(sAngleFd);
        sAngleFd = -1;
    }

	if(sAccFd >= 0)
	{
        close(sAccFd);
        sAccFd = -1;
    }
#if 0
	if(sCtrlFd >= 0)
	{
        close(sCtrlFd);
        sCtrlFd = -1;
    }
#endif	
	ALOGD("%s\n",__FUNCTION__);
	return 0;
}

static int angle_enable(int32_t handle, int en)
{	
	int sample_rate = MMA8452_RATE_12P5;
	
    if (sAngleFd < 0) 
    {
		ALOGE("%s:line=%d,error: sAngleFd=%d\n",__FUNCTION__, __LINE__, sAngleFd);
		return -1;
    }
	
	if(en)
	{
		if ( 0 > ioctl(sAngleFd, GSENSOR_IOCTL_START) ) 
		{
			ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}

		if ( 0 > ioctl(sAngleFd, GSENSOR_IOCTL_APP_SET_RATE, &sample_rate) ) 
		{
			ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}
	}
	else
	{
		if ( 0 > ioctl(sAngleFd, GSENSOR_IOCTL_CLOSE) ) 
		{
			ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}

	}

	ALOGD("%s: handle=%d, en=%d\n",__FUNCTION__, handle, en);

	return 0;
}


static int angle_get_acc_data(float angleData[3], float accData[3])
{
    struct sensor_axis angle = {0, 0, 0};
    struct sensor_axis acc = {0, 0, 0};
	
	if (sAngleFd < 0) 
    {
		ALOGE("%s:line=%d,error: sAngleFd=%d\n",__FUNCTION__, __LINE__, sAngleFd);
		return -1;
    }

	if (sAccFd < 0) 
    {
		ALOGE("%s:line=%d,error: sAngleFd=%d\n",__FUNCTION__, __LINE__, sAngleFd);
		return -1;
    }
	
	if ( 0 > ioctl(sAngleFd, GSENSOR_IOCTL_GETDATA, &angle) )
	{
		ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}

	if ( 0 > ioctl(sAccFd, GSENSOR_IOCTL_GETDATA, &acc) )
	{
		ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}

    angleData[0] = ( (angle.y) * ACCELERATION_RATIO_ANDROID_TO_HW);
    angleData[1] = ( -(angle.x) * ACCELERATION_RATIO_ANDROID_TO_HW);
    angleData[2] = ( (angle.z) * ACCELERATION_RATIO_ANDROID_TO_HW);

    accData[0] = ( (acc.y) * ACCELERATION_RATIO_ANDROID_TO_HW);
    accData[1] = ( -(acc.x) * ACCELERATION_RATIO_ANDROID_TO_HW);
    accData[2] = ( (acc.z) * ACCELERATION_RATIO_ANDROID_TO_HW);

	
	return 0;
}


static float angle_pitch_to_angle(float pitch, float accData)
{	
	float angle = 0.0f;
	
	if((pitch >= 0) && (accData > 0))
	{
		pitch = pitch;
	}
	else if((pitch >= 0) && (accData < 0))
	{
		pitch = 3.14159f - pitch;
	}
	else if((pitch < 0) && (accData > 0))
	{
		pitch = 6.28318f + pitch;
	}
	else if((pitch < 0) && (accData < 0))
	{
		pitch = 3.14159f - pitch;
	}

	angle = pitch * 180 / 3.14159f;	

	return angle;

}


static int angle_calc_angle(void)
{
	int ret = 0;

	float angleData[3],accData[3];
	float anglePitch, angleRoll;	
	float accPitch, accRoll;
	int angle[2];
	int keyCtrl[2] = {0,0};

	//get angle and accel data
	ret = angle_get_acc_data(angleData,accData);
	if(ret)
	{
		ALOGE("%s:line=%d,error:ret=%d\n",__FUNCTION__, __LINE__, ret);
		return -1;
	}

	//calculate angle
	anglePitch = atan2(angleData[0], sqrt((int)(angleData[1] * angleData[1]) + (int)(angleData[2] * angleData[2])));
	angleRoll = atan2(angleData[1], sqrt((int)(angleData[0] * angleData[0]) + (int)(angleData[2] * angleData[2])));

	accPitch = atan2(accData[0], sqrt((int)(accData[1] * accData[1]) + (int)(accData[2] * accData[2])));
	accRoll = atan2(accData[1], sqrt((int)(accData[0] * accData[0]) + (int)(accData[2] * accData[2])));

	anglePitch = angle_pitch_to_angle(anglePitch, angleData[2]);
	angleRoll = angle_pitch_to_angle(angleRoll, angleData[2]);

	accPitch = angle_pitch_to_angle(accPitch, accData[2]);
	accRoll = angle_pitch_to_angle(accRoll, accData[2]);
	
	if(anglePitch > accPitch)
	angle[0] = (int)(anglePitch - accPitch);
	else	
	angle[0] = (int)((anglePitch - accPitch) + 360) % 360;

	if(angleRoll > accRoll)
	angle[1] = (int)(angleRoll - accRoll);
	else
	angle[1] = (int)((angleRoll - accRoll) + 360) % 360;

	//it is gsensor inaccuracy
	if(angle[0] >= 355)
		angle[0] = fabs((360 - angle[0]));

	if(angle[1] >= 355)
		angle[1] = fabs((360 - angle[1]));
	
	//control key board
	if(fabs(angle[0]) > 185)
	{
		keyCtrl[0] = 0;	//close keyboard
		sCountAngle[DISABLE_KEY]++;
		sCountAngle[ENABLE_KEY] = 0;
	}
	else if(fabs(angle[0]) < 180)	
	{
		keyCtrl[0] = 1;	//open keyboard	
		sCountAngle[ENABLE_KEY]++;
		sCountAngle[DISABLE_KEY] = 0;
	}
	//else
	//ALOGE("%s:do nothing\n",__func__);
		
	keyCtrl[1] = angle[0];
	
	
	if((sCountAngle[DISABLE_KEY] > ANGLE_VALID_COUNT) || (sCountAngle[ENABLE_KEY] > ANGLE_VALID_COUNT))
	{
		if(sKeyCtrl != keyCtrl[0])
		{
#if 0
			if (sCtrlFd < 0) 
			{
				ALOGE("%s:line=%d,error: sAngleFd=%d\n",__FUNCTION__, __LINE__, sCtrlFd);
				return -1;
			}
			
			if ( 0 > ioctl(sCtrlFd, GSENSOR_IOCTL_KEYBOARD, keyCtrl) )
			{
				ALOGE("%s:line=%d,error=%s\n",__FUNCTION__, __LINE__, strerror(errno));
				return -1;
			}
#endif
			ALOGD("%s:angleData x=%f, y=%f, z=%f, accData x=%f, y=%f, z=%f\n", __FUNCTION__, angleData[0], angleData[1], angleData[2], accData[0],accData[1],accData[2]);	
			ALOGD("%s:anglePitch=%f, accPitch=%f, angleRoll=%f, accRoll=%f\n", __FUNCTION__, anglePitch, accPitch, angleRoll, accRoll);	
			ALOGD("%s:angle[0]=%d, angle[1]=%d\n", __FUNCTION__, angle[0], angle[1]);

			sKeyCtrl = keyCtrl[0];
		}

		if(sCountAngle[DISABLE_KEY] > ANGLE_VALID_COUNT)
			sCountAngle[DISABLE_KEY] = 0;

		if(sCountAngle[ENABLE_KEY] > ANGLE_VALID_COUNT)
			sCountAngle[ENABLE_KEY] = 0;

	}
	
	return 0;
}


#endif

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
	
#if defined(ANGLE_SUPPORT)
	angle_open_device();
#endif

    if (!mEnabled) {
        close_device();
		#if defined(ANGLE_SUPPORT)
		angle_close_device();
		#endif
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
			#if defined(ANGLE_SUPPORT)
			angle_open_device();
			#endif
        }
       
		if ( 1 == newState ) {
            I("to call 'GSENSOR_IOCTL_START'.");
			if ( 0 > (err = ioctl(dev_fd, GSENSOR_IOCTL_START) ) ) {
				E("fail to perform GSENSOR_IOCTL_START, err = %d, error is '%s'", err, strerror(errno));
				goto EXIT;
			}
            mEnabled |= (1 << what);
		}
		else {
            I("to call 'GSENSOR_IOCTL_CLOSE'.");
			if ( 0 > (err = ioctl(dev_fd, GSENSOR_IOCTL_CLOSE) ) ) {
				E("fail to perform GSENSOR_IOCTL_CLOSE, err = %d, error is '%s'", err, strerror(errno));
				goto EXIT;
			}
            mEnabled &= ~(1 << what);
		}

		#if defined(ANGLE_SUPPORT)	
		err = angle_enable(handle, en);
		if(err < 0)
		{
			ALOGE("%s:line=%d,error=%d\n",__FUNCTION__, __LINE__, err);
		}
		#endif
    }

EXIT:
	if ( !mEnabled ) {
    	close_device();
		#if defined(ANGLE_SUPPORT)
		angle_close_device();
		#endif
	}
    D("to exit : mEnabled = 0x%x.", mEnabled);
    return err;
}

int MmaSensor::setDelay(int32_t handle, int64_t ns)
{
	D("Entered : handle = 0x%x, ns = %lld.", handle, ns);
#ifdef GSENSOR_IOCTL_APP_SET_RATE
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

		if ( 0 > (result = ioctl(dev_fd, GSENSOR_IOCTL_APP_SET_RATE, &acceptable_sample_rate) ) ) {
				E("fail to perform GSENSOR_IOCTL_APP_SET_RATE, result = %d, error is '%s'", result, strerror(errno) );
		}
    }
    return result;
}

int MmaSensor::readEvents(sensors_event_t* data, int count)
{
	#if defined(ANGLE_SUPPORT)	
	int err = 0;
	#endif
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
			
			#if defined(ANGLE_SUPPORT)	
			err = angle_calc_angle();
			if(err < 0)
			{
				ALOGE("%s:line=%d,error=%d\n",__FUNCTION__, __LINE__, err);
			}
			#endif
			
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

