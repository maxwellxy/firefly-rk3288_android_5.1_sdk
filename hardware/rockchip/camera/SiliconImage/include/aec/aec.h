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
#ifndef __AEC_H__
#define __AEC_H__

/**
 * @file aec.h
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
 * @defgroup AECM Auto white Balance Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_exp_drv_api.h>
#include <cameric_drv/cameric_isp_hist_drv_api.h>

#include <cam_calibdb/cam_calibdb_api.h>

#ifdef __cplusplus
extern "C"
{
#endif



typedef struct AecContext_s *AecHandle_t;       /**< handle to AEC context */


/*****************************************************************************/
/**
 *          AecSemMode_t
 *
 * @brief   mode type of AEC Scene Evaluation
 *
 */
/*****************************************************************************/
typedef enum AecSemMode_e
{
    AEC_SCENE_EVALUATION_INVALID    = 0,        /* invalid (only used for initialization) */
    AEC_SCENE_EVALUATION_DISABLED   = 1,        /* Scene Evaluation disabled (fix setpoint) */
    AEC_SCENE_EVALUATION_FIX        = 2,        /* Scene Evaluation fix (static ROI) */
    AEC_SCENE_EVALUATION_ADAPTIVE   = 3,        /* Scene Evaluation adaptive (ROI caluclated by Scene Evaluation */
    AEC_SCENE_EVALUATION_MAX
} AecSemMode_t;


/*****************************************************************************/
/**
 *          AecDampingMode_t
 *
 * @brief   mode type of AEC Damping
 *
 */
/*****************************************************************************/
typedef enum AecDampingMode_e
{
    AEC_DAMPING_MODE_INVALID        = 0,        /* invalid (only used for initialization) */
    AEC_DAMPING_MODE_STILL_IMAGE    = 1,        /* damping mode still image */
    AEC_DAMPING_MODE_VIDEO          = 2,        /* damping mode video */
    AEC_DAMPING_MODE_MAX
} AecDampingMode_t;


/*****************************************************************************/
/**
 *          AecEcmMode_t
 *
 * @brief   mode type of AEC Exposure Conversion
 *
 */
/*****************************************************************************/
typedef enum AecEcmMode_e
{
    AEC_EXPOSURE_CONVERSION_INVALID = 0,        /* invalid (only used for initialization) */
    AEC_EXPOSURE_CONVERSION_LINEAR  = 1,        /* Exposure Conversion uses a linear function (eq. 38) */
    AEC_EXPOSURE_CONVERSION_MAX
} AecEcmMode_t;


/*****************************************************************************/
/**
 *          AecEcmFlickerPeriod_t
 *
 * @brief   flicker period types for the AEC algorithm
 *
 */
/*****************************************************************************/
typedef enum AecEcmFlickerPeriod_e
{
    AEC_EXPOSURE_CONVERSION_FLICKER_OFF   = 0x00,
    AEC_EXPOSURE_CONVERSION_FLICKER_100HZ = 0x01,
    AEC_EXPOSURE_CONVERSION_FLICKER_120HZ = 0x02
} AecEcmFlickerPeriod_t;


/*****************************************************************************/
/**
 *
 * @brief   callback function type for AEC/AFPS resolution change request
 *
 */
/*****************************************************************************/
typedef void (AfpsResChangeCb_t)
(
    void        *pPrivateContext,               /**< reference to user context as handed in via AecInstanceConfig_t */
    uint32_t    NewResolution                   /**< new resolution to switch to */
);


/*****************************************************************************/
/**
 * @brief   AEC Module instance configuration structure
 *
 *****************************************************************************/
typedef struct AecInstanceConfig_s
{
    AfpsResChangeCb_t   *pResChangeCbFunc;      /**< callback function to trigger resolution change */
    void                *pResChangeCbContext;   /**< reference to context to pass to callback */

    AecHandle_t         hAec;                   /**< handle returned by AecInit() */
} AecInstanceConfig_t;


/*****************************************************************************/
/**
 *          AecConfig_t
 *
 * @brief   AEC Module configuration structure; used for re-configuration as well
 *
 *****************************************************************************/
typedef struct AecConfig_s
{
    IsiSensorHandle_t       hSensor;                /**< sensor handle; not evaluated during re-configuration */
    IsiSensorHandle_t       hSubSensor;             /**< sensor handle; not evaluated during re-configuration */

    CamCalibDbHandle_t      hCamCalibDb;            /**< calibration database handle */

    AecDampingMode_t        DampingMode;            /**< damping mode */
    AecSemMode_t            SemMode;                /**< scene evaluation mode */

    float                   SetPoint;               /**< set point to hit by the ae control system */
    float                   ClmTolerance;

    float                   DampOverStill;          /**< damping coefficient for still image mode */
    float                   DampUnderStill;         /**< damping coefficient for still image mode */
    float                   DampOverVideo;          /**< damping coefficient for video mode */
    float                   DampUnderVideo;         /**< damping coefficient for video mode */

    bool_t                  AfpsEnabled;            /**< AFPS mode control */
    float                   AfpsMaxGain;            /**< AFPS max gain */

    AecEcmFlickerPeriod_t   EcmFlickerSelect;       /**< flicker period selection */
//    float                   EcmT0fac;             /**< start of flicker avoidance as multiple of flicker period: EcmT0 = EcmT0fac * EcmTflicker*/
//    float                   EcmA0;                /**< linear: slope of gain */
} AecConfig_t;


/*****************************************************************************/
/**
 * AecSetMeanLumaGridWeights
 *
 
 *
 *****************************************************************************/
RESULT AecSetMeanLumaGridWeights
(
    AecHandle_t handle,
    const unsigned char  *pWeights
);

/*****************************************************************************/
/**
 * @brief   This function creates and initializes an AEC instance.
 *
 * @param   pInstConfig     pointer to the instance configuration
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT AecInit
(
    AecInstanceConfig_t *pInstConfig
);



/*****************************************************************************/
/**
 * @brief   The function releases/frees the given AEC instance.
 *
 * @param   handle  AEC instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecRelease
(
    AecHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function configures the AEC instance.
 *
 * @param   handle      AEC instance handle
 * @param   pConfig     pointer to configuration structure
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AecConfigure
(
    AecHandle_t handle,
    AecConfig_t *pConfig
);



/*****************************************************************************/
/**
 * @brief   This function re-configures the AEC instance, e.g. after a
 *          resolution change
 *
 * @param   handle              AEC instance handle
 * @param   pConfig             pointer to configuration structure
 * @param   pNumFramesToSkip    reference of storage for number of frames that have to be skipped
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AecReConfigure
(
    AecHandle_t handle,
    AecConfig_t *pConfig,
    uint32_t    *pNumFramesToSkip
);



/*****************************************************************************/
/**
 * @brief   This functions processes the Scene Evaluation Module (SEM) of the
 *          given AEC instance for the current frame.
 *
 * @param   handle      AEC instance handle
 * @param   luma        5x5 array of the measured mean luma values
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecSemExecute
(
    AecHandle_t         handle,
    CamerIcMeanLuma_t   luma
);



/*****************************************************************************/
/**
 * @brief   This function processes the Control Loop Module (CLM) of the
 *          given AEC instance for the current frame.
 *
 * @param   handle      AEC instance handle
 * @param   bins        histogramm
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecClmExecute
(
    AecHandle_t         handle,
    CamerIcHistBins_t   bins
);



/*****************************************************************************/
/**
 * @brief   This function returns BOOL_TRUE if the AEC is settled.
 *
 * @param   handle      AEC instance handle
 * @param   pSettled    pointer to settled value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecSettled
(
    AecHandle_t handle,
    bool_t      *pSettled
);



/*****************************************************************************/
/**
 * @brief   This function returns the currently calculated Gain.
 *
 * @param   handle      AEC instance handle
 * @pram    pGain       pointer to gain value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 *
 *****************************************************************************/
RESULT AecGetCurrentGain
(
    AecHandle_t handle,
    float       *pGain
);



/*****************************************************************************/
/**
 * @brief   This function returns the currently calculated IntegrationTime.
 *
 * @param   handle              AEC instance handle
 * qparam   pIntegrationTime    pointer to integration time value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 *
 *****************************************************************************/
RESULT AecGetCurrentIntegrationTime
(
    AecHandle_t handle,
    float       *pIntegrationTime
);



/*****************************************************************************/
/**
 * @brief   This function returns the current histogram.
 *
 * @param   handle              AEC instance handle
 * qparam   pHistogram          pointer to the histogram bins
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 *
 *****************************************************************************/
RESULT AecGetCurrentHistogram
(
    AecHandle_t         handle,
    CamerIcHistBins_t   *pHistogram
);



/*****************************************************************************/
/**
 * @brief   This function returns the current luminance grid.
 *
 * @param   handle              AEC instance handle
 * qparam   pHistogram          pointer to the luminance grid
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 *
 *****************************************************************************/
RESULT AecGetCurrentLuminance
(
    AecHandle_t         handle,
    CamerIcMeanLuma_t   *pLuma
);

/*****************************************************************************/
/**
 * @brief   This function returns the current ClmTolerance.
 *
 * @param   handle              AEC instance handle
 * qparam   clmtolerance          pointer to the clmtolerance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 *
 *****************************************************************************/
RESULT AecGetClmTolerance
(
    AecHandle_t         handle,
    float   *clmtolerance
);

/*****************************************************************************/
/**
 * @brief   This function set the current ClmTolerance.
 *
 * @param   handle              AEC instance handle
 * qparam   clmtolerance          new clmtolerance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 *
 *****************************************************************************/
RESULT AecSetClmTolerance
(
	AecHandle_t 		handle,
	float	clmtolerance
);

/*****************************************************************************/
/**
 * @brief   This function returns the current object region.
 *
 * @param   handle              AEC instance handle
 * qparam   pHistogram          pointer to the object region
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 *
 *****************************************************************************/
RESULT AecGetCurrentObjectRegion
(
    AecHandle_t         handle,
    CamerIcMeanLuma_t   *pObjectRegion
);



/*****************************************************************************/
/**
 * @brief   This function returns the current configuration.
 *
 * @param   handle      AEC instance handle
 * @param   pConfig     reference of configuration structure to be filled with
 *                      the current configuration
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT AecGetCurrentConfig
(
    AecHandle_t handle,
    AecConfig_t *pConfig
);



/*****************************************************************************/
/**
 * @brief   This function returns the status of the AEC module.
 *
 * @param   handle      AEC instance handle
 * @param   pRunning    pointer to state value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecStatus
(
    AecHandle_t         handle,
    bool_t              *pRunning,
    AecSemMode_t        *pMode,
    float               *pSetPoint,
    float               *pClmTolerance,
    AecDampingMode_t    *pDampingMode,
    float               *pDampOverStill,
    float               *pDampUnderStill,
    float               *pDampOverVideo,
    float               *pDampUnderVideo
);



/*****************************************************************************/
/**
 * @brief   This function starts the AEC instance.
 *
 * @param   handle  AEC instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT AecStart
(
    AecHandle_t handle
);


/*****************************************************************************/
/**
 * @brief
 *
 * @param   handle  AEC instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecStop
(
    AecHandle_t handle
);



/*****************************************************************************/
/**
 * @brief
 *
 * @param   handle  AEC instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecReset
(
    AecHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions tries to lock the AEC instance.
 *
 * @param   handle  AEC instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecTryLock
(
    AecHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This functions unlocks the AEC instance.
 *
 * @param   handle  AEC instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AecUnLock
(
    AecHandle_t handle
);




#ifdef __cplusplus
}
#endif

/* @} AECM */


#endif /* __AEC_H__*/
