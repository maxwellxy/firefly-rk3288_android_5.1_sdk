/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: int_linux.c 5629 2011-06-11 03:13:08Z mcaramello $
 *
 ******************************************************************************/

/** 
    @defgroup INT
    @brief  This file contains the routines used to detect the Invensense Motion
            Processing Interrupts.
    
    @{
        @file int.c
        @brief Diagnostics Interface Module

    THIS NEEDS TO BE IMPLEMENTED DIFFERENTLY FOR EACH PLATFORM

*/

#define INT_C

/* ------------- */
/* - Includes. - */
/* ------------- */

#ifndef LINUX
#error Trying to build __FILE__ without LINUX defined
#endif

#include <string.h>
#include <fcntl.h>
#include "mltypes.h"
#include "mlsl.h"
#include "mldl.h"
#include "int.h"
#include "log.h"
#include "mlmath.h"
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "kernel/mpuirq.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-int"

/* --------------------- */
/* - Global Variables. - */
/* --------------------- */

/* --------------------- */
/* - Static Variables. - */
/* --------------------- */

/* --------------------- */
/* - Static Functions. - */
/* --------------------- */


/**
 *  @brief This routine opens the interrupt handling
 *
 *  @param dev Name of the character device to open support interrupts or 
 *             NULL to use the default of "/dev/mpu-0"
 *
 *  @return error code.
 */
void IntOpen(const char **ints,
             int *handles,
             int numHandles)
{
    int ii;
    for (ii = 0; ii < numHandles; ii++) {
        handles[ii] = open(ints[ii], O_RDWR);
        if (handles[ii] < 0) {
            MPL_LOGE("%s open error %d\n", ints[ii], errno);
        } else {
            MPL_LOGI("Opened %s: %d\n", ints[ii], handles[ii]);
        }
    }
}



/**
 * @brief   This function should be called from the main event loop in systems 
 *          that support interrupt polling.
 * @param data Data read
 * @param tv_sec timeout value in seconds
 * @param tv_usec timeout value in micro seconds
 */
int IntProcess(int *handles, int numHandles,
               struct mpuirq_data **data, 
               long tv_sec, long tv_usec)
{
    int ii;
    int numRead = 0;
    int result;
    int size;
    fd_set read_fd;
    fd_set excep_fd;
    int nfds = 0;
    struct timeval timeout;

    if (numHandles > 5) {
        LOG_RESULT_LOCATION(INV_ERROR_MEMORY_EXAUSTED);
        return INV_ERROR_MEMORY_EXAUSTED;
    }

    FD_ZERO(&read_fd);
    FD_ZERO(&excep_fd);
    for (ii = 0; ii < numHandles; ii++) {
        FD_SET(handles[ii], &read_fd);
        FD_SET(handles[ii], &excep_fd);
        nfds = MAX(nfds, handles[ii]);
    }
    timeout.tv_sec = tv_sec;
    timeout.tv_usec = tv_usec;

    result = select((nfds+1), &read_fd, NULL, &excep_fd, &timeout);
    if (result < 0) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    
    /* Timeout */
    if (0 == result) {
        MPL_LOGV("IntProcess Timeout\n");
        return INV_SUCCESS;
    }

    for (ii = 0; ii < numHandles; ii++) {
        if (FD_ISSET(handles[ii], &read_fd)) {
            if (data != NULL) {
                size = sizeof(*data[0]);
            } else {
                size = 0;
            }
            numRead += read(handles[ii],data[ii],size);
        }
    }
    
    return numRead;
}

inv_error_t IntSetTimeout(int handle, int timeout)
{
    MPL_LOGV("Calling ioctl with %d, %d\n",
             MPUIRQ_SET_TIMEOUT,
             timeout);
    ioctl(handle,MPUIRQ_SET_TIMEOUT,timeout);
    return INV_SUCCESS;
}

/**
 *  @brief closes the interrupt handling.
 *  @return INV_SUCCESS or non-zero error code
 */
inv_error_t IntClose(int *handles, int numHandles)
{
    int ii;
    for (ii = 0; ii < numHandles; ii++) {
        close(handles[ii]);
    }

    return INV_SUCCESS;
}


/**
 * @}
 */
