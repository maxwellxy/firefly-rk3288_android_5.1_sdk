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
#ifndef __CAMERIC_ISP_DPF_DRV_API_H__
#define __CAMERIC_ISP_DPF_DRV_API_H__

/**
 * @cond cameric_isp_dpf
 *
 * @file    cameric_isp_dpf_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP DPF driver API definitions.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_dpf_drv_api CamerIC ISP DPF Driver API definitions
 * @{
 *
 * @image html cameric20MP_isp_dpf.png "CamerIC ISP DPF driver" width=\textwidth
 * @image latex cameric20MP_isp_dpf.png "CamerIC ISP DPF driver" width=\textwidth
 *
 * A so called noise level function can be programmed as a piecewise linear
 * curve with 16 linear segments resulting in 17 boundary (y) values. This
 * allows the noise level curve to be approximated. The x-axis of the noise
 * level curve can be selected to be linear or logarithmic.
 *
 * The filter is implemented as a bilateral filter which combines a range filter and
 * a spatial filter. Filtering is applied on RGB Bayer data. The spatial filter
 * can be adapted in terms of weight for each pixel position inside the filter
 * kernel. Because of rotation symmetry and empty positions, 6 values for red/blue
 * and green are stored in the register file. The center coefficients 0 are
 * hardwired to "1".
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/list.h>

#ifdef __cplusplus
extern "C"
{
#endif




#define CAMERIC_DPF_MAX_NLF_COEFFS      17
#define CAMERIC_DPF_MAX_SPATIAL_COEFFS  6



/*****************************************************************************/
/**
 * @brief   This type defines the supported filter kernel sizes for the red
 *          and blue channels.
 */
/*****************************************************************************/
typedef enum CamerIcDpfRedBlueFilterSize_e
{
    CAMERIC_DPF_RB_FILTERSIZE_INVALID   = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_DPF_RB_FILTERSIZE_9x9       = 1,    /**< red and blue filter kernel size 9x9 (means 5x5 active pixel) */
    CAMERIC_DPF_RB_FILTERSIZE_13x9      = 2,    /**< red and blue filter kernel size 13x9 (means 7x5 active pixel) */
    CAMERIC_DPF_RB_FILTERSIZE_MAX               /**< upper border (only for an internal evaluation) */
} CamerIcDpfRedBlueFilterSize_t;



/*****************************************************************************/
/**
 * @brief   This type defines the supported gain usage modes in the DPF 
 *          preprocessing stage.
 */
/*****************************************************************************/
typedef enum CamerIcDpfGainUsage_e
{
    CAMERIC_DPF_GAIN_USAGE_INVALID       = 0,   /**< lower border (only for an internal evaluation) */
    CAMERIC_DPF_GAIN_USAGE_DISABLED      = 1,   /**< don't use any gains in preprocessing stage */
    CAMERIC_DPF_GAIN_USAGE_NF_GAINS      = 2,   /**< use only the noise function gains  from registers DPF_NF_GAIN_R, ... */
    CAMERIC_DPF_GAIN_USAGE_LSC_GAINS     = 3,   /**< use only the gains from LSC module */
    CAMERIC_DPF_GAIN_USAGE_NF_LSC_GAINS  = 4,   /**< use the moise function gains and the gains from LSC module */
    CAMERIC_DPF_GAIN_USAGE_AWB_GAINS     = 5,   /**< use only the gains from AWB module */
    CAMERIC_DPF_GAIN_USAGE_AWB_LSC_GAINS = 6,   /**< use the gains from AWB and LSC module */
    CAMERIC_DPF_GAIN_USAGE_MAX                  /**< upper border (only for an internal evaluation) */
} CamerIcDpfGainUsage_t;



/*****************************************************************************/
/**
 * @brief   This type defines the supported scaling of x axis.
 */
/*****************************************************************************/
typedef enum CamerIcDpfNoiseLevelLookUpScale_e
{
    CAMERIC_NLL_SCALE_INVALID       = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_NLL_SCALE_LINEAR        = 1,        /**< use a linear scaling */
    CAMERIC_NLL_SCALE_LOGARITHMIC   = 2,        /**< use a logarithmic scaling */
    CAMERIC_NLL_SCALE_MAX                       /**< upper border (only for an internal evaluation) */
} CamerIcDpfNoiseLevelLookUpScale_t;



/*****************************************************************************/
/**
 * @brief   This type defines the 
 */
/*****************************************************************************/
typedef struct CamerIcDpfInvStrength_s
{
    uint8_t WeightR;
    uint8_t WeightG;
    uint8_t WeightB;
} CamerIcDpfInvStrength_t;



/*****************************************************************************/
/**
 * @brief
 */
/*****************************************************************************/
typedef struct CamerIcDpfNoiseLevelLookUp_s
{
    uint16_t                            NllCoeff[CAMERIC_DPF_MAX_NLF_COEFFS];   /**< Noise-Level-Lookup coefficients */
    CamerIcDpfNoiseLevelLookUpScale_t   xScale;                                 /**< type of x-axis (logarithmic or linear type) */
} CamerIcDpfNoiseLevelLookUp_t;



/*****************************************************************************/
/**
 * @brief   This type defines the supported filter kernel sizes for the red
 *          and blue channels.
 */
/*****************************************************************************/
typedef struct CamerIcDpfSpatial_s
{
    uint8_t WeightCoeff[CAMERIC_DPF_MAX_SPATIAL_COEFFS];
} CamerIcDpfSpatial_t;



/*****************************************************************************/
/**
 * @brief   This type defines the configuration structure of the CamerIc
 *          DPF module.
 */
/*****************************************************************************/
typedef struct CamerIcDpfConfig_s
{
    CamerIcDpfGainUsage_t           GainUsage;              /**< which gains shall be used in preprocessing stage of dpf module */

    CamerIcDpfRedBlueFilterSize_t   RBFilterSize;           /**< size of filter kernel for red/blue pixel */

    bool_t                          ProcessRedPixel;        /**< enable filter processing for red pixel */
    bool_t                          ProcessGreenRPixel;     /**< enable filter processing for green pixel in red lines */
    bool_t                          ProcessGreenBPixel;     /**< enable filter processing for green pixel in blue lines */
    bool_t                          ProcessBluePixel;       /**< enable filter processing for blux pixel */

    CamerIcDpfSpatial_t             SpatialG;               /**< spatial weights for green pixel */
    CamerIcDpfSpatial_t             SpatialRB;              /**< spatial weights for red/blue pixel */
} CamerIcDpfConfig_t;



/*****************************************************************************/
/**
 * @brief   This function enables the CamerIc ISP DPF module.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range
 *
 *****************************************************************************/
extern RESULT CamerIcIspDpfEnable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function disables the CamerIc ISP DPF module.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range
 *
 *****************************************************************************/
extern RESULT CamerIcIspDpfDisable
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This functions returns the current status of the CamerIc ISP
 *          DPF module.
 *
 * @param   handle          CamerIc driver handle.
 * @param   pIsEnabled
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
extern RESULT CamerIcIspDpfIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This function configures the CamerIc ISP DPF module.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range
 *
 *****************************************************************************/
extern RESULT CamerIcIspDpfConfig
(
    CamerIcDrvHandle_t          handle,
    const CamerIcDpfConfig_t    *pDpfCfg
);



/*****************************************************************************/
/**
 * @brief   This function sets the noise function gains to the CamerIc ISP 
 *          DPF module.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range
 * @retval  RET_NULL_POINTER    NULL pointer detected
 *
 *****************************************************************************/
extern RESULT CamerIcIspDpfSetNoiseFunctionGain
(
    CamerIcDrvHandle_t      handle,
    const CamerIcGains_t    *pNfGains
);



/*****************************************************************************/
/**
 * @brief   Programs the given inverse strength to the CamerIc ISP DPF module.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range
 * @retval  RET_NULL_POINTER    NULL pointer detected
 *
 *****************************************************************************/
extern RESULT CamerIcIspDpfSetStrength
(
    CamerIcDrvHandle_t              handle,
    const CamerIcDpfInvStrength_t   *pDpfStrength
);



/*****************************************************************************/
/**
 * @brief   Programs the given Noise-Level-Lookup-Table to the CamerIc
 *          ISP DPF module
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Configuration successfully applied
 * @retval  RET_OUTOFRANGE      At least one perameter of out range
 * @retval  RET_NULL_POINTER    NULL pointer detected
 *
 *****************************************************************************/
extern RESULT CamerIcIspDpfSetNoiseLevelLookUp
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcDpfNoiseLevelLookUp_t  *pDpfNll
);







extern RESULT CamerIcIspDpfSetStaticDemoConfigGain12
(
    CamerIcDrvHandle_t  handle
);

extern RESULT CamerIcIspDpfSetStaticDemoConfigGain24
(
    CamerIcDrvHandle_t  handle
);

extern RESULT CamerIcIspDpfSetStaticDemoConfigGain48
(
    CamerIcDrvHandle_t  handle
);


#ifdef __cplusplus
}
#endif

/* @} cameric_isp_dpf_drv_api */

/* @endcond */

#endif /* __CAMERIC_ISP_DPF_DRV_API_H__ */

