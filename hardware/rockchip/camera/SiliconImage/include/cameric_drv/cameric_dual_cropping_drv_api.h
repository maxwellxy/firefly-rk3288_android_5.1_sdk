/******************************************************************************
 *
 * Copyright 2013, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
#ifndef __CAMERIC_DUAL_CROPPING_DRV_API_H__
#define __CAMERIC_DUAL_CROPPING_DRV_API_H__

/**
 * @file cameric_dual_cropping_drv_api.h
 *
 * @brief   This file contains the CamerIC Dual Cropping driver API definitions.
 *
 *****************************************************************************/
/**
 * @cond cameric_dual_cropping
 *
 * @defgroup cameric_dual_cropping_drv_api CamerIc Dual cropping Driver API definitions
 * @{
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/
/**
 * @brief   Enumeration type to configure the dual cropping unit. 
 *
 *****************************************************************************/
typedef enum CamerIcDualCropingMode_e
{
    CAMERIC_DCROP_INVALID          = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_DCROP_BYPASS           = 1,    /**< bypass the dual cropping unit */
    CAMERIC_DCROP_YUV              = 2,    /**< crop YUV data */
    CAMERIC_DCROP_RAW              = 3,    /**< crop RAW/Bayer data */
    CAMERIC_DCROP_MAX                      /**< upper border (only for an internal evaluation) */
} CamerIcDualCropingMode_t;

/*****************************************************************************/
/**
 * @brief   This function returns RET_SUCCESS if the dual cropping unit 
 *          available in hardware.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         CNR available
 * @retval  RET_NOTSUPP         CNR not available
 *
 *****************************************************************************/
extern RESULT CamerIcDualCropingIsAvailable
(
    CamerIcDrvHandle_t handle
);

/*****************************************************************************/
/**
 * @brief   This function returns the current configured cropping window
 *          for the given path.
 *
 * @param   handle              CamerIc driver handle
 * @param   path                Path index of CamerIC (@ref CamerIcMiPath_e)
 * @param   pMode               configured working mode
 * @param   pWin                current configured cropping window
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to get cropping window
 *
 *****************************************************************************/
extern RESULT CamerIcDualCropingGetOutputWindow
(
    CamerIcDrvHandle_t                  handle,
    CamerIcMiPath_t const               path,
    CamerIcDualCropingMode_t * const    pMode,
    CamerIcWindow_t * const             pWin
);

/*****************************************************************************/
/**
 * @brief   This function sets the current cropping window for the given path.
 *
 * @param   handle              CamerIc driver handle
 * @param   path                Path index of CamerIC (@ref CamerIcMiPath_e)
 * @param   mode                working mode
 * @param   pWin                cropping window to set
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to set cropping window
 *
 *****************************************************************************/
extern RESULT CamerIcDualCropingSetOutputWindow
(
    CamerIcDrvHandle_t              handle,
    CamerIcMiPath_t const           path,
    CamerIcDualCropingMode_t const  mode,
    CamerIcWindow_t * const         pWin
);

#ifdef __cplusplus
}
#endif

/* @} cameric_dual_cropping_drv_api */

/* @endcond */

#endif /* __CAMERIC_DUAL_CROPPING_DRV_API_H__ */

