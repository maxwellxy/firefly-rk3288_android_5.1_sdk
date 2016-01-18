/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: ustore_lite_fusion_delegate.c 5873 2011-08-11 03:13:48Z mcaramello $
 *
 ******************************************************************************/


#include "ustore_lite_fusion_delegate.h"

#include "ml.h"
#include "ml_invobj.h"

#include "ustore_manager.h"
#include "ustore_delegate_io.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-ustore-litefus"

/* Storeload delegate functions and data structure */

static inv_error_t inv_store_lite_fusion_data(void);
static inv_error_t inv_load_lite_fusion_data(void);

static long accel_bias[3];
#define LITE_FUSION_STORAGE_SIZE sizeof(accel_bias)

const static struct uloadstoredelegate ustore_lite_fusion = {
    /* .store = */   inv_store_lite_fusion_data
    /* .load  = */ , inv_load_lite_fusion_data
    /* .len   = */ , LITE_FUSION_STORAGE_SIZE
    /* .tag   = */ , INV_USTORE_ID_LITE_FUSION
};

inv_error_t inv_init_ustore_lite_fusion(void)
{
    inv_error_t result;
    result = inv_ustore_register_handler(&ustore_lite_fusion);
    return result;
}

inv_error_t inv_store_lite_fusion_data(void)
{
    inv_error_t result_get, result_store;

    result_get = inv_get_accel_bias(accel_bias);
    if (result_get != INV_SUCCESS) {
        LOG_RESULT_LOCATION(result_get);
        /* populate with dummy vals and do not return:
         * call to inv_ustore_mem is required for ustore to work properly. */
        accel_bias[0] = 0; accel_bias[1] = 0; accel_bias[2] = 0;
    }
    result_store = inv_ustore_mem(accel_bias, sizeof(accel_bias));
    if (result_store != INV_SUCCESS) {
        MPL_LOGE("inv_ustore_mem failed with accel_bias\n");
        LOG_RESULT_LOCATION(result_store);
        return result_store;
    }
    return result_get;
}

inv_error_t inv_load_lite_fusion_data(void)
{
    inv_error_t result_load, result_set;

    result_load = inv_uload_mem(accel_bias, sizeof(accel_bias));
    if (result_load != INV_SUCCESS) {
        MPL_LOGE("inv_uload_mem failed with accel_bias\n");
        LOG_RESULT_LOCATION(result_load);
        return result_load;
    }
    result_set = inv_set_accel_bias(accel_bias);
    if (result_set != INV_SUCCESS) {
        LOG_RESULT_LOCATION(result_set);
        return result_set;
    }
    return INV_SUCCESS;
}

