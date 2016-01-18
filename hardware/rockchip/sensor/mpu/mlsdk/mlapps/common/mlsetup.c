/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: mlsetup.c 6241 2011-10-28 20:54:58Z mcaramello $
 *
 *****************************************************************************/
#undef MPL_LOG_NDEBUG
#ifdef UNITTESTING
#define MPL_LOG_NDEBUG 1
#else
#define MPL_LOG_NDEBUG 0
#endif

/**
 *  @defgroup MLSETUP
 *  @brief  The Motion Library external slaves setup override suite.
 *
 *          Use these APIs to override the kernel/default settings in the
 *          corresponding data structures for gyros, accel, and compass.
 *
 *  @{
 *      @file mlsetup.c
 *      @brief The Motion Library external slaves setup override suite.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

/*
    Defines
*/
/* these have to appear before inclusion of mpu.h */
#define CONFIG_MPU_SENSORS_KXSD9         y   // Kionix accel
#define CONFIG_MPU_SENSORS_KXTF9         y   // Kionix accel
#define CONFIG_MPU_SENSORS_LIS331DLH     y   // ST accelerometer
#define CONFIG_MPU_SENSORS_LSM303DLX_A   y   // ST accelerometer in LSM303DLx combo
#define CONFIG_MPU_SENSORS_LIS3DH        y   // ST accelerometer
#define CONFIG_MPU_SENSORS_BMA150        y   // Bosch 150 accelerometer
#define CONFIG_MPU_SENSORS_BMA222        y   // Bosch 222 accelerometer
#define CONFIG_MPU_SENSORS_BMA250        y   // Bosch 250 accelerometer
#define CONFIG_MPU_SENSORS_ADXL34X       y   // AD 345 or 346 accelerometer
#define CONFIG_MPU_SENSORS_MMA8450       y   // Freescale MMA8450 accelerometer
#define CONFIG_MPU_SENSORS_MMA845X       y   // Freescale MMA845X accelerometer
#define CONFIG_MPU_SENSORS_MPU6050_ACCEL y   // Invensense MPU6050 built-in accelerometer

#define CONFIG_MPU_SENSORS_AK8975        y   // AKM compass
#define CONFIG_MPU_SENSORS_AMI30X        y   // AICHI AMI304/305 compass
#define CONFIG_MPU_SENSORS_AMI306        y   // AICHI AMI306 compass
#define CONFIG_MPU_SENSORS_HMC5883       y   // Honeywell compass
#define CONFIG_MPU_SENSORS_LSM303DLX_M   y   // ST compass in LSM303DLx combo
#define CONFIG_MPU_SENSORS_YAS529        y   // Yamaha compass
#define CONFIG_MPU_SENSORS_YAS530        y   // Yamaha compass
#define CONFIG_MPU_SENSORS_MMC314X       y   // MEMSIC compass
#define CONFIG_MPU_SENSORS_HSCDTD002B    y   // ALPS compass
#define CONFIG_MPU_SENSORS_HSCDTD004A    y   // ALPS HSCDTD004A compass

#define CONFIG_MPU_SENSORS_BMA085        y   // Bosch 085 pressure

#include "external_hardware.h"

#include <stdio.h>
#include <string.h>

#include "slave.h"
#include "compass.h"
#include "pressure.h"
#include "ml.h"
#include "mldl.h"
#include "mlstates.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mlsetup"

#include "mpu.h"
#include "mldl_cfg.h"

#include "mlsetup.h"

#ifdef LINUX
#include "errno.h"
#endif

/* Override these structures from mldl.c */
extern struct ext_slave_descr          g_slave_accel;
extern struct ext_slave_descr          g_slave_compass;
extern struct ext_slave_descr          g_slave_pressure;
/* Platform Data */
extern struct mpu_platform_data        g_pdata;
extern struct ext_slave_platform_data  g_pdata_slave_accel;
extern struct ext_slave_platform_data  g_pdata_slave_compass;
extern struct ext_slave_platform_data  g_pdata_slave_pressure;

/*
    Typedefs
*/
typedef void tSetupFuncAccel(void);
typedef void tSetupFuncCompass(void);
typedef void tSetupFuncPressure(void);

#ifdef LINUX
#include <sys/ioctl.h>
#endif

/*********************************************************************
              Dragon - PLATFORM_ID_MPU9150_PROTOTYPE
*********************************************************************/
/**
 * @internal
 * @brief  performs a 180' rotation around Z axis to reflect
 *         usage of the multi sensor board (MSB) with the
 *         beagleboard
 * @note   assumes well formed mounting matrix, with only
 *         one 1 for each row.
 */
static void Rotate180DegAroundZAxis(signed char matrix[])
{
    int ii;
    for(ii=0; ii<6; ii++) {
        matrix[ii] = -matrix[ii];
    }
}

/**
 * @internal
 * Sets the orientation based on the position of the mounting.  For different
 * devices the relative position to pin 1 will be different.
 *
 * Positions are:
 * - 0-3 are Z up
 * - 4-7 are Z down
 * - 8-11 are Z right
 * - 12-15 are Z left
 * - 16-19 are Z front
 * - 20-23 are Z back
 *
 * @param position The position of the orientation
 * @param orientation the location to store the new oreintation
 */
static inv_error_t SetupOrientation(unsigned int position,
                                 signed char *orientation)
{
    memset(orientation, 0, 9);
    switch (position){
    case 0:
        /*-------------------------*/
        orientation[0] = +1;
        orientation[4] =     +1;
        orientation[8] =         +1;
        break;
    case 1:
        /*-------------------------*/
        orientation[1] =     +1;
        orientation[3] = -1;
        orientation[8] =         +1;
        break;
    case 2:
        /*-------------------------*/
        orientation[0] = -1;
        orientation[4] =     -1;
        orientation[8] =         +1;
        break;
    case 3:
        /*-------------------------*/
        orientation[1] =     -1;
        orientation[3] = +1;
        orientation[8] =         +1;
        break;
    case 4:
        /*-------------------------*/
        orientation[0] = -1;
        orientation[4] =     +1;
        orientation[8] =         -1;
        break;
    case 5:
        /*-------------------------*/
        orientation[1] =     -1;
        orientation[3] = -1;
        orientation[8] =         -1;
        break;
    case 6:
        /*-------------------------*/
        orientation[0] = +1;
        orientation[4] =     -1;
        orientation[8] =         -1;
        break;
    case 7:
        /*-------------------------*/
        orientation[1] =     +1;
        orientation[3] = +1;
        orientation[8] =         -1;
        break;
    case 8:
        /*-------------------------*/
        orientation[2] =         +1;
        orientation[3] = +1;
        orientation[7] =     +1;
        break;
    case 9:
        /*-------------------------*/
        orientation[2] = +1;
        orientation[4] = +1;
        orientation[6] = -1;
        break;
    case 10:
        orientation[2] = +1;
        orientation[3] = -1;
        orientation[7] = -1;
        break;
    case 11:
        orientation[2] = +1;
        orientation[4] = -1;
        orientation[6] = +1;
        break;
    case 12:
        orientation[2] = -1;
        orientation[3] = -1;
        orientation[7] = +1;
        break;
    case 13:
        orientation[2] = -1;
        orientation[4] = -1;
        orientation[6] = -1;
        break;
    case 14:
        orientation[2] = -1;
        orientation[3] = +1;
        orientation[7] = -1;
        break;
    case 15:
        orientation[2] = -1;
        orientation[4] = +1;
        orientation[6] = +1;
        break;
    case 16:
        orientation[0] = -1;
        orientation[5] = +1;
        orientation[7] = +1;
        break;
    case 17:
        orientation[1] = -1;
        orientation[5] = +1;
        orientation[6] = -1;
        break;
    case 18:
        orientation[0] = +1;
        orientation[5] = -1;
        orientation[7] = -1;
        break;
    case 19:
        orientation[1] = -1;
        orientation[5] = +1;
        orientation[6] = +1;
        break;
    case 20:
        orientation[0] = +1;
        orientation[5] = -1;
        orientation[7] = +1;
        break;
    case 21:
        orientation[1] = -1;
        orientation[5] = -1;
        orientation[6] = +1;
        break;
    case 22:
        orientation[0] = -1;
        orientation[5] = -1;
        orientation[7] = -1;
        break;
    case 23:
        orientation[1] = +1;
        orientation[5] = -1;
        orientation[6] = -1;
        break;
    default:
        MPL_LOGE("Invalid position %d\n", position);
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }

    return INV_SUCCESS;
}

static void PrintMountingOrientation(
    const char * header, signed char *orientation)
{
    MPL_LOGV("%s:\n", header);
    MPL_LOGV("\t[[%3d, %3d, %3d]\n",
             orientation[0], orientation[1], orientation[2]);
    MPL_LOGV("\t [%3d, %3d, %3d]\n",
             orientation[3], orientation[4], orientation[5]);
    MPL_LOGV("\t [%3d, %3d, %3d]]\n",
             orientation[6], orientation[7], orientation[8]);
}

/*****************************
 *   Accel Setup Functions   *
 *****************************/

static inv_error_t SetupAccelSTLIS331Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 5;
        break;
    case PLATFORM_ID_ST_6AXIS:
        position = 0;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *lis331_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_LIS331;
    return INV_SUCCESS;
}

static inv_error_t SetupAccelSTLIS3DHCalibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 1;
        break;
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
        position = 3;
        break;
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *lis3dh_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_LIS3DH;
    return result;
}

static inv_error_t SetupAccelKionixKXSD9Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 7;
        break;
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *kxsd9_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_KXSD9;
    return result;
}

static inv_error_t SetupAccelKionixKXTF9Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB_EVB:
        position =0;
            break;
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 7;
        break;
#ifdef WIN32
    case PLATFORM_ID_DONGLE:
        position = 1;
        break;
#endif
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
        position = 1;
        break;
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *kxtf9_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_KXTF9;
    return result;
}

static inv_error_t SetupAccelLSM303Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
        position = 3;
        break;
    case PLATFORM_ID_MSB_V2:
        position = 1;
        break;
    case PLATFORM_ID_MSB_10AXIS:
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *lsm303dlx_a_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_LSM303;
    return result;
}

static inv_error_t SetupAccelBMA150Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 6;
        break;
#ifdef WIN32
    case PLATFORM_ID_DONGLE:
        position = 3;
        break;
#endif
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *bma150_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_BMA150;
    return result;
}

static inv_error_t SetupAccelBMA222Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 0;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *bma222_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_BMA222;
    return result;
}

static inv_error_t SetupAccelBMA250Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
        position = 0;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:

    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *bma250_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_BMA250;
    return result;
}

static inv_error_t SetupAccelADXL34XCalibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 6;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *adxl34x_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_ADXL34X;
    return result;
}


static inv_error_t SetupAccelMMA8450Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 5;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *mma8450_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_MMA8450;
    return result;
}


static inv_error_t SetupAccelMMA845XCalibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 5;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_accel.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_accel = *mma845x_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = ACCEL_SLAVEADDR_MMA845X;
    return result;
}


/**
 * @internal
 * Sets up the orientation matrix according to how the gyro was
 * mounted.
 *
 * @param platforId Platform identification for mounting information
 * @return INV_SUCCESS or non-zero error code
 */
static inv_error_t SetupAccelMPU6050Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
    case PLATFORM_ID_MPU6050_MSB:
        position = 6;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
	case PLATFORM_ID_MPU9150_USB_DONGLE:
        position = 1;
        break;
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    case PLATFORM_ID_MPU6050_EVB:
        position = 0;
        break;
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        return INV_ERROR_INVALID_PARAMETER;
    };

    SetupOrientation(position, g_pdata_slave_accel.orientation);
    /* Interrupt */
#ifndef LINUX
    g_slave_accel = *mpu6050_get_slave_descr();
#endif
    g_pdata_slave_accel.address         = 0x68;
    return result;
}

/*****************************
    Compass Setup Functions
******************************/
static inv_error_t SetupCompassAKM8975Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
    case PLATFORM_ID_MPU6050_MSB:
        position = 2;
        break;
#ifdef WIN32
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
        position = 4;
        break;
#endif
    case PLATFORM_ID_MPU9150_PROTOTYPE:
        position = 7;
        break;
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    case PLATFORM_ID_MPU9150_USB_DONGLE:
    case PLATFORM_ID_MSB_EVB:
        position = 5;
        break;
    case PLATFORM_ID_MPU6050_EVB:
        position = 4;
        break;
	 case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *ak8975_get_slave_descr();
#endif
    g_pdata_slave_compass.address         = COMPASS_SLAVEADDR_AKM;
    return result;
}

static inv_error_t SetupCompassMMCCalibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 7;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *mmc314x_get_slave_descr();
#endif
    g_pdata_slave_compass.address         = COMPASS_SLAVEADDR_MMC314X;
    return result;
}

static inv_error_t SetupCompassAMI304Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 4;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    g_pdata_slave_compass.address         = COMPASS_SLAVEADDR_AMI304;
#ifndef LINUX
    g_slave_compass = *ami30x_get_slave_descr();
#endif
    return result;
}

static inv_error_t SetupCompassAMI306Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 3;
        break;
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
        position = 1;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *ami306_get_slave_descr();
#endif
    g_pdata_slave_compass.address         = COMPASS_SLAVEADDR_AMI306;
    return result;
}

static inv_error_t SetupCompassHMC5883Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 6;
        break;
#ifdef WIN32
    case PLATFORM_ID_DONGLE:
        position = 2;
        break;
#endif
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:

    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };

    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *hmc5883_get_slave_descr();
#endif
    g_pdata_slave_compass.address = COMPASS_SLAVEADDR_HMC5883;
    return result;
}


static inv_error_t SetupCompassLSM303DLHCalibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_10AXIS:
        position = 1;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:

    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
    };
    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
#ifndef LINUX
    g_slave_compass = *lsm303dlx_m_get_slave_descr();
    g_slave_compass.id = COMPASS_ID_LSM303DLH;
#endif
    g_pdata_slave_compass.address = COMPASS_SLAVEADDR_HMC5883;
    return result;
}

static inv_error_t SetupCompassLSM303DLMCalibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
        position = 8;
        break;
    case PLATFORM_ID_MSB_V2:
        position = 12;
        break;
    case PLATFORM_ID_MSB_10AXIS:
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
    };
    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *lsm303dlx_m_get_slave_descr();
    g_slave_compass.id = COMPASS_ID_LSM303DLM;
#endif
    g_pdata_slave_compass.address = COMPASS_SLAVEADDR_HMC5883;
    return result;
}

static inv_error_t SetupCompassYAS530Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
        position = 1;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };
    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *yas530_get_slave_descr();
#endif
    g_pdata_slave_compass.address         = COMPASS_SLAVEADDR_YAS530;
    return result;
}

static inv_error_t SetupCompassYAS529Calibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 6;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };
    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *yas529_get_slave_descr();
#endif
    g_pdata_slave_compass.address         = COMPASS_SLAVEADDR_YAS529;
    return result;
}


static inv_error_t SetupCompassHSCDTD002BCalibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 2;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };
    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *hscdtd002b_get_slave_descr();
#endif
    g_pdata_slave_compass.address         = COMPASS_SLAVEADDR_HSCDTD00XX;
    return result;
}

static inv_error_t SetupCompassHSCDTD004ACalibration(unsigned short platformId)
{
    inv_error_t result = INV_SUCCESS;
    unsigned int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
        position = 1;
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_MPU6050_MSB:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    };
    result = SetupOrientation(position, g_pdata_slave_compass.orientation);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

#ifndef LINUX
    g_slave_compass = *hscdtd004a_get_slave_descr();
#endif
    g_pdata_slave_compass.address         = COMPASS_SLAVEADDR_HSCDTD00XX;
    return result;
}


/*****************************
    Pressure Setup Functions
******************************/
static inv_error_t SetupPressureBMA085Calibration(unsigned short platformId)
{
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    memset(g_pdata_slave_pressure.orientation, 0, sizeof(g_pdata_slave_pressure.orientation));

    g_pdata_slave_pressure.bus             = EXT_SLAVE_BUS_PRIMARY;
#ifndef LINUX
    g_slave_pressure = *bma085_get_slave_descr();
#endif
    g_pdata_slave_pressure.address         = PRESSURE_SLAVEADDR_BMA085;
    return INV_SUCCESS;
}

/**
 * @internal
 * Sets up the orientation matrix according to how the part was
 * mounted.
 *
 * @param platforId Platform identification for mounting information
 * @return INV_SUCCESS or non-zero error code
 */
static inv_error_t SetupAccelCalibration(unsigned short platformId,
                                      unsigned short accelId)
{
    /*----  setup the accels ----*/
    switch(accelId) {
    case ACCEL_ID_LSM303DLX:
        SetupAccelLSM303Calibration(platformId);
        break;
    case ACCEL_ID_LIS331:
        SetupAccelSTLIS331Calibration(platformId);
        break;
    case ACCEL_ID_KXSD9:
        SetupAccelKionixKXSD9Calibration(platformId);
        break;
    case ACCEL_ID_KXTF9:
        SetupAccelKionixKXTF9Calibration(platformId);
        break;
    case ACCEL_ID_BMA150:
        SetupAccelBMA150Calibration(platformId);
        break;
    case ACCEL_ID_BMA222:
        SetupAccelBMA222Calibration(platformId);
        break;
    case ACCEL_ID_BMA250:
        SetupAccelBMA250Calibration(platformId);
        break;
    case ACCEL_ID_ADXL34X:
        SetupAccelADXL34XCalibration(platformId);
        break;
    case ACCEL_ID_MMA8450:
        SetupAccelMMA8450Calibration(platformId);
        break;
    case ACCEL_ID_MMA845X:
        SetupAccelMMA845XCalibration(platformId);
        break;
    case ACCEL_ID_LIS3DH:
        SetupAccelSTLIS3DHCalibration(platformId);
        break;
    case ACCEL_ID_MPU6050:
        SetupAccelMPU6050Calibration(platformId);
        break;
    case ID_INVALID:
        break;
    default:
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }

    if (accelId != ID_INVALID && accelId != ACCEL_ID_MPU6050) {
        g_pdata_slave_accel.bus             = EXT_SLAVE_BUS_SECONDARY;
    } else if (accelId != ACCEL_ID_MPU6050) {
        g_pdata_slave_accel.bus             = EXT_SLAVE_BUS_PRIMARY;
    }

#ifndef WIN32
    if (accelId != ID_INVALID)
        Rotate180DegAroundZAxis(g_pdata_slave_accel.orientation);
#endif

    return INV_SUCCESS;
}

/**
 * @internal
 * Sets up the orientation matrix according to how the part was
 * mounted.
 *
 * @param platforId Platform identification for mounting information
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t SetupCompassCalibration(unsigned short platformId,
                                 unsigned short compassId)
{
    /*----  setup the compass ----*/
    switch(compassId) {
    case COMPASS_ID_AK8975:
        SetupCompassAKM8975Calibration(platformId);
        break;
    case COMPASS_ID_AMI30X:
        SetupCompassAMI304Calibration(platformId);
        break;
    case COMPASS_ID_AMI306:
        SetupCompassAMI306Calibration(platformId);
        break;
    case COMPASS_ID_LSM303DLH:
        SetupCompassLSM303DLHCalibration(platformId);
        break;
    case COMPASS_ID_LSM303DLM:
        SetupCompassLSM303DLMCalibration(platformId);
        break;
    case COMPASS_ID_HMC5883:
        SetupCompassHMC5883Calibration(platformId);
        break;
    case COMPASS_ID_YAS529:
        SetupCompassYAS529Calibration(platformId);
        break;
    case COMPASS_ID_YAS530:
        SetupCompassYAS530Calibration(platformId);
        break;
    case COMPASS_ID_MMC314X:
        SetupCompassMMCCalibration(platformId);
        break;
    case COMPASS_ID_HSCDTD002B:
        SetupCompassHSCDTD002BCalibration(platformId);
        break;
    case COMPASS_ID_HSCDTD004A:
        SetupCompassHSCDTD004ACalibration(platformId);
        break;
    case ID_INVALID:
        break;
    default:
        if (INV_ERROR_INVALID_PARAMETER) {
            LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
            return INV_ERROR_INVALID_PARAMETER;
        }
        break;
    }

    if (platformId == PLATFORM_ID_MSB_V2_MPU6050     ||
        platformId == PLATFORM_ID_MPU6050_MSB        ||
        platformId == PLATFORM_ID_MPU6050_USB_DONGLE ||
        platformId == PLATFORM_ID_MPU6050_PROTOTYPE  ||
        platformId == PLATFORM_ID_MPU9150_PROTOTYPE) {
        switch (compassId) {
        case ID_INVALID:
            g_pdata_slave_compass.bus = EXT_SLAVE_BUS_INVALID;
            break;
        case COMPASS_ID_AK8975:
        case COMPASS_ID_AMI306:
            g_pdata_slave_compass.bus             = EXT_SLAVE_BUS_SECONDARY;
            break;
        default:
            g_pdata_slave_compass.bus             = EXT_SLAVE_BUS_PRIMARY;
        };
    } else {
        g_pdata_slave_compass.bus             = EXT_SLAVE_BUS_PRIMARY;
    }

#ifndef WIN32
    if (compassId != ID_INVALID)
        Rotate180DegAroundZAxis(g_pdata_slave_compass.orientation);
#endif

    return INV_SUCCESS;
}

/**
 * @internal
 * Sets up the orientation matrix according to how the part was
 * mounted.
 *
 * @param platforId Platform identification for mounting information
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t SetupPressureCalibration(unsigned short platformId,
                                  unsigned short pressureId)
{
    inv_error_t result = INV_SUCCESS;
    /*----  setup the compass ----*/
    switch(pressureId) {
        case PRESSURE_ID_BMA085:
            result = SetupPressureBMA085Calibration(platformId);
            break;
        default:
            if (INV_ERROR_INVALID_PARAMETER) {
                LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
                return INV_ERROR_INVALID_PARAMETER;
            }
    };

    return result;
}

/**
 * @internal
 * Sets up the orientation matrix according to how the gyro was
 * mounted.
 *
 * @param platforId Platform identification for mounting information
 * @return INV_SUCCESS or non-zero error code
 */
static inv_error_t SetupGyroCalibration(unsigned short platformId)
{
    int position;
    MPL_LOGV("Calibrating '%s'\n", __func__);

    /* Orientation */
    switch (platformId) {
    case PLATFORM_ID_MSB:
    case PLATFORM_ID_MSB_10AXIS:
    case PLATFORM_ID_MPU6050_MSB:
#ifndef WIN32
        position = 4;
#else
        position = 6;
#endif
        break;
    case PLATFORM_ID_DONGLE:
    case PLATFORM_ID_MPU6050_USB_DONGLE:
        position = 1;
        break;
	case PLATFORM_ID_MPU9150_USB_DONGLE:
        position = 3;
        break;
    case PLATFORM_ID_MPU6050_PROTOTYPE:
    case PLATFORM_ID_MPU9150_PROTOTYPE:
    case PLATFORM_ID_ST_6AXIS:
    case PLATFORM_ID_MSB_V2:
    case PLATFORM_ID_MSB_V2_MPU6050:
#ifndef WIN32
        position = 2;
#else
        position = 0;
#endif
        break;
    case PLATFORM_ID_MPU6050_EVB:
    case PLATFORM_ID_MSB_EVB:
        position = 0;
        break;
    default:
        MPL_LOGE("Unsupported platform %d\n", platformId);
        return INV_ERROR_INVALID_PARAMETER;
    };

    SetupOrientation(position, g_pdata.orientation);
    /* Interrupt */
    g_pdata.int_config = BIT_INT_ANYRD_2CLEAR;
    return INV_SUCCESS;
}

/**
 *  @brief  Setup the Hw orientation and full scale.
 *  @param  platfromId
 *              an user defined Id to distinguish the Hw platform in
 *              use from others.
 *  @param  accelId
 *              the accelerometer specific id, as specified in the MPL.
 *  @param  compassId
 *              the compass specific id, as specified in the MPL.
 *  @return INV_SUCCESS or a non-zero error code.
 */
inv_error_t SetupPlatform(
                unsigned short platformId,
                unsigned short accelId,
                unsigned short compassId)
{
    int result;

    if (inv_get_state() > INV_STATE_SERIAL_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    memset(&g_slave_accel, 0, sizeof(g_slave_accel));
    memset(&g_slave_compass, 0, sizeof(g_slave_compass));
    memset(&g_slave_pressure, 0, sizeof(g_slave_pressure));
    memset(&g_pdata, 0, sizeof(g_pdata));

#ifdef LINUX
    /* On Linux initialize the global platform data with the driver defaults */
    {
        void *mpu_handle;
        int ii;

        struct ext_slave_descr *slave[EXT_SLAVE_NUM_TYPES];
        struct ext_slave_platform_data *pdata_slave[EXT_SLAVE_NUM_TYPES];
        slave[EXT_SLAVE_TYPE_GYROSCOPE] = NULL;
        slave[EXT_SLAVE_TYPE_ACCEL] = &g_slave_accel;
        slave[EXT_SLAVE_TYPE_COMPASS] = &g_slave_compass;
        slave[EXT_SLAVE_TYPE_PRESSURE] = &g_slave_pressure;

        pdata_slave[EXT_SLAVE_TYPE_GYROSCOPE] = NULL;
        pdata_slave[EXT_SLAVE_TYPE_ACCEL] = &g_pdata_slave_accel;
        pdata_slave[EXT_SLAVE_TYPE_COMPASS] = &g_pdata_slave_compass;
        pdata_slave[EXT_SLAVE_TYPE_PRESSURE] = &g_pdata_slave_pressure;

        MPL_LOGI("Getting the MPU_GET_PLATFORM_DATA\n");
        result = inv_serial_open("/dev/mpu",&mpu_handle);
        if (result) {
            MPL_LOGE("MPU_GET_PLATFORM_DATA failed %d\n", result);
        }
        for (ii = 0; ii < EXT_SLAVE_NUM_TYPES; ii++) {
            if (!slave[ii])
                continue;
            slave[ii]->type = ii;
            result = ioctl((int)mpu_handle, MPU_GET_EXT_SLAVE_DESCR,
                           slave[ii]);
            if (result)
                result = errno;
            if(result == INV_ERROR_INVALID_MODULE) {
                slave[ii] = NULL;
                result = 0;
            } else if (result) {
                LOG_RESULT_LOCATION(result);
                LOG_RESULT_LOCATION(INV_ERROR_INVALID_MODULE);
                return result;
            }
        }
        result = ioctl((int)mpu_handle, MPU_GET_MPU_PLATFORM_DATA, &g_pdata);
        if (result) {
            result = errno;
            LOG_RESULT_LOCATION(result);
            return result;
        }
        for (ii = 0; ii < EXT_SLAVE_NUM_TYPES; ii++) {
            if (!pdata_slave[ii])
                continue;
            pdata_slave[ii]->type = ii;
            result = ioctl(
                (int)mpu_handle, MPU_GET_EXT_SLAVE_PLATFORM_DATA,
                pdata_slave[ii]);
            if (result)
                result = errno;
            if (result == INV_ERROR_INVALID_MODULE) {
                pdata_slave[ii] = NULL;
                result = 0;
            } else if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
        if (result) {
            MPL_LOGE("MPU_GET_PLATFORM_DATA failed %d\n", result);
        }
        inv_serial_close(mpu_handle);
    }
#endif

    result = SetupGyroCalibration(platformId);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    PrintMountingOrientation("Gyroscope", g_pdata.orientation);
    result = SetupAccelCalibration(platformId, accelId);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    PrintMountingOrientation("Accelerometer", g_pdata_slave_accel.orientation);
    result = SetupCompassCalibration(platformId, compassId);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    PrintMountingOrientation("Compass", g_pdata_slave_compass.orientation);
    if (platformId == PLATFORM_ID_MSB_10AXIS) {
        result = SetupPressureCalibration(platformId, PRESSURE_ID_BMA085);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        PrintMountingOrientation("Pressure", g_pdata_slave_pressure.orientation);
    }

#ifdef LINUX
    /* On Linux override the orientation, level shifter etc */
    {
        void *mpu_handle;
        int ii;
        struct ext_slave_descr *slave[EXT_SLAVE_NUM_TYPES];
        struct ext_slave_platform_data *pdata_slave[EXT_SLAVE_NUM_TYPES];
        slave[EXT_SLAVE_TYPE_GYROSCOPE] = NULL;
        slave[EXT_SLAVE_TYPE_ACCEL] = &g_slave_accel;
        slave[EXT_SLAVE_TYPE_COMPASS] = &g_slave_compass;
        slave[EXT_SLAVE_TYPE_PRESSURE] = &g_slave_pressure;

        pdata_slave[EXT_SLAVE_TYPE_GYROSCOPE] = NULL;
        pdata_slave[EXT_SLAVE_TYPE_ACCEL] = &g_pdata_slave_accel;
        pdata_slave[EXT_SLAVE_TYPE_COMPASS] = &g_pdata_slave_compass;
        pdata_slave[EXT_SLAVE_TYPE_PRESSURE] = &g_pdata_slave_pressure;

        MPL_LOGI("Setting the MPU_SET_PLATFORM_DATA\n");
        result = inv_serial_open("/dev/mpu", &mpu_handle);
        if (result) {
            MPL_LOGE("MPU_SET_PLATFORM_DATA failed %d\n", result);
        }
        for (ii = 0; ii < EXT_SLAVE_NUM_TYPES; ii++) {
            if (!slave[ii])
                continue;
            slave[ii]->type = ii;
            result = ioctl((int)mpu_handle, MPU_SET_EXT_SLAVE_PLATFORM_DATA,
                           slave[ii]);
            if (result)
                result = errno;
            if (result == INV_ERROR_INVALID_MODULE) {
                slave[ii] = NULL;
                result = 0;
            } else if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
        result = ioctl((int)mpu_handle, MPU_SET_MPU_PLATFORM_DATA, &g_pdata);
        if (result) {
            result = errno;
            LOG_RESULT_LOCATION(result);
            return result;
        }
        for (ii = 0; ii < EXT_SLAVE_NUM_TYPES; ii++) {
            if (!pdata_slave[ii])
                continue;
            pdata_slave[ii]->type = ii;
            result = ioctl((int)mpu_handle, MPU_SET_EXT_SLAVE_PLATFORM_DATA,
                pdata_slave[ii]);
            if (result)
                result = errno;
            if (result == INV_ERROR_INVALID_MODULE) {
                pdata_slave[ii] = NULL;
                result = 0;
            } else if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
        if (result) {
            MPL_LOGE("MPU_SET_PLATFORM_DATA failed %d\n", result);
        }
        inv_serial_close(mpu_handle);
    }
#endif
    return INV_SUCCESS;
}

/**
 * @}
 */


