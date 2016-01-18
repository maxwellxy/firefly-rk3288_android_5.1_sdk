/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: compass_supervisor.h 6271 2011-11-09 01:05:14Z kkeal $
 *
 ******************************************************************************/

#ifndef INVENSENSE_INV_COMPASS_SUPERVISOR_H__
#define INVENSENSE_INV_COMPASS_SUPERVISOR_H__

#include "mltypes.h"
#include "mlinclude.h"
#include "ml.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_COMPASS_RATE_PROCESSES 8

struct compass_obj_t {
    long raw[3];
    long calibrated[3];
    long bias[3];
    long init_bias[3]; /* Used to center compass data for extreme local body fields */
    unsigned long delta_time; /* Time in milliseconds from last measurement */
    int accuracy; /* Only used for when receiving compass externally */
};

#define INV_COMPASS_PRIORITY_RAW_DATA                   100
// This is the advanced compass bias algorithm
#define INV_COMPASS_ADV_BIAS_FUSION                     200
// This performs an initial 9-axis sensor fusion using gyro
#define INV_COMPASS_PRIORITY_HEADING_FROM_GYRO          400
// This computes a rough compass bias using gyros
#define INV_COMPASS_PRIORITY_COMPASS_FROM_GYRO          450
// This converts raw compass data into calibrated data
#define INV_COMPASS_PRIORITY_SET_CALIBRATION            500
// This computes a magnetic disturbance
#define INV_COMPASS_PRIORITY_MAGNETIC_DISTURBANCE       550
// This is used to call 3rd party compass libraries
#define INV_COMPASS_PRIORITY_EXTERNAL_CALIBRATION       600
// This is used to call the fast no motion algorithm
#define INV_COMPASS_PRIORITY_FAST_NO_MOTION             700
// This performs the 9-axis sensor fusion
#define INV_COMPASS_SET_9AXIS_QUATERNION_ADJUSTMENT     800

inv_error_t inv_enable_compass_supervisor(void);
inv_error_t inv_disable_compass_supervisor(void);
inv_error_t inv_run_compass_rate_processes(struct inv_obj_t *inv_obj);
inv_error_t inv_unregister_compass_rate_process(
                inv_error_t (*func)(struct compass_obj_t *obj));
inv_error_t inv_register_compass_rate_process(
                inv_error_t (*func)(struct compass_obj_t *obj), int priority);
inv_error_t inv_calibrate_compass(struct compass_obj_t *obj);
int inv_get_external_accuracy();
void inv_set_external_compass_data(long *compass, int accuracy);

#ifdef __cplusplus
}
#endif

#endif /* INVENSENSE_INV_COMPASS_SUPERVISOR_H__ */

