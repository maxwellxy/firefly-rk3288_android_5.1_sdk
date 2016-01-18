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
 * @file som_ctrl_interface.h
 *
 * @brief
 *   SOM (Snapshot Output Module) C++ API.
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

#ifndef __SOM_CTRL_ITF_H__
#define __SOM_CTRL_ITF_H__

#include <hal/hal_api.h>
#include <bufferpool/media_buffer.h>

#include "cam_api/camdevice.h"

/**
 * @brief SomCtrlItf class declaration.
 */
class SomCtrlItf : public BufferCb
{
public:
    /**
     * @brief Standard constructor for the SomCtrlItf object.
     */
    SomCtrlItf( HalHandle_t hHal );
    ~SomCtrlItf();

public:
    enum State
    {
        Invalid = 0,
        Idle,
        Running
    };

private:
    SomCtrlItf (const SomCtrlItf& other);
    SomCtrlItf& operator = (const SomCtrlItf& other);

public:
    State state() const;

    virtual void  bufferCb( MediaBuffer_t *pBuffer );

    bool  start( const char *fileNameBase, 
                    uint32_t frames = 1,
                    uint32_t skip = 0,
                    bool exif = false,
                    bool average = false );
    bool  stop();

    bool  waitForFinished() const;

private:
    class    SomCtrlHolder;
    SomCtrlHolder *m_pSomCtrl;
};


/* @} module_name_api*/

#endif /*__SOM_CTRL_ITF_H__*/
