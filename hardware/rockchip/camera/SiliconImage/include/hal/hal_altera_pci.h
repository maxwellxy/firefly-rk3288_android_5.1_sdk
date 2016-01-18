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
 * @file hal_altera_pci.h
 *
 * @brief   Altera PCI variant of Hardware Abstraction Layer
 *
 *          This header file exports the register IO interface realized as an Altera
 *          PCI-Express board access inline function. You should use it for your
 *          PC implementation in combination with the Altera FPGA board.
 * @note    Do not include directly! Include hal_api.h instead with HAL_ALTERA defined.
 *
 *****************************************************************************/

#ifndef __HAL_ALTERA_PCI_H__
#define __HAL_ALTERA_PCI_H__

#include <fpga/altera_fpga.h>

#ifdef __cplusplus
extern "C"
{
#endif


#define HAL_NO_CAMS         3

/******************************************************************************
 * HAL device IDs
 *****************************************************************************/
#define HAL_DEVID_OCP       0x00000001  //!< HAL internal use only
#define HAL_DEVID_MARVIN    0x00000002  //!< MARVIN.*
#define HAL_DEVID_VDU       0x00000004  //!< VDU.clk + VDU.ocpclk
#define HAL_DEVID_PCLK      0x00000008  //!< VDU.pclk
#define HAL_DEVID_I2C_0     0x00000010  //!< HAL internal use only
#define HAL_DEVID_I2C_1     0x00000020  //!< HAL internal use only
#define HAL_DEVID_I2C_2     0x00000040  //!< HAL internal use only

#define HAL_DEVID_CAMREC_1A 0x00000080  //!< CAMREC1, not neccessarily the REC that CAM1 is hooked up to; see @ref HAL_DEVID_CAM_1_REC instead
#define HAL_DEVID_CAMREC_1B 0x00000100  //!< CAMREC1, not neccessarily the REC that CAM1 is hooked up to; see @ref HAL_DEVID_CAM_1_REC instead
#define HAL_DEVID_CAMREC_2  0x00000200  //!< CAMREC2, not neccessarily the REC that CAM2 is hooked up to; see @ref HAL_DEVID_CAM_2_REC instead
#define HAL_DEVID_INTERNAL  0x000003ff  //!< just internal devices included

#define HAL_DEVID_CAM_1A    0x01000000  //!< CAM1
#define HAL_DEVID_CAMPHY_1A 0x02000000  //!< CAMPHY1, not neccessarily the PHY that CAM1 is hooked up to; see @ref HAL_DEVID_CAM_1_PHY instead
#define HAL_DEVID_CAM_1B    0x04000000  //!< CAM1
#define HAL_DEVID_CAMPHY_1B 0x08000000  //!< CAMPHY1, not neccessarily the PHY that CAM1 is hooked up to; see @ref HAL_DEVID_CAM_1_PHY instead
#define HAL_DEVID_CAM_2     0x10000000  //!< CAM2
#define HAL_DEVID_CAMPHY_2  0x20000000  //!< CAMPHY2, not neccessarily the PHY that CAM2 is hooked up to; see @ref HAL_DEVID_CAM_2_PHY instead
#define HAL_DEVID_EXTERNAL  0x3F000000  //!< just external devices included

#define HAL_DEVID_ALL       (HAL_DEVID_INTERNAL | HAL_DEVID_EXTERNAL) //!< all devices included

#define HAL_DEVID_HIGHACTIVE_RESET_DEVICES 0x00000000 //!< HAL internal use only; bitwise or of all device masks of devices with high active resets.
#define HAL_DEVID_HIGHACTIVE_POWER_DEVICES 0x00000000 //!< HAL internal use only; bitwise or of all device masks of devices with high active power on select.
#define HAL_DEVID_RISING_CLKEDGE_DEVICES   0x00000000 //!< HAL internal use only; bitwise or of all device masks of devices with rising edge active clocks.

/******************************************************************************
 * HAL device base addresses
 *****************************************************************************/
// HalRegs:
#define HAL_BASEADDR_MARVIN    0x00000000 //!< Base address of MARVIN module.
#define HAL_BASEADDR_MARVIN_2  0x00010000 //!< Base address of MARVIN module of 2nd channel.
#define HAL_BASEADDR_MIPI_1A   0x00000000 //!< Base address of MIPI module.
#define HAL_BASEADDR_MIPI_1B   0x00000200 //!< Base address of MIPI module.
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
#define HAL_I2C_BUS_CAM_1A      0 //!< Num of I2C bus CAM1 is connected to
#define HAL_I2C_BUS_CAM_1B      1 //!< Num of I2C bus CAM1 is connected to
#define HAL_I2C_BUS_CAM_2       1 //!< Num of I2C bus CAM2 is connected to
#define HAL_I2C_BUS_HDMI_TX     2 //!< Num of I2C bus HDMI TX is connected to
#define HAL_I2C_BUS_CAMPHY_1A   0 //!< Num of I2C bus CAMPHY1 is connected to
#define HAL_I2C_BUS_CAMPHY_1B   1 //!< Num of I2C bus CAMPHY1 is connected to
//#define HAL_I2C_BUS_CAMPHY_1B   0 //!< Num of I2C bus CAMPHY1 is connected to
#define HAL_I2C_BUS_CAMPHY_2    1 //!< Num of I2C bus CAMPHY2 is connected to


/******************************************************************************
 * HAL CAM to PHY/REC mapping
 *****************************************************************************/
#if 0
#define HAL_DEVID_CAM_1A_PHY    HAL_DEVID_CAMPHY_1A //!< DEVID of CAMPHY CAM1 is connected to
#define HAL_DEVID_CAM_1B_PHY    HAL_DEVID_CAMPHY_1A //!< DEVID of CAMPHY CAM1 is connected to
#define HAL_DEVID_CAM_2_PHY     HAL_DEVID_CAMPHY_2  //!< DEVID of CAMPHY CAM2 is connected to
#define HAL_DEVID_CAM_1A_REC    HAL_DEVID_CAMREC_1A //!< DEVID of CAMREC CAM1 is connected to
#define HAL_DEVID_CAM_1B_REC    HAL_DEVID_CAMREC_1A //!< DEVID of CAMREC CAM1 is connected to
#define HAL_DEVID_CAM_2_REC     HAL_DEVID_CAMREC_2  //!< DEVID of CAMREC CAM2 is connected to
#endif

#define HAL_DEVID_CAM_1A_PHY    HAL_DEVID_CAMPHY_1A //!< DEVID of CAMPHY CAM1 is connected to
#define HAL_DEVID_CAM_1B_PHY    HAL_DEVID_CAMPHY_1B //!< DEVID of CAMPHY CAM1 is connected to
#define HAL_DEVID_CAM_2_PHY     HAL_DEVID_CAMPHY_2  //!< DEVID of CAMPHY CAM2 is connected to
#define HAL_DEVID_CAM_1A_REC    HAL_DEVID_CAMREC_1A //!< DEVID of CAMREC CAM1 is connected to
#define HAL_DEVID_CAM_1B_REC    HAL_DEVID_CAMREC_1B //!< DEVID of CAMREC CAM1 is connected to
#define HAL_DEVID_CAM_2_REC     HAL_DEVID_CAMREC_2  //!< DEVID of CAMREC CAM2 is connected to

/******************************************************************************
 * HalReadReg()
 *****************************************************************************/
INLINE uint32_t HalReadReg( HalHandle_t HalHandle, ulong_t reg_address )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    return AlteraFPGABoard_ReadReg( reg_address );
}


/******************************************************************************
 * HalWriteReg()
 *****************************************************************************/
INLINE void HalWriteReg( HalHandle_t HalHandle, ulong_t reg_address, uint32_t value )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    (void) AlteraFPGABoard_WriteReg( reg_address, value );
}


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

    return AlteraFPGABoard_ReadBAR( HAL_BASEREGION_SYSCTRL, FPGA_REG_ADDRESS_MOD(reg_address) );
}


/******************************************************************************
 * HalWriteSysReg()
 *****************************************************************************/
INLINE void HalWriteSysReg( HalHandle_t HalHandle, ulong_t reg_address, uint32_t value )
{
    (void) HalHandle;
    DCT_ASSERT(HalHandle != NULL);

    (void) AlteraFPGABoard_WriteBAR( HAL_BASEREGION_SYSCTRL, FPGA_REG_ADDRESS_MOD(reg_address), value );
}


#ifdef __cplusplus
}
#endif

#endif /* __HAL_ALTERA_PCI_H__ */
