/*
 * Copyright (C) 2011 Invensense, Inc.
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

#define LOG_NDEBUG 0
#define LOG_TAG "Sensors"
#define FUNC_LOG ALOGV("%s", __PRETTY_FUNCTION__)

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"
#include "MPLSensor.h"
#include "LightSensor.h"
//#include "MPLSensorSysApi.h"

/*****************************************************************************/

#define DELAY_OUT_TIME 0x7FFFFFFF

#define LIGHT_SENSOR_POLLTIME    2000000000

#define SENSORS_ROTATION_VECTOR  (1<<ID_RV)
#define SENSORS_LINEAR_ACCEL     (1<<ID_LA)
#define SENSORS_GRAVITY          (1<<ID_GR)
#define SENSORS_GYROSCOPE        (1<<ID_GY)
#define SENSORS_ACCELERATION     (1<<ID_A)
#define SENSORS_MAGNETIC_FIELD   (1<<ID_M)
#define SENSORS_ORIENTATION      (1<<ID_O)
#define SENSORS_LIGHT      (1<<ID_L)

#define SENSORS_ROTATION_VECTOR_HANDLE  (ID_RV)
#define SENSORS_LINEAR_ACCEL_HANDLE     (ID_LA)
#define SENSORS_GRAVITY_HANDLE          (ID_GR)
#define SENSORS_GYROSCOPE_HANDLE        (ID_GY)
#define SENSORS_ACCELERATION_HANDLE     (ID_A)
#define SENSORS_MAGNETIC_FIELD_HANDLE   (ID_M)
#define SENSORS_ORIENTATION_HANDLE      (ID_O)
#define SENSORS_LIGHT_HANDLE      (ID_L)

#define AKM_FTRACE 0
#define AKM_DEBUG 0
#define AKM_DATA 0

/*****************************************************************************/

/* The SENSORS Module */
struct sensor_t sSensorList[8];
/*= {
      { "MPL rotation vector",
        "Invensense",
        1, SENSORS_ROTATION_VECTOR_HANDLE,
        SENSOR_TYPE_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 20000,{ } },
      { "MPL linear accel",
        "Invensense",
        1, SENSORS_LINEAR_ACCEL_HANDLE,
        SENSOR_TYPE_LINEAR_ACCELERATION, 10240.0f, 1.0f, 0.5f, 20000,{ } },
      { "MPL gravity",
        "Invensense",
        1, SENSORS_GRAVITY_HANDLE,
        SENSOR_TYPE_GRAVITY, 10240.0f, 1.0f, 0.5f, 20000,{ } },
      { "MPL Gyro",
        "Invensense",
        1, SENSORS_GYROSCOPE_HANDLE,
        SENSOR_TYPE_GYROSCOPE, 10240.0f, 1.0f, 0.5f, 20000,{ } },
      { "MPL accel",
        "Invensense",
        1, SENSORS_ACCELERATION_HANDLE,
        SENSOR_TYPE_ACCELEROMETER, 10240.0f, 1.0f, 0.5f, 20000,{ } },
     { "MPL magnetic field",
        "Invensense",
        1, SENSORS_MAGNETIC_FIELD_HANDLE,
        SENSOR_TYPE_MAGNETIC_FIELD, 10240.0f, 1.0f, 0.5f, 20000,{ } },
      { "MPL Orientation (android deprecated format)",
          "Invensense",
          1, SENSORS_ORIENTATION_HANDLE,
          SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f, 9.7f, 20000,{ } },
		 { "Light sensor",
                 "Dyna Image Corporation",
                 1,  SENSORS_LIGHT_HANDLE,
                 SENSOR_TYPE_LIGHT, 10240.0f, 1.0f, 0.5f, 0, { } },

};
*/

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
	sSensorList[0].name       = "MPL rotation vector",
	sSensorList[0].vendor     = "Invensense",
	sSensorList[0].version    = 1,
	sSensorList[0].handle     = SENSORS_ROTATION_VECTOR_HANDLE,
	sSensorList[0].type       = SENSOR_TYPE_ROTATION_VECTOR,
	sSensorList[0].maxRange   = 10240.0f,
	sSensorList[0].resolution = 1.0f,
	sSensorList[0].power      = 0.5f,
	sSensorList[0].minDelay   = 20000,      

	sSensorList[1].name       = "MPL linear accel",
	sSensorList[1].vendor     = "Invensense",
	sSensorList[1].version    = 1,
	sSensorList[1].handle     = SENSORS_LINEAR_ACCEL_HANDLE,
	sSensorList[1].type       = SENSOR_TYPE_LINEAR_ACCELERATION,
	sSensorList[1].maxRange   = 10240.0f,  
	sSensorList[1].resolution = 1.0f,      
	sSensorList[1].power      = 0.5f,      
	sSensorList[1].minDelay   = 20000,     

	sSensorList[2].name       = "MPL gravity",
	sSensorList[2].vendor     = "Invensense",
	sSensorList[2].version    = 1,
	sSensorList[2].handle     = SENSORS_GRAVITY_HANDLE,
	sSensorList[2].type       = SENSOR_TYPE_GRAVITY,
	sSensorList[2].maxRange   =10240.0f,  
	sSensorList[2].resolution =1.0f,      
	sSensorList[2].power      =0.5f,      
	sSensorList[2].minDelay   =20000,     

	sSensorList[3].name       = "MPL Gyro",
	sSensorList[3].vendor     = "Invensense",
	sSensorList[3].version    = 1,
	sSensorList[3].handle     = SENSORS_GYROSCOPE_HANDLE,
	sSensorList[3].type       = SENSOR_TYPE_GYROSCOPE,
	sSensorList[3].maxRange   = 10240.0f,  
	sSensorList[3].resolution = 1.0f,      
	sSensorList[3].power      = 0.5f,      
	sSensorList[3].minDelay   = 20000,     
	  
	sSensorList[4].name       = "MPL accel",
	sSensorList[4].vendor     = "Invensense",
	sSensorList[4].version    = 1,
	sSensorList[4].handle     = SENSORS_ACCELERATION_HANDLE,
	sSensorList[4].type       = SENSOR_TYPE_ACCELEROMETER,
	sSensorList[4].maxRange   = 10240.0f,  
	sSensorList[4].resolution = 1.0f,      
	sSensorList[4].power      = 0.5f,      
	sSensorList[4].minDelay   = 20000,     

	sSensorList[5].name       = "MPL magnetic field",
	sSensorList[5].vendor     = "Invensense",
	sSensorList[5].version    = 1,
	sSensorList[5].handle     = SENSORS_MAGNETIC_FIELD_HANDLE,
	sSensorList[5].type       = SENSOR_TYPE_MAGNETIC_FIELD,
	sSensorList[5].maxRange   = 10240.0f,  
	sSensorList[5].resolution = 1.0f,      
	sSensorList[5].power      = 0.5f,      
	sSensorList[5].minDelay   = 20000,     

	sSensorList[6].name       = "MPL Orientation (android deprecated format)",
	sSensorList[6].vendor     = "Invensense",
	sSensorList[6].version    = 1,
	sSensorList[6].handle     = SENSORS_ORIENTATION_HANDLE,
	sSensorList[6].type       = SENSOR_TYPE_ORIENTATION,
	sSensorList[6].maxRange   = 10240.0f,  
	sSensorList[6].resolution = 1.0f,      
	sSensorList[6].power      = 0.5f,      
	sSensorList[6].minDelay   = 20000,     

	sSensorList[7].name       = "Light sensor",
	sSensorList[7].vendor     = "Invensense",
	sSensorList[7].version    = 1,
	sSensorList[7].handle     = SENSORS_LIGHT_HANDLE,
	sSensorList[7].type       = SENSOR_TYPE_LIGHT,
	sSensorList[7].maxRange   = 10240.0f,  
	sSensorList[7].resolution = 1.0f,      
	sSensorList[7].power      = 0.5f,      
	sSensorList[7].minDelay   = 20000,     
    	
    *list = sSensorList;
    return ARRAY_SIZE(sSensorList);
}   
    
static struct hw_module_methods_t sensors_module_methods = {
        open: open_sensors
};  
    
struct sensors_module_t HAL_MODULE_INFO_SYM = {
        common: {
                tag: HARDWARE_MODULE_TAG,
                version_major: 1,
                version_minor: 0,
                id: SENSORS_HARDWARE_MODULE_ID,
                name: "Invensense module",
                author: "Invensense Inc.",
                methods: &sensors_module_methods,
        },
        get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    /**
     * poll events.
     * @param data 
     *      用于返回 sensor_event 数据. 
     * @param count
     *      .Q :
     */
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
        mpl               = 0,  //all mpl entries must be consecutive and in this order
        mpl_accel,
        mpl_timer,
		  light,
        numSensorDrivers,       // wake pipe goes here
        mpl_power,              //special handle for MPL pm interaction
        numFds,
    };

    static const size_t wake = numFds - 2;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_RV:
            case ID_LA:
            case ID_GR:
            case ID_GY:
            case ID_A:
            case ID_M:
            case ID_O:
                return mpl;
			case ID_L:
                return light;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    FUNC_LOG;
    MPLSensor *p_mplsen = new MPLSensor();
    
    // setup the callback object for handing mpl callbacks
    setCallbackObject(p_mplsen); 

    mSensors[mpl] = p_mplsen;
    mPollFds[mpl].fd = mSensors[mpl]->getFd();
    mPollFds[mpl].events = POLLIN;
    mPollFds[mpl].revents = 0;

    mSensors[mpl_accel] = mSensors[mpl];
    mPollFds[mpl_accel].fd = ((MPLSensor*)mSensors[mpl])->getAccelFd();
    mPollFds[mpl_accel].events = POLLIN;
    mPollFds[mpl_accel].revents = 0;

    mSensors[mpl_timer] = mSensors[mpl];
    mPollFds[mpl_timer].fd = ((MPLSensor*)mSensors[mpl])->getTimerFd();
    mPollFds[mpl_timer].events = POLLIN;
    mPollFds[mpl_timer].revents = 0;

	    mSensors[light] = new LightSensor();
    mPollFds[light].fd = mSensors[light]->getFd();
    mPollFds[light].events = POLLIN;
    mPollFds[light].revents = 0;
	
    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;

    //setup MPL pm interaction handle
    mPollFds[mpl_power].fd = ((MPLSensor*)mSensors[mpl])->getPowerFd();
    mPollFds[mpl_power].events = POLLIN;
    mPollFds[mpl_power].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t()
{
    FUNC_LOG;
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled)
{
    FUNC_LOG;
    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err =  mSensors[index]->enable(handle, enabled);
    if (!err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
    FUNC_LOG;
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    //FUNC_LOG;
    int nbEvents = 0;
    int n = 0;
    int polltime = -1;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;

                //special handling for the mpl, which has multiple handles
                if(i==mpl) {
                    i+=2; //skip accel and timer
                    mPollFds[mpl_accel].revents = 0;
                    mPollFds[mpl_timer].revents = 0;
                }
                if(i==mpl_accel) {
                    i+=1; //skip timer
                    mPollFds[mpl_timer].revents = 0;
                }
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            int i;

            n = poll(mPollFds, numFds, nbEvents ? 0 : polltime);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
            if(mPollFds[mpl_power].revents & POLLIN) {
                ((MPLSensor*)mSensors[mpl])->handlePowerEvent();
                mPollFds[mpl_power].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
                          int handle, int enabled)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
                          int handle, int64_t ns)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
                      sensors_event_t* data, int count)
{
    //FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
    FUNC_LOG;
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


