//OV13850 the same with ov14825

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
 * @file OV13850.c
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

#include "OV13850_MIPI_priv.h"

#define  OV13850_NEWEST_TUNING_XML "22-May-2014_OUYANG_OV13850_FX288_v1.0"

#define CC_OFFSET_SCALING  2.0f
#define I2C_COMPLIANT_STARTBIT 1U

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( OV13850_INFO , "OV13850: ", INFO,    0U );
CREATE_TRACER( OV13850_WARN , "OV13850: ", WARNING, 1U );
CREATE_TRACER( OV13850_ERROR, "OV13850: ", ERROR,   1U );

CREATE_TRACER( OV13850_DEBUG, "OV13850: ", INFO,     0U );

CREATE_TRACER( OV13850_REG_INFO , "OV13850: ", INFO, 0);
CREATE_TRACER( OV13850_REG_DEBUG, "OV13850: ", INFO, 0U );

#define OV13850_SLAVE_ADDR       0x20U                           /**< i2c slave address of the OV13850 camera sensor */
#define OV13850_SLAVE_ADDR2      0x6cU
#define OV13850_SLAVE_AF_ADDR    0x18U                           /**< i2c slave address of the OV13850 integrated AD5820 */

#define OV13850_MIN_GAIN_STEP   ( 1.0f / 16.0f); /**< min gain step size used by GUI ( 32/(32-7) - 32/(32-6); min. reg value is 6 as of datasheet; depending on actual gain ) */
#define OV13850_MAX_GAIN_AEC    ( 8.0f )            /**< max. gain used by the AEC (arbitrarily chosen, recommended by Omnivision) */


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

#define MAX_VCMDRV_CURRENT      100U
#define MAX_VCMDRV_REG          1023U


/*!<
 * Lens movement is triggered every 133ms (VGA, 7.5fps processed frames
 * worst case assumed, usually even much slower, see OV5630 driver for
 * details). Thus the lens has to reach the requested position after
 * max. 133ms. Minimum mechanical ringing is expected with mode 1 ,
 * 100us per step. A movement over the full range needs max. 102.3ms
 * (see table 9 AD5820 datasheet).
 */
#define MDI_SLEW_RATE_CTRL 6U /* S3..0 */



/******************************************************************************
 * local variable declarations
 *****************************************************************************/
const char OV13850_g_acName[] = "OV13850_MIPI";

extern const IsiRegDescription_t OV13850_g_aRegDescription_fourlane[];
extern const IsiRegDescription_t OV13850_g_fourlane_resolution_4224_3136[];
extern const IsiRegDescription_t OV13850_g_fourlane_resolution_2112_1568[];
extern const IsiRegDescription_t OV13850_g_aRegDescription_twolane[];
extern const IsiRegDescription_t OV13850_g_twolane_resolution_4224_3136[];
extern const IsiRegDescription_t OV13850_g_twolane_resolution_2112_1568[];
extern const IsiRegDescription_t OV13850_g_aRegDescription_onelane[];
extern const IsiRegDescription_t OV13850_g_onelane_resolution_4224_3136[];
extern const IsiRegDescription_t OV13850_g_onelane_resolution_2112_1568[];
extern const IsiRegDescription_t OV13850_g_2112x1568P30_twolane_fpschg[];
extern const IsiRegDescription_t OV13850_g_2112x1568P25_twolane_fpschg[];
extern const IsiRegDescription_t OV13850_g_2112x1568P20_twolane_fpschg[];
extern const IsiRegDescription_t OV13850_g_2112x1568P15_twolane_fpschg[];
extern const IsiRegDescription_t OV13850_g_2112x1568P10_twolane_fpschg[];
extern const IsiRegDescription_t OV13850_g_4224x3136P7_twolane_fpschg[];
extern const IsiRegDescription_t OV13850_g_4224x3136P4_twolane_fpschg[];

extern const IsiRegDescription_t OV13850_g_2112x1568P30_fourlane_fpschg[];
extern const IsiRegDescription_t OV13850_g_2112x1568P25_fourlane_fpschg[];
extern const IsiRegDescription_t OV13850_g_2112x1568P20_fourlane_fpschg[];
extern const IsiRegDescription_t OV13850_g_2112x1568P15_fourlane_fpschg[];
extern const IsiRegDescription_t OV13850_g_2112x1568P10_fourlane_fpschg[];
extern const IsiRegDescription_t OV13850_g_4224x3136P15_fourlane_fpschg[];
extern const IsiRegDescription_t OV13850_g_4224x3136P7_fourlane_fpschg[];
extern const IsiRegDescription_t OV13850_g_4224x3136P4_fourlane_fpschg[];


const IsiSensorCaps_t OV13850_g_IsiSensorDefaultConfig;


#define OV13850_I2C_START_BIT        (I2C_COMPLIANT_STARTBIT)    // I2C bus start condition
#define OV13850_I2C_NR_ADR_BYTES     (2U)                        // 1 byte base address and 2 bytes sub address
#define OV13850_I2C_NR_DAT_BYTES     (1U)                        // 8 bit registers


static uint16_t g_suppoted_mipi_lanenum_type = SUPPORT_MIPI_ONE_LANE|SUPPORT_MIPI_TWO_LANE|SUPPORT_MIPI_FOUR_LANE;
#define DEFAULT_NUM_LANES SUPPORT_MIPI_TWO_LANE



/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static RESULT OV13850_IsiCreateSensorIss( IsiSensorInstanceConfig_t *pConfig );
static RESULT OV13850_IsiReleaseSensorIss( IsiSensorHandle_t handle );
static RESULT OV13850_IsiGetCapsIss( IsiSensorHandle_t handle, IsiSensorCaps_t *pIsiSensorCaps );
static RESULT OV13850_IsiSetupSensorIss( IsiSensorHandle_t handle, const IsiSensorConfig_t *pConfig );
static RESULT OV13850_IsiSensorSetStreamingIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV13850_IsiSensorSetPowerIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV13850_IsiCheckSensorConnectionIss( IsiSensorHandle_t handle );
static RESULT OV13850_IsiGetSensorRevisionIss( IsiSensorHandle_t handle, uint32_t *p_value);

static RESULT OV13850_IsiGetGainLimitsIss( IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain);
static RESULT OV13850_IsiGetIntegrationTimeLimitsIss( IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime );
static RESULT OV13850_IsiExposureControlIss( IsiSensorHandle_t handle, float NewGain, float NewIntegrationTime, uint8_t *pNumberOfFramesToSkip, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV13850_IsiGetCurrentExposureIss( IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV13850_IsiGetAfpsInfoIss ( IsiSensorHandle_t handle, uint32_t Resolution, IsiAfpsInfo_t* pAfpsInfo);
static RESULT OV13850_IsiGetGainIss( IsiSensorHandle_t handle, float *pSetGain );
static RESULT OV13850_IsiGetGainIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV13850_IsiSetGainIss( IsiSensorHandle_t handle, float NewGain, float *pSetGain );
static RESULT OV13850_IsiGetIntegrationTimeIss( IsiSensorHandle_t handle, float *pSetIntegrationTime );
static RESULT OV13850_IsiGetIntegrationTimeIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV13850_IsiSetIntegrationTimeIss( IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t *pNumberOfFramesToSkip );
static RESULT OV13850_IsiGetResolutionIss( IsiSensorHandle_t handle, uint32_t *pSetResolution );


static RESULT OV13850_IsiRegReadIss( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );
static RESULT OV13850_IsiRegWriteIss( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

static RESULT OV13850_IsiGetCalibKFactor( IsiSensorHandle_t handle, Isi1x1FloatMatrix_t **pIsiKFactor );
static RESULT OV13850_IsiGetCalibPcaMatrix( IsiSensorHandle_t   handle, Isi3x2FloatMatrix_t **pIsiPcaMatrix );
static RESULT OV13850_IsiGetCalibSvdMeanValue( IsiSensorHandle_t   handle, Isi3x1FloatMatrix_t **pIsiSvdMeanValue );
static RESULT OV13850_IsiGetCalibCenterLine( IsiSensorHandle_t   handle, IsiLine_t  **ptIsiCenterLine);
static RESULT OV13850_IsiGetCalibClipParam( IsiSensorHandle_t   handle, IsiAwbClipParm_t    **pIsiClipParam );
static RESULT OV13850_IsiGetCalibGlobalFadeParam( IsiSensorHandle_t       handle, IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam);
static RESULT OV13850_IsiGetCalibFadeParam( IsiSensorHandle_t   handle, IsiAwbFade2Parm_t   **ptIsiFadeParam);
static RESULT OV13850_IsiGetIlluProfile( IsiSensorHandle_t   handle, const uint32_t CieProfile, IsiIlluProfile_t **ptIsiIlluProfile );

static RESULT OV13850_IsiMdiInitMotoDriveMds( IsiSensorHandle_t handle );
static RESULT OV13850_IsiMdiSetupMotoDrive( IsiSensorHandle_t handle, uint32_t *pMaxStep );
static RESULT OV13850_IsiMdiFocusSet( IsiSensorHandle_t handle, const uint32_t Position );
static RESULT OV13850_IsiMdiFocusGet( IsiSensorHandle_t handle, uint32_t *pAbsStep );
static RESULT OV13850_IsiMdiFocusCalibrate( IsiSensorHandle_t handle );

static RESULT OV13850_IsiGetSensorMipiInfoIss( IsiSensorHandle_t handle, IsiSensorMipiInfo *ptIsiSensorMipiInfo);
static RESULT OV13850_IsiGetSensorIsiVersion(  IsiSensorHandle_t   handle, unsigned int* pVersion);
static RESULT OV13850_IsiGetSensorTuningXmlVersion(  IsiSensorHandle_t   handle, char** pTuningXmlVersion);


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
 *          OV13850_IsiCreateSensorIss
 *
 * @brief   This function creates a new OV13850 sensor instance handle.
 *
 * @param   pConfig     configuration structure to create the instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
static RESULT OV13850_IsiCreateSensorIss
(
    IsiSensorInstanceConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;
	int32_t current_distance;
    OV13850_Context_t *pOV13850Ctx;

    TRACE( OV13850_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pConfig == NULL) || (pConfig->pSensor ==NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    pOV13850Ctx = ( OV13850_Context_t * )malloc ( sizeof (OV13850_Context_t) );
    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pOV13850Ctx, 0, sizeof( OV13850_Context_t ) );

    result = HalAddRef( pConfig->HalHandle );
    if ( result != RET_SUCCESS )
    {
        free ( pOV13850Ctx );
        return ( result );
    }

    pOV13850Ctx->IsiCtx.HalHandle              = pConfig->HalHandle;
    pOV13850Ctx->IsiCtx.HalDevID               = pConfig->HalDevID;
    pOV13850Ctx->IsiCtx.I2cBusNum              = pConfig->I2cBusNum;
    pOV13850Ctx->IsiCtx.SlaveAddress           = ( pConfig->SlaveAddr == 0 ) ? OV13850_SLAVE_ADDR : pConfig->SlaveAddr;
    pOV13850Ctx->IsiCtx.NrOfAddressBytes       = 2U;

    pOV13850Ctx->IsiCtx.I2cAfBusNum            = pConfig->I2cAfBusNum;
    pOV13850Ctx->IsiCtx.SlaveAfAddress         = ( pConfig->SlaveAfAddr == 0 ) ? OV13850_SLAVE_AF_ADDR : pConfig->SlaveAfAddr;
    pOV13850Ctx->IsiCtx.NrOfAfAddressBytes     = 0U;

    pOV13850Ctx->IsiCtx.pSensor                = pConfig->pSensor;

    pOV13850Ctx->Configured             = BOOL_FALSE;
    pOV13850Ctx->Streaming              = BOOL_FALSE;
    pOV13850Ctx->TestPattern            = BOOL_FALSE;
    pOV13850Ctx->isAfpsRun              = BOOL_FALSE;
    /* ddl@rock-chips.com: v0.3.0 */
    current_distance = pConfig->VcmRatedCurrent - pConfig->VcmStartCurrent;
    current_distance = current_distance*MAX_VCMDRV_REG/MAX_VCMDRV_CURRENT;    
    pOV13850Ctx->VcmInfo.Step = (current_distance+(MAX_LOG-1))/MAX_LOG;
    pOV13850Ctx->VcmInfo.StartCurrent   = pConfig->VcmStartCurrent*MAX_VCMDRV_REG/MAX_VCMDRV_CURRENT;    
    pOV13850Ctx->VcmInfo.RatedCurrent   = pOV13850Ctx->VcmInfo.StartCurrent + MAX_LOG*pOV13850Ctx->VcmInfo.Step;
    pOV13850Ctx->VcmInfo.StepMode       = pConfig->VcmStepMode;  

    pOV13850Ctx->IsiSensorMipiInfo.sensorHalDevID = pOV13850Ctx->IsiCtx.HalDevID;
    if(pConfig->mipiLaneNum & g_suppoted_mipi_lanenum_type)
        pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes = pConfig->mipiLaneNum;
    else{
        pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes = DEFAULT_NUM_LANES;
    }

    pConfig->hSensor = ( IsiSensorHandle_t )pOV13850Ctx;

    result = HalSetCamConfig( pOV13850Ctx->IsiCtx.HalHandle, pOV13850Ctx->IsiCtx.HalDevID, false, true, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    result = HalSetClock( pOV13850Ctx->IsiCtx.HalHandle, pOV13850Ctx->IsiCtx.HalDevID, 24000000U);
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV13850_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiReleaseSensorIss
 *
 * @brief   This function destroys/releases an OV13850 sensor instance.
 *
 * @param   handle      OV13850 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
static RESULT OV13850_IsiReleaseSensorIss
(
    IsiSensorHandle_t handle
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    (void)OV13850_IsiSensorSetStreamingIss( pOV13850Ctx, BOOL_FALSE );
    (void)OV13850_IsiSensorSetPowerIss( pOV13850Ctx, BOOL_FALSE );

    (void)HalDelRef( pOV13850Ctx->IsiCtx.HalHandle );

    MEMSET( pOV13850Ctx, 0, sizeof( OV13850_Context_t ) );
    free ( pOV13850Ctx );

    TRACE( OV13850_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetCapsIss
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
static RESULT OV13850_IsiGetCapsIssInternal
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
        if(mipi_lanes == SUPPORT_MIPI_ONE_LANE){
            switch (pIsiSensorCaps->Index) 
            {
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_4224_3136P4;
                    break;
                }
                case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P15;
                    break;
                }
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }

            }            
        }else if(mipi_lanes == SUPPORT_MIPI_TWO_LANE){
            switch (pIsiSensorCaps->Index) 
            {
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_4224_3136P7;
                    break;
                }
	            case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_4224_3136P4;
                    break;
                }			
                case 2:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P30;
                    break;
                }
                case 3:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P25;
                    break;
                }
                case 4:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P20;
                    break;
                }
                case 5:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P15;
                    break;
                }
                case 6:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P10;
                    break;
                }
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }

            }   
        } else if(mipi_lanes == SUPPORT_MIPI_FOUR_LANE) {
			switch (pIsiSensorCaps->Index) 
            {
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_4224_3136P15;
                    break;
                }
                case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_4224_3136P7;
                    break;
                }
                case 2:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_4224_3136P4;
                    break;
                }				
                case 3:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P30;
                    break;
                }
                case 4:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P25;
                    break;
                }
                case 5:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P20;
                    break;
                }
                case 6:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P15;
                    break;
                }
                case 7:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_2112_1568P10;
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
        pIsiSensorCaps->BLC             = ( ISI_BLC_AUTO);
        pIsiSensorCaps->AGC             = ( ISI_AGC_OFF );
        pIsiSensorCaps->AWB             = ( ISI_AWB_OFF );
        pIsiSensorCaps->AEC             = ( ISI_AEC_OFF );
        pIsiSensorCaps->DPCC            = ( ISI_DPCC_OFF );

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

static RESULT OV13850_IsiGetCapsIss
(
    IsiSensorHandle_t handle,
    IsiSensorCaps_t   *pIsiSensorCaps
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    result = OV13850_IsiGetCapsIssInternal(pIsiSensorCaps, pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes);
    TRACE( OV13850_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_g_IsiSensorDefaultConfig
 *
 * @brief   recommended default configuration for application use via call
 *          to IsiGetSensorIss()
 *
 *****************************************************************************/
const IsiSensorCaps_t OV13850_g_IsiSensorDefaultConfig =
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
    ISI_RES_2112_1568P30,          // Res
    ISI_DWNSZ_SUBSMPL,          // DwnSz
    ISI_BLC_AUTO,               // BLC
    ISI_AGC_OFF,                // AGC
    ISI_AWB_OFF,                // AWB
    ISI_AEC_OFF,                // AEC
    ISI_DPCC_OFF,               // DPCC
    ISI_CIEPROF_D65,            // CieProfile, this is also used as start profile for AWB (if not altered by menu settings)
    ISI_SMIA_OFF,               // SmiaMode
    ISI_MIPI_MODE_RAW_10,       // MipiMode
    ISI_AFPS_NOTSUPP,           // AfpsResolutions
    ISI_SENSOR_OUTPUT_MODE_RAW,
    0,
};



/*****************************************************************************/
/**
 *          OV13850_SetupOutputFormat
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV13850 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV13850_SetupOutputFormat
(
    OV13850_Context_t       *pOV13850Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s%s (enter)\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );

    /* bus-width */
    switch ( pConfig->BusWidth )        /* only ISI_BUSWIDTH_12BIT supported, no configuration needed here */
    {
        case ISI_BUSWIDTH_10BIT:
        {
            break;
        }

        default:
        {
            TRACE( OV13850_ERROR, "%s%s: bus width not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s: mode not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s: field selection not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* only Bayer mode is supported by OV13850 sensor, so the YCSequence parameter is not checked */
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
            TRACE( OV13850_ERROR, "%s%s: 422 conversion not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s: bayer pattern not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s: HPol not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s: VPol not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s:  edge mode not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s:  gamma not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s: color conversion not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s: SMIA mode not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            TRACE( OV13850_ERROR, "%s%s: MIPI mode not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
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
            //TRACE( OV13850_ERROR, "%s%s: AFPS not supported\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"" );
            //return ( RET_NOTSUPP );
        }
    }

    TRACE( OV13850_INFO, "%s%s (exit)\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}

int OV13850_get_PCLK( OV13850_Context_t *pOV13850Ctx, int XVCLK)
{
	 // calculate PCLK
	 uint32_t temp1, temp2;
	 int Pll2_predivp, Pll2_prediv2x, Pll2_mult, Pll2_divsp, Pll2_divs2x;
	 long SCLK;
	 int Pll2_predivp_map[] = {1, 2};
	 int Pll2_prediv2x_map[] = {2, 3, 4, 5, 6, 8, 12, 16};
	 int Pll2_divs2x_map[] = {2, 3, 4, 5, 6, 7, 8, 10};
	 
	 OV13850_IsiRegReadIss(  pOV13850Ctx, 0x3611, &temp1 );
	 temp2 = (temp1>>3) & 0x01;
	 Pll2_predivp = Pll2_predivp_map[temp2];
	 temp2 = temp1 & 0x07;
	 Pll2_prediv2x = Pll2_prediv2x_map[temp2];

	 OV13850_IsiRegReadIss(  pOV13850Ctx, 0x3615, &temp1 );
	 temp2 = temp1 & 0x03;
	 OV13850_IsiRegReadIss(  pOV13850Ctx, 0x3614, &temp1 );
	 Pll2_mult = (temp2<<8) + temp1;
	 OV13850_IsiRegReadIss(  pOV13850Ctx, 0x3612, &temp1 );
	 temp2 = temp1 & 0x0f;
	 Pll2_divsp = temp2 + 1;
	 temp2 = (temp1>>4) & 0x07;
	 Pll2_divs2x = Pll2_divs2x_map[temp2];
	 
	 SCLK = XVCLK /Pll2_predivp * 2 / Pll2_prediv2x * Pll2_mult / Pll2_divsp * 2 /Pll2_divs2x * 4;
	 
	 return SCLK;
 }

/*****************************************************************************/
/**
 *          OV13850_SetupOutputWindow
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV13850 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_SetupOutputWindowInternal
(
    OV13850_Context_t        *pOV13850Ctx,
    const IsiSensorConfig_t *pConfig,
    bool_t set2Sensor,
    bool_t res_no_chg
)
{
    RESULT result     = RET_SUCCESS;
    uint16_t usFrameLengthLines = 0;
    uint16_t usLineLengthPck    = 0;
	uint16_t usTimeHts;
	uint16_t usTimeVts;
    float    rVtPixClkFreq      = 0.0f;
    int xclk = 24000000;

    TRACE( OV13850_INFO, "%s (enter)\n", __FUNCTION__);

	if(pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_ONE_LANE){
	    /* resolution */
	    switch ( pConfig->Resolution )
	    {
	        case ISI_RES_2112_1568P15:
	        {
				if (set2Sensor == BOOL_TRUE) {
				    TRACE( OV13850_DEBUG, "%s(%d): Resolution 2112x1568\n", __FUNCTION__,__LINE__ );
    				result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV13850Ctx, OV13850_g_onelane_resolution_2112_1568);
    				if ( result != RET_SUCCESS )
    				{
    					return ( result );
    				}
				}
	            usTimeHts = 0x12c0;
	            usTimeVts = 0x0680;
				pOV13850Ctx->IsiSensorMipiInfo.ulMipiFreq = 640;
	            break;
	            
	        }
	        
	        case ISI_RES_4224_3136P4:
	        {
	         	if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV13850Ctx, OV13850_g_onelane_resolution_4224_3136)) != RET_SUCCESS){
					result = RET_FAILURE;
					TRACE( OV13850_ERROR, "%s: failed to set one lane OV13850_g_onelane_resolution_4224_3136 \n", __FUNCTION__ );
	            }

	            usTimeHts = 0x12c0;
	            usTimeVts = 0x0d00;
				pOV13850Ctx->IsiSensorMipiInfo.ulMipiFreq = 640;
	            break;
	            
	        }

	        default:
	        {
	            TRACE( OV13850_ERROR, "%s: one lane Resolution not supported\n", __FUNCTION__ );
	            return ( RET_NOTSUPP );
	        }
	    }
		
	}
	else if(pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_TWO_LANE){
	    /* resolution */
	    switch ( pConfig->Resolution )
	    {
	        case ISI_RES_2112_1568P30:
	        case ISI_RES_2112_1568P25:
	        case ISI_RES_2112_1568P20:
	        case ISI_RES_2112_1568P15:
	        case ISI_RES_2112_1568P10:				
	        {
				if (set2Sensor == BOOL_TRUE) {                    
                    if (res_no_chg == BOOL_FALSE) {
                        result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV13850Ctx, OV13850_g_twolane_resolution_2112_1568);
                    }
                    if (pConfig->Resolution == ISI_RES_2112_1568P30) {                        
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P30_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_2112_1568P25) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P25_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_2112_1568P20) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P20_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_2112_1568P15) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P15_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_2112_1568P10) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P10_twolane_fpschg);
                    }					
				}
    			usTimeHts = 0x12c0; 
                if (pConfig->Resolution == ISI_RES_2112_1568P30) {
                    usTimeVts = 0x0680;
                } else if (pConfig->Resolution == ISI_RES_2112_1568P25) {
                    usTimeVts = 0x7cc;
                } else if (pConfig->Resolution == ISI_RES_2112_1568P20) {
                    usTimeVts = 0x9c0;
                } else if (pConfig->Resolution == ISI_RES_2112_1568P15) {
                    usTimeVts = 0xd00;
                } else if (pConfig->Resolution == ISI_RES_2112_1568P10) {
                    usTimeVts = 0x1380;
                }
                
    		    /* sleep a while, that sensor can take over new default values */
    		    osSleep( 10 );

				pOV13850Ctx->IsiSensorMipiInfo.ulMipiFreq = 600;
	            break;
	            
	        }
	        
	        case ISI_RES_4224_3136P7:
	        case ISI_RES_4224_3136P4:
	        {
                if (set2Sensor == BOOL_TRUE) {
                    if (res_no_chg == BOOL_FALSE) {
        			    result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV13850Ctx, OV13850_g_twolane_resolution_4224_3136);
        		    }

                    if (pConfig->Resolution == ISI_RES_4224_3136P7) {                        
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_4224x3136P7_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_4224_3136P4) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_4224x3136P4_twolane_fpschg);
                    }
        		    
        		}				
    			usTimeHts = 0x12c0;                
                if (pConfig->Resolution == ISI_RES_4224_3136P7) {                        
                    usTimeVts = 0x0d00;
                } else if (pConfig->Resolution == ISI_RES_4224_3136P4) {
                    usTimeVts = 0x16c0;
                }
    		    /* sleep a while, that sensor can take over new default values */
    		    osSleep( 10 );

				pOV13850Ctx->IsiSensorMipiInfo.ulMipiFreq = 600;
	            break;
	            
	        }

	        default:
	        {
	            TRACE( OV13850_ERROR, "%s: two lane Resolution not supported\n", __FUNCTION__ );
	            return ( RET_NOTSUPP );
	        }
	    }
		
	}
	else if(pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_FOUR_LANE){
	    switch ( pConfig->Resolution )
	    {
	        case ISI_RES_2112_1568P30:
	        case ISI_RES_2112_1568P25:
	        case ISI_RES_2112_1568P20:
	        case ISI_RES_2112_1568P15:
	        case ISI_RES_2112_1568P10:	
	        {
				if (set2Sensor == BOOL_TRUE) {                    
                    if (res_no_chg == BOOL_FALSE) {
                        result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV13850Ctx, OV13850_g_twolane_resolution_2112_1568);
                    }
                    if (pConfig->Resolution == ISI_RES_2112_1568P30) {                        
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P30_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_2112_1568P25) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P25_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_2112_1568P20) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P20_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_2112_1568P15) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P15_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_2112_1568P10) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_2112x1568P10_fourlane_fpschg);
                    }					
				}
    			usTimeHts = 0x2580; 
                if (pConfig->Resolution == ISI_RES_2112_1568P30) {
                    usTimeVts = 0x0680;
                } else if (pConfig->Resolution == ISI_RES_2112_1568P25) {
                    usTimeVts = 0x7cc;
                } else if (pConfig->Resolution == ISI_RES_2112_1568P20) {
                    usTimeVts = 0x9c0;
                } else if (pConfig->Resolution == ISI_RES_2112_1568P15) {
                    usTimeVts = 0xd00;
                } else if (pConfig->Resolution == ISI_RES_2112_1568P10) {
                    usTimeVts = 0x1380;
                }
                
    		    /* sleep a while, that sensor can take over new default values */
    		    osSleep( 10 );

				pOV13850Ctx->IsiSensorMipiInfo.ulMipiFreq = 640;
	            break;
	            
	        }
	        
	        case ISI_RES_4224_3136P15:
			case ISI_RES_4224_3136P7:
			case ISI_RES_4224_3136P4:
				
	        {
                if (set2Sensor == BOOL_TRUE) {
                    if (res_no_chg == BOOL_FALSE) {
        			    result = IsiRegDefaultsApply((IsiSensorHandle_t)pOV13850Ctx, OV13850_g_twolane_resolution_4224_3136);
        		    }

                    if (pConfig->Resolution == ISI_RES_4224_3136P15) {                        
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_4224x3136P15_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_4224_3136P7) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_4224x3136P7_fourlane_fpschg);
                    }else if (pConfig->Resolution == ISI_RES_4224_3136P4) {
                        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_4224x3136P4_fourlane_fpschg);
                    }
        		}				
    			usTimeHts = 0x2580;
                if (pConfig->Resolution == ISI_RES_4224_3136P15) { 
					usTimeVts = 0x0d00;
            	}
                else if (pConfig->Resolution == ISI_RES_4224_3136P7) {                        
                    usTimeVts = 0x1bdb;
                } else if (pConfig->Resolution == ISI_RES_4224_3136P4) {
                    usTimeVts = 0x30c0;
                }
    		    /* sleep a while, that sensor can take over new default values */
    		    osSleep( 10 );

				pOV13850Ctx->IsiSensorMipiInfo.ulMipiFreq = 640;
	            break;
	            
	        }

	        default:
	        {
	            TRACE( OV13850_ERROR, "%s: four lane Resolution not supported\n", __FUNCTION__ );
	            return ( RET_NOTSUPP );
	        }
	    }

	}

/* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */

	usLineLengthPck = usTimeHts;
    usFrameLengthLines = usTimeVts;

	// store frame timing for later use in AEC module
	rVtPixClkFreq = OV13850_get_PCLK(pOV13850Ctx, xclk);    
    pOV13850Ctx->VtPixClkFreq     = rVtPixClkFreq;
    pOV13850Ctx->LineLengthPck    = usLineLengthPck;
    pOV13850Ctx->FrameLengthLines = usFrameLengthLines;	

	//have to reset mipi freq here,zyc

    TRACE( OV13850_INFO, "%s  (exit): Resolution %dx%d@%dfps  MIPI %dlanes  res_no_chg: %d   rVtPixClkFreq: %f\n", __FUNCTION__,
                        ISI_RES_W_GET(pConfig->Resolution),ISI_RES_H_GET(pConfig->Resolution),
                        ISI_FPS_GET(pConfig->Resolution),
                        pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes,
                        res_no_chg,rVtPixClkFreq);

    return ( result );
}

static RESULT OV13850_SetupOutputWindow
(
    OV13850_Context_t        *pOV13850Ctx,
    const IsiSensorConfig_t *pConfig    
)
{
    bool_t res_no_chg;

    if ((ISI_RES_W_GET(pConfig->Resolution)==ISI_RES_W_GET(pOV13850Ctx->Config.Resolution)) && 
        (ISI_RES_W_GET(pConfig->Resolution)==ISI_RES_W_GET(pOV13850Ctx->Config.Resolution))) {
        res_no_chg = BOOL_TRUE;
        
    } else {
        res_no_chg = BOOL_FALSE;
    }

    return OV13850_SetupOutputWindowInternal(pOV13850Ctx,pConfig,BOOL_TRUE, BOOL_FALSE);
}



/*****************************************************************************/
/**
 *          OV13850_SetupImageControl
 *
 * @brief   Sets the image control functions (BLC, AGC, AWB, AEC, DPCC ...)
 *
 * @param   handle      OV13850 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV13850_SetupImageControl
(
    OV13850_Context_t        *pOV13850Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0U;

    TRACE( OV13850_INFO, "%s (enter)\n", __FUNCTION__);

    switch ( pConfig->Bls )      /* only ISI_BLS_OFF supported, no configuration needed */
    {
        case ISI_BLS_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV13850_ERROR, "%s: Black level not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* black level compensation */
    switch ( pConfig->BLC )
    {
        case ISI_BLC_OFF:
        {
            /* turn off black level correction (clear bit 0) */
            //result = OV13850_IsiRegReadIss(  pOV13850Ctx, OV13850_BLC_CTRL00, &RegValue );
            //result = OV13850_IsiRegWriteIss( pOV13850Ctx, OV13850_BLC_CTRL00, RegValue & 0x7F);
            break;
        }

        case ISI_BLC_AUTO:
        {
            /* turn on black level correction (set bit 0)
             * (0x331E[7] is assumed to be already setup to 'auto' by static configration) */
            //result = OV13850_IsiRegReadIss(  pOV13850Ctx, OV13850_BLC_CTRL00, &RegValue );
            //result = OV13850_IsiRegWriteIss( pOV13850Ctx, OV13850_BLC_CTRL00, RegValue | 0x80 );
            break;
        }

        default:
        {
            TRACE( OV13850_ERROR, "%s: BLC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* automatic gain control */
    switch ( pConfig->AGC )
    {
        case ISI_AGC_OFF:
        {
            // manual gain (appropriate for AEC with Marvin)
            //result = OV13850_IsiRegReadIss(  pOV13850Ctx, OV13850_AEC_MANUAL, &RegValue );
            //result = OV13850_IsiRegWriteIss( pOV13850Ctx, OV13850_AEC_MANUAL, RegValue | 0x02 );
            break;
        }

        default:
        {
            TRACE( OV13850_ERROR, "%s: AGC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* automatic white balance */
    switch( pConfig->AWB )
    {
        case ISI_AWB_OFF:
        {
            //result = OV13850_IsiRegReadIss(  pOV13850Ctx, OV13850_ISP_CTRL01, &RegValue );
            //result = OV13850_IsiRegWriteIss( pOV13850Ctx, OV13850_ISP_CTRL01, RegValue | 0x01 );
            break;
        }

        default:
        {
            TRACE( OV13850_ERROR, "%s: AWB not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    switch( pConfig->AEC )
    {
        case ISI_AEC_OFF:
        {
            //result = OV13850_IsiRegReadIss(  pOV13850Ctx, OV13850_AEC_MANUAL, &RegValue );
            //result = OV13850_IsiRegWriteIss( pOV13850Ctx, OV13850_AEC_MANUAL, RegValue | 0x01 );
            break;
        }

        default:
        {
            TRACE( OV13850_ERROR, "%s: AEC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }


    switch( pConfig->DPCC )
    {
        case ISI_DPCC_OFF:
        {
            // disable white and black pixel cancellation (clear bit 6 and 7)
            //result = OV13850_IsiRegReadIss( pOV13850Ctx, OV13850_ISP_CTRL00, &RegValue );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            //result = OV13850_IsiRegWriteIss( pOV13850Ctx, OV13850_ISP_CTRL00, (RegValue &0x7c) );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            break;
        }

        default:
        {
            TRACE( OV13850_ERROR, "%s: DPCC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }// I have not update this commented part yet, as I did not find DPCC setting in the current 8810 driver of Trillian board. - SRJ

    return ( result );
}


/*****************************************************************************/
/**
 *          OV13850_AecSetModeParameters
 *
 * @brief   This function fills in the correct parameters in OV13850-Instances
 *          according to AEC mode selection in IsiSensorConfig_t.
 *
 * @note    It is assumed that IsiSetupOutputWindow has been called before
 *          to fill in correct values in instance structure.
 *
 * @param   handle      OV13850 context
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_AecSetModeParameters
(
    OV13850_Context_t       *pOV13850Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

       TRACE( OV13850_INFO, "%s%s (enter)  Res: 0x%x  0x%x\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"",
        pOV13850Ctx->Config.Resolution, pConfig->Resolution);

    if ( (pOV13850Ctx->VtPixClkFreq == 0.0f) )
    {
        TRACE( OV13850_ERROR, "%s%s: Division by zero!\n", __FUNCTION__  );
        return ( RET_OUTOFRANGE );
    }

    //as of mail from Omnivision FAE the limit is VTS - 6 (above that we observed a frame
    //exposed way too dark from time to time)
    // (formula is usually MaxIntTime = (CoarseMax * LineLength + FineMax) / Clk
    //                     MinIntTime = (CoarseMin * LineLength + FineMin) / Clk )
    pOV13850Ctx->AecMaxIntegrationTime = ( ((float)(pOV13850Ctx->FrameLengthLines - 4)) * ((float)pOV13850Ctx->LineLengthPck) ) / pOV13850Ctx->VtPixClkFreq;
    pOV13850Ctx->AecMinIntegrationTime = 0.0001f;    

    pOV13850Ctx->AecMaxGain = OV13850_MAX_GAIN_AEC;
    pOV13850Ctx->AecMinGain = 1.0f; //as of sensor datasheet 32/(32-6)

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    pOV13850Ctx->AecIntegrationTimeIncrement = ((float)pOV13850Ctx->LineLengthPck) / pOV13850Ctx->VtPixClkFreq;
    pOV13850Ctx->AecGainIncrement = OV13850_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOV13850Ctx->AecCurGain               = pOV13850Ctx->AecMinGain;
    pOV13850Ctx->AecCurIntegrationTime    = 0.0f;
    pOV13850Ctx->OldCoarseIntegrationTime = 0;
    pOV13850Ctx->OldFineIntegrationTime   = 0;
    //pOV13850Ctx->GroupHold                = true; //must be true (for unknown reason) to correctly set gain the first time

    TRACE( OV13850_INFO, "%s%s (exit)\n", __FUNCTION__, pOV13850Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}


/*****************************************************************************/
/**
 *          OV13850_IsiSetupSensorIss
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV13850 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiSetupSensorIss
(
    IsiSensorHandle_t       handle,
    const IsiSensorConfig_t *pConfig
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV13850_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid configuration (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    if ( pOV13850Ctx->Streaming != BOOL_FALSE )
    {
        return RET_WRONG_STATE;
    }

    MEMCPY( &pOV13850Ctx->Config, pConfig, sizeof( IsiSensorConfig_t ) );

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    result = OV13850_IsiRegWriteIss ( pOV13850Ctx, OV13850_SOFTWARE_RST, 0x01U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    osSleep( 10 );

    // disable streaming during sensor setup
    // (this seems not to be necessary, however Omnivision is doing it in their
    // reference settings, simply overwrite upper bits since setup takes care
    // of 'em later on anyway)
    result = OV13850_IsiRegWriteIss( pOV13850Ctx, OV13850_MODE_SELECT, 0x00 );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV13850_ERROR, "%s: Can't write OV13850 Image System Register (disable streaming failed)\n", __FUNCTION__ );
        return ( result );
    }
    
    /* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */
	if(pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_ONE_LANE){
		result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_aRegDescription_onelane);
	}
	else if(pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_FOUR_LANE){
        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_aRegDescription_fourlane);
    }
	else if(pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_TWO_LANE){
        result = IsiRegDefaultsApply( pOV13850Ctx, OV13850_g_aRegDescription_twolane);
	}
    
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* sleep a while, that sensor can take over new default values */
    osSleep( 10 );


    /* 3.) verify default values to make sure everything has been written correctly as expected */
	#if 0
	result = IsiRegDefaultsVerify( pOV13850Ctx, OV13850_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
	#endif

    /* 4.) setup output format (RAW10|RAW12) */
    result = OV13850_SetupOutputFormat( pOV13850Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV13850_ERROR, "%s: SetupOutputFormat failed.\n", __FUNCTION__);
        return ( result );
    }

    /* 5.) setup output window */
    result = OV13850_SetupOutputWindow( pOV13850Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV13850_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV13850_SetupImageControl( pOV13850Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV13850_ERROR, "%s: SetupImageControl failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV13850_AecSetModeParameters( pOV13850Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV13850_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
        return ( result );
    }
    if (result == RET_SUCCESS)
    {
        pOV13850Ctx->Configured = BOOL_TRUE;
    }

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiChangeSensorResolutionIss
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
static RESULT OV13850_IsiChangeSensorResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s (enter)  Resolution: %dx%d@%dfps\n", __FUNCTION__,
        ISI_RES_W_GET(Resolution),ISI_RES_H_GET(Resolution), ISI_FPS_GET(Resolution));


    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (pNumberOfFramesToSkip == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pOV13850Ctx->Configured != BOOL_TRUE))
    {
        return RET_WRONG_STATE;
    }

    IsiSensorCaps_t Caps;
    Caps.Index = 0;
    Caps.Resolution = 0;
    while (OV13850_IsiGetCapsIss( handle, &Caps) == RET_SUCCESS) {
        if (Resolution == Caps.Resolution) {            
            break;
        }
        Caps.Index++;
    }

    if ( (Resolution & Caps.Resolution) == 0 )
    {
        return RET_OUTOFRANGE;
    }

    if ( Resolution == pOV13850Ctx->Config.Resolution )
    {
        // well, no need to worry
        *pNumberOfFramesToSkip = 0;
    }
    else
    {
        // change resolution
        char *szResName = NULL;

        bool_t res_no_chg;

        if (!((ISI_RES_W_GET(Resolution)==ISI_RES_W_GET(pOV13850Ctx->Config.Resolution)) && 
            (ISI_RES_W_GET(Resolution)==ISI_RES_W_GET(pOV13850Ctx->Config.Resolution))) ) {

            if (pOV13850Ctx->Streaming != BOOL_FALSE) {
                TRACE( OV13850_ERROR, "%s: Sensor is streaming, Change resolution is not allow\n",__FUNCTION__);
                return RET_WRONG_STATE;
            }
            res_no_chg = BOOL_FALSE;
        } else {
            res_no_chg = BOOL_TRUE;
        }
        		
        result = IsiGetResolutionName( Resolution, &szResName );
        TRACE( OV13850_INFO, "%s: NewRes=0x%08x (%s)\n", __FUNCTION__, Resolution, szResName);

        // update resolution in copy of config in context
        pOV13850Ctx->Config.Resolution = Resolution;

        // tell sensor about that
        result = OV13850_SetupOutputWindowInternal( pOV13850Ctx, &pOV13850Ctx->Config,BOOL_TRUE, res_no_chg);
        if ( result != RET_SUCCESS )
        {
            TRACE( OV13850_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
            return ( result );
        }

        // remember old exposure values
        float OldGain = pOV13850Ctx->AecCurGain;
        float OldIntegrationTime = pOV13850Ctx->AecCurIntegrationTime;

        // update limits & stuff (reset current & old settings)
        result = OV13850_AecSetModeParameters( pOV13850Ctx, &pOV13850Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV13850_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
            return ( result );
        }

        // restore old exposure values (at least within new exposure values' limits)
        uint8_t NumberOfFramesToSkip;
        float   DummySetGain;
        float   DummySetIntegrationTime;
        result = OV13850_IsiExposureControlIss( handle, OldGain, OldIntegrationTime, &NumberOfFramesToSkip, &DummySetGain, &DummySetIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV13850_ERROR, "%s: OV13850_IsiExposureControlIss failed.\n", __FUNCTION__);
            return ( result );
        }

        // return number of frames that aren't exposed correctly
        if (res_no_chg == BOOL_TRUE)
            *pNumberOfFramesToSkip = 0;
        else         
        	*pNumberOfFramesToSkip = NumberOfFramesToSkip + 1;
    }

    TRACE( OV13850_INFO, "%s (exit)  result: 0x%x   pNumberOfFramesToSkip: %d \n", __FUNCTION__, result,
        *pNumberOfFramesToSkip);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiSensorSetStreamingIss
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
static RESULT OV13850_IsiSensorSetStreamingIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    uint32_t RegValue = 0;
	uint32_t RegValue2 = 0;

    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;
	
    TRACE( OV13850_INFO, "%s (enter)  on = %d\n", __FUNCTION__,on);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pOV13850Ctx->Configured != BOOL_TRUE) || (pOV13850Ctx->Streaming == on) )
    {
        return RET_WRONG_STATE;
    }

    if (on == BOOL_TRUE)
    {
        /* enable streaming */
        result = OV13850_IsiRegReadIss ( pOV13850Ctx, OV13850_MODE_SELECT, &RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV13850_IsiRegWriteIss ( pOV13850Ctx, OV13850_MODE_SELECT, (RegValue | 0x01U) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
		
    }
    else
    {   
        /* disable streaming */
        result = OV13850_IsiRegReadIss ( pOV13850Ctx, OV13850_MODE_SELECT, &RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV13850_IsiRegWriteIss ( pOV13850Ctx, OV13850_MODE_SELECT, (RegValue & ~0x01U) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        TRACE(OV13850_ERROR," STREAM OFF ++++++++++++++");
    }

    if (result == RET_SUCCESS)
    {
        pOV13850Ctx->Streaming = on;
    }

    TRACE( OV13850_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiSensorSetPowerIss
 *
 * @brief   Performs the power-up/power-down sequence of the camera, if possible.
 *
 * @param   handle      OV13850 sensor instance handle
 * @param   on          new power state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiSensorSetPowerIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pOV13850Ctx->Configured = BOOL_FALSE;
    pOV13850Ctx->Streaming  = BOOL_FALSE;

    result = HalSetPower( pOV13850Ctx->IsiCtx.HalHandle, pOV13850Ctx->IsiCtx.HalDevID, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    result = HalSetReset( pOV13850Ctx->IsiCtx.HalHandle, pOV13850Ctx->IsiCtx.HalDevID, true );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    if (on == BOOL_TRUE)
    {
        result = HalSetPower( pOV13850Ctx->IsiCtx.HalHandle, pOV13850Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        result = HalSetReset( pOV13850Ctx->IsiCtx.HalHandle, pOV13850Ctx->IsiCtx.HalDevID, false );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        result = HalSetReset( pOV13850Ctx->IsiCtx.HalHandle, pOV13850Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        result = HalSetReset( pOV13850Ctx->IsiCtx.HalHandle, pOV13850Ctx->IsiCtx.HalDevID, false );

        osSleep( 50 );
    }

    TRACE( OV13850_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiCheckSensorConnectionIss
 *
 * @brief   Checks the I2C-Connection to sensor by reading sensor revision id.
 *
 * @param   handle      OV13850 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiCheckSensorConnectionIss
(
    IsiSensorHandle_t   handle
)
{
    uint32_t RevId;
    uint32_t value;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    RevId = OV13850_CHIP_ID_HIGH_BYTE_DEFAULT;
    RevId = (RevId << 8U) | (OV13850_CHIP_ID_LOW_BYTE_DEFAULT);

    result = OV13850_IsiGetSensorRevisionIss( handle, &value );
    if ( (result != RET_SUCCESS) || (RevId != value) )
    {
        TRACE( OV13850_ERROR, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );
        return ( RET_FAILURE );
    }


    TRACE( OV13850_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetSensorRevisionIss
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
static RESULT OV13850_IsiGetSensorRevisionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    uint32_t data;
	uint32_t vcm_pos = MAX_LOG;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *p_value = 0U;
    result = OV13850_IsiRegReadIss ( handle, OV13850_CHIP_ID_HIGH_BYTE, &data );
    *p_value = ( (data & 0xFF) << 8U );
    result = OV13850_IsiRegReadIss ( handle, OV13850_CHIP_ID_LOW_BYTE, &data );
    *p_value |= ( (data & 0xFF) );

    TRACE( OV13850_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiRegReadIss
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
static RESULT OV13850_IsiRegReadIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

  //  TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

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
        uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, OV13850_g_aRegDescription_twolane);
        if ( !NrOfBytes )
        {
            NrOfBytes = 1;
        }
 //       TRACE( OV13850_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);

        *p_value = 0;
        result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
    }

  //  TRACE( OV13850_ERROR, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiRegWriteIss
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
static RESULT OV13850_IsiRegWriteIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NrOfBytes;

  //  TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    NrOfBytes = IsiGetNrDatBytesIss( address, OV13850_g_aRegDescription_twolane);
    if ( !NrOfBytes )
    {
        NrOfBytes = 1;
    }
//    TRACE( OV13850_REG_DEBUG, "%s (IsiGetNrDatBytesIss %d 0x%08x 0x%08x)\n", __FUNCTION__, NrOfBytes, address, value);

    result = IsiI2cWriteSensorRegister( handle, address, (uint8_t *)(&value), NrOfBytes, BOOL_TRUE );

//    TRACE( OV13850_ERROR, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetGainLimitsIss
 *
 * @brief   Returns the exposure minimal and maximal values of an
 *          OV13850 instance
 *
 * @param   handle       OV13850 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetGainLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinGain,
    float               *pMaxGain
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinGain == NULL) || (pMaxGain == NULL) )
    {
        TRACE( OV13850_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinGain = pOV13850Ctx->AecMinGain;
    *pMaxGain = pOV13850Ctx->AecMaxGain;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetIntegrationTimeLimitsIss
 *
 * @brief   Returns the minimal and maximal integration time values of an
 *          OV13850 instance
 *
 * @param   handle       OV13850 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetIntegrationTimeLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinIntegrationTime,
    float               *pMaxIntegrationTime
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL) )
    {
        TRACE( OV13850_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinIntegrationTime = pOV13850Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pOV13850Ctx->AecMaxIntegrationTime;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV13850_IsiGetGainIss
 *
 * @brief   Reads gain values from the image sensor module.
 *
 * @param   handle                  OV13850 sensor instance handle
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV13850_IsiGetGainIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain = pOV13850Ctx->AecCurGain;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetGainIncrementIss
 *
 * @brief   Get smallest possible gain increment.
 *
 * @param   handle                  OV13850 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV13850_IsiGetGainIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV13850Ctx->AecGainIncrement;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiSetGainIss
 *
 * @brief   Writes gain values to the image sensor module.
 *          Updates current gain and exposure in sensor struct/state.
 *
 * @param   handle                  OV13850 sensor instance handle
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
RESULT OV13850_IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint16_t usGain = 0;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        TRACE( OV13850_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

  
    if( NewGain < pOV13850Ctx->AecMinGain ) NewGain = pOV13850Ctx->AecMinGain;
    if( NewGain > pOV13850Ctx->AecMaxGain ) NewGain = pOV13850Ctx->AecMaxGain;

    usGain = (uint16_t)(NewGain * 16.0f+0.5);

    // write new gain into sensor registers, do not write if nothing has changed
    if( (usGain != pOV13850Ctx->OldGain) )
    {
        result = OV13850_IsiRegWriteIss( pOV13850Ctx, 0x350a, (usGain>>8)&0x03);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV13850_IsiRegWriteIss( pOV13850Ctx, 0x350b, (usGain&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        pOV13850Ctx->OldGain = usGain;
    }

    //calculate gain actually set
    pOV13850Ctx->AecCurGain = ( (float)usGain ) / 16.0f;

    //return current state
    *pSetGain = pOV13850Ctx->AecCurGain;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

     

/*****************************************************************************/
/**
 *          OV13850_IsiGetIntegrationTimeIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  OV13850 sensor instance handle
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV13850_IsiGetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               *pSetIntegrationTime
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetIntegrationTime == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetIntegrationTime = pOV13850Ctx->AecCurIntegrationTime;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetIntegrationTimeIncrementIss
 *
 * @brief   Get smallest possible integration time increment.
 *
 * @param   handle                  OV13850 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV13850_IsiGetIntegrationTimeIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV13850Ctx->AecIntegrationTimeIncrement;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiSetIntegrationTimeIss
 *
 * @brief   Writes gain and integration time values to the image sensor module.
 *          Updates current integration time and exposure in sensor
 *          struct/state.
 *
 * @param   handle                  OV13850 sensor instance handle
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
RESULT OV13850_IsiSetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               NewIntegrationTime,
    float               *pSetIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t CoarseIntegrationTime = 0;
    //uint32_t FineIntegrationTime   = 0; //not supported by OV13850

    float ShutterWidthPck = 0.0f; //shutter width in pixel clock periods

    TRACE( OV13850_INFO, "%s: (enter) NewIntegrationTime: %f (min: %f   max: %f)\n", __FUNCTION__,
        NewIntegrationTime,
        pOV13850Ctx->AecMinIntegrationTime,
        pOV13850Ctx->AecMaxIntegrationTime);


    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetIntegrationTime == NULL) || (pNumberOfFramesToSkip == NULL) )
    {
        TRACE( OV13850_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    //maximum and minimum integration time is limited by the sensor, if this limit is not
    //considered, the exposure control loop needs lots of time to return to a new state
    //so limit to allowed range
    if ( NewIntegrationTime > pOV13850Ctx->AecMaxIntegrationTime ) NewIntegrationTime = pOV13850Ctx->AecMaxIntegrationTime;
    if ( NewIntegrationTime < pOV13850Ctx->AecMinIntegrationTime ) NewIntegrationTime = pOV13850Ctx->AecMinIntegrationTime;

    //the actual integration time is given by
    //integration_time = ( coarse_integration_time * line_length_pck + fine_integration_time ) / vt_pix_clk_freq
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck )
    //fine_integration_time   = integration_time * vt_pix_clk_freq - coarse_integration_time * line_length_pck
    //
    //fine integration is not supported by OV13850
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck + 0.5 )

    ShutterWidthPck = NewIntegrationTime * ( (float)pOV13850Ctx->VtPixClkFreq );

    // avoid division by zero
    if ( pOV13850Ctx->LineLengthPck == 0 )
    {
        TRACE( OV13850_ERROR, "%s: Division by zero!\n", __FUNCTION__ );
        return ( RET_DIVISION_BY_ZERO );
    }

    //calculate the integer part of the integration time in units of line length
    //calculate the fractional part of the integration time in units of pixel clocks
    //CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV13850Ctx->LineLengthPck) );
    //FineIntegrationTime   = ( (uint32_t)ShutterWidthPck ) - ( CoarseIntegrationTime * pOV13850Ctx->LineLengthPck );
    CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV13850Ctx->LineLengthPck) + 0.5f );

    // write new integration time into sensor registers
    // do not write if nothing has changed
    if( CoarseIntegrationTime != pOV13850Ctx->OldCoarseIntegrationTime )
    {
        result = OV13850_IsiRegWriteIss( pOV13850Ctx, 0x3500, (CoarseIntegrationTime & 0x0000F000U) >> 12U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV13850_IsiRegWriteIss( pOV13850Ctx, 0x3501, (CoarseIntegrationTime & 0x00000FF0U) >> 4U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV13850_IsiRegWriteIss( pOV13850Ctx, 0x3502, (CoarseIntegrationTime & 0x0000000FU) << 4U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );


        pOV13850Ctx->OldCoarseIntegrationTime = CoarseIntegrationTime;   // remember current integration time
        *pNumberOfFramesToSkip = 1U; //skip 1 frame
    }
    else
    {
        *pNumberOfFramesToSkip = 0U; //no frame skip
    }

    //if( FineIntegrationTime != pOV13850Ctx->OldFineIntegrationTime )
    //{
    //    result = OV13850_IsiRegWriteIss( pOV13850Ctx, ... , FineIntegrationTime );
    //    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    //    pOV13850Ctx->OldFineIntegrationTime = FineIntegrationTime; //remember current integration time
    //    *pNumberOfFramesToSkip = 1U; //skip 1 frame
    //}

    //calculate integration time actually set
    //pOV13850Ctx->AecCurIntegrationTime = ( ((float)CoarseIntegrationTime) * ((float)pOV13850Ctx->LineLengthPck) + ((float)FineIntegrationTime) ) / pOV13850Ctx->VtPixClkFreq;
    pOV13850Ctx->AecCurIntegrationTime = ((float)CoarseIntegrationTime) * ((float)pOV13850Ctx->LineLengthPck) / pOV13850Ctx->VtPixClkFreq;

    //return current state
    *pSetIntegrationTime = pOV13850Ctx->AecCurIntegrationTime;

   // TRACE( OV13850_ERROR, "%s: SetTi=%f NewTi=%f\n", __FUNCTION__, *pSetIntegrationTime,NewIntegrationTime);   
    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}




/*****************************************************************************/
/**
 *          OV13850_IsiExposureControlIss
 *
 * @brief   Camera hardware dependent part of the exposure control loop.
 *          Calculates appropriate register settings from the new exposure
 *          values and writes them to the image sensor module.
 *
 * @param   handle                  OV13850 sensor instance handle
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
RESULT OV13850_IsiExposureControlIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               NewIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_DEBUG, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pNumberOfFramesToSkip == NULL)
            || (pSetGain == NULL)
            || (pSetIntegrationTime == NULL) )
    {
        TRACE( OV13850_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    TRACE( OV13850_DEBUG, "%s: g=%f, Ti=%f\n", __FUNCTION__, NewGain, NewIntegrationTime );


    result = OV13850_IsiSetIntegrationTimeIss( handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip );
    result = OV13850_IsiSetGainIss( handle, NewGain, pSetGain );

    TRACE( OV13850_DEBUG, "%s: set: g=%f, Ti=%f, skip=%d\n", __FUNCTION__, *pSetGain, *pSetIntegrationTime, *pNumberOfFramesToSkip );
    TRACE( OV13850_DEBUG, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetCurrentExposureIss
 *
 * @brief   Returns the currently adjusted AE values
 *
 * @param   handle                  OV13850 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV13850_IsiGetCurrentExposureIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetGain == NULL) || (pSetIntegrationTime == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain            = pOV13850Ctx->AecCurGain;
    *pSetIntegrationTime = pOV13850Ctx->AecCurIntegrationTime;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetResolutionIss
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
RESULT OV13850_IsiGetResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *pSetResolution
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetResolution == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetResolution = pOV13850Ctx->Config.Resolution;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetAfpsInfoHelperIss
 *
 * @brief   Calc AFPS sub resolution settings for the given resolution
 *
 * @param   pOV13850Ctx             OV13850 sensor instance (dummy!) context
 * @param   Resolution              Any supported resolution to query AFPS params for
 * @param   pAfpsInfo               Reference of AFPS info structure to write the results to
 * @param   AfpsStageIdx            Index of current AFPS stage to use
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetAfpsInfoHelperIss(
    OV13850_Context_t   *pOV13850Ctx,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo,
    uint32_t            AfpsStageIdx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT(pOV13850Ctx != NULL);
    DCT_ASSERT(pAfpsInfo != NULL);
    DCT_ASSERT(AfpsStageIdx <= ISI_NUM_AFPS_STAGES);

    // update resolution in copy of config in context
    pOV13850Ctx->Config.Resolution = Resolution;

    // tell sensor about that
    result = OV13850_SetupOutputWindowInternal( pOV13850Ctx, &pOV13850Ctx->Config,BOOL_FALSE,BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV13850_ERROR, "%s: SetupOutputWindow failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // update limits & stuff (reset current & old settings)
    result = OV13850_AecSetModeParameters( pOV13850Ctx, &pOV13850Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV13850_ERROR, "%s: AecSetModeParameters failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // take over params
    pAfpsInfo->Stage[AfpsStageIdx].Resolution = Resolution;
    pAfpsInfo->Stage[AfpsStageIdx].MaxIntTime = pOV13850Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecMinGain           = pOV13850Ctx->AecMinGain;
    pAfpsInfo->AecMaxGain           = pOV13850Ctx->AecMaxGain;
    pAfpsInfo->AecMinIntTime        = pOV13850Ctx->AecMinIntegrationTime;
    pAfpsInfo->AecMaxIntTime        = pOV13850Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecSlowestResolution = Resolution;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV13850_IsiGetAfpsInfoIss
 *
 * @brief   Returns the possible AFPS sub resolution settings for the given resolution series
 *
 * @param   handle                  OV13850 sensor instance handle
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
RESULT OV13850_IsiGetAfpsInfoIss(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        TRACE( OV13850_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pAfpsInfo == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    // use currently configured resolution?
    if (Resolution == 0)
    {
        Resolution = pOV13850Ctx->Config.Resolution;
    }

    // prepare index
    uint32_t idx = 0;

    // set current resolution data in info struct
    pAfpsInfo->CurrResolution = pOV13850Ctx->Config.Resolution;
    pAfpsInfo->CurrMinIntTime = pOV13850Ctx->AecMinIntegrationTime;
    pAfpsInfo->CurrMaxIntTime = pOV13850Ctx->AecMaxIntegrationTime;

    // allocate dummy context used for Afps parameter calculation as a copy of current context
    OV13850_Context_t *pDummyCtx = (OV13850_Context_t*) malloc( sizeof(OV13850_Context_t) );
    if ( pDummyCtx == NULL )
    {
        TRACE( OV13850_ERROR,  "%s: Can't allocate dummy ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    *pDummyCtx = *pOV13850Ctx;

    // set AFPS mode in dummy context
    pDummyCtx->isAfpsRun = BOOL_TRUE;

#define AFPSCHECKANDADD(_res_) \
    { \
        RESULT lres = OV13850_IsiGetAfpsInfoHelperIss( pDummyCtx, _res_, pAfpsInfo, idx ); \
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
    switch (pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes)
    {
        case SUPPORT_MIPI_ONE_LANE:
        {

            break;
        }

        case SUPPORT_MIPI_TWO_LANE:
        {
    
		    switch(Resolution)
		    {
		        default:
		            TRACE( OV13850_DEBUG,  "%s: Resolution %08x not supported by AFPS\n",  __FUNCTION__, Resolution );
		            result = RET_NOTSUPP;
		            break;
				case ISI_RES_2112_1568P30:
				case ISI_RES_2112_1568P25:
				case ISI_RES_2112_1568P20:
				case ISI_RES_2112_1568P15:
				case ISI_RES_2112_1568P10:
					AFPSCHECKANDADD( ISI_RES_2112_1568P30 );
					AFPSCHECKANDADD( ISI_RES_2112_1568P25 );			
					AFPSCHECKANDADD( ISI_RES_2112_1568P20 );			
					AFPSCHECKANDADD( ISI_RES_2112_1568P15 );			
					AFPSCHECKANDADD( ISI_RES_2112_1568P10 );
					break;
				case ISI_RES_4224_3136P7:
				case ISI_RES_4224_3136P4:
					AFPSCHECKANDADD( ISI_RES_4224_3136P7 );			
					//AFPSCHECKANDADD( ISI_RES_4224_3136P4 );
					break;

		        // check next series here...
		    }
			break;
    	}
        case SUPPORT_MIPI_FOUR_LANE:
        {
		    switch(Resolution)
		    {
		        default:
		            TRACE( OV13850_DEBUG,  "%s: Resolution %08x not supported by AFPS\n",  __FUNCTION__, Resolution );
		            result = RET_NOTSUPP;
		            break;
				case ISI_RES_2112_1568P30:
				case ISI_RES_2112_1568P25:
				case ISI_RES_2112_1568P20:
				case ISI_RES_2112_1568P15:
				case ISI_RES_2112_1568P10:
					AFPSCHECKANDADD( ISI_RES_2112_1568P30 );
					AFPSCHECKANDADD( ISI_RES_2112_1568P25 );			
					AFPSCHECKANDADD( ISI_RES_2112_1568P20 );			
					AFPSCHECKANDADD( ISI_RES_2112_1568P15 );			
					AFPSCHECKANDADD( ISI_RES_2112_1568P10 );
					break;
				case ISI_RES_4224_3136P15:
				case ISI_RES_4224_3136P7:
				case ISI_RES_4224_3136P4:
					AFPSCHECKANDADD( ISI_RES_4224_3136P15 );
					AFPSCHECKANDADD( ISI_RES_4224_3136P7 );			
					//AFPSCHECKANDADD( ISI_RES_4224_3136P4 );
					break;

		        // check next series here...
		    }			
			break;
    	}
        default:
            TRACE( OV13850_ERROR,  "%s: pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes(0x%x) is invalidate!\n", 
                __FUNCTION__, pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes );
            result = RET_FAILURE;
            break;
	}

    // release dummy context again
    free(pDummyCtx);

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetCalibKFactor
 *
 * @brief   Returns the OV13850 specific K-Factor
 *
 * @param   handle       OV13850 sensor instance handle
 * @param   pIsiKFactor  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetCalibKFactor
(
    IsiSensorHandle_t   handle,
    Isi1x1FloatMatrix_t **pIsiKFactor
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiKFactor == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*pIsiKFactor = (Isi1x1FloatMatrix_t *)&OV13850_KFactor;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV13850_IsiGetCalibPcaMatrix
 *
 * @brief   Returns the OV13850 specific PCA-Matrix
 *
 * @param   handle          OV13850 sensor instance handle
 * @param   pIsiPcaMatrix   Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetCalibPcaMatrix
(
    IsiSensorHandle_t   handle,
    Isi3x2FloatMatrix_t **pIsiPcaMatrix
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiPcaMatrix == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*pIsiPcaMatrix = (Isi3x2FloatMatrix_t *)&OV13850_PCAMatrix;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns the sensor specific SvdMean-Vector
 *
 * @param   handle              OV13850 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetCalibSvdMeanValue
(
    IsiSensorHandle_t   handle,
    Isi3x1FloatMatrix_t **pIsiSvdMeanValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSvdMeanValue == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*pIsiSvdMeanValue = (Isi3x1FloatMatrix_t *)&OV13850_SVDMeanValue;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns a pointer to the sensor specific centerline, a straight
 *          line in Hesse normal form in Rg/Bg colorspace
 *
 * @param   handle              OV13850 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetCalibCenterLine
(
    IsiSensorHandle_t   handle,
    IsiLine_t           **ptIsiCenterLine
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiCenterLine == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*ptIsiCenterLine = (IsiLine_t*)&OV13850_CenterLine;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetCalibClipParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for Rg/Bg color
 *          space clipping
 *
 * @param   handle              OV13850 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetCalibClipParam
(
    IsiSensorHandle_t   handle,
    IsiAwbClipParm_t    **pIsiClipParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiClipParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*pIsiClipParam = (IsiAwbClipParm_t *)&OV13850_AwbClipParm;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetCalibGlobalFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle              OV13850 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetCalibGlobalFadeParam
(
    IsiSensorHandle_t       handle,
    IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiGlobalFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*ptIsiGlobalFadeParam = (IsiAwbGlobalFadeParm_t *)&OV13850_AwbGlobalFadeParm;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetCalibFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for near white
 *          pixel parameter calculations
 *
 * @param   handle              OV13850 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiGetCalibFadeParam
(
    IsiSensorHandle_t   handle,
    IsiAwbFade2Parm_t   **ptIsiFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*ptIsiFadeParam = (IsiAwbFade2Parm_t *)&OV13850_AwbFade2Parm;

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV13850_IsiGetIlluProfile
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
static RESULT OV13850_IsiGetIlluProfile
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiIlluProfile_t    **ptIsiIlluProfile
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiIlluProfile == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
    	#if 0
        uint16_t i;

        *ptIsiIlluProfile = NULL;

        /* check if we've a default profile */
        for ( i=0U; i<OV13850_ISIILLUPROFILES_DEFAULT; i++ )
        {
            if ( OV13850_IlluProfileDefault[i].id == CieProfile )
            {
                *ptIsiIlluProfile = &OV13850_IlluProfileDefault[i];
                break;
            }
        }

        result = ( *ptIsiIlluProfile != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
		#endif
    }

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetLscMatrixTable
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
static RESULT OV13850_IsiGetLscMatrixTable
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiLscMatrixTable_t **pLscMatrixTable
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pLscMatrixTable == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
    	#if 0
        uint16_t i;


        switch ( CieProfile )
        {
            case ISI_CIEPROF_A:
            {
                if ( ( pOV13850Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_A_1920x1080;
                }
                #if 0
                else if ( pOV13850Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_A_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV13850_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F2:
            {
                if ( ( pOV13850Ctx->Config.Resolution == ISI_RES_TV1080P30 ) )
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_F2_1920x1080;
                }
                #if 0
                else if ( pOV13850Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_F2_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV13850_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D50:
            {
                if ( ( pOV13850Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_D50_1920x1080;
                }
                #if 0
                else if ( pOV13850Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_D50_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV13850_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D65:
            case ISI_CIEPROF_D75:
            {
                if ( ( pOV13850Ctx->Config.Resolution == ISI_RES_TV1080P30 ) )
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_D65_1920x1080;
                }
                #if 0
                else if ( pOV13850Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_D65_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV13850_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F11:
            {
                if ( ( pOV13850Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_F11_1920x1080;
                }
                #if 0
                else if ( pOV13850Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV13850_LscMatrixTable_CIE_F11_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV13850_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            default:
            {
                TRACE( OV13850_ERROR, "%s: Illumination not supported\n", __FUNCTION__ );
                *pLscMatrixTable = NULL;
                break;
            }
        }

        result = ( *pLscMatrixTable != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
		#endif
    }

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV13850_IsiMdiInitMotoDriveMds
 *
 * @brief   General initialisation tasks like I/O initialisation.
 *
 * @param   handle              OV13850 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiMdiInitMotoDriveMds
(
    IsiSensorHandle_t   handle
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiMdiSetupMotoDrive
 *
 * @brief   Setup of the MotoDrive and return possible max step.
 *
 * @param   handle          OV13850 sensor instance handle
 *          pMaxStep        pointer to variable to receive the maximum
 *                          possible focus step
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiMdiSetupMotoDrive
(
    IsiSensorHandle_t   handle,
    uint32_t            *pMaxStep
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;
	uint32_t vcm_movefull_t;
    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMaxStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    /* ddl@rock-chips.com: v0.3.0 */
    if (pOV13850Ctx->VcmInfo.StepMode <= 7) {
        vcm_movefull_t = 52*(1<<(pOV13850Ctx->VcmInfo.StepMode-1));
    } else if ((pOV13850Ctx->VcmInfo.StepMode>=9) && (pOV13850Ctx->VcmInfo.StepMode<=15)) {
        vcm_movefull_t = 2*(1<<(pOV13850Ctx->VcmInfo.StepMode-9));
    } else {
        TRACE( OV13850_ERROR, "%s: pOV8825Ctx->VcmInfo.StepMode: %d is invalidate!\n",__FUNCTION__, pOV13850Ctx->VcmInfo.StepMode);
        DCT_ASSERT(0);
    }

    *pMaxStep = (MAX_LOG|(vcm_movefull_t<<16));

//    *pMaxStep = MAX_LOG;

    result = OV13850_IsiMdiFocusSet( handle, MAX_LOG );

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiMdiFocusSet
 *
 * @brief   Drives the lens system to a certain focus point.
 *
 * @param   handle          OV13850 sensor instance handle
 *          AbsStep         absolute focus point to apply
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiMdiFocusSet
(
    IsiSensorHandle_t   handle,
    const uint32_t      Position
)
{
	OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t nPosition;
    uint8_t  data[2] = { 0, 0 };

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
    	TRACE( OV13850_ERROR, "%s: pOV13850Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    /* map 64 to 0 -> infinity */
    //nPosition = ( Position >= MAX_LOG ) ? 0 : ( MAX_REG - (Position * 16U) );
	if( Position > MAX_LOG ){
		TRACE( OV13850_ERROR, "%s: pOV13850Ctx Position (%d) max_position(%d)\n", __FUNCTION__,Position, MAX_LOG);
		//Position = MAX_LOG;
	}
	/* ddl@rock-chips.com: v0.3.0 */
    if ( Position >= MAX_LOG )
        nPosition = pOV13850Ctx->VcmInfo.StartCurrent;
    else 
        nPosition = pOV13850Ctx->VcmInfo.StartCurrent + (pOV13850Ctx->VcmInfo.Step*(MAX_LOG-Position));
    /* ddl@rock-chips.com: v0.6.0 */
    if (nPosition > MAX_VCMDRV_REG)  
        nPosition = MAX_VCMDRV_REG;

    TRACE( OV13850_DEBUG, "%s: focus set position_reg_value(%d) position(%d) \n", __FUNCTION__, nPosition, Position);

    data[0] = (uint8_t)(0x00U | (( nPosition & 0x3F0U ) >> 4U));                 // PD,  1, D9..D4, see AD5820 datasheet
    data[1] = (uint8_t)( ((nPosition & 0x0FU) << 4U) | MDI_SLEW_RATE_CTRL );    // D3..D0, S3..S0

    TRACE( OV13850_DEBUG, "%s: value = %d, 0x%02x 0x%02x af_addr(0x%x) bus(%d)\n", __FUNCTION__, nPosition, data[0], data[1],pOV13850Ctx->IsiCtx.SlaveAfAddress,pOV13850Ctx->IsiCtx.I2cAfBusNum );

    result = HalWriteI2CMem( pOV13850Ctx->IsiCtx.HalHandle,
                             pOV13850Ctx->IsiCtx.I2cAfBusNum,
                             pOV13850Ctx->IsiCtx.SlaveAfAddress,
                             0,
                             pOV13850Ctx->IsiCtx.NrOfAfAddressBytes,
                             data,
                             2U );
	RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );


    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);
    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiMdiFocusGet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          OV13850 sensor instance handle
 *          pAbsStep        pointer to a variable to receive the current
 *                          focus point
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiMdiFocusGet
(
    IsiSensorHandle_t   handle,
    uint32_t            *pAbsStep
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;
    uint8_t  data[2] = { 0, 0 };


    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pAbsStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    result = HalReadI2CMem( pOV13850Ctx->IsiCtx.HalHandle,
                            pOV13850Ctx->IsiCtx.I2cAfBusNum,
                            pOV13850Ctx->IsiCtx.SlaveAfAddress,
                            0,
                            pOV13850Ctx->IsiCtx.NrOfAfAddressBytes,
                            data,
                            2U );

    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV13850_DEBUG, "%s: value = 0x%02x 0x%02x\n", __FUNCTION__, data[0], data[1] );

    /* Data[0] = PD,  1, D9..D4, see AD5820 datasheet */
    /* Data[1] = D3..D0, S3..S0 */
	#if 0
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
	#endif
	*pAbsStep = ( ((uint32_t)(data[0] & 0x3FU)) << 4U ) | ( ((uint32_t)data[1]) >> 4U );


    /* map 0 to 64 -> infinity */   /* ddl@rock-chips.com: v0.3.0 */
    if( *pAbsStep <= pOV13850Ctx->VcmInfo.StartCurrent)
    {
        *pAbsStep = MAX_LOG;
    }
    else if((*pAbsStep>pOV13850Ctx->VcmInfo.StartCurrent) && (*pAbsStep<=pOV13850Ctx->VcmInfo.RatedCurrent))
    {
        *pAbsStep = (pOV13850Ctx->VcmInfo.RatedCurrent - *pAbsStep ) / pOV13850Ctx->VcmInfo.Step;
    }
	else
	{
		*pAbsStep = 0;
	}
	

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);


    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiMdiFocusCalibrate
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV13850 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV13850_IsiMdiFocusCalibrate
(
    IsiSensorHandle_t   handle
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiActivateTestPattern
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV13850 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV13850_IsiActivateTestPattern
(
    IsiSensorHandle_t   handle,
    const bool_t        enable
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t ulRegValue = 0UL;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( BOOL_TRUE == enable )
    {
        /* enable test-pattern */
        result = OV13850_IsiRegReadIss( pOV13850Ctx, 0x5e00, &ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        ulRegValue |= ( 0x80U );

        result = OV13850_IsiRegWriteIss( pOV13850Ctx, 0x5e00, ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }
    else
    {
        /* disable test-pattern */
        result = OV13850_IsiRegReadIss( pOV13850Ctx, 0x5e00, &ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        ulRegValue &= ~( 0x80 );

        result = OV13850_IsiRegWriteIss( pOV13850Ctx, 0x5e00, ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }

     pOV13850Ctx->TestPattern = enable;
    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV13850_IsiGetSensorMipiInfoIss
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV13850 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT OV13850_IsiGetSensorMipiInfoIss
(
    IsiSensorHandle_t   handle,
    IsiSensorMipiInfo   *ptIsiSensorMipiInfo
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }


    if ( ptIsiSensorMipiInfo == NULL )
    {
        return ( result );
    }

    ptIsiSensorMipiInfo->ucMipiLanes = pOV13850Ctx->IsiSensorMipiInfo.ucMipiLanes;
    ptIsiSensorMipiInfo->ulMipiFreq= pOV13850Ctx->IsiSensorMipiInfo.ulMipiFreq;
    ptIsiSensorMipiInfo->sensorHalDevID = pOV13850Ctx->IsiSensorMipiInfo.sensorHalDevID;


    TRACE( OV13850_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV13850_IsiGetSensorIsiVersion
(  IsiSensorHandle_t   handle,
   unsigned int*     pVersion
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
    	TRACE( OV13850_ERROR, "%s: pOV13850Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pVersion == NULL)
	{
		TRACE( OV13850_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pVersion = CONFIG_ISI_VERSION;
	return result;
}

static RESULT OV13850_IsiGetSensorTuningXmlVersion
(  IsiSensorHandle_t   handle,
   char**     pTuningXmlVersion
)
{
    OV13850_Context_t *pOV13850Ctx = (OV13850_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV13850Ctx == NULL )
    {
    	TRACE( OV13850_ERROR, "%s: pOV13850Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pTuningXmlVersion == NULL)
	{
		TRACE( OV13850_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pTuningXmlVersion = OV13850_NEWEST_TUNING_XML;
	return result;
}


/*****************************************************************************/
/**
 *          OV13850_IsiGetSensorIss
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
RESULT OV13850_IsiGetSensorIss
(
    IsiSensor_t *pIsiSensor
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV13850_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pIsiSensor != NULL )
    {
        pIsiSensor->pszName                             = OV13850_g_acName;
        pIsiSensor->pRegisterTable                      = OV13850_g_aRegDescription_twolane;
        pIsiSensor->pIsiSensorCaps                      = &OV13850_g_IsiSensorDefaultConfig;
		pIsiSensor->pIsiGetSensorIsiVer					= OV13850_IsiGetSensorIsiVersion;//oyyf
		pIsiSensor->pIsiGetSensorTuningXmlVersion		= OV13850_IsiGetSensorTuningXmlVersion;//oyyf
        pIsiSensor->pIsiCreateSensorIss                 = OV13850_IsiCreateSensorIss;
        pIsiSensor->pIsiReleaseSensorIss                = OV13850_IsiReleaseSensorIss;
        pIsiSensor->pIsiGetCapsIss                      = OV13850_IsiGetCapsIss;
        pIsiSensor->pIsiSetupSensorIss                  = OV13850_IsiSetupSensorIss;
        pIsiSensor->pIsiChangeSensorResolutionIss       = OV13850_IsiChangeSensorResolutionIss;
        pIsiSensor->pIsiSensorSetStreamingIss           = OV13850_IsiSensorSetStreamingIss;
        pIsiSensor->pIsiSensorSetPowerIss               = OV13850_IsiSensorSetPowerIss;
        pIsiSensor->pIsiCheckSensorConnectionIss        = OV13850_IsiCheckSensorConnectionIss;
        pIsiSensor->pIsiGetSensorRevisionIss            = OV13850_IsiGetSensorRevisionIss;
        pIsiSensor->pIsiRegisterReadIss                 = OV13850_IsiRegReadIss;
        pIsiSensor->pIsiRegisterWriteIss                = OV13850_IsiRegWriteIss;

        /* AEC functions */
        pIsiSensor->pIsiExposureControlIss              = OV13850_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = OV13850_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = OV13850_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetCurrentExposureIss           = OV13850_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetGainIss                      = OV13850_IsiGetGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = OV13850_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = OV13850_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = OV13850_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = OV13850_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = OV13850_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetResolutionIss                = OV13850_IsiGetResolutionIss;
        pIsiSensor->pIsiGetAfpsInfoIss                  = OV13850_IsiGetAfpsInfoIss;

        /* AWB specific functions */
        pIsiSensor->pIsiGetCalibKFactor                 = OV13850_IsiGetCalibKFactor;
        pIsiSensor->pIsiGetCalibPcaMatrix               = OV13850_IsiGetCalibPcaMatrix;
        pIsiSensor->pIsiGetCalibSvdMeanValue            = OV13850_IsiGetCalibSvdMeanValue;
        pIsiSensor->pIsiGetCalibCenterLine              = OV13850_IsiGetCalibCenterLine;
        pIsiSensor->pIsiGetCalibClipParam               = OV13850_IsiGetCalibClipParam;
        pIsiSensor->pIsiGetCalibGlobalFadeParam         = OV13850_IsiGetCalibGlobalFadeParam;
        pIsiSensor->pIsiGetCalibFadeParam               = OV13850_IsiGetCalibFadeParam;
        pIsiSensor->pIsiGetIlluProfile                  = OV13850_IsiGetIlluProfile;
        pIsiSensor->pIsiGetLscMatrixTable               = OV13850_IsiGetLscMatrixTable;

        /* AF functions */
        pIsiSensor->pIsiMdiInitMotoDriveMds             = OV13850_IsiMdiInitMotoDriveMds;
        pIsiSensor->pIsiMdiSetupMotoDrive               = OV13850_IsiMdiSetupMotoDrive;
        pIsiSensor->pIsiMdiFocusSet                     = OV13850_IsiMdiFocusSet;
        pIsiSensor->pIsiMdiFocusGet                     = OV13850_IsiMdiFocusGet;
        pIsiSensor->pIsiMdiFocusCalibrate               = OV13850_IsiMdiFocusCalibrate;

        /* MIPI */
        pIsiSensor->pIsiGetSensorMipiInfoIss            = OV13850_IsiGetSensorMipiInfoIss;

        /* Testpattern */
        pIsiSensor->pIsiActivateTestPattern             = OV13850_IsiActivateTestPattern;
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( OV13850_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV13850_IsiGetSensorI2cInfo(sensor_i2c_info_t** pdata)
{
    sensor_i2c_info_t* pSensorI2cInfo;

    pSensorI2cInfo = ( sensor_i2c_info_t * )malloc ( sizeof (sensor_i2c_info_t) );

    if ( pSensorI2cInfo == NULL )
    {
        TRACE( OV13850_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pSensorI2cInfo, 0, sizeof( sensor_i2c_info_t ) );

    
    pSensorI2cInfo->i2c_addr = OV13850_SLAVE_ADDR;
    pSensorI2cInfo->i2c_addr2 = OV13850_SLAVE_ADDR2;
    pSensorI2cInfo->soft_reg_addr = OV13850_SOFTWARE_RST;
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
                while(OV13850_IsiGetCapsIssInternal(&Caps,lanes)==RET_SUCCESS) {
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
    pChipIDInfo_H->chipid_reg_addr = OV13850_CHIP_ID_HIGH_BYTE;  
    pChipIDInfo_H->chipid_reg_value = OV13850_CHIP_ID_HIGH_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_H );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_H );

    sensor_chipid_info_t* pChipIDInfo_L = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_L )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_L, 0, sizeof(*pChipIDInfo_L) ); 
    pChipIDInfo_L->chipid_reg_addr = OV13850_CHIP_ID_LOW_BYTE;
    pChipIDInfo_L->chipid_reg_value = OV13850_CHIP_ID_LOW_BYTE_DEFAULT;
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
    OV13850_IsiGetSensorIss,
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
    OV13850_IsiGetSensorI2cInfo,
};


