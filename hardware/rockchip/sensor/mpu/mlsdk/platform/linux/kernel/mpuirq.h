/*
 $License:
    Copyright (C) 2010 InvenSense Corporation, All Rights Reserved.
 $
 */

#ifndef __MPUIRQ__
#define __MPUIRQ__

#ifdef __KERNEL__
#include <linux/i2c-dev.h>
#include <linux/time.h>
#include <linux/ioctl.h>
#include "mldl_cfg.h"
#else
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#define MPUIRQ_SET_TIMEOUT           _IOW(MPU_IOCTL, 0x40, unsigned long)
#define MPUIRQ_GET_INTERRUPT_CNT     _IOR(MPU_IOCTL, 0x41, unsigned long)
#define MPUIRQ_GET_IRQ_TIME          _IOR(MPU_IOCTL, 0x42, struct timeval)
#define MPUIRQ_SET_FREQUENCY_DIVIDER _IOW(MPU_IOCTL, 0x43, unsigned long)

#ifdef __KERNEL__
void mpuirq_exit(void);
int mpuirq_init(struct i2c_client *mpu_client, struct mldl_cfg *mldl_cfg);
#endif

#endif
