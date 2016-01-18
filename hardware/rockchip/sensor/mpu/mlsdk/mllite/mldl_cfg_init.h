/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/**
 * @addtogroup MLDL
 * @brief  
 *
 * @{
 * @file     mldl_cfg_init.h
 * @brief    
 *
 *
 */
#ifndef __MLDL_CFG_INIT__
#define __MLDL_CFG_INIT__

#ifdef __cplusplus
extern "C" {
#endif

inv_error_t inv_mldl_cfg_init(struct mldl_cfg **mldl_cfg);
inv_error_t inv_mldl_cfg_exit(struct mldl_cfg **mldl_cfg);

#ifdef __cplusplus
}
#endif

#endif
