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

#undef  NDEBUG
#define NDEBUG 0

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/select.h>
#include <dlfcn.h>
#include <pthread.h>

#include <cutils/log.h>
#include <utils/KeyedVector.h>

#include "math.h"
#include "ml.h"
#include "mldl_cfg.h"
#include "mldl.h"
#include "ml_mputest.h"

extern "C" {
#include "mlsupervisor.h"
}

//#include "MPLSensorSysApi.h"

MplSys_Interface* getSysInterfaceObject()
{
    MPLSensorSysApi* s = static_cast<MPLSensorSysApi*>(MPLSensor::gMPLSensor);
    return static_cast<MplSys_Interface*>(s);
}

MPLSensorSysApi::MPLSensorSysApi() : MPLSensor()
{

}

MPLSensorSysApi::~MPLSensorSysApi()
{

}

MPLSensorSysApi::MplSys_Interface::~MplSys_Interface()
{

}

int MPLSensorSysApi::getBiases(float *b)
{
    FUNC_LOG;
    int rv;
    float err[3];
    ALOGV("get biases\n");
    pthread_mutex_lock(&mMplMutex);
    rv = inv_get_float_array(INV_ACCEL_BIAS, b);
    rv += inv_get_float_array(INV_GYRO_BIAS, &(b[3]));
    rv += inv_get_float_array(INV_MAG_BIAS, &(b[6]));
    inv_get_float_array(INV_MAG_BIAS_ERROR, err);
    pthread_mutex_unlock(&mMplMutex);
    ALOGV("rpcGetBiases: %f %f %f - %f %f %f - %f %f %f", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8]);
    ALOGV("rpcGetBiases: %f %f %f", err[0], err[1], err[2]);
    return rv;
}

int MPLSensorSysApi::setBiases(float *b)
{
    FUNC_LOG;
    int rv;
    pthread_mutex_lock(&mMplMutex);
    rv = inv_set_float_array(INV_ACCEL_BIAS, b);
    rv += inv_set_float_array(INV_GYRO_BIAS, b + 3);
    rv += inv_set_float_array(INV_MAG_BIAS, b + 6);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::setBiasUpdateFunc(long f)
{
    FUNC_LOG;
    /*
    int rv;
    pthread_mutex_lock(&mMplMutex);
    rv = inv_set_bias_update((unsigned short) f);
    ALOGE_IF(rv!=INV_SUCCESS, "SysApi :: setBiasUpdateFunc failed (f=%lx rv=%d)", f, rv);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
    */
    ALOGW("SysApi :: setBiasUpdateFunc is OBSOLETE and ineffective");
    return 0;
}

int MPLSensorSysApi::setSensors(long s)
{
    FUNC_LOG;
    int rv = INV_SUCCESS;

    pthread_mutex_lock(&mMplMutex);
    mMasterSensorMask = s;
    setPowerStates(mEnabled);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::getSensors(long* s)
{
    FUNC_LOG;
    int rv;
    pthread_mutex_lock(&mMplMutex);
    *s = inv_get_dl_config()->inv_mpu_cfg->requested_sensors;
    rv = 0;
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::resetCal()
{
    FUNC_LOG;
    int rv;
    pthread_mutex_lock(&mMplMutex);
    /* this api is removed in 4.1 */
    rv = INV_SUCCESS;//inv_reset_compass_calibration();
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::selfTest()
{
    FUNC_LOG;
    int rv;
    pthread_mutex_lock(&mMplMutex);
    do {
        rv = inv_self_test_set_accel_z_orient(1); //accel z is inverted
        if (rv != INV_SUCCESS) {
            ALOGE("error inv_self_test_set_accel_z_orient returned %d", rv);
            break;
        }
        rv = inv_self_test_run();
        if (rv != INV_SUCCESS) {
            ALOGE("error inv_self_test_run returned %d", rv);
            break;
        }
    } while (0);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::setLocalMagField(float x, float y, float z)
{
    FUNC_LOG;
    int rv;
    float data[3] = {x, y, z};
    pthread_mutex_lock(&mMplMutex);
    rv = inv_set_float_array(INV_LOCAL_FIELD, data);
    ALOGE_IF(rv != INV_SUCCESS, "error: setLocalMagField -- inv_set_float_array returned %d",rv);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}


