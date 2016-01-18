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
 * @file isi.h
 *
 * @brief Interface description for image sensor specific implementation (iss).
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup isi Independent Sensor Interface
 * @{
 *
 */
#ifndef __ISI_H__
#define __ISI_H__

#include <ebase/types.h>
#include <hal/hal_api.h>

#include <isi/isi_common.h>
#include <linux/version.h>


/*
*v0.2.0: 
*    1) support ov8858 and ov13850 driver
*v0.3.0:
*    1) add support vcm current information;  
*v0.4.0:
*	 1) add sensor drv version to prop
*v0.5.0:
*    1) add IsiWhiteBalanceIlluminationSet and IsiWhiteBalanceIlluminationChk api
*v0.6.0:
*    1) modify resolution macro ISI_RES_XXXX;
*v0.7.0
*    1) add struct sensor_caps_t in struct sensor_i2c_info_t, IsiGetSensorI2cInfo support enum resolution; 
*v0.8.0
*    1) support OTP;
*v0.9.0
*    1) support read OTP by transfer i2c info;
*    2) support new resolutin 4208x3120 & 2104x1560
*v0.0xa.0
*    2) support new resolutin 1600x1200 
*/

#define CONFIG_ISI_VERSION KERNEL_VERSION(0, 0x0a, 0x00) 


#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 *          IsiSensorHandle_t
 *
 * @brief   Handle to a sensor instance
 *
 */
/*****************************************************************************/
typedef void *IsiSensorHandle_t; // should be of type 'struct IsiSensorContext_s *'



/*****************************************************************************/
/**
 * @brief
 */
/*****************************************************************************/
typedef struct IsiSensor_s IsiSensor_t;



/*****************************************************************************/
/**
 * @brief   This structure defines the sensor capabilities.
 */
/*****************************************************************************/
typedef struct IsiSensorCaps_s
{
    uint32_t BusWidth;                  /**< supported bus-width */
    uint32_t Mode;                      /**< supported operating modes */
    uint32_t FieldSelection;            /**< sample fields */
    uint32_t YCSequence;
    uint32_t Conv422;
    uint32_t BPat;                      /**< bayer pattern */
    uint32_t HPol;                      /**< horizontal polarity */
    uint32_t VPol;                      /**< vertical polarity */
    uint32_t Edge;                      /**< sample edge */
    uint32_t Bls;                       /**< black level substraction */
    uint32_t Gamma;                     /**< gamma */
    uint32_t CConv;
    uint32_t Resolution;                /**< supported resolutions */
    uint32_t DwnSz;
    uint32_t BLC;
    uint32_t AGC;
    uint32_t AWB;
    uint32_t AEC;
    uint32_t DPCC;
    uint32_t CieProfile;
    uint32_t SmiaMode;
    uint32_t MipiMode;
    uint32_t AfpsResolutions;           /**< resolutions supported by Afps */
	uint32_t SensorOutputMode;

	uint32_t Index;
} IsiSensorCaps_t;



/*****************************************************************************/
/**
 *          IsiSensorConfig_t
 *
 * @brief   Sensor configuration struct
 */
/*****************************************************************************/
typedef IsiSensorCaps_t IsiSensorConfig_t;



/*****************************************************************************/
/**
 *          IsiSensorInstanceConfig_t
 *
 * @brief   Config structure to create a new sensor instance
 *
 */
/*****************************************************************************/
typedef struct IsiSensorInstanceConfig_s
{
    HalHandle_t         HalHandle;          /**< Handle of HAL session to use. */
    uint32_t            HalDevID;           /**< HAL device ID of this sensor. */
    uint8_t             I2cBusNum;          /**< The I2C bus the sensor is connected to. */
    uint16_t            SlaveAddr;          /**< The I2C slave addr the sensor is configured to. */
    uint8_t             I2cAfBusNum;        /**< The I2C bus the ad module is connected to. */
    uint16_t            SlaveAfAddr;        /**< The I2C slave addr of the af module is configured to */
    uint16_t            mipiLaneNum;

    IsiSensor_t         *pSensor;           /**< Sensor driver interface */

    IsiSensorHandle_t   hSensor;            /**< Sensor handle returned by IsiCreateSensorIss */

    uint32_t             VcmStartCurrent;   /* ddl@rock-chips.com: v0.3.0 */
    uint32_t             VcmRatedCurrent;
    uint32_t             VcmMaxCurrent;
    uint32_t             VcmDrvMaxCurrent;
    uint32_t             VcmStepMode;
    
} IsiSensorInstanceConfig_t;



/*****************************************************************************/
/**
 * @brief   This structure defines a single Afps resolution's data.
 */
/*****************************************************************************/
typedef struct IsiAfpsResInfo_s
{
    uint32_t Resolution;                /**< the corresponding resolution ID */
    float    MaxIntTime;                /**< the maximum supported integration time */
} IsiAfpsResInfo_t;



/*****************************************************************************/
/**
 * @brief   The number of supported Afps sub resolution stages.
 */
/*****************************************************************************/
#define ISI_NUM_AFPS_STAGES 32

/*****************************************************************************/
/**
 * @brief   This structure lists the supported Afps sub resolution stages.
 */
/*****************************************************************************/
typedef struct IsiAfpsInfo_s
{
    float       AecMinGain;                         /**< minimum gain for AEC in Afps mode */
    float       AecMaxGain;                         /**< maximum gain for AEC in Afps mode */
    float       AecMinIntTime;                      /**< minimum integration time for AEC in Afps mode */
    float       AecMaxIntTime;                      /**< maximum integration time for AEC in Afps mode */
    uint32_t    AecSlowestResolution;               /**< slowst resolution for AEC in Afps mode */

    IsiAfpsResInfo_t Stage[ISI_NUM_AFPS_STAGES];    /**< the list of supported resolutions with .MaxIntTime in ascending(!) order;
                                                         Resolution = 0 marks end of list if not all array elements are used */

    uint32_t    CurrResolution;                     /**< current resolution */
    float       CurrMinIntTime;                     /**< minimum integration time of current resolution */
    float       CurrMaxIntTime;                     /**< maximum integration time of current resolution */
} IsiAfpsInfo_t;

/*****************************************************************************/
/**
 *          IsiGetSensorIsiVer
 *
 * @brief   This function creates a new sensor instance.
 *
 * @param	handle		sensor instance handle
 * @param	version		sensor match isi version
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT IsiGetSensorIsiVer
(
    IsiSensorHandle_t   handle,
    unsigned int* pVersion
);

RESULT IsiGetSensorTuningXmlVersion
(
    IsiSensorHandle_t   handle,
    char** pTuningXmlVersion
);

/*****************************************************************************/
/**
 *          IsiCreateSensorIss
 *
 * @brief   This function creates a new sensor instance.
 *
 * @param   pConfig     configuration of the new sensor
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT IsiCreateSensorIss
(
    IsiSensorInstanceConfig_t   *pConfig
);



/*****************************************************************************/
/**
 *          IsiReleaseSensorIss
 *
 * @brief   The function destroys/releases a sensor instance.
 *
 * @param   handle      sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NOTSUPP
 *
 *****************************************************************************/
RESULT IsiReleaseSensorIss
(
    IsiSensorHandle_t   handle
);



/*****************************************************************************/
/**
 *          IsiGetCapsIss
 *
 * @brief   fills in the correct pointers for the sensor description struct
 *
 * @param   handle      Sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCapsIss
(
    IsiSensorHandle_t   handle,
    IsiSensorCaps_t     *pIsiSensorCaps
);



/*****************************************************************************/
/**
 *          IsiSetupSensorIss
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      Sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiSetupSensorIss
(
    IsiSensorHandle_t   handle,
    IsiSensorConfig_t   *pConfig
);



/*****************************************************************************/
/**
 *          IsiChangeSensorResolutionIss
 *
 * @brief   Change image sensor resolution while keeping all other static settings.
 *          Dynamic settings like current gain & integration time are kept as
 *          close as possible.
 *
 * @note    Re-read current & min/max values as they will probably have changed!
 *
 * @param   handle                  Sensor instance handle
 * @param   Resolution              new resolution (specified as capability)
 * @param   pNumberOfFramesToSkip   reference to storage for number of frames to skip
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT IsiChangeSensorResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    uint8_t             *pNumberOfFramesToSkip
);



/*****************************************************************************/
/**
 *          IsiSensorSetStreamingIss
 *
 * @brief   Enables/disables streaming of sensor data, if possible.
 *
 * @param   handle      Sensor instance handle
 * @param   on          new streaming state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT IsiSensorSetStreamingIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
);



/*****************************************************************************/
/**
 *          IsiSensorSetPowerIss
 *
 * @brief   Performs the power-up/power-down sequence of the camera, if possible.
 *
 * @param   handle      Sensor instance handle
 * @param   on          new power state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiSensorSetPowerIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
);



/*****************************************************************************/
/**
 *          IsiCheckSensorConnectionIss
 *
 * @brief   Checks the connection to the camera sensor, if possible.
 *
 * @param   handle      Sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiCheckSensorConnectionIss
(
    IsiSensorHandle_t   handle
);



/*****************************************************************************/
/**
 *          IsiGetSensorRevisionIss
 *
 * @brief   This function reads the sensor revision register and returns it.
 *
 * @param   handle      sensor instance handle
 * @param   p_value     pointer to value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_NOTSUPP
 *
 *****************************************************************************/
RESULT IsiGetSensorRevisionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
);



/*****************************************************************************/
/**
 *          IsiGetGainLimitsIss
 *
 * @brief   Returns the gain minimal and maximal values of a sensor
 *          instance
 *
 * @param   handle      sensor instance handle
 * @param   pMinGain    Pointer to a variable receiving minimal exposure value
 * @param   pMaxGain    Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetGainLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinGain,
    float               *pMaxGain
);



/*****************************************************************************/
/**
 *          IsiGetIntegrationTimeLimitsIss
 *
 * @brief   Returns the integration time minimal and maximal values of a sensor
 *          instance
 *
 * @param   handle                  sensor instance handle
 * @param   pMinIntegrationTime     Pointer to a variable receiving minimal exposure value
 * @param   pMaxIntegrationTime     Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetIntegrationTimeLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinIntegrationTime,
    float               *pMaxIntegrationTime
);


/*****************************************************************************/
/**
 *          IsiExposureControlIss
 *
 * @brief   Sets the exposure values (gain & integration time) of a sensor
 *          instance
 *
 * @param   handle                  sensor instance handle
 * @param   NewGain                 newly calculated gain to be set
 * @param   NewIntegrationTime      newly calculated integration time to be set
 * @param   pNumberOfFramesToSkip   number of frames to skip until AE is
 *                                  executed again
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiExposureControlIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               NewIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip,
    float               *pSetGain,
    float               *pSetIntegrationTime
);



/*****************************************************************************/
/**
 *          IsiGetCurrentExposureIss
 *
 * @brief   Returns the currently adjusted AE values
 *
 * @param   handle       sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCurrentExposureIss
(
    IsiSensorHandle_t   handle,
    float               *pCurGain,
    float               *pCurIntegrationTime
);



/*****************************************************************************/
/**
 *          IsiGetAfpsInfoIss
 *
 * @brief   Returns the possible AFPS sub resolution settings for the given resolution series
 *
 * @param   handle                  OV14825 sensor instance handle
 * @param   Resolution              Any resolution within the AFPS group to query;
 *                                  0 (zero) to use the currently configured resolution
 * @param   pAfpsInfo               Reference of AFPS info structure to store the results
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_NOTSUPP
 *
 *****************************************************************************/
RESULT IsiGetAfpsInfoIss(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo
);



/*****************************************************************************/
/**
 *          IsiGetGainIss
 *
 * @brief   Reads gain values from the image sensor module.
 *
 * @param   handle                  sensor instance handle
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetGainIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain
);



/*****************************************************************************/
/**
 *          IsiGetGainIncrementIss
 *
 * @brief   Get smalles possible gain increment.
 *
 * @param   handle                  sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetGainIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
);



/*****************************************************************************/
/**
 *          IsiSetGainIss
 *
 * @brief   Writes gain values to the image sensor module.
 *
 * @param   handle                  sensor instance handle
 * @param   NewGain                 gain to be set
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
);



/*****************************************************************************/
/**
 *          IsiGetIntegrationTimeIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  sensor instance handle
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               *pSetIntegrationTime
);



/*****************************************************************************/
/**
 *          IsiGetIntegrationTimeIncrementIss
 *
 * @brief   Get smalles possible integration time increment.
 *
 * @param   handle                  sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetIntegrationTimeIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
);



/*****************************************************************************/
/**
 *          IsiSetIntegrationTimeIss
 *
 * @brief   Writes integration time values to the image sensor module.
 *
 * @param   handle                  sensor instance handle
 * @param   NewIntegrationTime      integration time to be set
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiSetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               NewIntegrationTime,
    float               *pSetIntegrationTime
);



/*****************************************************************************/
/**
 *          IsiGetResolutionIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  sensor instance handle
 * @param   pSettResolution         set resolution
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *pSetResolution
);



/*****************************************************************************/
/**
 *          IsiWriteRegister
 *
 * @brief   writes a given number of bytes to the image sensor device by
 *          calling the corresponding sensor-function
 *
 * @param   handle              Handle to image sensor device
 * @param   RegAddress          register address
 * @param   RegValue            value to write
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NOTSUPP
 *
 *****************************************************************************/
RESULT IsiWriteRegister
(
    IsiSensorHandle_t   handle,
    const uint32_t      RegAddress,
    const uint32_t      RegValue
);



/*****************************************************************************/
/**
 *          IsiReadRegister
 *
 * @brief   reads a given number of bytes from the image sensor device
 *
 * @param   handle              Handle to image sensor device
 * @param   RegAddress          register address
 * @param   RegValue            value to write
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_NOTSUPP
 *
 *****************************************************************************/
RESULT IsiReadRegister
(
    IsiSensorHandle_t   handle,
    const uint32_t      RegAddress,
    uint32_t            *pRegValue
);



/*****************************************************************************/
/**
 *          IsiGetCalibKFactor
 *
 * @brief   Returns the sensor specific K-Factor
 *
 * @param   handle       Handle to image sensor device
 * @param   pIsiKFactor  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCalibKFactor
(
    IsiSensorHandle_t   handle,
    Isi1x1FloatMatrix_t **pIsiKFactor
);



/*****************************************************************************/
/**
 *          IsiGetCalibPcaMatrix
 *
 * @brief   Returns the sensor specific PCA-Matrix
 *
 * @param   handle          sensor instance handle
 * @param   pIsiPcaMatrix   Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCalibPcaMatrix
(
    IsiSensorHandle_t   handle,
    Isi3x2FloatMatrix_t **pIsiPcaMatrix
);


/*****************************************************************************/
/**
 *          IsiGetCalibSvdMeanValue
 *
 * @brief   Returns the sensor specific SvdMean-Vector
 *
 * @param   handle              sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCalibSvdMeanValue
(
    IsiSensorHandle_t   handle,
    Isi3x1FloatMatrix_t **pIsiSvdMeanValue
);



/*****************************************************************************/
/**
 *          IsiGetCalibCenterLine
 *
 * @brief   Returns the sensor specific K-Factor
 *
 * @param   handle          Handle to image sensor device
 * @param   ptIsiCenterLine Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCalibCenterLine
(
    IsiSensorHandle_t   handle,
    IsiLine_t           **ptIsiCenterLine
);



/*****************************************************************************/
/**
 *          IsiGetCalibClipParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for Rg/Bg color
 *          space clipping
 *
 * @param   handle              sensor instance handle
 * @param   ptIsiClipParam      Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCalibClipParam
(
    IsiSensorHandle_t   handle,
    IsiAwbClipParm_t    **ptIsiClipParam
);



/*****************************************************************************/
/**
 *          IsiGetCalibGlobalFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle               sensor instance handle
 * @param   ptIsiGlobalFadeParam Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCalibGlobalFadeParam
(
    IsiSensorHandle_t       handle,
    IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam
);



/*****************************************************************************/
/**
 *          IsiGetCalibFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle          sensor instance handle
 * @param   ptIsiFadeParam  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetCalibFadeParam
(
    IsiSensorHandle_t   handle,
    IsiAwbFade2Parm_t   **ptIsiFadeParam
);



/*****************************************************************************/
/**
 *          IsiGetIlluProfile
 *
 * @brief   Returns a pointer to illumination profile identified by CieProfile
 *          bitmask
 *
 * @param   handle              sensor instance handle
 * @param   CieProfile
 * @param   ptIsiIlluProfile    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetIlluProfile
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiIlluProfile_t    **ptIsiIlluProfile
);



/*****************************************************************************/
/**
 *          IsiGetLscMatrixTable
 *
 * @brief   Returns a pointer to vignetted LSC-Matrices identified by
 *          CieProfile bitmask
 *
 * @param   handle              sensor instance handle
 * @param   CieProfile
 * @param   pLscMatrixTable     Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiGetLscMatrixTable
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiLscMatrixTable_t **pLscMatrixTable
);



/*****************************************************************************/
/**
 *          IsiMdiInitMotoDrive
 *
 * @brief   General initialisation tasks like I/O initialisation.
 *
 * @param   handle          sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiMdiInitMotoDrive
(
    IsiSensorHandle_t   handle
);



/*****************************************************************************/
/**
 *          IsiMdiSetupMotoDrive
 *
 * @brief   Setup of the MotoDrive and return possible max step.
 *
 * @param   handle          sensor instance handle
 *          pMaxStep        pointer to variable to receive the maximum
 *                          possible focus step
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiMdiSetupMotoDrive
(
    IsiSensorHandle_t   handle,
    uint32_t            *pMaxStep
);



/*****************************************************************************/
/**
 *          IsiMdiFocusSet
 *
 * @brief   Drives the lens system to a certain focus point.
 *
 * @param   handle          sensor instance handle
 *          AbsStep         absolute focus point to apply
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiMdiFocusSet
(
    IsiSensorHandle_t   handle,
    const uint32_t      AbsStep
);



/*****************************************************************************/
/**
 *          IsiMdiFocusGet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          sensor instance handle
 *          pAbsStep        pointer to a variable to receive the current
 *                          focus point
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiMdiFocusGet
(
    IsiSensorHandle_t   handle,
    uint32_t            *pAbsStep
);



/*****************************************************************************/
/**
 *          IsiMdiFocusCalibrate
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiMdiFocusCalibrate
(
    IsiSensorHandle_t   handle
);



/*****************************************************************************/
/**
 *          IsiActivateTestPattern
 *
 * @brief   Activates or deactivates sensor's test-pattern (normally a defined
 *          colorbar )
 *
 * @param   handle          sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiActivateTestPattern
(
    IsiSensorHandle_t   handle,
    const bool_t        enable
);



/*****************************************************************************/
/**
 *          IsiDumpAllRegisters
 *
 * @brief   Activates or deactivates sensor's test-pattern (normally a defined
 *          colorbar )
 *
 * @param   handle          sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT IsiDumpAllRegisters
(
    IsiSensorHandle_t   handle,
    const uint8_t       *filename
);



/*****************************************************************************/
/**
 *          IsiTryToSetConfigFromPreferredCaps
 *
 * @brief   Tries to set the referenced sensor config parameter to the first of the
 *          given preferred capabilities that is included in the given capability
 *          mask. If none of the preferred capabilities is supported, the config
 *          parameter value remains unchanged.
 *
 * @note    Use this function for example to modify the retrieved default sensor
 *          config parameter for parameter according to some external preferences
 *          while taking the retrieved sensor capabilities for that config parameter
 *          into account.
 *
 * @param   pConfigParam    reference to parameter of sensor config structure
 * @param   prefList        reference to 0 (zero) terminated array of preferred
 *                          capability values in descending order
 * @param   capsmask        bitmask of supported capabilites for that parameter
 *
 * @return  Return the result of the function call.
 * @retval  BOOL_TRUE       preferred capability set in referenced config parameter
 * @retval  BOOL_FALSE      preferred capability not supported
 *
 *****************************************************************************/
bool_t IsiTryToSetConfigFromPreferredCaps
(
    uint32_t    *pConfigParam,
    uint32_t    *prefList,
    uint32_t    capsmask
);



/*****************************************************************************/
/**
 *          IsiTryToSetConfigFromPreferredCap
 *
 * @brief   Tries to set referenced sensor config parameter to the given preferred
 *          capability while checking that capability against the given capability
 *          mask. If that capability isn't supported, the config parameter value
 *          remains unchanged.
 *
 * @note    Use this function for example to modify the retrieved default sensor
 *          config parameter for parameter according to some external preferences
 *          while taking the retrieved sensor capabilities for that config parameter
 *          into account.
 *
 * @param   pConfigParam    reference to parameter of sensor config structure
 * @param   prefcap         preferred capability value
 * @param   capsmask        bitmask of supported capabilites for that parameter
 *
 * @return  Return the result of the function call.
 * @retval  BOOL_TRUE       preferred capability set in referenced config parameter
 * @retval  BOOL_FALSE      preferred capability not supported
 *
 *****************************************************************************/
bool_t IsiTryToSetConfigFromPreferredCap
(
    uint32_t        *pConfigParam,
    const uint32_t  prefcap,
    const uint32_t  capsmask
);


/*****************************************************************************/
/**
 *          IsiGetResolutionName
 *
 * @brief   Returns a pointer to the zero terminated name of the given resolution.
 *
 * @param   Resolution      resolution to query name for
 * @param   pszName         reference to string pointer
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFRANGE  pszName nevertheless points to a valid string
 *
 *****************************************************************************/
RESULT IsiGetResolutionName
(
    uint32_t    Resolution,
    char        **pszName
);


/*****************************************************************************/
/**
 *          IsiGetResolutionParam
 *
 * @brief   Returns the paramter of a resolution given by bitmask 
 *
 * @param   Resolution      resolution to query parameter for
 * @param   width           reference to width
 * @param   height          reference to height
 * @param   fps             reference to framerate per second
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFRANGE  pszName nevertheless points to a valid string
 *
 *****************************************************************************/
RESULT IsiGetResolutionParam
(
    uint32_t    Resolution,
    uint16_t    *width,
    uint16_t    *height,
    uint16_t    *fps
);


/*****************************************************************************/
/**
 *          IsiWhiteBalanceIlluminationChk
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          sensor instance handle
 *          name            illumination name for check sensor driver is support or not
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 
 *****************************************************************************/
RESULT IsiWhiteBalanceIlluminationChk
(
    IsiSensorHandle_t   handle,
    char                *name
);

/*****************************************************************************/
/**
 *          IsiWhiteBalanceIlluminationSet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          sensor instance handle
 *          name            illumination name which be set to sensor driver;
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 
 *****************************************************************************/
RESULT IsiWhiteBalanceIlluminationSet
(
    IsiSensorHandle_t   handle,
    char                *name
);



#ifdef __cplusplus
}
#endif


/* @} isi */


#endif /* __ISI_H__ */
