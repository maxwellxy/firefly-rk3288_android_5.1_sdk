/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: compass_supervisor_adv_callbacks.c 5873 2011-08-11 03:13:48Z mcaramello $
 *
 ******************************************************************************/

#include "compass_supervisor_inv_obj_callbacks.h"

#include "ml.h"
#include "mltypes.h"
#include "mlMathFunc.h"
#include "mldl.h"
#include "compass_supervisor.h"

#ifndef INV_FEATURE_ADVFUSION
#error compass_supervisor_adv_callbacks should only be used with INV_FEATURE_ADVFUSION
#error Enable ADV_FUSION or use compass_supervisor_lite_callbacks instead
#endif

static inv_error_t adv_fusion_calibration_compass(struct compass_obj_t *);
static inv_error_t adv_fusion_raw_compass_data(struct compass_obj_t *);

inv_error_t inv_register_inv_obj_compass_supervisor_callbacks(void)
{
    inv_error_t result;

    result = inv_register_compass_rate_process(adv_fusion_raw_compass_data,
        INV_COMPASS_PRIORITY_RAW_DATA);
    if (result != INV_SUCCESS)
        return result;

    result = inv_register_compass_rate_process(adv_fusion_calibration_compass,
        INV_COMPASS_PRIORITY_SET_CALIBRATION);
    return result;
}

inv_error_t inv_unregister_inv_obj_compass_supervisor_callbacks(void)
{
    inv_error_t result;

    result = inv_unregister_compass_rate_process(adv_fusion_raw_compass_data);
    if (result != INV_SUCCESS)
        return result;

    result = inv_unregister_compass_rate_process(adv_fusion_calibration_compass);
    return result;
}

static inv_error_t adv_fusion_raw_compass_data(struct compass_obj_t *compass_obj)
{
    inv_obj.mag->sensor_data[0] = compass_obj->raw[0];
    inv_obj.mag->sensor_data[1] = compass_obj->raw[1];
    inv_obj.mag->sensor_data[2] = compass_obj->raw[2];
    return INV_SUCCESS;
}

/** Takes the raw compass data and removes the bias and applies the scaling to get
* calibrated compass data
*/
static inv_error_t adv_fusion_calibration_compass(struct compass_obj_t *obj)
{
    int i,j;
    long magdata;
    long tmp[3];
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    for (i = 0; i < 3; i++) {
        magdata = obj->raw[i];
        inv_obj.mag->sensor_data[i] = magdata;
        tmp[i] = (obj->raw[i] << 16) - obj->bias[i];
        tmp[i] = inv_q30_mult(tmp[i], inv_obj.mag->sens);
    }

    for (i = 0; i < 3; i++) {
        inv_obj.mag->calibrated_data[i] = 0;
        for (j = 0; j < 3; j++) {
            inv_obj.mag->calibrated_data[i] += tmp[j] * 
                mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS]
                        ->orientation[i * 3 + j];
        }
    }

    return INV_SUCCESS;
}
