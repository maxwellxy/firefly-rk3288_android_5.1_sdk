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
 * @file vom_ctrl_interface.h
 *
 * @brief
 *   VOM (Video Output Module) C++ API.
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

#ifndef __VOM_CTRL_ITF_H__
#define __VOM_CTRL_ITF_H__

#include <hal/hal_api.h>
#include <bufferpool/media_buffer.h>
#include <common/cea_861.h>

#include "cam_api/camdevice.h"

/**
 * @brief VomCtrlItf class declaration.
 */
class VomCtrlItf : public BufferCb
{
public:
    /**
     * @brief Standard constructor for the VomCtrlItf object.
     */
    VomCtrlItf( HalHandle_t hHal );
    ~VomCtrlItf();

public:
    enum State
    {
        Invalid = 0,
        Idle,
        Running
    };

private:
    VomCtrlItf (const VomCtrlItf& other);
    VomCtrlItf& operator = (const VomCtrlItf& other);

public:
    State state() const;

    virtual void  bufferCb( MediaBuffer_t *pBuffer );

    bool  start( bool enable3D = false, Cea861VideoFormat_t format = CEA_861_VIDEOFORMAT_1920x1080p24 );
    bool  stop();

private:
    class    VomCtrlHolder;
    VomCtrlHolder *m_pVomCtrl;
};


/* @} module_name_api*/

#endif /*__VOM_CTRL_ITF_H__*/
