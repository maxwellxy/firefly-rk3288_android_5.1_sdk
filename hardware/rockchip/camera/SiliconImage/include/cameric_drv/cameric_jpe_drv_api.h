/******************************************************************************
 *
 * Copyright 2011, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
#ifndef __CAMERIC_JPE_DRV_API_H__
#define __CAMERIC_JPE_DRV_API_H__

/**
 * @cond    cameric_jpe
 *
 * @file    cameric_jpe_drv_api.h
 *
 * @brief   This file contains the CamerIC JPE driver API definitions.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_jpe_drv_api CamerIc JPE driver API definitions
 * @{
 *
 * @image html cameric20MP_jpe.png "CamerIC JPE driver" width=\textwidth
 * @image latex cameric20MP_jpe.png "CamerIC JPE driver" width=\textwidth
 *
 * The JPEG encoder is responsible for still image compression. It operates widely
 * independent from the system controller. This one only has to configure the encoder and
 * to start the encoding process. So the basic flow is:
 *
 * @arg Configuring
 * @arg JPEG header generation
 * @arg Image data encoding
 *
 * The encoding process starts with a raster to block conversion of the YUV 4:2:2 pixel
 * data provided e.g. by an imaging device. The incoming line oriented pixel data is
 * reordered into 8x8 pixel blocks, one for each component.
 *
 * Every 8x8 pixel block undergoes a baseline DCT computation, a zigzag reordering, a
 * quantization and a variable length encoding (Huffman based). Last encoding step is
 * the generation of the JFIF 1.02 compliant data stream by inserting markers and tables.
 *
 * @image html cameric20MP_jpe_block_diagram.png "Block Diagram of the JPEG Encoder Sub Module" width=0.75\textwidth
 * @image latex cameric20MP_jpe_block_diagram.png "Block Diagram of the JPEG Encoder Sub Module" width=0.75\textwidth
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif



/******************************************************************************/
/**
 * @brief   Enumeration type to configure CamerIC JPE working mode.
 *
 *****************************************************************************/
typedef enum CamerIcJpeMode_e
{
    CAMERIC_JPE_MODE_INVALID            = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_JPE_MODE_SINGLE_SHOT        = 1,    /**< only one frame */
    CAMERIC_JPE_MODE_SHORT_CONTINUOUS   = 2,    /**< motion jpeg ( only first frame with header ) */
    CAMERIC_JPE_MODE_LARGE_CONTINUOUS   = 3,    /**< motion jpeg ( every frame with header ) */
    CAMERIC_JPE_MODE_SCALADO            = 4,    /**< single snapshot with Scalado encoding */
    CAMERIC_JPE_MODE_MAX                        /**< upper border (only for an internal evaluation) */
} CamerIcJpeMode_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure CamerIC JPE luminance input scaling.
 *
 *****************************************************************************/
typedef enum CamerIcJpeLuminanceScale_e
{
    CAMERIC_JPE_LUMINANCE_SCALE_INVALID = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_JPE_LUMINANCE_SCALE_ENABLE  = 1,    /**< scaling Y input from [16..235] to [0..255] */
    CAMERIC_JPE_LUMINANCE_SCALE_DISABLE = 2,    /**< no Y input scaling */
    CAMERIC_JPE_LUMINANCE_SCALE_MAX             /**< upper border (only for an internal evaluation) */
} CamerIcJpeLuminanceScale_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure CamerIC JPE chrominance input scaling.
 *
 *****************************************************************************/
typedef enum CamerIcJpeChrominaceScale_e
{
    CAMERIC_JPE_CHROMINANCE_SCALE_INVALID = 0,  /**< lower border (only for an internal evaluation) */
    CAMERIC_JPE_CHROMINANCE_SCALE_ENABLE  = 1,  /**< scaling Cb/Cr input from [16..235] to [0..255] */
    CAMERIC_JPE_CHROMINANCE_SCALE_DISABLE = 2,  /**< no Cb/Cr input scaling */
    CAMERIC_JPE_CHROMINANCE_SCALE_MAX           /**< upper border (only for an internal evaluation) */
} CamerIcJpeChrominaceScale_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure CamerIC JPE compression level
 *
 *****************************************************************************/
typedef enum CamerIcJpeCompressionLevel_e
{
    CAMERIC_JPE_COMPRESSION_LEVEL_INVALID       = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_JPE_COMPRESSION_LEVEL_HIGH          = 1,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_LOW           = 2,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_01_PRECENT    = 3,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_20_PERCENT    = 4,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_30_PERCENT    = 5,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_40_PERCENT    = 6,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_50_PERCENT    = 7,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_60_PERCENT    = 8,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_70_PERCENT    = 9,    /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_80_PERCENT    = 10,   /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_90_PERCENT    = 11,   /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_99_PERCENT    = 12,   /**< */
    CAMERIC_JPE_COMPRESSION_LEVEL_MAX                   /**< upper border (only for an internal evaluation) */
} CamerIcJpeCompressionLevel_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure CamerIC JPE header 
 *
 *****************************************************************************/
typedef enum CamerIcJpeHeader_e
{
    CAMERIC_JPE_HEADER_INVALID          = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_JPE_HEADER_NONE             = 1,
    CAMERIC_JPE_HEADER_JFIF             = 2,
    CAMERIC_JPE_HEADER_MAX                      /**< upper border (only for an internal evaluation) */
} CamerIcJpeHeader_t;




/******************************************************************************/
/**
 * @brief   Enumeration type to configure CamerIC JPE compression level
 *
 *****************************************************************************/
typedef struct CamerIcJpeConfig_s
{
    CamerIcJpeMode_t                mode;       /**< encoder mode */
    CamerIcJpeCompressionLevel_t    level;      /**< compression level */

    CamerIcJpeLuminanceScale_t      yscale;     /**< luminace upscale mode (BT601 -> full range) */
    CamerIcJpeChrominaceScale_t     cscale;     /**< chrominance upscale mode (BT601 -> full range) */

    uint16_t                        width;      /**< frame width */
    uint16_t                        height;     /**< frame height */ 
} CamerIcJpeConfig_t;



/*****************************************************************************/
/**
 * @brief   This functions enables the CamerIC JPE module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcJpeEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions disables the CamerIC JPE module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcJpeDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function returns the status of the CamerIC JPE module.
 *
 * @param   handle              CamerIc driver handle
 * @param   pIsEnabled          Pointer to value to store current state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    pIsEnabled is a NULL pointer
 *
 *****************************************************************************/
extern RESULT CamerIcJpeIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This function configures the JPE module.
 *
 * @param   handle              CamerIC driver handle
 * @param   pConfig             pointer to jpe configuration structure
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NOTSUPP         selected working mode is not supported
 * @retval  RET_NULL_POINTER    null pointer
 * @retval  RET_BUSY            image effects already enabled
 *
 *****************************************************************************/
extern RESULT CamerIcJpeConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcJpeConfig_t      *pConfig
);



/*****************************************************************************/
/**
 * @brief   This functions registers an Event-Callback at CamerIC Jpeg
 *          Encoder Module. An event callback is called if the driver
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
extern RESULT CamerIcJpeRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
);



/*****************************************************************************/
/**
 * @brief   This functions deregisters/releases a registered Event-Callback
 *          at CamerIC Jpeg Encoder Interface Module. 
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
extern RESULT CamerIcJpeDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function starts the JPEG header generation
 *
 * @param   handle              CamerIc driver handle
 * @param   hmode               mode (see @ref CamerIcJpeMode_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcJpeStartHeaderGeneration
(
    CamerIcDrvHandle_t          handle,
    const CamerIcJpeHeader_t    header
);



/*****************************************************************************/
/**
 * @brief   This function starts CamerIC JPE data encoding.
 *
 * @param   handle              CamerIc driver handle
 * @param   mode                mode (see @ref CamerIcJpeMode_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range 
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcJpeStartEncoding
(
    CamerIcDrvHandle_t      handle,
    const CamerIcJpeMode_t  mode 
);



#ifdef __cplusplus
}
#endif

/* @} cameric_jpe_drv_api */

/* @endcond */

#endif /* __CAMERIC_JPE_DRV_API_H__ */

