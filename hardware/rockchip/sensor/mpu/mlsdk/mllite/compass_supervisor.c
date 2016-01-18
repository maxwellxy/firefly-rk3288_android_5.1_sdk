/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: compass_supervisor.c 6271 2011-11-09 01:05:14Z kkeal $
 *
 ******************************************************************************/

#include "compass_supervisor.h"
#include "compass_supervisor_inv_obj_callbacks.h"

#include "mlMathFunc.h"
#include "mlFIFO.h"
#include "mlos.h"
#include "compass.h"
#include "mldl.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-comp_sup"

#undef  MPL_LOG_NDEBUG
#define MPL_LOG_NDEBUG 0 /* Use 0 to turn on MPL_LOGV output */

typedef inv_error_t (*inv_compass_cb_t)(struct compass_obj_t *obj);

inv_error_t inv_try_compass(int *got_data);

struct compass_rate_t {
    // These describe callbacks happening everytime a new compass value is read
    int_fast8_t num_cb;
    HANDLE mutex;
    inv_compass_cb_t compass_process_cb[MAX_COMPASS_RATE_PROCESSES];
    int priority[MAX_COMPASS_RATE_PROCESSES];
    unsigned long polltime;
    unsigned long pollrate;
};
static struct compass_rate_t compass_rate_obj;

struct compass_obj_t inv_compass_obj;

struct inv_external_compass_t
{
    long compass[3];
    int accuracy;
    int received;
    int mode;
};
static struct inv_external_compass_t ec;

/** Set compass data from an external source
* @param[in] compass Compass data, length 3, in uT * 2^16 in body frame.
* @param[in] accuracy Accuracy of compass data.
*/
void inv_set_external_compass_data(long *compass, int accuracy)
{
    ec.received = 1;
    ec.accuracy = accuracy;
    ec.compass[0] = compass[0];
    ec.compass[1] = compass[1];
    ec.compass[2] = compass[2];
}

int inv_get_external_accuracy()
{
    if (ec.mode)
        return ec.accuracy;
    else
        return -1;
}


/** Call this function once to enable compass data being received externally through
* the inv_set_external_compass_data() function.
*/
void inv_set_external_compass_mode()
{
    ec.mode = 1;
}

inv_error_t inv_enable_compass_supervisor(void)
{
    inv_error_t result;

    memset(&ec, 0, sizeof(ec));
    result = inv_create_mutex(&compass_rate_obj.mutex);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_register_fifo_rate_process(inv_run_compass_rate_processes,
                INV_PRIORITY_COMPASS_SUPERVISOR);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    compass_rate_obj.num_cb = 0;
    compass_rate_obj.polltime = 0;
    compass_rate_obj.pollrate = 20;

    /* inv_obj compass supervisor callbacks are registered
     * by an external function, so that compass_supervisor.c
     * doesn't depend on inv_obj in any way.
     */
    result = inv_register_inv_obj_compass_supervisor_callbacks();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

inv_error_t inv_disable_compass_supervisor(void)
{
    inv_error_t result;
    result = inv_unregister_inv_obj_compass_supervisor_callbacks();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_unregister_fifo_rate_process(inv_run_compass_rate_processes);
    return result;
}

/**
 * @internal
 * @brief   This registers a function to be called for each set of
 *          gyro/quaternion/rotation matrix/etc output.
 * @param[in] func The callback function to register
 * @param[in] priority The unique priority number of the callback. Lower numbers
 *            are called first.
 * @return  error code.
 */
inv_error_t inv_register_compass_rate_process(
                inv_error_t (*func)(struct compass_obj_t *obj), int priority)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    int kk, nn;

    result = inv_lock_mutex(compass_rate_obj.mutex);
    if (INV_SUCCESS != result) {
        return result;
    }
    // Make sure we haven't registered this function already
    // Or used the same priority
    for (kk = 0; kk < compass_rate_obj.num_cb; ++kk) {
        if ((compass_rate_obj.compass_process_cb[kk] == func) ||
            (compass_rate_obj.priority[kk] == priority)) {
            inv_unlock_mutex(compass_rate_obj.mutex);
            return INV_ERROR_INVALID_PARAMETER;
        }
    }

    // Make sure we have not filled up our number of allowable callbacks
    if (compass_rate_obj.num_cb <= MAX_HIGH_RATE_PROCESSES - 1) {
        kk = 0;
        if (compass_rate_obj.num_cb != 0) {
            // set kk to be where this new callback goes in the array
            while ((kk < compass_rate_obj.num_cb) &&
                   (compass_rate_obj.priority[kk] < priority)) {
                kk++;
            }
            if (kk != compass_rate_obj.num_cb) {
                // We need to move the others
                for (nn = compass_rate_obj.num_cb; nn > kk; --nn) {
                    compass_rate_obj.compass_process_cb[nn] =
                                    compass_rate_obj.compass_process_cb[nn - 1];
                    compass_rate_obj.priority[nn] = 
                                            compass_rate_obj.priority[nn - 1];
                }
            }
        }
        // Add new callback
        compass_rate_obj.compass_process_cb[kk] = func;
        compass_rate_obj.priority[kk] = priority;
        compass_rate_obj.num_cb++;
    } else {
        result = INV_ERROR_MEMORY_EXAUSTED;
    }

    inv_unlock_mutex(compass_rate_obj.mutex);
    return result;
}

/**
 * @internal
 * @brief   This unregisters a function to be called for each set of
 *          gyro/quaternion/rotation matrix/etc output.
 * @return  error code.
 */
inv_error_t inv_unregister_compass_rate_process(inv_error_t (*func)(struct compass_obj_t *obj))
{
    INVENSENSE_FUNC_START;
    int kk, jj;
    inv_error_t result;

    result = inv_lock_mutex(compass_rate_obj.mutex);
    if (INV_SUCCESS != result) {
        return result;
    }
    // Make sure we haven't registered this function already
    result = INV_ERROR_INVALID_PARAMETER;
    for (kk = 0; kk < compass_rate_obj.num_cb; ++kk) {
        if (compass_rate_obj.compass_process_cb[kk] == func) {
            for (jj = kk + 1; jj < compass_rate_obj.num_cb; ++jj) {
                compass_rate_obj.compass_process_cb[jj - 1] =
                    compass_rate_obj.compass_process_cb[jj];
                compass_rate_obj.priority[jj - 1] =
                    compass_rate_obj.priority[jj];
            }
            compass_rate_obj.compass_process_cb[compass_rate_obj.num_cb - 1] = NULL;
            compass_rate_obj.priority[compass_rate_obj.num_cb - 1] = 0;
            compass_rate_obj.num_cb--;
            result = INV_SUCCESS;
            break;
        }
    }

    inv_unlock_mutex(compass_rate_obj.mutex);
    return result;

}

inv_error_t inv_run_compass_rate_processes(struct inv_obj_t *inv_obj)
{
    int kk,got_data;
    inv_error_t result, result2;

    result = inv_lock_mutex(compass_rate_obj.mutex);
    if (INV_SUCCESS != result) {
        MPL_LOGE("MLOsLockMutex returned %d\n", result);
        return result;
    }

    result = inv_try_compass(&got_data);

    if (result == INV_SUCCESS && got_data == 1) {
        for (kk = 0; kk < compass_rate_obj.num_cb; ++kk) {
            if (compass_rate_obj.compass_process_cb[kk]) {
                result2 = compass_rate_obj.compass_process_cb[kk](&inv_compass_obj);
                if (result == INV_SUCCESS)
                    result = result2;
                MPL_LOGW_IF(result2 > 0,
                    "Calling compass_process_cb %d/%d, "
                    "priority %d, callback %p, "
                    "polltime %ld, pollrate %ld, "
                    "returned %d\n",
                    kk, compass_rate_obj.num_cb,
                    compass_rate_obj.priority[kk],
                    compass_rate_obj.compass_process_cb[kk],
                    compass_rate_obj.polltime, compass_rate_obj.pollrate,
                    result2);
            }
        }
    }

    inv_unlock_mutex(compass_rate_obj.mutex);
    if (result == INV_ERROR_COMPASS_DATA_NOT_READY)
        result = INV_SUCCESS;

    return result;
}

/** This function will populate compass_obj.raw and compass_obj.delta_time with
* raw compass data and sets the intial compass reading. It also sets compass_obj
* @param[out] got_data Set to 1, if function was able to set compass data, 0 if not.
*/
inv_error_t inv_try_compass(int *got_data)
{
    inv_error_t result;
    unsigned long ctime;
    int i;

    *got_data = 0;

    /* Do nothing if compass isn't present or turned off */
    if (!inv_compass_present()) {
        return INV_SUCCESS;
    }

    if (ec.mode) {
        // Data from an external source
        if (ec.received) {
            inv_compass_obj.calibrated[0] = ec.compass[0];
            inv_compass_obj.calibrated[1] = ec.compass[1];
            inv_compass_obj.calibrated[2] = ec.compass[2];
            inv_compass_obj.accuracy = ec.accuracy;
            ec.received = 0;
            *got_data = 1;
        }
        return INV_SUCCESS;
    }

    /* Check if time to read compass data */
    ctime = inv_get_tick_count();
    inv_compass_obj.delta_time = ctime - compass_rate_obj.polltime;
    if ( inv_compass_obj.delta_time < compass_rate_obj.pollrate ) {
        return INV_SUCCESS;
    }

    result = inv_get_compass_data(inv_compass_obj.raw);

    /* external slave wants the data even if there is an error */
    if (result)
        return result;

    compass_rate_obj.polltime = ctime;

    /* Save the intial compass value to make bias convergence faster in local
     *  body high fields */
    if (IS_INV_ADVFEATURES_ENABLED(inv_obj)) {
        if (inv_obj.adv_fusion->got_init_compass_bias == 0) {
            inv_obj.adv_fusion->got_init_compass_bias = 1;
            for (i = 0; i < 3; i++) {
                inv_compass_obj.init_bias[i] = inv_compass_obj.raw[i];
            }
        }
    }

    *got_data = 1;

    return result;
}
