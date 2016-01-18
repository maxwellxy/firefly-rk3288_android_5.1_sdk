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
#ifndef __CAMERIC_ISP_CPROC_DRV_API_H__
#define __CAMERIC_ISP_CPROC_DRV_API_H__

/**
 * @cond    cameric_cproc
 *
 * @file    cameric_cproc_drv_api.h
 *
 * @brief   This file contains the CamerIC CPROC driver API definitions.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_cproc_drv_api CamerIC CPROC Driver API definitions
 * @{
 *
 * @if CAMERIC_LITE
 *
 * @image html cproc_lite.png "Overview of the CamerIC MI driver" width=\textwidth
 * @image latex cproc_lite.png "Overview of the CamerIC MI driver" width=\textwidth
 *
 * @endif
 *
 * @if CAMERIC_FULL
 *
 * @image html cameric20MP_cproc.png "CamerIC CPROC driver" width=\textwidth
 * @image latex cameric20MP_cproc.png "CamerIC CPROC driver" width=\textwidth
 *
 * @endif
 *
 * The CamerIC color processing module is responsible for color manipulation
 * of the incoming data stream in terms of hue, brightness, contrast and
 * saturation. The pixel luminance is modified to manipulate contrast and
 * brightness while the chrominance values is modified for hue and saturation
 * contrast.
 *
 * Color processing is done in terms of luminance and chrominance adjustment.
 *
 * @arg Contrast processing is done by multiplying the luminance value by the
 * contrast adjustment value defined in CPROC_CONTRAST which is in the range 
 * of 0.0 (0x00) and 1.992 (0xFF).
 *
 * @arg Brightness is adjusted by adding a value defined in CPROC_BRIGHTNESS
 * to the luminance value of a pixel. This 8 bit value is a signed number 
 * in two’s-complement.
 *
 * @arg Saturation manipulation in chrominance processing in terms of multiplying
 * Cr and Cb with a fixed-point number between 0.0 (0x00) and 1.992 (0xFF). This
 * value is to be programmed in CPROC_SATURATION.
 *
 * @arg Hue processing is a phase shift of the chrominance values between -90
 * (0x80) degree and +87.188 (0x7F) degree to be defined in CPROC_HUE.
 *
 * The color processing module can be configured to produce pixel values 
 * according to the BT.601 standard or in full value range. Full value range
 * should be used for JPEG encoding. See register COLOR_PROC_CTRL for details
 * of the possible configurations. The input pixel range is also configurable,
 * because pixels coming from the YCbCr path of the ISP may have both ranges 
 * depending on the camera sensor used. 
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
 * @brief   Enumeration type to configure the chrominace output range in the 
 *          CamerIC color processing unit.
 *
 *****************************************************************************/
typedef enum CamerIcCprocChrominanceRangeOut_e
{
    CAMERIC_CPROC_CHROM_RANGE_OUT_INVALID       = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_CPROC_CHROM_RANGE_OUT_BT601         = 1,    /**< CbCr_out clipping range 16..240 according to ITU-R BT.601 standard */
    CAMERIC_CPROC_CHROM_RANGE_OUT_FULL_RANGE    = 2,    /**< full UV_out clipping range 0..255 */
    CAMERIC_CPROC_CHROM_RANGE_OUT_MAX                   /**< upper border (only for an internal evaluation) */
} CamerIcCprocChrominaceRangeOut_t;


/******************************************************************************/
/**
 * @brief   Enumeration type to configure the luminance output range in the 
 *          CamerIC color processing unit.
 *
 *****************************************************************************/
typedef enum CamerIcCprocLuminanceRangeOut_e
{
    CAMERIC_CPROC_LUM_RANGE_OUT_INVALID         = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_CPROC_LUM_RANGE_OUT_BT601           = 1,    /**< Y_out clipping range 16..235; offset of 16 is added to Y_out according to ITU-R BT.601 standard */
    CAMERIC_CPROC_LUM_RANGE_OUT_FULL_RANGE      = 2,    /**< Y_out clipping range 0..255; no offset is added to Y_out */
    CAMERIC_CPROC_LUM_RANGE_OUT_MAX                     /**< upper border (only for an internal evaluation) */
} CamerIcCprocLuminanceRangeOut_t;


/******************************************************************************/
/**
 * @brief   Enumeration type to configure the luminance input range in the 
 *          CamerIC color processing unit.
 *
 *****************************************************************************/
typedef enum CamerIcCprocLuminanceRangeIn_e
{
    CAMERIC_CPROC_LUM_RANGE_IN_INVALID          = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_CPROC_LUM_RANGE_IN_BT601            = 1,    /**< Y_in range 64..940 according to ITU-R BT.601 standard; offset of 64 will be subtracted from Y_in */
    CAMERIC_CPROC_LUM_RANGE_IN_FULL_RANGE       = 2,    /**< Y_in full range 0..1023; no offset will be subtracted from Y_in */
    CAMERIC_CPROC_LUM_RANGE_IN_MAX                      /**< upper border (only for an internal evaluation) */
} CamerIcCprocLuminanceRangeIn_t;


/******************************************************************************/
/**
 * @brief   Structure to configure the color processing module
 *
 *****************************************************************************/
typedef struct CamerIcCprocConfig_s
{
    CamerIcCprocChrominaceRangeOut_t    ChromaOut;      /**< configuration of color processing chrominance pixel clipping range at output */
    CamerIcCprocLuminanceRangeOut_t     LumaOut;        /**< configuration of color processing luminance input range (offset processing) */
    CamerIcCprocLuminanceRangeIn_t      LumaIn;         /**< configuration of color processing luminance output clipping range */

    uint8_t                             contrast;       /**< contrast value to initially set */
    uint8_t                             brightness;     /**< brightness value to initially set */
    uint8_t                             saturation;     /**< saturation value to initially set */
    uint8_t                             hue;            /**< hue value to initially set */
} CamerIcCprocConfig_t;

/*****************************************************************************/
/**
 * @brief   This function returns RET_SUCCESS if the color processing 
 *          module available in hardware.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         CNR available
 * @retval  RET_NOTSUPP         CNR not available
 *
 *****************************************************************************/
extern RESULT CamerIcCprocIsAvailable
(
    CamerIcDrvHandle_t handle
);

/*****************************************************************************/
/**
 * @brief   This function configures the CamerIC CPROC Module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcCprocConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcCprocConfig_t    *pConfig
);


/*****************************************************************************/
/**
 * @brief   This function enables the CamerIC CPROC Module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcCprocEnable
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function disables the CamerIc CPROC module (bypass the color
 *          prcessing module)
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcCprocDisable
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function returns the status of the CamerIC CPROC module.
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
extern RESULT CamerIcCprocIsEnabled
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pIsEnabled
);


/*****************************************************************************/
/**
 * @brief   This function sets the ranges in the CamerIc CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   ChromaOut           Color processing chrominance pixel clipping 
 *                              range at output 
 * @param   LumaOut             Color processing luminance output clipping
 *                              range
 * @param   LumaIn              Color processing luminance input range 
 *                              (offset processing)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcCprocSetRanges
(
    CamerIcDrvHandle_t                      handle,
    const CamerIcCprocChrominaceRangeOut_t  ChromaOut,
    const CamerIcCprocLuminanceRangeOut_t   LumaOut,
    const CamerIcCprocLuminanceRangeIn_t    LumaIn
);


/*****************************************************************************/
/**
 * @brief   This function returns the contrast value currently configured in 
 *          CamerIC CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   contrast            pointer to store the current contrast value
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    pointer is a NULL pointer 
 *
 *****************************************************************************/
extern RESULT CamerIcCprocGetContrast
(
    CamerIcDrvHandle_t  handle,
    uint8_t             *contrast
);


/*****************************************************************************/
/**
 * @brief   This function sets the contrast value CamerIC CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   contrast            contrast value to set
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given contrast value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcCprocSetContrast
(
    CamerIcDrvHandle_t  handle,
    const uint8_t       contrast
);


/*****************************************************************************/
/**
 * @brief   This function returns the brightness value currently configured in 
 *          CamerIC CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   brightness          pointer to store the current brightness value
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given brightness value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcCprocGetBrightness
(
    CamerIcDrvHandle_t      handle,
    uint8_t                 *brightness
);


/*****************************************************************************/
/**
 * @brief   This function sets the brightness value in CamerIC CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   brightness          brightness value to set
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given brightness value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcCprocSetBrightness
(
    CamerIcDrvHandle_t      handle,
    const uint8_t           brightness
);


/*****************************************************************************/
/**
 * @brief   This function returns the saturation value currently configured in 
 *          CamerIC CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   saturation          pointer to store the current saturation value
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given brightness value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcCprocGetSaturation
(
    CamerIcDrvHandle_t      handle,
    uint8_t                 *saturation
);


/*****************************************************************************/
/**
 * @brief   This function sets the saturation value in CamerIC CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   saturation          saturation value to set
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given saturation value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcCprocSetSaturation
(
    CamerIcDrvHandle_t      handle,
    const uint8_t           saturation
);


/*****************************************************************************/
/**
 * @brief   This function returns the hue value currently configured in 
 *          CamerIC CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   hue                 pointer to store the current hue value
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given brightness value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcCprocGetHue
(
    CamerIcDrvHandle_t      handle,
    uint8_t                 *hue
);


/*****************************************************************************/
/**
 * @brief   This function sets the hue value in CamerIC CPROC module.
 *
 * @param   handle              CamerIc driver handle
 * @param   hue                 hue value to set
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given hue value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcCprocSetHue
(
    CamerIcDrvHandle_t      handle,
    const uint8_t           hue
);


#ifdef __cplusplus
}
#endif

/* @} cameric_cproc_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_CPROC_DRV_API_H__ */

