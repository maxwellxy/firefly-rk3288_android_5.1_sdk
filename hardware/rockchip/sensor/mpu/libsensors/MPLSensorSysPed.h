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

#ifndef INV_HAL_SYS_PED_H
#define INV_HAL_SYS_PED_H

#include "sensors.h"
#include "MPLSensorSysApi.h"
#include <gui/MplInterfaces.h>

#include "mlpedometer_fullpower.h"
#include "mlpedometer_lowpower.h"


class MPLSensorSysPed : public MPLSensorSysApi, public MplSysPed_Interface {

public:
    MPLSensorSysPed();
    virtual ~MPLSensorSysPed();

    enum PED_STATE {
        PED_NONE,
        PED_STANDALONE,
        PED_FULL,
        PED_SLEEP
    };

    int mPedState;                 //current pedometer state (NONE, STANDALONE, FULL)
    struct stepParams mStepParams;
    long mPedSteps;            //pedometer steps recorded
    double mPedWt;
    bool mSysPedEnabled;  //flag indicating if the sys-api ped is enabled
    bool mStartSysPed;

    virtual void computeLocalSensorMask(int);
    virtual bool needStateChange(bool, bool);
    virtual bool needDMPStop();
    virtual void enableFeatures();
    virtual void adjustFifoRate(int&);
    virtual void shutdownFeatures();
    void onStepCb(unsigned long, unsigned long);
    void setupPedFp();
    virtual int readEvents(sensors_event_t* data, int count);

    virtual int rpcStartPed();
    virtual int rpcStopPed();
    virtual int rpcGetSteps();
    virtual double rpcGetWalkTime();
    virtual int rpcClearPedData();

};

extern "C" {
MplSysPed_Interface* getSysPedInterfaceObject();
}
#endif

