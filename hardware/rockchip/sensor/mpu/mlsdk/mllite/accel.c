/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: accel.c 5873 2011-08-11 03:13:48Z mcaramello $
 *
 *******************************************************************************/

/**
 *  @defgroup ACCELDL
 *  @brief  Motion Library - Accel Driver Layer.
 *          Provides the interface to setup and handle an accel
 *          connected to either the primary or the seconday I2C interface
 *          of the gyroscope.
 *
 *  @{
 *      @file   accel.c
 *      @brief  Accel setup and handling methods.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <string.h>

#include "ml.h"
#include "mlinclude.h"
#include "dmpKey.h"
#include "mlFIFO.h"
#include "mldl.h"
#include "mldl_cfg.h"
#include "mlMathFunc.h"
#include "mlsl.h"
#include "mlos.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-accel"

#define ACCEL_DEBUG 0

/* --------------------- */
/* - Global Variables. - */
/* --------------------- */

/* --------------------- */
/* - Static Variables. - */
/* --------------------- */

/* --------------- */
/* - Prototypes. - */
/* --------------- */

/* -------------- */
/* - Externs.   - */
/* -------------- */

/* -------------- */
/* - Functions. - */
/* -------------- */

/**
 *  @brief  Used to determine if an accel is configured and
 *          used by the MPL.
 *  @return INV_SUCCESS if the accel is present.
 */
unsigned char inv_accel_present(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL] &&
        mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->resume &&
        mldl_cfg->inv_mpu_cfg->requested_sensors & INV_THREE_AXIS_ACCEL)
        return true;
    else
        return false;
}

/**
 *  @brief   Query the accel slave address.
 *  @return  The 7-bit accel slave address.
 */
unsigned char inv_get_slave_addr(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL])
        return mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]->address;
    else
        return 0;
}

/**
 *  @brief   Get the ID of the accel in use.
 *  @return  ID of the accel in use.
 */
unsigned short inv_get_accel_id(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]) {
        return mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->id;
    }
    return ID_INVALID;
}

/**
 *  @brief  Get a sample of accel data from the device.
 *  @param  data
 *              the buffer to store the accel raw data for
 *              X, Y, and Z axes.
 *  @return INV_SUCCESS or a non-zero error code.
 */
inv_error_t inv_get_accel_data(long *data)
{
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    inv_error_t result;
    unsigned char raw_data[2 * ACCEL_NUM_AXES];
    long tmp[ACCEL_NUM_AXES];
    int ii;
    signed char *mtx;
    char accel_id;

    if (NULL == data)
        return INV_ERROR_INVALID_PARAMETER;

    if (!inv_accel_present()) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_MODULE);
        return INV_ERROR_INVALID_MODULE;
    }
    mtx = mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]->orientation;
    accel_id = mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->id;

    if (mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->read_len > sizeof(raw_data))
        return INV_ERROR_ASSERTION_FAILURE;

    result = (inv_error_t) inv_mpu_read_accel(mldl_cfg,
                                              inv_get_serial_handle(),
                                              inv_get_serial_handle(),
                                              raw_data);
    if (result == INV_ERROR_ACCEL_DATA_NOT_READY) {
        return result;
    }
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    for (ii = 0; ii < ARRAY_SIZE(tmp); ii++) {
        if (EXT_SLAVE_LITTLE_ENDIAN ==
            mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->endian) {
            tmp[ii] = (long)((signed char)raw_data[2 * ii + 1]) * 256;
            tmp[ii] += (long)((unsigned char)raw_data[2 * ii]);
        } else if ((EXT_SLAVE_BIG_ENDIAN ==
                    mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->endian) ||
                   (EXT_SLAVE_FS16_BIG_ENDIAN ==
                    mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->endian)) {
            tmp[ii] = (long)((signed char)raw_data[2 * ii]) * 256;
            tmp[ii] += (long)((unsigned char)raw_data[2 * ii + 1]);
            if (accel_id == ACCEL_ID_KXSD9) {
                tmp[ii] = (long)((short)(((unsigned short)tmp[ii])
                                         + ((unsigned short)0x8000)));
            }
        } else if (EXT_SLAVE_FS8_BIG_ENDIAN ==
                   mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->endian) {
            tmp[ii] = (long)((signed char)raw_data[ii]) * 256;
        } else {
            result = INV_ERROR_FEATURE_NOT_IMPLEMENTED;
        }
    }

    for (ii = 0; ii < ARRAY_SIZE(tmp); ii++) {
        data[ii] = ((long)tmp[0] * mtx[3 * ii] +
                    (long)tmp[1] * mtx[3 * ii + 1] +
                    (long)tmp[2] * mtx[3 * ii + 2]);
    }

    //MPL_LOGI("ACCEL: %8ld, %8ld, %8ld\n", data[0], data[1], data[2]);
    return result;
}

/**
 *  @}
 */
