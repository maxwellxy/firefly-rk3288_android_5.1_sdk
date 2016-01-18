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
 * @cond    cameric_isp_flt
 *
 * @file cameric_isp_flt_drv_api.h
 *
 * @brief
 *  Interface description of FLT Module.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_flt_drv_api CamerIc ISP FLT Driver API definitions
 * @{
 *
 * @image html cameric20MP_isp_flt.png "CamerIC ISP FLT driver" width=\textwidth
 * @image latex cameric20MP_isp_flt.png "CamerIC ISP FLT driver" width=\textwidth
 *
 * The Filter block is a combined de-noising and sharpening filter, which is integrated in the
 * demosaicing module. This gate-count efficient implementation leads to restrictions for
 * usage of the filter block.
 *
 * The de-noising settings should be (automatically) adapted to the ambient lighting,
 * because CMOS sensors produce more noise in low light conditions. With low light the
 * setting should be moved to higher de-noising values.
 *
 * The level of sharpening depends on the used lens system as well as the shutter settings.
 * The longer the shutter time, the more blurring may occur, which could be masked by
 * sharpening.
 *
 */

#ifndef __CAMERIC_ISP_FLT_DRV_API_H__
#define __CAMERIC_ISP_FLT_DRV_API_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief   Enumeration type to configure de-noising level.
 *
 */
typedef enum CamerIcIspFltDeNoiseLevel_e
{
    CAMERIC_ISP_FLT_DENOISE_LEVEL_INVALID   = 0,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_0         = 1,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_1         = 2,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_2         = 3,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_3         = 4,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_4         = 5,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_5         = 6,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_6         = 7,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_7         = 8,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_8         = 9,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_9         = 10,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_10        = 11,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_TEST      = 12,
    CAMERIC_ISP_FLT_DENOISE_LEVEL_MAX
} CamerIcIspFltDeNoiseLevel_t;



/**
 * @brief   Enumeration type to configure sharpening level.
 *
 */
typedef enum CamerIcIspFltSharpeningLevel_e
{
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_INVALID   = 0,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_0         = 1,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_1         = 2,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_2         = 3,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_3         = 4,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_4         = 5,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_5         = 6,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_6         = 7,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_7         = 8,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_8         = 9,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_9         = 10,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_10        = 11,
    CAMERIC_ISP_FLT_SHARPENING_LEVEL_MAX
} CamerIcIspFltSharpeningLevel_t;



/*****************************************************************************/
/**
 * @brief   Enable FLT Module.
 *
 * @param   handle          CamerIc driver handle.
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspFltEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Disable FLT Module.
 *
 * @param   handle          CamerIc driver handle.
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspFltDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Get FLT Module status.
 *
 * @param   handle          CamerIc driver handle.
 * @param   pIsEnabled
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspFltIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   Get ISP filter parameter.
 *
 * @param   handle      CamerIc driver handle.
 * @param   pDeNoiseLevel
 * @param   pSharpeningLevel
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspFltGetFilterParameter
(
    CamerIcDrvHandle_t                      handle,
    CamerIcIspFltDeNoiseLevel_t             *pDeNoiseLevel,
    CamerIcIspFltSharpeningLevel_t          *pSharpeningLevel
);



/*****************************************************************************/
/**
 * @brief   Set ISP filter parameter.
 *
 * @param   handle      CamerIc driver handle.
 * @param   DeNoiseLevel
 * @param   SharpeningLevel
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspFltSetFilterParameter
(
    CamerIcDrvHandle_t                      handle,
    const CamerIcIspFltDeNoiseLevel_t       DeNoiseLevel,
    const CamerIcIspFltSharpeningLevel_t    SharpeningLevel
);


#ifdef __cplusplus
}
#endif

/* @} cameric_isp_flt_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_FLT_DRV_API_H__ */

