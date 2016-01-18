/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: pressure.h 5873 2011-08-11 03:13:48Z mcaramello $
 *
 *******************************************************************************/

#ifndef PRESSURE_H
#define PRESSURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "mpu.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "pressure_legacy.h"
#endif

    /* ------------ */
    /* - Defines. - */
    /* ------------ */

#define USE_PRESSURE_BMA                    0

#define PRESSURE_SLAVEADDR_INVALID          0x00
#define PRESSURE_SLAVEADDR_BMA085           0x77

/*
    Define default pressure to use if no selection is made
*/
#if USE_PRESSURE_BMA
#define DEFAULT_PRESSURE_TYPE              PRESSURE_ID_BMA
#endif

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    unsigned char inv_pressure_present(void);
    unsigned char inv_get_pressure_slave_addr(void);
    inv_error_t inv_suspend_pressure(void);
    inv_error_t inv_resume_presure(void);
    inv_error_t inv_get_pressure_data(long *data);
    unsigned short inv_get_pressure_id(void);

#ifdef __cplusplus
}
#endif
#endif                          // PRESSURE_H
