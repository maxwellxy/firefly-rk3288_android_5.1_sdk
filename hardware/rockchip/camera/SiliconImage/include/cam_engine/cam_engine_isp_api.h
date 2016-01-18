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
 * @cond    cam_engine_isp
 *
 * @file    cam_engine_isp_api.h
 *
 * @brief
 *
 *   Interface description of the CamEngine ISP.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_engine_isp_api CamEngine ISP API
 * @{
 *
 */

#ifndef __CAM_ENGINE_ISP_API_H__
#define __CAM_ENGINE_ISP_API_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <cameric_drv/cameric_isp_lsc_drv_api.h>

/******************************************************************************/
/**
 * @brief   Structure to configure the lense shade correction
 *
 * @note    This structure needs to be converted to driver structure
 *
 *****************************************************************************/
typedef struct CamEngineLscConfig_s
{
    struct
    {
        uint16_t LscXGradTbl[CAEMRIC_GRAD_TBL_SIZE];    /**< multiplication factors of x direction  */
        uint16_t LscYGradTbl[CAEMRIC_GRAD_TBL_SIZE];    /**< multiplication factors of y direction  */
        uint16_t LscXSizeTbl[CAEMRIC_GRAD_TBL_SIZE];    /**< sector sizes of x direction            */
        uint16_t LscYSizeTbl[CAEMRIC_GRAD_TBL_SIZE];    /**< sector sizes of y direction            */
    } grid;

    struct 
    {
        uint16_t LscRDataTbl[CAMERIC_DATA_TBL_SIZE];    /**< correction values of R color part */
        uint16_t LscGRDataTbl[CAMERIC_DATA_TBL_SIZE];   /**< correction values of G (red lines) color part */
        uint16_t LscGBDataTbl[CAMERIC_DATA_TBL_SIZE];   /**< correction values of G (blue lines) color part  */
        uint16_t LscBDataTbl[CAMERIC_DATA_TBL_SIZE];    /**< correction values of B color part  */
    } gain;
 
} CamEngineLscConfig_t;


/******************************************************************************/
/**
 * @brief   Enumeration type to configure the horizontal clip mode
 *
 * @note    Defines the maximum red/blue pixel shift in horizontal direction 
 *          At pixel positions, that require a larger displacement, the maximum
 *          shift value is used instead (vector clipping) 
 *
 *****************************************************************************/
typedef enum CamEngineCacHorizontalClipMode_e
{
    CAM_ENGINE_CAC_H_CLIPMODE_INVALID  = 0,    /**< lower border (only for an internal evaluation) */
    CAM_ENGINE_CAC_H_CLIPMODE_FIX4     = 1,    /**< horizontal vector clipping +/-4 pixel displacement (default) */
    CAM_ENGINE_CAC_H_CLIPMODE_DYN5     = 2,    /**< horizontal vector clipping to +/-4 or +/-5 pixel displacement 
                                                 depending on pixel position inside the bayer raster (dynamic 
                                                 switching between +/-4 and +/-5) */
    CAM_ENGINE_CAC_H_CLIPMODE_MAX              /**< upper border (only for an internal evaluation) */
} CamEngineCacHorizontalClipMode_t;


/******************************************************************************/
/**
 * @brief   Enumeration type to configure the vertical clip mode
 *
 * @note    Defines the maximum red/blue pixel shift in vertical direction 
 *
 *****************************************************************************/
typedef enum CamEngineCacVerticalClipMode_e
{
    CAM_ENGINE_CAC_V_CLIPMODE_INVALID  = 0,    /**< lower border (only for an internal evaluation) */
    CAM_ENGINE_CAC_V_CLIPMODE_FIX2     = 1,    /**< vertical vector clipping to +/-2 pixel */
    CAM_ENGINE_CAC_V_CLIPMODE_FIX3     = 2,    /**< vertical vector clipping to +/-3 pixel */
    CAM_ENGINE_CAC_V_CLIBMODE_DYN4     = 3,    /**< vertical vector clipping +/-3 or +/-4 pixel displacement 
                                                 depending on pixel position inside the bayer raster (dynamic
                                                 switching between +/-3 and +/-4) */
    CAM_ENGINE_CAC_V_CLIPMODE_MAX              /**< upper border (only for an internal evaluation) */
} CamEngineCacVerticalClipMode_t;


/******************************************************************************/
/**
 * @brief   Structure to configure the chromatic aberration correction
 *
 * @note    This structure needs to be converted to driver structure
 *
 *****************************************************************************/
typedef struct CamEngineCacConfig_s
{
    uint16_t                            width;          /**< width of the input image in pixel */
    uint16_t                            height;         /**< height of the input image in pixel */

    int16_t                             hCenterOffset;  /**< horizontal offset between image center and optical center of the input image in pixels */
    int16_t                             vCenterOffset;  /**< vertical offset between image center and optical center of the input image in pixels */

    CamEngineCacHorizontalClipMode_t    hClipMode;      /**< maximum red/blue pixel shift in horizontal direction */
    CamEngineCacVerticalClipMode_t      vClipMode;      /**< maximum red/blue pixel shift in vertical direction */

    float                            aBlue;          /**< parameters for radial shift calculation 9 bit twos complement with 4 fractional digits, valid range -16..15.9375 */
    float                            aRed;           /**< parameters for radial shift calculation 9 bit twos complement with 4 fractional digits, valid range -16..15.9375 */

    float                           bBlue;          /**< parameters for radial shift calculation 9 bit twos complement with 4 fractional digits, valid range -16..15.9375 */
    float                           bRed;           /**< parameters for radial shift calculation 9 bit twos complement with 4 fractional digits, valid range -16..15.9375 */

    float                           cBlue;          /**< parameters for radial shift calculation 9 bit twos complement with 4 fractional digits, valid range -16..15.9375 */
    float                          cRed;           /**< parameters for radial shift calculation 9 bit twos complement with 4 fractional digits, valid range -16..15.9375 */

    uint8_t                             Xns;            /**< horizontal normal shift parameter */
    uint8_t                             Xnf;            /**< horizontal scaling factor */
    
    uint8_t                             Yns;            /**< vertical normal shift parameter */
    uint8_t                             Ynf;            /**< vertical scaling factor */
} CamEngineCacConfig_t;


/******************************************************************************/
/**
 * @brief   Structure to configure the wide dynamic range curve.
 *
 * @note    This structure needs to be converted to driver structure
 *
 *****************************************************************************/
typedef struct CamEngineWdrCurve_s
{
    uint16_t            Ym[33];
    uint8_t             dY[33];
} CamEngineWdrCurve_t;


/******************************************************************************/
/**
 * @brief   Enumeration type for x scaling of the gamma curve 
 *
 * @note    This structure needs to be converted to driver structure
 *
 *****************************************************************************/
typedef enum CamEngineGammaOutXScale_e
{
    CAM_ENGINE_GAMMAOUT_XSCALE_INVALID  = 0,    /**< lower border (only for an internal evaluation) */
    CAM_ENGINE_GAMMAOUT_XSCALE_LOG      = 1,    /**< logarithmic segmentation from 0 to 4095 
                                                     (64,64,64,64,128,128,128,128,256,256,256,512,512,512,512,512) */
    CAM_ENGINE_GAMMAOUT_XSCALE_EQU      = 2,    /**< equidistant segmentation from 0 to 4095
                                                     (256, 256, ... ) */
    CAM_ENGINE_GAMMAOUT_XSCALE_MAX              /**< upper border (only for an internal evaluation) */
} CamEngineGammaOutXScale_t;


/******************************************************************************/
/**
 * @brief   Structure to configure the gamma curve.
 *
 * @note    This structure needs to be converted to driver structure
 *
 *****************************************************************************/
typedef struct CamEngineGammaOutCurve_s
{
    CamEngineGammaOutXScale_t   xScale;
    uint16_t                    GammaY[CAMERIC_ISP_GAMMA_CURVE_SIZE];
} CamEngineGammaOutCurve_t;


/*****************************************************************************/
/**
 * @brief   This function sets the black-level.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   Red                 red cells
 * @param   GreenR              green (red neighbors) cells
 * @param   GreenB              green (blue neighbors) cells
 * @param   Blue                blue cells
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineBlsSet
(
    CamEngineHandle_t   hCamEngine,
    uint16_t const      Red,
    uint16_t const      GreenR,
    uint16_t const      GreenB,
    uint16_t const      Blue
);


/*****************************************************************************/
/**
 * @brief   This function returns the black-level.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pRed                red cells
 * @param   pGreenR             green (red neighbors) cells
 * @param   pGreenB             green (blue neighbors) cells
 * @param   pBlue               blue cells
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineBlsGet
(
    CamEngineHandle_t   hCamEngine,
    uint16_t * const    pRed,
    uint16_t * const    pGreenR,
    uint16_t * const    pGreenB,
    uint16_t * const    pBlue
);


/*****************************************************************************/
/**
 * @brief   This function sets the white balance gains.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pGains              white balance gains
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineWbSetGains
(
    CamEngineHandle_t                   hCamEngine,
    const CamEngineWbGains_t * const    pGains
);


/*****************************************************************************/
/**
 * @brief   This function returns the white balance gains.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pGains              white balance gains
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineWbGetGains
(
    CamEngineHandle_t           hCamEngine,
    CamEngineWbGains_t * const  pGains
);


/*****************************************************************************/
/**
 * @brief   This function sets the cross talk matrix.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pCcMatrix           cross talk matrix
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineWbSetCcMatrix
(
    CamEngineHandle_t                   hCamEngine,
    const CamEngineCcMatrix_t * const   pCcMatrix
);


/*****************************************************************************/
/**
 * @brief   This function returns the cross talk matrix.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pCcMatrix           cross talk matrix
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineWbGetCcMatrix
(
    CamEngineHandle_t           hCamEngine,
    CamEngineCcMatrix_t * const pCcMatrix
);


/*****************************************************************************/
/**
 * @brief   This function sets the cross talk offset.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pCcOffset           cross talk offset
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineWbSetCcOffset
(
    CamEngineHandle_t                   hCamEngine,
    const CamEngineCcOffset_t * const   pCcOffset
);


/*****************************************************************************/
/**
 * @brief   This function returns the cross talk offset.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pCcOffset           cross talk offset
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineWbGetCcOffset
(
    CamEngineHandle_t           hCamEngine,
    CamEngineCcOffset_t * const pCcOffset
);


/*****************************************************************************/
/**
 * @brief   This function sets the demosaicing.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   Bypass              bypass mode
 * @param   Threshold           threshold
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineDemosaicSet
(
    CamEngineHandle_t   hCamEngine,
    bool_t const        Bypass,
    uint8_t const       Threshold
);


/*****************************************************************************/
/**
 * @brief   This function returns the demosaicing.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pBypass             bypass mode
 * @param   pThreshold          threshold
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineDemosaicGet
(
    CamEngineHandle_t   hCamEngine,
    bool_t * const      pBypass,
    uint8_t * const     pThreshold
);


/*****************************************************************************/
/**
 * @brief   This function returns the current status and configuration 
 *          of the LSC module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pRunning            activation status of LSC module
 * @param   pConfig             configuration of LSC module
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineLscStatus
(
    CamEngineHandle_t               hCamEngine,
    bool_t * const                  pRunning,
    CamEngineLscConfig_t * const    pConfig
);


/*****************************************************************************/
/**
 * @brief   This function enables the LSC module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineLscEnable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function disables the LSC module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineLscDisable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function returns the current configuration of the LSC module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pConfig             configuration of LSC module
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineLscGetConfig
(
    CamEngineHandle_t               hCamEngine,
    CamEngineLscConfig_t * const    pConfig
);


/*****************************************************************************/
/**
 * @brief   This function enables the wide dynamic range module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineWdrEnable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function disables the wide dynamic range module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineWdrDisable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function loads a WDR curve.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   WdrCurve            WDR curve to load
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineWdrSetCurve
(
    CamEngineHandle_t hCamEngine,
    CamEngineWdrCurve_t WdrCurve
);


/*****************************************************************************/
/**
 * @brief   This function returns the status of the gamma correction module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineGammaStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t * const      pRunning
);


/*****************************************************************************/
/**
 * @brief   This function enables the gamma correction module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineGammaEnable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function disables the gamma correction module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineGammaDisable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function loads a correction curve into gamma correction
 *          module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   GammaCurve          gamma curve to set 
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineGammaSetCurve
(
    CamEngineHandle_t           hCamEngine,
    CamEngineGammaOutCurve_t    GammaCurve
);


/*****************************************************************************/
/**
 * @brief   This function returns the current status and configuration 
 *          of the Cac module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pRunning            activation status of LSC module
 * @param   pConfig             configuration of LSC module
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineCacStatus
(
    CamEngineHandle_t               hCamEngine,
    bool_t * const                  pRunning,
    CamEngineCacConfig_t * const    pConfig
);


/*****************************************************************************/
/**
 * @brief   This function enables the CAC module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineCacEnable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function disables the CAC module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineCacDisable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function returns the status of the filter module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_INVALID_PARM    invalid parameter (i.e. a parameter is NULL)
 *
 *****************************************************************************/
RESULT CamEngineFilterStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t * const      pRunning,
    uint8_t * const     pDenoiseLevel,
    uint8_t * const     pSharpenLevel
);


/*****************************************************************************/
/**
 * @brief   This function enables the filter module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineFilterEnable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function disables the filter module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineFilterDisable
(
    CamEngineHandle_t hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function sets the levels of the filter module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   DenoiseLevel        denoising level to configure
 * @param   SharpenLevel        dharpening level to configure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 *
 *****************************************************************************/
RESULT CamEngineFilterSetLevel
(
    CamEngineHandle_t   hCamEngine,
    uint8_t const       DenoiseLevel,
    uint8_t const       SharpenLevel
);


/*****************************************************************************/
/**
 * @brief   This function returns RET_SUCCESS if the color noise reduction
 *          module available in hardware.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         CNR available
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_NOTSUPP         CNR module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineCnrIsAvailable
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function enables the color noise reduction module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_NOTSUPP         CNR module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineCnrEnable
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function disables the color noise reduction module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_NOTSUPP         CNR module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineCnrDisable
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function returns the color noise reduction status.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pRunning            BOOL_TRUE: running, BOOL_FALSE: stopped
 * @param   pThreshold1         Threshold Color Channel 1
 * @param   pThreshold2         Threshold Color Channel 2
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_NOTSUPP         CNR module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineCnrStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t * const      pRunning,
    uint32_t * const    pThreshold1,
    uint32_t * const    pThreshold2
);


/*****************************************************************************/
/**
 * @brief   This function sets the thresholds of the color noise reduction
 *          module.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   Threshold1          Threshold Color Channel 1
 * @param   Threshold2          Threshold Color Channel 2
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_NOTAVAILABLE    module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineCnrSetThresholds
(
    CamEngineHandle_t   hCamEngine,
    uint32_t const      Threshold1,
    uint32_t const      Threshold2
);


void CamEngineSetIsSocSensor
(
    CamEngineHandle_t   hCamEngine,
    bool isSoc
);

void CamEngineSetBufferSize
(
    CamEngineHandle_t   hCamEngine,
    uint32_t bufNum,
    uint32_t bufSize
);

RESULT CamEngineSetColorConversionRange
(
    CamEngineHandle_t           hCamEngine,
    CamerIcColorConversionRange_t YConvRange,
    CamerIcColorConversionRange_t CrConvRange
);

RESULT CamEngineSetColorConversionCoefficients
(
     CamEngineHandle_t           hCamEngine,
    CamerIc3x3Matrix_t    *pCConvCoefficients
);



#ifdef __cplusplus
}
#endif


/* @} cam_engine_isp_api */


#endif /* __CAM_ENGINE_ISP_API_H__ */

