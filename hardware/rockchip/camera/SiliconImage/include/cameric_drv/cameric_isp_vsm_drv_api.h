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
#ifndef __CAMERIC_ISP_VSM_DRV_API_H__
#define __CAMERIC_ISP_VSM_DRV_API_H__

/**
 * @cond    cameric_isp_vsm
 *
 * @file    cameric_isp_vsm_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP VSM (video stabilization) Driver
 *          API definitions
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_vsm_drv_api CamerIC ISP VSM Driver API definitions
 * @{
 *
 * The video stabilization measurement is done in two steps.
 * At first the vector accumulation unit calculates one line and one column projection vector
 * for each frame which is the accumulated sum of all row pixels respectively column pixels.
 * Two versions of these vectors are stored in SRAM buffers, one from the current frame
 * and one from the previous frame.
 *
 * In the second step the correlation units analyze the correlation between these vectors,
 * to find the global minimum of the correlation function in horizontal and vertical direction.
 * The correlation function is the sum of all absolute differences between the current and
 * the old stored vector with an index offset of delta. The point, where the correlation value
 * reaches its minimum, indicates the displacement value.
 *
 * To program the measure window consider the following restrictions: \n
 *
 * @arg vsm_h_size is limited from 64 to 1920, but must not exceed isp_h_size - vsm_h_offs.
 * @arg vsm_v_size is limited from 64 to 1088, but must not exceed isp_v_size - vsm_v_offs.
 * @arg vsm_h_size and vsm_v_size must be even values (bit 0 is fixed to zero).
 * @arg A border of at least 2 pixels around the measure window is recommended to
 * exclude interpolation artifacts near the edges of the input frame.
 * @arg For processing requirements the absolute max value for vsm_X_segments is
 * ( vsm_X_size - 48 ) / 16, (X either h or v).
 * @arg To prevent detection errors the delta range should be limited to the necessary
 * range. The recommended max horizontal/vertical segments value should not
 * exceed Size / 64.
 *
 * @image html cameric20MP_isp_vsm_window.png "Measure Window" width=0.75\textwidth
 * @image latex cameric20MP_isp_vsm_window.png "Measure Window" width=0.75\textwidth
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
 * @brief   CamerIc ISP video stabilization measurement displacement vector.
 *
 *****************************************************************************/
typedef struct CamerIcIspVsmDisplVec_s
{
    int16_t  delta_h;    /**< horizontal displacement */
    int16_t  delta_v;    /**< vertical displacement */
} CamerIcIspVsmDisplVec_t;



/*****************************************************************************/
/**
 * @brief   Data to be passed in event callback.
 *
 *****************************************************************************/
typedef struct CamerIcIspVsmEventData_s
{
    ulong_t                frameId;  /**< ID of the frame to which the measured
                                           displacement vector belongs. */
    CamerIcIspVsmDisplVec_t DisplVec; /**< latest calculated displacement vector */
} CamerIcIspVsmEventData_t;



/*****************************************************************************/
/**
 * @brief   This functions registers an Event-Callback at CamerIC ISP VSM
 *          measurement module. An event callback is called if the driver needs
 *          to inform the application layer about an asynchronous event or an
 *          error situation (i.e. please also @see CamerIcEventId_e).
 *
 * @param   handle              CamerIc driver handle 
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
 *                              event callback (maybe the driver is already 
 *                              running)
 *
 *****************************************************************************/
extern RESULT CamerIcIspVsmRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
);



/*****************************************************************************/
/**
 * @brief   This functions deregisters/releases a registered Event-Callback
 *          at CamerIc ISP VSM measurement module.
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
extern RESULT CamerIcIspVsmDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions enables the CamerIC ISP video stabilization
 *          measurement module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspVsmEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions disables the CamerIC ISP video stabilization
 *          measurement module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspVsmDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Get CamerIC ISP video stabilization measurement module status.
 *
 * @param   handle          CamerIc driver handle.
 * @param   pIsEnabled
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspVsmIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This function sets the position and size of a the measurement
 *          window in the CamerIC ISP video stabilization measurement module.
 *
 * @param   handle              CamerIc driver handle
 * @param   pMeasureWin         Pointer to measuring window parameters
 * @param   horSegments         Number of horizontal 16 point segments used
 *                              for first step of correlation function.
 *                              If h_segments is even, horizontal displacements
 *                              checked in first step are [-8*h_segments,
 *                              -8*h_segments + 16, ..., -16, 0, +16, ...,
 *                              +8*h_segments - 16, +8*h_segments],
 *                              If h_segments is odd, horizontal displacements
 *                              checked in first step are [-8*h_segments,
 *                              -8*h_segments + 16, ..., -8, +8, ...,
 *                              +8*h_segments - 16, +8*h_segments].
 * @param   verSegments         Number of vertical 16 point segments used
 *                              for first step of correlation function,
 *                              analogous to h_segments.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_INVALID_PARM    invalid window identifier
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    NULL pointer passed
 * @retval  RET_WRONG_CONFIG    Configuration is invalid
 * @retval  RET_OUTOFRANGE      Configuration parameters are out of range
 *
 *****************************************************************************/
extern RESULT CamerIcIspVsmSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pMeasureWin,
    uint8_t             horSegments,
    uint8_t             verSegments
);



/*****************************************************************************/
/**
 * @brief   This function retrieves the currently configured
 *          measuring window.
 *
 * @param   handle              CamerIc driver handle
 * @param   pMeasureWin         Pointer to configured measuring window
 *                              parameters if function returns RET_SUCCESS.
 * @param   pHorSegments        Pointer to configured number of horizontal 16
 *                              point segments used for first step of
 *                              correlation function, if function returns
 *                              RET_SUCCESS.
 * @param   verSegments         Pointer to configured number of horizontal 16
 *                              point segments used for first step of
 *                              correlation function, if function returns
 *                              RET_SUCCESS.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_INVALID_PARM    invalid window identifier
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    NULL pointer passed
 *
 *****************************************************************************/
extern RESULT CamerIcIspVsmGetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pMeasureWin,
    uint8_t            *pHorSegments,
    uint8_t            *pVerSegments
);



#ifdef __cplusplus
}
#endif

/* @} cameric_isp_vsm_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_VSM_DRV_API_H__ */

