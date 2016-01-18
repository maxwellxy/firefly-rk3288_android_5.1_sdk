/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: temp_comp_legacy.c 6132 2011-10-01 03:17:27Z mcaramello $
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "temp_comp.h"

#include "ml.h"
#include "mlFIFO.h"
#include "mlmath.h"
#include "mlsupervisor.h"
#include "mlMathFunc.h"
#include "mlos.h"
#include "mlSetGyroBias.h"
#include "dmpKey.h"
#include "mldl.h"
#include "mlstates.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "ml-tempcomp"

#undef MPL_LOG_NDEBUG
#define MPL_LOG_NDEBUG 1 /* Use 0 to turn on MPL_LOGV output */

#ifndef INV_FEATURE_GYROTC_LEGACY
#error temp_comp_legacy.c requires INV_FEATURE_GYROTC_LEGACY to be defined
#endif

/**
 *  @defgroup   TEMP_COMP
 *  @brief      Gyroscope learning temperature compensation.
 *
 *  @{
 *      @file   tempComp.c
 *      @brief  Set of feature to enable and handle learning temperature
 *              compensation for gyroscope data.
 */

/*
    Data Structures
*/
struct _TC {
    short haveSlope;
    int noMotionTimer;
    long motionTimer;
    float noMotionBiases[3];
    float noMotionTemp;
    long prevBias[3];
    float lastTemp;
    float lastGyroData[3];
    int gotLastData;
    unsigned long lastTime;
    long temperatureRange;
    int first_pass;
};

/*
    Globals
*/
extern struct inv_supervisor_cb_obj ml_supervisor_cb;

struct _TC tcData;

/*
    Prototypes
*/
inv_error_t inv_temp_comp_supervisor(struct inv_obj_t *inv_obj);

/*
    Functions
*/

/**
 *  @brief  Enable the temperature compensation algorithm and calibrated gyro
 *          temperature compensated output.
 *  @return INV_SUCCESS == 0.
 */
inv_error_t inv_enable_temp_comp(void)
{
    inv_error_t result;
    unsigned char is_registered;

    /* Check if already registered. */
    result = inv_check_state_callback(inv_temp_comp_reset, &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (is_registered) {
        return INV_SUCCESS;
    }

    result = inv_register_state_callback(inv_temp_comp_reset);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    tcData.lastTime = inv_get_tick_count();
    tcData.temperatureRange = inv_decode_temperature(32767) - inv_decode_temperature(-32768);
    tcData.first_pass = 0;
    result = inv_register_fifo_rate_process(inv_temp_comp_supervisor, INV_PRIORITY_TEMP_COMP);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    MPL_LOGV("Temp comp enabled.\n");

    return INV_SUCCESS;
}

/**
 *  @brief  Disable the temperature compensation algorithm and calibrated gyro
 *          temperature compensated output.
 *  @return INV_SUCCESS == 0.
 */
inv_error_t inv_disable_temp_comp(void)
{
    inv_error_t result;
    unsigned char is_registered;

    /* Check if already unregistered. */
    result = inv_check_state_callback(inv_temp_comp_reset, &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (!is_registered) {
        return INV_SUCCESS;
    }

    result = inv_unregister_state_callback(inv_temp_comp_reset);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = inv_unregister_fifo_rate_process(inv_temp_comp_supervisor);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    MPL_LOGV("Temp comp disabled.\n");

    return result;
}

/**
 *  @brief      @e inv_temp_comp_is_enabled checks if the temperature
 *              compensation algorithm is being used to update gyro
 *              biases.
 *
 *  @param[out] is_enabled  True if temp comp is enabled.
 *
 *  @return     INV_SUCCESS if successful.
 */
inv_error_t inv_temp_comp_is_enabled(unsigned char *is_enabled)
{
    inv_error_t result;
    unsigned char is_registered;

    if (inv_get_gyro_present()) {
        is_enabled[0] = false;
        return INV_SUCCESS;
    }

    result = inv_check_fifo_callback(inv_temp_comp_supervisor, &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    is_enabled[0] = is_registered;

    return INV_SUCCESS;
}

/**
 *  @brief  Reset the temperature compensation algorithm internal state
 *          machine.
 */
inv_error_t inv_temp_comp_reset(unsigned char new_state)
{
    MPL_LOGV("RESET\n");

    if (new_state == INV_STATE_DMP_STARTED) {
        memset(&tcData, 0, sizeof(tcData));
        tcData.temperatureRange =
            inv_decode_temperature(32767) - inv_decode_temperature(-32768);
        tcData.motionTimer = 1000;
        tcData.gotLastData = true;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Whether the temperature compensation slope has been already
 *          computed.
 */
int inv_temp_comp_has_slope(void)
{
    return (tcData.haveSlope);
}

/**
 *  @brief  Get the temperature change from last and previous time
 *          temp_comp_apply has executed.
 *          This is a sinple shorthand to avoid making the internal
 *          data structure struct _TC non file static.
 *  @return The temperature difference in degrees C.
 */
float inv_get_calibration_temp_difference(void)
{
    return (tcData.lastTemp - tcData.noMotionTemp);
}

/**
 *  @internal
 *  @brief  Get the temperature in float format.
 *          Represented in degree C.
 *  @param  temp
 *              a pointer to store the temperature.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */

#define ERROR_TEMP 	2462307L

static inv_error_t temp_comp_get_temp(float *temp)
{
    long temperature;
    //inv_error_t result;

    (void)inv_get_temperature(&temperature);
    //result = inv_get_temperature(&temperature);
    //if (result) {
    //    LOG_RESULT_LOCATION(result);
    //    return result;
    //}

	if(temperature == ERROR_TEMP)
		return INV_ERROR;
    *temp = ((float)temperature) / 65536.0f;
    return INV_SUCCESS;
}

/**
 *  @internal
 *  @brief  Get raw gyro data, transform to float, scale, and add in bias.
 *          Expressed in degree per second.
 *  @param  gyro
 *              A pointer to store the gyro data for the 3 axis.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
/**
 *  @brief  Find the right temperature bin.
 *  @param  temp
 *              The temperature in degree C.
 *  @return the temperature bin number, [0, BINS).
 */
int inv_temp_comp_find_bin(float temp)
{
    int bin;

    bin = (int)((temp - MIN_TEMP) / ((MAX_TEMP - MIN_TEMP) / BINS));
    if (bin < 0)
        bin = 0;
    if (bin > BINS - 1)
        bin = BINS - 1;
    return bin;
}

/**
 *  @internal
 *  @brief  Find the closest available temperature bin (typically 5 C wide) to
 *          the requested bin.  For a bin to be selected, it has to be either
 *          be declared valid (gyro_tc->temp_valid_data == 1) or it's got at least
 *          PTS_PER_BIN points in the temp. compensation table.
 *  @return the temperature bin number, [0, BINS).
 */
int temp_comp_find_closest_bin(int bin)
{
    int inc = 1;
    int incSign = 0;

    while (!inv_obj.gyro_tc->temp_valid_data[bin]
                && inv_obj.gyro_tc->temp_ptrs[bin] == 0
                && inc < 4) {
        if (incSign == 0) {
            bin += inc;
            incSign = 1;
        } else {
            bin -= inc;
            incSign = 0;
        }
        inc += 1;
    }
    return (inc < 4 ? bin : -1);
}


/**
 *  @brief  Prints the temperature compesation table for debugging
 *          purpose.
 */
void temp_comp_print_table(void)
{
#define STR_LEN 500
    char line[STR_LEN];
    int k, j;

    MPL_LOGI("\n");
    MPL_LOGI("gyro_tc->temp_data & gyro_tc->temp_ptrs\n");
    for (j = 0, line[0] = 0; j < BINS; j++) {
        sprintf(line, "pnt %2d - ", j);
        for (k = 0; k < PTS_PER_BIN; k++) {
            sprintf(line, "%s%+9.3f ", line, inv_obj.gyro_tc->temp_data[j][k]);
        }
        sprintf(line, "%s - %d", line, inv_obj.gyro_tc->temp_ptrs[j]);
        MPL_LOGI("%s\n", line);
    }

    MPL_LOGI("gyro_tc->x_gyro_temp_data\n");
    for (j = 0, line[0] = 0; j < BINS; j++) {
        sprintf(line, "pnt %2d - ", j);
        for (k = 0; k < PTS_PER_BIN; k++) {
            sprintf(line, "%s%+9.3f ", line, inv_obj.gyro_tc->x_gyro_temp_data[j][k]);
        }
        MPL_LOGI("%s\n", line);
    }
    MPL_LOGI("gyro_tc->y_gyro_temp_data\n");
    for (j = 0, line[0] = 0; j < BINS; j++) {
        sprintf(line, "pnt %2d - ", j);
        for (k = 0; k < PTS_PER_BIN; k++) {
            sprintf(line, "%s%+9.3f ", line, inv_obj.gyro_tc->y_gyro_temp_data[j][k]);
        }
        MPL_LOGI("%s\n", line);
    }
    MPL_LOGI("gyro_tc->z_gyro_temp_data\n");
    for (j = 0, line[0] = 0; j < BINS; j++) {
        sprintf(line, "pnt %2d - ", j);
        for (k = 0; k < PTS_PER_BIN; k++) {
            sprintf(line, "%s%+9.3f ", line, inv_obj.gyro_tc->z_gyro_temp_data[j][k]);
        }
        MPL_LOGI("%s\n", line);
    }
    MPL_LOGI("\n");
}

/**
 *  @brief  Add old data to temperature table.
 *          Adding current data risks adding a motion point, since there is a
 *          1 second delay between motion and the polling based motion detect.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
static inv_error_t temp_comp_add_data(void)
{
    float newGyroData[3];
    float newTemp;
    int i;

    /* since AddData runs only in "no motion" state, this point represents
       a no motion point */
    temp_comp_get_temp(&newTemp);
    newGyroData[0] = (float)inv_obj.gyro->bias[0] / (1L << 16);
    newGyroData[1] = (float)inv_obj.gyro->bias[1] / (1L << 16);
    newGyroData[2] = (float)inv_obj.gyro->bias[2] / (1L << 16);

    /* got data from a previous run */
    if (tcData.gotLastData) {
        int bin = inv_temp_comp_find_bin(tcData.lastTemp);
        int *tempPtr = &(inv_obj.gyro_tc->temp_ptrs[bin]);

        inv_obj.gyro_tc->temp_data[bin][*tempPtr] = tcData.lastTemp;
        inv_obj.gyro_tc->x_gyro_temp_data[bin][*tempPtr] = tcData.lastGyroData[0];
        inv_obj.gyro_tc->y_gyro_temp_data[bin][*tempPtr] = tcData.lastGyroData[1];
        inv_obj.gyro_tc->z_gyro_temp_data[bin][*tempPtr] = tcData.lastGyroData[2];

        if (!MPL_LOG_NDEBUG) {
            MPL_LOGV(
                "temp_comp -> add data to temp table : "
                "%+10.3f degC - %+10.3f %+10.3f %+10.3f dps\n",
                tcData.lastTemp,
                tcData.lastGyroData[0],
                tcData.lastGyroData[1],
                tcData.lastGyroData[2]
                );
            temp_comp_print_table();
        }

        /* Treat each bin as a circular buffer
           If pointer wraps around, set the data valid bit,
           indicating that the bin is full. */
        *tempPtr = (*tempPtr + 1) % PTS_PER_BIN;
        if (*tempPtr == 0)
            inv_obj.gyro_tc->temp_valid_data[bin] = true;

        /*  Track the current bias and temperature to be used as the offset
            (temp comp table will only be used to generate the slope) */
        for (i = 0; i < GYRO_NUM_AXES; i++) {
            tcData.noMotionBiases[i] = (float)inv_obj.gyro->bias[i] / 65536.0f;
        }
        tcData.noMotionTemp = tcData.lastTemp;
    }

    /*  if the biases have not changed since last call to this function,
        save current temperature and gyro data to be used next time;
        see above the use of lastTemp and lastGyroData */
    if (tcData.prevBias[0] == inv_obj.gyro->bias[0] &&
        tcData.prevBias[1] == inv_obj.gyro->bias[1] &&
        tcData.prevBias[2] == inv_obj.gyro->bias[2]) {
        tcData.lastTemp = newTemp;
        for (i = 0; i < 3; i++) {
            tcData.lastGyroData[i] = newGyroData[i];
        }
        tcData.gotLastData = true;
    } else {
        tcData.gotLastData = false;
    }

    /* save the current biases at this point to be
       able, at the next run, to verify they haven't changed
       (see block above) */
    tcData.prevBias[0] = inv_obj.gyro->bias[0];
    tcData.prevBias[1] = inv_obj.gyro->bias[1];
    tcData.prevBias[2] = inv_obj.gyro->bias[2];
    return INV_SUCCESS;
}


static inv_error_t inv_set_dmp_intercept(void)
{
    inv_error_t result;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    float intercept[3];
    extern struct inv_obj_t inv_obj;
    signed char *orient;
    unsigned char regs[12];

    orient = mldl_cfg->pdata->orientation;
    intercept[0] = orient[0] * inv_obj.gyro_tc->x_gyro_coef[0] +
        orient[1] * inv_obj.gyro_tc->y_gyro_coef[0] +
        orient[2] * inv_obj.gyro_tc->z_gyro_coef[0];
    intercept[1] = orient[3] * inv_obj.gyro_tc->x_gyro_coef[0] +
        orient[4] * inv_obj.gyro_tc->y_gyro_coef[0] +
        orient[5] * inv_obj.gyro_tc->z_gyro_coef[0];
    intercept[2] = orient[6] * inv_obj.gyro_tc->x_gyro_coef[0] +
        orient[7] * inv_obj.gyro_tc->y_gyro_coef[0] +
        orient[8] * inv_obj.gyro_tc->z_gyro_coef[0];

    // We're in units of DPS, convert it back to chip units
    intercept[0] = intercept[0] * (1L<<16) * inv_obj.gyro->sf / inv_obj.gyro->sens;
    intercept[1] = intercept[1] * (1L<<16) * inv_obj.gyro->sf / inv_obj.gyro->sens;
    intercept[2] = intercept[2] * (1L<<16) * inv_obj.gyro->sf / inv_obj.gyro->sens;

    inv_int32_to_big8((long)intercept[0],regs);
    inv_int32_to_big8((long)intercept[1],&regs[4]);
    inv_int32_to_big8((long)intercept[2],&regs[8]);

    result = inv_set_mpu_memory(KEY_D_2_96, 12, regs);
    return result;
}

static inv_error_t inv_init_dmp_tempcomp(void)
{
    short tr;
    long tr32;
    unsigned char regs[4];
    inv_error_t result;

    tcData.first_pass = 1;
    result = inv_get_mpu_memory(KEY_D_2_108, 4, regs);
    tr32 = inv_big8_to_int32(regs);
    if (tr32 == 0)
    {
        // We need to initialize the temperature and bias for temp comp
        // These will get overwritten once a no motion event occurs on the DMP

        // Set the current temperature
        result = inv_get_temperature_raw(&tr);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        tr32 = (long)tr<<15;
        result = inv_set_mpu_memory(KEY_D_2_108, 4, inv_int32_to_big8(tr32,regs));
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        // Set the bias
        result = inv_set_dmp_intercept();
    }
    return result;
}

/** Gyro slope in dps which gets pushed down to DMP for temperature correction on the DMP
*/
inv_error_t inv_set_dmp_slope(float slope_x, float slope_y, float slope_z)
{
    inv_error_t result;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    unsigned char regs[4];
    float scale;
    long sf;
    long slp;
    float slope[3];
    extern struct inv_obj_t inv_obj;
    signed char *orient;

    orient = mldl_cfg->pdata->orientation;
    slope[0] = orient[0] * slope_x + orient[1] * slope_y + orient[2] * slope_z;
    slope[1] = orient[3] * slope_x + orient[4] * slope_y + orient[5] * slope_z;
    slope[2] = orient[6] * slope_x + orient[7] * slope_y + orient[8] * slope_z;

    if (mldl_cfg->mpu_chip_info->gyro_sens_trim != 0) {
        sf = 2000 * 131 / mldl_cfg->mpu_chip_info->gyro_sens_trim;
    } else {
        sf = 2000;
    }
    // inv_obj.gyro->sf converts from chip*2^16 to gyro used in DMP
    scale = (float)inv_obj.gyro->sf * tcData.temperatureRange / (1L<<16) / sf;
    slp = (long)(scale * slope[0]);
    result = inv_set_mpu_memory(KEY_D_2_244, 4, inv_int32_to_big8(slp,regs));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    slp = (long)(scale * slope[1]);
    result = inv_set_mpu_memory(KEY_D_2_248, 4, inv_int32_to_big8(slp,regs));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    slp = (long)(scale * slope[2]);
    result = inv_set_mpu_memory(KEY_D_2_252, 4, inv_int32_to_big8(slp,regs));
    return result;
}


/**
 *  @brief  Walk through temperature table and construct a linear fit.
 *          Should only access bins near current temperature?
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
static inv_error_t temp_comp_recompute(void)
{
#define ORD 1
    float mXGyro[3] = {0};
    float mYGyro[3] = {0};
    float mZGyro[3] = {0};
    float m[3][3] = {{0}};
    float minv[3][3] = {{0}};
    float det = 0;
    int i, j, k;

    for (j = 0; j < BINS; j++) {
        for (k = 0; k < PTS_PER_BIN; k++) {
            float temp;
            if (!inv_obj.gyro_tc->temp_valid_data[j]
                    || k >= inv_obj.gyro_tc->temp_ptrs[j]) {
                continue;
            }

            temp = inv_obj.gyro_tc->temp_data[j][k];

            m[0][0] += 1.f;
            m[0][1] += temp;
            m[0][2] += temp * temp;
            m[1][0] += temp;
            m[1][1] += temp * temp;
            m[1][2] += temp * temp * temp;
            m[2][0] += temp * temp;
            m[2][1] += temp * temp * temp;
            m[2][2] += temp * temp * temp * temp;

            mXGyro[0] += inv_obj.gyro_tc->x_gyro_temp_data[j][k];
            mXGyro[1] += inv_obj.gyro_tc->x_gyro_temp_data[j][k] * temp;
            mXGyro[2] += inv_obj.gyro_tc->x_gyro_temp_data[j][k] * temp * temp;

            mYGyro[0] += inv_obj.gyro_tc->y_gyro_temp_data[j][k];
            mYGyro[1] += inv_obj.gyro_tc->y_gyro_temp_data[j][k] * temp;
            mYGyro[2] += inv_obj.gyro_tc->y_gyro_temp_data[j][k] * temp * temp;

            mZGyro[0] += inv_obj.gyro_tc->z_gyro_temp_data[j][k];
            mZGyro[1] += inv_obj.gyro_tc->z_gyro_temp_data[j][k] * temp;
            mZGyro[2] += inv_obj.gyro_tc->z_gyro_temp_data[j][k] * temp * temp;
        }
    }

#if (ORD == 2)
    /* Second order fit (not currently used, probably better to
       do piece-wise linear) */
    det =
        m[0][0] * m[1][1] * m[2][2] - m[0][0] * m[1][2] * m[2][1] -
        m[0][1] * m[1][0] * m[2][2] + m[0][1] * m[1][2] * m[2][0] +
        m[0][2] * m[1][0] * m[2][1] - m[0][2] * m[1][1] * m[2][0];

    /* check that determinant is big enough */
    if (fabs(det) < 100.f) {
        return;
    }

    /* Matrix inversion for second order fit */
    minv[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) / det;
    minv[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) / det;
    minv[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) / det;
    minv[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) / det;
    minv[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) / det;
    minv[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) / det;
    minv[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) / det;
    minv[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) / det;
    minv[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) / det;

    /* Populate coefficients for second order fit */
    for (i = 0; i < 3; i++) {
        inv_obj.gyro_tc->x_gyro_coef[i] = 0.f;
        inv_obj.gyro_tc->y_gyro_coef[i] = 0.f;
        inv_obj.gyro_tc->z_gyro_coef[i] = 0.f;
        for (j = 0; j < 3; j++) {
            inv_obj.gyro_tc->x_gyro_coef[i] += minv[j][i] * mXGyro[j];
            inv_obj.gyro_tc->y_gyro_coef[i] += minv[j][i] * mYGyro[j];
            inv_obj.gyro_tc->z_gyro_coef[i] += minv[j][i] * mZGyro[j];
        }
    }
#else
    /* Linear fit */
    det = m[0][0] * m[1][1] - m[0][1] * m[1][0];

    /* check that determinant is big enough;
       (temperature range has to be at least 8 deg. C wide) */
    if (fabs(det) < 100.f) {
        return INV_SUCCESS;
    }

    /* Matrix inversion for linear fit */
    minv[0][0] = +m[1][1] / det;
    minv[0][1] = -m[1][0] / det;
    minv[1][0] = -m[0][1] / det;
    minv[1][1] = +m[0][0] / det;

    /* Populate coefficients for first order fit */
    for (i = 0; i < 2; i++) {
        inv_obj.gyro_tc->x_gyro_coef[i] = 0.f;
        inv_obj.gyro_tc->y_gyro_coef[i] = 0.f;
        inv_obj.gyro_tc->z_gyro_coef[i] = 0.f;
        for (j = 0; j < 2; j++) {
            inv_obj.gyro_tc->x_gyro_coef[i] += minv[j][i] * mXGyro[j];
            inv_obj.gyro_tc->y_gyro_coef[i] += minv[j][i] * mYGyro[j];
            inv_obj.gyro_tc->z_gyro_coef[i] += minv[j][i] * mZGyro[j];
        }
    }
#endif

    {
        inv_error_t result;
        if (tcData.first_pass == 0) {
            result = inv_init_dmp_tempcomp();
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
        result = inv_set_dmp_slope(inv_obj.gyro_tc->x_gyro_coef[1],
                                   inv_obj.gyro_tc->y_gyro_coef[1],
                                   inv_obj.gyro_tc->z_gyro_coef[1]);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    tcData.haveSlope = true;

    MPL_LOGV(
        "temp_comp -> recomputed temp comp slopes : "
        "%+10.3f %+10.3f %+10.3f dps/C\n",
        inv_obj.gyro_tc->x_gyro_coef[1],
        inv_obj.gyro_tc->y_gyro_coef[1],
        inv_obj.gyro_tc->z_gyro_coef[1]);
    return INV_SUCCESS;
}

/**
 *  @brief  Apply temperature compensation table to gyro bias.
 */

/**
 *  @brief  Recompute the temperature compensation table using the values
 *          loaded from file.
 *          Apply the value by finding the best guess for the gyro offsets.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t temp_comp_load_calibration_handler(void)
{
    float newBiases[3];
    float newTemp;
    int bin;
    inv_error_t result;
    int i;

    MPL_LOGV("Load Calibration Handler\n");

    tcData.gotLastData = false;

    /* recompute the temp comp table */
    result = temp_comp_recompute();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = temp_comp_get_temp(&newTemp);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* find best guess for gyro offset */
    bin = inv_temp_comp_find_bin(newTemp);

    /* If we have a slope from the ORD order fit,
        apply it to the biases with B = a + b T */
    if (tcData.haveSlope) {
        newBiases[0] =
            inv_obj.gyro_tc->x_gyro_coef[0] + inv_obj.gyro_tc->x_gyro_coef[1] * newTemp;
        newBiases[1] =
            inv_obj.gyro_tc->y_gyro_coef[0] + inv_obj.gyro_tc->y_gyro_coef[1] * newTemp;
        newBiases[2] =
            inv_obj.gyro_tc->z_gyro_coef[0] + inv_obj.gyro_tc->z_gyro_coef[1] * newTemp;
        inv_set_gyro_bias_in_dps_float(newBiases, INV_SGB_TEMP_COMP);


        tcData.noMotionTemp = newTemp;
        for (i = 0; i < GYRO_NUM_AXES; i++) {
            tcData.noMotionBiases[i] = newBiases[i];
            inv_obj.gyro_tc->temp_bias[i] = inv_obj.gyro->bias[i];
        }

        inv_obj.lite_fusion->got_no_motion_bias = true;

    } else {

        /* set the bias to one from the closest bin */
        int ptr;
        bin = temp_comp_find_closest_bin(bin);
        if (bin < 0)
            return INV_SUCCESS;
        if (inv_obj.gyro_tc->temp_ptrs[bin] == 0)
            ptr = PTS_PER_BIN - 1;
        else
            ptr = inv_obj.gyro_tc->temp_ptrs[bin] - 1;

        newBiases[0] = inv_obj.gyro_tc->x_gyro_temp_data[bin][ptr];
        newBiases[1] = inv_obj.gyro_tc->y_gyro_temp_data[bin][ptr];
        newBiases[2] = inv_obj.gyro_tc->z_gyro_temp_data[bin][ptr];
        MPL_LOGV("No slope, setting gyro bias from closest bin "
                     " %4.3f %4.3f %4.3f\n",newBiases[0],newBiases[1],newBiases[2]);

        inv_set_gyro_bias_in_dps_float(newBiases, INV_SGB_TEMP_COMP);

        /* record reference temp and biases */
        tcData.noMotionTemp = inv_obj.gyro_tc->temp_data[bin][ptr];
        for (i = 0; i < GYRO_NUM_AXES; i++) {
            tcData.noMotionBiases[i] = newBiases[i];
            /* instead of loading it on inv_obj.gyro_tc->temp_bias, we assign it
               to inv_obj.mlNoMotionBias to mimick having no motion biases
               (more short term biases than a temp comp correction */
        }
        /* declare no motion only if the temperature change is not bigger
           than 5 deg. C */
        if (fabs(newTemp - tcData.noMotionTemp) < 5.f) {
            inv_obj.lite_fusion->got_no_motion_bias = true;
        }
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Main entry point of the temperature compensation algorithm.
 *  @param  inv_obj
 *              A pointer to the internal struct inv_obj_t data structure.
 *  @return INV_SUCCESS == 0.
 */
inv_error_t inv_temp_comp_supervisor(struct inv_obj_t *inv_obj)
{
    inv_error_t result;
    unsigned long deltaTime, time;
    char nomotion;

	if(!inv_get_gyro_present())
	{
		return INV_SUCCESS;
	}

    if (inv_obj->sys->cal_loaded_flag) {
        inv_obj->sys->cal_loaded_flag = false;
        result = temp_comp_load_calibration_handler();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    time = inv_get_tick_count();
    deltaTime = time - tcData.lastTime;
    tcData.lastTime = time;

    nomotion = (inv_get_motion_state() == INV_NO_MOTION);
    if (nomotion){
        tcData.noMotionTimer += deltaTime;
        if (tcData.noMotionTimer > 1000) {
            MPL_LOGV("Supervisor: no motion\n");
            tcData.noMotionTimer = 0;
            result = temp_comp_add_data();
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
            result = temp_comp_recompute();
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    } else {
        tcData.motionTimer += deltaTime;
        if (tcData.motionTimer > 1200) {
            MPL_LOGV("Supervisor: motion\n");
            tcData.motionTimer = 0;
        }
    }

    return INV_SUCCESS;
}

/**
 *  @brief  inv_get_gyro_temp_slope is used to get the temperature
 *          compensation algorithm's estimate of the gyroscope bias temperature
 *          coefficient.
 *          The argument array elements are ordered X,Y,Z.
 *          Values are in units of dps per deg C (degrees per second per degree
 *          Celcius). Values are scaled so that 1 dps per deg C = 2^16 LSBs.
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_gyro_temp_slope(long *data)
{
    unsigned char is_enabled;
    inv_error_t result;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_temp_comp_is_enabled(&is_enabled);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (is_enabled) {
        data[0] = (long)(inv_obj.gyro_tc->x_gyro_coef[1] * 65536.0f);
        data[1] = (long)(inv_obj.gyro_tc->y_gyro_coef[1] * 65536.0f);
        data[2] = (long)(inv_obj.gyro_tc->z_gyro_coef[1] * 65536.0f);
    } else {
        data[0] = inv_obj.gyro_tc->temp_slope[0];
        data[1] = inv_obj.gyro_tc->temp_slope[1];
        data[2] = inv_obj.gyro_tc->temp_slope[2];
    }
    return result;
}

/**
 *  @brief  inv_get_gyro_temp_slope_float is used to get the temperature
 *          compensation algorithm's estimate of the gyroscope bias temperature
 *          coefficient.
 *          The argument array elements are ordered X,Y,Z.
 *          Values are in units of dps per deg C (degrees per second per degree
 *          Celcius)
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_temp_slope_float(float *data)
{
    INVENSENSE_FUNC_START;
    unsigned char is_enabled;
    inv_error_t result;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_temp_comp_is_enabled(&is_enabled);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (is_enabled) {
        data[0] = inv_obj.gyro_tc->x_gyro_coef[1];
        data[1] = inv_obj.gyro_tc->y_gyro_coef[1];
        data[2] = inv_obj.gyro_tc->z_gyro_coef[1];
    } else {
        data[0] = (float)inv_obj.gyro_tc->temp_slope[0] / 65536.0f;
        data[1] = (float)inv_obj.gyro_tc->temp_slope[1] / 65536.0f;
        data[2] = (float)inv_obj.gyro_tc->temp_slope[2] / 65536.0f;
    }

    return result;
}

/**
 *  @brief  inv_set_gyro_temp_slope is used to set the temperature
 *          compensation algorithm's estimate of the gyroscope bias temperature
 *          coefficient.
 *          The argument array elements are ordered X,Y,Z.
 *          Values are in units of dps per deg C (degrees per second per degree
 *          Celcius), and scaled such that 1 dps per deg C = 2^16 LSBs.
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document.
 *
 *  @brief  inv_set_gyro_temp_slope is used to set Gyro temperature slope
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_temp_slope(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    int i;
    long sf;
    unsigned char regs[3];

    inv_obj.gyro_tc->factory_temp_comp = 1;
    inv_obj.gyro_tc->temp_slope[0] = data[0];
    inv_obj.gyro_tc->temp_slope[1] = data[1];
    inv_obj.gyro_tc->temp_slope[2] = data[2];
    for (i = 0; i < GYRO_NUM_AXES; i++) {
        sf = -inv_obj.gyro_tc->temp_slope[i] / 1118;
        if (sf > 127) {
            sf -= 256;
        }
        regs[i] = (unsigned char)sf;
    }
    result = inv_set_offsetTC(regs);

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_gyro_temp_slope_float is used to get the temperature
 *          compensation algorithm's estimate of the gyroscope bias temperature
 *          coefficient.
 *          The argument array elements are ordered X,Y,Z.
 *          Values are in units of dps per deg C (degrees per second per degree
 *          Celcius)

 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_temp_slope_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_gyro_temp_slope(arrayTmp);
}

/**
 *  @}
 */
