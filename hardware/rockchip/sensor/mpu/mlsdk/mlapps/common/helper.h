/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: helper-customer.h 6276 2011-11-09 22:40:46Z mcaramello $
 *
 *******************************************************************************/

#ifndef HELPER_C_H
#define HELPER_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "mlerrorcode.h"

/*
    Defines 
*/

#define CALL_N_CHECK(f) {                                                   \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                              \
        printf("Error in file %s, line %d : %s returned code %s (#%d)\n",   \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
    }                                                                       \
}

#define CALL_CHECK_N_RETURN_ERROR(f) {                                      \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                              \
        printf("Error in file %s, line %d : %s returned code %s (#%d)\n",   \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        return r35uLt;                                                      \
    }                                                                       \
}

// for functions returning void
#define CALL_CHECK_N_RETURN(f) {                                            \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                              \
        printf("Error in file %s, line %d : %s returned code %s (#%d)\n",   \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        return;                                                             \
    }                                                                       \
}

#define CALL_CHECK_N_EXIT(f) {                                              \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                              \
        printf("Error in file %s, line %d : %s returned code %s (#%d)\n",   \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        exit (r35uLt);                                                      \
    }                                                                       \
}

    
#define CALL_CHECK_N_CALLBACK(f, cb) {                                      \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                              \
        printf("Error in file %s, line %d : %s returned code %s (#%d)\n",   \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        cb;                                                                 \
    }                                                                       \
}

#define CALL_CHECK_N_GOTO(f, label) {                                       \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                              \
        printf("Error in file %s, line %d : %s returned code %s (#%d)\n",   \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        goto label;                                                         \
    }                                                                       \
}

#define DEFAULT_PLATFORM        PLATFORM_ID_MSB_V2
#define DEFAULT_ACCEL_ID        ACCEL_ID_KXTF9
#define DEFAULT_COMPASS_ID      COMPASS_ID_AK8975

#define DataLogger(x)           NULL
#define DataLoggerSelector(x)   //
#define DataLoggerCb(x)         NULL
#define findComm()              (9)
#define MenuHwChoice(p,a,c)     (*p = DEFAULT_PLATFORM, *a = DEFAULT_ACCEL_ID, \
                                 *c = DEFAULT_COMPASS_ID, INV_ERROR)

    char ConsoleGetChar(void);
    int ConsoleKbhit(void);
    struct mpuirq_data **InterruptPoll(
        int *handles, int numHandles, long tv_sec, long tv_usec);
    void InterruptPollDone(struct mpuirq_data ** data);

#ifdef __cplusplus
}
#endif

#endif // HELPER_C_H
