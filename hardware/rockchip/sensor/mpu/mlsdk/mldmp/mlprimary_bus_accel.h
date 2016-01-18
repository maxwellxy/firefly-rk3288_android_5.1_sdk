/*
  $License:
    Copyright (c) 2010 InvenSense Corporation, All Rights Reserved.
  $
*/
/***************************************************************************** *
 * $Id: $ 
 ******************************************************************************/
/**
 * @{
 * @file     mlprimary_bus_accel.h
 * @brief  workaround for implementations that put the accel on the primary bus 
 *
 *
 * Installs a callback function that reads data from the accel periodically and
 * writes it to DMP memory.  Installs DMP code that reads that memory and uses
 * it instead of any garbage dat the might be on the secondary i2c bus.
 */

#ifndef MLPRIMARY_BUS_ACCEL_H
#define MLPRIMARY_BUS_ACCEL_H

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    inv_error_t inv_set_primary_bus_accel(int on);

#ifdef __cplusplus
}
#endif

#endif // MLPRIMARY_BUS_ACCEL_H
