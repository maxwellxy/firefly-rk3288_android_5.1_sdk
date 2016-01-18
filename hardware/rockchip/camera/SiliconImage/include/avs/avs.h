/******************************************************************************
 *
 * Copyright 2012, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
#ifndef __AVS_H__
#define __AVS_H__

/**
 * @file avs.h
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
 * @defgroup AVSM Auto Video Stabilization Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_vsm_drv_api.h>

#include <cam_engine/cam_engine_common.h>


#ifdef __cplusplus
extern "C"
{
#endif


/******************************************************************************/
/**
 * @brief   Default parameters for default damping function.
 *
 ******************************************************************************/
#define NUM_ITP_POINTS_DEFAULT  17
#define THETA_DEFAULT           0.5
#define BASE_GAIN_DEFAULT       1.0
#define FALL_OFF_DEFAULT        0.5
#define ACCELERATION_DEFAULT    4.0



/******************************************************************************/
/**
 * @brief   Handle to AVS context.
 *
 ******************************************************************************/
typedef struct AvsContext_s *AvsHandle_t;



/******************************************************************************/
/**
 * @brief   Interpolation point for the AVS damping functions.
 *
 ******************************************************************************/
typedef struct AvsDampingPoint_s
{
    float offset; /**< absolute offset from 0.0 to 1.0 (= max offset) */
    float value;  /**< value of the damping function for this offset */
} AvsDampingPoint_t;



/*****************************************************************************/
/**
 *          AvsInstanceConfig_t
 *
 * @brief   AVS Module instance configuration structure
 *
 *****************************************************************************/
typedef struct AvsInstanceConfig_s
{
    AvsHandle_t             hAvs;         /**< handle returned by AvsInit() */
} AvsInstanceConfig_t;



/*****************************************************************************/
/**
 *          AvsDampFuncParams_t
 *
 * @brief   AVS damping function parameters for default damping function
 *          (alternative to configuring LUT).
 *
 *          The normalized damping function used here (ranging from 0.0 to 1.0,
 *          representing 0 to maximum absolute horizontal offset resp. 0 to
 *          maximum absolute  vertical offset) is, with
 *          (note that x may also be negative)
 *
 *          v = abs(x) - theta:
 *
 *                  _
 *                 |
 *                 | baseGain, if v < 0
 *          f(x) = |--
 *                 | baseGain - fallOff * sin(v/(1-theta) * PI/2)^acceleration,
 *                 |           if v >= 0
 *                 |_
 *
 *****************************************************************************/
typedef struct AvsDampFuncParams_s
{
    uint16_t numItpPoints; /**< Number of interpolation points to use
                                when generating lookup table. Must be >= 1.
                                Note that the first of these points will be
                                placed at x0 = theta, since for all points below
                                the first value of x0, f(x) = f(theta) =
                                baseGain is inferred.
                                Further note that if e.g. theta = 0.5 and you
                                want a distance of 1/32 between interpolation
                                points above x = theta, you need to set
                                numItpPoints to 32*(1 - 0.5) + 1 = 17. */
    float    theta;        /**< Can range from 0.0 to 1.0.
                                Default value is @ref THETA_DEFAULT.  */
    float    baseGain;     /**< Must be >= 0 and <= 1. Default value is @ref
                                BASE_GAIN_DEFAULT.  */
    float    fallOff;      /**< Must be <= baseGain. Default value is @ref
                                FALL_OFF_DEFAULT.  */
    float    acceleration; /**< Must be >= 0. Default value is @ref
                                ACCELERATION_DEFAULT. */
} AvsDampFuncParams_t;



/*****************************************************************************/
/**
 *          AvsConfig_t
 *
 * @brief   AVS Module configuration structure
 *
 *****************************************************************************/
typedef struct AvsConfig_s
{
    CamerIcDrvHandle_t      hCamerIc;
    CamerIcDrvHandle_t      hSubCamerIc;

    uint16_t          offsetDataArraySize;
                                  /**< Size of array in which we store resulting
                                       offset vectors for frames. */

    CamEngineWindow_t srcWindow;  /**< Source window from which this module
                                       shall select a region to crop. */
    CamEngineWindow_t dstWindow;  /**< Destination window which is a cropped
                                       region of the source window. */
    uint16_t           dampLutHorSize; /**< Number of elements pointed to by
                                           pDampLutHor. */
    uint16_t           dampLutVerSize; /**< Number of elements pointed to by
                                           pDampLutVer. */
    AvsDampingPoint_t *pDampLutHor; /**< Lookup table for damping function in
                                         horizontal direction. Values for
                                         offsets between two LUT entries are
                                         calculated by linear interpolation.
                                         If NULL, AVS uses pDampParamsHor
                                         instead.  */
    AvsDampingPoint_t *pDampLutVer; /**< Same as pDampLutHor, but for vertical
                                         direction.  */
    AvsDampFuncParams_t *pDampParamsHor;
                                    /**< Parameters for default horizontal
                                         damping function AVS uses to generate
                                         its own internal LUT. Only considered
                                         if pDampLutHor is NULL. If also NULL in
                                         this case, default parameters are used.
                                          */
    AvsDampFuncParams_t *pDampParamsVer;
                                    /**< Same as pDampParamsHor, but for
                                     *   vertical direction. */

} AvsConfig_t;



/*****************************************************************************/
/**
 *          AvsInit()
 *
 * @brief   This function initializes the Auto Video Stabilization Module.
 *
 * @param   pConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_INVALID_PARM
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT AvsInit
(
    AvsInstanceConfig_t *pInstConfig
);



/*****************************************************************************/
/**
 *          AvsRelease()
 *
 * @brief   The function releases/frees the Auto Video Stabilization Module.
 *
 * @param   handle  Handle to AVSM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AvsRelease
(
    AvsHandle_t handle
);



/*****************************************************************************/
/**
 *          AvsConfigure()
 *
 * @brief   This function configures the Auto Video Stabilization Module.
 *
 * @param   handle  Handle to AVSM
 * @param   pConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 * @retval  RET_NOTSUPP         hardware doesn't support AVS
 *
 *****************************************************************************/
RESULT AvsConfigure
(
    AvsHandle_t handle,
    AvsConfig_t *pConfig
);


/*****************************************************************************/
/**
 *          AvsReConfigure()
 *
 * @brief   This function re-configures the Auto Video Stabilization Module,
 *          e.g. in case of a resolution change.
 *
 * @param   handle  Handle to AVSM
 * @param   pConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 * @retval  RET_NOTSUPP         hardware doesn't support AVS
 *
 *****************************************************************************/
RESULT AvsReConfigure
(
    AvsHandle_t handle,
    AvsConfig_t *pConfig
);


/*****************************************************************************/
/**
 *          AvsStart()
 *
 * @brief   The function starts the Auto Video Stabilization Module.
 *
 * @param   handle  Handle to AVSM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AvsStart
(
    AvsHandle_t handle
);



/*****************************************************************************/
/**
 *          AvsStop()
 *
 * @brief   The function stops the Auto Video Stabilization Module.
 *
 * @param   handle  Handle to AVSM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AvsStop
(
    AvsHandle_t handle
);



/*****************************************************************************/
/**
 *          AvsGetConfig()
 *
 * @brief   The function retrieves the current configuration of the
 *          Auto Video Stabilization Module.
 *
 * @param   handle          Handle to AVSM
 * @param   pConfig         Pointer to retrieve current configuration
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT AvsGetConfig
(
    AvsHandle_t   handle,
    AvsConfig_t  *pConfig
);



/*****************************************************************************/
/**
 *          AvsGetStatus()
 *
 * @brief   The function retrieves the status of the
 *          Auto Video Stabilization Module.
 *
 * @param   handle          Handle to AVSM
 * @param   pRunning        Pointer to retrieve current state, can be NULL
 * @param   pCurrDisplVec   Pointer to retrieve current displacement vector,
 *                          can be NULL
 * @param   pCurrOffsetVec  Pointer to retrieve current offset vector
 *                          (offset of cropping window from center position),
 *                          can be NULL
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
RESULT AvsGetStatus
(
    AvsHandle_t         handle,
    bool_t             *pRunning,
    CamEngineVector_t  *pCurrDisplVec,
    CamEngineVector_t  *pCurrOffsetVec
);



/*****************************************************************************/
/**
 *          AvsProcessFrame()
 *
 * @brief   The function lets the Auto Video Stabilization Module process
 *          a frame.
 *          Note that only offset data is calculated for the given frame and
 *          stored. The resulting cropping window is not applied until
 *          AvsSetCroppingWindow() is called with the related frameId.
 *
 * @param   handle     Handle to AVSM
 * @param   frameId    Id of frame to be processed
 * @param   pDisplVec  Pointer to measured displacement vector
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE No more free storage for offset data,
 *                      offsetDataArraySize has been set too low on
 *                      configuration.
 *
 *****************************************************************************/
RESULT AvsProcessFrame
(
    AvsHandle_t        handle,
    ulong_t            frameId,
    CamEngineVector_t *pDisplVec
);



/*****************************************************************************/
/**
 *          AvsSetCroppingWindow()
 *
 * @brief   The function lets the Auto Video Stabilization Module configure a
 *          previously calculated cropping window in the image stabilization
 *          driver. The cropping window is the one calculated when
 *          AvsProcessFrame was called with the same frameId as this function.
 *          The offset data calculated for the frame is discarded after this
 *          operation.
 *
 * @param   handle     Handle to AVSM
 * @param   frameId    Id of frame for which cropping window shall be set
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE Data for frame identified by frameId cannot be found.
 *
 *****************************************************************************/
RESULT AvsSetCroppingWindow
(
    AvsHandle_t        handle,
    ulong_t            frameId
);



#ifdef __cplusplus
}
#endif

/* @} AVSM */


#endif /* __AVS_H__*/
