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
 * @file mapcaps.h
 *
 * @brief
 *   Mapping of ISI capabilities / configuration to CamerIC modes.
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

#ifndef __MAPCAPS_H__
#define __MAPCAPS_H__

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <isi/isi.h>
#include <common/mipi.h>
#include <cam_engine/cam_engine_api.h>


template <typename  T>
extern T isiCapValue( uint32_t cap );

template <typename  T>
extern bool isiCapValue( T& value, uint32_t cap );

template <typename  T>
extern const char* isiCapDescription( uint32_t cap );

template <typename  T>
extern bool isiCapDescription( const char* desc, uint32_t cap );

extern bool operator==(const CamEngineWindow_t &lhs, const CamEngineWindow_t &rhs);

extern bool operator!=(const CamEngineWindow_t &lhs, const CamEngineWindow_t &rhs);

/* @} module_name_api*/

#endif /*__MAPCAPS_H__*/

