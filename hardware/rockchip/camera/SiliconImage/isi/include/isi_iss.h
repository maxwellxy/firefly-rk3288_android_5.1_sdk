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
 * @file isi_iss.h
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
 * @defgroup isi_iss CamerIc Driver API
 * @{
 *
 */
#ifndef __ISI_ISS_H__
#define __ISI_ISS_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>
#include <common/list.h>
#include "isi.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 *          IsiRegisterFlags_t
 *
 * @brief   Register permission enumeration type
 */
/*****************************************************************************/
typedef enum IsiRegisterFlags_e
{
    // basic features
    eTableEnd           = 0x00,                                                 /**< special flag for end of register table */
    eReadable           = 0x01,
    eWritable           = 0x02,
    eVolatile           = 0x04,                                                 /**< register can change even if not written by I2C */
    eDelay              = 0x08,                                                 /**< wait n ms */
    eReserved           = 0x10,
    eNoDefault          = 0x20,                                                 /**< no default value specified */
    eTwoBytes           = 0x40,                                                 /**< SMIA sensors use 8-, 16- and 32-bit registers */
    eFourBytes          = 0x80,                                                 /**< SMIA sensors use 8-, 16- and 32-bit registers */

    // combined features
    eReadOnly           = eReadable,
    eWriteOnly          = eWritable,
    eReadWrite          = eReadable | eWritable,
    eReadWriteDel       = eReadable | eWritable | eDelay,
    eReadWriteVolatile  = eReadable | eWritable | eVolatile,
    eReadWriteNoDef     = eReadable | eWritable | eNoDefault,
    eReadWriteVolNoDef  = eReadable | eWritable | eVolatile | eNoDefault,
    eReadVolNoDef       = eReadable | eVolatile | eNoDefault,
    eReadOnlyVolNoDef   = eReadOnly | eVolatile | eNoDefault,

    // additional SMIA features
    eReadOnly_16            = eReadOnly          | eTwoBytes,
    eReadWrite_16           = eReadWrite         | eTwoBytes,
    eReadWriteDel_16        = eReadWriteDel      | eTwoBytes,
    eReadWriteVolatile_16   = eReadWriteVolatile | eTwoBytes,
    eReadWriteNoDef_16      = eReadWriteNoDef    | eTwoBytes,
    eReadWriteVolNoDef_16   = eReadWriteVolNoDef | eTwoBytes,
    eReadOnlyVolNoDef_16    = eReadOnly_16 | eVolatile | eNoDefault,
    eReadOnly_32            = eReadOnly          | eFourBytes,
    eReadWrite_32           = eReadWrite         | eFourBytes,
    eReadWriteVolatile_32   = eReadWriteVolatile | eFourBytes,
    eReadWriteNoDef_32      = eReadWriteNoDef    | eFourBytes,
    eReadWriteVolNoDef_32   = eReadWriteVolNoDef | eFourBytes
} IsiRegisterFlags_t;



/*****************************************************************************/
/**
 *          IsiRegDescription_t
 *
 * @brief   Sensor register description struct
 */
/*****************************************************************************/
typedef struct IsiRegisterFlags_s
{
    uint32_t    Addr;
    uint32_t    DefaultValue;
    const char* pName;
    uint32_t    Flags;
} IsiRegDescription_t;


typedef RESULT (IsiGetSensorIsiVer_t)               ( IsiSensorHandle_t handle, unsigned int* pVersion );//oyyf 
typedef RESULT (IsiGetSensorTuningXmlVersion_t)     ( IsiSensorHandle_t handle, char** pTuningXmlVersion);//oyyf 
typedef RESULT (IsiWhiteBalanceIlluminationChk_t)   ( IsiSensorHandle_t handle, char *name );//ddl@rock-chips.com
typedef RESULT (IsiWhiteBalanceIlluminationSet_t)   ( IsiSensorHandle_t handle, char *name );//ddl@rock-chips.com


typedef RESULT (IsiCreateSensorIss_t)               ( IsiSensorInstanceConfig_t *pConfig );
typedef RESULT (IsiReleaseSensorIss_t)              ( IsiSensorHandle_t handle );
typedef RESULT (IsiGetCapsIss_t)                    ( IsiSensorHandle_t handle, IsiSensorCaps_t *pIsiSensorCaps );
typedef RESULT (IsiSetupSensorIss_t)                ( IsiSensorHandle_t handle, const IsiSensorCaps_t *pIsiSensorCaps );
typedef RESULT (IsiChangeSensorResolutionIss_t)     ( IsiSensorHandle_t handle, const uint32_t Resolution, uint8_t *pNumberOfFramesToSkip );
typedef RESULT (IsiSensorSetStreamingIss_t)         ( IsiSensorHandle_t handle, bool_t on );
typedef RESULT (IsiSensorSetPowerIss_t)             ( IsiSensorHandle_t handle, bool_t on );
typedef RESULT (IsiCheckSensorConnectionIss_t)      ( IsiSensorHandle_t handle );
typedef RESULT (IsiGetSensorRevisionIss_t)          ( IsiSensorHandle_t handle, uint32_t *p_value );
typedef RESULT (IsiRegisterReadIss_t)               ( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );
typedef RESULT (IsiRegisterWriteIss_t)              ( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

/* AEC */
typedef RESULT (IsiExposureControlIss_t)            ( IsiSensorHandle_t handle, const float NewGain, const float NewIntegrationTime, uint8_t *pNumberOfFramesToSkip, float *pSetGain, float *pSetIntegrationTime );
typedef RESULT (IsiGetGainLimitsIss_t)              ( IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain);
typedef RESULT (IsiGetIntegrationTimeLimitsIss_t)   ( IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime );
typedef RESULT (IsiGetCurrentExposureIss_t)         ( IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime );
typedef RESULT (IsiGetGainIss_t)                    ( IsiSensorHandle_t handle, float *pSetGain );
typedef RESULT (IsiGetGainIncrementIss_t)           ( IsiSensorHandle_t handle, float *pIncr );
typedef RESULT (IsiSetGainIss_t)                    ( IsiSensorHandle_t handle, float NewGain, float *pSetGain );
typedef RESULT (IsiGetIntegrationTimeIss_t)         ( IsiSensorHandle_t handle, float *pSetIntegrationTime );
typedef RESULT (IsiGetIntegrationTimeIncrementIss_t)( IsiSensorHandle_t handle, float *pIncr );
typedef RESULT (IsiSetIntegrationTimeIss_t)         ( IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t *pNumberOfFramesToSkip );
typedef RESULT (IsiGetResolutionIss_t)              ( IsiSensorHandle_t handle, uint32_t *pSetResolution );
typedef RESULT (IsiGetAfpsInfoIss_t)                ( IsiSensorHandle_t handle, uint32_t Resolution, IsiAfpsInfo_t* pAfpsInfo);

/* AWB */
typedef RESULT (IsiGetCalibKFactor_t)               ( IsiSensorHandle_t handle, Isi1x1FloatMatrix_t **pIsiKFactor );
typedef RESULT (IsiGetCalibPcaMatrix_t)             ( IsiSensorHandle_t handle, Isi3x2FloatMatrix_t **pIsiPcaMatrix );
typedef RESULT (IsiGetCalibSvdMeanValue_t)          ( IsiSensorHandle_t handle, Isi3x1FloatMatrix_t **pIsiSvdMeanValue );
typedef RESULT (IsiGetCalibCenterLine_t)            ( IsiSensorHandle_t handle, IsiLine_t **ptIsiCenterLine);
typedef RESULT (IsiGetCalibClipParam_t)             ( IsiSensorHandle_t handle, IsiAwbClipParm_t **ptIsiClipParam );
typedef RESULT (IsiGetCalibGlobalFadeParam_t)       ( IsiSensorHandle_t handle, IsiAwbGlobalFadeParm_t **ptIsiGlobalFadeParam);
typedef RESULT (IsiGetCalibFadeParam_t)             ( IsiSensorHandle_t handle, IsiAwbFade2Parm_t **ptIsiFadeParam);
typedef RESULT (IsiGetIlluProfile_t)                ( IsiSensorHandle_t handle, const uint32_t CieProfile, IsiIlluProfile_t **ptIsiIlluProfile );
typedef RESULT (IsiGetLscMatrixTable_t)             ( IsiSensorHandle_t handle, const uint32_t CieProfile, IsiLscMatrixTable_t **pLscMatrixTable );

/* AF */
typedef RESULT (IsiMdiInitMotoDriveMds_t)           ( IsiSensorHandle_t handle );
typedef RESULT (IsiMdiSetupMotoDrive_t)             ( IsiSensorHandle_t handle, uint32_t *pMaxStep );
typedef RESULT (IsiMdiFocusSet_t)                   ( IsiSensorHandle_t handle, const uint32_t AbsStep );
typedef RESULT (IsiMdiFocusGet_t)                   ( IsiSensorHandle_t handle, uint32_t *pAbsStep );
typedef RESULT (IsiMdiFocusCalibrate_t)             ( IsiSensorHandle_t handle );

/*MIPI*/
typedef RESULT (IsiGetSensorMipiInfoIss_t)          ( IsiSensorHandle_t handle ,IsiSensorMipiInfo *ptIsiSensorMipiInfo );


/* Testpattern */
typedef RESULT (IsiActivateTestPattern_t)           ( IsiSensorHandle_t handle, const bool_t enable );

//add for OTP,zyc
typedef int (sensor_i2c_write_t)(void* context,int camsys_fd,const uint32_t reg_address, const uint32_t  value, int* i2c_base_info);
typedef int (sensor_i2c_read_t)(void* context,int camsys_fd,const uint32_t reg_address,int* i2c_base_info);

typedef RESULT (IsiCheckOTPInfo_t)
(
    sensor_i2c_write_t*  sensor_i2c_write_p,
    sensor_i2c_read_t*  sensor_i2c_read_p,
    void* context,
    int camsys_fd
);


/*****************************************************************************/
/**
 *          IsiSensor_t
 *
 * @brief
 *
 */
/*****************************************************************************/
struct IsiSensor_s
{
    const char                          *pszName;                       /**< name of the camera-sensor */
    const IsiRegDescription_t           *pRegisterTable;                /**< pointer to register table */
    const IsiSensorCaps_t               *pIsiSensorCaps;                /**< pointer to sensor capabilities */

	//oyyf add
	IsiGetSensorIsiVer_t				*pIsiGetSensorIsiVer;
	IsiGetSensorTuningXmlVersion_t		*pIsiGetSensorTuningXmlVersion;
	IsiWhiteBalanceIlluminationChk_t    *pIsiWhiteBalanceIlluminationChk;//ddl@rock-chips.com
    IsiWhiteBalanceIlluminationSet_t    *pIsiWhiteBalanceIlluminationSet;//ddl@rock-chips.com
    IsiCheckOTPInfo_t                   *pIsiCheckOTPInfo;  //for OTP,zyc
	
    IsiCreateSensorIss_t                *pIsiCreateSensorIss;           /**< create a sensor handle */
    IsiReleaseSensorIss_t               *pIsiReleaseSensorIss;          /**< release a sensor handle */
    IsiGetCapsIss_t                     *pIsiGetCapsIss;                /**< get sensor capabilities */
    IsiSetupSensorIss_t                 *pIsiSetupSensorIss;            /**< setup sensor capabilities */
    IsiChangeSensorResolutionIss_t      *pIsiChangeSensorResolutionIss; /**< change sensor resolution */
    IsiSensorSetStreamingIss_t          *pIsiSensorSetStreamingIss;     /**< enable/disable streaming of data once sensor is configured */
    IsiSensorSetPowerIss_t              *pIsiSensorSetPowerIss;         /**< turn sensor power on/off */
    IsiCheckSensorConnectionIss_t       *pIsiCheckSensorConnectionIss;

    IsiGetSensorRevisionIss_t           *pIsiGetSensorRevisionIss;      /**< read sensor revision register (if available) */
    IsiRegisterReadIss_t                *pIsiRegisterReadIss;           /**< read sensor register */
    IsiRegisterWriteIss_t               *pIsiRegisterWriteIss;          /**< write sensor register */

    /* AEC functions */
    IsiExposureControlIss_t             *pIsiExposureControlIss;
    IsiGetGainLimitsIss_t               *pIsiGetGainLimitsIss;
    IsiGetIntegrationTimeLimitsIss_t    *pIsiGetIntegrationTimeLimitsIss;
    IsiGetCurrentExposureIss_t          *pIsiGetCurrentExposureIss;     /**< get the currenntly adjusted ae values (gain and integration time) */
    IsiGetGainIss_t                     *pIsiGetGainIss;
    IsiGetGainIncrementIss_t            *pIsiGetGainIncrementIss;
    IsiSetGainIss_t                     *pIsiSetGainIss;
    IsiGetIntegrationTimeIss_t          *pIsiGetIntegrationTimeIss;
    IsiGetIntegrationTimeIncrementIss_t *pIsiGetIntegrationTimeIncrementIss;
    IsiSetIntegrationTimeIss_t          *pIsiSetIntegrationTimeIss;
    IsiGetResolutionIss_t               *pIsiGetResolutionIss;
    IsiGetAfpsInfoIss_t                 *pIsiGetAfpsInfoIss;

    /* AWB functions */
    IsiGetCalibKFactor_t                *pIsiGetCalibKFactor;           /**< get sensor specific K-Factor (comes from calibration) */
    IsiGetCalibPcaMatrix_t              *pIsiGetCalibPcaMatrix;         /**< get sensor specific PCA-Matrix (comes from calibration) */
    IsiGetCalibSvdMeanValue_t           *pIsiGetCalibSvdMeanValue;      /**< get sensor specific SVD-Means (comes from calibration) */
    IsiGetCalibCenterLine_t             *pIsiGetCalibCenterLine;
    IsiGetCalibClipParam_t              *pIsiGetCalibClipParam;
    IsiGetCalibGlobalFadeParam_t        *pIsiGetCalibGlobalFadeParam;
    IsiGetCalibFadeParam_t              *pIsiGetCalibFadeParam;
    IsiGetIlluProfile_t                 *pIsiGetIlluProfile;
    IsiGetLscMatrixTable_t              *pIsiGetLscMatrixTable;

    /* AF functions */
    IsiMdiInitMotoDriveMds_t            *pIsiMdiInitMotoDriveMds;
    IsiMdiSetupMotoDrive_t              *pIsiMdiSetupMotoDrive;
    IsiMdiFocusSet_t                    *pIsiMdiFocusSet;
    IsiMdiFocusGet_t                    *pIsiMdiFocusGet;
    IsiMdiFocusCalibrate_t              *pIsiMdiFocusCalibrate;

    /*MIPI*/
    IsiGetSensorMipiInfoIss_t           *pIsiGetSensorMipiInfoIss;

    /* Testpattern */
    IsiActivateTestPattern_t            *pIsiActivateTestPattern;       /**< enable/disable test-pattern */
};



/*****************************************************************************/
/**
 *          IsiGetSensorIss_t
 *
 * @brief   Only exported function of sensor specific code: fills in
 *          sensor decription struct
 *
 */
/*****************************************************************************/
typedef RESULT (IsiGetSensorIss_t) ( IsiSensor_t *pIsiSensor );


typedef struct sensor_chipid_info_s{
    void                *p_next;
    uint chipid_reg_addr;
    uint chipid_reg_value;
}sensor_chipid_info_t;
typedef struct sensor_caps_s {
    void *p_next;
    IsiSensorCaps_t caps;
}sensor_caps_t;
typedef struct sensor_i2c_info_s{
    uint i2c_addr;
    uint i2c_addr2;
    uint soft_reg_addr;
    uint soft_reg_value;
    uint reg_size;
    uint value_size;
    List chipid_info;
    List lane_res[3];

	unsigned int sensor_drv_version;//oyyf
}sensor_i2c_info_t;

typedef RESULT (IsiGetSensorI2cInfo_t) (sensor_i2c_info_t **pdata);
/*****************************************************************************/
/**
 *          IsiCamDrvConfig_t
 *
 * @brief   Camera sensor driver specific data
 *
 */
/*****************************************************************************/
typedef struct IsiCamDrvConfig_s
{
    uint32_t            CameraDriverID;
    IsiGetSensorIss_t   *pfIsiGetSensorIss;
    IsiSensor_t         IsiSensor;
    IsiGetSensorI2cInfo_t *pfIsiGetSensorI2cInfo;
} IsiCamDrvConfig_t;



#ifdef __cplusplus
}
#endif

#endif /* __ISI_ISS_H__ */

