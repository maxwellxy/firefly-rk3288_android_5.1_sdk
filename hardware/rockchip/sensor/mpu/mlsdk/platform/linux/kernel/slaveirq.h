/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

#ifndef __SLAVEIRQ__
#define __SLAVEIRQ__

#include <linux/i2c-dev.h>

#include <linux/mpu.h>
#include "mpuirq.h"

#define SLAVEIRQ_SET_TIMEOUT           _IOW(MPU_IOCTL, 0x50, unsigned long)
#define SLAVEIRQ_GET_INTERRUPT_CNT     _IOR(MPU_IOCTL, 0x51, unsigned long)
#define SLAVEIRQ_GET_IRQ_TIME          _IOR(MPU_IOCTL, 0x52, unsigned long)

void slaveirq_exit(struct ext_slave_platform_data *pdata);
int slaveirq_init(struct i2c_adapter *slave_adapter,
		  struct ext_slave_platform_data *pdata, char *name);

#endif
