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
#include "mlFIFO.h"

extern "C" {
#include "mlsupervisor.h"
}

#include "MPLSensorSysPed.h"

extern "C" {
//callback wrappers go here
void step_cb_wrapper(unsigned long val, unsigned long wtime)
{
    if (MPLSensor::gMPLSensor) {
        static_cast<MPLSensorSysPed*>(MPLSensor::gMPLSensor)->onStepCb(val, wtime);
    }
}
void step_cb_wrapper_fp(unsigned long val, double wtime)
{
    unsigned long wt = wtime;
    if (MPLSensor::gMPLSensor) {
        static_cast<MPLSensorSysPed*>(MPLSensor::gMPLSensor)->onStepCb(val, wt);
    }
}
} //end of extern C

MplSysPed_Interface* getSysPedInterfaceObject()
{
    MPLSensorSysPed* s = static_cast<MPLSensorSysPed*>(MPLSensor::gMPLSensor);
    return static_cast<MplSysPed_Interface*>(s);
}

/* ************** Invensense Library interactions  ******************* */
MPLSensorSysPed::MPLSensorSysPed() : MPLSensorSysApi()
{
    mPedSteps = 0;
    mPedWt = 0.0;

    mPedState=0;
    mSysPedEnabled = false;
    mStartSysPed = false;

    mStepParams.threshold = 25000000L;
    mStepParams.minUpTime = 16*20;
    mStepParams.maxUpTime = 60*20;
    mStepParams.minSteps = 5;
    mStepParams.minEnergy = 0x0d000000L;
    mStepParams.maxStepBufferTime = 125*20;
    mStepParams.clipThreshold = 0x06000000L;

}

MPLSensorSysPed::~MPLSensorSysPed()
{


}

/* override base-class version */
void MPLSensorSysPed::computeLocalSensorMask(int enabled_sensors)
{
    MPLSensor::computeLocalSensorMask(enabled_sensors);

    //we want the gyro + accel if any other sensor is going to be on
    if(mSysPedEnabled && mLocalSensorMask) {
        LOGV_IF(EXTRA_VERBOSE, "sys ped enabled -- bringing up DMP + Accel");
        mLocalSensorMask |= (INV_THREE_AXIS_ACCEL + INV_THREE_AXIS_GYRO);
        LOGV_IF(EXTRA_VERBOSE, "local_sensor_mask %ld", local_sensor_mask);
    }
}

/* override base-class version */
bool MPLSensorSysPed::needStateChange(bool changing_sensors, bool restart)
{
    bool irqSet[5] = {false, false, false, false, false};
    
    // make sure we put the DMP back in it's regular state if the stand alone
    // pedometer is running
    if ((mPedState == PED_STANDALONE  || mPedState == PED_SLEEP) && restart) {
        unsigned long cursteps;
        double curwt;
        
        if(mPedState == PED_SLEEP){     /* sleep standalone */
            inv_set_low_power_pedometer_num_of_steps(mPedSteps);
            inv_set_low_power_pedometer_walk_time(mPedWt);
            inv_start_low_power_pedometer();
            mPedState = PED_STANDALONE;
        }
        
        LOGV_IF(EXTRA_VERBOSE, "sys ped enabled, restarting, switching to full power ped");
        inv_get_low_power_pedometer_num_of_steps(&cursteps);
        inv_get_low_power_pedometer_walk_time(&curwt);
        if(mPedState == PED_STANDALONE) {
            inv_stop_low_power_pedometer();
        }
        inv_close_low_power_pedometer();
        mPedSteps = cursteps;
        mPedWt = curwt;
        clearIrqData(irqSet);
        initMPL();
        setupFIFO();
        setupPedFp();   // also adjusts the fifo rate to be 6 minimum
    }

    return (MPLSensor::needStateChange(changing_sensors, restart) || mStartSysPed);
}

/* override base-class version */
bool MPLSensorSysPed::needDMPStop()
{
    // we need to stop the DMP in order to restart in pedometer mode
    return mDmpStarted || mStartSysPed;
}

void MPLSensorSysPed::enableFeatures()
{
    if(mStartSysPed) {
        setupPedFp();
    }
}

void MPLSensorSysPed::adjustFifoRate(int& rate)
{
    if(mSysPedEnabled && (rate > 6)) {
        rate = 6;
    }
}

void MPLSensorSysPed::shutdownFeatures()
{
    //put the DMP in standalone ped mode -- any other features won't know about this
    // state, so we must restore the default state before any wakeup
    if(mSysPedEnabled && mPedState == PED_FULL) {
         LOGV_IF(EXTRA_VERBOSE, "sys ped enabled, switching to StandAlone mode");
         inv_dmp_close();
         inv_open_low_power_pedometer();
         inv_set_low_power_pedometer_num_of_steps(mPedSteps);
         inv_set_low_power_pedometer_walk_time(mPedWt);
         inv_set_low_power_pedometer_params(&mStepParams);
         inv_start_low_power_pedometer();
         mPedState = PED_STANDALONE;
    }
}

void MPLSensorSysPed::onStepCb(unsigned long val, unsigned long wtime)
{
    mPedSteps = val;
    mPedWt    = wtime;
}

void MPLSensorSysPed::setupPedFp()
{
    VFUNC_LOG;
    inv_error_t result;

    mPedState = PED_FULL;

    if (mCurFifoRate > 6) {
        LOGV_IF(EXTRA_VERBOSE, "need to adjust fifo rate (cur = %d)", mCurFifoRate);
        result = inv_set_fifo_rate(6);
        LOGE_IF(result != INV_SUCCESS, "setupPedFp : failed to adjust fifo rate");
        mCurFifoRate = 6;
    }
    LOGV("fifo rate - divider : %d, delay : %llu ns", 
         mCurFifoRate, (unsigned long long)mCurFifoRate * 5000000LLU);

    result = inv_enable_full_power_pedometer();
    if (result != INV_SUCCESS) {
        LOGE("Fatal error: inv_enable_full_power_pedometer returned %d\n", result);
    }

    result = inv_set_full_power_pedometer_params(&mStepParams);
    if (result != INV_SUCCESS) {
        LOGE("Fatal error: inv_set_full_power_pedometer_params returned %d\n", result);
    }

    result = inv_set_full_power_pedometer_step_callback(step_cb_wrapper_fp);
    if (result != INV_SUCCESS) {
        LOGE("Fatal error: inv_set_full_power_pedometer_step_callback returned %d\n", result);
    }

    result = inv_set_full_power_pedometer_step_count(mPedSteps);
    result = inv_set_full_power_pedometer_walk_time(mPedWt);

    LOGE_IF(result != INV_SUCCESS, "Fatal error: inv_set_full_power_pedometer_step_count failed (%d)", result);

}

int MPLSensorSysPed::readEvents(sensors_event_t* data, int count)
{
    //VFUNC_LOG;
    int i;
    bool irqSet[5] = {false, false, false, false, false};
    inv_error_t rv;
    if (count < 1)
        return -EINVAL;
    int numEventReceived = 0;

    clearIrqData(irqSet);

    pthread_mutex_lock(&mMplMutex);
    if (mDmpStarted) {
        //LOGV_IF(EXTRA_VERBOSE, "Update Data");
        rv = inv_update_data();
        LOGE_IF(rv != INV_SUCCESS, "inv_update_data error (code %d)", (int) rv);
    } else if((mPedState == PED_SLEEP || mPedState == PED_STANDALONE) && irqSet[ACCELIRQ_FD]) {
        LOGV_IF(EXTRA_VERBOSE, "Possible Ped event");
        //int idx = mIrqFds.indexOfKey(ACCELIRQ_FD);
        //LOGE_IF(idx < 1, "ERROR -- accel irq not present.  pedometer will not function");
        if(mPedState == PED_SLEEP && irqSet[ACCELIRQ_FD]) {
            // accel int -- turn ped back on
            LOGV_IF(EXTRA_VERBOSE, "motion int -- turn ped back on");
            int idx = mIrqFds.indexOfKey(ACCELIRQ_FD);
            LOGE_IF(idx < 1, "ERROR -- accel irq not present.  pedometer will not function");
            inv_error_t e = inv_set_low_power_pedometer_num_of_steps(mPedSteps);
            LOGE_IF(e != INV_SUCCESS, "failed to reset step count in Standalone Pedometer on motion int");
            inv_set_low_power_pedometer_walk_time(mPedWt);
            e = inv_start_low_power_pedometer();
            LOGE_IF(e != INV_SUCCESS, "failed to restart standalone pedometer on motion int");
            mPedState = PED_STANDALONE;
        } else if (mPedState == PED_STANDALONE && irqSet[ACCELIRQ_FD]) {
            LOGV_IF(EXTRA_VERBOSE, "no-motion int -- sleep sensors");
            // accel int -- switch to sleep
            unsigned long cursteps; /* sleep */
            double curwt;
            inv_get_low_power_pedometer_num_of_steps(&cursteps);
            inv_get_low_power_pedometer_walk_time(&curwt);
            mPedSteps = cursteps;
            mPedWt = curwt;
            int idx = mIrqFds.indexOfKey(ACCELIRQ_FD);
            LOGE_IF(idx < 1, "ERROR -- accel irq not present.  pedometer will not function");
            inv_error_t e = inv_stop_low_power_pedometer();
            LOGE_IF(e != INV_SUCCESS, "failed to sleep pedometer on no-motion event");
            mPedState = PED_SLEEP;
        }
    } else {
        //probably just one extra read after shutting down
        LOGV_IF(EXTRA_VERBOSE,
                "MPLSensorSysPed::readEvents called, but there's nothing to do.");
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

/****************** RPC interface implementation ********************* */

MPLSensorSysPed::MplSysPed_Interface::~MplSysPed_Interface()
{

}

int MPLSensorSysPed::rpcStartPed()
{
    VFUNC_LOG;
    pthread_mutex_lock(&mMplMutex);
    if(mEnabled == 0 && mPedState == PED_NONE) {
        //all sensors off and no ped running
        LOGV_IF(EXTRA_VERBOSE, "sys starting standalone pedometer");
        inv_dmp_close();
        inv_open_low_power_pedometer();
        inv_set_low_power_pedometer_num_of_steps(mPedSteps);
        inv_set_low_power_pedometer_walk_time(mPedWt);
        inv_set_low_power_pedometer_params(&mStepParams);
        inv_start_low_power_pedometer();
        mPedState = PED_STANDALONE;
        mSysPedEnabled= true;
    } else if (mPedState == PED_NONE) {
        LOGV_IF(EXTRA_VERBOSE, "sys starting fullpower pedometer");
        mSysPedEnabled = true;
        mPedState = PED_FULL;
        mStartSysPed = true;
        setPowerStates(mEnabled);
        mStartSysPed = false;
    }
    pthread_mutex_unlock(&mMplMutex);
    
    return INV_SUCCESS;
}

int MPLSensorSysPed::rpcStopPed()
{
    VFUNC_LOG;
    if(!mSysPedEnabled)
        return INV_SUCCESS;

    pthread_mutex_lock(&mMplMutex);

    if(mPedState == PED_STANDALONE) {
        LOGV_IF(EXTRA_VERBOSE, "sys stopping standalone pedometer");
        inv_stop_low_power_pedometer();
    }

    if(mPedState == PED_SLEEP || mPedState == PED_STANDALONE) {
        LOGV_IF(EXTRA_VERBOSE && mPedState == PED_SLEEP, "sys ped was asleep");
        inv_close_low_power_pedometer();
        mPedState = PED_NONE;
        mPedSteps = 0;
        mPedWt = 0;
        initMPL();
        setupFIFO();
    }

    if(mPedState == PED_FULL) {
        LOGV_IF(EXTRA_VERBOSE, "sys stopping full power pedometer");
        inv_disable_full_power_pedometer();
        mPedState = PED_NONE;
        mPedSteps = 0;
        mPedWt = 0;
    }

    mSysPedEnabled = false;

    mStartSysPed = false;
    setPowerStates(mEnabled);

    pthread_mutex_unlock(&mMplMutex);

    //update delay locks the mutex, as it is callable from the sm context
    update_delay();

    return INV_SUCCESS;

}

int MPLSensorSysPed::rpcGetSteps()
{
    VFUNC_LOG;
    unsigned long steps = 0;

    pthread_mutex_lock(&mMplMutex);
    if(mPedState == PED_FULL) {
        steps = mPedSteps;
    } else if(mPedState == PED_STANDALONE) {
        if(inv_get_low_power_pedometer_num_of_steps(&steps) == INV_SUCCESS) {
            mPedSteps = steps;
        }
    } else if(mPedState == PED_SLEEP) {
        steps = mPedSteps; //if state is PED_SLEEP, just return the current count
    }

    pthread_mutex_unlock(&mMplMutex);
    LOGV_IF(EXTRA_VERBOSE, "getSteps returning %d", (int)steps);
    return (int)steps;
}

double MPLSensorSysPed::rpcGetWalkTime()
{
    VFUNC_LOG;
    double timeMs = 0.0;
    pthread_mutex_lock(&mMplMutex);
    if(mPedState == PED_FULL) {
        if(inv_get_full_power_pedometer_walk_time(&timeMs) == INV_SUCCESS)
            mPedWt = timeMs;
    } else if(mPedState == PED_STANDALONE ) {
        if(inv_get_low_power_pedometer_walk_time(&timeMs) == INV_SUCCESS)
            mPedWt = timeMs;
    } else if(mPedState == PED_SLEEP){
        timeMs = mPedWt; //if state is PED_SLEEP, just return the current wt
    }

    pthread_mutex_unlock(&mMplMutex);
    return timeMs;
}

int MPLSensorSysPed::rpcClearPedData()
{
    VFUNC_LOG;
    pthread_mutex_lock(&mMplMutex);
    if(mPedState == PED_FULL) {
        inv_set_full_power_pedometer_step_count(0);
        inv_set_full_power_pedometer_walk_time(0);
    } else if(mPedState == PED_STANDALONE) {
        inv_set_low_power_pedometer_num_of_steps(0);
        inv_set_low_power_pedometer_walk_time(0);
    }
    mPedSteps = 0;
    mPedWt = 0;
    pthread_mutex_unlock(&mMplMutex);
    return INV_SUCCESS;
}



