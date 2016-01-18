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
#ifndef __AWB_H__
#define __AWB_H__

/**
 * @file awb.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup AWBM Auto white Balance Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cam_types.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_awb_drv_api.h>
#include <cameric_drv/cameric_isp_lsc_drv_api.h>
#include <cameric_drv/cameric_isp_hist_drv_api.h>

#include <cam_calibdb/cam_calibdb_api.h>

#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 * @brief
 */
/*****************************************************************************/
typedef struct AwbContext_s *AwbHandle_t;           /**< handle to AWB context */



/*****************************************************************************/
/**
 * @brief
 */
/*****************************************************************************/
typedef enum AwbWorkingFlags_e
{
    AWB_WORKING_FLAG_USE_DAMPING        = 0x01,
    AWB_WORKING_FLAG_USE_CC_OFFSET      = 0x02
} AwbWorkingFlags_t;



/*****************************************************************************/
/**
 * @brief
 *
 */
/*****************************************************************************/
typedef enum AwbRunMode_e
{
    AWB_MODE_INVALID                    = 0,        /**< initialization value */
    AWB_MODE_MANUAL                     = 1,        /**< run manual white balance */
    AWB_MODE_AUTO                       = 2,        /**< run auto white balance */
    AWB_MODE_MAX
} AwbMode_t;



/*****************************************************************************/
/**
 * @brief   type for evaluatiing number of white pixel
 */
/*****************************************************************************/
typedef enum AwbNumWhitePixelEval_e
{
    AWB_NUM_WHITE_PIXEL_INVALID         = 0,        /**< initialization value */
    AWB_NUM_WHITE_PIXEL_LTMIN           = 1,        /**< less than configured minimum */
    AWB_NUM_WHITE_PIXEL_GTMAX           = 2,        /**< greater than defined maximum */
    AWB_NUM_WHITE_PIXEL_TARGET_RANGE    = 3,        /**< in min max range */
    AWB_NUM_WHITE_PIXEL_MAX
} AwbNumWhitePixelEval_t;



/*****************************************************************************/
/**
 * @brief
 *
 */
/*****************************************************************************/
typedef struct AwbComponent_s
{
    float   fRed;
    float   fGreen;
    float   fBlue;
} AwbComponent_t;



/*****************************************************************************/
/**
 * @brief   A structure/tupple to represent gain values for four (R,Gr,Gb,B)
 *          channels.
 *
 * @note    The gain values are represented as float numbers.
 */
/*****************************************************************************/
typedef struct AwbGains_s
{
    float fRed;         /**< gain value for the red channel */
    float fGreenR;      /**< gain value for the green channel in red lines */
    float fGreenB;      /**< gain value for the green channel in blue lines */
    float fBlue;        /**< gain value for the blue channel */
} AwbGains_t;



/*****************************************************************************/
/**
 * @brief   A structure/tupple to represent gain values for four (R,Gr,Gb,B)
 *          channels.
 *
 * @note    The gain values are represented as signed numbers.
 */
/*****************************************************************************/
typedef struct AwbXTalkOffset_s
{
    float fRed;         /**< value for the red channel */
    float fGreen;       /**< value for the green channel in red lines */
    float fBlue;        /**< value for the blue channel */
} AwbXTalkOffset_t;



/*****************************************************************************/
/**
 *          AwbInstanceConfig_t
 *
 * @brief   AWB Module instance configuration structure
 *
 *****************************************************************************/
typedef struct AwbInstanceConfig_s
{
    AwbHandle_t                     hAwb;               /**< handle returns by AwbInit() */
} AwbInstanceConfig_t;



/*****************************************************************************/
/**
 *          AwbConfig_t
 *
 * @brief   AWB Module configuration structure
 *
 *****************************************************************************/
typedef struct AwbConfig_s
{
    AwbMode_t                       Mode;               /**< White Balance working mode (MANUAL | AUTO) */
    
    CamerIcDrvHandle_t              hCamerIc;           /**< cameric driver handle */
    CamerIcDrvHandle_t              hSubCamerIc;        /**< cameric driver handle */

    uint16_t                        width;              /**< picture width */
    uint16_t                        height;             /**< picture height */
    float                           framerate;          /**< frame rate */

    uint32_t                        Flags;              /**< working flags (@see AwbWorkingFlags_e) */

    CamCalibDbHandle_t              hCamCalibDb;        /**< calibration database handle */

    CamerIcIspAwbMeasuringMode_t    MeasMode;           /**< specifies the means measuring mode (YCbCr or RGB) */
    CamerIcAwbMeasuringConfig_t     MeasConfig;         /**< measuring config */

    float                           fStableDeviation;   /**< min deviation in percent to enter stable state */
    float                           fRestartDeviation;  /**< max tolerated deviation in precent for staying in stable state */
} AwbConfig_t;



/*****************************************************************************/
/**
 *          AwbRgProj_t
 *
 * @brief   AWB Projection Borders in R/G Layer
 *
 *****************************************************************************/
typedef struct AwbRgProj_s
{
    float                           fRgProjIndoorMin;
    float                           fRgProjOutdoorMin;
    float                           fRgProjMax;
    float                           fRgProjMaxSky;

	float 							fRgProjALimit;    //oyyf
	float							fRgProjAWeight;		//oyyf
	float 							fRgProjYellowLimit;		//oyyf
	float							fRgProjIllToCwf;		//oyyf
	float							fRgProjIllToCwfWeight;	//oyyf
} AwbRgProj_t;



/*****************************************************************************/
/**
 * @brief   This function converts float based gains into CamerIC 2.8 fixpoint
 *          format.
 *
 * @param   pAwbGains           gains in float based format
 * @param   pCamerIcGains       gains in fix point format (2.8)
 *
 * @return                      Returns the result of the function call.
 * @retval  RET_SUCCESS         gains sucessfully converted
 * @retval  RET_NULL_POINTER    null pointer parameter
 *
 *****************************************************************************/
RESULT AwbGains2CamerIcGains
(
    AwbGains_t      *pAwbGains,
    CamerIcGains_t  *pCamerIcGains
);



/*****************************************************************************/
/**
 * @brief   This function converts CamerIC 2.8 fixpoint format into float
 *          based gains.
 *
 * @param   pCamerIcGains       gains in fix point format (2.8)
 * @param   pAwbGains           gains in float based format
 *
 * @return                      Returns the result of the function call.
 * @retval  RET_SUCCESS         gains sucessfully converted
 * @retval  RET_NULL_POINTER    null pointer parameter
 *
 *****************************************************************************/
RESULT CamerIcGains2AwbGains
(
    CamerIcGains_t  *pCamerIcGains,
    AwbGains_t      *pAwbGains
);



/*****************************************************************************/
/**
 * @brief       This function converts float based Color correction matrix
 *              values into CamerIC 4.7 fixpoint format.
 *
 * @param[in]   pAwbXTalkOffset     offset as float values
 * @param[out]  pCamerIcXTalkOffset offsets as 2's complement integer
 *
 * @return                          Returns the result of the function call.
 * @retval      RET_SUCCESS         offsets sucessfully converted
 * @retval      RET_NULL_POINTER    null pointer parameter
 *
 *****************************************************************************/
RESULT AwbXtalk2CamerIcXtalk
(
    Cam3x3FloatMatrix_t *pAwbXTalkMatrix,
    CamerIc3x3Matrix_t  *pXTalkMatrix
);



/*****************************************************************************/
/**
 * @brief       This function converts CamerIC 4.7 fixpoint format based Color
 *              correction matrix into float based values.
 *
 * @param[in]   pCamerIcXTalkOffset offsets as 2's complement integer
 * @param[out]  pAwbXTalkOffset     offset as float values
 *
 * @return                          Returns the result of the function call.
 * @retval      RET_SUCCESS         offsets sucessfully converted
 * @retval      RET_NULL_POINTER    null pointer parameter
 *
 *****************************************************************************/
RESULT CamerIcXtalk2AwbXtalk
(
    CamerIc3x3Matrix_t  *pXTalkMatrix,
    Cam3x3FloatMatrix_t *pAwbXTalkMatrix
);



/*****************************************************************************/
/**
 * @brief   This function converts float based offset values into CamerIC 12.0
 *          fix point format based offset values.
 *
 * @param   pAwbXTalkOffset     offset as float values
 * @param   pCamerIcXTalkOffset offsets as 2's complement integer
 *
 * @return                      Returns the result of the function call.
 * @retval  RET_SUCCESS         offsets sucessfully converted
 * @retval  RET_NULL_POINTER    null pointer parameter
 *
 *****************************************************************************/
RESULT AwbXTalkOffset2CamerIcXTalkOffset
(
    Cam1x3FloatMatrix_t     *pAwbXTalkOffset,
    CamerIcXTalkOffset_t    *pCamerIcXTalkOffset
);



/*****************************************************************************/
/**
 * @brief   This function converts CamerIC 12.0 fix point format based offset
 *          values into float based offset value.
 *
 * @param   pCamerIcXTalkOffset offsets as 2's complement integer
 * @param   pAwbXTalkOffset     offset as float values
 *
 * @return                      Returns the result of the function call.
 * @retval  RET_SUCCESS         offsets sucessfully converted
 * @retval  RET_NULL_POINTER    null pointer parameter
 *
 *****************************************************************************/
RESULT CamerIcXTalkOffset2AwbXTalkOffset
(
    CamerIcXTalkOffset_t    *pCamerIcXTalkOffset,
    AwbXTalkOffset_t        *pAwbXTalkOffset
);



/*****************************************************************************/
/**
 * @brief   This function initializes the Auto White Balance Module.
 *
 * @param   handle      AWB instance handle
 * @param   pInstConfig pointer instance configuration structure
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_INVALID_PARM
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT AwbInit
(
    AwbInstanceConfig_t *pInstConfig
);



/*****************************************************************************/
/**
 * @brief   The function releases/frees the Auto White Balance module.
 *
 * @param   handle      AWB instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbRelease
(
    AwbHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function configures the Auto White Balance Module.
 *
 * @param   handle      AWB instance handle
 * @param   pConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AwbConfigure
(
    AwbHandle_t handle,
    AwbConfig_t *pConfig
);



/*****************************************************************************/
/**
 * @brief   This function re-configures the Auto White Balance Module
 *          after e.g. resolution change
 *
 * @param   handle      AWB instance handle
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AwbReConfigure
(
    AwbHandle_t handle,
    AwbConfig_t *pConfig
);



/*****************************************************************************/
/**
 * @brief   This function configures the Auto White Balance Module.
 *
 * @param   handle      AWB instance handle
 * @param   pRgConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AwbRgProjConfigure
(
    AwbHandle_t     handle,
    AwbRgProj_t     *pRgConfig
);



/*****************************************************************************/
/**
 * @brief   This function returns true if the AWB hit the convergence criteria.
 *
 * @param   handle      AWB instance handle
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AwbStable
(
    AwbHandle_t handle,
    bool_t      *pStable
);



/*****************************************************************************/
/**
 * @brief   The function
 *
 * @param   handle      AWB instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbStart
(
    AwbHandle_t         handle,
    const AwbMode_t     mode,
    const uint32_t      idx
);



/*****************************************************************************/
/**
 * @brief   The function stops the auto white balance.
 *
 * @param   handle      AWB instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbStop
(
    AwbHandle_t         handle
);



/*****************************************************************************/
/**
 * @brief   The function resets the auto white balance module.
 *
 * @param   handle      AWB instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbReset
(
    AwbHandle_t     handle
);



/*****************************************************************************/
/**
 * @brief   The function sets/changes AWB processing flags
 *
 * @param   handle      AWB instance handle
 * @param   Flags       AWB processing flags to set
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbSetFlags
(
    AwbHandle_t     handle,
    const uint32_t  Flags
);



/*****************************************************************************/
/**
 * @brief   The function sets the histogram
 *
 * @param   handle      AWB instance handle
 * @param   bins        histogramm
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbSetHistogram
(
    AwbHandle_t         handle,
    CamerIcHistBins_t   bins
);



/*****************************************************************************/
/**
 * @brief   The function returns AWB processing flags
 *
 * @param   handle      AWB instance handle
 * @param   pFlags      pointer to store current AWB processing flags
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbGetFlags
(
    AwbHandle_t handle,
    uint32_t    *pFlags
);



/*****************************************************************************/
/**
 * @brief   The function returns current status values of the AWB.
 *
 * @param   handle      AWB instance handle
 * @param   pRunning    pointer to return current run-state of AWB module
 * @param   pMode       pointer to return current operation mode of AWB module
 * @param   pIlluIdx    pointer to return current start profile index
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbStatus
(
    AwbHandle_t     handle,
    bool_t          *pRunning,      /**< BOOL_TRUE: running, BOOL_FALSE: stopped */
    AwbMode_t       *pMode,
    uint32_t        *pIlluIdx,
    AwbRgProj_t     *pRgProj
);


/******************************************************************************
 * AwbSettled()
 *****************************************************************************/
RESULT AwbSettled
(
    AwbHandle_t handle,
    bool_t      *pSettled,
    uint32_t 	*pDNoWhitePixel
);

/*****************************************************************************/
/**
 * @brief   The function starts AWB processing
 *
 * @param   handle      AWB instance handle
 * @param   pMeasResult pointer tu current AWB measuring data from hardware
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbProcessFrame
(
    AwbHandle_t                         handle,
    const CamerIcAwbMeasuringResult_t   *pMeasResult,
    const float                         fGain,
    const float                         fIntegrationTime
);



/*****************************************************************************/
/**
 * @brief   The function
 *
 * @param   handle  AWB instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbTryLock
(
    AwbHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   The function
 *
 * @param   handle  AWB instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AwbUnLock
(
    AwbHandle_t handle
);

RESULT AwbGetGainParam
(
	AwbHandle_t handle,
	float *f_RgProj, 
	float *f_s, 
	float *f_s_Max1, 
	float *f_s_Max2, 
	float *f_Bg1, 
	float *f_Rg1, 
	float *f_Bg2, 
	float *f_Rg2
);

RESULT AwbGetIlluEstInfo
(
	AwbHandle_t handle,
	float *ExpPriorIn,
	float *ExpPriorOut,
	char (*name)[20],
	float likehood[],
	float wight[],
	int *curIdx,
	int *region,
	int *count
);

#ifdef __cplusplus
}
#endif

/* @} AWBM */


#endif /* __AWB_H__*/
