/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: accel.h 5873 2011-08-11 03:13:48Z mcaramello $
 *
 *******************************************************************************/

#ifndef ACCEL_H
#define ACCEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "mpu.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "accel_legacy.h"
#endif

    /* ------------ */
    /* - Defines. - */
    /* ------------ */

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    unsigned char inv_accel_present(void);
    unsigned char inv_get_slave_addr(void);
    inv_error_t inv_get_accel_data(long *data);
    unsigned short inv_get_accel_id(void);

#ifdef __cplusplus
}
#endif
#endif                          // ACCEL_H
