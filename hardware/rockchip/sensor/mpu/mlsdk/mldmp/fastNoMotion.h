/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id$
 *
 *****************************************************************************/

#ifndef MLDMP_FAST_NO_MOTION_H__
#define MLDMP_FAST_NO_MOTION_H__

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    inv_error_t inv_enable_fast_nomot(void);
    inv_error_t inv_disable_fast_nomot(void);
    inv_error_t inv_fast_nomot_is_enabled(unsigned char *is_enabled);
    inv_error_t inv_update_fast_nomot(long *gyro);
    
    void inv_get_fast_nomot_accel_param(long *cntr, float *param);
    void inv_get_fast_nomot_compass_param(long *cntr, float *param);
    void inv_set_fast_nomot_accel_threshold(float thresh);
    void inv_set_fast_nomot_compass_threshold(float thresh);
    void int_set_fast_nomot_gyro_threshold(float thresh);
#ifdef __cplusplus
}
#endif


#endif // MLDMP_FAST_NO_MOTION_H__

