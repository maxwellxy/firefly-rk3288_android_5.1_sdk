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
 * @cond    cam_engine_mi
 *
 * @file    cam_engine_mi_api.h
 *
 * @brief
 *
 *   Interface description of the CamEngine MI.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_engine_mi_api CamEngine MI Api
 * @{
 *
 */

#ifndef __CAM_ENGINE_MI_API_H__
#define __CAM_ENGINE_MI_API_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/**
 * @brief   Enumeration type to specify the picture orientation on the self
 *          path
 *
 * @note    same as CamerIcMiOrientation_t, but we want to make 
 *          it independently from driver here to easily port software on 
 *          OS with kernel-/user-mode
 *
 *****************************************************************************/
//! self picture operating modes
typedef enum  CamEngineMiOrientation_e
{
    CAM_ENGINE_MI_ORIENTATION_INVALID          = 0,    /**< lower border (only for an internal evaluation) */
    CAM_ENGINE_MI_ORIENTATION_ORIGINAL         = 1,    /**< no rotation, no horizontal or vertical flipping */
    CAM_ENGINE_MI_ORIENTATION_VERTICAL_FLIP    = 2,    /**< vertical   flipping (no additional rotation) */
    CAM_ENGINE_MI_ORIENTATION_HORIZONTAL_FLIP  = 3,    /**< horizontal flipping (no additional rotation) */
    CAM_ENGINE_MI_ORIENTATION_ROTATE90         = 4,    /**< rotation  90 degrees ccw (no additional flipping) */
    CAM_ENGINE_MI_ORIENTATION_ROTATE180        = 5,    /**< rotation 180 degrees ccw (equal to horizontal plus vertical flipping) */
    CAM_ENGINE_MI_ORIENTATION_ROTATE270        = 6,    /**< rotation 270 degrees ccw (no additional flipping) */
    CAM_ENGINE_MI_ORIENTATION_MAX                      /**< upper border (only for an internal evaluation) */
} CamEngineMiOrientation_t;

/*****************************************************************************/
/**
 * @brief   This function returns RET_SUCCESS if the dual cropping unit 
 *          available in hardware.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         DCROP available
 * @retval  RET_WRONG_STATE     invalid state (i.e. cam-eninge is not running)
 * @retval  RET_NOTSUPP         DCROP module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineDualCroppingIsAvailable
(
    CamEngineHandle_t   hCamEngine
);

/*****************************************************************************/
/**
 * @brief   This function swap the color channels.
 *
 * @note    Normal order is U than V byte, which works for image formats
 *          like N21, ... But there are also some image formats available 
 *          which using a swapped byte order like NV12, ... .
 *          This function enables or disable color channel swapping to 
 *          support all available image formats.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   swap                enable/disable swapping color channels 
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change datamode
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
RESULT CamEngineMiSwapUV
(
    CamEngineHandle_t   hCamEngine,
    bool_t const        swap
);


/*****************************************************************************/
/**
 * @brief   This function checks if the picture orientation is allowed in 
 *          currently running mode. 
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   orientation         Picture orientation (@ref CamEngineMiOrientation_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change datamode
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
RESULT CamEngineIsPictureOrientationAllowed
(
    CamEngineHandle_t           hCamEngine,
    CamEngineMiOrientation_t    orientation
);


/*****************************************************************************/
/**
 * @brief   This function enables/disables horizontal flip.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   orientation         Picture orientation (@ref CamEngineMiOrientation_e) to set
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_CONFIG    image effects isn't configured
 * @retval  RET_NOTAVAILABLE    module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineSetPictureOrientation
(
    CamEngineHandle_t           hCamEngine,
    CamEngineMiOrientation_t    orientation
);


#ifdef __cplusplus
}
#endif

/* @} cam_engine_mi_api */

#endif /* __CAM_ENGINE_MI_API_H__ */


