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
 * @file TC358749XBG.c
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

#include "TC358749XBG_priv.h"


#define CC_OFFSET_SCALING  2.0f
#define I2C_COMPLIANT_STARTBIT 1U

#define TC358749XBG_HDMI2MIPI 1


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( TC358749XBG_INFO , "TC358749XBG: ", INFO,    1U );
CREATE_TRACER( TC358749XBG_WARN , "TC358749XBG: ", WARNING, 1U );
CREATE_TRACER( TC358749XBG_ERROR, "TC358749XBG: ", ERROR,   1U );

CREATE_TRACER( TC358749XBG_DEBUG, "TC358749XBG: ", INFO,     1U );

CREATE_TRACER( TC358749XBG_REG_INFO , "TC358749XBG: ", INFO, 1U);
CREATE_TRACER( TC358749XBG_REG_DEBUG, "TC358749XBG: ", INFO, 1U );

#define TC358749XBG_SLAVE_ADDR       0x1FU //0x20                           /**< i2c slave address of the TC358749XBG camera sensor */
#define TC358749XBG_SLAVE_ADDR2       0x0F
#define TC358749XBG_SLAVE_AF_ADDR    0x00U                           /**< i2c slave address of the TC358749XBG integrated AD5820 */



/******************************************************************************
 * local variable declarations
 *****************************************************************************/
const char TC358749XBG_g_acName[] = "TC358749XBG_ SOC_PARREL";
extern const IsiRegDescription_t TC358749XBG_g_aRegDescription[];
extern const IsiRegDescription_t TC358749XBG_g_svga[];
extern const IsiRegDescription_t TC358749XBG_g_1600x1200[];
extern const IsiRegDescription_t TC358749XBG_g_hdmi_input_check[];
extern const IsiRegDescription_t TC358749XBG_g_edio[];
extern const IsiRegDescription_t TC358749XBG_g_aRegVedioON[];




const IsiSensorCaps_t TC358749XBG_g_IsiSensorDefaultConfig;


#define TC358749XBG_I2C_START_BIT        (I2C_COMPLIANT_STARTBIT)    // I2C bus start condition
#define TC358749XBG_I2C_NR_ADR_BYTES     (2U)                        // 1 byte base address and 2 bytes sub address
#define TC358749XBG_I2C_NR_DAT_BYTES     (2U)                        // 16 bit registers

static uint16_t g_suppoted_mipi_lanenum_type = SUPPORT_MIPI_FOUR_LANE;
#define DEFAULT_NUM_LANES SUPPORT_MIPI_ONE_LANE


/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static RESULT TC358749XBG_IsiCreateSensorIss( IsiSensorInstanceConfig_t *pConfig );
static RESULT TC358749XBG_IsiReleaseSensorIss( IsiSensorHandle_t handle );
static RESULT TC358749XBG_IsiGetCapsIss( IsiSensorHandle_t handle, IsiSensorCaps_t *pIsiSensorCaps );
static RESULT TC358749XBG_IsiSetupSensorIss( IsiSensorHandle_t handle, const IsiSensorConfig_t *pConfig );
static RESULT TC358749XBG_IsiSensorSetStreamingIss( IsiSensorHandle_t handle, bool_t on );
static RESULT TC358749XBG_IsiSensorSetPowerIss( IsiSensorHandle_t handle, bool_t on );
static RESULT TC358749XBG_IsiCheckSensorConnectionIss( IsiSensorHandle_t handle );
static RESULT TC358749XBG_IsiGetSensorRevisionIss( IsiSensorHandle_t handle, uint32_t *p_value);

static RESULT TC358749XBG_IsiGetGainLimitsIss( IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain);
static RESULT TC358749XBG_IsiGetIntegrationTimeLimitsIss( IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime );
static RESULT TC358749XBG_IsiExposureControlIss( IsiSensorHandle_t handle, float NewGain, float NewIntegrationTime, uint8_t *pNumberOfFramesToSkip, float *pSetGain, float *pSetIntegrationTime );
static RESULT TC358749XBG_IsiGetCurrentExposureIss( IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime );
static RESULT TC358749XBG_IsiGetAfpsInfoIss ( IsiSensorHandle_t handle, uint32_t Resolution, IsiAfpsInfo_t* pAfpsInfo);
static RESULT TC358749XBG_IsiGetGainIss( IsiSensorHandle_t handle, float *pSetGain );
static RESULT TC358749XBG_IsiGetGainIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT TC358749XBG_IsiSetGainIss( IsiSensorHandle_t handle, float NewGain, float *pSetGain );
static RESULT TC358749XBG_IsiGetIntegrationTimeIss( IsiSensorHandle_t handle, float *pSetIntegrationTime );
static RESULT TC358749XBG_IsiGetIntegrationTimeIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT TC358749XBG_IsiSetIntegrationTimeIss( IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t *pNumberOfFramesToSkip );
static RESULT TC358749XBG_IsiGetResolutionIss( IsiSensorHandle_t handle, uint32_t *pSetResolution );

static RESULT TC358749XBG_IsiRegVedioReadIss( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );

static RESULT TC358749XBG_IsiRegReadIss( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );
static RESULT TC358749XBG_IsiRegVedioWriteIss( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

static RESULT TC358749XBG_IsiRegWriteIss( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

static RESULT TC358749XBG_IsiGetCalibKFactor( IsiSensorHandle_t handle, Isi1x1FloatMatrix_t **pIsiKFactor );
static RESULT TC358749XBG_IsiGetCalibPcaMatrix( IsiSensorHandle_t   handle, Isi3x2FloatMatrix_t **pIsiPcaMatrix );
static RESULT TC358749XBG_IsiGetCalibSvdMeanValue( IsiSensorHandle_t   handle, Isi3x1FloatMatrix_t **pIsiSvdMeanValue );
static RESULT TC358749XBG_IsiGetCalibCenterLine( IsiSensorHandle_t   handle, IsiLine_t  **ptIsiCenterLine);
static RESULT TC358749XBG_IsiGetCalibClipParam( IsiSensorHandle_t   handle, IsiAwbClipParm_t    **pIsiClipParam );
static RESULT TC358749XBG_IsiGetCalibGlobalFadeParam( IsiSensorHandle_t       handle, IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam);
static RESULT TC358749XBG_IsiGetCalibFadeParam( IsiSensorHandle_t   handle, IsiAwbFade2Parm_t   **ptIsiFadeParam);
static RESULT TC358749XBG_IsiGetIlluProfile( IsiSensorHandle_t   handle, const uint32_t CieProfile, IsiIlluProfile_t **ptIsiIlluProfile );

static RESULT TC358749XBG_IsiMdiInitMotoDriveMds( IsiSensorHandle_t handle );
static RESULT TC358749XBG_IsiMdiSetupMotoDrive( IsiSensorHandle_t handle, uint32_t *pMaxStep );
static RESULT TC358749XBG_IsiMdiFocusSet( IsiSensorHandle_t handle, const uint32_t Position );
static RESULT TC358749XBG_IsiMdiFocusGet( IsiSensorHandle_t handle, uint32_t *pAbsStep );
static RESULT TC358749XBG_IsiMdiFocusCalibrate( IsiSensorHandle_t handle );

static RESULT TC358749XBG_IsiGetSensorMipiInfoIss( IsiSensorHandle_t handle, IsiSensorMipiInfo *ptIsiSensorMipiInfo);
static RESULT TC358749XBG_IsiGetSensorIsiVersion(  IsiSensorHandle_t   handle, unsigned int* pVersion);

static RESULT TC358749XBG_HdmiGetInputCheckRegIss(IsiSensorHandle_t   handle, uint32_t  *p_value);
static RESULT TC358749XBG_HdmiInputCheckRegReadIss( IsiSensorHandle_t   handle, const uint32_t address,uint32_t  *p_value);



/*****************************************************************************/
/**
 *          TC358749XBG_IsiCreateSensorIss
 *
 * @brief   This function creates a new TC358749XBG sensor instance handle.
 *
 * @param   pConfig     configuration structure to create the instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiCreateSensorIss
(
    IsiSensorInstanceConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TC358749XBG_Context_t *pTC358749XBGCtx;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( (pConfig == NULL) || (pConfig->pSensor ==NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    pTC358749XBGCtx = ( TC358749XBG_Context_t * )malloc ( sizeof (TC358749XBG_Context_t) );
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pTC358749XBGCtx, 0, sizeof( TC358749XBG_Context_t ) );

    result = HalAddRef( pConfig->HalHandle );
    if ( result != RET_SUCCESS )
    {
        free ( pTC358749XBGCtx );
        return ( result );
    }

    pTC358749XBGCtx->IsiCtx.HalHandle              = pConfig->HalHandle;
    pTC358749XBGCtx->IsiCtx.HalDevID               = pConfig->HalDevID;
    pTC358749XBGCtx->IsiCtx.I2cBusNum              = pConfig->I2cBusNum;
    pTC358749XBGCtx->IsiCtx.SlaveAddress           = ( pConfig->SlaveAddr == 0 ) ? TC358749XBG_SLAVE_ADDR : pConfig->SlaveAddr;
    pTC358749XBGCtx->IsiCtx.NrOfAddressBytes       = 2U;

    pTC358749XBGCtx->IsiCtx.I2cAfBusNum            = pConfig->I2cAfBusNum;
    pTC358749XBGCtx->IsiCtx.SlaveAfAddress         = ( pConfig->SlaveAfAddr == 0 ) ? TC358749XBG_SLAVE_AF_ADDR : pConfig->SlaveAfAddr;
    pTC358749XBGCtx->IsiCtx.NrOfAfAddressBytes     = 0;

    pTC358749XBGCtx->IsiCtx.pSensor                = pConfig->pSensor;

    pTC358749XBGCtx->Configured             = BOOL_FALSE;
    pTC358749XBGCtx->Streaming              = BOOL_FALSE;
    pTC358749XBGCtx->TestPattern            = BOOL_FALSE;
    pTC358749XBGCtx->isAfpsRun              = BOOL_FALSE;

    pTC358749XBGCtx->IsiSensorMipiInfo.sensorHalDevID = pTC358749XBGCtx->IsiCtx.HalDevID;
    if(pConfig->mipiLaneNum & g_suppoted_mipi_lanenum_type){
        pTC358749XBGCtx->IsiSensorMipiInfo.ucMipiLanes = pConfig->mipiLaneNum;
		TRACE( TC358749XBG_ERROR, "%s set lane numbers :%d\n", __FUNCTION__,pConfig->mipiLaneNum);
    }else{
        TRACE( TC358749XBG_ERROR, "%s don't support lane numbers :%d,set to default %d\n", __FUNCTION__,pConfig->mipiLaneNum,DEFAULT_NUM_LANES);
        pTC358749XBGCtx->IsiSensorMipiInfo.ucMipiLanes = DEFAULT_NUM_LANES;
    }

    pConfig->hSensor = ( IsiSensorHandle_t )pTC358749XBGCtx;

//    result = HalSetCamConfig( pTC358749XBGCtx->IsiCtx.HalHandle, pTC358749XBGCtx->IsiCtx.HalDevID, true, true, false );
//    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

//    result = HalSetClock( pTC358749XBGCtx->IsiCtx.HalHandle, pTC358749XBGCtx->IsiCtx.HalDevID, 10000000U);
//    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiReleaseSensorIss
 *
 * @brief   This function destroys/releases an TC358749XBG sensor instance.
 *
 * @param   handle      TC358749XBG sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiReleaseSensorIss
(
    IsiSensorHandle_t handle
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    (void)TC358749XBG_IsiSensorSetStreamingIss( pTC358749XBGCtx, BOOL_FALSE );
    (void)TC358749XBG_IsiSensorSetPowerIss( pTC358749XBGCtx, BOOL_FALSE );

    (void)HalDelRef( pTC358749XBGCtx->IsiCtx.HalHandle );

    MEMSET( pTC358749XBGCtx, 0, sizeof( TC358749XBG_Context_t ) );
    free ( pTC358749XBGCtx );

    TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCapsIss
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
static RESULT TC358749XBG_IsiGetCapsIssInternal
(
    IsiSensorCaps_t   *pIsiSensorCaps,
    uint32_t  mipi_lanes
)
{

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);


    if ( pIsiSensorCaps == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {

        if(mipi_lanes == SUPPORT_MIPI_FOUR_LANE){            
            switch (pIsiSensorCaps->Index) 
            {
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_TV1080P30;
                    break;
                }
                case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_TV720P30;
                    break;
                }
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }

            }
        }
        
        pIsiSensorCaps->BusWidth        = ISI_BUSWIDTH_8BIT_ZZ;
        pIsiSensorCaps->Mode            = ISI_MODE_BT601;
        pIsiSensorCaps->FieldSelection  = ISI_FIELDSEL_BOTH;
        pIsiSensorCaps->YCSequence      = ISI_YCSEQ_CBYCRY;           
        pIsiSensorCaps->Conv422         = ISI_CONV422_NOCOSITED;
        pIsiSensorCaps->BPat            = ISI_BPAT_RGRGGBGB ;
        pIsiSensorCaps->HPol            = ISI_HPOL_REFPOS; //hsync?
        pIsiSensorCaps->VPol            = ISI_VPOL_NEG; //VPolarity
        pIsiSensorCaps->Edge            = ISI_EDGE_RISING;
        pIsiSensorCaps->Bls             = ISI_BLS_OFF; //close;
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
        pIsiSensorCaps->MipiMode        = ISI_MIPI_MODE_YUV422_8;
        pIsiSensorCaps->AfpsResolutions = ( ISI_AFPS_NOTSUPP );
		pIsiSensorCaps->SensorOutputMode = ISI_SENSOR_OUTPUT_MODE_YUV;
    }
end:
    TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}


static RESULT TC358749XBG_IsiGetCapsIss
(
    IsiSensorHandle_t handle,
    IsiSensorCaps_t   *pIsiSensorCaps
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    result = TC358749XBG_IsiGetCapsIssInternal(pIsiSensorCaps, pTC358749XBGCtx->IsiSensorMipiInfo.ucMipiLanes);


    
	TRACE( TC358749XBG_ERROR, "%d ( pIsiSensorCaps->Index)\n", pIsiSensorCaps->Index);
	TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          TC358749XBG_g_IsiSensorDefaultConfig
 *
 * @brief   recommended default configuration for application use via call
 *          to IsiGetSensorIss()
 *
 *****************************************************************************/
const IsiSensorCaps_t TC358749XBG_g_IsiSensorDefaultConfig =
{
    ISI_BUSWIDTH_8BIT_ZZ,         // BusWidth
    ISI_MODE_BT601, //ISI_MODE_MIPI,              // MIPI
    ISI_FIELDSEL_BOTH,          // FieldSel
    ISI_YCSEQ_CBYCRY,           // YCSeq
    ISI_CONV422_NOCOSITED,      // Conv422
    ISI_BPAT_BGBGGRGR,//ISI_BPAT_BGBGGRGR,          // BPat
    ISI_HPOL_REFPOS,            // HPol
    ISI_VPOL_NEG,               // VPol
    ISI_EDGE_FALLING,            // Edge
    ISI_BLS_OFF,                // Bls
    ISI_GAMMA_OFF,              // Gamma
    ISI_CCONV_OFF,              // CConv
    ISI_RES_SVGAP30,          // Res
    ISI_DWNSZ_SUBSMPL,          // DwnSz
    ISI_BLC_OFF ,               // BLC
    ISI_AGC_OFF,                // AGC
    ISI_AWB_OFF,                // AWB
    ISI_AEC_OFF,                // AEC
    ISI_DPCC_OFF,               // DPCC
    0,            // CieProfile, this is also used as start profile for AWB (if not altered by menu settings)
    ISI_SMIA_OFF,               // SmiaMode
    ISI_MIPI_MODE_YUV422_8,       // MipiMode
    ISI_AFPS_NOTSUPP,           // AfpsResolutions
    ISI_SENSOR_OUTPUT_MODE_YUV,
    0,
};



/*****************************************************************************/
/**
 *          TC358749XBG_SetupOutputFormat
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      TC358749XBG sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT TC358749XBG_SetupOutputFormat
(
    TC358749XBG_Context_t       *pTC358749XBGCtx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;
    return result;
}

int TC358749XBG_get_PCLK( TC358749XBG_Context_t *pTC358749XBGCtx, int XVCLK)
{
    // calculate PCLK
    
    return 0;
}

/*****************************************************************************/
/**
 *          TC358749XBG_SetupOutputWindow
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      TC358749XBG sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_SetupOutputWindow
(
    TC358749XBG_Context_t        *pTC358749XBGCtx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result     = RET_SUCCESS;

	TRACE( TC358749XBG_ERROR, "%s (enter.....)\n", __FUNCTION__);
	pTC358749XBGCtx->IsiSensorMipiInfo.ulMipiFreq = 648;
#if 0    //kings
        /* resolution */
    switch ( pConfig->Resolution )
    {
        case ISI_RES_SVGAP30:
        {
            if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pTC358749XBGCtx,TC358749XBG_g_svga)) != RET_SUCCESS){
                TRACE( TC358749XBG_ERROR, "%s: failed to set  ISI_RES_SVGAP30 \n", __FUNCTION__ );
            }else{

                TRACE( TC358749XBG_ERROR, "%s: success to set  ISI_RES_SVGAP30 \n", __FUNCTION__ );
            }
            break;
        }
        case ISI_RES_1600_1200P7:
        {
            if((result = IsiRegDefaultsApply((IsiSensorHandle_t)pTC358749XBGCtx,TC358749XBG_g_1600x1200)) != RET_SUCCESS){
                TRACE( TC358749XBG_ERROR, "%s: failed to set  ISI_RES_1600_1200P7 \n", __FUNCTION__ );
            }else{

                TRACE( TC358749XBG_ERROR, "%s: success to set  ISI_RES_1600_1200P7  \n", __FUNCTION__ );
            }
            break;
        }
        default:
        {
            TRACE( TC358749XBG_ERROR, "%s: Resolution not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

#endif
    return ( result );
}




/*****************************************************************************/
/**
 *          TC358749XBG_SetupImageControl
 *
 * @brief   Sets the image control functions (BLC, AGC, AWB, AEC, DPCC ...)
 *
 * @param   handle      TC358749XBG sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT TC358749XBG_SetupImageControl
(
    TC358749XBG_Context_t        *pTC358749XBGCtx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0U;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);


    return ( result );
}


/*****************************************************************************/
/**
 *          TC358749XBG_AecSetModeParameters
 *
 * @brief   This function fills in the correct parameters in TC358749XBG-Instances
 *          according to AEC mode selection in IsiSensorConfig_t.
 *
 * @note    It is assumed that IsiSetupOutputWindow has been called before
 *          to fill in correct values in instance structure.
 *
 * @param   handle      TC358749XBG context
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_AecSetModeParameters
(
    TC358749XBG_Context_t       *pTC358749XBGCtx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    return ( result );
}
//static int int11;
/*****************************************************************************/
/**
 *          TC358749XBG_IsiSetupSensorIss
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      TC358749XBG sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiSetupSensorIss
(
    IsiSensorHandle_t       handle,
    const IsiSensorConfig_t *pConfig
)
{
    uint32_t data;
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);
	
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid configuration (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    if ( pTC358749XBGCtx->Streaming != BOOL_FALSE )
    {
        return RET_WRONG_STATE;
    }

    MEMCPY( &pTC358749XBGCtx->Config, pConfig, sizeof( IsiSensorConfig_t ) );

	    /* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */
	    result = IsiRegDefaultsApply( pTC358749XBGCtx, TC358749XBG_g_aRegDescription );
	    if ( result != RET_SUCCESS )
	    {
	        return ( result );
	    }

	    /* sleep a while, that sensor can take over new default values */
	    osSleep( 10 );
	    #if 1   //kings


	    /* 3.) verify default values to make sure everything has been written correctly as expected */
	    result = IsiRegDefaultsVerify( pTC358749XBGCtx, TC358749XBG_g_aRegDescription );
	    if ( result != RET_SUCCESS )
	    {
	        return ( result );
	    }
	    // output of pclk for measurement (only debugging)
	    //result = TC358749XBG_IsiRegWriteIss( pTC358749XBGCtx, 0x3009U, 0x10U );
	    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        //KINGS hdmi input timing check
        //TC358749XBG_HdmiGetInputCheckRegIss(pTC358749XBGCtx,&data);
		
	    #endif

	    /* 4.) setup output format (RAW10|RAW12) */
	    result = TC358749XBG_SetupOutputFormat( pTC358749XBGCtx, pConfig );
	    if ( result != RET_SUCCESS )
	    {
	        TRACE( TC358749XBG_ERROR, "%s: SetupOutputFormat failed.\n", __FUNCTION__);
	        return ( result );
	    }

	    /* 5.) setup output window */
	    result = TC358749XBG_SetupOutputWindow( pTC358749XBGCtx, pConfig );
	    if ( result != RET_SUCCESS )
	    {
	        TRACE( TC358749XBG_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
	        return ( result );
	    }

	    result = TC358749XBG_SetupImageControl( pTC358749XBGCtx, pConfig );
	    if ( result != RET_SUCCESS )
	    {
	        TRACE( TC358749XBG_ERROR, "%s: SetupImageControl failed.\n", __FUNCTION__);
	        return ( result );
	    }

	    result = TC358749XBG_AecSetModeParameters( pTC358749XBGCtx, pConfig );
	    if ( result != RET_SUCCESS )
	    {
	        TRACE( TC358749XBG_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
	        return ( result );
	    }

    if (result == RET_SUCCESS)
    {
        pTC358749XBGCtx->Configured = BOOL_TRUE;
    }

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiChangeSensorResolutionIss
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
static RESULT TC358749XBG_IsiChangeSensorResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    uint8_t             *pNumberOfFramesToSkip
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (pNumberOfFramesToSkip == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pTC358749XBGCtx->Configured != BOOL_TRUE) || (pTC358749XBGCtx->Streaming != BOOL_FALSE) )
    {
        return RET_WRONG_STATE;
    }

    IsiSensorCaps_t Caps;    
    Caps.Index = 0;
    Caps.Resolution = 0;
    while (TC358749XBG_IsiGetCapsIss( handle, &Caps) == RET_SUCCESS) {
        if (Resolution == Caps.Resolution) {            
            break;
        }
        Caps.Index++;
    }

    if ( (Resolution & Caps.Resolution) == 0 )
    {
        return RET_OUTOFRANGE;
    }

    if ( Resolution == pTC358749XBGCtx->Config.Resolution )
    {
        // well, no need to worry
        *pNumberOfFramesToSkip = 0;
    }
    else
    {
        // change resolution
        char *szResName = NULL;
        result = IsiGetResolutionName( Resolution, &szResName );
        TRACE( TC358749XBG_DEBUG, "%s: NewRes=0x%08x (%s)\n", __FUNCTION__, Resolution, szResName);

        // update resolution in copy of config in context
        pTC358749XBGCtx->Config.Resolution = Resolution;

        // tell sensor about that
        result = TC358749XBG_SetupOutputWindow( pTC358749XBGCtx, &pTC358749XBGCtx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( TC358749XBG_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
            return ( result );
        }

        // remember old exposure values
        float OldGain = pTC358749XBGCtx->AecCurGain;
        float OldIntegrationTime = pTC358749XBGCtx->AecCurIntegrationTime;

        // update limits & stuff (reset current & old settings)
        result = TC358749XBG_AecSetModeParameters( pTC358749XBGCtx, &pTC358749XBGCtx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( TC358749XBG_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
            return ( result );
        }

        // restore old exposure values (at least within new exposure values' limits)
        uint8_t NumberOfFramesToSkip = 2;
        float   DummySetGain;
        float   DummySetIntegrationTime;
        result = TC358749XBG_IsiExposureControlIss( handle, OldGain, OldIntegrationTime, &NumberOfFramesToSkip, &DummySetGain, &DummySetIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            TRACE( TC358749XBG_ERROR, "%s: TC358749XBG_IsiExposureControlIss failed.\n", __FUNCTION__);
            return ( result );
        }

        // return number of frames that aren't exposed correctly
        *pNumberOfFramesToSkip = NumberOfFramesToSkip + 1;
    }

    TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiSensorSetStreamingIss
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
static RESULT TC358749XBG_IsiSensorSetStreamingIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    uint32_t RegValue = 0;

    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pTC358749XBGCtx->Configured != BOOL_TRUE) || (pTC358749XBGCtx->Streaming == on) )
    {
        return RET_WRONG_STATE;
    }

    if (on == BOOL_TRUE)
    {
        /* enable streaming */
		//{0x854A	,0x00,"0x0100",eReadWrite},//01
		//{0x0004 ,0xD70C,"0x0100",eReadWrite_16},  //ConfCtl 0CD7
		result = TC358749XBG_IsiRegVedioReadIss ( handle, 0x854A, &RegValue);
		TRACE( TC358749XBG_ERROR, "0x854A = 0x%8x(BOOL_TRUE)\n", RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
		result = TC358749XBG_IsiRegVedioReadIss ( handle, 0x854A, &RegValue);
		TRACE( TC358749XBG_ERROR, "0x854A = 0x%8x(BOOL_TRUE)\n", RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
		result = TC358749XBG_IsiRegVedioReadIss ( handle, 0x0004, &RegValue);
		TRACE( TC358749XBG_ERROR, "0x0004 = 0x%8x(BOOL_TRUE)\n", RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
		
		//result = TC358749XBG_IsiRegVedioWriteIss( handle, 0x854A, 0x01);
		//result = TC358749XBG_IsiRegVedioWriteIss( handle, 0x854A, 0x01);
		//osSleep( 1 );
        //result = TC358749XBG_IsiRegVedioWriteIss( handle, 0x0004, 0xD70C);
        TRACE( TC358749XBG_ERROR, "%s (BOOL_TRUE)\n", __FUNCTION__);
		result = RET_SUCCESS;

    }
    else
    {
        /* disable streaming */
		//result = TC358749XBG_IsiRegVedioWriteIss( handle, 0x854A, 0x00);
		//osSleep( 1 );
        //result = TC358749XBG_IsiRegVedioWriteIss( handle, 0x0004, 0xD60C);
        //result = TC358749XBG_IsiRegWriteIss( handle, 0x4200, 0x0f);
   /*
		result = TC358749XBG_IsiRegReadIss ( handle, OV8858_MODE_SELECT, &RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = TC358749XBG_IsiRegWriteIss ( handle, OV8858_MODE_SELECT, (RegValue & ~OV8858_MODE_SELECT_ON) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        */
        TRACE( TC358749XBG_ERROR, "%s (BOOL_FLUSE)\n", __FUNCTION__);
		result = RET_SUCCESS;

    }

    if (result == RET_SUCCESS)
    {
        pTC358749XBGCtx->Streaming = on;
    }

    TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiSensorSetPowerIss
 *
 * @brief   Performs the power-up/power-down sequence of the camera, if possible.
 *
 * @param   handle      TC358749XBG sensor instance handle
 * @param   on          new power state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiSensorSetPowerIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pTC358749XBGCtx->Configured = BOOL_FALSE;
    pTC358749XBGCtx->Streaming  = BOOL_FALSE;
#if 1
    TRACE( TC358749XBG_DEBUG, "%s power off \n", __FUNCTION__);
    result = HalSetPower( pTC358749XBGCtx->IsiCtx.HalHandle, pTC358749XBGCtx->IsiCtx.HalDevID, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( TC358749XBG_DEBUG, "%s reset on\n", __FUNCTION__);
    result = HalSetReset( pTC358749XBGCtx->IsiCtx.HalHandle, pTC358749XBGCtx->IsiCtx.HalDevID, true );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    if (on == BOOL_TRUE)
    {
        TRACE( TC358749XBG_DEBUG, "%s power on \n", __FUNCTION__);
        result = HalSetPower( pTC358749XBGCtx->IsiCtx.HalHandle, pTC358749XBGCtx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( TC358749XBG_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pTC358749XBGCtx->IsiCtx.HalHandle, pTC358749XBGCtx->IsiCtx.HalDevID, false );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( TC358749XBG_DEBUG, "%s reset on \n", __FUNCTION__);
        result = HalSetReset( pTC358749XBGCtx->IsiCtx.HalHandle, pTC358749XBGCtx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( TC358749XBG_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pTC358749XBGCtx->IsiCtx.HalHandle, pTC358749XBGCtx->IsiCtx.HalDevID, false );

        osSleep( 50 );
    }
#endif
    TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiCheckSensorConnectionIss
 *
 * @brief   Checks the I2C-Connection to sensor by reading sensor revision id.
 *
 * @param   handle      TC358749XBG sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiCheckSensorConnectionIss
(
    IsiSensorHandle_t   handle
)
{
    uint32_t RevId;
    uint32_t value;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }
	result = TC358749XBG_IsiGetSensorRevisionIss( handle, &value );
#if 0
    RevId = TC358749XBG_CHIP_ID_HIGH_BYTE_DEFAULT;
    //RevId = (RevId << 16U) | (TC358749XBG_CHIP_ID_MIDDLE_BYTE_DEFAULT<<8U);
    //RevId = RevId | TC358749XBG_CHIP_ID_LOW_BYTE_DEFAULT;
	   
    if ( (result != RET_SUCCESS) || (RevId != value) )
    {
        TRACE( TC358749XBG_ERROR, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );
        return ( RET_FAILURE );
    }

    TRACE( TC358749XBG_DEBUG, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );
#endif
    TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetSensorRevisionIss
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
static RESULT TC358749XBG_IsiGetSensorRevisionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    uint32_t data;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);
#if 0
    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *p_value = 0U;
    result = TC358749XBG_IsiRegReadIss ( handle, TC358749XBG_CHIP_ID_HIGH_BYTE, &data );
	/*
    *p_value = ( (data & 0xFF) << 16U );
    result = TC358749XBG_IsiRegReadIss ( handle, TC358749XBG_CHIP_ID_MIDDLE_BYTE, &data );
    *p_value |= ( (data & 0xFF) << 8U );
    result = TC358749XBG_IsiRegReadIss ( handle, TC358749XBG_CHIP_ID_LOW_BYTE, &data );
    *p_value |= ( (data & 0xFF));
    */
    *p_value = data;

    TRACE( TC358749XBG_ERROR, "%s (exit),*p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);
#endif
    //KINGS hdmi input timing check
    TC358749XBG_HdmiGetInputCheckRegIss(handle,&data);
	
    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiRegReadIss
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
static RESULT TC358749XBG_IsiRegReadIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

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
        uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, TC358749XBG_g_aRegDescription );
		TRACE( TC358749XBG_ERROR, "%s (exit: NrOfBytes=%d)\n", __FUNCTION__, NrOfBytes);
        if ( !NrOfBytes )
        {
            NrOfBytes = 1;
        }
        TRACE( TC358749XBG_ERROR, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);

        *p_value = 0;
        result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
    }

    TRACE( TC358749XBG_ERROR, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);

    return ( result );
}

static RESULT TC358749XBG_IsiRegVedioReadIss
	(
		IsiSensorHandle_t	handle,
		const uint32_t		address,
		uint32_t			*p_value
	)
	{
		RESULT result = RET_SUCCESS;
	
		TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);
	
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
			uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, TC358749XBG_g_aRegVedioON );
			TRACE( TC358749XBG_ERROR, "%s (exit: NrOfBytes=%d)\n", __FUNCTION__, NrOfBytes);
			if ( !NrOfBytes )
			{
				NrOfBytes = 1;
			}
			TRACE( TC358749XBG_ERROR, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);
	
			*p_value = 0;
			result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
		}
	
		TRACE( TC358749XBG_ERROR, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);
	
		return ( result );
	}


/*****************************************************************************/
/**
 *          TC358749XBG_HdmiGetInputCheckRegIss
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
static RESULT TC358749XBG_HdmiGetInputCheckRegIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    uint32_t data;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }

	//----------------------------------------------------------------------
	/*
	*p_value = 0U;
		result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8C09, &data );
		*p_value = data;
		TRACE( TC358749XBG_ERROR, "%s (exit),0x8C09: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);
    
	*p_value = 0U;
			result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8C0A, &data );
			*p_value = data;
			TRACE( TC358749XBG_ERROR, "%s (exit),0x8C0A: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);
	*/
	//----------------------------------------------------------------------
	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8520, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8520: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);
	
    *p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x852E, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x852E: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x852F, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x852F: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x858A, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x858A: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x858B, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x858B: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8580, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8580: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8581, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8581: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8582, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8582: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8583, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8583: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x858C, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x858C: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x858D, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x858D: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8584, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8584: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8585, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8585: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8586, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8586: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8587, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8587: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8588, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8588: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8589, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8589: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

	*p_value = 0U;
    result = TC358749XBG_HdmiInputCheckRegReadIss ( handle, 0x8526, &data );
    *p_value = data;
    TRACE( TC358749XBG_ERROR, "%s (exit),0x8526: *p_value=0x%08x,data=0x%08x\n", __FUNCTION__,*p_value,data);

    return ( result );
}


/*****************************************************************************/
/**
 *          TC358749XBG_HdmiInputCheckRegReadIss
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
static RESULT TC358749XBG_HdmiInputCheckRegReadIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

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
        uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, TC358749XBG_g_hdmi_input_check );
		TRACE( TC358749XBG_ERROR, "%s (exit: NrOfBytes=%d)\n", __FUNCTION__, NrOfBytes);
        if ( !NrOfBytes )
        {
            NrOfBytes = 1;
        }
        TRACE( TC358749XBG_ERROR, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);

        *p_value = 0;
        result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
    }

    TRACE( TC358749XBG_ERROR, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);

    return ( result );
}


/*****************************************************************************/
/**
 *          TC358749XBG_IsiRegWriteIss
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
static RESULT TC358749XBG_IsiRegWriteIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NrOfBytes;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    NrOfBytes = IsiGetNrDatBytesIss( address, TC358749XBG_g_aRegDescription );
    if ( !NrOfBytes )
    {
        NrOfBytes = 1;
    }
    TRACE( TC358749XBG_ERROR, "%s (IsiGetNrDatBytesIss %d 0x%08x 0x%08x)\n", __FUNCTION__, NrOfBytes, address, value);

    result = IsiI2cWriteSensorRegister( handle, address, (uint8_t *)(&value), NrOfBytes, BOOL_TRUE );

    TRACE( TC358749XBG_ERROR, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, value);

    return ( result );
}

static RESULT TC358749XBG_IsiRegVedioWriteIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NrOfBytes;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    NrOfBytes = IsiGetNrDatBytesIss( address, TC358749XBG_g_aRegVedioON );
    if ( !NrOfBytes )
    {
        NrOfBytes = 1;
    }
    TRACE( TC358749XBG_ERROR, "%s (IsiGetNrDatBytesIss %d 0x%08x 0x%08x)\n", __FUNCTION__, NrOfBytes, address, value);

    result = IsiI2cWriteSensorRegister( handle, address, (uint8_t *)(&value), NrOfBytes, BOOL_TRUE );

    TRACE( TC358749XBG_ERROR, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, value);

    return ( result );
}


/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetGainLimitsIss
 *
 * @brief   Returns the exposure minimal and maximal values of an
 *          TC358749XBG instance
 *
 * @param   handle       TC358749XBG sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetGainLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinGain,
    float               *pMaxGain
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0

    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinGain == NULL) || (pMaxGain == NULL) )
    {
        TRACE( TC358749XBG_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinGain = pTC358749XBGCtx->AecMinGain;
    *pMaxGain = pTC358749XBGCtx->AecMaxGain;
#endif
    *pMinGain = 0;
    *pMaxGain = 0;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetIntegrationTimeLimitsIss
 *
 * @brief   Returns the minimal and maximal integration time values of an
 *          TC358749XBG instance
 *
 * @param   handle       TC358749XBG sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetIntegrationTimeLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinIntegrationTime,
    float               *pMaxIntegrationTime
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL) )
    {
        TRACE( TC358749XBG_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinIntegrationTime = pTC358749XBGCtx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pTC358749XBGCtx->AecMaxIntegrationTime;
#endif
    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetGainIss
 *
 * @brief   Reads gain values from the image sensor module.
 *
 * @param   handle                  TC358749XBG sensor instance handle
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT TC358749XBG_IsiGetGainIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain = pTC358749XBGCtx->AecCurGain;
#endif
	*pSetGain = 0;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetGainIncrementIss
 *
 * @brief   Get smallest possible gain increment.
 *
 * @param   handle                  TC358749XBG sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT TC358749XBG_IsiGetGainIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pTC358749XBGCtx->AecGainIncrement;
#endif
	*pIncr = 0;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiSetGainIss
 *
 * @brief   Writes gain values to the image sensor module.
 *          Updates current gain and exposure in sensor struct/state.
 *
 * @param   handle                  TC358749XBG sensor instance handle
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
RESULT TC358749XBG_IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint16_t usGain = 0;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

  
    if( NewGain < pTC358749XBGCtx->AecMinGain ) NewGain = pTC358749XBGCtx->AecMinGain;
    if( NewGain > pTC358749XBGCtx->AecMaxGain ) NewGain = pTC358749XBGCtx->AecMaxGain;

    usGain = (uint16_t)NewGain;

    // write new gain into sensor registers, do not write if nothing has changed
    if( (usGain != pTC358749XBGCtx->OldGain) )
    {

        pTC358749XBGCtx->OldGain = usGain;
    }

    //calculate gain actually set
    pTC358749XBGCtx->AecCurGain = NewGain;

    //return current state
    *pSetGain = pTC358749XBGCtx->AecCurGain;
    TRACE( TC358749XBG_DEBUG, "%s: g=%f\n", __FUNCTION__, *pSetGain );
#endif
    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetIntegrationTimeIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  TC358749XBG sensor instance handle
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT TC358749XBG_IsiGetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               *pSetIntegrationTime
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetIntegrationTime == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetIntegrationTime = pTC358749XBGCtx->AecCurIntegrationTime;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);
#endif
    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetIntegrationTimeIncrementIss
 *
 * @brief   Get smallest possible integration time increment.
 *
 * @param   handle                  TC358749XBG sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT TC358749XBG_IsiGetIntegrationTimeIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pTC358749XBGCtx->AecIntegrationTimeIncrement;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);
#endif
    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiSetIntegrationTimeIss
 *
 * @brief   Writes gain and integration time values to the image sensor module.
 *          Updates current integration time and exposure in sensor
 *          struct/state.
 *
 * @param   handle                  TC358749XBG sensor instance handle
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
RESULT TC358749XBG_IsiSetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               NewIntegrationTime,
    float               *pSetIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( TC358749XBG_DEBUG, "%s: Ti=%f\n", __FUNCTION__, *pSetIntegrationTime );
    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}




/*****************************************************************************/
/**
 *          TC358749XBG_IsiExposureControlIss
 *
 * @brief   Camera hardware dependent part of the exposure control loop.
 *          Calculates appropriate register settings from the new exposure
 *          values and writes them to the image sensor module.
 *
 * @param   handle                  TC358749XBG sensor instance handle
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
RESULT TC358749XBG_IsiExposureControlIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               NewIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;
#if 0
    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pNumberOfFramesToSkip == NULL)
            || (pSetGain == NULL)
            || (pSetIntegrationTime == NULL) )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    TRACE( TC358749XBG_DEBUG, "%s: g=%f, Ti=%f\n", __FUNCTION__, NewGain, NewIntegrationTime );


    result = TC358749XBG_IsiSetIntegrationTimeIss( handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip );
    result = TC358749XBG_IsiSetGainIss( handle, NewGain, pSetGain );

	//*pNumberOfFramesToSkip = 2;

    TRACE( TC358749XBG_DEBUG, "%s: set: g=%f, Ti=%f, skip=%d\n", __FUNCTION__, *pSetGain, *pSetIntegrationTime, *pNumberOfFramesToSkip );
    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);
#endif
    TRACE( TC358749XBG_ERROR, ">>>>>>>>>>>>>>>>>>>>>>>>>%s: (exit)\n", __FUNCTION__);
    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCurrentExposureIss
 *
 * @brief   Returns the currently adjusted AE values
 *
 * @param   handle                  TC358749XBG sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT TC358749XBG_IsiGetCurrentExposureIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetGain == NULL) || (pSetIntegrationTime == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain            = pTC358749XBGCtx->AecCurGain;
    *pSetIntegrationTime = pTC358749XBGCtx->AecCurIntegrationTime;
#endif
    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetResolutionIss
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
RESULT TC358749XBG_IsiGetResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *pSetResolution
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetResolution == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetResolution = pTC358749XBGCtx->Config.Resolution;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetAfpsInfoHelperIss
 *
 * @brief   Calc AFPS sub resolution settings for the given resolution
 *
 * @param   pTC358749XBGCtx             TC358749XBG sensor instance (dummy!) context
 * @param   Resolution              Any supported resolution to query AFPS params for
 * @param   pAfpsInfo               Reference of AFPS info structure to write the results to
 * @param   AfpsStageIdx            Index of current AFPS stage to use
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetAfpsInfoHelperIss(
    TC358749XBG_Context_t   *pTC358749XBGCtx,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo,
    uint32_t            AfpsStageIdx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    DCT_ASSERT(pTC358749XBGCtx != NULL);
    DCT_ASSERT(pAfpsInfo != NULL);
    DCT_ASSERT(AfpsStageIdx <= ISI_NUM_AFPS_STAGES);

    // update resolution in copy of config in context
    pTC358749XBGCtx->Config.Resolution = Resolution;

    // tell sensor about that
    result = TC358749XBG_SetupOutputWindow( pTC358749XBGCtx, &pTC358749XBGCtx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( TC358749XBG_ERROR, "%s: SetupOutputWindow failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // update limits & stuff (reset current & old settings)
    result = TC358749XBG_AecSetModeParameters( pTC358749XBGCtx, &pTC358749XBGCtx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( TC358749XBG_ERROR, "%s: AecSetModeParameters failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // take over params
    pAfpsInfo->Stage[AfpsStageIdx].Resolution = Resolution;
    pAfpsInfo->Stage[AfpsStageIdx].MaxIntTime = pTC358749XBGCtx->AecMaxIntegrationTime;
    pAfpsInfo->AecMinGain           = pTC358749XBGCtx->AecMinGain;
    pAfpsInfo->AecMaxGain           = pTC358749XBGCtx->AecMaxGain;
    pAfpsInfo->AecMinIntTime        = pTC358749XBGCtx->AecMinIntegrationTime;
    pAfpsInfo->AecMaxIntTime        = pTC358749XBGCtx->AecMaxIntegrationTime;
    pAfpsInfo->AecSlowestResolution = Resolution;
	
#endif
    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetAfpsInfoIss
 *
 * @brief   Returns the possible AFPS sub resolution settings for the given resolution series
 *
 * @param   handle                  TC358749XBG sensor instance handle
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
RESULT TC358749XBG_IsiGetAfpsInfoIss(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);
#if 0
    if ( pTC358749XBGCtx == NULL )
    {
        TRACE( TC358749XBG_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pAfpsInfo == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    // use currently configured resolution?
    if (Resolution == 0)
    {
        Resolution = pTC358749XBGCtx->Config.Resolution;
    }
#endif
    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCalibKFactor
 *
 * @brief   Returns the TC358749XBG specific K-Factor
 *
 * @param   handle       TC358749XBG sensor instance handle
 * @param   pIsiKFactor  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetCalibKFactor
(
    IsiSensorHandle_t   handle,
    Isi1x1FloatMatrix_t **pIsiKFactor
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiKFactor == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiKFactor = NULL;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCalibPcaMatrix
 *
 * @brief   Returns the TC358749XBG specific PCA-Matrix
 *
 * @param   handle          TC358749XBG sensor instance handle
 * @param   pIsiPcaMatrix   Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetCalibPcaMatrix
(
    IsiSensorHandle_t   handle,
    Isi3x2FloatMatrix_t **pIsiPcaMatrix
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiPcaMatrix == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiPcaMatrix = NULL;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns the sensor specific SvdMean-Vector
 *
 * @param   handle              TC358749XBG sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetCalibSvdMeanValue
(
    IsiSensorHandle_t   handle,
    Isi3x1FloatMatrix_t **pIsiSvdMeanValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSvdMeanValue == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiSvdMeanValue = NULL;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns a pointer to the sensor specific centerline, a straight
 *          line in Hesse normal form in Rg/Bg colorspace
 *
 * @param   handle              TC358749XBG sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetCalibCenterLine
(
    IsiSensorHandle_t   handle,
    IsiLine_t           **ptIsiCenterLine
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiCenterLine == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiCenterLine = NULL;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCalibClipParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for Rg/Bg color
 *          space clipping
 *
 * @param   handle              TC358749XBG sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetCalibClipParam
(
    IsiSensorHandle_t   handle,
    IsiAwbClipParm_t    **pIsiClipParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiClipParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsiClipParam = NULL;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCalibGlobalFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle              TC358749XBG sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetCalibGlobalFadeParam
(
    IsiSensorHandle_t       handle,
    IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiGlobalFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiGlobalFadeParam = NULL;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetCalibFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for near white
 *          pixel parameter calculations
 *
 * @param   handle              TC358749XBG sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiGetCalibFadeParam
(
    IsiSensorHandle_t   handle,
    IsiAwbFade2Parm_t   **ptIsiFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *ptIsiFadeParam = NULL;

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetIlluProfile
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
static RESULT TC358749XBG_IsiGetIlluProfile
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiIlluProfile_t    **ptIsiIlluProfile
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiIlluProfile == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetLscMatrixTable
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
static RESULT TC358749XBG_IsiGetLscMatrixTable
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiLscMatrixTable_t **pLscMatrixTable
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pLscMatrixTable == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          TC358749XBG_IsiMdiInitMotoDriveMds
 *
 * @brief   General initialisation tasks like I/O initialisation.
 *
 * @param   handle              TC358749XBG sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiMdiInitMotoDriveMds
(
    IsiSensorHandle_t   handle
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiMdiSetupMotoDrive
 *
 * @brief   Setup of the MotoDrive and return possible max step.
 *
 * @param   handle          TC358749XBG sensor instance handle
 *          pMaxStep        pointer to variable to receive the maximum
 *                          possible focus step
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiMdiSetupMotoDrive
(
    IsiSensorHandle_t   handle,
    uint32_t            *pMaxStep
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMaxStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }


    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiMdiFocusSet
 *
 * @brief   Drives the lens system to a certain focus point.
 *
 * @param   handle          TC358749XBG sensor instance handle
 *          AbsStep         absolute focus point to apply
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiMdiFocusSet
(
    IsiSensorHandle_t   handle,
    const uint32_t      Position
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t nPosition;
    uint8_t  data[2] = { 0, 0 };

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiMdiFocusGet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          TC358749XBG sensor instance handle
 *          pAbsStep        pointer to a variable to receive the current
 *                          focus point
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiMdiFocusGet
(
    IsiSensorHandle_t   handle,
    uint32_t            *pAbsStep
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;
    uint8_t  data[2] = { 0, 0 };

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pAbsStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiMdiFocusCalibrate
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          TC358749XBG sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT TC358749XBG_IsiMdiFocusCalibrate
(
    IsiSensorHandle_t   handle
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiActivateTestPattern
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          TC358749XBG sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT TC358749XBG_IsiActivateTestPattern
(
    IsiSensorHandle_t   handle,
    const bool_t        enable
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    return ( result );
}



/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetSensorMipiInfoIss
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          TC358749XBG sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 ******************************************************************************/
static RESULT TC358749XBG_IsiGetSensorMipiInfoIss
(
    IsiSensorHandle_t   handle,
    IsiSensorMipiInfo   *ptIsiSensorMipiInfo
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }


    if ( ptIsiSensorMipiInfo == NULL )
    {
        return ( result );
    }
	
    ptIsiSensorMipiInfo->ucMipiLanes = pTC358749XBGCtx->IsiSensorMipiInfo.ucMipiLanes;
    ptIsiSensorMipiInfo->ulMipiFreq= pTC358749XBGCtx->IsiSensorMipiInfo.ulMipiFreq;
    ptIsiSensorMipiInfo->sensorHalDevID = pTC358749XBGCtx->IsiSensorMipiInfo.sensorHalDevID;

    TRACE( TC358749XBG_ERROR, "ucMipiLanes=%d,ulMipiFreq=%d,sensorHalDevID=%d\n",  ptIsiSensorMipiInfo->ucMipiLanes,ptIsiSensorMipiInfo->ulMipiFreq,ptIsiSensorMipiInfo->sensorHalDevID );
    TRACE( TC358749XBG_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT TC358749XBG_IsiGetSensorIsiVersion
(  IsiSensorHandle_t   handle,
   unsigned int*     pVersion
)
{
    TC358749XBG_Context_t *pTC358749XBGCtx = (TC358749XBG_Context_t *)handle;
	uint8_t p_value;
	int ret;

    RESULT result = RET_SUCCESS;


    TRACE( TC358749XBG_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pTC358749XBGCtx == NULL )
    {
    	TRACE( TC358749XBG_ERROR, "%s: pTC358749XBGCtx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pVersion == NULL)
	{
		TRACE( TC358749XBG_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pVersion = CONFIG_ISI_VERSION;
	/*need add szy*/
	/*uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, TC358749XBG_g_hdmi_input_check );
	TRACE( TC358749XBG_ERROR, "%s (exit: NrOfBytes=%d)\n", __FUNCTION__, NrOfBytes);
	if ( !NrOfBytes )
	{
		NrOfBytes = 1;
	}
	TRACE( TC358749XBG_ERROR, "%s (IsiGetNrDatBytesIss %d 0x%08x)\n", __FUNCTION__, NrOfBytes, address);*/
	ret = IsiI2cReadSensorRegister( handle, 0x8521, &p_value, 1, BOOL_TRUE );
	TRACE( TC358749XBG_ERROR, "%s (exit: 0x8521 0x%08x)\n", __FUNCTION__, p_value);
	p_value = p_value & (0x0f); 
	if(p_value == 0xf) {
		property_set("sys.hdmiin.resolution", "1");
		TRACE( TC358749XBG_ERROR, "%s (exit: 1080P)\n", __FUNCTION__);
	} else if(p_value == 0xc) {
		property_set("sys.hdmiin.resolution", "2");
		TRACE( TC358749XBG_ERROR, "%s (exit: 720P)\n", __FUNCTION__);
	}
	return result;
}


/*****************************************************************************/
/**
 *          TC358749XBG_IsiGetSensorIss
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
RESULT TC358749XBG_IsiGetSensorIss
(
    IsiSensor_t *pIsiSensor
)
{
    RESULT result = RET_SUCCESS;

    TRACE( TC358749XBG_ERROR, "%s (enter)\n", __FUNCTION__);

    if ( pIsiSensor != NULL )
    {
        pIsiSensor->pszName                             = TC358749XBG_g_acName;
        pIsiSensor->pRegisterTable                      = TC358749XBG_g_aRegDescription;
        pIsiSensor->pIsiSensorCaps                      = &TC358749XBG_g_IsiSensorDefaultConfig;
		pIsiSensor->pIsiGetSensorIsiVer					= TC358749XBG_IsiGetSensorIsiVersion;

        pIsiSensor->pIsiCreateSensorIss                 = TC358749XBG_IsiCreateSensorIss;
        pIsiSensor->pIsiReleaseSensorIss                = TC358749XBG_IsiReleaseSensorIss;
        pIsiSensor->pIsiGetCapsIss                      = TC358749XBG_IsiGetCapsIss;
        pIsiSensor->pIsiSetupSensorIss                  = TC358749XBG_IsiSetupSensorIss;
        pIsiSensor->pIsiChangeSensorResolutionIss       = TC358749XBG_IsiChangeSensorResolutionIss;
        pIsiSensor->pIsiSensorSetStreamingIss           = TC358749XBG_IsiSensorSetStreamingIss;
        pIsiSensor->pIsiSensorSetPowerIss               = TC358749XBG_IsiSensorSetPowerIss;
        pIsiSensor->pIsiCheckSensorConnectionIss        = TC358749XBG_IsiCheckSensorConnectionIss;
        pIsiSensor->pIsiGetSensorRevisionIss            = TC358749XBG_IsiGetSensorRevisionIss;
        pIsiSensor->pIsiRegisterReadIss                 = TC358749XBG_IsiRegReadIss;
        pIsiSensor->pIsiRegisterWriteIss                = TC358749XBG_IsiRegWriteIss;
        pIsiSensor->pIsiGetResolutionIss                = TC358749XBG_IsiGetResolutionIss;

        /* AEC functions */
        pIsiSensor->pIsiExposureControlIss              = TC358749XBG_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = TC358749XBG_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = TC358749XBG_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetCurrentExposureIss           = TC358749XBG_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetGainIss                      = TC358749XBG_IsiGetGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = TC358749XBG_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = TC358749XBG_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = TC358749XBG_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = TC358749XBG_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = TC358749XBG_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetAfpsInfoIss                  = TC358749XBG_IsiGetAfpsInfoIss;

        /* AWB specific functions */
        pIsiSensor->pIsiGetCalibKFactor                 = TC358749XBG_IsiGetCalibKFactor;
        pIsiSensor->pIsiGetCalibPcaMatrix               = TC358749XBG_IsiGetCalibPcaMatrix;
        pIsiSensor->pIsiGetCalibSvdMeanValue            = TC358749XBG_IsiGetCalibSvdMeanValue;
        pIsiSensor->pIsiGetCalibCenterLine              = TC358749XBG_IsiGetCalibCenterLine;
        pIsiSensor->pIsiGetCalibClipParam               = TC358749XBG_IsiGetCalibClipParam;
        pIsiSensor->pIsiGetCalibGlobalFadeParam         = TC358749XBG_IsiGetCalibGlobalFadeParam;
        pIsiSensor->pIsiGetCalibFadeParam               = TC358749XBG_IsiGetCalibFadeParam;
        pIsiSensor->pIsiGetIlluProfile                  = TC358749XBG_IsiGetIlluProfile;
        pIsiSensor->pIsiGetLscMatrixTable               = TC358749XBG_IsiGetLscMatrixTable;

        /* AF functions */
        pIsiSensor->pIsiMdiInitMotoDriveMds             = TC358749XBG_IsiMdiInitMotoDriveMds;
        pIsiSensor->pIsiMdiSetupMotoDrive               = TC358749XBG_IsiMdiSetupMotoDrive;
        pIsiSensor->pIsiMdiFocusSet                     = TC358749XBG_IsiMdiFocusSet;
        pIsiSensor->pIsiMdiFocusGet                     = TC358749XBG_IsiMdiFocusGet;
        pIsiSensor->pIsiMdiFocusCalibrate               = TC358749XBG_IsiMdiFocusCalibrate;

        /* MIPI */
        pIsiSensor->pIsiGetSensorMipiInfoIss            = TC358749XBG_IsiGetSensorMipiInfoIss;

        /* Testpattern */
        pIsiSensor->pIsiActivateTestPattern             = TC358749XBG_IsiActivateTestPattern;
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( TC358749XBG_ERROR, "%s (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT TC358749XBG_IsiGetSensorI2cInfo(sensor_i2c_info_t** pdata)
{
    sensor_i2c_info_t* pSensorI2cInfo;

    TRACE( TC358749XBG_ERROR, "%s (Enter)\n", __FUNCTION__);
    pSensorI2cInfo = ( sensor_i2c_info_t * )malloc ( sizeof (sensor_i2c_info_t) );

    if ( pSensorI2cInfo == NULL )
    {
        TRACE( TC358749XBG_ERROR,  "%s: Can't allocate ov14825 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pSensorI2cInfo, 0, sizeof( sensor_i2c_info_t ) );

    
    pSensorI2cInfo->i2c_addr = TC358749XBG_SLAVE_ADDR;
    pSensorI2cInfo->i2c_addr2 = TC358749XBG_SLAVE_ADDR2;
    pSensorI2cInfo->soft_reg_addr = TC358749XBG_SOFTWARE_RST;
    //pSensorI2cInfo->soft_reg_addr = 0xFFFF;       //目的是什么也不修改
    pSensorI2cInfo->soft_reg_value = 0x807F; //Assert reset,Exit Sleep,wait!
    pSensorI2cInfo->reg_size = 2;
    pSensorI2cInfo->value_size = 2;

    {
        IsiSensorCaps_t Caps;
        sensor_caps_t *pCaps;
        uint32_t lanes,i;        

        for (i=0; i<3; i++) {
            lanes = (1<<i);
            ListInit(&pSensorI2cInfo->lane_res[i]);
            if (g_suppoted_mipi_lanenum_type & lanes) {
                Caps.Index = 0;            
                while(TC358749XBG_IsiGetCapsIssInternal(&Caps,lanes)==RET_SUCCESS) {
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
    pChipIDInfo_H->chipid_reg_addr = TC358749XBG_CHIP_ID_HIGH_BYTE;  
    pChipIDInfo_H->chipid_reg_value = TC358749XBG_CHIP_ID_HIGH_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_H );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_H );

/* 

    sensor_chipid_info_t* pChipIDInfo_M = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_M )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_M, 0, sizeof(*pChipIDInfo_M) ); 
    pChipIDInfo_M->chipid_reg_addr = TC358749XBG_CHIP_ID_MIDDLE_BYTE;
    pChipIDInfo_M->chipid_reg_value = TC358749XBG_CHIP_ID_MIDDLE_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_M );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_M );
   
    sensor_chipid_info_t* pChipIDInfo_L = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_L )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_L, 0, sizeof(*pChipIDInfo_L) ); 
    pChipIDInfo_L->chipid_reg_addr = TC358749XBG_CHIP_ID_LOW_BYTE;
    pChipIDInfo_L->chipid_reg_value = TC358749XBG_CHIP_ID_LOW_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_L );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_L );

    //-----------------------kings add read system ctrl REG -----------------------------
	sensor_chipid_info_t* pChipIDInfo_0006 = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_0006 )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_0006, 0, sizeof(*pChipIDInfo_0006) ); 
    pChipIDInfo_0006->chipid_reg_addr = 0x0006;
    pChipIDInfo_0006->chipid_reg_value = 0x0010;
    ListPrepareItem( pChipIDInfo_0006 );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_0006 );

    sensor_chipid_info_t* pChipIDInfo_000A = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_000A )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_000A, 0, sizeof(*pChipIDInfo_000A) ); 
    pChipIDInfo_000A->chipid_reg_addr = 0x000A;
    pChipIDInfo_000A->chipid_reg_value = 0x0100;
    ListPrepareItem( pChipIDInfo_000A );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_000A );

	sensor_chipid_info_t* pChipIDInfo_000C = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_000C )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_000C, 0, sizeof(*pChipIDInfo_000C) ); 
    pChipIDInfo_000C->chipid_reg_addr = 0x000C;
    pChipIDInfo_000C->chipid_reg_value = 0x3435;
    ListPrepareItem( pChipIDInfo_000C );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_000C );

	sensor_chipid_info_t* pChipIDInfo_000E = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_000E )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_000E, 0, sizeof(*pChipIDInfo_000E) ); 
    pChipIDInfo_000E->chipid_reg_addr = 0x000E;
    pChipIDInfo_000E->chipid_reg_value = 0x3637;
    ListPrepareItem( pChipIDInfo_000E );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_000E );

	sensor_chipid_info_t* pChipIDInfo_0010 = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_0010 )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_0010, 0, sizeof(*pChipIDInfo_0010) ); 
    pChipIDInfo_0010->chipid_reg_addr = 0x0010;
    pChipIDInfo_0010->chipid_reg_value = 0x0024;
    ListPrepareItem( pChipIDInfo_0010 );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_0010 );
	//----------------------kings add end---------------------------------------------
*/
	//oyyf sensor drv version
	pSensorI2cInfo->sensor_drv_version = CONFIG_SENSOR_DRV_VERSION;

    *pdata = pSensorI2cInfo;

	TRACE( TC358749XBG_ERROR, "%s (Exit)\n", __FUNCTION__);
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
    TC358749XBG_IsiGetSensorIss,
    {
        0,                      /**< IsiSensor_t.pszName */
        0,                      /**< IsiSensor_t.pRegisterTable */
        0,                      /**< IsiSensor_t.pIsiSensorCaps */
        0,						/**< IsiSensor_t.pIsiGetSensorIsiVer_t>*/   //oyyf add
        0,                      /**< IsiSensor_t.pIsiGetSensorTuningXmlVersion_t>*/   //oyyf add 
        0,                      /**< IsiSensor_t.pIsiWhiteBalanceIlluminationChk>*/   //ddl@rock-chips.com 
        0,                      /**< IsiSensor_t.pIsiWhiteBalanceIlluminationSet>*/   //ddl@rock-chips.com
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
    TC358749XBG_IsiGetSensorI2cInfo,
};



