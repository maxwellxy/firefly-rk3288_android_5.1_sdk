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
 * @file exa_ctrl_interface.h
 *
 * @brief
 *   EXA (External Algorithm Module) C++ API.
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup module_name Module Name
 * @{
 *
 */

#ifndef __EXA_CTRL_ITF_H__
#define __EXA_CTRL_ITF_H__

#include <hal/hal_api.h>
#include <bufferpool/media_buffer.h>
#include <exa_ctrl/exa_ctrl_api.h>

#include "cam_api/camdevice.h"

/**
 * @brief ExaCtrlItf class declaration.
 */
class ExaCtrlItf : public BufferCb
{
public:
    /**
     * @brief Standard constructor for the ExaCtrlItf object.
     */
    ExaCtrlItf( HalHandle_t hHal );
    ~ExaCtrlItf();

public:
    enum State
    {
        Invalid = 0,
        Idle,
        Running
    };

private:
    ExaCtrlItf (const ExaCtrlItf& other);
    ExaCtrlItf& operator = (const ExaCtrlItf& other);

public:
    State state() const;

    virtual void  bufferCb( MediaBuffer_t *pBuffer );

    bool  start( exaCtrlSampleCb_t sampleCb, void *pSampleContext, uint8_t sampleSkip = 10 );
    bool  stop();

private:
    class    ExaCtrlHolder;
    ExaCtrlHolder *m_pExaCtrl;
};


/* @} module_name_api*/

#endif /*__EXA_CTRL_ITF_H__*/
