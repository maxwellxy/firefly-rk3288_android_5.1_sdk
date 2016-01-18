/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/**
 * @defgroup 
 * @brief  
 *
 * @{
 * @file     external_slave_init.h
 * @brief    
 *
 *
 */

#ifndef _EXTERNAL_SLAVE_AKMD8963_H_
#define _EXTERNAL_SLAVE_AKMD8963_H_

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

inv_error_t inv_external_slave_akm8963_open(void);
inv_error_t inv_external_slave_akm8963_close(void);

#ifdef __cplusplus
}
#endif

#endif
