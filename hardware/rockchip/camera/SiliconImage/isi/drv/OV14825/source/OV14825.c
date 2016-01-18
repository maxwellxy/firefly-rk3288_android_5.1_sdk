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
 * @file OV14825.c
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
#include <common/list.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"

#include "OV14825_MIPI_priv.h"

#define  OV14825_NEWEST_TUNING_XML "13-Mar-2012_AN_OV14825_sample_01_v1.0"

#define CC_OFFSET_SCALING  2.0f
#define I2C_COMPLIANT_STARTBIT 1U

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( OV14825_INFO , "OV14825: ", INFO,    0U );
CREATE_TRACER( OV14825_WARN , "OV14825: ", WARNING, 1U );
CREATE_TRACER( OV14825_ERROR, "OV14825: ", ERROR,   1U );

CREATE_TRACER( OV14825_DEBUG, "OV14825: ", INFO,     0U );

CREATE_TRACER( OV14825_REG_INFO , "OV14825: ", INFO, 0);
CREATE_TRACER( OV14825_REG_DEBUG, "OV14825: ", INFO, 0U );

#define OV14825_SLAVE_ADDR       0x36U                           /**< i2c slave address of the OV14825 camera sensor */
#define OV14825_SLAVE_AF_ADDR    0x0CU                           /**< i2c slave address of the OV14825 integrated AD5820 */

#define OV14825_MIN_GAIN_STEP   ( 16.0f / 325.0f ); /**< min gain step size used by GUI ( 32/(32-7) - 32/(32-6); min. reg value is 6 as of datasheet; depending on actual gain ) */
#define OV14825_MAX_GAIN_AEC    ( 8.0f )            /**< max. gain used by the AEC (arbitrarily chosen, recommended by Omnivision) */


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
const char OV14825_g_acName[] = "OV14825_MIPI";
extern const IsiRegDescription_t OV14825_g_aRegDescription[];
const IsiSensorCaps_t OV14825_g_IsiSensorDefaultConfig;

/* AWB specific value (from OV14825_tables.c) */
extern const Isi1x1FloatMatrix_t    OV14825_KFactor;
extern const Isi3x2FloatMatrix_t    OV14825_PCAMatrix;
extern const Isi3x1FloatMatrix_t    OV14825_SVDMeanValue;
extern const IsiLine_t              OV14825_CenterLine;
extern const IsiAwbClipParm_t       OV14825_AwbClipParm;
extern const IsiAwbGlobalFadeParm_t OV14825_AwbGlobalFadeParm;
extern const IsiAwbFade2Parm_t      OV14825_AwbFade2Parm;

/* illumination profiles */
#include "OV14825_MIPI_a.h"       /* CIE A - default profile */
#include "OV14825_MIPI_f2.h"      /* CIE F2 (cool white flourescent CWF) */
#include "OV14825_MIPI_d50.h"     /* CIE D50 (D50 lightbox) */
#include "OV14825_MIPI_d65.h"     /* CIE D65 (D65) note: indoor because of our lightbox */
#include "OV14825_MIPI_d75.h"     /* CIE D75 (D75) overcast daylight, 7500K */
#include "OV14825_MIPI_f11.h"     /* CIE F11 (TL84) */

#define OV14825_I2C_START_BIT        (I2C_COMPLIANT_STARTBIT)    // I2C bus start condition
#define OV14825_I2C_NR_ADR_BYTES     (3U)                        // 1 byte base address and 2 bytes sub address
#define OV14825_I2C_NR_DAT_BYTES     (1U)                        // 8 bit registers


#define OV14825_ISIILLUPROFILES_DEFAULT  6U
static IsiIlluProfile_t OV14825_IlluProfileDefault[OV14825_ISIILLUPROFILES_DEFAULT] =
{
    {
        .p_next             = NULL,

        .name               = "A",
        .id                 = ISI_CIEPROF_A,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV14825_XTalkCoeff_CIE_A,
        .pCrossTalkOffset   = &OV14825_XTalkOffset_CIE_A,

        .pGaussMeanValue    = &OV14825_GaussMeanValue_CIE_A,
        .pCovarianceMatrix  = &OV14825_CovarianceMatrix_CIE_A,
        .pGaussFactor       = &OV14825_GaussFactor_CIE_A,
        .pThreshold         = &OV14825_Threshold_CIE_A,
        .pComponentGain     = &OV14825_CompGain_CIE_A,

        .pSaturationCurve   = &OV14825_SaturationCurve_CIE_A,
        .pCcMatrixTable     = &OV14825_CcMatrixTable_CIE_A,
        .pCcOffsetTable     = &OV14825_CcOffsetTable_CIE_A,

        .pVignettingCurve   = &OV14825_VignettingCurve_CIE_A,
    },
    {
        .p_next             = NULL,

        .name               = "F2",
        .id                 = ISI_CIEPROF_F2,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV14825_XTalkCoeff_F2,
        .pCrossTalkOffset   = &OV14825_XTalkOffset_F2,

        .pGaussMeanValue    = &OV14825_GaussMeanValue_F2,
        .pCovarianceMatrix  = &OV14825_CovarianceMatrix_F2,
        .pGaussFactor       = &OV14825_GaussFactor_F2,
        .pThreshold         = &OV14825_Threshold_F2,
        .pComponentGain     = &OV14825_CompGain_F2,

        .pSaturationCurve   = &OV14825_SaturationCurve_F2,
        .pCcMatrixTable     = &OV14825_CcMatrixTable_F2,
        .pCcOffsetTable     = &OV14825_CcOffsetTable_F2,

        .pVignettingCurve   = &OV14825_VignettingCurve_F2,
    },
    {
        .p_next             = NULL,

        .name               = "D50",
        .id                 = ISI_CIEPROF_D50,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,             /* from lightbox */
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_TRUE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV14825_XTalkCoeff_D50,
        .pCrossTalkOffset   = &OV14825_XTalkOffset_D50,

        .pGaussMeanValue    = &OV14825_GaussMeanValue_D50,
        .pCovarianceMatrix  = &OV14825_CovarianceMatrix_D50,
        .pGaussFactor       = &OV14825_GaussFactor_D50,
        .pThreshold         = &OV14825_Threshold_D50,
        .pComponentGain     = &OV14825_CompGain_D50,

        .pSaturationCurve   = &OV14825_SaturationCurve_D50,
        .pCcMatrixTable     = &OV14825_CcMatrixTable_D50,
        .pCcOffsetTable     = &OV14825_CcOffsetTable_D50,

        .pVignettingCurve   = &OV14825_VignettingCurve_D50,
    },
    {
        .p_next             = NULL,

        .name               = "D65",
        .id                 = ISI_CIEPROF_D65,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV14825_XTalkCoeff_D65,
        .pCrossTalkOffset   = &OV14825_XTalkOffset_D65,

        .pGaussMeanValue    = &OV14825_GaussMeanValue_D65,
        .pCovarianceMatrix  = &OV14825_CovarianceMatrix_D65,
        .pGaussFactor       = &OV14825_GaussFactor_D65,
        .pThreshold         = &OV14825_Threshold_D65,
        .pComponentGain     = &OV14825_CompGain_D65,

        .pSaturationCurve   = &OV14825_SaturationCurve_D65,
        .pCcMatrixTable     = &OV14825_CcMatrixTable_D65,
        .pCcOffsetTable     = &OV14825_CcOffsetTable_D65,

        .pVignettingCurve   = &OV14825_VignettingCurve_D65,
    },
    {
        .p_next             = NULL,

        .name               = "D75",
        .id                 = ISI_CIEPROF_D75,
        .DoorType           = ISI_DOOR_TYPE_OUTDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV14825_XTalkCoeff_D75,
        .pCrossTalkOffset   = &OV14825_XTalkOffset_D75,

        .pGaussMeanValue    = &OV14825_GaussMeanValue_D75,
        .pCovarianceMatrix  = &OV14825_CovarianceMatrix_D75,
        .pGaussFactor       = &OV14825_GaussFactor_D75,
        .pThreshold         = &OV14825_Threshold_D75,
        .pComponentGain     = &OV14825_CompGain_D75,

        .pSaturationCurve   = &OV14825_SaturationCurve_D75,
        .pCcMatrixTable     = &OV14825_CcMatrixTable_D75,
        .pCcOffsetTable     = &OV14825_CcOffsetTable_D75,

        .pVignettingCurve   = &OV14825_VignettingCurve_D75,
    },
    {
        .p_next             = NULL,

        .name               = "F11",
        .id                 = ISI_CIEPROF_F11,
        .DoorType           = ISI_DOOR_TYPE_INDOOR,
        .AwbType            = ISI_AWB_TYPE_AUTO,

        .bOutdoorClip       = BOOL_FALSE,

        /* legacy stuff */
        .pCrossTalkCoeff    = &OV14825_XTalkCoeff_F11,
        .pCrossTalkOffset   = &OV14825_XTalkOffset_F11,

        .pGaussMeanValue    = &OV14825_GaussMeanValue_F11,
        .pCovarianceMatrix  = &OV14825_CovarianceMatrix_F11,
        .pGaussFactor       = &OV14825_GaussFactor_F11,
        .pThreshold         = &OV14825_Threshold_F11,
        .pComponentGain     = &OV14825_CompGain_F11,

        .pSaturationCurve   = &OV14825_SaturationCurve_F11,
        .pCcMatrixTable     = &OV14825_CcMatrixTable_F11,
        .pCcOffsetTable     = &OV14825_CcOffsetTable_F11,

        .pVignettingCurve   = &OV14825_VignettingCurve_F11,
    }
};




/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static RESULT OV14825_IsiCreateSensorIss( IsiSensorInstanceConfig_t *pConfig );
static RESULT OV14825_IsiReleaseSensorIss( IsiSensorHandle_t handle );
static RESULT OV14825_IsiGetCapsIss( IsiSensorHandle_t handle, IsiSensorCaps_t *pIsiSensorCaps );
static RESULT OV14825_IsiSetupSensorIss( IsiSensorHandle_t handle, const IsiSensorConfig_t *pConfig );
static RESULT OV14825_IsiSensorSetStreamingIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV14825_IsiSensorSetPowerIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV14825_IsiCheckSensorConnectionIss( IsiSensorHandle_t handle );
static RESULT OV14825_IsiGetSensorRevisionIss( IsiSensorHandle_t handle, uint32_t *p_value);

static RESULT OV14825_IsiGetGainLimitsIss( IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain);
static RESULT OV14825_IsiGetIntegrationTimeLimitsIss( IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime );
static RESULT OV14825_IsiExposureControlIss( IsiSensorHandle_t handle, float NewGain, float NewIntegrationTime, uint8_t *pNumberOfFramesToSkip, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV14825_IsiGetCurrentExposureIss( IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV14825_IsiGetAfpsInfoIss ( IsiSensorHandle_t handle, uint32_t Resolution, IsiAfpsInfo_t* pAfpsInfo);
static RESULT OV14825_IsiGetGainIss( IsiSensorHandle_t handle, float *pSetGain );
static RESULT OV14825_IsiGetGainIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV14825_IsiSetGainIss( IsiSensorHandle_t handle, float NewGain, float *pSetGain );
static RESULT OV14825_IsiGetIntegrationTimeIss( IsiSensorHandle_t handle, float *pSetIntegrationTime );
static RESULT OV14825_IsiGetIntegrationTimeIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV14825_IsiSetIntegrationTimeIss( IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t *pNumberOfFramesToSkip );
static RESULT OV14825_IsiGetResolutionIss( IsiSensorHandle_t handle, uint32_t *pSetResolution );


static RESULT OV14825_IsiRegReadIss( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );
static RESULT OV14825_IsiRegWriteIss( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

static RESULT OV14825_IsiGetCalibKFactor( IsiSensorHandle_t handle, Isi1x1FloatMatrix_t **pIsiKFactor );
static RESULT OV14825_IsiGetCalibPcaMatrix( IsiSensorHandle_t   handle, Isi3x2FloatMatrix_t **pIsiPcaMatrix );
static RESULT OV14825_IsiGetCalibSvdMeanValue( IsiSensorHandle_t   handle, Isi3x1FloatMatrix_t **pIsiSvdMeanValue );
static RESULT OV14825_IsiGetCalibCenterLine( IsiSensorHandle_t   handle, IsiLine_t  **ptIsiCenterLine);
static RESULT OV14825_IsiGetCalibClipParam( IsiSensorHandle_t   handle, IsiAwbClipParm_t    **pIsiClipParam );
static RESULT OV14825_IsiGetCalibGlobalFadeParam( IsiSensorHandle_t       handle, IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam);
static RESULT OV14825_IsiGetCalibFadeParam( IsiSensorHandle_t   handle, IsiAwbFade2Parm_t   **ptIsiFadeParam);
static RESULT OV14825_IsiGetIlluProfile( IsiSensorHandle_t   handle, const uint32_t CieProfile, IsiIlluProfile_t **ptIsiIlluProfile );

static RESULT OV14825_IsiMdiInitMotoDriveMds( IsiSensorHandle_t handle );
static RESULT OV14825_IsiMdiSetupMotoDrive( IsiSensorHandle_t handle, uint32_t *pMaxStep );
static RESULT OV14825_IsiMdiFocusSet( IsiSensorHandle_t handle, const uint32_t Position );
static RESULT OV14825_IsiMdiFocusGet( IsiSensorHandle_t handle, uint32_t *pAbsStep );
static RESULT OV14825_IsiMdiFocusCalibrate( IsiSensorHandle_t handle );

static RESULT OV14825_IsiGetSensorMipiInfoIss( IsiSensorHandle_t handle, IsiSensorMipiInfo *ptIsiSensorMipiInfo);
static RESULT OV14825_IsiGetSensorIsiVersion(  IsiSensorHandle_t   handle, unsigned int* pVersion);
static RESULT OV14825_IsiGetSensorTuningXmlVersion(  IsiSensorHandle_t  handle, char** pTuningXmlVersion);


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
 *          OV14825_IsiCreateSensorIss
 *
 * @brief   This function creates a new OV14825 sensor instance handle.
 *
 * @param   pConfig     configuration structure to create the instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
static RESULT OV14825_IsiCreateSensorIss
(
    IsiSensorInstanceConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    OV14825_Context_t *pOV14825Ctx;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pConfig == NULL) || (pConfig->pSensor ==NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    pOV14825Ctx = ( OV14825_Context_t * )malloc ( sizeof (OV14825_Context_t) );
    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pOV14825Ctx, 0, sizeof( OV14825_Context_t ) );

    result = HalAddRef( pConfig->HalHandle );
    if ( result != RET_SUCCESS )
    {
        free ( pOV14825Ctx );
        return ( result );
    }

    pOV14825Ctx->IsiCtx.HalHandle              = pConfig->HalHandle;
    pOV14825Ctx->IsiCtx.HalDevID               = pConfig->HalDevID;
    pOV14825Ctx->IsiCtx.I2cBusNum              = pConfig->I2cBusNum;
    pOV14825Ctx->IsiCtx.SlaveAddress           = ( pConfig->SlaveAddr == 0 ) ? OV14825_SLAVE_ADDR : pConfig->SlaveAddr;
    pOV14825Ctx->IsiCtx.NrOfAddressBytes       = 2U;

    pOV14825Ctx->IsiCtx.I2cAfBusNum            = pConfig->I2cAfBusNum;
    pOV14825Ctx->IsiCtx.SlaveAfAddress         = ( pConfig->SlaveAfAddr == 0 ) ? OV14825_SLAVE_AF_ADDR : pConfig->SlaveAfAddr;
    pOV14825Ctx->IsiCtx.NrOfAfAddressBytes     = 0;

    pOV14825Ctx->IsiCtx.pSensor                = pConfig->pSensor;

    pOV14825Ctx->Configured             = BOOL_FALSE;
    pOV14825Ctx->Streaming              = BOOL_FALSE;
    pOV14825Ctx->TestPattern            = BOOL_FALSE;
    pOV14825Ctx->isAfpsRun              = BOOL_FALSE;

    pConfig->hSensor = ( IsiSensorHandle_t )pOV14825Ctx;

    result = HalSetCamConfig( pOV14825Ctx->IsiCtx.HalHandle, pOV14825Ctx->IsiCtx.HalDevID, true, true, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    result = HalSetClock( pOV14825Ctx->IsiCtx.HalHandle, pOV14825Ctx->IsiCtx.HalDevID, 10000000U);
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiReleaseSensorIss
 *
 * @brief   This function destroys/releases an OV14825 sensor instance.
 *
 * @param   handle      OV14825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
static RESULT OV14825_IsiReleaseSensorIss
(
    IsiSensorHandle_t handle
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    (void)OV14825_IsiSensorSetStreamingIss( pOV14825Ctx, BOOL_FALSE );
    (void)OV14825_IsiSensorSetPowerIss( pOV14825Ctx, BOOL_FALSE );

    (void)HalDelRef( pOV14825Ctx->IsiCtx.HalHandle );

    MEMSET( pOV14825Ctx, 0, sizeof( OV14825_Context_t ) );
    free ( pOV14825Ctx );

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetCapsIss
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
static RESULT OV14825_IsiGetCapsIss
(
    IsiSensorHandle_t handle,
    IsiSensorCaps_t   *pIsiSensorCaps
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSensorCaps == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        pIsiSensorCaps->BusWidth        = ISI_BUSWIDTH_12BIT;
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

        pIsiSensorCaps->Resolution      = ( ISI_RES_TV1080P15 | ISI_RES_TV1080P10 | ISI_RES_TV1080P5
                                          | ISI_RES_4416_3312
                                          );

        pIsiSensorCaps->BLC             =   ISI_BLC_AUTO;
        pIsiSensorCaps->AGC             = ( ISI_AGC_AUTO | ISI_AGC_OFF );
        pIsiSensorCaps->AWB             = ( ISI_AWB_AUTO | ISI_AWB_OFF );
        pIsiSensorCaps->AEC             = ( ISI_AEC_AUTO | ISI_AEC_OFF );
        pIsiSensorCaps->DPCC            = ( ISI_DPCC_AUTO | ISI_DPCC_OFF );

        pIsiSensorCaps->DwnSz           = ISI_DWNSZ_SUBSMPL;
        pIsiSensorCaps->CieProfile      = ( ISI_CIEPROF_A
                                          | ISI_CIEPROF_D50
                                          | ISI_CIEPROF_D65
                                          | ISI_CIEPROF_D75
                                          | ISI_CIEPROF_F2
                                          | ISI_CIEPROF_F11 );
        pIsiSensorCaps->SmiaMode        = ISI_SMIA_OFF;
        pIsiSensorCaps->MipiMode        = ISI_MIPI_MODE_RAW_12;
        pIsiSensorCaps->AfpsResolutions = ( ISI_RES_TV1080P15 | ISI_RES_TV1080P10 ////| ISI_RES_TV1080P5 -> disabled for AFPS until image distortion is fixed
                                          );
		pIsiSensorCaps->SensorOutputMode = ISI_SENSOR_OUTPUT_MODE_RAW;
    }

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_g_IsiSensorDefaultConfig
 *
 * @brief   recommended default configuration for application use via call
 *          to IsiGetSensorIss()
 *
 *****************************************************************************/
const IsiSensorCaps_t OV14825_g_IsiSensorDefaultConfig =
{
    ISI_BUSWIDTH_12BIT,         // BusWidth
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
    ISI_MIPI_MODE_RAW_12,       // MipiMode
    ( ISI_AFPS_NOTSUPP | ISI_RES_TV1080P15 | ISI_RES_TV1080P10 /*| ISI_RES_TV1080P5*/ ), // AfpsResolutions
    ISI_SENSOR_OUTPUT_MODE_RAW,
    0,
};



/*****************************************************************************/
/**
 *          OV14825_SetupOutputFormat
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV14825 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV14825_SetupOutputFormat
(
    OV14825_Context_t       *pOV14825Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s%s (enter)\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );

    /* bus-width */
    switch ( pConfig->BusWidth )        /* only ISI_BUSWIDTH_12BIT supported, no configuration needed here */
    {
        case ISI_BUSWIDTH_12BIT:
        {
            break;
        }

        default:
        {
            TRACE( OV14825_ERROR, "%s%s: bus width not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s: mode not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s: field selection not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* only Bayer mode is supported by OV14825 sensor, so the YCSequence parameter is not checked */
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
            TRACE( OV14825_ERROR, "%s%s: 422 conversion not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s: bayer pattern not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s: HPol not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s: VPol not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s:  edge mode not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s:  gamma not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s: color conversion not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV14825_ERROR, "%s%s: SMIA mode not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->MipiMode )        /* only ISI_MIPI_MODE_RAW_12 supported, no configuration needed */
    {
        case ISI_MIPI_MODE_RAW_12:
        {
            break;
        }

        default:
        {
            TRACE( OV14825_ERROR, "%s%s: MIPI mode not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
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
            //TRACE( OV14825_ERROR, "%s%s: AFPS not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
            //return ( RET_NOTSUPP );
        }
    }

    TRACE( OV14825_INFO, "%s%s (exit)\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_SetupOutputWindow
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV14825 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_SetupOutputWindow
(
    OV14825_Context_t       *pOV14825Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result     = RET_SUCCESS;


    uint16_t usFrameLengthLines = 0;
    uint16_t usLineLengthPck    = 0;
    float    rVtPixClkFreq      = 0.0f;

    uint16_t usHSize    = 0;
    uint16_t usVSize    = 0;

    uint16_t usYStart   = 0;
    uint16_t usYHeight  = 0;
    uint16_t usXStart   = 0;

    uint16_t usHTS      = 0;
    uint16_t usVTS      = 0;

    uint8_t ucCropping;

    uint8_t ucPLL0  = 0;
    uint8_t ucPLL1  = 0;
    uint8_t ucPLL2  = 0;
    uint8_t ucPLL3  = 0;
    uint8_t ucPLL4  = 0;

    uint8_t ucSPLL0 = 0;
    uint8_t ucSPLL1 = 0;
    uint8_t ucSPLL2 = 0;

    uint8_t ucReg_0x3614 = 0;
    uint8_t ucReg_0x360c = 0;
    uint8_t ucReg_0x3702 = 0;
    uint8_t ucReg_0x3704 = 0;
    uint8_t ucReg_0x3707 = 0;
    uint8_t ucReg_0x370a = 0;
    uint8_t ucReg_0x370b = 0;
    uint8_t ucReg_0x370c = 0;
    uint8_t ucReg_0x370d = 0;
    uint8_t ucReg_0x370f = 0;
    uint8_t ucReg_0x3713 = 0;
    uint8_t ucReg_0x3714 = 0;
    uint8_t ucReg_0x3715 = 0;
    uint8_t ucReg_0x3716 = 0;
    uint8_t ucReg_0x371c = 0;
    uint8_t ucReg_0x371d = 0;
    uint8_t ucReg_0x3721 = 0;
    uint8_t ucReg_0x3724 = 0;
    uint8_t ucReg_0x3725 = 0;
    uint8_t ucReg_0x3727 = 0;
    uint8_t ucReg_0x3728 = 0;

    uint8_t ucReg_0x3803 = 0;
    uint8_t ucReg_0x3810 = 0;
    uint8_t ucReg_0x3811 = 0;
    uint8_t ucReg_0x3818 = 0;

    uint8_t ucReg_0x4050 = 0;
    uint8_t ucReg_0x4051 = 0;
    uint8_t ucReg_0x4053 = 0;
    uint8_t ucReg_0x4837 = 0;
    uint8_t ucReg_0x503d = 0;
    uint8_t ucReg_0x5042 = 0;
    uint8_t ucReg_0x5047 = 0;

    uint16_t usBandStep50Hz = 0;
    uint16_t usBandStep60Hz = 0;
    uint8_t  ucMaxBands50Hz = 0;
    uint8_t  ucMaxBands60Hz = 0;

    TRACE( OV14825_INFO, "%s%s (enter)\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"");

    /* resolution */
    switch ( pConfig->Resolution )
    {
        case ISI_RES_TV1080P15:
        {
            TRACE( OV14825_INFO, "%s%s: Resolution 1920x1080P15\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );

            /* image cropping */
            usYStart    = 592U;
            usYHeight   = 2172U;
            usXStart    = 296U;
            ucCropping  = 0xD0U;

            /* image size */
            usHSize = 1920U;
            usVSize = 1080U;

            usHTS   = 1718U;
            usVTS   = 1092U;

            ucPLL4  = 0x08U;
            ucPLL3  = 0x00U;
            ucPLL2  = 0xBDU;
            ucPLL1  = 0xF5U;
            ucPLL0  = 0x88U;

            ucSPLL2 = 0x00U;
            ucSPLL1 = 0x40U;
            ucSPLL0 = 0x28U;

            ucReg_0x3614 = 0x5AU;
            ucReg_0x370d = 0x6DU; /* enable vertical and horizontal binning */

            usBandStep50Hz = 2784U;
            usBandStep60Hz = 2304U;
            ucMaxBands50Hz = 7U;
            ucMaxBands60Hz = 6U;

            /* don't know what this is about, maybe MIPI timming ??? */
            ucReg_0x360c = 0x44U;
            ucReg_0x3702 = 0x08U;
            ucReg_0x3704 = 0x0AU;
            ucReg_0x3707 = 0x73U;
            ucReg_0x370a = 0x81U;
            ucReg_0x370b = 0x20U;
            ucReg_0x370c = 0x04U;
            ucReg_0x370f = 0x00U;
            ucReg_0x3713 = 0x6aU;
            ucReg_0x3714 = 0x17U;
            ucReg_0x3715 = 0x16U;
            ucReg_0x3716 = 0x85U;
            ucReg_0x371c = 0x28U;
            ucReg_0x371d = 0x10U;
            ucReg_0x3721 = 0x28U;
            ucReg_0x3724 = 0x0aU;
            ucReg_0x3725 = 0x0bU;
            ucReg_0x3727 = 0x60U;
            ucReg_0x3728 = 0x82U;

            ucReg_0x3803 = 0x07U;

            /* mirror/flip in binning format */
            ucReg_0x4050 = 0xc1U;
            ucReg_0x4051 = 0x80U;
            ucReg_0x4053 = 0xADU; //0xAD = binning average, 0xA5U = binning sum
            ucReg_0x4837 = 0x18U;
            ucReg_0x5042 = 0x11U;
            ucReg_0x5047 = 0xf0U;

            ucReg_0x503d = 0x08U;

            ucReg_0x3810 = 0x22U;
            ucReg_0x3811 = 0x02U;
            ucReg_0x3818 = 0x45U;

            rVtPixClkFreq       = 84375000.0f; // note: this is the MIPI byte clock, will be changed to pixel clock below
            usLineLengthPck     = usHTS << 1U;
            usFrameLengthLines  = usVTS;

            break;
        }

        case ISI_RES_TV1080P10:
        {
            TRACE( OV14825_INFO, "%s%s: Resolution 1920x1080P10\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );

            /* image cropping */
            usYStart    = 592U;
            usYHeight   = 2172U;
            usXStart    = 296U;
            ucCropping  = 0xD0U;

            /* image size */
            usHSize = 1920U;
            usVSize = 1080U;

            usHTS   = 1718U;
            usVTS   = 1092U;

            ucPLL4  = 0x08U;
            ucPLL3  = 0x10U;
            ucPLL2  = 0xBAU;
            ucPLL1  = 0xF1U;
            ucPLL0  = 0x88U;

            ucSPLL2 = 0x00U;
            ucSPLL1 = 0x40U;
            ucSPLL0 = 0x28U;

            ucReg_0x3614 = 0x5AU;
            ucReg_0x370d = 0x6DU; /* enable vertical and horizontal binning */

            usBandStep50Hz = 2784U;
            usBandStep60Hz = 2304U;
            ucMaxBands50Hz = 7U;
            ucMaxBands60Hz = 6U;

            /* don't know what this is about, maybe MIPI timming ??? */
            ucReg_0x360c = 0x44U;
            ucReg_0x3702 = 0x08U;
            ucReg_0x3704 = 0x0AU;
            ucReg_0x3707 = 0x73U;
            ucReg_0x370a = 0x81U;
            ucReg_0x370b = 0x20U;
            ucReg_0x370c = 0x04U;
            ucReg_0x370f = 0x00U;
            ucReg_0x3713 = 0x6aU;
            ucReg_0x3714 = 0x17U;
            ucReg_0x3715 = 0x16U;
            ucReg_0x3716 = 0x85U;
            ucReg_0x371c = 0x28U;
            ucReg_0x371d = 0x10U;
            ucReg_0x3721 = 0x28U;
            ucReg_0x3724 = 0x0aU;
            ucReg_0x3725 = 0x0bU;
            ucReg_0x3727 = 0x60U;
            ucReg_0x3728 = 0x82U;

            ucReg_0x3803 = 0x07U;

            /* mirror/flip in binning format */
            ucReg_0x4050 = 0xc1U;
            ucReg_0x4051 = 0x80U;
            ucReg_0x4053 = 0xADU; //0xAD = binning average, 0xA5U = binning sum
            ucReg_0x4837 = 0x18U;
            ucReg_0x5042 = 0x11U;
            ucReg_0x5047 = 0xf0U;

            ucReg_0x503d = 0x08U;

            ucReg_0x3810 = 0x22U;
            ucReg_0x3811 = 0x02U;
            ucReg_0x3818 = 0x45U;

            rVtPixClkFreq       = 56250000.0f; // note: this is the MIPI byte clock, will be changed to pixel clock below
            usLineLengthPck     = usHTS << 1U;
            usFrameLengthLines  = usVTS;

            break;
        }

        case ISI_RES_TV1080P5:
        {
            TRACE( OV14825_INFO, "%s%s: Resolution 1920x1080P5\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );

            /* image cropping */
            usYStart    = 592U;
            usYHeight   = 2172U;
            usXStart    = 296U;
            ucCropping  = 0xD0U;

            /* image size */
            usHSize = 1920U;
            usVSize = 1080U;

            usHTS   = 1718U;
            usVTS   = 1092U;

            ucPLL4  = 0x08U;
            ucPLL3  = 0x10U;
            ucPLL2  = 0xBDU;
            ucPLL1  = 0xF1U;
            ucPLL0  = 0x88U;

            ucSPLL2 = 0x00U;
            ucSPLL1 = 0x40U;
            ucSPLL0 = 0x28U;

            ucReg_0x3614 = 0x5AU;
            ucReg_0x370d = 0x6DU; /* enable vertical and horizontal binning */

            usBandStep50Hz = 2784U;
            usBandStep60Hz = 2304U;
            ucMaxBands50Hz = 7U;
            ucMaxBands60Hz = 6U;

            /* don't know what this is about, maybe MIPI timming ??? */
            ucReg_0x360c = 0x44U;
            ucReg_0x3702 = 0x08U;
            ucReg_0x3704 = 0x0AU;
            ucReg_0x3707 = 0x73U;
            ucReg_0x370a = 0x81U;
            ucReg_0x370b = 0x20U;
            ucReg_0x370c = 0x04U;
            ucReg_0x370f = 0x00U;
            ucReg_0x3713 = 0x6aU;
            ucReg_0x3714 = 0x17U;
            ucReg_0x3715 = 0x16U;
            ucReg_0x3716 = 0x85U;
            ucReg_0x371c = 0x28U;
            ucReg_0x371d = 0x10U;
            ucReg_0x3721 = 0x28U;
            ucReg_0x3724 = 0x0aU;
            ucReg_0x3725 = 0x0bU;
            ucReg_0x3727 = 0x60U;
            ucReg_0x3728 = 0x82U;

            ucReg_0x3803 = 0x07U;

            /* mirror/flip in binning format */
            ucReg_0x4050 = 0xc1U;
            ucReg_0x4051 = 0x80U;
            ucReg_0x4053 = 0xADU; //0xAD = binning average, 0xA5U = binning sum
            ucReg_0x4837 = 0x18U;
            ucReg_0x5042 = 0x11U;
            ucReg_0x5047 = 0xf0U;

            ucReg_0x503d = 0x08U;

            ucReg_0x3810 = 0x22U;
            ucReg_0x3811 = 0x02U;
            ucReg_0x3818 = 0x45U;

            rVtPixClkFreq       = 28125000.0f; // note: this is the MIPI byte clock, will be changed to pixel clock below
            usLineLengthPck     = usHTS << 1U;
            usFrameLengthLines  = usVTS;

            break;
        }

        case ISI_RES_4416_3312:
        {
            TRACE( OV14825_INFO, "%s%s: Resolution 4416x3312\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );

            /* image cropping */
            usYStart    = 16U;
            usYHeight   = 3320U;
            usXStart    = 12U;
            ucCropping  = 0x50U;

            /* image size */
            usHSize = 4416U;
            usVSize = 3312U;

            usHTS   = 3064U;
            usVTS   = 3336U;

            ucPLL4  = 0x08U;
            ucPLL3  = 0x00U;
            ucPLL2  = 0xBDU; //0xBBU;
            ucPLL1  = 0xF5U;
            ucPLL0  = 0x88U; //0x8DU;

            ucSPLL2 = 0x00U;
            ucSPLL1 = 0x40U;
            ucSPLL0 = 0x28U; //0x13U;

            ucReg_0x3614 = 0x5AU;
            ucReg_0x370d = 0x0DU;    /* disable vertical and horizontal binning */

            usBandStep50Hz = 5856U;
            usBandStep60Hz = 4864U;
            ucMaxBands50Hz = 10U;
            ucMaxBands60Hz =  9U;

            /* don't know what this is about, maybe MIPI timming ??? */
            ucReg_0x360c = 0x42U;
            ucReg_0x3702 = 0x20U;
            ucReg_0x3704 = 0x28U;
            ucReg_0x3707 = 0x73U;
            ucReg_0x370a = 0x80U;
            ucReg_0x370b = 0x00U;
            ucReg_0x370c = 0x84U;
            ucReg_0x370f = 0x61U;
            ucReg_0x3713 = 0xF6U;
            ucReg_0x3714 = 0x5FU;
            ucReg_0x3715 = 0x58U;
            ucReg_0x3716 = 0x17U;
            ucReg_0x371c = 0x46U;
            ucReg_0x371d = 0x40U;
            ucReg_0x3721 = 0x08U;
            ucReg_0x3724 = 0x30U;
            ucReg_0x3725 = 0x2EU;
            ucReg_0x3727 = 0x62U;
            ucReg_0x3728 = 0x0CU;

            ucReg_0x3803 = 0x0BU;

            /* mirror/flip in binning format */
            ucReg_0x4050 = 0xc0U;
            ucReg_0x4051 = 0x00U;
            ucReg_0x4053 = 0xa1U;
            ucReg_0x4837 = 0x1bU;
            ucReg_0x5042 = 0x11U;
            ucReg_0x5047 = 0x00U;

            ucReg_0x503d = 0x00U;

            ucReg_0x3810 = 0x44U;
            ucReg_0x3811 = 0x96U;
            ucReg_0x3818 = 0x40U;

            rVtPixClkFreq       = 84375000.0f; // note: this is the MIPI byte clock, will be changed to pixel clock below
            usLineLengthPck     = usHTS << 1U;
            usFrameLengthLines  = usVTS;

            break;
        }

        default:
        {
            TRACE( OV14825_ERROR, "%s%s: Resolution not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->MipiMode )        /* only ISI_MIPI_MODE_RAW_12 supported */
    {
        case ISI_MIPI_MODE_RAW_12:
        {
            rVtPixClkFreq = 8.0f / 12.0f * rVtPixClkFreq;
            break;
        }

        default:
        {
            TRACE( OV14825_ERROR, "%s%s: MIPI mode not supported\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    TRACE( OV14825_DEBUG, "%s%s: Resolution %dx%d @ %f fps\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"", usHSize, usVSize, rVtPixClkFreq / ( usLineLengthPck * usFrameLengthLines ) );

    if (!pOV14825Ctx->isAfpsRun)
    {
        // set camera output size
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_HW_2, (usHSize & 0x1F00U) >> 8U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_HW_1, (usHSize & 0x00FFU) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_VH_2, (usVSize & 0x0F00U) >> 8U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_VH_1, (usVSize & 0x00FFU) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        /* HTS */
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_HTS_2, (usHTS & 0x1F00U) >> 8U );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_HTS_1, (usHTS & 0x00FFU) );

        /* VTS */
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_VTS_2, (usVTS & 0xFF00U) >> 8U );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_VTS_1, (usVTS & 0x00FFU) );

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_CONTROL_1C, ((usYStart & 0x0F00U) >> 8U) | ucCropping );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_CONTROL_1D, (usYStart & 0x00FFU) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_CONTROL_1E, (usYHeight & 0x0F00U) >> 8U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_CONTROL_1F, (usYHeight & 0x00FFU) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_CONTROL_20, (usXStart & 0x0F00U) >> 8U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_CONTROL_21, (usXStart & 0x00FFU) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        // set pll values
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_PLL_CTRL4, ucPLL4 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_PLL_CTRL3, ucPLL3 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_PLL_CTRL2, ucPLL2 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_PLL_CTRL1, ucPLL1 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_PLL_CTRL0, ucPLL0 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_SC_PLL_CTRL_S2, ucSPLL2 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_SC_PLL_CTRL_S1, ucSPLL1 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result =  OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_SC_PLL_CTRL_S0, ucSPLL0 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3614, ucReg_0x3614 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_ARRAY_CONTROL_0D, ucReg_0x370d );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_360C, ucReg_0x360c );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3702, ucReg_0x3702 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3704, ucReg_0x3704 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3707, ucReg_0x3707 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_370A, ucReg_0x370a );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_370B, ucReg_0x370b );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_370C, ucReg_0x370c );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_370F, ucReg_0x370f );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3713, ucReg_0x3713 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3714, ucReg_0x3714 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3715, ucReg_0x3715 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3716, ucReg_0x3716 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_371C, ucReg_0x371c );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_371D, ucReg_0x371d );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3721, ucReg_0x3721 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3724, ucReg_0x3724 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3725, ucReg_0x3725 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3727, ucReg_0x3727 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3728, ucReg_0x3728 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3803, ucReg_0x3803 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        /* mirror/flip in binning format */
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_4050, ucReg_0x4050 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_4051, ucReg_0x4051 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_4053, ucReg_0x4053 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_4837, ucReg_0x4837 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_5042, ucReg_0x5042 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_5047, ucReg_0x5047 );

        ucReg_0x503d |= ( BOOL_TRUE == pOV14825Ctx->TestPattern ) ? 0x80 : 0x00;
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_ISP_CTRL3D, ucReg_0x503d );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_HOFFS, ucReg_0x3810 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_3811, ucReg_0x3811 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_TIMING_TC_REG18, ucReg_0x3818 );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_B50_STEP_2, (usBandStep50Hz & 0x0000FF00U) >> 8U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_B50_STEP_1,  usBandStep50Hz & 0x000000FFU );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_B60_STEP_2, (usBandStep60Hz & 0x0000FF00U) >> 8U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_B60_STEP_1,  usBandStep60Hz & 0x000000FFU );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_CTRL0D    ,  ucMaxBands50Hz );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_CTRL0E    ,  ucMaxBands60Hz );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }

    //store frame timing for later use in AEC module
    pOV14825Ctx->VtPixClkFreq     = rVtPixClkFreq;
    pOV14825Ctx->LineLengthPck    = usLineLengthPck;
    pOV14825Ctx->FrameLengthLines = usFrameLengthLines;

    TRACE( OV14825_INFO, "%s%s (exit)\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_SetupImageControl
 *
 * @brief   Sets the image control functions (BLC, AGC, AWB, AEC, DPCC ...)
 *
 * @param   handle      OV14825 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV14825_SetupImageControl
(
    OV14825_Context_t       *pOV14825Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV14825_INFO, "%s%s (enter)\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );

    if (!pOV14825Ctx->isAfpsRun)
    {
        switch ( pConfig->Bls )      /* only ISI_BLS_OFF supported, no configuration needed */
        {
            case ISI_BLS_OFF:
            {
                break;
            }

            default:
            {
                TRACE( OV14825_ERROR, "%s: Black level not supported\n", __FUNCTION__ );
                return ( RET_NOTSUPP );
            }
        }

        /* black level compensation */
        switch ( pConfig->BLC )      /* only ISI_BLS_AUTO supported, no configuration needed */
        {
            case ISI_BLC_AUTO:
            {
                break;
            }

            default:
            {
                TRACE( OV14825_ERROR, "%s: BLC not supported\n", __FUNCTION__ );
                return ( RET_NOTSUPP );
            }
        }

        /* automatic gain control */
        switch ( pConfig->AGC )
        {
            case ISI_AGC_OFF:
            {
                // manual gain (appropriate for AEC with Marvin) ( set bit 1 )
                result = OV14825_IsiRegReadIss(  pOV14825Ctx, OV14825_AEC_MANUAL, &RegValue );
                result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_MANUAL, (RegValue | 0x02U) );
                break;
            }

            case ISI_AGC_AUTO:
            {
                // auto gain (appropriate for AEC with Marvin) ( clear bit 1 )
                result = OV14825_IsiRegReadIss(  pOV14825Ctx, OV14825_AEC_MANUAL, &RegValue );
                result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_MANUAL, (RegValue & ~0x02U) );
                break;
            }

            default:
            {
                TRACE( OV14825_ERROR, "%s: AGC not supported\n", __FUNCTION__ );
                return ( RET_NOTSUPP );
            }
        }

        /* automatic white balance */
        switch( pConfig->AWB )
        {
            case ISI_AWB_OFF:
            {
                result = OV14825_IsiRegReadIss(  pOV14825Ctx, OV14825_ISP_CTRL01, &RegValue );
                result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_ISP_CTRL01, (RegValue & ~0x01U) );
                break;
            }

            case ISI_AWB_AUTO:
            {
                result = OV14825_IsiRegReadIss(  pOV14825Ctx, OV14825_ISP_CTRL01, &RegValue );
                result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_ISP_CTRL01, (RegValue | 0x01U) );
                break;
            }

            default:
            {
                TRACE( OV14825_ERROR, "%s: AWB not supported\n", __FUNCTION__ );
                return ( RET_NOTSUPP );
            }
        }

        switch( pConfig->AEC )
        {
            case ISI_AEC_OFF:
            {
                // manual exposure (appropriate for AEC with Marvin) ( set bit 0 )
                result = OV14825_IsiRegReadIss(  pOV14825Ctx, OV14825_AEC_MANUAL, &RegValue );
                result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_MANUAL, (RegValue | 0x01U) );
                break;
            }

            case ISI_AEC_AUTO:
            {
                // auto exposure (appropriate for AEC with Marvin) ( set bit 0 )
                result = OV14825_IsiRegReadIss(  pOV14825Ctx, OV14825_AEC_MANUAL, &RegValue );
                result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_MANUAL, (RegValue & ~0x01U) );
                break;
            }

            default:
            {
                TRACE( OV14825_ERROR, "%s: AEC not supported\n", __FUNCTION__ );
                return ( RET_NOTSUPP );
            }
        }


        switch( pConfig->DPCC )
        {
            case ISI_DPCC_OFF:
            {
                // disable white and black pixel cancellation (clear bit 2 and 1)
                result = OV14825_IsiRegReadIss( pOV14825Ctx, OV14825_ISP_CTRL00, &RegValue );
                RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
                result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_ISP_CTRL00, (RegValue & ~0x06U) );
                RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
                break;
            }

            case ISI_DPCC_AUTO:
            {
                // enable white and black pixel cancellation (set bit 2 and 1)
                result = OV14825_IsiRegReadIss( pOV14825Ctx, OV14825_ISP_CTRL00, &RegValue );
                RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
                result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_ISP_CTRL00, (RegValue | 0x06U) );
                RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
                break;
            }

            default:
            {
                TRACE( OV14825_ERROR, "%s: DPCC not supported\n", __FUNCTION__ );
                return ( RET_NOTSUPP );
            }
        }
    }

    TRACE( OV14825_INFO, "%s%s (exit)\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"" );

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_AecSetModeParameters
 *
 * @brief   This function fills in the correct parameters in OV14825-Instances
 *          according to AEC mode selection in IsiSensorConfig_t.
 *
 * @note    It is assumed that IsiSetupOutputWindow has been called before
 *          to fill in correct values in instance structure.
 *
 * @param   handle      OV14825 context
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_AecSetModeParameters
(
    OV14825_Context_t       *pOV14825Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s%s (enter)\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"");

    if ( (pOV14825Ctx->VtPixClkFreq == 0.0f) )
    {
        TRACE( OV14825_ERROR, "%s%s: Division by zero!\n", __FUNCTION__  );
        return ( RET_OUTOFRANGE );
    }

    //as of mail from Omnivision FAE the limit is VTS - 6 (above that we observed a frame
    //exposed way too dark from time to time)
    // (formula is usually MaxIntTime = (CoarseMax * LineLength + FineMax) / Clk
    //                     MinIntTime = (CoarseMin * LineLength + FineMin) / Clk )
    pOV14825Ctx->AecMaxIntegrationTime = ( ((float)(pOV14825Ctx->FrameLengthLines - 6)) * ((float)pOV14825Ctx->LineLengthPck) ) / pOV14825Ctx->VtPixClkFreq;
    pOV14825Ctx->AecMinIntegrationTime = 0.0f;

    TRACE( OV14825_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"", pOV14825Ctx->AecMaxIntegrationTime  );

    pOV14825Ctx->AecMaxGain = OV14825_MAX_GAIN_AEC;
    pOV14825Ctx->AecMinGain = 16.0f / 13.0f; //as of sensor datasheet 32/(32-6)

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    pOV14825Ctx->AecIntegrationTimeIncrement = ((float)pOV14825Ctx->LineLengthPck) / pOV14825Ctx->VtPixClkFreq;
    pOV14825Ctx->AecGainIncrement = OV14825_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOV14825Ctx->AecCurGain               = pOV14825Ctx->AecMinGain;
    pOV14825Ctx->AecCurIntegrationTime    = 0.0f;
    pOV14825Ctx->OldMultiplier            = 0;
    pOV14825Ctx->OldBase                  = 6U;
    pOV14825Ctx->OldCoarseIntegrationTime = 0;
    pOV14825Ctx->OldFineIntegrationTime   = 0;
    pOV14825Ctx->GroupHold                = true; //must be true (for unknown reason) to correctly set gain the first time

    TRACE( OV14825_INFO, "%s%s (exit)\n", __FUNCTION__, pOV14825Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiSetupSensorIss
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV14825 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiSetupSensorIss
(
    IsiSensorHandle_t       handle,
    const IsiSensorConfig_t *pConfig
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid configuration (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    if ( pOV14825Ctx->Streaming != BOOL_FALSE )
    {
        return RET_WRONG_STATE;
    }

    MEMCPY( &pOV14825Ctx->Config, pConfig, sizeof( IsiSensorConfig_t ) );

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    result = OV14825_IsiRegWriteIss ( pOV14825Ctx, OV14825_SMIA_R0103, 0x01U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    osSleep( 10 );

    TRACE( OV14825_DEBUG, "%s: OV14825 System-Reset executed\n", __FUNCTION__);

    /* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */
    result = IsiRegDefaultsApply( pOV14825Ctx, OV14825_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* sleep a while, that sensor can take over new default values */
    osSleep( 10 );

    /* 3.) verify default values to make sure everything has been written correctly as expected */
    result = IsiRegDefaultsVerify( pOV14825Ctx, OV14825_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    // output of pclk for measurement (only debugging)
    result = OV14825_IsiRegWriteIss( pOV14825Ctx, 0x3009U, 0x10U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    /* 4.) setup output format (RAW10|RAW12) */
    result = OV14825_SetupOutputFormat( pOV14825Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV14825_ERROR, "%s: SetupOutputFormat failed.\n", __FUNCTION__);
        return ( result );
    }

    /* 5.) setup output window */
    result = OV14825_SetupOutputWindow( pOV14825Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV14825_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV14825_SetupImageControl( pOV14825Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV14825_ERROR, "%s: SetupImageControl failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV14825_AecSetModeParameters( pOV14825Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV14825_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
        return ( result );
    }

    if (result == RET_SUCCESS)
    {
        pOV14825Ctx->Configured = BOOL_TRUE;
    }

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiChangeSensorResolutionIss
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
static RESULT OV14825_IsiChangeSensorResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (pNumberOfFramesToSkip == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pOV14825Ctx->Configured != BOOL_TRUE) || (pOV14825Ctx->Streaming != BOOL_FALSE) )
    {
        return RET_WRONG_STATE;
    }

    IsiSensorCaps_t Caps;
    result = OV14825_IsiGetCapsIss( handle, &Caps);
    if (RET_SUCCESS != result)
    {
        return result;
    }

    if ( (Resolution & Caps.Resolution) == 0 )
    {
        return RET_OUTOFRANGE;
    }

    if ( Resolution == pOV14825Ctx->Config.Resolution )
    {
        // well, no need to worry
        *pNumberOfFramesToSkip = 0;
    }
    else
    {
        // change resolution
        char *szResName = NULL;
        result = IsiGetResolutionName( Resolution, &szResName );
        TRACE( OV14825_DEBUG, "%s: NewRes=0x%08x (%s)\n", __FUNCTION__, Resolution, szResName);

        // update resolution in copy of config in context
        pOV14825Ctx->Config.Resolution = Resolution;

        // tell sensor about that
        result = OV14825_SetupOutputWindow( pOV14825Ctx, &pOV14825Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV14825_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
            return ( result );
        }

        // remember old exposure values
        float OldGain = pOV14825Ctx->AecCurGain;
        float OldIntegrationTime = pOV14825Ctx->AecCurIntegrationTime;

        // update limits & stuff (reset current & old settings)
        result = OV14825_AecSetModeParameters( pOV14825Ctx, &pOV14825Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV14825_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
            return ( result );
        }

        // restore old exposure values (at least within new exposure values' limits)
        uint8_t NumberOfFramesToSkip;
        float   DummySetGain;
        float   DummySetIntegrationTime;
        result = OV14825_IsiExposureControlIss( handle, OldGain, OldIntegrationTime, &NumberOfFramesToSkip, &DummySetGain, &DummySetIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV14825_ERROR, "%s: OV14825_IsiExposureControlIss failed.\n", __FUNCTION__);
            return ( result );
        }

        // return number of frames that aren't exposed correctly
        *pNumberOfFramesToSkip = NumberOfFramesToSkip + 1;
    }

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiSensorSetStreamingIss
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
static RESULT OV14825_IsiSensorSetStreamingIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    uint32_t RegValue = 0;

    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pOV14825Ctx->Configured != BOOL_TRUE) || (pOV14825Ctx->Streaming == on) )
    {
        return RET_WRONG_STATE;
    }

    if (on == BOOL_TRUE)
    {
        // sensor wake up
        TRACE( OV14825_DEBUG, "%s: ON\n", __FUNCTION__);
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_SMIA_R0100, 0x01U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }
    else
    {
        // sensor sleep
        TRACE( OV14825_DEBUG, "%s: OFF\n", __FUNCTION__);
        result |= OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_SMIA_R0100, 0x00U ); //FIXME: is it ok to just write 0 (zero) here?
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }

    if (result == RET_SUCCESS)
    {
        pOV14825Ctx->Streaming = on;
    }

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiSensorSetPowerIss
 *
 * @brief   Performs the power-up/power-down sequence of the camera, if possible.
 *
 * @param   handle      OV14825 sensor instance handle
 * @param   on          new power state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiSensorSetPowerIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pOV14825Ctx->Configured = BOOL_FALSE;
    pOV14825Ctx->Streaming  = BOOL_FALSE;

    TRACE( OV14825_DEBUG, "%s power off \n", __FUNCTION__);
    result = HalSetPower( pOV14825Ctx->IsiCtx.HalHandle, pOV14825Ctx->IsiCtx.HalDevID, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV14825_DEBUG, "%s reset on\n", __FUNCTION__);
    result = HalSetReset( pOV14825Ctx->IsiCtx.HalHandle, pOV14825Ctx->IsiCtx.HalDevID, true );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    if (on == BOOL_TRUE)
    {
        TRACE( OV14825_DEBUG, "%s power on \n", __FUNCTION__);
        result = HalSetPower( pOV14825Ctx->IsiCtx.HalHandle, pOV14825Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV14825_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pOV14825Ctx->IsiCtx.HalHandle, pOV14825Ctx->IsiCtx.HalDevID, false );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV14825_DEBUG, "%s reset on \n", __FUNCTION__);
        result = HalSetReset( pOV14825Ctx->IsiCtx.HalHandle, pOV14825Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV14825_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pOV14825Ctx->IsiCtx.HalHandle, pOV14825Ctx->IsiCtx.HalDevID, false );

        osSleep( 50 );
    }

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiCheckSensorConnectionIss
 *
 * @brief   Checks the I2C-Connection to sensor by reading sensor revision id.
 *
 * @param   handle      OV14825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiCheckSensorConnectionIss
(
    IsiSensorHandle_t   handle
)
{
    uint32_t RevId;
    uint32_t value;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    RevId = OV14825_CHIP_ID_HIGH_BYTE_DEFAULT;
    RevId = (RevId << 8U) | OV14825_CHIP_ID_LOW_BYTE_DEFAULT;

    result = OV14825_IsiGetSensorRevisionIss( handle, &value );
    if ( (result != RET_SUCCESS) || (RevId != value) )
    {
        TRACE( OV14825_ERROR, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );
        return ( RET_FAILURE );
    }

    TRACE( OV14825_DEBUG, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetSensorRevisionIss
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
static RESULT OV14825_IsiGetSensorRevisionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    uint32_t data;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *p_value = 0;
    result = OV14825_IsiRegReadIss ( handle, OV14825_CHIP_ID_HIGH_BYTE, &data );
    *p_value = ( (data & 0xFFU) << 8U );
    result = OV14825_IsiRegReadIss ( handle, OV14825_CHIP_ID_LOW_BYTE, &data );
    *p_value |= ( data & 0xFFU );

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiRegReadIss
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
static RESULT OV14825_IsiRegReadIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

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
        uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, OV14825_g_aRegDescription );
        if ( !NrOfBytes )
        {
            NrOfBytes = 1;
        }
        TRACE( OV14825_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);

        *p_value = 0;
        result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
    }

    TRACE( OV14825_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiRegWriteIss
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
static RESULT OV14825_IsiRegWriteIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NrOfBytes;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    NrOfBytes = IsiGetNrDatBytesIss( address, OV14825_g_aRegDescription );
    if ( !NrOfBytes )
    {
        NrOfBytes = 1;
    }
    TRACE( OV14825_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x 0x%08x)\n", __FUNCTION__, NrOfBytes, address, value);

    result = IsiI2cWriteSensorRegister( handle, address, (uint8_t *)(&value), NrOfBytes, BOOL_TRUE );

    TRACE( OV14825_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetGainLimitsIss
 *
 * @brief   Returns the exposure minimal and maximal values of an
 *          OV14825 instance
 *
 * @param   handle       OV14825 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetGainLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinGain,
    float               *pMaxGain
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinGain == NULL) || (pMaxGain == NULL) )
    {
        TRACE( OV14825_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinGain = pOV14825Ctx->AecMinGain;
    *pMaxGain = pOV14825Ctx->AecMaxGain;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetIntegrationTimeLimitsIss
 *
 * @brief   Returns the minimal and maximal integration time values of an
 *          OV14825 instance
 *
 * @param   handle       OV14825 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetIntegrationTimeLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinIntegrationTime,
    float               *pMaxIntegrationTime
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL) )
    {
        TRACE( OV14825_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinIntegrationTime = pOV14825Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pOV14825Ctx->AecMaxIntegrationTime;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV14825_IsiGetGainIss
 *
 * @brief   Reads gain values from the image sensor module.
 *
 * @param   handle                  OV14825 sensor instance handle
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV14825_IsiGetGainIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain = pOV14825Ctx->AecCurGain;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetGainIncrementIss
 *
 * @brief   Get smallest possible gain increment.
 *
 * @param   handle                  OV14825 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV14825_IsiGetGainIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV14825Ctx->AecGainIncrement;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiSetGainIss
 *
 * @brief   Writes gain values to the image sensor module.
 *          Updates current gain and exposure in sensor struct/state.
 *
 * @param   handle                  OV14825 sensor instance handle
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
RESULT OV14825_IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint16_t usMultiplier = 0;
    uint8_t  ucMultiplier = 0;
    uint8_t  ucBase       = 0;

    float fBase  = 0.0f;
    float fDelta = 0.0f;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        TRACE( OV14825_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    //gain = (A[0]+1) * (B[7]+1) * (B[6]+1) * (B[5]+1) * (B[4]+1) * ( 32 / (32-B[3:0]) )
    //A/B = register 0x350A/B
    //minimum value is A=0, B=6
    //lower multiplier bits have to be set first

    //maximum and minimum gain is limited by the sensor, if this limit is not
    //considered, the exposure control loop needs lots of time to return to a new state
    //so limit to allowed range
    if( NewGain < pOV14825Ctx->AecMinGain ) NewGain = pOV14825Ctx->AecMinGain;
    if( NewGain > pOV14825Ctx->AecMaxGain ) NewGain = pOV14825Ctx->AecMaxGain;

    //determine multiplier
    usMultiplier = 0;
    ucMultiplier = 1U;
    while( NewGain > (33.0f / 17.0f) ){ //middle between 32/(32-15) (last base) and 2 (first multiplier)
       NewGain /= 2.0f;
       usMultiplier = (usMultiplier << 1U) | 0x0010U;
       ucMultiplier = ucMultiplier << 1U;
    }
    DCT_ASSERT( (usMultiplier & ~0x1f0U) == 0 );

    //calculate ucBase by rounding down in a first attempt
    fBase  = 32.0f - 32.0f / NewGain;
    ucBase = (uint8_t)fBase;

    //calculate delta to mid point of the adjacent combinations
    fDelta = ( 16.0f/(32.0f-((float)ucBase + 1)) - 16.0f/(32.0f-((float)(ucBase))) );

    //calculate ucBase with proper rounding
    fBase  = 32.0f - 32.0f / ( NewGain + fDelta );
    ucBase = (uint8_t)fBase;
    if( ucBase > 0x0f ) ucBase = 0x0f; //should never hit, perhaps due to floating point inaccuracy

    // write new gain into sensor registers, do not write if nothing has changed
    if( (usMultiplier != pOV14825Ctx->OldMultiplier) || (ucBase != pOV14825Ctx->OldBase  ) )
    {
        if( !pOV14825Ctx->GroupHold ) //don't touch if already held
        {
          result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0x00 ); //group 0 hold
          RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        }

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_AGC_ADJ_2, (usMultiplier & 0x0100U) >> 8U  );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_AGC_ADJ_1, (usMultiplier & 0x00F0U) | ucBase );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        if( !pOV14825Ctx->GroupHold )
        {
            result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0x10 ); //finish group 0
            RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0xa0 ); //and launch
            RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        }

        pOV14825Ctx->OldMultiplier = usMultiplier;
        pOV14825Ctx->OldBase       = ucBase;
    }

    //calculate gain actually set
    pOV14825Ctx->AecCurGain = ((float)ucMultiplier) * 32.0f / (32.0f - ((float)ucBase));

    //return current state
    *pSetGain = pOV14825Ctx->AecCurGain;
    TRACE( OV14825_DEBUG, "%s: g=%f\n", __FUNCTION__, *pSetGain );

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetIntegrationTimeIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  OV14825 sensor instance handle
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV14825_IsiGetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               *pSetIntegrationTime
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetIntegrationTime == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetIntegrationTime = pOV14825Ctx->AecCurIntegrationTime;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetIntegrationTimeIncrementIss
 *
 * @brief   Get smallest possible integration time increment.
 *
 * @param   handle                  OV14825 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV14825_IsiGetIntegrationTimeIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV14825Ctx->AecIntegrationTimeIncrement;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiSetIntegrationTimeIss
 *
 * @brief   Writes gain and integration time values to the image sensor module.
 *          Updates current integration time and exposure in sensor
 *          struct/state.
 *
 * @param   handle                  OV14825 sensor instance handle
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
RESULT OV14825_IsiSetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               NewIntegrationTime,
    float               *pSetIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t CoarseIntegrationTime = 0;
    //uint32_t FineIntegrationTime   = 0; //not supported by OV14825

    float ShutterWidthPck = 0.0f; //shutter width in pixel clock periods

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetIntegrationTime == NULL) || (pNumberOfFramesToSkip == NULL) )
    {
        TRACE( OV14825_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    //maximum and minimum integration time is limited by the sensor, if this limit is not
    //considered, the exposure control loop needs lots of time to return to a new state
    //so limit to allowed range
    if ( NewIntegrationTime > pOV14825Ctx->AecMaxIntegrationTime ) NewIntegrationTime = pOV14825Ctx->AecMaxIntegrationTime;
    if ( NewIntegrationTime < pOV14825Ctx->AecMinIntegrationTime ) NewIntegrationTime = pOV14825Ctx->AecMinIntegrationTime;

    //the actual integration time is given by
    //integration_time = ( coarse_integration_time * line_length_pck + fine_integration_time ) / vt_pix_clk_freq
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck )
    //fine_integration_time   = integration_time * vt_pix_clk_freq - coarse_integration_time * line_length_pck
    //
    //fine integration is not supported by OV14825
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck + 0.5 )

    ShutterWidthPck = NewIntegrationTime * ( (float)pOV14825Ctx->VtPixClkFreq );

    // avoid division by zero
    if ( pOV14825Ctx->LineLengthPck == 0 )
    {
        TRACE( OV14825_ERROR, "%s: Division by zero!\n", __FUNCTION__ );
        return ( RET_DIVISION_BY_ZERO );
    }

    //calculate the integer part of the integration time in units of line length
    //calculate the fractional part of the integration time in units of pixel clocks
    //CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV14825Ctx->LineLengthPck) );
    //FineIntegrationTime   = ( (uint32_t)ShutterWidthPck ) - ( CoarseIntegrationTime * pOV14825Ctx->LineLengthPck );
    CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV14825Ctx->LineLengthPck) + 0.5f );

    // write new integration time into sensor registers
    // do not write if nothing has changed
    if( CoarseIntegrationTime != pOV14825Ctx->OldCoarseIntegrationTime )
    {
        if( !pOV14825Ctx->GroupHold ) //don't touch if already held
        {
            result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0x00 ); //group 0 hold
            RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        }

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_EXPO_2, (CoarseIntegrationTime & 0x0000F000U) >> 12U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_EXPO_1, (CoarseIntegrationTime & 0x00000FF0U) >> 4U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_AEC_EXPO_0, (CoarseIntegrationTime & 0x0000000FU) << 4U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        if( !pOV14825Ctx->GroupHold )
        {
            result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0x10 ); //finish group 0
            RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0xa0 ); //and launch
            RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        }

        pOV14825Ctx->OldCoarseIntegrationTime = CoarseIntegrationTime;   // remember current integration time
        *pNumberOfFramesToSkip = 1U; //skip 1 frame
    }
    else
    {
        *pNumberOfFramesToSkip = 0U; //no frame skip
    }

    //if( FineIntegrationTime != pOV14825Ctx->OldFineIntegrationTime )
    //{
    //    result = OV14825_IsiRegWriteIss( pOV14825Ctx, ... , FineIntegrationTime );
    //    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    //    pOV14825Ctx->OldFineIntegrationTime = FineIntegrationTime; //remember current integration time
    //    *pNumberOfFramesToSkip = 1U; //skip 1 frame
    //}

    //calculate integration time actually set
    //pOV14825Ctx->AecCurIntegrationTime = ( ((float)CoarseIntegrationTime) * ((float)pOV14825Ctx->LineLengthPck) + ((float)FineIntegrationTime) ) / pOV14825Ctx->VtPixClkFreq;
    pOV14825Ctx->AecCurIntegrationTime = ((float)CoarseIntegrationTime) * ((float)pOV14825Ctx->LineLengthPck) / pOV14825Ctx->VtPixClkFreq;

    //return current state
    *pSetIntegrationTime = pOV14825Ctx->AecCurIntegrationTime;

    TRACE( OV14825_DEBUG, "%s: Ti=%f\n", __FUNCTION__, *pSetIntegrationTime );
    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}




/*****************************************************************************/
/**
 *          OV14825_IsiExposureControlIss
 *
 * @brief   Camera hardware dependent part of the exposure control loop.
 *          Calculates appropriate register settings from the new exposure
 *          values and writes them to the image sensor module.
 *
 * @param   handle                  OV14825 sensor instance handle
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
RESULT OV14825_IsiExposureControlIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               NewIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pNumberOfFramesToSkip == NULL)
            || (pSetGain == NULL)
            || (pSetIntegrationTime == NULL) )
    {
        TRACE( OV14825_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    TRACE( OV14825_DEBUG, "%s: g=%f, Ti=%f\n", __FUNCTION__, NewGain, NewIntegrationTime );

    result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0x00 ); //group 0 hold
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    pOV14825Ctx->GroupHold = true;

    result = OV14825_IsiSetIntegrationTimeIss( handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip );
    result = OV14825_IsiSetGainIss( handle, NewGain, pSetGain );

    result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0x10 ); //finish group 0
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_GROUP_ACCESS, 0xa0 ); //and launch
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    pOV14825Ctx->GroupHold = false;

    TRACE( OV14825_DEBUG, "%s: set: g=%f, Ti=%f, skip=%d\n", __FUNCTION__, *pSetGain, *pSetIntegrationTime, *pNumberOfFramesToSkip );
    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetCurrentExposureIss
 *
 * @brief   Returns the currently adjusted AE values
 *
 * @param   handle                  OV14825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV14825_IsiGetCurrentExposureIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetGain == NULL) || (pSetIntegrationTime == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain            = pOV14825Ctx->AecCurGain;
    *pSetIntegrationTime = pOV14825Ctx->AecCurIntegrationTime;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetResolutionIss
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
RESULT OV14825_IsiGetResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *pSetResolution
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetResolution == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetResolution = pOV14825Ctx->Config.Resolution;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetAfpsInfoHelperIss
 *
 * @brief   Calc AFPS sub resolution settings for the given resolution
 *
 * @param   pOV14825Ctx             OV14825 sensor instance (dummy!) context
 * @param   Resolution              Any supported resolution to query AFPS params for
 * @param   pAfpsInfo               Reference of AFPS info structure to write the results to
 * @param   AfpsStageIdx            Index of current AFPS stage to use
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetAfpsInfoHelperIss(
    OV14825_Context_t   *pOV14825Ctx,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo,
    uint32_t            AfpsStageIdx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT(pOV14825Ctx != NULL);
    DCT_ASSERT(pAfpsInfo != NULL);
    DCT_ASSERT(AfpsStageIdx <= ISI_NUM_AFPS_STAGES);

    // update resolution in copy of config in context
    pOV14825Ctx->Config.Resolution = Resolution;

    // tell sensor about that
    result = OV14825_SetupOutputWindow( pOV14825Ctx, &pOV14825Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV14825_ERROR, "%s: SetupOutputWindow failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // update limits & stuff (reset current & old settings)
    result = OV14825_AecSetModeParameters( pOV14825Ctx, &pOV14825Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV14825_ERROR, "%s: AecSetModeParameters failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // take over params
    pAfpsInfo->Stage[AfpsStageIdx].Resolution = Resolution;
    pAfpsInfo->Stage[AfpsStageIdx].MaxIntTime = pOV14825Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecMinGain           = pOV14825Ctx->AecMinGain;
    pAfpsInfo->AecMaxGain           = pOV14825Ctx->AecMaxGain;
    pAfpsInfo->AecMinIntTime        = pOV14825Ctx->AecMinIntegrationTime;
    pAfpsInfo->AecMaxIntTime        = pOV14825Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecSlowestResolution = Resolution;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV14825_IsiGetAfpsInfoIss
 *
 * @brief   Returns the possible AFPS sub resolution settings for the given resolution series
 *
 * @param   handle                  OV14825 sensor instance handle
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
RESULT OV14825_IsiGetAfpsInfoIss(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        TRACE( OV14825_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pAfpsInfo == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    // use currently configured resolution?
    if (Resolution == 0)
    {
        Resolution = pOV14825Ctx->Config.Resolution;
    }

    // prepare index
    uint32_t idx = 0;

    // set current resolution data in info struct
    pAfpsInfo->CurrResolution = pOV14825Ctx->Config.Resolution;
    pAfpsInfo->CurrMinIntTime = pOV14825Ctx->AecMinIntegrationTime;
    pAfpsInfo->CurrMaxIntTime = pOV14825Ctx->AecMaxIntegrationTime;

    // allocate dummy context used for Afps parameter calculation as a copy of current context
    OV14825_Context_t *pDummyCtx = (OV14825_Context_t*) malloc( sizeof(OV14825_Context_t) );
    if ( pDummyCtx == NULL )
    {
        TRACE( OV14825_ERROR,  "%s: Can't allocate dummy ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    *pDummyCtx = *pOV14825Ctx;

    // set AFPS mode in dummy context
    pDummyCtx->isAfpsRun = BOOL_TRUE;

#define AFPSCHECKANDADD(_res_) \
    if ( (pOV14825Ctx->Config.AfpsResolutions & (_res_)) != 0 ) \
    { \
        RESULT lres = OV14825_IsiGetAfpsInfoHelperIss( pDummyCtx, _res_, pAfpsInfo, idx ); \
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
            TRACE( OV14825_DEBUG,  "%s: Resolution %08x not supported by AFPS\n",  __FUNCTION__, Resolution );
            result = RET_NOTSUPP;
            break;

        // 1080p15 series in ascending integration time order (most probably the same as descending frame rate order)
        case ISI_RES_TV1080P15:
        case ISI_RES_TV1080P10:
        case ISI_RES_TV1080P5:
            AFPSCHECKANDADD( ISI_RES_TV1080P15 );
            AFPSCHECKANDADD( ISI_RES_TV1080P10 );
            AFPSCHECKANDADD( ISI_RES_TV1080P5  );
            break;

        // check next series here...
    }

    // release dummy context again
    free(pDummyCtx);

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetCalibKFactor
 *
 * @brief   Returns the OV14825 specific K-Factor
 *
 * @param   handle       OV14825 sensor instance handle
 * @param   pIsiKFactor  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetCalibKFactor
(
    IsiSensorHandle_t   handle,
    Isi1x1FloatMatrix_t **pIsiKFactor
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiKFactor == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiKFactor = (Isi1x1FloatMatrix_t *)&OV14825_KFactor;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV14825_IsiGetCalibPcaMatrix
 *
 * @brief   Returns the OV14825 specific PCA-Matrix
 *
 * @param   handle          OV14825 sensor instance handle
 * @param   pIsiPcaMatrix   Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetCalibPcaMatrix
(
    IsiSensorHandle_t   handle,
    Isi3x2FloatMatrix_t **pIsiPcaMatrix
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiPcaMatrix == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiPcaMatrix = (Isi3x2FloatMatrix_t *)&OV14825_PCAMatrix;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns the sensor specific SvdMean-Vector
 *
 * @param   handle              OV14825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetCalibSvdMeanValue
(
    IsiSensorHandle_t   handle,
    Isi3x1FloatMatrix_t **pIsiSvdMeanValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSvdMeanValue == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiSvdMeanValue = (Isi3x1FloatMatrix_t *)&OV14825_SVDMeanValue;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns a pointer to the sensor specific centerline, a straight
 *          line in Hesse normal form in Rg/Bg colorspace
 *
 * @param   handle              OV14825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetCalibCenterLine
(
    IsiSensorHandle_t   handle,
    IsiLine_t           **ptIsiCenterLine
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiCenterLine == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiCenterLine = (IsiLine_t*)&OV14825_CenterLine;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetCalibClipParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for Rg/Bg color
 *          space clipping
 *
 * @param   handle              OV14825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetCalibClipParam
(
    IsiSensorHandle_t   handle,
    IsiAwbClipParm_t    **pIsiClipParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiClipParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiClipParam = (IsiAwbClipParm_t *)&OV14825_AwbClipParm;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetCalibGlobalFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle              OV14825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetCalibGlobalFadeParam
(
    IsiSensorHandle_t       handle,
    IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiGlobalFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiGlobalFadeParam = (IsiAwbGlobalFadeParm_t *)&OV14825_AwbGlobalFadeParm;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetCalibFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for near white
 *          pixel parameter calculations
 *
 * @param   handle              OV14825 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiGetCalibFadeParam
(
    IsiSensorHandle_t   handle,
    IsiAwbFade2Parm_t   **ptIsiFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiFadeParam = (IsiAwbFade2Parm_t *)&OV14825_AwbFade2Parm;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV14825_IsiGetIlluProfile
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
static RESULT OV14825_IsiGetIlluProfile
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiIlluProfile_t    **ptIsiIlluProfile
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
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
        for ( i=0U; i<OV14825_ISIILLUPROFILES_DEFAULT; i++ )
        {
            if ( OV14825_IlluProfileDefault[i].id == CieProfile )
            {
                *ptIsiIlluProfile = &OV14825_IlluProfileDefault[i];
                break;
            }
        }

        result = ( *ptIsiIlluProfile != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
    }

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetLscMatrixTable
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
static RESULT OV14825_IsiGetLscMatrixTable
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiLscMatrixTable_t **pLscMatrixTable
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
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
                if ( ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P15 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P10 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P5  ) )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_A_1920x1080;
                }
                else if ( pOV14825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_A_4416x3312;
                }
                else
                {
                    TRACE( OV14825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F2:
            {
                if ( ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P15 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P10 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P5  ) )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_F2_1920x1080;
                }
                else if ( pOV14825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_F2_4416x3312;
                }
                else
                {
                    TRACE( OV14825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D50:
            {
                if ( ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P15 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P10 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P5  ) )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_D50_1920x1080;
                }
                else if ( pOV14825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_D50_4416x3312;
                }
                else
                {
                    TRACE( OV14825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D65:
            case ISI_CIEPROF_D75:
            {
                if ( ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P15 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P10 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P5  ) )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_D65_1920x1080;
                }
                else if ( pOV14825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_D65_4416x3312;
                }
                else
                {
                    TRACE( OV14825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F11:
            {
                if ( ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P15 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P10 )
                  || ( pOV14825Ctx->Config.Resolution == ISI_RES_TV1080P5  ) )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_F11_1920x1080;
                }
                else if ( pOV14825Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV14825_LscMatrixTable_CIE_F11_4416x3312;
                }
                else
                {
                    TRACE( OV14825_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            default:
            {
                TRACE( OV14825_ERROR, "%s: Illumination not supported\n", __FUNCTION__ );
                *pLscMatrixTable = NULL;
                break;
            }
        }

        result = ( *pLscMatrixTable != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
    }

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV14825_IsiMdiInitMotoDriveMds
 *
 * @brief   General initialisation tasks like I/O initialisation.
 *
 * @param   handle              OV14825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiMdiInitMotoDriveMds
(
    IsiSensorHandle_t   handle
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiMdiSetupMotoDrive
 *
 * @brief   Setup of the MotoDrive and return possible max step.
 *
 * @param   handle          OV14825 sensor instance handle
 *          pMaxStep        pointer to variable to receive the maximum
 *                          possible focus step
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiMdiSetupMotoDrive
(
    IsiSensorHandle_t   handle,
    uint32_t            *pMaxStep
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMaxStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pMaxStep = MAX_LOG;

    result = OV14825_IsiMdiFocusSet( handle, MAX_LOG );

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiMdiFocusSet
 *
 * @brief   Drives the lens system to a certain focus point.
 *
 * @param   handle          OV14825 sensor instance handle
 *          AbsStep         absolute focus point to apply
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiMdiFocusSet
(
    IsiSensorHandle_t   handle,
    const uint32_t      Position
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t nPosition;
    uint8_t  data[2] = { 0, 0 };

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* map 64 to 0 -> infinity */
    nPosition = ( Position >= MAX_LOG ) ? 0 : ( MAX_REG - (Position * 16U) );

    data[0] = (uint8_t)(0x40U | (( nPosition & 0x3F0U ) >> 4U));                 // PD,  1, D9..D4, see AD5820 datasheet
    data[1] = (uint8_t)( ((nPosition & 0x0FU) << 4U) | MDI_SLEW_RATE_CTRL );    // D3..D0, S3..S0

    TRACE( OV14825_DEBUG, "%s: value = %d, 0x%02x 0x%02x\n", __FUNCTION__, nPosition, data[0], data[1] );

    result = HalWriteI2CMem( pOV14825Ctx->IsiCtx.HalHandle,
                             pOV14825Ctx->IsiCtx.I2cAfBusNum,
                             pOV14825Ctx->IsiCtx.SlaveAfAddress,
                             0,
                             pOV14825Ctx->IsiCtx.NrOfAfAddressBytes,
                             data,
                             2U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiMdiFocusGet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          OV14825 sensor instance handle
 *          pAbsStep        pointer to a variable to receive the current
 *                          focus point
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiMdiFocusGet
(
    IsiSensorHandle_t   handle,
    uint32_t            *pAbsStep
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;
    uint8_t  data[2] = { 0, 0 };

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pAbsStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    result = HalReadI2CMem( pOV14825Ctx->IsiCtx.HalHandle,
                            pOV14825Ctx->IsiCtx.I2cAfBusNum,
                            pOV14825Ctx->IsiCtx.SlaveAfAddress,
                            0,
                            pOV14825Ctx->IsiCtx.NrOfAfAddressBytes,
                            data,
                            2U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV14825_DEBUG, "%s: value = 0x%02x 0x%02x\n", __FUNCTION__, data[0], data[1] );

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

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiMdiFocusCalibrate
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV14825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV14825_IsiMdiFocusCalibrate
(
    IsiSensorHandle_t   handle
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiActivateTestPattern
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV14825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV14825_IsiActivateTestPattern
(
    IsiSensorHandle_t   handle,
    const bool_t        enable
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t ulRegValue = 0;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( BOOL_TRUE == enable )
    {
        /* enable test-pattern */
        result = OV14825_IsiRegReadIss( pOV14825Ctx, OV14825_ISP_CTRL3D, &ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        ulRegValue |= ( 0x80U );

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_ISP_CTRL3D, ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }
    else
    {
        /* disable test-pattern */
        result = OV14825_IsiRegReadIss( pOV14825Ctx, OV14825_ISP_CTRL3D, &ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        ulRegValue &= ~( 0x80U );

        result = OV14825_IsiRegWriteIss( pOV14825Ctx, OV14825_ISP_CTRL3D, ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }

    pOV14825Ctx->TestPattern = enable;

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV14825_IsiGetSensorMipiInfoIss
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV14825 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV14825_IsiGetSensorMipiInfoIss
(
    IsiSensorHandle_t   handle,
    IsiSensorMipiInfo   *ptIsiSensorMipiInfo
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }


    if ( ptIsiSensorMipiInfo == NULL )
    {
        return ( result );
    }

    TRACE( OV14825_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV14825_IsiGetSensorIsiVersion
(  IsiSensorHandle_t   handle,
   unsigned int*     pVersion
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
    	TRACE( OV14825_ERROR, "%s: pOV14825Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pVersion == NULL)
	{
		TRACE( OV14825_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pVersion = CONFIG_ISI_VERSION;

	return result;
	
}

static RESULT OV14825_IsiGetSensorTuningXmlVersion
(  IsiSensorHandle_t   handle,
   char**     pTuningXmlVersion
)
{
    OV14825_Context_t *pOV14825Ctx = (OV14825_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV14825_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV14825Ctx == NULL )
    {
    	TRACE( OV14825_ERROR, "%s: pOV14825Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pTuningXmlVersion == NULL)
	{
		TRACE( OV14825_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pTuningXmlVersion = OV14825_NEWEST_TUNING_XML;
	return result;
}

/*****************************************************************************/
/**
 *          OV14825_IsiGetSensorIss
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
RESULT OV14825_IsiGetSensorIss
(
    IsiSensor_t *pIsiSensor
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV14825_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pIsiSensor != NULL )
    {
        pIsiSensor->pszName                             = OV14825_g_acName;
        pIsiSensor->pRegisterTable                      = OV14825_g_aRegDescription;
        pIsiSensor->pIsiSensorCaps                      = &OV14825_g_IsiSensorDefaultConfig;
		pIsiSensor->pIsiGetSensorIsiVer					= OV14825_IsiGetSensorIsiVersion;//oyyf
		pIsiSensor->pIsiGetSensorTuningXmlVersion		= OV14825_IsiGetSensorTuningXmlVersion;//oyyf

        pIsiSensor->pIsiCreateSensorIss                 = OV14825_IsiCreateSensorIss;
        pIsiSensor->pIsiReleaseSensorIss                = OV14825_IsiReleaseSensorIss;
        pIsiSensor->pIsiGetCapsIss                      = OV14825_IsiGetCapsIss;
        pIsiSensor->pIsiSetupSensorIss                  = OV14825_IsiSetupSensorIss;
        pIsiSensor->pIsiChangeSensorResolutionIss       = OV14825_IsiChangeSensorResolutionIss;
        pIsiSensor->pIsiSensorSetStreamingIss           = OV14825_IsiSensorSetStreamingIss;
        pIsiSensor->pIsiSensorSetPowerIss               = OV14825_IsiSensorSetPowerIss;
        pIsiSensor->pIsiCheckSensorConnectionIss        = OV14825_IsiCheckSensorConnectionIss;
        pIsiSensor->pIsiGetSensorRevisionIss            = OV14825_IsiGetSensorRevisionIss;
        pIsiSensor->pIsiRegisterReadIss                 = OV14825_IsiRegReadIss;
        pIsiSensor->pIsiRegisterWriteIss                = OV14825_IsiRegWriteIss;

        /* AEC functions */
        pIsiSensor->pIsiExposureControlIss              = OV14825_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = OV14825_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = OV14825_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetCurrentExposureIss           = OV14825_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetGainIss                      = OV14825_IsiGetGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = OV14825_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = OV14825_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = OV14825_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = OV14825_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = OV14825_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetResolutionIss                = OV14825_IsiGetResolutionIss;
        pIsiSensor->pIsiGetAfpsInfoIss                  = OV14825_IsiGetAfpsInfoIss;

        /* AWB specific functions */
        pIsiSensor->pIsiGetCalibKFactor                 = OV14825_IsiGetCalibKFactor;
        pIsiSensor->pIsiGetCalibPcaMatrix               = OV14825_IsiGetCalibPcaMatrix;
        pIsiSensor->pIsiGetCalibSvdMeanValue            = OV14825_IsiGetCalibSvdMeanValue;
        pIsiSensor->pIsiGetCalibCenterLine              = OV14825_IsiGetCalibCenterLine;
        pIsiSensor->pIsiGetCalibClipParam               = OV14825_IsiGetCalibClipParam;
        pIsiSensor->pIsiGetCalibGlobalFadeParam         = OV14825_IsiGetCalibGlobalFadeParam;
        pIsiSensor->pIsiGetCalibFadeParam               = OV14825_IsiGetCalibFadeParam;
        pIsiSensor->pIsiGetIlluProfile                  = OV14825_IsiGetIlluProfile;
        pIsiSensor->pIsiGetLscMatrixTable               = OV14825_IsiGetLscMatrixTable;

        /* AF functions */
        pIsiSensor->pIsiMdiInitMotoDriveMds             = OV14825_IsiMdiInitMotoDriveMds;
        pIsiSensor->pIsiMdiSetupMotoDrive               = OV14825_IsiMdiSetupMotoDrive;
        pIsiSensor->pIsiMdiFocusSet                     = OV14825_IsiMdiFocusSet;
        pIsiSensor->pIsiMdiFocusGet                     = OV14825_IsiMdiFocusGet;
        pIsiSensor->pIsiMdiFocusCalibrate               = OV14825_IsiMdiFocusCalibrate;

        /* MIPI */
        pIsiSensor->pIsiGetSensorMipiInfoIss            = OV14825_IsiGetSensorMipiInfoIss;

        /* Testpattern */
        pIsiSensor->pIsiActivateTestPattern             = OV14825_IsiActivateTestPattern;
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( OV14825_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV14825_IsiGetSensorI2cInfo(sensor_i2c_info_t** pdata)
{
    sensor_i2c_info_t* pSensorI2cInfo;

    pSensorI2cInfo = ( sensor_i2c_info_t * )malloc ( sizeof (sensor_i2c_info_t) );

    if ( pSensorI2cInfo == NULL )
    {
        TRACE( OV14825_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pSensorI2cInfo, 0, sizeof( sensor_i2c_info_t ) );

    
    pSensorI2cInfo->i2c_addr = OV14825_SLAVE_ADDR;
    pSensorI2cInfo->soft_reg_addr = OV14825_SMIA_R0103;
    pSensorI2cInfo->soft_reg_value = 0x01U;
    pSensorI2cInfo->reg_size = 2;
    pSensorI2cInfo->value_size = 1;

    pSensorI2cInfo->resolution = ( ISI_RES_TV1080P15 | ISI_RES_TV1080P10 | ISI_RES_TV1080P5
                                          | ISI_RES_4416_3312);
    
    ListInit(&pSensorI2cInfo->chipid_info);

    sensor_chipid_info_t* pChipIDInfo_H = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_H )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_H, 0, sizeof(*pChipIDInfo_H) );    
    pChipIDInfo_H->chipid_reg_addr = OV14825_CHIP_ID_HIGH_BYTE;  
    pChipIDInfo_H->chipid_reg_value = OV14825_CHIP_ID_HIGH_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_H );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_H );

    sensor_chipid_info_t* pChipIDInfo_L = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_L )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_L, 0, sizeof(*pChipIDInfo_L) ); 
    pChipIDInfo_L->chipid_reg_addr = OV14825_CHIP_ID_LOW_BYTE;  
    pChipIDInfo_L->chipid_reg_value = OV14825_CHIP_ID_LOW_BYTE_DEFAULT;
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
    OV14825_IsiGetSensorIss,
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
    OV14825_IsiGetSensorI2cInfo,
};

