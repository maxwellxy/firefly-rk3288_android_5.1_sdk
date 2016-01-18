/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

#ifndef __TIMERIRQ__
#define __TIMERIRQ__

#include <linux/mpu.h>

#define TIMERIRQ_SET_TIMEOUT           _IOW(MPU_IOCTL, 0x60, unsigned long)
#define TIMERIRQ_GET_INTERRUPT_CNT     _IOW(MPU_IOCTL, 0x61, unsigned long)
#define TIMERIRQ_START                 _IOW(MPU_IOCTL, 0x62, unsigned long)
#define TIMERIRQ_STOP                  _IO(MPU_IOCTL, 0x63)

#endif
