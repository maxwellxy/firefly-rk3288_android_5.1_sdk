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
#ifndef __CAMERIC_IE_DRV_API_H__
#define __CAMERIC_IE_DRV_API_H__

/**
 * @cond    cameric_ie
 *
 * @file    cameric_ie_drv_api.h
 *
 * @brief   This file contains the CamerIC IE driver API definitions.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_ie_drv_api CamerIC IE Driver API definitions
 * @{
 *
 * @if CAMERIC_LITE
 *
 * @image html ie_lite.png "Overview of the CamerIC IE driver" width=\textwidth
 * @image latex ie_lite.png "Overview of the CamerIC IE driver" width=\textwidth
 *
 * @endif
 *
 * @if CAMERIC_FULL
 *
 * @image html cameric20MP_ie.png "CamerIC IE driver" width=\textwidth
 * @image latex cameric20MP_ie.png "CamerIC IE driver" width=\textwidth
 *
 * @endif
 *
 * The Image Effects block modifies an image by pixel modifications. A set of
 * different modifications can be applied:
 *
 * @arg grayscale-effect: In this mode only the luminance component of the image
 * is processed. The chrominance part is set to 128.
 *
 * @arg negative-effect: This effect inverts the luminance and chrominance part of a
 * picture. The values are inverted according to the BT.601 pixel value range
 * (16...235 for luminance and 16...240 for chrominance).
 *
 * @arg sepia-effect: In sepia mode the two chrominance components are processed.
 * The chrominance should be modified in the appropriate brown hue to create a 
 * historical like image color. The new chrominance parts of resulting pixels 
 * are calculated according to the programmed tint color value (IMG_EFF_TINT) 
 * and the luminance part of the original pixel. The Y component is passed through.
 *
 * @arg color selection-effect: Converting picture to grayscale while maintaining
 * one color component above a configurable threshold value (0...255).
 *
 * The following effects are realized with a programmable 3x3 Laplace filter 
 * which uses a two line buffers. The kernel for filtering (see Table 2) is 
 * programmable and for the following two effects (emboss and sketch) the 
 * configuration registers are separately available with their default 
 * values (IMG_EFF_MAT_1 to IMG_EFF_MAT_5).
 *
 * Programmable Laplace Filter Kernel: 
 *
 * 1.1 | 1.2 | 1.3 \n
 * ----+-----+---- \n
 * 2.1 | 2.2 | 2.3 \n
 * ----+-----+---- \n
 * 3.1 | 3.2 | 3.3 \n
 *
 * @arg emboss-effect: The emboss effect is created by the result of a 3x3
 * Laplace Filter with the luminance components. For emboss picture appearing 
 * an offset 128 is added to the result. The chrominance components are not 
 * processed and are set to 128.
 *
 * @arg sketch effect: The sketch mode is also a result of the 3x3 Laplace 
 * Filter with an extracting of the edges in whitely background. A fixed 
 * threshold value of 31 is defined. All luminance values smaller 31 are 
 * set to 235 to visualize the whitely background. Other luminance values
 * are subtracted from 235 to visualize the edges in picture. The chrominance
 * components are not processed and are set to 128.
 *
 * @arg The sharpen effect is applied to the Y component only. The Cb and Cr
 * components are not modified by this effect. The sharpen function shall be
 * realized as a coring filter which consists of a highpass filter and a 
 * threshold function for the highpass signal. The output signal is the sum
 * of highpass and original signal as shown in Figure 2. The coefficients 
 * for the highpass filter are shown in Table 3. The factor (sharp_factor)
 * for the highpass signal shall be programmable through a register in the
 * range 0.1 to 1.5. The threshold (coring_thr) for the coring function shall
 * have a range between 0 and 255. The registers for the convolution mask of 
 * the highpass filter shall be shared between sketch and sharpening effect.
 *
 * In addition a sharpening function shall be implemented, which enhances the
 * perceived sharpness of images by applying a kind of coring function to the
 * Y component of the image.
 *
 * The Image Effects module gets YCbCr 4:2:2 data via a 16 bit ([15:8]: Y, 
 * [7:0]: Cb/Cr) data interface from the Color Processing Module. An image 
 * can be modified in eight different effect modes (see chapter 1.1).
 * 
 * The YCbCr 4:2:2 output picture is sent to the Superimpose Module via a 16 
 * bit data interface.
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
 * @brief   Enumeration type to configure the IE working mode.
 *
 *****************************************************************************/
typedef enum CamerIcIeMode_e
{
    CAMERIC_IE_MODE_INVALID                 = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_IE_MODE_GRAYSCALE               = 1,        /**< Set a fixed chrominance of 128 (neutral grey) */
    CAMERIC_IE_MODE_NEGATIVE                = 2,        /**< Luminance and chrominance data is being inverted */
    CAMERIC_IE_MODE_SEPIA                   = 3,        /**< Chrominance is changed to produce a historical like brownish image color */
    CAMERIC_IE_MODE_COLOR                   = 4,        /**< Converting picture to grayscale while maintaining one color component. */
    CAMERIC_IE_MODE_EMBOSS                  = 5,        /**< Edge detection, will look like an relief made of metal */
    CAMERIC_IE_MODE_SKETCH                  = 6,        /**< Edge detection, will look like a pencil drawing */
    CAMERIC_IE_MODE_SHARPEN                 = 7,        /**< Edge detection, will look like a sharper drawing */
    CAMERIC_IE_MODE_MAX                                 /**< upper border (only for an internal evaluation) */
} CamerIcIeMode_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure the IE working range.
 *
 *****************************************************************************/
typedef enum CamerIcIeRange_e
{
    CAMERIC_IE_RANGE_INVALID                = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_IE_RANGE_BT601                  = 1,        /**< pixel value range accoring to BT.601 */
    CAMERIC_IE_RANGE_FULL_RANGE             = 2,        /**< YCbCr full range 0..255 */
    CAMERIC_IE_RANG_MAX                                 /**< upper border (only for an internal evaluation) */
} CamerIcIeRange_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure the color selection effect
 *
 *****************************************************************************/
typedef enum CamerIcIeColorSelection_e
{
    CAMERIC_IE_COLOR_SELECTION_INVALID      = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_IE_COLOR_SELECTION_RGB          = 1,        /**< red, green and blue */
    CAMERIC_IE_COLOR_SELECTION_B            = 2,        /**< blue */
    CAMERIC_IE_COLOR_SELECTION_G            = 3,        /**< green */
    CAMERIC_IE_COLOR_SELECTION_GB           = 4,        /**< green and blue */
    CAMERIC_IE_COLOR_SELECTION_R            = 5,        /**< red */
    CAMERIC_IE_COLOR_SELECTION_RB           = 6,        /**< red and blue */
    CAMERIC_IE_COLOR_SELECTION_RG           = 7,        /**< red and green */
    CAMERIC_IE_COLOR_SELECTION_MAX                      /**< upper border (only for an internal evaluation) */
} CamerIcIeColorSelection_t;



/******************************************************************************/
/**
 * @brief   Structure to configure the Image Effects module
 *
 *****************************************************************************/
typedef struct CamerIcIeConfig_s
{
    CamerIcIeMode_t                     mode;           /**< working mode (see @ref CamerIcIeMode_e) */
    CamerIcIeRange_t                    range;          /**< working range (see @ref CamerIcIeRange_e) */

    union ModeConfig_u
    {
        struct Sepia_s                                  /**< active when sepia effect */
        {
            uint8_t                     TintCb;
            uint8_t                     TintCr;
        } Sepia;

        struct ColorSelection_s                         /**< active when color selection effect */
        {
            CamerIcIeColorSelection_t   col_selection;
            uint8_t                     col_threshold;
        } ColorSelection;

        struct Emboss_s                                 /**< active when emboss effect */
        {
            int8_t                      coeff[9];
        } Emboss;

        struct Sketch_s                                 /**< active when sketch effect */
        {
            int8_t                      coeff[9];
        } Sketch;
        
        struct Sharpen_s                                /**< active when sharpen */
        {
            uint8_t                     factor;         /**< sharpen factor */
            uint8_t                     threshold;      /**< corring threshold */
            int8_t                      coeff[9];       /**< convolution coefficients */
        } Sharpen;

    } ModeConfig;

} CamerIcIeConfig_t;



/*****************************************************************************/
/**
 * @brief   This function enables the IE Module.
 *
 * @param   handle          	CamerIc driver handle
 *
 * @return                  	Return the result of the function call.
 * @retval	RET_SUCCESS			operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIeEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function disables the IE module (bypass the image effects
 * 			module).
 *
 * @param   handle          	CamerIc driver handle
 *
 * @return                  	Return the result of the function call.
 * @retval	RET_SUCCESS			operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcIeDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function returns the status of the IE module.
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
extern RESULT CamerIcIeIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This function configures the IE module
 *
 * @param   handle              CamerIC driver handle
 * @param   pConfig             pointer to image effects configuration structure
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NOTSUPP         selected working mode is not supported
 * @retval  RET_NULL_POINTER    null pointer
 * @retval  RET_BUSY            image effects already enabled
 *
 *****************************************************************************/
extern RESULT CamerIcIeConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcIeConfig_t       *pConfig
);


/*****************************************************************************/
/**
 * @brief   This function sets the Cb tint value in CamerIC IE module.
 *
 * @param   handle              CamerIc driver handle
 * @param   tint                tint value to set
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given brightness value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcIeSetTintCb
(
    CamerIcDrvHandle_t      handle,
    const uint8_t           tint
);


/*****************************************************************************/
/**
 * @brief   This function sets the Cr tint value in CamerIC IE module.
 *
 * @param   handle              CamerIc driver handle
 * @param   tint                tint value to set
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given brightness value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcIeSetTintCr
(
    CamerIcDrvHandle_t      handle,
    const uint8_t           tint
);


/*****************************************************************************/
/**
 * @brief   This function sets color selection in CamerIC IE module.
 *
 * @param   handle              CamerIc driver handle
 * @param   color               selected color
 * @param   threshold           threshold
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given brightness value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcIeSetColorSelection
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIeColorSelection_t color,
    const uint8_t                   threshold
);


/*****************************************************************************/
/**
 * @brief   This function sets sharpening in CamerIC IE module.
 *
 * @param   handle              CamerIc driver handle
 * @param   factor              sharpening factor color
 * @param   threshold           threshold
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      given brightness value is out of range
 *
 *****************************************************************************/
extern RESULT CamerIcIeSetSharpen
(
    CamerIcDrvHandle_t              handle,
    const uint8_t                   factor,
    const uint8_t                   threshold
);


#ifdef __cplusplus
}
#endif

/* @} cameric_ie_drv_api */

/* @endcond */

#endif /* __CAMERIC_IE_DRV_API_H__ */

