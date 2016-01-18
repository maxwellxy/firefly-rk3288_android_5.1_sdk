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
 * @file dom_ctrl_interface.h
 *
 * @brief
 *   DOM (Display Output Module) C++ API.
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

#ifndef __DOM_CTRL_ITF_H__
#define __DOM_CTRL_ITF_H__

#include <hal/hal_api.h>
#include <bufferpool/media_buffer.h>

#include "cam_api/camdevice.h"

/**
 * @brief DomCtrlItf class declaration.
 */
class DomCtrlItf : public BufferCb
{
public:
    /**
     * @brief Standard constructor for the DomCtrlItf object.
     */
    DomCtrlItf( HalHandle_t hHal, void *hParent = NULL );
    ~DomCtrlItf();

public:
    enum State
    {
        Invalid = 0,
        Idle,
        Running
    };

private:
    DomCtrlItf (const DomCtrlItf& other);
    DomCtrlItf& operator = (const DomCtrlItf& other);

public:
    State state() const;
    void* handle() const;

    virtual void  bufferCb( MediaBuffer_t *pBuffer );

    bool  start();
    bool  stop();

private:
    class    DomCtrlHolder;
    DomCtrlHolder *m_pDomCtrl;
};


/* @} module_name_api*/

#endif /*__DOM_CTRL_ITF_H__*/
