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
#ifndef __CAMERIC_ISP_WDR_DRV_API_H__
#define __CAMERIC_ISP_WDR_DRV_API_H__

/**
 * @cond cameric_isp_wdr
 *
 * @file    cameric_isp_wdr_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP WDR driver API definitions.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cameric_isp_wdr_drv_api CamerIC ISP WDR Driver API definitions
 * @{
 *
 * @image html cameric20MP_isp_wdr.png "CamerIC ISP WDR driver" width=\textwidth
 * @image latex cameric20MP_isp_wdr.png "CamerIC ISP WDR driver" width=\textwidth
 *
 * The dynamic range of real-word scenes is much higher than the available
 * dynamic range of displays and printed photos. The image sensor thus captures
 * a small range of the real world's scene radiance and maps it to the available
 * output range of the sensor. Radiance levels above or below the sensor's value
 * range are clipped to black or white in the sensor.
 *
 * The auto-exposure control (AEC) controls which portion of the scene radiance
 * is mapped to the sensor value range. The AEC uses a model-driven scene evaluation
 * to determine the best exposure value.
 *
 * Nevertheless, there is a chance that portions in the scene are mapped to the
 * dark grey tones or near white tones. This is the case especially in high
 * contrast scenes, when there is anyhow not the chance to perfectly reproduce
 * the full scene radiance range.
 *
 * Global tone mapping can be used to reduce this effect. It aims at shifting
 * textures in dark grey or near white tones into the mid tone range and thus allows
 * to optimize the perceptual reproduction of the scene. Compared with a standard
 * camera, the dynamic range of input intensities appears to be widened, since
 * more structure becomes visible out of the dark and bright image regions.
 *
 * This step is being performed directly before the Gamma-Correction. Basically
 * by applying a scene dependent tone curve the required intensity shift is
 * being performed. During this step the following constraints have to be considered:
 *
 * @arg Shifting of textures from dark grey into mid tones increases noise.
 * Thus this step should only be performed for images with a sufficiently low noise
 * level in dark grey tones.
 *
 * @arg Changing the intensity level of a pixel also effects the color saturation.
 * In order to avoid color clipping a correction of the color saturation is to be
 * performed. This ensures that after tone mapping the colors have the same hue as before.
 *
 * The tone and color clipping correction unit for wide dynamic range applications (WDR)
 * for CamerIC performs scene dependent correction such as brightening of dark
 * texture tones. Suitably, the CamerIC denoising pre-filter is being used
 * for edge-preserving noise reduction especially in dark textures.
 * Additionally, color clipping compensation is being performed with tone mapping.
 *
 * The scene dependent tone mapping curve is determined based on the measured image
 * histogram. This curve is calculated in the software during runtime and is
 * updated for the next frame. The software also determines a proper exposure during
 * runtime, which optimally follows the paradigm of protecting the raw image
 * capture from having too many clipped pixels. This paradigm is different
 * than the current one, i.e., the middle-gray approach where the overall relative
 * luminance is controlled to a reference target value. This can lead to severe
 * overexposure for backlit scenes and underexposure for frontlit scenes. However,
 * only controlling the amount of overexposed pixels in the AEC is not sufficient
 * since sensor and quantization noise can sacrifice the tone mapping outcome.
 * Consequently the AEC is following the luminance middle-gray approach with
 * the constraint of reducing the number of clipped pixels as far as allowable
 * within the capture settings (amount of noise, scene's dynamic range, priority
 * on object region provided by the AEC scene evaluation, white-balance
 * dynamic range reduction, etc.).
 *
 * @image html cameric20MP_isp_wdr_functional.png "Functional overview of the WDR Unit" width=0.75\textwidth
 * @image latex cameric20MP_isp_wdr_functional.png "Functional overview of the WDR Unit" width=0.75\textwidth
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CAMERIC_WDR_MAX_COEFFS      33
#define CAMERIC_WDR_MAX_SEGMENTS    ( CAMERIC_WDR_MAX_COEFFS - 1 )



/*******************************************************************************
 *
 *          CamerIcIspWdrToneCurve_t
 *
 * @brief   
 *
 */
typedef struct CamerIcIspWdrToneCurve_s
{
    uint16_t    Segment[CAMERIC_WDR_MAX_SEGMENTS];
    uint16_t    Ym[CAMERIC_WDR_MAX_COEFFS];
} CamerIcIspWdrToneCurve_t;



/*****************************************************************************/
/**
 *          CamerIcIspWdrEnable()
 *
 * @brief   Enable WDR Module
 *
 * @param   handle          CamerIc driver handle
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspWdrEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 *          CamerIcIspWdrDisable()
 *
 * @brief   Disable WDR Module
 *
 * @param   handle          CamerIc driver handle
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspWdrDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 *          CamerIcIspWdrSetRgbOffset()
 *
 * @brief   Set RGB Offset
 *
 * @param   handle          CamerIc driver handle
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspWdrSetRgbOffset
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      RgbOffset
);



/*****************************************************************************/
/**
 *          CamerIcIspWdrSetLumOffset()
 *
 * @brief   Set Luminance Offset
 *
 * @param   handle          CamerIc driver handle
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspWdrSetLumOffset
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      LumOffset
);



/*****************************************************************************/
/**
 *          CamerIcIspWdrSetStaticDemoConfig()
 *
 * @brief   Set static WDR configuration; quick hack for demo purposes only!
 *
 * @param   handle          CamerIc driver handle
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspWdrSetStaticDemoConfig
(
    CamerIcDrvHandle_t  handle
);

/***********************************************************************
****************************************************************
*Modify by Fei for demo Only Start!!!
****************************************************************
************************************************************************/
RESULT CamerIcIspWdrSetStaticDemoConfigCurve0
(
    CamerIcDrvHandle_t  handle
);


RESULT CamerIcIspWdrSetStaticDemoConfigCurve1
(
    CamerIcDrvHandle_t  handle
);

RESULT CamerIcIspWdrSetStaticDemoConfigCurve2
(
    CamerIcDrvHandle_t  handle
);

RESULT CamerIcIspWdrSetStaticDemoConfigCurve3
(
    CamerIcDrvHandle_t  handle
);

/***********************************************************************
****************************************************************
*Modify by Fei for demo Only End!!!
****************************************************************
************************************************************************/
RESULT CamerIcIspWdrSetCurve
(
    CamerIcDrvHandle_t  handle, 
    uint16_t            *Ym,
    uint8_t             *dY
);

#ifdef __cplusplus
}
#endif

/* @} cameric_isp_wdr_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_DPF_DRV_API_H__ */

