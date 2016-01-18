/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: compass.c 6205 2011-10-15 00:39:10Z mcaramello $
 *
 *******************************************************************************/

/** 
 *  @defgroup COMPASSDL 
 *  @brief  Motion Library - Compass Driver Layer.
 *          Provides the interface to setup and handle an compass
 *          connected to either the primary or the seconday I2C interface 
 *          of the gyroscope.
 *
 *  @{
 *      @file   compass.c
 *      @brief  Compass setup and handling methods.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <string.h>

#include "compass.h"

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
#define MPL_LOG_TAG "MPL-compass"

#define COMPASS_DEBUG 0

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
 *  @brief  Used to determine if a compass is
 *          configured and used by the MPL.
 *  @return INV_SUCCESS if the compass is present.
 */
unsigned char inv_compass_present(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS] &&
        mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->resume &&
        mldl_cfg->inv_mpu_cfg->requested_sensors & INV_THREE_AXIS_COMPASS)
        return true;
    else
        return false;
}

/**
 *  @brief   Query the compass slave address.
 *  @return  The 7-bit compass slave address.
 */
unsigned char inv_get_compass_slave_addr(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS])
        return mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS]->address;
    else
        return 0;
}

/**
 *  @brief   Get the ID of the compass in use.
 *  @return  ID of the compass in use.
 */
unsigned short inv_get_compass_id(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]) {
        return mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->id;
    }
    return ID_INVALID;
}

/**
 *  @brief  Get a sample of compass data from the device.
 *  @param  data
 *              the buffer to store the compass raw data for
 *              X, Y, and Z axes.
 *  @return INV_SUCCESS or a non-zero error code.
 */
inv_error_t inv_get_compass_data(long *data)
{
    inv_error_t result;
    int ii;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    unsigned char *tmp = inv_obj.mag->raw_data;

    if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->read_len >
        sizeof(inv_obj.mag->raw_data)) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_CONFIGURATION);
        return INV_ERROR_INVALID_CONFIGURATION;
    }

    if (mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS]->bus ==
        EXT_SLAVE_BUS_PRIMARY ||
        !(mldl_cfg->inv_mpu_cfg->requested_sensors & INV_DMP_PROCESSOR)) {
        /*--- read the compass sensor data.
          The compass read function may return an INV_ERROR_COMPASS_* errors
          when the data is not ready (read/refresh frequency mismatch) or 
          the internal data sampling timing of the device was not respected. 
          Returning the error code will make the sensor fusion supervisor 
          ignore this compass data sample. ---*/
        result = (inv_error_t) inv_mpu_read_compass(mldl_cfg,
                                                    inv_get_serial_handle(),
                                                    inv_get_serial_handle(),
                                                    tmp);
        if (result) {
            if (COMPASS_DEBUG) {
                MPL_LOGV("inv_mpu_read_compass returned %d\n", result);
            }
            return result;
        }
        for (ii = 0; ii < 3; ii++) {
            if (EXT_SLAVE_BIG_ENDIAN ==
                mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->endian)
                data[ii] =
                    ((long)((signed char)tmp[2 * ii]) << 8) + tmp[2 * ii + 1];
            else
                data[ii] =
                    ((long)((signed char)tmp[2 * ii + 1]) << 8) + tmp[2 * ii];
        }
    } else { /* compass on the 2nd bus  or  DMP is off */
        result = inv_get_external_sensor_data(data, 3);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }
    data[0] = inv_q30_mult(data[0], inv_obj.mag->asa[0]);
    data[1] = inv_q30_mult(data[1], inv_obj.mag->asa[1]);
    data[2] = inv_q30_mult(data[2], inv_obj.mag->asa[2]);

    return INV_SUCCESS;
}

/** 
 *  @brief  Sets the compass bias.
 *  @param  bias 
 *              Compass bias, length 3. Scale is chip units * 2^16. 
 *              Frame is mount frame which may be different from body frame.
 *  @return INV_SUCCESS = 0 if successful. A non-zero error code otherwise.
 */
inv_error_t inv_set_compass_bias(struct compass_obj_t *obj, long *bias)
{
    inv_error_t result = INV_SUCCESS;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    extern struct compass_obj_t inv_compass_obj;

    if (obj == NULL) {
        obj = &inv_compass_obj;
    }

    obj->bias[0] = bias[0];
    obj->bias[1] = bias[1];
    obj->bias[2] = bias[2];

    inv_obj.mag->bias[0] = inv_q30_mult(bias[0], inv_obj.mag->sens);
    inv_obj.mag->bias[1] = inv_q30_mult(bias[1], inv_obj.mag->sens);
    inv_obj.mag->bias[2] = inv_q30_mult(bias[2], inv_obj.mag->sens);

    if (inv_dmpkey_supported(KEY_CPASS_BIAS_X)) {
        unsigned char reg[4];
        long biasB[3];
        signed char *orC =
            mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS]->orientation;

        // Now transform to body frame
        biasB[0] = bias[0] * orC[0] + bias[1] * orC[1] + bias[2] * orC[2];
        biasB[1] = bias[0] * orC[3] + bias[1] * orC[4] + bias[2] * orC[5];
        biasB[2] = bias[0] * orC[6] + bias[1] * orC[7] + bias[2] * orC[8];

        result =
            inv_set_mpu_memory(KEY_CPASS_BIAS_X, 4,
                               inv_int32_to_big8(biasB[0], reg));
        result =
            inv_set_mpu_memory(KEY_CPASS_BIAS_Y, 4,
                               inv_int32_to_big8(biasB[1], reg));
        result =
            inv_set_mpu_memory(KEY_CPASS_BIAS_Z, 4,
                               inv_int32_to_big8(biasB[2], reg));
    }
    return result;
}

/**
 *  @brief  Write a single register on the compass slave device, regardless
 *          of the bus it is connected to and the MPU's configuration.
 *  @param  reg
 *              the register to write to on the slave compass device.
 *  @param  val
 *              the value to write.
 *  @return INV_SUCCESS = 0 if successful. A non-zero error code otherwise.
 */
inv_error_t inv_compass_write_reg(unsigned char reg, unsigned char val)
{
    struct ext_slave_config config;
    unsigned char data[2];
    inv_error_t result;

    data[0] = reg;
    data[1] = val;

    config.key = MPU_SLAVE_WRITE_REGISTERS;
    config.len = 2;
    config.apply = true;
    config.data = data;

    result = inv_mpu_config_compass(inv_get_dl_config(),
                                    inv_get_serial_handle(),
                                    inv_get_serial_handle(), &config);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 *  @brief  Read values from the compass slave device registers, regardless
 *          of the bus it is connected to and the MPU's configuration.
 *  @param  reg
 *              the register to read from on the slave compass device.
 *  @param  val
 *              a buffer of 3 elements to store the values read from the 
 *              compass device.
 *  @return INV_SUCCESS = 0 if successful. A non-zero error code otherwise.
 */
inv_error_t inv_compass_read_reg(unsigned char reg, unsigned char *val)
{
    struct ext_slave_config config;
    unsigned char data[2];
    inv_error_t result;

    data[0] = reg;

    config.key = MPU_SLAVE_READ_REGISTERS;
    config.len = 2;
    config.apply = true;
    config.data = data;

    result = inv_mpu_get_compass_config(inv_get_dl_config(),
                                        inv_get_serial_handle(),
                                        inv_get_serial_handle(), &config);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    *val = data[1];
    return result;
}

/**
 *  @brief  Read values from the compass slave device scale registers,
 *   regardless of the bus it is connected to and the MPU's configuration.
 *  @param  reg
 *              the register to read from on the slave compass device.
 *  @param  val
 *              a buffer of 3 elements to store the values read from the 
 *              compass device.
 *  @return INV_SUCCESS = 0 if successful. A non-zero error code otherwise.
 */
inv_error_t inv_compass_read_scale(long *val)
{
    struct ext_slave_config config;
    unsigned char data[3];
    inv_error_t result;

    config.key = MPU_SLAVE_READ_SCALE;
    config.len = 3;
    config.apply = true;
    config.data = data;

    result = inv_mpu_get_compass_config(inv_get_dl_config(),
                                        inv_get_serial_handle(),
                                        inv_get_serial_handle(), &config);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    val[0] = ((long)(data[0] - 128) << 22) + (1L << 30);
    val[1] = ((long)(data[1] - 128) << 22) + (1L << 30);
    val[2] = ((long)(data[2] - 128) << 22) + (1L << 30);
    return result;
}

/**
 * @}
 */
 
 
