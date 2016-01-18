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
 * @file OV5640.c
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

#include "OV5640_priv.h"


#define CC_OFFSET_SCALING  2.0f
#define I2C_COMPLIANT_STARTBIT 1U

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( OV5640_INFO , "OV5640: ", INFO,    0U );
CREATE_TRACER( OV5640_WARN , "OV5640: ", WARNING, 1U );
CREATE_TRACER( OV5640_ERROR, "OV5640: ", ERROR,   1U );

CREATE_TRACER( OV5640_DEBUG, "OV5640: ", INFO,     1U );

CREATE_TRACER( OV5640_REG_INFO , "OV5640: ", INFO, 1);
CREATE_TRACER( OV5640_REG_DEBUG, "OV5640: ", INFO, 0U );

#define OV5640_SLAVE_ADDR       0x78U                           /**< i2c slave address of the OV5640 camera sensor */
#define OV5640_SLAVE_AF_ADDR    0x78U                           /**< i2c slave address of the OV5640 integrated AD5820 */

#define SOC_AF 1

#define AF_Address    0x3022U
#define AF_CMD        0x03U
#define AF_IDLE        0x08U
#define AF_ACK_Address 0x3023U
#define AF_ACK_VALUE   0x01U


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
const char OV5640_g_acName[] = "OV5640_ SOC_PARREL";
extern const IsiRegDescription_t OV5640_g_aRegDescription[];
extern const IsiRegDescription_t OV5640_g_svga[];
extern const IsiRegDescription_t OV5640_g_2592x1944[];
extern const IsiRegDescription_t OV5640_g_video_720p[];
extern const IsiRegDescription_t OV5640_af_firmware_new[];
extern const IsiRegDescription_t OV5640_af_init[];


const IsiSensorCaps_t OV5640_g_IsiSensorDefaultConfig;


#define OV5640_I2C_START_BIT        (I2C_COMPLIANT_STARTBIT)    // I2C bus start condition
#define OV5640_I2C_NR_ADR_BYTES     (2U)                        // 1 byte base address and 2 bytes sub address
#define OV5640_I2C_NR_DAT_BYTES     (1U)                        // 8 bit registers


/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static RESULT OV5640_IsiCreateSensorIss( IsiSensorInstanceConfig_t *pConfig );
static RESULT OV5640_IsiReleaseSensorIss( IsiSensorHandle_t handle );
static RESULT OV5640_IsiGetCapsIss( IsiSensorHandle_t handle, IsiSensorCaps_t *pIsiSensorCaps );
static RESULT OV5640_IsiSetupSensorIss( IsiSensorHandle_t handle, const IsiSensorConfig_t *pConfig );
static RESULT OV5640_IsiSensorSetStreamingIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV5640_IsiSensorSetPowerIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV5640_IsiCheckSensorConnectionIss( IsiSensorHandle_t handle );
static RESULT OV5640_IsiGetSensorRevisionIss( IsiSensorHandle_t handle, uint32_t *p_value);

static RESULT OV5640_IsiGetGainLimitsIss( IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain);
static RESULT OV5640_IsiGetIntegrationTimeLimitsIss( IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime );
static RESULT OV5640_IsiExposureControlIss( IsiSensorHandle_t handle, float NewGain, float NewIntegrationTime, uint8_t *pNumberOfFramesToSkip, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV5640_IsiGetCurrentExposureIss( IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV5640_IsiGetAfpsInfoIss ( IsiSensorHandle_t handle, uint32_t Resolution, IsiAfpsInfo_t* pAfpsInfo);
static RESULT OV5640_IsiGetGainIss( IsiSensorHandle_t handle, float *pSetGain );
static RESULT OV5640_IsiGetGainIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV5640_IsiSetGainIss( IsiSensorHandle_t handle, float NewGain, float *pSetGain );
static RESULT OV5640_IsiGetIntegrationTimeIss( IsiSensorHandle_t handle, float *pSetIntegrationTime );
static RESULT OV5640_IsiGetIntegrationTimeIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV5640_IsiSetIntegrationTimeIss( IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t *pNumberOfFramesToSkip );
static RESULT OV5640_IsiGetResolutionIss( IsiSensorHandle_t handle, uint32_t *pSetResolution );


static RESULT OV5640_IsiRegReadIss( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );
static RESULT OV5640_IsiRegWriteIss( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

static RESULT OV5640_IsiGetCalibKFactor( IsiSensorHandle_t handle, Isi1x1FloatMatrix_t **pIsiKFactor );
static RESULT OV5640_IsiGetCalibPcaMatrix( IsiSensorHandle_t   handle, Isi3x2FloatMatrix_t **pIsiPcaMatrix );
static RESULT OV5640_IsiGetCalibSvdMeanValue( IsiSensorHandle_t   handle, Isi3x1FloatMatrix_t **pIsiSvdMeanValue );
static RESULT OV5640_IsiGetCalibCenterLine( IsiSensorHandle_t   handle, IsiLine_t  **ptIsiCenterLine);
static RESULT OV5640_IsiGetCalibClipParam( IsiSensorHandle_t   handle, IsiAwbClipParm_t    **pIsiClipParam );
static RESULT OV5640_IsiGetCalibGlobalFadeParam( IsiSensorHandle_t       handle, IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam);
static RESULT OV5640_IsiGetCalibFadeParam( IsiSensorHandle_t   handle, IsiAwbFade2Parm_t   **ptIsiFadeParam);
static RESULT OV5640_IsiGetIlluProfile( IsiSensorHandle_t   handle, const uint32_t CieProfile, IsiIlluProfile_t **ptIsiIlluProfile );

static RESULT OV5640_IsiMdiInitMotoDriveMds( IsiSensorHandle_t handle );
static RESULT OV5640_IsiMdiSetupMotoDrive( IsiSensorHandle_t handle, uint32_t *pMaxStep );
static RESULT OV5640_IsiMdiFocusSet( IsiSensorHandle_t handle, const uint32_t Position );
static RESULT OV5640_IsiMdiFocusGet( IsiSensorHandle_t handle, uint32_t *pAbsStep );
static RESULT OV5640_IsiMdiFocusCalibrate( IsiSensorHandle_t handle );

static RESULT OV5640_IsiGetSensorMipiInfoIss( IsiSensorHandle_t handle, IsiSensorMipiInfo *ptIsiSensorMipiInfo);
static RESULT OV5640_IsiGetSensorIsiVersion(  IsiSensorHandle_t   handle, unsigned int* pVersion);

/*****************************************************************************/
/**
 *          OV5640_IsiCreateSensorIss
 *
 * @brief   This function creates a new OV5640 sensor instance handle.
 *
 * @param   pConfig     configuration structure to create the instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
static RESULT OV5640_IsiCreateSensorIss
(
    IsiSensorInstanceConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    OV5640_Context_t *pOV5640Ctx;

    TRACE( OV5640_INFO, "%s (---------enter-----------)\n", __FUNCTION__);

    if ( (pConfig == NULL) || (pConfig->pSensor ==NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    pOV5640Ctx = ( OV5640_Context_t * )malloc ( sizeof (OV5640_Context_t) );
    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pOV5640Ctx, 0, sizeof( OV5640_Context_t ) );

    result = HalAddRef( pConfig->HalHandle );
    if ( result != RET_SUCCESS )
    {
        free ( pOV5640Ctx );
        return ( result );
    }

    pOV5640Ctx->IsiCtx.HalHandle              = pConfig->HalHandle;
    pOV5640Ctx->IsiCtx.HalDevID               = pConfig->HalDevID;
    pOV5640Ctx->IsiCtx.I2cBusNum              = pConfig->I2cBusNum;
    pOV5640Ctx->IsiCtx.SlaveAddress           = ( pConfig->SlaveAddr == 0 ) ? OV5640_SLAVE_ADDR : pConfig->SlaveAddr;
    pOV5640Ctx->IsiCtx.NrOfAddressBytes       = 2U;

    pOV5640Ctx->IsiCtx.I2cAfBusNum            = 3;//pConfig->I2cAfBusNum;
    pOV5640Ctx->IsiCtx.SlaveAfAddress         = ( pConfig->SlaveAfAddr == 0 ) ? OV5640_SLAVE_AF_ADDR : pConfig->SlaveAfAddr;
    pOV5640Ctx->IsiCtx.NrOfAfAddressBytes     = 2U;

    pOV5640Ctx->IsiCtx.pSensor                = pConfig->pSensor;

    pOV5640Ctx->Configured             = BOOL_FALSE;
    pOV5640Ctx->Streaming              = BOOL_FALSE;
    pOV5640Ctx->TestPattern            = BOOL_FALSE;
    pOV5640Ctx->isAfpsRun              = BOOL_FALSE;

    pConfig->hSensor = ( IsiSensorHandle_t )pOV5640Ctx;

    result = HalSetCamConfig( pOV5640Ctx->IsiCtx.HalHandle, pOV5640Ctx->IsiCtx.HalDevID, true, true, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    result = HalSetClock( pOV5640Ctx->IsiCtx.HalHandle, pOV5640Ctx->IsiCtx.HalDevID, 10000000U);
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiReleaseSensorIss
 *
 * @brief   This function destroys/releases an OV5640 sensor instance.
 *
 * @param   handle      OV5640 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
static RESULT OV5640_IsiReleaseSensorIss
(
    IsiSensorHandle_t handle
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    (void)OV5640_IsiSensorSetStreamingIss( pOV5640Ctx, BOOL_FALSE );
    (void)OV5640_IsiSensorSetPowerIss( pOV5640Ctx, BOOL_FALSE );

    (void)HalDelRef( pOV5640Ctx->IsiCtx.HalHandle );

    MEMSET( pOV5640Ctx, 0, sizeof( OV5640_Context_t ) );
    free ( pOV5640Ctx );

    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetCapsIss
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

static RESULT OV5640_IsiGetCapsIssInternal
(
    IsiSensorCaps_t   *pIsiSensorCaps    
)
{

    RESULT result = RET_SUCCESS;


    if ( pIsiSensorCaps == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        switch (pIsiSensorCaps->Index) 
        {
            case 0:
            {
                pIsiSensorCaps->Resolution = ISI_RES_2592_1944P7;
                break;
            }
            case 1:
            {
                //remove svga,preview is from 720p.just for increase fps.
                pIsiSensorCaps->Resolution = ISI_RES_TV720P30;
                break;
            }
            default:
            {
                result = RET_OUTOFRANGE;
                goto end;
            }

        }
    
        pIsiSensorCaps->BusWidth        = ISI_BUSWIDTH_12BIT;
        pIsiSensorCaps->Mode            = ISI_MODE_PICT|ISI_MODE_BT601;
        pIsiSensorCaps->FieldSelection  = ISI_FIELDSEL_BOTH;
        pIsiSensorCaps->YCSequence      = ISI_YCSEQ_YCBYCR|ISI_YCSEQ_CBYCRY;           
        pIsiSensorCaps->Conv422         = ISI_CONV422_NOCOSITED;
        pIsiSensorCaps->BPat            = ISI_BPAT_RGRGGBGB ;
        pIsiSensorCaps->HPol            = ISI_HPOL_REFPOS;
        pIsiSensorCaps->VPol            = ISI_VPOL_NEG;
        pIsiSensorCaps->Edge            = ISI_EDGE_RISING;
        pIsiSensorCaps->Bls             = ISI_BLS_OFF;
        pIsiSensorCaps->Gamma           = ISI_GAMMA_ON;
        pIsiSensorCaps->CConv           = ISI_CCONV_ON;


        pIsiSensorCaps->BLC             = ( ISI_BLC_AUTO );
        pIsiSensorCaps->AGC             = ( ISI_AGC_AUTO );
        pIsiSensorCaps->AWB             = ( ISI_AWB_AUTO );
        pIsiSensorCaps->AEC             = ( ISI_AEC_AUTO );
        pIsiSensorCaps->DPCC            = ( ISI_DPCC_AUTO );

        pIsiSensorCaps->DwnSz           = ISI_DWNSZ_SUBSMPL;
        pIsiSensorCaps->CieProfile      = 0;
        pIsiSensorCaps->SmiaMode        = ISI_SMIA_OFF;
        pIsiSensorCaps->MipiMode        = ISI_MIPI_OFF;
        pIsiSensorCaps->AfpsResolutions = ( ISI_AFPS_NOTSUPP );
        pIsiSensorCaps->SensorOutputMode = ISI_SENSOR_OUTPUT_MODE_YUV;
    }
end:

    return ( result );
}

 
static RESULT OV5640_IsiGetCapsIss
(
    IsiSensorHandle_t handle,
    IsiSensorCaps_t   *pIsiSensorCaps
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    result = OV5640_IsiGetCapsIssInternal(pIsiSensorCaps);
    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_g_IsiSensorDefaultConfig
 *
 * @brief   recommended default configuration for application use via call
 *          to IsiGetSensorIss()
 *
 *****************************************************************************/
const IsiSensorCaps_t OV5640_g_IsiSensorDefaultConfig =
{
    ISI_BUSWIDTH_12BIT,         // BusWidth
    ISI_MODE_BT601,              // MIPI
    ISI_FIELDSEL_BOTH,          // FieldSel
    ISI_YCSEQ_CBYCRY,           // YCSeq
    ISI_CONV422_NOCOSITED,      // Conv422
    ISI_BPAT_RGRGGBGB,//ISI_BPAT_BGBGGRGR,          // BPat
    ISI_HPOL_REFPOS,            // HPol
    ISI_VPOL_NEG,               // VPol
    ISI_EDGE_RISING, //ISI_EDGE_RISING,            // Edge
    ISI_BLS_OFF,                // Bls
    ISI_GAMMA_ON,              // Gamma
    ISI_CCONV_ON,              // CConv
    ISI_RES_SVGAP30,          // Res
    ISI_DWNSZ_SUBSMPL,          // DwnSz
    ISI_BLC_AUTO,               // BLC
    ISI_AGC_AUTO,                // AGC
    ISI_AWB_AUTO,                // AWB
    ISI_AEC_AUTO,                // AEC
    ISI_DPCC_AUTO,               // DPCC
    0,            // CieProfile, this is also used as start profile for AWB (if not altered by menu settings)
    ISI_SMIA_OFF,               // SmiaMode
    ISI_MIPI_OFF,       // MipiMode
    ISI_AFPS_NOTSUPP,           // AfpsResolutions
    ISI_SENSOR_OUTPUT_MODE_YUV,
    0,
};



/*****************************************************************************/
/**
 *          OV5640_SetupOutputFormat
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV5640 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5640_SetupOutputFormat
(
    OV5640_Context_t       *pOV5640Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;
    return result;

    TRACE( OV5640_INFO, "%s%s (enter)\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );

    /* bus-width */
    switch ( pConfig->BusWidth )        /* only ISI_BUSWIDTH_12BIT supported, no configuration needed here */
    {
        case ISI_BUSWIDTH_10BIT:
        case ISI_BUSWIDTH_8BIT_ZZ:
        case ISI_BUSWIDTH_12BIT:
        {
            break;
        }

        default:
        {
            TRACE( OV5640_ERROR, "%s%s: bus width not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* mode */
    switch ( pConfig->Mode )            /* only ISI_MODE_BAYER supported, no configuration needed here */
    {
        case( ISI_MODE_MIPI ):
        case ISI_MODE_BAYER:
        case ISI_MODE_BT601:
        case ISI_MODE_PICT:
        case  ISI_MODE_DATA:
        {
            break;
        }

        default:
        {
            TRACE( OV5640_ERROR, "%s%s: mode not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5640_ERROR, "%s%s: field selection not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

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
        case ISI_CONV422_COSITED:
        case ISI_CONV422_INTER:
        {
            break;
        }

        default:
        {
            TRACE( OV5640_ERROR, "%s%s: 422 conversion not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5640_ERROR, "%s%s: bayer pattern not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5640_ERROR, "%s%s: HPol not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5640_ERROR, "%s%s: VPol not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5640_ERROR, "%s%s:  edge mode not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* gamma */
    switch ( pConfig->Gamma )           /* only ISI_GAMMA_OFF supported, no configuration needed */
    {
        case ISI_GAMMA_ON:
        case ISI_GAMMA_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV5640_ERROR, "%s%s:  gamma not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* color conversion */
    switch ( pConfig->CConv )           /* only ISI_CCONV_OFF supported, no configuration needed */
    {
        case ISI_CCONV_OFF:
        case ISI_CCONV_ON:
        {
            break;
        }

        default:
        {
            TRACE( OV5640_ERROR, "%s%s: color conversion not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5640_ERROR, "%s%s: SMIA mode not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV5640_ERROR, "%s%s: MIPI mode not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
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
            //TRACE( OV5640_ERROR, "%s%s: AFPS not supported\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"" );
            //return ( RET_NOTSUPP );
        }
    }

    TRACE( OV5640_INFO, "%s%s (exit)\n", __FUNCTION__, pOV5640Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}

int OV5640_get_PCLK( OV5640_Context_t *pOV5640Ctx, int XVCLK)
{
    // calculate PCLK
    
    return 0;
}

/*****************************************************************************/
/**
 *          OV5640_SetupOutputWindow
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV5640 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_SetupOutputWindow
(
    OV5640_Context_t        *pOV5640Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result     = RET_SUCCESS;
    static uint32_t oldRes;

        /* resolution */
    switch ( pConfig->Resolution )
    {
        case ISI_RES_SVGAP30:
        {
            if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV5640Ctx,OV5640_g_svga)) != RET_SUCCESS){
                TRACE( OV5640_ERROR, "%s: failed to set  ISI_RES_SVGAP30 \n", __FUNCTION__ );
            }else{

                TRACE( OV5640_INFO, "%s: success to set  ISI_RES_SVGAP30 \n", __FUNCTION__ );
            }
            break;
        }
        /*case ISI_RES_1600_1200:
        {
            if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV5640Ctx,OV5640_g_1600x1200)) != RET_SUCCESS){
                TRACE( OV5640_ERROR, "%s: failed to set  ISI_RES_1600_1200 \n", __FUNCTION__ );
            }else{

                TRACE( OV5640_INFO, "%s: success to set  ISI_RES_1600_1200  \n", __FUNCTION__ );
            }
            break;
        }*/
        case ISI_RES_TV720P30:
        {
            if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV5640Ctx,OV5640_g_video_720p)) != RET_SUCCESS){
                TRACE( OV5640_ERROR, "%s: failed to set  ISI_RES_TV720P30 \n", __FUNCTION__ );
            }else{

                TRACE( OV5640_INFO, "%s: success to set  ISI_RES_TV720P30  \n", __FUNCTION__ );
            }
            break;
        }
		case ISI_RES_2592_1944P7:
        {
            if(oldRes == ISI_RES_TV720P30){
                if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV5640Ctx,OV5640_g_svga)) != RET_SUCCESS){
                    TRACE( OV5640_ERROR, "%s: failed to set  ISI_RES_SVGAP30 \n", __FUNCTION__ );
                }else{

                    TRACE( OV5640_INFO, "%s: success to set  ISI_RES_SVGAP30 \n", __FUNCTION__ );
                }
            }
                
            if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV5640Ctx,OV5640_g_2592x1944)) != RET_SUCCESS){
                TRACE( OV5640_ERROR, "%s: failed to set  ISI_RES_2592_1944P7 \n", __FUNCTION__ );
            }else{

                TRACE( OV5640_INFO, "%s: success to set  ISI_RES_2592_1944P7  \n", __FUNCTION__ );
            }
            break;
        }
        default:
        {
            TRACE( OV5640_ERROR, "%s: Resolution not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    oldRes = pConfig->Resolution;

    return ( result );
}




/*****************************************************************************/
/**
 *          OV5640_SetupImageControl
 *
 * @brief   Sets the image control functions (BLC, AGC, AWB, AEC, DPCC ...)
 *
 * @param   handle      OV5640 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5640_SetupImageControl
(
    OV5640_Context_t        *pOV5640Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0U;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);


    return ( result );
}


/*****************************************************************************/
/**
 *          OV5640_AecSetModeParameters
 *
 * @brief   This function fills in the correct parameters in OV5640-Instances
 *          according to AEC mode selection in IsiSensorConfig_t.
 *
 * @note    It is assumed that IsiSetupOutputWindow has been called before
 *          to fill in correct values in instance structure.
 *
 * @param   handle      OV5640 context
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_AecSetModeParameters
(
    OV5640_Context_t       *pOV5640Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    return ( result );
}


/*****************************************************************************/
/**
 *          OV5640_IsiSetupSensorIss
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV5640 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiSetupSensorIss
(
    IsiSensorHandle_t       handle,
    const IsiSensorConfig_t *pConfig
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid configuration (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    if ( pOV5640Ctx->Streaming != BOOL_FALSE )
    {
        return RET_WRONG_STATE;
    }

    MEMCPY( &pOV5640Ctx->Config, pConfig, sizeof( IsiSensorConfig_t ) );

    /* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */
    result = IsiRegDefaultsApply( pOV5640Ctx, OV5640_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
		

    /* sleep a while, that sensor can take over new default values */
    osSleep( 10 );
    #if 0


    /* 3.) verify default values to make sure everything has been written correctly as expected */
    result = IsiRegDefaultsVerify( pOV5640Ctx, OV5640_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    // output of pclk for measurement (only debugging)
    result = OV5640_IsiRegWriteIss( pOV5640Ctx, 0x3009U, 0x10U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    #endif

    /* 4.) setup output format (RAW10|RAW12) */
    result = OV5640_SetupOutputFormat( pOV5640Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5640_ERROR, "%s: SetupOutputFormat failed.\n", __FUNCTION__);
        return ( result );
    }

    /* 5.) setup output window */
    result = OV5640_SetupOutputWindow( pOV5640Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5640_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV5640_SetupImageControl( pOV5640Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5640_ERROR, "%s: SetupImageControl failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV5640_AecSetModeParameters( pOV5640Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5640_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
        return ( result );
    }
#if 0
    result = IsiRegDefaultsApply( pOV5640Ctx, OV5640_af_firmware);
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5640_ERROR, "%s: Download OV5640_af_firmware failed\n", __FUNCTION__ );
        //return ( result );
    }
    
    result = IsiRegDefaultsApply( pOV5640Ctx, OV5640_af_init);
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5640_ERROR, "%s: Download OV5640_af_init failed\n", __FUNCTION__ );
        //return ( result );
    }
    
    //osSleep( 1000 );
    uint32_t value;
    result = IsiReadRegister( pOV5640Ctx, 0x3023, &value );
    if(value != 0x0){
    		TRACE( OV5640_ERROR, "%s: value:%d ;read TAG failed\n", __FUNCTION__,value);
    }
    
#endif
    if (result == RET_SUCCESS)
    {
        pOV5640Ctx->Configured = BOOL_TRUE;
    }
    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiChangeSensorResolutionIss
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
static RESULT OV5640_IsiChangeSensorResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (pNumberOfFramesToSkip == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pOV5640Ctx->Configured != BOOL_TRUE) || (pOV5640Ctx->Streaming != BOOL_FALSE) )
    {
        return RET_WRONG_STATE;
    }

    IsiSensorCaps_t Caps;    
    Caps.Index = 0;
    Caps.Resolution = 0;
    while (OV5640_IsiGetCapsIss( handle, &Caps) == RET_SUCCESS) {
        if (Resolution == Caps.Resolution) {            
            break;
        }
        Caps.Index++;
    }

    if ( (Resolution & Caps.Resolution) == 0 )
    {
        return RET_OUTOFRANGE;
    }

    if ( Resolution == pOV5640Ctx->Config.Resolution )
    {
        // well, no need to worry
        *pNumberOfFramesToSkip = 0;
    }
    else
    {
        // change resolution
        char *szResName = NULL;
        result = IsiGetResolutionName( Resolution, &szResName );
        TRACE( OV5640_DEBUG, "%s: NewRes=0x%08x (%s)\n", __FUNCTION__, Resolution, szResName);

        // update resolution in copy of config in context
        pOV5640Ctx->Config.Resolution = Resolution;

        // tell sensor about that
        result = OV5640_SetupOutputWindow( pOV5640Ctx, &pOV5640Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV5640_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
            return ( result );
        }

        // remember old exposure values
        float OldGain = pOV5640Ctx->AecCurGain;
        float OldIntegrationTime = pOV5640Ctx->AecCurIntegrationTime;

        // update limits & stuff (reset current & old settings)
        result = OV5640_AecSetModeParameters( pOV5640Ctx, &pOV5640Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV5640_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
            return ( result );
        }

        // restore old exposure values (at least within new exposure values' limits)
        uint8_t NumberOfFramesToSkip = 0;
        float   DummySetGain;
        float   DummySetIntegrationTime;
        result = OV5640_IsiExposureControlIss( handle, OldGain, OldIntegrationTime, &NumberOfFramesToSkip, &DummySetGain, &DummySetIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV5640_ERROR, "%s: OV5640_IsiExposureControlIss failed.\n", __FUNCTION__);
            return ( result );
        }

        // return number of frames that aren't exposed correctly
        *pNumberOfFramesToSkip = 0;//NumberOfFramesToSkip + 1;
    }

    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiSensorSetStreamingIss
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
static RESULT OV5640_IsiSensorSetStreamingIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    uint32_t RegValue = 0;

    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pOV5640Ctx->Configured != BOOL_TRUE) || (pOV5640Ctx->Streaming == on) )
    {
        return RET_WRONG_STATE;
    }

    if (on == BOOL_TRUE)
    {
        /* enable streaming */
        // hkw add;
        result = OV5640_IsiRegWriteIss( handle, 0x0100, 0x1);
       //result = RET_SUCCESS;

    }
    else
    {
        /* disable streaming */
        result = OV5640_IsiRegWriteIss( handle, 0x0100, 0x0);
				//result = RET_SUCCESS;

    }

    if (result == RET_SUCCESS)
    {
        pOV5640Ctx->Streaming = on;
    }

    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiSensorSetPowerIss
 *
 * @brief   Performs the power-up/power-down sequence of the camera, if possible.
 *
 * @param   handle      OV5640 sensor instance handle
 * @param   on          new power state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiSensorSetPowerIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pOV5640Ctx->Configured = BOOL_FALSE;
    pOV5640Ctx->Streaming  = BOOL_FALSE;

    TRACE( OV5640_DEBUG, "%s power off \n", __FUNCTION__);
    result = HalSetPower( pOV5640Ctx->IsiCtx.HalHandle, pOV5640Ctx->IsiCtx.HalDevID, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV5640_DEBUG, "%s reset on\n", __FUNCTION__);
    result = HalSetReset( pOV5640Ctx->IsiCtx.HalHandle, pOV5640Ctx->IsiCtx.HalDevID, true );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    if (on == BOOL_TRUE)
    {
        TRACE( OV5640_DEBUG, "%s power on \n", __FUNCTION__);
        result = HalSetPower( pOV5640Ctx->IsiCtx.HalHandle, pOV5640Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV5640_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pOV5640Ctx->IsiCtx.HalHandle, pOV5640Ctx->IsiCtx.HalDevID, false );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV5640_DEBUG, "%s reset on \n", __FUNCTION__);
        result = HalSetReset( pOV5640Ctx->IsiCtx.HalHandle, pOV5640Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV5640_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pOV5640Ctx->IsiCtx.HalHandle, pOV5640Ctx->IsiCtx.HalDevID, false );

        osSleep( 50 );
    }

    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiCheckSensorConnectionIss
 *
 * @brief   Checks the I2C-Connection to sensor by reading sensor revision id.
 *
 * @param   handle      OV5640 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiCheckSensorConnectionIss
(
    IsiSensorHandle_t   handle
)
{
    uint32_t RevId;
    uint32_t value;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    RevId = OV5640_CHIP_ID_HIGH_BYTE_DEFAULT;
    RevId = (RevId << 16U) | (OV5640_CHIP_ID_MIDDLE_BYTE_DEFAULT<<8U);
    RevId = RevId | OV5640_CHIP_ID_LOW_BYTE_DEFAULT;

    result = OV5640_IsiGetSensorRevisionIss( handle, &value );
    //if ( (result != RET_SUCCESS) || (RevId != value) )
    if ( (result != RET_SUCCESS) || ((RevId & 0x00ffff) != (value & 0x00ffff)) )
    {
        TRACE( OV5640_ERROR, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );
        return ( RET_FAILURE );
    }

    TRACE( OV5640_DEBUG, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );

    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetSensorRevisionIss
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
static RESULT OV5640_IsiGetSensorRevisionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    uint32_t data;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *p_value = 0U;
    result = OV5640_IsiRegReadIss ( handle, OV5640_CHIP_ID_HIGH_BYTE, &data );
    *p_value = ( (data & 0xFF) << 16U );
    result = OV5640_IsiRegReadIss ( handle, OV5640_CHIP_ID_MIDDLE_BYTE, &data );
    *p_value |= ( (data & 0xFF) << 8U );
    result = OV5640_IsiRegReadIss ( handle, OV5640_CHIP_ID_LOW_BYTE, &data );
    *p_value |= ( (data & 0xFF));

    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiRegReadIss
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
static RESULT OV5640_IsiRegReadIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

  //  TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

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
        uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, OV5640_g_aRegDescription );
        if ( !NrOfBytes )
        {
            NrOfBytes = 1;
        }
 //       TRACE( OV5640_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);

        *p_value = 0;
        result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
    }

  //  TRACE( OV5640_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiRegWriteIss
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
static RESULT OV5640_IsiRegWriteIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NrOfBytes;

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }
		OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;
    NrOfBytes = IsiGetNrDatBytesIss( address, OV5640_g_aRegDescription );
    if ( !NrOfBytes )
    {
        NrOfBytes = 1;
    }
    TRACE( OV5640_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x 0x%08x)\n", __FUNCTION__, NrOfBytes, address, value);

    //result = IsiI2cWriteSensorRegister( handle, address, (uint8_t *)(&value), NrOfBytes, BOOL_TRUE );
		result = HalWriteI2CMem_Rate( pOV5640Ctx->IsiCtx.HalHandle,
                             pOV5640Ctx->IsiCtx.I2cBusNum,
                             pOV5640Ctx->IsiCtx.SlaveAddress,
                             address,
                             pOV5640Ctx->IsiCtx.NrOfAddressBytes,
                             (uint8_t *)(&value),
                             1U,
			     									 50); 
    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetGainLimitsIss
 *
 * @brief   Returns the exposure minimal and maximal values of an
 *          OV5640 instance
 *
 * @param   handle       OV5640 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetGainLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinGain,
    float               *pMaxGain
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinGain == NULL) || (pMaxGain == NULL) )
    {
        TRACE( OV5640_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinGain = pOV5640Ctx->AecMinGain;
    *pMaxGain = pOV5640Ctx->AecMaxGain;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetIntegrationTimeLimitsIss
 *
 * @brief   Returns the minimal and maximal integration time values of an
 *          OV5640 instance
 *
 * @param   handle       OV5640 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetIntegrationTimeLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinIntegrationTime,
    float               *pMaxIntegrationTime
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL) )
    {
        TRACE( OV5640_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinIntegrationTime = pOV5640Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pOV5640Ctx->AecMaxIntegrationTime;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV5640_IsiGetGainIss
 *
 * @brief   Reads gain values from the image sensor module.
 *
 * @param   handle                  OV5640 sensor instance handle
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5640_IsiGetGainIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain = pOV5640Ctx->AecCurGain;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetGainIncrementIss
 *
 * @brief   Get smallest possible gain increment.
 *
 * @param   handle                  OV5640 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5640_IsiGetGainIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV5640Ctx->AecGainIncrement;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiSetGainIss
 *
 * @brief   Writes gain values to the image sensor module.
 *          Updates current gain and exposure in sensor struct/state.
 *
 * @param   handle                  OV5640 sensor instance handle
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
RESULT OV5640_IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint16_t usGain = 0;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        TRACE( OV5640_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

  
    if( NewGain < pOV5640Ctx->AecMinGain ) NewGain = pOV5640Ctx->AecMinGain;
    if( NewGain > pOV5640Ctx->AecMaxGain ) NewGain = pOV5640Ctx->AecMaxGain;

    usGain = (uint16_t)NewGain;

    // write new gain into sensor registers, do not write if nothing has changed
    if( (usGain != pOV5640Ctx->OldGain) )
    {

        pOV5640Ctx->OldGain = usGain;
    }

    //calculate gain actually set
    pOV5640Ctx->AecCurGain = NewGain;

    //return current state
    *pSetGain = pOV5640Ctx->AecCurGain;
    TRACE( OV5640_DEBUG, "%s: g=%f\n", __FUNCTION__, *pSetGain );

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetIntegrationTimeIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  OV5640 sensor instance handle
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5640_IsiGetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               *pSetIntegrationTime
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetIntegrationTime == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetIntegrationTime = pOV5640Ctx->AecCurIntegrationTime;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetIntegrationTimeIncrementIss
 *
 * @brief   Get smallest possible integration time increment.
 *
 * @param   handle                  OV5640 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5640_IsiGetIntegrationTimeIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV5640Ctx->AecIntegrationTimeIncrement;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiSetIntegrationTimeIss
 *
 * @brief   Writes gain and integration time values to the image sensor module.
 *          Updates current integration time and exposure in sensor
 *          struct/state.
 *
 * @param   handle                  OV5640 sensor instance handle
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
RESULT OV5640_IsiSetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               NewIntegrationTime,
    float               *pSetIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV5640_DEBUG, "%s: Ti=%f\n", __FUNCTION__, *pSetIntegrationTime );
    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}




/*****************************************************************************/
/**
 *          OV5640_IsiExposureControlIss
 *
 * @brief   Camera hardware dependent part of the exposure control loop.
 *          Calculates appropriate register settings from the new exposure
 *          values and writes them to the image sensor module.
 *
 * @param   handle                  OV5640 sensor instance handle
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
RESULT OV5640_IsiExposureControlIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               NewIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pNumberOfFramesToSkip == NULL)
            || (pSetGain == NULL)
            || (pSetIntegrationTime == NULL) )
    {
        TRACE( OV5640_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    TRACE( OV5640_DEBUG, "%s: g=%f, Ti=%f\n", __FUNCTION__, NewGain, NewIntegrationTime );


    result = OV5640_IsiSetIntegrationTimeIss( handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip );
    result = OV5640_IsiSetGainIss( handle, NewGain, pSetGain );

    TRACE( OV5640_DEBUG, "%s: set: g=%f, Ti=%f, skip=%d\n", __FUNCTION__, *pSetGain, *pSetIntegrationTime, *pNumberOfFramesToSkip );
    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetCurrentExposureIss
 *
 * @brief   Returns the currently adjusted AE values
 *
 * @param   handle                  OV5640 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV5640_IsiGetCurrentExposureIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetGain == NULL) || (pSetIntegrationTime == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain            = pOV5640Ctx->AecCurGain;
    *pSetIntegrationTime = pOV5640Ctx->AecCurIntegrationTime;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetResolutionIss
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
RESULT OV5640_IsiGetResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *pSetResolution
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetResolution == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetResolution = pOV5640Ctx->Config.Resolution;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetAfpsInfoHelperIss
 *
 * @brief   Calc AFPS sub resolution settings for the given resolution
 *
 * @param   pOV5640Ctx             OV5640 sensor instance (dummy!) context
 * @param   Resolution              Any supported resolution to query AFPS params for
 * @param   pAfpsInfo               Reference of AFPS info structure to write the results to
 * @param   AfpsStageIdx            Index of current AFPS stage to use
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetAfpsInfoHelperIss(
    OV5640_Context_t   *pOV5640Ctx,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo,
    uint32_t            AfpsStageIdx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT(pOV5640Ctx != NULL);
    DCT_ASSERT(pAfpsInfo != NULL);
    DCT_ASSERT(AfpsStageIdx <= ISI_NUM_AFPS_STAGES);

    // update resolution in copy of config in context
    pOV5640Ctx->Config.Resolution = Resolution;

    // tell sensor about that
    result = OV5640_SetupOutputWindow( pOV5640Ctx, &pOV5640Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5640_ERROR, "%s: SetupOutputWindow failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // update limits & stuff (reset current & old settings)
    result = OV5640_AecSetModeParameters( pOV5640Ctx, &pOV5640Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV5640_ERROR, "%s: AecSetModeParameters failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // take over params
    pAfpsInfo->Stage[AfpsStageIdx].Resolution = Resolution;
    pAfpsInfo->Stage[AfpsStageIdx].MaxIntTime = pOV5640Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecMinGain           = pOV5640Ctx->AecMinGain;
    pAfpsInfo->AecMaxGain           = pOV5640Ctx->AecMaxGain;
    pAfpsInfo->AecMinIntTime        = pOV5640Ctx->AecMinIntegrationTime;
    pAfpsInfo->AecMaxIntTime        = pOV5640Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecSlowestResolution = Resolution;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV5640_IsiGetAfpsInfoIss
 *
 * @brief   Returns the possible AFPS sub resolution settings for the given resolution series
 *
 * @param   handle                  OV5640 sensor instance handle
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
RESULT OV5640_IsiGetAfpsInfoIss(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        TRACE( OV5640_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pAfpsInfo == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    // use currently configured resolution?
    if (Resolution == 0)
    {
        Resolution = pOV5640Ctx->Config.Resolution;
    }

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetCalibKFactor
 *
 * @brief   Returns the OV5640 specific K-Factor
 *
 * @param   handle       OV5640 sensor instance handle
 * @param   pIsiKFactor  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetCalibKFactor
(
    IsiSensorHandle_t   handle,
    Isi1x1FloatMatrix_t **pIsiKFactor
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiKFactor == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiKFactor = NULL;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV5640_IsiGetCalibPcaMatrix
 *
 * @brief   Returns the OV5640 specific PCA-Matrix
 *
 * @param   handle          OV5640 sensor instance handle
 * @param   pIsiPcaMatrix   Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetCalibPcaMatrix
(
    IsiSensorHandle_t   handle,
    Isi3x2FloatMatrix_t **pIsiPcaMatrix
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiPcaMatrix == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiPcaMatrix = NULL;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns the sensor specific SvdMean-Vector
 *
 * @param   handle              OV5640 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetCalibSvdMeanValue
(
    IsiSensorHandle_t   handle,
    Isi3x1FloatMatrix_t **pIsiSvdMeanValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSvdMeanValue == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiSvdMeanValue = NULL;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns a pointer to the sensor specific centerline, a straight
 *          line in Hesse normal form in Rg/Bg colorspace
 *
 * @param   handle              OV5640 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetCalibCenterLine
(
    IsiSensorHandle_t   handle,
    IsiLine_t           **ptIsiCenterLine
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiCenterLine == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiCenterLine = NULL;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetCalibClipParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for Rg/Bg color
 *          space clipping
 *
 * @param   handle              OV5640 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetCalibClipParam
(
    IsiSensorHandle_t   handle,
    IsiAwbClipParm_t    **pIsiClipParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiClipParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiClipParam = NULL;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetCalibGlobalFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle              OV5640 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetCalibGlobalFadeParam
(
    IsiSensorHandle_t       handle,
    IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiGlobalFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiGlobalFadeParam = NULL;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetCalibFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for near white
 *          pixel parameter calculations
 *
 * @param   handle              OV5640 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiGetCalibFadeParam
(
    IsiSensorHandle_t   handle,
    IsiAwbFade2Parm_t   **ptIsiFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiFadeParam = NULL;

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV5640_IsiGetIlluProfile
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
static RESULT OV5640_IsiGetIlluProfile
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiIlluProfile_t    **ptIsiIlluProfile
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiIlluProfile == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetLscMatrixTable
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
static RESULT OV5640_IsiGetLscMatrixTable
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiLscMatrixTable_t **pLscMatrixTable
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pLscMatrixTable == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV5640_IsiMdiInitMotoDriveMds
 *
 * @brief   General initialisation tasks like I/O initialisation.
 *
 * @param   handle              OV5640 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiMdiInitMotoDriveMds
(
    IsiSensorHandle_t   handle
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);
#ifdef SOC_AF
    return RET_SOC_AF;
#else
    return ( result );
#endif
}



/*****************************************************************************/
/**
 *          OV5640_IsiMdiSetupMotoDrive
 *
 * @brief   Setup of the MotoDrive and return possible max step.
 *
 * @param   handle          OV5640 sensor instance handle
 *          pMaxStep        pointer to variable to receive the maximum
 *                          possible focus step
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiMdiSetupMotoDrive
(
    IsiSensorHandle_t   handle,
    uint32_t            *pMaxStep
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMaxStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pMaxStep = MAX_LOG;

    //result = OV5640_IsiMdiFocusSet( handle, MAX_LOG );

    //return MAX_LOG;
    TRACE( OV5640_INFO, "%s:this is SOC camera af (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiMdiFocusSet
 *
 * @brief   Drives the lens system to a certain focus point.
 *
 * @param   handle          OV5640 sensor instance handle
 *          AbsStep         absolute focus point to apply
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiMdiFocusSet
(
    IsiSensorHandle_t   handle,
    const uint32_t      Position
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;


    RESULT result = RET_SUCCESS;
    int cnt = 0;
    uint32_t data;
    uint8_t ack_cmd[1]={1};    
	TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* map 64 to 0 -> infinity */
/*
    result = HalWriteI2CMem( pOV5640Ctx->IsiCtx.HalHandle,
                             pOV5640Ctx->IsiCtx.I2cAfBusNum,
                             pOV5640Ctx->IsiCtx.SlaveAfAddress,
                             0,
                             pOV5640Ctx->IsiCtx.NrOfAfAddressBytes,
                             data,
                             2U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
*/

	//osSleep(1000);
	TRACE( OV5640_ERROR, "%s: Position:%d\n", __FUNCTION__,Position);
	if(Position == 1) { 
		result = HalWriteI2CMem_Rate( pOV5640Ctx->IsiCtx.HalHandle,
                             pOV5640Ctx->IsiCtx.I2cAfBusNum,
                             pOV5640Ctx->IsiCtx.SlaveAfAddress,
                             AF_ACK_Address,
                             pOV5640Ctx->IsiCtx.NrOfAfAddressBytes,
                             &ack_cmd[0],
                             1U,
							 350);
		result = IsiRegDefaultsApply( pOV5640Ctx, OV5640_af_firmware_new);
		if ( result != RET_SUCCESS )
		{
			TRACE( OV5640_ERROR, "%s: Download OV5640_af_firmware failed\n", __FUNCTION__ );
			return ( result );
		}

		TRACE( OV5640_ERROR, "%s: Download OV5640_af_firmware success\n", __FUNCTION__ );
		result = IsiRegDefaultsApply( pOV5640Ctx, OV5640_af_init);
		if ( result != RET_SUCCESS )
		{
			TRACE( OV5640_ERROR, "%s: Download OV5640_af_init failed\n", __FUNCTION__ );
			return ( result );
		}
	}else if (Position == 2) {

		TRACE( OV5640_ERROR, "%s: trigger one shot focus\n", __FUNCTION__ );
		
		result = OV5640_IsiRegWriteIss( pOV5640Ctx, AF_ACK_Address, AF_ACK_VALUE);
		if ( result != RET_SUCCESS )
		{
			TRACE( OV5640_ERROR, "%s: trigger one shot focus failed\n", __FUNCTION__ );
			return ( result );
		}
	
		result = OV5640_IsiRegWriteIss( pOV5640Ctx, AF_Address, AF_CMD);
		if ( result != RET_SUCCESS )
		{
			TRACE( OV5640_ERROR, "%s: trigger one shot focus failed\n", __FUNCTION__ );
			return ( result );
		}
		do {
			osSleep(10);
			if(OV5640_IsiRegReadIss ( pOV5640Ctx, AF_ACK_Address, &data )){
				TRACE( OV5640_ERROR, "%s: zyh read tag fail \n", __FUNCTION__);
			}

			TRACE( OV5640_INFO, "%s: zyh read tag fail,data:%d \n", __FUNCTION__,data);
		}while(data!=0x00 && cnt++<100);

		
	}
	else {
		TRACE( OV5640_ERROR, "%s: download firmware and one shot focus failed\n", __FUNCTION__ );
	}	
	//osSleep( 100 );
	TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

	return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiMdiFocusGet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          OV5640 sensor instance handle
 *          pAbsStep        pointer to a variable to receive the current
 *                          focus point
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiMdiFocusGet
(
    IsiSensorHandle_t   handle,
    uint32_t            *pAbsStep
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;
    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pAbsStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }
   /* result = HalReadI2CMem( pOV5640Ctx->IsiCtx.HalHandle,
                            pOV5640Ctx->IsiCtx.I2cAfBusNum,
                            pOV5640Ctx->IsiCtx.SlaveAfAddress,
                            0,
                            pOV5640Ctx->IsiCtx.NrOfAfAddressBytes,
                            data,
                            2U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
   */

    /* Data[0] = PD,  1, D9..D4, see AD5820 datasheet */
    /* Data[1] = D3..D0, S3..S0 */
    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiMdiFocusCalibrate
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV5640 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV5640_IsiMdiFocusCalibrate
(
    IsiSensorHandle_t   handle
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiActivateTestPattern
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV5640 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV5640_IsiActivateTestPattern
(
    IsiSensorHandle_t   handle,
    const bool_t        enable
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    return ( result );
}



/*****************************************************************************/
/**
 *          OV5640_IsiGetSensorMipiInfoIss
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV5640 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV5640_IsiGetSensorMipiInfoIss
(
    IsiSensorHandle_t   handle,
    IsiSensorMipiInfo   *ptIsiSensorMipiInfo
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }


    if ( ptIsiSensorMipiInfo == NULL )
    {
        return ( result );
    }

    TRACE( OV5640_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV5640_IsiGetSensorIsiVersion
(  IsiSensorHandle_t   handle,
   unsigned int*     pVersion
)
{
    OV5640_Context_t *pOV5640Ctx = (OV5640_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV5640_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV5640Ctx == NULL )
    {
    	TRACE( OV5640_ERROR, "%s: pOV5640Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pVersion == NULL)
	{
		TRACE( OV5640_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pVersion = CONFIG_ISI_VERSION;
	return result;
}

/*****************************************************************************/
/**
 *          OV5640_IsiGetSensorIss
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
RESULT OV5640_IsiGetSensorIss
(
    IsiSensor_t *pIsiSensor
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV5640_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pIsiSensor != NULL )
    {
        pIsiSensor->pszName                             = OV5640_g_acName;
        pIsiSensor->pRegisterTable                      = OV5640_g_aRegDescription;
        pIsiSensor->pIsiSensorCaps                      = &OV5640_g_IsiSensorDefaultConfig;
				pIsiSensor->pIsiGetSensorIsiVer					        = OV5640_IsiGetSensorIsiVersion;
        pIsiSensor->pIsiCreateSensorIss                 = OV5640_IsiCreateSensorIss;
        pIsiSensor->pIsiReleaseSensorIss                = OV5640_IsiReleaseSensorIss;
        pIsiSensor->pIsiGetCapsIss                      = OV5640_IsiGetCapsIss;
        pIsiSensor->pIsiSetupSensorIss                  = OV5640_IsiSetupSensorIss;
        pIsiSensor->pIsiChangeSensorResolutionIss       = OV5640_IsiChangeSensorResolutionIss;
        pIsiSensor->pIsiSensorSetStreamingIss           = OV5640_IsiSensorSetStreamingIss;
        pIsiSensor->pIsiSensorSetPowerIss               = OV5640_IsiSensorSetPowerIss;
        pIsiSensor->pIsiCheckSensorConnectionIss        = OV5640_IsiCheckSensorConnectionIss;
        pIsiSensor->pIsiGetSensorRevisionIss            = OV5640_IsiGetSensorRevisionIss;
        pIsiSensor->pIsiRegisterReadIss                 = OV5640_IsiRegReadIss;
        pIsiSensor->pIsiRegisterWriteIss                = OV5640_IsiRegWriteIss;

        /* AEC functions */
        pIsiSensor->pIsiExposureControlIss              = OV5640_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = OV5640_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = OV5640_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetCurrentExposureIss           = OV5640_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetGainIss                      = OV5640_IsiGetGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = OV5640_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = OV5640_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = OV5640_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = OV5640_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = OV5640_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetResolutionIss                = OV5640_IsiGetResolutionIss;
        pIsiSensor->pIsiGetAfpsInfoIss                  = OV5640_IsiGetAfpsInfoIss;

        /* AWB specific functions */
        pIsiSensor->pIsiGetCalibKFactor                 = OV5640_IsiGetCalibKFactor;
        pIsiSensor->pIsiGetCalibPcaMatrix               = OV5640_IsiGetCalibPcaMatrix;
        pIsiSensor->pIsiGetCalibSvdMeanValue            = OV5640_IsiGetCalibSvdMeanValue;
        pIsiSensor->pIsiGetCalibCenterLine              = OV5640_IsiGetCalibCenterLine;
        pIsiSensor->pIsiGetCalibClipParam               = OV5640_IsiGetCalibClipParam;
        pIsiSensor->pIsiGetCalibGlobalFadeParam         = OV5640_IsiGetCalibGlobalFadeParam;
        pIsiSensor->pIsiGetCalibFadeParam               = OV5640_IsiGetCalibFadeParam;
        pIsiSensor->pIsiGetIlluProfile                  = OV5640_IsiGetIlluProfile;
        pIsiSensor->pIsiGetLscMatrixTable               = OV5640_IsiGetLscMatrixTable;

        /* AF functions */
        pIsiSensor->pIsiMdiInitMotoDriveMds             = OV5640_IsiMdiInitMotoDriveMds;
        pIsiSensor->pIsiMdiSetupMotoDrive               = OV5640_IsiMdiSetupMotoDrive;
        pIsiSensor->pIsiMdiFocusSet                     = OV5640_IsiMdiFocusSet;
        pIsiSensor->pIsiMdiFocusGet                     = OV5640_IsiMdiFocusGet;
        pIsiSensor->pIsiMdiFocusCalibrate               = OV5640_IsiMdiFocusCalibrate;

        /* MIPI */
        pIsiSensor->pIsiGetSensorMipiInfoIss            = OV5640_IsiGetSensorMipiInfoIss;

        /* Testpattern */
        pIsiSensor->pIsiActivateTestPattern             = OV5640_IsiActivateTestPattern;
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( OV5640_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV5640_IsiGetSensorI2cInfo(sensor_i2c_info_t** pdata)
{
    sensor_i2c_info_t* pSensorI2cInfo;

    pSensorI2cInfo = ( sensor_i2c_info_t * )malloc ( sizeof (sensor_i2c_info_t) );

    if ( pSensorI2cInfo == NULL )
    {
        TRACE( OV5640_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pSensorI2cInfo, 0, sizeof( sensor_i2c_info_t ) );

    
    pSensorI2cInfo->i2c_addr = OV5640_SLAVE_ADDR;
    pSensorI2cInfo->soft_reg_addr = OV5640_SOFTWARE_RST;
    pSensorI2cInfo->soft_reg_value = OV5640_SOFTWARE_RST_VALUE;
    pSensorI2cInfo->reg_size = 2;
    pSensorI2cInfo->value_size = 1;

    {
        IsiSensorCaps_t Caps;
        sensor_caps_t *pCaps;
        uint32_t lanes,i;
        
        ListInit(&pSensorI2cInfo->lane_res[0]);
        ListInit(&pSensorI2cInfo->lane_res[1]);
        ListInit(&pSensorI2cInfo->lane_res[2]);
        
        Caps.Index = 0;            
        while(OV5640_IsiGetCapsIssInternal(&Caps)==RET_SUCCESS) {
            pCaps = malloc(sizeof(sensor_caps_t));
            if (pCaps != NULL) {
                memcpy(&pCaps->caps,&Caps,sizeof(IsiSensorCaps_t));
                ListPrepareItem(pCaps);
                ListAddTail(&pSensorI2cInfo->lane_res[0], pCaps);
            }
            Caps.Index++;
        }
    }
    
    ListInit(&pSensorI2cInfo->chipid_info);

    sensor_chipid_info_t* pChipIDInfo_H = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_H )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_H, 0, sizeof(*pChipIDInfo_H) );    
    pChipIDInfo_H->chipid_reg_addr = OV5640_CHIP_ID_HIGH_BYTE;  
    pChipIDInfo_H->chipid_reg_value = OV5640_CHIP_ID_HIGH_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_H );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_H );

    sensor_chipid_info_t* pChipIDInfo_M = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_M )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_M, 0, sizeof(*pChipIDInfo_M) ); 
    pChipIDInfo_M->chipid_reg_addr = OV5640_CHIP_ID_MIDDLE_BYTE;
    pChipIDInfo_M->chipid_reg_value = OV5640_CHIP_ID_MIDDLE_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_M );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_M );
    
    sensor_chipid_info_t* pChipIDInfo_L = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_L )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_L, 0, sizeof(*pChipIDInfo_L) ); 
    pChipIDInfo_L->chipid_reg_addr = OV5640_CHIP_ID_LOW_BYTE;
    pChipIDInfo_L->chipid_reg_value = OV5640_CHIP_ID_LOW_BYTE_DEFAULT;
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
    OV5640_IsiGetSensorIss,
    {
        0,                      /**< IsiSensor_t.pszName */
        0,                      /**< IsiSensor_t.pRegisterTable */
        0,                      /**< IsiSensor_t.pIsiSensorCaps */
        0,											/**< IsiSensor_t.pIsiGetSensorIsiVer_t>*/   //oyyf add
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
    OV5640_IsiGetSensorI2cInfo,
};



