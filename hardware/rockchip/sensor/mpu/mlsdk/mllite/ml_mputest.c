/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: ml_mputest.c 6132 2011-10-01 03:17:27Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup MPU_SELF_TEST
 *  @brief  C wrapper to integrate the MPU Self Test wrapper in MPL.
 *          Provides ML name compliant naming and an additional API that
 *          automates the suspension of normal MPL operations, runs the test,
 *          and resume.
 *
 *  @{
 *      @file   ml_mputest.c
 *      @brief  C wrapper to integrate the MPU Self Test wrapper in MPL.
 *              The main logic of the test and APIs can be found in mputest.c
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "mputest.h"
#include "ml_mputest.h"

#include "mlmath.h"
#include "mlinclude.h"
#include "ml.h"
#include "mlstates.h"
#include "mldl.h"
#include "mldl_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    Globals
*/
extern signed char g_z_sign;

/**
 *  @brief  Runs the MPU Self Test at MPL runtime (short version).
 *          If the DMP is operating, stops the DMP temporarely,
 *          runs the MPU Self Test, and re-starts the DMP.
 *
 *  @return INV_SUCCESS or a non-zero error code otherwise.
 */
inv_error_t inv_self_test_run(void)
{
    if (inv_get_state() < INV_STATE_DMP_OPENED) {
        MPL_LOGE("Self Test cannot run before inv_dmp_open()\n");
        return INV_ERROR_SM_IMPROPER_STATE;
    }
    return inv_device_test(inv_get_serial_handle(),
                           INV_SIX_AXIS_GYRO_ACCEL, false, true);
}

/**
 *  @brief  Runs the MPU Calibration Test at MPL runtime (long version).
 *          If the DMP is operating, stops the DMP temporarely,
 *          runs the MPU Calibration Test, and re-starts the DMP.
 *
 *  @return INV_SUCCESS or a non-zero error code otherwise.
 */
inv_error_t inv_self_test_calibration_run(void)
{
    if (inv_get_state() < INV_STATE_DMP_OPENED) {
        MPL_LOGE("Self Test cannot run before inv_dmp_open()\n");
        return INV_ERROR_SM_IMPROPER_STATE;
    }
    return inv_device_test(inv_get_serial_handle(),
                           INV_SIX_AXIS_GYRO_ACCEL, true, true);
}

/**
 *  @brief  Runs the MPU test for bias correction only at MPL runtime
 *          (the short version is run but test results are ignored).
 *          If the DMP is operating, stops the DMP temporarely,
 *          runs the bias calculation routines, and re-starts the DMP.
 *
 *  @return INV_SUCCESS or a non-zero error code otherwise.
 */
inv_error_t inv_self_test_bias_run(void)
{
    if (inv_get_state() < INV_STATE_DMP_OPENED) {
        MPL_LOGE("Self Test cannot run before inv_dmp_open()\n");
        return INV_ERROR_SM_IMPROPER_STATE;
    }
    return inv_device_test(inv_get_serial_handle(),
                           INV_SIX_AXIS_GYRO_ACCEL, false, false);
}

/**
 *  @brief  Runs the Accelerometer Calibration Test at MPL runtime.
 *          If the DMP is operating, stops the DMP temporarely,
 *          runs the Accel Calibration Test, and re-starts the DMP.
 *
 *  @return INV_SUCCESS or a non-zero error code otherwise.
 */
inv_error_t inv_self_test_accel_z_run(void)
{
    if (inv_get_state() < INV_STATE_DMP_OPENED) {
        MPL_LOGE("Self Test cannot run before inv_dmp_open()\n");
        return INV_ERROR_SM_IMPROPER_STATE;
    }
    return inv_device_test(inv_get_serial_handle(),
                           INV_Z_ACCEL, false, false);
}

/**
 *  @brief  Set the orientation of the acceleroemter Z axis as it will be
 *          expected when running the MPU Self Test.
 *          Specifies the orientation of the accelerometer Z axis : Z axis
 *          pointing upwards or downwards.
 *  @param  z_sign
 *              The sign of the accelerometer Z axis; valid values are +1 and
 *              -1 for +Z and -Z respectively.  Any other value will cause the
 *              setting to be ignored and an error code to be returned.
 *              Note that this setting is an override on top of the chip
 *              mounting matrix in use and its purpose is to allow the accel
 *              test to be run with the -Z axis facing up instead of +Z axis.
 *  @return INV_SUCCESS or a non-zero error code.
 */
inv_error_t inv_self_test_set_accel_z_orient(signed char z_sign)
{
    if (z_sign != +1 && z_sign != -1) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    g_z_sign = z_sign;
    return INV_SUCCESS;
}


#ifdef __cplusplus
}
#endif

/**
 *  @}
 */

