/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: ml_adv.h 6075 2011-09-23 03:59:04Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup ML
 *  @{
 *      @file ml_adv.h
 *      @brief Motion Library functions available in advanced fusion.
**/

#ifndef INV_ML_ADV_H
#define INV_ML_ADV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"

    inv_error_t inv_get_mag_bias_error(long *data);
    inv_error_t inv_get_mag_scale(long *data);
    inv_error_t inv_get_local_field(long *data);

    inv_error_t inv_get_mag_bias_error_float(float *data);
    inv_error_t inv_get_mag_scale_float(float *data);
    inv_error_t inv_get_local_field_float(float *data);

    inv_error_t inv_get_compass_accuracy(int *accuracy);

    inv_error_t inv_set_local_field(long *data);
    inv_error_t inv_set_mag_scale(long *data);

    inv_error_t inv_set_local_field_float(float *data);
    inv_error_t inv_set_mag_scale_float(float *data);

#ifdef __cplusplus
}
#endif
#endif /* INV_ML_ADV_H */

