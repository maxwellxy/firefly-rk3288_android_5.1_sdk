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
#ifndef __CAMERIC_MIPI_DRV_API_H__
#define __CAMERIC_MIPI_DRV_API_H__

/**
 * @cond    cameric_mipi
 *
 * @file    cameric_mipi_drv_api.h
 *
 * @brief   This file contains the CamerIC ISP MIPI driver API definitions.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_mipi_drv_api CamerIC MIPI driver API definitions
 * @{
 *
 * @image html cameric20MP_mipi.png "CamerIC MIPI driver" width=\textwidth
 * @image latex cameric20MP_mipi.png "CamerIC MIPI driver" width=\textwidth
 *
 * The CamerIC is equipped with an interface to connect to an image sensor
 * compatible to the Mobile Industry Processor Interface (MIPI) Alliance
 * Standard for camera serial interface 2 (CSI-2). These sensors use a
 * single clock lane and up to 4 data lanes using a serial, differential
 * link each. Since these transmission line specifications usually requires
 * a high speed mixed signal design, which is not part of the CamerIC IP,
 * MIPI-CSI2 compatible data can be fed into CamerIC by using the logical
 * PHY-Protocol interface (PPI) defined in the MIPI Alliance Standard for
 * D-PHY, Version 0.65.
 *
 */
#include <ebase/types.h>

#include <common/mipi.h>
#include <common/return_codes.h>



#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 * @brief   This function enables CamerIC MIPI module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcMipiEnable
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);



/*****************************************************************************/
/**
 * @brief   This function disables CamerIC MIPI module.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcMipiDisable
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);



/*****************************************************************************/
/**
 * @brief   This function returns the status of the mipi module.
 *
 * @param   handle              CamerIc driver handle.
 * @param   pIsEnabled          Pointer to value to store current state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    pIsEnabled is a NULL pointer.
 *
 *****************************************************************************/
extern RESULT CamerIcMipiIsEnabled
(
    CamerIcDrvHandle_t      handle,
    const int32_t           idx,
    bool_t                  *pIsEnabled
);



/*****************************************************************************/
/**
 * @brief   This function returns the number of currently used MIPI lanes.
 *
 * @param   handle              CamerIc driver handle.
 * @param   no_lanes            number of MIPI lanes to use (1..4)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_INVALID_PARM    no_lanes is a NULL pointer.
 *
 *****************************************************************************/
extern RESULT CamerIcMipiGetNumberOfLanes
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    uint32_t            *no_lanes
);



/*****************************************************************************/
/**
 * @brief   This function sets the number of MIPI lanes to use.
 *
 * @param   handle              CamerIC driver handle.
 * @param   no_lanes            Number of MIPI lanes to use (1..4)
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_OUTOFRANGE      number of lanes is out of range.
 *
 *****************************************************************************/
extern RESULT CamerIcMipiSetNumberOfLanes
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    const uint32_t      no_lanes
);



/*****************************************************************************/
/**
 * @brief   This function sets the MIPI virtual channel and data type of the
 *          packages to process.
 *
 * @param   handle              CamerIC driver handle.
 * @param   vc                  virtul channel number
 * @param   dt                  data type of image data
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_BUSY            module is currently in use.
 * @retval  RET_OUTOFRANGE      at least one config parameter is invalid
 * @retval  RET_NOTSUPP         at least one config parameter is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcMipiSetVirtualChannelAndDataType
(
    CamerIcDrvHandle_t          handle,
    const int32_t               idx,
    const MipiVirtualChannel_t  vc,
    const MipiDataType_t        dt
);


/*****************************************************************************/
/**
 * @brief   This function sets the MIPI compression scheme and predictor block.
 *
 * @param   handle              CamerIC driver handle.
 * @param   cs                  compression scheme
 * @param   pred                predictor block
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_BUSY            module is currently in use.
 * @retval  RET_OUTOFRANGE      at least one config parameter is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcMipiSetCompressionSchemeAndPredictorBlock
(
    CamerIcDrvHandle_t                  handle,
    const int32_t                       idx,
    const MipiDataCompressionScheme_t   cs,
    const MipiPredictorBlock_t          pred
);


/*****************************************************************************/
/**
 * @brief   This function enables compressed data processing.
 *
 * @param   handle              CamerIC driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_BUSY            module is currently in use
 * @retval  RET_WRONG_CONFIG    at least one config parameter set before is invalid
 *
 *****************************************************************************/
extern RESULT CamerIcMipiEnableCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);


/*****************************************************************************/
/**
 * @brief   This function disables compressed data processing.
 *
 * @param   handle              CamerIC driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_BUSY            module is currently in use
 *
 *****************************************************************************/
extern RESULT CamerIcMipiDisableCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);


/*****************************************************************************/
/**
 * @brief   This function returns the status of the compressed data processing mode.
 *
 * @param   handle              CamerIc driver handle.
 * @param   pIsEnabled          Pointer to value to store current state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 * @retval  RET_NULL_POINTER    pIsEnabled is a NULL pointer
 *
 *****************************************************************************/
extern RESULT CamerIcMipiIsEnabledCompressedMode
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx,
    bool_t              *pIsEnabled
);


#ifdef __cplusplus
}
#endif

/* @} cameric_mipi_drv_api */

/* @endcond */

#endif /* __CAMERIC_MIPI_DRV_API_H__ */

