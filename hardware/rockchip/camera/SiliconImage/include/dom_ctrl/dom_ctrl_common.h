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
 * @file dom_ctrl_common.h
 *
 * @brief
 *   Common stuff used by dom ctrl API & implementation.
 *
 *****************************************************************************/
/**
 * @page dom_ctrl_page DOM Ctrl
 * The Display Output Module displays image buffers in an X11 window.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref dom_ctrl
 *
 * @defgroup dom_ctrl_common DOM Ctrl Common
 * @{
 *
 */

#ifndef __DOM_CTRL_COMMON_H__
#define __DOM_CTRL_COMMON_H__

#ifdef __cplusplus
extern "C"
{
#endif

//TODO: remove FPS measurement stuff
#define DOM_FPS

/**
 * @brief Handle to dom ctrl process context.
 *
 */
typedef struct domCtrlContext_s *domCtrlHandle_t;

/**
 * @brief IDs of supported commands.
 *
 */
enum domCtrl_command_e
{
    DOM_CTRL_CMD_START          = 0,
    DOM_CTRL_CMD_STOP           = 1,
    DOM_CTRL_CMD_SHUTDOWN       = 2,
    DOM_CTRL_CMD_PROCESS_BUFFER = 3
};

/**
 * @brief Data type used for commands (@ref domCtrl_command_e).
 *
 */
typedef enum domCtrl_command_e domCtrlCmdId_t;

/**
 *  @brief Command completion signalling callback
 *
 *  Callback for signalling completion of commands which could require further application interaction.
 *
 */
typedef void (* domCtrlCompletionCb_t)
(
    domCtrlCmdId_t          CmdId,          //!< The ID of the command which was completed (see @ref domCtrl_command_e).
    RESULT                  result,         //!< Result of the executed command.
    const void              *pUserContext   //!< Opaque user data pointer that was passed in on dom control creation.
);



typedef struct domCtrlDrawContext_s *domCtrlDrawHandle_t;


typedef struct domCtrlDrawConfig_s
{
    domCtrlDrawHandle_t     hDrawContext;
} domCtrlDrawConfig_t;


typedef struct ibdCmd_t     domCtrlDrawCmd_t;

/* @} dom_ctrl_common */

#ifdef __cplusplus
}
#endif

#endif /* __DOM_CTRL_COMMON_H__ */
