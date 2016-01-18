/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: mlarray_adv.c 6075 2011-09-23 03:59:04Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup MLARRAY
 *  @{
 *      @file   mlarray_adv.c
 *      @brief  APIs to read different data sets from FIFO.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */
#include "ml.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "mlMathFunc.h"
#include "mlmath.h"
#include "mlstates.h"
#include "mlFIFO.h"
#include "mlsupervisor.h"
#include "mldl.h"
#include "dmpKey.h"
#include "compass.h"
#include "temp_comp.h"

#ifndef INV_FEATURE_ADVFUSION
#error mlarray_adv.c only used with INV_FEATURE_ADVFUSION.
#endif

/**
 *  @cond MPL
 *  @brief  inv_get_mag_bias_error is used to get magnetometer Bias error.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_bias_error(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    if (inv_obj.adv_fusion->large_field == 0) {
        data[0] = inv_obj.adv_fusion->compass_bias_error[0];
        data[1] = inv_obj.adv_fusion->compass_bias_error[1];
        data[2] = inv_obj.adv_fusion->compass_bias_error[2];
    } else {
        data[0] = P_INIT;
        data[1] = P_INIT;
        data[2] = P_INIT;
    }

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_scale is used to get magnetometer scale.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_scale(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.adv_fusion->compass_scale[0];
    data[1] = inv_obj.adv_fusion->compass_scale[1];
    data[2] = inv_obj.adv_fusion->compass_scale[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_local_field is used to get local magnetic field data.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_local_field(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.adv_fusion->local_field[0];
    data[1] = inv_obj.adv_fusion->local_field[1];
    data[2] = inv_obj.adv_fusion->local_field[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_bias_error_float is used to get an array of three numbers representing
 *			the current estimated error in the compass biases. These numbers are unitless and serve
 *          as rough estimates in which numbers less than 100 typically represent
 *          reasonably well calibrated compass axes.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_bias_error_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    if (inv_obj.adv_fusion->large_field == 0) {
        data[0] = (float)inv_obj.adv_fusion->compass_bias_error[0];
        data[1] = (float)inv_obj.adv_fusion->compass_bias_error[1];
        data[2] = (float)inv_obj.adv_fusion->compass_bias_error[2];
    } else {
        data[0] = (float)P_INIT;
        data[1] = (float)P_INIT;
        data[2] = (float)P_INIT;
    }

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_scale_float is used to get magnetometer scale.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_scale_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.adv_fusion->compass_scale[0] / 65536.0f;
    data[1] = (float)inv_obj.adv_fusion->compass_scale[1] / 65536.0f;
    data[2] = (float)inv_obj.adv_fusion->compass_scale[2] / 65536.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_local_field_float is used to get local magnetic field data.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_local_field_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.adv_fusion->local_field[0] / 65536.0f;
    data[1] = (float)inv_obj.adv_fusion->local_field[1] / 65536.0f;
    data[2] = (float)inv_obj.adv_fusion->local_field[2] / 65536.0f;

    return result;
}

/**
 * Returns the curren compass accuracy.
 *
 * - 0: Unknown: The accuracy is unreliable and compass data should not be used
 * - 1: Low: The compass accuracy is low.
 * - 2: Medium: The compass accuracy is medium.
 * - 3: High: The compas acurracy is high and can be trusted
 *
 * @param accuracy The accuracy level in the range 0-3
 *
 * @return ML_SUCCESS or non-zero error code
 */
inv_error_t inv_get_compass_accuracy(int *accuracy)
{
    if (inv_get_state() != INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    *accuracy = inv_obj.adv_fusion->compass_accuracy;
    return INV_SUCCESS;
}

/**
 *  @cond MPL
 *  @brief  inv_set_local_field is used to set local magnetic field
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_local_field(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    inv_obj.adv_fusion->local_field[0] = data[0];
    inv_obj.adv_fusion->local_field[1] = data[1];
    inv_obj.adv_fusion->local_field[2] = data[2];
    inv_obj.adv_fusion->new_local_field = 1;

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @cond MPL
 *  @brief  inv_set_mag_scale is used to set magnetometer scale
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_mag_scale(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    inv_obj.adv_fusion->compass_scale[0] = data[0];
    inv_obj.adv_fusion->compass_scale[1] = data[1];
    inv_obj.adv_fusion->compass_scale[2] = data[2];

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @cond MPL
 *  @brief  inv_set_local_field_float is used to set local magnetic field
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_local_field_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_local_field(arrayTmp);
}

/**
 *  @cond MPL
 *  @brief  inv_set_mag_scale_float is used to set magnetometer scale
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_mag_scale_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_mag_scale(arrayTmp);
}

