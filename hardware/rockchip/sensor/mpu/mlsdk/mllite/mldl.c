/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: mldl.c 6075 2011-09-23 03:59:04Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup   MLDL
 *  @brief      Motion Library - Driver Layer.
 *              The Motion Library Driver Layer provides the intrface to the
 *              system drivers that are used by the Motion Library.
 *
 *  @{
 *      @file   mldl.c
 *      @brief  The Motion Library Driver Layer.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <string.h>

#include "mpu.h"
#include "mpu6050b1.h"
#include "mldl.h"
#include "mldl_cfg.h"
#include "compass.h"
#include "mlsl.h"
#include "mlos.h"
#include "mlinclude.h"
#include "ml.h"
#include "dmpKey.h"
#include "mlFIFOHW.h"
#include "compass.h"
#include "pressure.h"
#include "mldl_cfg_init.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mldl"

#define _mldlDebug(x)           //{x}

/* --------------------- */
/* -    Variables.     - */
/* --------------------- */

#define MAX_LOAD_WRITE_SIZE (MPU_MEM_BANK_SIZE/2)   /* 128 */

static void *g_mlsl_handle;
int_fast8_t g_int_trigger[NUM_OF_INTSOURCES];
static struct mldl_cfg *g_mldl_cfg;

/*******************************************************************************
 * Functions for accessing the DMP memory via keys
 ******************************************************************************/

unsigned short (*p_get_dmp_address) (unsigned short key) = NULL;
struct mpu_ram g_original_ram = {0, 0};

/**
 *  @internal
 *  @brief Sets the function to use to convert keys to addresses. This
 *         will changed for each DMP code loaded.
 *  @param func
 *              Function used to convert keys to addresses.
 *  @endif
 */
void inv_set_get_address(unsigned short (*func) (unsigned short key))
{
    INVENSENSE_FUNC_START;
    MPL_LOGV("%s %d", __func__, (int)func);
    p_get_dmp_address = func;
}

/**
 *  @internal
 *  @brief  Check if the feature is supported in the currently loaded
 *          DMP code basing on the fact that the key is assigned a
 *          value or not.
 *  @param  key     the DMP key
 *  @return whether the feature associated with the key is supported
 *          or not.
 */
uint_fast8_t inv_dmpkey_supported(unsigned short key)
{
    unsigned short memAddr;

    if (p_get_dmp_address == NULL) {
        MPL_LOGE("%s : p_get_dmp_address is NULL\n", __func__);
        return false;
    }

    memAddr = p_get_dmp_address(key);
    if (memAddr >= 0xffff) {
        MPL_LOGV("inv_set_mpu_memory unsupported key\n");
        return false;
    }

    return true;
}

/**
 *  @internal
 *  @brief  used to get the specified number of bytes from the original
 *          MPU memory location specified by the key.
 *          Reads the specified number of bytes from the MPU location
 *          that was used to program the MPU specified by the key. Each
 *          set of code specifies a function that changes keys into
 *          addresses. This function is set with setGetAddress().
 *
 *  @param  key     The key to use when looking up the address.
 *  @param  length  Number of bytes to read.
 *  @param  buffer  Result for data.
 *
 *  @return INV_SUCCESS if the command is successful, INV_ERROR otherwise.
 *          The key not corresponding to a memory address will result in
 *          INV_ERROR.
 *  @endif
 */
inv_error_t inv_get_mpu_memory_original(unsigned short key,
                                        unsigned short length,
                                        unsigned char *buffer)
{
    unsigned short offset;

    if (p_get_dmp_address == NULL) {
        return INV_ERROR_NOT_OPENED;
    }

    offset = p_get_dmp_address(key);
    if (offset >= g_original_ram.length ||
        (offset + length) > (g_original_ram.length)) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    memcpy(buffer, &g_original_ram.ram[offset], length);

    return INV_SUCCESS;
}

unsigned short inv_dl_get_address(unsigned short key)
{
    unsigned short offset;
    if (p_get_dmp_address == NULL) {
        return INV_ERROR_NOT_OPENED;
    }

    offset = p_get_dmp_address(key);
    return offset;
}

/* ---------------------- */
/* -  Static Functions. - */
/* ---------------------- */

/**
 *  @brief  Open the driver layer and resets the internal
 *          gyroscope, accelerometer, and compass data
 *          structures.
 *  @param  mlslHandle
 *              the serial handle.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_dl_open(void *mlsl_handle)
{
    inv_error_t result;
    memset(g_int_trigger, INT_CLEAR, sizeof(g_int_trigger));

    g_mlsl_handle = mlsl_handle;
    result = inv_mldl_cfg_init(&g_mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        inv_mldl_cfg_exit(&g_mldl_cfg);
        return result;
    }

    result = (inv_error_t) inv_mpu_open(
        g_mldl_cfg, g_mlsl_handle,
        g_mlsl_handle, g_mlsl_handle, g_mlsl_handle);
    if (result) {
        LOG_RESULT_LOCATION(result);
        inv_mldl_cfg_exit(&g_mldl_cfg);
    }

    return result;
}

/**
 *  @brief  Closes/Cleans up the ML Driver Layer.
 *          Put the device in sleep mode.
 *  @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_dl_close(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    result = (inv_error_t) inv_mpu_suspend(g_mldl_cfg,
                                           g_mlsl_handle,
                                           g_mlsl_handle,
                                           g_mlsl_handle,
                                           g_mlsl_handle,
                                           INV_ALL_SENSORS);

    result = (inv_error_t) inv_mpu_close(
        g_mldl_cfg, g_mlsl_handle,
        g_mlsl_handle, g_mlsl_handle, g_mlsl_handle);
    /* Clear all previous settings */
    inv_mldl_cfg_exit(&g_mldl_cfg);
    g_mlsl_handle = NULL;
    p_get_dmp_address = NULL;
    return result;
}

/**
 * @brief Sets the requested_sensors
 *
 * Accessor to set the requested_sensors field of the mldl_cfg structure.
 * Typically set at initialization.
 *
 * @param sensors
 * Bitfield of the sensors that are going to be used.  Combination of the
 * following:
 *  - INV_X_GYRO
 *  - INV_Y_GYRO
 *  - INV_Z_GYRO
 *  - INV_DMP_PROCESSOR
 *  - INV_X_ACCEL
 *  - INV_Y_ACCEL
 *  - INV_Z_ACCEL
 *  - INV_X_COMPASS
 *  - INV_Y_COMPASS
 *  - INV_Z_COMPASS
 *  - INV_X_PRESSURE
 *  - INV_Y_PRESSURE
 *  - INV_Z_PRESSURE
 *  - INV_THREE_AXIS_GYRO
 *  - INV_THREE_AXIS_ACCEL
 *  - INV_THREE_AXIS_COMPASS
 *  - INV_THREE_AXIS_PRESSURE
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_init_requested_sensors(unsigned long sensors)
{
    g_mldl_cfg->inv_mpu_cfg->requested_sensors = sensors;

    return INV_SUCCESS;
}

/**
 * @brief Starts the DMP running
 *
 * Resumes the sensor if any of the sensor axis or components are requested
 *
 * @param sensors
 * Bitfield of the sensors to turn on.  Combination of the following:
 *  - INV_X_GYRO
 *  - INV_Y_GYRO
 *  - INV_Z_GYRO
 *  - INV_DMP_PROCESSOR
 *  - INV_X_ACCEL
 *  - INV_Y_ACCEL
 *  - INV_Z_ACCEL
 *  - INV_X_COMPASS
 *  - INV_Y_COMPASS
 *  - INV_Z_COMPASS
 *  - INV_X_PRESSURE
 *  - INV_Y_PRESSURE
 *  - INV_Z_PRESSURE
 *  - INV_THREE_AXIS_GYRO
 *  - INV_THREE_AXIS_ACCEL
 *  - INV_THREE_AXIS_COMPASS
 *  - INV_THREE_AXIS_PRESSURE
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_dl_start(unsigned long sensors)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    g_mldl_cfg->inv_mpu_cfg->requested_sensors = sensors;
    result = inv_mpu_resume(g_mldl_cfg,
                            g_mlsl_handle,
                            g_mlsl_handle,
                            g_mlsl_handle,
                            g_mlsl_handle,
                            sensors);
    return result;
}

/**
 * @brief Stops the DMP running and puts it in low power as requested
 *
 * Suspends each sensor according to the bitfield, if all axis and components
 * of the sensor is off.
 *
 * @param sensors Bitfiled of the sensors to leave on.  Combination of the
 * following:
 *  - INV_X_GYRO
 *  - INV_Y_GYRO
 *  - INV_Z_GYRO
 *  - INV_X_ACCEL
 *  - INV_Y_ACCEL
 *  - INV_Z_ACCEL
 *  - INV_X_COMPASS
 *  - INV_Y_COMPASS
 *  - INV_Z_COMPASS
 *  - INV_X_PRESSURE
 *  - INV_Y_PRESSURE
 *  - INV_Z_PRESSURE
 *  - INV_THREE_AXIS_GYRO
 *  - INV_THREE_AXIS_ACCEL
 *  - INV_THREE_AXIS_COMPASS
 *  - INV_THREE_AXIS_PRESSURE
 *
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_dl_stop(unsigned long sensors)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    result = inv_mpu_suspend(g_mldl_cfg,
                             g_mlsl_handle,
                             g_mlsl_handle,
                             g_mlsl_handle,
                             g_mlsl_handle,
                             sensors);
    return result;
}

/**
 *  @brief  Get a pointer to the internal data structure
 *          storing the configuration for the MPU, the accelerometer
 *          and the compass in use.
 *  @return a pointer to the data structure of type 'struct mldl_cfg'.
 */
struct mldl_cfg *inv_get_dl_config(void)
{
    return g_mldl_cfg;
}

/**
 *  @brief   Query the MPU slave address.
 *  @return  The 7-bit mpu slave address.
 */
unsigned char inv_get_mpu_slave_addr(void)
{
    INVENSENSE_FUNC_START;
    return g_mldl_cfg->mpu_chip_info->addr;
}

/**
 *  @internal
 * @brief   MLDLCfgDMP configures the Digital Motion Processor internal to
 *          the MPU. The DMP can be enabled or disabled and the start address
 *          can be set.
 *
 * @param   enableRun   Enables the DMP processing if set to true.
 * @param   enableFIFO  Enables DMP output to the FIFO if set to true.
 * @param   startAddress start address
 *
 * @return  Zero if the command is successful, an error code otherwise.
*/
inv_error_t inv_get_dl_ctrl_dmp(unsigned char enableRun,
                                unsigned char enableFIFO)
{
    INVENSENSE_FUNC_START;

    g_mldl_cfg->mpu_gyro_cfg->dmp_enable = enableRun;
    g_mldl_cfg->mpu_gyro_cfg->fifo_enable = enableFIFO;
    g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;

    return INV_SUCCESS;
}

/**
 * @brief   inv_set_dl_cfg_int configures the interrupt function on the
 *          specified pin. The basic interrupt signal characteristics can be set
 *          (i.e. active high/low, open drain/push pull, etc.) and the
 *          triggers can be set.
 *          Currently only INTPIN_MPU is supported.
 *
 * @param   triggers
 *              bitmask of triggers to enable for interrupt.
 *              The available triggers are:
 *              - BIT_MPU_RDY_EN
 *              - BIT_DMP_INT_EN
 *              - BIT_RAW_RDY_EN
 *
 * @return  Zero if the command is successful, an error code otherwise.
*/
inv_error_t inv_set_dl_cfg_int(unsigned char triggers)
{
    inv_error_t result = INV_SUCCESS;


    g_mldl_cfg->mpu_gyro_cfg->int_config = triggers;
    if (!(g_mldl_cfg->inv_mpu_state->status & MPU_DEVICE_IS_SUSPENDED)) {
        result = inv_serial_single_write(
            g_mlsl_handle, g_mldl_cfg->mpu_chip_info->addr,
            MPUREG_INT_CFG,
            (g_mldl_cfg->mpu_gyro_cfg->int_config |
             g_mldl_cfg->pdata->int_config));
    } else {
        g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;
    }

    return result;
}

/**
 * @brief   configures the output sampling rate on the MPU.
 *          Three parameters control the sampling:
 *
 *          1) Low pass filter bandwidth, and
 *          2) output sampling divider.
 *
 *          The output sampling rate is determined by the divider and the low
 *          pass filter setting. If the low pass filter is set to
 *          'MPUFILTER_256HZ_NOLPF2', then the sample rate going into the
 *          divider is 8kHz; for all other settings it is 1kHz.
 *          The 8-bit divider will divide this frequency to get the resulting
 *          sample frequency.
 *          For example, if the filter setting is not 256Hz and the divider is
 *          set to 7, then the sample rate is as follows:
 *          sample rate = internal sample rate / div = 1kHz / 8 = 125Hz (or 8ms).
 *
 *          The low pass filter selection codes control both the cutoff frequency of
 *          the internal low pass filter and internal analog sampling rate. The
 *          latter, in turn, affects the final output sampling rate according to the
 *          sample rate divider settig.
 *              0 -> 256 Hz  cutoff BW, 8 kHz analog sample rate,
 *              1 -> 188 Hz  cutoff BW, 1 kHz analog sample rate,
 *              2 ->  98 Hz  cutoff BW, 1 kHz analog sample rate,
 *              3 ->  42 Hz  cutoff BW, 1 kHz analog sample rate,
 *              4 ->  20 Hz  cutoff BW, 1 kHz analog sample rate,
 *              5 ->  10 Hz  cutoff BW, 1 kHz analog sample rate,
 *              6 ->   5 Hz  cutoff BW, 1 kHz analog sample rate,
 *              7 -> 2.1 kHz cutoff BW, 8 kHz analog sample rate.
 *
 * @param   lpf         low pass filter,   0 to 7.
 * @param   divider     Output sampling rate divider, 0 to 255.
 *
 * @return  ML_SUCESS if successful; a non-zero error code otherwise.
**/
inv_error_t inv_dl_cfg_sampling(unsigned char lpf, unsigned char divider)
{
    /*---- do range checking ----*/
    if (lpf >= NUM_MPU_FILTER) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    g_mldl_cfg->mpu_gyro_cfg->lpf = lpf;
    g_mldl_cfg->mpu_gyro_cfg->divider = divider;
    g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;

    return INV_SUCCESS;
}

/**
 *  @brief  set the full scale range for the gyros.
 *          The full scale selection codes correspond to:
 *              0 -> 250  dps,
 *              1 -> 500  dps,
 *              2 -> 1000 dps,
 *              3 -> 2000 dps.
 *          Full scale range affect the MPU's measurement
 *          sensitivity.
 *
 *  @param  fullScale
 *              the gyro full scale range in dps.
 *
 *  @return INV_SUCCESS or non-zero error code.
**/
inv_error_t inv_set_full_scale(float fullScale)
{
    if (fullScale == 250.f)
        g_mldl_cfg->mpu_gyro_cfg->full_scale = MPU_FS_250DPS;
    else if (fullScale == 500.f)
        g_mldl_cfg->mpu_gyro_cfg->full_scale = MPU_FS_500DPS;
    else if (fullScale == 1000.f)
        g_mldl_cfg->mpu_gyro_cfg->full_scale = MPU_FS_1000DPS;
    else if (fullScale == 2000.f)
        g_mldl_cfg->mpu_gyro_cfg->full_scale = MPU_FS_2000DPS;
    else {                      // not a valid setting
        MPL_LOGE("Invalid full scale range specification for gyros : %f\n",
                 fullScale);
        MPL_LOGE
            ("\tAvailable values : +/- 250 dps, +/- 500 dps, +/- 1000 dps, +/- 2000 dps\n");
        return INV_ERROR_INVALID_PARAMETER;
    }
    g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;

    return INV_SUCCESS;
}

/**
 * @brief   This function sets the external sync for the MPU sampling.
 *          It can be synchronized on the LSB of any of the gyros, any of the
 *          external accels, or on the temp readings.
 *
 * @param   extSync External sync selection, 0 to 7.
 * @return  Zero if the command is successful; an error code otherwise.
**/
inv_error_t inv_set_external_sync(unsigned char extSync)
{
    INVENSENSE_FUNC_START;

    /*---- do range checking ----*/
    if (extSync >= NUM_MPU_EXT_SYNC) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    g_mldl_cfg->mpu_gyro_cfg->ext_sync = extSync;
    g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;

    return INV_SUCCESS;
}

inv_error_t inv_set_ignore_system_suspend(unsigned char ignore)
{
    INVENSENSE_FUNC_START;

    g_mldl_cfg->inv_mpu_cfg->ignore_system_suspend = ignore;

    return INV_SUCCESS;
}

/**
 * @brief   inv_clock_source function sets the clock source for the MPU gyro
 *          processing.
 *          The source can be any of the following:
 *          - Internal 8MHz oscillator,
 *          - PLL with X gyro as reference,
 *          - PLL with Y gyro as reference,
 *          - PLL with Z gyro as reference,
 *          - PLL with external 32.768Mhz reference, or
 *          - PLL with external 19.2MHz reference
 *
 *          For best accuracy and timing, it is highly recommended to use one
 *          of the gyros as the clock source; however this gyro must be
 *          enabled to use its clock (see 'MLDLPowerMgmtMPU()').
 *
 * @param   clkSource   Clock source selection.
 *                      Can be one of:
 *                      - CLK_INTERNAL,
 *                      - CLK_PLLGYROX,
 *                      - CLK_PLLGYROY,
 *                      - CLK_PLLGYROZ,
 *                      - CLK_PLLEXT32K, or
 *                      - CLK_PLLEXT19M.
 *
 * @return  Zero if the command is successful; an error code otherwise.
**/
inv_error_t inv_clock_source(unsigned char clkSource)
{
    INVENSENSE_FUNC_START;

    /*---- do range checking ----*/
    if (clkSource >= NUM_CLK_SEL) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    g_mldl_cfg->mpu_gyro_cfg->clk_src = clkSource;
    g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;

    return INV_SUCCESS;
}

/**
 *  @brief  Set the Temperature Compensation offset.
 *  @param  tc
 *              a pointer to the temperature compensations offset
 *              for the 3 gyro axes.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_set_offsetTC(const unsigned char *tc)
{
    int ii;
    inv_error_t result;

    for (ii = 0; ii < ARRAY_SIZE(g_mldl_cfg->mpu_offsets->tc); ii++) {
        g_mldl_cfg->mpu_offsets->tc[ii] = tc[ii];
    }

    if (!(g_mldl_cfg->inv_mpu_state->status & MPU_DEVICE_IS_SUSPENDED)) {
        unsigned char reg;
        result = inv_serial_single_write(
            g_mlsl_handle, g_mldl_cfg->mpu_chip_info->addr,
            MPUREG_XG_OFFS_TC,
            ((g_mldl_cfg->mpu_offsets->tc[0] << 1) & BITS_XG_OFFS_TC));
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        reg = ((g_mldl_cfg->mpu_offsets->tc[1] << 1) & BITS_YG_OFFS_TC);
        if (g_mldl_cfg->pdata->level_shifter)
            reg |= BIT_I2C_MST_VDDIO;
        result = inv_serial_single_write(
            g_mlsl_handle, g_mldl_cfg->mpu_chip_info->addr,
            MPUREG_YG_OFFS_TC, reg);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_serial_single_write(
            g_mlsl_handle, g_mldl_cfg->mpu_chip_info->addr,
            MPUREG_ZG_OFFS_TC,
            ((g_mldl_cfg->mpu_offsets->tc[2] << 1) & BITS_ZG_OFFS_TC));
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else {
        g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Set the gyro offset.
 *  @param  offset
 *              a pointer to the gyro offset for the 3 gyro axes. This is scaled
 *              as it would be written to the hardware registers.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_set_offset(const short *offset)
{
    inv_error_t result;
    unsigned char regs[7];
    int ii;
    long sf;

    MPL_LOGV("%s(%04x, %04x, %04x); from %04x, %04x %04x\n", __func__,
             offset[0], offset[1], offset[2],
             g_mldl_cfg->mpu_offsets->gyro[0],
             g_mldl_cfg->mpu_offsets->gyro[1],
             g_mldl_cfg->mpu_offsets->gyro[2]);

    sf = (2000L * 131) / g_mldl_cfg->mpu_chip_info->gyro_sens_trim;
    for (ii = 0; ii < ARRAY_SIZE(g_mldl_cfg->mpu_offsets->gyro); ii++) {
        // Record the bias in the units the register uses
        g_mldl_cfg->mpu_offsets->gyro[ii] = offset[ii];
        // Convert the bias to 1 dps = 1<<16
        inv_obj.gyro->bias[ii] = -offset[ii] * sf;
        regs[1 + ii * 2] = (unsigned char)(offset[ii] >> 8) & 0xff;
        regs[1 + ii * 2 + 1] = (unsigned char)(offset[ii] & 0xff);
    }

    if (!(g_mldl_cfg->inv_mpu_state->status & MPU_DEVICE_IS_SUSPENDED)) {
        regs[0] = MPUREG_X_OFFS_USRH;
        result = inv_serial_write(g_mlsl_handle,
                                  g_mldl_cfg->mpu_chip_info->addr, 7, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else {
        g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;
    }
    return INV_SUCCESS;
}

/**
 *  @internal
 *  @brief  used to get the specified number of bytes in the specified MPU
 *          memory bank.
 *          The memory bank is one of the following:
 *          - MPUMEM_RAM_BANK_0,
 *          - MPUMEM_RAM_BANK_1,
 *          - MPUMEM_RAM_BANK_2, or
 *          - MPUMEM_RAM_BANK_3.
 *
 *  @param  bank    Memory bank to write.
 *  @param  memAddr Starting address for write.
 *  @param  length  Number of bytes to write.
 *  @param  buffer  Result for data.
 *
 *  @return zero if the command is successful, an error code otherwise.
 *  @endif
 */
inv_error_t
inv_get_mpu_memory_one_bank(unsigned char bank,
                            unsigned char memAddr,
                            unsigned short length, unsigned char *buffer)
{
    inv_error_t result;

    if ((bank >= MPU_MEM_NUM_RAM_BANKS) ||
        //(memAddr >= MPU_MEM_BANK_SIZE) || always 0, memAddr is an u_char, therefore limited to 255
        ((memAddr + length) > MPU_MEM_BANK_SIZE) || (NULL == buffer)) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    if (g_mldl_cfg->inv_mpu_state->status & MPU_DEVICE_IS_SUSPENDED) {
        if (INV_CACHE_DMP == 0) {
            MPL_LOGE("INV_CACHE_DMP == 0:"
                     " tried to inv_get_mpu_memory while device suspended\n");
            result = INV_ERROR_SERIAL_WRITE;
        } else {
            memcpy(
                buffer,
                &g_mldl_cfg->mpu_ram->ram[bank * MPU_MEM_BANK_SIZE + memAddr],
                length);
            result = INV_SUCCESS;
        }
    } else {
        result = inv_serial_read_mem(
            g_mlsl_handle, g_mldl_cfg->mpu_chip_info->addr,
            ((bank << 8) | memAddr), length, buffer);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    return result;
}

/**
 *  @internal
 *  @brief  used to set the specified number of bytes in the specified MPU
 *          memory bank.
 *          The memory bank is one of the following:
 *          - MPUMEM_RAM_BANK_0,
 *          - MPUMEM_RAM_BANK_1,
 *          - MPUMEM_RAM_BANK_2, or
 *          - MPUMEM_RAM_BANK_3.
 *
 *  @param  bank    Memory bank to write.
 *  @param  memAddr Starting address for write.
 *  @param  length  Number of bytes to write.
 *  @param  buffer  Result for data.
 *
 *  @return zero if the command is successful, an error code otherwise.
 *  @endif
 */
inv_error_t inv_set_mpu_memory_one_bank(unsigned char bank,
                                        unsigned short memAddr,
                                        unsigned short length,
                                        const unsigned char *buffer)
{
    inv_error_t result = INV_SUCCESS;
    int different = 1;

    if ((bank >= MPU_MEM_NUM_RAM_BANKS) || (memAddr >= MPU_MEM_BANK_SIZE) ||
        ((memAddr + length) > MPU_MEM_BANK_SIZE) || (NULL == buffer)) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    if (INV_CACHE_DMP != 0) {
        different = memcmp(
            &g_mldl_cfg->mpu_ram->ram[bank * MPU_MEM_BANK_SIZE + memAddr],
            buffer, length);
        memcpy(&g_mldl_cfg->mpu_ram->ram[bank * MPU_MEM_BANK_SIZE + memAddr],
               buffer, length);
    }

    if (!(g_mldl_cfg->inv_mpu_state->status & MPU_DEVICE_IS_SUSPENDED)) {
        result = inv_serial_write_mem(
            g_mlsl_handle, g_mldl_cfg->mpu_chip_info->addr,
            ((bank << 8) | memAddr), length, buffer);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else if (different) {
        g_mldl_cfg->inv_mpu_state->status |= MPU_GYRO_NEEDS_CONFIG;
        if (INV_CACHE_DMP == 0) {
            MPL_LOGE("INV_CACHE_DMP == 0:"
                     " tried to inv_set_mpu_memory while device suspended\n");
            result = INV_ERROR_SERIAL_WRITE;
        }
    }

    return result;
}

/**
 *  @internal
 *  @brief  used to get the specified number of bytes from the MPU location
 *          specified by the key.
 *          Reads the specified number of bytes from the MPU location
 *          specified by the key. Each set of code specifies a function
 *          that changes keys into addresses. This function is set with
 *          setGetAddress().
 *
 *  @param  key     The key to use when looking up the address.
 *  @param  length  Number of bytes to read.
 *  @param  buffer  Result for data.
 *
 *  @return INV_SUCCESS if the command is successful, INV_ERROR otherwise. The key
 *          not corresponding to a memory address will result in INV_ERROR.
 *  @endif
 */
inv_error_t inv_get_mpu_memory(unsigned short key,
                               unsigned short length, unsigned char *buffer)
{
    unsigned char bank;
    inv_error_t result;
    unsigned short memAddr;

    if (p_get_dmp_address == NULL) {
        return INV_ERROR_NOT_OPENED;
    }

    memAddr = p_get_dmp_address(key);
    if (memAddr >= 0xffff)
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    bank = memAddr >> 8;        // Get Bank
    memAddr &= 0xff;

    while (memAddr + length > MPU_MEM_BANK_SIZE) {
        // We cross a bank in the middle
        unsigned short sub_length = MPU_MEM_BANK_SIZE - memAddr;
        result = inv_get_mpu_memory_one_bank(bank, (unsigned char)memAddr,
                                             sub_length, buffer);
        if (INV_SUCCESS != result)
            return result;
        bank++;
        length -= sub_length;
        buffer += sub_length;
        memAddr = 0;
    }
    result = inv_get_mpu_memory_one_bank(bank, (unsigned char)memAddr,
                                         length, buffer);

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 *  @internal
 *  @brief  used to set the specified number of bytes from the MPU location
 *          specified by the key.
 *          Set the specified number of bytes from the MPU location
 *          specified by the key. Each set of DMP code specifies a function
 *          that changes keys into addresses. This function is set with
 *          setGetAddress().
 *
 *  @param  key     The key to use when looking up the address.
 *  @param  length  Number of bytes to write.
 *  @param  buffer  Result for data.
 *
 *  @return INV_SUCCESS if the command is successful, INV_ERROR otherwise. The key
 *          not corresponding to a memory address will result in INV_ERROR.
 *  @endif
 */
inv_error_t inv_set_mpu_memory(unsigned short key,
                               unsigned short length,
                               const unsigned char *buffer)
{
    inv_error_t result = INV_SUCCESS;
    unsigned short memAddr;
    unsigned char bank;

    if (p_get_dmp_address == NULL) {
        MPL_LOGE("%s : p_get_dmp_address is NULL\n", __func__);
        return INV_ERROR_INVALID_MODULE;
    }
    memAddr = p_get_dmp_address(key);

    if (memAddr >= 0xffff) {
        MPL_LOGE("inv_set_mpu_memory unsupported key\n");
        return INV_ERROR_INVALID_MODULE; // This key not supported
    }

    bank = (unsigned char)(memAddr >> 8);
    memAddr &= 0xff;

    while (memAddr + length > MPU_MEM_BANK_SIZE) {
        // We cross a bank in the middle
        unsigned short sub_length = MPU_MEM_BANK_SIZE - memAddr;

        result = inv_set_mpu_memory_one_bank(bank, memAddr, sub_length, buffer);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        bank++;
        length -= sub_length;
        buffer += sub_length;
        memAddr = 0;
    }
    result = inv_set_mpu_memory_one_bank(bank, memAddr, length, buffer);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 *  @brief  Load the DMP with the given code and configuration.
 *  @param  buffer
 *              the DMP data.
 *  @param  length
 *              the length in bytes of the DMP data.
 *  @param  config
 *              the DMP configuration.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_dmp(const unsigned char *buffer,
                         unsigned short length, unsigned short config)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    g_original_ram.ram = (unsigned char *)buffer;
    g_original_ram.length = length;

    g_mldl_cfg->mpu_gyro_cfg->dmp_cfg1 = (config >> 8);
    g_mldl_cfg->mpu_gyro_cfg->dmp_cfg2 = (config & 0xff);

    result = inv_mpu_set_firmware(g_mldl_cfg, g_mlsl_handle, buffer, length);

    return result;
}

/**
 *  @brief  Get the silicon revision ID.
 *  @return The silicon revision ID
 *          (0 will be read if inv_mpu_open returned an error)
 */
unsigned char inv_get_silicon_rev(void)
{
    return g_mldl_cfg->mpu_chip_info->silicon_revision;
}

/**
 *  @brief  Get the product revision ID.
 *  @return The product revision ID
 *          (0 will be read if inv_mpu_open returned an error)
 */
unsigned char inv_get_product_rev(void)
{
    return g_mldl_cfg->mpu_chip_info->product_revision;
}

/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 * @todo these belong with an interface to the kernel driver layer
 *******************************************************************************
 *******************************************************************************
 ******************************************************************************/

/**
 * @brief   inv_get_interrupt_status returns the interrupt status from the specified
 *          interrupt pin.
 * @param   intPin
 *              Currently only the value INTPIN_MPU is supported.
 * @param   status
 *              The available statuses are:
 *              - BIT_MPU_RDY_EN
 *              - BIT_DMP_INT_EN
 *              - BIT_RAW_RDY_EN
 *
 * @return  INV_SUCCESS or a non-zero error code.
 */
inv_error_t inv_get_interrupt_status(unsigned char intPin,
                                     unsigned char *status)
{
    INVENSENSE_FUNC_START;

    inv_error_t result;

    switch (intPin) {

    case INTPIN_MPU:
            /*---- return the MPU interrupt status ----*/
        result = inv_serial_read(g_mlsl_handle, g_mldl_cfg->mpu_chip_info->addr,
                                 MPUREG_INT_STATUS, 1, status);
        break;

    default:
        result = INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return result;
}

/**
 *  @brief   query the current status of an interrupt source.
 *  @param   srcIndex
 *              index of the interrupt source.
 *              Currently the only source supported is INTPIN_MPU.
 *
 *  @return  1 if the interrupt has been triggered.
 */
unsigned char inv_get_interrupt_trigger(unsigned char srcIndex)
{
    INVENSENSE_FUNC_START;
    return g_int_trigger[srcIndex];
}

/**
 * @brief clear the 'triggered' status for an interrupt source.
 * @param srcIndex
 *          index of the interrupt source.
 *          Currently only INTPIN_MPU is supported.
 */
void inv_clear_interrupt_trigger(unsigned char srcIndex)
{
    INVENSENSE_FUNC_START;
    g_int_trigger[srcIndex] = 0;
}

/**
 * @brief   inv_interrupt_handler function should be called when an interrupt is
 *          received.  The source parameter identifies which interrupt source
 *          caused the interrupt. Note that this routine should not be called
 *          directly from the interrupt service routine.
 *
 * @param   intSource   MPU, AUX1, AUX2, or timer. Can be one of: INTSRC_MPU, INTSRC_AUX1,
 *                      INTSRC_AUX2, or INT_SRC_TIMER.
 *
 * @return  Zero if the command is successful; an error code otherwise.
 */
inv_error_t inv_interrupt_handler(unsigned char intSource)
{
    INVENSENSE_FUNC_START;
    /*---- range check ----*/
    if (intSource >= NUM_OF_INTSOURCES) {
        return INV_ERROR;
    }

    /*---- save source of interrupt ----*/
    g_int_trigger[intSource] = INT_TRIGGERED;

#ifdef ML_USE_DMP_SIM
    if (intSource == INTSRC_AUX1 || intSource == INTSRC_TIMER) {
        MLSimHWDataInput();
    }
#endif

    return INV_SUCCESS;
}

/***************************/
        /**@}*//* end of defgroup */
/***************************/
