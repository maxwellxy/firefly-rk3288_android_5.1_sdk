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
#ifndef __CAMERIC_ISP_EXP_DRV_API_H__
#define __CAMERIC_ISP_EXP_DRV_API_H__

/**
 * @cond    cameric_isp_exp
 *
 * @file    cameric_isp_exp_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP AE Driver API definitions
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_exp_drv_api CamerIC ISP AE Driver API definitions
 * @{
 *
 * @image html cameric20MP_isp_exp.png "CamerIC ISP AE driver" width=\textwidth
 * @image latex cameric20MP_isp_exp.png "CamerIC ISP AE driver" width=\textwidth
 *
 * The exposure measurement module implements the measurement part of an autoexposure
 * feature. To obtain sufficient measurement values for a software control loop, the sensor array
 * needs to be divided into several areas, where a mean luminance value for each area is
 * calculated by hardware.
 *
 * @image html cameric20MP_isp_exp_grid.png "Luminance Measurement Grid" width=0.8\textwidth
 * @image latex cameric20MP_isp_exp_grid.png "Luminance Measurement Grid" width=0.8\textwidth
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif


#define CAMERIC_ISP_EXP_GRID_ITEMS          25  /**< number of grid items (see @ref CamerIcMeanLuma_t) */


#define CAMERIC_ISP_EXP_GRID_00              0  /**< array position grid item 00 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_10              1  /**< array position grid item 10 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_20              2  /**< array position grid item 20 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_30              3  /**< array position grid item 30 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_40              4  /**< array position grid item 40 (see @ref CamerIcMeanLuma_t) */
                                                                                
#define CAMERIC_ISP_EXP_GRID_01              5  /**< array position grid item 01 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_11              6  /**< array position grid item 11 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_21              7  /**< array position grid item 21 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_31              8  /**< array position grid item 31 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_41              9  /**< array position grid item 41 (see @ref CamerIcMeanLuma_t) */
                                                                                
#define CAMERIC_ISP_EXP_GRID_02             10  /**< array position grid item 02 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_12             11  /**< array position grid item 12 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_22             12  /**< array position grid item 22 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_32             13  /**< array position grid item 32 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_42             14  /**< array position grid item 42 (see @ref CamerIcMeanLuma_t) */
                                                                                
#define CAMERIC_ISP_EXP_GRID_03             15  /**< array position grid item 03 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_13             16  /**< array position grid item 13 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_23             17  /**< array position grid item 23 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_33             18  /**< array position grid item 33 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_43             19  /**< array position grid item 43 (see @ref CamerIcMeanLuma_t) */
                                                                                
#define CAMERIC_ISP_EXP_GRID_04             20  /**< array position grid item 04 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_14             21  /**< array position grid item 14 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_24             22  /**< array position grid item 24 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_34             23  /**< array position grid item 34 (see @ref CamerIcMeanLuma_t) */
#define CAMERIC_ISP_EXP_GRID_44             24  /**< array position grid item 44 (see @ref CamerIcMeanLuma_t) */



/*****************************************************************************/
/**
 * @brief   Enumeration type to configure CamerIC ISP exposure measuring mode.
 *
 *****************************************************************************/
typedef enum CamerIcIspExpMeasuringMode_e
{
    CAMERIC_ISP_EXP_MEASURING_MODE_INVALID    = 0,    /**< invalid histogram measuring mode   */
    CAMERIC_ISP_EXP_MEASURING_MODE_1          = 1,    /**< Y = (R + G + B) x (85/256)         */
    CAMERIC_ISP_EXP_MEASURING_MODE_2          = 2,    /**< Y = 16 + 0.25R + 0.5G + 0.1094B    */
    CAMERIC_ISP_EXP_MEASURING_MODE_MAX,    
} CamerIcIspExpMeasuringMode_t;



/*****************************************************************************/
/**
 * @brief   Array type for ISP EXP measurment values.
 *
 *****************************************************************************/
typedef uint8_t CamerIcMeanLuma_t[CAMERIC_ISP_EXP_GRID_ITEMS];



/*****************************************************************************/
/**
 * @brief   This functions registers an Event-Callback at CamerIC ISP histogram
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
extern RESULT CamerIcIspExpRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
);



/*****************************************************************************/
/**
 * @brief   This functions deregisters/releases a registered Event-Callback
 *          at CamerIc ISP histogram measurement module. 
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
extern RESULT CamerIcIspExpDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions enables the CamerIC ISP exposure measurement
 *          module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspExpEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions disables the CamerIC ISP exposure measurement
 *          module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspExpDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Get CamerIC ISP exposure measurement module status.
 *
 * @param   handle          CamerIc driver handle.
 * @param   pIsEnabled
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspExpIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This functions configures the measurement mode of the CamerIC 
 *          ISP exposure measurement module.
 *
 * @param   handle              CamerIc driver handle
 * @param   mode                measurement mode (@see CamerIcIspHistMode_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspExpSetMeasuringMode
(
    CamerIcDrvHandle_t  		        handle,
    const CamerIcIspExpMeasuringMode_t  mode	
);


/******************************************************************************
 * CamerIcIspExpGetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspExpGetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pWindow,
    CamerIcWindow_t   *pGrid
);
/*****************************************************************************/
/**
 * @brief   This function sets the position and size of a the measurement
 *          window in the CamerIC ISP exposure measurement module.
 *
 * @note    This function calculates the grid-size by dividing the width and
 *          height by 5.
 *
 * @param   handle              CamerIc driver handle
 * @param   x                   start x position of measuring window
 * @param   y                   start y position of measuring window
 * @param   width               width of measuring window
 * @param   height              height of measuring window
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_INVALID_PARM    invalid window identifier
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIspExpSetMeasuringWindow
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      x,
    const uint16_t      y,
    const uint16_t      width,
    const uint16_t      height
);



#ifdef __cplusplus
}
#endif

/* @} cameric_isp_exp_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_EXP_DRV_API_H__ */

