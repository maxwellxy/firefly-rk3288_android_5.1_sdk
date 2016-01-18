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
 * @file OV5647.c
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

#include "OV5647_priv.h"


#define CC_OFFSET_SCALING  2.0f
#define I2C_COMPLIANT_STARTBIT 1U

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( OV5647_INFO , "OV5647: ", INFO,    1U );
CREATE_TRACER( OV5647_WARN , "OV5647: ", WARNING, 1U );
CREATE_TRACER( OV5647_ERROR, "OV5647: ", ERROR,   1U );

CREATE_TRACER( OV5647_DEBUG, "OV5647: ", INFO,     0U );

CREATE_TRACER( OV5647_REG_INFO , "OV5647: ", INFO, 0);
CREATE_TRACER( OV5647_REG_DEBUG, "OV5647: ", INFO, 0U );

#define OV5647_SLAVE_ADDR       0x6cU                           /**< i2c slave address of the OV5647 camera sensor */
#define OV5647_SLAVE_AF_ADDR    0x00U                           /**< i2c slave address of the OV5647 integrated AD5820 */

#define OV5647_MIN_GAIN_STEP   ( 16.0f ); /**< min gain step size used by GUI ( 32/(32-7) - 32/(32-6); min. reg value is 6 as of datasheet; depending on actual gain ) */
#define OV5647_MAX_GAIN_AEC    ( 976.0f )            /**< max. gain used by the AEC (arbitrarily chosen, recommended by Omnivision) */


/*!<
 * Focus position values:
 * 65 logical positions ( 0 - 64 )
 * where 64 is the setting for infinity and 0 for macro
 * corresponding to
 * 1024 register settings (0 - 1023)
 * where 0 is the setting for infinity and 1023 for macro
 */
#define MAX_LOG   64U
#define MAX_REG 1023U



/*!<
 * Lens movement is triggered every 133ms (VGA, 7.5fps processed frames
 * worst case assumed, usually even much slower, see OV5630 driver for
 * details). Thus the lens has to reach the requested position after
 * max. 133ms. Minimum mechanical ringing is expected with mode 1 ,
 * 100us per step. A movement over the full range needs max. 102.3ms
 * (see table 9 AD5820 datasheet).
 */
#define MDI_SLEW_RATE_CTRL 2U /* S3..0 */



/******************************************************************************
 * local variable declarations
 *****************************************************************************/
const char OV5647_g_acName[] = "OV5647_MIPI";
extern const IsiRegDescription_t OV5647_g_aRegDescription[];
extern const IsiRegDescription_t OV5647_g_2592_1944[];

const IsiSensorCaps_t OV5647_g_IsiSensorDefaultConfig;

/* AWB specific value (from OV5647_tables.c) */
extern const Isi1x1FloatMatrix_t    OV5647_KFactor;
extern const Isi3x2FloatMatrix_t    OV5647_PCAMatrix;
extern const Isi3x1FloatMatrix_t    OV5647_SVDMeanValue;
extern const IsiLine_t              OV5647_CenterLine;
extern const IsiAwbClipParm_t       OV5647_AwbClipParm;
extern const IsiAwbGlobalFadeParm_t OV5647_AwbGlobalFadeParm;
extern const IsiAwbFade2Parm_t      OV5647_AwbFade2Parm;

/* illumination profiles */
#include "OV5647_a.h"       /* CIE A - default profile */
#include "OV5647_f2.h"      /* CIE F2 (cool white flourescent CWF) */
#include "OV5647_d50.h"     /* CIE D50 (D50 lightbox) */
#include "OV5647_d65.h"     /* CIE D65 (D65) note: indoor because of our lightbox */
#include "OV5647_d75.h"     /* CIE D75 (D75) overcast daylight, 7500K */
#include "OV5647_f11.h"     /* CIE F11 (TL84) */

#define OV5647_I2C_START_BIT        (I2C_COMPLIANT_STARTBIT)    // I2C bus start condition
#define OV5647_I2C_NR_ADR_BYTES     (2U)                        // 1 byte base address and 2 bytes sub address
#define OV5647_I2C_NR_DAT_BYTES     (1U)                        // 8 bit registers


#define OV5647_ISIILLUPROFILES_DEFAULT  6U
static IsiIlluProfile_t OV5647_IlluProfileDefault[OV5647_ISIILLUPROFILES_DEFAULT] =
{
    {
        .p_next             = NULL,

        .name               = "A",
        .id                 = ISI_CIEPROF_A,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV5647_XTalkCoeff_CIE_A,
        .pCrossTalkOffset   = &OV5647_XTalkOffset_CIE_A,

        .pGaussMeanValue    = &OV5647_GaussMeanValue_CIE_A,
        .pCovarianceMatrix  = &OV5647_CovarianceMatrix_CIE_A,
        .pGaussFactor       = &OV5647_GaussFactor_CIE_A,
        .pThreshold         = &OV5647_Threshold_CIE_A,
        .pComponentGain     = &OV5647_CompGain_CIE_A,

        .pSaturationCurve   = &OV5647_SaturationCurve_CIE_A,
        .pCcMatrixTable     = &OV5647_CcMatrixTable_CIE_A,
        .pCcOffsetTable     = &OV5647_CcOffsetTable_CIE_A,

        .pVignettingCurve   = &OV5647_VignettingCurve_CIE_A,
    },
    {
        .p_next             = NULL,

        .name               = "F2",
        .id                 = ISI_CIEPROF_F2,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV5647_XTalkCoeff_F2,
        .pCrossTalkOffset   = &OV5647_XTalkOffset_F2,

        .pGaussMeanValue    = &OV5647_GaussMeanValue_F2,
        .pCovarianceMatrix  = &OV5647_CovarianceMatrix_F2,
        .pGaussFactor       = &OV5647_GaussFactor_F2,
        .pThreshold         = &OV5647_Threshold_F2,
        .pComponentGain     = &OV5647_CompGain_F2,

        .pSaturationCurve   = &OV5647_SaturationCurve_F2,
        .pCcMatrixTable     = &OV5647_CcMatrixTable_F2,
        .pCcOffsetTable     = &OV5647_CcOffsetTable_F2,

        .pVignettingCurve   = &OV5647_VignettingCurve_F2,
    },
    {
        .p_next             = NULL,

        .name               = "D50",
        .id                 = ISI_CIEPROF_D50,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,             /* from lightbox */
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_TRUE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV5647_XTalkCoeff_D50,
        .pCrossTalkOffset   = &OV5647_XTalkOffset_D50,

        .pGaussMeanValue    = &OV5647_GaussMeanValue_D50,
        .pCovarianceMatrix  = &OV5647_CovarianceMatrix_D50,
        .pGaussFactor       = &OV5647_GaussFactor_D50,
        .pThreshold         = &OV5647_Threshold_D50,
        .pComponentGain     = &OV5647_CompGain_D50,

        .pSaturationCurve   = &OV5647_SaturationCurve_D50,
        .pCcMatrixTable     = &OV5647_CcMatrixTable_D50,
        .pCcOffsetTable     = &OV5647_CcOffsetTable_D50,

        .pVignettingCurve   = &OV5647_VignettingCurve_D50,
    },
    {
        .p_next             = NULL,

        .name               = "D65",
        .id                 = ISI_CIEPROF_D65,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV5647_XTalkCoeff_D65,
        .pCrossTalkOffset   = &OV5647_XTalkOffset_D65,

        .pGaussMeanValue    = &OV5647_GaussMeanValue_D65,
        .pCovarianceMatrix  = &OV5647_CovarianceMatrix_D65,
        .pGaussFactor       = &OV5647_GaussFactor_D65,
        .pThreshold         = &OV5647_Threshold_D65,
        .pComponentGain     = &OV5647_CompGain_D65,

        .pSaturationCurve   = &OV5647_SaturationCurve_D65,
        .pCcMatrixTable     = &OV5647_CcMatrixTable_D65,
        .pCcOffsetTable     = &OV5647_CcOffsetTable_D65,

        .pVignettingCurve   = &OV5647_VignettingCurve_D65,
    },
    {
        .p_next             = NULL,

        .name               = "D75",
        .id                 = ISI_CIEPROF_D75,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV5647_XTalkCoeff_D75,
        .pCrossTalkOffset   = &OV5647_XTalkOffset_D75,

        .pGaussMeanValue    = &OV5647_GaussMeanValue_D75,
        .pCovarianceMatrix  = &OV5647_CovarianceMatrix_D75,
        .pGaussFactor       = &OV5647_GaussFactor_D75,
        .pThreshold         = &OV5647_Threshold_D75,
        .pComponentGain     = &OV5647_CompGain_D75,

        .pSaturationCurve   = &OV5647_SaturationCurve_D75,
        .pCcMatrixTable     = &OV5647_CcMatrixTable_D75,
        .pCcOffsetTable     = &OV5647_CcOffsetTable_D75,

        .pVignettingCurve   = &OV5647_VignettingCurve_D75,
    },
    {
        .p_next             = NULL,

        .name               = "F11",
        .id                 = ISI_CIEPROF_F11,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV5647_XTalkCoeff_F11,
        .pCrossTalkOffset   = &OV5647_XTalkOffset_F11,

        .pGaussMeanValue    = &OV5647_GaussMeanValue_F11,
        .pCovarianceMatrix  = &OV5647_CovarianceMatrix_F11,
        .pGaussFactor       = &OV5647_GaussFactor_F11,
        .pThreshold         = &OV5647_Threshold_F11,
        .pComponentGain     = &OV5647_CompGain_F11,

        .pSaturationCurve   = &OV5647_SaturationCurve_F11,
        .pCcMatrixTable     = &OV5647_CcMatrixTable_F11,
        .pCcOffsetTable     = &OV5647_CcOffsetTable_F11,

        .pVignettingCurve   = &OV5647_VignettingCurve_F11,
    }
};




/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static RESULT OV5647_IsiCreateSensorIss( IsiSensorInstanceConfig_t *pConfig );
static RESULT OV5647_IsiReleaseSensorIss( IsiSensorHandle_t handle );
static RESULT OV5647_IsiGetCapsIss( IsiSensorHandle_t handle, IsiSensorCaps_t *pIsiSensorCaps );
static RESULT OV5647_IsiSetupSensorIss( IsiSensorHandle_t handle, const IsiSensorConfig_t *pConfig );
static RESULT OV5647_IsiSensorSetStreamingIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV5647_IsiSensorSetPowerIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV5647_IsiCheckSensorConnectionIss( IsiSensorHandle_t handle );
static RESULT OV5647_IsiGetSensorRevisionIss( IsiSensorHandle_t handle, uint32_t *p_value);

static RESULT OV5647_IsiGetGainLimitsIss( IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain);
static RESULT OV5647_IsiGetIntegrationTimeLimitsIss( IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime );
static RESULT OV5647_IsiExposureControlIss( IsiSensorHandle_t handle, float NewGain, float NewIntegrationTime, uint8_t *pNumberOfFramesToSkip, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV5647_IsiGetCurrentExposureIss( IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV5647_IsiGetAfpsInfoIss ( IsiSensorHandle_t handle, uint32_t Resolution, IsiAfpsInfo_t* pAfpsInfo);
static RESULT OV5647_IsiGetGainIss( IsiSensorHandle_t handle, float *pSetGain );
static RESULT OV5647_IsiGetGainIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV5647_IsiSetGainIss( IsiSensorHandle_t handle, float NewGain, float *pSetGain );
static RESULT OV5647_IsiGetIntegrationTimeIss( IsiSensorHandle_t handle, float *pSetIntegrationTime );
static RESULT OV5647_IsiGetIntegrationTimeIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV5647_IsiSetIntegrationTimeIss( IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t *pNumberOfFramesToSkip );
static RESULT OV5647_IsiGetResolutionIss( IsiSensorHandle_t handle, uint32_t *pSetResolution );


static RESULT OV5647_IsiRegReadIss( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );
static RESULT OV5647_IsiRegWriteIss( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

static RESULT OV5647_IsiGetCalibKFactor( IsiSensorHandle_t handle, Isi1x1FloatMatrix_t **pIsiKFactor );
static RESULT OV5647_IsiGetCalibPcaMatrix( IsiSensorHandle_t   handle, Isi3x2FloatMatrix_t **pIsiPcaMatrix );
static RESULT OV5647_IsiGetCalibSvdMeanValue( IsiSensorHandle_t   handle, Isi3x1FloatMatrix_t **pIsiSvdMeanValue );
static RESULT OV5647_IsiGetCalibCenterLine( IsiSensorHandle_t   handle, IsiLine_t  **ptIsiCenterLine);
static RESULT OV5647_IsiGetCalibClipParam( IsiSensorHandle_t   handle, IsiAwbClipParm_t    **pIsiClipParam );
static RESULT OV5647_IsiGetCalibGlobalFadeParam( IsiSensorHandle_t       handle, IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam);
static RESULT OV5647_IsiGetCalibFadeParam( IsiSensorHandle_t   handle, IsiAwbFade2Parm_t   **ptIsiFadeParam);
static RESULT OV5647_IsiGetIlluProfile( IsiSensorHandle_t   handle, const uint32_t CieProfile, IsiIlluProfile_t **ptIsiIlluProfile );

static RESULT OV5647_IsiMdiInitMotoDriveMds( IsiSensorHandle_t handle );
static RESULT OV5647_IsiMdiSetupMotoDrive( IsiSensorHandle_t handle, uint32_t *pMaxStep );
static RESULT OV5647_IsiMdiFocusSet( IsiSensorHandle_t handle, const uint32_t Position );
static RESULT OV5647_IsiMdiFocusGet( IsiSensorHandle_t handle, uint32_t *pAbsStep );
static RESULT OV5647_IsiMdiFocusCalibrate( IsiSensorHandle_t handle );

static RESULT OV5647_IsiGetSensorMipiInfoIss( IsiSensorHandle_t handle, IsiSensorMipiInfo *ptIsiSensorMipiInfo);
static RESULT OV5647_IsiGetSensorIsiVersion(  IsiSensorHandle_t   handle, unsigned int* pVersion);


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
 *          OV5647_IsiCreateSensorIss
 *
 * @brief   This function creates a new OV5647 sensor instance handle.
 *
 * @param   pConfig     configuration structure to create the instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
static RESULT OV5647_IsiCreateSensorIss
(
    IsiSensorInstanceConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    OV5647_Context_t *pOV5647Ctx;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pConfig == NULL) || (pConfig->pSensor ==NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    pOV5647Ctx = ( OV5647_Context_t * )malloc ( sizeof (OV5647_Context_t) );
    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pOV5647Ctx, 0, sizeof( OV5647_Context_t ) );

    result = HalAddRef( pConfig->HalHandle );
    if ( result != RET_SUCCESS )
    {
        free ( pOV5647Ctx );
        return ( result );
    }

    pOV5647Ctx->IsiCtx.HalHandle              = pConfig->HalHandle;
    pOV5647Ctx->IsiCtx.HalDevID               = pConfig->HalDevID;
    pOV5647Ctx->IsiCtx.I2cBusNum              = pConfig->I2cBusNum;
    pOV5647Ctx->IsiCtx.SlaveAddress           = ( pConfig->SlaveAddr == 0 ) ? OV5647_SLAVE_ADDR : pConfig->SlaveAddr;
    pOV5647Ctx->IsiCtx.NrOfAddressBytes       = 2U;

    pOV5647Ctx->IsiCtx.I2cAfBusNum            = pConfig->I2cAfBusNum;
    pOV5647Ctx->IsiCtx.SlaveAfAddress         = ( pConfig->SlaveAfAddr == 0 ) ? OV5647_SLAVE_AF_ADDR : pConfig->SlaveAfAddr;
    pOV5647Ctx->IsiCtx.NrOfAfAddressBytes     = 0;

    pOV5647Ctx->IsiCtx.pSensor                = pConfig->pSensor;

    pOV5647Ctx->Configured             = BOOL_FALSE;
    pOV5647Ctx->Streaming              = BOOL_FALSE;
    pOV5647Ctx->TestPattern            = BOOL_FALSE;
    pOV5647Ctx->isAfpsRun              = BOOL_FALSE;

    pConfig->hSensor = ( IsiSensorHandle_t )pOV5647Ctx;

    result = HalSetCamConfig( pOV5647Ctx->IsiCtx.HalHandle, pOV5647Ctx->IsiCtx.HalDevID, true, true, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    result = HalSetClock( pOV5647Ctx->IsiCtx.HalHandle, pOV5647Ctx->IsiCtx.HalDevID, 10000000U);
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiReleaseSensorIss
 *
 * @brief   This function destroys/releases an OV5647 sensor instance.
 *
 * @param   handle      OV5647 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
static RESULT OV5647_IsiReleaseSensorIss
(
    IsiSensorHandle_t handle
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    (void)OV5647_IsiSensorSetStreamingIss( pOV5647Ctx, BOOL_FALSE );
    (void)OV5647_IsiSensorSetPowerIss( pOV5647Ctx, BOOL_FALSE );

    (void)HalDelRef( pOV5647Ctx->IsiCtx.HalHandle );

    MEMSET( pOV5647Ctx, 0, sizeof( OV5647_Context_t ) );
    free ( pOV5647Ctx );

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetCapsIss
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
static RESULT OV5647_IsiGetCapsIss
(
    IsiSensorHandle_t handle,
    IsiSensorCaps_t   *pIsiSensorCaps
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSensorCaps == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        pIsiSensorCaps->BusWidth        = ISI_BUSWIDTH_10BIT;
        pIsiSensorCaps->Mode            = ISI_MODE_BAYER;
        pIsiSensorCaps->FieldSelection  = ISI_FIELDSEL_BOTH;
        pIsiSensorCaps->YCSequence      = ISI_YCSEQ_YCBYCR;           /**< only Bayer supported, will not be evaluated */
        pIsiSensorCaps->Conv422         = ISI_CONV422_NOCOSITED;
        pIsiSensorCaps->BPat            = ISI_BPAT_RGRGGBGB ;//ISI_BPAT_BGBGGRGR;
        pIsiSensorCaps->HPol            = ISI_HPOL_REFPOS;
        pIsiSensorCaps->VPol            = ISI_VPOL_NEG;
        pIsiSensorCaps->Edge            = ISI_EDGE_FALLING;
        pIsiSensorCaps->Bls             = ISI_BLS_OFF;
        pIsiSensorCaps->Gamma           = ISI_GAMMA_OFF;
        pIsiSensorCaps->CConv           = ISI_CCONV_OFF;

        pIsiSensorCaps->Resolution      = (ISI_RES_1296_972|ISI_RES_2592_1944);

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
        pIsiSensorCaps->MipiMode        = ISI_MIPI_OFF;
        pIsiSensorCaps->AfpsResolutions = ( ISI_AFPS_NOTSUPP );
		pIsiSensorCaps->SensorOutputMode = ISI_SENSOR_OUTPUT_MODE_RAW;
    }

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_g_IsiSensorDefaultConfig
 *
 * @brief   recommended default configuration for application use via call
 *          to IsiGetSensorIss()
 *
 *****************************************************************************/
const IsiSensorCaps_t OV5647_g_IsiSensorDefaultConfig =
{
    ISI_BUSWIDTH_10BIT,         // BusWidth
    ISI_MODE_BAYER,              // MIPI
    ISI_FIELDSEL_BOTH,          // FieldSel
    ISI_YCSEQ_YCBYCR,           // YCSeq
    ISI_CONV422_NOCOSITED,      // Conv422
    ISI_BPAT_RGRGGBGB,//ISI_BPAT_BGBGGRGR,          // BPat
    ISI_HPOL_REFPOS,            // HPol
    ISI_VPOL_NEG,               // VPol
    ISI_EDGE_RISING,            // Edge
    ISI_BLS_OFF,                // Bls
    ISI_GAMMA_OFF,              // Gamma
    ISI_CCONV_OFF,              // CConv
    ISI_RES_1296_972,          // Res
    ISI_DWNSZ_SUBSMPL,          // DwnSz
    ISI_BLC_AUTO,               // BLC
    ISI_AGC_OFF,                // AGC
    ISI_AWB_OFF,                // AWB
    ISI_AEC_OFF,                // AEC
    ISI_DPCC_OFF,               // DPCC
    ISI_CIEPROF_F11,            // CieProfile, this is also used as start profile for AWB (if not altered by menu settings)
    ISI_SMIA_OFF,               // SmiaMode
    ISI_MIPI_OFF,       // MipiMode
    ISI_AFPS_NOTSUPP,           // AfpsResolutions
    ISI_SENSOR_OUTPUT_MODE_RAW,
    0,
};



/*****************************************************************************/
/**
 *          OV5647_SetupOutputFormat
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV5647 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5647_SetupOutputFormat
(
    OV5647_Context_t       *pOV5647Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s%s (enter)\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );

    /* bus-width */
    switch ( pConfig->BusWidth )        /* only ISI_BUSWIDTH_12BIT supported, no configuration needed here */
    {
        case ISI_BUSWIDTH_10BIT:
        {
            break;
        }

        default:
        {
            TRACE( OV5647_ERROR, "%s%s: bus width not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* mode */
    switch ( pConfig->Mode )            /* only ISI_MODE_BAYER supported, no configuration needed here */
    {
        case( ISI_MODE_MIPI ):
        case ISI_MODE_BAYER:
        {
            break;
        }

        default:
        {
            TRACE( OV5647_ERROR, "%s%s: mode not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5647_ERROR, "%s%s: field selection not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* only Bayer mode is supported by OV5647 sensor, so the YCSequence parameter is not checked */
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
            TRACE( OV5647_ERROR, "%s%s: 422 conversion not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* bayer-pattern */
    switch ( pConfig->BPat )            /* only ISI_BPAT_BGBGGRGR supported, no configuration needed */
    {
        case ISI_BPAT_BGBGGRGR:
        case ISI_BPAT_RGRGGBGB:
        {
            break;
        }

        default:
        {
            TRACE( OV5647_ERROR, "%s%s: bayer pattern not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5647_ERROR, "%s%s: HPol not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5647_ERROR, "%s%s: VPol not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5647_ERROR, "%s%s:  edge mode not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5647_ERROR, "%s%s:  gamma not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5647_ERROR, "%s%s: color conversion not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5647_ERROR, "%s%s: SMIA mode not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->MipiMode )        /* only ISI_MIPI_MODE_RAW_12 supported, no configuration needed */
    {
        case ISI_MIPI_MODE_RAW_10:
        case ISI_MIPI_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV5647_ERROR, "%s%s: MIPI mode not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
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
            //TRACE( OV5647_ERROR, "%s%s: AFPS not supported\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"" );
            //return ( RET_NOTSUPP );
        }
    }

    TRACE( OV5647_INFO, "%s%s (exit)\n", __FUNCTION__, pOV5647Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}

int OV5647_get_PCLK( OV5647_Context_t *pOV5647Ctx, int XVCLK)
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
    OV5647_IsiRegReadIss(  pOV5647Ctx, 0x3007, &temp1 );
    Div_cnt5b = temp1>>3;
    temp2 = temp1 & 0x03;
    Pre_Div_sp2x = Pre_Div_sp2x_map[temp2];
    if(temp1 & 0x04) {
        R_div_sp= 2;
    }
    else{
        R_div_sp = 1;
    }

    OV5647_IsiRegReadIss(  pOV5647Ctx, 0x3012, &temp1 );
    Div124_sp = temp1 >> 6;
    div12_sp = (temp1>>4) & 0x03;
    Sdiv_sp = temp1 & 0x0f;
    pll2 = XVCLK * 2 / Pre_Div_sp2x * R_div_sp * (32 - Div_cnt5b ) / (Sdiv_sp + 1);
    
    //temp1 = ReadSCCB(0x6c, 0x3003);
    OV5647_IsiRegReadIss(  pOV5647Ctx, 0x3003, &temp1 );
    temp2 = temp1 & 0x07;
    Pre_div02x = Pre_div02x_map[temp2];
    temp2 = temp1>>6;
    Div124 = Div124_map[temp2];
    
    //temp1 = ReadSCCB(0x6c, 0x3005);
    OV5647_IsiRegReadIss(  pOV5647Ctx, 0x3005, &temp1 );
    Sdiv0 = temp1 & 0x0f;
    
    //temp1 = ReadSCCB(0x6c, 0x3006);
    OV5647_IsiRegReadIss(  pOV5647Ctx, 0x3006, &temp1 );
    Sdiv1 = temp1 & 0x0f;
    temp2 = (temp1>>4) & 0x07;
    R_seld52x = R_seld52x_map[temp2];
    
    //temp1 = ReadSCCB(0x6c, 0x3004);
    OV5647_IsiRegReadIss(  pOV5647Ctx, 0x3004, &temp1 );
    Div_cnt7b = temp1 & 0x7f;
    VCO = XVCLK * 2 / Pre_div02x * Div124 * (129 - Div_cnt7b);
    if(temp1 & 0x80) {
        pllsysclk = pll2 / (Sdiv1 + 1) * 2 / R_seld52x;
    }
    else{
        pllsysclk = VCO / (Sdiv0 +1) / (Sdiv1 + 1) * 2 / R_seld52x;
    }
    
    //temp1 = ReadSCCB(0x6c, 0x3104);
    OV5647_IsiRegReadIss(  pOV5647Ctx, 0x3104, &temp1 );
    if(temp1 & 0x20) {
        PCLK = pllsysclk;
    }
    else{
        PCLK = pll2;
    }
    
    return PCLK;
}

/*****************************************************************************/
/**
 *          OV5647_SetupOutputWindow
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV5647 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_SetupOutputWindow
(
    OV5647_Context_t        *pOV5647Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result     = RET_SUCCESS;

        /* resolution */
    switch ( pConfig->Resolution )
    {
        case ISI_RES_1296_972:
        {
            if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV5647Ctx,OV5647_g_aRegDescription)) != RET_SUCCESS){
                TRACE( OV5647_ERROR, "%s: failed to set  ISI_RES_1296_972 \n", __FUNCTION__ );
            }
            break;
        }
        case ISI_RES_2592_1944:
        {
            if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV5647Ctx,OV5647_g_2592_1944)) != RET_SUCCESS){
                TRACE( OV5647_ERROR, "%s: failed to set  ISI_RES_1296_972 \n", __FUNCTION__ );  
            }

            break;
            
        }
        default:
        {
            TRACE( OV5647_ERROR, "%s: Resolution not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }


    return ( result );
}




/*****************************************************************************/
/**
 *          OV5647_SetupImageControl
 *
 * @brief   Sets the image control functions (BLC, AGC, AWB, AEC, DPCC ...)
 *
 * @param   handle      OV5647 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5647_SetupImageControl
(
    OV5647_Context_t        *pOV5647Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0U;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);


    return ( result );
}


/*****************************************************************************/
/**
 *          OV5647_AecSetModeParameters
 *
 * @brief   This function fills in the correct parameters in OV5647-Instances
 *          according to AEC mode selection in IsiSensorConfig_t.
 *
 * @note    It is assumed that IsiSetupOutputWindow has been called before
 *          to fill in correct values in instance structure.
 *
 * @param   handle      OV5647 context
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_AecSetModeParameters
(
    OV5647_Context_t       *pOV5647Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    return ( result );
}


/*****************************************************************************/
/**
 *          OV5647_IsiSetupSensorIss
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV5647 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiSetupSensorIss
(
    IsiSensorHandle_t       handle,
    const IsiSensorConfig_t *pConfig
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid configuration (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    if ( pOV5647Ctx->Streaming != BOOL_FALSE )
    {
        return RET_WRONG_STATE;
    }

    MEMCPY( &pOV5647Ctx->Config, pConfig, sizeof( IsiSensorConfig_t ) );

    #if 0
    /* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */
    result = IsiRegDefaultsApply( pOV5647Ctx, OV5647_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* sleep a while, that sensor can take over new default values */
    osSleep( 10 );


    /* 3.) verify default values to make sure everything has been written correctly as expected */
    result = IsiRegDefaultsVerify( pOV5647Ctx, OV5647_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    // output of pclk for measurement (only debugging)
    result = OV5647_IsiRegWriteIss( pOV5647Ctx, 0x3009U, 0x10U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    #endif

    /* 4.) setup output format (RAW10|RAW12) */
    result = OV5647_SetupOutputFormat( pOV5647Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5647_ERROR, "%s: SetupOutputFormat failed.\n", __FUNCTION__);
        return ( result );
    }

    /* 5.) setup output window */
    result = OV5647_SetupOutputWindow( pOV5647Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5647_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV5647_SetupImageControl( pOV5647Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5647_ERROR, "%s: SetupImageControl failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV5647_AecSetModeParameters( pOV5647Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5647_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
        return ( result );
    }
    if (result == RET_SUCCESS)
    {
        pOV5647Ctx->Configured = BOOL_TRUE;
    }

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiChangeSensorResolutionIss
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
static RESULT OV5647_IsiChangeSensorResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (pNumberOfFramesToSkip == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pOV5647Ctx->Configured != BOOL_TRUE) || (pOV5647Ctx->Streaming != BOOL_FALSE) )
    {
        return RET_WRONG_STATE;
    }

    IsiSensorCaps_t Caps;
    result = OV5647_IsiGetCapsIss( handle, &Caps);
    if (RET_SUCCESS != result)
    {
        return result;
    }

    if ( (Resolution & Caps.Resolution) == 0 )
    {
        return RET_OUTOFRANGE;
    }

    if ( Resolution == pOV5647Ctx->Config.Resolution )
    {
        // well, no need to worry
        *pNumberOfFramesToSkip = 0;
    }
    else
    {
        // change resolution
        char *szResName = NULL;
        result = IsiGetResolutionName( Resolution, &szResName );
        TRACE( OV5647_DEBUG, "%s: NewRes=0x%08x (%s)\n", __FUNCTION__, Resolution, szResName);

        // update resolution in copy of config in context
        pOV5647Ctx->Config.Resolution = Resolution;

        // tell sensor about that
        result = OV5647_SetupOutputWindow( pOV5647Ctx, &pOV5647Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV5647_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
            return ( result );
        }

        // remember old exposure values
        float OldGain = pOV5647Ctx->AecCurGain;
        float OldIntegrationTime = pOV5647Ctx->AecCurIntegrationTime;

        // update limits & stuff (reset current & old settings)
        result = OV5647_AecSetModeParameters( pOV5647Ctx, &pOV5647Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV5647_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
            return ( result );
        }

        // restore old exposure values (at least within new exposure values' limits)
        uint8_t NumberOfFramesToSkip;
        float   DummySetGain;
        float   DummySetIntegrationTime;
        result = OV5647_IsiExposureControlIss( handle, OldGain, OldIntegrationTime, &NumberOfFramesToSkip, &DummySetGain, &DummySetIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV5647_ERROR, "%s: OV5647_IsiExposureControlIss failed.\n", __FUNCTION__);
            return ( result );
        }

        // return number of frames that aren't exposed correctly
        *pNumberOfFramesToSkip = NumberOfFramesToSkip + 1;
    }

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiSensorSetStreamingIss
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
static RESULT OV5647_IsiSensorSetStreamingIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    uint32_t RegValue = 0;

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pOV5647Ctx->Configured != BOOL_TRUE) || (pOV5647Ctx->Streaming == on) )
    {
        return RET_WRONG_STATE;
    }

    if (on == BOOL_TRUE)
    {
        /* enable streaming */
        result = OV5647_IsiRegWriteIss( handle, 0x0100, 0x1);

    }
    else
    {
        /* disable streaming */
        result = OV5647_IsiRegWriteIss( handle, 0x0100, 0x0);


    }

    if (result == RET_SUCCESS)
    {
        pOV5647Ctx->Streaming = on;
    }

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiSensorSetPowerIss
 *
 * @brief   Performs the power-up/power-down sequence of the camera, if possible.
 *
 * @param   handle      OV5647 sensor instance handle
 * @param   on          new power state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiSensorSetPowerIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pOV5647Ctx->Configured = BOOL_FALSE;
    pOV5647Ctx->Streaming  = BOOL_FALSE;

    TRACE( OV5647_DEBUG, "%s power off \n", __FUNCTION__);
    result = HalSetPower( pOV5647Ctx->IsiCtx.HalHandle, pOV5647Ctx->IsiCtx.HalDevID, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV5647_DEBUG, "%s reset on\n", __FUNCTION__);
    result = HalSetReset( pOV5647Ctx->IsiCtx.HalHandle, pOV5647Ctx->IsiCtx.HalDevID, true );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    if (on == BOOL_TRUE)
    {
        TRACE( OV5647_DEBUG, "%s power on \n", __FUNCTION__);
        result = HalSetPower( pOV5647Ctx->IsiCtx.HalHandle, pOV5647Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV5647_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pOV5647Ctx->IsiCtx.HalHandle, pOV5647Ctx->IsiCtx.HalDevID, false );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV5647_DEBUG, "%s reset on \n", __FUNCTION__);
        result = HalSetReset( pOV5647Ctx->IsiCtx.HalHandle, pOV5647Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV5647_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pOV5647Ctx->IsiCtx.HalHandle, pOV5647Ctx->IsiCtx.HalDevID, false );

        osSleep( 50 );
    }

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiCheckSensorConnectionIss
 *
 * @brief   Checks the I2C-Connection to sensor by reading sensor revision id.
 *
 * @param   handle      OV5647 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiCheckSensorConnectionIss
(
    IsiSensorHandle_t   handle
)
{
    uint32_t RevId;
    uint32_t value;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    RevId = OV5647_CHIP_ID_HIGH_BYTE_DEFAULT;
    RevId = (RevId << 16U) | (OV5647_CHIP_ID_MIDDLE_BYTE_DEFAULT<<8U);
    RevId = RevId | OV5647_CHIP_ID_LOW_BYTE_DEFAULT;

    result = OV5647_IsiGetSensorRevisionIss( handle, &value );
    if ( (result != RET_SUCCESS) || (RevId != value) )
    {
        TRACE( OV5647_ERROR, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );
        return ( RET_FAILURE );
    }

    TRACE( OV5647_DEBUG, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetSensorRevisionIss
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
static RESULT OV5647_IsiGetSensorRevisionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    uint32_t data;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *p_value = 0U;
    result = OV5647_IsiRegReadIss ( handle, OV5647_CHIP_ID_HIGH_BYTE, &data );
    *p_value = ( (data & 0xFF) << 16U );
    result = OV5647_IsiRegReadIss ( handle, OV5647_CHIP_ID_MIDDLE_BYTE, &data );
    *p_value |= ( (data & 0xFF) << 8U );
    result = OV5647_IsiRegReadIss ( handle, OV5647_CHIP_ID_LOW_BYTE, &data );
    *p_value |= ( (data & 0xFF));

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiRegReadIss
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
static RESULT OV5647_IsiRegReadIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

  //  TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

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
        uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, OV5647_g_aRegDescription );
        if ( !NrOfBytes )
        {
            NrOfBytes = 1;
        }
 //       TRACE( OV5647_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);

        *p_value = 0;
        result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
    }

  //  TRACE( OV5647_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiRegWriteIss
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
static RESULT OV5647_IsiRegWriteIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NrOfBytes;

  //  TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    NrOfBytes = IsiGetNrDatBytesIss( address, OV5647_g_aRegDescription );
    if ( !NrOfBytes )
    {
        NrOfBytes = 1;
    }
//    TRACE( OV5647_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x 0x%08x)\n", __FUNCTION__, NrOfBytes, address, value);

    result = IsiI2cWriteSensorRegister( handle, address, (uint8_t *)(&value), NrOfBytes, BOOL_TRUE );

//    TRACE( OV5647_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetGainLimitsIss
 *
 * @brief   Returns the exposure minimal and maximal values of an
 *          OV5647 instance
 *
 * @param   handle       OV5647 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetGainLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinGain,
    float               *pMaxGain
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinGain == NULL) || (pMaxGain == NULL) )
    {
        TRACE( OV5647_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinGain = pOV5647Ctx->AecMinGain;
    *pMaxGain = pOV5647Ctx->AecMaxGain;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetIntegrationTimeLimitsIss
 *
 * @brief   Returns the minimal and maximal integration time values of an
 *          OV5647 instance
 *
 * @param   handle       OV5647 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetIntegrationTimeLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinIntegrationTime,
    float               *pMaxIntegrationTime
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL) )
    {
        TRACE( OV5647_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinIntegrationTime = pOV5647Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pOV5647Ctx->AecMaxIntegrationTime;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV5647_IsiGetGainIss
 *
 * @brief   Reads gain values from the image sensor module.
 *
 * @param   handle                  OV5647 sensor instance handle
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5647_IsiGetGainIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain = pOV5647Ctx->AecCurGain;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetGainIncrementIss
 *
 * @brief   Get smallest possible gain increment.
 *
 * @param   handle                  OV5647 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5647_IsiGetGainIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV5647Ctx->AecGainIncrement;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiSetGainIss
 *
 * @brief   Writes gain values to the image sensor module.
 *          Updates current gain and exposure in sensor struct/state.
 *
 * @param   handle                  OV5647 sensor instance handle
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
RESULT OV5647_IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint16_t usGain = 0;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        TRACE( OV5647_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

  
    if( NewGain < pOV5647Ctx->AecMinGain ) NewGain = pOV5647Ctx->AecMinGain;
    if( NewGain > pOV5647Ctx->AecMaxGain ) NewGain = pOV5647Ctx->AecMaxGain;

    usGain = (uint16_t)NewGain;

    // write new gain into sensor registers, do not write if nothing has changed
    if( (usGain != pOV5647Ctx->OldGain) )
    {

        pOV5647Ctx->OldGain = usGain;
    }

    //calculate gain actually set
    pOV5647Ctx->AecCurGain = NewGain;

    //return current state
    *pSetGain = pOV5647Ctx->AecCurGain;
    TRACE( OV5647_DEBUG, "%s: g=%f\n", __FUNCTION__, *pSetGain );

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetIntegrationTimeIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  OV5647 sensor instance handle
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5647_IsiGetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               *pSetIntegrationTime
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetIntegrationTime == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetIntegrationTime = pOV5647Ctx->AecCurIntegrationTime;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetIntegrationTimeIncrementIss
 *
 * @brief   Get smallest possible integration time increment.
 *
 * @param   handle                  OV5647 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5647_IsiGetIntegrationTimeIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV5647Ctx->AecIntegrationTimeIncrement;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiSetIntegrationTimeIss
 *
 * @brief   Writes gain and integration time values to the image sensor module.
 *          Updates current integration time and exposure in sensor
 *          struct/state.
 *
 * @param   handle                  OV5647 sensor instance handle
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
RESULT OV5647_IsiSetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               NewIntegrationTime,
    float               *pSetIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t CoarseIntegrationTime = 0;
    //uint32_t FineIntegrationTime   = 0; //not supported by OV5647

    float ShutterWidthPck = 0.0f; //shutter width in pixel clock periods

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetIntegrationTime == NULL) || (pNumberOfFramesToSkip == NULL) )
    {
        TRACE( OV5647_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    //maximum and minimum integration time is limited by the sensor, if this limit is not
    //considered, the exposure control loop needs lots of time to return to a new state
    //so limit to allowed range
    if ( NewIntegrationTime > pOV5647Ctx->AecMaxIntegrationTime ) NewIntegrationTime = pOV5647Ctx->AecMaxIntegrationTime;
    if ( NewIntegrationTime < pOV5647Ctx->AecMinIntegrationTime ) NewIntegrationTime = pOV5647Ctx->AecMinIntegrationTime;

    //the actual integration time is given by
    //integration_time = ( coarse_integration_time * line_length_pck + fine_integration_time ) / vt_pix_clk_freq
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck )
    //fine_integration_time   = integration_time * vt_pix_clk_freq - coarse_integration_time * line_length_pck
    //
    //fine integration is not supported by OV5647
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck + 0.5 )

    ShutterWidthPck = NewIntegrationTime * ( (float)pOV5647Ctx->VtPixClkFreq );

    // avoid division by zero
    if ( pOV5647Ctx->LineLengthPck == 0 )
    {
        TRACE( OV5647_ERROR, "%s: Division by zero!\n", __FUNCTION__ );
        return ( RET_DIVISION_BY_ZERO );
    }

    //calculate the integer part of the integration time in units of line length
    //calculate the fractional part of the integration time in units of pixel clocks
    //CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV5647Ctx->LineLengthPck) );
    //FineIntegrationTime   = ( (uint32_t)ShutterWidthPck ) - ( CoarseIntegrationTime * pOV5647Ctx->LineLengthPck );
    CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV5647Ctx->LineLengthPck) + 0.5f );

    // write new integration time into sensor registers
    // do not write if nothing has changed
    if( CoarseIntegrationTime != pOV5647Ctx->OldCoarseIntegrationTime )
    {

        pOV5647Ctx->OldCoarseIntegrationTime = CoarseIntegrationTime;   // remember current integration time
        *pNumberOfFramesToSkip = 1U; //skip 1 frame
    }
    else
    {
        *pNumberOfFramesToSkip = 0U; //no frame skip
    }

    //if( FineIntegrationTime != pOV5647Ctx->OldFineIntegrationTime )
    //{
    //    result = OV5647_IsiRegWriteIss( pOV5647Ctx, ... , FineIntegrationTime );
    //    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    //    pOV5647Ctx->OldFineIntegrationTime = FineIntegrationTime; //remember current integration time
    //    *pNumberOfFramesToSkip = 1U; //skip 1 frame
    //}

    //calculate integration time actually set
    //pOV5647Ctx->AecCurIntegrationTime = ( ((float)CoarseIntegrationTime) * ((float)pOV5647Ctx->LineLengthPck) + ((float)FineIntegrationTime) ) / pOV5647Ctx->VtPixClkFreq;
    pOV5647Ctx->AecCurIntegrationTime = ((float)CoarseIntegrationTime) * ((float)pOV5647Ctx->LineLengthPck) / pOV5647Ctx->VtPixClkFreq;

    //return current state
    *pSetIntegrationTime = pOV5647Ctx->AecCurIntegrationTime;

    TRACE( OV5647_DEBUG, "%s: Ti=%f\n", __FUNCTION__, *pSetIntegrationTime );
    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}




/*****************************************************************************/
/**
 *          OV5647_IsiExposureControlIss
 *
 * @brief   Camera hardware dependent part of the exposure control loop.
 *          Calculates appropriate register settings from the new exposure
 *          values and writes them to the image sensor module.
 *
 * @param   handle                  OV5647 sensor instance handle
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
RESULT OV5647_IsiExposureControlIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               NewIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pNumberOfFramesToSkip == NULL)
            || (pSetGain == NULL)
            || (pSetIntegrationTime == NULL) )
    {
        TRACE( OV5647_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    TRACE( OV5647_DEBUG, "%s: g=%f, Ti=%f\n", __FUNCTION__, NewGain, NewIntegrationTime );


    result = OV5647_IsiSetIntegrationTimeIss( handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip );
    result = OV5647_IsiSetGainIss( handle, NewGain, pSetGain );

    TRACE( OV5647_DEBUG, "%s: set: g=%f, Ti=%f, skip=%d\n", __FUNCTION__, *pSetGain, *pSetIntegrationTime, *pNumberOfFramesToSkip );
    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetCurrentExposureIss
 *
 * @brief   Returns the currently adjusted AE values
 *
 * @param   handle                  OV5647 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5647_IsiGetCurrentExposureIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetGain == NULL) || (pSetIntegrationTime == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain            = pOV5647Ctx->AecCurGain;
    *pSetIntegrationTime = pOV5647Ctx->AecCurIntegrationTime;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetResolutionIss
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
RESULT OV5647_IsiGetResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *pSetResolution
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetResolution == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetResolution = pOV5647Ctx->Config.Resolution;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetAfpsInfoHelperIss
 *
 * @brief   Calc AFPS sub resolution settings for the given resolution
 *
 * @param   pOV5647Ctx             OV5647 sensor instance (dummy!) context
 * @param   Resolution              Any supported resolution to query AFPS params for
 * @param   pAfpsInfo               Reference of AFPS info structure to write the results to
 * @param   AfpsStageIdx            Index of current AFPS stage to use
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetAfpsInfoHelperIss(
    OV5647_Context_t   *pOV5647Ctx,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo,
    uint32_t            AfpsStageIdx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT(pOV5647Ctx != NULL);
    DCT_ASSERT(pAfpsInfo != NULL);
    DCT_ASSERT(AfpsStageIdx <= ISI_NUM_AFPS_STAGES);

    // update resolution in copy of config in context
    pOV5647Ctx->Config.Resolution = Resolution;

    // tell sensor about that
    result = OV5647_SetupOutputWindow( pOV5647Ctx, &pOV5647Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5647_ERROR, "%s: SetupOutputWindow failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // update limits & stuff (reset current & old settings)
    result = OV5647_AecSetModeParameters( pOV5647Ctx, &pOV5647Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5647_ERROR, "%s: AecSetModeParameters failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // take over params
    pAfpsInfo->Stage[AfpsStageIdx].Resolution = Resolution;
    pAfpsInfo->Stage[AfpsStageIdx].MaxIntTime = pOV5647Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecMinGain           = pOV5647Ctx->AecMinGain;
    pAfpsInfo->AecMaxGain           = pOV5647Ctx->AecMaxGain;
    pAfpsInfo->AecMinIntTime        = pOV5647Ctx->AecMinIntegrationTime;
    pAfpsInfo->AecMaxIntTime        = pOV5647Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecSlowestResolution = Resolution;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV5647_IsiGetAfpsInfoIss
 *
 * @brief   Returns the possible AFPS sub resolution settings for the given resolution series
 *
 * @param   handle                  OV5647 sensor instance handle
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
RESULT OV5647_IsiGetAfpsInfoIss(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        TRACE( OV5647_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pAfpsInfo == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    // use currently configured resolution?
    if (Resolution == 0)
    {
        Resolution = pOV5647Ctx->Config.Resolution;
    }

    // prepare index
    uint32_t idx = 0;

    // set current resolution data in info struct
    pAfpsInfo->CurrResolution = pOV5647Ctx->Config.Resolution;
    pAfpsInfo->CurrMinIntTime = pOV5647Ctx->AecMinIntegrationTime;
    pAfpsInfo->CurrMaxIntTime = pOV5647Ctx->AecMaxIntegrationTime;

    // allocate dummy context used for Afps parameter calculation as a copy of current context
    OV5647_Context_t *pDummyCtx = (OV5647_Context_t*) malloc( sizeof(OV5647_Context_t) );
    if ( pDummyCtx == NULL )
    {
        TRACE( OV5647_ERROR,  "%s: Can't allocate dummy ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    *pDummyCtx = *pOV5647Ctx;

    // set AFPS mode in dummy context
    pDummyCtx->isAfpsRun = BOOL_TRUE;

#define AFPSCHECKANDADD(_res_) \
    if ( (pOV5647Ctx->Config.AfpsResolutions & (_res_)) != 0 ) \
    { \
        RESULT lres = OV5647_IsiGetAfpsInfoHelperIss( pDummyCtx, _res_, pAfpsInfo, idx ); \
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
            TRACE( OV5647_DEBUG,  "%s: Resolution %08x not supported by AFPS\n",  __FUNCTION__, Resolution );
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

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetCalibKFactor
 *
 * @brief   Returns the OV5647 specific K-Factor
 *
 * @param   handle       OV5647 sensor instance handle
 * @param   pIsiKFactor  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetCalibKFactor
(
    IsiSensorHandle_t   handle,
    Isi1x1FloatMatrix_t **pIsiKFactor
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiKFactor == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiKFactor = (Isi1x1FloatMatrix_t *)&OV5647_KFactor;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV5647_IsiGetCalibPcaMatrix
 *
 * @brief   Returns the OV5647 specific PCA-Matrix
 *
 * @param   handle          OV5647 sensor instance handle
 * @param   pIsiPcaMatrix   Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetCalibPcaMatrix
(
    IsiSensorHandle_t   handle,
    Isi3x2FloatMatrix_t **pIsiPcaMatrix
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiPcaMatrix == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiPcaMatrix = (Isi3x2FloatMatrix_t *)&OV5647_PCAMatrix;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns the sensor specific SvdMean-Vector
 *
 * @param   handle              OV5647 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetCalibSvdMeanValue
(
    IsiSensorHandle_t   handle,
    Isi3x1FloatMatrix_t **pIsiSvdMeanValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSvdMeanValue == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiSvdMeanValue = (Isi3x1FloatMatrix_t *)&OV5647_SVDMeanValue;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns a pointer to the sensor specific centerline, a straight
 *          line in Hesse normal form in Rg/Bg colorspace
 *
 * @param   handle              OV5647 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetCalibCenterLine
(
    IsiSensorHandle_t   handle,
    IsiLine_t           **ptIsiCenterLine
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiCenterLine == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiCenterLine = (IsiLine_t*)&OV5647_CenterLine;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetCalibClipParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for Rg/Bg color
 *          space clipping
 *
 * @param   handle              OV5647 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetCalibClipParam
(
    IsiSensorHandle_t   handle,
    IsiAwbClipParm_t    **pIsiClipParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiClipParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiClipParam = (IsiAwbClipParm_t *)&OV5647_AwbClipParm;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetCalibGlobalFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle              OV5647 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetCalibGlobalFadeParam
(
    IsiSensorHandle_t       handle,
    IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiGlobalFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiGlobalFadeParam = (IsiAwbGlobalFadeParm_t *)&OV5647_AwbGlobalFadeParm;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetCalibFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for near white
 *          pixel parameter calculations
 *
 * @param   handle              OV5647 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiGetCalibFadeParam
(
    IsiSensorHandle_t   handle,
    IsiAwbFade2Parm_t   **ptIsiFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiFadeParam = (IsiAwbFade2Parm_t *)&OV5647_AwbFade2Parm;

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV5647_IsiGetIlluProfile
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
static RESULT OV5647_IsiGetIlluProfile
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiIlluProfile_t    **ptIsiIlluProfile
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
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
        for ( i=0U; i<OV5647_ISIILLUPROFILES_DEFAULT; i++ )
        {
            if ( OV5647_IlluProfileDefault[i].id == CieProfile )
            {
                *ptIsiIlluProfile = &OV5647_IlluProfileDefault[i];
                break;
            }
        }

        result = ( *ptIsiIlluProfile != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
    }

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetLscMatrixTable
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
static RESULT OV5647_IsiGetLscMatrixTable
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiLscMatrixTable_t **pLscMatrixTable
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
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
                if ( ( pOV5647Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_A_1920x1080;
                }
                #if 0
                else if ( pOV5647Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_A_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV5647_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F2:
            {
                if ( ( pOV5647Ctx->Config.Resolution == ISI_RES_TV1080P30 ) )
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_F2_1920x1080;
                }
                #if 0
                else if ( pOV5647Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_F2_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV5647_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D50:
            {
                if ( ( pOV5647Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_D50_1920x1080;
                }
                #if 0
                else if ( pOV5647Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_D50_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV5647_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D65:
            case ISI_CIEPROF_D75:
            {
                if ( ( pOV5647Ctx->Config.Resolution == ISI_RES_TV1080P30 ) )
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_D65_1920x1080;
                }
                #if 0
                else if ( pOV5647Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_D65_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV5647_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F11:
            {
                if ( ( pOV5647Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_F11_1920x1080;
                }
                #if 0
                else if ( pOV5647Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV5647_LscMatrixTable_CIE_F11_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV5647_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            default:
            {
                TRACE( OV5647_ERROR, "%s: Illumination not supported\n", __FUNCTION__ );
                *pLscMatrixTable = NULL;
                break;
            }
        }

        result = ( *pLscMatrixTable != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
    }

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV5647_IsiMdiInitMotoDriveMds
 *
 * @brief   General initialisation tasks like I/O initialisation.
 *
 * @param   handle              OV5647 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiMdiInitMotoDriveMds
(
    IsiSensorHandle_t   handle
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiMdiSetupMotoDrive
 *
 * @brief   Setup of the MotoDrive and return possible max step.
 *
 * @param   handle          OV5647 sensor instance handle
 *          pMaxStep        pointer to variable to receive the maximum
 *                          possible focus step
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiMdiSetupMotoDrive
(
    IsiSensorHandle_t   handle,
    uint32_t            *pMaxStep
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMaxStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pMaxStep = MAX_LOG;

    result = OV5647_IsiMdiFocusSet( handle, MAX_LOG );

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiMdiFocusSet
 *
 * @brief   Drives the lens system to a certain focus point.
 *
 * @param   handle          OV5647 sensor instance handle
 *          AbsStep         absolute focus point to apply
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiMdiFocusSet
(
    IsiSensorHandle_t   handle,
    const uint32_t      Position
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t nPosition;
    uint8_t  data[2] = { 0, 0 };

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* map 64 to 0 -> infinity */
    nPosition = ( Position >= MAX_LOG ) ? 0 : ( MAX_REG - (Position * 16U) );

    data[0] = (uint8_t)(0x40U | (( nPosition & 0x3F0U ) >> 4U));                 // PD,  1, D9..D4, see AD5820 datasheet
    data[1] = (uint8_t)( ((nPosition & 0x0FU) << 4U) | MDI_SLEW_RATE_CTRL );    // D3..D0, S3..S0

    TRACE( OV5647_DEBUG, "%s: value = %d, 0x%02x 0x%02x\n", __FUNCTION__, nPosition, data[0], data[1] );

    result = HalWriteI2CMem( pOV5647Ctx->IsiCtx.HalHandle,
                             pOV5647Ctx->IsiCtx.I2cAfBusNum,
                             pOV5647Ctx->IsiCtx.SlaveAfAddress,
                             0,
                             pOV5647Ctx->IsiCtx.NrOfAfAddressBytes,
                             data,
                             2U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiMdiFocusGet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          OV5647 sensor instance handle
 *          pAbsStep        pointer to a variable to receive the current
 *                          focus point
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiMdiFocusGet
(
    IsiSensorHandle_t   handle,
    uint32_t            *pAbsStep
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;
    uint8_t  data[2] = { 0, 0 };

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pAbsStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    result = HalReadI2CMem( pOV5647Ctx->IsiCtx.HalHandle,
                            pOV5647Ctx->IsiCtx.I2cAfBusNum,
                            pOV5647Ctx->IsiCtx.SlaveAfAddress,
                            0,
                            pOV5647Ctx->IsiCtx.NrOfAfAddressBytes,
                            data,
                            2U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV5647_DEBUG, "%s: value = 0x%02x 0x%02x\n", __FUNCTION__, data[0], data[1] );

    /* Data[0] = PD,  1, D9..D4, see AD5820 datasheet */
    /* Data[1] = D3..D0, S3..S0 */
    *pAbsStep = ( ((uint32_t)(data[0] & 0x3FU)) << 4U ) | ( ((uint32_t)data[1]) >> 4U );

    /* map 0 to 64 -> infinity */
    if( *pAbsStep == 0 )
    {
        *pAbsStep = MAX_LOG;
    }
    else
    {
        *pAbsStep = ( MAX_REG - *pAbsStep ) / 16U;
    }

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiMdiFocusCalibrate
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV5647 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5647_IsiMdiFocusCalibrate
(
    IsiSensorHandle_t   handle
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiActivateTestPattern
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV5647 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV5647_IsiActivateTestPattern
(
    IsiSensorHandle_t   handle,
    const bool_t        enable
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5647_IsiGetSensorMipiInfoIss
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV5647 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV5647_IsiGetSensorMipiInfoIss
(
    IsiSensorHandle_t   handle,
    IsiSensorMipiInfo   *ptIsiSensorMipiInfo
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }


    if ( ptIsiSensorMipiInfo == NULL )
    {
        return ( result );
    }

    TRACE( OV5647_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV5647_IsiGetSensorIsiVersion
(  IsiSensorHandle_t   handle,
   unsigned int*     pVersion
)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV5647_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5647Ctx == NULL )
    {
    	TRACE( OV5647_ERROR, "%s: pOV5647Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pVersion == NULL)
	{
		TRACE( OV5647_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pVersion = CONFIG_ISI_VERSION;
	return result;
}

/*****************************************************************************/
/**
 *          OV5647_IsiGetSensorIss
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
RESULT OV5647_IsiGetSensorIss
(
    IsiSensor_t *pIsiSensor
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV5647_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pIsiSensor != NULL )
    {
        pIsiSensor->pszName                             = OV5647_g_acName;
        pIsiSensor->pRegisterTable                      = OV5647_g_aRegDescription;
        pIsiSensor->pIsiSensorCaps                      = &OV5647_g_IsiSensorDefaultConfig;
		pIsiSensor->pIsiGetSensorIsiVer					= OV5647_IsiGetSensorIsiVersion;
		
        pIsiSensor->pIsiCreateSensorIss                 = OV5647_IsiCreateSensorIss;
        pIsiSensor->pIsiReleaseSensorIss                = OV5647_IsiReleaseSensorIss;
        pIsiSensor->pIsiGetCapsIss                      = OV5647_IsiGetCapsIss;
        pIsiSensor->pIsiSetupSensorIss                  = OV5647_IsiSetupSensorIss;
        pIsiSensor->pIsiChangeSensorResolutionIss       = OV5647_IsiChangeSensorResolutionIss;
        pIsiSensor->pIsiSensorSetStreamingIss           = OV5647_IsiSensorSetStreamingIss;
        pIsiSensor->pIsiSensorSetPowerIss               = OV5647_IsiSensorSetPowerIss;
        pIsiSensor->pIsiCheckSensorConnectionIss        = OV5647_IsiCheckSensorConnectionIss;
        pIsiSensor->pIsiGetSensorRevisionIss            = OV5647_IsiGetSensorRevisionIss;
        pIsiSensor->pIsiRegisterReadIss                 = OV5647_IsiRegReadIss;
        pIsiSensor->pIsiRegisterWriteIss                = OV5647_IsiRegWriteIss;

        /* AEC functions */
        pIsiSensor->pIsiExposureControlIss              = OV5647_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = OV5647_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = OV5647_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetCurrentExposureIss           = OV5647_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetGainIss                      = OV5647_IsiGetGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = OV5647_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = OV5647_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = OV5647_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = OV5647_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = OV5647_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetResolutionIss                = OV5647_IsiGetResolutionIss;
        pIsiSensor->pIsiGetAfpsInfoIss                  = OV5647_IsiGetAfpsInfoIss;

        /* AWB specific functions */
        pIsiSensor->pIsiGetCalibKFactor                 = OV5647_IsiGetCalibKFactor;
        pIsiSensor->pIsiGetCalibPcaMatrix               = OV5647_IsiGetCalibPcaMatrix;
        pIsiSensor->pIsiGetCalibSvdMeanValue            = OV5647_IsiGetCalibSvdMeanValue;
        pIsiSensor->pIsiGetCalibCenterLine              = OV5647_IsiGetCalibCenterLine;
        pIsiSensor->pIsiGetCalibClipParam               = OV5647_IsiGetCalibClipParam;
        pIsiSensor->pIsiGetCalibGlobalFadeParam         = OV5647_IsiGetCalibGlobalFadeParam;
        pIsiSensor->pIsiGetCalibFadeParam               = OV5647_IsiGetCalibFadeParam;
        pIsiSensor->pIsiGetIlluProfile                  = OV5647_IsiGetIlluProfile;
        pIsiSensor->pIsiGetLscMatrixTable               = OV5647_IsiGetLscMatrixTable;

        /* AF functions */
        pIsiSensor->pIsiMdiInitMotoDriveMds             = OV5647_IsiMdiInitMotoDriveMds;
        pIsiSensor->pIsiMdiSetupMotoDrive               = OV5647_IsiMdiSetupMotoDrive;
        pIsiSensor->pIsiMdiFocusSet                     = OV5647_IsiMdiFocusSet;
        pIsiSensor->pIsiMdiFocusGet                     = OV5647_IsiMdiFocusGet;
        pIsiSensor->pIsiMdiFocusCalibrate               = OV5647_IsiMdiFocusCalibrate;

        /* MIPI */
        pIsiSensor->pIsiGetSensorMipiInfoIss            = OV5647_IsiGetSensorMipiInfoIss;

        /* Testpattern */
        pIsiSensor->pIsiActivateTestPattern             = OV5647_IsiActivateTestPattern;
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( OV5647_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV5647_IsiGetSensorI2cInfo(sensor_i2c_info_t** pdata)
{
    sensor_i2c_info_t* pSensorI2cInfo;

    pSensorI2cInfo = ( sensor_i2c_info_t * )malloc ( sizeof (sensor_i2c_info_t) );

    if ( pSensorI2cInfo == NULL )
    {
        TRACE( OV5647_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pSensorI2cInfo, 0, sizeof( sensor_i2c_info_t ) );

    
    pSensorI2cInfo->i2c_addr = OV5647_SLAVE_ADDR;
    pSensorI2cInfo->soft_reg_addr = OV5647_SOFTWARE_RST;
    pSensorI2cInfo->soft_reg_value = 0x01;
    pSensorI2cInfo->reg_size = 2;
    pSensorI2cInfo->value_size = 1;

    pSensorI2cInfo->resolution = ( ISI_RES_TV720P15  );
    
    ListInit(&pSensorI2cInfo->chipid_info);

    sensor_chipid_info_t* pChipIDInfo_H = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_H )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_H, 0, sizeof(*pChipIDInfo_H) );    
    pChipIDInfo_H->chipid_reg_addr = OV5647_CHIP_ID_HIGH_BYTE;  
    pChipIDInfo_H->chipid_reg_value = OV5647_CHIP_ID_HIGH_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_H );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_H );

    sensor_chipid_info_t* pChipIDInfo_M = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_M )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_M, 0, sizeof(*pChipIDInfo_M) ); 
    pChipIDInfo_M->chipid_reg_addr = OV5647_CHIP_ID_MIDDLE_BYTE;
    pChipIDInfo_M->chipid_reg_value = OV5647_CHIP_ID_MIDDLE_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_M );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_M );
    
    sensor_chipid_info_t* pChipIDInfo_L = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_L )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_L, 0, sizeof(*pChipIDInfo_L) ); 
    pChipIDInfo_L->chipid_reg_addr = OV5647_CHIP_ID_LOW_BYTE;
    pChipIDInfo_L->chipid_reg_value = OV5647_CHIP_ID_LOW_BYTE_DEFAULT;
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
    OV5647_IsiGetSensorIss,
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
    OV5647_IsiGetSensorI2cInfo,
};



