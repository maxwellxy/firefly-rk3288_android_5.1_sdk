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
#ifndef __CAMERIC_ISP_AWB_DRV_API_H__
#define __CAMERIC_ISP_AWB_DRV_API_H__

/**
 * @cond    cameric_isp_awb
 *
 * @file    cameric_isp_awb_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP AWB driver API definitions.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_awb_drv_api CamerIC ISP AWB driver API definitions
 * @{
 *
 * @image html cameric20MP_isp_awb.png "CamerIC ISP AWB driver" width=\textwidth
 * @image latex cameric20MP_isp_awb.png "CamerIC ISP AWB driver" width=\textwidth
 *
 * The AWB module is able to measure the occurrence of near-white pixels 
 * in an image processed by the CamerIC. To be able to measure the desired
 * pixels the AWB module supports to configure the measurement window. 
 *
 * Further on the component gains could be adjusted to control the white point.
 * The Automatic White Balancing driver serves as an abstraction layer, so the
 * application does not need to know which bit has to be set where in the 
 * registers of the AWB module.
 *
 * The mean value measurement is done by hardware and the gain update has to
 * be done by software of the host processor.
 *
 */

#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/**
 * @brief   Enumeration type to configure CamerIC ISP measuring mode
 *
 *****************************************************************************/
typedef enum CamerIcIspAwbMeasuringMode_e
{
    CAMERIC_ISP_AWB_MEASURING_MODE_INVALID    = 0,      /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_AWB_MEASURING_MODE_YCBCR      = 1,      /**< near white discrimination mode using YCbCr color space */
    CAMERIC_ISP_AWB_MEASURING_MODE_RGB        = 2,      /**< RGB based measurement mode */
    CAMERIC_ISP_AWB_MEASURING_MODE_MAX,                 /**< upper border (only for an internal evaluation) */
} CamerIcIspAwbMeasuringMode_t;



/******************************************************************************/
/**
 * @brief   CamerIC AWB measurement configuration
 *
 ******************************************************************************/
typedef struct CamerIcAwbMeasuringConfig_s
{
    uint8_t MaxY;           /**< YCbCr Mode: only pixels values Y <= ucMaxY contribute to WB measurement (set to 0 to disable this feature) */
                            /**< RGB Mode  : unused */
    uint8_t RefCr_MaxR;     /**< YCbCr Mode: Cr reference value */
                            /**< RGB Mode  : only pixels values R < MaxR contribute to WB measurement */
    uint8_t MinY_MaxG;      /**< YCbCr Mode: only pixels values Y >= ucMinY contribute to WB measurement */
                            /**< RGB Mode  : only pixels values G < MaxG contribute to WB measurement */
    uint8_t RefCb_MaxB;     /**< YCbCr Mode: Cb reference value */
                            /**< RGB Mode  : only pixels values B < MaxB contribute to WB measurement */
    uint8_t MaxCSum;        /**< YCbCr Mode: chrominance sum maximum value, only consider pixels with Cb+Cr smaller than threshold for WB measurements */
                            /**< RGB Mode  : unused */
    uint8_t MinC;           /**< YCbCr Mode: chrominance minimum value, only consider pixels with Cb/Cr each greater than threshold value for WB measurements */
                            /**< RGB Mode  : unused */
} CamerIcAwbMeasuringConfig_t;



/******************************************************************************/
/**
 * @brief   CamerIc AWB Module measurement values 
 *
 ******************************************************************************/
typedef struct CamerIcAwbMeasuringResult_s
{
    uint32_t    NoWhitePixel;           /**< number of white pixel */
    uint8_t     MeanY__G;               /**< Y/G  value in YCbCr/RGB Mode */
    uint8_t     MeanCb__B;              /**< Cb/B value in YCbCr/RGB Mode */
    uint8_t     MeanCr__R;              /**< Cr/R value in YCbCr/RGB Mode */
} CamerIcAwbMeasuringResult_t;



/*****************************************************************************/
/**
 * @brief   This function registers an event callback at CamerIc ISP AWB
 *          driver module.
 *
 * @param   handle          CamerIc driver handle 
 * @param   func            Callback function
 * @param   pUserContext    User-Context
 *
 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
);



/*****************************************************************************/
/**
 * @brief   This function deregisters/clears a registered event callback 
 *          at CamerIc ISP AWB driver module.
 *
 * @param   handle          CamerIc driver handle 
 *
 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function enables the CamerIc ISP AWB measuring module.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         ISP AWB module successfully enabled
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function disables the CamerIc ISP AWB measuring module 
 *          (bypassing of the white balance measuring module).
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         ISP AWB module successfully disabled
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Get CamerIc ISP AWB measuring module status.
 *
 * @param   handle          CamerIc driver handle.
 * @param   pIsEnabled
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This function configures the measuring mode
 *
 * @param   handle              CamerIc driver handle
 * @param   mode                measuring mode
 * @param   pMeasConfig         Configuration of the measuring trapezoid
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbSetMeasuringMode
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcIspAwbMeasuringMode_t  mode,
    const CamerIcAwbMeasuringConfig_t   *pMeasConfig
);



/*****************************************************************************/
/**
 * @brief   This function configures the CamerIC ISP AWB measuring window.
 *
 * @param   handle              CamerIc driver handle
 * @param   x                   start x position of measuring window
 * @param   y                   start y position of measuring window
 * @param   width               width of measuring window
 * @param   height              height of measuring window
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      x,
    const uint16_t      y,
    const uint16_t      width,
    const uint16_t      height
);


/*****************************************************************************/
/**
 * @brief   This functions reads out the currently configured white balance
 *          gains.
 *
 * @param   handle              CamerIc driver handle
 * @param   pGains              Pointer to store the white balance gains
 *                              (@ref CamerIcGains_t)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read WB gains
 *                              from CamerIC (maybe driver not initialized)
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbGetGains
(
    CamerIcDrvHandle_t  handle,
    CamerIcGains_t      *pGains
);



/*****************************************************************************/
/**
 * @brief   This functions sets the white balance gains.
 *
 * @param   handle              CamerIc driver handle
 * @param   pGains              Pointer to white balance gains to set
 *                              (@ref CamerIcGains_t)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read WB gains
 *                              from CamerIC (maybe driver not initialized)
 *
 *****************************************************************************/
extern RESULT CamerIcIspAwbSetGains
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pGains
);




#ifdef __cplusplus
}
#endif

/* @} cameric_isp_awb_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_AWB_DRV_API_H__ */

