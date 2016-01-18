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

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>

#include <poll.h>
#include <pthread.h>

#include <linux/input.h>

#include <cutils/atomic.h>
#include <math.h>

#include "nusensors.h"
#include "LightSensor.h"
#include "ProximitySensor.h"
#include "MmaSensor.h"
#include "AkmSensor.h"
#include "GyroSensor.h"
#include "PressureSensor.h"
#include "TemperatureSensor.h"

#if defined(CALIBRATION_SUPPORT)
typedef		unsigned short	    uint16;
typedef		unsigned long	    uint32;
typedef		unsigned char	    uint8;

#include <fcntl.h>
#include <sys/ioctl.h>

#define RKNAND_DIASBLE_SECURE_BOOT _IOW('d', 127, unsigned int)
#define RKNAND_ENASBLE_SECURE_BOOT _IOW('d', 126, unsigned int)
#define RKNAND_GET_SN_SECTOR       _IOW('d', 3, unsigned int)

#define RKNAND_GET_VENDOR_SECTOR0       _IOW('v', 16, unsigned int)
#define RKNAND_STORE_VENDOR_SECTOR0     _IOW('v', 17, unsigned int)

#define RKNAND_GET_VENDOR_SECTOR1       _IOW('v', 18, unsigned int)
#define RKNAND_STORE_VENDOR_SECTOR1     _IOW('v', 19, unsigned int)

#define DRM_KEY_OP_TAG              0x4B4D5244 // "DRMK" 
#define SN_SECTOR_OP_TAG            0x41444E53 // "SNDA"
#define DIASBLE_SECURE_BOOT_OP_TAG  0x42534444 // "DDSB"
#define ENASBLE_SECURE_BOOT_OP_TAG  0x42534E45 // "ENSB"
#define VENDOR_SECTOR_OP_TAG        0x444E4556 // "VEND"

#define RKNAND_SYS_STORGAE_DATA_LEN 512

typedef struct tagRKNAND_SYS_STORGAE
{
    uint32  tag;
    uint32  len;
    uint8   data[RKNAND_SYS_STORGAE_DATA_LEN];
}RKNAND_SYS_STORGAE;


typedef struct tagSN_SECTOR_INFO
{
    uint32 snSectag;           // "SNDA" 0x41444E53
    uint32 snSecLen;           // 512
    uint16 snLen;              // 0:no sn , 0~30,sn len
    uint8 snData[30];          // sn data
    uint32 reserved2[(0x200-0x20)/4];
}SN_SECTOR_INFO,*pSN_SECTOR_INFO;

#define MAX_COUNT_CALIBRATION 100
static int sCaliFd = -1;
static int gCountCali = 0;
static RKNAND_SYS_STORGAE gCalibrationData;
static int gAccelCaliData[3] = {0, 0, 0};


static int sensor_get_calibration_from_vendor1(void)
{
    uint32 i;
    int ret ;

    int sCaliFd = open("/dev/rknand_sys_storage",O_RDWR,0);
    if(sCaliFd < 0){
        ALOGE("open /dev/rknand_sys_storage open fail:%s\n",strerror(errno));
        return -1;
    }

    gCalibrationData.tag = VENDOR_SECTOR_OP_TAG;
    gCalibrationData.len = RKNAND_SYS_STORGAE_DATA_LEN-8;

    ret = ioctl(sCaliFd, RKNAND_GET_VENDOR_SECTOR1, &gCalibrationData);
    if(ret){
        ALOGE("get vendor_sector error:%s\n",strerror(errno));		
		close(sCaliFd);
        return -1;
    }


	if(((gCalibrationData.data[0]&0x7f) > 120) || ((gCalibrationData.data[1]&0x7f) > 120) || ((gCalibrationData.data[2]&0x7f) > 120))
	{
		ALOGE("%s:calibration data error:gCalibrationData=0x%x,0x%x,0x%x\n",__func__, gCalibrationData.data[0] & 0x7f, gCalibrationData.data[1] & 0x7f, gCalibrationData.data[2] & 0x7f);
		gAccelCaliData[0] = 0;
		gAccelCaliData[1] = 0;
		gAccelCaliData[2] = 0;
	}
	else
	{
		gAccelCaliData[0] = (gCalibrationData.data[0] & 0x80)?-(gCalibrationData.data[0]&0x7f):(gCalibrationData.data[0]&0x7f);
		gAccelCaliData[1] = (gCalibrationData.data[1] & 0x80)?-(gCalibrationData.data[1]&0x7f):(gCalibrationData.data[1]&0x7f);
		gAccelCaliData[2] = (gCalibrationData.data[2] & 0x80)?-(gCalibrationData.data[2]&0x7f):(gCalibrationData.data[2]&0x7f);

		gAccelCaliData[0] = gAccelCaliData[0]*1000;
		gAccelCaliData[1] = gAccelCaliData[1]*1000;
		gAccelCaliData[2] = gAccelCaliData[2]*1000;

	}
	
	close(sCaliFd);
	sCaliFd = -1;

	ALOGD("%s:gAccelCaliData=%d,%d,%d\n",__func__, gAccelCaliData[0], gAccelCaliData[1], gAccelCaliData[2]);
	
    return 0;
}

#endif

/*****************************************************************************/

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {		
        light           = 0,
        proximity       = 1,
        mma             = 2,
        akm             = 3,
        gyro            = 4, 
        pressure        = 5,
        temperature		= 6,
        numSensorDrivers,
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
                return mma;
            case ID_M:
			case ID_O:
                return akm;	
            case ID_P:
                return proximity;
            case ID_L:
                return light;	
			case ID_GY:
				return gyro;
			case ID_PR:
				return pressure;
			case ID_TMP:
				return temperature;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
	D("Entered.");
	
    mSensors[light] = new LightSensor();
    mPollFds[light].fd = mSensors[light]->getFd();
    mPollFds[light].events = POLLIN;
    mPollFds[light].revents = 0;

    mSensors[proximity] = new ProximitySensor();
    mPollFds[proximity].fd = mSensors[proximity]->getFd();
    mPollFds[proximity].events = POLLIN;
    mPollFds[proximity].revents = 0;
	

    mSensors[mma] = new MmaSensor();
    mPollFds[mma].fd = mSensors[mma]->getFd();
    mPollFds[mma].events = POLLIN;
    mPollFds[mma].revents = 0;

    mSensors[akm] = new AkmSensor();
    mPollFds[akm].fd = mSensors[akm]->getFd();
    mPollFds[akm].events = POLLIN;
    mPollFds[akm].revents = 0;

	mSensors[gyro] = new GyroSensor();
    mPollFds[gyro].fd = mSensors[gyro]->getFd();
    mPollFds[gyro].events = POLLIN;
    mPollFds[gyro].revents = 0;

	mSensors[pressure] = new PressureSensor();
    mPollFds[pressure].fd = mSensors[pressure]->getFd();
    mPollFds[pressure].events = POLLIN;
    mPollFds[pressure].revents = 0;

	mSensors[temperature] = new TemperatureSensor();
    mPollFds[temperature].fd = mSensors[temperature]->getFd();
    mPollFds[temperature].events = POLLIN;
    mPollFds[temperature].revents = 0;

    int wakeFds[2];
    int result = pipe(wakeFds);
    LOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;

}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
#if defined(CALIBRATION_SUPPORT)
	if((index == mma) && enabled)
	{			
		sensor_get_calibration_from_vendor1();
	}
#endif
    int err =  mSensors[index]->enable(handle, enabled);
    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        LOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
	D("Entered : count = %d", count);
    int nbEvents = 0;
    int n = 0;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);	// num of evens received.
				D("nb = %d.", nb);
				#if defined(CALIBRATION_SUPPORT)
				if(i == mma)
				{
					data->acceleration.x -= gAccelCaliData[0] * ACCELERATION_RATIO_ANDROID_TO_HW;
					data->acceleration.y -= gAccelCaliData[1] * ACCELERATION_RATIO_ANDROID_TO_HW;
					data->acceleration.z -= gAccelCaliData[2] * ACCELERATION_RATIO_ANDROID_TO_HW;
				}
				#endif
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;
				D("count = %d, nbEvents = %d, data = 0x%p.", count, nbEvents, data);
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            if (n<0) {
                LOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                LOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                LOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
		D("n =0x%x, count = 0x%x.", n, count);
    } while (n && count);

	D("to return : nbEvents = %d", nbEvents);
    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
	D("Entered.");
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device)
{
	LOGD("%s\n",SENSOR_VERSION_AND_TIME);
    int status = -EINVAL;

    sensors_poll_context_t *dev = new sensors_poll_context_t();
    memset(&dev->device, 0, sizeof(sensors_poll_device_t));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = 0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    *device = &dev->device.common;
    status = 0;
    return status;
}
