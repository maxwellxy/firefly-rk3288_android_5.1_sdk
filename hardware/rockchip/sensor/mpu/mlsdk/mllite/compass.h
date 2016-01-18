/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: compass.h 5844 2011-08-05 17:32:54Z kkeal $
 *
 *******************************************************************************/

#ifndef COMPASS_H
#define COMPASS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "mpu.h"
#include "compass_supervisor.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "compass_legacy.h"
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

    unsigned char inv_compass_present(void);
    unsigned char inv_get_compass_slave_addr(void);
    inv_error_t inv_get_compass_data(long *data);
    inv_error_t inv_set_compass_bias(struct compass_obj_t *obj, long *bias);
    unsigned short inv_get_compass_id(void);
    inv_error_t inv_compass_write_reg(unsigned char reg, unsigned char val);
    inv_error_t inv_compass_read_reg(unsigned char reg, unsigned char *val);
    inv_error_t inv_compass_read_scale(long *val);

#ifdef __cplusplus
}
#endif
#endif                          // COMPASS_H
