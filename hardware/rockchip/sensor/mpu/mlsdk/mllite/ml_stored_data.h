/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: ml_stored_data.h 5873 2011-08-11 03:13:48Z mcaramello $
 *
 ******************************************************************************/

#ifndef INV_STORED_DATA_H
#define INV_STORED_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/*
    Includes.
*/

#include "mltypes.h"
#include "ml_stored_data_legacy.h"

/*
    Defines
*/
#define INV_CAL_ACCEL_LEN    (12)
#define INV_CAL_COMPASS_LEN  (555)
#define INV_CAL_HDR_LEN      (6)
#define INV_CAL_CHK_LEN      (4)

/*
    APIs
*/
    inv_error_t inv_load_calibration(void);
    inv_error_t inv_store_calibration(void);

/*
    Other prototypes
*/
    inv_error_t inv_load_cal(unsigned char *calData);
    inv_error_t inv_store_cal(unsigned char *calData, int length);
    unsigned int inv_get_cal_length(void);

#ifdef __cplusplus
}
#endif
#endif                          /* INV_STORED_DATA_H */
