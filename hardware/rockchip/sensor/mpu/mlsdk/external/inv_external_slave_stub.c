/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: $
 *
 *****************************************************************************/

#undef MPL_LOG_TAG
#define	MPL_LOG_TAG		"EXT"
#undef MPL_LOG_NDEBUG
#define MPL_LOG_NDEBUG 1

#include "mpu.h"
#include "mltypes.h"
#include "mldl_cfg.h"
#include "mldl.h"
#include "ml.h"
#include "dmpKey.h"

#include "log.h"
#include "compass.h"
#include "compass_supervisor.h"

/**
 * Initialization
 *
 * @todo implement any library initialization here.
 */
static inv_error_t library_init(void)
{
    int ii, jj;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    unsigned char compass_register = 0x00;
    unsigned char reg_value 0x00;
    /* The orientation of the compass if needed can be found here in a 3x3
     * matrix of signed char values of -1, 0, and 1.  Raw data is not
     * rotated, but calibrated data should be rotated */
    signed char * compass_orientation =
        mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS]->orientation;

    /* Direct reads and writes can be done here, using the following api's
     * but they are better done in the drivers .init() function */
    if ((result) = inv_compass_write_reg(compass_register, reg_value)) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    /* Example read register */
    if ((result) = inv_compass_read_reg(compass_register, &reg_value)) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /*
     * @todo add other library initialization here
     */

    /* if there are no errors return INV_SUCCESS */
    return INV_SUCCESS;
}

/**
 * exit
 *
 * @todo implement any library shutdown and storage here.
 */
static inv_error_t library_exit(void)
{
    return INV_SUCCESS;
}

/* @todo implement and call a function similar to this */
extern inv_error_t your_compass_cal_function(const long *raw,
                                             long delta_time_ms,
                                             long *calibrated_data,
                                             long *micro_bias,
                                             long *macro_bias);

static inv_error_t inv_external_slave_process_sensor_data(
    struct compass_obj_t *obj)
{
    /* @todo, call appropriate library functions to transform raw data and
     * fill out the rest of the compass_obj structure */
    result = your_compass_cal_function(obj->raw,        /* in */
                                       obj->delta_time, /* in */
                                       obj->calibrated, /* out */
                                       obj->bias,       /* out */
                                       obj->init_bias   /* out */
        );
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    
    return INV_SUCCESS;
}

/** 
 * Open the compass library as a slave to the MPL library
 *
 * Initializes the library and registers a callback funtion to receive and
 * process raw data.
 *
 * @todo Rename this function replacing stub with the compass part number,
 * I.E. inv_external_slave_mpu3050_close
 *
 * @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_external_slave_stub_open(void)
{
    inv_error_t result;

    /* basic initialization of your library */
    result = library_init();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* Register to have the following function called every time new compass
     * raw data is available. */
    result = inv_register_compass_rate_process(
        inv_external_slave_process_sensor_data,
        INV_COMPASS_PRIORITY_EXTERNAL_CALIBRATION);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    
    /* Tell the library that this library will be providing calibrated compass
     * data */
    inv_obj.adv_fusion->external_slave_library = true;
    /* Return INV_SUCCESS if there are no errors */
    return INV_SUCCESS;
}

/** 
 * Close the previously opened slave compass library
 * 
 * Unregisters the callback function previously registered and causes the
 * library to exit
 *
 * @todo Rename this function replacing stub with the compass part number,
 * I.E. inv_external_slave_mpu3050_close
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_external_slave_stub_close(void)
{
    inv_error_t result;

    /* Unregister the previously registered raw data handler */
    result = inv_unregister_compass_rate_process(
        inv_external_slave_process_sensor_data);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = library_exit();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* tell the library that we are no longer providing calibrated compass
     * data */
    inv_obj.adv_fusion->external_slave_library = false;

    /* Return INV_SUCCESS to indicate that there were no errors */
    return INV_SUCCESS;
}
