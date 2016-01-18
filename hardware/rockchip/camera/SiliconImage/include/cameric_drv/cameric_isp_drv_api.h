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
#ifndef __CAMERIC_ISP_DRV_API_H__
#define __CAMERIC_ISP_DRV_API_H__

/**
 * @file    cameric_isp_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP driver API.
 *
 *****************************************************************************/
/**
 * @if CAMERIC_LITE
 *
 * @defgroup cameric_isp_drv_api CamerIC ISP lite driver API definitions
 * @{
 *
 * @image html isp_lite.png "Software ISP-lite driver" width=\textwidth
 * @image latex isp_lite.png "Software ISP-lite driver" width=\textwidth
 *
 * @endif
 *
 * @if CAMERIC_FULL
 *
 * @defgroup cameric_isp_drv_api CamerIC ISP driver API definitions
 * @{
 *
 * @image html cameric20MP_isp.png "CamerIC ISP driver" width=\textwidth
 * @image latex cameric20MP_isp.png "CamerIC ISP driver" width=\textwidth
 *
 * @endif
 *
 * The ISP sub-module performs "Image Signal Processing", but it also contains input
 * acquisition, output formatting, as well as status and error interrupt generation.
 *
 * The CamerIC IP contains full digital ISP functionalities which are:
 * @arg Defect pixel detection / correction
 * @arg Sensor crosstalk compensation
 * @arg Black level compensation
 * @arg Lens shade correction
 * @arg RGB Bayer demosaicing
 * @arg Chromatic aberration correction
 * @arg Filtering (noise, sharpness/blurring)
 * @arg Auto focus measurement
 * @arg Auto white balancing
 * @arg Auto exposure measurement
 * @arg Histogram calculation
 * @arg Color correction matrix
 * @arg Global tone mapping
 * @arg Gamma correction
 * @arg Color space conversion to YCbCr
 * @arg Mechanical shutter control
 * @arg Flash light control
 * @arg Video Stabilization
 *
 * The image processing functionality of the CamerIC ISP can be accessed using
 * separate CamerIC ISP driver modules:
 * @arg CamerIC ISP BLS (@ref cameric_isp_bls_drv_api)
 * @arg CamerIC ISP DEGAMMA (@ref cameric_isp_degamma_drv_api)
 * @arg CamerIC ISP LSC (@ref cameric_isp_lsc_drv_api)
 * @arg CamerIC ISP AWB (@ref cameric_isp_awb_drv_api)
 * @arg CamerIC ISP AFM (@ref cameric_isp_afm_drv_api)
 * @arg CamerIC ISP HIST (@ref cameric_isp_hist_drv_api)
 * @arg CamerIC ISP AE (@ref cameric_isp_exp_drv_api)
 * @arg CamerIC ISP DPCC (@ref cameric_isp_dpcc_drv_api)
 * @arg CamerIC ISP DPF (@ref cameric_isp_dpf_drv_api)
 * @arg CamerIC ISP FLT (@ref cameric_isp_flt_drv_api)
 * @arg CamerIC ISP CAC (@ref cameric_isp_cac_drv_api)
 * @arg CamerIC ISP WDR (@ref cameric_isp_wdr_drv_api)
 * @arg CamerIC ISP VSM (@ref cameric_isp_vsm_drv_api)
 * @arg CamerIC ISP IS (@ref cameric_isp_is_drv_api)
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif



/******************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This macro defines the number of elements in a gamma-curve.
 *
 *****************************************************************************/
#define CAMERIC_ISP_GAMMA_CURVE_SIZE        17
/* @endcond */




/******************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   Enumeration type to configure the ISP working mode.
 *
 *****************************************************************************/
typedef enum CamerIcIspMode_e
{
    CAMERIC_ISP_MODE_INVALID                = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_MODE_RAW                    = 1,        /**< RAW picture with BT.601 sync (ISP bypass) */
    CAMERIC_ISP_MODE_656                    = 2,        /**< ITU-R BT.656 (YUV with embedded sync) */
    CAMERIC_ISP_MODE_601                    = 3,        /**< ITU-R BT.601 (YUV input with H and Vsync signals) */
    CAMERIC_ISP_MODE_BAYER_RGB              = 4,        /**< Bayer RGB processing with H and Vsync signals */
    CAMERIC_ISP_MODE_DATA                   = 5,        /**< data mode (ISP bypass, sync signals interpreted as data enable) */
    CAMERIC_ISP_MODE_RGB656                 = 6,        /**< Bayer RGB processing with BT.656 synchronization */
    CAMERIC_ISP_MODE_RAW656                 = 7,        /**< RAW picture with ITU-R BT.656 synchronization (ISP bypass) */
    CAMERIC_ISP_MODE_MAX                                /**< upper border (only for an internal evaluation) */
} CamerIcIspMode_t;
/* @endcond */



/******************************************************************************/
/**
 * @brief   Enumeration type to configure the edge sampling in ISP
 *          input acquisition.
 *
 *****************************************************************************/
typedef enum CamerIcIspSampleEdge_e
{
    CAMERIC_ISP_SAMPLE_EDGE_INVALID         = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_SAMPLE_EDGE_FALLING         = 1,        /**< sample falling edga */
    CAMERIC_ISP_SAMPLE_EDGE_RISING          = 2,        /**< sample rising edge */
    CAMERIC_ISP_SAMPLE_EDGE_MAX                         /**< upper border (only for an internal evaluation) */
} CamerIcIspSampleEdge_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure the vertical and horizontal
 *          polarity in ISP input acquisition.
 *
 *****************************************************************************/
typedef enum CamerIcIspPolarity_e
{
    CAMERIC_ISP_POLARITY_INVALID            = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_POLARITY_HIGH               = 1,        /**< high active */
    CAMERIC_ISP_POLARITY_LOW                = 2,        /**< low active */
    CAMERIC_ISP_POLARITY_MAX                            /**< upper border (only for an internal evaluation) */
} CamerIcIspPolarity_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure the bayer pattern in the ISP
 *          input acquisition.
 *
 *****************************************************************************/
typedef enum CamerIcIspBayerPattern_e
{
    CAMERIC_ISP_BAYER_PATTERN_INVALID       = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB      = 1,        /**< 1st line: RGRG... , 2nd line GBGB... , etc. */
    CAMERIC_ISP_BAYER_PATTERN_GRGRBGBG      = 2,        /**< 1st line: GRGR... , 2nd line BGBG... , etc. */
    CAMERIC_ISP_BAYER_PATTERN_GBGBRGRG      = 3,        /**< 1st line: GBGB... , 2nd line RGRG... , etc. */
    CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR      = 4,        /**< 2st line: BGBG... , 2nd line GRGR... , etc. */
    CAMERIC_ISP_BAYER_PATTERN_MAX                       /**< upper border (only for an internal evaluation) */
} CamerIcIspBayerPattern_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure the color subsampling mode in the 
 *          ISP input acquisition.
 *
 *****************************************************************************/
typedef enum CamerIcIspColorSubsampling_e
{
    CAMERIC_ISP_CONV422_INVALID             = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_CONV422_COSITED             = 1,        /**< co-sited color subsampling Y0Cb0Cr0 - Y1 */
    CAMERIC_ISP_CONV422_INTERLEAVED         = 2,        /**< interleaved color subsampling Y0Cb0 - Y1Cr1 (not recommended) */
    CAMERIC_ISP_CONV422_NONCOSITED          = 3,        /**< non-cosited color subsampling Y0Cb(0+1)/2 - Y1Cr(0+1)/2 */
    CAMERIC_ISP_CONV422_MAX                             /**< upper border (only for an internal evaluation) */
} CamerIcIspColorSubsampling_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure CCIR sequence in the ISP input 
 *          acquisition.
 *
 *****************************************************************************/
typedef enum CamerIcIspCCIRSequence_e
{
    CAMERIC_ISP_CCIR_SEQUENCE_INVALID       = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_CCIR_SEQUENCE_YCbYCr        = 1,        /**< YCbYCr */
    CAMERIC_ISP_CCIR_SEQUENCE_YCrYCb        = 2,        /**< YCrYCb */
    CAMERIC_ISP_CCIR_SEQUENCE_CbYCrY        = 3,        /**< CbYCrY */
    CAMERIC_ISP_CCIR_SEQUENCE_CrYCbY        = 4,        /**< CrYCbY */
    CAMERIC_ISP_CCIR_SEQUENCE_MAX                       /**< upper border (only for an internal evaluation) */
} CamerIcIspCCIRSequence_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure field sampling in the ISP input 
 *          acquisition.
 *
 *****************************************************************************/
typedef enum CamerIcIspFieldSelection_e
{
    CAMERIC_ISP_FIELD_SELECTION_INVALID     = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_FIELD_SELECTION_BOTH        = 1,        /**< sample all fields (don't care about fields) */
    CAMERIC_ISP_FIELD_SELECTION_EVEN        = 2,        /**< sample only even fields */
    CAMERIC_ISP_FIELD_SELECTION_ODD         = 3,        /**< sample only odd fields */
    CAMERIC_ISP_FIELD_SELECTION_MAX                     /**< upper border (only for an internal evaluation) */
} CamerIcIspFieldSelection_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to configure camera bus width in the CamerIC.
 *
 *****************************************************************************/
typedef enum CamerIcIspInputSelection_e
{
    CAMERIC_ISP_INPUT_INVALID               = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_INPUT_12BIT                 = 1,        /**< 12 Bit input */
    CAMERIC_ISP_INPUT_10BIT_ZZ              = 2,        /**< 10 Bit input with 2 zereos as LSB's */
    CAMERIC_ISP_INPUT_10BIT_EX              = 3,        /**< 10 Bit input with 2 MSB's as LSB's */
    CAMERIC_ISP_INPUT_8BIT_ZZ               = 4,        /**< 8 Bit input with 4 zeroes as LSB's */
    CAMERIC_ISP_INPUT_8BIT_EX               = 5,        /**< 8 Bit input with 4 MSB's as LSB's */
    CAMERIC_ISP_INPUT_MAX                               /**< upper border (only for an internal evaluation) */
} CamerIcIspInputSelection_t;



/******************************************************************************/
/**
 * @brief   Enumeration type to  configure the input interface selector on 
 *          the CamerIC input (if_select).
 *
 *****************************************************************************/
typedef enum CamerIcIspLatencyFifo_e
{
    CAMERIC_ISP_LATENCY_FIFO_INVALID            = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_LATENCY_FIFO_INPUT_FORMATTER    = 1,    /**< use input formatter input for latency fifo */
    CAMERIC_ISP_LATENCY_FIFO_DMA_READ_RAW       = 2,    /**< use dma rgb read input for latency fifo */
    CAMERIC_ISP_LATENCY_FIFO_DMA_READ_YUV       = 3,    /**< use dma rgb read input for latency fifo */
    CAMERIC_ISP_LATENCY_FIFO_MAX                        /**< upper border (only for an internal evaluation) */
} CamerIcIspLatencyFifo_t;



/******************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   Enumeration type to configure configure the demosaicing bypass.
 *
 *****************************************************************************/
typedef enum CamerIcIspDemosaicBypass_e
{
    CAMERIC_ISP_DEMOSAIC_INVALID            = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_DEMOSAIC_NORMAL_OPERATION   = 1,        /**< normal operation for RGB Bayer pattern input */
    CAMERIC_ISP_DEMOSAIC_BYPASS             = 2,        /**< demosaicing bypass for Black&White input data */
    CAMERIC_ISP_DEMOSAIC_MAX                            /**< upper border (only for an internal evaluation) */
} CamerIcIspDemosaicBypass_t;
/* @endcond */



/******************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   Enumeration type to configure configure the color conversion range.
 *
 * @note    The color conversion unit - also called color space matrix - 
 *          performs a regular RGB to YCbCr 4:4:4 color space conversion. The
 *          nine coefficients take values between -2 and +1.992.
 *
 *          \arg YCbCr range limited output \n
 *          Y    = c0*R + c1*G + c2*B + 64  \n
 *          Cb   = c3*R + c4*G + c5*B + 512 \n
 *          Cr   = c6*R + c7*G + c8*B + 512 \n
 *
 *          \arg YCbCr full range output    \n
 *          Y    = c0*R + c1*G + c2*B       \n
 *          Cb   = c3*R + c4*G + c5*B + 512 \n
 *          Cr   = c6*R + c7*G + c8*B + 512 \n
 *
 *****************************************************************************/
typedef enum CamerIcColorConversionRange_e
{
    CAMERIC_ISP_CCONV_RANGE_INVALID          = 0,       /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_CCONV_RANGE_LIMITED_RANGE    = 1,       /**< YCbCr range limited output according to ITU-R BT.601 standard */
    CAMERIC_ISP_CCONV_RANGE_FULL_RANGE       = 2,       /**< YCbCr full range output */
    CAMERIC_ISP_CCONV_RANGE_MAX                         /**< upper border (only for an internal evaluation) */
} CamerIcColorConversionRange_t;
/* @endcond */



/******************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   A structure to represent a gamma-curve.
 *
 *****************************************************************************/
typedef struct CamerIcGammaCurve_s
{
    uint16_t GammaY[CAMERIC_ISP_GAMMA_CURVE_SIZE];      /**< array of y coordinates */
} CamerIcIspGammaCurve_t;
/* @endcond */



/******************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   Enumeration type to configure the gamma curve segementation.
 *
 *****************************************************************************/
typedef enum CamerIcIspGammaSegmentationMode_e
{
    CAMERIC_ISP_SEGMENTATION_MODE_INVALID       = 0,    /**< lower border (only for an internal evaluation) */
    CAMERIC_ISP_SEGMENTATION_MODE_LOGARITHMIC   = 1,    /**< logarithmic segmentation from 0 to 4095 
                                                             (64,64,64,64,128,128,128,128,256,256,256,512,512,512,512,512) */
    CAMERIC_ISP_SEGMENTATION_MODE_EQUIDISTANT   = 2,    /**< equidistant segmentation from 0 to 4095
                                                             (256, 256, ... ) */
    CAMERIC_ISP_SEGMENTATION_MODE_MAX                   /**< upper border (only for an internal evaluation) */
} CamerIcIspGammaSegmentationMode_t;
/* @endcond */



/*****************************************************************************/
/**
 * @brief   This function registers a Request-Callback at the CamerIC ISP 
 *          Module. A request callback is called if the driver needs an 
 *          interaction from the application layer (i.e. a new data buffer 
 *          to fill, please also @ref CamerIcRequestId_e).
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
 *                              request callback
 *
 *****************************************************************************/
extern RESULT CamerIcIspRegisterRequestCb
(
    CamerIcDrvHandle_t      handle,
    CamerIcRequestFunc_t    func,
    void                    *pUserContext
);



/*****************************************************************************/
/**
 * @brief   This functions deregisters/releases a registered Request-Callback
 *          at CamerIc ISP Module. 
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
extern RESULT CamerIcIspDeRegisterRequestCb
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This functions registers an Event-Callback at CamerIC ISP  
 *          Module. An event callback is called if the driver needs to 
 *          inform the application layer about an asynchronous event or
 *          an error situation (see @ref CamerIcEventId_e).
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
extern RESULT CamerIcIspRegisterEventCb
(
    CamerIcDrvHandle_t  handle,
    CamerIcEventFunc_t  func,
    void                *pUserContext
);



/*****************************************************************************/
/**
 * @brief   This functions deregisters/releases a registered Event-Callback
 *          at CamerIC ISP Module. 
 *
 * @param   handle              CamerIC driver handle 
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to deregister the 
 *                              event callback (maybe the driver is still running)
 *
 *****************************************************************************/
extern RESULT CamerIcIspDeRegisterEventCb
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function configures the ISP working mode.
 *
 * @param   handle              CamerIC driver handle
 * @param   mode                new working mode
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to configure working
 *                              mode (maybe the driver is already running)
 * @retval  RET_NOTSUPP         selected working mode is not supported
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetMode
(
    CamerIcDrvHandle_t      handle,
    const CamerIcIspMode_t  mode
);
/* @endcond */



/*****************************************************************************/
/**
 * @brief   This function configures the CamerIC ISP input acquisition module.
 *
 * @param   handle              CamerIC driver handle
 * @param   sampleEdge          sample edge (@ref CamerIcIspSampleEdge_e)
 * @param   hSyncPol            horizontal sync polarity (@ref CamerIcIspPolarity_e)
 * @param   vSyncPol            vertical sync polarity (@ref CamerIcIspPolarity_e)
 * @param   bayerPattern        bayer pattern (@ref CamerIcIspBayerPattern_e)
 * @param   subSampling         color subsampling mode (@ref CamerIcIspColorSubsampling_e)
 * @param   seqCCIR             CCIR output sequence (@ref CamerIcIspCCIRSequence_e)
 * @param   fieldSelection      field sampling (@ref CamerIcIspFieldSelection_e)
 * @param   inputSelection      input format (@ref CamerIcIspInputSelection_e) 
 * @param   latencyFifo         latency fifo input (@ref CamerIcIspLatencyFifo_e)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to configure working
 *                              mode (maybe the driver is already running)
 * @retval  RET_NOTSUPP         a configuration is not supported
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetAcqProperties
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcIspSampleEdge_t        sampleEdge,
    const CamerIcIspPolarity_t          hSyncPol,
    const CamerIcIspPolarity_t          vSyncPol,
    const CamerIcIspBayerPattern_t      bayerPattern,
    const CamerIcIspColorSubsampling_t  subSampling,
    const CamerIcIspCCIRSequence_t      seqCCIR,
    const CamerIcIspFieldSelection_t    fieldSelection,
    const CamerIcIspInputSelection_t    inputSelection,
    const CamerIcIspLatencyFifo_t       latencyFifo
);


/*****************************************************************************/
/**
 * @brief   This function return the curretnly configred Bayer pattern in 
 *          CamerIC ISP input acquisition module.
 *
 * @param   handle              CamerIC driver handle
 * @param   pBayerPattern       reference to bayer pattern variable
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    pBayerPattern is a NULL pointer
 *
 *****************************************************************************/
extern RESULT CamerIcIspGetAcqBayerPattern
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspBayerPattern_t    *pBayerPattern
);


/*****************************************************************************/
/**
 * @brief   This function configures the CamerIC ISP input resolution.
 *
 * @note    It's possible to crop the image by using a smaller resolution.
 *
 * @param   handle              CamerIC driver handle
 * @param   hOffset             horizontal offset
 * @param   vOffset             vertical offset
 * @param   height              height
 * @param   width               width
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change the input
 *                              window (maybe the driver is already running)
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetAcqResolution
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      hOffset,
    const uint16_t      vOffset,
    const uint16_t      width,
    const uint16_t      height
);



/*****************************************************************************/
/**
 * @brief   This function configures the CamerIC ISP output formatter window.
 *
 * @param   handle              CamerIC driver handle
 * @param   hOffset             horizontal offset
 * @param   vOffset             vertical offset
 * @param   height              height
 * @param   width               width
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change the output
 *                              window (maybe the driver is already running)
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetOutputFormatterResolution
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      hOffset,
    const uint16_t      vOffset,
    const uint16_t      width,
    const uint16_t      height
);



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function sets CamerIc ISP image stabilization output window.
 *
 * @param   handle              CamerIc driver handle
 * @param   hOffset             horizontal offset
 * @param   vOffset             vertical offset
 * @param   height              height
 * @param   width               width
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change the image
 *                              stabilization outpu window (maybe the driver 
 *                              is already running)
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetImageStabilizationResolution
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      hOffset,
    const uint16_t      vOffset,
    const uint16_t      width,
    const uint16_t      height
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function sets the demosaicing working mode and threshold.
 *
 * @param   handle              CamerIc driver handle
 * @param   pBypassMode         Bypass mode
 * @param   pThreshold          Demosaicing threshold
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change the
 *                              demosaicing mode or demosaiing threshold
 *                              (maybe the driver is already running)
 * @retval  RET_NOTSUPP         demosaicing mode is not supported
 *
 *****************************************************************************/
extern RESULT CamerIcIspGetDemosaic
(
    CamerIcDrvHandle_t            handle,
    CamerIcIspDemosaicBypass_t    *pBypassMode,
    uint8_t                       *pThreshold
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function sets the demosaicing working mode and threshold.
 *
 * @param   handle              CamerIc driver handle
 * @param   BypassMode          bypass mode to set
 * @param   Threshold           Demosaicing threshold to set
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change the
 *                              demosaicing mode or demosaiing threshold
 *                              (maybe the driver is already running)
 * @retval  RET_NOTSUPP         demosaicing mode is not supported
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetDemosaic
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcIspDemosaicBypass_t    BypassMode,
    const uint8_t                       Threshold
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function sets the color conversion range. 
 *
 * @note    The color conversion unit - also called color space matrix - 
 *          performs a regular RGB to YCbCr 4:4:4 color space conversion. The
 *          nine coefficients take values between -2 and +1.992.
 *
 *          \arg YCbCr range limited output \n
 *          Y    = c0*R + c1*G + c2*B + 64  \n
 *          Cb   = c3*R + c4*G + c5*B + 512 \n
 *          Cr   = c6*R + c7*G + c8*B + 512 \n
 *
 *          \arg YCbCr full range output    \n
 *          Y    = c0*R + c1*G + c2*B       \n
 *          Cb   = c3*R + c4*G + c5*B + 512 \n
 *          Cr   = c6*R + c7*G + c8*B + 512 \n
 *
 *          \arg default matrix             \n 
 *          ( 0x021 0x040 0x00D )       (   \n
 *          ( 0x1ED 0x1DB 0x038 )   =   (   \n
 *          ( 0x038 0x1D1 0x1F7 )       (   \n
 *
 * @param   handle              CamerIc driver handle
 * @param   YConvRange          luminance conversion range
 * @param   CrConvRange         chrominance conversion range
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change the color
 *                              conversion matrix (maybe the driver is already
 *                              running)
 * @retval  RET_NOTSUPP         conversion range is not supported
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetColorConversionRange
(
    CamerIcDrvHandle_t                  handle,
    const CamerIcColorConversionRange_t YConvRange,
    const CamerIcColorConversionRange_t CrConvRange
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function returns the currently configured color conversion 
 *          coefficients. 
 *
 * @param   handle              CamerIc driver handle
 * @param   pCConvCoefficients  storage class for color conversion coefficients
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change the color
 *                              conversion matrix (maybe the driver is not 
 *                              initialized) 
 *
 *****************************************************************************/
extern RESULT CamerIcIspGetColorConversionCoefficients
(
    CamerIcDrvHandle_t  handle,
    CamerIc3x3Matrix_t  *pCConvCoefficients
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function sets the color conversion coefficients. 
 *
 * @param   handle      CamerIc driver handle
 * @param   pCConvCoefficients  Color conversion coefficients to set
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to change the color
 *                              conversion matrix (maybe the driver is already
 *                              running)
 *
 * @endif
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetColorConversionCoefficients
(
    CamerIcDrvHandle_t          handle,
    const CamerIc3x3Matrix_t    *pCConvCoefficients
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function returns the currently configured cross-talk 
 *          coefficients.
 *
 * @param   handle              CamerIc driver handle
 * @param   pCTalkCoefficients  Cross-talk coefficients
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read the cross
 *                              talk matrix (maybe the driver is not initialized)
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 * @endif
 *
 *****************************************************************************/
extern RESULT CamerIcIspGetCrossTalkCoefficients
(
    CamerIcDrvHandle_t  handle,
    CamerIc3x3Matrix_t  *pCTalkCoefficients
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This functions sets the cross-talk coefficients.
 *
 * @param   handle              CamerIc driver handle
 * @param   pCTalkCoefficients  Cross-talk coefficients
 *
 * @return                      Return the result of the function call.
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read the cross
 *                              talk matrix (maybe the driver is not initialized)
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetCrossTalkCoefficients
(
    CamerIcDrvHandle_t          handle,
    const CamerIc3x3Matrix_t    *pCTalkCoefficients
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function returns the currently configured cross-talk 
 *          offsets.
 *
 * @param   handle              CamerIc driver handle
 * @param   pCrossTalkOffset    Cross-talk offsets
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read the cross
 *                              talk matrix (maybe the driver is not initialized)
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 * @endif
 *
 *****************************************************************************/
extern RESULT CamerIcIspGetCrossTalkOffset
(
    CamerIcDrvHandle_t          handle,
    CamerIcXTalkOffset_t        *pCrossTalkOffset
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This functions sets the cross-talk offsets.
 *
 * @param   handle              CamerIc driver handle
 * @param   pCrossTalkOffset    Cross-talk offsets
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to set the cross
 *                              talk offsets (maybe the driver is already
 *                              running)
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetCrossTalkOffset
(
    CamerIcDrvHandle_t          handle,
    const CamerIcXTalkOffset_t  *pCrossTalkOffset
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This functions enables or disables the ISP AWB module.
 *
 * @param   handle              CamerIc driver handle
 * @param   enable              BOOL_TRUE enables ISP AWB module, 
 *                              BOOL_FALSE bypass the ISP AWB module
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to enable or disable
 *                              AWB module 
 *
 *****************************************************************************/
extern RESULT CamerIcIspActivateWB
(
    CamerIcDrvHandle_t  handle,
    const bool_t        enable
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This functions returns wether the ISP AWB module is enable or not. 
 *
 * @param   handle              CamerIc driver handle
 * @param   pIsEnabled          Pointer to value to store current ISP AWB state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read AWB state
 *                              (maybe driver not initialized)
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsWBActivated
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pIsEnabled
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function eanbles the gamma-out correction.
 *
 * @param   handle              CamerIc driver handle
 * @param   enable              BOOL_TRUE to enable, BOOL_FALSE to disable
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to switch gamma-out
 *
 *****************************************************************************/
extern RESULT CamerIcIspGammaOutEnable
(
    CamerIcDrvHandle_t  handle
);
/* @endcond */


/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function disables the gamma-out correction.
 *
 * @param   handle              CamerIc driver handle
 * @param   enable              BOOL_TRUE to enable, BOOL_FALSE to disable
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to switch gamma-out
 *
 *****************************************************************************/
extern RESULT CamerIcIspGammaOutDisable
(
    CamerIcDrvHandle_t  handle
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function returns the gamma-out status.
 *
 * @param   handle              CamerIc driver handle
 * @param   pIsEnabled          Pointer to value to store current gamma-out
 *                              state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read gamma-out
 *                              status
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcIspIsGammaOutActivated
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pIsEnabled
);
/* @endcond */



/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function sets a gamma out curve.
 *
 * @param   handle              CamerIc driver handle
 * @param   mode                segmentation of the x-axis
 *                              (@ref CamerIcIspGammaSegmentationMode_e) 
 * @param   pCurve              pointer to a gamma-out curve
 *                              (@ref CamerIcGammaCurve_s)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read gamma-out
 *                              status
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcIspSetGammOutCurve
(
    CamerIcDrvHandle_t                      handle,
    const CamerIcIspGammaSegmentationMode_t mode,
    const CamerIcIspGammaCurve_t            *pCurve
);

/*****************************************************************************/
/**
 * @cond    CAMERIC_FULL
 *
 * @brief   This function reads the current gamma out curve.
 *
 * @param   handle              CamerIc driver handle
 * @param   pMode               segmentation of the x-axis
 *                              (@ref CamerIcIspGammaSegmentationMode_e) 
 * @param   pCurve              pointer to a gamma-out curve
 *                              (@ref CamerIcGammaCurve_s)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to read gamma-out
 *                              status
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
extern RESULT CamerIcIspGetGammOutCurve
(
    CamerIcDrvHandle_t                  handle,
    CamerIcIspGammaSegmentationMode_t   *pMode,
    CamerIcIspGammaCurve_t              *pCurve
);

/* @endcond */



#ifdef __cplusplus
}
#endif

/* @} cameric_isp_drv_api */

#endif /* __CAMERIC_ISP_DRV_API_H__ */

