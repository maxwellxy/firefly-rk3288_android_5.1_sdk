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
 * @cond    cam_engine_ie
 *
 * @file    cam_engine_imgeffects_api.h
 *
 * @brief
 *
 *   Interface description of the CamEngine Image Effects.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_engine_imgeffects_api CamEngine Image Effects API
 * @{
 *
 */

#ifndef __CAM_ENGINE_IMG_EFFECTS_API_H__
#define __CAM_ENGINE_IMG_EFFECTS_API_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif

//FIXME
#include <cameric_drv/cameric_ie_drv_api.h>


/*****************************************************************************/
/**
 * @brief   This function enables the image effects.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_CONFIG    image effects isn't configured
 * @retval  RET_NOTAVAILABLE    module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineEnableImageEffect
(
    CamEngineHandle_t hCamEngine,
    CamerIcIeConfig_t *pConfig
);


/*****************************************************************************/
/**
 * @brief   This function disables the image effects.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_NOTAVAILABLE    module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineDisableImageEffect
(
    CamEngineHandle_t hCamEngine
);


RESULT CamEngineImageEffectSetTintCb
(
    CamEngineHandle_t   hCamEngine,
    const uint8_t       tint
);


RESULT CamEngineImageEffectSetTintCr
(
    CamEngineHandle_t   hCamEngine,
    const uint8_t       tint
);


RESULT CamEngineImageEffectSetColorSelection
(
    CamEngineHandle_t               hCamEngine,
    const CamerIcIeColorSelection_t color,
    const uint8_t                   threshold
);


RESULT CamEngineImageEffectSetSharpen
(
    CamEngineHandle_t               hCamEngine,
    const uint8_t                   factor,
    const uint8_t                   threshold
);


#ifdef __cplusplus
}
#endif

/* @} cam_engine_imgeffects_api */

#endif /* __CAM_ENGINE_IMG_EFFECTS_API_H__ */

