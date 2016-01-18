/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: mlsupervisor.c 6276 2011-11-09 22:40:46Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup   ML_SUPERVISOR
 *  @brief      Basic sensor fusion supervisor functionalities.
 *
 *  @{
 *      @file   mlsupervisor.c
 *      @brief  Basic sensor fusion supervisor functionalities.
 */

#include "ml.h"
#include "mldl.h"
#include "mldl_cfg.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "compass.h"
#include "pressure.h"
#include "dmpKey.h"
#include "dmpDefault.h"
#include "mlstates.h"
#include "mlFIFO.h"
#include "mlFIFOHW.h"
#include "mlMathFunc.h"
#include "mlsupervisor.h"
#include "mlmath.h"
#include "compass_supervisor.h"
#include "mlsl.h"
#include "mlos.h"

#include <log.h>
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-sup"

static long compassCalStableCount = 0;
static long compassCalCount = 0;
static long s_compass_test_bias[3]; // Compass bias in mounting frame, raw units * 2^16

#define SUPERVISOR_DEBUG 0

/**
 *  @brief  This initializes all variables that should be reset on
 */
void inv_init_sensor_fusion_supervisor(void)
{
    inv_obj.lite_fusion->acc_state = SF_STARTUP_SETTLE;
    compassCalStableCount = 0;
    compassCalCount = 0;
    if (inv_compass_present()) {
        struct mldl_cfg *mldl_cfg = inv_get_dl_config();
        if (mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS]->bus == 
            EXT_SLAVE_BUS_SECONDARY) {
            (void)inv_send_external_sensor_data(
                INV_ELEMENT_1 | INV_ELEMENT_2 | INV_ELEMENT_3, INV_16_BIT);
        }
    }

}

static int MLUpdateCompassCalibration3DOF(int command, long *data,
                                          unsigned long deltaTime)
{
    INVENSENSE_FUNC_START;
    int retValue = INV_SUCCESS;
    static float m[10][10] = { {0} };
    float mInv[10][10] = { {0} };
    float mTmp[10][10] = { {0} };
    static float xTransY[4] = { 0 };
    float magSqr = 0;
    float inpData[3] = { 0 };
    int i, j;
    int a, b;
    float d;
    switch (command) {
    case CAL_ADD_DATA:
        inpData[0] = (float)data[0];
        inpData[1] = (float)data[1];
        inpData[2] = (float)data[2];
        m[0][0] += (-2 * inpData[0]) * (-2 * inpData[0]);
        m[0][1] += (-2 * inpData[0]) * (-2 * inpData[1]);
        m[0][2] += (-2 * inpData[0]) * (-2 * inpData[2]);
        m[0][3] += (-2 * inpData[0]);
        m[1][0] += (-2 * inpData[1]) * (-2 * inpData[0]);
        m[1][1] += (-2 * inpData[1]) * (-2 * inpData[1]);
        m[1][2] += (-2 * inpData[1]) * (-2 * inpData[2]);
        m[1][3] += (-2 * inpData[1]);
        m[2][0] += (-2 * inpData[2]) * (-2 * inpData[0]);
        m[2][1] += (-2 * inpData[2]) * (-2 * inpData[1]);
        m[2][2] += (-2 * inpData[2]) * (-2 * inpData[2]);
        m[2][3] += (-2 * inpData[2]);
        m[3][0] += (-2 * inpData[0]);
        m[3][1] += (-2 * inpData[1]);
        m[3][2] += (-2 * inpData[2]);
        m[3][3] += 1.0f;
        magSqr =
            inpData[0] * inpData[0] + inpData[1] * inpData[1] +
            inpData[2] * inpData[2];
        xTransY[0] += (-2 * inpData[0]) * magSqr;
        xTransY[1] += (-2 * inpData[1]) * magSqr;
        xTransY[2] += (-2 * inpData[2]) * magSqr;
        xTransY[3] += magSqr;
        break;
    case CAL_RUN:
        a = 4;
        b = a;
        for (i = 0; i < b; i++) {
            for (j = 0; j < b; j++) {
                a = b;
                inv_matrix_det_inc(&m[0][0], &mTmp[0][0], &a, i, j);
                mInv[j][i] = SIGNM(i + j) * inv_matrix_det(&mTmp[0][0], &a);
            }
        }
        a = b;
        d = inv_matrix_det(&m[0][0], &a);
        if (d == 0) {
            return INV_ERROR;
        }
        for (i = 0; i < 3; i++) {
            float tmp = 0;
            for (j = 0; j < 4; j++) {
                tmp += mInv[j][i] / d * xTransY[j];
            }
            s_compass_test_bias[i] = -(long)(tmp * (1L<<16));
        }
        break;
    case CAL_RESET:
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                m[i][j] = 0;
            }
            xTransY[i] = 0;
        }
    default:
        break;
    }
    return retValue;
}

inv_error_t inv_simple_compass_cal(struct compass_obj_t *compass_obj)
{
    static long magMax[3] = {
        -1073741824L,
        -1073741824L,
        -1073741824L
    };
    static long magMin[3] = {
        1073741824L,
        1073741824L,
        1073741824L
    };
    int gyroMag;
    int i;


    //Most basic compass calibration, used only with lite MPL
    gyroMag = inv_get_gyro_sum_of_sqr() >> GYRO_MAG_SQR_SHIFT;


    if (inv_obj.adv_fusion->resetting_compass == 1) {
        for (i = 0; i < 3; i++) {
            magMax[i] = -1073741824L;
            magMin[i] = 1073741824L;
        }
        compassCalStableCount = 0;
        compassCalCount = 0;
        inv_obj.adv_fusion->resetting_compass = 0;
    }
    if ((gyroMag > 400) || (inv_get_gyro_present() == 0)) {
        if (compassCalStableCount < 1000) {
            for (i = 0; i < 3; i++) {
                if (inv_obj.mag->sensor_data[i] > magMax[i]) {
                    magMax[i] = inv_obj.mag->sensor_data[i];
                }
                if (inv_obj.mag->sensor_data[i] < magMin[i]) {
                    magMin[i] = inv_obj.mag->sensor_data[i];
                }
            }
            MLUpdateCompassCalibration3DOF(CAL_ADD_DATA,
                                            inv_obj.mag->sensor_data,
                                            compass_obj->delta_time);
            compassCalStableCount += compass_obj->delta_time;
            for (i = 0; i < 3; i++) {
                if ( inv_q30_mult(magMax[i] - magMin[i], inv_obj.mag->sens) < 40 ) {
                    compassCalStableCount = 0;
                }
            }
        } else {
            if (compassCalStableCount >= 1000) {
                if (MLUpdateCompassCalibration3DOF
                    (CAL_RUN, inv_obj.mag->sensor_data,
                        compass_obj->delta_time) == INV_SUCCESS) {
                    inv_obj.adv_fusion->got_compass_bias = 1;
                    inv_obj.adv_fusion->compass_accuracy = 3;
                    for (i = 0; i < 3; i++) {
                        inv_obj.adv_fusion->compass_bias_error[i] = 35;
                    }

                    if (inv_obj.adv_fusion->compass_state == SF_UNCALIBRATED)
                        inv_obj.adv_fusion->compass_state = SF_STARTUP_SETTLE;
                    inv_set_compass_bias(NULL, s_compass_test_bias);
                }
                compassCalCount = 0;
                compassCalStableCount = 0;
                for (i = 0; i < 3; i++) {
                    magMax[i] = -1073741824L;
                    magMin[i] = 1073741824L;
                }
                MLUpdateCompassCalibration3DOF(CAL_RESET,
                                                inv_obj.mag->sensor_data,
                                                compass_obj->delta_time);
            }
        }
    }
    compassCalCount += compass_obj->delta_time;
    if (compassCalCount > 20000) {
        compassCalCount = 0;
        compassCalStableCount = 0;
        for (i = 0; i < 3; i++) {
            magMax[i] = -1073741824L;
            magMin[i] = 1073741824L;
        }
        MLUpdateCompassCalibration3DOF(CAL_RESET,
                                        inv_obj.mag->sensor_data,
                                        compass_obj->delta_time);
    }

    return INV_SUCCESS;
}


/**
 *  @brief  Entry point for software sensor fusion operations.
 *          Manages hardware interaction, calls sensor fusion supervisor for
 *          bias calculation.
 *  @return INV_SUCCESS or non-zero error code on error.
 */
inv_error_t inv_pressure_supervisor(void)
{
    long pressureSensorData[1];
    static unsigned long pressurePolltime = 0;
    if (inv_pressure_present()) {   /* check for pressure data */
        unsigned long ctime = inv_get_tick_count();
        if ((pressurePolltime == 0 || ((ctime - pressurePolltime) > 80))) { //every 1/8 second
            if (SUPERVISOR_DEBUG) {
                MPL_LOGV("Fetch pressure data\n");
                MPL_LOGV("delta time = %ld\n", ctime - pressurePolltime);
            }
            pressurePolltime = ctime;
            if (inv_get_pressure_data(&pressureSensorData[0]) == INV_SUCCESS) {
                inv_obj.pressure->meas = pressureSensorData[0];
            }
        }
    }
    return INV_SUCCESS;
}

/**
 *  @}
 */
