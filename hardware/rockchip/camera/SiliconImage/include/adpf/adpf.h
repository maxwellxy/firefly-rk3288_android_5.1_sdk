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
#ifndef __ADPF_H__
#define __ADPF_H__

/**
 * @file adpf.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup ADPF Auto denoising pre-filter module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>



#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 *          AdpfHandle_t
 *
 * @brief   ADPF Module instance handle
 *
 *****************************************************************************/
typedef struct AdpfContext_s *AdpfHandle_t;         /**< handle to ADPF context */



/*****************************************************************************/
/**
 * @brief   A structure/tupple to represent gain values for four (R,Gr,Gb,B)
 *          channels.
 *
 * @note    The gain values are represented as float numbers.
 */
/*****************************************************************************/
typedef struct AdpfGains_s
{
    float   fRed;                               /**< gain value for the red channel */
    float   fGreenR;                            /**< gain value for the green channel in red lines */
    float   fGreenB;                            /**< gain value for the green channel in blue lines */
    float   fBlue;                              /**< gain value for the blue channel */
} AdpfGains_t;



/*****************************************************************************/
/**
 *          AdpfInstanceConfig_t
 *
 * @brief   ADPF Module instance configuration structure
 *
 *****************************************************************************/
typedef struct AdpfInstanceConfig_s
{
    AdpfHandle_t            hAdpf;              /**< handle returned by AdpfInit() */
} AdpfInstanceConfig_t;



/*****************************************************************************/
/**
 *          AdpfConfigType_t
 *
 * @brief   ADPF Configuration type
 *
 *****************************************************************************/
typedef enum AdpfConfigType_e
{
    ADPF_USE_CALIB_INVALID  = 0,                /**< invalid (could be zeroed memory) */
    ADPF_USE_CALIB_DATABASE = 1,
    ADPF_USE_DEFAULT_CONFIG = 2
} AdpfConfigType_t;


/*****************************************************************************/
/**
 *          AdpfConfig_t
 *
 * @brief   ADPF Module configuration structure
 *
 *****************************************************************************/
typedef struct AdpfConfig_s
{
    float                           fSensorGain;        /**< initial sensor gain */

    CamerIcDrvHandle_t              hCamerIc;           /**< handle to cameric driver */
    CamerIcDrvHandle_t              hSubCamerIc;        /**< handle to 2nd cameric drivder (3D) */

    AdpfConfigType_t                type;               /**< configuration type */
    union AdpfConfigData_u
    {
        struct AdpfDefaultConfig_s
        {
            uint32_t                SigmaGreen;         /**< sigma value for green pixel */
            uint32_t                SigmaRedBlue;       /**< sigma value for red/blue pixel */
            float                   fGradient;          /**< gradient value for dynamic strength calculation */
            float                   fOffset;            /**< offset value for dynamic strength calculation */
            float                   fMin;               /**< upper bound for dynamic strength calculation */
            float                   fDiv;               /**< division factor for dynamic strength calculation */
            AdpfGains_t             NfGains;            /**< noise function gains */
        } def;

        struct AdpfDatabaseConfig_s
        {
            uint16_t                width;              /**< picture width */
            uint16_t                height;             /**< picture height */
            uint16_t                framerate;          /**< frame rate */
            CamCalibDbHandle_t      hCamCalibDb;        /**< calibration database handle */
        } db;
    } data;
} AdpfConfig_t;



/*****************************************************************************/
/**
 * @brief   This function converts float based gains into CamerIC 4.8 fixpoint
 *          format.
 *
 * @param   pAdpfGains          gains in float based format
 * @param   pCamerIcGains       gains in fix point format
 *
 * @return                      Returns the result of the function call.
 * @retval  RET_SUCCESS         gains sucessfully converted
 * @retval  RET_NULL_POINTER    null pointer parameter
 *
 *****************************************************************************/
RESULT AdpfGains2CamerIcGains
(
    AdpfGains_t     *pAdpfGains,
    CamerIcGains_t  *pCamerIcGains
);



/*****************************************************************************/
/**
 * @brief   This function converts CamerIC 4.8 fixpoint format into float
 *          based gains.
 *
 * @param   pCamerIcGains       gains in fix point format
 * @param   pAdpfGains          gains in float based format
 *
 * @return                      Returns the result of the function call.
 * @retval  RET_SUCCESS         gains sucessfully converted
 * @retval  RET_NULL_POINTER    null pointer parameter
 *
 *****************************************************************************/
RESULT CamerIcGains2AdpfGains
(
    CamerIcGains_t  *pCamerIcGains,
    AdpfGains_t     *pAdpfGains
);



/*****************************************************************************/
/**
 *          AdpfInit()
 *
 * @brief   This function initializes the Auto denoising pre-filter module
 *
 * @param   pInstConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_INVALID_PARM
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT AdpfInit
(
    AdpfInstanceConfig_t *pInstConfig
);



/*****************************************************************************/
/**
 *          AdpfRelease()
 *
 * @brief   The function releases/frees the Auto denoising pre-filter module
 *
 * @param   handle  Handle to ADPFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpfRelease
(
    AdpfHandle_t handle
);



/*****************************************************************************/
/**
 *          AdpfConfigure()
 *
 * @brief   This function configures the Auto denoising pre-filter module
 *
 * @param   handle  Handle to ADPFM
 * @param   pConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AdpfConfigure
(
    AdpfHandle_t handle,
    AdpfConfig_t *pConfig
);



/*****************************************************************************/
/**
 *          AdpfReConfigure()
 *
 * @brief   This function re-configures the Auto Denoising Pre-Filter Module
 *          after e.g. resolution change
 *
 * @param   handle  Handle to ADPFM
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AdpfReConfigure
(
    AdpfHandle_t handle,
    AdpfConfig_t *pConfig
);



/*****************************************************************************/
/**
 *          AdpfStart()
 *
 * @brief   The function starts the Auto denoising pre-filter module
 *
 * @param   handle  Handle to ADPFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpfStart
(
    AdpfHandle_t handle
);



/*****************************************************************************/
/**
 *          AdpfStop()
 *
 * @brief   The function stops the Auto denoising pre-filter module
 *
 * @param   handle  Handle to ADPFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpfStop
(
    AdpfHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function returns the current configuration.
 *
 * @param   handle      ADPF instance handle
 * @param   pConfig     reference of configuration structure to be filled with
 *                      the current configuration
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT AdpfGetCurrentConfig
(
    AdpfHandle_t handle,
    AdpfConfig_t *pConfig
);


/*****************************************************************************/
/**
 *          AdpfStop()
 *
 * @brief   The function returns the status of the Auto denoising pre-filter
 *          module
 *
 * @param   handle  Handle to ADPFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpfStatus
(
    AdpfHandle_t handle,
    bool_t       *pRunning
);



/*****************************************************************************/
/**
 *          AdpfProcessFrame()
 *
 * @brief   The function calculates and adjusts a new DPF-setup regarding
 *          the current sensor-gain
 *
 * @param   handle  Handle to ADPFM
 *          gain    current sensor-gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpfProcessFrame
(
    AdpfHandle_t    handle,
    const float     gain
);



#ifdef __cplusplus
}
#endif


/* @} ADPF */


#endif /* __ADPF_H__*/
