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
 * @file OV14825_MIPI_priv.h
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#ifndef __OV14825_MIPI_PRIV_H__
#define __OV14825_MIPI_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>



#ifdef __cplusplus
extern "C"
{
#endif

/*v0.1.0:
*   1).add senosr drv version in get sensor i2c info func
*v0.2.0:
*   1). support for isi v0.5.0
*/
#define CONFIG_SENSOR_DRV_VERSION KERNEL_VERSION(0, 2, 0) 


/*****************************************************************************
 * unknown registers (but set by evaluation platform)
 *****************************************************************************/
#define OV14825_301B                       (0x301B)
#define OV14825_301C                       (0x301C)
#define OV14825_3020                       (0x3020)
#define OV14825_3106                       (0x3106)             // needed for lane switching ???
#define OV14825_3600                       (0x3600)
#define OV14825_3601                       (0x3601)
#define OV14825_3602                       (0x3602)
#define OV14825_3604                       (0x3604)
#define OV14825_3605                       (0x3605)
#define OV14825_3609                       (0x3609)             // Bit[3]: 1 turn of internal regulator
#define OV14825_360A                       (0x360A)
#define OV14825_360B                       (0x360B)
#define OV14825_360C                       (0x360C)
#define OV14825_360D                       (0x360D)
#define OV14825_360F                       (0x360F)
#define OV14825_3611                       (0x3611)
#define OV14825_3613                       (0x3613)
#define OV14825_3614                       (0x3614)             // binning sum/ binning avarage Bit [7:4]
#define OV14825_3702                       (0x3702)
#define OV14825_3704                       (0x3704)
#define OV14825_3705                       (0x3705)
#define OV14825_3707                       (0x3707)
#define OV14825_3708                       (0x3708)
#define OV14825_370A                       (0x370A)
#define OV14825_370B                       (0x370B)
#define OV14825_370C                       (0x370C)
#define OV14825_370E                       (0x370E)
#define OV14825_370F                       (0x370F)
#define OV14825_3710                       (0x3710)
#define OV14825_3711                       (0x3711)
#define OV14825_3713                       (0x3713)
#define OV14825_3714                       (0x3714)
#define OV14825_3715                       (0x3715)
#define OV14825_3716                       (0x3716)
#define OV14825_3717                       (0x3717)
#define OV14825_3718                       (0x3718)
#define OV14825_3719                       (0x3719)
#define OV14825_371B                       (0x371B)
#define OV14825_371C                       (0x371C)
#define OV14825_371D                       (0x371D)
#define OV14825_371E                       (0x371E)
#define OV14825_3721                       (0x3721)
#define OV14825_3723                       (0x3723)             // 0x3B DPC off, 0x39 DPC on
#define OV14825_3724                       (0x3724)
#define OV14825_3725                       (0x3725)
#define OV14825_3726                       (0x3726)
#define OV14825_3727                       (0x3727)
#define OV14825_3728                       (0x3728)
#define OV14825_3803                       (0x3803)
#define OV14825_3808                       (0x3808)
#define OV14825_3811                       (0x3811)
#define OV14825_380A                       (0x380A)
#define OV14825_3817                       (0x3817)
#define OV14825_3819                       (0x3819)
#define OV14825_382C                       (0x382C)
#define OV14825_382D                       (0x382D)
#define OV14825_3A1A                       (0x3A1A)
#define OV14825_3A25                       (0x3A25)
#define OV14825_3B09                       (0x3B09)
#define OV14825_4002                       (0x4002)
#define OV14825_4004                       (0x4004)
#define OV14825_4005                       (0x4005)
#define OV14825_4009                       (0x4009)
#define OV14825_404F                       (0x404F)
#define OV14825_4050                       (0x4050)
#define OV14825_4051                       (0x4051)
#define OV14825_4053                       (0x4053)             // Bit[3] enable/disable binning
#define OV14825_4709                       (0x4709)
#define OV14825_4801                       (0x4801)
#define OV14825_4806                       (0x4806)
#define OV14825_4837                       (0x4837)
#define OV14825_4842                       (0x4842)
#define OV14825_503B                       (0x503B)
#define OV14825_503C                       (0x503c)
#define OV14825_5042                       (0x5042)
#define OV14825_5047                       (0x5047)
#define OV14825_5780                       (0x5780)
#define OV14825_5B00                       (0x5B00)
#define OV14825_5B01                       (0x5B01)
#define OV14825_5B03                       (0x5B03)



/*****************************************************************************
 * unknown registers default values (but set by evaluation platform)
 *****************************************************************************/
#define OV14825_301B_DEFAULT               (0xE0) //       (0xF0)
#define OV14825_301C_DEFAULT               (0xF8) //       (0xF0)
#define OV14825_3020_DEFAULT               (0x01) //       (0x21)
#define OV14825_3106_DEFAULT               (0x09) //       (0x05) note: we've only 1 lane => 0x09 per default
#define OV14825_3600_DEFAULT               (0x2D) //       (0x24)
#define OV14825_3601_DEFAULT               (0x1F) //       (0x08)
#define OV14825_3602_DEFAULT               (0x42) //       (0x57)
#define OV14825_3604_DEFAULT               (0x9C) //(0x80) (0x90)
#define OV14825_3605_DEFAULT               (0x30) //(0x11)
#define OV14825_3609_DEFAULT               (0x00) //       (0x00)
#define OV14825_360A_DEFAULT               (0x2E) //       (0x3D)
#define OV14825_360B_DEFAULT               (0x00) //
#define OV14825_360C_DEFAULT               (0x44) //       (0x42)
#define OV14825_360D_DEFAULT               (0x23) //(0x13) (0x33)
#define OV14825_360F_DEFAULT               (0x24) //       (0x04)
#define OV14825_3611_DEFAULT               (0x6C) //       (0x7C)
#define OV14825_3613_DEFAULT               (0x84) //       (0x04)
#define OV14825_3614_DEFAULT               (0x35) //(0x0A) (0x0F)
#define OV14825_3702_DEFAULT               (0x20) //(0x21)
#define OV14825_3704_DEFAULT               (0x28) //(0x0A)
#define OV14825_3705_DEFAULT               (0xD1) //
#define OV14825_3707_DEFAULT               (0x73) //(0x13)
#define OV14825_3708_DEFAULT               (0x01) //
#define OV14825_370A_DEFAULT               (0x81) //(0x80)
#define OV14825_370B_DEFAULT               (0x20) //(0x00)
#define OV14825_370C_DEFAULT               (0x84) //(0x24)
#define OV14825_370E_DEFAULT               (0x04) //
#define OV14825_370F_DEFAULT               (0x00) //(0x61)
#define OV14825_3710_DEFAULT               (0x40) //
#define OV14825_3711_DEFAULT               (0x1C) //
#define OV14825_3713_DEFAULT               (0xA8) //(0xA4)
#define OV14825_3714_DEFAULT               (0x5F) //(0x27)
#define OV14825_3715_DEFAULT               (0x58) //(0x27)
#define OV14825_3716_DEFAULT               (0x17) //(0x80)
#define OV14825_3717_DEFAULT               (0x80) //
#define OV14825_3718_DEFAULT               (0x11) //
#define OV14825_3719_DEFAULT               (0x11) //
#define OV14825_371B_DEFAULT               (0xA0) //
#define OV14825_371C_DEFAULT               (0x46) //(0x76)
#define OV14825_371D_DEFAULT               (0x40) //(0x26)
#define OV14825_371E_DEFAULT               (0x2C) //
#define OV14825_3721_DEFAULT               (0x28) //(0x22)
#define OV14825_3723_DEFAULT               (0x30) //
#define OV14825_3724_DEFAULT               (0x2E) //(0x64)
#define OV14825_3725_DEFAULT               (0x2E) //(0x27)
#define OV14825_3726_DEFAULT               (0x70) //
#define OV14825_3727_DEFAULT               (0x62) //(0x66)
#define OV14825_3728_DEFAULT               (0x82) //(0x0C)
#define OV14825_3803_DEFAULT               (0x07) //(0x0D)
#define OV14825_3808_DEFAULT               (0x00) //
#define OV14825_380A_DEFAULT               (0x00) //
#define OV14825_3811_DEFAULT               (0x02) //(0x96)
#define OV14825_3817_DEFAULT               (0x24) //
#define OV14825_3819_DEFAULT               (0x80) //
#define OV14825_382C_DEFAULT               (0x02) //
#define OV14825_382D_DEFAULT               (0x01) //
#define OV14825_3A1A_DEFAULT               (0x06) //
#define OV14825_3A25_DEFAULT               (0x83) //
#define OV14825_3B09_DEFAULT               (0x0A) //
#define OV14825_4002_DEFAULT               (0xC5) //
#define OV14825_4004_DEFAULT               (0x02) //
#define OV14825_4005_DEFAULT               (0x10) //
#define OV14825_4009_DEFAULT               (0x40) //
#define OV14825_404F_DEFAULT               (0xFF) //
#define OV14825_4050_DEFAULT               (0xC1) //(0xC0)
#define OV14825_4051_DEFAULT               (0x80) //(0x00)
#define OV14825_4053_DEFAULT               (0xA5) //(0x81)
#define OV14825_4709_DEFAULT               (0x00) //
#define OV14825_4801_DEFAULT               (0x0F) //
#define OV14825_4806_DEFAULT               (0x80) //
#define OV14825_4837_DEFAULT               (0x1B) //(0x15)
#define OV14825_4842_DEFAULT               (0x01) //
#define OV14825_503B_DEFAULT               (0x01) //
#define OV14825_503C_DEFAULT               (0x10) //
#define OV14825_5042_DEFAULT               (0x21) //
#define OV14825_5047_DEFAULT               (0xF0) //
#define OV14825_5780_DEFAULT               (0xFC) //
#define OV14825_5B00_DEFAULT               (0x10) //
#define OV14825_5B01_DEFAULT               (0x5B) //
#define OV14825_5B03_DEFAULT               (0x00) //




/*****************************************************************************
 * System control registers
 *****************************************************************************/
#define OV14825_SMIA_R0100                 (0x0100)             //RW - Bit[7:1]: Not used
                                                                //     Bit[0]: SMIA
                                                                //         0: software_standby
                                                                //         1: Streaming
#define OV14825_SMIA_R0103                 (0x0103)             //RW - Bit[7:1]: Not used
                                                                //RW - Bit[0]: software_reset
#define OV14825_PAD_OEN2                   (0x3000)             //RW - Bit[7]: io_y_oen[5]
                                                                //     Bit[6]: io_y_oen[4]
                                                                //     Bit[5]: io_y_oen[3]
                                                                //     Bit[4]: io_y_oen[2]
                                                                //     Bit[3]: io_y_oen[1]
                                                                //     Bit[2]: io_y_oen[0]
                                                                //     Bit[1:0]: Not used
#define OV14825_PAD_OUT0                   (0x3001)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3]: io_y_o[11]
                                                                //     Bit[2]: io_y_o[10]
                                                                //     Bit[1]: io_strobe_o
                                                                //     Bit[0]: io_sda_o
#define OV14825_SCCB_ID                    (0x3002)             //RW - Bit[7:0]: sccb_id
#define OV14825_PLL_CTRL4                  (0x3003)             //RW - Bit[7:2]: PLL parameters
                                                                //     Bit[1:0]: mipi_line_sel
                                                                //         00: 1 line
                                                                //         01: 2 line
                                                                //         10: 4 line
                                                                //         11: 1 line
#define OV14825_PLL_CTRL3                  (0x3004)             //RW - Bit[7:0]: PLL parameters
#define OV14825_PLL_CTRL2                  (0x3005)             //RW - Bit[7:0]: PLL parameters
#define OV14825_PLL_CTRL1                  (0x3006)             //RW - Bit[7:0]: PLL parameters
#define OV14825_PLL_CTRL0                  (0x3007)             //RW - Bit[7:0]: PLL parameters
#define OV14825_SC_PAD_OEN0                (0x3008)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3]: io_y_oen[11]
                                                                //     Bit[2]: io_y_oen[10]
                                                                //     Bit[1]: io_strobe_oen
                                                                //     Bit[0]: io_sda_oen
#define OV14825_SC_PAD_OEN1                (0x3009)             //RW - Bit[7]: io_frex_oen
                                                                //     Bit[6]: io_vsync_oen
                                                                //     Bit[5]: io_href_oen
                                                                //     Bit[4]: io_pclk_oen
                                                                //     Bit[3]: io_y_oen[9]
                                                                //     Bit[2]: io_y_oen[8]
                                                                //     Bit[1]: io_y_oen[7]
                                                                //     Bit[0]: io_y_oen[6]
#define OV14825_CHIP_ID_HIGH_BYTE          (0x300A)             //R -  Bit[7:0]: Chip ID high byte
#define OV14825_CHIP_ID_LOW_BYTE           (0x300B)             //R -  Bit[7:0]: Chip ID low byte
#define OV14825_SC_PAD_OUT1                (0x300C)             //RW - Bit[7]: io_frex_o
                                                                //     Bit[6]: io_vsync_o
                                                                //     Bit[5]: io_href_o
                                                                //     Bit[4]: io_pclk_o
                                                                //     Bit[3]: io_y_o[9]
                                                                //     Bit[2]: io_y_o[8]
                                                                //     Bit[1]: io_y_o[7]
                                                                //     Bit[0]: io_y_o[6]
#define OV14825_SC_PAD_OUT2                (0x300D)             //RW - Bit[7]: io_y_o[5]
                                                                //     Bit[6]: io_y_o[4]
                                                                //     Bit[5]: io_y_o[3]
                                                                //     Bit[4]: io_y_o[2]
                                                                //     Bit[3]: io_y_o[1]
                                                                //     Bit[2]: io_y_o[0]
                                                                //     Bit[1:0]: Not used
#define OV14825_SC_PAD_SEL0                (0x300E)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3]: io_y_sel[11]
                                                                //     Bit[2]: io_y_sel[10]
                                                                //     Bit[1]: io_strobe_sel
                                                                //     Bit[0]: io_sda_sel
#define OV14825_SC_PAD_SEL1                (0x300F)             //RW - Bit[7]: io_frex_sel
                                                                //     Bit[6]: io_vsync_sel
                                                                //     Bit[5]: io_href_sel
                                                                //     Bit[4]: io_pclk_sel
                                                                //     Bit[3]: io_y_sel[9]
                                                                //     Bit[2]: io_y_sel[8]
                                                                //     Bit[1]: io_y_sel[7]
                                                                //     Bit[0]: io_y_sel[6]
#define OV14825_SC_PAD_SEL2                (0x3010)             //RW - Bit[7]: io_y_sel[5]
                                                                //     Bit[6]: io_y_sel[4]
                                                                //     Bit[5]: io_y_sel[3]
                                                                //     Bit[4]: io_y_sel[2]
                                                                //     Bit[3]: io_y_sel[1]
                                                                //     Bit[2]: io_y_sel[0]
                                                                //     Bit[1:0]: Not used
#define OV14825_SC_PLL_CTRL_S2             (0x3011)             //RW - Bit[7:0]: PLL parameters
#define OV14825_SC_PLL_CTRL_S1             (0x3012)             //RW - Bit[7:0]: PLL parameters
#define OV14825_SC_PLL_CTRL_S0             (0x3013)             //RW - Bit[7:0]: PLL parameters
// (0x3014 - 0x3017) reserved
#define OV14825_SC_MIPI_SC_CTRL            (0x3018)             //RW - Bit[7:5]: Not used
                                                                //     Bit[4:3]: MIPI PHY power down
                                                                //         00: Normal
                                                                //         11: Power down
                                                                //     Bit[2] mipi_en
                                                                //         0: LVDS enable
                                                                //         1: MIPI enable
                                                                //     Bit[1:0]: Not used


/*****************************************************************************
 * SRB registers
 *****************************************************************************/
#define OV14825_GROUP1_START_ADDRESS       (0x3200)             //RW - Bit[7:0]: Start address[7:0]
                                                                //       For example, 0x04 means start address is 0x40
#define OV14825_GROUP2_START_ADDRESS       (0x3201)             //RW - Bit[7:0]: Start address[7:0]
                                                                //       For example, 0x04 means start address is 0x40
#define OV14825_GROUP3_START_ADDRESS       (0x3202)             //RW - Bit[7:0]: Start address[7:0]
                                                                //       For example, 0x04 means start address is 0x40
#define OV14825_GROUP4_START_ADDRESS       (0x3203)             //RW - Bit[7:0]: Start address[7:0]
                                                                //       For example, 0x04 means start address is 0x40
// (0x3204~0x3211) reserved
#define OV14825_GROUP_ACCESS               (0x3212)             //W - Group Access
                                                                //     Bit[7]: Group latch enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[6]: Debug mode (must be 0)
                                                                //     Bit[5]: Group launch
                                                                //     Bit[4]: Group hold end
                                                                //     Bit[3:2]: Not used
                                                                //     Bit[1:0]: Group ID
                                                                //         00: Group 1 register access
                                                                //         01: Group 2 register access
                                                                //         10: Group 3 register access
                                                                //         11: Group 4 register access
#define OV14825_GROUP_STATUS               (0x3213)             //R - Group Status
                                                                //     Bit[7:6]: Not used
                                                                //     Bit[5]: Group hold
                                                                //     Bit[4]: Group launch
                                                                //     Bit[3]: Group write
                                                                //     Bit[2:0]: Group select



/*****************************************************************************
 * manual AEC/AGC registers
 *****************************************************************************/
#define OV14825_AEC_EXPO_2                 (0x3500)             //RW - AEC Peak Exposure
                                                                //     Bit[7:4]: Not used
                                                                //     Bit[3:0]: Exposure[19:16]
#define OV14825_AEC_EXPO_1                 (0x3501)             //RW - AEC Peak Exposure
                                                                //     Bit[7:0]: Exposure[15:8]
#define OV14825_AEC_EXPO_0                 (0x3502)             //RW - AEC Peak Exposure
                                                                //     Bit[7:0]: Exposure[7:0]
#define OV14825_AEC_MANUAL                 (0x3503)             //RW - AEC Manual Mode Control
                                                                //     Bit[7:2]: Not used
                                                                //     Bit[1]: AGC manual enable
                                                                //         0: Auto enable
                                                                //         1: Manual enable
                                                                //     Bit[0]: AEC manual enable
                                                                //         0: Auto enable
                                                                //         1: Manual enable
// (0x3504~0x3509) reserved
#define OV14825_AEC_AGC_ADJ_2              (0x350A)             //RW - Gain Output to Sensor
                                                                //     Bit[7:1]: Not used
                                                                //     Bit[0]: Gain high bit
                                                                //             Gain = (0x350A[0]+1) ?? (0x350B[7]+1) ??
                                                                //             (0x350B[6]+1) ?? (0x350B[5]+1) ??
                                                                //             (0x350B[4]+1) ?? (0x350B[3:0]/16+1)
#define OV14825_AEC_AGC_ADJ_1              (0x350B)             //RW - Gain Output to Sensor
                                                                //     Bit[7:0]: Gain low bits
                                                                //               Gain = (0x350A[0]+1) ?? (0x350B[7]+1) ??
                                                                //               (0x350B[6]+1) ?? (0x350B[5]+1) ??
                                                                //               (0x350B[4]+1) ?? (0x350B[3:0]/16+1)
// (0x350C~0x350F) reserved

/*****************************************************************************
 * sensor control registers
 *****************************************************************************/
#define OV14825_ARRAY_CONTROL_0D           (0x370D)             //RW - Bit[7]: Not used
                                                                //     Bit[6]: Horizontal binning
                                                                //     Bit[5]: Vertical binning
                                                                //     Bit[4:0]: Not used



/*****************************************************************************
 * timing control registers
 *****************************************************************************/
#define OV14825_TIMING_HW_2                (0x3804)             //RW - Bit[7:5]: Not used
                                                                //     Bit[4:0]: HREF horizontal width[12:8]
#define OV14825_TIMING_HW_1                (0x3805)             //RW - Bit[7:0]: HREF horizontal width[7:0]
#define OV14825_TIMING_VH_2                (0x3806)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: HREF vertical height[11:8]
#define OV14825_TIMING_VH_1                (0x3807)             //RW - Bit[7:0]: HREF vertical height[7:0]
#define OV14825_TIMING_HOFFS               (0x3810)             //RW - Timing Control Register
                                                                //     Bit[7:4]: Horizontal offset
                                                                //     Bit[3:0]: Vertical offset
// (0x3811~0x3816) reserved
#define OV14825_TIMING_TC_REG18            (0x3818)             //RW - Timing Control Register
                                                                //     Bit[7]: Not used
                                                                //     Bit[6]: Horizontal mirror
                                                                //     Bit[5]: Vertical flip
                                                                //     Bit[4:0]: Not used
// 0x3819 reserved
#define OV14825_TIMING_HTS_2               (0x380C)             //RW - Bit[7:5]: Not used
                                                                //     Bit[4:0]: Total horizontal size[12:8]
#define OV14825_TIMING_HTS_1               (0x380D)             //RW - Bit[7:0]: Total horizontal size[7:0]
#define OV14825_TIMING_VTS_2               (0x380E)             //RW - Bit[7:0]: Total vertical size[15:8]
#define OV14825_TIMING_VTS_1               (0x380F)             //RW - Bit[7:0]: Total vertical size[7:0]
// (0x381A~0x381B) reserved
#define OV14825_TIMING_CONTROL_1C          (0x381C)             //RW - Bit[7:5]: Not used
                                                                //     Bit[4]: Cropping enable
                                                                //     Bit[3:0]: VS[11:8]
#define OV14825_TIMING_CONTROL_1D          (0x381D)             //RW - Bit[7:0]: VS[7:0]
#define OV14825_TIMING_CONTROL_1E          (0x381E)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: VH[11:8]
#define OV14825_TIMING_CONTROL_1F          (0x381F)             //RW - Bit[7:0]: VH[7:0]
#define OV14825_TIMING_CONTROL_20          (0x3820)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: HS[11:8]
#define OV14825_TIMING_CONTROL_21          (0x3821)             //RW - Bit[7:0]: HS[7:0]



/*****************************************************************************
 * AEC/AGC control registers
 *****************************************************************************/
#define OV14825_AEC_CTRL00                 (0x3A00)             //RW - Bit[7:6]: Not used
                                                                //     Bit[5]: Band enable
                                                                //         0: Band function disable
                                                                //         1: Band function enable
                                                                //     Bit[4]: Auto band enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[3]: Line complete
                                                                //     Bit[2]: Night mode
                                                                //         0: Night mode disable
                                                                //         1: Night mode enable
                                                                //     Bit[1]: Not used
                                                                //     Bit[0]: Freeze
                                                                //         0: Freeze disable
                                                                //         1: Freeze enable
#define OV14825_AEC_CTRL01                 (0x3A01)             //RW - Bit[7:0]: Minimum exposure output limit
#define OV14825_AEC_MAX_EXPO_60_3          (0x3A02)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: Maximum exposure output limit[19:16] for 60 Hz
#define OV14825_AEC_MAX_EXPO_60_2          (0x3A03)             //RW - Bit[7:0]: Maximum exposure output limit[15:8] for 60 Hz
#define OV14825_AEC_MAX_EXPO_60_1          (0x3A04)             //RW - Bit[7:0]: Maximum exposure output limit[7:0] for 60 Hz
#define OV14825_AEC_CTRL05                 (0x3A05)             //RW - Bit[7]: F50 reverse
                                                                //         0: Hold 50, 60 Hz detect input
                                                                //         1: Switch 50, 60 Hz detect input
                                                                //     Bit[6]: Frame insert enable
                                                                //         0: In night mode, insert frame disable
                                                                //         1: In night mode, insert frame  enable
                                                                //     Bit[5]: Step auto enable
                                                                //         0: Step manual mode
                                                                //         1: Step auto mode
                                                                //     Bit[4:0]: Step auto ratio In step auto mode,
                                                                //               step ratio setting to adjust speed
#define OV14825_AEC_CTRL06                 (0x3A06)             //RW - Bit[7:5]: Not used
                                                                //     Bit[4:0]: Step manual1 Increase mode fast step
#define OV14825_AEC_CTRL07                 (0x3A07)             //RW - Bit[7:4]: Step manual2
                                                                //               Step manual
                                                                //               Slow step
                                                                //     Bit[3:0]: Step manual3
                                                                //               Step manual
                                                                //               Decrease mode fast step
#define OV14825_AEC_B50_STEP_2             (0x3A08)             //RW - Bit[7:2]: Not used
                                                                //     Bit[1:0]: AEC band50 width[9:8]
#define OV14825_AEC_B50_STEP_1             (0x3A09)             //RW - Bit[7:0]: AEC band50 width[7:0]
#define OV14825_AEC_B60_STEP_2             (0x3A0A)             //RW - Bit[7:2]: Not used
                                                                //     Bit[1:0]: AEC band60 width[9:8]
#define OV14825_AEC_B60_STEP_1             (0x3A0B)             //RW - Bit[7:0]: AEC band60 width[7:0]
// 0x3A0C reserved
#define OV14825_AEC_CTRL0D                 (0x3A0D)             //RW - 60Hz Max Bands In One Frame
                                                                //     Bit[7:6]: Not used
                                                                //     Bit[5:0]: Band 60 max
#define OV14825_AEC_CTRL0E                 (0x3A0E)             //RW - 50Hz Max Bands In One Frame
                                                                //     Bit[7:6]: Not used
                                                                //     Bit[5:0]: Band 50 max
#define OV14825_AEC_CTRL0F                 (0x3A0F)             //RW - Stable Range High Limit
                                                                //     Bit[7:0]: AEC stable range high threshold 1
#define OV14825_AEC_CTRL10                 (0x3A10)             //RW - Stable Range Low Limit
                                                                //     Bit[7:0]: AEC stable range low threshold 1
#define OV14825_AEC_CTRL11                 (0x3A11)             //RW - Step Manual Mode, Fast Zone High Limit
                                                                //     Bit[7:0]: Fast zone high threshold
#define OV14825_AEC_CTRL12                 (0x3A12)             //RW - Bit[7:0]: AEC manual average
#define OV14825_AEC_CTRL13                 (0x3A13)             //RW - Bit[7]: Not used
                                                                //     Bit[6]: Pre gain enable
                                                                //     Bit[5:0]: Pre gain value
#define OV14825_AEC_MAX_EXPO_50_2          (0x3A14)             //RW - 50Hz Maximum Exposure Output Limit
                                                                //     Bit[7:4]: Not used
                                                                //     Bit[3:0]: Max exposure[19:16] for 50Hz
#define OV14825_AEC_MAX_EXPO_50_1          (0x3A15)             //RW - 50Hz Maximum Exposure Output Limit
                                                                //     Bit[7:0]: Max exposure[15:8] for 50Hz
#define OV14825_AEC_MAX_EXPO_50_0          (0x3A16)             //RW - 50Hz Maximum Exposure Output Limit
                                                                //     Bit[7:0]: Max exposure[7:0] for 50Hz
#define OV14825_AEC_CTRL17                 (0x3A17)             //RW - Gain Base When in Night Mode
                                                                //     Bit[7:2]: Not used
                                                                //     Bit[1:0]: Gain night threshold[1:0]
                                                                //         00: Night mode gain threshold as 1x
                                                                //         01: Night mode gain threshold as 2x
                                                                //         10: Night mode gain threshold as 4x
                                                                //         11: Night mode gain threshold as 8x
#define OV14825_AEC_GAIN_CEILING_2         (0x3A18)             //RW - Gain Output Max Limit
                                                                //     Bit[7:1]: Not used
                                                                //     Bit[0]: AEC gain ceiling[8]
#define OV14825_AEC_GAIN_CEILING_1         (0x3A19)             //RW - Gain Output Max Limit
                                                                //     Bit[7:0]: AEC gain ceiling[7:0]
// 0x3A1A reserved
#define OV14825_AEC_CTRL1B                 (0x3A1B)             //RW - Stable Range High Limit (go out)
                                                                //     Bit[7:0]: Upper limit of stable to unstable range
#define OV14825_LED_ADD_RW_2               (0x3A1C)             //RW - Bit[7:0]: Strobe width[15:8] for LED1&2 mode Unit: lines
#define OV14825_LED_ADD_RW_1               (0x3A1D)             //RW - Bit[7:0]: Strobe width[7:0] for LED1&2 mode Unit: lines
#define OV14825_AEC_CTRL1E                 (0x3A1E)             //RW - Stable Range Low Limit (go out)
                                                                //     Bit[7:0]: Lower limit of stable to unstable range
#define OV14825_AEC_CTRL1F                 (0x3A1F)             //RW - Step Manual Mode - fast zone low limit
                                                                //     Bit[7:0]: Lower limit of fast AEC/AGC control zone


/*****************************************************************************
 * strobe control registers
 *****************************************************************************/
#define OV14825_STROBE_CTRL00              (0x3B00)             //RW - Bit[7]: Strobe request ON/OFF
                                                                //     Bit[6]: Strobe output pulse polarity
                                                                //         0: positive pulse
                                                                //         1: negative pulse
                                                                //     Bit[5:4]: Not used
                                                                //     Bit[3:2]: Pulse width in xenon mode
                                                                //         00: 1 line
                                                                //         01: 2 lines
                                                                //         10: 3 lines
                                                                //         11: 4 lines
                                                                //     Bit[1:0]: Strobe mode select
                                                                //         00: Xenon
                                                                //         01: LED1
                                                                //         10: LED2
                                                                //         11: LED3
#define OV14825_STROBE_CTRL06               (0x3B06)             //RW - Bit[7:6]: Reserved
                                                                //     Bit[5:4]: Strobe control mode at frame exposure
                                                                //         00: Strobe used with external FREX
                                                                //         01: Strobe pulse with internal trigger
                                                                //         10: Not used
                                                                //         11: Not used
                                                                //     Bit[3:0]: Strobe pulse width[3:0] (when register 0x3B06[5:4] = 01)
#define OV14825_STROBE_CTRL07               (0x3B07)            //RW - Bit[1]: Strobe source selection
                                                                //         0: Frame exposure mode strobe source
                                                                //         1: Rolling shutter mode strobe source
#define OV14825_STROBE_CTRL0A               (0x3B0A)            //RW - Bit[7:0]: Strobe pulse width[19:12](when register 0x3B06[5:4] = 01)
#define OV14825_STROBE_CTRL0B               (0x3B0B)            //RW - Bit[7:0]: Strobe pulse width[11:4](when register 0x3B06[5:4] = 01)



/*****************************************************************************
 * auto light frequency detection registers
 *****************************************************************************/
#define OV14825_ALFD_CTRL1                 (0x3C01)             //RW - Bit[7]: Auto detection enable
                                                                //         0: Enable auto detection
                                                                //         1: Disable auto detection
#define OV14825_ALFD_CTRLC                 (0x3C0C)             //R -  Bit[0]: Light frequency indicator
                                                                //         0: 60 Hz
                                                                //         1: 50 Hz



/*****************************************************************************
 * OTP registers
 *****************************************************************************/
#define OV14825_OTP_DUMP_PROGRAM_2         (0x3D60)             //RW - Bit[7:1]: Not used
                                                                //     Bit[0]: Program OTP
#define OV14825_OTP_DUMP_PROGRAM_1         (0x3D61)             //RW - Bit[7:1]: Not used
                                                                //     Bit[0]: Load / dump OTP
/*
0x3D0n (n<0x10)
0x3Dn (n>0x0F) OTP DATA ??N?? 0x00 RW                           //RW - Bit[7:0]: Data dumped or data to be programmed for bits[8*(n+1)-1:8*n]
*/



/*****************************************************************************
 * BLC control registers
 *****************************************************************************/
#define OV14825_BLC_CTRL08                 (0x4008)             //RW - Bit[7:2]: Not used
                                                                //     Bit[1:0]: Black level target[9:8] BLC target is a 12-bit value.
                                                                //               For example, 0x40 means 0x04 on 8-bit value
#define OV14825_BLC_CTRL09                 (0x4009)             //RW - Bit[7:0]: Black level target[7:0] BLC target is a 12-bit value.
                                                                //               For example, 0x40 means 0x04 on 8-bit value



/*****************************************************************************
 * ISP top registers
 *****************************************************************************/
#define OV14825_ISP_CTRL00                 (0x5000)             //RW - Bit[7]: LENC correction enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[6:3]: Not used
                                                                //     Bit[2]: Black pixel cancellation enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[1]: White pixel cancellation enable
                                                                //         0: Disable
                                                                //         1: Enable
#define OV14825_ISP_CTRL01                 (0x5001)             //RW - Bit[7:1]: Not used
                                                                //     Bit[0]: Auto white balance enable
                                                                //         0: Disable
                                                                //         1: Enable
#define OV14825_ISP_CTRL02                 (0x5002)             //RW - Bit[7:3]: Not used
                                                                //     Bit[2]: ISP subsample enable
                                                                //         0: Disable
                                                                //         1: Enable
// (0x5003~0x501E) reserved
#define OV14825_ISP_CTRL1F                 (0x501F)             //RW - Bit[7:6]: Not used
                                                                //     Bit[5]: Bypass ISP
                                                                //     Bit[4:0]: Not used
// (0x5020~0x503C) reserved
#define OV14825_ISP_CTRL3D                 (0x503D)             //RW - Bit[7]: Pre ISP color bar enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[6]: Not used
                                                                //     Bit[5:4]: color bar pattern
                                                                //         00: normal
                                                                //         01: vertical fade-out
                                                                //         10: horizontal fade-out
                                                                //         11: vertical fade-in
                                                                //     Bit[3:0]: Not used
// (0x503E~0x5040) reserved
#define OV14825_ISP_CTRL41                 (0x5041)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3]: OTP enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[2]: Average enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[1:0]: Not used
// (0x5042~0x5055) reserved
#define OV14825_ISP_RED_GAIN_2             (0x5056)             //R - Bit[7:4]: Not used
                                                                //    Bit[3:0]: AWB red gain man[11:8]
#define OV14825_ISP_RED_GAIN_1             (0x5057)             //R - Bit[7:0]: AWB red gain man[7:0]
#define OV14825_ISP_GRN_GAIN_2             (0x5058)             //R - Bit[7:4]: Not used
                                                                //    Bit[3:0]: AWB green gain man[11:8]
#define OV14825_ISP_GRN_GAIN_1             (0x5059)             //R - Bit[7:0]: AWB green gain man[7:0]
#define OV14825_ISP_BLU_GAIN_2             (0x505A)             //R - Bit[7:4]: Not used
                                                                //    Bit[3:0]: AWB blue gain man[11:8]
#define OV14825_ISP_BLU_GAIN_1             (0x505B)             //R - Bit[7:0]: AWB blue gain man[7:0]

/*****************************************************************************
 * AWB control registers
 *****************************************************************************/

#define OV14825_AWB_CTRL00                 (0x5180)             //RW - Bit[7]: Not used
                                                                //     Bit[6]: Fast AWB enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[5]: Freeze gain enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[4]: Not used
                                                                //     Bit[3]: Gain manual enable
                                                                //         0: Disable
                                                                //         1: Enable
                                                                //     Bit[2:0]: Not used
// (0x5081~0x5085) reserved
#define OV14825_AWB_CTRL06                 (0x5186)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: AWB red gain[11:8] 0x400 is 1x gain
#define OV14825_AWB_CTRL07                 (0x5187)             //RW - Bit[7:0]: AWB red gain[7:0]
#define OV14825_AWB_CTRL08                 (0x5188)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: AWB green gain[11:8] 0x400 is 1x gain
#define OV14825_AWB_CTRL09                 (0x5189)             //RW - Bit[7:0]: AWB green gain[7:0]
#define OV14825_AWB_CTRL0A                 (0x518A)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: AWB blue gain[11:8] 0x400 is 1x gain
#define OV14825_AWB_CTRL0B                 (0x518B)             //RW - Bit[7:0]: AWB blue gain[7:0]
#define OV14825_AWB_CTRL0C                 (0x518C)             //RW - Bit[7:4]: Red gain up limit
                                                                //       Maximum red gain is this value
                                                                //       shift 8 bits and add 0xFF
                                                                //     Bit[3:0]: Red gain down limit
                                                                //       Minimum red gain is this value
                                                                //       shift 8 bits and add 0x00
#define OV14825_AWB_CTRL0D                 (0x518D)             //RW - Bit[7:4]: Green gain up limit
                                                                //       Maximum green gain is this
                                                                //       value shift 8 bits and add 0xFF
                                                                //     Bit[3:0]: Green gain down limit
                                                                //       Minimum green gain is this value
                                                                //       shift 8 bits and add 0x00
#define OV14825_AWB_CTRL0E                 (0x518E)             //RW - Bit[7:4]: Blue gain up limit
                                                                //       Maximum blue gain is this value
                                                                //       shift 8 bits and add 0xFF
                                                                //     Bit[3:0]: Blue gain down limit
                                                                //       Minimum blue gain is this value
                                                                //       shift 8 bits and add 0x00
#define OV14825_AWB_CTRL0F                 (0x518F)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: Frame number to do AWB
#define OV14825_AWB_CTRL10                 (0x5190)             //R -  Bit[7:4]: Not used
                                                                //     Bit[3:0]: Current red gain[11:8]
#define OV14825_AWB_CTRL11                 (0x5191)             //R -  Bit[7:0]: Current red gain[7:0]
#define OV14825_AWB_CTRL12                 (0x5192)             //R -  Bit[7:4]: Not used
                                                                //     Bit[3:0]: Current green gain[11:8]
#define OV14825_AWB_CTRL13                 (0x5193)             //R -  Bit[7:0]: Current green gain[7:0]
#define OV14825_AWB_CTRL14                 (0x5194)             //R -  Bit[7:4]: Not used
                                                                //     Bit[3:0]: Current blue gain[11:8]
#define OV14825_AWB_CTRL15                 (0x5195)             //R -  Bit[7:0]: Current blue gain[7:0]



/*****************************************************************************
 * AVG control registers
 *****************************************************************************/
#define OV14825_AVG_CTRL00                 (0x5680)             //RW - Bit[7:5]: Not used
                                                                //     Bit[4:0]: X start offset[12:8]
#define OV14825_AVG_CTRL01                 (0x5681)             //RW - Bit[7:0]: X start offset[7:0]
#define OV14825_AVG_CTRL02                 (0x5682)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: Y start offset[11:8]
#define OV14825_AVG_CTRL03                 (0x5683)             //RW - Bit[7:0]: Y start offset[7:0]
#define OV14825_AVG_CTRL04                 (0x5684)             //RW - Bit[7:5]: Not used
                                                                //     Bit[4:0]: Window width[12:8]
#define OV14825_AVG_CTRL05                 (0x5685)             //RW - Bit[7:0]: Window width[7:0]
#define OV14825_AVG_CTRL06                 (0x5686)             //RW - Bit[7:4]: Not used
                                                                //     Bit[3:0]: Window height[11:8]
#define OV14825_AVG_CTRL07                 (0x5687)             //RW - Bit[7:0]: Window height[7:0]
#define OV14825_AVG_CTRL08                 (0x5688)             //RW - Bit[7:1]: Not used
                                                                //     Bit[0]: Average size manual
                                                                //         0: Disable
                                                                //         1: Enable
#define OV14825_AVERAGE_CTRL10             (0x568A)             //R -  Bit[7:0]: Average readout



/*****************************************************************************
 * LENC control registers
 *****************************************************************************/
#define OV14825_LENC_CTRL00                (0x5800)             //RW - LENC Parameters
#define OV14825_LENC_CTRL01                (0x5801)             //RW - LENC Parameters
#define OV14825_LENC_CTRL02                (0x5802)             //RW - LENC Parameters
#define OV14825_LENC_CTRL03                (0x5803)             //RW - LENC Parameters
#define OV14825_LENC_CTRL04                (0x5804)             //RW - LENC Parameters
#define OV14825_LENC_CTRL05                (0x5805)             //RW - LENC Parameters
#define OV14825_LENC_CTRL06                (0x5806)             //RW - LENC Parameters
#define OV14825_LENC_CTRL07                (0x5807)             //RW - LENC Parameters
#define OV14825_LENC_CTRL87                (0x5887)             //RW - LENC Parameters



/*****************************************************************************
 * ISP window registers
 *****************************************************************************/
#define OV14825_WIN_CTRL00                 (0x5A00)             //RW - Bit[4:0]: X start offset[12:8]
#define OV14825_WIN_CTRL01                 (0x5A01)             //RW - Bit[7:0]: X start offset[7:0]
#define OV14825_WIN_CTRL02                 (0x5A02)             //RW - Bit[3:0]: Y start offset[11:8]
#define OV14825_WIN_CTRL03                 (0x5A03)             //RW - Bit[7:0]: Y start offset[7:0]
#define OV14825_WIN_CTRL04                 (0x5A04)             //RW - Bit[4:0]: Window width[12:8]
#define OV14825_WIN_CTRL05                 (0x5A05)             //RW - Bit[7:0]: Window width[7:0]
#define OV14825_WIN_CTRL06                 (0x5A06)             //RW - Bit[3:0]: Window height[11:8]
#define OV14825_WIN_CTRL07                 (0x5A07)             //RW - Bit[7:0]: Window height[7:0]
#define OV14825_WIN_CTRL08                 (0x5A08)             //RW - Bit[0]: Window size manual
                                                                //         0: Disable
                                                                //         1: Enable





/*****************************************************************************
 * Default values
 *****************************************************************************/

// column 1)
//    Reset values as of datasheet OV14825_CSP3_DS_2.03_Dreamchip.pdf
//    04/06/2011, values not given in the datasheet are values from 2) or 3).
// column 2)
//   reference setting OV14825_1080p_settings.txt
// column 3)
//   reference setting OV14825_1080p_settings XCKL_10MHz-1 lane_Dreamchip.txt
//
// Make sure that these static settings are reflecting the capabilities defined
// in IsiGetCapsIss (further dynamic setup may alter these default settings but
// often does not if there is no choice available).

//                                                   3)       2)     1)

/*****************************************************************************
 * System control registers
 *****************************************************************************/
#define OV14825_SMIA_R0100_DEFAULT                  (0x00)
#define OV14825_SMIA_R0103_DEFAULT                  (0x00)
#define OV14825_PAD_OEN2_DEFAULT                    (0x00)
#define OV14825_PAD_OUT0_DEFAULT                    (0x00)
#define OV14825_SCCB_ID_DEFAULT                     (0x6C)
#define OV14825_PLL_CTRL4_DEFAULT                   (0x0A) // (0x0A) (0x0A)
#define OV14825_PLL_CTRL3_DEFAULT                   (0x00) // (0x00) (0x00)
#define OV14825_PLL_CTRL2_DEFAULT                   (0xA7) // (0xA7) (0x9D)
#define OV14825_PLL_CTRL1_DEFAULT                   (0x80) // (0x80) (0xC3)
#define OV14825_PLL_CTRL0_DEFAULT                   (0x08) // (0x00) (0x0D)
#define OV14825_SC_PAD_OEN0_DEFAULT                 (0x00)
#define OV14825_SC_PAD_OEN1_DEFAULT                 (0x00)
#define OV14825_CHIP_ID_HIGH_BYTE_DEFAULT           (0xE8)
#define OV14825_CHIP_ID_LOW_BYTE_DEFAULT            (0x10)
#define OV14825_SC_PAD_OUT1_DEFAULT                 (0x00)
#define OV14825_SC_PAD_OUT2_DEFAULT                 (0x00)
#define OV14825_SC_PAD_SEL0_DEFAULT                 (0x00)
#define OV14825_SC_PAD_SEL1_DEFAULT                 (0x00)
#define OV14825_SC_PAD_SEL2_DEFAULT                 (0x00)
#define OV14825_SC_PLL_CTRL_S2_DEFAULT              (0x00)
#define OV14825_SC_PLL_CTRL_S1_DEFAULT              (0x40)
#define OV14825_SC_PLL_CTRL_S0_DEFAULT              (0x1F)
#define OV14825_SC_MIPI_SC_CTRL_DEFAULT             (0x04) //        (0x1C)



/*****************************************************************************
 * SRG control registers
 *****************************************************************************/
#define OV14825_GROUP1_START_ADDRESS_DEFAULT        (0x00)
#define OV14825_GROUP2_START_ADDRESS_DEFAULT        (0x00)
#define OV14825_GROUP3_START_ADDRESS_DEFAULT        (0x00)
#define OV14825_GROUP4_START_ADDRESS_DEFAULT        (0x00)
#define OV14825_GROUP_ACCESS_DEFAULT                (0x00)
#define OV14825_GROUP_STATUS_DEFAULT                (0x00)



/*****************************************************************************
 * Manual AEC/AGC control registers
 *****************************************************************************/
#define OV14825_AEC_EXPO_2_DEFAULT                  (0x00)
#define OV14825_AEC_EXPO_1_DEFAULT                  (0x00)
#define OV14825_AEC_EXPO_0_DEFAULT                  (0x00)
#define OV14825_AEC_MANUAL_DEFAULT                  (0x00)
#define OV14825_AEC_AGC_ADJ_2_DEFAULT               (0x00)
#define OV14825_AEC_AGC_ADJ_1_DEFAULT               (0x06)



/*****************************************************************************
 * Sensor control registers
 *****************************************************************************/
#define OV14825_ARRAY_CONTROL_0D_DEFAULT            (0x6D) //(0x01)



/*****************************************************************************
 * Timing control registers
 *****************************************************************************/
#define OV14825_TIMING_HW_2_DEFAULT                 (0x07) //(0x11)
#define OV14825_TIMING_HW_1_DEFAULT                 (0x80) //(0x40)
#define OV14825_TIMING_VH_2_DEFAULT                 (0x04) //(0x0C)
#define OV14825_TIMING_VH_1_DEFAULT                 (0x38) //(0xF0)
#define OV14825_TIMING_HOFFS_DEFAULT                (0x22) //(0x86)
#define OV14825_TIMING_TC_REG18_DEFAULT             (0x45) //(0x40)
#define OV14825_TIMING_HTS_2_DEFAULT                (0x0E) //(0x08)
#define OV14825_TIMING_HTS_1_DEFAULT                (0x4c) //(0xC8)
#define OV14825_TIMING_VTS_2_DEFAULT                (0x04) //(0x0D)
#define OV14825_TIMING_VTS_1_DEFAULT                (0x44) //(0x08)
#define OV14825_TIMING_CONTROL_1C_DEFAULT           (0xB2) //(0x20)
#define OV14825_TIMING_CONTROL_1D_DEFAULT           (0x50) //(0x0C)
#define OV14825_TIMING_CONTROL_1E_DEFAULT           (0x08) //(0x0C)
#define OV14825_TIMING_CONTROL_1F_DEFAULT           (0x7C) //(0xFC)
#define OV14825_TIMING_CONTROL_20_DEFAULT           (0x01) //(0x00)
#define OV14825_TIMING_CONTROL_21_DEFAULT           (0x28) //(0x10)



/*****************************************************************************
 * AEC/AGC control registers
 *****************************************************************************/
#define OV14825_AEC_CTRL00_DEFAULT                  (0x78) //(0x38)
#define OV14825_AEC_CTRL01_DEFAULT                  (0x04)
#define OV14825_AEC_MAX_EXPO_60_3_DEFAULT           (0x06)
#define OV14825_AEC_MAX_EXPO_60_2_DEFAULT           (0x84)
#define OV14825_AEC_MAX_EXPO_60_1_DEFAULT           (0x00)
#define OV14825_AEC_CTRL05_DEFAULT                  (0x30)
#define OV14825_AEC_CTRL06_DEFAULT                  (0x10)
#define OV14825_AEC_CTRL07_DEFAULT                  (0x18)
#define OV14825_AEC_B50_STEP_2_DEFAULT              (0x14) //(0x0E)
#define OV14825_AEC_B50_STEP_1_DEFAULT              (0x60) //(0x90)
#define OV14825_AEC_B60_STEP_2_DEFAULT              (0x11) //(0x0C)
#define OV14825_AEC_B60_STEP_1_DEFAULT              (0x10) //(0x20)
#define OV14825_AEC_CTRL0D_DEFAULT                  (0x04) //(0x11)
#define OV14825_AEC_CTRL0E_DEFAULT                  (0x03) //(0x0E)
#define OV14825_AEC_CTRL0F_DEFAULT                  (0x78)
#define OV14825_AEC_CTRL10_DEFAULT                  (0x68)
#define OV14825_AEC_CTRL11_DEFAULT                  (0xD0)
#define OV14825_AEC_CTRL12_DEFAULT                  (0x00)
#define OV14825_AEC_CTRL13_DEFAULT                  (0x46) //(0x50)
#define OV14825_AEC_MAX_EXPO_50_2_DEFAULT           (0x06)
#define OV14825_AEC_MAX_EXPO_50_1_DEFAULT           (0x84)
#define OV14825_AEC_MAX_EXPO_50_0_DEFAULT           (0x00)
#define OV14825_AEC_CTRL17_DEFAULT                  (0x89)
#define OV14825_AEC_GAIN_CEILING_2_DEFAULT          (0x00) //(0x03)
#define OV14825_AEC_GAIN_CEILING_1_DEFAULT          (0x7F) //(0xE0)
#define OV14825_AEC_CTRL1B_DEFAULT                  (0x78)
#define OV14825_LED_ADD_RW_2_DEFAULT                (0x06)
#define OV14825_LED_ADD_RW_1_DEFAULT                (0x18)
#define OV14825_AEC_CTRL1E_DEFAULT                  (0x68)
#define OV14825_AEC_CTRL1F_DEFAULT                  (0x40)



/*****************************************************************************
 * Strobe control registers
 *****************************************************************************/
#define OV14825_STROBE_CTRL00_DEFAULT               (0x00)
#define OV14825_STROBE_CTRL06_DEFAULT               (0x04)
#define OV14825_STROBE_CTRL07_DEFAULT               (0x08)
#define OV14825_STROBE_CTRL0A_DEFAULT               (0x00)
#define OV14825_STROBE_CTRL0B_DEFAULT               (0x00)



/*****************************************************************************
 * Auto light frequency detection registers
 *****************************************************************************/
#define OV14825_ALFD_CTRL1_DEFAULT                  (0x32)
#define OV14825_ALFD_CTRLC_DEFAULT                  (0x00)



/*****************************************************************************
 * OTP control registers
 *****************************************************************************/
#define OV14825_OTP_DUMP_PROGRAM_2_DEFAULT          (0x00)
#define OV14825_OTP_DUMP_PROGRAM_1_DEFAULT          (0x00)



/*****************************************************************************
 * BLC control registers
 *****************************************************************************/
#define OV14825_BLC_CTRL08_DEFAULT                  (0x00)
#define OV14825_BLC_CTRL09_DEFAULT                  (0x40) //(0x10)



/*****************************************************************************
 * ISP topl registers
 *****************************************************************************/
#define OV14825_ISP_CTRL00_DEFAULT                  (0x00) //(0x06) (0xDF)
#define OV14825_ISP_CTRL01_DEFAULT                  (0x00) //(0x4F)
#define OV14825_ISP_CTRL02_DEFAULT                  (0x00) //(0xE4)
#define OV14825_ISP_CTRL1F_DEFAULT                  (0x00)
#define OV14825_ISP_CTRL3D_DEFAULT                  (0x08) //(0x00)
#define OV14825_ISP_CTRL41_DEFAULT                  (0x0E) //(0x0F)
#define OV14825_ISP_RED_GAIN_2_DEFAULT              (0x00)
#define OV14825_ISP_RED_GAIN_1_DEFAULT              (0x00)
#define OV14825_ISP_GRN_GAIN_2_DEFAULT              (0x00)
#define OV14825_ISP_GRN_GAIN_1_DEFAULT              (0x00)
#define OV14825_ISP_BLU_GAIN_2_DEFAULT              (0x00)
#define OV14825_ISP_BLU_GAIN_1_DEFAULT              (0x00)



/*****************************************************************************
 * AWB control registers
 *****************************************************************************/
#define OV14825_AWB_CTRL00_DEFAULT                  (0x40)
#define OV14825_AWB_CTRL06_DEFAULT                  (0x04)
#define OV14825_AWB_CTRL07_DEFAULT                  (0x00)
#define OV14825_AWB_CTRL08_DEFAULT                  (0x04)
#define OV14825_AWB_CTRL09_DEFAULT                  (0x00)
#define OV14825_AWB_CTRL0A_DEFAULT                  (0x04)
#define OV14825_AWB_CTRL0B_DEFAULT                  (0x00)
#define OV14825_AWB_CTRL0C_DEFAULT                  (0xC0)
#define OV14825_AWB_CTRL0D_DEFAULT                  (0xC0)
#define OV14825_AWB_CTRL0E_DEFAULT                  (0xC0)
#define OV14825_AWB_CTRL0F_DEFAULT                  (0x51)
#define OV14825_AWB_CTRL10_DEFAULT                  (0x04)
#define OV14825_AWB_CTRL11_DEFAULT                  (0x00)
#define OV14825_AWB_CTRL12_DEFAULT                  (0x04)
#define OV14825_AWB_CTRL13_DEFAULT                  (0x00)
#define OV14825_AWB_CTRL14_DEFAULT                  (0x04)
#define OV14825_AWB_CTRL15_DEFAULT                  (0x00)



/*****************************************************************************
 * AVG control registers
 *****************************************************************************/
#define OV14825_AVG_CTRL00_DEFAULT                  (0x00)
#define OV14825_AVG_CTRL01_DEFAULT                  (0x00)
#define OV14825_AVG_CTRL02_DEFAULT                  (0x00)
#define OV14825_AVG_CTRL03_DEFAULT                  (0x00)
#define OV14825_AVG_CTRL04_DEFAULT                  (0x10)
#define OV14825_AVG_CTRL05_DEFAULT                  (0xA0)
#define OV14825_AVG_CTRL06_DEFAULT                  (0x0C)
#define OV14825_AVG_CTRL07_DEFAULT                  (0x78)
#define OV14825_AVG_CTRL08_DEFAULT                  (0x02)
#define OV14825_AVERAGE_CTRL10_DEFAULT              (0x00)



/*****************************************************************************
 * LENC control registers
 *****************************************************************************/
#define OV14825_LENC_CTRL00_DEFAULT                 (0x00)
#define OV14825_LENC_CTRL01_DEFAULT                 (0x00)
#define OV14825_LENC_CTRL02_DEFAULT                 (0x00)
#define OV14825_LENC_CTRL03_DEFAULT                 (0x00)
#define OV14825_LENC_CTRL04_DEFAULT                 (0x00)
#define OV14825_LENC_CTRL05_DEFAULT                 (0x00)
#define OV14825_LENC_CTRL06_DEFAULT                 (0x00)
#define OV14825_LENC_CTRL07_DEFAULT                 (0x00)
#define OV14825_LENC_CTRL87_DEFAULT                 (0x00)



/*****************************************************************************
 * ISP window registers
 *****************************************************************************/
#define OV14825_WIN_CTRL00_DEFAULT                 (0x00)           //RW - Bit[4:0]: X start offset[12:8]
#define OV14825_WIN_CTRL01_DEFAULT                 (0x00)           //RW - Bit[7:0]: X start offset[7:0]
#define OV14825_WIN_CTRL02_DEFAULT                 (0x00)           //RW - Bit[3:0]: Y start offset[11:8]
#define OV14825_WIN_CTRL03_DEFAULT                 (0x00)           //RW - Bit[7:0]: Y start offset[7:0]
#define OV14825_WIN_CTRL04_DEFAULT                 (0x10)           //RW - Bit[4:0]: Window width[12:8]
#define OV14825_WIN_CTRL05_DEFAULT                 (0xA0)           //RW - Bit[7:0]: Window width[7:0]
#define OV14825_WIN_CTRL06_DEFAULT                 (0x0C)           //RW - Bit[3:0]: Window height[11:8]
#define OV14825_WIN_CTRL07_DEFAULT                 (0x78)           //RW - Bit[7:0]: Window height[7:0]
#define OV14825_WIN_CTRL08_DEFAULT                 (0x00)           //RW - Bit[0]: Window size manual
                                                                    //         0: Disable
                                                                    //         1: Enable



/*****************************************************************************
 * ov14825 context structure
 *****************************************************************************/
typedef struct OV14825_Context_s
{
    IsiSensorContext_t  IsiCtx;                 /**< common context of ISI and ISI driver layer; @note: MUST BE FIRST IN DRIVER CONTEXT */

    //// modify below here ////

    IsiSensorConfig_t   Config;                 /**< sensor configuration */
    bool_t              Configured;             /**< flags that config was applied to sensor */
    bool_t              Streaming;              /**< flags that sensor is streaming data */
    bool_t              TestPattern;            /**< flags that sensor is streaming test-pattern */

    bool_t              isAfpsRun;              /**< if true, just do anything required for Afps parameter calculation, but DON'T access SensorHW! */

    bool_t              GroupHold;

    float               VtPixClkFreq;           /**< pixel clock */
    uint16_t            LineLengthPck;          /**< line length with blanking */
    uint16_t            FrameLengthLines;       /**< frame line length */

    float               AecMaxGain;
    float               AecMinGain;
    float               AecMaxIntegrationTime;
    float               AecMinIntegrationTime;

    float               AecIntegrationTimeIncrement; /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */
    float               AecGainIncrement;            /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */

    float               AecCurGain;
    float               AecCurIntegrationTime;

    uint16_t            OldMultiplier;               /**< gain multiplier */
    uint8_t             OldBase;                     /**< fine gain */
    uint32_t            OldCoarseIntegrationTime;
    uint32_t            OldFineIntegrationTime;

    IsiSensorMipiInfo   IsiSensorMipiInfo;
} OV14825_Context_t;



#ifdef __cplusplus
}
#endif



#endif /* __OV14825_PRIV_H__ */
