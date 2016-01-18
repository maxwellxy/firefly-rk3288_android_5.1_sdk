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
//see also the EXTRA_VERBOSE define in the MPLSensor.h header file

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <pthread.h>

#include <cutils/log.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <string.h>

#include "MPLSensor.h"

#include "math.h"
#include "ml.h"
#include "mlFIFO.h"
#include "mlsl.h"
#include "mlos.h"
#include "ml_mputest.h"
#include "ml_stored_data.h"
#include "mldl_cfg.h"
#include "mldl.h"
#include "temp_comp.h"
#include "mlBiasNoMotion.h"

#include "mpu.h"
#include "kernel/timerirq.h"
#include "kernel/mpuirq.h"

extern "C" {
#include "mlsupervisor.h"
}

#include "mlcontrol.h"

/* this mask must turn on only the sensors that are present and managed by the MPL */
#define ALL_MPL_SENSORS_NP (INV_THREE_AXIS_ACCEL | INV_THREE_AXIS_COMPASS | INV_THREE_AXIS_GYRO)

/* ***************************************************************************
 * MPL interface misc.
 */

MPLSensor* MPLSensor::gMPLSensor = NULL;

/* we need to pass some callbacks to the MPL.  The mpl is a C library, so
 * wrappers for the C++ callback implementations are required.
 */
extern "C" {
//callback wrappers go here
void mot_cb_wrapper(uint16_t val)
{
    if (MPLSensor::gMPLSensor) {
        MPLSensor::gMPLSensor->cbOnMotion(val);
    }
}

void procData_cb_wrapper()
{
    if(MPLSensor::gMPLSensor) {
        MPLSensor::gMPLSensor->cbProcData();
    }
}

void setCallbackObject(MPLSensor* gbpt)
{
    MPLSensor::gMPLSensor = gbpt;
}

MPLSensor* getCallbackObject() {
    return MPLSensor::gMPLSensor;
}

} //end of extern C


/*****************************************************************************
 * 9 axis enable/disable class implementation
 */

NineAxisSensorFusion::NineAxisSensorFusion()
{
    fp_enable_9x_fusion =  NULL;
    fp_disable_9x_fusion = NULL; 
    enabled = -1;
}

NineAxisSensorFusion::~NineAxisSensorFusion()
{
    fp_enable_9x_fusion =  NULL;
    fp_disable_9x_fusion = NULL; 
    enabled = -1;
}

void NineAxisSensorFusion::registerEnabler(inv_error_t (*fp_enable_9x_fusion)(void))
{
    this->fp_enable_9x_fusion  = fp_enable_9x_fusion;
    enabled = -1;
}

void NineAxisSensorFusion::registerDisabler(inv_error_t (*fp_disable_9x_fusion)(void))
{
    this->fp_disable_9x_fusion = fp_disable_9x_fusion;
    enabled = -1;
}

inv_error_t NineAxisSensorFusion::enable()
{
    inv_error_t result = INV_ERROR;
    /*
    if (!(inv_get_dl_config()->inv_mpu_cfg->requested_sensors & 
            INV_THREE_AXIS_COMPASS)) {
        LOGE("Warning : enable 9 axis sensor fusion aborted - "
             "compass is OFF in requested_sensors mask\n");
        return INV_SUCCESS;
    }
    */
    if (enabled == true) {
        LOGE("Cannot enable 9 axis sensor fusion - already enabled\n");
        return INV_ERROR;
    }
    if (!fp_enable_9x_fusion) {
        LOGE("Error : enable 9 axis sensor fusion function not registered\n");
        return INV_ERROR;
    }
    result = fp_enable_9x_fusion();
    if (result) {
        LOGE("Error while trying to enable 9 axis sensor fusion : %d\n", 
             result);
        enabled = false;
    } else {
        if (enabled == -1)
            LOGI("Enabled 9 axis sensor fusion - first time\n");
        else
            LOGI("Enabled 9 axis sensor fusion\n");
        enabled = true;
    }
    return result;
}

inv_error_t NineAxisSensorFusion::disable()
{
    inv_error_t result = INV_SUCCESS;
    /*
    if (!(inv_get_dl_config()->inv_mpu_cfg->requested_sensors & 
            INV_THREE_AXIS_COMPASS)) {
        LOGE("Warning : disable 9 axis sensor fusion aborted - "
             "compass is OFF in requested_sensors mask\n");
        return INV_SUCCESS;
    }
    */
    if(enabled == -1) {
        LOGW("Cannot disable 9 axis sensor fusion - it was NEVER enabled\n");
        return INV_ERROR;
    }
    if (enabled == false) {
        LOGW("Cannot disable 9 axis sensor fusion - already disabled\n");
        return INV_ERROR;
    }
    if (!fp_disable_9x_fusion) {
        LOGE("Error : disable 9 axis sensor fusion function not registered\n");
        return INV_ERROR;
    }
    result = fp_disable_9x_fusion();
    if (result)
        LOGE("Error while trying to disable 9 axis sensor fusion : %d\n", 
             result);
    else
        LOGI("Disabled 9 axis sensor fusion\n");
    enabled = false;
    return result;
}

/*****************************************************************************
 * sensor class implementation
 */
  

MPLSensor::MPLSensor() : SensorBase(NULL, NULL),
                         mMpuAccuracy(0),
                         mNewData(0), 
                         mDmpStarted(false),
                         mMasterSensorMask(INV_ALL_SENSORS),
                         mLocalSensorMask(ALL_MPL_SENSORS_NP),
                         mPollTime(-1),
                         mCurFifoRate(-1),
                         mHaveGoodMpuCal(false),
                         mUseTimerIrqAccel(false),
                         mUsetimerIrqCompass(true),
                         mUseTimerirq(false),
                         mSampleCount(0),
                         mMplMutex(PTHREAD_MUTEX_INITIALIZER),
                         mEnabled(0),
                         mPendingMask(0)
{
    VFUNC_LOG;
    inv_error_t rv;
    int mpu_int_fd, i;
    char *port = NULL;
    unsigned char *ver_str;

    LOGV_IF(EXTRA_VERBOSE, "MPLSensor constructor : numSensors = %d", numSensors);

    mForceSleep = false;

    for (i = 0; i < (int)ARRAY_SIZE(mPollFds); i++) {
        mPollFds[i].fd = -1;
        mPollFds[i].events = 0;
    }

    pthread_mutex_lock(&mMplMutex);

    mpu_int_fd = open("/dev/mpuirq", O_RDWR);
    if (mpu_int_fd == -1) {
        LOGE("could not open the mpu irq device node");
    } else {
        fcntl(mpu_int_fd, F_SETFL, O_NONBLOCK);
        //ioctl(mpu_int_fd, MPUIRQ_SET_TIMEOUT, 0);
        mIrqFds.add(MPUIRQ_FD, mpu_int_fd);
        mPollFds[MPUIRQ_FD].fd = mpu_int_fd;
        mPollFds[MPUIRQ_FD].events = POLLIN;
    }

    accel_fd = open("/dev/accelirq", O_RDWR);
    if (accel_fd == -1) {
        LOGE("could not open the accel irq device node");
    } else {
        fcntl(accel_fd, F_SETFL, O_NONBLOCK);
        //ioctl(accel_fd, SLAVEIRQ_SET_TIMEOUT, 0);
        mIrqFds.add(ACCELIRQ_FD, accel_fd);
        mPollFds[ACCELIRQ_FD].fd = accel_fd;
        mPollFds[ACCELIRQ_FD].events = POLLIN;
    }

    timer_fd = open("/dev/timerirq", O_RDWR);
    if (timer_fd == -1) {
        LOGE("could not open the timer irq device node");
    } else {
        fcntl(timer_fd, F_SETFL, O_NONBLOCK);
        //ioctl(timer_fd, TIMERIRQ_SET_TIMEOUT, 0);
        mIrqFds.add(TIMERIRQ_FD, timer_fd);
        mPollFds[TIMERIRQ_FD].fd = timer_fd;
        mPollFds[TIMERIRQ_FD].events = POLLIN;
    }

    data_fd = mpu_int_fd;

    if ((accel_fd == -1) && (timer_fd != -1)) {
        //no accel irq and timer available
        mUseTimerIrqAccel = true;
        LOGD("MPLSensor falling back to timerirq for accel data");
    }

    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[RotationVector].version = sizeof(sensors_event_t);
    mPendingEvents[RotationVector].sensor = ID_RV;
    mPendingEvents[RotationVector].type = SENSOR_TYPE_ROTATION_VECTOR;
    mPendingEvents[RotationVector].acceleration.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[LinearAccel].version = sizeof(sensors_event_t);
    mPendingEvents[LinearAccel].sensor = ID_LA;
    mPendingEvents[LinearAccel].type = SENSOR_TYPE_LINEAR_ACCELERATION;
    mPendingEvents[LinearAccel].acceleration.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Gravity].version = sizeof(sensors_event_t);
    mPendingEvents[Gravity].sensor = ID_GR;
    mPendingEvents[Gravity].type = SENSOR_TYPE_GRAVITY;
    mPendingEvents[Gravity].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Gyro].version = sizeof(sensors_event_t);
    mPendingEvents[Gyro].sensor = ID_GY;
    mPendingEvents[Gyro].type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvents[Gyro].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Accelerometer].version = sizeof(sensors_event_t);
    mPendingEvents[Accelerometer].sensor = ID_A;
    mPendingEvents[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[Accelerometer].acceleration.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[MagneticField].sensor = ID_M;
    mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Orientation].version = sizeof(sensors_event_t);
    mPendingEvents[Orientation].sensor = ID_O;
    mPendingEvents[Orientation].type = SENSOR_TYPE_ORIENTATION;
    mPendingEvents[Orientation].orientation.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    mHandlers[RotationVector] = &MPLSensor::rvHandler;
    mHandlers[LinearAccel] = &MPLSensor::laHandler;
    mHandlers[Gravity] = &MPLSensor::gravHandler;
    mHandlers[Gyro] = &MPLSensor::gyroHandler;
    mHandlers[Accelerometer] = &MPLSensor::accelHandler;
    mHandlers[MagneticField] = &MPLSensor::compassHandler;
    mHandlers[Orientation] = &MPLSensor::orienHandler;

    for (int i = 0; i < numSensors; i++)
        mDelays[i] = 30000000LLU; // 30 ms by default

    if (inv_serial_start(port) != INV_SUCCESS) {
        LOGE("Fatal Error : could not open MPL serial interface");
    }
    
    inv_get_version(&ver_str);
    LOGI("%s\n", (char *)ver_str);

    //initialize library parameters
    initMPL();

    //setup the FIFO contents
    enableFIFO();

    //we start the motion processing only when a sensor is enabled...
    //rv = inv_dmp_start();
    //LOGE_IF(rv != INV_SUCCESS, "Fatal error: could not start the DMP correctly. (code = %d)\n", rv);
    //dmp_started = true;

    pthread_mutex_unlock(&mMplMutex);

}

MPLSensor::~MPLSensor()
{
    VFUNC_LOG;
    pthread_mutex_lock(&mMplMutex);
    if (inv_dmp_stop() != INV_SUCCESS) {
        LOGD("Error: could not stop the DMP correctly.\n");
    }

    if (inv_dmp_close() != INV_SUCCESS) {
        LOGD("Error: could not close the DMP");
    }

    if (inv_serial_stop() != INV_SUCCESS) {
        LOGD("Error : could not close the serial port");
    }
    pthread_mutex_unlock(&mMplMutex);
}

/* clear any data from our various filehandles */
void MPLSensor::clearIrqData(bool* irq_set)
{
    unsigned int i;
    int nread;
    struct mpuirq_data irqdata;

    poll(mPollFds, ARRAY_SIZE(mPollFds), 0); //check which ones need to be cleared

    for (i = 0; i < ARRAY_SIZE(mPollFds); i++) {
        int cur_fd = mPollFds[i].fd;
        int j = 0;
        if (mPollFds[i].revents & POLLIN) {
            nread = read(cur_fd, &irqdata, sizeof(irqdata));
            if (nread > 0) {
                irq_set[i] = true;
                //LOGV_IF(EXTRA_VERBOSE, "irq: %d %d (%d)", i, irqdata.interruptcount, j++);
            }
        }
        mPollFds[i].revents = 0;
    }
}

bool MPLSensor::needDMPStop() 
{ 
    return mDmpStarted;
}

#define GY_ENABLED ((1<<ID_GY) & enabled_sensors)
#define A_ENABLED  ((1<<ID_A)  & enabled_sensors)
#define O_ENABLED  ((1<<ID_O)  & enabled_sensors)
#define M_ENABLED  ((1<<ID_M)  & enabled_sensors)
#define LA_ENABLED ((1<<ID_LA) & enabled_sensors)
#define GR_ENABLED ((1<<ID_GR) & enabled_sensors)
#define RV_ENABLED ((1<<ID_RV) & enabled_sensors)

void MPLSensor::computeLocalSensorMask(int enabled_sensors)
{
    do {

        if (LA_ENABLED || GR_ENABLED || RV_ENABLED || O_ENABLED) {
            mLocalSensorMask = ALL_MPL_SENSORS_NP;
            break;
        }

        if (!A_ENABLED && !M_ENABLED && !GY_ENABLED) {
            mLocalSensorMask = 0;
            break;
        }

        if (GY_ENABLED) {
            mLocalSensorMask |= INV_THREE_AXIS_GYRO;
        } else {
            mLocalSensorMask &= ~INV_THREE_AXIS_GYRO;
        }

        if (A_ENABLED) {
            mLocalSensorMask |= (INV_THREE_AXIS_ACCEL);
        } else {
            mLocalSensorMask &= ~(INV_THREE_AXIS_ACCEL);
        }

        if (M_ENABLED) {
            mLocalSensorMask |= INV_THREE_AXIS_COMPASS;
        } else {
            mLocalSensorMask &= ~(INV_THREE_AXIS_COMPASS);
        }

    } while (0);
}

/* set the power states of the various sensors based on the bits set in the
 * enabled_sensors parameter.
 * this function modifies globalish state variables.  It must be called with the mMplMutex held. */
void MPLSensor::setPowerStates(int enabled_sensors)
{
    VFUNC_LOG;
    bool irq_set[5] = {false, false, false, false, false};
    unsigned long sen_mask;
    bool changing_sensors;
    bool restart;
    inv_error_t rv;    // record the new sensor state

    LOGV("enabled_sensors: %d dmp_started: %d", enabled_sensors, mDmpStarted);

    /* NOTE : 
       this method changes the mLocalSensorMask that is used just after */
    computeLocalSensorMask(enabled_sensors);
    /* record the new sensor state */
    sen_mask = mLocalSensorMask & mMasterSensorMask;

    changing_sensors = (
        (inv_get_dl_config()->inv_mpu_cfg->requested_sensors != sen_mask) 
            && (sen_mask != 0));
    restart = (!mDmpStarted) && (sen_mask != 0);

    if (needStateChange(changing_sensors, restart)) {

        LOGV_IF(EXTRA_VERBOSE, "cs:%d rs:%d ", changing_sensors, restart);

        if (needDMPStop()) {
            nineAxisSF.disable();
            inv_dmp_stop();
            clearIrqData(irq_set);
            mDmpStarted = false;
        }

        if (sen_mask != inv_get_dl_config()->inv_mpu_cfg->requested_sensors) {
            LOGV("calling inv_set_mpu_sensors(%06lx)", sen_mask);
            rv = inv_set_mpu_sensors(sen_mask);
            LOGE_IF(rv != INV_SUCCESS,
                    "error: unable to set MPL sensor power states "
                    "(sens = %ld, retcode = %d)", sen_mask, rv);
        }

        enableFeatures();

        if (((mUsetimerIrqCompass && (sen_mask == INV_THREE_AXIS_COMPASS))
                || (mUseTimerIrqAccel && (sen_mask & INV_THREE_AXIS_ACCEL)))
                && ((sen_mask & INV_DMP_PROCESSOR) == 0)) {
            LOGV_IF(EXTRA_VERBOSE, "Allowing TimerIRQ");
            mUseTimerirq = true;
        } else {
            if (mUseTimerirq) {
                ioctl(mIrqFds.valueFor(TIMERIRQ_FD), TIMERIRQ_STOP, 0);
                clearIrqData(irq_set);
            }
            LOGV_IF(EXTRA_VERBOSE, "Not allowing TimerIRQ");
            mUseTimerirq = false;
        }

        if (!mDmpStarted) {
            if (mHaveGoodMpuCal) {
                rv = inv_store_calibration();
                if (!rv)
                    LOGV("Calibration file successfully stored");
                else
                    LOGE("Error : unable to store MPL calibration file");
                mHaveGoodMpuCal = false;
            }
            LOGV("Starting DMP");
            nineAxisSF.enable();
            rv = inv_dmp_start();
            LOGE_IF(rv != INV_SUCCESS, "unable to start dmp");
            mDmpStarted = true;
        }
    }

    //check if we should stop the DMP
    if (mDmpStarted && (sen_mask == 0)) {
        LOGV("Stopping DMP");
        nineAxisSF.disable();
        rv = inv_dmp_stop();
        LOGE_IF(rv != INV_SUCCESS, "error: unable to stop DMP (retcode = %d)", rv);
        if (mUseTimerirq) {
            ioctl(mIrqFds.valueFor(TIMERIRQ_FD), TIMERIRQ_STOP, 0);
        }
        clearIrqData(irq_set);

        shutdownFeatures();

        mDmpStarted = false;
        mPollTime = -1;
        mCurFifoRate = -1;
    }
}

void MPLSensor::loadCompassCalibrationEnabler(void* h_dmp_lib, const char* suffix)
{
    const char prefix[] = "inv_";
    char symname[35];
    const char* error;
    inv_error_t (*enable_disable)(void);

    /* enabler */    
    strcpy(symname, prefix);
    strcat(symname, "enable_");
    strcat(symname, suffix);

    error = dlerror();
    
    enable_disable = (inv_error_t(*)()) dlsym(h_dmp_lib, symname);
    error = dlerror();
    if(error != NULL) {
        LOGE("%s : could not load compass calibration enable function '%s'", 
             error, symname);
    } else {
        LOGI("Loaded symbol '%s'\n", symname);
    }
    nineAxisSF.registerEnabler(enable_disable);
    
    /* disabler */
    strcpy(symname, prefix);
    strcat(symname, "disable_");
    strcat(symname, suffix);

    error = dlerror();

    enable_disable = (inv_error_t(*)()) dlsym(h_dmp_lib, symname);
    error = dlerror();
    if(error != NULL) {
        LOGE("%s : could not load compass calibration disable function '%s'", 
             error, symname);
    } else {
        LOGI("Loaded symbol '%s'\n", symname);
    }
    nineAxisSF.registerDisabler(enable_disable);
}

/**
 * container function for all the calls we make once to set up the MPL.
 */
void MPLSensor::initMPL()
{
    VFUNC_LOG;
    inv_error_t result;
    unsigned short bias_update_mask = INV_ALL;
    struct mldl_cfg *mldl_cfg;

    if (inv_dmp_open() != INV_SUCCESS) {
        LOGE("Fatal Error : could not open DMP correctly.\n");
    }

    result = inv_set_mpu_sensors(ALL_MPL_SENSORS_NP); /* default to all sensors, also makes 9axis enable work */
    LOGE_IF(result != INV_SUCCESS,
            "Fatal Error : could not set enabled sensors.");

    result = inv_load_calibration();
    if (result)
        LOGV("Calibration file successfully loaded");
    else
        LOGE("Could not open or load MPL calibration file (%d)", result);

    /* check for the 9axis fusion library */
    void *h_dmp_lib = dlopen(MPL_LIB_NAME, RTLD_NOW);
    if(!h_dmp_lib) {
        LOGE("%s not found, 9x sensor fusion disabled (%s)",
             MPL_LIB_NAME, dlerror());
    } else {
        mldl_cfg = inv_get_dl_config();
        
        if (!mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]) {
            LOGW("No compass configured on this platform : "
                 "mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS] = NULL\n");
                 
        } else if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->id == COMPASS_ID_AK8975) {
            void *h_akm_lib = dlopen(AKM_LIB_NAME, RTLD_NOW);
            if (h_akm_lib) {
                const char* error;
                error = dlerror();
                inv_error_t (*fp_inv_external_slave_akm8975_open)() =
                    (inv_error_t(*)()) dlsym(
                        h_akm_lib, "inv_external_slave_akm8975_open");
                if (fp_inv_external_slave_akm8975_open) 
				{
                    result = (*fp_inv_external_slave_akm8975_open)();
                    LOGE_IF(result != INV_SUCCESS, 
                            "inv_external_slave_akm8975_open failed.");
                    loadCompassCalibrationEnabler(h_dmp_lib, 
                                                  "9x_fusion_external");
                } else {
                    LOGE("Unable to find symbol 'inv_external_slave_akm8975_open'");
                }
            } else {
                LOGE("could not find AKM partner library %s => "
                     "using InvenSense internals.", AKM_LIB_NAME);
                loadCompassCalibrationEnabler(h_dmp_lib, "9x_fusion");
            }
            
        } else if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->id == COMPASS_ID_AMI306) {
            void *h_ami_lib = dlopen(AICHI_LIB_NAME, RTLD_NOW);
            if (h_ami_lib) {
                const char* error;
                error = dlerror();
                inv_error_t (*fp_inv_external_slave_ami306_open)() =
                        (inv_error_t(*)()) dlsym(
                            h_ami_lib, "inv_external_slave_ami306_open");
                if (fp_inv_external_slave_ami306_open) {
                    result = (*fp_inv_external_slave_ami306_open)();
                    LOGE_IF(result != INV_SUCCESS, 
                            "inv_external_slave_ami306_open failed.");
                    loadCompassCalibrationEnabler(h_dmp_lib, 
                                                  "9x_fusion_external");
                } else {
                    LOGE("Unable to find symbol 'inv_external_slave_ami306_open'");
                }
            } else {
                LOGE("could not find Aichi partner library %s => "
                     "using InvenSense internals.", AICHI_LIB_NAME);
                loadCompassCalibrationEnabler(h_dmp_lib, "9x_fusion");
            }
            
        } else {
            loadCompassCalibrationEnabler(h_dmp_lib, "9x_fusion");
        }
        nineAxisSF.enable();
    }

    /* enable basic bias trackers for gyros */
    result = inv_enable_bias_no_motion();
    LOGE_IF(result, "enable_bias_no_motion returned %d\n", result);
    result = inv_enable_bias_from_gravity(true);
    LOGE_IF(result, "enable_bias_from_gravity returned %d\n", result);
    result = inv_enable_bias_from_LPF(true);
    LOGE_IF(result, "enable_bias_from_LPF returned %d\n", result);
    result = inv_set_dead_zone_normal(true);
    LOGE_IF(result, "set_dead_zone_normal returned %d\n", result);
    result = inv_enable_temp_comp();
    LOGE_IF(result, "enable_temp_comp returned %d\n", result);

    /* enable MPU interrupts */
    result = inv_set_motion_interrupt(true);
    LOGE_IF(result, "set_motion_interrupt returned %d\n", result);
    result = inv_set_fifo_interrupt(true);
    LOGE_IF(result, "set_fifo_interrupt returned %d\n", result);

    mCurFifoRate = 6;
    result = inv_set_fifo_rate(mCurFifoRate);
    LOGE_IF(result, "set_fifo_rate returned %d\n", result);

    mMpuAccuracy = SENSOR_STATUS_ACCURACY_MEDIUM;
    
    setupCallbacks();
}

/** setup the fifo contents.
 */
void MPLSensor::enableFIFO()
{
    VFUNC_LOG;
    inv_error_t result;

    result = inv_send_accel(INV_ALL, INV_32_BIT);
    LOGE_IF(result, "Fatal error: inv_send_accel returned %d\n", result);
    
    result = inv_send_quaternion(INV_32_BIT);
    LOGE_IF(result, "Fatal error: inv_send_quaternion returned %d\n", result);
    
    result = inv_send_linear_accel(INV_ALL, INV_32_BIT);
    LOGE_IF(result, "Fatal error: inv_send_linear_accel returned %d\n", result);
    
    result = inv_send_linear_accel_in_world(INV_ALL, INV_32_BIT);
    LOGE_IF(result, "Fatal error: inv_send_linear_accel_in_world returned %d\n",
            result);
            
    result = inv_send_gravity(INV_ALL, INV_32_BIT);
    LOGE_IF(result, "Fatal error: inv_send_gravity returned %d\n", result);
    
#if defined USE_TYPE_GYROSCOPE_COMPENSATED
    result = inv_send_gyro(INV_ALL, INV_32_BIT);
    LOGE_IF(result, "Fatal error: inv_send_gyro returned %d\n", result);
#else
    result = inv_send_sensor_data(INV_ELEMENT_2 | INV_ELEMENT_3 | INV_ELEMENT_4,
                                  INV_16_BIT);
    LOGE_IF(result, 
            "Fatal error: inv_send_sensor_data('raw gyro') returned %d\n",
            result);
#endif
}

/**
 *  set up the callbacks that we use in all cases (outside of gestures, etc)
 */
void MPLSensor::setupCallbacks()
{
    VFUNC_LOG;
    int result;
    
    result = inv_set_fifo_processed_callback(procData_cb_wrapper);
    LOGE_IF(result, "set_fifo_processed_callback returned %d", result);
    result = inv_set_motion_callback(mot_cb_wrapper);
    LOGE_IF(result, "set_motion_callback returned %d", result);
}

/**
 * handle the motion/no motion output from the MPL.
 */
void MPLSensor::cbOnMotion(uint16_t motionType)
{
    VFUNC_LOG;

    switch(motionType) {
        case INV_MOTION:
            LOGI("**** Motion ****\n");
            break;

        case INV_NO_MOTION:
            /* after the first no motion, the gyro should be calibrated well */
            mMpuAccuracy = SENSOR_STATUS_ACCURACY_HIGH;
            if ((inv_get_dl_config()->inv_mpu_cfg->requested_sensors) & INV_THREE_AXIS_GYRO) {
                //if gyros are on and we got a no motion, set a flag
                // indicating that the cal file can be written.
                mHaveGoodMpuCal = true;
            }
            LOGI("**** No Motion ****\n");
            break;

        default:
            break;
    }

    return;
}


void MPLSensor::cbProcData()
{
    mNewData = 1;
    mSampleCount++;
    //LOGV_IF(EXTRA_VERBOSE, "new data (%d)", sampleCount);
}

//these handlers transform mpl data into one of the Android sensor types
//  scaling and coordinate transforms should be done in the handlers

void MPLSensor::gyroHandler(sensors_event_t* s, uint32_t* pending_mask,
                             int index)
{
    VFUNC_LOG;
    inv_error_t res;
#if defined USE_TYPE_GYROSCOPE_COMPENSATED
    res = inv_get_gyro_float(s->gyro.v);
#else
    res = inv_get_gyro_raw_float(s->gyro.v);
#endif
    s->gyro.v[0] = s->gyro.v[0] * M_PI / 180.0;
    s->gyro.v[1] = s->gyro.v[1] * M_PI / 180.0;
    s->gyro.v[2] = s->gyro.v[2] * M_PI / 180.0;
    s->gyro.status = mMpuAccuracy;
    if (res == INV_SUCCESS)
        *pending_mask |= (1 << index);
}

void MPLSensor::accelHandler(sensors_event_t* s, uint32_t* pending_mask,
                             int index)
{
    VFUNC_LOG;
    inv_error_t res;
    res = inv_get_accel_float(s->acceleration.v);
    //res = inv_get_accel_float(s->acceleration.v);
    s->acceleration.v[0] = s->acceleration.v[0] * 9.81;
    s->acceleration.v[1] = s->acceleration.v[1] * 9.81;
    s->acceleration.v[2] = s->acceleration.v[2] * 9.81;
    //LOGV_IF(EXTRA_VERBOSE, "accel data: %f %f %f", s->acceleration.v[0], s->acceleration.v[1], s->acceleration.v[2]);
    s->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
    if (res == INV_SUCCESS)
        *pending_mask |= (1 << index);
}

int MPLSensor::estimateCompassAccuracy()
{
    int rv;
    inv_error_t res = inv_get_compass_accuracy(&rv);
    LOGE_IF(res, "error returned from inv_get_compass_accuracy");
    return rv;
}

void MPLSensor::compassHandler(sensors_event_t* s, uint32_t* pending_mask,
                               int index)
{
    VFUNC_LOG;
    inv_error_t res, res2;
    float bias_error[3];
    float total_be;
    static int bias_error_settled = 0;

    res = inv_get_magnetometer_float(s->magnetic.v);
    LOGE_IF(res, "compass_handler inv_get_magnetometer_float returned %d", res);

    s->magnetic.status = estimateCompassAccuracy();

    if (res == INV_SUCCESS)
        *pending_mask |= (1 << index);
}

void MPLSensor::rvHandler(sensors_event_t* s, uint32_t* pending_mask,
                          int index)
{
    VFUNC_LOG;
    float quat[4];
    float norm = 0;
    float ang = 0;
    inv_error_t r;

    r = inv_get_quaternion_float(quat);
    if (r) {
        *pending_mask &= ~(1 << index);
        return;
    } else {
        *pending_mask |= (1 << index);
    }

    norm = quat[1] * quat[1] + quat[2] * quat[2] + quat[3] * quat[3]
            + FLT_EPSILON;

    if (norm > 1.0f) {
        //renormalize
        norm = sqrtf(norm);
        float inv_norm = 1.0f / norm;
        quat[1] = quat[1] * inv_norm;
        quat[2] = quat[2] * inv_norm;
        quat[3] = quat[3] * inv_norm;
    }

    if (quat[0] < 0.0) {
        quat[1] = -quat[1];
        quat[2] = -quat[2];
        quat[3] = -quat[3];
    }

    s->gyro.v[0] = quat[1];
    s->gyro.v[1] = quat[2];
    s->gyro.v[2] = quat[3];

    s->gyro.status = ((mMpuAccuracy < estimateCompassAccuracy()) ? mMpuAccuracy
                     : estimateCompassAccuracy());
}

void MPLSensor::laHandler(sensors_event_t* s, uint32_t* pending_mask,
                          int index)
{
    VFUNC_LOG;
    inv_error_t res;
    res = inv_get_float_array(INV_LINEAR_ACCELERATION, s->gyro.v);
    s->gyro.v[0] *= 9.81;
    s->gyro.v[1] *= 9.81;
    s->gyro.v[2] *= 9.81;
    s->gyro.status = mMpuAccuracy;
    if (res == INV_SUCCESS)
        *pending_mask |= (1 << index);
}

void MPLSensor::gravHandler(sensors_event_t* s, uint32_t* pending_mask,
                            int index)
{
    VFUNC_LOG;
    inv_error_t res;
    res = inv_get_float_array(INV_GRAVITY, s->gyro.v);
    s->gyro.v[0] *= 9.81;
    s->gyro.v[1] *= 9.81;
    s->gyro.v[2] *= 9.81;
    s->gyro.status = mMpuAccuracy;
    if (res == INV_SUCCESS)
        *pending_mask |= (1 << index);
}

void MPLSensor::calcOrientationSensor(float *R, float *values)
{
    float tmp;

    //Azimuth
    if ((R[7] > 0.7071067f) || ((R[8] < 0) && (fabs(R[7]) > fabs(R[6])))) {
        values[0] = (float) atan2f(-R[3], R[0]);
    } else {
        values[0] = (float) atan2f(R[1], R[4]);
    }
    values[0] *= 57.295779513082320876798154814105f;
    if (values[0] < 0) {
        values[0] += 360.0f;
    }
    
    //Pitch
    tmp = R[7];
    if (tmp > 1.0f)
        tmp = 1.0f;
    if (tmp < -1.0f)
        tmp = -1.0f;
    values[1] = -asinf(tmp) * 57.295779513082320876798154814105f;
    if (R[8] < 0) {
        values[1] = 180.0f - values[1];
    }
    if (values[1] > 180.0f) {
        values[1] -= 360.0f;
    }
    
    //Roll
    if ((R[7] > 0.7071067f)) {
        values[2] = (float) atan2f(R[6], R[7]);
    } else {
        values[2] = (float) atan2f(R[6], R[8]);
    }
    values[2] *= 57.295779513082320876798154814105f;
    if (values[2] > 90.0f) {
        values[2] = 180.0f - values[2];
    }
    if (values[2] < -90.0f) {
        values[2] = -180.0f - values[2];
    }
}

void MPLSensor::orienHandler(sensors_event_t* s, uint32_t* pending_mask,
                             int index) // note that this is the handler for the android 'orientation' sensor, not the mpl orientation output
{
    VFUNC_LOG;
    inv_error_t res;
    float euler[3];
    float heading[1];
    float rot_mat[9];

    res = inv_get_float_array(INV_ROTATION_MATRIX, rot_mat);

    //ComputeAndOrientation(heading[0], euler, s->orientation.v);
    calcOrientationSensor(rot_mat, s->orientation.v);

    s->orientation.status
            = ((mMpuAccuracy < estimateCompassAccuracy()) ? mMpuAccuracy
                                                            : estimateCompassAccuracy());

    if (res == INV_SUCCESS)
        *pending_mask |= (1 << index);
    else
        LOGD("orien_handler: data not valid (%d)", (int) res);

}

int MPLSensor::enable(int32_t handle, int en)
{
    VFUNC_LOG;
    android::String8 sname;
    int what = -1;

    switch (handle) {
    case ID_A:
        what = Accelerometer;
        sname = "Accelerometer";
        break;
    case ID_M:
        what = MagneticField;
        sname = "MagneticField";
        break;
    case ID_O:
        what = Orientation;
        sname = "Orientation";
        break;
    case ID_GY:
        what = Gyro;
        sname = "Gyro";
        break;
    case ID_GR:
        what = Gravity;
        sname = "Gravity";
        break;
    case ID_RV:
        what = RotationVector;
        sname = "RotationVector";
        break;
    case ID_LA:
        what = LinearAccel;
        sname = "LinearAccel";
        break;
    default: //this takes care of all the gestures
        what = handle;
        sname = "Others";
        break;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    int newState = en ? 1 : 0;
    int err = 0;
    
    LOGV("enable - sensor %s (handle %d) %s -> %s", sname.string(), handle, 
            ((mEnabled & (1 << what)) ? "en" : "dis"),
            ((uint32_t(newState) << what) ? "en" : "dis"));
//    LOGV_IF((uint32_t(newState) << what) != (mEnabled & (1 << what)),
//            "%s sensor state change what=%d", sname.string(), what);

    pthread_mutex_lock(&mMplMutex);
    if ((uint32_t(newState) << what) != (mEnabled & (1 << what))) {
        uint32_t sensor_type;
        short flags = newState;
        mEnabled &= ~(1 << what);
        mEnabled |= (uint32_t(flags) << what);
        LOGV_IF(EXTRA_VERBOSE, "mEnabled = %x", mEnabled);
        setPowerStates(mEnabled);
        pthread_mutex_unlock(&mMplMutex);
        if (!newState)
            update_delay();
        return err;
    }
    pthread_mutex_unlock(&mMplMutex);
    
    return err;
}

int MPLSensor::setDelay(int32_t handle, int64_t ns)
{
    VFUNC_LOG;
    android::String8 sname;
    int what = -1;

    switch (handle) {
    case ID_A:
        what = Accelerometer;
        sname = "Accelerometer";
        break;
    case ID_M:
        what = MagneticField;
        sname = "MagneticField";
        break;
    case ID_O:
        what = Orientation;
        sname = "Orientation";
        break;
    case ID_GY:
        what = Gyro;
        sname = "Gyro";
        break;
    case ID_GR:
        what = Gravity;
        sname = "Gravity";
        break;
    case ID_RV:
        what = RotationVector;
        sname = "RotationVector";
        break;
    case ID_LA:
        what = LinearAccel;
        sname = "LinearAccel";
        break;
    default: //this takes care of all the gestures
        what = handle;
        sname = "Others";
        break;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    if (ns < 0)
        return -EINVAL;
        
    LOGV("setDelay - sensor %s (handle %d), rate %d ms (%.2f Hz)", 
         sname.string(), handle, (int)(ns / 1000000LL), 1000000000.f / ns);

    pthread_mutex_lock(&mMplMutex);
    mDelays[what] = ns;
    pthread_mutex_unlock(&mMplMutex);
    
    return update_delay();
}

int MPLSensor::update_delay()
{
    VFUNC_LOG;
    int rv = 0;
    bool irq_set[5];

    pthread_mutex_lock(&mMplMutex);

    if (mEnabled) {
        uint64_t wanted = -1LLU;
        
        // search the minimum delay requested across all enabled sensors
        for (int i = 0; i < numSensors; i++) {
            if (mEnabled & (1 << i)) {
                uint64_t ns = mDelays[i];
                wanted = wanted < ns ? wanted : ns;
            }
        }
        // Limit all rates to 100Hz max. => 10ms or 10000000ns
        if (wanted < 10000000LLU) {
            wanted = 10000000LLU;
        }

        // mpu fifo rate is in increments of 5ms
        int rate = (wanted / 5000000LLU) - ((wanted % 5000000LLU == 0) ? 1 : 0);
        if (rate == 0) // disallow fifo rate 0
            rate = 1;

        adjustFifoRate(rate);

        if (rate != mCurFifoRate) {
            LOGV("set fifo rate - divider : %d, delay : %llu ms (%.2f Hz)", 
                 rate, wanted / 1000000LL, 1000000000.f / wanted);
            inv_error_t res = inv_dmp_stop();
            LOGE_IF(res != INV_SUCCESS, "error stopping the DMP");
            res = inv_set_fifo_rate(rate);
            LOGE_IF(res != INV_SUCCESS, "error setting FIFO rate");
            res = inv_dmp_start();
            LOGE_IF(res != INV_SUCCESS, "error re-starting the DMP");

            mCurFifoRate = rate;
            rv = (res == INV_SUCCESS);
        }

        if (((inv_get_dl_config()->inv_mpu_cfg->requested_sensors & 
                INV_DMP_PROCESSOR) == 0)) {
            if (mUseTimerirq) {
                ioctl(mIrqFds.valueFor(TIMERIRQ_FD), TIMERIRQ_STOP, 0);
                clearIrqData(irq_set);
                if (inv_get_dl_config()->inv_mpu_cfg->requested_sensors
                        == INV_THREE_AXIS_COMPASS) {
                    ioctl(mIrqFds.valueFor(TIMERIRQ_FD), TIMERIRQ_START,
                          (unsigned long) (wanted / 1000000LLU));
                    LOGV_IF(EXTRA_VERBOSE, "updated timerirq period to %d",
                            (int) (wanted / 1000000LLU));
                } else {
                    ioctl(mIrqFds.valueFor(TIMERIRQ_FD), TIMERIRQ_START,
                          (unsigned long) inv_get_sample_step_size_ms());
                    LOGV_IF(EXTRA_VERBOSE, "updated timerirq period to %d",
                            (int) inv_get_sample_step_size_ms());
                }
            }
        }

    }
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

/* return the current time in nanoseconds */
int64_t MPLSensor::now_ns(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    //LOGV("Time %lld", (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec);
    return (int64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;
}

int MPLSensor::readEvents(sensors_event_t* data, int count)
{
    VFUNC_LOG;
    int i;
    bool irq_set[5] = {false, false, false, false, false};
    inv_error_t rv;
    if (count < 1)
        return -EINVAL;
    int numEventReceived = 0;

    clearIrqData(irq_set);

    pthread_mutex_lock(&mMplMutex);
    if (mDmpStarted) {
        //LOGV_IF(EXTRA_VERBOSE, "Update Data");
        rv = inv_update_data();
        LOGE_IF(rv != INV_SUCCESS, "inv_update_data error (code %d)", (int) rv);
    }

    else {
        //probably just one extra read after shutting down
        LOGV_IF(EXTRA_VERBOSE,
                "MPLSensor::readEvents called, but there's nothing to do.");
    }

    pthread_mutex_unlock(&mMplMutex);

    if (!mNewData) {
        LOGV_IF(EXTRA_VERBOSE, "no new data");
        return 0;
    }
    mNewData = 0;
    int64_t tt = now_ns();
    pthread_mutex_lock(&mMplMutex);
    for (int i = 0; i < numSensors; i++) {
        if (mEnabled & (1 << i)) {
            CALL_MEMBER_FN(this,mHandlers[i])(mPendingEvents + i,
                                              &mPendingMask, i);
            mPendingEvents[i].timestamp = tt;
        }
    }

    for (int j = 0; count && mPendingMask && j < numSensors; j++) {
        if (mPendingMask & (1 << j)) {
            mPendingMask &= ~(1 << j);
            if (mEnabled & (1 << j)) {
                *data++ = mPendingEvents[j];
                count--;
                numEventReceived++;
            }
        }

    }

    pthread_mutex_unlock(&mMplMutex);
    return numEventReceived;
}

int MPLSensor::getFd() const
{
    LOGV("MPLSensor::getFd returning %d", data_fd);
    return data_fd;
}

int MPLSensor::getAccelFd() const
{
    LOGV("MPLSensor::getAccelFd returning %d", accel_fd);
    return accel_fd;
}

int MPLSensor::getTimerFd() const
{
    LOGV("MPLSensor::getTimerFd returning %d", timer_fd);
    return timer_fd;
}

int MPLSensor::getPowerFd() const
{
    int hdl = (int) inv_get_serial_handle();
    LOGV("MPLSensor::getPowerFd returning %d", hdl);
    return hdl;
}

int MPLSensor::getPollTime()
{
    return mPollTime;
}

bool MPLSensor::hasPendingEvents() const
{
    //if we are using the polling workaround, force the main loop to check for data every time
    return (mPollTime != -1);
}

void MPLSensor::handlePowerEvent()
{
    VFUNC_LOG;
    mpuirq_data irqd;

    int fd = (int) inv_get_serial_handle();
    read(fd, &irqd, sizeof(irqd));

    if (irqd.data == MPU_PM_EVENT_SUSPEND_PREPARE) {
        //going to sleep
        sleepEvent();
    } else if (irqd.data == MPU_PM_EVENT_POST_SUSPEND) {
        //waking up
        wakeEvent();
    }

    ioctl(fd, MPU_PM_EVENT_HANDLED, 0);
}

void MPLSensor::sleepEvent()
{
    VFUNC_LOG;
    pthread_mutex_lock(&mMplMutex);
    if (mEnabled != 0) {
        mForceSleep = true;
        mOldEnabledMask = mEnabled;
        setPowerStates(0);
    }
    pthread_mutex_unlock(&mMplMutex);
}

void MPLSensor::wakeEvent()
{
    VFUNC_LOG;
    pthread_mutex_lock(&mMplMutex);
    if (mForceSleep) {
        setPowerStates((mOldEnabledMask | mEnabled));
    }
    mForceSleep = false;
    pthread_mutex_unlock(&mMplMutex);
}
