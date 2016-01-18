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
/**
 * @file    cameric_drv_api.h
 *
 * @brief   This file defines the CamerIC driver API
 *
 *****************************************************************************/
/**
 * @defgroup cameric_drv_api CamerIC driver API definitions
 * @{
 *
 *
 */

#ifndef __CAMERIC_DRV_API_H__
#define __CAMERIC_DRV_API_H__

#include <ebase/types.h>

#include <common/return_codes.h>
#include <common/picture_buffer.h>

#include <hal/hal_api.h>

#include <cameric_drv/cameric_drv_common.h>

#ifdef __cplusplus
extern "C"
{
#endif


/******************************************************************************/
/**
 * @brief   Handle type of the CamerIC software driver.
 *
 *****************************************************************************/
typedef struct CamerIcDrvContext_s  *CamerIcDrvHandle_t;


/******************************************************************************/
/**
 * @brief   Enumeration type of the CamerIC features
 *
 *****************************************************************************/
typedef enum CamerIcFeatureIdMask_e
{
    CAMERIC_FEATURE_ID_MASK_INVALID         = 0x00000000UL, /**< lower border (invalid module id) */
    CAMERIC_FEATURE_ID_MASK_PARALLEL_ITF    = 0x00000001UL, /**< has parallel (legacy) interface */
    CAMERIC_FEATURE_ID_MASK_SMIA_ITF        = 0x00000002UL, /**< has SMIA interface */
    CAMERIC_FEATURE_ID_MASK_MIPI0_ITF       = 0x00000004UL, /**< has MIPI-0 interface */
    CAMERIC_FEATURE_ID_MASK_MIPI1_ITF       = 0x00000008UL, /**< has a 2nd MIPI-1 interface */

    CAMERIC_FEATURE_ID_MASK_LAST        
} CamerIcFeatureIdMask_t;


/******************************************************************************/
/**
 * @brief   Enumeration type of the CamerIC drivers top-level modules.
 *
 *****************************************************************************/
typedef enum CamerIcModuleIdMask_e
{
    // main module
    CAMERIC_MODULE_ID_MASK_INVALID          = 0x00000000UL, /**< lower border (invalid module id) */
    CAMERIC_MODULE_ID_MASK_SMIA             = 0x00000001UL, /**< identifier for the SMIA module */
    CAMERIC_MODULE_ID_MASK_MIPI             = 0x00000002UL, /**< identifier for the MIPI module */
    CAMERIC_MODULE_ID_MASK_MIPI_2           = 0x00000004UL, /**< identifier for the 2nd MIPI module */
    CAMERIC_MODULE_ID_MASK_ISP              = 0x00000008UL, /**< identifier for the ISP module */
    CAMERIC_MODULE_ID_MASK_CPROC            = 0x00000010UL, /**< identifier for the image effects module */
    CAMERIC_MODULE_ID_MASK_IE               = 0x00000020UL, /**< identifier for the color processing unit */
    CAMERIC_MODULE_ID_MASK_SIMP             = 0x00000040UL, /**< identifier for the super impose unit */
    CAMERIC_MODULE_ID_MASK_MI               = 0x00000080UL, /**< identifier for the memory interface module */
    CAMERIC_MODULE_ID_MASK_JPE              = 0x00000100UL, /**< identifier for the JPEG encoder block */

    // isp sub modules
    CAMERIC_MODULE_ID_MASK_BLS              = 0x00001000UL, /**< identifier for ISP black level substraction module */
    CAMERIC_MODULE_ID_MASK_DEGAMMA          = 0x00002000UL, /**< identifier for ISP degamma module */
    CAMERIC_MODULE_ID_MASK_LSC              = 0x00004000UL, /**< identifier for ISP lense shade correction module */
    CAMERIC_MODULE_ID_MASK_DPCC             = 0x00008000UL, /**< identifier for ISP defect pixel cluster correction module */
    CAMERIC_MODULE_ID_MASK_DPF              = 0x00010000UL, /**< identifier for ISP denoising pre-filter module */
    CAMERIC_MODULE_ID_MASK_IS               = 0x00020000UL, /**< identifier for ISP image stabilization module */
    CAMERIC_MODULE_ID_MASK_CNR              = 0x00040000UL, /**< identifier for ISP color noise reduction module */
   
    // measuring modules
    CAMERIC_MODULE_ID_MASK_AWB              = 0x01000000UL, /**< identifier for ISP awb measuring module */
    CAMERIC_MODULE_ID_MASK_ELAWB            = 0x02000000UL, /**< identifier for ISP eliptic awb measuring module */
    CAMERIC_MODULE_ID_MASK_EXPOSURE         = 0x04000000UL, /**< identifier for ISP exposure measuring module */
    CAMERIC_MODULE_ID_MASK_HIST             = 0x08000000UL, /**< identifier for ISP histogram measuring module */
    CAMERIC_MODULE_ID_MASK_AFM              = 0x10000000UL, /**< identifier for ISP af measuring module */
    CAMERIC_MODULE_ID_MASK_VSM              = 0x20000000UL, /**< identifier for ISP video stabilization measuring module */

    CAMERIC_MODULE_ID_MASK_LAST
} CamerIcModuleIdMask_t;


/******************************************************************************/
/**
 * @brief   Enumeration type for setting/configuration of the main path muxer
 *          (vi_mp_mux).
 *
 *****************************************************************************/
typedef enum CamerIcMainPathMux_e
{
    CAMERIC_MP_MUX_INVALID                  = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_MP_MUX_JPEG_DIRECT              = 1,        /**< data from DMA read port to JPEG encoder */
    CAMERIC_MP_MUX_MI                       = 2,        /**< data from main resizer/scaler MRSZ to MI */
    CAMERIC_MP_MUX_JPEG                     = 3,        /**< data from main resizer/scaler MRSZ to JPEG encoder */
    CAMERIC_MP_MUX_MAX                                  /**< upper border (only for an internal evaluation) */
} CamerIcMainPathMux_t;


/******************************************************************************/
/**
 * @brief   Enumeration type for setting/configuration of the self path muxer
 *          (please see register definition VI_DMA_SPMUX in user manual).
 *
 *****************************************************************************/
typedef enum CamerIcSelfPathMux_e
{
    CAMERIC_SP_MUX_INVALID                  = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_SP_MUX_CAMERA                   = 1,        /**< data from CAMERA port to self path */
    CAMERIC_SP_MUX_DMA_READ                 = 2,        /**< data from DMA Read port to self path */
    CAMERIC_SP_MUX_MAX                                  /**< upper border (only for an internal evaluation) */
} CamerIcSelfPathMux_t;


/******************************************************************************/
/**
 * @brief   Enumeration type for setting/configuration of the Y/C splitter
 *          (please see register definition VI_CHAN_MODE in user manual).
 *
 *****************************************************************************/
typedef enum CamerIcYcSplitterChannelMode_e
{
    CAMERIC_YCSPLIT_CHMODE_INVALID          = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_YCSPLIT_CHMODE_OFF              = 1,        /**< disable Y/C splitter */
    CAMERIC_YCSPLIT_CHMODE_MP               = 2,        /**< enable mainpath */
    CAMERIC_YCSPLIT_CHMODE_SP               = 3,        /**< enable selfpath */
    CAMERIC_YCSPLIT_CHMODE_MP_SP            = 4,        /**< enable mainpath and selfpath */
    CAMERIC_YCSPLIT_CHMODE_MAX                          /**< upper border (only for an internal evaluation) */
} CamerIcYcSplitterChannelMode_t;


/******************************************************************************/
/**
 * @brief   Enumeration type for setting/configuration of the image effects
 *          muxer (see register definition VI_DMA_IEMUX in user manual).
 *
 *****************************************************************************/
typedef enum CamerIcImgEffectsMux_e
{
    CAMERIC_IE_MUX_INVALID                  = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_IE_MUX_CAMERA                   = 1,        /**< data from CAMERA port to image effects */
    CAMERIC_IE_MUX_DMA_READ                 = 2,        /**< data from DMA Read port to image effects */
    CAMERIC_IE_MUX_MAX                                  /**< upper border (only for an internal evaluation) */
} CamerIcImgEffectsMux_t;


/******************************************************************************/
/**
 * @brief   Enumeration type for setting/configuration of the DMA read switch
 *          (vi_dma_switch).
 *
 *****************************************************************************/
typedef enum CamerIcDmaReadPath_e
{
    CAMERIC_DMA_READ_INVALID                = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_DMA_READ_SPMUX                  = 1,        /**< data feeding in selfpath */
    CAMERIC_DMA_READ_SUPERIMPOSE            = 2,        /**< data feeding in superimpose module */
    CAMERIC_DMA_READ_IMAGE_EFFECTS          = 3,        /**< data feeding in image effects module */
    CAMERIC_DMA_READ_JPEG                   = 4,        /**< data feeding in jpeg encoder */
    CAMERIC_DMA_READ_ISP                    = 5,        /**< data feeding in isp */
    CAMERIC_DMA_READ_MAX                                /**< upper border (only for an internal evaluation) */
} CamerIcDmaReadPath_t;


/******************************************************************************/
/**
 * @brief   Enumeration type for setting/configuration of the input interface
 *          selector (if_select).
 *
 *****************************************************************************/
typedef enum CamerIcInterfaceSelect_e
{
    CAMERIC_ITF_SELECT_INVALID              = 0,        /**< lower border (only for an internal evaluation) */
    CAMERIC_ITF_SELECT_PARALLEL             = 1,        /**< selects the parallel interface */
    CAMERIC_ITF_SELECT_SMIA                 = 2,        /**< selects the SMIA interface */
    CAMERIC_ITF_SELECT_MIPI                 = 3,        /**< selects the MIPI interface */
    CAMERIC_ITF_SELECT_MIPI_2               = 4,        /**< selects the 2nd MIPI interface */
    CAMERIC_ITF_SELECT_MAX                              /**< upper border (only for an internal evaluation) */
} CamerIcInterfaceSelect_t;


/******************************************************************************/
/**
 * @brief   Enumeration of asynchronous command completion IDs 
 *          (callback parameter).
 *
 * @note    This is a list of all asynchronous command supported by CamerIC
 *          driver subsystem.
 *
 *****************************************************************************/
typedef enum CamerIcCommandId_e
{
    CAMERIC_COMMAND_INVALID             = 0x0000,                               /**< invalid command (only for an internal evaluation) */

    /* CamerIc ISP module */
    CAMERIC_ISP_COMMAND_BASE            = 0x0100,                               /**< CamerIc ISP Driver Base Command id */
    CAMERIC_ISP_COMMAND_CAPTURE_FRAMES  = (CAMERIC_ISP_COMMAND_BASE + 0x0000),  /**< frame capturing completed */
    CAMERIC_ISP_COMMAND_STOP_INPUT      = (CAMERIC_ISP_COMMAND_BASE + 0x0001),  /**< stopping completed */

    /* CamerIc MI module */
    CAMERIC_MI_COMMAND_BASE             = 0x0200,                               /**< CamerIc MI Driver Base Command id */
    CAMERIC_MI_COMMAND_DMA_TRANSFER     = (CAMERIC_MI_COMMAND_BASE  + 0x0000)   /**< dma completed */
} CamerIcCommandId_t;


/******************************************************************************/
/**
 * @brief   Completion of asynchronous command callback (function-handler)
 *
 * @note    Callback for signalling the completion of an asynchronous command.
 *          The command identifies the completed command. The content of pParam
 *          depends on the completed command.
 *
 * @return  void
 *
 *****************************************************************************/
typedef void (* CamerIcCompletionFunc_t)
(
    const CamerIcCommandId_t    cmdId,          /**< completed command id */
    const RESULT                result,         /**< result of the completed command */
    void                        *pParam,        /**< parameter structure/argument list of the completed command */
    void                        *pUserContext   /**< user context */
);


/******************************************************************************/
/**
 * @brief   Completion of asynchronous command callback
 *
 * @note    Callback for signalling the completion of an asynchronous command.
 *          The command_id identifies the command. The content of pParameterSet
 *          depends on the completed command.
 *
 *****************************************************************************/
typedef struct CamerIcCompletionCb_s
{
    CamerIcCompletionFunc_t     func;           /**< pointer to callback function */
    void                        *pUserContext;  /**< pointer to the user context */
    void                        *pParam;        /**< pointer to parameter-structure */
} CamerIcCompletionCb_t;


/*****************************************************************************/
/**
 * @brief   Enumeration of asynchronous request IDs (callback parameter).
 *
 * @note    This is a list of all asynchronous requests supported by CamerIC
 *          driver system.
 *
 *****************************************************************************/
typedef enum CamerIcRequestId_e
{
    CAMERIC_REQUEST_INVALID                 = 0x0000,                               /**< invalid request (only for an internal evaluation) */

    /* CamerIc ISP module */
    CAMERIC_ISP_REQUEST_BASE                = 0x0100,                               /**< CamerIc ISP Driver Base Request id */

    /* CamerIc MI module */
    CAMERIC_MI_REQUEST_BASE                 = 0x0200,                               /**< CamerIc MI Driver Base Request id */
    CAMERIC_MI_REQUEST_GET_EMPTY_MP_BUFFER  = (CAMERIC_MI_REQUEST_BASE + 0x0000),   /**< Request id to get an empty buffer for Mainpath */
    CAMERIC_MI_REQUEST_GET_EMPTY_SP_BUFFER  = (CAMERIC_MI_REQUEST_BASE + 0x0001)    /**< Request id to get an empty buffer for Selfpath */
} CamerIcRequestId_t;


/******************************************************************************/
/**
 * @brief   Request callback function definition
 *
 * @note    This callback is used to request something from the application
 *          software, e.g. an input or output buffer.
 *
 * @return  RESULT
 *
 *****************************************************************************/
typedef RESULT (* CamerIcRequestFunc_t)
(
    const CamerIcRequestId_t    reqId,
    void                        **param,
    void                        *pUserContext
);


/*****************************************************************************/
/**
 * @brief   Enumeration of asynchronous event IDs (callback parameter).
 *
 * @note    This is a list of all asynchronous events supported by CamerIC
 *          driver system.
 *
 *****************************************************************************/
typedef enum CamerIcEventId_e
{
    CAMERIC_EVENT_INVALID                   = 0x0000,                               /**< invalid event (only for an internal evaluation) */

    /* CamerIc ISP module */
    CAMERIC_ISP_EVENT_BASE                  = 0x1000,                               /**< base of ISP events */
    CAMERIC_ISP_EVENT_DATA_LOSS             = (CAMERIC_ISP_EVENT_BASE + 0x0000),    /**< data loss */
    CAMERIC_ISP_EVENT_PICTURE_SIZE_ERROR    = (CAMERIC_ISP_EVENT_BASE + 0x0001),    /**< picture size error */
    CAMERIC_ISP_EVENT_FRAME_IN              = (CAMERIC_ISP_EVENT_BASE + 0x0002),    /**< a frame was sampled in */
    CAMERIC_ISP_EVENT_FRAME_OUT             = (CAMERIC_ISP_EVENT_BASE + 0x0003),    /**< a frame was completly transfered to Memory Interface */
    CAMERIC_ISP_EVENT_AWB                   = (CAMERIC_ISP_EVENT_BASE + 0x0004),    /**< AWB survey completed for current frame (a new set of AWB values is available) */
    CAMERIC_ISP_EVENT_HISTOGRAM             = (CAMERIC_ISP_EVENT_BASE + 0x0005),    /**< histogram survey completed for current frame (a new histogram is available) */
    CAMERIC_ISP_EVENT_MEANLUMA              = (CAMERIC_ISP_EVENT_BASE + 0x0006),    /**< luminance survey completed for current frame (a new set of mean luminace values is available */
    CAMERIC_ISP_EVENT_AFM                   = (CAMERIC_ISP_EVENT_BASE + 0x0007),    /**< AF survey completed (a new set of AF values is available) */
    CAMERIC_ISP_EVENT_VSM                   = (CAMERIC_ISP_EVENT_BASE + 0x0008),    /**< VSM measurement completed (param points to a  struct CamerIcIspVsmEventData_t) */

    /* CamerIc MI module */
    CAMERIC_MI_EVENT_BASE                   = 0x2000,                               /**< base of memory interface events */
    CAMERIC_MI_EVENT_FULL_MP_BUFFER         = (CAMERIC_MI_EVENT_BASE + 0x0000),     /**< new frame on mainpath available */
    CAMERIC_MI_EVENT_FULL_SP_BUFFER         = (CAMERIC_MI_EVENT_BASE + 0x0001),     /**< new frame on selfpath available */
    CAMERIC_MI_EVENT_FLUSHED_MP_BUFFER      = (CAMERIC_MI_EVENT_BASE + 0x0002),     /**< delivers a flushed buffer on mainpath */
    CAMERIC_MI_EVENT_FLUSHED_SP_BUFFER      = (CAMERIC_MI_EVENT_BASE + 0x0003),     /**< delivers a flushed buffer on selfpath */
    CAMERIC_MI_EVENT_DROPPED_MP_BUFFER      = (CAMERIC_MI_EVENT_BASE + 0x0004),     /**< informs that a frame was dropped/skipped on mainpath */
    CAMERIC_MI_EVENT_DROPPED_SP_BUFFER      = (CAMERIC_MI_EVENT_BASE + 0x0005),     /**< informs that a frame was dropped/skipped on selfpath */
    
    CAMERIC_JPE_EVENT_BASE                  = 0x3000,                               /**< base of jpeg encode events */
    CAMERIC_JPE_EVENT_HEADER_GENERATED      = (CAMERIC_JPE_EVENT_BASE + 0x0000),    /**< header was generated by jpeg encoder */
    CAMERIC_JPE_EVENT_DATA_ENCODED          = (CAMERIC_JPE_EVENT_BASE + 0x0001)     /**< finished encoding of frame data */
} CamerIcEventId_t;


/******************************************************************************/
/**
 * @brief   Event callback
 *
 * @note    This callback is used to signal something to the application
 *          software, e.g. an error or an information.
 *
 * @return  void
 *
 *****************************************************************************/
typedef void (* CamerIcEventFunc_t)
(
    const CamerIcEventId_t  evtId,
    void                    *param,
    void                    *pUserContext
);


/******************************************************************************/
/**
 * @brief   CamerIC driver configuration structure 
 *
 *****************************************************************************/
typedef struct CamerIcDrvConfig_s
{
    ulong_t                 base;               /**< base address of CamerIC hardware (start of the register map) */

    HalHandle_t             HalHandle;          /**< HAL handle */
    uint32_t                ModuleMask;         /**< mask of all modules to crate/initialize (see @ref CamerIcModuleIdMask_t) */

    CamerIcDrvHandle_t      DrvHandle;          /**< returned driver handle created by @ref CamerIcDriverInit */
} CamerIcDrvConfig_t;


/*****************************************************************************/
/**
 * @brief   This function returnes the .
 *
 * @param   pFeatureMask        
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 *
 *****************************************************************************/
extern RESULT CamerIcDriverFeastureMask
(
    uint32_t *pFeatureMask
);


/*****************************************************************************/
/**
 * @brief   This function initializes and creates the CamerIC driver context.
 *
 * @param   pConfig             configuration of the CamerIC driver
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_OUTOFMEM        not enough memory
 *
 * @note    On succes, this function fills the DrvHandle from the configuration
 *          structure @ref CamerIcDrvConfig_s. This returned DrvHandle has to be
 *          used for later driver calls and should be store in application context.
 *
 *****************************************************************************/
extern RESULT CamerIcDriverInit
(
    CamerIcDrvConfig_t  *pConfig
);


/*****************************************************************************/
/**
 * @brief   This function releases the CamerIC software driver context.
 *
 * @param   *handle             Pointer the the CamerIC driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to release its
 *                              context
 *
 * @note    Allocated memory by @ref CamerIcDriverInit is also released
 *
 *****************************************************************************/
extern RESULT CamerIcDriverRelease
(
    CamerIcDrvHandle_t *handle
);


/*****************************************************************************/
/**
 * @brief   This function starts the CamerIC software driver.
 *
 * @param   handle              CamerIC driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to stop the driver
 *                              (maybe the driver is already running)
 *
 * @note    This function also starts the interrupt handling
 *
 *****************************************************************************/
extern RESULT CamerIcDriverStart
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function stops the CamerIC software driver.
 *
 * @param   handle              CamerIC driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to stop the driver
 *                              (maybe the driver is already stopped)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverStop
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function returns the revision id of the CamerIC.
 *
 * @param   handle              CamerIC driver handle
 * @param   revision            pointer to value to store revision id
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    invalid parameter (revision is NULL pointer)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverGetRevision
(
    CamerIcDrvHandle_t      handle,
    uint32_t                *revision
);


/*****************************************************************************/
/**
 * @brief   This function returns the current IF_SELECT muxer adjustement.
 *
 * @param   handle              CamerIC driver handle
 * @param   IfSelect            pointer to store current adjustement
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    invalid parameter (revision is NULL pointer)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverGetIfSelect
(
    CamerIcDrvHandle_t          handle,
    CamerIcInterfaceSelect_t    *IfSelect
);


/*****************************************************************************/
/**
 * @brief   This function sets/changes the IF_SELECT muxer adjustement
 *
 * @param   handle              CamerIC driver handle
 * @param   IfSelect            new adjustement to setup
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    invalid parameter (revision is NULL pointer)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverSetIfSelect
(
    CamerIcDrvHandle_t              handle,
    const CamerIcInterfaceSelect_t  IfSelect
);


/*****************************************************************************/
/**
 * @brief   This function returns the current VI_DMA_SWITCH muxer adjustement.
 *
 * @param   handle              CamerIC driver handle
 * @param   DmaRead             pointer to store current adjustement
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    invalid parameter (revision is NULL pointer)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverGetViDmaSwitch
(
    CamerIcDrvHandle_t      handle,
    CamerIcDmaReadPath_t    *DmaRead
);


/*****************************************************************************/
/**
 * @brief   This function sets/changes the VI_DMA_SWITCH muxer adjustement
 *
 * @param   handle              CamerIC driver handle
 * @param   DmaRead             new adjustement to setup
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    invalid parameter (revision is NULL pointer)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverSetViDmaSwitch
(
    CamerIcDrvHandle_t          handle,
    const CamerIcDmaReadPath_t  DmaRead
);


/*****************************************************************************/
/**
 * @brief   This function returns the current VI_MP_MUX muxer adjustement.
 *
 * @param   handle              CamerIC driver handle
 * @param   MpMux               pointer to store current adjustement
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    invalid parameter (revision is NULL pointer)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverGetViMpMux
(
    CamerIcDrvHandle_t      handle,
    CamerIcMainPathMux_t    *MpMux
);


/*****************************************************************************/
/**
 * @brief   This function sets/changes the VI_MP_MUX muxer adjustement
 *
 * @param   handle              CamerIC driver handle
 * @param   MpMux               new adjustement to setup
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    invalid parameter (revision is NULL pointer)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverSetViMpMux
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMainPathMux_t  MpMux
);


/*****************************************************************************/
/**
 * @brief   This function configures the datapath trough the CamerIC
 *
 * @param   handle              CamerIC driver handle
 * @param   MpMux               main-path-muxer setup (vi_mp_mux)
 * @param   SpMux               self-path-muxer setup (vi_dma_spmux)
 * @param   YcSplitter          Y/C splitter setup (vi_chan_mode)
 * @param   IeMux               ie-muxer setup (vi_dma_iemux)
 * @param   DmaRead             dma-read muxer setup (vi_dma_switch) 
 * @param   IfSelect            interface select (if_select)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_NOTSUPP         configuration is not supported
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to confuigure datapath
 *                              (maybe the driver is already running)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverSetDataPath
(
    CamerIcDrvHandle_t                      handle,

    const CamerIcMainPathMux_t              MpMux,
    const CamerIcSelfPathMux_t              SpMux,
    const CamerIcYcSplitterChannelMode_t    YcSplitter,
    const CamerIcImgEffectsMux_t            IeMux,
    const CamerIcDmaReadPath_t              DmaRead,
    const CamerIcInterfaceSelect_t          IfSelect
);


/*****************************************************************************/
/**
 * @brief   This function loads a frame into the CamerIC via dma. If the 
 *          frame was loaded the given completion callback is called.
 *
 * @param   handle              CamerIC driver handle
 * @param   pPicBuffer          picture to load into CamerIC
 * @param   pCompletionCb       points to completion callback structure
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_INVALID_PARM    invalid parameter given
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to load a frame
 *                              (maybe the driver is stopped)
 * @retval  RET_BUSY            dma port is busy
 *
 *****************************************************************************/
extern RESULT CamerIcDriverLoadPicture
(
    CamerIcDrvHandle_t      handle,
    PicBufMetaData_t        *pPicBuffer,
    CamerIcCompletionCb_t   *pCompletionCb,
    bool_t                  cont
);


/*****************************************************************************/
/**
 * @brief   This functions starts the CamerIC hardware and driver to capture 
 *          a given number of frames. If the number of frames reached the given 
 *          completion callback is called.
 *
 * @note    A frame number of 0 starts a permanent streaming.
 *
 * @note    This function is asynchronous so it returns RET_PENDING on success.
 *
 * @param   handle              CamerIC driver handle
 * @param   numFrames           number of frames to capture
 * @param   pCompletionCb       points to completion callback structure
 *
 * @return                      Return the result of the function call.
 * @retval  RET_PENDING         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_INVALID_PARM    invalid parameter given
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to load a frame
 *                              (maybe the driver is stopped)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverCaptureFrames
(
    CamerIcDrvHandle_t      handle,
    const uint32_t          numFrames,
    CamerIcCompletionCb_t   *pCompletionCb
);


/*****************************************************************************/
/**
 * @brief   This functions (re)starts the CamerIC ISP hardware after it was
 *          stopped.
 *
 * @param   handle              CamerIC driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to restart streaming
 *                              (maybe the driver is stopped)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverStartInput
(
    CamerIcDrvHandle_t      handle
);


/*****************************************************************************/
/**
 * @brief   This functions stops the CamerIC ISP hardware at end of the 
 *          current frame.
 *
 * @note    This function is asynchronous so it returns RET_PENDING on success.
 *
 * @param   handle              CamerIC driver handle
 * @param   pCompletionCb       points to completion callback structure
 *
 * @return                      Return the result of the function call.
 * @retval  RET_PENDING         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_INVALID_PARM    invalid parameter given
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to stop streaming
 *                              (maybe the driver is stopped)
 *
 *****************************************************************************/
extern RESULT CamerIcDriverStopInput
(
    CamerIcDrvHandle_t      handle,
    CamerIcCompletionCb_t   *pCompletionCb
);


void CamerIcDriverSetIsSwapByte
(
    CamerIcDrvHandle_t      handle,
    bool_t  swapbyte
);

uint32_t CamerIcSetDataPathWhileStreaming
(
    CamerIcDrvHandle_t      handle,
    CamerIcWindow_t* pWin,
    uint32_t outWidth, 
    uint32_t outHeight
    
);


#ifdef __cplusplus
}
#endif

/* @} cameric_drv_api */

#endif /* __CAMERIC_DRV_API_H__ */

