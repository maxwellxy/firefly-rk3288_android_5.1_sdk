/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: key0_96.c 6056 2011-09-20 18:24:54Z mcaramello $
 *
 *****************************************************************************/
#include "key0_96.h"
#include "dmpKey.h"
#include "mlFIFO.h"
#include "mlsupervisor.h"
#include "log.h"
#include "mldl.h"

static long inv_key_96_value = (1L<<30);
static long inv_prev_key_96_value = 1000;
static int acc_count = 0;

void set_key_0_96(long value)
{
    inv_key_96_value = value;
}

/* This controls setting key_0_96 */
inv_error_t inv_key_0_96(struct inv_obj_t *inv_obj)
{
    unsigned char regs[4];
    inv_error_t result = INV_SUCCESS;
    unsigned long accMag,gyroMag;
    long delta_time;

    accMag = inv_accel_sum_of_sqr();
    gyroMag = inv_get_gyro_sum_of_sqr() >> GYRO_MAG_SQR_SHIFT;
    delta_time = inv_get_fifo_rate()*5;

    if (inv_obj->lite_fusion->acc_state != SF_STARTUP_SETTLE) {
        if (((accMag > 260000L) || (accMag < 6000)) || (gyroMag > 2250000L)) {
            inv_obj->lite_fusion->acc_state = SF_DISTURBANCE; //No accels, fast swing
            acc_count = 0;
        } else {
            if ((gyroMag < 400) && (accMag < 200000L) && (accMag > 26214)
                && ((inv_obj->lite_fusion->acc_state == SF_DISTURBANCE)
                    || (inv_obj->lite_fusion->acc_state == SF_SLOW_SETTLE))) {
                acc_count += delta_time;
                if (acc_count > 0) {
                    inv_obj->lite_fusion->acc_state = SF_FAST_SETTLE;
                    acc_count = 0;
                }
            } else {
                if (inv_obj->lite_fusion->acc_state == SF_DISTURBANCE) {
                    acc_count += delta_time;
                    if (acc_count > 500) {
                        inv_obj->lite_fusion->acc_state = SF_SLOW_SETTLE;
                        acc_count = 0;
                    }
                } else if (inv_obj->lite_fusion->acc_state == SF_SLOW_SETTLE) {
                    acc_count += delta_time;
                    if (acc_count > 1000) {
                        inv_obj->lite_fusion->acc_state = SF_NORMAL;
                        acc_count = 0;
                    }
                } else if (inv_obj->lite_fusion->acc_state == SF_FAST_SETTLE) {
                    acc_count += delta_time;
                    if (acc_count > 250) {
                        inv_obj->lite_fusion->acc_state = SF_NORMAL;
                        acc_count = 0;
                    }
                }
            }
        }
    }
    if (inv_obj->lite_fusion->acc_state == SF_STARTUP_SETTLE) {
        acc_count += delta_time;
        if (acc_count > 50) {
            inv_key_96_value = 1073741824L;    //Startup settling
            inv_obj->lite_fusion->acc_state = SF_NORMAL;
            acc_count = 0;
        }
    } else if ((inv_obj->lite_fusion->acc_state == SF_NORMAL)
               || (inv_obj->lite_fusion->acc_state == SF_SLOW_SETTLE)) {
        inv_key_96_value = inv_obj->accel->sens * 64;   //Normal
    } else if ((inv_obj->lite_fusion->acc_state == SF_DISTURBANCE)) {
        inv_key_96_value = inv_obj->accel->sens * 64;   //Don't use accel (should be 0)
    } else if (inv_obj->lite_fusion->acc_state == SF_FAST_SETTLE) {
        inv_key_96_value = inv_obj->accel->sens * 512;  //Amplify accel
    }
    if (!inv_get_gyro_present()) {
        inv_key_96_value = inv_obj->accel->sens * 128;
    }

    if (inv_key_96_value != inv_prev_key_96_value) {
        regs[0] = (unsigned char)((inv_key_96_value >> 24) & 0xff);
        regs[1] = (unsigned char)((inv_key_96_value >> 16) & 0xff);
        regs[2] = (unsigned char)((inv_key_96_value >> 8) & 0xff);
        regs[3] = (unsigned char)(inv_key_96_value & 0xff);
        result = inv_set_mpu_memory(KEY_D_0_96, 4, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        inv_prev_key_96_value = inv_key_96_value;
    }
    return result;
}
