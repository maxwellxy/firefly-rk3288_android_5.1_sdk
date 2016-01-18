/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: ml_invobj.h 5971 2011-08-30 22:38:06Z mcaramello $
 *
 *****************************************************************************/

#ifndef __INV_ML_INVOBJ_H__
#define __INV_ML_INVOBJ_H__

#include "ml.h"

/* This header file provides extern access to the iobj_ 
 * objects which are the pointees in inv_obj.
 * These objects are declared and statically allocated in ml_invobj.c.
 * Only use the extern access when required, i.e. for constant
 * initialization of store and load structures.
 */

extern struct inv_gyro_param iobj_gyro; 
extern struct inv_accel_param iobj_accel; 
extern struct inv_mag_data iobj_mag; 
extern struct inv_pressure_data iobj_pressure; 

#ifdef INV_FEATURE_GYROTC_LEGACY
extern struct inv_tempcomp_data iobj_gyro_tc; 
#endif

extern struct inv_litefusion_data iobj_lite_fusion; 

#ifdef INV_FEATURE_ADVFUSION
extern struct inv_advfusion_data iobj_adv_fusion; 
#endif

extern struct inv_cal_params iobj_calmat; 
extern struct inv_system_data iobj_sys; 

#endif // __INV_ML_INVOBJ_H__

