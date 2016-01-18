/******************************************************************************
 *
 * Copyright 2011, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file mipi_drv_api.h
 *
 * @brief   Definition of MIPI driver API.
 *
 *****************************************************************************/

#ifndef __MIPI_DRV_API_H__
#define __MIPI_DRV_API_H__

#include <common/mipi.h>

#include <mipi_drv/mipi_drv_common.h>


#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/**
 * @brief   This function creates and initializesthe MIPI driver context.
 *
 * @param   pConfig             Reference to configuration structure.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_OUTOFMEM        not enough memory
 *
 * @note    On succes, this function fills in the DrvHandle from the configuration
 *          structure @ref MipiDrvConfig_s. This returned DrvHandle has to be
 *          used for later driver calls and should be store in application context.
 *
 *****************************************************************************/
extern RESULT MipiDrvInit
(
    MipiDrvConfig_t  *pConfig
);



/*****************************************************************************/
/**
 * @brief   This function releases the MIPI software driver context.
 *
 * @param   MipiDrvHandle       Handle to MIPI driver context as returned by @ref MipiDrvInit.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to release its
 *                              context
 *
 * @note    Allocated memory by @ref MipiDrvInit is also released
 *
 *****************************************************************************/
extern RESULT MipiDrvRelease
(
    MipiDrvHandle_t MipiDrvHandle
);


/*****************************************************************************/
/**
 * @brief   This function configures the MIPI software driver.
 *
 * @param   MipiDrvHandle       Handle to MIPI driver context as returned by @ref MipiDrvInit.
 * @param   pMipiConfig         Refrence to MIPI configuration to use.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to stop the driver
 *                              (maybe the driver is already running)
 *
 *****************************************************************************/
extern RESULT MipiDrvConfig
(
    MipiDrvHandle_t MipiDrvHandle,
    MipiConfig_t    *pMipiConfig
);


/*****************************************************************************/
/**
 * @brief   This function starts the MIPI software driver.
 *
 * @param   MipiDrvHandle       Handle to MIPI driver context as returned by @ref MipiDrvInit.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to stop the driver
 *                              (maybe the driver is already running)
 *
 * @note    This function also starts the interrupt handling
 *
 *****************************************************************************/
extern RESULT MipiDrvStart
(
    MipiDrvHandle_t MipiDrvHandle
);


/*****************************************************************************/
/**
 * @brief   This function stops the MIPI software driver.
 *
 * @param   MipiDrvHandle       Handle to MIPI driver context as returned by @ref MipiDrvInit.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_FAILURE         common error occured
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_WRONG_STATE     driver is in wrong state to stop the driver
 *                              (maybe the driver is already stopped)
 *
 *****************************************************************************/
extern RESULT MipiDrvStop
(
    MipiDrvHandle_t MipiDrvHandle
);


#ifdef __cplusplus
}
#endif

#endif /* __MIPI_DRV_API_H__ */
