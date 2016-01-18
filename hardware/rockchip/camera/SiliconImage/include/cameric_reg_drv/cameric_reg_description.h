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
#ifndef __CAMERIC_REG_DESCRIPTION_API_H__
#define __CAMERIC_REG_DESCRIPTION_API_H__

/**
 * @file cameric_reg_description_api.h
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
 * @defgroup cameric_isp_wdr_drv_api CamerIc ISP Driver API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <hal/hal_api.h>


/******************************************************************************/
/**
 *          CamerIcModuleId_t
 *
 * @brief   CamerIc Module Identifier
 *
 *****************************************************************************/
typedef enum CamerIcModuleId_e
{
    CAMERIC_MODULE_INVALID      =  0,   /* invalid module identifier (for range check) */
    CAMERIC_MODULE_MAIN_CONTROL =  1,   /* CamerIc Sub-Module: MAIN_CONTROL */
    CAMERIC_MODULE_IMG_EFFECTS  =  2,   /* CamerIc Sub-Module: IMAGE_EFFECTS */
    CAMERIC_MODULE_SUPER_IMPOSE =  3,   /* CamerIc Sub-Module: SUPER_IMPOSE */
    CAMERIC_MODULE_ISP_MAIN     =  4,   /* CamerIc Sub-Module: ISP_MAIN */
    CAMERIC_MODULE_ISP_FLASH    =  5,   /* CamerIc Sub-Module: ISP_FLASH */
    CAMERIC_MODULE_ISP_SHUTTER  =  6,   /* CamerIc Sub-Module: ISP_SHUTTER */
    CAMERIC_MODULE_CPROC        =  7,   /* CamerIc Sub-Module: COLOR_PROCESSING */
    CAMERIC_MODULE_DCROP        =  8,   /* CamerIc Sub-Module: MAIN_PATH_RESIZER */
    CAMERIC_MODULE_MRSZ         =  9,   /* CamerIc Sub-Module: MAIN_PATH_RESIZER */
    CAMERIC_MODULE_SRSZ         = 10,   /* CamerIc Sub-Module: SELF_PATH_RESIZER */
    CAMERIC_MODULE_MI           = 11,   /* CamerIc Sub-Module: MEMORY_INTERFACE */
    CAMERIC_MODULE_JPE          = 12,   /* CamerIc Sub-Module: JPEG_ENCODER */
    CAMERIC_MODULE_SMIA         = 13,   /* CamerIc Sub-Module: SMIA */
    CAMERIC_MODULE_MIPI         = 14,   /* CamerIc Sub-Module: MIPI */
    CAMERIC_MODULE_MIPI2        = 15,   /* CamerIc Sub-Module: MIPI */
    CAMERIC_MODULE_ISP_AFM      = 16,   /* CamerIc Sub-Module: ISP_AUTOFOCUS_MEASUREMENT */
    CAMERIC_MODULE_ISP_BP       = 17,   /* CamerIc Sub-Module: ISP_BAD_PIXEL (normally replaced by DPCC) */
    CAMERIC_MODULE_ISP_LSC      = 18,   /* CamerIc Sub-Module: ISP_LENSE_SHADE_CORRECTION */
    CAMERIC_MODULE_ISP_IS       = 19,   /* CamerIc Sub-Module: ISP_IMAGE_STABILIZATION */
    CAMERIC_MODULE_ISP_HIST     = 20,   /* CamerIc Sub-Module: ISP_HISTOGRAM */
    CAMERIC_MODULE_ISP_FILTER   = 21,   /* CamerIc Sub-Module: ISP_FILTER */
    CAMERIC_MODULE_ISP_CAC      = 22,   /* CamerIc Sub-Module: ISP_CHROMATIC_ABERRATION_CORRECTION */
    CAMERIC_MODULE_ISP_CNR      = 23,   /* CamerIc Sub-Module: ISP_CHROMA_NOISE_REDUCTION */
    CAMERIC_MODULE_ISP_EXP      = 24,   /* CamerIc Sub-Module: ISP_EXPOSURE */
    CAMERIC_MODULE_ISP_BLS      = 25,   /* CamerIc Sub-Module: ISP_BLACK_LEVEL_SUBSTRACTION */
    CAMERIC_MODULE_ISP_DPF      = 26,   /* CamerIc Sub-Module: ISP_DENOISING_PRE_FFILTER */
    CAMERIC_MODULE_ISP_DPCC     = 27,   /* CamerIc Sub-Module: ISP_DEFECT_PIXEL_CLUSTER_CORRECTION */
    CAMERIC_MODULE_ISP_WDR      = 28,   /* CamerIc Sub-Module: ISP_WHITE_DYNAMIC_RANGE */
    CAMERIC_MODULE_ISP_AWB      = 29,   /* CamerIc Sub-Module: ISP_AUTO_WHITE_BALANCE_MEASUREMENT */
    CAMERIC_MODULE_ISP_VSM      = 30,   /* CamerIc Sub-Module: ISP_AUTO_VIDEO_STABILIZATION_MEASUREMENT */
    CAMERIC_MODULE_MAX                  /* max module identifier (for range check) */
} CamerIcModuleId_t;



/******************************************************************************/
/**
 *          Permissions_t
 *
 * @brief   Type which defines the register-permission
 *
 *****************************************************************************/
typedef enum Permissions_e
{
    PERM_INVALID        = 0,    /**< invalid permission */
    PERM_READ_ONLY      = 1,    /**< read-only register */
    PERM_READ_WRITE     = 2,    /**< read-writeable register */
    PERM_WRITE_ONLY     = 3     /**< write-only register */
} Permissions_t;



/******************************************************************************/
/**
 *          RegName_t
 *
 * @brief   Type for defining the register name 
 *
 *****************************************************************************/
typedef char RegName_t[30];



/******************************************************************************/
/**
 *          RegName_t
 *
 * @brief   Type for defining the register name 
 *
 *****************************************************************************/
typedef char RegHint_t[100];



/******************************************************************************/
/**
 *          RegDescription_t
 *
 * @brief   Type for defining a register description
 *
 *****************************************************************************/
typedef struct RegDescription_s
{
    ulong_t         Address;            /**< adddress of the register */
    Permissions_t   Permission;         /**< access mode to the register */
    RegName_t       Name;               /**< name string of the register */
    RegHint_t       Hint;               /**< hint string of the register */
    uint32_t        ResetValue;         /**< reset value of the register */
    uint32_t        UsedBits;           /**< bitmask of used bits */
    uint32_t        WriteAbleBits;      /**< bitmask of writeable bits */
} RegDescription_t;



/******************************************************************************/
/**
 *          CamerIcDrvConfig_t
 *
 * @brief   Configuration to setup CamerIc Register Description Driver
 *
 *****************************************************************************/
typedef struct CamerIcRegDescriptionDrvConfig_s
{
    HalHandle_t     HalHandle;          /**< HAL handle to access CamerIc Hardware */
} CamerIcRegDescriptionDrvConfig_t;



/*******************************************************************************
 *
 *          CamerIcInitRegDescriptionDrv
 *  
 * @brief   Initializes the CamerIc Register Description driver
 *
 * @param	pConfig    pointes to configuration structure
 *
 * @return	Return the result of the function call.
 * @retval	RET_SUCCESS		
 * @retval	RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcInitRegDescriptionDrv
(
    const CamerIcRegDescriptionDrvConfig_t  *pConfig
);



/*******************************************************************************
 *
 *          CamerIcReleaseRegDescriptionDrv
 *  
 * @brief   Releases the CamerIc Register Description driver
 *
 * @param	none
 *
 * @return	Return the result of the function call.
 * @retval	RET_SUCCESS		
 * @retval	RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcReleaseRegDescriptionDrv
(
    void
);



/*******************************************************************************
 *
 *          CamerIcGetRegister
 *  
 * @brief   Returns the register content 
 *
 * @param	address     register to read
 *          value       pointer to a value
 *
 * @return	Return the result of the function call.
 * @retval	RET_SUCCESS		
 * @retval	RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcGetRegister
(
    const ulong_t   address,
    uint32_t        *value
);



/*******************************************************************************
 *
 *          CamerIcSetRegister
 *  
 * @brief   Set register content 
 *
 * @param	address     register to set
 *          value       value to write
 *
 * @return	Return the result of the function call.
 * @retval	RET_SUCCESS		
 * @retval	RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcSetRegister
(
    const ulong_t   address,
    const uint32_t  value
);



/*******************************************************************************
 *
 *          CamerIcGetRegisterMap
 *  
 * @brief  	Returns the number and Register-Description of a CamerIc-Submodule
 *
 * @param[in]   Module          Module Identifier
 * @param[out]  NumRegisters    Number of Registers
 * @param[out]  Registers       Pointer to Register-Description
 *
 * @return	Return the result of the function call.
 * @retval	RET_SUCCESS		
 * @retval	RET_FAILURE
 * @return 	RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT CamerIcGetRegisterDescription
(
    const CamerIcModuleId_t ModuleId,
    uint32_t                *NumRegisters,
    RegDescription_t        **Registers
);



#ifdef __cplusplus
}
#endif

/* @} cameric_isp_wdr_drv_api */

#endif /* __CAMERIC_REG_DESCRIPTION_API_H__ */

