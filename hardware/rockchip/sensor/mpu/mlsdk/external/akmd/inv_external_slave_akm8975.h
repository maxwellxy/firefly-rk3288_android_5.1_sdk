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

#ifndef _EXTERNAL_SLAVE_AKMD8975_H_
#define _EXTERNAL_SLAVE_AKMD8975_H_

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

inv_error_t inv_external_slave_akm8975_open(void);
inv_error_t inv_external_slave_akm8975_close(void);
inv_error_t inv_external_slave_akm8975_get_debug_data(void *data, int size);

#ifdef __cplusplus
}
#endif

#endif
