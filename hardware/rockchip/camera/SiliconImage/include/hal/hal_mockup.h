/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file hal_mockup.h
 *
 * @brief   MockedUp variant of Hardware Abstraction Layer
 *
 *          Exports the mockup version of the HAL API's inline functions. You could
 *          use it for your PC implementation to check general interaction of your
 *          code with the HAL.
 * @note    Do not include directly! Include hal_api.h instead with HAL_MOCKUP defined.
 *
 *****************************************************************************/

#ifndef __HAL_MOCKUP_H__
#define __HAL_MOCKUP_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "camsys_head.h"
#include <common/list.h>


/******************************************************************************
 * HAL device IDs
 *****************************************************************************/

#define HAL_DEVID_MARVIN    CAMSYS_DEVID_MARVIN//!< MARVIN.*
/// ddl for compile  begin
#define HAL_DEVID_VDU       0x00000004  //!< VDU.clk + VDU.ocpclk
#define HAL_DEVID_PCLK      0x00000008  //!< VDU.pclk
/// ddl for compile  end
#define HAL_DEVID_INTERNAL  CAMSYS_DEVID_INTERNAL  //!< just internal devices included

#define HAL_DEVID_CAM_1A    CAMSYS_DEVID_SENSOR_1A//!< CAM1
#define HAL_DEVID_CAM_1B    CAMSYS_DEVID_SENSOR_1B
#define HAL_DEVID_CAM_2     CAMSYS_DEVID_SENSOR_2
#define HAL_DEVID_EXTERNAL  CAMSYS_DEVID_EXTERNAL  //!< just external devices included


#define HAL_DEVID_ALL       (HAL_DEVID_INTERNAL | HAL_DEVID_EXTERNAL) //!< all devices included


/******************************************************************************
 * HAL device base addresses
 *****************************************************************************/
// HalRegs:
#define HAL_BASEADDR_MARVIN    0x00000000 //!< Base address of MARVIN module.
#define HAL_BASEADDR_MARVIN_2  0x00010000 //!< Base address of MARVIN module of 2nd channel.
#define HAL_BASEADDR_MIPI_1    0x00000000 //!< Base address of MIPI module.
#define HAL_BASEADDR_MIPI_2    0x00010000 //!< Base address of MIPI module of 2nd channel.
#define HAL_BASEADDR_VDU       0x00008000 //!< Base address of VDU module.

// HalSysRegs:
#define HAL_BASEADDR_SYSCTRL   0x00000000 //!< HAL internal use only
#define HAL_BASEADDR_I2C_0     0x00001000 //!< HAL internal use only
#define HAL_BASEADDR_I2C_1     0x00001800 //!< HAL internal use only
#define HAL_BASEADDR_I2C_2     0x00000800 //!< HAL internal use only


/******************************************************************************
 * HAL device base region
 *****************************************************************************/
// HalRegs:
#define HAL_BASEREGION_MARVIN    0 //!< Base region of MARVIN module(s).
#define HAL_BASEREGION_MIPI      0 //!< Base region of MIPI module(s).
#define HAL_BASEREGION_VDU       0 //!< Base region of VDU module.

// HalSysRegs:
#define HAL_BASEREGION_SYSCTRL   2 //!< HAL internal use only
#define HAL_BASEREGION_I2C       2 //!< HAL internal use only


/******************************************************************************
 * HAL I2C bus location
 *****************************************************************************/
#define HAL_I2C_BUS_CAM_1       0 //!< Num of I2C bus CAM1 is connected to
#define HAL_I2C_BUS_CAM_2       1 //!< Num of I2C bus CAM2 is connected to
#define HAL_I2C_BUS_HDMI_TX     2 //!< Num of I2C bus HDMI TX is connected to
#define HAL_I2C_BUS_CAMPHY_1    0 //!< Num of I2C bus CAMPHY1 is connected to
#define HAL_I2C_BUS_CAMPHY_2    1 //!< Num of I2C bus CAMPHY1 is connected to
/// ddl for compile  begin
#define HAL_I2C_BUS_CAM_1A      3
#define HAL_I2C_BUS_CAM_1B      1
/// ddl for compile  end

/******************************************************************************
 * HAL CAM to PHY/REC mapping
 *****************************************************************************/
#define HAL_DEVID_CAM_1_PHY     HAL_DEVID_CAMPHY_1 //!< DEVID of CAMPHY CAM1 is connected to
#define HAL_DEVID_CAM_2_PHY     HAL_DEVID_CAMPHY_1 //!< DEVID of CAMPHY CAM2 is connected to
#define HAL_DEVID_CAM_1_REC     HAL_DEVID_CAMREC_1 //!< DEVID of CAMREC CAM1 is connected to
#define HAL_DEVID_CAM_2_REC     HAL_DEVID_CAMREC_1 //!< DEVID of CAMREC CAM2 is connected to

extern bool_t	gIsNewIon;

/******************************************************************************
 * HalReadMaskedReg()
 *****************************************************************************/
INLINE uint32_t HalReadMaskedReg( HalHandle_t HalHandle, ulong_t reg_address, uint32_t reg_mask, uint32_t shift_mask )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    uint32_t tmp_value = HalReadReg( HalHandle, reg_address );
    return HalGetMaskedValue( tmp_value, reg_mask, shift_mask );
}


/******************************************************************************
 * HalWriteMaskedReg()
 *****************************************************************************/
INLINE void HalWriteMaskedReg( HalHandle_t HalHandle, ulong_t reg_address, uint32_t reg_mask, uint32_t shift_mask, uint32_t value )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    uint32_t tmp_value = HalReadReg( HalHandle, reg_address );
    tmp_value = HalSetMaskedValue( tmp_value, reg_mask, shift_mask, value );
    HalWriteReg( HalHandle, reg_address, tmp_value );
}


/******************************************************************************
 * HalReadSysReg()
 *****************************************************************************/
INLINE uint32_t HalReadSysReg( HalHandle_t HalHandle, ulong_t reg_address )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    return 0;
}


/******************************************************************************
 * HalWriteSysReg()
 *****************************************************************************/
INLINE void HalWriteSysReg( HalHandle_t HalHandle, ulong_t reg_address, uint32_t value )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    (void) reg_address;
    (void) value;
}


#ifdef __cplusplus
}
#endif

#endif /* __HAL_MOCKUP_H__ */
