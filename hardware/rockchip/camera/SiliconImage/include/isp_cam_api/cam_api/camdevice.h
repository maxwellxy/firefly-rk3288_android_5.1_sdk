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
 * @file camdevice.h
 *
 * @brief
 *   Cam Device C++ API.
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

#ifndef __CAMDEVICE_H__
#define __CAMDEVICE_H__

#include "cam_api/cam_engine_interface.h"
#include <exa_ctrl/exa_ctrl_api.h>

class DomCtrlItf;
class VomCtrlItf;
class SomCtrlItf;
class ExaCtrlItf;
struct BufferCbContext;

/**
 * @brief BufferCb class declaration.
 */
class BufferCb
{
public:
    virtual void bufferCb( MediaBuffer_t* pMediaBuffer ) = 0;
    virtual ~BufferCb() = 0;
};

inline BufferCb::~BufferCb() { }

/**
 * @brief CamDevice class declaration.
 */
class CamDevice : public CamEngineItf
{
public:
    /**
     * @brief Standard constructor for the CamDevice object.
     */
    CamDevice( HalHandle_t hHal, AfpsResChangeCb_t *pcbResChange = NULL, void *ctxCbResChange = NULL,void* hParent=NULL, int mipiLaneNum=1 );
    ~CamDevice();

private:
    CamDevice (const CamDevice& other);
    CamDevice& operator = (const CamDevice& other);

public:
    //zyc add
    bool setSensorResConfig(uint32_t mask);
    
    
    void enableDisplayOutput( bool enable = true );
    void enableVideoOutput( bool enable = true );
    void enableExternalAlgorithm( bool enable = true );

    bool connectCamera( bool preview = true, BufferCb *bufferCb = NULL );
    void disconnectCamera();
    void resetCamera();

    bool startPreview();
    bool pausePreview();
    bool stopPreview ();

    bool captureSnapshot( const char* fileName, int type, uint32_t resolution, CamEngineLockType_t locks = CAM_ENGINE_LOCK_ALL );

    void registerExternalAlgorithmCallback( exaCtrlSampleCb_t sampleCb, void *pSampleContext, uint8_t sampleSkip );

private:
    friend class PfidItf;

    bool            m_enableDom;
    bool            m_enableVom;
    bool            m_enableExa;

    DomCtrlItf      *m_domCtrl;
    VomCtrlItf      *m_vomCtrl;
    SomCtrlItf      *m_somCtrl;
    ExaCtrlItf      *m_exaCtrl;

    BufferCbContext *m_bufferCbCtx;

    exaCtrlSampleCb_t m_sampleCb;
    void            *m_sampleCbCtx;
    uint8_t         m_sampleSkip;
};


/* @} module_name_api*/

#endif /*__CAMDEVICE_H__ */
