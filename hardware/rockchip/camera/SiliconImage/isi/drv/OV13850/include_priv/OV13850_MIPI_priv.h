//OV13850_MIPI_priv.h
/*****************************************************************************/
/*!
 *  \file        OV13850_priv.h \n
 *  \version     1.0 \n
 *  \author      Meinicke \n
 *  \brief       Private header file for sensor specific code of the OV13850. \n
 *
 *  \revision    $Revision: 432 $ \n
 *               $Author: neugebaa $ \n
 *               $Date: 2009-06-30 11:48:59 +0200 (Di, 30 Jun 2009) $ \n
 *               $Id: OV13850_priv.h 432 2009-06-30 09:48:59Z neugebaa $ \n
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
#ifndef _OV13850_PRIV_H
#define _OV13850_PRIV_H

#include "isi_priv.h"

#if( OV13850_DRIVER_USAGE == USE_CAM_DRV_EN )
*/


#ifndef __OV13850_PRIV_H__
#define __OV13850_PRIV_H__

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
*v0.1.0 : ov13850 driver ok, can preview, no tuning yet //oyyf
*
*v0.2.0 : ov13850 add 1lane setting, 4224x3136 1lane can't preview yet  //oyyf
*v0.3.0:
*   1). limit AecMinIntegrationTime 0.0001 for aec.
*v0.4.0:
*   1). add sensor drv version in get sensor i2c info func
*v0.5.0:
*   1). support for isi v0.5.0
*v0.6.0:
*   1). af optimization.
v0.7.0
*   1). support for isi v0.6.0
*v0.8.0
*   1). support for isi v0.7.0
*v0.9.0
*	1). support mutil framerate and Afps;
*	2). skip frames when resolution change in OV13850_IsiChangeSensorResolutionIss;
*/


#define CONFIG_SENSOR_DRV_VERSION KERNEL_VERSION(0, 9, 0x00) 


#define OV13850_CHIP_ID_HIGH_BYTE            (0x300a) // r - 
#define OV13850_CHIP_ID_LOW_BYTE          (0x300b) // r - 

#define OV13850_CHIP_ID_HIGH_BYTE_DEFAULT            (0xD8) // r - 
#define OV13850_CHIP_ID_LOW_BYTE_DEFAULT          (0x50) // r - 

#define OV13850_MODE_SELECT  (0x0100)

#define OV13850_SOFTWARE_RST                 (0x0103) // rw - Bit[7:1]not used  Bit[0]software_reset

typedef struct OV13850_VcmInfo_s                 /* ddl@rock-chips.com: v0.3.0 */
{
    uint32_t StartCurrent;
    uint32_t RatedCurrent;
    uint32_t Step;
    uint32_t StepMode;
} OV13850_VcmInfo_t;

typedef struct OV13850_Context_s
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
	OV13850_VcmInfo_t    VcmInfo; 
} OV13850_Context_t;

#ifdef __cplusplus
}
#endif

#endif

