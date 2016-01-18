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
#ifndef __CAMERIC_MI_DRV_API_H__
#define __CAMERIC_MI_DRV_API_H__

/**
 * @file cameric_mi_drv_api.h
 *
 * @brief   This file contains the CamerIC MI driver API definitions.
 *
 *****************************************************************************/
/**
 * @cond cameric_mi
 *
 * @defgroup cameric_mi_drv_api CamerIc MI Driver API definitions
 * @{
 *
 * @image html cameric20MP_mi.png "CamerIC MI driver" width=\textwidth
 * @image latex cameric20MP_mi.png "CamerIC MI driver" width=\textwidth
 *
 * The Memory Interface module is able to write data from the main picture path
 * to the system memory. For the main picture path alternatively one of raw data,
 * main picture path resize, or JPEG encoder output paths could be used. The MI
 * module is also able to write data from the self picture path to the system memory.
 * Further on the DMA read port path could be used to read an image from the system
 * memory, e.g. as input for the Super Impose (SI) module. The Memory Interface Driver
 * serves as an abstraction layer, so the application does not need to know which bit
 * has to be set where in the registers of the MI module.
 *
 * The Memory interface module driver also abstracts the scaler-units in the
 * main and self path (see MRSZ or SRSZ).
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CAMERIC_ALIGN_SIZE          ALIGN_SIZE_1K
/* ddl@rock-chips.com: v1.2.0 */
#define CAMERIC_MI_ALIGN(addr)		( ALIGN_UP(addr, (ctx->pMiContext->c_burstlength*8)) )

/*****************************************************************************/
/**
 * @brief   Enumeration type to identify the CamerIC output path
 *
 *****************************************************************************/
typedef enum CamerIcMiPath_e
{
    CAMERIC_MI_PATH_INVALID         = -1,       /**< lower border (only for an internal evaluation) */
    CAMERIC_MI_PATH_MAIN            =  0,       /**< main path index */
    CAMERIC_MI_PATH_SELF            =  1,       /**< self path index */
    CAMERIC_MI_PATH_MAX                         /**< upper border (only for an internal evaluation) */
} CamerIcMiPath_t;


/*****************************************************************************/
/**
 * @brief   Enumeration type to specify the data output mode on a given path
 *
 * @note    This enumeration type is used to specify the organization of the image data
 *          for each pixel, whether in planar format or packed format
 *
 *****************************************************************************/
typedef enum CamerIcMiDataMode_e
{
    CAMERIC_MI_DATAMODE_INVALID         = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_MI_DATAMODE_DISABLED        = 1,    /**< disables the path */
    CAMERIC_MI_DATAMODE_JPEG            = 2,    /**< data output format is JPEG (only valid for mainpath @ref CamerIcMiPath_e) */
    CAMERIC_MI_DATAMODE_YUV444          = 3,    /**< data output format is YUV444 */
    CAMERIC_MI_DATAMODE_YUV422          = 4,    /**< data output format is YUV422 */
    CAMERIC_MI_DATAMODE_YUV420          = 5,    /**< data output format is YUV420 */
    CAMERIC_MI_DATAMODE_YUV400          = 6,    /**< data output format is YUV400 */
    CAMERIC_MI_DATAMODE_RGB888          = 7,    /**< data output format is RGB888 (only valid for selpath @ref CamerIcMiPath_e) */
    CAMERIC_MI_DATAMODE_RGB666          = 8,    /**< data output format is RGB666 (only valid for selpath @ref CamerIcMiPath_e) */
    CAMERIC_MI_DATAMODE_RGB565          = 9,    /**< data output format is RGB565 (only valid for selpath @ref CamerIcMiPath_e) */
    CAMERIC_MI_DATAMODE_RAW8            = 10,   /**< data output format is RAW8 (only valid for mainpath @ref CamerIcMiPath_e) */
    CAMERIC_MI_DATAMODE_RAW12           = 11,   /**< data output format is RAW12 (only valid for mainpath @ref CamerIcMiPath_e) */
    CAMERIC_MI_DATAMODE_DPCC            = 12,   /**< path dumps out the current measured defect pixel table */
    CAMERIC_MI_DATAMODE_MAX                     /**< upper border (only for an internal evaluation) */
} CamerIcMiDataMode_t;


/*****************************************************************************/
/**
 * @brief   Enumeration type to specify the data storage layout
 *
 * @note    This enumeration type is used to specify the organization of the
 *          image data for each pixel, whether in planar format or packed
 *          format. The valid domain is bordered from
 *          CAMERIC_MI_DATASTORAGE_INVALID to CAMERIC_MI_DATASTORAGE_MAX.
 *
 *****************************************************************************/
typedef enum CamerIcMiDataLayout_e
{
    CAMERIC_MI_DATASTORAGE_INVALID      = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_MI_DATASTORAGE_PLANAR       = 1,    /**< Y values for all pixels are put together, as well as U and V,
                                                     like: YYYYYY......, UUUUUUU......., VVVVVV...... */
    CAMERIC_MI_DATASTORAGE_SEMIPLANAR   = 2,    /**< YUV values are packed together as: YYYY......, UVUVUVUV...... */
    CAMERIC_MI_DATASTORAGE_INTERLEAVED  = 3,    /**< YUV values are packed together as: YUV, YUV, YUV, ...... */
    CAMERIC_MI_DATASTORAGE_MAX                  /**< upper border (only for an internal evaluation) */
} CamerIcMiDataLayout_t;


/*****************************************************************************/
/**
 * @brief   Enumeration type to specify the burst length for writing to or
 *          reading from memory
 *
 *****************************************************************************/
typedef enum CamerIcMiBurstLength_e
{
    CAMERIC_MI_BURSTLENGTH_INVALID		= 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_MI_BURSTLENGTH_4            = 1,    /**< burstlength of 4 or smaller */
    CAMERIC_MI_BURSTLENGTH_8            = 2,    /**< burstlength of 8 or smaller */
    CAMERIC_MI_BURSTLENGTH_16           = 3,    /**< burstlength of 16 or smaller */
    CAMERIC_MI_BURSTLENGTH_MAX                  /**< upper border (only for an internal evaluation) */
} CamerIcMiBurstLength_t;


/*****************************************************************************/
/**
 * @brief   Enumeration type to specify the picture orientation on the self
 *          path
 *
 *****************************************************************************/
//! self picture operating modes
typedef enum  CamerIcMiOrientation_e
{
    CAMERIC_MI_ORIENTATION_INVALID          = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_MI_ORIENTATION_ORIGINAL         = 1,    /**< no rotation, no horizontal or vertical flipping */
    CAMERIC_MI_ORIENTATION_VERTICAL_FLIP    = 2,    /**< vertical   flipping (no additional rotation) */
    CAMERIC_MI_ORIENTATION_HORIZONTAL_FLIP  = 3,    /**< horizontal flipping (no additional rotation) */
    CAMERIC_MI_ORIENTATION_ROTATE90         = 4,    /**< rotation  90 degrees ccw (no additional flipping) */
    CAMERIC_MI_ORIENTATION_ROTATE180        = 5,    /**< rotation 180 degrees ccw (equal to horizontal plus vertical flipping) */
    CAMERIC_MI_ORIENTATION_ROTATE270        = 6,    /**< rotation 270 degrees ccw (no additional flipping) */
    CAMERIC_MI_ORIENTATION_MAX                      /**< upper border (only for an internal evaluation) */
} CamerIcMiOrientation_t;


/*****************************************************************************/
/**
 * @brief   This function registers a Request-Callback at the CamerIC Memory
 *          Interface Module. A request callback is called if the driver
 *          needs an interaction from the application layer (i.e. a new image
 *          buffer to fill, please also see @ref CamerIcRequestId_e).
 *
 * @param   handle              CamerIc driver handle
 * @param   func                Callback function
 * @param   pUserContext        User-Context
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_BUSY            already a callback registered
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_INVALID_PARM    given parameter is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to register a
 *                              request callback
 *
 *****************************************************************************/
extern RESULT CamerIcMiRegisterRequestCb
(
    CamerIcDrvHandle_t      handle,
    CamerIcRequestFunc_t    func,
    void 			        *pUserContext
);


/*****************************************************************************/
/**
 * @brief   This functions deregisters/releases a registered Request-Callback
 *          at CamerIC Memory Interface Module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to deregister the
 *                              request callback
 *
 *****************************************************************************/
extern RESULT CamerIcMiDeRegisterRequestCb
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This functions registers an Event-Callback at CamerIC Memory
 *          Interface Module. An event callback is called if the driver
 *          needs to inform the application layer about an asynchronous event
 *          or an error situation (i.e. please also see @ref CamerIcEventId_e).
 *
 * @param   handle              CamerIc driver handle
 * @param   func                Callback function
 * @param   pUserContext        User-Context
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_BUSY            already a callback registered
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_INVALID_PARM    given parameter is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to register a
 *                              event callback
 *
 *****************************************************************************/
extern RESULT CamerIcMiRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void 			    *pUserContext
);


/*****************************************************************************/
/**
 * @brief   This functions deregisters/releases a registered Event-Callback
 *          at CamerIC Memory Interface Module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to deregister the
 *                              event callback
 *
 *****************************************************************************/
extern RESULT CamerIcMiDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This functions configures the burstlength to use for luminance
 *          and chrominance planes.
 *
 * @note    It could be more efficient to configure different burstlength
 *          depending on the pixel subsampling (YUV422, YUV400, ... )
 *
 * @param   handle              CamerIc driver handle
 * @param   y_burstlength       burstlegth of luminace plane
 * @param   c_burstlength       burstlegth of chrominace plane
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to set burstlength
 *
 *****************************************************************************/
extern RESULT CamerIcMiSetBurstLength
(
    CamerIcDrvHandle_t              handle,
    const CamerIcMiBurstLength_t    y_burstlength,
    const CamerIcMiBurstLength_t    c_burstlength
);


/*****************************************************************************/
/**
 * @brief   This function configures the input and output resolution of
 *          a given path.
 *
 * @param   handle              CamerIc driver handle
 * @param   path                Path index of CamerIC (@ref CamerIcMiPath_e)
 * @param   in_width            input width
 * @param   in_height           input height
 * @param   out_width           output width
 * @param   out_height          output height
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to set burstlength
 *
 *****************************************************************************/
extern RESULT CamerIcMiSetResolution
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMiPath_t       path,
    const uint32_t              in_width,
    const uint32_t              in_height,
    const uint32_t              out_width,
    const uint32_t              out_height
);


/*****************************************************************************/
/**
 * @brief   This function configures the data mode for the given path.
 *
 * @param   handle              CamerIc driver handle
 * @param   path                Path index of CamerIC (@ref CamerIcMiPath_e)
 * @param   mode                Data output mode (@ref CamerIcMiDataMode_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change datamode
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcMiSetDataMode
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMiPath_t       path,
    const CamerIcMiDataMode_t   mode
);


/*****************************************************************************/
/**
 * @brief   This function configures the data layout for the given path.
 *
 * @param   handle              CamerIc driver handle
 * @param   path                Path index of CamerIC (@ref CamerIcMiPath_e)
 * @param   layout              Data layout (@ref CamerIcMiDataLayout_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change datamode
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcMiSetDataLayout
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMiPath_t       path,
    const CamerIcMiDataLayout_t layout
);


/*****************************************************************************/
/**
 * @brief   This function swap the color channels.
 *
 * @note    Normal order is U than V byte, which works for image formats
 *          like N21, ... But there are also some image formats available 
 *          which using a swapped byte order like NV12, ... .
 *          This function enables or disable color channel swapping to 
 *          support all available image formats.
 *
 * @param   handle              CamerIc driver handle
 * @param   path                Path index of CamerIC (@ref CamerIcMiPath_e)
 * @param   swap                enable/disable swapping color channels 
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change datamode
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcMiSwapColorChannel
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMiPath_t       path,
    const bool_t                swap
);


/*****************************************************************************/
/**
 * @brief   This function checks if the picture orientation is allowed in 
 *          currently running mode. 
 *
 * @param   handle              CamerIc driver handle
 * @param   path                Path index of CamerIC (@ref CamerIcMiPath_e)
 * @param   orientation         Picture orientation (@ref CamerIcMiOrientation_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change datamode
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcMiIsPictureOrientationAllowed
(
    CamerIcDrvHandle_t              handle,
    const CamerIcMiPath_t           path,
    const CamerIcMiOrientation_t    orientation
);


/*****************************************************************************/
/**
 * @brief   This function configures the picture orientation for the
 *          Self path.
 *
 * @param   handle              CamerIc driver handle
 * @param   path                Path index of CamerIC (@ref CamerIcMiPath_e)
 * @param   orientation         Picture orientation (@ref CamerIcMiDataLayout_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change datamode
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcMiSetPictureOrientation
(
    CamerIcDrvHandle_t              handle,
    const CamerIcMiPath_t           path,
    const CamerIcMiOrientation_t    orientation
);


/*****************************************************************************/
/**
 * @brief   This function configures the picture orientation for the
 *          Self path.
 *
 * @param   handle              CamerIc driver handle
 * @param   numFramesToSkip     Number of frames to skip (e.g. after sensor resolution change)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    given handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change datamode
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcMiSetFramesToSkip
(
    CamerIcDrvHandle_t              handle,
    uint32_t                        numFramesToSkip
);

#ifdef __cplusplus
}
#endif

/* @} cameric_mi_drv_api */

/* @endcond */


#endif /* __CAMERIC_MI_DRV_API_H__ */

