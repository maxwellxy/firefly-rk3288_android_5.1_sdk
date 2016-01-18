/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: ml_mputest.h 6132 2011-10-01 03:17:27Z mcaramello $
 *
 *****************************************************************************/

#ifndef _INV_MPUTEST_H_
#define _INV_MPUTEST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "ml_mputest_legacy.h"

/* user APIs */
inv_error_t inv_self_test_set_accel_z_orient(signed char z_sign);

inv_error_t inv_self_test_run(void);
inv_error_t inv_self_test_calibration_run(void);
inv_error_t inv_self_test_bias_run(void);
inv_error_t inv_self_test_accel_z_run(void);

/* other functions */
#define inv_set_self_test_parameters inv_set_test_parameters

#ifdef __cplusplus
}
#endif

#endif /* _INV_MPUTEST_H_ */

