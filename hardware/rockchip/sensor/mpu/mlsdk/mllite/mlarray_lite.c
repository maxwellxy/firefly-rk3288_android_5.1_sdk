/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: mlarray_lite.c 6164 2011-10-06 18:04:23Z mcaramello $
 *
 ******************************************************************************/

/**
 *  @defgroup MLARRAY
 *  @{
 *      @file   mlarray_lite.c
 *      @brief  APIs to read different data sets from FIFO.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */
#include "ml.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "mlMathFunc.h"
#include "mlmath.h"
#include "mlstates.h"
#include "mlFIFO.h"
#include "mlsupervisor.h"
#include "mldl.h"
#include "dmpKey.h"
#include "compass.h"
#include "temp_comp.h"

/**
 *  @brief  inv_get_rot_mat is used to get the rotation matrix
 *          representation of the current sensor fusion solution.
 *          The array format will be R11, R12, R13, R21, R22, R23, R31, R32,
 *          R33, representing the matrix:
 *          <center>R11 R12 R13</center>
 *          <center>R21 R22 R23</center>
 *          <center>R31 R32 R33</center>
 *          Values are scaled, where 1.0 = 2^30 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_rot_mat(long *data)
{
    inv_error_t result = INV_SUCCESS;
    long qdata[4];
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    inv_get_quaternion(qdata);
    inv_quaternion_to_rotation(qdata, data);

    return result;
}


/**
 *  @brief  inv_get_rot_vector is used to get the rotation vector
 *          representation of the current sensor fusion solution.
 *          Values are scaled, where 1.0 = 2^30 LSBs.
 *          A rotation vector is a method to represent a 4-element quaternion
 *          vector in 3-elements. To get the quaternion from the 3-elements,
 *          The last 3-elements of the quaternion will be the given rotation
 *          vector. The first element of the quaternion will be the positive
 *          value that will be required to make the magnitude of the quaternion
 *          1.0 or 2^30 in fixed point units.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_rot_vector(long *data)
{
    inv_error_t result = INV_SUCCESS;
    long qdata[4];
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (!data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_quaternion(qdata);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    inv_quaternion_to_rotation_vector(qdata, data);

    return result;
}

/**
 *  @brief  inv_get_rot_vector is used to get the rotation vector
 *          representation of the current sensor fusion solution.
 *          Values are scaled, where 1.0 = 2^30 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_rot_vector_float(float *data)
{
    inv_error_t result = INV_SUCCESS;
    long ldata[3];
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (!data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_vector(ldata);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    data[0] = inv_q30_to_float(ldata[0]);
    data[1] = inv_q30_to_float(ldata[1]);
    data[2] = inv_q30_to_float(ldata[2]);
    return result;
}

/**
 *  @internal
 *  @brief  inv_get_angular_velocity is used to get an estimate of the body
 *          frame angular velocity, which is computed from the current and
 *          previous sensor fusion solutions.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_angular_velocity(long *data)
{

    return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    /* not implemented. old (invalid) implementation:
       if ( inv_get_state() < INV_STATE_DMP_OPENED )
       return INV_ERROR_SM_IMPROPER_STATE;

       if (NULL == data) {
       return INV_ERROR_INVALID_PARAMETER;
       }
       data[0] = inv_obj.ang_v_body[0];
       data[1] = inv_obj.ang_v_body[1];
       data[2] = inv_obj.ang_v_body[2];

       return result;
     */
}

/**
 *  @brief  inv_get_euler_angles is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles may follow various conventions. This function is equivelant
 *          to inv_get_euler_angles_x, refer to inv_get_euler_angles_x for more
 *          information.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_euler_angles(long *data)
{
    return inv_get_euler_angles_x(data);
}

/**
 *  @brief  inv_get_euler_angles_x is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles are returned according to the X convention.
 *          This is typically the convention used for mobile devices where the X
 *          axis is the width of the screen, Y axis is the height, and Z the
 *          depth. In this case roll is defined as the rotation around the X
 *          axis of the device.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>Y axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Z axis                </TD></TR>
 *          </TABLE>
 *
 *          Values are scaled where 1.0 = 2^16.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_euler_angles_x(long *data)
{
    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[6];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (long)((float)
               (atan2f(rotMatrix[7], rotMatrix[8]) * 57.29577951308) *
               65536L);
    data[1] = (long)((float)((double)asin(tmp) * 57.29577951308) * 65536L);
    data[2] =
        (long)((float)
               (atan2f(rotMatrix[3], rotMatrix[0]) * 57.29577951308) *
               65536L);
    return result;
}

/**
 *  @brief  inv_get_euler_angles_y is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles are returned according to the Y convention.
 *          This convention is typically used in augmented reality applications,
 *          where roll is defined as the rotation around the axis along the
 *          height of the screen of a mobile device, namely the Y axis.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>Y axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Z axis                </TD></TR>
 *          </TABLE>
 *
 *          Values are scaled where 1.0 = 2^16.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_euler_angles_y(long *data)
{
    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[7];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (long)((float)
               (atan2f(rotMatrix[8], rotMatrix[6]) * 57.29577951308f) *
               65536L);
    data[1] = (long)((float)((double)asin(tmp) * 57.29577951308) * 65536L);
    data[2] =
        (long)((float)
               (atan2f(rotMatrix[4], rotMatrix[1]) * 57.29577951308f) *
               65536L);
    return result;
}

/**  @brief  inv_get_euler_angles_z is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          This convention is mostly used in application involving the use
 *          of a camera, typically placed on the back of a mobile device, that
 *          is along the Z axis.  In this convention roll is defined as the
 *          rotation around the Z axis.
 *          Euler angles are returned according to the Y convention.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>Z axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Y axis                </TD></TR>
 *          </TABLE>
 *
 *          Values are scaled where 1.0 = 2^16.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */

inv_error_t inv_get_euler_angles_z(long *data)
{
    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[8];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (long)((float)
               (atan2f(rotMatrix[6], rotMatrix[7]) * 57.29577951308) *
               65536L);
    data[1] = (long)((float)((double)asin(tmp) * 57.29577951308) * 65536L);
    data[2] =
        (long)((float)
               (atan2f(rotMatrix[5], rotMatrix[2]) * 57.29577951308) *
               65536L);
    return result;
}

/**
 *  @brief  inv_get_gyro_bias is used to get the gyroscope biases.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled such that 1 dps = 2^16 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_gyro_bias(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    data[0] = inv_obj.gyro->bias[0];
    data[1] = inv_obj.gyro->bias[1];
    data[2] = inv_obj.gyro->bias[2];

    return result;
}

/**
 *  @brief  inv_get_accel_bias is used to get the accelerometer baises.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled such that 1 g (gravity) = 2^16 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_accel_bias(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    data[0] = inv_obj.accel->bias[0];
    data[1] = inv_obj.accel->bias[1];
    data[2] = inv_obj.accel->bias[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_bias is used to get Magnetometer Bias
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>. Compass bias in mounting frame.
 *              1uT = 2^16.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_bias(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.mag->bias[0];
    data[1] = inv_obj.mag->bias[1];
    data[2] = inv_obj.mag->bias[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_raw_data is used to get Raw magnetometer data.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_raw_data(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.mag->sensor_data[0];
    data[1] = inv_obj.mag->sensor_data[1];
    data[2] = inv_obj.mag->sensor_data[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_magnetometer is used to get magnetometer data.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_magnetometer(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.mag->calibrated_data[0];
    data[1] = inv_obj.mag->calibrated_data[1];
    data[2] = inv_obj.mag->calibrated_data[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_pressure is used to get Pressure data.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to data to be passed back to the user.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_pressure(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.pressure->meas;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_heading is used to get heading from Rotation Matrix.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to data to be passed back to the user.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */

inv_error_t inv_get_heading(long *data)
{
    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    result = inv_get_rot_mat_float(rotMatrix);
    if ((rotMatrix[7] < 0.707) && (rotMatrix[7] > -0.707)) {
        tmp =
            (float)(atan2f(rotMatrix[4], rotMatrix[1]) * 57.29577951308 -
                    90.0f);
    } else {
        tmp =
            (float)(atan2f(rotMatrix[5], rotMatrix[2]) * 57.29577951308 +
                    90.0f);
    }
    if (tmp < 0) {
        tmp += 360.0f;
    }
    data[0] = (long)((360 - tmp) * 65536.0f);

    return result;
}

/**
 *  @brief  inv_get_gyro_cal_matrix is used to get the gyroscope
 *          calibration matrix. The gyroscope calibration matrix defines the relationship
 *          between the gyroscope sensor axes and the sensor fusion solution axes.
 *          Calibration matrix data members will have a value of 1, 0, or -1.
 *          The matrix has members
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *          The argument array elements are ordered C11, C12, C13, C21, C22, C23, C31, C32, C33.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_gyro_cal_matrix(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.calmat->gyro[0];
    data[1] = inv_obj.calmat->gyro[1];
    data[2] = inv_obj.calmat->gyro[2];
    data[3] = inv_obj.calmat->gyro[3];
    data[4] = inv_obj.calmat->gyro[4];
    data[5] = inv_obj.calmat->gyro[5];
    data[6] = inv_obj.calmat->gyro[6];
    data[7] = inv_obj.calmat->gyro[7];
    data[8] = inv_obj.calmat->gyro[8];

    return result;
}

/**
 *  @brief  inv_get_accel_cal_matrix is used to get the accelerometer
 *          calibration matrix.
 *          Calibration matrix data members will have a value of 1, 0, or -1.
 *          The matrix has members
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *          The argument array elements are ordered C11, C12, C13, C21, C22, C23, C31, C32, C33.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_accel_cal_matrix(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.calmat->accel[0];
    data[1] = inv_obj.calmat->accel[1];
    data[2] = inv_obj.calmat->accel[2];
    data[3] = inv_obj.calmat->accel[3];
    data[4] = inv_obj.calmat->accel[4];
    data[5] = inv_obj.calmat->accel[5];
    data[6] = inv_obj.calmat->accel[6];
    data[7] = inv_obj.calmat->accel[7];
    data[8] = inv_obj.calmat->accel[8];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_cal_matrix is used to get magnetometer calibration matrix.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_cal_matrix(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.calmat->compass[0];
    data[1] = inv_obj.calmat->compass[1];
    data[2] = inv_obj.calmat->compass[2];
    data[3] = inv_obj.calmat->compass[3];
    data[4] = inv_obj.calmat->compass[4];
    data[5] = inv_obj.calmat->compass[5];
    data[6] = inv_obj.calmat->compass[6];
    data[7] = inv_obj.calmat->compass[7];
    data[8] = inv_obj.calmat->compass[8];

    return result;
}

/**
 *  @brief  inv_get_gyro_float is used to get the most recent gyroscope measurement.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of dps (degrees per second).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[3];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    result = inv_get_gyro(ldata);
    data[0] = (float)ldata[0] / 65536.0f;
    data[1] = (float)ldata[1] / 65536.0f;
    data[2] = (float)ldata[2] / 65536.0f;

    return result;
}

/**
 *  @internal
 *  @brief  inv_get_angular_velocity_float is used to get an array of three data points representing the angular
 *          velocity as derived from <b>both</b> gyroscopes and accelerometers.
 *          This requires that ML_SENSOR_FUSION be enabled, to fuse data from
 *          the gyroscope and accelerometer device, appropriately scaled and
 *          oriented according to the respective mounting matrices.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_angular_velocity_float(float *data)
{
    return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    /* not implemented. old (invalid) implementation:
       return inv_get_gyro_float(data);
     */
}

/**
 *  @brief  inv_get_temperature_float is used to get the most recent
 *          temperature measurement.
 *          The argument array should only have one element.
 *          The value is in units of deg C (degrees Celsius).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to data to be passed back to the user.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_temperature_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[1];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_temperature(ldata);
    data[0] = (float)ldata[0] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_rot_mat_float is used to get an array of nine data points representing the rotation
 *          matrix generated from all available sensors.
 *          The array format will be R11, R12, R13, R21, R22, R23, R31, R32,
 *          R33, representing the matrix:
 *          <center>R11 R12 R13</center>
 *          <center>R21 R22 R23</center>
 *          <center>R31 R32 R33</center>
 *          <b>Please refer to the "9-Axis Sensor Fusion Application Note" document,
 *          section 7 "Sensor Fusion Output", for details regarding rotation
 *          matrix output</b>.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long at least</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_rot_mat_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    {
        long qdata[4], rdata[9];
        inv_get_quaternion(qdata);
        inv_quaternion_to_rotation(qdata, rdata);
        data[0] = (float)rdata[0] / 1073741824.0f;
        data[1] = (float)rdata[1] / 1073741824.0f;
        data[2] = (float)rdata[2] / 1073741824.0f;
        data[3] = (float)rdata[3] / 1073741824.0f;
        data[4] = (float)rdata[4] / 1073741824.0f;
        data[5] = (float)rdata[5] / 1073741824.0f;
        data[6] = (float)rdata[6] / 1073741824.0f;
        data[7] = (float)rdata[7] / 1073741824.0f;
        data[8] = (float)rdata[8] / 1073741824.0f;
    }

    return result;
}

/**
 *  @brief  inv_get_quaternion_float is used to get the quaternion representation
 *          of the current sensor fusion solution.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 4 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an ML error code otherwise.
 */
 /* inv_get_quaternion_float implemented in mlFIFO.c */

/**
 *  @brief  inv_get_linear_accel_float is used to get an estimate of linear
 *          acceleration, based on the most recent accelerometer measurement
 *          and sensor fusion solution.
 *          The values are in units of g (gravity).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_linear_accel_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[3];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_linear_accel(ldata);
    data[0] = (float)ldata[0] / 65536.0f;
    data[1] = (float)ldata[1] / 65536.0f;
    data[2] = (float)ldata[2] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_linear_accel_in_world_float is used to get an estimate of
 *          linear acceleration, in the world frame,  based on the most
 *          recent accelerometer measurement and sensor fusion solution.
 *          The values are in units of g (gravity).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_linear_accel_in_world_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[3];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_linear_accel_in_world(ldata);
    data[0] = (float)ldata[0] / 65536.0f;
    data[1] = (float)ldata[1] / 65536.0f;
    data[2] = (float)ldata[2] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_gravity_float is used to get an estimate of the body frame
 *          gravity vector, based on the most recent sensor fusion solution.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long at least</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gravity_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[3];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    result = inv_get_gravity(ldata);
    data[0] = (float)ldata[0] / 65536.0f;
    data[1] = (float)ldata[1] / 65536.0f;
    data[2] = (float)ldata[2] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_gyro_cal_matrix_float is used to get the gyroscope
 *          calibration matrix. The gyroscope calibration matrix defines the relationship
 *          between the gyroscope sensor axes and the sensor fusion solution axes.
 *          Calibration matrix data members will have a value of 1.0, 0, or -1.0.
 *          The matrix has members
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *          The argument array elements are ordered C11, C12, C13, C21, C22, C23, C31, C32, C33.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_cal_matrix_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.calmat->gyro[0] / 1073741824.0f;
    data[1] = (float)inv_obj.calmat->gyro[1] / 1073741824.0f;
    data[2] = (float)inv_obj.calmat->gyro[2] / 1073741824.0f;
    data[3] = (float)inv_obj.calmat->gyro[3] / 1073741824.0f;
    data[4] = (float)inv_obj.calmat->gyro[4] / 1073741824.0f;
    data[5] = (float)inv_obj.calmat->gyro[5] / 1073741824.0f;
    data[6] = (float)inv_obj.calmat->gyro[6] / 1073741824.0f;
    data[7] = (float)inv_obj.calmat->gyro[7] / 1073741824.0f;
    data[8] = (float)inv_obj.calmat->gyro[8] / 1073741824.0f;

    return result;
}

/**
 *  @brief  inv_get_accel_cal_matrix_float is used to get the accelerometer
 *          calibration matrix.
 *          Calibration matrix data members will have a value of 1.0, 0, or -1.0.
 *          The matrix has members
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *          The argument array elements are ordered C11, C12, C13, C21, C22, C23, C31, C32, C33.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */

inv_error_t inv_get_accel_cal_matrix_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.calmat->accel[0] / 1073741824.0f;
    data[1] = (float)inv_obj.calmat->accel[1] / 1073741824.0f;
    data[2] = (float)inv_obj.calmat->accel[2] / 1073741824.0f;
    data[3] = (float)inv_obj.calmat->accel[3] / 1073741824.0f;
    data[4] = (float)inv_obj.calmat->accel[4] / 1073741824.0f;
    data[5] = (float)inv_obj.calmat->accel[5] / 1073741824.0f;
    data[6] = (float)inv_obj.calmat->accel[6] / 1073741824.0f;
    data[7] = (float)inv_obj.calmat->accel[7] / 1073741824.0f;
    data[8] = (float)inv_obj.calmat->accel[8] / 1073741824.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_cal_matrix_float is used to get an array of nine data points
 *			representing the calibration matrix for the compass:
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long at least</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_cal_matrix_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.calmat->compass[0] / 1073741824.0f;
    data[1] = (float)inv_obj.calmat->compass[1] / 1073741824.0f;
    data[2] = (float)inv_obj.calmat->compass[2] / 1073741824.0f;
    data[3] = (float)inv_obj.calmat->compass[3] / 1073741824.0f;
    data[4] = (float)inv_obj.calmat->compass[4] / 1073741824.0f;
    data[5] = (float)inv_obj.calmat->compass[5] / 1073741824.0f;
    data[6] = (float)inv_obj.calmat->compass[6] / 1073741824.0f;
    data[7] = (float)inv_obj.calmat->compass[7] / 1073741824.0f;
    data[8] = (float)inv_obj.calmat->compass[8] / 1073741824.0f;
    return result;
}

/**
 *  @brief  inv_get_gyro_bias_float is used to get the gyroscope biases.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of dps (degrees per second).

 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_bias_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.gyro->bias[0] / 65536.0f;
    data[1] = (float)inv_obj.gyro->bias[1] / 65536.0f;
    data[2] = (float)inv_obj.gyro->bias[2] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_accel_bias_float is used to get the accelerometer baises.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of g (gravity).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_accel_bias_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.accel->bias[0] / 65536.0f;
    data[1] = (float)inv_obj.accel->bias[1] / 65536.0f;
    data[2] = (float)inv_obj.accel->bias[2] / 65536.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_bias_float is used to get an array of three data points representing
 *			the compass biases.
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_bias_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = ((float)inv_obj.mag->bias[0]) / 65536.0f;
    data[1] = ((float)inv_obj.mag->bias[1]) / 65536.0f;
    data[2] = ((float)inv_obj.mag->bias[2]) / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_gyro_and_accel_sensor_float is used to get the most recent set of all sensor data.
 *          The argument array elements are ordered gyroscope X,Y, and Z,
 *          accelerometer X, Y, and Z, and magnetometer X,Y, and Z.
 *          \if UMPL The magnetometer elements are not populated in UMPL. \endif
 *          The gyroscope and accelerometer data is not scaled or offset, it is
 *          copied directly from the sensor registers, and cast as a float.
 *          In the case of accelerometers with 8-bit output resolution, the data
 *          is scaled up to match the 2^14 = 1 g typical represntation of +/- 2 g
 *          full scale range

 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *                     
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_and_accel_sensor_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[6];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_gyro_and_accel_sensor(ldata);
    data[0] = (float)ldata[0];
    data[1] = (float)ldata[1];
    data[2] = (float)ldata[2];
    data[3] = (float)ldata[3];
    data[4] = (float)ldata[4];
    data[5] = (float)ldata[5];
    data[6] = (float)inv_obj.mag->sensor_data[0];
    data[7] = (float)inv_obj.mag->sensor_data[1];
    data[8] = (float)inv_obj.mag->sensor_data[2];

    return result;
}

/**
 *  @brief  inv_get_euler_angles_x is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles are returned according to the X convention.
 *          This is typically the convention used for mobile devices where the X
 *          axis is the width of the screen, Y axis is the height, and Z the
 *          depth. In this case roll is defined as the rotation around the X
 *          axis of the device.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>Y axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Z axis                </TD></TR>
 *
           </TABLE>
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_euler_angles_x_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[6];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (float)(atan2f(rotMatrix[7],
                       rotMatrix[8]) * 57.29577951308);
    data[1] = (float)((double)asin(tmp) * 57.29577951308);
    data[2] =
        (float)(atan2f(rotMatrix[3], rotMatrix[0]) * 57.29577951308);

    return result;
}

/**
 *  @brief  inv_get_euler_angles_float is used to get an array of three data points three data points
 *			representing roll, pitch, and yaw corresponding to the INV_EULER_ANGLES_X output and it is
 *          therefore the default convention for Euler angles.
 *          Please refer to the INV_EULER_ANGLES_X for a detailed description.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_euler_angles_float(float *data)
{
    return inv_get_euler_angles_x_float(data);
}

/**  @brief  inv_get_euler_angles_y_float is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles are returned according to the Y convention.
 *          This convention is typically used in augmented reality applications,
 *          where roll is defined as the rotation around the axis along the
 *          height of the screen of a mobile device, namely the Y axis.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>Y axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Z axis                </TD></TR>
 *          </TABLE>
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_euler_angles_y_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[7];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (float)(atan2f(rotMatrix[8], rotMatrix[6]) * 57.29577951308);
    data[1] = (float)((double)asin(tmp) * 57.29577951308);
    data[2] =
        (float)(atan2f(rotMatrix[4], rotMatrix[1]) * 57.29577951308);

    return result;
}

/**  @brief  inv_get_euler_angles_z_float is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          This convention is mostly used in application involving the use
 *          of a camera, typically placed on the back of a mobile device, that
 *          is along the Z axis.  In this convention roll is defined as the
 *          rotation around the Z axis.
 *          Euler angles are returned according to the Y convention.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>Z axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Y axis                </TD></TR>
 *          </TABLE>
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_euler_angles_z_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[8];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (float)(atan2f(rotMatrix[6], rotMatrix[7]) * 57.29577951308);
    data[1] = (float)((double)asin(tmp) * 57.29577951308);
    data[2] =
        (float)(atan2f(rotMatrix[5], rotMatrix[2]) * 57.29577951308);

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_raw_data_float is used to get Raw magnetometer data
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_raw_data_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.mag->sensor_data[0];
    data[1] = (float)inv_obj.mag->sensor_data[1];
    data[2] = (float)inv_obj.mag->sensor_data[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_magnetometer_float is used to get magnetometer data
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_magnetometer_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.mag->calibrated_data[0] / 65536.0f;
    data[1] = (float)inv_obj.mag->calibrated_data[1] / 65536.0f;
    data[2] = (float)inv_obj.mag->calibrated_data[2] / 65536.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_pressure_float is used to get a single value representing the pressure in Pascal
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to the data to be passed back to the user.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_pressure_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.pressure->meas;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_heading_float is used to get single number representing the heading of the device
 *          relative to the Earth, in which 0 represents North, 90 degrees
 *          represents East, and so on.
 *          The heading is defined as the direction of the +Y axis if the Y
 *          axis is horizontal, and otherwise the direction of the -Z axis.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to the data to be passed back to the user.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_heading_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    inv_get_rot_mat_float(rotMatrix);
    if ((rotMatrix[7] < 0.707) && (rotMatrix[7] > -0.707)) {
        tmp =
            (float)(atan2f(rotMatrix[4], rotMatrix[1]) * 57.29577951308 -
                    90.0f);
    } else {
        tmp =
            (float)(atan2f(rotMatrix[5], rotMatrix[2]) * 57.29577951308 +
                    90.0f);
    }
    if (tmp < 0) {
        tmp += 360.0f;
    }
    data[0] = 360 - tmp;

    return result;
}

/**
 *  @brief  inv_set_gyro_bias is used to set the gyroscope bias.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled at 1 dps = 2^16 LSBs.
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_bias(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    long biasTmp;
    long sf = 0;
    short offset[GYRO_NUM_AXES];
    int i;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (mldl_cfg->mpu_chip_info->gyro_sens_trim != 0) {
        sf = 2000 * 131 / mldl_cfg->mpu_chip_info->gyro_sens_trim;
    } else {
        sf = 2000;
    }
    for (i = 0; i < GYRO_NUM_AXES; i++) {
        inv_obj.gyro->bias[i] = data[i];
        biasTmp = -inv_obj.gyro->bias[i] / sf;
        if (biasTmp < 0)
            biasTmp += 65536L;
        offset[i] = (short)biasTmp;
    }
    result = inv_set_offset(offset);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    // Turn off bias from Gravity once we have gyro biases as it is less useful
    result = inv_disable_bias_from_gravity();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_accel_bias is used to set the accelerometer bias.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled in units of g (gravity),
 *          where 1 g = 2^16 LSBs.
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_accel_bias(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    long biasTmp;
    int i, j;
    unsigned char regs[6];
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    for (i = 0; i < ACCEL_NUM_AXES; i++) {
        inv_obj.accel->bias[i] = data[i];
        if (inv_obj.accel->sens != 0 && mldl_cfg && mldl_cfg->pdata) {
            long long tmp64;
            inv_obj.accel->scaled_bias[i] = data[i];
			/*
            for (j = 0; j < ACCEL_NUM_AXES; j++) {
                inv_obj.accel->scaled_bias[i] +=
                    data[j] *
                    (long)mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]
                        ->orientation[i * 3 + j];
            }
            */
            tmp64 = (long long)inv_obj.accel->scaled_bias[i] << 13;
            biasTmp = (long)(tmp64 / inv_obj.accel->sens);
        } else {
            biasTmp = 0;
        }
        if (biasTmp < 0)
            biasTmp += 65536L;
        regs[2 * i + 0] = (unsigned char)(biasTmp / 256);
        regs[2 * i + 1] = (unsigned char)(biasTmp % 256);
    }
    result = inv_set_mpu_memory(KEY_D_1_8, 2, &regs[0]);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_mpu_memory(KEY_D_1_10, 2, &regs[2]);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_mpu_memory(KEY_D_1_2, 2, &regs[4]);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 *  @cond MPL
 *  @brief  inv_set_mag_bias is used to set Compass Bias
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data  Compass Bias in Compass Mounting Frame. 2^16 = 1uT
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_mag_bias(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    long data_raw[3];

    // convert to raw units sclaed by 2^16
    data_raw[0] = (long)(data[0] * (1LL<30) / inv_obj.mag->sens);
    data_raw[1] = (long)(data[1] * (1LL<30) / inv_obj.mag->sens);
    data_raw[2] = (long)(data[2] * (1LL<30) / inv_obj.mag->sens);
    inv_set_compass_bias(NULL, data_raw);
    if(IS_INV_ADVFEATURES_ENABLED(inv_obj)) {
        inv_obj.adv_fusion->got_compass_bias = 1;
        inv_obj.adv_fusion->got_init_compass_bias = 1;
        inv_obj.adv_fusion->compass_state = SF_STARTUP_SETTLE;
    }

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_gyro_bias_float is used to set the gyroscope bias.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of dps (degrees per second).
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_bias_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_gyro_bias(arrayTmp);

}

/**
 *  @brief  inv_set_accel_bias_float is used to set the accelerometer bias.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of g (gravity).
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_accel_bias_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_accel_bias(arrayTmp);

}

/**
 *  @cond MPL
 *  @brief  inv_set_mag_bias_float is used to set compass bias
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen()\ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data  Compass Bias in Compass Mounting Frame in units of uT.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_mag_bias_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_mag_bias(arrayTmp);
}

