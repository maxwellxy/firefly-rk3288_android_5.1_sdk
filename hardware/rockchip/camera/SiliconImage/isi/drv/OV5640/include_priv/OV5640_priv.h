#ifndef __OV5640_PRIV_H__
#define __OV5640_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>


/*
 *              OV5640 VERSION NOTE
 *
 *v0.1.0x00 : 1. OV5640 init;
 *v0.2.0x00 : 1. add OV5640 focus;
 *v0.3.0x00 : 1. chang frome 720p to 5M failed ,fix it
 *v0.4.0:
*   1).add senosr drv version in get sensor i2c info func
*v0.5.0:
*   1). support for isi v0.5.0
*v0.6.0
*   1). support for isi v0.6.0
*v0.7.0
*   1). support for isi v0.7.0
*v0.8.0
*   1). for fps, remove svga setting and preview is from 720p. 
*   2). becaus fps's increasing,so the rate fps is increasing.
*v0.9.0
*   1)for preview pic err,set I2C write speed to 100K;
*	  2)don't check sensor ID hight byte,it may fail sometimes;  
*/
#define CONFIG_SENSOR_DRV_VERSION KERNEL_VERSION(0, 8, 0) 

#ifdef __cplusplus
extern "C"
{
#endif

#define OV5640_DELAY_5MS                    (0x0000) //delay 5 ms
#define OV5640_MODE_SELECT                  (0x0100) // rw - Bit[7:1]not used  Bit[0]Streaming set 0: software_standby  1: streaming       
#define OV5640_SOFTWARE_RST                 (0x3008) //(0x0103) // rw - Bit[7:1]not used  Bit[0]software_reset
#define OV5640_SOFTWARE_RST_VALUE            (0x80)

#define OV5640_CHIP_ID_HIGH_BYTE_DEFAULT            (0x56) // r - 
#define OV5640_CHIP_ID_MIDDLE_BYTE_DEFAULT          (0x40) // r - 
#define OV5640_CHIP_ID_LOW_BYTE_DEFAULT             (0x56) // r - 

#define OV5640_CHIP_ID_HIGH_BYTE            (0x300a) // r - 
#define OV5640_CHIP_ID_MIDDLE_BYTE          (0x300b) // r - 
#define OV5640_CHIP_ID_LOW_BYTE             (0x300a) // r - 
/*****************************************************************************
* Further defines for driver management
*****************************************************************************/
#define OV5640_DRIVER_INIT              (0x00000001)

/*****************************************************************************
 * ov14825 context structure
 *****************************************************************************/
typedef struct OV5640_Context_s
{
    IsiSensorContext_t  IsiCtx;                 /**< common context of ISI and ISI driver layer; @note: MUST BE FIRST IN DRIVER CONTEXT */

    //// modify below here ////

    IsiSensorConfig_t   Config;                 /**< sensor configuration */
    bool_t              Configured;             /**< flags that config was applied to sensor */
    bool_t              Streaming;              /**< flags that sensor is streaming data */
    bool_t              TestPattern;            /**< flags that sensor is streaming test-pattern */

    bool_t              isAfpsRun;              /**< if true, just do anything required for Afps parameter calculation, but DON'T access SensorHW! */

    bool_t              GroupHold;

    float               VtPixClkFreq;           /**< pixel clock */
    uint16_t            LineLengthPck;          /**< line length with blanking */
    uint16_t            FrameLengthLines;       /**< frame line length */

    float               AecMaxGain;
    float               AecMinGain;
    float               AecMaxIntegrationTime;
    float               AecMinIntegrationTime;

    float               AecIntegrationTimeIncrement; /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */
    float               AecGainIncrement;            /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */

    float               AecCurGain;
    float               AecCurIntegrationTime;

    uint16_t            OldGain;               /**< gain multiplier */
    uint32_t            OldCoarseIntegrationTime;
    uint32_t            OldFineIntegrationTime;

    IsiSensorMipiInfo   IsiSensorMipiInfo;
} OV5640_Context_t;

#ifdef __cplusplus
}
#endif

#endif

