/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: mlstates.h 6061 2011-09-20 23:39:17Z mcaramello $
 *
 *******************************************************************************/

#ifndef INV_STATES_H__
#define INV_STATES_H__

#include "mltypes.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlstates_legacy.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* See inv_state_transition for the transition mask */
#define INV_STATE_SERIAL_CLOSED      (0)
#define INV_STATE_SERIAL_OPENED      (1)
#define INV_STATE_DMP_OPENED         (2)
#define INV_STATE_DMP_STARTED        (3)
#define INV_STATE_DMP_STOPPED        (INV_STATE_DMP_OPENED)
#define INV_STATE_DMP_CLOSED         (INV_STATE_SERIAL_OPENED)

#define INV_STATE_NAME(x)            (#x)

    typedef inv_error_t(*state_change_callback_t) (unsigned char newState);

    char *inv_state_name(unsigned char x);
    inv_error_t inv_state_transition(unsigned char newState);
    unsigned char inv_get_state(void);
    inv_error_t inv_register_state_callback(state_change_callback_t callback);
    inv_error_t inv_unregister_state_callback(state_change_callback_t callback);
    inv_error_t inv_run_state_callbacks(unsigned char newState);
    inv_error_t inv_check_state_callback(state_change_callback_t callback,
        unsigned char *is_registered);

#ifdef __cplusplus
}
#endif
#endif                          // INV_STATES_H__
