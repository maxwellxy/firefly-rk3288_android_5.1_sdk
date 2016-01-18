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
 * @file dom_ctrl_api.h
 *
 * @brief
 *   Definition of dom ctrl API.
 *
 *****************************************************************************/
/**
 * @page dom_ctrl_page DOM Ctrl
 * The Display Output Module displays image buffers in an X11 window.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref dom_ctrl_api
 * - @ref dom_ctrl_common
 * - @ref dom_ctrl
 *
 * @defgroup dom_ctrl_api DOM Ctrl API
 * @{
 *
 */

#ifndef __DOM_CTRL_API_H__
#define __DOM_CTRL_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cea_861.h>

#include <bufferpool/media_buffer.h>
#include <hal/hal_api.h>

#include "dom_ctrl_common.h"

typedef enum domImagePresentation_e
{
    DOMCTRL_IMAGE_PRESENTATION_INVALID       = 0,
    DOMCTRL_IMAGE_PRESENTATION_SINGLE        = 1,
    DOMCTRL_IMAGE_PRESENTATION_3D_VERTICAL   = 2,
    DOMCTRL_IMAGE_PRESENTATION_3D_ANAGLYPH   = 3,
    DOMCTRL_IMAGE_PRESENTATION_MAX
} domImagePresentation_t;

typedef struct domCtrlConfig_s
{
    uint32_t                MaxPendingCommands; //!< Number of commands that can be queued and thus be pending at a time.
    uint32_t                MaxBuffers;
    domCtrlCompletionCb_t   domCbCompletion;    //!< Callback function for command completion.
    void                    *pUserContext;      //!< User context passed on to completion callback.
    HalHandle_t             HalHandle;          //!< HAL session to use for HW access
    domImagePresentation_t  ImgPresent;

    void                    *hParent;           //!< IN: Anonymous handle to window parent; NULL if stand alone window. Real handle type is implementation dependent; user must assure correctness at design time.
    int32_t                 posX;               //!< IN: Window, Widget, Control, ... top left corner relative to parent or display area.
    int32_t                 posY;               //!< IN: Window, Widget, Control, ... top left corner relative to parent or display area.
    uint32_t                width;              //!< IN: Window, Widget, Control, ... dimension. If 0 (zero) either parent (parent != NULL) or default (parent == NULL) dimensions are used.
    uint32_t                height;             //!< IN: Window, Widget, Control, ... dimension. If 0 (zero) either parent (parent != NULL) or default (parent == NULL) dimensions are used.

    domCtrlHandle_t         domCtrlHandle;      //!< Handle to created dom control context, set by @ref domCtrlInit if successfull, undefined otherwise.
} domCtrlConfig_t;

extern RESULT domCtrlInit
(
    domCtrlConfig_t *pConfig            //!< Reference to configuration structure.
);

extern RESULT domCtrlShutDown
(
    domCtrlHandle_t domCtrlHandle       //!< Handle to dom control context as returned by @ref domCtrlInit.
);

extern RESULT domCtrlStart
(
    domCtrlHandle_t domCtrlHandle       //!< Handle to dom control context as returned by @ref domCtrlInit.
);

extern RESULT domCtrlStop
(
    domCtrlHandle_t domCtrlHandle       //!< Handle to dom control context as returned by @ref domCtrlInit.
);

extern RESULT  domCtrlShowBuffer
(
    domCtrlHandle_t         hDomContext,
    MediaBuffer_t           *pBuffer
);


/*****************************************************************************/
/**
 * @brief   TODO
 *
 *****************************************************************************/
extern RESULT  domCtrlStartDraw
(
    domCtrlHandle_t         hDomContext,
    domCtrlDrawConfig_t     *pDrawConfig
);



/*****************************************************************************/
/**
 * @brief   TODO
 *
 *****************************************************************************/
extern RESULT  domCtrlStopDraw
(
    domCtrlHandle_t         hDomContext,
    domCtrlDrawHandle_t     hDrawContext
);



/*****************************************************************************/
/**
 * @brief   TODO
 *
 *****************************************************************************/
extern RESULT  domCtrlDraw
(
    domCtrlHandle_t         hDomContext,
    domCtrlDrawHandle_t     hDrawContext,
    uint32_t                numCmds,
    domCtrlDrawCmd_t        *pDrawCmds
);

/* @} dom_ctrl_api */

#ifdef __cplusplus
}
#endif

#endif /* __DOM_CTRL_API_H__ */
