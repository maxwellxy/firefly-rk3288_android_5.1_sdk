/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: temp_comp.h 6132 2011-10-01 03:17:27Z mcaramello $
 *
 *****************************************************************************/

#ifndef __INV_TEMP_COMP_H__
#define __INV_TEMP_COMP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"

/* APIs */
inv_error_t inv_enable_temp_comp(void);
inv_error_t inv_disable_temp_comp(void);
inv_error_t inv_temp_comp_is_enabled(unsigned char *is_enabled);
/* Formerly declared in ml.h: */
inv_error_t inv_get_gyro_temp_slope(long *data);
inv_error_t inv_get_gyro_temp_slope_float(float *data);
inv_error_t inv_set_gyro_temp_slope(long *data);
inv_error_t inv_set_gyro_temp_slope_float(float *data);

/* Private APIs */
int   inv_temp_comp_has_slope(void);
int   inv_temp_comp_find_bin(float temp);
inv_error_t  inv_temp_comp_reset(unsigned char new_state);
float inv_get_calibration_temp_difference(void);

inv_error_t inv_set_dmp_slope(float slope_x, float slope_y, float slope_z);

#ifdef __cplusplus
}
#endif


#endif // __INV_TEMP_COMP_H__
