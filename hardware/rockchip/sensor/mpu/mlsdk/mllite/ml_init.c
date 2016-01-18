/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
#include <string.h> // memset

#include "ml.h"
#include "mlsupervisor.h" // For P_INIT, SF_STARTUPSETTLE, etc.
#include <string.h>

static void inv_init_gyro_obj( struct inv_gyro_param *iobj_gyro );
static void inv_init_accel_obj( struct inv_accel_param *iobj_accel );
static void inv_init_mag_obj( struct inv_mag_data *iobj_mag );
static void inv_init_pressure_obj( struct inv_pressure_data *iobj_pressure );
static void inv_init_lite_fusion_obj( struct inv_litefusion_data *iobj_lite_fusion );
static void inv_init_calmat_obj( struct inv_cal_params *iobj_calmat );
static void inv_init_sys_obj( struct inv_system_data *iobj_sys );
#ifdef INV_FEATURE_ADVFUSION
void inv_init_adv_fusion_obj(struct inv_advfusion_data *iobj_adv_fusion);
#endif
#ifdef INV_FEATURE_GYROTC
static void inv_init_gyro_tc_obj( struct inv_tempcomp_data *iobj_gyro_tc );
#endif

static void inv_init_gyro_obj( struct inv_gyro_param *iobj_gyro )
{
    memset(iobj_gyro, 0, sizeof(struct inv_gyro_param));
}

static void inv_init_accel_obj( struct inv_accel_param *iobj_accel )
{
    memset(iobj_accel, 0, sizeof(struct inv_accel_param));
}

static void inv_init_mag_obj( struct inv_mag_data *iobj_mag )
{
    memset(iobj_mag, 0, sizeof(struct inv_mag_data));
    iobj_mag->sens = 322122560L;  // Should only change when the sensor full-scale range (FSR) is changed.
    iobj_mag->asa[0] = (1L << 30);
    iobj_mag->asa[1] = (1L << 30);
    iobj_mag->asa[2] = (1L << 30);
}

static void inv_init_pressure_obj( struct inv_pressure_data *iobj_pressure )
{
    memset(iobj_pressure, 0, sizeof(struct inv_pressure_data));
}

#ifdef INV_FEATURE_GYROTC
static void inv_init_gyro_tc_obj( struct inv_tempcomp_data *iobj_gyro_tc )
{
    memset(iobj_gyro_tc, 0, sizeof(struct inv_tempcomp_data));
}
#endif

static void inv_init_lite_fusion_obj( struct inv_litefusion_data *iobj_lite_fusion )
{
    memset(iobj_lite_fusion, 0, sizeof(struct inv_litefusion_data));
    iobj_lite_fusion->motion_duration = 1536;
    iobj_lite_fusion->motion_state = INV_MOTION;

    inv_obj.lite_fusion->acc_state = SF_STARTUP_SETTLE;
    inv_obj.lite_fusion->got_no_motion_bias = 0;
    inv_obj.lite_fusion->accel_lpf_gain = 1073744L;
    inv_obj.lite_fusion->no_motion_accel_threshold = 7000000L;
    /* Set to the ceiling of the square root of no_motion_accel_threshold above */
    inv_obj.lite_fusion->no_motion_accel_sqrt_threshold = 2646;
}

#ifdef INV_FEATURE_ADVFUSION
/*static*/ void inv_init_adv_fusion_obj( struct inv_advfusion_data *iobj_adv_fusion )
{
    int ii;
    memset(iobj_adv_fusion, 0, sizeof(struct inv_advfusion_data));
    iobj_adv_fusion->compass_correction[0] = 1073741824L;
    iobj_adv_fusion->compass_disturb_correction[0] = 1073741824L;
    iobj_adv_fusion->compass_correction_offset[0] = 1073741824L;
    iobj_adv_fusion->compass_state = SF_UNCALIBRATED;
    iobj_adv_fusion->gyro_bias_err = 7*65536L;

    for (ii = 0; ii < 3; ii++) {
        inv_obj.adv_fusion->compass_scale[ii] = 65536L;
        inv_obj.adv_fusion->compass_bias_error[ii] = P_INIT;
    }
}
#endif

static void inv_init_calmat_obj( struct inv_cal_params *iobj_calmat )
{
    memset(iobj_calmat, 0, sizeof(struct inv_cal_params));
    iobj_calmat->compass[0] = 322122560L;
    iobj_calmat->compass[4] = 322122560L;
    iobj_calmat->compass[8] = 322122560L;
}

static void inv_init_sys_obj( struct inv_system_data *iobj_sys )
{
    memset(iobj_sys, 0, sizeof(struct inv_system_data));
}

void inv_init_ml(void)
{
    /** For each inv_obj memebr, macros enforce
	 *  the INV_FEATURE_ contracts. This should
	 *  ensure the passed pointers are not null.
	 */
	/* gyro is always non-null. */
    inv_init_gyro_obj( inv_obj.gyro );
	/* accel is always non-null. */
    inv_init_accel_obj( inv_obj.accel );
#ifdef INV_FEATURE_MAGNETOMETER
	inv_init_mag_obj( inv_obj.mag );
#endif
#ifdef INV_FEATURE_PRESSURE
	inv_init_pressure_obj( inv_obj.pressure );
#endif
#ifdef INV_FEATURE_GYROTC
    inv_init_gyro_tc_obj( inv_obj.gyro_tc );
#endif
	/* lite_fusion is always non-null. */
    inv_init_lite_fusion_obj( inv_obj.lite_fusion );
#ifdef INV_FEATURE_ADVFUSION
    inv_init_adv_fusion_obj( inv_obj.adv_fusion );
#endif
#ifdef INV_FEATURE_CALMATS
    inv_init_calmat_obj( inv_obj.calmat );
#endif
#ifdef INV_FEATURE_SYSSTRUCT
    inv_init_sys_obj( inv_obj.sys );
#endif
}






