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
#ifndef __CAMERIC_ISP_IS_DRV_API_H__
#define __CAMERIC_ISP_IS_DRV_API_H__

/**
 * @cond    cameric_isp_is
 *
 * @file    cameric_isp_is_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP IS (image stabilization) Driver
 *          API definitions
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_is_drv_api CamerIC ISP IS Driver API definitions
 * @{
 *
 * @image html cameric20MP_isp_is.png "CamerIC ISP IS driver" width=\textwidth
 * @image latex cameric20MP_isp_is.png "CamerIC ISP IS driver" width=\textwidth
 *
 * For image stabilization a sub frame must be chosen from the input frame, because
 * a certain border around the output image is needed to have some margin for correcting
 * the position of the image. Based on externally generated camera global motion data,
 * image stabilization offers the functionality to compensate for that camera motion by
 * moving the chosen sub frame across the input frame according to the signaled camera
 * motion. The information source for global motion may be a motion sensor or extracted
 * from the image content by a video encoder.
 *
 * To prevent the sub frame from running out of the input frame, the sub frame is re-centered at a
 * programmable rate that is proportional to its distance from the center of the input frame.
 *
 * @image html cameric20MP_isp_is_parameters.png "Illustration of image stabilization parameters" width=\textwidth
 * @image latex cameric20MP_isp_is_parameters.png "Illustration of image stabilization parameters" width=\textwidth
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include "cameric_drv_api.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 * @brief   This functions enables the CamerIC ISP image stabilization module.
 *
 *          Important note: This only enables the image stabilization
 *          functionality of the IS HW. If disabled, the IS will still output
 *          data, but no displacement will be applied to the configured
 *          output window.
 *
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions disables the CamerIC ISP image stabilization module.
 *
 *          See also important note for CamerIcIspIsEnable.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Get CamerIC ISP image stabilization status.
 *
 * @param   handle          CamerIc driver handle.
 * @param   pIsEnabled      Points to value indicating if image stabilization is
 *                          enabled if function returns RET_SUCCESS
 *
 * @return                  Return the result of the function call.
 * @retval RET_SUCCESS      operation succeeded
 * @retval RET_WRONG_HANDLE handle is invalid
 * @retval RET_NULL_POINTER NULL pointer passed
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This functions sets the recenter exponent in the
 *          CamerIC ISP image stabilization.
 *
 * @param   handle              CamerIc driver handle
 * @param   recenterExp         Exponent for recentering. A value of 0 switches
 *                              recentering off. A value of 1 .. 7 applies a
 *                              recentering by
 *                              (current displacement)/(2^recenterExp) pixels
 *                              in both directions for each frame, where the
 *                              center position is the one programmed by
 *                              CamerIcIspIsSetOutputWindow.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_CONFIG    invalid value of recenterExp
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsSetRecenterExponent
(
    CamerIcDrvHandle_t handle,
    uint8_t            recenterExp
);



/*****************************************************************************/
/**
 * @brief   This functions gets the recenter exponent currently configured in
 *          the CamerIC ISP image stabilization.
 *
 * @param   handle              CamerIc driver handle
 * @param   pRecenterExp        Points to current recenter exponent if function
 *                              returns RET_SUCCESS.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    NULL pointer passed
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsGetRecenterExponent
(
    CamerIcDrvHandle_t  handle,
    uint8_t            *pRecenterExp
);



/*****************************************************************************/
/**
 * @brief   This functions sets the output window in the
 *          CamerIC ISP image stabilization. The offsets given by the window are
 *          regarded as "centered", i.e. this is the final window which is
 *          output when the configured displacement is (0,0).
 *
 * @param   handle              CamerIc driver handle
 * @param   pOutWin             Pointer to output window to be configured.
 *                              Maximum value for each element is 0x3FFF.
 * @param   force_upd           Update the window immediatly.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    NULL pointer passed
 * @retval  RET_WRONG_CONFIG    invalid window configuration
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsSetOutputWindow
(
    CamerIcDrvHandle_t    handle,
    CamerIcWindow_t      *pOutWin,
    bool_t                force_upd
);



/*****************************************************************************/
/**
 * @brief   This functions gets the output window currently configured in the
 *          CamerIC ISP image stabilization.
 *
 * @param   handle              CamerIc driver handle
 * @param   pOutWin             Points to current configured output window
 *                              if function returns RET_SUCCESS.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    NULL pointer passed
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsGetOutputWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pOutWin
);



/*****************************************************************************/
/**
 * @brief   This functions gets the window as currently output by the HW,
 *          depending on the configured "centered" output window, the configured
 *          displacement and possibly applied recentering.
 *
 * @param   handle              CamerIc driver handle
 * @param   pDisplWin           Points to current displaced output window
 *                              if function returns RET_SUCCESS.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    NULL pointer passed
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsGetDisplacedOutputWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pDisplWin
);



/*****************************************************************************/
/**
 * @brief   This functions sets the maximum displacement for the
 *          CamerIC ISP image stabilization.
 *
 * @param   handle              CamerIc driver handle
 * @param   maxDisplHor         Maximum absolute value of horizontal
 *                              displacement from configured "centered" output
 *                              window. Allowed range is 0x0000 to 0x0FFF.
 * @param   maxDisplVer         Maximum absolute value of vertical
 *                              displacement from configured "centered" output
 *                              window. Allowed range is 0x0000 to 0x0FFF.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_CONFIG    invalid configuration
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsSetMaxDisplacement
(
    CamerIcDrvHandle_t    handle,
    uint16_t              maxDisplHor,
    uint16_t              maxDisplVer
);



/*****************************************************************************/
/**
 * @brief   This functions gets the maximum displacement currently configured in
 *          the CamerIC ISP image stabilization.
 *
 * @param   handle              CamerIc driver handle
 * @param   pMaxDisplHor        If function returns RET_SUCCESS, points to
 *                              currently configured maximum absolute value of
 *                              horizontal displacement from configured
 *                              "centered" output window.
 * @param   pMaxDisplVer        If function returns RET_SUCCESS, points to
 *                              currently configured maximum absolute value of
 *                              vertical displacement from configured
 *                              "centered" output window.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    NULL pointer passed
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsGetMaxDisplacement
(
    CamerIcDrvHandle_t    handle,
    uint16_t             *pMaxDisplHor,
    uint16_t             *pMaxDisplVer
);



/*****************************************************************************/
/**
 * @brief   This functions sets the displacement to be applied in the
 *          CamerIC ISP image stabilization.
 *
 * @param   handle              CamerIc driver handle
 * @param   displHor            Horizontal displacement from configured
 *                              "centered" output window to be applied. The
 *                              absolute vale of displHor must not exceed the
 *                              currently configured maximum horizontal
 *                              displacement value. Allowed range is
 *                              given by the configuration applied by the last
 *                              call of CamerIcIspIsSetMaxDisplacement.
 * @param   displVer            Vertical displacement from configured
 *                              "centered" output window to be applied. The
 *                              absolute vale of displVer must not exceed the
 *                              currently configured maximum vertical
 *                              displacement value. Allowed range is
 *                              given by the configuration applied by the last
 *                              call of CamerIcIspIsSetMaxDisplacement.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_CONFIG    invalid configuration
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsSetDisplacement
(
    CamerIcDrvHandle_t    handle,
    int16_t               displHor,
    int16_t               displVer
);



/*****************************************************************************/
/**
 * @brief   This functions gets the displacement currently configured in the
 *          CamerIC ISP image stabilization.
 *
 * @param   handle              CamerIc driver handle
 * @param   pDisplHor           If function returns RET_SUCCESS, points to
 *                              currently configured horizontal displacement
 *                              from configured "centered" output window.
 * @param   pDisplVer           If function returns RET_SUCCESS, points to
 *                              currently configured vertical displacement
 *                              from configured "centered" output window.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    NULL pointer passed
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsGetDisplacement
(
    CamerIcDrvHandle_t    handle,
    int16_t              *pDisplHor,
    int16_t              *pDisplVer
);



#ifdef __cplusplus
}
#endif

/* @} cameric_isp_is_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_IS_DRV_API_H__ */

