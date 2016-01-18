//ov8825 the same with ov14825

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
 * @file OV8825.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"

#include "OV8825_MIPI_priv.h"

#define  OV8825_NEWEST_TUNING_XML "30-April-2014_OUYANG_OV8825_FX288_v0.1.0"

#define CC_OFFSET_SCALING  2.0f
#define I2C_COMPLIANT_STARTBIT 1U

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( OV8825_INFO , "OV8825: ", INFO,    0U );
CREATE_TRACER( OV8825_WARN , "OV8825: ", WARNING, 1U );
CREATE_TRACER( OV8825_ERROR, "OV8825: ", ERROR,   1U );
CREATE_TRACER( OV8825_DEBUG, "OV8825: ", INFO,     1U );
CREATE_TRACER(OV8825_NOTICE0, "OV8825: ", TRACE_NOTICE0, 1);
CREATE_TRACER(OV8825_NOTICE1, "OV8825: ", TRACE_NOTICE1, 1);


#define OV8825_SLAVE_ADDR       0x6cU                           /**< i2c slave address of the OV8825 camera sensor */
#define OV8825_SLAVE_AF_ADDR    0x6cU                           /**< i2c slave address of the OV8825 integrated AD5820 */

#define OV8825_MIN_GAIN_STEP   ( 1.0f / 16.0f); /**< min gain step size used by GUI ( 32/(32-7) - 32/(32-6); min. reg value is 6 as of datasheet; depending on actual gain ) */
#define OV8825_MAX_GAIN_AEC    ( 8.0f )            /**< max. gain used by the AEC (arbitrarily chosen, recommended by Omnivision) */


/*!<
 * Focus position values:
 * 65 logical positions ( 0 - 64 )
 * where 64 is the setting for infinity and 0 for macro
 * corresponding to
 * 1024 register settings (0 - 1023)
 * where 0 is the setting for infinity and 1023 for macro
 */
#define MAX_LOG   64U
#define MAX_VCMDRV_CURRENT      100U
#define MAX_VCMDRV_REG          1023U

#if 0
/*!<
 * Lens movement is triggered every 133ms (VGA, 7.5fps processed frames
 * worst case assumed, usually even much slower, see OV5630 driver for
 * details). Thus the lens has to reach the requested position after
 * max. 133ms. Minimum mechanical ringing is expected with mode 1 ,
 * 100us per step. A movement over the full range needs max. 102.3ms
 * (see table 9 AD5820 datasheet).
 */
#define MDI_SLEW_RATE_CTRL 13U /* S3..0 */
#endif


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
const char OV8825_g_acName[] = "OV8825_MIPI";
extern const IsiRegDescription_t OV8825_g_aRegDescription_onelane[];
extern const IsiRegDescription_t OV8825_g_aRegDescription_twolane[];
const IsiSensorCaps_t OV8825_g_IsiSensorDefaultConfig;

/* AWB specific value (from OV8825_tables.c) */
extern const Isi1x1FloatMatrix_t    OV8825_KFactor;
extern const Isi3x2FloatMatrix_t    OV8825_PCAMatrix;
extern const Isi3x1FloatMatrix_t    OV8825_SVDMeanValue;
extern const IsiLine_t              OV8825_CenterLine;
extern const IsiAwbClipParm_t       OV8825_AwbClipParm;
extern const IsiAwbGlobalFadeParm_t OV8825_AwbGlobalFadeParm;
extern const IsiAwbFade2Parm_t      OV8825_AwbFade2Parm;

/* illumination profiles */
#include "OV8825_a.h"       /* CIE A - default profile */
#include "OV8825_f2.h"      /* CIE F2 (cool white flourescent CWF) */
#include "OV8825_d50.h"     /* CIE D50 (D50 lightbox) */
#include "OV8825_d65.h"     /* CIE D65 (D65) note: indoor because of our lightbox */
#include "OV8825_d75.h"     /* CIE D75 (D75) overcast daylight, 7500K */
#include "OV8825_f11.h"     /* CIE F11 (TL84) */

#define OV8825_I2C_START_BIT        (I2C_COMPLIANT_STARTBIT)    // I2C bus start condition
#define OV8825_I2C_NR_ADR_BYTES     (2U)                        // 1 byte base address and 2 bytes sub address
#define OV8825_I2C_NR_DAT_BYTES     (1U)                        // 8 bit registers


static uint16_t g_suppoted_mipi_lanenum_type = SUPPORT_MIPI_ONE_LANE|SUPPORT_MIPI_TWO_LANE;
#define DEFAULT_NUM_LANES SUPPORT_MIPI_ONE_LANE


#define OV8825_ISIILLUPROFILES_DEFAULT  6U
static IsiIlluProfile_t OV8825_IlluProfileDefault[OV8825_ISIILLUPROFILES_DEFAULT] =
{
    {
        .p_next             = NULL,

        .name               = "A",
        .id                 = ISI_CIEPROF_A,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV8825_XTalkCoeff_CIE_A,
        .pCrossTalkOffset   = &OV8825_XTalkOffset_CIE_A,

        .pGaussMeanValue    = &OV8825_GaussMeanValue_CIE_A,
        .pCovarianceMatrix  = &OV8825_CovarianceMatrix_CIE_A,
        .pGaussFactor       = &OV8825_GaussFactor_CIE_A,
        .pThreshold         = &OV8825_Threshold_CIE_A,
        .pComponentGain     = &OV8825_CompGain_CIE_A,

        .pSaturationCurve   = &OV8825_SaturationCurve_CIE_A,
        .pCcMatrixTable     = &OV8825_CcMatrixTable_CIE_A,
        .pCcOffsetTable     = &OV8825_CcOffsetTable_CIE_A,

        .pVignettingCurve   = &OV8825_VignettingCurve_CIE_A,
    },
    {
        .p_next             = NULL,

        .name               = "F2",
        .id                 = ISI_CIEPROF_F2,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV8825_XTalkCoeff_F2,
        .pCrossTalkOffset   = &OV8825_XTalkOffset_F2,

        .pGaussMeanValue    = &OV8825_GaussMeanValue_F2,
        .pCovarianceMatrix  = &OV8825_CovarianceMatrix_F2,
        .pGaussFactor       = &OV8825_GaussFactor_F2,
        .pThreshold         = &OV8825_Threshold_F2,
        .pComponentGain     = &OV8825_CompGain_F2,

        .pSaturationCurve   = &OV8825_SaturationCurve_F2,
        .pCcMatrixTable     = &OV8825_CcMatrixTable_F2,
        .pCcOffsetTable     = &OV8825_CcOffsetTable_F2,

        .pVignettingCurve   = &OV8825_VignettingCurve_F2,
    },
    {
        .p_next             = NULL,

        .name               = "D50",
        .id                 = ISI_CIEPROF_D50,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,             /* from lightbox */
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_TRUE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV8825_XTalkCoeff_D50,
        .pCrossTalkOffset   = &OV8825_XTalkOffset_D50,

        .pGaussMeanValue    = &OV8825_GaussMeanValue_D50,
        .pCovarianceMatrix  = &OV8825_CovarianceMatrix_D50,
        .pGaussFactor       = &OV8825_GaussFactor_D50,
        .pThreshold         = &OV8825_Threshold_D50,
        .pComponentGain     = &OV8825_CompGain_D50,

        .pSaturationCurve   = &OV8825_SaturationCurve_D50,
        .pCcMatrixTable     = &OV8825_CcMatrixTable_D50,
        .pCcOffsetTable     = &OV8825_CcOffsetTable_D50,

        .pVignettingCurve   = &OV8825_VignettingCurve_D50,
    },
    {
        .p_next             = NULL,

        .name               = "D65",
        .id                 = ISI_CIEPROF_D65,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV8825_XTalkCoeff_D65,
        .pCrossTalkOffset   = &OV8825_XTalkOffset_D65,

        .pGaussMeanValue    = &OV8825_GaussMeanValue_D65,
        .pCovarianceMatrix  = &OV8825_CovarianceMatrix_D65,
        .pGaussFactor       = &OV8825_GaussFactor_D65,
        .pThreshold         = &OV8825_Threshold_D65,
        .pComponentGain     = &OV8825_CompGain_D65,

        .pSaturationCurve   = &OV8825_SaturationCurve_D65,
        .pCcMatrixTable     = &OV8825_CcMatrixTable_D65,
        .pCcOffsetTable     = &OV8825_CcOffsetTable_D65,

        .pVignettingCurve   = &OV8825_VignettingCurve_D65,
    },
    {
        .p_next             = NULL,

        .name               = "D75",
        .id                 = ISI_CIEPROF_D75,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV8825_XTalkCoeff_D75,
        .pCrossTalkOffset   = &OV8825_XTalkOffset_D75,

        .pGaussMeanValue    = &OV8825_GaussMeanValue_D75,
        .pCovarianceMatrix  = &OV8825_CovarianceMatrix_D75,
        .pGaussFactor       = &OV8825_GaussFactor_D75,
        .pThreshold         = &OV8825_Threshold_D75,
        .pComponentGain     = &OV8825_CompGain_D75,

        .pSaturationCurve   = &OV8825_SaturationCurve_D75,
        .pCcMatrixTable     = &OV8825_CcMatrixTable_D75,
        .pCcOffsetTable     = &OV8825_CcOffsetTable_D75,

        .pVignettingCurve   = &OV8825_VignettingCurve_D75,
    },
    {
        .p_next             = NULL,

        .name               = "F11",
        .id                 = ISI_CIEPROF_F11,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV8825_XTalkCoeff_F11,
        .pCrossTalkOffset   = &OV8825_XTalkOffset_F11,

        .pGaussMeanValue    = &OV8825_GaussMeanValue_F11,
        .pCovarianceMatrix  = &OV8825_CovarianceMatrix_F11,
        .pGaussFactor       = &OV8825_GaussFactor_F11,
        .pThreshold         = &OV8825_Threshold_F11,
        .pComponentGain     = &OV8825_CompGain_F11,

        .pSaturationCurve   = &OV8825_SaturationCurve_F11,
        .pCcMatrixTable     = &OV8825_CcMatrixTable_F11,
        .pCcOffsetTable     = &OV8825_CcOffsetTable_F11,

        .pVignettingCurve   = &OV8825_VignettingCurve_F11,
    }
};




/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static RESULT OV8825_IsiCreateSensorIss( IsiSensorInstanceConfig_t *pConfig );
static RESULT OV8825_IsiReleaseSensorIss( IsiSensorHandle_t handle );
static RESULT OV8825_IsiGetCapsIss( IsiSensorHandle_t handle, IsiSensorCaps_t *pIsiSensorCaps );
static RESULT OV8825_IsiSetupSensorIss( IsiSensorHandle_t handle, const IsiSensorConfig_t *pConfig );
static RESULT OV8825_IsiSensorSetStreamingIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV8825_IsiSensorSetPowerIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV8825_IsiCheckSensorConnectionIss( IsiSensorHandle_t handle );
static RESULT OV8825_IsiGetSensorRevisionIss( IsiSensorHandle_t handle, uint32_t *p_value);

static RESULT OV8825_IsiGetGainLimitsIss( IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain);
static RESULT OV8825_IsiGetIntegrationTimeLimitsIss( IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime );
static RESULT OV8825_IsiExposureControlIss( IsiSensorHandle_t handle, float NewGain, float NewIntegrationTime, uint8_t *pNumberOfFramesToSkip, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV8825_IsiGetCurrentExposureIss( IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV8825_IsiGetAfpsInfoIss ( IsiSensorHandle_t handle, uint32_t Resolution, IsiAfpsInfo_t* pAfpsInfo);
static RESULT OV8825_IsiGetGainIss( IsiSensorHandle_t handle, float *pSetGain );
static RESULT OV8825_IsiGetGainIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV8825_IsiSetGainIss( IsiSensorHandle_t handle, float NewGain, float *pSetGain );
static RESULT OV8825_IsiGetIntegrationTimeIss( IsiSensorHandle_t handle, float *pSetIntegrationTime );
static RESULT OV8825_IsiGetIntegrationTimeIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV8825_IsiSetIntegrationTimeIss( IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t *pNumberOfFramesToSkip );
static RESULT OV8825_IsiGetResolutionIss( IsiSensorHandle_t handle, uint32_t *pSetResolution );


static RESULT OV8825_IsiRegReadIss( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );
static RESULT OV8825_IsiRegWriteIss( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

static RESULT OV8825_IsiGetCalibKFactor( IsiSensorHandle_t handle, Isi1x1FloatMatrix_t **pIsiKFactor );
static RESULT OV8825_IsiGetCalibPcaMatrix( IsiSensorHandle_t   handle, Isi3x2FloatMatrix_t **pIsiPcaMatrix );
static RESULT OV8825_IsiGetCalibSvdMeanValue( IsiSensorHandle_t   handle, Isi3x1FloatMatrix_t **pIsiSvdMeanValue );
static RESULT OV8825_IsiGetCalibCenterLine( IsiSensorHandle_t   handle, IsiLine_t  **ptIsiCenterLine);
static RESULT OV8825_IsiGetCalibClipParam( IsiSensorHandle_t   handle, IsiAwbClipParm_t    **pIsiClipParam );
static RESULT OV8825_IsiGetCalibGlobalFadeParam( IsiSensorHandle_t       handle, IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam);
static RESULT OV8825_IsiGetCalibFadeParam( IsiSensorHandle_t   handle, IsiAwbFade2Parm_t   **ptIsiFadeParam);
static RESULT OV8825_IsiGetIlluProfile( IsiSensorHandle_t   handle, const uint32_t CieProfile, IsiIlluProfile_t **ptIsiIlluProfile );

static RESULT OV8825_IsiMdiInitMotoDriveMds( IsiSensorHandle_t handle );
static RESULT OV8825_IsiMdiSetupMotoDrive( IsiSensorHandle_t handle, uint32_t *pMaxStep );
static RESULT OV8825_IsiMdiFocusSet( IsiSensorHandle_t handle, const uint32_t Position );
static RESULT OV8825_IsiMdiFocusGet( IsiSensorHandle_t handle, uint32_t *pAbsStep );
static RESULT OV8825_IsiMdiFocusCalibrate( IsiSensorHandle_t handle );

static RESULT OV8825_IsiGetSensorMipiInfoIss( IsiSensorHandle_t handle, IsiSensorMipiInfo *ptIsiSensorMipiInfo);
static RESULT OV8825_IsiGetSensorIsiVersion(  IsiSensorHandle_t   handle, unsigned int* pVersion);
static RESULT OV8825_IsiGetSensorTuningXmlVersion(  IsiSensorHandle_t   handle, char** pTuningXmlVersion);


static float dctfloor( const float f )
{
    if ( f < 0 )
    {
        return ( (float)((int32_t)f - 1L) );
    }
    else
    {
        return ( (float)((uint32_t)f) );
    }
}



/*****************************************************************************/
/**
 *          OV8825_IsiCreateSensorIss
 *
 * @brief   This function creates a new OV8825 sensor instance handle.
 *
 * @param   pConfig     configuration structure to create the instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
static RESULT OV8825_IsiCreateSensorIss
(
    IsiSensorInstanceConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;
    int32_t current_distance;
    OV8825_Context_t *pOV8825Ctx;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pConfig == NULL) || (pConfig->pSensor ==NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    pOV8825Ctx = ( OV8825_Context_t * )malloc ( sizeof (OV8825_Context_t) );
    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pOV8825Ctx, 0, sizeof( OV8825_Context_t ) );

    result = HalAddRef( pConfig->HalHandle );
    if ( result != RET_SUCCESS )
    {
        free ( pOV8825Ctx );
        return ( result );
    }

    pOV8825Ctx->IsiCtx.HalHandle              = pConfig->HalHandle;
    pOV8825Ctx->IsiCtx.HalDevID               = pConfig->HalDevID;
    pOV8825Ctx->IsiCtx.I2cBusNum              = pConfig->I2cBusNum;
    pOV8825Ctx->IsiCtx.SlaveAddress           = ( pConfig->SlaveAddr == 0 ) ? OV8825_SLAVE_ADDR : pConfig->SlaveAddr;
    pOV8825Ctx->IsiCtx.NrOfAddressBytes       = 2U;

    pOV8825Ctx->IsiCtx.I2cAfBusNum            = pConfig->I2cAfBusNum;
    pOV8825Ctx->IsiCtx.SlaveAfAddress         = ( pConfig->SlaveAfAddr == 0 ) ? OV8825_SLAVE_AF_ADDR : pConfig->SlaveAfAddr;
    pOV8825Ctx->IsiCtx.NrOfAfAddressBytes     = 2U;

    pOV8825Ctx->IsiCtx.pSensor                = pConfig->pSensor;

    pOV8825Ctx->Configured             = BOOL_FALSE;
    pOV8825Ctx->Streaming              = BOOL_FALSE;
    pOV8825Ctx->TestPattern            = BOOL_FALSE;
    pOV8825Ctx->isAfpsRun              = BOOL_FALSE;
    /* ddl@rock-chips.com: v0.3.0 */
    current_distance = pConfig->VcmRatedCurrent - pConfig->VcmStartCurrent;
    current_distance = current_distance*MAX_VCMDRV_REG/MAX_VCMDRV_CURRENT;    
    pOV8825Ctx->VcmInfo.Step = (current_distance+(MAX_LOG-1))/MAX_LOG;
    pOV8825Ctx->VcmInfo.StartCurrent   = pConfig->VcmStartCurrent*MAX_VCMDRV_REG/MAX_VCMDRV_CURRENT;    
    pOV8825Ctx->VcmInfo.RatedCurrent   = pOV8825Ctx->VcmInfo.StartCurrent + MAX_LOG*pOV8825Ctx->VcmInfo.Step;
    pOV8825Ctx->VcmInfo.StepMode       = pConfig->VcmStepMode;    

    pOV8825Ctx->IsiSensorMipiInfo.sensorHalDevID = pOV8825Ctx->IsiCtx.HalDevID;
    if(pConfig->mipiLaneNum & g_suppoted_mipi_lanenum_type)
        pOV8825Ctx->IsiSensorMipiInfo.ucMipiLanes = pConfig->mipiLaneNum;
    else{
        TRACE( OV8825_ERROR, "%s don't support lane numbers :%d,set to default %d\n", __FUNCTION__,pConfig->mipiLaneNum,DEFAULT_NUM_LANES);
        pOV8825Ctx->IsiSensorMipiInfo.ucMipiLanes = DEFAULT_NUM_LANES;
    }

    pConfig->hSensor = ( IsiSensorHandle_t )pOV8825Ctx;

    result = HalSetCamConfig( pOV8825Ctx->IsiCtx.HalHandle, pOV8825Ctx->IsiCtx.HalDevID, false, true, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    result = HalSetClock( pOV8825Ctx->IsiCtx.HalHandle, pOV8825Ctx->IsiCtx.HalDevID, 24000000U);
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiReleaseSensorIss
 *
 * @brief   This function destroys/releases an OV8825 sensor instance.
 *
 * @param   handle      OV8825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
static RESULT OV8825_IsiReleaseSensorIss
(
    IsiSensorHandle_t handle
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    (void)OV8825_IsiSensorSetStreamingIss( pOV8825Ctx, BOOL_FALSE );
    (void)OV8825_IsiSensorSetPowerIss( pOV8825Ctx, BOOL_FALSE );

    (void)HalDelRef( pOV8825Ctx->IsiCtx.HalHandle );

    MEMSET( pOV8825Ctx, 0, sizeof( OV8825_Context_t ) );
    free ( pOV8825Ctx );

    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetCapsIss
 *
 * @brief   fills in the correct pointers for the sensor description struct
 *
 * @param   param1      pointer to sensor capabilities structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetCapsIssInternal
(
    IsiSensorCaps_t   *pIsiSensorCaps,
    uint32_t mipi_lanes
    
)
{
    RESULT result = RET_SUCCESS;


    if ( pIsiSensorCaps == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        if(mipi_lanes == SUPPORT_MIPI_FOUR_LANE){            
            switch (pIsiSensorCaps->Index) 
            {
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }

            }
        } else if(mipi_lanes == SUPPORT_MIPI_TWO_LANE) {
            switch (pIsiSensorCaps->Index) 
            {
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P15;
                    break;
                }
                case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_TV1080P30;
                    break;
                }
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }
            }
        }  else if(mipi_lanes == SUPPORT_MIPI_ONE_LANE) {
            switch (pIsiSensorCaps->Index) 
            {
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P7;
                    break;
                }
                case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_TV1080P15;
                    break;
                }
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }

            }
        } 
    
        pIsiSensorCaps->BusWidth        = ISI_BUSWIDTH_10BIT;
        pIsiSensorCaps->Mode            = ISI_MODE_MIPI;
        pIsiSensorCaps->FieldSelection  = ISI_FIELDSEL_BOTH;
        pIsiSensorCaps->YCSequence      = ISI_YCSEQ_YCBYCR;           /**< only Bayer supported, will not be evaluated */
        pIsiSensorCaps->Conv422         = ISI_CONV422_NOCOSITED;
        pIsiSensorCaps->BPat            = ISI_BPAT_BGBGGRGR;
        pIsiSensorCaps->HPol            = ISI_HPOL_REFPOS;
        pIsiSensorCaps->VPol            = ISI_VPOL_NEG;
        pIsiSensorCaps->Edge            = ISI_EDGE_FALLING;
        pIsiSensorCaps->Bls             = ISI_BLS_OFF;
        pIsiSensorCaps->Gamma           = ISI_GAMMA_OFF;
        pIsiSensorCaps->CConv           = ISI_CCONV_OFF;
        pIsiSensorCaps->BLC             = ( ISI_BLC_AUTO | ISI_BLC_OFF);
        pIsiSensorCaps->AGC             = ( ISI_AGC_OFF );
        pIsiSensorCaps->AWB             = ( ISI_AWB_OFF );
        pIsiSensorCaps->AEC             = ( ISI_AEC_OFF );
        pIsiSensorCaps->DPCC            = ( ISI_DPCC_AUTO | ISI_DPCC_OFF );

        pIsiSensorCaps->DwnSz           = ISI_DWNSZ_SUBSMPL;
        pIsiSensorCaps->CieProfile      = ( ISI_CIEPROF_A
                                          | ISI_CIEPROF_D50
                                          | ISI_CIEPROF_D65
                                          | ISI_CIEPROF_D75
                                          | ISI_CIEPROF_F2
                                          | ISI_CIEPROF_F11 );
        pIsiSensorCaps->SmiaMode        = ISI_SMIA_OFF;
        pIsiSensorCaps->MipiMode        = ISI_MIPI_MODE_RAW_10;
        pIsiSensorCaps->AfpsResolutions = ( ISI_AFPS_NOTSUPP );
		pIsiSensorCaps->SensorOutputMode = ISI_SENSOR_OUTPUT_MODE_RAW;
    }

end:
    return ( result );
}



 
static RESULT OV8825_IsiGetCapsIss
(
    IsiSensorHandle_t handle,
    IsiSensorCaps_t   *pIsiSensorCaps
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s (enter) index: %d\n", __FUNCTION__, pIsiSensorCaps->Index);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    result = OV8825_IsiGetCapsIssInternal(pIsiSensorCaps, pOV8825Ctx->IsiSensorMipiInfo.ucMipiLanes);
    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);
end:
    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_g_IsiSensorDefaultConfig
 *
 * @brief   recommended default configuration for application use via call
 *          to IsiGetSensorIss()
 *
 *****************************************************************************/
const IsiSensorCaps_t OV8825_g_IsiSensorDefaultConfig =
{
    ISI_BUSWIDTH_10BIT,         // BusWidth
    ISI_MODE_MIPI,              // MIPI
    ISI_FIELDSEL_BOTH,          // FieldSel
    ISI_YCSEQ_YCBYCR,           // YCSeq
    ISI_CONV422_NOCOSITED,      // Conv422
    ISI_BPAT_BGBGGRGR,          // BPat
    ISI_HPOL_REFPOS,            // HPol
    ISI_VPOL_NEG,               // VPol
    ISI_EDGE_RISING,            // Edge
    ISI_BLS_OFF,                // Bls
    ISI_GAMMA_OFF,              // Gamma
    ISI_CCONV_OFF,              // CConv
    ISI_RES_TV1080P15,          // Res
    ISI_DWNSZ_SUBSMPL,          // DwnSz
    ISI_BLC_AUTO,               // BLC
    ISI_AGC_OFF,                // AGC
    ISI_AWB_OFF,                // AWB
    ISI_AEC_OFF,                // AEC
    ISI_DPCC_OFF,               // DPCC
    ISI_CIEPROF_F11,            // CieProfile, this is also used as start profile for AWB (if not altered by menu settings)
    ISI_SMIA_OFF,               // SmiaMode
    ISI_MIPI_MODE_RAW_10,       // MipiMode
    ISI_AFPS_NOTSUPP,           // AfpsResolutions
    ISI_SENSOR_OUTPUT_MODE_RAW,
    0,
};



/*****************************************************************************/
/**
 *          OV8825_SetupOutputFormat
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV8825 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_SetupOutputFormat
(
    OV8825_Context_t       *pOV8825Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s%s (enter)\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );

    /* bus-width */
    switch ( pConfig->BusWidth )        /* only ISI_BUSWIDTH_12BIT supported, no configuration needed here */
    {
        case ISI_BUSWIDTH_10BIT:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: bus width not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* mode */
    switch ( pConfig->Mode )            /* only ISI_MODE_BAYER supported, no configuration needed here */
    {
        case( ISI_MODE_MIPI ):
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: mode not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* field-selection */
    switch ( pConfig->FieldSelection )  /* only ISI_FIELDSEL_BOTH supported, no configuration needed */
    {
        case ISI_FIELDSEL_BOTH:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: field selection not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* only Bayer mode is supported by OV8825 sensor, so the YCSequence parameter is not checked */
    switch ( pConfig->YCSequence )
    {
        default:
        {
            break;
        }
    }

    /* 422 conversion */
    switch ( pConfig->Conv422 )         /* only ISI_CONV422_NOCOSITED supported, no configuration needed */
    {
        case ISI_CONV422_NOCOSITED:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: 422 conversion not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* bayer-pattern */
    switch ( pConfig->BPat )            /* only ISI_BPAT_BGBGGRGR supported, no configuration needed */
    {
        case ISI_BPAT_BGBGGRGR:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: bayer pattern not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* horizontal polarity */
    switch ( pConfig->HPol )            /* only ISI_HPOL_REFPOS supported, no configuration needed */
    {
        case ISI_HPOL_REFPOS:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: HPol not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* vertical polarity */
    switch ( pConfig->VPol )            /* only ISI_VPOL_NEG supported, no configuration needed */
    {
        case ISI_VPOL_NEG:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: VPol not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }


    /* edge */
    switch ( pConfig->Edge )            /* only ISI_EDGE_RISING supported, no configuration needed */
    {
        case ISI_EDGE_RISING:
        {
            break;
        }

        case ISI_EDGE_FALLING:          /*TODO for MIPI debug*/
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s:  edge mode not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* gamma */
    switch ( pConfig->Gamma )           /* only ISI_GAMMA_OFF supported, no configuration needed */
    {
        case ISI_GAMMA_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s:  gamma not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* color conversion */
    switch ( pConfig->CConv )           /* only ISI_CCONV_OFF supported, no configuration needed */
    {
        case ISI_CCONV_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: color conversion not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->SmiaMode )        /* only ISI_SMIA_OFF supported, no configuration needed */
    {
        case ISI_SMIA_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: SMIA mode not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->MipiMode )        /* only ISI_MIPI_MODE_RAW_12 supported, no configuration needed */
    {
        case ISI_MIPI_MODE_RAW_10:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s%s: MIPI mode not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->AfpsResolutions ) /* no configuration needed */
    {
        case ISI_AFPS_NOTSUPP:
        {
            break;
        }
        default:
        {
            // don't care about what comes in here
            //TRACE( OV8825_ERROR, "%s%s: AFPS not supported\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"" );
            //return ( RET_NOTSUPP );
        }
    }

    TRACE( OV8825_INFO, "%s%s (exit)\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}

int OV8825_get_PCLK( OV8825_Context_t *pOV8825Ctx, int XVCLK)
{
    // calculate PCLK
    uint32_t PCLK, temp1, temp2;
    int Div_cnt5b, Pre_Div_sp2x, R_div_sp, Div124_sp, div12_sp, Sdiv_sp, pll2;
    int Pre_div02x, Div124, Div_cnt7b, Sdiv0, Sdiv1, R_seld52x, VCO, pllsysclk;
    int Pre_Div_sp2x_map[] = {2, 3, 4, 6};
    int Pre_div02x_map[] = {2, 3, 4, 5, 6, 8, 12, 16};
    int Div124_map[] = {1, 1, 2, 4};
    int R_seld52x_map[] = {2, 3, 4, 5, 6, 7, 8, 10};

    
    //temp1 = ReadSCCB(0x6c, 0x3007);
    OV8825_IsiRegReadIss(  pOV8825Ctx, 0x3007, &temp1 );
    Div_cnt5b = temp1>>3;
    temp2 = temp1 & 0x03;
    Pre_Div_sp2x = Pre_Div_sp2x_map[temp2];
    if(temp1 & 0x04) {
        R_div_sp= 2;
    }
    else{
        R_div_sp = 1;
    }

    OV8825_IsiRegReadIss(  pOV8825Ctx, 0x3012, &temp1 );
    Div124_sp = temp1 >> 6;
    div12_sp = (temp1>>4) & 0x03;
    Sdiv_sp = temp1 & 0x0f;
    pll2 = XVCLK * 2 / Pre_Div_sp2x * R_div_sp * (32 - Div_cnt5b ) / (Sdiv_sp + 1);
    
    //temp1 = ReadSCCB(0x6c, 0x3003);
    OV8825_IsiRegReadIss(  pOV8825Ctx, 0x3003, &temp1 );
    temp2 = temp1 & 0x07;
    Pre_div02x = Pre_div02x_map[temp2];
    temp2 = temp1>>6;
    Div124 = Div124_map[temp2];
    
    //temp1 = ReadSCCB(0x6c, 0x3005);
    OV8825_IsiRegReadIss(  pOV8825Ctx, 0x3005, &temp1 );
    Sdiv0 = temp1 & 0x0f;
    
    //temp1 = ReadSCCB(0x6c, 0x3006);
    OV8825_IsiRegReadIss(  pOV8825Ctx, 0x3006, &temp1 );
    Sdiv1 = temp1 & 0x0f;
    temp2 = (temp1>>4) & 0x07;
    R_seld52x = R_seld52x_map[temp2];
    
    //temp1 = ReadSCCB(0x6c, 0x3004);
    OV8825_IsiRegReadIss(  pOV8825Ctx, 0x3004, &temp1 );
    Div_cnt7b = temp1 & 0x7f;
    VCO = XVCLK * 2 / Pre_div02x * Div124 * (129 - Div_cnt7b);
    if(temp1 & 0x80) {
        pllsysclk = pll2 / (Sdiv1 + 1) * 2 / R_seld52x;
    }
    else{
        pllsysclk = VCO / (Sdiv0 +1) / (Sdiv1 + 1) * 2 / R_seld52x;
    }
    
    //temp1 = ReadSCCB(0x6c, 0x3104);
    OV8825_IsiRegReadIss(  pOV8825Ctx, 0x3104, &temp1 );
    if(temp1 & 0x20) {
        PCLK = pllsysclk;
    }
    else{
        PCLK = pll2;
    }
    
    return PCLK*10000;
}

/*****************************************************************************/
/**
 *          OV8825_SetupOutputWindow
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV8825 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_SetupOutputWindow
(
    OV8825_Context_t        *pOV8825Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result     = RET_SUCCESS;
    uint16_t usFrameLengthLines = 0;
    uint16_t usLineLengthPck    = 0;
    float    rVtPixClkFreq      = 0.0f;
    int xclk = 2400;

    uint8_t usModeSelect = 0;
    uint8_t usPllCtrl1 = 0;
    uint8_t usReg0x3005 = 0;
    uint8_t usPllCtrl3 = 0;
    uint8_t usScClkSel = 0;
    uint8_t usAECExpoM = 0;
    uint8_t usAECExpoL = 0;
    uint8_t usSenCtrl0 = 0;
    uint8_t usSenCtrl2 = 0;
    uint8_t usSenCtrl3 = 0;
    uint8_t usSenCtrl4 = 0;
    uint8_t usSenCtrl5 = 0;
    uint8_t usSenCtrl6 = 0;
    uint8_t usSenCtrl8 = 0;
    uint8_t usSenCtrl9 = 0;
    uint8_t usSenCtrlA = 0;
    uint8_t usSenCtrl11 = 0;
    uint8_t usSenCtrl12 = 0;
    uint8_t us3724 = 0;
    uint8_t us3725 = 0;
    uint8_t us3726 = 0;
    uint8_t us3727 = 0;
    uint16_t usTimeVs = 0;
    uint16_t usTimeVh = 0;
    uint16_t usTimeIspHo = 0;
    uint16_t usTimeIspVo = 0;
    uint16_t usTimeHts = 0;
    uint16_t usTimeVts = 0;
    uint8_t usTimeHoffsL = 0;
    uint8_t usTimeVoffsL = 0;
    uint8_t usTimeXinc = 0;
    uint8_t usTimeYinc = 0;
    uint8_t usTimeReg20 = 0;
    uint8_t usTimeReg21 = 0;
    uint8_t usPsramCtrl = 0;
    uint8_t usBLSctr5 = 0;
    uint16_t usVFIFOReadST= 0;
    uint8_t usMIPIPclk = 0;
    uint8_t usHscalCtrl = 0;
    uint8_t usVscalCtrl = 0;
   // uint8_t usModeSelect = 0;
    uint8_t usSCClkRst2 = 0;
    uint8_t usSCClkRst0 = 0;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);


	if(pOV8825Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_TWO_LANE){

    	pOV8825Ctx->IsiSensorMipiInfo.ulMipiFreq = 328;
        /* resolution */
        switch ( pConfig->Resolution )
        {
            case ISI_RES_TV1080P30:
            {
                TRACE( OV8825_NOTICE1, "%s(%d): ISI_RES_TV1080P30", __FUNCTION__,__LINE__ );
                usModeSelect = 0x00;
                usPllCtrl1 = 0xd8;
                usPllCtrl3 = 0x00;
                usScClkSel = 0x01;
                usAECExpoM = 0x74;
                usAECExpoL = 0x60;
                usSenCtrl0 = 0x20;
                usSenCtrl2 = 0x50;
                usSenCtrl3 = 0xcc;
                usSenCtrl4 = 0x19;
                usSenCtrl5 = 0x32;
                usSenCtrl6 = 0x4b;
                usSenCtrl8 = 0x84;
                usSenCtrl9 = 0x40;
                usSenCtrlA = 0x31;
                usSenCtrl11 = 0x0f;
                usSenCtrl12 = 0x9c;
                us3724 = 0x01;
                us3725 = 0x92;
                us3726 = 0x01;
                us3727 = 0xc7;
                usTimeVs = 0x0130;
                usTimeVh = 0x0867;
                usTimeIspHo = 0x0780;
                usTimeIspVo = 0x0438;
                usTimeHts = 0x0df0;
                usTimeVts = 0x074c;
                usTimeHoffsL = 0x10;
                usTimeVoffsL = 0x06;
                usTimeXinc = 0x11;
                usTimeYinc = 0x11;
                usTimeReg20 = 0x80;
                usTimeReg21 = 0x16;
                usPsramCtrl = 0x02;
                usBLSctr5 = 0x18;
                usVFIFOReadST= 0x0100;
                usMIPIPclk = 0x1e;
                usHscalCtrl = 0x53;
                usVscalCtrl = 0x53;
                usModeSelect = 0x01;
                usSCClkRst2 = 0xf0;
                usSCClkRst0 = 0x70;

                //rVtPixClkFreq = OV8825_get_PCLK(pOV8825Ctx, xclk);
                usLineLengthPck = usTimeHts;
                usFrameLengthLines = usTimeVts;
                break;
                
            }
            
            case ISI_RES_3264_2448P15:
            {
                TRACE( OV8825_NOTICE1, "%s(%d): ISI_RES_3264_2448", __FUNCTION__,__LINE__ );
                usModeSelect = 0x00;
                usPllCtrl1 = 0xd8;
                usPllCtrl3 = 0x10;
                usScClkSel = 0x81;
                usAECExpoM = 0x9a;
                usAECExpoL = 0xa0;
                usSenCtrl0 = 0x10;
                usSenCtrl2 = 0x28;
                usSenCtrl3 = 0x6c;
                usSenCtrl4 = 0x40;
                usSenCtrl5 = 0x19;
                usSenCtrl6 = 0x27;
                usSenCtrl8 = 0x48;
                usSenCtrl9 = 0x20;
                usSenCtrlA = 0x31;
                usSenCtrl11 = 0x07;
                usSenCtrl12 = 0x4e;
                us3724 = 0x00;
                us3725 = 0xd4;
                us3726 = 0x00;
                us3727 = 0xf0;
                usTimeVs = 0x0000;
                usTimeVh = 0x099b;
                usTimeIspHo = 0x0cc0;
                usTimeIspVo = 0x0990;
                usTimeHts = 0x0e00;
                usTimeVts = 0x09b0;
                usTimeHoffsL = 0x10;
                usTimeVoffsL = 0x06;
                usTimeXinc = 0x11;
                usTimeYinc = 0x11;
                usTimeReg20 = 0x80;
                usTimeReg21 = 0x16;
                usPsramCtrl = 0x02;
                usBLSctr5 = 0x1a;
                usVFIFOReadST= 0x0020;
                usMIPIPclk = 0x1e;
                usHscalCtrl = 0x00;
                usVscalCtrl = 0x00;
                usModeSelect = 0x01;
                usSCClkRst2 = 0xf0;
                usSCClkRst0 = 0x70;

                //rVtPixClkFreq = OV8825_get_PCLK(pOV8825Ctx, xclk);
                usLineLengthPck = usTimeHts;
                usFrameLengthLines = usTimeVts;
                break;
                
            }

            default:
            {
                TRACE( OV8825_ERROR, "%s: Resolution(0x%x) not supported\n", __FUNCTION__,pConfig->Resolution );
                return ( RET_NOTSUPP );
            }
        }

        //TRACE( OV8825_INFO, "%s: Resolution %dx%d\n", __FUNCTION__, usHSize, usVSize );

        #if 0  /* ddl@rock-chips.com: v0.1.2 */
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_MODE_SELECT, 0x00);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        #endif
        
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PLL_CTRL1, usPllCtrl1);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PLL_CTRL3, usPllCtrl3);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SC_CLK_SEL, usScClkSel);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_EXPO_M, usAECExpoM);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_EXPO_L, usAECExpoL);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL0, usSenCtrl0);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL2, usSenCtrl2);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL3, usSenCtrl3);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL4, usSenCtrl4);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL5, usSenCtrl5);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL6, usSenCtrl6);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL8, usSenCtrl8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL9, usSenCtrl9);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRLA, usSenCtrlA);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL11, usSenCtrl11);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL12, usSenCtrl12);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_3724, us3724);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_3725, us3725);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_3726, us3726);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_3727, us3727);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VS_H, (usTimeVs&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VS_L, (usTimeVs&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VH_H, (usTimeVh&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VH_L, (usTimeVh&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_ISPHO_H, (usTimeIspHo&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_ISPHO_L, (usTimeIspHo&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_ISPVO_H, (usTimeIspVo&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_ISPVO_L, (usTimeIspVo&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_HTS_H, (usTimeHts&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_HTS_L, (usTimeHts&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VTS_H, (usTimeVts&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VTS_L, (usTimeVts&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_HOFFS_LOW, usTimeHoffsL);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VOFFS_LOW, usTimeVoffsL);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_X_INC, usTimeXinc);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_Y_INC, usTimeYinc);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_REG20, usTimeReg20);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_REG21, usTimeReg21);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PSRAM_CTRL0 , usPsramCtrl);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_VFIFO_READ_ST_HIGH, (usVFIFOReadST&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_VFIFO_READ_ST_LOW, (usVFIFOReadST&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_MIPI_PCLK_PERIOD, usMIPIPclk);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_HSCALE_CTRL, usHscalCtrl);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_VSCALE_CTRL, usVscalCtrl);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
       // result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_MODE_SELECT, usModeSelect);
       //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SC_CLKRST2 , usSCClkRst2);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SC_CLKRST0, usSCClkRst0);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        rVtPixClkFreq = OV8825_get_PCLK(pOV8825Ctx, xclk);
        
        // store frame timing for later use in AEC module
        pOV8825Ctx->VtPixClkFreq     = rVtPixClkFreq;
        pOV8825Ctx->LineLengthPck    = usLineLengthPck;
        pOV8825Ctx->FrameLengthLines = usFrameLengthLines;
    }else if(pOV8825Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_ONE_LANE){

    	pOV8825Ctx->IsiSensorMipiInfo.ulMipiFreq = 656;
        /* resolution */
        switch ( pConfig->Resolution )
        {
            case ISI_RES_TV1080P15:
            {
                TRACE( OV8825_NOTICE1, "%s(%d): ISI_RES_TV1080P15", __FUNCTION__,__LINE__ );
                usModeSelect = 0x00;
                usPllCtrl1 = 0xd2; //0xd8;//0xd2;
                usReg0x3005 = 0x00;
                usPllCtrl3 = 0x10;//0x20;//0x10;
                usScClkSel = 0x01;
                usAECExpoM = 0x74;
                usAECExpoL = 0x60;
                usSenCtrl0 = 0x20;
                usSenCtrl2 = 0x50;
                usSenCtrl3 = 0xcc;
                usSenCtrl4 = 0x19;
                usSenCtrl5 = 0x32;
                usSenCtrl6 = 0x4b;
                usSenCtrl8 = 0x84;
                usSenCtrl9 = 0x40;
                usSenCtrlA = 0x31;
                usSenCtrl11 = 0x0f;
                usSenCtrl12 = 0x9c;
                us3724 = 0x01;
                us3725 = 0x92;
                us3726 = 0x01;
                us3727 = 0xc7;
                usTimeVs = 0x0130;
                usTimeVh = 0x867;
                usTimeIspHo = 0x780;
                usTimeIspVo = 0x438;
                usTimeHts = 0xdf0;
                usTimeVts = 0x74c;
                usTimeHoffsL = 0x10;
                usTimeVoffsL = 0x06;
                usTimeXinc = 0x11;
                usTimeYinc = 0x11;
                usTimeReg20 = 0x80;
                usTimeReg21 = 0x16;
                usPsramCtrl = 0x02;
                usBLSctr5 = 0x18;
                usVFIFOReadST= 0x0100;
                usMIPIPclk = 0x15;//0x1e;//0x15;
                usHscalCtrl = 0x53;
                usVscalCtrl = 0x53;
                usModeSelect = 0x01;
                usSCClkRst2 = 0xf0;
                usSCClkRst0 = 0x70;

                //rVtPixClkFreq = OV8825_get_PCLK(pOV8825Ctx, xclk);
                usLineLengthPck = usTimeHts;
                usFrameLengthLines = usTimeVts;
                break;
                
            }
            
            case ISI_RES_3264_2448P7:
            {
                TRACE( OV8825_NOTICE1, "%s(%d): ISI_RES_3264_2448", __FUNCTION__,__LINE__ );
                usModeSelect = 0x00;
                usPllCtrl1 = 0xd8;   //0xce;
                usReg0x3005 = 0x00;
                usPllCtrl3 = 0x40;   //0x70;
                usScClkSel = 0x81;
                usAECExpoM = 0x9a;
                usAECExpoL = 0xa0;
                usSenCtrl0 = 0x10;
                usSenCtrl2 = 0x28;
                usSenCtrl3 = 0x6c;
                usSenCtrl4 = 0x40;
                usSenCtrl5 = 0x19;
                usSenCtrl6 = 0x27;
                usSenCtrl8 = 0x48;
                usSenCtrl9 = 0x20;
                usSenCtrlA = 0x31;
                usSenCtrl11 = 0x07;
                usSenCtrl12 = 0x4e;
                us3724 = 0x00;
                us3725 = 0xd4;
                us3726 = 0x00;
                us3727 = 0xf0;
                usTimeVs = 0x0000;
                usTimeVh = 0x099b;
                usTimeIspHo = 0x0cc0;
                usTimeIspVo = 0x0990;
                usTimeHts = 0x0e00;
                usTimeVts = 0x9b0;//0x0cb0;
                usTimeHoffsL = 0x10;
                usTimeVoffsL = 0x06;
                usTimeXinc = 0x11;
                usTimeYinc = 0x11;
                usTimeReg20 = 0x80;
                usTimeReg21 = 0x16;
                usPsramCtrl = 0x02;
                usBLSctr5 = 0x18;
                usVFIFOReadST= 0x0020;
                usMIPIPclk = 0x1e;
                usHscalCtrl = 0x00;
                usVscalCtrl = 0x00;
                usModeSelect = 0x01;
                usSCClkRst2 = 0xf0;
                usSCClkRst0 = 0x70;

                //rVtPixClkFreq = OV8825_get_PCLK(pOV8825Ctx, xclk);
                usLineLengthPck = usTimeHts;
                usFrameLengthLines = usTimeVts;
                break;
                
            }

            default:
            {
                TRACE( OV8825_ERROR, "%s: Resolution(0x%x) not supported\n", __FUNCTION__,pConfig->Resolution );
                return ( RET_NOTSUPP );
            }
        }

        //TRACE( OV8825_INFO, "%s: Resolution %dx%d\n", __FUNCTION__, usHSize, usVSize );
        #if 0   /* ddl@rock-chips.com: v0.1.2 */
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_MODE_SELECT, 0x00);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        #endif
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PLL_CTRL1, usPllCtrl1);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PLL_CTRL2, usReg0x3005);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PLL_CTRL3, usPllCtrl3);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SC_CLK_SEL, usScClkSel);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_EXPO_M, usAECExpoM);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_EXPO_L, usAECExpoL);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL0, usSenCtrl0);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL2, usSenCtrl2);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL3, usSenCtrl3);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL4, usSenCtrl4);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL5, usSenCtrl5);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL6, usSenCtrl6);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL8, usSenCtrl8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL9, usSenCtrl9);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRLA, usSenCtrlA);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL11, usSenCtrl11);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SENCTRL12, usSenCtrl12);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_3724, us3724);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_3725, us3725);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_3726, us3726);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_3727, us3727);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VS_H, (usTimeVs&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VS_L, (usTimeVs&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VH_H, (usTimeVh&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VH_L, (usTimeVh&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_ISPHO_H, (usTimeIspHo&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_ISPHO_L, (usTimeIspHo&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_ISPVO_H, (usTimeIspVo&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_ISPVO_L, (usTimeIspVo&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_HTS_H, (usTimeHts&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_HTS_L, (usTimeHts&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VTS_H, (usTimeVts&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VTS_L, (usTimeVts&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_HOFFS_LOW, usTimeHoffsL);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_VOFFS_LOW, usTimeVoffsL);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_X_INC, usTimeXinc);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_Y_INC, usTimeYinc);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_REG20, usTimeReg20);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_TIMING_REG21, usTimeReg21);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PSRAM_CTRL0 , usPsramCtrl);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_VFIFO_READ_ST_HIGH, (usVFIFOReadST&0xff00)>>8);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_VFIFO_READ_ST_LOW, (usVFIFOReadST&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_MIPI_PCLK_PERIOD, usMIPIPclk);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_HSCALE_CTRL, usHscalCtrl);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_VSCALE_CTRL, usVscalCtrl);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
       // result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_MODE_SELECT, usModeSelect);
       //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SC_CLKRST2 , usSCClkRst2);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_SC_CLKRST0, usSCClkRst0);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        rVtPixClkFreq = OV8825_get_PCLK(pOV8825Ctx, xclk);
        
        // store frame timing for later use in AEC module
        pOV8825Ctx->VtPixClkFreq     = rVtPixClkFreq;
        pOV8825Ctx->LineLengthPck    = usLineLengthPck;
        pOV8825Ctx->FrameLengthLines = usFrameLengthLines;	

    }

//have to reset mipi freq here,zyc

    TRACE( OV8825_NOTICE1, "%s  Resolution: %dx%d@%dfps(exit)\n", __FUNCTION__, 
        ISI_RES_W_GET(pConfig->Resolution),
        ISI_RES_H_GET(pConfig->Resolution),
        ISI_FPS_GET(pConfig->Resolution));

    return ( result );
}




/*****************************************************************************/
/**
 *          OV8825_SetupImageControl
 *
 * @brief   Sets the image control functions (BLC, AGC, AWB, AEC, DPCC ...)
 *
 * @param   handle      OV8825 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_SetupImageControl
(
    OV8825_Context_t        *pOV8825Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0U;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    switch ( pConfig->Bls )      /* only ISI_BLS_OFF supported, no configuration needed */
    {
        case ISI_BLS_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s: Black level not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* black level compensation */
    switch ( pConfig->BLC )
    {
        case ISI_BLC_OFF:
        {
            /* turn off black level correction (clear bit 0) */
            //result = OV8825_IsiRegReadIss(  pOV8825Ctx, OV8825_BLC_CTRL00, &RegValue );
            //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_BLC_CTRL00, RegValue & 0x7F);
            break;
        }

        case ISI_BLC_AUTO:
        {
            /* turn on black level correction (set bit 0)
             * (0x331E[7] is assumed to be already setup to 'auto' by static configration) */
            //result = OV8825_IsiRegReadIss(  pOV8825Ctx, OV8825_BLC_CTRL00, &RegValue );
            //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_BLC_CTRL00, RegValue | 0x80 );
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s: BLC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* automatic gain control */
    switch ( pConfig->AGC )
    {
        case ISI_AGC_OFF:
        {
            // manual gain (appropriate for AEC with Marvin)
            //result = OV8825_IsiRegReadIss(  pOV8825Ctx, OV8825_AEC_MANUAL, &RegValue );
            //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_MANUAL, RegValue | 0x02 );
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s: AGC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* automatic white balance */
    switch( pConfig->AWB )
    {
        case ISI_AWB_OFF:
        {
            //result = OV8825_IsiRegReadIss(  pOV8825Ctx, OV8825_ISP_CTRL01, &RegValue );
            //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_ISP_CTRL01, RegValue | 0x01 );
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s: AWB not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    switch( pConfig->AEC )
    {
        case ISI_AEC_OFF:
        {
            //result = OV8825_IsiRegReadIss(  pOV8825Ctx, OV8825_AEC_MANUAL, &RegValue );
            //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_MANUAL, RegValue | 0x01 );
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s: AEC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }


    switch( pConfig->DPCC )
    {
        case ISI_DPCC_OFF:
        {
            // disable white and black pixel cancellation (clear bit 6 and 7)
            //result = OV8825_IsiRegReadIss( pOV8825Ctx, OV8825_ISP_CTRL00, &RegValue );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_ISP_CTRL00, (RegValue &0x7c) );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            break;
        }

        case ISI_DPCC_AUTO:
        {
            // enable white and black pixel cancellation (set bit 6 and 7)
            //result = OV8825_IsiRegReadIss( pOV8825Ctx, OV8825_ISP_CTRL00, &RegValue );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            //result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_ISP_CTRL00, (RegValue | 0x83) );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            break;
        }

        default:
        {
            TRACE( OV8825_ERROR, "%s: DPCC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }// I have not update this commented part yet, as I did not find DPCC setting in the current 8810 driver of Trillian board. - SRJ

    return ( result );
}


/*****************************************************************************/
/**
 *          OV8825_AecSetModeParameters
 *
 * @brief   This function fills in the correct parameters in OV8825-Instances
 *          according to AEC mode selection in IsiSensorConfig_t.
 *
 * @note    It is assumed that IsiSetupOutputWindow has been called before
 *          to fill in correct values in instance structure.
 *
 * @param   handle      OV8825 context
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_AecSetModeParameters
(
    OV8825_Context_t       *pOV8825Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s%s (enter)\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"");

    if ( (pOV8825Ctx->VtPixClkFreq == 0.0f) )
    {
        TRACE( OV8825_ERROR, "%s%s: Division by zero!\n", __FUNCTION__  );
        return ( RET_OUTOFRANGE );
    }

    //as of mail from Omnivision FAE the limit is VTS - 6 (above that we observed a frame
    //exposed way too dark from time to time)
    // (formula is usually MaxIntTime = (CoarseMax * LineLength + FineMax) / Clk
    //                     MinIntTime = (CoarseMin * LineLength + FineMin) / Clk )
    pOV8825Ctx->AecMaxIntegrationTime = ( ((float)(pOV8825Ctx->FrameLengthLines - 4)) * ((float)pOV8825Ctx->LineLengthPck) ) / pOV8825Ctx->VtPixClkFreq;
    pOV8825Ctx->AecMinIntegrationTime = 0.0001f;

    TRACE( OV8825_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"", pOV8825Ctx->AecMaxIntegrationTime  );

    pOV8825Ctx->AecMaxGain = OV8825_MAX_GAIN_AEC;
    pOV8825Ctx->AecMinGain = 1.0f; //as of sensor datasheet 32/(32-6)

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    pOV8825Ctx->AecIntegrationTimeIncrement = ((float)pOV8825Ctx->LineLengthPck) / pOV8825Ctx->VtPixClkFreq;
    pOV8825Ctx->AecGainIncrement = OV8825_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOV8825Ctx->AecCurGain               = pOV8825Ctx->AecMinGain;
    pOV8825Ctx->AecCurIntegrationTime    = 0.0f;
    pOV8825Ctx->OldCoarseIntegrationTime = 0;
    pOV8825Ctx->OldFineIntegrationTime   = 0;
    //pOV8825Ctx->GroupHold                = true; //must be true (for unknown reason) to correctly set gain the first time

    TRACE( OV8825_INFO, "%s%s (exit)\n", __FUNCTION__, pOV8825Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}


/*****************************************************************************/
/**
 *          OV8825_IsiSetupSensorIss
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV8825 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiSetupSensorIss
(
    IsiSensorHandle_t       handle,
    const IsiSensorConfig_t *pConfig
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid configuration (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    if ( pOV8825Ctx->Streaming != BOOL_FALSE )
    {
        return RET_WRONG_STATE;
    }

    MEMCPY( &pOV8825Ctx->Config, pConfig, sizeof( IsiSensorConfig_t ) );

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    result = OV8825_IsiRegWriteIss ( pOV8825Ctx, OV8825_SOFTWARE_RST, 0x01U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    osSleep( 10 );

    TRACE( OV8825_DEBUG, "%s: OV8825 System-Reset executed\n", __FUNCTION__);
    // disable streaming during sensor setup
    // (this seems not to be necessary, however Omnivision is doing it in their
    // reference settings, simply overwrite upper bits since setup takes care
    // of 'em later on anyway)
    result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_MODE_SELECT, 0x00 );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8825_ERROR, "%s: Can't write OV8825 Image System Register (disable streaming failed)\n", __FUNCTION__ );
        return ( result );
    }
    
    /* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */
    if(pOV8825Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_ONE_LANE){
        result = IsiRegDefaultsApply( pOV8825Ctx, OV8825_g_aRegDescription_onelane);
        }
	else if(pOV8825Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_TWO_LANE)
        result = IsiRegDefaultsApply( pOV8825Ctx, OV8825_g_aRegDescription_twolane);
    
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* sleep a while, that sensor can take over new default values */
    osSleep( 10 );


    /* 3.) verify default values to make sure everything has been written correctly as expected */
	#if 0
	result = IsiRegDefaultsVerify( pOV8825Ctx, OV8825_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
	#endif
	
    #if 0
    // output of pclk for measurement (only debugging)
    result = OV8825_IsiRegWriteIss( pOV8825Ctx, 0x3009U, 0x10U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    #endif

    /* 4.) setup output format (RAW10|RAW12) */
    result = OV8825_SetupOutputFormat( pOV8825Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8825_ERROR, "%s: SetupOutputFormat failed.\n", __FUNCTION__);
        return ( result );
    }

    /* 5.) setup output window */
    result = OV8825_SetupOutputWindow( pOV8825Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8825_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV8825_SetupImageControl( pOV8825Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8825_ERROR, "%s: SetupImageControl failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV8825_AecSetModeParameters( pOV8825Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8825_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
        return ( result );
    }
    if (result == RET_SUCCESS)
    {
        pOV8825Ctx->Configured = BOOL_TRUE;
    }

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiChangeSensorResolutionIss
 *
 * @brief   Change image sensor resolution while keeping all other static settings.
 *          Dynamic settings like current gain & integration time are kept as
 *          close as possible. Sensor needs 2 frames to engage (first 2 frames
 *          are not correctly exposed!).
 *
 * @note    Re-read current & min/max values as they will probably have changed!
 *
 * @param   handle                  Sensor instance handle
 * @param   Resolution              new resolution ID
 * @param   pNumberOfFramesToSkip   reference to storage for number of frames to skip
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
static RESULT OV8825_IsiChangeSensorResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (pNumberOfFramesToSkip == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pOV8825Ctx->Configured != BOOL_TRUE) || (pOV8825Ctx->Streaming != BOOL_FALSE) )
    {
        return RET_WRONG_STATE;
    }

    IsiSensorCaps_t Caps;    
    Caps.Index = 0;
    Caps.Resolution = 0;
    while (OV8825_IsiGetCapsIss( handle, &Caps) == RET_SUCCESS) {
        if (Resolution == Caps.Resolution) {            
            break;
        }
        Caps.Index++;
    }

    if (Resolution != Caps.Resolution) {
        return RET_OUTOFRANGE;
    }

    if ( Resolution == pOV8825Ctx->Config.Resolution )
    {
        // well, no need to worry
        *pNumberOfFramesToSkip = 0;
    }
    else
    {
        // change resolution
        char *szResName = NULL;
        result = IsiGetResolutionName( Resolution, &szResName );
        TRACE( OV8825_DEBUG, "%s: NewRes=0x%08x (%s)\n", __FUNCTION__, Resolution, szResName);

        // update resolution in copy of config in context
        pOV8825Ctx->Config.Resolution = Resolution;

        // tell sensor about that
        result = OV8825_SetupOutputWindow( pOV8825Ctx, &pOV8825Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV8825_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
            return ( result );
        }

        // remember old exposure values
        float OldGain = pOV8825Ctx->AecCurGain;
        float OldIntegrationTime = pOV8825Ctx->AecCurIntegrationTime;

        // update limits & stuff (reset current & old settings)
        result = OV8825_AecSetModeParameters( pOV8825Ctx, &pOV8825Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV8825_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
            return ( result );
        }

        // restore old exposure values (at least within new exposure values' limits)
        uint8_t NumberOfFramesToSkip;
        float   DummySetGain;
        float   DummySetIntegrationTime;
        result = OV8825_IsiExposureControlIss( handle, OldGain, OldIntegrationTime, &NumberOfFramesToSkip, &DummySetGain, &DummySetIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV8825_ERROR, "%s: OV8825_IsiExposureControlIss failed.\n", __FUNCTION__);
            return ( result );
        }

        // return number of frames that aren't exposed correctly
        *pNumberOfFramesToSkip = NumberOfFramesToSkip + 1;
    }

    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiSensorSetStreamingIss
 *
 * @brief   Enables/disables streaming of sensor data, if possible.
 *
 * @param   handle      Sensor instance handle
 * @param   on          new streaming state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
static RESULT OV8825_IsiSensorSetStreamingIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    uint32_t RegValue = 0;

    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pOV8825Ctx->Configured != BOOL_TRUE) || (pOV8825Ctx->Streaming == on) )
    {
        return RET_WRONG_STATE;
    }

    if (on == BOOL_TRUE)
    {
        /* enable streaming */
        OV8825_IsiRegWriteIss ( pOV8825Ctx, OV8825_BLC_CTRL03, 0x82 );
        OV8825_IsiRegWriteIss ( pOV8825Ctx, OV8825_BLC_CTRL03, 0x02 );
        OV8825_IsiRegWriteIss ( pOV8825Ctx, OV8825_SC_CLKRST0, 0x70 );
        /* ddl@rock-chips.com: v0.1.2 */
        result = OV8825_IsiRegReadIss ( pOV8825Ctx, OV8825_MODE_SELECT, &RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss ( pOV8825Ctx, OV8825_MODE_SELECT, (RegValue | 0x01U) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result ); 
    }
    else
    {        
        /* disable streaming */
        #if 0     /* ddl@rock-chips.com: v0.1.2 */
        result = OV8825_IsiRegReadIss ( pOV8825Ctx, OV8825_MODE_SELECT, &RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss ( pOV8825Ctx, OV8825_MODE_SELECT, (RegValue & ~0x01U) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        #else
        OV8825_IsiRegWriteIss ( pOV8825Ctx, OV8825_SC_CLKRST0, 0x71 );
        #endif
    }

    if (result == RET_SUCCESS)
    {
        pOV8825Ctx->Streaming = on;
    }

    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiSensorSetPowerIss
 *
 * @brief   Performs the power-up/power-down sequence of the camera, if possible.
 *
 * @param   handle      OV8825 sensor instance handle
 * @param   on          new power state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiSensorSetPowerIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pOV8825Ctx->Configured = BOOL_FALSE;
    pOV8825Ctx->Streaming  = BOOL_FALSE;

    result = HalSetPower( pOV8825Ctx->IsiCtx.HalHandle, pOV8825Ctx->IsiCtx.HalDevID, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    result = HalSetReset( pOV8825Ctx->IsiCtx.HalHandle, pOV8825Ctx->IsiCtx.HalDevID, true );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    if (on == BOOL_TRUE)
    {
        result = HalSetPower( pOV8825Ctx->IsiCtx.HalHandle, pOV8825Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        result = HalSetReset( pOV8825Ctx->IsiCtx.HalHandle, pOV8825Ctx->IsiCtx.HalDevID, false );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        result = HalSetReset( pOV8825Ctx->IsiCtx.HalHandle, pOV8825Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );
        result = HalSetReset( pOV8825Ctx->IsiCtx.HalHandle, pOV8825Ctx->IsiCtx.HalDevID, false );

        osSleep( 50 );
    }

    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiCheckSensorConnectionIss
 *
 * @brief   Checks the I2C-Connection to sensor by reading sensor revision id.
 *
 * @param   handle      OV8825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiCheckSensorConnectionIss
(
    IsiSensorHandle_t   handle
)
{
    uint32_t RevId;
    uint32_t value;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    RevId = OV8825_CHIP_ID_HIGH_BYTE_DEFAULT;
    RevId = (RevId << 16U) | (OV8825_CHIP_ID_MIDDLE_BYTE_DEFAULT<<8U);
    RevId = RevId | OV8825_CHIP_ID_LOW_BYTE_DEFAULT;

    result = OV8825_IsiGetSensorRevisionIss( handle, &value );
    if ( (result != RET_SUCCESS) || (RevId != value) )
    {
        TRACE( OV8825_ERROR, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );
        return ( RET_FAILURE );
    }


    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetSensorRevisionIss
 *
 * @brief   reads the sensor revision register and returns this value
 *
 * @param   handle      pointer to sensor description struct
 * @param   p_value     pointer to storage value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetSensorRevisionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    uint32_t data;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *p_value = 0U;
    result = OV8825_IsiRegReadIss ( handle, OV8825_CHIP_ID_HIGH_BYTE, &data );
    *p_value = ( (data & 0xFF) << 16U );
    result = OV8825_IsiRegReadIss ( handle, OV8825_CHIP_ID_MIDDLE_BYTE, &data );
    *p_value |= ( (data & 0xFF) << 8U );
    result = OV8825_IsiRegReadIss ( handle, OV8825_CHIP_ID_LOW_BYTE, &data );
    *p_value |= ( (data & 0xFF));

    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiRegReadIss
 *
 * @brief   grants user read access to the camera register
 *
 * @param   handle      pointer to sensor description struct
 * @param   address     sensor register to write
 * @param   p_value     pointer to value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiRegReadIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

  //  TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, OV8825_g_aRegDescription_onelane);
        if ( !NrOfBytes )
        {
            NrOfBytes = 1;
        }
 //       TRACE( OV8825_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);

        *p_value = 0;
        result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
    }

  //  TRACE( OV8825_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiRegWriteIss
 *
 * @brief   grants user write access to the camera register
 *
 * @param   handle      pointer to sensor description struct
 * @param   address     sensor register to write
 * @param   value       value to write
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
static RESULT OV8825_IsiRegWriteIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NrOfBytes;

  //  TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    NrOfBytes = IsiGetNrDatBytesIss( address, OV8825_g_aRegDescription_onelane);
    if ( !NrOfBytes )
    {
        NrOfBytes = 1;
    }
//    TRACE( OV8825_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x 0x%08x)\n", __FUNCTION__, NrOfBytes, address, value);

    result = IsiI2cWriteSensorRegister( handle, address, (uint8_t *)(&value), NrOfBytes, BOOL_TRUE );

//    TRACE( OV8825_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetGainLimitsIss
 *
 * @brief   Returns the exposure minimal and maximal values of an
 *          OV8825 instance
 *
 * @param   handle       OV8825 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetGainLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinGain,
    float               *pMaxGain
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinGain == NULL) || (pMaxGain == NULL) )
    {
        TRACE( OV8825_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinGain = pOV8825Ctx->AecMinGain;
    *pMaxGain = pOV8825Ctx->AecMaxGain;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetIntegrationTimeLimitsIss
 *
 * @brief   Returns the minimal and maximal integration time values of an
 *          OV8825 instance
 *
 * @param   handle       OV8825 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetIntegrationTimeLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinIntegrationTime,
    float               *pMaxIntegrationTime
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL) )
    {
        TRACE( OV8825_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinIntegrationTime = pOV8825Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pOV8825Ctx->AecMaxIntegrationTime;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV8825_IsiGetGainIss
 *
 * @brief   Reads gain values from the image sensor module.
 *
 * @param   handle                  OV8825 sensor instance handle
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_IsiGetGainIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain = pOV8825Ctx->AecCurGain;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetGainIncrementIss
 *
 * @brief   Get smallest possible gain increment.
 *
 * @param   handle                  OV8825 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_IsiGetGainIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV8825Ctx->AecGainIncrement;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiSetGainIss
 *
 * @brief   Writes gain values to the image sensor module.
 *          Updates current gain and exposure in sensor struct/state.
 *
 * @param   handle                  OV8825 sensor instance handle
 * @param   NewGain                 gain to be set
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT OV8825_IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint16_t usGain = 0;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        TRACE( OV8825_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

  
    if( NewGain < pOV8825Ctx->AecMinGain ) NewGain = pOV8825Ctx->AecMinGain;
    if( NewGain > pOV8825Ctx->AecMaxGain ) NewGain = pOV8825Ctx->AecMaxGain;

    usGain = (uint16_t)(NewGain * 16.0f+0.5);

    // write new gain into sensor registers, do not write if nothing has changed
    if( (usGain != pOV8825Ctx->OldGain) )
    {
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_AGC_ADJ_H, (usGain>>8)&0x03);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_AGC_ADJ_L, (usGain&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        pOV8825Ctx->OldGain = usGain;
    }

    //calculate gain actually set
    pOV8825Ctx->AecCurGain = ( (float)usGain ) / 16.0f;

    //return current state
    *pSetGain = pOV8825Ctx->AecCurGain;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

#if 0
RESULT OV8825_IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint16_t usMultiplier = 0;
    uint8_t  ucMultiplier = 0;
    uint8_t  ucBase       = 0;
    uint16_t usGain = 0;
    int i=0;
    int gain_reg;

    float fBase  = 0.0f;
    float fDelta = 0.0f;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        TRACE( OV8825_INFO, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    //gain = (A[0]+1) * (B[7]+1) * (B[6]+1) * (B[5]+1) * (B[4]+1) * ( (B[3:0])/16 + 1 )
    //A/B = register 0x350A/B
    //minimum value is A=0, B=6
    //lower multiplier bits have to be set first

    //maximum and minimum gain is limited by the sensor, if this limit is not
    //considered, the exposure control loop needs lots of time to return to a new state
    //so limit to allowed range
    if( NewGain < pOV8825Ctx->AecMinGain ) NewGain = pOV8825Ctx->AecMinGain;
    if( NewGain > pOV8825Ctx->AecMaxGain ) NewGain = pOV8825Ctx->AecMaxGain;

    usGain = (int)(NewGain*16.0f);
    gain_reg = 0;
    for(i=0; i<5; i++){
        if(usGain>31){
            usGain = usGain/2;
            gain_reg = ( gain_reg |(0x10 << i) );        
        }else{
            break;
        }
    }

    usMultiplier = gain_reg;
    ucBase = usGain - 16;
    //gain_reg = gain_reg | (usGain - 16);

    // write new gain into sensor registers, do not write if nothing has changed
    if( (usMultiplier != pOV8825Ctx->OldMultiplier) || (ucBase != pOV8825Ctx->OldBase  ) )
    {
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_AGC_ADJ_H, (usMultiplier & 0x0300U) >> 8U  );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_AGC_ADJ_L, (usMultiplier & 0x00F0U) | ucBase );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        pOV8825Ctx->OldMultiplier = usMultiplier;
        pOV8825Ctx->OldBase       = ucBase;
    }

    //calculate gain actually set
    pOV8825Ctx->AecCurGain = ((float)ucMultiplier + 1)*(((float)ucBase)/16.0f + 1.0f);

    //return current state
    *pSetGain = pOV8825Ctx->AecCurGain;
    TRACE( OV8825_INFO, "%s: g=%f\n", __FUNCTION__, *pSetGain );

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}
#endif                  

/*****************************************************************************/
/**
 *          OV8825_IsiGetIntegrationTimeIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  OV8825 sensor instance handle
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_IsiGetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               *pSetIntegrationTime
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetIntegrationTime == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetIntegrationTime = pOV8825Ctx->AecCurIntegrationTime;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetIntegrationTimeIncrementIss
 *
 * @brief   Get smallest possible integration time increment.
 *
 * @param   handle                  OV8825 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_IsiGetIntegrationTimeIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV8825Ctx->AecIntegrationTimeIncrement;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiSetIntegrationTimeIss
 *
 * @brief   Writes gain and integration time values to the image sensor module.
 *          Updates current integration time and exposure in sensor
 *          struct/state.
 *
 * @param   handle                  OV8825 sensor instance handle
 * @param   NewIntegrationTime      integration time to be set
 * @param   pSetIntegrationTime     set integration time
 * @param   pNumberOfFramesToSkip   number of frames to skip until AE is
 *                                  executed again
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_FAILURE
 * @retval  RET_DIVISION_BY_ZERO
 *
 *****************************************************************************/
RESULT OV8825_IsiSetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               NewIntegrationTime,
    float               *pSetIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t CoarseIntegrationTime = 0;
    //uint32_t FineIntegrationTime   = 0; //not supported by OV8825

    float ShutterWidthPck = 0.0f; //shutter width in pixel clock periods

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetIntegrationTime == NULL) || (pNumberOfFramesToSkip == NULL) )
    {
        TRACE( OV8825_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    //maximum and minimum integration time is limited by the sensor, if this limit is not
    //considered, the exposure control loop needs lots of time to return to a new state
    //so limit to allowed range
    if ( NewIntegrationTime > pOV8825Ctx->AecMaxIntegrationTime ) NewIntegrationTime = pOV8825Ctx->AecMaxIntegrationTime;
    if ( NewIntegrationTime < pOV8825Ctx->AecMinIntegrationTime ) NewIntegrationTime = pOV8825Ctx->AecMinIntegrationTime;

    //the actual integration time is given by
    //integration_time = ( coarse_integration_time * line_length_pck + fine_integration_time ) / vt_pix_clk_freq
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck )
    //fine_integration_time   = integration_time * vt_pix_clk_freq - coarse_integration_time * line_length_pck
    //
    //fine integration is not supported by OV8825
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck + 0.5 )

    ShutterWidthPck = NewIntegrationTime * ( (float)pOV8825Ctx->VtPixClkFreq );

    // avoid division by zero
    if ( pOV8825Ctx->LineLengthPck == 0 )
    {
        TRACE( OV8825_ERROR, "%s: Division by zero!\n", __FUNCTION__ );
        return ( RET_DIVISION_BY_ZERO );
    }

    //calculate the integer part of the integration time in units of line length
    //calculate the fractional part of the integration time in units of pixel clocks
    //CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV8825Ctx->LineLengthPck) );
    //FineIntegrationTime   = ( (uint32_t)ShutterWidthPck ) - ( CoarseIntegrationTime * pOV8825Ctx->LineLengthPck );
    CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV8825Ctx->LineLengthPck) + 0.5f );

    // write new integration time into sensor registers
    // do not write if nothing has changed
    if( CoarseIntegrationTime != pOV8825Ctx->OldCoarseIntegrationTime )
    {
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_EXPO_H, (CoarseIntegrationTime & 0x0000F000U) >> 12U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_EXPO_M, (CoarseIntegrationTime & 0x00000FF0U) >> 4U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_AEC_EXPO_L, (CoarseIntegrationTime & 0x0000000FU) << 4U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );


        pOV8825Ctx->OldCoarseIntegrationTime = CoarseIntegrationTime;   // remember current integration time
        *pNumberOfFramesToSkip = 1U; //skip 1 frame
    }
    else
    {
        *pNumberOfFramesToSkip = 0U; //no frame skip
    }

    //if( FineIntegrationTime != pOV8825Ctx->OldFineIntegrationTime )
    //{
    //    result = OV8825_IsiRegWriteIss( pOV8825Ctx, ... , FineIntegrationTime );
    //    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    //    pOV8825Ctx->OldFineIntegrationTime = FineIntegrationTime; //remember current integration time
    //    *pNumberOfFramesToSkip = 1U; //skip 1 frame
    //}

    //calculate integration time actually set
    //pOV8825Ctx->AecCurIntegrationTime = ( ((float)CoarseIntegrationTime) * ((float)pOV8825Ctx->LineLengthPck) + ((float)FineIntegrationTime) ) / pOV8825Ctx->VtPixClkFreq;
    pOV8825Ctx->AecCurIntegrationTime = ((float)CoarseIntegrationTime) * ((float)pOV8825Ctx->LineLengthPck) / pOV8825Ctx->VtPixClkFreq;

    //return current state
    *pSetIntegrationTime = pOV8825Ctx->AecCurIntegrationTime;

   // TRACE( OV8825_ERROR, "%s: SetTi=%f NewTi=%f\n", __FUNCTION__, *pSetIntegrationTime,NewIntegrationTime);
    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}




/*****************************************************************************/
/**
 *          OV8825_IsiExposureControlIss
 *
 * @brief   Camera hardware dependent part of the exposure control loop.
 *          Calculates appropriate register settings from the new exposure
 *          values and writes them to the image sensor module.
 *
 * @param   handle                  OV8825 sensor instance handle
 * @param   NewGain                 newly calculated gain to be set
 * @param   NewIntegrationTime      newly calculated integration time to be set
 * @param   pNumberOfFramesToSkip   number of frames to skip until AE is
 *                                  executed again
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_FAILURE
 * @retval  RET_DIVISION_BY_ZERO
 *
 *****************************************************************************/
RESULT OV8825_IsiExposureControlIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               NewIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pNumberOfFramesToSkip == NULL)
            || (pSetGain == NULL)
            || (pSetIntegrationTime == NULL) )
    {
        TRACE( OV8825_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    TRACE( OV8825_DEBUG, "%s: g=%f, Ti=%f\n", __FUNCTION__, NewGain, NewIntegrationTime );


    result = OV8825_IsiSetIntegrationTimeIss( handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip );
    result = OV8825_IsiSetGainIss( handle, NewGain, pSetGain );

    TRACE( OV8825_DEBUG, "%s: set: g=%f, Ti=%f, skip=%d\n", __FUNCTION__, *pSetGain, *pSetIntegrationTime, *pNumberOfFramesToSkip );
    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetCurrentExposureIss
 *
 * @brief   Returns the currently adjusted AE values
 *
 * @param   handle                  OV8825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_IsiGetCurrentExposureIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetGain == NULL) || (pSetIntegrationTime == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain            = pOV8825Ctx->AecCurGain;
    *pSetIntegrationTime = pOV8825Ctx->AecCurIntegrationTime;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetResolutionIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  sensor instance handle
 * @param   pSettResolution         set resolution
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_IsiGetResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *pSetResolution
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetResolution == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetResolution = pOV8825Ctx->Config.Resolution;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetAfpsInfoHelperIss
 *
 * @brief   Calc AFPS sub resolution settings for the given resolution
 *
 * @param   pOV8825Ctx             OV8825 sensor instance (dummy!) context
 * @param   Resolution              Any supported resolution to query AFPS params for
 * @param   pAfpsInfo               Reference of AFPS info structure to write the results to
 * @param   AfpsStageIdx            Index of current AFPS stage to use
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetAfpsInfoHelperIss(
    OV8825_Context_t   *pOV8825Ctx,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo,
    uint32_t            AfpsStageIdx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT(pOV8825Ctx != NULL);
    DCT_ASSERT(pAfpsInfo != NULL);
    DCT_ASSERT(AfpsStageIdx <= ISI_NUM_AFPS_STAGES);

    // update resolution in copy of config in context
    pOV8825Ctx->Config.Resolution = Resolution;

    // tell sensor about that
    result = OV8825_SetupOutputWindow( pOV8825Ctx, &pOV8825Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8825_ERROR, "%s: SetupOutputWindow failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // update limits & stuff (reset current & old settings)
    result = OV8825_AecSetModeParameters( pOV8825Ctx, &pOV8825Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8825_ERROR, "%s: AecSetModeParameters failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // take over params
    pAfpsInfo->Stage[AfpsStageIdx].Resolution = Resolution;
    pAfpsInfo->Stage[AfpsStageIdx].MaxIntTime = pOV8825Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecMinGain           = pOV8825Ctx->AecMinGain;
    pAfpsInfo->AecMaxGain           = pOV8825Ctx->AecMaxGain;
    pAfpsInfo->AecMinIntTime        = pOV8825Ctx->AecMinIntegrationTime;
    pAfpsInfo->AecMaxIntTime        = pOV8825Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecSlowestResolution = Resolution;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV8825_IsiGetAfpsInfoIss
 *
 * @brief   Returns the possible AFPS sub resolution settings for the given resolution series
 *
 * @param   handle                  OV8825 sensor instance handle
 * @param   Resolution              Any resolution within the AFPS group to query;
 *                                  0 (zero) to use the currently configured resolution
 * @param   pAfpsInfo               Reference of AFPS info structure to store the results
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_NOTSUPP
 *
 *****************************************************************************/
RESULT OV8825_IsiGetAfpsInfoIss(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        TRACE( OV8825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pAfpsInfo == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    // use currently configured resolution?
    if (Resolution == 0)
    {
        Resolution = pOV8825Ctx->Config.Resolution;
    }

    // prepare index
    uint32_t idx = 0;

    // set current resolution data in info struct
    pAfpsInfo->CurrResolution = pOV8825Ctx->Config.Resolution;
    pAfpsInfo->CurrMinIntTime = pOV8825Ctx->AecMinIntegrationTime;
    pAfpsInfo->CurrMaxIntTime = pOV8825Ctx->AecMaxIntegrationTime;

    // allocate dummy context used for Afps parameter calculation as a copy of current context
    OV8825_Context_t *pDummyCtx = (OV8825_Context_t*) malloc( sizeof(OV8825_Context_t) );
    if ( pDummyCtx == NULL )
    {
        TRACE( OV8825_ERROR,  "%s: Can't allocate dummy ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    *pDummyCtx = *pOV8825Ctx;

    // set AFPS mode in dummy context
    pDummyCtx->isAfpsRun = BOOL_TRUE;

#define AFPSCHECKANDADD(_res_) \
    if ( (pOV8825Ctx->Config.AfpsResolutions & (_res_)) != 0 ) \
    { \
        RESULT lres = OV8825_IsiGetAfpsInfoHelperIss( pDummyCtx, _res_, pAfpsInfo, idx ); \
        if ( lres == RET_SUCCESS ) \
        { \
            ++idx; \
        } \
        else \
        { \
            UPDATE_RESULT( result, lres ); \
        } \
    }

    // check which AFPS series is requested and build its params list for the enabled AFPS resolutions
    switch(Resolution)
    {
        default:
            TRACE( OV8825_DEBUG,  "%s: Resolution %08x not supported by AFPS\n",  __FUNCTION__, Resolution );
            result = RET_NOTSUPP;
            break;

        #if 0
        // 1080p15 series in ascending integration time order (most probably the same as descending frame rate order)
        case ISI_RES_TV1080P15:
        case ISI_RES_TV1080P10:
        case ISI_RES_TV1080P5:
            AFPSCHECKANDADD( ISI_RES_TV1080P15 );
            AFPSCHECKANDADD( ISI_RES_TV1080P10 );
            AFPSCHECKANDADD( ISI_RES_TV1080P5  );
            break;
        #endif

        // check next series here...
    }

    // release dummy context again
    free(pDummyCtx);

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetCalibKFactor
 *
 * @brief   Returns the OV8825 specific K-Factor
 *
 * @param   handle       OV8825 sensor instance handle
 * @param   pIsiKFactor  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetCalibKFactor
(
    IsiSensorHandle_t   handle,
    Isi1x1FloatMatrix_t **pIsiKFactor
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiKFactor == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiKFactor = (Isi1x1FloatMatrix_t *)&OV8825_KFactor;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV8825_IsiGetCalibPcaMatrix
 *
 * @brief   Returns the OV8825 specific PCA-Matrix
 *
 * @param   handle          OV8825 sensor instance handle
 * @param   pIsiPcaMatrix   Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetCalibPcaMatrix
(
    IsiSensorHandle_t   handle,
    Isi3x2FloatMatrix_t **pIsiPcaMatrix
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiPcaMatrix == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiPcaMatrix = (Isi3x2FloatMatrix_t *)&OV8825_PCAMatrix;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns the sensor specific SvdMean-Vector
 *
 * @param   handle              OV8825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetCalibSvdMeanValue
(
    IsiSensorHandle_t   handle,
    Isi3x1FloatMatrix_t **pIsiSvdMeanValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSvdMeanValue == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiSvdMeanValue = (Isi3x1FloatMatrix_t *)&OV8825_SVDMeanValue;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns a pointer to the sensor specific centerline, a straight
 *          line in Hesse normal form in Rg/Bg colorspace
 *
 * @param   handle              OV8825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetCalibCenterLine
(
    IsiSensorHandle_t   handle,
    IsiLine_t           **ptIsiCenterLine
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiCenterLine == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiCenterLine = (IsiLine_t*)&OV8825_CenterLine;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetCalibClipParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for Rg/Bg color
 *          space clipping
 *
 * @param   handle              OV8825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetCalibClipParam
(
    IsiSensorHandle_t   handle,
    IsiAwbClipParm_t    **pIsiClipParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiClipParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiClipParam = (IsiAwbClipParm_t *)&OV8825_AwbClipParm;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetCalibGlobalFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle              OV8825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetCalibGlobalFadeParam
(
    IsiSensorHandle_t       handle,
    IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiGlobalFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiGlobalFadeParam = (IsiAwbGlobalFadeParm_t *)&OV8825_AwbGlobalFadeParm;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetCalibFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for near white
 *          pixel parameter calculations
 *
 * @param   handle              OV8825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetCalibFadeParam
(
    IsiSensorHandle_t   handle,
    IsiAwbFade2Parm_t   **ptIsiFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiFadeParam = (IsiAwbFade2Parm_t *)&OV8825_AwbFade2Parm;

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV8825_IsiGetIlluProfile
 *
 * @brief   Returns a pointer to illumination profile idetified by CieProfile
 *          bitmask
 *
 * @param   handle              sensor instance handle
 * @param   CieProfile
 * @param   ptIsiIlluProfile    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetIlluProfile
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiIlluProfile_t    **ptIsiIlluProfile
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiIlluProfile == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        uint16_t i;

        *ptIsiIlluProfile = NULL;

        /* check if we've a default profile */
        for ( i=0U; i<OV8825_ISIILLUPROFILES_DEFAULT; i++ )
        {
            if ( OV8825_IlluProfileDefault[i].id == CieProfile )
            {
                *ptIsiIlluProfile = &OV8825_IlluProfileDefault[i];
                break;
            }
        }

        result = ( *ptIsiIlluProfile != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
    }

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetLscMatrixTable
 *
 * @brief   Returns a pointer to illumination profile idetified by CieProfile
 *          bitmask
 *
 * @param   handle              sensor instance handle
 * @param   CieProfile
 * @param   ptIsiIlluProfile    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiGetLscMatrixTable
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiLscMatrixTable_t **pLscMatrixTable
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pLscMatrixTable == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        uint16_t i;


        switch ( CieProfile )
        {
            case ISI_CIEPROF_A:
            {
                if ( ( pOV8825Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_A_1920x1080;
                }
                #if 0
                else if ( pOV8825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_A_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F2:
            {
                if ( ( pOV8825Ctx->Config.Resolution == ISI_RES_TV1080P30 ) )
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_F2_1920x1080;
                }
                #if 0
                else if ( pOV8825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_F2_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D50:
            {
                if ( ( pOV8825Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_D50_1920x1080;
                }
                #if 0
                else if ( pOV8825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_D50_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D65:
            case ISI_CIEPROF_D75:
            {
                if ( ( pOV8825Ctx->Config.Resolution == ISI_RES_TV1080P30 ) )
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_D65_1920x1080;
                }
                #if 0
                else if ( pOV8825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_D65_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F11:
            {
                if ( ( pOV8825Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_F11_1920x1080;
                }
                #if 0
                else if ( pOV8825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8825_LscMatrixTable_CIE_F11_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            default:
            {
                TRACE( OV8825_ERROR, "%s: Illumination not supported\n", __FUNCTION__ );
                *pLscMatrixTable = NULL;
                break;
            }
        }

        result = ( *pLscMatrixTable != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
    }

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV8825_IsiMdiInitMotoDriveMds
 *
 * @brief   General initialisation tasks like I/O initialisation.
 *
 * @param   handle              OV8825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiMdiInitMotoDriveMds
(
    IsiSensorHandle_t   handle
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiMdiSetupMotoDrive
 *
 * @brief   Setup of the MotoDrive and return possible max step.
 *
 * @param   handle          OV8825 sensor instance handle
 *          pMaxStep        pointer to variable to receive the maximum
 *                          possible focus step
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiMdiSetupMotoDrive
(
    IsiSensorHandle_t   handle,
    uint32_t            *pMaxStep
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;
    uint32_t vcm_movefull_t;
    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMaxStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    /* ddl@rock-chips.com: v0.3.0 */
    if (pOV8825Ctx->VcmInfo.StepMode <= 7) {
        vcm_movefull_t = 52*(1<<(pOV8825Ctx->VcmInfo.StepMode-1));
    } else if ((pOV8825Ctx->VcmInfo.StepMode>=9) && (pOV8825Ctx->VcmInfo.StepMode<=15)) {
        vcm_movefull_t = 2*(1<<(pOV8825Ctx->VcmInfo.StepMode-9));
    } else {
        TRACE( OV8825_ERROR, "%s: pOV8825Ctx->VcmInfo.StepMode: %d is invalidate!\n",__FUNCTION__, pOV8825Ctx->VcmInfo.StepMode);
        DCT_ASSERT(0);
    }

    *pMaxStep = (MAX_LOG|(vcm_movefull_t<<16));

    result = OV8825_IsiMdiFocusSet( handle, MAX_LOG );

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiMdiFocusSet
 *
 * @brief   Drives the lens system to a certain focus point.
 *
 * @param   handle          OV8825 sensor instance handle
 *          AbsStep         absolute focus point to apply
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiMdiFocusSet
(
    IsiSensorHandle_t   handle,
    const uint32_t      Position
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t nPosition;
    uint8_t  data[2] = { 0, 0 };

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
    	TRACE( OV8825_ERROR, "%s: pOV8825Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    /* map 64 to 0 -> infinity */

	if( Position > MAX_LOG ){
		TRACE( OV8825_ERROR, "%s: pOV8825Ctx Position (%d) max_position(%d)\n", __FUNCTION__,Position, MAX_LOG);
		//Position = MAX_LOG;
	}	
    /* ddl@rock-chips.com: v0.3.0 */
    if ( Position >= MAX_LOG )
        nPosition = pOV8825Ctx->VcmInfo.StartCurrent;
    else 
        nPosition = pOV8825Ctx->VcmInfo.StartCurrent + (pOV8825Ctx->VcmInfo.Step*(MAX_LOG-Position));
    /* ddl@rock-chips.com: v0.6.0 */
    if (nPosition > MAX_VCMDRV_REG)  
        nPosition = MAX_VCMDRV_REG;

    TRACE( OV8825_INFO, "%s: focus set position_reg_value(%d) position(%d) \n", __FUNCTION__, nPosition, Position);
    
    data[1] = (uint8_t)(0x00U | (( nPosition & 0x3F0U ) >> 4U));                 // PD,  1, D9..D4, see AD5820 datasheet
    data[0] = (uint8_t)( ((nPosition & 0x0FU) << 4U) | pOV8825Ctx->VcmInfo.StepMode );    // D3..D0, S3..S0

    TRACE( OV8825_INFO, "%s: value = %d, 0x%02x 0x%02x\n", __FUNCTION__, nPosition, data[0], data[1] );

    result = HalWriteI2CMem( pOV8825Ctx->IsiCtx.HalHandle,
                             pOV8825Ctx->IsiCtx.I2cAfBusNum,
                             pOV8825Ctx->IsiCtx.SlaveAfAddress,
                             0x3618,
                             pOV8825Ctx->IsiCtx.NrOfAfAddressBytes,
                             &data[0],
                             1U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    
    result = HalWriteI2CMem( pOV8825Ctx->IsiCtx.HalHandle,
                             pOV8825Ctx->IsiCtx.I2cAfBusNum,
                             pOV8825Ctx->IsiCtx.SlaveAfAddress,
                             0x3619,
                             pOV8825Ctx->IsiCtx.NrOfAfAddressBytes,
                             &data[1],
                             1U );
                             
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiMdiFocusGet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          OV8825 sensor instance handle
 *          pAbsStep        pointer to a variable to receive the current
 *                          focus point
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiMdiFocusGet
(
    IsiSensorHandle_t   handle,
    uint32_t            *pAbsStep
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;
    uint8_t  data[2] = { 0, 0 };

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pAbsStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    /*
    result = HalReadI2CMem( pOV8825Ctx->IsiCtx.HalHandle,
                            pOV8825Ctx->IsiCtx.I2cAfBusNum,
                            pOV8825Ctx->IsiCtx.SlaveAfAddress,
                            0,
                            pOV8825Ctx->IsiCtx.NrOfAfAddressBytes,
                            data,
                            2U );
*/
	result = HalReadI2CMem( pOV8825Ctx->IsiCtx.HalHandle,
                             pOV8825Ctx->IsiCtx.I2cAfBusNum,
                             pOV8825Ctx->IsiCtx.SlaveAfAddress,
                             0x3618,
                             pOV8825Ctx->IsiCtx.NrOfAfAddressBytes,
                             &data[0],
                             1U );
	RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
	
	RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
	result = HalReadI2CMem( pOV8825Ctx->IsiCtx.HalHandle,
                             pOV8825Ctx->IsiCtx.I2cAfBusNum,
                             pOV8825Ctx->IsiCtx.SlaveAfAddress,
                             0x3619,
                             pOV8825Ctx->IsiCtx.NrOfAfAddressBytes,
                             &data[1],
                             1U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
	

    TRACE( OV8825_INFO, "%s: value = 0x%02x 0x%02x\n", __FUNCTION__, data[1], data[0] );

    /* Data[0] = PD,  1, D9..D4, see AD5820 datasheet */
    /* Data[1] = D3..D0, S3..S0 */
    uint32_t tmp;
    tmp = *pAbsStep = ( ((uint32_t)(data[1] & 0x3FU)) << 4U ) | ( ((uint32_t)data[0]) >> 4U );

    /* map 0 to 64 -> infinity */   /* ddl@rock-chips.com: v0.3.0 */
    if( *pAbsStep <= pOV8825Ctx->VcmInfo.StartCurrent)
    {
        *pAbsStep = MAX_LOG;
    }
    else if((*pAbsStep>pOV8825Ctx->VcmInfo.StartCurrent) && (*pAbsStep<=pOV8825Ctx->VcmInfo.RatedCurrent))
    {
        *pAbsStep = (pOV8825Ctx->VcmInfo.RatedCurrent - *pAbsStep ) / pOV8825Ctx->VcmInfo.Step;
    }
	else
	{
		*pAbsStep = 0;
	}


    TRACE( OV8825_INFO, "%s: get nposition(%d) (exit)\n", __FUNCTION__, *pAbsStep);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiMdiFocusCalibrate
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV8825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8825_IsiMdiFocusCalibrate
(
    IsiSensorHandle_t   handle
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiActivateTestPattern
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV8825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV8825_IsiActivateTestPattern
(
    IsiSensorHandle_t   handle,
    const bool_t        enable
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t ulRegValue = 0UL;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( BOOL_TRUE == enable )
    {
        /* enable test-pattern */
        result = OV8825_IsiRegReadIss( pOV8825Ctx, OV8825_PRE_ISP_CTRL00, &ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        ulRegValue |= ( 0x80U );

        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PRE_ISP_CTRL00, ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }
    else
    {
        /* disable test-pattern */
        result = OV8825_IsiRegReadIss( pOV8825Ctx, OV8825_PRE_ISP_CTRL00, &ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        ulRegValue &= ~( 0x80 );

        result = OV8825_IsiRegWriteIss( pOV8825Ctx, OV8825_PRE_ISP_CTRL00, ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }

     pOV8825Ctx->TestPattern = enable;
    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8825_IsiGetSensorMipiInfoIss
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV8825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV8825_IsiGetSensorMipiInfoIss
(
    IsiSensorHandle_t   handle,
    IsiSensorMipiInfo   *ptIsiSensorMipiInfo
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }


    if ( ptIsiSensorMipiInfo == NULL )
    {
        return ( result );
    }

    ptIsiSensorMipiInfo->ucMipiLanes = pOV8825Ctx->IsiSensorMipiInfo.ucMipiLanes;
    ptIsiSensorMipiInfo->ulMipiFreq= pOV8825Ctx->IsiSensorMipiInfo.ulMipiFreq;
    ptIsiSensorMipiInfo->sensorHalDevID = pOV8825Ctx->IsiSensorMipiInfo.sensorHalDevID;
    


    TRACE( OV8825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV8825_IsiGetSensorIsiVersion
(  IsiSensorHandle_t   handle,
   unsigned int*     pVersion
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
    	TRACE( OV8825_ERROR, "%s: pOV8825Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pVersion == NULL)
	{
		TRACE( OV8825_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pVersion = CONFIG_ISI_VERSION;
	return result;
}

static RESULT OV8825_IsiGetSensorTuningXmlVersion
(  IsiSensorHandle_t   handle,
   char**     pTuningXmlVersion
)
{
    OV8825_Context_t *pOV8825Ctx = (OV8825_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV8825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8825Ctx == NULL )
    {
    	TRACE( OV8825_ERROR, "%s: pOV8825Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pTuningXmlVersion == NULL)
	{
		TRACE( OV8825_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pTuningXmlVersion = OV8825_NEWEST_TUNING_XML;
	return result;
}


/*****************************************************************************/
/**
 *          OV8825_IsiGetSensorIss
 *
 * @brief   fills in the correct pointers for the sensor description struct
 *
 * @param   param1      pointer to sensor description struct
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8825_IsiGetSensorIss
(
    IsiSensor_t *pIsiSensor
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV8825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pIsiSensor != NULL )
    {
        pIsiSensor->pszName                             = OV8825_g_acName;
        pIsiSensor->pRegisterTable                      = OV8825_g_aRegDescription_onelane;
        pIsiSensor->pIsiSensorCaps                      = &OV8825_g_IsiSensorDefaultConfig;
		pIsiSensor->pIsiGetSensorIsiVer					= OV8825_IsiGetSensorIsiVersion;//oyyf
		pIsiSensor->pIsiGetSensorTuningXmlVersion		= OV8825_IsiGetSensorTuningXmlVersion;//oyyf
        pIsiSensor->pIsiCreateSensorIss                 = OV8825_IsiCreateSensorIss;
        pIsiSensor->pIsiReleaseSensorIss                = OV8825_IsiReleaseSensorIss;
        pIsiSensor->pIsiGetCapsIss                      = OV8825_IsiGetCapsIss;
        pIsiSensor->pIsiSetupSensorIss                  = OV8825_IsiSetupSensorIss;
        pIsiSensor->pIsiChangeSensorResolutionIss       = OV8825_IsiChangeSensorResolutionIss;
        pIsiSensor->pIsiSensorSetStreamingIss           = OV8825_IsiSensorSetStreamingIss;
        pIsiSensor->pIsiSensorSetPowerIss               = OV8825_IsiSensorSetPowerIss;
        pIsiSensor->pIsiCheckSensorConnectionIss        = OV8825_IsiCheckSensorConnectionIss;
        pIsiSensor->pIsiGetSensorRevisionIss            = OV8825_IsiGetSensorRevisionIss;
        pIsiSensor->pIsiRegisterReadIss                 = OV8825_IsiRegReadIss;
        pIsiSensor->pIsiRegisterWriteIss                = OV8825_IsiRegWriteIss;

        /* AEC functions */
        pIsiSensor->pIsiExposureControlIss              = OV8825_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = OV8825_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = OV8825_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetCurrentExposureIss           = OV8825_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetGainIss                      = OV8825_IsiGetGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = OV8825_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = OV8825_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = OV8825_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = OV8825_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = OV8825_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetResolutionIss                = OV8825_IsiGetResolutionIss;
        pIsiSensor->pIsiGetAfpsInfoIss                  = OV8825_IsiGetAfpsInfoIss;

        /* AWB specific functions */
        pIsiSensor->pIsiGetCalibKFactor                 = OV8825_IsiGetCalibKFactor;
        pIsiSensor->pIsiGetCalibPcaMatrix               = OV8825_IsiGetCalibPcaMatrix;
        pIsiSensor->pIsiGetCalibSvdMeanValue            = OV8825_IsiGetCalibSvdMeanValue;
        pIsiSensor->pIsiGetCalibCenterLine              = OV8825_IsiGetCalibCenterLine;
        pIsiSensor->pIsiGetCalibClipParam               = OV8825_IsiGetCalibClipParam;
        pIsiSensor->pIsiGetCalibGlobalFadeParam         = OV8825_IsiGetCalibGlobalFadeParam;
        pIsiSensor->pIsiGetCalibFadeParam               = OV8825_IsiGetCalibFadeParam;
        pIsiSensor->pIsiGetIlluProfile                  = OV8825_IsiGetIlluProfile;
        pIsiSensor->pIsiGetLscMatrixTable               = OV8825_IsiGetLscMatrixTable;

        /* AF functions */
        pIsiSensor->pIsiMdiInitMotoDriveMds             = OV8825_IsiMdiInitMotoDriveMds;
        pIsiSensor->pIsiMdiSetupMotoDrive               = OV8825_IsiMdiSetupMotoDrive;
        pIsiSensor->pIsiMdiFocusSet                     = OV8825_IsiMdiFocusSet;
        pIsiSensor->pIsiMdiFocusGet                     = OV8825_IsiMdiFocusGet;
        pIsiSensor->pIsiMdiFocusCalibrate               = OV8825_IsiMdiFocusCalibrate;

        /* MIPI */
        pIsiSensor->pIsiGetSensorMipiInfoIss            = OV8825_IsiGetSensorMipiInfoIss;

        /* Testpattern */
        pIsiSensor->pIsiActivateTestPattern             = OV8825_IsiActivateTestPattern;
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( OV8825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV8825_IsiGetSensorI2cInfo(sensor_i2c_info_t** pdata)
{
    sensor_i2c_info_t* pSensorI2cInfo;

    pSensorI2cInfo = ( sensor_i2c_info_t * )malloc ( sizeof (sensor_i2c_info_t) );

    if ( pSensorI2cInfo == NULL )
    {
        TRACE( OV8825_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pSensorI2cInfo, 0, sizeof( sensor_i2c_info_t ) );

    
    pSensorI2cInfo->i2c_addr = OV8825_SLAVE_ADDR;
    pSensorI2cInfo->soft_reg_addr = OV8825_SOFTWARE_RST;
    pSensorI2cInfo->soft_reg_value = 0x01;
    pSensorI2cInfo->reg_size = 2;
    pSensorI2cInfo->value_size = 1;

    {
        IsiSensorCaps_t Caps;
        sensor_caps_t *pCaps;
        uint32_t lanes,i;        

        for (i=0; i<3; i++) {
            lanes = (1<<i);
            ListInit(&pSensorI2cInfo->lane_res[i]);
            if (g_suppoted_mipi_lanenum_type & lanes) {
                Caps.Index = 0;            
                while(OV8825_IsiGetCapsIssInternal(&Caps,lanes)==RET_SUCCESS) {
                    pCaps = malloc(sizeof(sensor_caps_t));
                    if (pCaps != NULL) {
                        memcpy(&pCaps->caps,&Caps,sizeof(IsiSensorCaps_t));
                        ListPrepareItem(pCaps);
                        ListAddTail(&pSensorI2cInfo->lane_res[i], pCaps);
                    }
                    Caps.Index++;
                }
            }
        }
    } 
    
    ListInit(&pSensorI2cInfo->chipid_info);

    sensor_chipid_info_t* pChipIDInfo_H = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_H )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_H, 0, sizeof(*pChipIDInfo_H) );    
    pChipIDInfo_H->chipid_reg_addr = OV8825_CHIP_ID_HIGH_BYTE;  
    pChipIDInfo_H->chipid_reg_value = OV8825_CHIP_ID_HIGH_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_H );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_H );

    sensor_chipid_info_t* pChipIDInfo_M = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_M )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_M, 0, sizeof(*pChipIDInfo_M) ); 
    pChipIDInfo_M->chipid_reg_addr = OV8825_CHIP_ID_MIDDLE_BYTE;
    pChipIDInfo_M->chipid_reg_value = OV8825_CHIP_ID_MIDDLE_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_M );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_M );
    
    sensor_chipid_info_t* pChipIDInfo_L = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_L )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_L, 0, sizeof(*pChipIDInfo_L) ); 
    pChipIDInfo_L->chipid_reg_addr = OV8825_CHIP_ID_LOW_BYTE;
    pChipIDInfo_L->chipid_reg_value = OV8825_CHIP_ID_LOW_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_L );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_L );

	//oyyf sensor drv version
	pSensorI2cInfo->sensor_drv_version = CONFIG_SENSOR_DRV_VERSION;

    *pdata = pSensorI2cInfo;
    return RET_SUCCESS;
}

/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/*****************************************************************************/
/**
 */
/*****************************************************************************/
IsiCamDrvConfig_t IsiCamDrvConfig =
{
    0,
    OV8825_IsiGetSensorIss,
    {
        0,                      /**< IsiSensor_t.pszName */
        0,                      /**< IsiSensor_t.pRegisterTable */
        0,                      /**< IsiSensor_t.pIsiSensorCaps */
        0,						/**< IsiSensor_t.pIsiGetSensorIsiVer_t>*/   //oyyf add
        0,                      /**< IsiSensor_t.pIsiGetSensorTuningXmlVersion_t>*/   //oyyf add 
        0,                      /**< IsiSensor_t.pIsiWhiteBalanceIlluminationChk>*/   //ddl@rock-chips.com 
        0,                      /**< IsiSensor_t.pIsiWhiteBalanceIlluminationSet>*/   //ddl@rock-chips.com
        0,                      /**< IsiSensor_t.pIsiCheckOTPInfo>*/  //zyc 
        0,                      /**< IsiSensor_t.pIsiCreateSensorIss */
        0,                      /**< IsiSensor_t.pIsiReleaseSensorIss */
        0,                      /**< IsiSensor_t.pIsiGetCapsIss */
        0,                      /**< IsiSensor_t.pIsiSetupSensorIss */
        0,                      /**< IsiSensor_t.pIsiChangeSensorResolutionIss */
        0,                      /**< IsiSensor_t.pIsiSensorSetStreamingIss */
        0,                      /**< IsiSensor_t.pIsiSensorSetPowerIss */
        0,                      /**< IsiSensor_t.pIsiCheckSensorConnectionIss */
        0,                      /**< IsiSensor_t.pIsiGetSensorRevisionIss */
        0,                      /**< IsiSensor_t.pIsiRegisterReadIss */
        0,                      /**< IsiSensor_t.pIsiRegisterWriteIss */

        0,                      /**< IsiSensor_t.pIsiExposureControlIss */
        0,                      /**< IsiSensor_t.pIsiGetGainLimitsIss */
        0,                      /**< IsiSensor_t.pIsiGetIntegrationTimeLimitsIss */
        0,                      /**< IsiSensor_t.pIsiGetCurrentExposureIss */
        0,                      /**< IsiSensor_t.pIsiGetGainIss */
        0,                      /**< IsiSensor_t.pIsiGetGainIncrementIss */
        0,                      /**< IsiSensor_t.pIsiSetGainIss */
        0,                      /**< IsiSensor_t.pIsiGetIntegrationTimeIss */
        0,                      /**< IsiSensor_t.pIsiGetIntegrationTimeIncrementIss */
        0,                      /**< IsiSensor_t.pIsiSetIntegrationTimeIss */
        0,                      /**< IsiSensor_t.pIsiGetResolutionIss */
        0,                      /**< IsiSensor_t.pIsiGetAfpsInfoIss */

        0,                      /**< IsiSensor_t.pIsiGetCalibKFactor */
        0,                      /**< IsiSensor_t.pIsiGetCalibPcaMatrix */
        0,                      /**< IsiSensor_t.pIsiGetCalibSvdMeanValue */
        0,                      /**< IsiSensor_t.pIsiGetCalibCenterLine */
        0,                      /**< IsiSensor_t.pIsiGetCalibClipParam */
        0,                      /**< IsiSensor_t.pIsiGetCalibGlobalFadeParam */
        0,                      /**< IsiSensor_t.pIsiGetCalibFadeParam */
        0,                      /**< IsiSensor_t.pIsiGetIlluProfile */
        0,                      /**< IsiSensor_t.pIsiGetLscMatrixTable */

        0,                      /**< IsiSensor_t.pIsiMdiInitMotoDriveMds */
        0,                      /**< IsiSensor_t.pIsiMdiSetupMotoDrive */
        0,                      /**< IsiSensor_t.pIsiMdiFocusSet */
        0,                      /**< IsiSensor_t.pIsiMdiFocusGet */
        0,                      /**< IsiSensor_t.pIsiMdiFocusCalibrate */

        0,                      /**< IsiSensor_t.pIsiGetSensorMipiInfoIss */

        0,                      /**< IsiSensor_t.pIsiActivateTestPattern */
    },
    OV8825_IsiGetSensorI2cInfo,
};


