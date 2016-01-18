/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: mlsetup.h 6241 2011-10-28 20:54:58Z mcaramello $
 *
 *******************************************************************************/

#ifndef MLSETUP_H
#define MLSETUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpu.h"
#include "mltypes.h"

    enum mpu_platform_id {
        PLATFORM_ID_INVALID = ID_INVALID,   // 0
        PLATFORM_ID_MSB,                    // (0x0001) MSB (Multi sensors board)
        PLATFORM_ID_ST_6AXIS,               // (0x0002) 6 Axis with ST accelerometer
        PLATFORM_ID_DONGLE,                 // (0x0003) 9 Axis USB dongle with
        PLATFORM_ID_MPU6050_PROTOTYPE,      // (0x0004) MPU6050 prototype board
        PLATFORM_ID_MPU6050_MSB,            // (0x0005) MSB with MPU6050
        PLATFORM_ID_MPU6050_USB_DONGLE,     // (0x0006) MPU6050 and AKM on USB dongle.
        PLATFORM_ID_MSB_10AXIS,             // (0x0007) MSB with pressure sensor
        PLATFORM_ID_MPU9150_PROTOTYPE,      // (0x0008) MPU9150 prototype board
        PLATFORM_ID_MSB_V2,                 // (0x0009) Version 2 MSB
        PLATFORM_ID_MSB_V2_MPU6050,         // (0x000A) Version 2 MSB with MPU6050
        PLATFORM_ID_MPU6050_EVB,            // (0x000B) MPU6050 EVB (shipped to cust.)
        PLATFORM_ID_MPU9150_USB_DONGLE,     // (0x000C) MPU9150 USB Dongle with MPU6050 rev C
        PLATFORM_ID_MSB_EVB,                // (0X000D) MSB with 3050.

        NUM_PLATFORM_IDS
    };

/* main entry APIs */
inv_error_t SetupPlatform(unsigned short platformId,
                          unsigned short accelSelection,
                          unsigned short compassSelection);

#ifdef __cplusplus
}
#endif

#endif /* MLSETUP_H */
