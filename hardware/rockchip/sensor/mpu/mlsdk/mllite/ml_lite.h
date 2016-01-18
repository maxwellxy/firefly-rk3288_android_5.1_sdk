/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: ml_lite.h 6027 2011-09-12 18:54:07Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup ML
 *  @{
 *      @file ml_lite.h
 *      @brief Motion Library functions available in lite fusion.
 */

#ifndef INV_ML_LITE_H
#define INV_ML_LITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"

    inv_error_t inv_get_gyro(long *data);
    inv_error_t inv_get_accel(long *data);
    inv_error_t inv_get_temperature(long *data);
    inv_error_t inv_get_temperature_raw(short *data);
    inv_error_t inv_get_rot_mat(long *data);
    inv_error_t inv_get_rot_vector(long *data);
    inv_error_t inv_get_quaternion(long *data);
    inv_error_t inv_get_linear_accel(long *data);
    inv_error_t inv_get_linear_accel_in_world(long *data);
    inv_error_t inv_get_gravity(long *data);
    inv_error_t inv_get_angular_velocity(long *data);
    inv_error_t inv_get_euler_angles(long *data);
    inv_error_t inv_get_euler_angles_x(long *data);
    inv_error_t inv_get_euler_angles_y(long *data);
    inv_error_t inv_get_euler_angles_z(long *data);
    inv_error_t inv_get_gyro_bias(long *data);
    inv_error_t inv_get_accel_bias(long *data);
    inv_error_t inv_get_mag_bias(long *data);
    inv_error_t inv_get_gyro_and_accel_sensor(long *data);
    inv_error_t inv_get_mag_raw_data(long *data);
    inv_error_t inv_get_magnetometer(long *data);
    inv_error_t inv_get_pressure(long *data);
    inv_error_t inv_get_heading(long *data);
    inv_error_t inv_get_gyro_cal_matrix(long *data);
    inv_error_t inv_get_accel_cal_matrix(long *data);
    inv_error_t inv_get_mag_cal_matrix(long *data);

    inv_error_t inv_get_gyro_float(float *data);
    inv_error_t inv_get_accel_float(float *data);
    inv_error_t inv_get_temperature_float(float *data);
    inv_error_t inv_get_rot_mat_float(float *data);
    inv_error_t inv_get_rot_vector_float(float *data);
    inv_error_t inv_get_quaternion_float(float *data);
    inv_error_t inv_get_linear_accel_float(float *data);
    inv_error_t inv_get_linear_accel_in_world_float(float *data);
    inv_error_t inv_get_gravity_float(float *data);
    inv_error_t inv_get_angular_velocity_float(float *data);
    inv_error_t inv_get_euler_angles_float(float *data);
    inv_error_t inv_get_euler_angles_x_float(float *data);
    inv_error_t inv_get_euler_angles_y_float(float *data);
    inv_error_t inv_get_euler_angles_z_float(float *data);
    inv_error_t inv_get_gyro_bias_float(float *data);
    inv_error_t inv_get_accel_bias_float(float *data);
    inv_error_t inv_get_mag_bias_float(float *data);
    inv_error_t inv_get_gyro_and_accel_sensor_float(float *data);
    inv_error_t inv_get_mag_raw_data_float(float *data);
    inv_error_t inv_get_magnetometer_float(float *data);
    inv_error_t inv_get_pressure_float(float *data);
    inv_error_t inv_get_heading_float(float *data);
    inv_error_t inv_get_gyro_cal_matrix_float(float *data);
    inv_error_t inv_get_accel_cal_matrix_float(float *data);
    inv_error_t inv_get_mag_cal_matrix_float(float *data);

    inv_error_t inv_set_gyro_bias(long *data);
    inv_error_t inv_set_accel_bias(long *data);
    inv_error_t inv_set_mag_bias(long *data);

    inv_error_t inv_set_gyro_bias_float(float *data);
    inv_error_t inv_set_accel_bias_float(float *data);
    inv_error_t inv_set_mag_bias_float(float *data);

#ifdef __cplusplus
}
#endif
#endif /* INV_ML_LITE_H */

