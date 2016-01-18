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
#ifndef __CAMERIC_ISP_ELAWB_DRV_API_H__
#define __CAMERIC_ISP_ELAWB_DRV_API_H__

/**
 * @cond    cameric_isp_elawb
 *
 * @file    cameric_isp_elawb_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP Elliptic AWB driver API definitions.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_elawb_drv_api CamerIC ISP Elliptic AWB driver API definitions
 * @{
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
 * @brief   Enumeration type to identify a measuring ellipsis
 *
 *****************************************************************************/
typedef enum CamerIcIspElAwbElipseId_e
{
    CAMERIC_ISP_AWB_ELIPSIS_ID_INVALID = 0,     /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_AWB_ELIPSIS_ID_1       = 1,     /**< ellipse #1 */
    CAMERIC_ISP_AWB_ELIPSIS_ID_2       = 2,     /**< ellipse #2 */
    CAMERIC_ISP_AWB_ELIPSIS_ID_3       = 3,     /**< ellipse #3 */
    CAMERIC_ISP_AWB_ELIPSIS_ID_4       = 4,     /**< ellipse #4 */
    CAMERIC_ISP_AWB_ELIPSIS_ID_5       = 5,     /**< ellipse #5 */
    CAMERIC_ISP_AWB_ELIPSIS_ID_6       = 6,     /**< ellipse #6 */
    CAMERIC_ISP_AWB_ELIPSIS_ID_7       = 7,     /**< ellipse #7 */
    CAMERIC_ISP_AWB_ELIPSIS_ID_8       = 8,     /**< ellipse #8 */
    CAMERIC_ISP_AWB_ELIPSIS_ID_MAX,             /**< upper border (only for an internal evaluation) */
} CamerIcIspElAwbElipseId_t;




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
extern RESULT CamerIcIspElAwbRegisterEventCb
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
extern RESULT CamerIcIspElAwbDeRegisterEventCb
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
extern RESULT CamerIcIspElAwbEnable
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
extern RESULT CamerIcIspElAwbDisable
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
extern RESULT CamerIcIspElAwbIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
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
extern RESULT CamerIcIspElAwbSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      x,
    const uint16_t      y,
    const uint16_t      width,
    const uint16_t      height
);



/*****************************************************************************/
/**
 * @brief   This function returns a currently configured measuring ellisiod
 *          from CamerIC ISP AWB measuring module.
 *
 * @param   handle              CamerIc driver handle
 * @param   x                   pointer to store center x position of elipsis
 * @param   y                   pointer to store center y position of elipsis
 * @param   a1                  pointer to store rotaion matrix coefficient (1,1)
 * @param   a2                  pointer to store rotaion matrix coefficient (1,2)
 * @param   a3                  pointer to store rotaion matrix coefficient (2,1)
 * @param   a4                  pointer to store rotaion matrix coefficient (2,2)
 * @param   r_max_sqr           pointer to store max radius square compare value               
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 *
 *****************************************************************************/
extern RESULT CamerIcIspElAwbGetMeasuringEllipse
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspElAwbElipseId_t id,
    uint16_t                        *x,
    uint16_t                        *y,
    uint16_t                        *a1,
    uint16_t                        *a2,
    uint16_t                        *a3,
    uint16_t                        *a4,
    uint32_t                        *r_max_sqr
);



/*****************************************************************************/
/**
 * @brief   This function configures a measuring ellisiod in CamerIC ISP 
 *          AWB measuring module.
 *
 * @param   handle              CamerIc driver handle
 * @param   x                   center x position of elipsis
 * @param   y                   center y position of elipsis
 * @param   a1                  rotaion matrix coefficient (1,1)
 * @param   a2                  rotaion matrix coefficient (1,2)
 * @param   a3                  rotaion matrix coefficient (2,1)
 * @param   a4                  rotaion matrix coefficient (2,2)
 * @param   r_max_sqr           max radius square compare value               
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 *
 *****************************************************************************/
extern RESULT CamerIcIspElAwbSetMeasuringEllipse
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspElAwbElipseId_t id,
    const uint16_t                  x,
    const uint16_t                  y,
    const uint16_t                  a1,
    const uint16_t                  a2,
    const uint16_t                  a3,
    const uint16_t                  a4,
    const uint32_t                  r_max_sqr
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
extern RESULT CamerIcIspElAwbGetGains
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
extern RESULT CamerIcIspElAwbSetGains
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pGains
);




#ifdef __cplusplus
}
#endif

/* @} cameric_isp_elawb_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_ELAWB_DRV_API_H__ */

