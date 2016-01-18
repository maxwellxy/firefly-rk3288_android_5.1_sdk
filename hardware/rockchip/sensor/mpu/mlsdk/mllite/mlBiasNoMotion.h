/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: mlBiasNoMotion.h 6132 2011-10-01 03:17:27Z mcaramello $
 *
 *****************************************************************************/

#ifndef INV_BIAS_NO_MOTION_H__
#define INV_BIAS_NO_MOTION_H__

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    inv_error_t inv_enable_bias_no_motion(void);
    inv_error_t inv_disable_bias_no_motion(void);
    inv_error_t inv_bias_nomot_is_enabled(unsigned char *is_enabled);

#ifdef __cplusplus
}
#endif
#endif                          // INV_BIAS_NO_MOTION_H__
