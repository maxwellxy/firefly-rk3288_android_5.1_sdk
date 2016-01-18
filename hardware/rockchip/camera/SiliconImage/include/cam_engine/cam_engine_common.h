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
 * @file cam_engine_common.h
 *
 * @brief
 *   Common definitions of the CamEngine.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_engine_common CamEngine Common Definitions
 * @{
 *
 */
#ifndef __CAM_ENGINE_COMMON_H__
#define __CAM_ENGINE_COMMON_H__

#include <bufferpool/media_buffer.h>
#include <cameric_drv/cameric_mi_drv_api.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/**
 * @brief   Handle to a CamEngine instance.
 *
 *****************************************************************************/
typedef struct CamEngineContext_s *CamEngineHandle_t;


/*****************************************************************************/
/**
 * @brief   Commands for the CamEngine
 *
 *****************************************************************************/
typedef enum CamEngineCmdId_e
{
    CAM_ENGINE_CMD_INVALID                  = 0,    /**< invalid command (only for initialization) */
    CAM_ENGINE_CMD_START                    = 1,    /**< start a cam-engine instance */
    CAM_ENGINE_CMD_STOP                     = 2,    /**< stop a cam-engine instance */
    CAM_ENGINE_CMD_SHUTDOWN                 = 3,    /**< shutdown a stopped cam-engine instance */

    CAM_ENGINE_CMD_START_STREAMING          = 4,    /**< start streaming */
    CAM_ENGINE_CMD_STOP_STREAMING           = 5,    /**< stop streaming */

    CAM_ENGINE_CMD_ACQUIRE_LOCK             = 6,    /**< locks the auto algorithms */
    CAM_ENGINE_CMD_RELEASE_LOCK             = 7,    /**< releases locks of the auto algorithms */

    CAM_ENGINE_CMD_INTERNAL_BASE            = 1000,                                 /**< base for internal commands */
    CAM_ENGINE_CMD_AAA_LOCKED               = (CAM_ENGINE_CMD_INTERNAL_BASE + 0),   /**< selected auto-algorithms are locked now */ 
    CAM_ENGINE_CMD_HW_STREAMING_FINISHED    = (CAM_ENGINE_CMD_INTERNAL_BASE + 20),  /**< send by CamerIc hw, if ISP disabled (streaming finished) */
    CAM_ENGINE_CMD_HW_DMA_FINISHED          = (CAM_ENGINE_CMD_INTERNAL_BASE + 21),  /**< send by CamerIc hw, if DMA transfer completed */
    CAM_ENGINE_CMD_HW_JPE_DATA_ENCODED      = (CAM_ENGINE_CMD_INTERNAL_BASE + 22),  /**< send by CamerIc hw, if JPE data encoded */

    CAM_ENGINE_CMD_MAX
} CamEngineCmdId_t;


/*****************************************************************************/
/**
 * @brief States of the CamEngine.
 *
 *****************************************************************************/
typedef enum CamEngineState_e
{
    CAM_ENGINE_STATE_INVALID        = 0,    /**< FSM state is invalid since CamEngine instance does not exist. */
    CAM_ENGINE_STATE_INITIALIZED    = 1,    /**< FSM is in state initialized. */
    CAM_ENGINE_STATE_RUNNING        = 2,    /**< FSM is in state running. */
    CAM_ENGINE_STATE_STREAMING      = 3,    /**< FSM is in state streaming. */
    CAM_ENGINE_STATE_MAX
} CamEngineState_t;


/*****************************************************************************/
/**
 * @brief Chain index of the CamEngine.
 *
 *****************************************************************************/
typedef enum CamEngineChainIdx_e
{
    CHAIN_INVALID   = -1,
    CHAIN_MASTER    = 0,
    CHAIN_SLAVE     = 1,
    CHAIN_MAX       = 2
} CamEngineChainIdx_t;


/*****************************************************************************/
/**
 * @brief Processing paths of the CamEngine.
 *
 *****************************************************************************/
typedef enum CamEnginePathType_e
{
    CAM_ENGINE_PATH_INVALID = -1,
    CAM_ENGINE_PATH_MAIN    =  0,
    CAM_ENGINE_PATH_SELF    =  1,
    CAM_ENGINE_PATH_MAX
} CamEnginePathType_t;


/*****************************************************************************/
/**
 * @brief Scenario mode type
 *
 *****************************************************************************/
typedef enum CamEngineModeType_e
{
    CAM_ENGINE_MODE_INVALID             = 0,
    CAM_ENGINE_MODE_SENSOR_2D           = 1,        /**< mode: 2D (single pipe) */
    CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB   = 2,        /**< mode: 2D (with video stabilization) */
    CAM_ENGINE_MODE_SENSOR_3D           = 3,        /**< mode: 3D (dual pipe) */
    CAM_ENGINE_MODE_IMAGE_PROCESSING    = 4,        /**< mode: image rendering/processing */
    CAM_ENGINE_MODE_MAX
} CamEngineModeType_t;


/*****************************************************************************/
/**
 * @brief Input config types of the CamEngine.
 *
 *****************************************************************************/
typedef enum CamEngineConfigType_e
{
    CAM_ENGINE_CONFIG_INVALID = 0,
    CAM_ENGINE_CONFIG_SENSOR  = 1,
    CAM_ENGINE_CONFIG_IMAGE   = 2,
    CAM_ENGINE_CONFIG_MAX
#if 0
    CAM_ENGINE_CONFIG_SCENARIO_INVALID      = 0,    /**< invalid configuration type */
    CAM_ENGINE_CONFIG_SCENARIO_SENSOR_2D    = 1,    /**< configuration type for 2D mode (single pipe) */
    CAM_ENGINE_CONFIG_SCENARIO_SENSOR_3D    = 2,    /**< configuration type for 3D mode (dual pipe) */
    CAM_ENGINE_CONFIG_SCENARIO_IMAGE_PROC   = 3,    /**< configuration type for image processing */
    CAM_ENGINE_CONFIG_SCENARIO_IMG_STAB     = 4,    /**< configuration type for image stabilization scenario */
    CAM_ENGINE_CONFIG_SCENARIO_MAX                  /**< max configuration type for range check */
#endif    
} CamEngineConfigType_t;


/*****************************************************************************/
/**
 * @brief Lock types for the auto algorithms. Can be OR combined.
 *
 *****************************************************************************/
typedef enum CamEngineLockType_e
{
    CAM_ENGINE_LOCK_NO      = 0x00,
    CAM_ENGINE_LOCK_AF      = 0x01,
    CAM_ENGINE_LOCK_AEC     = 0x02,
    CAM_ENGINE_LOCK_AWB     = 0x04,

    CAM_ENGINE_LOCK_ALL     = ( CAM_ENGINE_LOCK_AF | CAM_ENGINE_LOCK_AEC | CAM_ENGINE_LOCK_AWB )
} CamEngineLockType_t;


/*****************************************************************************/
/**
 * @brief Flicker period types for the AEC algorithm.
 *
 *****************************************************************************/
typedef enum CamEngineFlickerPeriod_e
{
    CAM_ENGINE_FLICKER_OFF   = 0x00,
    CAM_ENGINE_FLICKER_100HZ = 0x01,
    CAM_ENGINE_FLICKER_120HZ = 0x02
} CamEngineFlickerPeriod_t;

typedef enum CamEngineFlashMode_e
{
    CAM_ENGINE_FLASH_OFF = 0x00,
    CAM_ENGINE_FLASH_AUTO = 0x01,
    CAM_ENGINE_FLASH_ON = 0x02,
    CAM_ENGINE_FLASH_RED_EYE = 0x03,
    CAM_ENGINE_FLASH_TORCH = 0x05
} CamEngineFlashMode_t;

typedef enum CamEngineFlashTriggerPol_e
{
    CAM_ENGINE_FLASH_LOW_ACTIVE = 0x0,
    CAM_ENGINE_FLASH_HIGH_ACTIVE = 0x1
}CamEngineFlashTriggerPol_t;

typedef struct CamEngineFlashCfg_s 
{
    CamEngineFlashMode_t mode;  
    CamEngineFlashTriggerPol_t active_pol;
    int32_t flashtype;
    unsigned int dev_mask; 
} CamEngineFlashCfg_t;
/*****************************************************************************/
/**
 *  @brief Command completion signaling callback
 *
 *  Callback for signaling command completion which could require application
 *  interaction. The cmdId (see @ref CamEngineCmdId_t) identifies the completed
 *  command.
 *
 *****************************************************************************/
typedef void (* CamEngineCompletionCb_t)
(
    CamEngineCmdId_t    cmdId,          /**< command Id of the notifying event */
    RESULT              result,         /**< result of the executed command */
    const void          *pUserCbCtx     /**< user data pointer that was passed on creation (see @ref CamEngineInstanceConfig_t) */
);


/*****************************************************************************/
/**
 *  @brief AFPS resolution change request signaling callback
 *
 *  Callback for signaling an AFPS resolution (better: frame rate) change
 *  request to the application.
 *
 *****************************************************************************/
typedef void (*CamEngineAfpsResChangeCb_t)
(
    uint32_t            NewResolution,  /**< new resolution to switch to */
    const void          *pUserCbCtx     /**< user data pointer that was passed on creation (see @ref CamEngineInstanceConfig_t) */
);


/*****************************************************************************/
/**
 *  @brief Full buffer signaling callback
 *
 *  Callback for signaling a full buffer which should be handled by the
 *  application. The path (see @ref CamEnginePath_t) identifies the output
 *  path.
 *
 *****************************************************************************/
typedef void (*CamEngineBufferCb_t)
(
    CamEnginePathType_t path,           /**< output path of the media buffer */
    MediaBuffer_t       *pMediaBuffer,  /**< full media buffer */
    void                *pBufferCbCtx   /**< user data pointer that was passed on registering the callback (see @ref CamEngineRegisterBufferCb) */
);


/*****************************************************************************/
/**
 * @brief   Generic structure to define a window.
 *
 *****************************************************************************/
typedef struct CamEngineWindow_s
{
    uint16_t    hOffset;
    uint16_t    vOffset;
    uint16_t    width;
    uint16_t    height;
} CamEngineWindow_t;


/*****************************************************************************/
/**
 * @brief   Generic structure to define a vector.
 *
 *****************************************************************************/
typedef struct CamEngineVector_s
{
    int16_t   x;
    int16_t   y;
} CamEngineVector_t;


/*****************************************************************************/
/**
 * @brief   Generic structure for the white balance gains of the four color
 *          components.
 *
 *****************************************************************************/
typedef struct CamEngineWbGains_s
{
    float Red;
    float GreenR;
    float GreenB;
    float Blue;
} CamEngineWbGains_t;


/*****************************************************************************/
/**
 * @brief   Generic structure for the cross talk matrix of the four color
 *          components.
 *
 *****************************************************************************/
typedef struct CamEngineCcMatrix_s
{
    float Coeff[9U];
} CamEngineCcMatrix_t;


/*****************************************************************************/
/**
 * @brief   Generic structure for the cross talk offset of the four color
 *          components.
 *
 *****************************************************************************/
typedef struct CamEngineCcOffset_s
{
    int16_t Red;
    int16_t Green;
    int16_t Blue;
} CamEngineCcOffset_t;


/*****************************************************************************/
/**
 * @brief   Generic structure for the black level of the four color components.
 *
 *****************************************************************************/
typedef struct CamEngineBlackLevel_s
{
    uint16_t Red;
    uint16_t GreenR;
    uint16_t GreenB;
    uint16_t Blue;
} CamEngineBlackLevel_t;


/*****************************************************************************/
/**
 * @brief   Configuration structure of the output path.
 *
 *****************************************************************************/
typedef struct CamEnginePathConfig_s
{
    uint16_t                width;          /**< output width */
    uint16_t                height;         /**< output height */
    CamerIcMiDataMode_t     mode;           /**< output data mode */
    CamerIcMiDataLayout_t   layout;         /**< output data layout */
    bool_t                  dcEnable;       /**< enable cropping in dual cropping unit */
    CamEngineWindow_t       dcWin;          /**< image size after dual cropping unit */
} CamEnginePathConfig_t;


typedef struct CamEngineBestSensorResReq_s
{
    uint32_t request_w;
    uint32_t request_h;
    uint32_t request_fps;
    float    request_exp_t;   
    bool_t   request_fullfov;
    bool_t   requset_aspect;
    uint32_t resolution;

} CamEngineBestSensorResReq_t;

#ifdef __cplusplus
}
#endif


/* @} cam_engine_common */


#endif /* __CAM_ENGINE_COMMON_H__ */

