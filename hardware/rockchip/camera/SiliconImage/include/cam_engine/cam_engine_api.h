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
 * @file cam_engine_api.h
 *
 * @brief
 *   Interface description of the CamEngine.
 *
 *****************************************************************************/
/**
 *
 * @mainpage General Concept
 *
 * The CamEengine provides the application programming interface (API) to control
 * the camerIC ISP hardware and its sub modules from C/C++ applications. It abstracts
 * a common camera interface and contains functions to control the usage modes of
 * a typical image capturing device.
 *
 * The camerIC ISP offers two data output path. The @e self @e path is intended for preview;
 * the @e main @e path offers an additional JPEG encoding block and is intended for capturing
 * video streams and high resolution still images. The CamEngine controls the output
 * modality for the chosen usage mode (see @ref CamEngineSetPathConfig). A callback to receive
 * full buffers of either the @e self @e or @e main @e path can be registered using the
 * @ref CamEngineRegisterBufferCb function. The @e self @e path can either be rendered in
 * an X11 application window or forward to the video display unit for rendering on a connected
 * HDMI display. The stream from the @e main @e path can be captured to disk.
 *
 * The CamEngine follows the object model of the camerIC evaluation software. Before usage, the
 * CamEngine needs to be statically configured by the @ref CamEngineInit function. Then, an instance
 * of the CamEngine can be started using the @ref CamEngineStart function. From now on commands
 * can be send to the CamEngine. The instance can be stopped using the @ref CamEngineStop
 * function and all resources are released with the @ref CamEngineShutDown function.
 *
 * @section cameric_3d_section CamerIC 3D configuration
 *
 * The standard CamerIC IP may be accompanied by a CamerIC slave IP which processes
 * the data of the 2nd video sensor. The camerIC slave can be a full featured CamerIC
 * or an area optimized version of the standard camerIC. For camerIC 3D the data flow
 * concept includes serial operation of the camerIC slave and the camerIC master module.
 * This is also necessary for the video stabilization module.
 *
 * The CamerIC 3D supports the flowing data flow concepts:
 * @arg @ref CAM_ENGINE_MODE_SENSOR_2D
 * @image html SENSOR_2D.png "2D (single pipe) scenario mode" width=0.5\textwidth
 * @image latex SENSOR_2D.png "2D (single pipe) scenario mode" width=0.5\textwidth
 *
 * @arg @ref CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB
 * @image html SENSOR_2D_IMGSTAB.png "2D (with video stabilization) scenario mode" width=0.5\textwidth
 * @image latex SENSOR_2D_IMGSTAB.png "2D (with video stabilization) scenario mode" width=0.5\textwidth
 *
 * @arg @ref CAM_ENGINE_MODE_SENSOR_3D
 * @image html SENSOR_3D.png "3D (dual pipe) scenario mode" width=0.5\textwidth
 * @image latex SENSOR_3D.png "3D (dual pipe) scenario mode" width=0.5\textwidth
 *
 * @arg @ref CAM_ENGINE_MODE_IMAGE_PROCESSING
 * @image html IMAGE_PROCESSING.png "Image rendering/processing scenario mode" width=0.5\textwidth
 * @image latex IMAGE_PROCESSING.png "Image rendering/processing scenario mode" width=0.5\textwidth
 *
 * @defgroup cam_engine_api CamEngine API
 * @{
 */

#ifndef __CAM_ENGINE_API_H__
#define __CAM_ENGINE_API_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/picture_buffer.h>
#include <bufferpool/media_buffer.h>

//FIXME
#include <common/mipi.h>
#include <isi/isi.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_drv_api.h>
#include <cameric_drv/cameric_mi_drv_api.h>
#include <cameric_drv/cameric_isp_flash_drv_api.h>
#include <cam_calibdb/cam_calibdb_api.h>

#include "cam_engine_common.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @brief   Configuration structure of the cam-engine instance.
 *
 */
typedef struct CamEngineInstanceConfig_s
{
    uint32_t                    maxPendingCommands; /**< Number of commands that can be queued and thus be pending at a time. */
    bool_t                      isSystem3D;         /**< System capable for 3D */

    CamEngineCompletionCb_t     cbCompletion;       /**< Callback function for command completion. */
    CamEngineAfpsResChangeCb_t  cbAfpsResChange;    /**< Afps resolution chnage request callback */
    void                        *pUserCbCtx;        /**< User context passed on to completion & Afps callbacks. */

    HalHandle_t                 hHal;               /**< HAL handle (hardware abstraction layer). */

    CamEngineHandle_t           hCamEngine;         /**< Handle to cam-engine context, set by @ref CamEngineInit if successful, undefined otherwise. */
} CamEngineInstanceConfig_t;


/**
 * @brief   Configuration structure of the cam-engine.
 *
 */
typedef struct CamEngineConfig_s
{
    CamEngineModeType_t     mode;
	int mipiLaneNum;
	uint32_t mipiLaneFreq;
	uint32_t phyAttachedDevId;
    CamEnginePathConfig_t   pathConfigMaster[CAM_ENGINE_PATH_MAX];
    CamEnginePathConfig_t   pathConfigSlave[CAM_ENGINE_PATH_MAX];

    CamCalibDbHandle_t      hCamCalibDb;                            /**< handle to calibration data base */

    union CamEngineConfigData_u
    {
        struct CamEngineConfigSensor_s
        {
            /* sensor interface */
            IsiSensorHandle_t               hSensor;                /**< main sensor handle (measuring running on this sensor pipe */
            IsiSensorHandle_t               hSubSensor;             /**< sub sensor handle */

            /* input interface */
            CamerIcInterfaceSelect_t        ifSelect;

            /* input acquisition */
            CamerIcIspSampleEdge_t          sampleEdge;
            CamerIcIspPolarity_t            hSyncPol;
            CamerIcIspPolarity_t            vSyncPol;
            CamerIcIspBayerPattern_t        bayerPattern;
            CamerIcIspColorSubsampling_t    subSampling;
            CamerIcIspCCIRSequence_t        seqCCIR;
            CamerIcIspFieldSelection_t      fieldSelection;
            CamerIcIspInputSelection_t      inputSelection;

            /* isp configuration */
            CamerIcIspMode_t                mode;
            CamEngineWindow_t               acqWindow;
            CamEngineWindow_t               outWindow;
            CamEngineWindow_t               isWindow;
            CamEngineWindow_t               isCroppingWindow;       /**< window size for image stabilization */

            MipiDataType_t                  mipiMode;

            bool_t                          enableTestpattern;

            CamEngineFlickerPeriod_t        flickerPeriod;
            bool_t                          enableAfps;
        } sensor;

        struct CamEngineConfigImage_s
        {
            /* image information */
            PicBufType_t                    type;
            PicBufLayout_t                  layout;
            
            uint8_t                         *pBuffer;
            uint16_t                        width;
            uint16_t                        height;

            CamEngineWbGains_t              *pWbGains;
            CamEngineCcMatrix_t             *pCcMatrix;
            CamEngineCcOffset_t             *pCcOffset;
            CamEngineBlackLevel_t           *pBlvl;

            float                           vGain;
            float                           vItime;
        } image;

    } data;

} CamEngineConfig_t;


/*****************************************************************************/
/**
 * @brief   The function creates and initializes a CamEngine instance.
 *
 * @param   pConfig     Instance configuration structure.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_INVALID_PARM    invalid configuration
 * @retval  RET_OUTOFRANGE      a configuration parameter is out of range
 * @retval  RET_WRONG_HANDLE    invalid HAL handle
 * @retval  RET_OUTOFMEM        not enough memory available
 *
 *****************************************************************************/
RESULT CamEngineInit
(
    CamEngineInstanceConfig_t *pConfig
);


/*****************************************************************************/
/**
 * @brief   This function shutdowns and destroys a CamEngine instance.
 *
 * @param   hCamEngine Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_PENDING         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state to shutdown
 *
 *****************************************************************************/
RESULT CamEngineShutDown
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   The function starts a CamEngine instance. This puts the CamEngine
 *          into the running state. The CamEngine is configured and ready for
            streaming but streaming not yet started. Configuration settings
            of the auto and image processing algorithms may be accessed and
            altered.
 *
 * @param   hCamEngine  Handle to the CamEngine instance.
 * @param   pConfig     Configuration structure.
 *
 * @return  Return the result of the function call.
 * @return  RET_PENDING         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state to shutdown
 *
 *****************************************************************************/
RESULT CamEngineStart
(
    CamEngineHandle_t   hCamEngine,
    CamEngineConfig_t   *pConfig
);


/*****************************************************************************/
/**
 * @brief   The function stops a CamEngine instance.
 *
 * @param   hCamEngine  Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_PENDING         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineStop
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function starts streaming. This puts the CamEngine
 *          into the streaming state.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 * @param   frames          Number of frames to capture, frames = 0, means continuously.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineStartStreaming
(
    CamEngineHandle_t   hCamEngine,
    uint32_t            frames
);


/*****************************************************************************/
/**
 * @brief   This function stops streaming.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineStopStreaming
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function registers a buffer callback. Only one buffer callback
 *          may be registered at a time.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 * @param   fpCallback      Buffer callback to register.
 * @param   pUserContext    User context to pass on to callback.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_NULL_POINTER    callback is null pointer
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineRegisterBufferCb
(
    CamEngineHandle_t   hCamEngine,
    CamEngineBufferCb_t fpCallback,
    void*               pBufferCbCtx
);


/*****************************************************************************/
/**
 * @brief   This function unregisters the buffer callback.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineDeRegisterBufferCb
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function returns the current buffer callback.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineGetBufferCb
(
    CamEngineHandle_t    hCamEngine,
    CamEngineBufferCb_t* fpCallback,
    void**               ppBufferCbCtx
);


/*****************************************************************************/
/**
 * @brief   This function returns the state of the CamEngine.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 *
 * @return  Return the the state of the CamEngine.
 *
 *****************************************************************************/
CamEngineState_t CamEngineGetState
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function returns the scenario mode of the CamEngine.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 *
 * @return  Return the the mode of the CamEngine.
 *
 *****************************************************************************/
CamEngineModeType_t CamEngineGetMode
(
    CamEngineHandle_t   hCamEngine
);


/*****************************************************************************/
/**
 * @brief   This function sets the configuration of main and self path.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 * @param   pConfigMain     Configuration structure for main path.
 * @param   pConfigSelf     Configuration structure for self path.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineSetPathConfig
(
    CamEngineHandle_t               hCamEngine,
    const CamEngineChainIdx_t		chainIdx,
    const CamEnginePathConfig_t     *pConfigMain,
    const CamEnginePathConfig_t     *pConfigSelf
);


/*****************************************************************************/
/**
 * @brief   This function sets the configuration of main and self path.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   acqWindow           acquisition input window (resolution from sensor)
 * @param   outWindow           acquisition output window
 * @param   îsWindow            image stabilization window 
 * @param   isCroppingWindow    image stabilization cropping window (only need
 *                              if image stabilization enabled)
 * 
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineSetAcqResolution
(
    CamEngineHandle_t   hCamEngine,
    CamEngineWindow_t   acqWindow,
    CamEngineWindow_t   outWindow,
    CamEngineWindow_t   isWindow,
    CamEngineWindow_t   isCroppingWindow,
    uint32_t            numFramesToSkip
);


/*****************************************************************************/
/**
 * @brief   This function sets the configuration of the ECM.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   flickerPeriod       Flicker period ID.
 * @param   enableAfps          Enable AFPS mode.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 * @retval  RET_INVALID_PARM    invalid parameter
 *
 *****************************************************************************/
RESULT CamEngineSetEcm
(
    CamEngineHandle_t           hCamEngine,
    CamEngineFlickerPeriod_t    flickerPeriod,
    bool_t                      enableAfps
);


/*****************************************************************************/
/**
 * @brief   This function sets the configuration of main and self path.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 * @param   pConfigMain     Configuration structure for main path.
 * @param   pConfigSelf     Configuration structure for self path.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineSetCalibDb
(
    CamEngineHandle_t   hCamEngine,
    CamCalibDbHandle_t  hCamCalibDb

);


/*****************************************************************************/
/**
 * @brief   This function runs the auto algorithms and locks the settings
            of the requested locks, e.g. autofocus, auto exposure control
            and auto white balance.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 * @param   locks              Requested locks, locks = 0xFF, means all.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineSearchAndLock
(
    CamEngineHandle_t   hCamEngine,
    CamEngineLockType_t locks
);


/*****************************************************************************/
/**
 * @brief   This function cancels the requested locks if locked.
 *
 * @param   hCamEngine      Handle to the CamEngine instance.
 * @param   locks           Requested locks, locks = 0xFF, means all.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_STATE     instance is in wrong state
 *
 *****************************************************************************/
RESULT CamEngineUnlock
(
    CamEngineHandle_t   hCamEngine,
    CamEngineLockType_t locks
);

uint32_t CamerEngineSetDataPathWhileStreaming
(
    CamEngineHandle_t  pCamEngineCtx,
    CamerIcWindow_t* pWin,
    uint32_t outWidth, 
    uint32_t outHeight
    
);

/******************************************************************************
 * CamEngineStartPixelIf
 *****************************************************************************/
RESULT CamEngineStartPixelIfApi
(
    CamEngineHandle_t  hCamEngine,
    CamEngineConfig_t   *pConfig
);


/******************************************************************************
 * CamEngineConfigureFlash
 *****************************************************************************/
RESULT CamEngineConfigureFlash
(
    CamEngineHandle_t  hCamEngine,
    CamerIcIspFlashCfg_t *cfgFsh
);

/******************************************************************************
 * CamEngineStartFlash
 *****************************************************************************/
RESULT CamEngineStartFlash
(
    CamEngineHandle_t  hCamEngine,
    bool_t operate_now
);


/******************************************************************************
 * CamEngineStopFlash
 *****************************************************************************/
RESULT CamEngineStopFlash
(
    CamEngineHandle_t  hCamEngine,
    bool_t operate_now
);

RESULT CamEngineSetAecPoint
(
    CamEngineHandle_t  hCamEngine,
    float point
);

#ifdef __cplusplus
}
#endif


/* @} cam_engine_api */


#endif /* __CAM_ENGINE_API_H__ */

