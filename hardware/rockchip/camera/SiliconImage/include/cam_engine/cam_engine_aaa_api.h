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
 * @cond    cam_engine_aaa
 *
 * @file    cam_engine_aaa_api.h
 *
 * @brief
 *
 *   Interface description of the CamEngine Auto Algorithms.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_engine_aaa_api CamEngine Auto Algorithms API
 * @{
 *
 */

#ifndef __CAM_ENGINE_AAA_API_H__
#define __CAM_ENGINE_AAA_API_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#include "cam_engine_common.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/**
 * @brief   Auto-White-Balance mode.
 *
 *****************************************************************************/
typedef enum CamEngineAwbMode_e
{
    CAM_ENGINE_AWB_MODE_INVALID = 0,    /**< invalid mode (only for initialization) */
    CAM_ENGINE_AWB_MODE_MANUAL  = 1,    /**< manual mode */
    CAM_ENGINE_AWB_MODE_AUTO    = 2,    /**< run auto mode */
    CAM_ENGINE_AWB_MODE_MAX
} CamEngineAwbMode_t;



/*****************************************************************************/
/**
 *          CamEngineAwbRgProj_t
 *
 * @brief   AWB Projection Configuration in R/G Layer
 *
 *****************************************************************************/
typedef struct CamEngineAwbRgProj_s
{
    float   fRgProjIndoorMin;
    float   fRgProjOutdoorMin;
    float   fRgProjMax;
    float   fRgProjMaxSky;
} CamEngineAwbRgProj_t;



/*****************************************************************************/
/**
 * @brief   Auto-Exposure-Control scene evaluation mode.
 *
 *****************************************************************************/
typedef enum CamEngineAecSemMode_e
{
    CAM_ENGINE_AEC_SCENE_EVALUATION_INVALID       = 0,    /**< invalid mode (only for initialization) */
    CAM_ENGINE_AEC_SCENE_EVALUATION_DISABLED      = 1,    /**< scene evaluation disabled (fix setpoint) */
    CAM_ENGINE_AEC_SCENE_EVALUATION_FIX           = 2,    /**< scene evaluation fix (static ROI) */
    CAM_ENGINE_AEC_SCENE_EVALUATION_ADAPTIVE      = 3,    /**< scene evaluation adaptive (adaptive ROI) */
    CAM_ENGINE_AEC_SCENE_EVALUATION_MAX
} CamEngineAecSemMode_t;



/*****************************************************************************/
/**
 * @brief   Auto-Exposure-Damping mode (video | still image )
 */
/*****************************************************************************/
typedef enum CamEngineAecDampingMode_e
{
    CAM_ENGINE_AEC_DAMPING_MODE_INVALID        = 0,        /* invalid (only used for initialization) */
    CAM_ENGINE_AEC_DAMPING_MODE_STILL_IMAGE    = 1,        /* damping mode still image */
    CAM_ENGINE_AEC_DAMPING_MODE_VIDEO          = 2,        /* damping mode video */
    CAM_ENGINE_AEC_DAMPING_MODE_MAX
} CamEngineAecDampingMode_t;



/*****************************************************************************/
/**
 * @brief   Auto-Exposure-Control histogram.
 *
 *****************************************************************************/
#define CAM_ENGINE_AEC_HIST_NUM_BINS           16  /**< number of bins */
typedef uint32_t CamEngineAecHistBins_t[CAM_ENGINE_AEC_HIST_NUM_BINS];



/*****************************************************************************/
/**
 * @brief   Auto-Exposure-Control luminance grid.
 *
 *****************************************************************************/
#define CAM_ENGINE_AEC_EXP_GRID_ITEMS          25  /**< number of grid items (see @ref CamerIcMeanLuma_t) */
typedef uint8_t CamEngineAecMeanLuma_t[CAM_ENGINE_AEC_EXP_GRID_ITEMS];



/*****************************************************************************/
/**
 * @brief   Auto-Focus-Control mode.
 *
 *****************************************************************************/
typedef enum CamEngineAfMode_e
{
    CAM_ENGINE_AUTOFOCUS_MODE_INVALID       = 0,    /**< invalid mode (only for initialization) */
    CAM_ENGINE_AUTOFOCUS_MODE_ONESHOT       = 1,    /**< one-shot mode (runs autofocus search once and stops atomatically after positioing the lense to max) */
    CAM_ENGINE_AUTOFOCUS_MODE_CONTINOUSLY   = 2,    /**< continuously observe the focus */
    CAM_ENGINE_AUTOFOCUS_MODE_EXTERNAL      = 3,    /**< attach an external algorithm */
    CAM_ENGINE_AUTOFOCUS_MODE_STOP          = 4,    /**< stop current autofocus search */
    CAM_ENGINE_AUTOFOCUS_MODE_MAX
} CamEngineAfMode_t;



/*****************************************************************************/
/**
 * @brief   Auto-Focus-Control search algorithm.
 *
 *****************************************************************************/
typedef enum CamEngineAfSearchAlgorithm_e
{
    CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_INVALID           = 0,    /**< invalid search algorithm */
    CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_FULL_RANGE        = 1,    /**< full range */
    CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE    = 2,    /**< adaptive range */
    CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_HILL_CLIMBING     = 3,    /**< hill climbing */
    CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_MAX
} CamEngineAfSearchAlgorithm_t;


typedef enum CamEngineAfEvtId_e 
{
    CAM_ENGINE_AUTOFOCUS_MOVE        = 0,  /* <Notify on autofocus start and stop. This is useful in continuous > */
    CAM_ENGINE_AUTOFOCUS_FINISHED    = 1,
} CamEngineAfEvtId_t;

typedef struct CamEngineAfMoveEvt_s 
{
    bool_t start;
} CamEngineAfMoveEvt_t;

typedef struct CamEngineAfFinshEvt_s
{
    bool_t focus;
} CamEngineAfFinshEvt_t;

typedef struct CamEngineAfEvt_s 
{
    CamEngineAfEvtId_t              evnt_id;
    union {
        CamEngineAfMoveEvt_t         mveEvt;
        CamEngineAfFinshEvt_t        fshEvt;
    } info;
    void                   *pEvntCtx;   
} CamEngineAfEvt_t;


typedef struct CamEngineAfEvtQue_s 
{
    List                   list;
    osQueue                queue;
} CamEngineAfEvtQue_t;

typedef enum CamEngineAecHistMeasureMode_e
{
    AverageMetering = 0,
    CentreWeightMetering = 1
} CamEngineAecHistMeasureMode_t;

typedef enum CamEngine3aLock_e
{
    Lock_awb = 1,
    Lock_aec = 2,
    Lock_af = 4
} CamEngine3aLock_t;

/*****************************************************************************/
/**
 * @brief   This functions starts the Auto-White-Balance.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   mode                working mode ( Manual | Auto )
 * @param   index               illumination profile index
 *                              Manual: profile to run
 *                              Auto: start-profile
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_INVALID_PARM    invalid configuration
 * @retval  RET_OUTOFRANGE      a configuration parameter is out of range
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAwbStart
(
    CamEngineHandle_t           hCamEngine,     /**< handle CamEngine */
    const CamEngineAwbMode_t    mode,           /**< run-mode */
    const uint32_t              index,          /**< AUTO: start-profile, MANUAL: profile to run */
    const bool_t                damp            /**< damping on/off */
);



/*****************************************************************************/
/**
 * @brief   This functions stops the Auto-White-Balance.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAwbStop
(
    CamEngineHandle_t           hCamEngine      /**< handle CamEngine */
);



/*****************************************************************************/
/**
 * @brief   This functions resets the Auto-White-Balance.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAwbReset
(
    CamEngineHandle_t           hCamEngine      /**< handle CamEngine */
);
/* ddl@rock-chips.com: v0.0x29.0 */
/******************************************************************************
 * CamEngine3aLock()
 *****************************************************************************/
RESULT CamEngine3aLock
(
    CamEngineHandle_t hCamEngine,
    CamEngine3aLock_t lock
);
/******************************************************************************
 * CamEngine3aUnLock()
 *****************************************************************************/
RESULT CamEngine3aUnLock
(
    CamEngineHandle_t hCamEngine,
    CamEngine3aLock_t unlock
);
/*****************************************************************************/
/**
 * @brief   This functions returns the Auto-White-Balance status.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   pRunning            BOOL_TRUE: running, BOOL_FALSE: stopped
 * @param   pMode               working mode ( Manual | Auto )
 * @param   pCieProfile         illumination profile
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAwbStatus
(
    CamEngineHandle_t           hCamEngine,     /**< handle CamEngine */
    bool_t                      *pRunning,      /**< BOOL_TRUE: running, BOOL_FALSE: stopped */
    CamEngineAwbMode_t          *pMode,
    uint32_t                    *pCieProfile,
    CamEngineAwbRgProj_t        *pRgProj,
    bool_t                      *pDamping       /**< BOOL_TRUE: damping on, BOOL_FALSE: damping off */
);

RESULT CamEngineIsAwbStable
(
    CamEngineHandle_t       hCamEngine,
    bool_t                  *pIsStable,
    uint32_t				*pDNoWhitePixel
);


/*****************************************************************************/
/**
 * @brief   This functions starts the Auto-Exposure-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAecStart
(
    CamEngineHandle_t           hCamEngine      /**< handle CamEngine */
);



/*****************************************************************************/
/**
 * @brief   This functions stops the Auto-Exposure-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAecStop
(
    CamEngineHandle_t           hCamEngine      /**< handle CamEngine */
);



/*****************************************************************************/
/**
 * @brief   This functions resets the Auto-Exposure-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAecReset
(
    CamEngineHandle_t           hCamEngine      /**< handle CamEngine */
);



/*****************************************************************************/
/**
 * @brief   This functions configures the Auto-Exposure-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAecConfigure
(
    CamEngineHandle_t               hCamEngine,     /**< handle CamEngine */
    const CamEngineAecSemMode_t     mode,           /**< scene evaluation mode */
    const float                     setPoint,       /**< set point to hit by the ae control system */
    const float                     clmTolerance,   /**< tolerance */
    const CamEngineAecDampingMode_t dampingMode,    /**< damping mode */
    const float                     dampOverStill,  /**< damping coefficient still image */
    const float                     dampUnderStill, /**< damping coefficient still image */
    const float                     dampOverVideo,  /**< damping coefficient video */
    const float                     dampUnderVideo  /**< damping coefficient video */
);



/*****************************************************************************/
/**
 * @brief   This functions returns the Auto-Exposure-Control status.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   pRunning            BOOL_TRUE: running, BOOL_FALSE: stopped
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAecStatus
(
    CamEngineHandle_t           hCamEngine,        /**< handle CamEngine */
    bool_t                      *pRunning,         /**< BOOL_TRUE: running, BOOL_FALSE: stopped */
    CamEngineAecSemMode_t       *pMode,            /**< scene evaluation mode */
    float                       *pSetPoint,        /**< set point to hit by the ae control system */
    float                       *pClmTolerance,    /**< tolerance */
    CamEngineAecDampingMode_t   *pDampingMode,     /**< damping mode */
    float                       *pDampOverStill,   /**< damping coefficient still image */
    float                       *pDampUnderStill,  /**< damping coefficient still image */
    float                       *pDampOverVideo,   /**< damping coefficient video */
    float                       *pDampUnderVideo   /**< damping coefficient video */
);



/*****************************************************************************/
/**
 * @brief   This function returns the current Auto-Exposure-Control histogram.
 *
 * @param   handle              Handle to the CamEngine instance.
 * qparam   pHistogram          pointer to the histogram bins
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAecGetHistogram
(
    CamEngineHandle_t        hCamEngine,
    CamEngineAecHistBins_t   *pHistogram
);



/*****************************************************************************/
/**
 * @brief   This function returns the current Auto-Exposure-Control luminance grid.
 *
 * @param   handle              Handle to the CamEngine instance.
 * qparam   pLuma               pointer to the luminance grid
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAecGetLuminance
(
    CamEngineHandle_t        hCamEngine,
    CamEngineAecMeanLuma_t   *pLuma
);



/*****************************************************************************/
/**
 * @brief   This function returns the current Auto-Exposure-Control object region.
 *
 * @param   handle              Handle to the CamEngine instance.
 * qparam   pLuma               pointer to the object region
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAecGetObjectRegion
(
    CamEngineHandle_t        hCamEngine,
    CamEngineAecMeanLuma_t   *pObjectRegion
);
/******************************************************************************
 * CamEngineAecGetMeasuringWindow()
 *****************************************************************************/
RESULT CamEngineAecGetMeasuringWindow
(
    CamEngineHandle_t               hCamEngine,
    CamEngineWindow_t               *pWindow,
    CamEngineWindow_t               *pGrid
);
/******************************************************************************
 * CamEngineAecSetMeasuringWindow()
 *****************************************************************************/
RESULT CamEngineAecHistSetMeasureWinAndMode
(   
    CamEngineHandle_t               hCamEngine,
    int16_t                  x,
    int16_t                  y,
    uint16_t                  width,
    uint16_t                  height,
    CamEngineAecHistMeasureMode_t mode
);

/*****************************************************************************/
/**
 * @brief   This function returns if Auto-Focus-Control is available with the
 *          connected sensor board.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   pRunning            BOOL_TRUE: available, BOOL_FALSE: not available
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAfAvailable
(
    CamEngineHandle_t       hCamEngine,
    bool_t                  *pAvailable
);



/*****************************************************************************/
/**
 * @brief   This function starts the Auto-Focus-Control (continous mode).
 *
 * @param   hCamEngine              Handle to the CamEngine instance.
 * @param   AutoFocusSearchAgoritm  search algorithm ( ADAPTIVE_RANGE | HILL_CLIMBING )
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAfStart
(
    CamEngineHandle_t                    hCamEngine,
    const CamEngineAfSearchAlgorithm_t   searchAgoritm
);



/*****************************************************************************/
/**
 * @brief   This function starts the Auto-Focus-Control (one-shot mode).
 *
 * @param   hCamEngine              Handle to the CamEngine instance.
 * @param   AutoFocusSearchAgoritm  search algorithm ( ADAPTIVE_RANGE | HILL_CLIMBING )
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAfOneShot
(
    CamEngineHandle_t                    hCamEngine,
    const CamEngineAfSearchAlgorithm_t   searchAgoritm
);



/*****************************************************************************/
/**
 * @brief   This function stops the Auto-Focus-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAfStop
(
    CamEngineHandle_t hCamEngine
);



/*****************************************************************************/
/**
 * @brief   This function returns the Auto-Focus-Control status.
 *
 * @param   hCamEngine              Handle to the CamEngine instance.
 * @param   pRunning                BOOL_TRUE: running, BOOL_FALSE: stopped
 * @param   pAutoFocusSearchAgoritm search algorithm ( ADAPTIVE_RANGE | HILL_CLIMBING )
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAfStatus
(
    CamEngineHandle_t                   hCamEngine,
    bool_t                              *pRunning,
    CamEngineAfSearchAlgorithm_t        *pSearchAgoritm,
    float                               *sharpness
);

/******************************************************************************
 * CamEngineAfShotCheck()
 *****************************************************************************/
RESULT CamEngineAfShotCheck
(
    CamEngineHandle_t               hCamEngine,
    bool_t                          *shot
);

/******************************************************************************
 * CamEngineAfShotCheck()
 *****************************************************************************/
RESULT CamEngineAfRegisterEvtQue
(
    CamEngineHandle_t               hCamEngine,
    CamEngineAfEvtQue_t             *evtQue
);

/******************************************************************************
 * CamEngineAfReset()
 *****************************************************************************/
RESULT CamEngineAfReset
(
    CamEngineHandle_t                    hCamEngine,
    const CamEngineAfSearchAlgorithm_t   searchAgoritm
);

/******************************************************************************
 * CamEngineAfGetMeasuringWindow()
 *****************************************************************************/
RESULT CamEngineAfGetMeasuringWindow
(
    CamEngineHandle_t               hCamEngine,
    CamEngineWindow_t               *pWindow
);

/******************************************************************************
 * CamEngineAfSetMeasuringWindow()
 *****************************************************************************/
RESULT CamEngineAfSetMeasuringWindow
(   
    CamEngineHandle_t               hCamEngine,
    int16_t                  x,
    int16_t                  y,
    uint16_t                  width,
    uint16_t                  height
);

/*****************************************************************************/
/**
 * @brief   This function starts the Adaptive-DPF-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAdpfStart
(
    CamEngineHandle_t hCamEngine
);



/*****************************************************************************/
/**
 * @brief   This function stops the Adaptive-DPF-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAdpfStop
(
    CamEngineHandle_t hCamEngine
);



/*****************************************************************************/
/**
 * @brief   This functions configures the Adaptive-DPF-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAdpfConfigure
(
    CamEngineHandle_t           hCamEngine,      /**< handle CamEngine */
    const float                 gradient,        /**< gradient */
    const float                 offset,          /**< offset */
    const float                 min,             /**< upper bound */
    const float                 div,             /**< division factor */
    const uint8_t               sigmaGreen,      /**< sigma green */
    const uint8_t               sigmaRedBlue     /**< sigma red/blue */
);



/*****************************************************************************/
/**
 * @brief   This function returns the Adaptive-DPF-Control status.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   pRunning            BOOL_TRUE: running, BOOL_FALSE: stopped
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAdpfStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t              *pRunning,
    float               *pGradient,        /**< gradient */
    float               *pOffset,          /**< offset */
    float               *pMin,             /**< upper bound */
    float               *pDiv,             /**< division factor */
    uint8_t             *pSigmaGreen,      /**< sigma green */
    uint8_t             *pSigmaRedBlue     /**< sigma red/blue */

);


/*****************************************************************************/
/**
 * @brief   This function starts the Adaptive-DPCC-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAdpccStart
(
    CamEngineHandle_t hCamEngine
);



/*****************************************************************************/
/**
 * @brief   This function stops the Adaptive-DPCC-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAdpccStop
(
    CamEngineHandle_t hCamEngine
);



/*****************************************************************************/
/**
 * @brief   This function returns the Adaptive-DPF-Control status.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   pRunning            BOOL_TRUE: running, BOOL_FALSE: stopped
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAdpccStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t              *pRunning
);




/*****************************************************************************/
/**
 * @brief   This function starts the Auto-Video-Stabilization-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAvsStart
(
    CamEngineHandle_t hCamEngine
);



/*****************************************************************************/
/**
 * @brief   This function stops the Auto-Video-Stabilization-Control.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAvsStop
(
    CamEngineHandle_t hCamEngine
);



/*****************************************************************************/
/**
 * @brief   This functions configures the Auto-Video-Stabilization.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   useParams           Whether to use the specified params or default
 *                              parameters.
 * @param   numItpPoints        See @ref AvsDampFuncParams_t.numItpPoints.
 * @param   theta               See @ref AvsDampFuncParams_t.theta.
 * @param   baseGain            See @ref AvsDampFuncParams_t.baseGain.
 * @param   fallOff             See @ref AvsDampFuncParams_t.fallOff.
 * @param   acceleration        See @ref AvsDampFuncParams_t.acceleration.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_WRONG_CONFIG    invalid configuration
 *
 *****************************************************************************/
RESULT CamEngineAvsConfigure
(
    CamEngineHandle_t   hCamEngine,
    const bool          useParams,
    const uint16_t      numItpPoints,
    const float         theta,
    const float         baseGain,
    const float         fallOff,
    const float         acceleration
);



/*****************************************************************************/
/**
 * @brief   This functions configures the Auto-Video-Stabilization.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   pUsingParams        Whether the returned parameters have been used
 *                              to generate a damping function or if the
 *                              function results from explicitely passed
 *                              interpolation points. If *pUsingParams is
 *                              BOOL_TRUE, the following parameters contain the
 *                              configured values of damping function parameters
 *                              if the function returns successfully.
 * @param   pNumItpPoints       See @ref AvsDampFuncParams_t.numItpPoints.
 * @param   pTheta              See @ref AvsDampFuncParams_t.theta.
 * @param   pBaseGain           See @ref AvsDampFuncParams_t.baseGain.
 * @param   pFallOff            See @ref AvsDampFuncParams_t.fallOff.
 * @param   pAcceleration       See @ref AvsDampFuncParams_t.acceleration.
 * @param   pNumDampData        Number of elements pointed to by *ppDampXData
 *                              resp. *ppDampYData.
 * @param   ppDampXData         *ppDampXData points to array of damping function
 *                              interpolation points x values on successful
 *                              return of function.
 * @param   ppDampYData         *ppDampYData points to array of damping function
 *                              interpolation points y values on successful
 *                              return of function.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 * @retval  RET_NULL_POINTER    a NULL pointer has been passed
 *
 *****************************************************************************/
RESULT CamEngineAvsGetConfig
(
    CamEngineHandle_t   hCamEngine,
    bool_t              *pUsingParams,
    uint16_t            *pNumItpPoints,
    float               *pTheta,
    float               *pBaseGain,
    float               *pFallOff,
    float               *pAcceleration,
    int                 *pNumDampData,
    double             **ppDampXData,
    double             **ppDampYData
);



/*****************************************************************************/
/**
 * @brief   This function returns the Auto-Video-Stabilization-Control status.
 *
 * @param   hCamEngine          Handle to the CamEngine instance.
 * @param   pRunning            BOOL_TRUE: running, BOOL_FALSE: stopped
 * @param   pCurrDisplVec       Current displacement vector (measured by VSM)
 * @param   pCurrOffsetVec      Current offset vector (calculated by AVS)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         function succeed
 * @retval  RET_WRONG_HANDLE    invalid instance handle
 *
 *****************************************************************************/
RESULT CamEngineAvsGetStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t              *pRunning,
    CamEngineVector_t   *pCurrDisplVec,
    CamEngineVector_t   *pCurrOffsetVec
);

RESULT CamEngineAecGetClmTolerance
(
    CamEngineHandle_t        hCamEngine,
    float   *clmTolerance
);

RESULT CamEngineSetAecClmTolerance
(
	CamEngineHandle_t		 hCamEngine,
	float	clmTolerance
);

RESULT CamEngineAwbGetinfo
(
	CamEngineHandle_t hCamEngine,
	float *f_RgProj, 
	float *f_s, 
	float *f_s_Max1, 
	float *f_s_Max2, 
	float *f_Bg1, 
	float *f_Rg1, 
	float *f_Bg2, 
	float *f_Rg2
);

RESULT CamEngineAwbGetIlluEstInfo
(
	CamEngineHandle_t hCamEngine,
	float *ExpPriorIn,
	float *ExpPriorOut,
	char (*name)[20],
	float likehood[],
	float wight[],
	int *curIdx,
	int *region,
	int *count
);


#ifdef __cplusplus
}
#endif


/* @} cam_engine_aaa_api */


#endif /* __CAM_ENGINE_AAA_API_H__ */

