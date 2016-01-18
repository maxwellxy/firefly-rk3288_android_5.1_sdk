
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
#ifndef __CAMERIC_ISP_AFM_DRV_API_H__
#define __CAMERIC_ISP_AFM_DRV_API_H__

/**
 * @cond    cameric_isp_afm
 *
 * @file    cameric_isp_afm_drv_api.h
 *
 * @brief   This file defines the CamerIC driver API for the CamerIC ISP 
 *          auto-focus measurement module (AFM).
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_afm_drv_api CamerIC ISP AFM driver API definitions
 * @{
 *
 * @image html cameric20MP_isp_afm.png "CamerIC ISP AFM driver" width=\textwidth
 * @image latex cameric20MP_isp_afm.png "CamerIC ISP AFM driver" width=\textwidth
 *
 * An auto-focus measurement block (ISP_AFM) is implemented to support
 * auto focus control. A substantial part of auto focus control will be done
 * in software: 
 * @arg The search algorithm which looks for maximum sharpness in the image
 * @arg and the movement of lens 
 *
 * The CamerIC auto-foucs measurement module measures the image sharpness 
 * in 3 windows selectable in size. An overlapping of these 3 windows is not allowed.
 *
 * The algorithm for the AF measurement is the Tenengrad function, which is based on a
 * standard edge detection mask. The Sobel horizontal and vertical operators Sx and Sy
 * are used to find the strength of the horizontal and vertical gradients in the pixel data
 * array.
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_afm_drv_api.h>



#ifdef __cplusplus
extern "C"
{
#endif



/******************************************************************************/
/**
 * @struct  CamerIcAfmMeasuringResult_s
 *
 * @brief   A structure to represent a complete set of measuring values.
 *
 *****************************************************************************/
typedef struct CamerIcAfmMeasuringResult_s
{
    uint32_t    SharpnessA;         /**< sharpness of window A */
    uint32_t    SharpnessB;         /**< sharpness of window B */
    uint32_t    SharpnessC;         /**< sharpness of window C */

    uint32_t    LuminanceA;         /**< luminance of window A */
    uint32_t    LuminanceB;         /**< luminance of window B */
    uint32_t    LuminanceC;         /**< luminance of window C */

    uint32_t    PixelCntA;
    uint32_t    PixelCntB;
    uint32_t    PixelCntC;

    CamerIcWindow_t   WindowA;      /* ddl@rock-chips.com: v1.6.0 */
    CamerIcWindow_t   WindowB;
    CamerIcWindow_t   WindowC;
    
} CamerIcAfmMeasuringResult_t;



/******************************************************************************/
/**
 * @enum    CamerIcIspAfmWindowId_e
 *
 * @brief   Enumeration type to identify the Autofocus measuring window.
 *
 *****************************************************************************/
typedef enum CamerIcIspAfmWindowId_e
{
    CAMERIC_ISP_AFM_WINDOW_INVALID  = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_AFM_WINDOW_A        = 1,    /**< Window A (1st window) */
    CAMERIC_ISP_AFM_WINDOW_B        = 2,    /**< Window B (2nd window) */
    CAMERIC_ISP_AFM_WINDOW_C        = 3,    /**< Window C (3rd window) */
    CAMERIC_ISP_AFM_WINDOW_MAX,             /**< upper border (only for an internal evaluation) */
} CamerIcIspAfmWindowId_t;



/*****************************************************************************/
/**
 * @brief   This functions registers an Event-Callback at CamerIC ISP AFM
 *          Module. An event callback is called if the driver needs to 
 *          inform the application layer about an asynchronous event or
 *          an error situation (i.e. please also @see CamerIcEventId_e).
 *
 * @param   handle              CamerIC driver handle 
 * @param   func                Callback function
 * @param   pUserContext        User-Context
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_BUSY            already a callback registered
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    a parameter is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to register a 
 *                              event callback (maybe the driver is already running)
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
);



/*****************************************************************************/
/**
 * @brief   This functions deregisters/releases a registered Event-Callback
 *          at CamerIc ISP AFM Module. 
 *
 * @param   handle              CamerIC driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to deregister the 
 *                              request callback
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions enables the CamerIC ISP AFM module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions disables the CamerIC ISP AFM module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Get CamerIC ISP AFM module status.
 *
 * @param   handle          CamerIc driver handle.
 * @param   pIsEnabled
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This functions sets the threshold in the CamerIC ISP AFM module.
 *
 * @param   handle              CamerIc driver handle
 * @param   threshold           threshold value
 *
 * @note    S(i,j) = Gx(i,j)^2 + Gy(i,j)^2 >= threshold 
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmSetThreshold
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      threshold
);
/*****************************************************************************/
/**
 * @brief   This function get the position and size of a a given measuring
 *          window in the CamerIC ISP AFM module.
 *
 * @param   handle              CamerIc driver handle
 * @param   WdwId               window identifier (@see CamerIcIspAfmWindowId_e)
 * @param   pWindow             position of measuring window
 *  
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_INVALID_PARM    invalid window identifier
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspAfmGetMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId,
    CamerIcWindow_t                 *pWindow
);

/*****************************************************************************/
/**
 * @brief   This function sets the position and size of a a given measuring
 *          window in the CamerIC ISP AFM module.
 *
 * @param   handle              CamerIc driver handle
 * @param   WdwId               window identifier (@see CamerIcIspAfmWindowId_e)
 * @param   x                   start x position of measuring window
 * @param   y                   start y position of measuring window
 * @param   width               width of measuring window
 * @param   height              height of measuring window
 *  
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_INVALID_PARM    invalid window identifier
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmSetMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId,
    const uint16_t                  x,
    const uint16_t                  y,
    const uint16_t                  width,
    const uint16_t                  height
);



/*****************************************************************************/
/**
 * @brief   This function enables the sharpness measuring of a given 
 *          measuring window in the CamerIC ISP AFM module.
 *
 * @param   handle              CamerIc driver handle
 * @param   WdwId               window identifier (@see CamerIcIspAfmWindowId_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmEnableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
);



/*****************************************************************************/
/**
 * @brief   This function returns BOOL_TRUE if the sharpness measuring of a 
 *          given measuring window in the CamerIC ISP AFM module enabled.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 *
 *****************************************************************************/
extern bool_t CamerIcIspAfmMeasuringWindowIsEnabled
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
);



/*****************************************************************************/
/**
 * @brief   This function disables the sharpness measuring of a given
 *          measuring window in the CamerIC ISP AFM module.
 *
 * @param   handle              CamerIc driver handle
 * @param   WdwId               window identifier (@see CamerIcIspAfmWindowId_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspAfmDisableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspAfmWindowId_t   WdwId
);



#ifdef __cplusplus
}
#endif

/* @} cameric_isp_afm_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_AFM_DRV_API_H__ */

