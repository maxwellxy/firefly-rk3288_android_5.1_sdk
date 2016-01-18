//OV8858_MIPI_priv.h
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
*v0.1.0x00 : first version:preview && focus;   1: 2lane 3264x2448 is ok    2: 4lane can't preview now //hkw
*v0.2.0x00 : fix 2lane 3264x2448;
*v0.3.0:  tunning first version;
*v0.4.0:
*   1). limit AecMinIntegrationTime 0.0001 for aec.
*v0.5.0:
*   1). add sensor drv version in get sensor i2c info func
*v0.6.0:
*   1). support for isi v0.5.0
*v0.7.0:
*	1)fix AE difference between preview and capture;
*	2)fix MOTOR speed;
*v0.8.0:
*   1)support 1 lane;
*v0.9.0:
*   1) support for isi v0.6.0
*v0.a.0
*   1). support for isi v0.7.0
*v0.b.0
*   1). support mutil framerate and Afps;
*v0.c.0
*   1)  Skip frames when resolution change in OV8858_IsiChangeSensorResolutionIss;
*v0.d.0
*   1). support OTP;
*v0.e.0
*   1). support OTP i2c info;
*v0.f.0
*   1). support R1A&R2A OTP info;
*v1.0.0
*   1). fix some issues in v0.f.0;
*v1.1.0
*   1). fix somme issuse in r2a
*   2). support different otp rg bg typetical value
*v1.2.0
*   1). support another type of R1A.
*/


#define CONFIG_SENSOR_DRV_VERSION  KERNEL_VERSION(1, 2, 0)

/*****************************************************************************
 * System control registers
 *****************************************************************************/
//hkw  
#define OV8858_MODE_SELECT                  (0x0100) // rw - Bit[7:1]not used  Bit[0]Streaming set 0: software_standby  1: streaming       
#define OV8858_MODE_SELECT_OFF              (0x00U)
#define OV8858_MODE_SELECT_ON				(0x01U)

#define OV8858_SOFTWARE_RST                 (0x0103) // rw - Bit[7:1]not used  Bit[0]software_reset
#define OV8858_SOFTWARE_RST_VALUE			(0x01)

#define OV8858_CHIP_ID_HIGH_BYTE            (0x300a) // r - 
#define OV8858_CHIP_ID_MIDDLE_BYTE          (0x300b) // r - 
#define OV8858_CHIP_ID_LOW_BYTE             (0x300c) // r -  
#define OV8858_CHIP_ID_HIGH_BYTE_DEFAULT            (0x00) // r - 
#define OV8858_CHIP_ID_MIDDLE_BYTE_DEFAULT          (0x88) // r - 
#define OV8858_CHIP_ID_LOW_BYTE_DEFAULT             (0x58) //(0x65) // r - 

                                                    //     Bit[3] gain manual enable 0:manual disable use register 0x350a/0x350b   1:manual enable use register 0x3504/0x3505
#define OV8858_AEC_AGC_ADJ_H                (0x3508) // rw- Bit[2:0]gain output to sensor Gain[10:8]
#define OV8858_AEC_AGC_ADJ_L                (0x3509) // rw- Bit[7:0]gain output to sensor Gain[7:0] 

#define OV8858_AEC_EXPO_H                   (0x3500) // rw- Bit[3:0] exposure[19:16]
#define OV8858_AEC_EXPO_M                   (0x3501) // rw- Bit[7:0] exposure[15:8]
#define OV8858_AEC_EXPO_L                   (0x3502) // rw- Bit[7:0] exposure[7:0] low 4 bits are fraction bits which are not supportted and should always be 0.



/*****************************************************************************
 * ov14825 context structure
 *****************************************************************************/
typedef struct OV8858_VcmInfo_s                 /* ddl@rock-chips.com: v0.3.0 */
{
    uint32_t StartCurrent;
    uint32_t RatedCurrent;
    uint32_t Step;
    uint32_t StepMode;
} OV8858_VcmInfo_t;
typedef struct OV8858_Context_s
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
	OV8858_VcmInfo_t    VcmInfo; 
} OV8858_Context_t;

#ifdef __cplusplus
}
#endif

#endif
