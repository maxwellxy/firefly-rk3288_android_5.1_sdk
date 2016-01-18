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
 * @cond    cam_engine_jpe
 *
 * @file    cam_engine_jpe_api.h
 *
 * @brief
 *
 *   Interface description of the CamEngine JPE.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_engine_jpe_api CamEngine JPE API
 * @{
 *
 */

#ifndef __CAM_ENGINE_JPE_API_H__
#define __CAM_ENGINE_JPE_API_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif

//FIXME
#include <cameric_drv/cameric_jpe_drv_api.h>


/*****************************************************************************/
/**
 * @brief   This function enables the jpe.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   pConfig             configuration of jpe
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_CONFIG    image effects isn't configured
 * @retval  RET_NOTAVAILABLE    module not available by driver or hardware
 *
 *****************************************************************************/
RESULT CamEngineEnableJpe
(
    CamEngineHandle_t hCamEngine,
    CamerIcJpeConfig_t *pConfig
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
RESULT CamEngineDisableJpe
(
    CamEngineHandle_t hCamEngine
);


#ifdef __cplusplus
}
#endif

/* @} cam_engine_jpe_api */

#endif /* __CAM_ENGINE_JPE_API_H__ */

