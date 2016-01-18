/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/**
 *  @addtogroup MLDL
 */

#include <string.h>
#include "mltypes.h"
#include "mpu.h"
#include "mldl_cfg.h"
#include "mlos.h"
#include "mldl_cfg_init.h"

/*---- structure containing control variables used by MLDL ----*/
struct mpu_ram                  g_mpu_ram;
struct mpu_gyro_cfg             g_mpu_gyro_cfg;
struct mpu_offsets              g_mpu_offsets;
struct mpu_chip_info            g_mpu_chip_info;
struct inv_mpu_cfg              g_inv_mpu_cfg;
struct inv_mpu_state            g_inv_mpu_state;

/* Slave related information */
struct ext_slave_descr          g_slave_accel;
struct ext_slave_descr          g_slave_compass;
struct ext_slave_descr          g_slave_pressure;
/* Platform Data */
struct mpu_platform_data        g_pdata;
struct ext_slave_platform_data  g_pdata_slave_accel;
struct ext_slave_platform_data  g_pdata_slave_compass;
struct ext_slave_platform_data  g_pdata_slave_pressure;

static struct mldl_cfg g_mldl_cfg;

/**
 *  @brief  Open the driver layer and resets the internal
 *          gyroscope, accelerometer, and compass data
 *          structures.
 *  @param  mlslHandle
 *              the serial handle.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_mldl_cfg_init(struct mldl_cfg **mldl_cfg)
{
    if (!mldl_cfg)
        return INV_ERROR_INVALID_PARAMETER;

    memset(&g_mldl_cfg, 0, sizeof(g_mldl_cfg));

    g_mldl_cfg.mpu_ram       = &g_mpu_ram;
    g_mldl_cfg.mpu_gyro_cfg  = &g_mpu_gyro_cfg;
    g_mldl_cfg.mpu_offsets   = &g_mpu_offsets;
    g_mldl_cfg.mpu_chip_info = &g_mpu_chip_info;
    g_mldl_cfg.inv_mpu_cfg   = &g_inv_mpu_cfg;
    g_mldl_cfg.inv_mpu_state = &g_inv_mpu_state;

    g_mldl_cfg.slave[EXT_SLAVE_TYPE_ACCEL]    = &g_slave_accel;
    g_mldl_cfg.slave[EXT_SLAVE_TYPE_COMPASS]  = &g_slave_compass;
    g_mldl_cfg.slave[EXT_SLAVE_TYPE_PRESSURE] = &g_slave_pressure;

    g_mldl_cfg.pdata = &g_pdata;

    g_mldl_cfg.pdata_slave[EXT_SLAVE_TYPE_ACCEL]    = &g_pdata_slave_accel;
    g_mldl_cfg.pdata_slave[EXT_SLAVE_TYPE_COMPASS]  = &g_pdata_slave_compass;
    g_mldl_cfg.pdata_slave[EXT_SLAVE_TYPE_PRESSURE] = &g_pdata_slave_pressure;

    /* default incase the driver doesn't set it */
    g_mldl_cfg.mpu_chip_info->addr  = 0x68;
    g_mldl_cfg.mpu_ram->length = MPU_MEM_NUM_RAM_BANKS * MPU_MEM_BANK_SIZE;
    g_mldl_cfg.mpu_ram->ram = kmalloc(g_mldl_cfg.mpu_ram->length, GFP_KERNEL);
    if (!g_mldl_cfg.mpu_ram->ram) {
        LOG_RESULT_LOCATION(INV_ERROR_MEMORY_EXAUSTED);
        return INV_ERROR_MEMORY_EXAUSTED;
    }

    *mldl_cfg = &g_mldl_cfg;
    return INV_SUCCESS;
}

inv_error_t inv_mldl_cfg_exit(struct mldl_cfg **mldl_cfg)
{
    kfree(g_mldl_cfg.mpu_ram->ram);
    *mldl_cfg = NULL;
    return INV_SUCCESS;
}
