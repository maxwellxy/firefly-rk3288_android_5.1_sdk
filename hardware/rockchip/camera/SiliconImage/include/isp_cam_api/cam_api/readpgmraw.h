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
 * @file readpgmraw.h
 *
 * @brief
 *   Read DCT PGM Raw format.
 *
 *****************************************************************************/
/**
 *
 * @mainpage Module Documentation
 *
 *
 * Doc-Id: xx-xxx-xxx-xxx (NAME Implementation Specification)\n
 * Author: NAME
 *
 * DESCRIBE_HERE
 *
 *
 * The manual is divided into the following sections:
 *
 * -@subpage module_name_api_page \n
 * -@subpage module_name_page \n
 *
 * @page module_name_api_page Module Name API
 * This module is the API for the NAME. DESCRIBE IN DETAIL / ADD USECASES...
 *
 * for a detailed list of api functions refer to:
 * - @ref module_name_api
 *
 * @defgroup module_name_api Module Name API
 * @{
 */

#ifndef __READPGMRAW_H__
#define __READPGMRAW_H__

#include <common/picture_buffer.h>

#include <cam_engine/cam_engine_api.h>

typedef struct raw_hdr_data_s
{
    CamEngineWbGains_t      *wbGains;
    CamEngineCcMatrix_t     *ccMatrix;
    CamEngineCcOffset_t     *ccOffset;
    CamEngineBlackLevel_t   *blvl;
} raw_hdr_data_t;

bool    readRaw( const char* fileName, PicBufMetaData_t *pPicBuf );


/* @} module_name_api*/

#endif /*__READPGMRAW_H__*/
