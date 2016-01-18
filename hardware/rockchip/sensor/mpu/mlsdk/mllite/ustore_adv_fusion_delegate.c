/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: ustore_adv_fusion_delegate.c 5873 2011-08-11 03:13:48Z mcaramello $
 *
 ******************************************************************************/

#include "ustore_adv_fusion_delegate.h"

#include "ml.h"
#include "ml_invobj.h"

#include "ustore_manager.h"
#include "ustore_delegate_io.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-ustore-advfus"

/* Storeload objects and sizes */
struct storeload_obj {
    void * obj;
    int    size;
};

static const struct storeload_obj adv_fusion_objs[] = {
      {&iobj_adv_fusion.got_compass_bias     , sizeof(iobj_adv_fusion.got_compass_bias)      }
    , {&iobj_adv_fusion.got_init_compass_bias, sizeof(iobj_adv_fusion.got_init_compass_bias) }
    , {&iobj_adv_fusion.compass_state        , sizeof(iobj_adv_fusion.compass_state)         }
    , {&iobj_adv_fusion.compass_bias_error   , sizeof(iobj_adv_fusion.compass_bias_error)    }
    , {&iobj_adv_fusion.compass_peaks        , sizeof(iobj_adv_fusion.compass_peaks)         }
    , {&iobj_adv_fusion.compass_scale        , sizeof(iobj_adv_fusion.compass_scale)         }
    , {&iobj_adv_fusion.compass_prev_xty     , sizeof(iobj_adv_fusion.compass_prev_xty)      }
    , {&iobj_adv_fusion.compass_prev_m       , sizeof(iobj_adv_fusion.compass_prev_m)        }
};
static const int total_objs = sizeof(adv_fusion_objs) / sizeof(adv_fusion_objs[0]);

#define ADV_FUSION_STORAGE_SIZE  (sizeof(iobj_adv_fusion.got_compass_bias)      +\
                                  sizeof(iobj_adv_fusion.got_init_compass_bias) +\
                                  sizeof(iobj_adv_fusion.compass_state)         +\
                                  sizeof(iobj_adv_fusion.compass_bias_error)    +\
                                  sizeof(iobj_adv_fusion.compass_peaks)         +\
                                  sizeof(iobj_adv_fusion.compass_scale)         +\
                                  sizeof(iobj_adv_fusion.compass_prev_xty)      +\
                                  sizeof(iobj_adv_fusion.compass_prev_m)        )

/* Storeload delegate functions and data structure */

static inv_error_t inv_store_adv_fusion_data(void);
static inv_error_t inv_load_adv_fusion_data(void);

static const struct uloadstoredelegate ustore_adv_fusion = {
    /* .store = */   inv_store_adv_fusion_data
    /* .load  = */ , inv_load_adv_fusion_data
    /* .len   = */ , ADV_FUSION_STORAGE_SIZE
    /* .tag   = */ , INV_USTORE_ID_ADV_FUSION
};

inv_error_t inv_init_ustore_adv_fusion(void)
{
    inv_error_t result;
    result = inv_ustore_register_handler(&ustore_adv_fusion);
    return result;
}

inv_error_t inv_store_adv_fusion_data(void)
{
    inv_error_t result;
    int ii;
    for (ii = 0; ii < total_objs; ii++){
        result = inv_ustore_mem(adv_fusion_objs[ii].obj , adv_fusion_objs[ii].size);
        if (result != INV_SUCCESS) {
            MPL_LOGE("inv_ustore_mem failed with adv_fusion_objs[%d]\n",ii);
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }
    return INV_SUCCESS;
}

inv_error_t inv_load_adv_fusion_data(void)
{
    inv_error_t result;
    int ii;
    for (ii = 0; ii < total_objs; ii++){
        result = inv_uload_mem(adv_fusion_objs[ii].obj , adv_fusion_objs[ii].size);
        if (result != INV_SUCCESS) {
            MPL_LOGE("inv_uload_mem failed with adv_fusion_objs[%d]\n",ii);
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }
    return INV_SUCCESS;
}
