/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: helper.c 4367 2010-12-21 03:02:55Z prao $
 *
 *******************************************************************************/

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif
#ifdef LINUX
#include <sys/select.h>
#endif
#include <time.h>
#include <string.h>

#include "ml.h"
#include "slave.h"
#include "mldl.h"
#include "mltypes.h"
#include "mlstates.h"
#include "compass.h"

#include "mlsl.h"
#include "ml.h"

#include "helper.h"
#include "mlsetup.h"
#include "fopenCMake.h"
#include "int.h"
#include "mlos.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-helper"

#ifdef AIO
extern inv_error_t MLSLSetYamahaCompassDataMode(unsigned char mode);
#endif

// Keyboard hit function
int ConsoleKbhit(void)
{
#ifdef _WIN32
    return _kbhit();
#else
    struct timeval tv;
    fd_set read_fd;

    tv.tv_sec=0;
    tv.tv_usec=0;
    FD_ZERO(&read_fd);
    FD_SET(0,&read_fd);

    if(select(1, &read_fd, NULL, NULL, &tv) == -1)
        return 0;

    if(FD_ISSET(0,&read_fd))
        return 1;

    return 0;
#endif
}

char ConsoleGetChar(void) {
#ifdef _WIN32
    return _getch();
#else
    return getchar();
#endif
}
struct mpuirq_data** InterruptPoll(int *handles, int numHandles, long tv_sec, long tv_usec)
{
    struct mpuirq_data **data;
    void *tmp;
    int ii;
    const int irq_data_size = sizeof(**data) * numHandles + 
        sizeof(*data) * numHandles;

    tmp = (void *)inv_malloc(irq_data_size);
    memset(tmp, 0, irq_data_size);
    data = (struct mpuirq_data **)tmp;
    for (ii = 0; ii < numHandles; ii++) {
        data[ii] = (struct mpuirq_data *)((unsigned long)tmp +
            (sizeof(*data) * numHandles) + sizeof(**data) * ii);
    }

    if (IntProcess(handles, numHandles, data, tv_sec, tv_usec) > 0) {
        for (ii = 0; ii < numHandles; ii++) {
            if (data[ii]->interruptcount) {
                inv_interrupt_handler(ii);
            }
        }
    }
    
    /* Return data incase the application needs to look at the timestamp or
       other part of the data */
    return data;
}

void InterruptPollDone(struct mpuirq_data ** data)
{
    inv_free(data);
}
