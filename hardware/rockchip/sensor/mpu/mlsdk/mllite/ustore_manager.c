/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: ustore_manager.c 5873 2011-08-11 03:13:48Z mcaramello $
 *
 ******************************************************************************/


#include "ustore_manager.h"
#include "ustore_manager_io.h"
#include "ustore_delegate_io.h"

#include "ml.h"
#include "mltypes.h"
#include "mlstates.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-ustore"

#define NUM_STORE_LOAD_DELEGATES 3

struct _SD {
    const struct uloadstoredelegate * delegates[NUM_STORE_LOAD_DELEGATES];
    unsigned short numdelegates;
};
static struct _SD sd_data;
static char initialized = 0;

inv_error_t inv_ustore_enable(void)
{
    sd_data.numdelegates = 0;
    initialized = 1;
    return INV_SUCCESS;
}

inv_error_t inv_ustore_register_handler(const struct uloadstoredelegate * d)
{
    if (!initialized)
        return INV_ERROR_FEATURE_NOT_ENABLED;

    if (d == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (sd_data.numdelegates < NUM_STORE_LOAD_DELEGATES) {
        sd_data.delegates[sd_data.numdelegates++] = d;
    } else {
        return INV_ERROR_MEMORY_EXAUSTED;
    }
    return INV_SUCCESS;
}

inv_error_t inv_uload_calibration()
{
    inv_error_t result = INV_SUCCESS;
    inv_error_t result2;
    int ii;
    
    if (!initialized)
        return INV_ERROR_FEATURE_NOT_ENABLED;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    result = inv_uload_open();
    if (result != INV_SUCCESS){
        LOG_RESULT_LOCATION(result);
        return result;
    }
    
    for (ii = 0; ii < sd_data.numdelegates; ii++) {
        const struct uloadstoredelegate *d = sd_data.delegates[ii]; // shortcut.
        unsigned short read_tag;
        /*  Read the following from memory:
         *  [ 0 1 ] [ 2 3 ] [ 4 .... (len+4-1)]
         *   tag     len     data (size "len")
         */
        result = inv_uload_mem(&read_tag, sizeof(unsigned short));
        if (read_tag != d->tag){
            MPL_LOGD("Bad tag in uload_calibration: uload delegate %d "
                     "expected %d, got %d\n",
                     ii, d->tag, read_tag);
            result = INV_ERROR_CALIBRATION_LOAD;
            break; // close and exit.
        }

        inv_ustoreload_set_max_len(d->len);
        result = d->load();
        inv_ustoreload_reset_len();
        if (result != INV_SUCCESS){
            MPL_LOGD("uload delegate %d returned error %d\n",ii,result);
            break;
        }
    }

    result2 = inv_uload_close();
    if (result == INV_SUCCESS && result2 != INV_SUCCESS) {
        LOG_RESULT_LOCATION(result2);
        return result2;
    }
    return result;
}

inv_error_t inv_ustore_calibration()
{
    inv_error_t result = INV_SUCCESS;
    inv_error_t result2;
    int ii;
    unsigned int store_length;
    const struct uloadstoredelegate *d; // shortcut.;
    
    if (!initialized)
        return INV_ERROR_FEATURE_NOT_ENABLED;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    result = inv_ustore_open();
    if (result != INV_SUCCESS){
        LOG_RESULT_LOCATION(result);
        return result;
    }

	store_length = sizeof(store_length);
    for (ii = 0; ii < sd_data.numdelegates; ii++) {
        d = sd_data.delegates[ii]; // shortcut.;
		store_length += sizeof(d->tag) + d->len;
	}
    result = inv_ustore_mem(&(store_length), sizeof(store_length));
    if (result != INV_SUCCESS){
        LOG_RESULT_LOCATION(result);
    }

    for (ii = 0; ii < sd_data.numdelegates; ii++) {
        d = sd_data.delegates[ii]; // shortcut.;
        /*  Write the following to memory:
         *  [ 0 1 ] [ 4 .... (len+4-1)]
         *   tag     data (size "len")
         */
        result = inv_ustore_mem(&(d->tag), sizeof(unsigned short));
        if (result != INV_SUCCESS){
            LOG_RESULT_LOCATION(result);
            break;
        }
   
        inv_ustoreload_set_max_len(d->len);
        result = d->store();
        inv_ustoreload_reset_len();
        if (result != INV_SUCCESS){
            MPL_LOGD("ustore delegate %d returned error %d\n",ii,result);
            break;
        }
    }

    result2 = inv_ustore_close();
    if (result == INV_SUCCESS && result2 != INV_SUCCESS) {
        LOG_RESULT_LOCATION(result2);
        return result2;
    }
    return result;
}


