#ifndef __GC0308_PRIV_H__
#define __GC0308_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>



#ifdef __cplusplus
extern "C"
{
#endif

/*v0.1.0:
*   1).add senosr drv version in get sensor i2c info func
*v0.2.0:
*   1). support for isi v0.5.0
*v0.3.0
*   1). support for isi v0.6.0
*v0.4.0
*   1). support for isi v0.7.0
*v0.5.0
*   1). fix format bugs,outformat need to be cb Y cr Y
*/
#define CONFIG_SENSOR_DRV_VERSION KERNEL_VERSION(0, 5, 0) 


#define GC0308_DELAY_5MS                    (0x0000) //delay 5 ms
#define GC0308_MODE_SELECT                  (0x0100) // rw - Bit[7:1]not used  Bit[0]Streaming set 0: software_standby  1: streaming       
#define GC0308_SOFTWARE_RST                 (0xfe) // rw - Bit[7:1]not used  Bit[0]software_reset

#define GC0308_CHIP_ID_HIGH_BYTE_DEFAULT            (0x9b) // r - 
#define GC0308_CHIP_ID_MIDDLE_BYTE_DEFAULT          (0x9b) // r - 
#define GC0308_CHIP_ID_LOW_BYTE_DEFAULT             (0x9b) // r - 

#define GC0308_CHIP_ID_HIGH_BYTE            (0x00) // r - 
#define GC0308_CHIP_ID_MIDDLE_BYTE          (0x00) // r - 
#define GC0308_CHIP_ID_LOW_BYTE             (0x00) // r - 
/*****************************************************************************
* Further defines for driver management
*****************************************************************************/
#define GC0308_DRIVER_INIT              (0x00000001)

/*****************************************************************************
 * ov14825 context structure
 *****************************************************************************/
typedef struct GC0308_Context_s
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
} GC0308_Context_t;

#ifdef __cplusplus
}
#endif

#endif

