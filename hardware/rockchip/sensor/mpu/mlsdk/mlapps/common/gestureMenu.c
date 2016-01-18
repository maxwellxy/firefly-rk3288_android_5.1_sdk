/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 * $Id: gestureMenu.c 6205 2011-10-15 00:39:10Z mcaramello $ 
 *****************************************************************************/

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <string.h>

#include "ml.h"
#include "mlmath.h"
#include "gesture.h"
#include "orientation.h"
#include "gestureMenu.h"
#include "mlFIFO.h"

#undef MPL_LOG_TAG
#define MPL_LOG_TAG "gest"
#include "log.h"
#include "mldl_cfg.h"

static unsigned long sensors[] = {
    INV_NINE_AXIS,
    INV_THREE_AXIS_GYRO,
    INV_DMP_PROCESSOR | INV_THREE_AXIS_ACCEL,
    INV_THREE_AXIS_ACCEL,
    INV_DMP_PROCESSOR | INV_THREE_AXIS_COMPASS,
    INV_THREE_AXIS_COMPASS,
    INV_SIX_AXIS_GYRO_ACCEL,
    INV_DMP_PROCESSOR | INV_SIX_AXIS_ACCEL_COMPASS,
    INV_SIX_AXIS_ACCEL_COMPASS,
};

static char *sensors_string[] = {
    "INV_NINE_AXIS",
    "INV_THREE_AXIS_GYRO",
    "INV_DMP_PROCESSOR | INV_THREE_AXIS_ACCEL",
    "INV_THREE_AXIS_ACCEL",
    "INV_DMP_PROCESSOR | INV_THREE_AXIS_COMPASS",
    "INV_THREE_AXIS_COMPASS",
    "INV_SIX_AXIS_GYRO_ACCEL",
    "INV_DMP_PROCESSOR | INV_SIX_AXIS_ACCEL_COMPASS",
    "INV_SIX_AXIS_ACCEL_COMPASS",
};

/** 
 * Prints the menu with the current thresholds
 * 
 * @param params The parameters to print
 */
void PrintGestureMenu(tGestureMenuParams const * const params) 
{
    MPL_LOGI("Press h at any time to re-display this menu\n");
    MPL_LOGI("TAP PARAMETERS:\n");
    MPL_LOGI("    Use LEFT and RIGHT arrows to adjust Tap Time     \n\n");
    MPL_LOGI("    j          : Increase X threshold     : %5d\n",
             params->xTapThreshold);
    MPL_LOGI("    J (Shift-j): Decrease X threshold\n");
    MPL_LOGI("    k          : Increase Y threshold     : %5d\n",
             params->yTapThreshold);
    MPL_LOGI("    K (Shift-k): Decrease Y threshold\n");
    MPL_LOGI("    i          : Increase Z threshold     : %5d\n",
             params->zTapThreshold);
    MPL_LOGI("    I (Shift-i): Decrease Z threshold\n");
    MPL_LOGI("    l          : Increase tap time        : %5d\n",
             params->tapTime);
    MPL_LOGI("    L (Shift-l): Decrease tap time\n");
    MPL_LOGI("    o          : Increase next tap time   : %5d\n",
             params->nextTapTime);
    MPL_LOGI("    O (Shift-o): Increase next tap time\n");
    MPL_LOGI("    u          : Increase max Taps        : %5d\n",
             params->maxTaps);
    MPL_LOGI("    U (Shift-u): Increase max Taps\n");

    MPL_LOGI("SHAKE PARAMETERS:\n");
    MPL_LOGI("    x          : Increase X threshold     : %5d\n",
             params->xShakeThresh);
    MPL_LOGI("    X (Shift-x): Decrease X threshold\n");
    MPL_LOGI("    y          : Increase Y threshold     : %5d\n",
             params->yShakeThresh);
    MPL_LOGI("    Y (Shift-y): Decrease Y threshold\n");
    MPL_LOGI("    z          : Increase Z threshold     : %5d\n",
             params->zShakeThresh);
    MPL_LOGI("    Z (Shift-z): Decrease Z threshold\n");
    MPL_LOGI("    s          : Toggle Shake Function    : %5d\n",
             params->shakeFunction);
    MPL_LOGI("    t          : Increase Shake Time      : %5d\n",
             params->shakeTime);
    MPL_LOGI("    T (Shift-T): Decrease Shake Time\n");
    MPL_LOGI("    n          : Increase Next Shake Time : %5d\n",
             params->nextShakeTime);
    MPL_LOGI("    N (Shift-n): Decrease Next Shake Time\n");
    MPL_LOGI("    m          : Increase max Shakes      : %5d\n",
             params->maxShakes);
    MPL_LOGI("    M (Shift-m): Decrease max Shakes\n");
    MPL_LOGI("SNAP  PARAMETERS:\n");
    MPL_LOGI("    p          : Increase Pitch threshold : %5d\n",
             params->xSnapThresh);
    MPL_LOGI("    P (Shift-p): Decrease Pitch threshold\n");
    MPL_LOGI("    r          : Increase Roll  threshold : %5d\n",
             params->ySnapThresh);
    MPL_LOGI("    R (Shift-r): Decrease Roll  threshold\n");
    MPL_LOGI("    a          : Increase yAw   threshold : %5d\n",
             params->zSnapThresh);
    MPL_LOGI("    A (Shift-a): Decrease yAw   threshold\n");
    MPL_LOGI("YAW ROTATION PARAMETERS:\n");
    MPL_LOGI("    e          : Increase yaW Rotate time : %5d\n",
             params->yawRotateTime);
    MPL_LOGI("    E (Shift-r): Decrease yaW Rotate time\n");
    MPL_LOGI("    w          : Increase yaW Rotate threshold : %5d\n",
             params->yawRotateThreshold);
    MPL_LOGI("    W (Shift-w): Decrease yaW Rotate threshold\n");
    MPL_LOGI("ORIENTATION PARAMETER:\n");
    MPL_LOGI("    d          : Increase orientation angle threshold : %5f\n",
             params->orientationThreshold);
    MPL_LOGI("    D (Shift-d): Decrease orientation angle threshold\n");
    MPL_LOGI("FIFO RATE:\n");
    MPL_LOGI("    f          : Increase fifo divider    : %5d\n",
             inv_get_fifo_rate());
    MPL_LOGI("    F (Shift-f): Decrease fifo divider\n");
    MPL_LOGI("REQUESTED SENSORS:\n");
    MPL_LOGI("    S (Shift-s): Toggle in use sensors : %s\n",
             sensors_string[params->sensorsIndex]);
    MPL_LOGI("    F (Shift-f): Decrease fifo divider\n");

    /* V,v, B,b, Q,q, C,c, G,g, are available letters upper and lowercase */
    /* S is available */

    MPL_LOGI("\n\n");
}

/** 
 * Handles a keyboard input and updates an appropriate threshold, prints then
 * menu or returns false if the character is not processed.
 * 
 * @param params The parameters to modify if the thresholds are updated
 * @param ch     The input character
 * 
 * @return        true if the character was processed, false otherwise
 */
inv_error_t GestureMenuProcessChar(tGestureMenuParams * const params, char ch)
{
    int result = INV_SUCCESS;
    /* Dynamic keyboard processing */

    switch (ch) {
    case 'j':
        params->xTapThreshold += 20;
        // Intentionally fall through
    case 'J':  {
        params->xTapThreshold -= 10;
        if (params->xTapThreshold < 0) params->xTapThreshold = 0;
        result = inv_set_tap_threshold(INV_TAP_AXIS_X, params->xTapThreshold);
        if (INV_SUCCESS != result) {
            MPL_LOGE("MLSetTapThresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_tap_threshold(INV_TAP_AXIS_X, %d)\n",
                 params->xTapThreshold);
    } break;
    case 'k':
        params->yTapThreshold += 20;
        // Intentionally fall through
    case 'K':  {
        params->yTapThreshold -= 10;
        if (params->yTapThreshold < 0) params->yTapThreshold = 0;
        result = inv_set_tap_threshold(INV_TAP_AXIS_Y, params->yTapThreshold);
        if (INV_SUCCESS != result) {
            MPL_LOGE("MLSetTapThresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_tap_threshold(INV_TAP_AXIS_Y, %d)\n",
                 params->yTapThreshold);
    } break;
    case 'i':
        params->zTapThreshold += 20;
        // Intentionally fall through
    case 'I':  {
        params->zTapThreshold -= 10;
        if (params->zTapThreshold < 0) params->zTapThreshold = 0;
        result = inv_set_tap_threshold(INV_TAP_AXIS_Z, params->zTapThreshold);
        if (INV_SUCCESS != result) {
            MPL_LOGE("MLSetTapThresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_tap_threshold(INV_TAP_AXIS_Z, %d)\n",
                 params->zTapThreshold);
    } break;
        
    case  'l':
        params->tapTime += 20;
        // Intentionally fall through
    case 'L':  {
        params->tapTime -= 10;
        if (params->tapTime < 0) params->tapTime = 0;
        result = inv_set_next_tap_time(params->tapTime);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_next_tap_time returned :%d\n", result);
        }
        MPL_LOGI("inv_set_next_tap_time(%d)\n", params->tapTime);
    } break;
    case  'o':
        params->nextTapTime += 20;
        // Intentionally fall through
    case 'O':  {
        params->nextTapTime -= 10;
        if (params->nextTapTime < 0) params->nextTapTime = 0;
        result = MLSetNextTapTime(params->nextTapTime);
        if (INV_SUCCESS != result) {
            MPL_LOGE("MLSetNextTapTime returned :%d\n", result);
        }
        MPL_LOGI("MLSetNextTapTime(%d)\n", params->nextTapTime);
    } break;
    case  'u':
        params->maxTaps += 2;
        // Intentionally fall through
    case 'U':  {
        params->maxTaps -= 1;
        if (params->maxTaps < 0) params->maxTaps = 0;
        result = inv_set_max_taps(params->maxTaps);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_max_taps returned :%d\n", result);
        }
        MPL_LOGI("inv_set_max_taps(%d)\n", params->maxTaps);
    } break;
    case 's': {
        int shakeParam;
        params->shakeFunction = (params->shakeFunction + 1) % 2;
        switch (params->shakeFunction)
        {
        case 0:
            shakeParam = INV_NO_RETRACTION;
            MPL_LOGE("inv_set_shake_func(INV_NO_RETRACTION)\n");
            break;
        case 1:
            shakeParam = INV_RETRACTION;
            MPL_LOGI("inv_set_shake_func(INV_RETRACTION)\n");
            break;
        };
        result = inv_set_shake_func(shakeParam);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_shake_func returned :%d\n", result);
        }
    } break;
    case 'x': 
        params->xShakeThresh += 200;
        // Intentionally fall through
    case 'X': {
        params->xShakeThresh -= 100;
        result = inv_set_shake_thresh(INV_PITCH_SHAKE, params->xShakeThresh);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_shake_thresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_shake_thresh(INV_PITCH_SHAKE, %d)\n", params->xShakeThresh);
    } break;
    case 'y': 
        params->yShakeThresh += 200;
        // Intentionally fall through
    case 'Y': {
        params->yShakeThresh -= 100;
        result = inv_set_shake_thresh(INV_ROLL_SHAKE, params->yShakeThresh);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_shake_thresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_shake_thresh(INV_ROLL_SHAKE, %d)\n", params->yShakeThresh);
    } break;
    case 'z':
        params->zShakeThresh += 200;
        // Intentionally fall through
    case 'Z':{
        params->zShakeThresh -= 100;
        result = inv_set_shake_thresh(INV_YAW_SHAKE, params->zShakeThresh);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_shake_thresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_shake_thresh(INV_YAW_SHAKE, %d)\n",params->zShakeThresh);
    } break;
    case 'r':
        params->ySnapThresh += 20;
        // Intentionally fall through
    case 'R': {
        params->ySnapThresh -= 10;
        result = inv_set_hard_shake_thresh(INV_ROLL_SHAKE, params->ySnapThresh);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_hard_shake_thresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_hard_shake_thresh(INV_ROLL_SHAKE, %d)\n",params->ySnapThresh);
    } break;
    case 'p':
        params->xSnapThresh += 20;
        // Intentionally fall through
    case 'P': {
        params->xSnapThresh -= 10;
        result = inv_set_hard_shake_thresh(INV_PITCH_SHAKE, params->xSnapThresh);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_hard_shake_thresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_hard_shake_thresh(INV_PITCH_SHAKE, %d)\n",
                 params->xSnapThresh);
    } break;
    case 'a':
        params->zSnapThresh += 20;
    case 'A': {
        params->zSnapThresh -= 10;
        result = inv_set_hard_shake_thresh(INV_YAW_SHAKE, params->zSnapThresh);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_hard_shake_thresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_hard_shake_thresh(INV_YAW_SHAKE, %d)\n",params->zSnapThresh);
    } break;
        
    case 't':
        params->shakeTime += 20;
    case 'T':{
        params->shakeTime -= 10;
        result = inv_set_shake_time(params->shakeTime);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_shake_time returned :%d\n", result);
        }
        MPL_LOGI("inv_set_shake_time(%d)\n", params->shakeTime);
    } break;
    case 'n':
        params->nextShakeTime += 20;
    case 'N':{
        params->nextShakeTime -= 10;
        result = inv_set_next_shake_time(params->nextShakeTime);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_next_shake_time returned :%d\n", result);
        }
        MPL_LOGI("inv_set_next_shake_time(%d)\n", params->nextShakeTime);
    } break;
    case 'm':
        params->maxShakes += 2;
    case 'M':{
        params->maxShakes -= 1;
        result = inv_set_max_shakes(INV_SHAKE_ALL, params->maxShakes);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_max_shakes returned :%d\n", result);
        }
        MPL_LOGI("inv_set_max_shakes(%d)\n", params->maxShakes);
    } break;
    case 'e':
        params->yawRotateTime += 20;
    case 'E':{
        params->yawRotateTime -= 10;
        result = inv_set_yaw_rotate_time(params->yawRotateTime);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_yaw_rotate_time returned :%d\n", result);
        }
        MPL_LOGI("inv_set_yaw_rotate_time(%d)\n", params->yawRotateTime);
    } break;
    case 'w':
        params->yawRotateThreshold += 2;
    case 'W':{
        params->yawRotateThreshold -= 1;
        result = inv_set_yaw_rotate_thresh(params->yawRotateThreshold);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_yaw_rotate_thresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_yaw_rotate_thresh(%d)\n", params->yawRotateThreshold);
    } break;
    case 'c':
        params->shakeRejectValue += 0.20f;
    case 'C':{
        params->shakeRejectValue -= 0.10f;
        result = inv_set_tap_shake_reject(params->shakeRejectValue);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_tap_shake_reject returned :%d\n", result);
        }
        MPL_LOGI("inv_set_tap_shake_reject(%f)\n", params->shakeRejectValue);
    } break;
    case 'd':
        params->orientationThreshold += 10;
    case 'D':{
        params->orientationThreshold -= 5;
        if (params->orientationThreshold > 90) {
            params->orientationThreshold = 90;
        }

        if (params->orientationThreshold < 0 ) {
            params->orientationThreshold = 0;
        }
        
        result = inv_set_orientation_thresh(params->orientationThreshold, 
                                           5, 80, 
                                           INV_X_AXIS | INV_Y_AXIS | INV_Z_AXIS);
        if (INV_SUCCESS != result) {
            MPL_LOGE("inv_set_orientation_thresh returned :%d\n", result);
        }
        MPL_LOGI("inv_set_orientation_thresh(%f, %d, %d,"
                 " INV_X_AXIS | INV_Y_AXIS | INV_Z_AXIS)\n",
                 params->orientationThreshold, 5, 80);
    } break;
    case 'f':
        result = inv_set_fifo_rate(inv_get_fifo_rate() + 1);
        MPL_LOGI("inv_set_fifo_rate(%d)\n",inv_get_fifo_rate());
        break;
    case 'F':
    {
        unsigned short newRate = inv_get_fifo_rate();
        if (newRate > 0)
            newRate--;
        result = inv_set_fifo_rate(newRate);
        MPL_LOGI("inv_set_fifo_rate(%d)\n",inv_get_fifo_rate());
        break;
    }
    case 'S':
        params->sensorsIndex++;
        if (params->sensorsIndex >= ARRAY_SIZE(sensors)) {
            params->sensorsIndex = 0;
        }
        result = inv_set_mpu_sensors(
            sensors[params->sensorsIndex] & params->available_sensors);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        MPL_LOGI("%d = inv_set_mpu_sensors(%s)\n", result,
                 sensors_string[params->sensorsIndex]);
        break;
    case 'h':
    case 'H': {
        PrintGestureMenu(params);
    } break;
    default: {
        result = INV_ERROR;
    } break;
    };
    return result;
}

/** 
 * Initializes the tGestureMenuParams to a set of defaults.
 * 
 * @param params The parameters to initialize.
 */
void GestureMenuSetDefaults(tGestureMenuParams * const params)
{
    params->xTapThreshold           = 100;
    params->yTapThreshold           = 100;
    params->zTapThreshold           = 100;
    params->tapTime                 = 100;
    params->nextTapTime             = 600;
    params->maxTaps                 = 2;
    params->shakeRejectValue        = 0.8f;
    params->xShakeThresh            = 750;
    params->yShakeThresh            = 750;
    params->zShakeThresh            = 750;
    params->xSnapThresh             = 160;
    params->ySnapThresh             = 320;
    params->zSnapThresh             = 160;
    params->shakeTime               = 100;
    params->nextShakeTime           = 1000;
    params->shakeFunction           = 0;
    params->maxShakes               = 3;
    params->yawRotateTime           = 80;
    params->yawRotateThreshold      = 70;
    params->orientationThreshold    = 60;
    params->sensorsIndex            = 0;
    params->available_sensors       = INV_NINE_AXIS;
}

void GestureMenuSetAvailableSensors(tGestureMenuParams * const params,
                                    unsigned long available_sensors)
{
    params->available_sensors       = available_sensors;
}
/** 
 * Call the appropriate MPL set threshold functions and checkes the error codes
 * 
 * @param params The parametrs to use in setting the thresholds
 * 
 * @return INV_SUCCESS or the first error code encountered.
 */
inv_error_t GestureMenuSetMpl(tGestureMenuParams const * const params)
{
    inv_error_t result = INV_SUCCESS;

    result = inv_set_tap_threshold(INV_TAP_AXIS_X, params->xTapThreshold);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_tap_threshold returned :%d\n", result);
        return result;
    }
    result = inv_set_tap_threshold(INV_TAP_AXIS_Y, params->yTapThreshold);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_tap_threshold returned :%d\n", result);
        return result;
    }
    result = inv_set_tap_threshold(INV_TAP_AXIS_Z, params->zTapThreshold);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_tap_threshold returned :%d\n", result);
        return result;
    }
    result = inv_set_next_tap_time(params->tapTime);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_next_tap_time returned :%d\n", result);
        return result;
    }
    result = MLSetNextTapTime(params->nextTapTime);
    if (INV_SUCCESS != result) {
        MPL_LOGE("MLSetNextTapTime returned :%d\n", result);
        return result;
    }
    result = inv_set_max_taps(params->maxTaps);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_max_taps returned :%d\n", result);
        return result;
    }
    result = inv_set_tap_shake_reject(params->shakeRejectValue);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_tap_shake_reject returned :%d\n", result);
        return result;
    }

    //Set up shake gesture
    result = inv_set_shake_func(params->shakeFunction);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_shake_func returned :%d\n", result);
        return result;
    }
    result = inv_set_shake_thresh(INV_ROLL_SHAKE, params->xShakeThresh);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_shake_thresh returned :%d\n", result);
        return result;
    }
    result = inv_set_shake_thresh(INV_PITCH_SHAKE, params->yShakeThresh);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_shake_thresh returned :%d\n", result);
        return result;
    }
    result = inv_set_shake_thresh(INV_YAW_SHAKE, params->zShakeThresh);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_shake_thresh returned :%d\n", result);
        return result;
    }
    result = inv_set_shake_time(params->shakeTime);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_shake_time returned :%d\n", result);
        return result;
    }
    result = inv_set_next_shake_time(params->nextShakeTime);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_next_shake_time returned :%d\n", result);
        return result;
    }
    result = inv_set_max_shakes(INV_SHAKE_ALL,params->maxShakes);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_max_shakes returned :%d\n", result);
        return result;
    }

    // Yaw rotate settings
    result = inv_set_yaw_rotate_time(params->yawRotateTime);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_yaw_rotate_time returned :%d\n", result);
        return result;
    }
    result = inv_set_yaw_rotate_thresh(params->yawRotateThreshold);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_yaw_rotate_thresh returned :%d\n", result);
        return result;
    }

    // Orientation settings
    result = inv_set_orientation_thresh(params->orientationThreshold, 5, 80,
                                        INV_X_AXIS | INV_Y_AXIS | INV_Z_AXIS);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_orientation_thresh returned: %d\n", result);
        return result;
    }

    // Requested Sensors
    result = inv_set_mpu_sensors(
        sensors[params->sensorsIndex] & params->available_sensors);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_set_mpu_sensors returned: %d %lx\n", result, 
                 sensors[params->sensorsIndex] & params->available_sensors);
        return result;
    }

    return INV_SUCCESS;
}

void PrintGesture(tGesture* gesture) 
{
    float speed;
    char type[1024];
    switch (gesture->type) 
    {
    case INV_TAP:
    {
        if (gesture->meta < 0) {
            snprintf(type,sizeof(type),"-");
        } else {
            snprintf(type,sizeof(type),"+");
        }

        switch (ABS(gesture->meta))
        {
        case 1:
            strcat(type,"X");
            break;
        case 2:
            strcat(type,"Y");
            break;
        case 3:
            strcat(type,"Z");
            break;
        default:
            strcat(type,"ERROR");
            break;
        };
        MPL_LOGI("TAP: %s  %2d, X: %6d Y: %6d Z: %6d XY: %6.2f, YZ: %6.2f, XZ: %6.2f\n",
                 type,
                 gesture->num,
                 gesture->strength,
                 gesture->speed,
                 gesture->reserved,
                 (180 / M_PI) * atan2(
                    (float)gesture->strength, (float)gesture->speed),
                 (180 / M_PI) * atan2(
                    (float)gesture->speed, (float)gesture->reserved),
                 (180 / M_PI) * atan2(
                    (float)gesture->strength, (float)gesture->reserved)
            );
    }
    break;
    case INV_ROLL_SHAKE:
    case INV_PITCH_SHAKE:
    case INV_YAW_SHAKE:
    {
        if (gesture->strength){
            snprintf(type, sizeof(type), "Snap : ");
        } else {
            snprintf(type, sizeof(type), "Shake: ");
        }

        if (gesture->meta==0) {
            strcat(type, "+");
        } else {
            strcat(type, "-");
        }

        if (gesture->type == INV_ROLL_SHAKE) {
            strcat(type, "Roll  ");
        } else if (gesture->type == INV_PITCH_SHAKE) {
            strcat(type, "Pitch ");
        } else if (gesture->type == INV_YAW_SHAKE) {
            strcat(type, "Yaw   ");
        }
        
        speed = (float)gesture->speed + 
            (float)(gesture->reserved / (float)(1 << 16));
        MPL_LOGI("%s:%3d (speed: %8.2f)\n",type, gesture->num, speed);
    }
    break;
    case INV_YAW_IMAGE_ROTATE:
    {
        if (gesture->meta == 0) {
            snprintf(type, sizeof(type), "Positive ");
        } else {
            snprintf(type, sizeof(type), "Negative ");
        }
        MPL_LOGI("%s Yaw Image Rotation\n", type);
    }
    break;
    default:
        MPL_LOGE("Unknown Gesture received\n");
        break;
    }
}

/** 
 * Prints the new or current orientation using MPL_LOGI and remembers the last
 * orientation to print orientation flips.
 * 
 * @param orientation the new or current orientation.  0 to reset.
 */
void PrintOrientation(unsigned short orientation)
{
    // Determine if it was a flip
    static int sLastOrientation = 0;
    int flip = orientation | sLastOrientation;

    if ((INV_X_UP | INV_X_DOWN) == flip) {
        MPL_LOGI("Flip about the X Axis: \n");
    } else if ((INV_Y_UP | INV_Y_DOWN) == flip) {
        MPL_LOGI("Flip about the Y axis: \n");
    } else if ((INV_Z_UP | INV_Z_DOWN) == flip) {
        MPL_LOGI("Flip about the Z axis: \n");
    }
    sLastOrientation = orientation;

    switch (orientation) {
    case INV_X_UP:
        MPL_LOGI("X Axis is up\n");
        break;
    case INV_X_DOWN:
        MPL_LOGI("X Axis is down\n");
        break;
    case INV_Y_UP:
        MPL_LOGI("Y Axis is up\n");
        break;
    case INV_Y_DOWN:
        MPL_LOGI("Y Axis is down\n");
        break;
    case INV_Z_UP:
        MPL_LOGI("Z Axis is up\n");
        break;
    case INV_Z_DOWN:
        MPL_LOGI("Z Axis is down\n");
        break;
    case 0:
        break; /* Not an error.  Resets sLastOrientation */
    default:
        MPL_LOGE("%s: Unreconized orientation %hx\n", __func__, orientation);
        break;
    }
}


