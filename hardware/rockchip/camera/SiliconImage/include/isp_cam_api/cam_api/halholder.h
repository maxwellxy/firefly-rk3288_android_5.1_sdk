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
 * @file holholder.h
 *
 * @brief
 *   Hal Holder C++ API.
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

#ifndef __HALHOLDER_H__
#define __HALHOLDER_H__

#include <hal/hal_api.h>

/**
 * @brief HalHolder class declaration.
 */
class HalHolder
{
public:
    static HalHandle_t handle(char* dev_filename, HalPara_t *para);
    ~HalHolder();

    static int NumOfCams();

    static HalHolder* m_halHolder;

private:
    HalHolder(char* dev_filename, HalPara_t *para);
    HalHolder (const HalHolder& other);
    HalHolder& operator = (const HalHolder& other);

private:
    static HalHandle_t m_hHal;
};


/* @} module_name_api*/

#endif /*__HALHOLDER_H__*/
