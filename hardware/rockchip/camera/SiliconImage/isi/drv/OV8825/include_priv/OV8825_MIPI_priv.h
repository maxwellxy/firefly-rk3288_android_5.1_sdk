//OV8825_MIPI_priv.h
/*****************************************************************************/
/*!
 *  \file        OV8810_priv.h \n
 *  \version     1.0 \n
 *  \author      Meinicke \n
 *  \brief       Private header file for sensor specific code of the OV8810. \n
 *
 *  \revision    $Revision: 432 $ \n
 *               $Author: neugebaa $ \n
 *               $Date: 2009-06-30 11:48:59 +0200 (Di, 30 Jun 2009) $ \n
 *               $Id: OV8810_priv.h 432 2009-06-30 09:48:59Z neugebaa $ \n
 */
/*  This is an unpublished work, the copyright in which vests in Silicon Image
 *  GmbH. The information contained herein is the property of Silicon Image GmbH
 *  and is supplied without liability for errors or omissions. No part may be
 *  reproduced or used expect as authorized by contract or other written
 *  permission. Copyright(c) Silicon Image GmbH, 2009, all rights reserved.
 */
/*****************************************************************************/

/*Modify by oyyf@rock-chips.com*/
/*
#ifndef _OV8810_PRIV_H
#define _OV8810_PRIV_H

#include "isi_priv.h"

#if( OV8810_DRIVER_USAGE == USE_CAM_DRV_EN )
*/


#ifndef __OV8810_PRIV_H__
#define __OV8810_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>



#ifdef __cplusplus
extern "C"
{
#endif


/*
*              SILICONIMAGE LIBISP VERSION NOTE
*
*v0.1.0x00 : 1. set focus pos to MAX_LOG in streamoff for VCM noise when changeResolution and exit camera;
*            2. MDI_SLEW_RATE_CTRL 11 -> 1;
*v0.2.0:
*            this version sync below version:
*   v0.1.1:     1. MDI_SLEW_RATE_CTRL 1 -> 3;
*   v0.1.2:     1. invalidate 0x0100 setting in OV8825_SetupOutputWindow and OV8825_IsiSensorSetStreamingIss for VCM noise;
*
*v0.3.0:
*   1). add support vcm current and stepmode setting, isi version must v0.3.0
*v0.4.0 : 
*   1). decrease OV8825_MAX_GAIN_AEC to reduce noise for dark enviroment. 
*v0.5.0 : 
*   1). Resolution in pIsiSensorCaps for 2 lane had been configed wrong last commit, fix it.    
*v0.6.0:
*   1). stepmode setting haven't been send to sensor,fix it;
*   2). check value which send to sensor is larger than MAX_VCMDRV_REG or not;
*v0.7.0:
*   1). limit AecMinIntegrationTime 0.0001 for aec.
*v0.8.0:
*   1).add senosr drv version in get sensor i2c info func
*v0.9.0:
*   1). support for isi v0.5.0
*v0.a.0
*   1). support for isi v0.7.0
*v0.b.0
*   1). support for isi v0.8.0
*/


#define CONFIG_SENSOR_DRV_VERSION KERNEL_VERSION(0, 0xb, 0) 



/*****************************************************************************
 * System control registers
 *****************************************************************************/
#define OV8825_DELAY_5MS                    (0x0000) //delay 5 ms
#define OV8825_MODE_SELECT                  (0x0100) // rw - Bit[7:1]not used  Bit[0]Streaming set 0: software_standby  1: streaming       
#define OV8825_SOFTWARE_RST                 (0x0103) // rw - Bit[7:1]not used  Bit[0]software_reset
#define OV8825_PAD_ONE2                     (0x3000) // rw - Bit[7:5]not used 
                                                     //      Bit[4] io_strobe_oen
                                                     //      Bit[3] io_sda_oen
                                                     //      Bit[2] io_frex_oen
                                                     //      Bit[1] io_vsync_oen   
                                                     //      Bit[0] io_shutter_oen
#define OV8825_3001 (0x3001)
#define OV8825_3001_default (0x00)

#define OV8825_SCCB_ID                      (0x3002) // rw - Bit[7:0] sccb_id                                                    
#define OV8825_PLL_CTRL0                    (0x3003) // rw - PLL1 Control
                                                     //      Bit[7:6] DivB[1:0] 00: /1  01: /1  10: /2  11: /4
                                                     //      Bit[5:3] Debug Mode
                                                     //      Bit[2:0] PreDiv[2:0] 000:/1  001:/1.5 010:/2 011:/2.5 100:/3 101:/4 110:/6 111:/8
#define OV8825_PLL_CTRL1                    (0x3004) // rw - PLL1 Control
                                                     //      Bit[7] sclk_dac 0:Sysclk from PLL1  1:Sysclk from PLL2
                                                     //      Bit[6:0] 129-DivP[6:0]
#define OV8825_PLL_CTRL2                    (0x3005) // rw - PLL1 Control   
#define OV8825_PLL_CTRL3                    (0x3006) // rw - PLL1 Control
#define OV8825_PLL_CTRL4                    (0x3007) // rw - PLL1 Control
#define OV8825_CHIP_ID_HIGH_BYTE            (0x300a) // r - 
#define OV8825_CHIP_ID_MIDDLE_BYTE          (0x300b) // r - 
#define OV8825_CHIP_ID_LOW_BYTE             (0x300c) // r - 
#define OV8825_SC_PAD_OUT2                  (0x300d) // rw - 
#define OV8825_SC_PAD_SEL0                  (0x300e) // rw - 
#define OV8825_SC_PAD_SEL1                  (0x300f) // rw - 
#define OV8825_SC_PAD_SEL2                  (0x3010) // rw - 
#define OV8825_SC_PLL_CTRL_S2               (0x3011) // rw - 
#define OV8825_SC_PLL_CTRL_S0               (0x3012) // rw -
#define OV8825_SC_PLL_CTRL_S1               (0x3013) // rw - 
#define OV8825_MIPI_SC_CTRL                 (0x3018) // rw - 
#define OV8825_SC_CLKRST0                   (0x301a) // rw - 
#define OV8825_SC_CLKRST1                   (0x301b) // rw - 
#define OV8825_SC_CLKRST2                   (0x301c) // rw - 
#define OV8825_SC_CLKRST3                   (0x301d) // rw - 
#define OV8825_SC_CLKRST4                   (0x301e) // rw - 
#define OV8825_SC_FREX_MASK0                (0x301f) // rw - 
#define OV8825_SC_CLK_SEL                   (0x3020) // rw - 
#define OV8825_SC_MISC_CTRL                 (0x3021) // rw - 
#define OV8825_SRAM_TST                     (0x3022) // rw - 
#define OV8825_SRAM_VAL0                    (0x3023) // rw - 
#define OV8825_SRAM_VAL1                    (0x3024) // rw - 
#define OV8825_SENSOR_REVISION              (0x3025) // r -
#define OV8825_SCCB_CTRL0                   (0x3100) // rw - 
#define OV8825_SCCB_CTRL1                   (0x3101) // rw -  
#define OV8825_SCCB_CTRL2                   (0x3102) // rw - 
#define OV8825_SCCB_PLL                     (0x3104) // rw -  
#define OV8825_SRB_CTRL                     (0x3106) // rw - 
#define OV8825_GROUP_ADDR0                  (0x3200) // rw -
#define OV8825_GROUP_ADDR1                  (0x3201) // rw -
#define OV8825_GROUP_ADDR2                  (0x3202) // rw -
#define OV8825_GROUP_ADDR3                  (0x3203) // rw -
#define OV8825_GROUP_LENG0                  (0x3204) // rw -
#define OV8825_GROUP_LENG1                  (0x3205) // rw -
#define OV8825_GROUP_LENG2                  (0x3206) // rw -
#define OV8825_GROUP_LENG3                  (0x3207) // rw -
#define OV8825_GROUP_ACCESS                 (0x3208) // w -
#define OV8825_GRP0_PERIOD                  (0x3209) // rw -
#define OV8825_GRP1_PERIOD                  (0x320a) // rw -
#define OV8825_GRP_SWCTRL                   (0x320b) // rw -
#define OV8825_GRP_SRAM                     (0x320c) // rw -
#define OV8825_GRP_ACT                      (0x320d) // r -
#define OV8825_FRAME_CNT_GPR0               (0x320e) // r -
#define OV8825_FRAME_CNT_GPR1               (0x320f) // r -
#define OV8825_MWB_GAIN00                   (0x3400) // rw- Bit[3:0] MWB red gain[11:8]
#define OV8825_MWB_GAIN01                   (0x3401) // rw- Bit[7:0] MWB red gain[7:0]  red gain = MWB red gain[11:0]/0x400
#define OV8825_MWB_GAIN02                   (0x3402) // rw- Bit[3:0] MWB green gain[11:8]
#define OV8825_MWB_GAIN03                   (0x3403) // rw- Bit[7:0] MWB green gain[7:0]  green gain = MWB green gain[11:0]/0x400
#define OV8825_MWB_GAIN04                   (0x3404) // rw- Bit[3:0] MWB blue gain[11:8]
#define OV8825_MWB_GAIN05                   (0x3405) // rw- Bit[7:0] MWB blue gain[7:0]  blue gain = MWB blue gain[11:0]/0x400
#define OV8825_MWB_GAIN06                   (0x5001) // rw- Bit[7:0]not used   Bit[0]-0:Auto 1:mauual
#define OV8825_AEC_EXPO_H                   (0x3500) // rw- Bit[3:0] exposure[19:16]
#define OV8825_AEC_EXPO_M                   (0x3501) // rw- Bit[7:0] exposure[15:8]
#define OV8825_AEC_EXPO_L                   (0x3502) // rw- Bit[7:0] exposure[7:0] low 4 bits are fraction bits which are not supportted and should always be 0.
#define OV8825_AEC_MANUAL                   (0x3503) // rw- Bit[5:4] gain delay option  Bit[2]VTS manual enable  Bit[1]AGC manual enable  Bit[0]AEC manual enable 
#define OV8825_MAN_GAIN_H                   (0x3504) // rw- Bit[1:0] man gain[9:8]
#define OV8825_MAN_GAIN_L                   (0x3505) // rw- Bit[7:0] man gain[7:0]
#define OV8825_ADD_VTS_H                    (0x3506) // rw- Bit[7:0] add dummy line VTS[15:8]
#define OV8825_ADD_VTS_L                    (0x3507) // rw- Bit[7:0] add dummy line VTS[7:0]
#define OV8825_AEC09                        (0x3509) // rw- Bit[4] sensor gain convert enalbe 0:use sensor gain(0x350a 0x350b) 0x00 is 1x gain
                                                     //     Bit[3] gain manual enable 0:manual disable use register 0x350a/0x350b   1:manual enable use register 0x3504/0x3505
#define OV8825_AEC_AGC_ADJ_H                (0x350a) // rw- Bit[2:0]gain output to sensor Gain[10:8]
#define OV8825_AEC_AGC_ADJ_L                (0x350b) // rw- Bit[7:0]gain output to sensor Gain[7:0] 
                                                     //     when 0x3509[4]=0, this gain is sensor gain. real gain=2^N(16+x)/16. N is the number of 1 in bit[9:4], x is the low bit in bit[3:0]
                                                     //     when 0x3509[4]=1, this gain is real gain. the low 4bits is fraction bits
#define OV8825_ANACTRL0                     (0x3600) // rw   
#define OV8825_ANACTRL1                     (0x3601) // rw 
#define OV8825_ANACTRL2                     (0x3602) // rw 
#define OV8825_ANACTRL3                     (0x3603) // rw 
#define OV8825_ANACTRL4                     (0x3604) // rw 
#define OV8825_ANACTRL5                     (0x3605) // rw 
#define OV8825_ANACTRL6                     (0x3606) // rw 
#define OV8825_ANACTRL7                     (0x3607) // rw 
#define OV8825_ANACTRL8                     (0x3608) // rw 
#define OV8825_ANACTRL9                     (0x3609) // rw 
#define OV8825_ANACTRLA                     (0x360A) // rw 
#define OV8825_ANACTRLB                     (0x360B) // rw 
#define OV8825_ANACTRLC                     (0x360c) // rw 
#define OV8825_ANACTRLD                     (0x360d) // rw 
#define OV8825_ANACTRLE                     (0x360e) // rw 
#define OV8825_ANACTRLF                     (0x360f) // rw 
#define OV8825_ANACTRL10                    (0x3610) // rw 
#define OV8825_ANACTRL11                    (0x3611) // rw 
#define OV8825_ANACTRL12                    (0x3612) // rw - Bit[7]analog control Bit[6:5] PAD io drive capability 00:1x 01:2x 10:3x 11:4x  Bit[0]analog control
#define OV8825_ANACTRL13                    (0x3613) // rw 
#define OV8825_ANACTRL14                    (0x3614) // rw 
#define OV8825_ANACTRL15                    (0x3615) // rw 
#define OV8825_ANACTRL16                    (0x3616) // rw 
#define OV8825_ANACTRL17                    (0x3617) // rw 
#define OV8825_A_VCM_LOW                    (0x3618) // rw - Bit[7:4]-din[3:0]  Bit[3]-s3  Bit[2:0]-s[2:0]
#define OV8825_A_VCM_MID0                   (0x3619) // rw
#define OV8825_A_VCM_MID1                   (0x361A) // rw
#define OV8825_A_VCM_MID2                   (0x361B) // rw
#define OV8825_A_VCM_HIGH                   (0x361C) // rw
#define OV8825_ANACTRL1D                    (0x361D) // rw
#define OV8825_ANACTRL1E                    (0x361E) // rw
#define OV8825_SENCTRL0                     (0x3700) // rw
#define OV8825_SENCTRL1                     (0x3701) // rw
#define OV8825_SENCTRL2                     (0x3702) // rw
#define OV8825_SENCTRL3                     (0x3703) // rw
#define OV8825_SENCTRL4                     (0x3704) // rw
#define OV8825_SENCTRL5                     (0x3705) // rw
#define OV8825_SENCTRL6                     (0x3706) // rw
#define OV8825_SENCTRL7                     (0x3707) // rw
#define OV8825_SENCTRL8                     (0x3708) // rw
#define OV8825_SENCTRL9                     (0x3709) // rw
#define OV8825_SENCTRLA                     (0x370a) // rw
#define OV8825_SENCTRLB                     (0x370b) // rw
#define OV8825_SENCTRLC                     (0x370c) // rw
#define OV8825_SENCTRLD                     (0x370d) // rw
#define OV8825_SENCTRLE                     (0x370e) // rw
#define OV8825_SENCTRLF                     (0x370f) // rw
#define OV8825_SENCTRL10                    (0x3710) // rw
#define OV8825_SENCTRL11                    (0x3711) // rw
#define OV8825_SENCTRL12                    (0x3712) // rw
#define OV8825_TIMING_HS_H                  (0x3800) // rw horizonal start
#define OV8825_TIMING_HS_L                  (0x3801) // rw
#define OV8825_TIMING_VS_H                  (0x3802) // rw vertical start
#define OV8825_TIMING_VS_L                  (0x3803) // rw
#define OV8825_TIMING_HW_H                  (0x3804) // rw horizonal end
#define OV8825_TIMING_HW_L                  (0x3805) // rw
#define OV8825_TIMING_VH_H                  (0x3806) // rw vertical end
#define OV8825_TIMING_VH_L                  (0x3807) // rw
#define OV8825_TIMING_ISPHO_H               (0x3808) // rw
#define OV8825_TIMING_ISPHO_L               (0x3809) // rw
#define OV8825_TIMING_ISPVO_H               (0x380a) // rw
#define OV8825_TIMING_ISPVO_L               (0x380b) // rw
#define OV8825_TIMING_HTS_H                 (0x380c) // rw
#define OV8825_TIMING_HTS_L                 (0x380d) // rw
#define OV8825_TIMING_VTS_H                 (0x380e) // rw
#define OV8825_TIMING_VTS_L                 (0x380f) // rw
#define OV8825_TIMING_HOFFS_HIGH            (0x3810) // rw
#define OV8825_TIMING_HOFFS_LOW             (0x3811) // rw
#define OV8825_TIMING_VOFFS_HIGH            (0x3812) // rw
#define OV8825_TIMING_VOFFS_LOW             (0x3813) // rw
#define OV8825_TIMING_X_INC                 (0x3814) // rw
#define OV8825_TIMING_Y_INC                 (0x3815) // rw
#define OV8825_TIMING_HSYNC_START_HIGH      (0x3816) // rw
#define OV8825_TIMING_HSYNC_START_LOW       (0x3817) // rw
#define OV8825_TIMING_HSYNC_END_HIGH        (0x3818) // rw
#define OV8825_TIMING_HSYNC_END_LOW         (0x3819) // rw
#define OV8825_TIMING_HSYNC_FIRST_HIGH      (0x381a) // rw
#define OV8825_TIMING_HSYNC_FIRST_LOW       (0x381b) // rw
#define OV8825_TIMING_THN_X_OUPUT_SIZE_HIGH (0x381c) // rw
#define OV8825_TIMING_THN_X_OUPUT_SIZE_LOW  (0x381d) // rw
#define OV8825_TIMING_THN_Y_OUPUT_SIZE_HIGH (0x381e) // rw
#define OV8825_TIMING_THN_Y_OUPUT_SIZE_LOW  (0x381f) // rw
#define OV8825_TIMING_REG20                 (0x3820) // rw Bit[2:1] vertical flip enable 00:nomal  11:vertical flip
#define OV8825_TIMING_REG21                 (0x3821) // rw Bit[2:1] herizon mirror enable 00:nomal  11:herizon flip
#define OV8825_TIMING_REG22                 (0x3822) // rw
#define OV8825_TIMING_REG23                 (0x3823) // rw
#define OV8825_TIMING_REG24                 (0x3824) // rw
#define OV8825_TIMING_REG25                 (0x3825) // rw
#define OV8825_TIMING_REG26                 (0x3826) // rw
#define OV8825_TIMING_REG27                 (0x3827) // rw
#define OV8825_TIMING_REG28                 (0x3828) // rw
#define OV8825_TIMING_REG29                 (0x3829) // rw
#define OV8825_TIMING_XHS_CTRL              (0x382a) // rw
#define OV8825_STROBE_CTRL00                (0x3b00) // rw
#define OV8825_STROBE_CTRL01                (0x3b01) // rw
#define OV8825_STROBE_CTRL02                (0x3b02) // rw
#define OV8825_FREX_CTRL00                  (0x3b05) // rw
#define OV8825_FREX_CTRL01                  (0x3b06) // rw
#define OV8825_FREX_CTRL02                  (0x3b07) // rw
#define OV8825_FREX_CTRL03                  (0x3b08) // rw
#define OV8825_FREX_CTRL04                  (0x3b09) // rw
#define OV8825_FREX_CTRL05                  (0x3b0a) // rw
#define OV8825_FREX_CTRL06                  (0x3b0b) // rw
#define OV8825_FREX_CTRL07                  (0x3b0c) // rw
#define OV8825_FREX_CTRL08                  (0x3b0d) // rw
#define OV8825_FREX_CTRL09                  (0x3b0e) // rw
#define OV8825_FREX_CTRL0A                  (0x3b0f) // rw
#define OV8825_FREX_CTRL0B                  (0x3b10) // rw
#define OV8825_FREX_CTRL0C                  (0x3b11) // rw
#define OV8825_FREX_CTRL0D                  (0x3b1F) // rw
#define OV8825_FREX_CTRL0E                  (0x3b20) // rw

#define OV8825_OPT_DATA00                   (0x3d00) // rw
#define OV8825_OPT_DATA01                   (0x3d01) // rw
#define OV8825_OPT_DATA02                   (0x3d02) // rw
#define OV8825_OPT_DATA03                   (0x3d03) // rw
#define OV8825_OPT_DATA04                   (0x3d04) // rw
#define OV8825_OPT_DATA05                   (0x3d05) // rw
#define OV8825_OPT_DATA06                   (0x3d06) // rw
#define OV8825_OPT_DATA07                   (0x3d07) // rw
#define OV8825_OPT_DATA08                   (0x3d08) // rw
#define OV8825_OPT_DATA09                   (0x3d09) // rw
#define OV8825_OPT_DATA0A                   (0x3d0a) // rw
#define OV8825_OPT_DATA0B                   (0x3d0b) // rw
#define OV8825_OPT_DATA0C                   (0x3d0c) // rw
#define OV8825_OPT_DATA0D                   (0x3d0d) // rw
#define OV8825_OPT_DATA0E                   (0x3d0e) // rw
#define OV8825_OPT_DATA0F                   (0x3d0f) // rw
#define OV8825_OPT_DATA10                   (0x3d10) // rw
#define OV8825_OPT_DATA11                   (0x3d11) // rw
#define OV8825_OPT_DATA12                   (0x3d12) // rw
#define OV8825_OPT_DATA13                   (0x3d13) // rw
#define OV8825_OPT_DATA14                   (0x3d14) // rw
#define OV8825_OPT_DATA15                   (0x3d15) // rw
#define OV8825_OPT_DATA16                   (0x3d16) // rw
#define OV8825_OPT_DATA17                   (0x3d17) // rw
#define OV8825_OPT_DATA18                   (0x3d18) // rw
#define OV8825_OPT_DATA19                   (0x3d19) // rw
#define OV8825_OPT_DATA1A                   (0x3d1a) // rw
#define OV8825_OPT_DATA1B                   (0x3d1b) // rw
#define OV8825_OPT_DATA1C                   (0x3d1c) // rw
#define OV8825_OPT_DATA1D                   (0x3d1d) // rw
#define OV8825_OPT_DATA1E                   (0x3d1e) // rw
#define OV8825_OPT_DATA1F                   (0x3d1f) // rw
#define OV8825_OPT_DUMP_PROGRAM0            (0x3d80) // rw
#define OV8825_OPT_DUMP_PROGRAM1            (0x3d81) // r
#define OV8825_OPT_DUMP_PROGRAM2            (0x3d82) // rw
#define OV8825_OPT_DUMP_PROGRAM3            (0x3d83) // rw
#define OV8825_OPT_DUMP_PROGRAM4            (0x3d84) // rw
#define OV8825_OPT_DUMP_PROGRAM5            (0x3d85) // rw
#define OV8825_OPT_DUMP_PROGRAM6            (0x3d86) // rw
#define OV8825_OPT_DUMP_PROGRAM7            (0x3d87) // rw

#define OV8825_PSRAM_CTRL0                  (0x3F00) // rw
#define OV8825_PSRAM_CTRL1                  (0x3F01) // rw
#define OV8825_PSRAM_CTRL2                  (0x3F02) // rw
#define OV8825_PSRAM_CTRL3                  (0x3F03) // rw
#define OV8825_PSRAM_CTRL4                  (0x3F04) // rw
#define OV8825_PSRAM_CTRL5                  (0x3F05) // rw
#define OV8825_PSRAM_CTRL6                  (0x3F06) // rw
#define OV8825_PSRAM_CTRL7                  (0x3F07) // rw

#define OV8825_BLC_CTRL00                   (0x4000) // rw Bit[7] BLC bypass enable 0:disable 1:enable
#define OV8825_BLC_CTRL01                   (0x4001) // rw
#define OV8825_BLC_CTRL02                   (0x4002) // rw
#define OV8825_BLC_CTRL03                   (0x4003) // rw
#define OV8825_BLC_CTRL04                   (0x4004) // rw
#define OV8825_BLC_CTRL05                   (0x4005) // rw
#define OV8825_BLC_CTRL07                   (0x4007) // rw
#define OV8825_BLC_TARGET                   (0x4008) // rw
#define OV8825_BLC_CTRL09                   (0x4009) // rw
#define OV8825_BLC_CTRL0C                   (0x400c) // rw
#define OV8825_BLC_CTRL0D                   (0x400d) // rw
#define OV8825_BLC_CTRL0E                   (0x400e) // rw
#define OV8825_BLC_CTRL0F                   (0x400f) // rw
#define OV8825_BLC_CTRL10                   (0x4010) // rw
#define OV8825_BLC_CTRL11                   (0x4011) // rw
#define OV8825_BLC_CTRL12                   (0x4012) // rw
#define OV8825_BLC_CTRL13                   (0x4013) // rw
#define OV8825_404e                         (0x404e) //rw
#define OV8825_404f                         (0x404f) //rw
#define OV8825_FC_CTRL00                    (0x4200) // rw
#define OV8825_FC_CTRL01                    (0x4201) // rw
#define OV8825_FC_CTRL02                    (0x4202) // rw
#define OV8825_FC_CTRL03                    (0x4203) // rw
#define OV8825_FC_MAX_VALUE                 (0x4300) // rw
#define OV8825_FC_MIN_VALUE                 (0x4301) // rw
#define OV8825_FC_MAX_MIN_VALUE             (0x4302) // rw
#define OV8825_FMT_CTRL3                    (0x4303) // rw
#define OV8825_FMT_CTRL4                    (0x4304) // rw
#define OV8825_FMT_PAD_LOW1                 (0x4305) // rw
#define OV8825_FMT_PAD_LOW2                 (0x4306) // rw
#define OV8825_EMBEDED_CTRL                 (0x4307) // rw
#define OV8825_FMT_TST_X_START_HIGH         (0x4308) // rw
#define OV8825_FMT_TST_X_START_LOW          (0x4309) // rw
#define OV8825_FMT_TST_Y_START_HIGH         (0x430a) // rw
#define OV8825_FMT_TST_Y_START_LOW          (0x430b) // rw
#define OV8825_FMT_TST_WIDTH_HIGH           (0x430c) // rw
#define OV8825_FMT_TST_WIDTH_LOW            (0x430d) // rw
#define OV8825_FMT_TST_HEIGHT_HIGH          (0x430e) // rw
#define OV8825_FMT_TST_HEIGHT_LOW           (0x430f) // rw
#define OV8825_VFIFO_CTRL0                  (0x4600) // rw
#define OV8825_VFIFO_READ_ST_HIGH           (0x4601) // rw
#define OV8825_VFIFO_READ_ST_LOW            (0x4602) // rw
#define OV8825_VFIFO_CTRL3                  (0x4603) // rw
#define OV8825_VFIFO_PCLK_DIV_MAM           (0x4604) // rw
#define OV8825_MIPI_CTRL00                  (0x4800) // rw
#define OV8825_MIPI_CTRL01                  (0x4801) // rw
#define OV8825_MIPI_CTRL02                  (0x4802) // rw
#define OV8825_MIPI_CTRL03                  (0x4803) // rw
#define OV8825_MIPI_CTRL04                  (0x4804) // rw
#define OV8825_MIPI_CTRL05                  (0x4805) // rw
#define OV8825_MIPI_CTRL06                  (0x4806) // rw
#define OV8825_MIPI_CTRL07                  (0x4807) // rw
#define OV8825_MIPI_CTRL08                  (0x4808) // rw
#define OV8825_MIPI_CTRL09                  (0x4809) // rw
#define OV8825_MIPI_FCNT_MAX_H              (0x4810) // rw
#define OV8825_MIPI_FCNT_MAX_L              (0x4811) // rw
#define OV8825_MIPI_SPKT_WC_REG_H           (0x4812) // rw
#define OV8825_MIPI_SPKT_WC_REG_L           (0x4813) // rw
#define OV8825_MIPI_CTRL14                  (0x4814) // rw
#define OV8825_MIPI_DT_SPKT                 (0x4815) // rw
#define OV8825_MIPI_HS_ZERO_MIN_H           (0x4818) // rw
#define OV8825_MIPI_HS_ZERO_MIN_L           (0x4819) // rw
#define OV8825_MIPI_HS_TRAL_MIN_H           (0x481A) // rw
#define OV8825_MIPI_HS_TRAL_MIN_L           (0x481B) // rw
#define OV8825_MIPI_CLK_ZERO_MIN_H          (0x481C) // rw
#define OV8825_MIPI_CLK_ZERO_MIN_L          (0x481D) // rw
#define OV8825_MIPI_CLK_PREPARE_MIN_H       (0x481E) // rw
#define OV8825_MIPI_CLK_PREPARE_MIN_L       (0x481F) // rw
#define OV8825_MIPI_CLK_POST_MIN_H          (0x4820) // rw
#define OV8825_MIPI_CLK_POST_MIN_L          (0x4821) // rw
#define OV8825_MIPI_CLK_TRAIL_MIN_H         (0x4822) // rw
#define OV8825_MIPI_CLK_TRAIL_MIN_L         (0x4823) // rw
#define OV8825_MIPI_LPX_PCLK_MIN_H          (0x4824) // rw
#define OV8825_MIPI_LPX_PCLK_MIN_L          (0x4825) // rw
#define OV8825_MIPI_HS_PREPARE_MIN_H        (0x4826) // rw
#define OV8825_MIPI_HS_PREPARE_MIN_L        (0x4827) // rw
#define OV8825_MIPI_HS_EXIT_MIN_H           (0x4828) // rw
#define OV8825_MIPI_HS_EXIT_MIN_L           (0x4829) // rw
#define OV8825_MIPI_UI_HS_ZERO_MIN          (0x482a) // rw
#define OV8825_MIPI_UI_HS_TRAIL_MIN         (0x482b) // rw
#define OV8825_MIPI_UI_CLK_ZERO_MIN         (0x482c) // rw
#define OV8825_MIPI_UI_CLK_PREPARE_MIN      (0x482d) // rw
#define OV8825_MIPI_UI_CLK_POST_MIN         (0x482e) // rw
#define OV8825_MIPI_UI_CLK_TRAIL_MIN        (0x482f) // rw
#define OV8825_MIPI_UI_LPX_P_MIN            (0x4830) // rw
#define OV8825_MIPI_UI_HS_PREPARE_MIN       (0x4831) // rw
#define OV8825_MIPI_UI_HS_EXIT_MIN          (0x4832) // rw
#define OV8825_MIPI_REG_MIN_H               (0x4833) // rw
#define OV8825_MIPI_REG_MIN_L               (0x4834) // rw
#define OV8825_MIPI_REG_MAX_H               (0x4835) // rw
#define OV8825_MIPI_REG_MAX_L               (0x4836) // rw
#define OV8825_MIPI_PCLK_PERIOD             (0x4837) // rw
#define OV8825_MIPI_WKUP_DLY                (0x4838) // rw
#define OV8825_MIPI_DIR_DLY                 (0x483a) // rw
#define OV8825_MIPI_LP_GPIO                 (0x483b) // rw
#define OV8825_MIPI_CTRL33                  (0x483c) // rw
#define OV8825_MIPI_T_TA_GO                 (0x483d) // rw
#define OV8825_MIPI_T_TA_SURE               (0x483e) // rw
#define OV8825_MIPI_T_TA_GET                (0x483f) // rw
#define OV8825_START_OFFSET_H               (0x4840) // rw
#define OV8825_START_OFFSET_L               (0x4841) // rw
#define OV8825_START_START                  (0x4842) // rw
#define OV8825_SNR_PCLK_DIV                 (0x4843) // rw
#define OV8825_MIPI_CTRL4A                  (0x484A) // rw
#define OV8825_MIPI_CTRL4B                  (0x484B) // rw
#define OV8825_MIPI_CTRL4C                  (0x484C) // rw
#define OV8825_MIPI_CTRL4D                  (0x484D) // rw
#define OV8825_MIPI_CTRL4E                  (0x484E) // rw
#define OV8825_MIPI_CTRL4F                  (0x484F) // rw
#define OV8825_MIPI_CTRL5A                  (0x484A) // rw
#define OV8825_MIPI_CTRL5B                  (0x484B) // rw
#define OV8825_MIPI_CTRL5C                  (0x484C) // rw
#define OV8825_ISP_CTRL00                   (0x5000) // rw Bit[7] LENC enable  Bit[2] bad pixel cancellation enable  Bit[1] white pixel cancellation enable
#define OV8825_ISP_CTRL01                   (0x5001) // rw Bit[0] Manual white balance enable
#define OV8825_ISP_CTRL02                   (0x5002) // rw NOT Used
#define OV8825_ISP_CTRL03                   (0x5003) // rw Bit[5] Buffer control enable
#define OV8825_ISP_CTRL04                   (0x5004) // rw Bit[3] Debug mode
#define OV8825_ISP_CTRL05                   (0x5005) // rw Bit[4] MWB bias ON enable
#define OV8825_ISP_CTRL06                   (0x5006) // rw Debug mode 
#define OV8825_ISP_CTRL07                   (0x5007) // rw Debug mode 
#define OV8825_ISP_CTRL08                   (0x5008) // rw Debug mode 
#define OV8825_ISP_CTRL09                   (0x5009) // rw Debug mode 
#define OV8825_ISP_CTRL0A                   (0x500a) // rw Debug mode 
#define OV8825_ISP_CTRL0B                   (0x500b) // rw Debug mode
#define OV8825_ISP_CTRL0C                   (0x500c) // rw Debug mode 
#define OV8825_ISP_CTRL0D                   (0x500d) // rw Debug mode 
#define OV8825_ISP_CTRL0E                   (0x500e) // rw Debug mode 
#define OV8825_ISP_CTRL0F                   (0x500f) // rw Debug mode 
#define OV8825_ISP_CTRL10                   (0x5010) // rw Debug mode
#define OV8825_ISP_CTRL11                   (0x5011) // rw Debug mode 
#define OV8825_ISP_CTRL1F                   (0x501f) // rw Bit[5] Bypass isp 
#define OV8825_ISP_CTRL25                   (0x5025) // rw Bit[1:0] avg_sel 00:sensor raw 01:after LENC 10:after MWB gain
#define OV8825_ISP_CTRL2A                   (0x502a) // rw Bit[7:4] mux_pad_ctrl[3:0]
#define OV8825_ISP_CTRL3D                   (0x503d) // rw Debug mode 
#define OV8825_ISP_CTRL3E                   (0x503e) // rw Debug mode 
#define OV8825_ISP_CTRL3F                   (0x503f) // rw Debug mode 
#define OV8825_ISP_CTRL41                   (0x5041) // rw Bit[7] manual scale select  Bit[5]manual scale enable  Bit[4]post bining filter enable Bit[2]Average enable
#define OV8825_ISP_CTRL43                   (0x5043) // rw Debug mode 
#define OV8825_ISP_CTRL44                   (0x5044) // rw Debug mode 
#define OV8825_ISP_CTRL45                   (0x5045) // rw Debug mode 
#define OV8825_ISP_CTRL46                   (0x5046) // rw Debug mode 
#define OV8825_ISP_CTRL47                   (0x5047) // rw Debug mode
#define OV8825_ISP_CTRL48                   (0x5048) // rw Debug mode 
#define OV8825_ISP_CTRL49                   (0x5049) // rw Debug mode 
#define OV8825_ISP_CTRL4A                   (0x504a) // rw Debug mode 
#define OV8825_ISP_CTRL4B                   (0x504b) // rw Debug mode 
#define OV8825_ISP_CTRL4C                   (0x504c) // rw Debug mode
#define OV8825_ISP_CTRL4D                   (0x504d) // rw Debug mode 
#define OV8825_SCALE_CTRL00                 (0x5600) // rw
#define OV8825_SCALE_CTRL01                 (0x5601) // rw
#define OV8825_SCALE_CTRL02                 (0x5602) // rw
#define OV8825_SCALE_CTRL03                 (0x5603) // rw
#define OV8825_SCALE_CTRL04                 (0x5604) // rw
#define OV8825_HSCALE_CTRL                  (0x5068) // rw
#define OV8825_VSCALE_CTRL                  (0x506a) // rw
#define OV8825_SCALE_CTRL05                 (0x5605) // r
#define OV8825_SCALE_CTRL06                 (0x5606) // r
#define OV8825_SCALE_CTRL07                 (0x5607) // r
#define OV8825_SCALE_CTRL08                 (0x5608) // r
#define OV8825_AVG_CTRL00                   (0x5680) //rw
#define OV8825_AVG_CTRL01                   (0x5681) //rw
#define OV8825_AVG_CTRL02                   (0x5682) //rw
#define OV8825_AVG_CTRL03                   (0x5683) //rw
#define OV8825_AVG_CTRL04                   (0x5684) //rw
#define OV8825_AVG_CTRL05                   (0x5685) //rw
#define OV8825_AVG_CTRL06                   (0x5686) //rw
#define OV8825_AVG_CTRL07                   (0x5687) //rw
#define OV8825_AVG_CTRL08                   (0x5688) //rw
#define OV8825_DPCC_CTRL00                  (0x5780) //rw
#define OV8825_DPCC_CTRL01                  (0x5781) //rw
#define OV8825_DPCC_CTRL02                  (0x5782) //rw
#define OV8825_DPCC_CTRL03                  (0x5783) //rw
#define OV8825_DPCC_CTRL04                  (0x5784) //rw
#define OV8825_DPCC_CTRL05                  (0x5785) //rw
#define OV8825_DPCC_CTRL06                  (0x5786) //rw
#define OV8825_DPCC_CTRL07                  (0x5787) //rw
#define OV8825_DPCC_CTRL08                  (0x5788) //rw
#define OV8825_DPCC_CTRL09                  (0x5789) //rw
#define OV8825_DPCC_CTRL0A                  (0x578a) //rw
#define OV8825_DPCC_CTRL0B                  (0x578b) //rw
#define OV8825_DPCC_CTRL0C                  (0x578c) //rw
#define OV8825_DPCC_CTRL0D                  (0x578d) //rw
#define OV8825_DPCC_CTRL0E                  (0x578e) //rw
#define OV8825_DPCC_CTRL0F                  (0x578f) //rw
#define OV8825_DPCC_CTRL10                  (0x5790) //rw
#define OV8825_DPCC_CTRL11                  (0x5791) //rw
#define OV8825_LENC_CTRL00                  (0x5800) //rw
#define OV8825_LENC_CTRL01                  (0x5801) //rw
#define OV8825_LENC_CTRL02                  (0x5802) //rw
#define OV8825_LENC_CTRL03                  (0x5803) //rw
#define OV8825_LENC_CTRL04                  (0x5804) //rw
#define OV8825_LENC_CTRL05                  (0x5805) //rw
#define OV8825_LENC_CTRL06                  (0x5806) //rw
#define OV8825_LENC_CTRL07                  (0x5807) //rw
#define OV8825_LENC_CTRL08                  (0x5808) //rw
#define OV8825_LENC_CTRL09                  (0x5809) //rw
#define OV8825_LENC_CTRL0A                  (0x580a) //rw
#define OV8825_LENC_CTRL0B                  (0x580b) //rw
#define OV8825_LENC_CTRL0C                  (0x580c) //rw
#define OV8825_LENC_CTRL0D                  (0x580d) //rw
#define OV8825_LENC_CTRL0E                  (0x580e) //rw
#define OV8825_LENC_CTRL0F                  (0x580f) //rw
#define OV8825_LENC_CTRL10                  (0x5810) //rw
#define OV8825_LENC_CTRL11                  (0x5811) //rw
#define OV8825_LENC_CTRL12                  (0x5812) //rw
#define OV8825_LENC_CTRL13                  (0x5813) //rw
#define OV8825_LENC_CTRL14                  (0x5814) //rw
#define OV8825_LENC_CTRL15                  (0x5815) //rw
#define OV8825_LENC_CTRL16                  (0x5816) //rw
#define OV8825_LENC_CTRL17                  (0x5817) //rw
#define OV8825_LENC_CTRL18                  (0x5818) //rw
#define OV8825_LENC_CTRL19                  (0x5819) //rw
#define OV8825_LENC_CTRL1A                  (0x581a) //rw
#define OV8825_LENC_CTRL1B                  (0x581b) //rw
#define OV8825_LENC_CTRL1C                  (0x581c) //rw
#define OV8825_LENC_CTRL1D                  (0x581d) //rw
#define OV8825_LENC_CTRL1E                  (0x581e) //rw
#define OV8825_LENC_CTRL1F                  (0x581f) //rw
#define OV8825_LENC_CTRL20                  (0x5820) //rw
#define OV8825_LENC_CTRL21                  (0x5821) //rw
#define OV8825_LENC_CTRL22                  (0x5822) //rw
#define OV8825_LENC_CTRL23                  (0x5823) //rw
#define OV8825_LENC_CTRL24                  (0x5824) //rw
#define OV8825_LENC_CTRL25                  (0x5825) //rw
#define OV8825_LENC_CTRL26                  (0x5826) //rw
#define OV8825_LENC_CTRL27                  (0x5827) //rw
#define OV8825_LENC_CTRL28                  (0x5828) //rw
#define OV8825_LENC_CTRL29                  (0x5829) //rw
#define OV8825_LENC_CTRL2A                  (0x582a) //rw
#define OV8825_LENC_CTRL2B                  (0x582b) //rw
#define OV8825_LENC_CTRL2C                  (0x582c) //rw
#define OV8825_LENC_CTRL2D                  (0x582d) //rw
#define OV8825_LENC_CTRL2E                  (0x582e) //rw
#define OV8825_LENC_CTRL2F                  (0x582f) //rw
#define OV8825_LENC_CTRL30                  (0x5830) //rw
#define OV8825_LENC_CTRL31                  (0x5831) //rw
#define OV8825_LENC_CTRL32                  (0x5832) //rw
#define OV8825_LENC_CTRL33                  (0x5833) //rw
#define OV8825_LENC_CTRL34                  (0x5834) //rw
#define OV8825_LENC_CTRL35                  (0x5835) //rw
#define OV8825_LENC_CTRL36                  (0x5836) //rw
#define OV8825_LENC_CTRL37                  (0x5837) //rw
#define OV8825_LENC_CTRL38                  (0x5838) //rw
#define OV8825_LENC_CTRL39                  (0x5839) //rw
#define OV8825_LENC_CTRL3A                  (0x583a) //rw
#define OV8825_LENC_CTRL3B                  (0x583b) //rw
#define OV8825_LENC_CTRL3C                  (0x583c) //rw
#define OV8825_LENC_CTRL3D                  (0x583d) //rw
#define OV8825_LENC_CTRL3E                  (0x583e) //rw
#define OV8825_LENC_CTRL3F                  (0x583f) //rw
#define OV8825_LENC_CTRL40                  (0x5840) //rw
#define OV8825_LENC_CTRL41                  (0x5841) //rw
#define OV8825_LENC_CTRL42                  (0x5842) //rw
#define OV8825_LENC_CTRL43                  (0x5843) //rw
#define OV8825_LENC_CTRL44                  (0x5844) //rw
#define OV8825_LENC_CTRL45                  (0x5845) //rw
#define OV8825_LENC_CTRL46                  (0x5846) //rw
#define OV8825_LENC_CTRL47                  (0x5847) //rw
#define OV8825_LENC_CTRL48                  (0x5848) //rw
#define OV8825_LENC_CTRL49                  (0x5849) //rw
#define OV8825_LENC_CTRL50                  (0x5850) //r
#define OV8825_LENC_CTRL51                  (0x5851) //r
#define OV8825_LENC_CTRL52                  (0x5852) //r
#define OV8825_LENC_CTRL53                  (0x5853) //r
#define OV8825_LENC_CTRL54                  (0x5854) //r
#define OV8825_LENC_CTRL55                  (0x5855) //r
#define OV8825_LENC_CTRL56                  (0x5856) //r
#define OV8825_PBLC_CTRL00                  (0x5c00) //rw
#define OV8825_PBLC_CTRL01                  (0x5c01) //rw
#define OV8825_PBLC_CTRL02                  (0x5c02) //rw
#define OV8825_PBLC_CTRL03                  (0x5c03) //rw
#define OV8825_PBLC_CTRL04                  (0x5c04) //rw
#define OV8825_PBLC_CTRL05                  (0x5c05) //rw
#define OV8825_PBLC_CTRL06                  (0x5c06) //rw
#define OV8825_PBLC_CTRL07                  (0x5c07) //rw
#define OV8825_PBLC_CTRL08                  (0x5c08) //rw
#define OV8825_PRE_ISP_CTRL00                  (0x5e00) //rw
#define OV8825_PRE_ISP_CTRL01                  (0x5e01) //rw
#define OV8825_PRE_ISP_CTRL02                  (0x5e02) //rw
#define OV8825_PRE_ISP_CTRL03                  (0x5e03) //rw
#define OV8825_PRE_ISP_CTRL04                  (0x5e04) //rw
#define OV8825_PRE_ISP_CTRL05                  (0x5e05) //rw
#define OV8825_PRE_ISP_CTRL06                  (0x5e06) //rw
#define OV8825_PRE_ISP_CTRL07                  (0x5e07) //rw
#define OV8825_PRE_ISP_CTRL08                  (0x5e08) //rw
#define OV8825_PRE_ISP_CTRL09                  (0x5e09) //rw
#define OV8825_PRE_ISP_CTRL10                  (0x5e0a) //rw
#define OV8825_PRE_ISP_CTRL11                  (0x5e0b) //rw
#define OV8825_PRE_ISP_CTRL12                  (0x5e0c) //r
#define OV8825_PRE_ISP_CTRL13                  (0x5e0d) //r
#define OV8825_PRE_ISP_CTRL14                  (0x5e0e) //r
#define OV8825_PRE_ISP_CTRL15                  (0x5e0f) //r
#define OV8825_PRE_ISP_CTRL16                  (0x5e10) //rw
#define OV8825_PRE_ISP_CTRL17                  (0x5e11) //rw
#define OV8825_ILLUMINATION_CTRL00             (0x6600) //rw
#define OV8825_ILLUMINATION_CTRL01             (0x6601) //rw
#define OV8825_ILLUMINATION_CTRL02             (0x6602) //rw
#define OV8825_ILLUMINATION_CTRL03             (0x6603) //rw
#define OV8825_ILLUMINATION_CTRL04             (0x6604) //rw
#define OV8825_ILLUMINATION_CTRL05             (0x6605) //rw
#define OV8825_ILLUMINATION_CTRL06             (0x6606) //rw
#define OV8825_ILLUMINATION_CTRL07             (0x6607) //rw
#define OV8825_ILLUMINATION_CTRL08             (0x6608) //rw
#define OV8825_ILLUMINATION_CTRL09             (0x6609) //rw
#define OV8825_ILLUMINATION_CTRL0A             (0x660a) //rw
#define OV8825_ILLUMINATION_CTRL0B             (0x660b) //rw
#define OV8825_ILLUMINATION_CTRL0C             (0x660c) //rw
#define OV8825_ILLUMINATION_CTRL0D             (0x660d) //rw
#define OV8825_ILLUMINATION_CTRL0E             (0x660e) //rw
#define OV8825_ILLUMINATION_CTRL0F             (0x660f) //rw
#define OV8825_ILLUMINATION_CTRL10             (0x6610) //rw
#define OV8825_ILLUMINATION_CTRL11             (0x6611) //rw
#define OV8825_TMP_CTRL00                      (0x6700) //rw
#define OV8825_TMP_CTRL01                      (0x6701) //rw
#define OV8825_TMP_CTRL02                      (0x6702) //rw
#define OV8825_TMP_CTRL03                      (0x6703) //rw
#define OV8825_TMP_CTRL04                      (0x6704) //rw
#define OV8825_TMP_CTRL05                      (0x6705) //rw
#define OV8825_TMP_CTRL06                      (0x6706) //rw
#define OV8825_TMP_CTRL07                      (0x6707) //rw
#define OV8825_TMP_CTRL08                      (0x6708) //rw
#define OV8825_TMP_CTRL09                      (0x6709) //rw
#define OV8825_TMP_CTRL0A                      (0x670a) //rw
#define OV8825_TMP_CTRL0B                      (0x670b) //rw
#define OV8825_TMP_CTRL0C                      (0x670c) //rw
#define OV8825_TMP_CTRL0D                      (0x670d) //rw
#define OV8825_TMP_CTRL0E                      (0x670e) //rw
#define OV8825_TMP_CTRL0F                      (0x670f) //rw
#define OV8825_TMP_CTRL10                      (0x6710) //rw
#define OV8825_TMP_CTRL11                      (0x6711) //rw
#define OV8825_TMP_CTRL12                      (0x6712) //rw
#define OV8825_TMP_CTRL13                      (0x6713) //rw
#define OV8825_TMP_CTRL14                      (0x6714) //rw
#define OV8825_TMP_CTRL15                      (0x6715) //rw
#define OV8825_TMP_CTRL16                      (0x6716) //rw
#define OV8825_TMP_CTRL17                      (0x6717) //rw
#define OV8825_TMP_CTRL18                      (0x6718) //rw
#define OV8825_TMP_CTRL19                      (0x6719) //rw
#define OV8825_TMP_CTRL1A                      (0x671a) //rw
#define OV8825_TMP_CTRL1B                      (0x671b) //rw
#define OV8825_TMP_CTRL1C                      (0x671c) //rw
#define OV8825_TMP_CTRL1D                      (0x671d) //rw
#define OV8825_TMP_CTRL1E                      (0x671e) //rw
#define OV8825_TMP_CTRL1F                      (0x671f) //rw
#define OV8825_TMP_CTRL20                      (0x6720) //rw
#define OV8825_TMP_CTRL21                      (0x6721) //rw
#define OV8825_6800                            (0x6800)
#define OV8825_6801                            (0x6801)
#define OV8825_6802                            (0x6802)
#define OV8825_6803                            (0x6803)
#define OV8825_6804                            (0x6804)
#define OV8825_CADC_CTRL00                     (0x6900) //rw
#define OV8825_CADC_CTRL01                     (0x6901) //rw

#define OV8825_3300                    (0x3300) //rw
#define OV8825_3724                    (0x3724) //rw
#define OV8825_3725                    (0x3725) //rw
#define OV8825_3726                    (0x3726) //rw
#define OV8825_3727                    (0x3727) //rw

/*****************************************************************************
 * System control registers
 *****************************************************************************/
#define OV8825_DELAY_5MS_DEFAULT                    (0x05)
#define OV8825_MODE_SELECT_DEFAULT                  (0x00)
#define OV8825_SOFTWARE_RST_DEFAULT                 (0x00)
#define OV8825_PAD_ONE2_DEFAULT                     (0x16)
#define OV8825_SCCB_ID_DEFAULT                      (0x6c)
#define OV8825_PLL_CTRL0_DEFAULT                    (0xce)
#define OV8825_PLL_CTRL1_DEFAULT                    (0xd4)
#define OV8825_PLL_CTRL2_DEFAULT                    (0x00)
#define OV8825_PLL_CTRL3_DEFAULT                    (0x10)
#define OV8825_PLL_CTRL4_DEFAULT                    (0x3B)
#define OV8825_SC_PAD_OUT2_DEFAULT                  (0x00) // rw - 
#define OV8825_SC_PAD_SEL0_DEFAULT                  (0x00) // rw - 
#define OV8825_SC_PAD_SEL1_DEFAULT                  (0x00) // rw - 
#define OV8825_SC_PAD_SEL2_DEFAULT                  (0x00) // rw - 
#define OV8825_SC_PLL_CTRL_S2_DEFAULT               (0x01) // rw - 
#define OV8825_SC_PLL_CTRL_S0_DEFAULT               (0x80) // rw -
#define OV8825_SC_PLL_CTRL_S1_DEFAULT               (0x39) // rw - 
#define OV8825_MIPI_SC_CTRL_DEFAULT                 (0x00) // rw - 
#define OV8825_SC_CLKRST0_DEFAULT                   (0x71) // rw - 
#define OV8825_SC_CLKRST1_DEFAULT                   (0xf0) // rw - 
#define OV8825_SC_CLKRST2_DEFAULT                   (0xf4) // rw - 
#define OV8825_SC_CLKRST3_DEFAULT                   (0xb4) // rw - 
#define OV8825_SC_CLKRST4_DEFAULT                   (0xf1) // rw - 
#define OV8825_SC_FREX_MASK0_DEFAULT                (0x09) // rw - 
#define OV8825_SC_CLK_SEL_DEFAULT                   (0x01) // rw - 
#define OV8825_SC_MISC_CTRL_DEFAULT                 (0x03) // rw - 
#define OV8825_SRAM_TST_DEFAULT                     (0x00) // rw - 
#define OV8825_SRAM_VAL0_DEFAULT                    (0xff) // rw - 
#define OV8825_SRAM_VAL1_DEFAULT                    (0x09) // rw -
#define OV8825_SCCB_CTRL0_DEFAULT                   (0x00) // rw - 
#define OV8825_SCCB_CTRL1_DEFAULT                   (0x12) // rw -  
#define OV8825_SCCB_CTRL2_DEFAULT                   (0x00) // rw - 
#define OV8825_SCCB_PLL_DEFAULT                     (0x20) // rw -  
#define OV8825_SRB_CTRL_DEFAULT                     (0x15) // rw - 
#define OV8825_GROUP_ADDR0_DEFAULT                  (0x00) // rw -
#define OV8825_GROUP_ADDR1_DEFAULT                  (0x10) // rw -
#define OV8825_GROUP_ADDR2_DEFAULT                  (0x1f) // rw -
#define OV8825_GROUP_ADDR3_DEFAULT                  (0x1f) // rw -
#define OV8825_GROUP_LENG0_DEFAULT                  (0x00) // rw -
#define OV8825_GROUP_LENG1_DEFAULT                  (0x00) // rw -
#define OV8825_GROUP_LENG2_DEFAULT                  (0x00) // rw -
#define OV8825_GROUP_LENG3_DEFAULT                  (0x00) // rw -
#define OV8825_GRP0_PERIOD_DEFAULT                  (0x00) // rw -
#define OV8825_GRP1_PERIOD_DEFAULT                  (0x00) // rw -
#define OV8825_GRP_SWCTRL_DEFAULT                   (0x01) // rw -
#define OV8825_GRP_SRAM_DEFAULT                     (0x0a) // rw -
#define OV8825_MWB_GAIN00_DEFAULT                   (0x04) // rw-
#define OV8825_MWB_GAIN01_DEFAULT                   (0x00) // rw-
#define OV8825_MWB_GAIN02_DEFAULT                   (0x04) // rw-
#define OV8825_MWB_GAIN03_DEFAULT                   (0x00) // rw-
#define OV8825_MWB_GAIN04_DEFAULT                   (0x04) // rw-
#define OV8825_MWB_GAIN05_DEFAULT                   (0x00) // rw-
#define OV8825_MWB_GAIN06_DEFAULT                   (0x00) // rw-
#define OV8825_AEC_EXPO_H_DEFAULT                   (0x00) // rw- Bit[3:0] exposure[19:16]
#define OV8825_AEC_EXPO_M_DEFAULT                   (0x4e) // rw- Bit[7:0] exposure[15:8]
#define OV8825_AEC_EXPO_L_DEFAULT                   (0xa0) // rw- Bit[7:0] exposure[7:0] low 4 bits are fraction bits which are not supportted and should always be 0.
#define OV8825_AEC_MANUAL_DEFAULT                   (0x07) // rw- Bit[5:4] gain delay option  Bit[2]VTS manual enable  Bit[1]AGC manual enable  Bit[0]AEC manual enable 
#define OV8825_MAN_GAIN_H_DEFAULT                   (0x00) // rw- Bit[1:0] man gain[9:8]
#define OV8825_MAN_GAIN_L_DEFAULT                   (0x00) // rw- Bit[7:0] man gain[7:0]
#define OV8825_ADD_VTS_H_DEFAULT                    (0x00) // rw- Bit[7:0] add dummy line VTS[15:8]
#define OV8825_ADD_VTS_L_DEFAULT                    (0x00) // rw- Bit[7:0] add dummy line VTS[7:0]
#define OV8825_AEC09_DEFAULT                        (0x10)  // rw- Bit[4] sensor gain convert enalbe 0:use sensor gain(0x350a 0x350b) 0x00 is 1x gain
                                                           //     Bit[3] gain manual enable 0:manual disable use register 0x350a/0x350b   1:manual enable use register 0x3504/0x3505
#define OV8825_AEC_AGC_ADJ_H_DEFAULT                (0x00) // rw- Bit[2:0]gain output to sensor Gain[10:8]
#define OV8825_AEC_AGC_ADJ_L_DEFAULT                (0x1f) // rw-
#define OV8825_ANACTRL0_DEFAULT                     (0x06) // rw   
#define OV8825_ANACTRL1_DEFAULT                     (0x34) // rw 
#define OV8825_ANACTRL2_DEFAULT                     (0x42) // rw 
#define OV8825_ANACTRL3_DEFAULT                     (0x5c) // rw 
#define OV8825_ANACTRL4_DEFAULT                     (0x98) // rw 
#define OV8825_ANACTRL5_DEFAULT                     (0xf5) // rw 
#define OV8825_ANACTRL6_DEFAULT                     (0x02) // rw 
#define OV8825_ANACTRL7_DEFAULT                     (0x02) // rw 
#define OV8825_ANACTRL8_DEFAULT                     (0x00) // rw 
#define OV8825_ANACTRL9_DEFAULT                     (0xb4) // rw 
#define OV8825_ANACTRLA_DEFAULT                     (0x7c) // rw 
#define OV8825_ANACTRLB_DEFAULT                     (0xc9) // rw 
#define OV8825_ANACTRLC_DEFAULT                     (0x0b) // rw 
#define OV8825_ANACTRLD_DEFAULT                     (0x00) // rw 
#define OV8825_ANACTRLE_DEFAULT                     (0x00) // rw 
#define OV8825_ANACTRLF_DEFAULT                     (0x48) // rw 
#define OV8825_ANACTRL10_DEFAULT                    (0x00) // rw 
#define OV8825_ANACTRL11_DEFAULT                    (0x00) // rw 
#define OV8825_ANACTRL12_DEFAULT                    (0x00) // rw - Bit[7]analog control Bit[6:5] PAD io drive capability 00:1x 01:2x 10:3x 11:4x  Bit[0]analog control
#define OV8825_ANACTRL13_DEFAULT                    (0x02) // rw 
#define OV8825_ANACTRL14_DEFAULT                    (0x0f) // rw 
#define OV8825_ANACTRL15_DEFAULT                    (0x00) // rw 
#define OV8825_ANACTRL16_DEFAULT                    (0x03) // rw 
#define OV8825_ANACTRL17_DEFAULT                    (0xa1) // rw 
#define OV8825_A_VCM_LOW_DEFAULT                    (0x00) // rw - Bit[7:4]-din[3:0]  Bit[3]-s3  Bit[2:0]-s[2:0]
#define OV8825_A_VCM_MID0_DEFAULT                   (0x00) // rw
#define OV8825_A_VCM_MID1_DEFAULT                   (0x00) // rw
#define OV8825_A_VCM_MID2_DEFAULT                   (0x00) // rw
#define OV8825_A_VCM_HIGH_DEFAULT                   (0x00) // rw
#define OV8825_ANACTRL1D_DEFAULT                    (0x00) // rw
#define OV8825_ANACTRL1E_DEFAULT                    (0x00) // rw
#define OV8825_SENCTRL0_DEFAULT                     (0x20) // rw
#define OV8825_SENCTRL1_DEFAULT                     (0x44) // rw
#define OV8825_SENCTRL2_DEFAULT                     (0x50) // rw
#define OV8825_SENCTRL3_DEFAULT                     (0xcc) // rw
#define OV8825_SENCTRL4_DEFAULT                     (0x19) // rw
#define OV8825_SENCTRL5_DEFAULT                     (0x32) // rw
#define OV8825_SENCTRL6_DEFAULT                     (0x4b) // rw
#define OV8825_SENCTRL7_DEFAULT                     (0x63) // rw
#define OV8825_SENCTRL8_DEFAULT                     (0x84) // rw
#define OV8825_SENCTRL9_DEFAULT                     (0x40) // rw
#define OV8825_SENCTRLA_DEFAULT                     (0x33) // rw
#define OV8825_SENCTRLB_DEFAULT                     (0x01) // rw
#define OV8825_SENCTRLC_DEFAULT                     (0x50) // rw
#define OV8825_SENCTRLD_DEFAULT                     (0x00) // rw
#define OV8825_SENCTRLE_DEFAULT                     (0x00) // rw
#define OV8825_SENCTRLF_DEFAULT                     (0x00) // rw
#define OV8825_SENCTRL10_DEFAULT                    (0x02) // rw
#define OV8825_SENCTRL11_DEFAULT                    (0x0f) // rw
#define OV8825_SENCTRL12_DEFAULT                    (0x9c) // rw
#define OV8825_TIMING_HS_H_DEFAULT                  (0x00) // rw
#define OV8825_TIMING_HS_L_DEFAULT                  (0x00) // rw
#define OV8825_TIMING_VS_H_DEFAULT                  (0x00) // rw
#define OV8825_TIMING_VS_L_DEFAULT                  (0x00) // rw
#define OV8825_TIMING_HW_H_DEFAULT                  (0x0c) // rw
#define OV8825_TIMING_HW_L_DEFAULT                  (0xdf) // rw
#define OV8825_TIMING_VH_H_DEFAULT                  (0x09) // rw
#define OV8825_TIMING_VH_L_DEFAULT                  (0x9B) // rw
#define OV8825_TIMING_ISPHO_H_DEFAULT               (0x06) // rw
#define OV8825_TIMING_ISPHO_L_DEFAULT               (0x60) // rw
#define OV8825_TIMING_ISPVO_H_DEFAULT               (0x04) // rw
#define OV8825_TIMING_ISPVO_L_DEFAULT               (0xc8) // rw
#define OV8825_TIMING_HTS_H_DEFAULT                 (0x0d) // rw
#define OV8825_TIMING_HTS_L_DEFAULT                 (0xbc) // rw
#define OV8825_TIMING_VTS_H_DEFAULT                 (0x04) // rw
#define OV8825_TIMING_VTS_L_DEFAULT                 (0xf0) // rw
#define OV8825_TIMING_HOFFS_HIGH_DEFAULT            (0x00) // rw
#define OV8825_TIMING_HOFFS_LOW_DEFAULT             (0x08) // rw
#define OV8825_TIMING_VOFFS_HIGH_DEFAULT            (0x00) // rw
#define OV8825_TIMING_VOFFS_LOW_DEFAULT             (0x04) // rw
#define OV8825_TIMING_X_INC_DEFAULT                 (0x31) // rw
#define OV8825_TIMING_Y_INC_DEFAULT                 (0x31) // rw
#define OV8825_TIMING_HSYNC_START_HIGH_DEFAULT      (0x02) // rw
#define OV8825_TIMING_HSYNC_START_LOW_DEFAULT       (0x40) // rw
#define OV8825_TIMING_HSYNC_END_HIGH_DEFAULT        (0x00) // rw
#define OV8825_TIMING_HSYNC_END_LOW_DEFAULT         (0x40) // rw
#define OV8825_TIMING_HSYNC_FIRST_HIGH_DEFAULT      (0x00) // rw
#define OV8825_TIMING_HSYNC_FIRST_LOW_DEFAULT       (0x00) // rw
#define OV8825_TIMING_THN_X_OUPUT_SIZE_HIGH_DEFAULT (0x00) // rw
#define OV8825_TIMING_THN_X_OUPUT_SIZE_LOW_DEFAULT  (0x00) // rw
#define OV8825_TIMING_THN_Y_OUPUT_SIZE_HIGH_DEFAULT (0x00) // rw
#define OV8825_TIMING_THN_Y_OUPUT_SIZE_LOW_DEFAULT  (0x00) // rw
#define OV8825_TIMING_REG20_DEFAULT                 (0x81) // rw
#define OV8825_TIMING_REG21_DEFAULT                 (0x17) // rw
#define OV8825_TIMING_REG22_DEFAULT                 (0x48) // rw
#define OV8825_TIMING_REG23_DEFAULT                 (0x40) // rw
#define OV8825_TIMING_REG24_DEFAULT                 (0x00) // rw
#define OV8825_TIMING_REG25_DEFAULT                 (0x00) // rw
#define OV8825_TIMING_REG26_DEFAULT                 (0x00) // rw
#define OV8825_TIMING_REG27_DEFAULT                 (0x00) // rw
#define OV8825_TIMING_REG28_DEFAULT                 (0x0b) // rw
#define OV8825_TIMING_REG29_DEFAULT                 (0x00) // rw
#define OV8825_TIMING_XHS_CTRL_DEFAULT              (0x10) // rw
#define OV8825_STROBE_CTRL00_DEFAULT                (0x00) // rw
#define OV8825_STROBE_CTRL01_DEFAULT                (0x00) // rw
#define OV8825_STROBE_CTRL02_DEFAULT                (0x00) // rw
#define OV8825_FREX_CTRL00_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL01_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL02_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL03_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL04_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL05_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL06_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL07_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL08_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL09_DEFAULT                  (0x01) // rw
#define OV8825_FREX_CTRL0A_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL0B_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL0C_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL0D_DEFAULT                  (0x00) // rw
#define OV8825_FREX_CTRL0E_DEFAULT                  (0x00) // rw
#define OV8825_BLC_CTRL00_DEFAULT                   (0x29) // rw
#define OV8825_BLC_CTRL01_DEFAULT                   (0x02) // rw
#define OV8825_BLC_CTRL02_DEFAULT                   (0x45) // rw
#define OV8825_BLC_CTRL03_DEFAULT                   (0x08) // rw
#define OV8825_BLC_CTRL04_DEFAULT                   (0x04) // rw
#define OV8825_BLC_CTRL05_DEFAULT                   (0x18) // rw
#define OV8825_BLC_CTRL07_DEFAULT                   (0x00) // rw
#define OV8825_BLC_TARGET_DEFAULT                   (0x00) // rw
#define OV8825_BLC_CTRL09_DEFAULT                   (0x10) // rw
#define OV8825_BLC_CTRL0C_DEFAULT                   (0x00) // rw
#define OV8825_BLC_CTRL0D_DEFAULT                   (0x00) // rw
#define OV8825_BLC_CTRL0E_DEFAULT                   (0x00) // rw
#define OV8825_BLC_CTRL0F_DEFAULT                   (0x00) // rw
#define OV8825_BLC_CTRL10_DEFAULT                   (0x00) // rw
#define OV8825_BLC_CTRL11_DEFAULT                   (0x00) // rw
#define OV8825_BLC_CTRL12_DEFAULT                   (0x00) // rw
#define OV8825_BLC_CTRL13_DEFAULT                   (0x00) // rw
#define OV8825_404e_DEFAULT                         (0x37) //rw
#define OV8825_404f_DEFAULT                         (0x8f) //rw
#define OV8825_FC_CTRL00_DEFAULT                    (0x00) // rw
#define OV8825_FC_CTRL01_DEFAULT                    (0x00) // rw
#define OV8825_FC_CTRL02_DEFAULT                    (0x00) // rw
#define OV8825_FC_CTRL03_DEFAULT                    (0x00) // rw
#define OV8825_FC_MAX_VALUE_DEFAULT                 (0xff) // rw
#define OV8825_FC_MIN_VALUE_DEFAULT                 (0x00) // rw
#define OV8825_FC_MAX_MIN_VALUE_DEFAULT             (0x0c) // rw
#define OV8825_FMT_CTRL3_DEFAULT                    (0x00) // rw
#define OV8825_FMT_CTRL4_DEFAULT                    (0x08) // rw
#define OV8825_FMT_PAD_LOW1_DEFAULT                 (0x40) // rw
#define OV8825_FMT_PAD_LOW2_DEFAULT                 (0x0e) // rw
#define OV8825_EMBEDED_CTRL_DEFAULT                 (0x00) // rw
#define OV8825_FMT_TST_X_START_HIGH_DEFAULT         (0x00) // rw
#define OV8825_FMT_TST_X_START_LOW_DEFAULT          (0x00) // rw
#define OV8825_FMT_TST_Y_START_HIGH_DEFAULT         (0x00) // rw
#define OV8825_FMT_TST_Y_START_LOW_DEFAULT          (0x00) // rw
#define OV8825_FMT_TST_WIDTH_HIGH_DEFAULT           (0x00) // rw
#define OV8825_FMT_TST_WIDTH_LOW_DEFAULT            (0x00) // rw
#define OV8825_FMT_TST_HEIGHT_HIGH_DEFAULT          (0x00) // rw
#define OV8825_FMT_TST_HEIGHT_LOW_DEFAULT           (0x00) // rw
#define OV8825_VFIFO_CTRL0_DEFAULT                  (0x04) // rw
#define OV8825_VFIFO_READ_ST_HIGH_DEFAULT           (0x00) // rw
#define OV8825_VFIFO_READ_ST_LOW_DEFAULT            (0x30) // rw
#define OV8825_VFIFO_CTRL3_DEFAULT                  (0x0a) // rw
#define OV8825_VFIFO_PCLK_DIV_MAM_DEFAULT           (0x01) // rw
#define OV8825_MIPI_CTRL00_DEFAULT                  (0x04) // rw
#define OV8825_MIPI_CTRL01_DEFAULT                  (0x0f) // rw
#define OV8825_MIPI_CTRL02_DEFAULT                  (0x00) // rw
#define OV8825_MIPI_CTRL03_DEFAULT                  (0x50) // rw
#define OV8825_MIPI_CTRL04_DEFAULT                  (0x8d) // rw
#define OV8825_MIPI_CTRL05_DEFAULT                  (0x10) // rw
#define OV8825_MIPI_CTRL06_DEFAULT                  (0x88) // rw
#define OV8825_MIPI_CTRL07_DEFAULT                  (0xa0) // rw
#define OV8825_MIPI_CTRL08_DEFAULT                  (0x02) // rw
#define OV8825_MIPI_CTRL09_DEFAULT                  (0x01) // rw
#define OV8825_MIPI_FCNT_MAX_H_DEFAULT              (0xff) // rw
#define OV8825_MIPI_FCNT_MAX_L_DEFAULT              (0xff) // rw
#define OV8825_MIPI_SPKT_WC_REG_H_DEFAULT           (0x00) // rw
#define OV8825_MIPI_SPKT_WC_REG_L_DEFAULT           (0x00) // rw
#define OV8825_MIPI_CTRL14_DEFAULT                  (0x2a) // rw
#define OV8825_MIPI_DT_SPKT_DEFAULT                 (0x00) // rw
#define OV8825_MIPI_HS_ZERO_MIN_H_DEFAULT           (0x00) // rw
#define OV8825_MIPI_HS_ZERO_MIN_L_DEFAULT           (0x96) // rw
#define OV8825_MIPI_HS_TRAL_MIN_H_DEFAULT           (0x00) // rw
#define OV8825_MIPI_HS_TRAL_MIN_L_DEFAULT           (0x3c) // rw
#define OV8825_MIPI_CLK_ZERO_MIN_H_DEFAULT          (0x01) // rw
#define OV8825_MIPI_CLK_ZERO_MIN_L_DEFAULT          (0x1c) // rw
#define OV8825_MIPI_CLK_PREPARE_MIN_H_DEFAULT       (0x00) // rw
#define OV8825_MIPI_CLK_PREPARE_MIN_L_DEFAULT       (0x3c) // rw
#define OV8825_MIPI_CLK_POST_MIN_H_DEFAULT          (0x00) // rw
#define OV8825_MIPI_CLK_POST_MIN_L_DEFAULT          (0x56) // rw
#define OV8825_MIPI_CLK_TRAIL_MIN_H_DEFAULT         (0x00) // rw
#define OV8825_MIPI_CLK_TRAIL_MIN_L_DEFAULT         (0x3c) // rw
#define OV8825_MIPI_LPX_PCLK_MIN_H_DEFAULT          (0x00) // rw
#define OV8825_MIPI_LPX_PCLK_MIN_L_DEFAULT          (0x32) // rw
#define OV8825_MIPI_HS_PREPARE_MIN_H_DEFAULT        (0x00) // rw
#define OV8825_MIPI_HS_PREPARE_MIN_L_DEFAULT        (0x32) // rw
#define OV8825_MIPI_HS_EXIT_MIN_H_DEFAULT           (0x00) // rw
#define OV8825_MIPI_HS_EXIT_MIN_L_DEFAULT           (0x64) // rw
#define OV8825_MIPI_UI_HS_ZERO_MIN_DEFAULT          (0x05) // rw
#define OV8825_MIPI_UI_HS_TRAIL_MIN_DEFAULT         (0x04) // rw
#define OV8825_MIPI_UI_CLK_ZERO_MIN_DEFAULT         (0x00) // rw
#define OV8825_MIPI_UI_CLK_PREPARE_MIN_DEFAULT      (0x00) // rw
#define OV8825_MIPI_UI_CLK_POST_MIN_DEFAULT         (0x34) // rw
#define OV8825_MIPI_UI_CLK_TRAIL_MIN_DEFAULT        (0x00) // rw
#define OV8825_MIPI_UI_LPX_P_MIN_DEFAULT            (0x00) // rw
#define OV8825_MIPI_UI_HS_PREPARE_MIN_DEFAULT       (0x04) // rw
#define OV8825_MIPI_UI_HS_EXIT_MIN_DEFAULT          (0x00) // rw
#define OV8825_MIPI_REG_MIN_H_DEFAULT               (0x00) // rw
#define OV8825_MIPI_REG_MIN_L_DEFAULT               (0x00) // rw
#define OV8825_MIPI_REG_MAX_H_DEFAULT               (0xff) // rw
#define OV8825_MIPI_REG_MAX_L_DEFAULT               (0xff) // rw
#define OV8825_MIPI_PCLK_PERIOD_DEFAULT             (0x28) // rw
#define OV8825_MIPI_WKUP_DLY_DEFAULT                (0x02) // rw
#define OV8825_MIPI_DIR_DLY_DEFAULT                 (0x08) // rw
#define OV8825_MIPI_LP_GPIO_DEFAULT                 (0x33) // rw
#define OV8825_MIPI_CTRL33_DEFAULT                  (0x42) // rw
#define OV8825_MIPI_T_TA_GO_DEFAULT                 (0x10) // rw
#define OV8825_MIPI_T_TA_SURE_DEFAULT               (0x06) // rw
#define OV8825_MIPI_T_TA_GET_DEFAULT                (0x14) // rw
#define OV8825_START_OFFSET_H_DEFAULT               (0x00) // rw
#define OV8825_START_OFFSET_L_DEFAULT               (0x00) // rw
#define OV8825_START_START_DEFAULT                  (0x01) // rw
#define OV8825_SNR_PCLK_DIV_DEFAULT                 (0x02) // rw
#define OV8825_MIPI_CTRL4A_DEFAULT                  (0x00) // rw
#define OV8825_MIPI_CTRL4B_DEFAULT                  (0x33) // rw
#define OV8825_MIPI_CTRL4C_DEFAULT                  (0x02) // rw
#define OV8825_MIPI_CTRL4D_DEFAULT                  (0xb6) // rw
#define OV8825_MIPI_CTRL4E_DEFAULT                  (0xb6) // rw
#define OV8825_MIPI_CTRL4F_DEFAULT                  (0xb6) // rw
#define OV8825_MIPI_CTRL5A_DEFAULT                  (0xb6) // rw
#define OV8825_MIPI_CTRL5B_DEFAULT                  (0x01) // rw
#define OV8825_MIPI_CTRL5C_DEFAULT                  (0x3f) // rw
#define OV8825_ISP_CTRL00_DEFAULT                   (0x06) // rw Bit[7] LENC enable  Bit[2] bad pixel cancellation enable  Bit[1] white pixel cancellation enable
#define OV8825_ISP_CTRL01_DEFAULT                   (0x00) // rw Bit[0] Manual white balance enable
#define OV8825_ISP_CTRL02_DEFAULT                   (0x00) // rw NOT Used
#define OV8825_ISP_CTRL03_DEFAULT                   (0x20) // rw Bit[5] Buffer control enable
#define OV8825_ISP_CTRL04_DEFAULT                   (0x0c) // rw Bit[3] Debug mode
#define OV8825_ISP_CTRL05_DEFAULT                   (0xdc) // rw Bit[4] MWB bias ON enable
#define OV8825_ISP_CTRL06_DEFAULT                   (0x11) // rw Debug mode 
#define OV8825_ISP_CTRL07_DEFAULT                   (0x50) // rw Debug mode 
#define OV8825_ISP_CTRL08_DEFAULT                   (0x0c) // rw Debug mode 
#define OV8825_ISP_CTRL09_DEFAULT                   (0xfc) // rw Debug mode 
#define OV8825_ISP_CTRL0A_DEFAULT                   (0x11) // rw Debug mode 
#define OV8825_ISP_CTRL0B_DEFAULT                   (0x40) // rw Debug mode
#define OV8825_ISP_CTRL0C_DEFAULT                   (0x0c) // rw Debug mode 
#define OV8825_ISP_CTRL0D_DEFAULT                   (0xf0) // rw Debug mode 
#define OV8825_ISP_CTRL0E_DEFAULT                   (0x11) // rw Debug mode 
#define OV8825_ISP_CTRL0F_DEFAULT                   (0x50) // rw Debug mode 
#define OV8825_ISP_CTRL10_DEFAULT                   (0x0c) // rw Debug mode
#define OV8825_ISP_CTRL11_DEFAULT                   (0xfc) // rw Debug mode 
#define OV8825_ISP_CTRL1F_DEFAULT                   (0x00) // rw Bit[5] Bypass isp 
#define OV8825_ISP_CTRL25_DEFAULT                   (0x00) // rw Bit[1:0] avg_sel 00:sensor raw 01:after LENC 10:after MWB gain
#define OV8825_ISP_CTRL2A_DEFAULT                   (0x00) // rw Bit[7:4] mux_pad_ctrl[3:0]
#define OV8825_ISP_CTRL3D_DEFAULT                   (0x08) // rw Debug mode 
#define OV8825_ISP_CTRL3E_DEFAULT                   (0x00) // rw Debug mode 
#define OV8825_ISP_CTRL3F_DEFAULT                   (0x20) // rw Debug mode
#define OV8825_ISP_CTRL41_DEFAULT                   (0x0c) // rw Bit[7] manual scale select  Bit[5]manual scale enable  Bit[4]post bining filter enable Bit[2]Average enable
#define OV8825_ISP_CTRL43_DEFAULT                   (0x08) // rw Debug mode 
#define OV8825_ISP_CTRL44_DEFAULT                   (0x00) // rw Debug mode 
#define OV8825_ISP_CTRL45_DEFAULT                   (0x00) // rw Debug mode 
#define OV8825_ISP_CTRL46_DEFAULT                   (0x00) // rw Debug mode 
#define OV8825_ISP_CTRL47_DEFAULT                   (0x00) // rw Debug mode
#define OV8825_ISP_CTRL48_DEFAULT                   (0x10) // rw Debug mode 
#define OV8825_ISP_CTRL49_DEFAULT                   (0x00) // rw Debug mode 
#define OV8825_ISP_CTRL4A_DEFAULT                   (0x00) // rw Debug mode 
#define OV8825_ISP_CTRL4B_DEFAULT                   (0x00) // rw Debug mode 
#define OV8825_ISP_CTRL4C_DEFAULT                   (0x00) // rw Debug mode
#define OV8825_ISP_CTRL4D_DEFAULT                   (0x00) // rw Debug mode
#define OV8825_SCALE_CTRL00_DEFAULT                 (0x0c) // rw
#define OV8825_SCALE_CTRL01_DEFAULT                 (0x00) // rw
#define OV8825_SCALE_CTRL02_DEFAULT                 (0x00) // rw
#define OV8825_SCALE_CTRL03_DEFAULT                 (0x00) // rw
#define OV8825_SCALE_CTRL04_DEFAULT                 (0x01) // rw
#define OV8825_HSCALE_CTRL_DEFAULT                  (0x00) // rw
#define OV8825_VSCALE_CTRL_DEFAULT                  (0x00) // rw
#define OV8825_AVG_CTRL00_DEFAULT                   (0x00) //rw
#define OV8825_AVG_CTRL01_DEFAULT                   (0x00) //rw
#define OV8825_AVG_CTRL02_DEFAULT                   (0x00) //rw
#define OV8825_AVG_CTRL03_DEFAULT                   (0x00) //rw
#define OV8825_AVG_CTRL04_DEFAULT                   (0x10) //rw
#define OV8825_AVG_CTRL05_DEFAULT                   (0xa0) //rw
#define OV8825_AVG_CTRL06_DEFAULT                   (0x0c) //rw
#define OV8825_AVG_CTRL07_DEFAULT                   (0x78) //rw
#define OV8825_AVG_CTRL08_DEFAULT                   (0x02) //rw

#define OV8825_DPCC_CTRL00_DEFAULT                  (0xfc) //rw
#define OV8825_DPCC_CTRL01_DEFAULT                  (0x13) //rw
#define OV8825_DPCC_CTRL02_DEFAULT                  (0x03) //rw
#define OV8825_DPCC_CTRL03_DEFAULT                  (0x08) //rw
#define OV8825_DPCC_CTRL04_DEFAULT                  (0x0c) //rw
#define OV8825_DPCC_CTRL05_DEFAULT                  (0x10) //rw
#define OV8825_DPCC_CTRL06_DEFAULT                  (0x08) //rw
#define OV8825_DPCC_CTRL07_DEFAULT                  (0x10) //rw
#define OV8825_DPCC_CTRL08_DEFAULT                  (0x10) //rw
#define OV8825_DPCC_CTRL09_DEFAULT                  (0x08) //rw
#define OV8825_DPCC_CTRL0A_DEFAULT                  (0x04) //rw
#define OV8825_DPCC_CTRL0B_DEFAULT                  (0x02) //rw
#define OV8825_DPCC_CTRL0C_DEFAULT                  (0x02) //rw
#define OV8825_DPCC_CTRL0D_DEFAULT                  (0x0c) //rw
#define OV8825_DPCC_CTRL0E_DEFAULT                  (0x06) //rw
#define OV8825_DPCC_CTRL0F_DEFAULT                  (0x02) //rw
#define OV8825_DPCC_CTRL10_DEFAULT                  (0x02) //rw
#define OV8825_DPCC_CTRL11_DEFAULT                  (0xff) //rw

#define OV8825_LENC_CTRL00_DEFAULT                  (0x0f) //rw
#define OV8825_LENC_CTRL01_DEFAULT                  (0x0d) //rw
#define OV8825_LENC_CTRL02_DEFAULT                  (0x09) //rw
#define OV8825_LENC_CTRL03_DEFAULT                  (0x0a) //rw
#define OV8825_LENC_CTRL04_DEFAULT                  (0x0d) //rw
#define OV8825_LENC_CTRL05_DEFAULT                  (0x14) //rw
#define OV8825_LENC_CTRL06_DEFAULT                  (0x0a) //rw
#define OV8825_LENC_CTRL07_DEFAULT                  (0x04) //rw
#define OV8825_LENC_CTRL08_DEFAULT                  (0x03) //rw
#define OV8825_LENC_CTRL09_DEFAULT                  (0x03) //rw
#define OV8825_LENC_CTRL0A_DEFAULT                  (0x05) //rw
#define OV8825_LENC_CTRL0B_DEFAULT                  (0x0a) //rw
#define OV8825_LENC_CTRL0C_DEFAULT                  (0x05) //rw
#define OV8825_LENC_CTRL0D_DEFAULT                  (0x02) //rw
#define OV8825_LENC_CTRL0E_DEFAULT                  (0x00) //rw
#define OV8825_LENC_CTRL0F_DEFAULT                  (0x00) //rw
#define OV8825_LENC_CTRL10_DEFAULT                  (0x03) //rw
#define OV8825_LENC_CTRL11_DEFAULT                  (0x05) //rw
#define OV8825_LENC_CTRL12_DEFAULT                  (0x09) //rw
#define OV8825_LENC_CTRL13_DEFAULT                  (0x03) //rw
#define OV8825_LENC_CTRL14_DEFAULT                  (0x01) //rw
#define OV8825_LENC_CTRL15_DEFAULT                  (0x01) //rw
#define OV8825_LENC_CTRL16_DEFAULT                  (0x04) //rw
#define OV8825_LENC_CTRL17_DEFAULT                  (0x09) //rw
#define OV8825_LENC_CTRL18_DEFAULT                  (0x09) //rw
#define OV8825_LENC_CTRL19_DEFAULT                  (0x08) //rw
#define OV8825_LENC_CTRL1A_DEFAULT                  (0x06) //rw
#define OV8825_LENC_CTRL1B_DEFAULT                  (0x06) //rw
#define OV8825_LENC_CTRL1C_DEFAULT                  (0x08) //rw
#define OV8825_LENC_CTRL1D_DEFAULT                  (0x06) //rw
#define OV8825_LENC_CTRL1E_DEFAULT                  (0x33) //rw
#define OV8825_LENC_CTRL1F_DEFAULT                  (0x11) //rw
#define OV8825_LENC_CTRL20_DEFAULT                  (0x0e) //rw
#define OV8825_LENC_CTRL21_DEFAULT                  (0x0f) //rw
#define OV8825_LENC_CTRL22_DEFAULT                  (0x11) //rw
#define OV8825_LENC_CTRL23_DEFAULT                  (0x3f) //rw
#define OV8825_LENC_CTRL24_DEFAULT                  (0x08) //rw
#define OV8825_LENC_CTRL25_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL26_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL27_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL28_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL29_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL2A_DEFAULT                  (0x42) //rw
#define OV8825_LENC_CTRL2B_DEFAULT                  (0x42) //rw
#define OV8825_LENC_CTRL2C_DEFAULT                  (0x44) //rw
#define OV8825_LENC_CTRL2D_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL2E_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL2F_DEFAULT                  (0x60) //rw
#define OV8825_LENC_CTRL30_DEFAULT                  (0x62) //rw
#define OV8825_LENC_CTRL31_DEFAULT                  (0x42) //rw
#define OV8825_LENC_CTRL32_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL33_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL34_DEFAULT                  (0x44) //rw
#define OV8825_LENC_CTRL35_DEFAULT                  (0x44) //rw
#define OV8825_LENC_CTRL36_DEFAULT                  (0x44) //rw
#define OV8825_LENC_CTRL37_DEFAULT                  (0x48) //rw
#define OV8825_LENC_CTRL38_DEFAULT                  (0x28) //rw
#define OV8825_LENC_CTRL39_DEFAULT                  (0x46) //rw
#define OV8825_LENC_CTRL3A_DEFAULT                  (0x48) //rw
#define OV8825_LENC_CTRL3B_DEFAULT                  (0x68) //rw
#define OV8825_LENC_CTRL3C_DEFAULT                  (0x28) //rw
#define OV8825_LENC_CTRL3D_DEFAULT                  (0xae) //rw
#define OV8825_LENC_CTRL3E_DEFAULT                  (0x40) //rw
#define OV8825_LENC_CTRL3F_DEFAULT                  (0x20) //rw
#define OV8825_LENC_CTRL40_DEFAULT                  (0x18) //rw
#define OV8825_LENC_CTRL41_DEFAULT                  (0x0d) //rw
#define OV8825_LENC_CTRL42_DEFAULT                  (0x00) //rw
#define OV8825_LENC_CTRL43_DEFAULT                  (0xef) //rw
#define OV8825_LENC_CTRL44_DEFAULT                  (0x01) //rw
#define OV8825_LENC_CTRL45_DEFAULT                  (0x3f) //rw
#define OV8825_LENC_CTRL46_DEFAULT                  (0x01) //rw
#define OV8825_LENC_CTRL47_DEFAULT                  (0x3f) //rw
#define OV8825_LENC_CTRL48_DEFAULT                  (0x00) //rw
#define OV8825_LENC_CTRL49_DEFAULT                  (0xd5) //rw
#define OV8825_PBLC_CTRL00_DEFAULT                  (0x80) //rw
#define OV8825_PBLC_CTRL01_DEFAULT                  (0x00) //rw
#define OV8825_PBLC_CTRL02_DEFAULT                  (0x00) //rw
#define OV8825_PBLC_CTRL03_DEFAULT                  (0x00) //rw
#define OV8825_PBLC_CTRL04_DEFAULT                  (0x00) //rw
#define OV8825_PBLC_CTRL05_DEFAULT                  (0x00) //rw
#define OV8825_PBLC_CTRL06_DEFAULT                  (0x00) //rw
#define OV8825_PBLC_CTRL07_DEFAULT                  (0x80) //rw
#define OV8825_PBLC_CTRL08_DEFAULT                  (0x10) //rw
#define OV8825_PRE_ISP_CTRL00_DEFAULT                  (0x00) //rw
#define OV8825_PRE_ISP_CTRL01_DEFAULT                  (0x41) //rw
#define OV8825_PRE_ISP_CTRL02_DEFAULT                  (0x00) //rw
#define OV8825_PRE_ISP_CTRL03_DEFAULT                  (0x01) //rw
#define OV8825_PRE_ISP_CTRL04_DEFAULT                  (0x00) //rw
#define OV8825_PRE_ISP_CTRL05_DEFAULT                  (0x00) //rw
#define OV8825_PRE_ISP_CTRL06_DEFAULT                  (0x01) //rw
#define OV8825_PRE_ISP_CTRL07_DEFAULT                  (0x00) //rw
#define OV8825_PRE_ISP_CTRL08_DEFAULT                  (0x00) //rw
#define OV8825_PRE_ISP_CTRL09_DEFAULT                  (0x00) //rw
#define OV8825_PRE_ISP_CTRL10_DEFAULT                  (0x00) //rw
#define OV8825_PRE_ISP_CTRL11_DEFAULT                  (0x01) //rw
#define OV8825_PRE_ISP_CTRL16_DEFAULT                  (0x0c) //rw
#define OV8825_PRE_ISP_CTRL17_DEFAULT                  (0x00) //rw
#define OV8825_ILLUMINATION_CTRL00_DEFAULT             (0x10) //rw
#define OV8825_ILLUMINATION_CTRL01_DEFAULT             (0x10) //rw
#define OV8825_ILLUMINATION_CTRL02_DEFAULT             (0x10) //rw
#define OV8825_ILLUMINATION_CTRL03_DEFAULT             (0x10) //rw
#define OV8825_ILLUMINATION_CTRL04_DEFAULT             (0x11) //rw
#define OV8825_ILLUMINATION_CTRL05_DEFAULT             (0x11) //rw
#define OV8825_ILLUMINATION_CTRL06_DEFAULT             (0x1F) //rw
#define OV8825_ILLUMINATION_CTRL07_DEFAULT             (0x1F) //rw
#define OV8825_ILLUMINATION_CTRL08_DEFAULT             (0x1F) //rw
#define OV8825_ILLUMINATION_CTRL09_DEFAULT             (0x1F) //rw
#define OV8825_ILLUMINATION_CTRL0A_DEFAULT             (0x00) //rw
#define OV8825_ILLUMINATION_CTRL0B_DEFAULT             (0x00) //rw
#define OV8825_ILLUMINATION_CTRL0C_DEFAULT             (0x00) //rw
#define OV8825_ILLUMINATION_CTRL0D_DEFAULT             (0x00) //rw
#define OV8825_ILLUMINATION_CTRL0E_DEFAULT             (0x00) //rw
#define OV8825_ILLUMINATION_CTRL0F_DEFAULT             (0x02) //rw
#define OV8825_ILLUMINATION_CTRL10_DEFAULT             (0x01) //rw
#define OV8825_ILLUMINATION_CTRL11_DEFAULT             (0x01) //rw
#define OV8825_TMP_CTRL00_DEFAULT                      (0x05) //rw
#define OV8825_TMP_CTRL01_DEFAULT                      (0x19) //rw
#define OV8825_TMP_CTRL02_DEFAULT                      (0xfd) //rw
#define OV8825_TMP_CTRL03_DEFAULT                      (0xd7) //rw
#define OV8825_TMP_CTRL04_DEFAULT                      (0xff) //rw
#define OV8825_TMP_CTRL05_DEFAULT                      (0xff) //rw
#define OV8825_TMP_CTRL06_DEFAULT                      (0x78) //rw
#define OV8825_TMP_CTRL0B_DEFAULT                      (0x00) //rw
#define OV8825_CADC_CTRL00_DEFAULT                     (0x60) //rw
#define OV8825_CADC_CTRL01_DEFAULT                     (0x04) //rw
#define OV8825_OPT_DATA00_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA01_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA02_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA03_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA04_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA05_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA06_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA07_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA08_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA09_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA0A_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA0B_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA0C_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA0D_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA0E_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA0F_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA10_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA11_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA12_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA13_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA14_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA15_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA16_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA17_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA18_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA19_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA1A_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA1B_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA1C_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA1D_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA1E_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DATA1F_DEFAULT                   (0x00) // rw
#define OV8825_OPT_DUMP_PROGRAM0_DEFAULT            (0x00) // rw
#define OV8825_OPT_DUMP_PROGRAM1_DEFAULT            (0x00) // r
#define OV8825_OPT_DUMP_PROGRAM2_DEFAULT            (0x7d) // rw
#define OV8825_OPT_DUMP_PROGRAM3_DEFAULT            (0x05) // rw
#define OV8825_OPT_DUMP_PROGRAM4_DEFAULT            (0x00) // rw
#define OV8825_OPT_DUMP_PROGRAM5_DEFAULT            (0x03) // rw
#define OV8825_OPT_DUMP_PROGRAM6_DEFAULT            (0x03) // rw
#define OV8825_OPT_DUMP_PROGRAM7_DEFAULT            (0x06) // rw
#define OV8825_PSRAM_CTRL0_DEFAULT                  (0x00) // rw
#define OV8825_PSRAM_CTRL1_DEFAULT                  (0xfc) // rw
#define OV8825_PSRAM_CTRL2_DEFAULT                  (0x06) // rw
#define OV8825_PSRAM_CTRL3_DEFAULT                  (0x66) // rw
#define OV8825_PSRAM_CTRL4_DEFAULT                  (0x00) // rw
#define OV8825_PSRAM_CTRL5_DEFAULT                  (0x10) // rw
#define OV8825_PSRAM_CTRL6_DEFAULT                  (0x00) // rw
#define OV8825_PSRAM_CTRL7_DEFAULT                  (0x00) // rw

#define OV8825_3300_DEFAULT                    (0x00) //rw
#define OV8825_3724_DEFAULT                    (0x01) //rw
#define OV8825_3725_DEFAULT                    (0x92) //rw
#define OV8825_3726_DEFAULT                    (0x01) //rw
#define OV8825_3727_DEFAULT                    (0xc7) //rw
#define OV8825_6800_DEFAULT                            (0x10)
#define OV8825_6801_DEFAULT                            (0x02)
#define OV8825_6802_DEFAULT                            (0x90)
#define OV8825_6803_DEFAULT                            (0x10)
#define OV8825_6804_DEFAULT                            (0x59)

#define OV8825_CHIP_ID_HIGH_BYTE_DEFAULT            (0x88) // r - 
#define OV8825_CHIP_ID_MIDDLE_BYTE_DEFAULT          (0x25) // r - 
#define OV8825_CHIP_ID_LOW_BYTE_DEFAULT             (0x00) // r - 
/*****************************************************************************
* Further defines for driver management
*****************************************************************************/
#define OV8825_DRIVER_INIT              (0x00000001)

/*****************************************************************************
 * ov14825 context structure
 *****************************************************************************/
typedef struct OV8825_VcmInfo_s                 /* ddl@rock-chips.com: v0.3.0 */
{
    uint32_t StartCurrent;
    uint32_t RatedCurrent;
    uint32_t Step;
    uint32_t StepMode;
} OV8825_VcmInfo_t;
 
typedef struct OV8825_Context_s
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

    uint16_t            OldGain;               /**< gain multiplier */
    uint32_t            OldCoarseIntegrationTime;
    uint32_t            OldFineIntegrationTime;

    IsiSensorMipiInfo   IsiSensorMipiInfo;

    OV8825_VcmInfo_t    VcmInfo;              /* ddl@rock-chips.com: v0.3.0 */
} OV8825_Context_t;

#ifdef __cplusplus
}
#endif

#endif
