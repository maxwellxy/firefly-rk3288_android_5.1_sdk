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
#ifndef __ADPCC_H__
#define __ADPCC_H__

/**
 * @file adpcc.h
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
 * @defgroup ADPCC Auto defect pixel correction
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
 *          AdpccHandle_t
 *
 * @brief   ADPCC Module instance handle
 *
 *****************************************************************************/
typedef struct AdpccfContext_s *AdpccHandle_t;      /**< handle to ADCC context */



/*****************************************************************************/
/**
 *          AdpccInstanceConfig_t
 *
 * @brief   ADPCC Module instance configuration structure
 *
 *****************************************************************************/
typedef struct AdpccInstanceConfig_s
{
    AdpccHandle_t           hAdpcc;                 /**< handle returned by AdpccInit() */
} AdpccInstanceConfig_t;



/*****************************************************************************/
/**
 *          AdpccfConfigType_t
 *
 * @brief   ADPCC Configuration type
 *
 *****************************************************************************/
typedef enum AdpccConfigType_e
{
    ADPCC_USE_CALIB_INVALID  = 0,                   /**< invalid (could be zeroed memory) */
    ADPCC_USE_CALIB_DATABASE = 1,
    ADPCC_USE_DEFAULT_CONFIG = 2
} AdpccConfigType_t;



/*****************************************************************************/
/**
 *          AdpccConfig_t
 *
 * @brief   ADPCC Module configuration structure
 *
 *****************************************************************************/
typedef struct AdpccConfig_s
{
    float                           fSensorGain;        /**< initial start gain */
    
    CamerIcDrvHandle_t              hCamerIc;               /**< handle to cameric driver */
    CamerIcDrvHandle_t              hSubCamerIc;            /**< handle to 2nd cameric drivder (3D) */

    AdpccConfigType_t               type;               /**< configuration type */
    union AdpccConfigData_u
    {
        struct AdpccDefaultConfig_s
        {
            uint32_t                isp_dpcc_mode;
            uint32_t                isp_dpcc_output_mode;
            uint32_t                isp_dpcc_set_use;
            uint32_t                isp_dpcc_methods_set_1;
            uint32_t                isp_dpcc_methods_set_2;
            uint32_t                isp_dpcc_methods_set_3;
            uint32_t                isp_dpcc_line_thresh_1;
            uint32_t                isp_dpcc_line_mad_fac_1;
            uint32_t                isp_dpcc_pg_fac_1;
            uint32_t                isp_dpcc_rnd_thresh_1;
            uint32_t                isp_dpcc_rg_fac_1;
            uint32_t                isp_dpcc_line_thresh_2;
            uint32_t                isp_dpcc_line_mad_fac_2;
            uint32_t                isp_dpcc_pg_fac_2;
            uint32_t                isp_dpcc_rnd_thresh_2;
            uint32_t                isp_dpcc_rg_fac_2;
            uint32_t                isp_dpcc_line_thresh_3;
            uint32_t                isp_dpcc_line_mad_fac_3;
            uint32_t                isp_dpcc_pg_fac_3;
            uint32_t                isp_dpcc_rnd_thresh_3;
            uint32_t                isp_dpcc_rg_fac_3;
            uint32_t                isp_dpcc_ro_limits;
            uint32_t                isp_dpcc_rnd_offs;
        } def;

        struct AdpccDatabaseConfig_s
        {
            uint16_t                width;              /**< picture width */
            uint16_t                height;             /**< picture height */
            uint16_t                framerate;          /**< frame rate */
            CamCalibDbHandle_t      hCamCalibDb;        /**< calibration database handle */
        } db;
    } data;
} AdpccConfig_t;



/*****************************************************************************/
/**
 *          AdpccInit()
 *
 * @brief   This function initializes the Auto defect pixel correction module
 *
 * @param   pInstConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_INVALID_PARM
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT AdpccInit
(
    AdpccInstanceConfig_t *pInstConfig
);



/*****************************************************************************/
/**
 *          AdpccRelease()
 *
 * @brief   The function releases/frees the Auto defect pixel correction module
 *
 * @param   handle  Handle to ADPCCM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpccRelease
(
    AdpccHandle_t handle
);



/*****************************************************************************/
/**
 *          AdpccConfigure()
 *
 * @brief   This function configures the Auto defect pixel correction module
 *
 * @param   handle  Handle to ADPCCM
 * @param   pConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AdpccConfigure
(
    AdpccHandle_t handle,
    AdpccConfig_t *pConfig
);



/*****************************************************************************/
/**
 *          AdpccReConfigure()
 *
 * @brief   This function re-configures the Auto Defect Pixel Correction Module
 *          after e.g. resolution change
 *
 * @param   handle  Handle to ADPCCM
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AdpccReConfigure
(
    AdpccHandle_t handle,
    AdpccConfig_t *pConfig
);



/*****************************************************************************/
/**
 *          AdpccStart()
 *
 * @brief   The function starts the Auto defect pixel correction module
 *
 * @param   handle  Handle to ADPCCM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpccStart
(
    AdpccHandle_t handle
);



/*****************************************************************************/
/**
 *          AdpccStop()
 *
 * @brief   The function stops the Auto defect pixel correction module
 *
 * @param   handle  Handle to AECM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpccStop
(
    AdpccHandle_t handle
);


/*****************************************************************************/
/**
 *          AdpccStatus()
 *
 * @brief   The function returns the status of the Auto defect pixel correction
 *          module
 *
 * @param   handle  Handle to AECM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpccStatus
(
    AdpccHandle_t   handle,
    bool_t          *pRunning
);



/*****************************************************************************/
/**
 *          AdpccProcessFrame()
 *
 * @brief   The function calculates and adjusts a new DPCC-setup regarding
 *          the current sensor-gain
 *
 * @param   handle  Handle to AECM
 *          gain    current sensor-gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AdpccProcessFrame
(
    AdpccHandle_t   handle,
    const float     gain
);



#ifdef __cplusplus
}
#endif


/* @} ADPCC */


#endif /* __ADPCC_H__*/
