/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: mldl_cfg_mpu.c 6132 2011-10-01 03:17:27Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @addtogroup MLDL
 *
 *  @{
 *      @file   mldl_cfg_mpu.c
 *      @brief  The Motion Library Driver Layer.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <string.h>
#include <stddef.h>
#include "mldl_cfg.h"
#include "mlsl.h"
#include "mpu.h"
#include "mldl_print_cfg.h"

#ifdef LINUX
#include <sys/ioctl.h>
#include "errno.h"
#endif
#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mldl_cfg_mpu:"

/* --------------------- */
/* -    Variables.     - */
/* --------------------- */


/* ---------------------- */
/* -  Static Functions. - */
/* ---------------------- */

/******************************************************************************
 ******************************************************************************
 * Exported functions
 ******************************************************************************
 *****************************************************************************/

static int mldl_cfg_push_gyro(void *gyro_handle, __u32 key,
                              __u32 len, void *data)
{
    struct ext_slave_config config;
    config.key = key;
    config.len = len;
    config.apply = 0;
    config.data = data;
    if (ioctl((int)gyro_handle, MPU_CONFIG_GYRO, &config))
        return errno;
    return 0;
}

static int mldl_cfg_pull_gyro(void *gyro_handle, __u32 key,
                              __u32 len, void *data)
{
    struct ext_slave_config config;
	int ret=0;
    config.key = key;
    config.len = len;
    config.apply = 0;
    config.data = data;
    if(ioctl((int)gyro_handle, MPU_GET_CONFIG_GYRO, &config))
        return errno;
    return 0;
}

static int mldl_cfg_push_mpu_ram(void *gyro_handle,
                                 struct mpu_ram *mpu_ram)
{
    return mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_RAM, mpu_ram->length,
                              mpu_ram->ram);
}

static int mldl_cfg_push_mpu_gyro_cfg(void *gyro_handle,
                                      struct mpu_gyro_cfg *mpu_gyro_cfg)
{
    int result;
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_INT_CONFIG,
                                sizeof(mpu_gyro_cfg->int_config),
                                &mpu_gyro_cfg->int_config);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_EXT_SYNC,
                                sizeof(mpu_gyro_cfg->ext_sync),
                                &mpu_gyro_cfg->ext_sync);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_FULL_SCALE,
                                sizeof(mpu_gyro_cfg->full_scale),
                                &mpu_gyro_cfg->full_scale);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_LPF,
                                sizeof(mpu_gyro_cfg->lpf),
                                &mpu_gyro_cfg->lpf);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_CLK_SRC,
                                sizeof(mpu_gyro_cfg->clk_src),
                                &mpu_gyro_cfg->clk_src);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_DIVIDER,
                                sizeof(mpu_gyro_cfg->divider),
                                &mpu_gyro_cfg->divider);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_DMP_ENABLE,
                                sizeof(mpu_gyro_cfg->dmp_enable),
                                &mpu_gyro_cfg->dmp_enable);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_FIFO_ENABLE,
                                sizeof(mpu_gyro_cfg->fifo_enable),
                                &mpu_gyro_cfg->fifo_enable);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_DMP_CFG1,
                                sizeof(mpu_gyro_cfg->dmp_cfg1),
                                &mpu_gyro_cfg->dmp_cfg1);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_DMP_CFG2,
                                sizeof(mpu_gyro_cfg->dmp_cfg2),
                                &mpu_gyro_cfg->dmp_cfg2);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

static int mldl_cfg_push_mpu_offsets(void *gyro_handle,
                                     struct mpu_offsets *mpu_offsets)
{
    int result;
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_TC,
                                sizeof(mpu_offsets->tc),
                                mpu_offsets->tc);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_gyro(gyro_handle, MPU_SLAVE_GYRO,
                                sizeof(mpu_offsets->gyro),
                                mpu_offsets->gyro);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

static int mldl_cfg_push_inv_mpu_cfg(void *gyro_handle,
                                     struct inv_mpu_cfg *inv_mpu_cfg)
{
    int result;
    result = ioctl((int) gyro_handle, MPU_SET_IGNORE_SYSTEM_SUSPEND,
                   inv_mpu_cfg->ignore_system_suspend);
    if (result)
        result = errno;
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

static int mldl_cfg_push(void *gyro_handle,
                         struct mldl_cfg *mldl_cfg)
{
    int result;
    result = mldl_cfg_push_mpu_ram(gyro_handle, mldl_cfg->mpu_ram);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_mpu_gyro_cfg(gyro_handle, mldl_cfg->mpu_gyro_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_mpu_offsets(gyro_handle, mldl_cfg->mpu_offsets);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_push_inv_mpu_cfg(gyro_handle, mldl_cfg->inv_mpu_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

static int mldl_cfg_pull_mpu_ram(void *gyro_handle,
                                 struct mpu_ram *mpu_ram)
{
    return mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_RAM, mpu_ram->length,
                              mpu_ram->ram);
}

static int mldl_cfg_pull_mpu_gyro_cfg(void *gyro_handle,
                                      struct mpu_gyro_cfg *mpu_gyro_cfg)
{
    int result;
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_INT_CONFIG,
                                sizeof(mpu_gyro_cfg->int_config),
                                &mpu_gyro_cfg->int_config);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_EXT_SYNC,
                                sizeof(mpu_gyro_cfg->ext_sync),
                                &mpu_gyro_cfg->ext_sync);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_FULL_SCALE,
                                sizeof(mpu_gyro_cfg->full_scale),
                                &mpu_gyro_cfg->full_scale);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_LPF,
                                sizeof(mpu_gyro_cfg->lpf),
                                &mpu_gyro_cfg->lpf);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_CLK_SRC,
                                sizeof(mpu_gyro_cfg->clk_src),
                                &mpu_gyro_cfg->clk_src);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_DIVIDER,
                                sizeof(mpu_gyro_cfg->divider),
                                &mpu_gyro_cfg->divider);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_DMP_ENABLE,
                                sizeof(mpu_gyro_cfg->dmp_enable),
                                &mpu_gyro_cfg->dmp_enable);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_FIFO_ENABLE,
                                sizeof(mpu_gyro_cfg->fifo_enable),
                                &mpu_gyro_cfg->fifo_enable);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_DMP_CFG1,
                                sizeof(mpu_gyro_cfg->dmp_cfg1),
                                &mpu_gyro_cfg->dmp_cfg1);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_DMP_CFG2,
                                sizeof(mpu_gyro_cfg->dmp_cfg2),
                                &mpu_gyro_cfg->dmp_cfg2);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

static int mldl_cfg_pull_mpu_offsets(void *gyro_handle,
                                     struct mpu_offsets *mpu_offsets)
{
    int result;
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_TC,
                                sizeof(mpu_offsets->tc),
                                mpu_offsets->tc);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_GYRO,
                                sizeof(mpu_offsets->gyro),
                                mpu_offsets->gyro);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}
static int mldl_cfg_pull_mpu_chip_info(void *gyro_handle,
                                       struct mpu_chip_info *mpu_chip_info)
{
    int result;
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_ADDR,
                                sizeof(mpu_chip_info->addr),
                                &mpu_chip_info->addr);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_PRODUCT_REVISION,
                                sizeof(mpu_chip_info->product_revision),
                                &mpu_chip_info->product_revision);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_SILICON_REVISION,
                                sizeof(mpu_chip_info->silicon_revision),
                                &mpu_chip_info->silicon_revision);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_PRODUCT_ID,
                                sizeof(mpu_chip_info->product_id),
                                &mpu_chip_info->product_id);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_GYRO_SENS_TRIM,
                                sizeof(mpu_chip_info->gyro_sens_trim),
                                &mpu_chip_info->gyro_sens_trim);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_gyro(gyro_handle, MPU_SLAVE_ACCEL_SENS_TRIM,
                                sizeof(mpu_chip_info->accel_sens_trim),
                                &mpu_chip_info->accel_sens_trim);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

static int mldl_cfg_pull_inv_mpu_cfg(void *gyro_handle,
                                     struct inv_mpu_cfg *inv_mpu_cfg)
{
    int result;
    result = ioctl((int) gyro_handle, MPU_GET_REQUESTED_SENSORS,
                   &inv_mpu_cfg->requested_sensors);
    if (result)
        result = errno;
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = ioctl((int) gyro_handle, MPU_GET_IGNORE_SYSTEM_SUSPEND,
                   &inv_mpu_cfg->ignore_system_suspend);
    if (result)
        result = errno;
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}
static int mldl_cfg_pull_inv_mpu_state(void *gyro_handle,
                                       struct inv_mpu_state *inv_mpu_state)
{
    int result;
    result = ioctl((int) gyro_handle, MPU_GET_MLDL_STATUS,
                   &inv_mpu_state->status);
    if (result)
        result = errno;
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = ioctl((int) gyro_handle, MPU_GET_I2C_SLAVES_ENABLED,
                   &inv_mpu_state->i2c_slaves_enabled);
    if (result)
        result = errno;
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}
static int mldl_cfg_pull_ext_slave_descr(void *gyro_handle,
                                         struct ext_slave_descr *slave)
{
    if (ioctl((int)gyro_handle, MPU_GET_EXT_SLAVE_DESCR, slave))
        return errno;
    return 0;
}
static int mldl_cfg_pull_mpu_platform_data(void *gyro_handle,
                                           struct mpu_platform_data *pdata)
{
    if (ioctl((int)gyro_handle, MPU_GET_MPU_PLATFORM_DATA, pdata))
        return errno;
    return 0;
}
static int mldl_cfg_pull_ext_slave_platform_data(
    void *gyro_handle,
    struct ext_slave_platform_data *pdata_slave)
{
    if (ioctl((int)gyro_handle,
              MPU_GET_EXT_SLAVE_PLATFORM_DATA, pdata_slave))
        return errno;
    return 0;
}

static int mldl_cfg_pull(void *gyro_handle,
                         struct mldl_cfg *mldl_cfg)
{
    int result;
    int ii;
    result = mldl_cfg_pull_mpu_ram(gyro_handle, mldl_cfg->mpu_ram);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_mpu_gyro_cfg(gyro_handle, mldl_cfg->mpu_gyro_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_mpu_offsets(gyro_handle, mldl_cfg->mpu_offsets);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_mpu_chip_info(gyro_handle, mldl_cfg->mpu_chip_info);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_inv_mpu_cfg(gyro_handle, mldl_cfg->inv_mpu_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull_inv_mpu_state(gyro_handle, mldl_cfg->inv_mpu_state);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    for (ii = 0; ii < EXT_SLAVE_NUM_TYPES; ii++) {
        if (!mldl_cfg->slave[ii])
            continue;
        mldl_cfg->slave[ii]->type = ii;
        result = mldl_cfg_pull_ext_slave_descr(gyro_handle,
                                               mldl_cfg->slave[ii]);
        if (result == INV_ERROR_INVALID_MODULE) {
            mldl_cfg->slave[ii] = NULL;
            result = 0;
        } else if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }
    result = mldl_cfg_pull_mpu_platform_data(gyro_handle, mldl_cfg->pdata);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    for (ii = 0; ii < EXT_SLAVE_NUM_TYPES; ii++) {
        if (!mldl_cfg->pdata_slave[ii])
            continue;
        mldl_cfg->pdata_slave[ii]->type = ii;
        result = mldl_cfg_pull_ext_slave_platform_data(
            gyro_handle,
            mldl_cfg->pdata_slave[ii]);
        if (result == INV_ERROR_INVALID_MODULE) {
            mldl_cfg->pdata_slave[ii] = NULL;
            result = 0;
        } else if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }
    return result;
}

/**
 * Initializes the pdata structure to defaults.
 *
 * Opens the device to read silicon revision, product id and whoami.  Leaves
 * the device in suspended state for low power.
 *
 * @param mldl_cfg handle to the config structure
 * @param mlsl_handle handle to the mpu serial layer
 * @param accel_handle handle to the accel serial layer
 * @param compass_handle handle to the compass serial layer
 * @param pressure_handle handle to the pressure serial layer
 *
 * @return INV_SUCCESS if silicon revision, product id and woami are supported
 *         by this software.
 */
int inv_mpu_open(struct mldl_cfg *mldl_cfg,
                 void *mlsl_handle,
                 void *accel_handle,
                 void *compass_handle,
                 void *pressure_handle)
{
    int result;

    result = mldl_cfg_pull(mlsl_handle, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = inv_mpu_suspend(mldl_cfg, mlsl_handle, NULL, NULL, NULL,
                             INV_ALL_SENSORS);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    mldl_print_cfg(mldl_cfg);

    return result;
}

/**
 * Stub for driver close.  Just verify that the devices are suspended
 *
 * @param mldl_cfg handle to the config structure
 * @param mlsl_handle handle to the mpu serial layer
 * @param accel_handle handle to the accel serial layer
 * @param compass_handle handle to the compass serial layer
 * @param pressure_handle handle to the compass serial layer
 *
 * @return INV_SUCCESS or non-zero error code
 */
int inv_mpu_close(struct mldl_cfg *mldl_cfg,
		  void *mlsl_handle,
		  void *accel_handle,
		  void *compass_handle,
		  void *pressure_handle)
{
    int result = INV_SUCCESS;

    result = inv_mpu_suspend(mldl_cfg, mlsl_handle, NULL, NULL, NULL,
                             INV_ALL_SENSORS);
    return result;
}

/**
 *  @brief  resume the MPU device and all the other sensor
 *          devices from their low power state.
 *
 *  @mldl_cfg
 *              pointer to the configuration structure
 *  @mlsl_handle
 *              the main file handle to the MPU device.
 *  @accel_handle
 *              an handle to the accelerometer device, if sitting
 *              onto a separate bus. Can match mlsl_handle if
 *              the accelerometer device operates on the same
 *              primary bus of MPU.
 *  @compass_handle
 *              an handle to the compass device, if sitting
 *              onto a separate bus. Can match mlsl_handle if
 *              the compass device operates on the same
 *              primary bus of MPU.
 *  @pressure_handle
 *              an handle to the pressure sensor device, if sitting
 *              onto a separate bus. Can match mlsl_handle if
 *              the pressure sensor device operates on the same
 *              primary bus of MPU.
 *  @sensors
 *              sensor enable mask requested.
 *
 *  @return  INV_SUCCESS or a non-zero error code.
 */
int inv_mpu_resume(struct mldl_cfg* mldl_cfg,
                   void *mlsl_handle,
                   void *accel_handle,
                   void *compass_handle,
                   void *pressure_handle,
                   unsigned long sensors)
{
    int result;

    result = mldl_cfg_push(mlsl_handle, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    mldl_print_cfg(mldl_cfg);

    result = ioctl((int)mlsl_handle, MPU_RESUME, sensors);
    if (result)
        result = errno;
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull(mlsl_handle, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    MPL_LOGI("%s(,,,,%04lx) -> %04lx\n", __func__,
             sensors,
             (unsigned long)mldl_cfg->inv_mpu_cfg->requested_sensors);

    return result;
}


int inv_mpu_suspend(struct mldl_cfg *mldl_cfg,
                    void *mlsl_handle,
                    void *accel_handle,
                    void *compass_handle,
                    void *pressure_handle,
                    unsigned long sensors)
{
    int result;

    result = mldl_cfg_push(mlsl_handle, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = ioctl((int)mlsl_handle, MPU_SUSPEND, sensors);
    if (result)
        result = errno;
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = mldl_cfg_pull(mlsl_handle, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    mldl_print_cfg(mldl_cfg);

    MPL_LOGI("%s(,,,,%04lx) -> %04lx\n", __func__,
             sensors,
             (unsigned long)mldl_cfg->inv_mpu_cfg->requested_sensors);
    return result;
}

/**
 * Sets the firmware cache
 *
 * @param mldl_cfg pointer to the configuration
 * @param mlsl_handle serial handle
 * @param data firmware
 * @param size sizeof the firmware
 *
 * @return INV_SUCCESS or non-zero error code
 */
int inv_mpu_set_firmware(struct mldl_cfg *mldl_cfg,
			 void *mlsl_handle,
			 const unsigned char *data,
			 int size)
{
#if INV_CACHE_DMP == 0
#error "mldl_cfg_mpu.c does not support INV_CACHE_DMP == 0"
#endif
    if (size > mldl_cfg->mpu_ram->length) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }
    memcpy(mldl_cfg->mpu_ram->ram, data, size);
    return INV_SUCCESS;
}

/**
 * Send slave configuration information
 *
 * @param mldl_cfg pointer to the mldl configuration structure
 * @param gyro_handle handle to the gyro sensor
 * @param slave_handle handle to the slave sensor
 * @param slave slave description
 * @param pdata slave platform data
 * @param data where to store the read data
 *
 * @return 0 or non-zero error code
 */
int inv_mpu_slave_read(struct mldl_cfg *mldl_cfg,
                       void *gyro_handle,
                       void *slave_handle,
                       struct ext_slave_descr *slave,
                       struct ext_slave_platform_data *pdata,
                       unsigned char *data)
{
    int result;
    if (!mldl_cfg || !gyro_handle || !data || !slave) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }

    switch (slave->type) {
    case EXT_SLAVE_TYPE_ACCEL:
        result = ioctl((int)gyro_handle, MPU_READ_ACCEL, data);
        if (result)
            result = errno;
        break;
    case EXT_SLAVE_TYPE_COMPASS:
        result = ioctl((int)gyro_handle, MPU_READ_COMPASS, data);
        if (result)
            result = errno;
        break;
    case EXT_SLAVE_TYPE_PRESSURE:
        result = ioctl((int)gyro_handle, MPU_READ_PRESSURE, data);
        if (result)
            result = errno;
        break;
    default:
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return result;
}

/**
 * Send slave configuration information
 *
 * @param mldl_cfg pointer to the mldl configuration structure
 * @param gyro_handle handle to the gyro sensor
 * @param slave_handle handle to the slave sensor
 * @param data the data being sent
 * @param slave slave description
 * @param pdata slave platform data
 *
 * @return 0 or non-zero error code
 */
int inv_mpu_slave_config(struct mldl_cfg *mldl_cfg,
                         void *gyro_handle,
                         void *slave_handle,
                         struct ext_slave_config *data,
                         struct ext_slave_descr *slave,
                         struct ext_slave_platform_data *pdata)
{
    int result;
    if (!mldl_cfg || !data || !slave || !pdata || !slave) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }

    switch (slave->type) {
    case EXT_SLAVE_TYPE_ACCEL:
        result = ioctl((int)gyro_handle, MPU_CONFIG_ACCEL, data);
        if (result)
            result = errno;
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case EXT_SLAVE_TYPE_COMPASS:
        result = ioctl((int)gyro_handle, MPU_CONFIG_COMPASS, data);
        if (result)
            result = errno;
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case EXT_SLAVE_TYPE_PRESSURE:
        result = ioctl((int)gyro_handle, MPU_CONFIG_PRESSURE, data);
        if (result)
            result = errno;
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    default:
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return result;
}

/**
 * Request slave configuration information
 *
 * Use this specifically after requesting a slave configuration to see what the
 * slave accually accepted.
 *
 * @param mldl_cfg pointer to the mldl configuration structure
 * @param gyro_handle handle to the gyro sensor
 * @param slave_handle handle to the slave sensor
 * @param data the data being requested.
 * @param slave slave description
 * @param pdata slave platform data
 *
 * @return 0 or non-zero error code
 */
int inv_mpu_get_slave_config(struct mldl_cfg *mldl_cfg,
                         void *gyro_handle,
                         void *slave_handle,
                         struct ext_slave_config *data,
                         struct ext_slave_descr *slave,
                         struct ext_slave_platform_data *pdata)
{
    int result;
    if (!mldl_cfg || !data || !slave) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }
    switch (slave->type) {
    case EXT_SLAVE_TYPE_ACCEL:
        result = ioctl((int)gyro_handle, MPU_GET_CONFIG_ACCEL, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case EXT_SLAVE_TYPE_COMPASS:
        result = ioctl((int)gyro_handle, MPU_GET_CONFIG_COMPASS, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case EXT_SLAVE_TYPE_PRESSURE:
        result = ioctl((int)gyro_handle, MPU_GET_CONFIG_PRESSURE, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    default:
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }
    return result;
}
/**
 *@}
 */
