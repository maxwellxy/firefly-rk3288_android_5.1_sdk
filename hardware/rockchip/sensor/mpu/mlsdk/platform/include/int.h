/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: int.h 5629 2011-06-11 03:13:08Z mcaramello $
 *
 *******************************************************************************/

#ifndef _INT_H
#define _INT_H

#include "mltypes.h"
#include "mpu.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* ------------ */
    /* - Defines. - */
    /* ------------ */

    /* ---------- */
    /* - Enums. - */
    /* ---------- */

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    void IntOpen(const char **ints,
                 int *handles,
                 int numHandles);
    int IntProcess(int *handles, int numHandles,
                   struct mpuirq_data **data, 
                   long tv_sec, long tv_usec);
    inv_error_t IntClose(int *handles, int numHandles);
    inv_error_t IntSetTimeout(int handle, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* _TEMPLATE_H */
