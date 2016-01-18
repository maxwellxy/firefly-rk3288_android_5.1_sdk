/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: mlsupervisor_9axis.h 6271 2011-11-09 01:05:14Z kkeal $
 *
 *****************************************************************************/

#ifndef MLDMP_MLSUPERVISOR_H__
#define MLDMP_MLSUPERVISOR_H__

#include "mltypes.h"

#include "temp_comp.h"

struct inv_fusion_t {
    int compassCount;
    long quat[4];
};

#ifdef __cplusplus
extern "C" {
#endif

inv_error_t inv_enable_9x_fusion(void);
inv_error_t inv_disable_9x_fusion(void);

inv_error_t inv_enable_9x_fusion_legacy(void);
inv_error_t inv_disable_9x_fusion_legacy(void);

inv_error_t inv_enable_9x_fusion_new(void);
inv_error_t inv_disable_9x_fusion_new(void);

inv_error_t inv_enable_9x_fusion_basic(void);
inv_error_t inv_disable_9x_fusion_basic(void);

inv_error_t inv_enable_9x_fusion_external(void);
inv_error_t inv_disable_9x_fusion_external(void);

inv_error_t inv_enable_9x_fusion_outside(void);
inv_error_t inv_disable_9x_fusion_outside(void);


inv_error_t inv_enable_maintain_heading(void);
inv_error_t inv_disable_maintain_heading(void);

void inv_set_compass_state(long compassState, long accState, 
                           unsigned long deltaTime,
                           int magDisturb, int gotBias,
                           int *new_state,
                           int *new_accuracy);

#ifdef __cplusplus
}
#endif

#endif // MLDMP_MLSUPERVISOR_H__
