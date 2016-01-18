/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: mputest.c 6276 2011-11-09 22:40:46Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup   MPU_SELF_TEST
 *  @brief      MPU Self Test functions.
 *
 *  These functions provide an in-site test of the MPU chips.
 *  The main entry point is the inv_mpu_test function.
 *  This runs the tests (as described in the accompanying documentation) and
 *      writes a configuration file containing initial calibration data.
 *  inv_mpu_test returns INV_SUCCESS if the chip passes the tests.
 *  Otherwise, an error code is returned.
 *  The functions in this file rely on MLSL and MLOS: refer to the MPL
 *      documentation for more information regarding the system interface
 *      files.
 *
 *  @{
 *      @file   mputest.c
 *      @brief  MPU Self Test routines for assessing gyro sensor status
 *              after surface mount has happened on the target host platform.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#ifdef LINUX
#include <unistd.h>
#endif

#include "mpu.h"
#include "mldl.h"
#include "mldl_cfg.h"
#include "accel.h"
#include "mlFIFO.h"
#include "slave.h"
#include "ml.h"
#include "ml_stored_data.h"
#include "checksum.h"
#include "mlMathFunc.h"
#include "mlstates.h"

#include "mlsl.h"
#include "mlos.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mpust"

#ifdef __cplusplus
extern "C" {
#endif

/*
    Defines
*/

#define VERBOSE_OUT 0

#define X   (0)
#define Y   (1)
#define Z   (2)

/*--- Test parameters defaults. See set_test_parameters for more details ---*/

#define DEF_MPU_ADDR             (0x68)        /* I2C address of the mpu     */

#define DEF_GYRO_FULLSCALE       (250)         /* gyro full scale dps        */
#define DEF_GYRO_SENS            (32768.f / DEF_GYRO_FULLSCALE)
                                               /* gyro sensitivity LSB/dps   */
#define DEF_PACKET_THRESH        (75)          /* 75 ms / (1ms / sample) OR
                                                  600 ms / (8ms / sample)   */
#define DEF_TOTAL_TIMING_TOL     (.03f)        /* 3% = 2 pkts + 1% proc tol. */
#define DEF_BIAS_THRESH_SELF     (60)          /* dps */
#define DEF_BIAS_THRESH_CAL      (40)
                                               /* 0.4 dps-rms in LSB-rms     */
#define DEF_TESTS_PER_AXIS       (1)           /* num of periods used to test
                                                  each axis */
#define DEF_N_ACCEL_SAMPLES      (20)          /* num of accel samples to
                                                  average from, if applic.   */
#define ML_INIT_CAL_LEN          (36)          /* length in bytes of
                                                  calibration data file      */
#define DEF_PERIOD_SELF          (75)          /* ms of time, self test */
#define DEF_PERIOD_CAL           (600)         /* ms of time, full cal */

/*
    Macros
*/

#define FLOAT_TO_SHORT(f)   (                                               \
                                (fabs(f - (short)f) >= 0.5) ? (             \
                                    ((short)f) + (f < 0 ? (-1) : (+1))) :   \
                                    ((short)f)                              \
                            )

/*
    Types
*/
typedef struct {
    float gyro_sens;
    int gyro_fs;
    int packet_thresh;
    float total_timing_tol;
    int bias_thresh;
    unsigned int tests_per_axis;
    unsigned short accel_samples;
} test_setup_t;

/*
    Global variables
*/
static unsigned char dataout[20];
static unsigned char data_store[ML_INIT_CAL_LEN];

static test_setup_t test_setup = {
    DEF_GYRO_SENS,
    DEF_GYRO_FULLSCALE,
    DEF_PACKET_THRESH,
    DEF_TOTAL_TIMING_TOL,
    (int)DEF_BIAS_THRESH_SELF,  /* now obsolete - has no effect */
    DEF_TESTS_PER_AXIS,
    DEF_N_ACCEL_SAMPLES
};

static float adj_gyro_sens;
static char a_name[3][2] = {"X", "Y", "Z"};

/*
    NOTE :  modify get_slave_descr parameter below to reflect
                the DEFAULT accelerometer in use. The accelerometer in use
                can be modified at run-time using the inv_test_setup_accel API.
    NOTE :  modify the expected z axis orientation (Z axis pointing
                upward or downward)
*/

signed char g_z_sign = +1;
struct mldl_cfg *mldl_cfg = NULL;

#ifndef LINUX
/**
 *  @internal
 *  @brief  usec precision sleep function.
 *  @param  number of micro seconds (us) to sleep for.
 */
static void usleep(unsigned long t)
{
    unsigned long start = inv_get_tick_count();
    while (inv_get_tick_count()-start < t / 1000);
}
#endif

/**
 *  @brief  Modify the self test limits from their default values.
 *
 *  @param  slave_addr
 *              the slave address the MPU device is setup to respond at.
 *              The default is DEF_MPU_ADDR = 0x68.
 *  @param  sensitivity
 *              the read sensitivity of the device in LSB/dps as it is trimmed.
 *              NOTE :  if using the self test as part of the MPL, the
 *                      sensitivity the different sensitivity trims are already
 *                      taken care of.
 *  @param  p_thresh
 *              number of packets expected to be received in one test period.
 *              Depends on the sampling frequency of choice (set by default to
 *              1 kHz) and low pass filter cut-off frequency selection (set
 *              to 42 Hz).
 *              The default is DEF_PACKET_THRESH = 75 packets.
 *  @param  total_time_tol
 *              time skew tolerance, taking into account imprecision in turning
 *              the FIFO on and off and the processor time imprecision (for
 *              1 GHz processor).
 *              The default is DEF_TOTAL_TIMING_TOL = 3 %, about 2 packets for
                a 75ms period.
 *  @param  bias_thresh
 *              bias level threshold, the maximun acceptable no motion bias
 *              for a production quality part.
 *              The default is DEF_BIAS_THRESH = 40 dps.
 *  @param  accel_samples
 *              the number of samples to be collected from the accelerometer
 *              device to estimate its initial biases.
 */
void inv_set_test_parameters(unsigned int slave_addr, float sensitivity,
                             int p_thresh, float total_time_tol,
                             int bias_thresh, unsigned short accel_samples)
{
    mldl_cfg->mpu_chip_info->addr = slave_addr;
    test_setup.gyro_sens = sensitivity;
    test_setup.gyro_fs = (int)(32768.f / sensitivity);
    test_setup.packet_thresh = p_thresh;
    test_setup.total_timing_tol = total_time_tol;
    test_setup.bias_thresh = bias_thresh;
    test_setup.accel_samples = accel_samples;
}


/**
 *  @brief  Test the gyroscope sensor.
 *          Implements the core logic of the MPU Self Test.
 *          Produces the PASS/FAIL result. Loads the calculated gyro biases
 *          and temperature datum into the corresponding pointers.
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  gyro_biases
 *              output pointer to store the initial bias calculation provided
 *              by the MPU Self Test.  Requires 3 elements for gyro X, Y,
 *              and Z.
 *  @param  temp_avg
 *              output pointer to store the initial average temperature as
 *              provided by the MPU Self Test.
 *  @param  perform_full_test
 *              If 1:
 *              Complete calibration test:
 *              Calculate offset, drive frequency, and noise and compare it
 *              against set thresholds.
 *              When 0:
 *              Skip the noise and drive frequency calculation,
 *              simply calculate the gyro biases.
 *
 *  @return 0 on success.
 *          On error, the return value is a bitmask representing:
 *          0, 1, 2     Failures with PLLs on X, Y, Z gyros respectively
 *                        (decimal values will be 1, 2, 4 respectively).
 *          3, 4, 5     Excessive offset with X, Y, Z gyros respectively
 *                        (decimal values will be 8, 16, 32 respectively).
 *          6, 7, 8     Excessive noise with X, Y, Z gyros respectively
 *                        (decimal values will be 64, 128, 256 respectively).
 *          9           If any of the RMS noise values is zero, it may be
 *                        due to a non-functional gyro or FIFO/register failure.
 *                        (decimal value will be 512).
 */
int test_gyro(void *mlsl_handle,
                  short gyro_biases[3], short *temp_avg,
                  uint_fast8_t perform_full_test)
{
    int ret_val = 0;
    inv_error_t result;
    int total_count = 0;
    int total_count_axis[3] = {0, 0, 0};
    int packet_count;
    short x[DEF_PERIOD_CAL * DEF_TESTS_PER_AXIS / 8 * 4] = {0};
    short y[DEF_PERIOD_CAL * DEF_TESTS_PER_AXIS / 8 * 4] = {0};
    short z[DEF_PERIOD_CAL * DEF_TESTS_PER_AXIS / 8 * 4] = {0};
    int temperature = 0;
    float avg[3];
    float rms[3];
    unsigned long test_start = inv_get_tick_count();
    int i, j, tmp;
    char tmpStr[200];
    unsigned char regs[7] = {0};

    /* make sure the DMP is disabled first */
    result = inv_serial_single_write(
                mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                MPUREG_USER_CTRL, 0x00);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* reset the gyro offset values */
    regs[0] = MPUREG_XG_OFFS_USRH;
    result = inv_serial_write(mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                              6, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* sample rate */
    if (perform_full_test) {
        /* = 8ms */
        result = inv_serial_single_write(
                mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                MPUREG_SMPLRT_DIV, 0x07);
        test_setup.bias_thresh = (int)(
                DEF_BIAS_THRESH_CAL * test_setup.gyro_sens);
    } else {
        /* = 1ms */
        result = inv_serial_single_write(
                mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                MPUREG_SMPLRT_DIV, 0x00);
        test_setup.bias_thresh = (int)(
                DEF_BIAS_THRESH_SELF * test_setup.gyro_sens);
    }
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    regs[0] = 0x03; /* filter = 42Hz, analog_sample rate = 1 KHz */
    switch (test_setup.gyro_fs) {
        case 2000:
            regs[0] |= 0x18;
            break;
        case 1000:
            regs[0] |= 0x10;
            break;
        case 500:
            regs[0] |= 0x08;
            break;
        case 250:
        default:
            regs[0] |= 0x00;
            break;
    }
    result = inv_serial_single_write(
                mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                MPUREG_CONFIG, regs[0]);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_serial_single_write(
                mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                MPUREG_INT_ENABLE, 0x00);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* 1st, timing test */
    for (j = 0; j < 3; j++) {
        MPL_LOGI("Collecting gyro data from %s gyro PLL\n", a_name[j]);

        /* turn on all gyros, use gyro X for clocking
           Set to Y and Z for 2nd and 3rd iteration */
        result = inv_serial_single_write(
                    mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                    MPUREG_PWR_MGMT_1, j + 1);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        /* wait for 2 ms after switching clock source */
        usleep(2000);

        /* enable & reset FIFO */
        result = inv_serial_single_write(
                    mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                    MPUREG_USER_CTRL, BIT_FIFO_EN | BIT_FIFO_RST);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        tmp = test_setup.tests_per_axis;
        while (tmp-- > 0) {
            const unsigned char fifo_en_reg = MPUREG_FIFO_EN;
            /* enable XYZ gyro in FIFO and nothing else */
            result = inv_serial_single_write(mlsl_handle,
                        mldl_cfg->mpu_chip_info->addr, fifo_en_reg,
                        BIT_GYRO_XOUT | BIT_GYRO_YOUT | BIT_GYRO_ZOUT);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }

            /* wait one period for data */
            if (perform_full_test)
                usleep(DEF_PERIOD_CAL * 1000);
            else
                usleep(DEF_PERIOD_SELF * 1000);

            /* stop storing gyro in the FIFO */
            result = inv_serial_single_write(
                        mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                        fifo_en_reg, 0x00);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }

            /* Getting number of bytes in FIFO */
            result = inv_serial_read(
                           mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                           MPUREG_FIFO_COUNTH, 2, dataout);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }

            /* number of 6 B packets in the FIFO */
            packet_count = inv_big8_to_int16(dataout) / 6;
            sprintf(tmpStr, "Packet Count: %d - ", packet_count);

            if (abs(packet_count - test_setup.packet_thresh)
                        <= /* Within total_timing_tol % range, rounded up */
                (int)(test_setup.total_timing_tol *
                      test_setup.packet_thresh + 1)) {
                for (i = 0; i < packet_count; i++) {
                    /* getting FIFO data */
                    result = inv_serial_read_fifo(mlsl_handle,
                                mldl_cfg->mpu_chip_info->addr, 6, dataout);
                    if (result) {
                        LOG_RESULT_LOCATION(result);
                        return result;
                    }
                    x[total_count + i] = inv_big8_to_int16(&dataout[0]);
                    y[total_count + i] = inv_big8_to_int16(&dataout[2]);
                    z[total_count + i] = inv_big8_to_int16(&dataout[4]);
                    if (VERBOSE_OUT) {
                        MPL_LOGI("Gyros %-4d    : %+13d %+13d %+13d\n",
                                    total_count + i, x[total_count + i],
                                    y[total_count + i], z[total_count + i]);
                    }
                }
                total_count += packet_count;
                total_count_axis[j] += packet_count;
                sprintf(tmpStr, "%sOK", tmpStr);
            } else {
                ret_val |= 1 << j;
                sprintf(tmpStr, "%sNOK - samples ignored", tmpStr);
            }
            MPL_LOGI("%s\n", tmpStr);
        }

        /* remove gyros from FIFO */
        result = inv_serial_single_write(
                    mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                    MPUREG_FIFO_EN, 0x00);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        /* Read Temperature */
        result = inv_serial_read(mlsl_handle, mldl_cfg->mpu_chip_info->addr,
                    MPUREG_TEMP_OUT_H, 2, dataout);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        temperature += (short)inv_big8_to_int16(dataout);
    }

    MPL_LOGI("\n");
    MPL_LOGI("Total %d samples\n", total_count);
    MPL_LOGI("\n");

    /* 2nd, check bias from X, Y, and Z PLL clock source */
    tmp = total_count != 0 ? total_count : 1;
    for (i = 0, avg[X] = .0f, avg[Y] = .0f, avg[Z] = .0f;
         i < total_count; i++) {
        avg[X] += 1.f * x[i] / tmp;
        avg[Y] += 1.f * y[i] / tmp;
        avg[Z] += 1.f * z[i] / tmp;
    }
    MPL_LOGI("bias          : %+13.3f %+13.3f %+13.3f (LSB)\n",
             avg[X], avg[Y], avg[Z]);
    if (VERBOSE_OUT) {
        MPL_LOGI("              : %+13.3f %+13.3f %+13.3f (dps)\n",
                 avg[X] / adj_gyro_sens,
                 avg[Y] / adj_gyro_sens,
                 avg[Z] / adj_gyro_sens);
    }
    for (j = 0; j < 3; j++) {
        if (fabs(avg[j]) > test_setup.bias_thresh) {
            MPL_LOGI("%s-Gyro bias (%.0f) exceeded threshold "
                    "(threshold = %d)\n",
                    a_name[j], avg[j], test_setup.bias_thresh);
            ret_val |= 1 << (3+j);
        }
    }

    /* 3rd, check RMS for dead gyros
      If any of the RMS noise value returns zero,
      then we might have dead gyro or FIFO/register failure,
      the part is sleeping, or the part is not responsive */
        for (i = 0, rms[X] = 0.f, rms[Y] = 0.f, rms[Z] = 0.f;
         i < total_count; i++) {
        rms[X] += (x[i] - avg[X]) * (x[i] - avg[X]);
        rms[Y] += (y[i] - avg[Y]) * (y[i] - avg[Y]);
        rms[Z] += (z[i] - avg[Z]) * (z[i] - avg[Z]);
    }
    if (rms[X] == 0 || rms[Y] == 0 || rms[Z] == 0) {
        ret_val |= 1 << 9;
    }

    /* 4th, temperature average */
    temperature /= 3;
    if (VERBOSE_OUT)
        MPL_LOGI("Temperature   : %+13.3f %13s %13s (deg. C)\n",
                 (float)inv_decode_temperature(temperature) / (1L << 16),
                 "", "");

    /* load into final storage */
    *temp_avg = (short)temperature;
    gyro_biases[X] = FLOAT_TO_SHORT(avg[X]);
    gyro_biases[Y] = FLOAT_TO_SHORT(avg[Y]);
    gyro_biases[Z] = FLOAT_TO_SHORT(avg[Z]);

    MPL_LOGI("\n");
    MPL_LOGI("Test time : %ld ms\n", inv_get_tick_count() - test_start);

    return ret_val;
}

#ifdef TRACK_IDS
/**
 *  @internal
 *  @brief  Retrieve the unique MPU device identifier from the internal OTP
 *          bank 0 memory.
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @return 0 on success, a non-zero error code from the serial layer on error.
 */
static inv_error_t get_mpu_unique_id(void *mlsl_handle)
{
    inv_error_t result;
    unsigned char otp0[8];


    result =
        inv_serial_read_mem(mlsl_handle, mldl_cfg->mpu_chip_info->addr,
            (BIT_PRFTCH_EN | BIT_CFG_USER_BANK | MPU_MEM_OTP_BANK_0) << 8 |
            0x00, 6, otp0);
    if (result)
        goto close;

    MPL_LOGI("\n");
    MPL_LOGI("DIE_ID   : %06X\n",
                ((int)otp0[1] << 8 | otp0[0]) & 0x1fff);
    MPL_LOGI("WAFER_ID : %06X\n",
                (((int)otp0[2] << 8 | otp0[1]) & 0x03ff ) >> 5);
    MPL_LOGI("A_LOT_ID : %06X\n",
                ( ((int)otp0[4] << 16 | (int)otp0[3] << 8 |
                otp0[2]) & 0x3ffff) >> 2);
    MPL_LOGI("W_LOT_ID : %06X\n",
                ( ((int)otp0[5] << 8 | otp0[4]) & 0x3fff) >> 2);
    MPL_LOGI("WP_ID    : %06X\n",
                ( ((int)otp0[6] << 8 | otp0[5]) & 0x03ff) >> 7);
    MPL_LOGI("REV_ID   : %06X\n", otp0[6] >> 2);
    MPL_LOGI("\n");

close:
    result =
        inv_serial_single_write(mlsl_handle, mldl_cfg->mpu_chip_info->addr, MPUREG_BANK_SEL, 0x00);
    return result;
}
#endif /* TRACK_IDS */

/**
 *  @brief
 */
int populate_data_store(unsigned long sensor_mask,
                        short temp_avg, short gyro_biases[],
                        short accel_biases[], long accel_sens[])
{
    int ptr = 0;
    int tmp;
    long long lltmp;
    uint32_t chk;

    /* total len of factory cal */
    data_store[ptr++] = 0;
    data_store[ptr++] = 0;
    data_store[ptr++] = 0;
    data_store[ptr++] = ML_INIT_CAL_LEN;

    /* record type 5 - initial calibration */
    data_store[ptr++] = 0;
    data_store[ptr++] = 5;

    if (sensor_mask & INV_THREE_AXIS_GYRO) {
        /* temperature */
        tmp = temp_avg;
        if (tmp < 0)
            tmp += 2 << 16;
        inv_int16_to_big8(tmp, &data_store[ptr]);
        ptr += 2;
        /* NOTE : 2 * test_setup.gyro_fs == 65536 / (32768 / test_setup.gyro_fs) */
        /* x gyro avg */
        lltmp = (long)gyro_biases[0] * 2 * test_setup.gyro_fs;
        if (lltmp < 0)
            lltmp += 1LL << 32;
        inv_int32_to_big8((uint32_t)lltmp, &data_store[ptr]);
        ptr += 4;
        /* y gyro avg */
        lltmp = (long)gyro_biases[1] * 2 * test_setup.gyro_fs;
        if (lltmp < 0)
            lltmp += 1LL << 32;
        inv_int32_to_big8((uint32_t)lltmp, &data_store[ptr]);
        ptr += 4;
        /* z gyro avg */
        lltmp = (long)gyro_biases[2] * 2 * test_setup.gyro_fs;
        if (lltmp < 0)
            lltmp += 1LL << 32;
        inv_int32_to_big8((uint32_t)lltmp, &data_store[ptr]);
        ptr += 4;
    } else {
        ptr += (2 + 4 + 4 + 4);
    }

    if (sensor_mask & INV_THREE_AXIS_ACCEL) {
        signed char *mtx =
            mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]->orientation;
        short tmp[3];
        int ii;
        /* need store the biases in chip frame - that is,
           inverting the rotation applied in inv_get_accel_data */
        memcpy(tmp, accel_biases, sizeof(tmp));
        for (ii = 0; ii < ARRAY_SIZE(accel_biases); ii++) {
            accel_biases[ii] = tmp[0] * mtx[3 * 0 + ii] +
                               tmp[1] * mtx[3 * 1 + ii] +
                               tmp[2] * mtx[3 * 2 + ii];
        }
/*
        if (sensor_mask & INV_X_ACCEL) {
            // x accel avg 
            lltmp = (long)accel_biases[0] * 65536L / accel_sens[0];
            if (lltmp < 0)
                lltmp += 1LL << 32;
            inv_int32_to_big8((uint32_t)lltmp, &data_store[ptr]);
        }
	*/
        ptr += 4;
        if (sensor_mask & INV_Y_ACCEL) {
            /* y accel avg */
            lltmp = (long)accel_biases[1] * 65536L / accel_sens[1];
            if (lltmp < 0)
                lltmp += 1LL << 32;
            inv_int32_to_big8((uint32_t)lltmp, &data_store[ptr]);
        }
        ptr += 4;
        /* z accel avg */
        if (sensor_mask & INV_Z_ACCEL) {
            lltmp = (long)accel_biases[2] * 65536L / accel_sens[2];
            if (lltmp < 0)
                lltmp += 1LL << 32;
            inv_int32_to_big8((uint32_t)lltmp, &data_store[ptr]);
        }
        ptr += 4;
    } else {
        ptr += 12;
    }

    /* add a checksum for data */
    chk = inv_checksum(
        data_store + INV_CAL_HDR_LEN,
        ML_INIT_CAL_LEN - INV_CAL_HDR_LEN - INV_CAL_CHK_LEN);
    inv_int32_to_big8(chk, &data_store[ptr]);
    ptr += 4;

    if (ptr != ML_INIT_CAL_LEN) {
        MPL_LOGI("Invalid calibration data length: exp %d, got %d\n",
                 ML_INIT_CAL_LEN, ptr);
        return -1;
    }

    return 0;
}

/**
 *  @brief  If requested via inv_test_setup_accel(), test the accelerometer
 *          biases and calculate the necessary bias correction.
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  enable_axis
 *              specify which axis has to be checked and corrected: provides
 *              a switch mode between 3 axis calibration and Z axis only
 *              calibration.
 *  @param  bias
 *              output pointer to store the initial bias calculation provided
 *              by the MPU Self Test.  Requires 3 elements to store accel X, Y,
 *              and Z axis bias.
 *  @param  gravity
 *              The gravity value given the parts' sensitivity: for example
 *              if the accelerometer is set to +/- 2 gee ==> the gravity
 *              value will be 2^14 = 16384.
 *  @param  perform_full_test
 *              If 1:
 *              calculates offsets and noise and compare it against set
 *              thresholds. The final exist status will reflect if any of the
 *              value is outside of the expected range.
 *              When 0;
 *              skip the noise calculation and pass/fail assessment; simply
 *              calculates the accel biases.
 *
 *  @return 0 on success. A non-zero error code on error.
 */
int test_accel(void *mlsl_handle, int enable_axes,
                   short *bias, long gravity,
                   uint_fast8_t perform_full_test)
{
    short *p_vals;
    float avg[3] = {0.f, 0.f, 0.f}, zg = 0.f;
    float rms[3];
    float accel_rms_thresh = 1000000.f; /* enourmous to make the test always
                                           passes - future deployment */
    int accel_error = false;
    const long sample_period = inv_get_sample_step_size_ms() * 1000;
    int ii;

    p_vals = (short*)inv_malloc(sizeof(short) * 3 * test_setup.accel_samples);

    /* collect the samples  */
    for(ii = 0; ii < test_setup.accel_samples; ii++) {
        unsigned result = INV_ERROR_ACCEL_DATA_NOT_READY;
        int tries = 0;
        long accel_data[3];
        short *vals = &p_vals[3 * ii];

        /* ignore data not ready errors but don't try more than 5 times */
        while (result == INV_ERROR_ACCEL_DATA_NOT_READY && tries++ < 5) {
            result = inv_get_accel_data(accel_data);
            usleep(sample_period);
        }
        if (result || tries >= 5) {
            MPL_LOGV("cannot reliably fetch data from the accelerometer");
            accel_error = true;
            goto accel_early_exit;
        }
        vals[X] = (short)accel_data[X];
        vals[Y] = (short)accel_data[Y];
        vals[Z] = (short)accel_data[Z];
        avg[X] += 1.f * vals[X] / test_setup.accel_samples;
        avg[Y] += 1.f * vals[Y] / test_setup.accel_samples;
        avg[Z] += 1.f * vals[Z] / test_setup.accel_samples;
        if (VERBOSE_OUT)
            MPL_LOGI("Accel         : %+13d %+13d %+13d (LSB)\n",
                     vals[X], vals[Y], vals[Z]);
    }

    if (((enable_axes << 4) & INV_THREE_AXIS_ACCEL) == INV_THREE_AXIS_ACCEL) {
        MPL_LOGI("Accel biases  : %+13.3f %+13.3f %+13.3f (LSB)\n",
                 avg[X], avg[Y], avg[Z]);
        if (VERBOSE_OUT)
            MPL_LOGI("Accel biases  : %+13.3f %+13.3f %+13.3f (gee)\n",
                     avg[X] / gravity, avg[Y] / gravity, avg[Z] / gravity);

        bias[X] = FLOAT_TO_SHORT(avg[X]);
        bias[Y] = FLOAT_TO_SHORT(avg[Y]);
        zg = avg[Z] - g_z_sign * gravity;
        bias[Z] = FLOAT_TO_SHORT(zg);

        MPL_LOGI("Accel correct.: %+13d %+13d %+13d (LSB)\n",
                 bias[X], bias[Y], bias[Z]);
        if (VERBOSE_OUT)
            MPL_LOGI("Accel correct.: "
                     "%+13.3f %+13.3f %+13.3f (gee)\n",
                     1.f * bias[X] / gravity,
                     1.f * bias[Y] / gravity,
                     1.f * bias[Z] / gravity);

        if (perform_full_test) {
            /* accel RMS - for now the threshold is only indicative */
            for (ii = 0,
                     rms[X] = 0.f, rms[Y] = 0.f, rms[Z] = 0.f;
                 ii <  test_setup.accel_samples; ii++) {
                short *vals = &p_vals[3 * ii];
                rms[X] += (vals[X] - avg[X]) * (vals[X] - avg[X]);
                rms[Y] += (vals[Y] - avg[Y]) * (vals[Y] - avg[Y]);
                rms[Z] += (vals[Z] - avg[Z]) * (vals[Z] - avg[Z]);
            }
            for (ii = 0; ii < 3; ii++) {
                if (rms[ii] >  accel_rms_thresh * accel_rms_thresh
                                * test_setup.accel_samples) {
                    MPL_LOGI("%s-Accel RMS (%.2f) exceeded threshold "
                             "(threshold = %.2f)\n", a_name[ii],
                             sqrt(rms[ii] / test_setup.accel_samples),
                             accel_rms_thresh);
                    accel_error = true;
                    goto accel_early_exit;
                }
            }
            MPL_LOGI("Accel RMS     : %+13.3f %+13.3f %+13.3f (LSB-rms)\n",
                     sqrt(rms[X] / DEF_N_ACCEL_SAMPLES),
                     sqrt(rms[Y] / DEF_N_ACCEL_SAMPLES),
                     sqrt(rms[Z] / DEF_N_ACCEL_SAMPLES));
        }
    } else {
        MPL_LOGI("Accel Z bias    : %+13.3f (LSB)\n", avg[Z]);
        if (VERBOSE_OUT)
            MPL_LOGI("Accel Z bias    : %+13.3f (gee)\n", avg[Z] / gravity);

        zg = avg[Z] - g_z_sign * gravity;
        bias[Z] = FLOAT_TO_SHORT(zg);

        MPL_LOGI("Accel Z correct.: %+13d (LSB)\n", bias[Z]);
        if (VERBOSE_OUT)
            MPL_LOGI("Accel Z correct.: "
                     "%+13.3f (gee)\n", 1.f * bias[Z] / gravity);
    }

accel_early_exit:
    if (accel_error) {
        bias[0] = bias[1] = bias[2] = 0;
        return (1);     /* error */
    }
    inv_free(p_vals);

    return (0);         /* success */
}

/**
 *  @brief  determine the accelerometr sensitivity in internal representation
 *          basing on the type of part is in use.
 *  @param  accel_sens
 *              accelerometer sensitivity for the 3 axes.
 */
static void get_accel_sensitivity(long accel_sens[])
{
    if (mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]) {
        float fs;
        RANGE_FIXEDPOINT_TO_FLOAT(
            mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->range, fs);
        accel_sens[0] = (long)((1L << 15) / fs);
        accel_sens[1] = (long)((1L << 15) / fs);
        accel_sens[2] = (long)((1L << 15) / fs);
        if (MPL_PROD_KEY(mldl_cfg->mpu_chip_info->product_id,
                         mldl_cfg->mpu_chip_info->product_revision) ==
            MPU_PRODUCT_KEY_B1_E1_5) {
            accel_sens[2] /= 2;
        } else {
            unsigned short trim_corr =
                (1L << 14) / mldl_cfg->mpu_chip_info->accel_sens_trim;
            accel_sens[0] /= trim_corr;
            accel_sens[1] /= trim_corr;
            accel_sens[2] /= trim_corr;
        }
    } else {
        /* would be 0, but 1 to avoid divide-by-0 below */
        accel_sens[0] = accel_sens[1] = accel_sens[2] = 1;
    }
}

/**
 *  @brief  The main entry point of the MPU Self Test, triggering the run of
 *          the single tests, for gyros and accelerometers.
 *          Prepares the MPU for the test, taking the device out of low power
 *          state if necessary, switching the MPU secondary I2C interface into
 *          bypass mode and restoring the original power state at the end of
 *          the test.
 *          This function is also responsible for encoding the output of each
 *          test in the correct format as it is stored on the file/medium of
 *          choice (according to inv_serial_write_cal() function).
 *          The format needs to stay perfectly consistent with the one expected
 *          by the corresponding loader in ml_stored_data.c; currectly the
 *          loaded in use is inv_load_cal_V1 (record type 1 - initial
 *          calibration).
 *
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  perform_full_test
 *              If 1:
 *              Complete calibration test:
 *              Calculate offset, drive frequency, and noise and compare it
 *              against set thresholds.
 *              When 0:
 *              Skip the noise and drive frequency calculation,
 *              simply calculate the gyro biases.
 *  @param  provide_result
 *              If 1:
 *              Report the final result using a bit-mask like error code as
 *              described in the test_gyro() function.
 *
 *  @return 0 on success.  A non-zero error code on error.
 *          Propagates the errors from the tests up to the caller.
 */
int inv_device_test(void *mlsl_handle,
                    uint_fast8_t sensor_mask,
                    uint_fast8_t perform_full_test,
                    uint_fast8_t provide_result)
{
    int result = 0, gyro_test_result = 0, accel_test_result = 0;
    short temp_avg = 0;
    short gyro_biases[3] = {0, 0, 0};
    short accel_biases[3] = {0, 0, 0};
    long accel_sens[3] = {0};
    unsigned long saved_sensor_mask;
    unsigned char saved_state = inv_get_state();

    mldl_cfg = inv_get_dl_config();
    saved_sensor_mask = mldl_cfg->inv_mpu_cfg->requested_sensors;

    if (sensor_mask & (INV_THREE_AXIS_GYRO & ~INV_DMP_PROCESSOR)) {
        result = inv_set_mpu_sensors(INV_THREE_AXIS_GYRO & ~INV_DMP_PROCESSOR);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return -1;
        }
        if (saved_state < INV_STATE_DMP_STARTED) {
            result = inv_dmp_start();
            if (result) {
                LOG_RESULT_LOCATION(result);
                return -1;
            }
        }
#ifdef TRACK_IDS
        result = get_mpu_unique_id(mlsl_handle);
        if (result != INV_SUCCESS) {
            MPL_LOGI("Could not read the device's unique ID\n");
            MPL_LOGI("\n");
            return -1;
        }
#endif
        MPL_LOGI("Collecting one group of %d ms samples for each axis\n",
            (perform_full_test ? DEF_PERIOD_CAL : DEF_PERIOD_SELF));
        MPL_LOGI("\n");

        /* adjust the gyro sensitivity according to the gyro_sens_trim value */
        adj_gyro_sens = test_setup.gyro_sens *
            mldl_cfg->mpu_chip_info->gyro_sens_trim / 131.072f;
        test_setup.gyro_fs = (int)(32768.f / adj_gyro_sens);

        /* collect gyro and temperature data, test gyro, report result */
        gyro_test_result = test_gyro(mlsl_handle,
                                gyro_biases, &temp_avg, perform_full_test);
        MPL_LOGI("\n");
        if (gyro_test_result == 0) {
            MPL_LOGI_IF(provide_result, "Test : PASSED\n");
        } else {
            MPL_LOGI_IF(provide_result, "Test : FAILED %d/%04X - Biases NOT stored\n",
                        gyro_test_result, gyro_test_result);
            goto gyro_test_failed;
        }
        MPL_LOGI_IF(provide_result, "\n");
    }

    if (sensor_mask & INV_THREE_AXIS_ACCEL) {
        if (!mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]) {
            MPL_LOGI("\n");
            MPL_LOGI("No accelerometer configured\n");
        } else {
            result = inv_set_mpu_sensors(INV_THREE_AXIS_ACCEL);
            if (result)
                return -1;
            if (inv_get_state() < INV_STATE_DMP_STARTED) {
                result = inv_dmp_start();
                if (result)
                    return -1;
            }

            get_accel_sensitivity(accel_sens);
            /* collect accel data.  if this step is skipped,
               ensure the array still contains zeros. */
            accel_test_result =
                test_accel(mlsl_handle, sensor_mask >> 4,
                           accel_biases, accel_sens[Z],
                           perform_full_test);
            if (accel_test_result)
                goto accel_test_failed;

            /* if only Z accel is requested,
               clear out the biases from the other 2 axes */
            if ((sensor_mask & INV_THREE_AXIS_ACCEL) == INV_Z_ACCEL)
                    accel_biases[X] = accel_biases[Y] = 0;
        }
    }

	
	ALOGE("in %s: accel_bias is %d %d %d", __func__, accel_biases[0],
			accel_biases[1], accel_biases[2]);
    result = populate_data_store(sensor_mask, temp_avg, gyro_biases,
                                     accel_biases, accel_sens);
    if (result)
        return -1;
    result = inv_serial_write_cal(data_store, ML_INIT_CAL_LEN);
    if (result) {
        MPL_LOGI("Error : cannot write calibration on file - error %d\n",
            result);
        LOG_RESULT_LOCATION(result);
        return -1;
    }

gyro_test_failed:
accel_test_failed:
    /* restore the setting had at the beginning */
    mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;
    result = inv_set_mpu_sensors(saved_sensor_mask);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return -1;
    }
    /* turn off only if it was off when the function was called */
    if (saved_state < INV_STATE_DMP_STARTED) {
        result = inv_dmp_stop();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return -1;
        }
    }

    if (gyro_test_result)
        return gyro_test_result;
    if (accel_test_result)
        return accel_test_result;

    return result;
}

#ifdef __cplusplus
}
#endif

/**
 *  @}
 */


