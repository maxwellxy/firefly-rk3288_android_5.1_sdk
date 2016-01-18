/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: ml_invobj.c 5971 2011-08-30 22:38:06Z mcaramello $
 *
 *****************************************************************************/

#include "ml_invobj.h"
#include "ml.h"

struct inv_gyro_param iobj_gyro; 
struct inv_accel_param iobj_accel; 
struct inv_mag_data iobj_mag; 
struct inv_pressure_data iobj_pressure; 

#ifdef INV_FEATURE_GYROTC_LEGACY
struct inv_tempcomp_data iobj_gyro_tc; 
#endif

struct inv_litefusion_data iobj_lite_fusion; 

#ifdef INV_FEATURE_ADVFUSION
struct inv_advfusion_data iobj_adv_fusion; 
#endif

struct inv_cal_params iobj_calmat; 
struct inv_system_data iobj_sys; 

struct inv_obj_t inv_obj = {
 /* .gyro        = */   &iobj_gyro
 /* .accel       = */ , &iobj_accel
 /* .mag         = */ , &iobj_mag
 /* .pressure    = */ , &iobj_pressure

#ifdef INV_FEATURE_GYROTC_LEGACY
 /* .gyro_tc     = */ , &iobj_gyro_tc
#else
 /* .gyro_tc     = */ , NULL
#endif

 /* .lite_fusion = */ , &iobj_lite_fusion

#ifdef INV_FEATURE_ADVFUSION
 /* .adv_fusion  = */ , &iobj_adv_fusion
#else
 /* .adv_fusion  = */ , NULL
#endif

 /* .calmat      = */ , &iobj_calmat
 /* .sys         = */ , &iobj_sys
};


