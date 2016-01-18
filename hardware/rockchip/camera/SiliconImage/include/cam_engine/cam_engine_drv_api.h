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
 * @cond    cam_engine_drv
 *
 * @file    cam_engine_drv_api.h
 *
 * @brief
 *
 *   Interface function to the CamerIc Driver.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_engine_drv_api CamEngine DRV API
 * @{
 *
 */

#ifndef __CAM_ENGINE_ISP_DRV_H__
#define __CAM_ENGINE_ISP_DRV_H__

#include <ebase/types.h>

#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/**
 * @brief   This function returns the CamerIc (Master) Revision ID.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   revision            reference to CamerIc Master Revision ID
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_INVALID_PARM    invalid parameter (revision is a NULL pointer)
 *
 *****************************************************************************/
RESULT CamEngineCamerIcMasterId
(
    CamEngineHandle_t   hCamEngine,
    uint32_t            *revision
);


/*****************************************************************************/
/**
 * @brief   This function returns the CamerIc (Slave) Revision ID.
 *
 * @param   hCamEngine          handle to the CamEngine instance
 * @param   revision            reference to CamerIc Slave Revision ID
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_INVALID_PARM    invalid parameter (revision is a NULL pointer)
 *
 *****************************************************************************/
RESULT CamEngineCamerIcSlaveId
(
    CamEngineHandle_t   hCamEngine,
    uint32_t            *revision
);


#ifdef __cplusplus
}
#endif


/* @} cam_engine_isp_api */


#endif /* __CAM_ENGINE_ISP_DRV_H__ */

