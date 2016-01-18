/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id:$
 *
 ******************************************************************************/

#include "compass_supervisor_inv_obj_callbacks.h"
#include "compass_supervisor.h"

#include "ml.h"
#include "mldl.h"
#include "mlMathFunc.h"


#ifdef INV_FEATURE_ADV_FUSION
#error compass_supervisor_lite_callbacks should not be used with INV_FEATURE_ADV_FUSION
#error Disable ADV_FUSION or use compass_supervisor_adv_callbacks instead
#endif

static inv_error_t lite_fusion_calibrate_compass(struct compass_obj_t *);
static inv_error_t lite_fusion_raw_compass_data(struct compass_obj_t *);

inv_error_t inv_register_inv_obj_compass_supervisor_callbacks(void)
{
    inv_error_t result;
    result = inv_register_compass_rate_process(lite_fusion_raw_compass_data,
        INV_COMPASS_PRIORITY_RAW_DATA);
    if (result != INV_SUCCESS)
        return result;

    result = inv_register_compass_rate_process(lite_fusion_calibrate_compass,
        INV_COMPASS_PRIORITY_SET_CALIBRATION);
    return result;
}

inv_error_t inv_unregister_inv_obj_compass_supervisor_callbacks(void)
{
    inv_error_t result;

    result = inv_unregister_compass_rate_process(lite_fusion_raw_compass_data);
    if (result != INV_SUCCESS)
        return result;

    result = inv_unregister_compass_rate_process(lite_fusion_calibrate_compass);
    return result;
}

static inv_error_t lite_fusion_raw_compass_data(struct compass_obj_t *compass_obj)
{
    inv_obj.mag->sensor_data[0] = compass_obj->raw[0];
    inv_obj.mag->sensor_data[1] = compass_obj->raw[1];
    inv_obj.mag->sensor_data[2] = compass_obj->raw[2];
    return INV_SUCCESS;
}

/** Takes the raw compass data and removes the bias and applies the scaling to get
* calibrated compass data
*/
static inv_error_t lite_fusion_calibrate_compass(struct compass_obj_t *obj)
{
    int i,j;
    long tmp[3];
    long magdata;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    for (i = 0; i < 3; i++) {
        magdata = obj->raw[i] - obj->init_bias[i];
        inv_obj.mag->sensor_data[i] = magdata;
        tmp[i] = inv_q_shift_mult(inv_obj.mag->sensor_data[i],
                                  inv_obj.mag->sens, 14) - inv_obj.mag->bias[i];
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

