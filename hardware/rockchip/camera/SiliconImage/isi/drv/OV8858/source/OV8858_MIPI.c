//OV8858 the same with ov14825

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
 * @file OV8858.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"

#include "OV8858_MIPI_priv.h"

#define  OV8858_NEWEST_TUNING_XML "18-7-2014_oyyf-hkw_OV8858_CMK-CB0407-FV1_v0.1.2"

//hkw no use;
#define CC_OFFSET_SCALING  2.0f
#define I2C_COMPLIANT_STARTBIT 1U

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( OV8858_INFO , "OV8858: ", INFO,    0U );
CREATE_TRACER( OV8858_WARN , "OV8858: ", WARNING, 1U );
CREATE_TRACER( OV8858_ERROR, "OV8858: ", ERROR,   1U );

CREATE_TRACER( OV8858_DEBUG, "OV8858: ", INFO,     0U );

CREATE_TRACER( OV8858_NOTICE0 , "OV8858: ", TRACE_NOTICE0, 1);
CREATE_TRACER( OV8858_NOTICE1, "OV8858: ", TRACE_NOTICE1, 1U );


#define OV8858_SLAVE_ADDR       0x6cU                           /**< i2c slave address of the OV8858 camera sensor */
#define OV8858_SLAVE_ADDR2      0x20U
#define OV8858_SLAVE_AF_ADDR    0x18U         //?                  /**< i2c slave address of the OV8858 integrated AD5820 */
#define Sensor_OTP_SLAVE_ADDR   0x6cU
#define Sensor_OTP_SLAVE_ADDR2   0x20U

#define OV8858_MAXN_GAIN 		(128.0f)
#define OV8858_MIN_GAIN_STEP   ( 1.0f / OV8858_MAXN_GAIN); /**< min gain step size used by GUI ( 32/(32-7) - 32/(32-6); min. reg value is 6 as of datasheet; depending on actual gain ) */
#define OV8858_MAX_GAIN_AEC    ( 8.0f )            /**< max. gain used by the AEC (arbitrarily chosen, recommended by Omnivision) */


/*!<
 * Focus position values:
 * 65 logical positions ( 0 - 64 )
 * where 64 is the setting for infinity and 0 for macro
 * corresponding to
 * 1024 register settings (0 - 1023)
 * where 0 is the setting for infinity and 1023 for macro
 */
#define MAX_LOG   64U
#define MAX_REG 1023U

#define MAX_VCMDRV_CURRENT      100U
#define MAX_VCMDRV_REG          1023U




/*!<
 * Lens movement is triggered every 133ms (VGA, 7.5fps processed frames
 * worst case assumed, usually even much slower, see OV5630 driver for
 * details). Thus the lens has to reach the requested position after
 * max. 133ms. Minimum mechanical ringing is expected with mode 1 ,
 * 100us per step. A movement over the full range needs max. 102.3ms
 * (see table 9 AD5820 datasheet).
 */
#define MDI_SLEW_RATE_CTRL 5U /* S3..0 for MOTOR hkw*/



/******************************************************************************
 * local variable declarations
 *****************************************************************************/
const char OV8858_g_acName[] = "OV8858_MIPI";
//extern const IsiRegDescription_t OV8858_g_aRegDescription[];
extern const IsiRegDescription_t OV8858_g_aRegDescription_onelane[];
extern const IsiRegDescription_t OV8858_g_aRegDescription_twolane[];
extern const IsiRegDescription_t OV8858_g_aRegDescription_fourlane[];
extern const IsiRegDescription_t OV8858_g_1632x1224_onelane[];
extern const IsiRegDescription_t OV8858_g_1632x1224_twolane[];
//extern const IsiRegDescription_t OV8858_g_1632x1224P20_twolane[];
//extern const IsiRegDescription_t OV8858_g_1632x1224P10_twolane[];
extern const IsiRegDescription_t OV8858_g_1632x1224P30_twolane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224P25_twolane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224P20_twolane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224P15_twolane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224P10_twolane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224_fourlane[];
extern const IsiRegDescription_t OV8858_g_1632x1224P30_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224P25_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224P20_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224P15_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_1632x1224P10_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_3264x2448_onelane[];
extern const IsiRegDescription_t OV8858_g_3264x2448_twolane[];
extern const IsiRegDescription_t OV8858_g_3264x2448P15_twolane_fpschg[];
extern const IsiRegDescription_t OV8858_g_3264x2448P7_twolane_fpschg[];
extern const IsiRegDescription_t OV8858_g_3264x2448_fourlane[];
extern const IsiRegDescription_t OV8858_g_3264x2448P30_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_3264x2448P25_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_3264x2448P20_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_3264x2448P15_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_3264x2448P10_fourlane_fpschg[];
extern const IsiRegDescription_t OV8858_g_3264x2448P7_fourlane_fpschg[];

//R2A
extern const IsiRegDescription_t OV8858_g_aRegDescription_twolane_R2A[];
extern const IsiRegDescription_t OV8858_g_3264x2448_twolane_R2A[];
extern const IsiRegDescription_t OV8858_g_1632x1224_twolane_R2A[];
extern const IsiRegDescription_t OV8858_g_aRegDescription_fourlane_R2A[];
extern const IsiRegDescription_t OV8858_g_3264x2448_fourlane_R2A[];
extern const IsiRegDescription_t OV8858_g_1632x1224_fourlane_R2A[];




const IsiSensorCaps_t OV8858_g_IsiSensorDefaultConfig;



#define OV8858_I2C_START_BIT        (I2C_COMPLIANT_STARTBIT)    // I2C bus start condition
#define OV8858_I2C_NR_ADR_BYTES     (2U)                        // 1 byte base address and 2 bytes sub address
#define OV8858_I2C_NR_DAT_BYTES     (1U)                        // 8 bit registers

static uint16_t g_suppoted_mipi_lanenum_type = SUPPORT_MIPI_ONE_LANE|SUPPORT_MIPI_TWO_LANE|SUPPORT_MIPI_FOUR_LANE;
#define DEFAULT_NUM_LANES SUPPORT_MIPI_TWO_LANE




/******************************************************************************
 * local function prototypes
 *****************************************************************************/
static RESULT OV8858_IsiCreateSensorIss( IsiSensorInstanceConfig_t *pConfig );
static RESULT OV8858_IsiReleaseSensorIss( IsiSensorHandle_t handle );
static RESULT OV8858_IsiGetCapsIss( IsiSensorHandle_t handle, IsiSensorCaps_t *pIsiSensorCaps );
static RESULT OV8858_IsiSetupSensorIss( IsiSensorHandle_t handle, const IsiSensorConfig_t *pConfig );
static RESULT OV8858_IsiSensorSetStreamingIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV8858_IsiSensorSetPowerIss( IsiSensorHandle_t handle, bool_t on );
static RESULT OV8858_IsiCheckSensorConnectionIss( IsiSensorHandle_t handle );
static RESULT OV8858_IsiGetSensorRevisionIss( IsiSensorHandle_t handle, uint32_t *p_value);

static RESULT OV8858_IsiGetGainLimitsIss( IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain);
static RESULT OV8858_IsiGetIntegrationTimeLimitsIss( IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime );
static RESULT OV8858_IsiExposureControlIss( IsiSensorHandle_t handle, float NewGain, float NewIntegrationTime, uint8_t *pNumberOfFramesToSkip, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV8858_IsiGetCurrentExposureIss( IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime );
static RESULT OV8858_IsiGetAfpsInfoIss ( IsiSensorHandle_t handle, uint32_t Resolution, IsiAfpsInfo_t* pAfpsInfo);
static RESULT OV8858_IsiGetGainIss( IsiSensorHandle_t handle, float *pSetGain );
static RESULT OV8858_IsiGetGainIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV8858_IsiSetGainIss( IsiSensorHandle_t handle, float NewGain, float *pSetGain );
static RESULT OV8858_IsiGetIntegrationTimeIss( IsiSensorHandle_t handle, float *pSetIntegrationTime );
static RESULT OV8858_IsiGetIntegrationTimeIncrementIss( IsiSensorHandle_t handle, float *pIncr );
static RESULT OV8858_IsiSetIntegrationTimeIss( IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t *pNumberOfFramesToSkip );
static RESULT OV8858_IsiGetResolutionIss( IsiSensorHandle_t handle, uint32_t *pSetResolution );


static RESULT OV8858_IsiRegReadIss( IsiSensorHandle_t handle, const uint32_t address, uint32_t *p_value );
static RESULT OV8858_IsiRegWriteIss( IsiSensorHandle_t handle, const uint32_t address, const uint32_t value );

static RESULT OV8858_IsiGetCalibKFactor( IsiSensorHandle_t handle, Isi1x1FloatMatrix_t **pIsiKFactor );
static RESULT OV8858_IsiGetCalibPcaMatrix( IsiSensorHandle_t   handle, Isi3x2FloatMatrix_t **pIsiPcaMatrix );
static RESULT OV8858_IsiGetCalibSvdMeanValue( IsiSensorHandle_t   handle, Isi3x1FloatMatrix_t **pIsiSvdMeanValue );
static RESULT OV8858_IsiGetCalibCenterLine( IsiSensorHandle_t   handle, IsiLine_t  **ptIsiCenterLine);
static RESULT OV8858_IsiGetCalibClipParam( IsiSensorHandle_t   handle, IsiAwbClipParm_t    **pIsiClipParam );
static RESULT OV8858_IsiGetCalibGlobalFadeParam( IsiSensorHandle_t       handle, IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam);
static RESULT OV8858_IsiGetCalibFadeParam( IsiSensorHandle_t   handle, IsiAwbFade2Parm_t   **ptIsiFadeParam);
static RESULT OV8858_IsiGetIlluProfile( IsiSensorHandle_t   handle, const uint32_t CieProfile, IsiIlluProfile_t **ptIsiIlluProfile );

static RESULT OV8858_IsiMdiInitMotoDriveMds( IsiSensorHandle_t handle );
static RESULT OV8858_IsiMdiSetupMotoDrive( IsiSensorHandle_t handle, uint32_t *pMaxStep );
static RESULT OV8858_IsiMdiFocusSet( IsiSensorHandle_t handle, const uint32_t Position );
static RESULT OV8858_IsiMdiFocusGet( IsiSensorHandle_t handle, uint32_t *pAbsStep );
static RESULT OV8858_IsiMdiFocusCalibrate( IsiSensorHandle_t handle );

static RESULT OV8858_IsiGetSensorMipiInfoIss( IsiSensorHandle_t handle, IsiSensorMipiInfo *ptIsiSensorMipiInfo);
static RESULT OV8858_IsiGetSensorIsiVersion(  IsiSensorHandle_t   handle, unsigned int* pVersion);
static RESULT OV8858_IsiGetSensorTuningXmlVersion(  IsiSensorHandle_t   handle, char** pTuningXmlVersion);


static float dctfloor( const float f )
{
    if ( f < 0 )
    {
        return ( (float)((int32_t)f - 1L) );
    }
    else
    {
        return ( (float)((uint32_t)f) );
    }
}

/* OTP START*/
static int OV8858_R2A_read_i2c(    
    IsiSensorHandle_t   handle,
    const uint32_t      address
){
    uint32_t temp = 0;
    if(OV8858_IsiRegReadIss(handle,address,&temp) != RET_SUCCESS){
        TRACE( OV8858_ERROR, "%s read OTP register 0x%x erro!\n", __FUNCTION__,address);
    }
    return temp;
}

static RESULT OV8858_R2A_write_i2c(    
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
){
    RESULT result = RET_SUCCESS;
    if((result = OV8858_IsiRegWriteIss(handle,address,value)) != RET_SUCCESS){
        TRACE( OV8858_ERROR, "%s write OTP register (0x%x,0x%x) erro!\n", __FUNCTION__,address,value);
    }
    return result;
}

struct otp_struct_R1A {
    int flag; // bit[7]: info, bit[6]:wb, bit[5]:vcm, bit[4]:lenc
    int module_integrator_id;
    int lens_id;
    int production_year;
    int production_month;
    int production_day;
    int rg_ratio;
    int bg_ratio;
    int light_rg;
    int light_bg;
    int lenc[110];
    int VCM_start;
    int VCM_end;
    int VCM_dir;
};

struct otp_struct_R2A {
    int flag; // bit[7]: info, bit[6]:wb, bit[5]:vcm, bit[4]:lenc
    int module_integrator_id;
    int lens_id;
    int production_year;
    int production_month;
    int production_day;
    int rg_ratio;
    int bg_ratio;
    int lenc[240];
	int checksum;
    int VCM_start;
    int VCM_end;
    int VCM_dir;
};

static struct otp_struct_R1A g_otp_info_R1A ={0};
static struct otp_struct_R2A g_otp_info_R2A ={0};

#define OV8858_R1A	0xb0
#define OV8858_R2A	0xb2
static int g_sensor_version;

//for test,just for compile
int  RG_Ratio_Typical=0x100;
int  BG_Ratio_Typical=0x100;

#define  RG_Ratio_Typical_tongju (0x15c)
#define  BG_Ratio_Typical_tongju (0x13f)

#define  RG_Ratio_Typical_guangzhen (0x13c)
#define  BG_Ratio_Typical_guangzhen (0x128)

#define  RG_Ratio_Typical_R2A_bbk (0x144)
#define  BG_Ratio_Typical_R2A_bbk  (0x12f)

#define  RG_Ratio_Typical_R2A_LG_9569A2 (0x129)
#define  BG_Ratio_Typical_R2A_LG_9569A2 (0x138)

#if 0
static int check_read_otp(
    sensor_i2c_write_t*  sensor_i2c_write_p,
    sensor_i2c_read_t*  sensor_i2c_read_p,
    void* context,
    int camsys_fd
)
{
    struct otp_struct *otp_ptr = &g_otp_info;

    int otp_flag=0x0, addr, temp, i;
    //set 0x5002[3] to “0”
    int temp1;
    int i2c_base_info[3];

    i2c_base_info[0] = Sensor_OTP_SLAVE_ADDR; //otp i2c addr
    i2c_base_info[1] = 2; //otp i2c reg size
    i2c_base_info[2] = 1; //otp i2c value size
    //stream on 
    sensor_i2c_write_p(context,camsys_fd, OV8858_MODE_SELECT, OV8858_MODE_SELECT_ON, i2c_base_info );
    
    temp1 = sensor_i2c_read_p(context,camsys_fd,0x5002,i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x5002, (0x00 & 0x08) | (temp1 & (~0x08)), i2c_base_info);
    // read OTP into buffer
    sensor_i2c_write_p(context,camsys_fd,0x3d84, 0xC0, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d88, 0x70, i2c_base_info); // OTP start address
    sensor_i2c_write_p(context,camsys_fd,0x3d89, 0x10, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d8A, 0x71, i2c_base_info); // OTP end address
    sensor_i2c_write_p(context,camsys_fd,0x3d8B, 0x84, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d81, 0x01, i2c_base_info); // load otp into buffer
    osSleep(10);
    otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7010, i2c_base_info);

    addr = 0;
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x7011; // base address of info group 1
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x7016; // base address of info group 2
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x701b; // base address of info group 3
    }
    if(addr != 0) {
        (*otp_ptr).flag = 0x80; // valid info and AWB in OTP
        (*otp_ptr).module_integrator_id = sensor_i2c_read_p(context,camsys_fd,addr, i2c_base_info);
        (*otp_ptr).lens_id = sensor_i2c_read_p(context,camsys_fd, addr + 1, i2c_base_info);
        (*otp_ptr).production_year = sensor_i2c_read_p(context,camsys_fd, addr + 2, i2c_base_info);
        (*otp_ptr).production_month = sensor_i2c_read_p(context,camsys_fd, addr + 3, i2c_base_info);
        (*otp_ptr).production_day = sensor_i2c_read_p(context,camsys_fd,addr + 4, i2c_base_info);
    }
    else {
        (*otp_ptr).flag = 0x00; // not info and AWB in OTP
        (*otp_ptr).module_integrator_id = 0;
        (*otp_ptr).lens_id = 0;
        (*otp_ptr).production_year = 0;
        (*otp_ptr).production_month = 0;
        (*otp_ptr).production_day = 0;
    }

    // OTP base information and WB calibration data
    otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7020, i2c_base_info);
    addr = 0;
    // OTP AWB Calibration
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x7021; // base address of info group 1
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x7026; // base address of info group 2
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x702b; // base address of info group 3
    }

    if(addr != 0) {
        (*otp_ptr).flag |= 0x40; // valid info and AWB in OTP
        temp = sensor_i2c_read_p(context,camsys_fd,addr + 4, i2c_base_info);
        (*otp_ptr).rg_ratio = (sensor_i2c_read_p(context,camsys_fd,addr, i2c_base_info)<<2) + ((temp>>6) & 0x03);
        (*otp_ptr).bg_ratio = (sensor_i2c_read_p(context,camsys_fd,addr + 1, i2c_base_info)<<2) + ((temp>>4) & 0x03);
        (*otp_ptr).light_rg = (sensor_i2c_read_p(context,camsys_fd,addr + 2, i2c_base_info)<<2) + ((temp>>2) & 0x03);
        (*otp_ptr).light_bg = (sensor_i2c_read_p(context,camsys_fd,addr + 3, i2c_base_info)<<2) + ((temp) & 0x03);
        TRACE( OV8858_NOTICE0, "%s awb info in OTP(0x%x,0x%x,0x%x,0x%x)!\n", __FUNCTION__,(*otp_ptr).rg_ratio,(*otp_ptr).bg_ratio,
                                (*otp_ptr).light_rg,(*otp_ptr).light_bg);

    }
    else {
        (*otp_ptr).rg_ratio = 0;
        (*otp_ptr).bg_ratio = 0;
        (*otp_ptr).light_rg = 0;
        (*otp_ptr).light_bg = 0;
        TRACE( OV8858_ERROR, "%s no awb info in OTP!\n", __FUNCTION__);
    }
    
    // OTP VCM Calibration
    otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7030, i2c_base_info);
    addr = 0;
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x7031; // base address of VCM Calibration group 1
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x7034; // base address of VCM Calibration group 2
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x7037; // base address of VCM Calibration group 3
    }
    if(addr != 0) {
        (*otp_ptr).flag |= 0x20;
        temp = sensor_i2c_read_p(context,camsys_fd,addr + 2, i2c_base_info);
        (* otp_ptr).VCM_start = (sensor_i2c_read_p(context,camsys_fd,addr, i2c_base_info)<<2) | ((temp>>6) & 0x03);
        (* otp_ptr).VCM_end = (sensor_i2c_read_p(context,camsys_fd,addr + 1, i2c_base_info) << 2) | ((temp>>4) & 0x03);
        (* otp_ptr).VCM_dir = (temp>>2) & 0x03;
    }
    else {
        (* otp_ptr).VCM_start = 0;
        (* otp_ptr).VCM_end = 0;
        (* otp_ptr).VCM_dir = 0;
        TRACE( OV8858_INFO, "%s no VCM info in OTP!\n", __FUNCTION__);
    }
    // OTP Lenc Calibration
    otp_flag = sensor_i2c_read_p(context,camsys_fd,0x703a, i2c_base_info);
    addr = 0;
    int  checksum2=0;
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x703b; // base address of Lenc Calibration group 1
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x70a9; // base address of Lenc Calibration group 2
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x7117; // base address of Lenc Calibration group 3
    }
    if(addr != 0) {
        (*otp_ptr).flag |= 0x10;
        for(i=0;i<110;i++) {
            (* otp_ptr).lenc[i]=sensor_i2c_read_p(context,camsys_fd,addr + i, i2c_base_info);
            TRACE( OV8858_INFO, "%s lsc 0x%x!\n", __FUNCTION__,(*otp_ptr).lenc[i]);
        }
    }
    else {
        for(i=0;i<110;i++) {
        (* otp_ptr).lenc[i]=0;
        }
    }
    for(i=0x7010;i<=0x7184;i++) {
        sensor_i2c_write_p(context,camsys_fd,i,0, i2c_base_info); // clear OTP buffer, recommended use continuous write to accelarate
    }
    //set 0x5002[3] to “1”
    temp1 = sensor_i2c_read_p(context,camsys_fd,0x5002, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x5002, (0x08 & 0x08) | (temp1 & (~0x08)), i2c_base_info);

    //stream off 
    sensor_i2c_write_p(context,camsys_fd, OV8858_MODE_SELECT, OV8858_MODE_SELECT_OFF, i2c_base_info );
    if((*otp_ptr).flag != 0)
        return RET_SUCCESS;
    else
        return RET_NOTSUPP;
}
#endif

static int check_read_otp_R1A(
    sensor_i2c_write_t*  sensor_i2c_write_p,
    sensor_i2c_read_t*  sensor_i2c_read_p,
    void* context,
    int camsys_fd
)
{
    struct otp_struct_R1A *otp_ptr = &g_otp_info_R1A;
	int otp_flag=0x0, addr, temp, temp1, i;
    int i2c_base_info[3];

    i2c_base_info[0] = 0; //otp i2c addr
    i2c_base_info[1] = 2; //otp i2c reg size
    i2c_base_info[2] = 1; //otp i2c value size

#if 0
    int otp_flag=0x0, addr, temp, i;
    //set 0x5002[3] to “0”
    int temp1;
    int i2c_base_info[3];

    i2c_base_info[0] = Sensor_OTP_SLAVE_ADDR; //otp i2c addr
    i2c_base_info[1] = 2; //otp i2c reg size
    i2c_base_info[2] = 1; //otp i2c value size
    //stream on 
    sensor_i2c_write_p(context,camsys_fd, OV8858_MODE_SELECT, OV8858_MODE_SELECT_ON, i2c_base_info );
    
    temp1 = sensor_i2c_read_p(context,camsys_fd,0x5002,i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x5002, (0x00 & 0x08) | (temp1 & (~0x08)), i2c_base_info);
    // read OTP into buffer
    sensor_i2c_write_p(context,camsys_fd,0x3d84, 0xC0, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d88, 0x70, i2c_base_info); // OTP start address
    sensor_i2c_write_p(context,camsys_fd,0x3d89, 0x10, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d8A, 0x71, i2c_base_info); // OTP end address
    sensor_i2c_write_p(context,camsys_fd,0x3d8B, 0x84, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d81, 0x01, i2c_base_info); // load otp into buffer
    osSleep(10);
    otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7010, i2c_base_info);
#endif
	otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7010, i2c_base_info);
    addr = 0;
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x7011; // base address of info group 1
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x7016; // base address of info group 2
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x701b; // base address of info group 3
    }
    if(addr != 0) {
        (*otp_ptr).flag = 0x80; // valid info and AWB in OTP
        (*otp_ptr).module_integrator_id = sensor_i2c_read_p(context,camsys_fd,addr, i2c_base_info);
        (*otp_ptr).lens_id = sensor_i2c_read_p(context,camsys_fd, addr + 1, i2c_base_info);
        (*otp_ptr).production_year = sensor_i2c_read_p(context,camsys_fd, addr + 2, i2c_base_info);
        (*otp_ptr).production_month = sensor_i2c_read_p(context,camsys_fd, addr + 3, i2c_base_info);
        (*otp_ptr).production_day = sensor_i2c_read_p(context,camsys_fd,addr + 4, i2c_base_info);
        TRACE( OV8858_ERROR, "%s awb info in module_integrator_id(0x%x) lens_id(0x%x) production_year_month_day(%d_%d_%d) !\n", 
		__FUNCTION__,(*otp_ptr).module_integrator_id,(*otp_ptr).lens_id,(*otp_ptr).production_year,(*otp_ptr).production_month,(*otp_ptr).production_day);
        TRACE( OV8858_ERROR, "%s awb info in OTP(0x%x,0x%x)!\n", __FUNCTION__,(*otp_ptr).rg_ratio,(*otp_ptr).bg_ratio);
        if((*otp_ptr).module_integrator_id == 0x04 && (*otp_ptr).lens_id == 0x50)
        {
            RG_Ratio_Typical = RG_Ratio_Typical_guangzhen;
            BG_Ratio_Typical = BG_Ratio_Typical_guangzhen;
        }else{
            RG_Ratio_Typical = RG_Ratio_Typical_tongju;
            BG_Ratio_Typical = BG_Ratio_Typical_tongju;
        }
    }
    else {
        (*otp_ptr).flag = 0x00; // not info and AWB in OTP
        (*otp_ptr).module_integrator_id = 0;
        (*otp_ptr).lens_id = 0;
        (*otp_ptr).production_year = 0;
        (*otp_ptr).production_month = 0;
        (*otp_ptr).production_day = 0;
    }   

    // OTP base information and WB calibration data
    otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7020, i2c_base_info);
    addr = 0;
    // OTP AWB Calibration
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x7021; // base address of info group 1
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x7026; // base address of info group 2
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x702b; // base address of info group 3
    }

    if(addr != 0) {
        (*otp_ptr).flag |= 0x40; // valid info and AWB in OTP
        temp = sensor_i2c_read_p(context,camsys_fd,addr + 4, i2c_base_info);
        (*otp_ptr).rg_ratio = (sensor_i2c_read_p(context,camsys_fd,addr, i2c_base_info)<<2) + ((temp>>6) & 0x03);
        (*otp_ptr).bg_ratio = (sensor_i2c_read_p(context,camsys_fd,addr + 1, i2c_base_info)<<2) + ((temp>>4) & 0x03);
        (*otp_ptr).light_rg = (sensor_i2c_read_p(context,camsys_fd,addr + 2, i2c_base_info)<<2) + ((temp>>2) & 0x03);
        (*otp_ptr).light_bg = (sensor_i2c_read_p(context,camsys_fd,addr + 3, i2c_base_info)<<2) + ((temp) & 0x03);
        TRACE( OV8858_NOTICE0, "%s awb info in OTP(0x%x,0x%x,0x%x,0x%x)!\n", __FUNCTION__,(*otp_ptr).rg_ratio,(*otp_ptr).bg_ratio,
                                (*otp_ptr).light_rg,(*otp_ptr).light_bg);

    }
    else {
        (*otp_ptr).rg_ratio = 0;
        (*otp_ptr).bg_ratio = 0;
        (*otp_ptr).light_rg = 0;
        (*otp_ptr).light_bg = 0;
        TRACE( OV8858_ERROR, "%s no awb info in OTP!\n", __FUNCTION__);
    }
    
    // OTP VCM Calibration
    otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7030, i2c_base_info);
    addr = 0;
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x7031; // base address of VCM Calibration group 1
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x7034; // base address of VCM Calibration group 2
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x7037; // base address of VCM Calibration group 3
    }
    if(addr != 0) {
        (*otp_ptr).flag |= 0x20;
        temp = sensor_i2c_read_p(context,camsys_fd,addr + 2, i2c_base_info);
        (* otp_ptr).VCM_start = (sensor_i2c_read_p(context,camsys_fd,addr, i2c_base_info)<<2) | ((temp>>6) & 0x03);
        (* otp_ptr).VCM_end = (sensor_i2c_read_p(context,camsys_fd,addr + 1, i2c_base_info) << 2) | ((temp>>4) & 0x03);
        (* otp_ptr).VCM_dir = (temp>>2) & 0x03;
    }
    else {
        (* otp_ptr).VCM_start = 0;
        (* otp_ptr).VCM_end = 0;
        (* otp_ptr).VCM_dir = 0;
        TRACE( OV8858_INFO, "%s no VCM info in OTP!\n", __FUNCTION__);
    }
    // OTP Lenc Calibration
    otp_flag = sensor_i2c_read_p(context,camsys_fd,0x703a, i2c_base_info);
    addr = 0;
    int  checksum2=0;
    if((otp_flag & 0xc0) == 0x40) {
        addr = 0x703b; // base address of Lenc Calibration group 1
    }
    else if((otp_flag & 0x30) == 0x10) {
        addr = 0x70a9; // base address of Lenc Calibration group 2
    }
    else if((otp_flag & 0x0c) == 0x04) {
        addr = 0x7117; // base address of Lenc Calibration group 3
    }
    if(addr != 0) {
        (*otp_ptr).flag |= 0x10;
        for(i=0;i<110;i++) {
            (* otp_ptr).lenc[i]=sensor_i2c_read_p(context,camsys_fd,addr + i, i2c_base_info);
            TRACE( OV8858_INFO, "%s lsc 0x%x!\n", __FUNCTION__,(*otp_ptr).lenc[i]);
        }
    }
    else {
        for(i=0;i<110;i++) {
        (* otp_ptr).lenc[i]=0;
        }
    }
    for(i=0x7010;i<=0x7184;i++) {
        sensor_i2c_write_p(context,camsys_fd,i,0, i2c_base_info); // clear OTP buffer, recommended use continuous write to accelarate
    }
    //set 0x5002[3] to "1"
    temp1 = sensor_i2c_read_p(context,camsys_fd,0x5002, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x5002, (0x08 & 0x08) | (temp1 & (~0x08)), i2c_base_info);

    //stream off 
    sensor_i2c_write_p(context,camsys_fd, OV8858_MODE_SELECT, OV8858_MODE_SELECT_OFF, i2c_base_info );
    if((*otp_ptr).flag != 0)
        return RET_SUCCESS;
    else
        return RET_NOTSUPP;

}


static int check_read_otp_R2A(
    sensor_i2c_write_t*  sensor_i2c_write_p,
    sensor_i2c_read_t*  sensor_i2c_read_p,
    void* context,
    int camsys_fd
)
{
    struct otp_struct_R2A *otp_ptr = &g_otp_info_R2A;
	int otp_flag=0x0, addr, temp, temp1, i;
    int i2c_base_info[3];

    i2c_base_info[0] = 0; //otp i2c addr
    i2c_base_info[1] = 2; //otp i2c reg size
    i2c_base_info[2] = 1; //otp i2c value size

	// OTP base information and WB calibration data
	otp_flag = sensor_i2c_read_p(context,camsys_fd, 0x7010, i2c_base_info);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7011; // base address of info group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7019; // base address of info group 2
	}
	if(addr != 0) {
		(*otp_ptr).flag = 0xC0; // valid info and AWB in OTP
		(*otp_ptr).module_integrator_id = sensor_i2c_read_p(context,camsys_fd,addr, i2c_base_info);
		(*otp_ptr).lens_id = sensor_i2c_read_p(context,camsys_fd, addr + 1, i2c_base_info);
		(*otp_ptr).production_year = sensor_i2c_read_p(context,camsys_fd, addr + 2, i2c_base_info);
		(*otp_ptr).production_month = sensor_i2c_read_p(context,camsys_fd, addr + 3, i2c_base_info);
		(*otp_ptr).production_day = sensor_i2c_read_p(context,camsys_fd, addr + 4, i2c_base_info);
		temp = sensor_i2c_read_p(context,camsys_fd, addr + 7, i2c_base_info);
		(*otp_ptr).rg_ratio = (sensor_i2c_read_p(context,camsys_fd, addr + 5, i2c_base_info)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (sensor_i2c_read_p(context,camsys_fd, addr + 6, i2c_base_info)<<2) + ((temp>>4) & 0x03);
		TRACE( OV8858_ERROR, "%s awb info in module_integrator_id(0x%x) lens_id(0x%x) production_year_month_day(%d_%d_%d) !\n", 
		__FUNCTION__,(*otp_ptr).module_integrator_id,(*otp_ptr).lens_id,(*otp_ptr).production_year,(*otp_ptr).production_month,(*otp_ptr).production_day);
        TRACE( OV8858_ERROR, "%s awb info in OTP(0x%x,0x%x)!\n", __FUNCTION__,(*otp_ptr).rg_ratio,(*otp_ptr).bg_ratio);		

        #if 1
    	if((*otp_ptr).module_integrator_id == 0x07 && (*otp_ptr).lens_id == 0x00)
        {
            RG_Ratio_Typical = RG_Ratio_Typical_R2A_bbk;
            BG_Ratio_Typical = BG_Ratio_Typical_R2A_bbk;
        }else if((*otp_ptr).module_integrator_id == 0x0d && (*otp_ptr).lens_id == 0x12)
        {
            RG_Ratio_Typical = RG_Ratio_Typical_tongju;
            BG_Ratio_Typical = BG_Ratio_Typical_tongju;
        }
        #endif
	}
	else {
		(*otp_ptr).flag = 0x00; // not info and AWB in OTP
		(*otp_ptr).module_integrator_id = 0;
		(*otp_ptr).lens_id = 0;
		(*otp_ptr).production_year = 0;
		(*otp_ptr).production_month = 0;
		(*otp_ptr).production_day = 0;
		(*otp_ptr).rg_ratio = 0;
		(*otp_ptr).bg_ratio = 0;
		TRACE( OV8858_ERROR, "%s no awb info in OTP!\n", __FUNCTION__);
	}  

	// OTP VCM Calibration
	otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7021, i2c_base_info);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7022; // base address of VCM Calibration group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7025; // base address of VCM Calibration group 2
	}
	if(addr != 0) {
		(*otp_ptr).flag |= 0x20;
		temp = sensor_i2c_read_p(context,camsys_fd,addr + 2, i2c_base_info);
		(* otp_ptr).VCM_start = (sensor_i2c_read_p(context,camsys_fd,addr, i2c_base_info)<<2) | ((temp>>6) & 0x03);
		(* otp_ptr).VCM_end = (sensor_i2c_read_p(context,camsys_fd,addr + 1, i2c_base_info) << 2) | ((temp>>4) & 0x03);
		(* otp_ptr).VCM_dir = (temp>>2) & 0x03;
	}
	else {
		(* otp_ptr).VCM_start = 0;
		(* otp_ptr).VCM_end = 0;
		(* otp_ptr).VCM_dir = 0;
		TRACE( OV8858_INFO, "%s no VCM info in OTP!\n", __FUNCTION__);
	}
	// OTP Lenc Calibration
	otp_flag = sensor_i2c_read_p(context,camsys_fd,0x7028, i2c_base_info);
	addr = 0;
	int checksum2=0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7029; // base address of Lenc Calibration group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x711a; // base address of Lenc Calibration group 2
	}
	if(addr != 0) {
		for(i=0;i<240;i++) {
			(* otp_ptr).lenc[i]=sensor_i2c_read_p(context,camsys_fd,addr + i, i2c_base_info);
			checksum2 += (* otp_ptr).lenc[i]; 
			TRACE( OV8858_INFO, "%s lsc 0x%x!\n", __FUNCTION__,(*otp_ptr).lenc[i]);
		}
		checksum2 = (checksum2)%255 +1;
		(* otp_ptr).checksum = sensor_i2c_read_p(context,camsys_fd,addr + 240, i2c_base_info);
		if((* otp_ptr).checksum == checksum2){
			(*otp_ptr).flag |= 0x10;
		}
	}
	else {
		for(i=0;i<240;i++) {
			(* otp_ptr).lenc[i]=0;
		}
	}
	for(i=0x7010;i<=0x720a;i++) {
		sensor_i2c_write_p(context,camsys_fd,i,0, i2c_base_info); // clear OTP buffer, recommended use continuous write to accelarate
	}
	//set 0x5002[3] to "1"
	temp1 = sensor_i2c_read_p(context,camsys_fd,0x5002, i2c_base_info);
	sensor_i2c_write_p(context,camsys_fd,0x5002, (0x08 & 0x08) | (temp1 & (~0x08)), i2c_base_info);

	//stream off 
	sensor_i2c_write_p(context,camsys_fd, OV8858_MODE_SELECT, OV8858_MODE_SELECT_OFF, i2c_base_info);
	if((*otp_ptr).flag != 0)
		return RET_SUCCESS;
	else
		return RET_NOTSUPP;

}

// return value:
// bit[7]: 0 no otp info, 1 valid otp info
// bit[6]: 0 no otp wb, 1 valib otp wb
// bit[5]: 0 no otp vcm, 1 valid otp vcm
// bit[4]: 0 no otp lenc/invalid otp lenc, 1 valid otp lenc
static int check_read_otp(
	sensor_i2c_write_t*  sensor_i2c_write_p,
	sensor_i2c_read_t*	sensor_i2c_read_p,
	void* context,
	int camsys_fd
)
{
	int i2c_base_info[3];
	int temp1;
	int ret=0;
		
    i2c_base_info[0] = 0; //otp i2c addr
    i2c_base_info[1] = 2; //otp i2c reg size
    i2c_base_info[2] = 1; //otp i2c value size
    //stream on 
    ret = sensor_i2c_write_p(context,camsys_fd, OV8858_MODE_SELECT, OV8858_MODE_SELECT_ON, i2c_base_info );

    if(ret < 0){
		TRACE( OV8858_ERROR, "%s: Don't worry, we will try OTP slave addr2!\n", __FUNCTION__);
		i2c_base_info[0] = Sensor_OTP_SLAVE_ADDR2; //otp i2c addr
		ret = sensor_i2c_write_p(context,camsys_fd, OV8858_MODE_SELECT, OV8858_MODE_SELECT_ON, i2c_base_info );
		if(ret < 0)
			return RET_NOTSUPP;
		TRACE( OV8858_ERROR, "%s: OTP slave addr2 works!\n", __FUNCTION__);
	}

    temp1 = sensor_i2c_read_p(context,camsys_fd,0x5002,i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x5002, (0x00 & 0x08) | (temp1 & (~0x08)), i2c_base_info);
    // read OTP into buffer
    sensor_i2c_write_p(context,camsys_fd,0x3d84, 0xC0, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d88, 0x70, i2c_base_info); // OTP start address
    sensor_i2c_write_p(context,camsys_fd,0x3d89, 0x10, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d8A, 0x71, i2c_base_info); // OTP end address
    sensor_i2c_write_p(context,camsys_fd,0x3d8B, 0x84, i2c_base_info);
    sensor_i2c_write_p(context,camsys_fd,0x3d81, 0x01, i2c_base_info); // load otp into buffer
    osSleep(10);
    g_sensor_version = sensor_i2c_read_p(context,camsys_fd, 0x302a, i2c_base_info);
	
	if(g_sensor_version == 0xb1)//one type of R1A, this reg value is oxb1.
		g_sensor_version = OV8858_R1A;

	if(g_sensor_version == OV8858_R2A)
		return	check_read_otp_R2A(sensor_i2c_write_p, sensor_i2c_read_p, context, camsys_fd);
	else if(g_sensor_version == OV8858_R1A)
		return	check_read_otp_R1A(sensor_i2c_write_p, sensor_i2c_read_p, context, camsys_fd);
	else
		return RET_NOTSUPP;

}


// return value:
// bit[7]: 0 no otp info, 1 valid otp info
// bit[6]: 0 no otp wb, 1 valib otp wb
// bit[5]: 0 no otp vcm, 1 valid otp vcm
// bit[4]: 0 no otp lenc, 1 valid otp lenc
static int apply_otp_R1A(IsiSensorHandle_t   handle,struct otp_struct_R1A *otp_ptr)
{
    int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;
    // apply OTP WB Calibration
    if ((*otp_ptr).flag & 0x40) {
        if((*otp_ptr).light_rg == 0){
            // no light source infomation in OTP ,light factor = 1
            rg = (*otp_ptr).rg_ratio;
        }else{
            rg = (*otp_ptr).rg_ratio * ((*otp_ptr).light_rg+512)/1024;
        }
        
        if((*otp_ptr).light_bg == 0){
            // no light source infomation in OTP ,light factor = 1
            bg = (*otp_ptr).bg_ratio;
        }else{
            bg = (*otp_ptr).bg_ratio * ((*otp_ptr).light_bg+512)/1024;
        }
        //calculate G gain
        R_gain = (RG_Ratio_Typical*1000) / rg;
        B_gain = (BG_Ratio_Typical*1000) / bg;
        G_gain = 1000;
        if (R_gain < 1000 || B_gain < 1000)
        {
            if (R_gain < B_gain)
                 Base_gain = R_gain;
          else
                Base_gain = B_gain;
        }
        else
        {
         Base_gain = G_gain;
        }
        R_gain = 0x400 * R_gain / (Base_gain);
        B_gain = 0x400 * B_gain / (Base_gain);
        G_gain = 0x400 * G_gain / (Base_gain);
    // update sensor WB gain
        if (R_gain>0x400) {
            OV8858_R2A_write_i2c( handle,0x5032, R_gain>>8);
            OV8858_R2A_write_i2c( handle,0x5033, R_gain & 0x00ff);
        }
        if (G_gain>0x400) {
            OV8858_R2A_write_i2c( handle,0x5034, G_gain>>8);
            OV8858_R2A_write_i2c( handle,0x5035, G_gain & 0x00ff);
        }
        if (B_gain>0x400) {
            OV8858_R2A_write_i2c( handle,0x5036, B_gain>>8);
            OV8858_R2A_write_i2c( handle,0x5037, B_gain & 0x00ff);
        }
    }
    // apply OTP Lenc Calibration
    if ((*otp_ptr).flag & 0x10) {
        temp = OV8858_R2A_read_i2c( handle,0x5000);
        temp = 0x80 | temp;
        OV8858_R2A_write_i2c( handle,0x5000, temp);
        for(i=0;i<110;i++) {
            OV8858_R2A_write_i2c( handle,0x5800 + i, (*otp_ptr).lenc[i]);
        }
    }
    TRACE( OV8858_NOTICE0,  "%s: success!!!\n",  __FUNCTION__ );
    return (*otp_ptr).flag;
}

// return value:
// bit[7]: 0 no otp info, 1 valid otp info
// bit[6]: 0 no otp wb, 1 valib otp wb
// bit[5]: 0 no otp vcm, 1 valid otp vcm
// bit[4]: 0 no otp lenc, 1 valid otp lenc
static int apply_otp_R2A(IsiSensorHandle_t   handle,struct otp_struct_R2A *otp_ptr)
{
	int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;
	
	// apply OTP WB Calibration
	if ((*otp_ptr).flag & 0x40) {
		rg = (*otp_ptr).rg_ratio;
		bg = (*otp_ptr).bg_ratio;
		//calculate G gain
		R_gain = (RG_Ratio_Typical*1000) / rg;
		B_gain = (BG_Ratio_Typical*1000) / bg;
		G_gain = 1000;
		if (R_gain < 1000 || B_gain < 1000)
		{
			if (R_gain < B_gain)
			Base_gain = R_gain;
			else
			Base_gain = B_gain;
		}
		else
		{
			Base_gain = G_gain;
		}
		R_gain = 0x400 * R_gain / (Base_gain);
		B_gain = 0x400 * B_gain / (Base_gain);
		G_gain = 0x400 * G_gain / (Base_gain);
		// update sensor WB gain
		if (R_gain>0x400) {
			OV8858_R2A_write_i2c(handle, 0x5032, R_gain>>8);
			OV8858_R2A_write_i2c(handle, 0x5033, R_gain & 0x00ff);
		}
		if(G_gain>0x400) {
			OV8858_R2A_write_i2c(handle, 0x5034, G_gain>>8);
			OV8858_R2A_write_i2c(handle, 0x5035, G_gain & 0x00ff);
		}
		if(B_gain>0x400) {
			OV8858_R2A_write_i2c(handle, 0x5036, B_gain>>8);
			OV8858_R2A_write_i2c(handle, 0x5037, B_gain & 0x00ff);
		}
	}
	// apply OTP Lenc Calibration
	if ((*otp_ptr).flag & 0x10) {
		temp = OV8858_R2A_read_i2c(handle, 0x5000);
		temp = 0x80 | temp;
		OV8858_R2A_write_i2c(handle, 0x5000, temp);
		for(i=0;i<240;i++) {
			OV8858_R2A_write_i2c(handle, 0x5800 + i, (*otp_ptr).lenc[i]);
		}
	}
	TRACE( OV8858_NOTICE0,  "%s: success!!!\n",  __FUNCTION__ );
	return (*otp_ptr).flag;
}


/* OTP END*/


/*****************************************************************************/
/**
 *          OV8858_IsiCreateSensorIss
 *
 * @brief   This function creates a new OV8858 sensor instance handle.
 *
 * @param   pConfig     configuration structure to create the instance
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
static RESULT OV8858_IsiCreateSensorIss
(
    IsiSensorInstanceConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;
	int32_t current_distance;
    OV8858_Context_t *pOV8858Ctx;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pConfig == NULL) || (pConfig->pSensor ==NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    pOV8858Ctx = ( OV8858_Context_t * )malloc ( sizeof (OV8858_Context_t) );
    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR,  "%s: Can't allocate OV8858 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pOV8858Ctx, 0, sizeof( OV8858_Context_t ) );

    result = HalAddRef( pConfig->HalHandle );
    if ( result != RET_SUCCESS )
    {
        free ( pOV8858Ctx );
        return ( result );
    }
    
    pOV8858Ctx->IsiCtx.HalHandle              = pConfig->HalHandle;
    pOV8858Ctx->IsiCtx.HalDevID               = pConfig->HalDevID;
    pOV8858Ctx->IsiCtx.I2cBusNum              = pConfig->I2cBusNum;
    pOV8858Ctx->IsiCtx.SlaveAddress           = ( pConfig->SlaveAddr == 0 ) ? OV8858_SLAVE_ADDR : pConfig->SlaveAddr;
    pOV8858Ctx->IsiCtx.NrOfAddressBytes       = 2U;

    pOV8858Ctx->IsiCtx.I2cAfBusNum            = pConfig->I2cAfBusNum;
    pOV8858Ctx->IsiCtx.SlaveAfAddress         = ( pConfig->SlaveAfAddr == 0 ) ? OV8858_SLAVE_AF_ADDR : pConfig->SlaveAfAddr;
    pOV8858Ctx->IsiCtx.NrOfAfAddressBytes     = 0U;

    pOV8858Ctx->IsiCtx.pSensor                = pConfig->pSensor;

    pOV8858Ctx->Configured             = BOOL_FALSE;
    pOV8858Ctx->Streaming              = BOOL_FALSE;
    pOV8858Ctx->TestPattern            = BOOL_FALSE;
    pOV8858Ctx->isAfpsRun              = BOOL_FALSE;
    /* ddl@rock-chips.com: v0.3.0 */
    current_distance = pConfig->VcmRatedCurrent - pConfig->VcmStartCurrent;
    current_distance = current_distance*MAX_VCMDRV_REG/MAX_VCMDRV_CURRENT;    
    pOV8858Ctx->VcmInfo.Step = (current_distance+(MAX_LOG-1))/MAX_LOG;
    pOV8858Ctx->VcmInfo.StartCurrent   = pConfig->VcmStartCurrent*MAX_VCMDRV_REG/MAX_VCMDRV_CURRENT;    
    pOV8858Ctx->VcmInfo.RatedCurrent   = pOV8858Ctx->VcmInfo.StartCurrent + MAX_LOG*pOV8858Ctx->VcmInfo.Step;
    pOV8858Ctx->VcmInfo.StepMode       = pConfig->VcmStepMode;    
	
	pOV8858Ctx->IsiSensorMipiInfo.sensorHalDevID = pOV8858Ctx->IsiCtx.HalDevID;
	if(pConfig->mipiLaneNum & g_suppoted_mipi_lanenum_type)
        pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes = pConfig->mipiLaneNum;
    else{
        TRACE( OV8858_ERROR, "%s don't support lane numbers :%d,set to default %d\n", __FUNCTION__,pConfig->mipiLaneNum,DEFAULT_NUM_LANES);
        pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes = DEFAULT_NUM_LANES;
    }
	
    pConfig->hSensor = ( IsiSensorHandle_t )pOV8858Ctx;

    result = HalSetCamConfig( pOV8858Ctx->IsiCtx.HalHandle, pOV8858Ctx->IsiCtx.HalDevID, false, true, false ); //pwdn,reset active;hkw
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    result = HalSetClock( pOV8858Ctx->IsiCtx.HalHandle, pOV8858Ctx->IsiCtx.HalDevID, 24000000U);
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV8858_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiReleaseSensorIss
 *
 * @brief   This function destroys/releases an OV8858 sensor instance.
 *
 * @param   handle      OV8858 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
static RESULT OV8858_IsiReleaseSensorIss
(
    IsiSensorHandle_t handle
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    (void)OV8858_IsiSensorSetStreamingIss( pOV8858Ctx, BOOL_FALSE );
    (void)OV8858_IsiSensorSetPowerIss( pOV8858Ctx, BOOL_FALSE );

    (void)HalDelRef( pOV8858Ctx->IsiCtx.HalHandle );

    MEMSET( pOV8858Ctx, 0, sizeof( OV8858_Context_t ) );
    free ( pOV8858Ctx );

    TRACE( OV8858_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetCapsIss
 *
 * @brief   fills in the correct pointers for the sensor description struct
 *
 * @param   param1      pointer to sensor capabilities structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8858_IsiGetCapsIssInternal
(
    IsiSensorCaps_t   *pIsiSensorCaps,
    uint32_t  mipi_lanes
)
{
    RESULT result = RET_SUCCESS;
    
    if ( pIsiSensorCaps == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        if (mipi_lanes == SUPPORT_MIPI_FOUR_LANE) {            
            switch (pIsiSensorCaps->Index) 
            {                
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P30;
                    break;
                }
                case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P25;
                    break;
                }
                case 2:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P20;
                    break;
                }
                case 3:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P15;
                    break;
                }
                case 4:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P10;
                    break;
                }
                case 5:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P7;
                    break;
                }
                case 6:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P30;
                    break;
                }
                case 7:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P25;
                    break;
                }

                case 8:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P20;
                    break;
                }

                case 9:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P15;
                    break;
                }
                
                case 10:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P10;
                    break;
                }
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }

            }
        } else if(mipi_lanes == SUPPORT_MIPI_TWO_LANE) {
            switch (pIsiSensorCaps->Index) 
            {
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P15;
                    break;
                }
                case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P7;
                    break;
                }
                case 2:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P30;
                    break;
                }

                case 3:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P25;
                    break;
                }

                case 4:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P20;
                    break;
                }

                case 5:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P15;
                    break;
                }
                
                case 6:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P10;
                    break;
                }
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }

            }
        }  else if(mipi_lanes == SUPPORT_MIPI_ONE_LANE) {
            switch (pIsiSensorCaps->Index) 
            {
                case 0:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_3264_2448P7;
                    break;
                }
                case 1:
                {
                    pIsiSensorCaps->Resolution = ISI_RES_1632_1224P15;
                    break;
                }
                default:
                {
                    result = RET_OUTOFRANGE;
                    goto end;
                }

            }
        }              
    
        pIsiSensorCaps->BusWidth        = ISI_BUSWIDTH_10BIT; //
        pIsiSensorCaps->Mode            = ISI_MODE_MIPI;
        pIsiSensorCaps->FieldSelection  = ISI_FIELDSEL_BOTH;
        pIsiSensorCaps->YCSequence      = ISI_YCSEQ_YCBYCR;           /**< only Bayer supported, will not be evaluated */
        pIsiSensorCaps->Conv422         = ISI_CONV422_NOCOSITED;
        pIsiSensorCaps->BPat            = ISI_BPAT_BGBGGRGR;
        pIsiSensorCaps->HPol            = ISI_HPOL_REFPOS; //hsync?
        pIsiSensorCaps->VPol            = ISI_VPOL_NEG; //VPolarity
        pIsiSensorCaps->Edge            = ISI_EDGE_FALLING; //?
        pIsiSensorCaps->Bls             = ISI_BLS_OFF; //close;
        pIsiSensorCaps->Gamma           = ISI_GAMMA_OFF;//close;
        pIsiSensorCaps->CConv           = ISI_CCONV_OFF;//close;<
        pIsiSensorCaps->BLC             = ( ISI_BLC_AUTO | ISI_BLC_OFF);
        pIsiSensorCaps->AGC             = ( ISI_AGC_OFF );//close;
        pIsiSensorCaps->AWB             = ( ISI_AWB_OFF );
        pIsiSensorCaps->AEC             = ( ISI_AEC_OFF );
        pIsiSensorCaps->DPCC            = ( ISI_DPCC_AUTO | ISI_DPCC_OFF );//坏点

        pIsiSensorCaps->DwnSz           = ISI_DWNSZ_SUBSMPL; //;
        pIsiSensorCaps->CieProfile      = ( ISI_CIEPROF_A  //光源；
                                          | ISI_CIEPROF_D50
                                          | ISI_CIEPROF_D65
                                          | ISI_CIEPROF_D75
                                          | ISI_CIEPROF_F2
                                          | ISI_CIEPROF_F11 );
        pIsiSensorCaps->SmiaMode        = ISI_SMIA_OFF;
        pIsiSensorCaps->MipiMode        = ISI_MIPI_MODE_RAW_10; 
        pIsiSensorCaps->AfpsResolutions = ( ISI_AFPS_NOTSUPP ); //跳帧;没用
		pIsiSensorCaps->SensorOutputMode = ISI_SENSOR_OUTPUT_MODE_RAW;//
    }
end:
    return result;
}
 
static RESULT OV8858_IsiGetCapsIss
(
    IsiSensorHandle_t handle,
    IsiSensorCaps_t   *pIsiSensorCaps
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }
    
    result = OV8858_IsiGetCapsIssInternal(pIsiSensorCaps,pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes );
    
    TRACE( OV8858_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_g_IsiSensorDefaultConfig
 *
 * @brief   recommended default configuration for application use via call
 *          to IsiGetSensorIss()
 *
 *****************************************************************************/
const IsiSensorCaps_t OV8858_g_IsiSensorDefaultConfig =
{
    ISI_BUSWIDTH_10BIT,         // BusWidth
    ISI_MODE_MIPI,              // MIPI
    ISI_FIELDSEL_BOTH,          // FieldSel
    ISI_YCSEQ_YCBYCR,           // YCSeq
    ISI_CONV422_NOCOSITED,      // Conv422
    ISI_BPAT_BGBGGRGR,          // BPat
    ISI_HPOL_REFPOS,            // HPol
    ISI_VPOL_NEG,               // VPol
    ISI_EDGE_RISING,            // Edge
    ISI_BLS_OFF,                // Bls
    ISI_GAMMA_OFF,              // Gamma
    ISI_CCONV_OFF,              // CConv
    ISI_RES_3264_2448P15, 
    ISI_DWNSZ_SUBSMPL,          // DwnSz
    ISI_BLC_AUTO,               // BLC
    ISI_AGC_OFF,                // AGC
    ISI_AWB_OFF,                // AWB
    ISI_AEC_OFF,                // AEC
    ISI_DPCC_OFF,               // DPCC
    ISI_CIEPROF_F11,            // CieProfile, this is also used as start profile for AWB (if not altered by menu settings)
    ISI_SMIA_OFF,               // SmiaMode
    ISI_MIPI_MODE_RAW_10,       // MipiMode
    ISI_AFPS_NOTSUPP,           // AfpsResolutions
    ISI_SENSOR_OUTPUT_MODE_RAW,
    0,
};



/*****************************************************************************/
/**
 *          OV8858_SetupOutputFormat
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV8858 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 验证上面模式等；
 *****************************************************************************/
RESULT OV8858_SetupOutputFormat
(
    OV8858_Context_t       *pOV8858Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s%s (enter)\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );

    /* bus-width */
    switch ( pConfig->BusWidth )        /* only ISI_BUSWIDTH_12BIT supported, no configuration needed here */
    {
        case ISI_BUSWIDTH_10BIT:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: bus width not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* mode */
    switch ( pConfig->Mode )            /* only ISI_MODE_BAYER supported, no configuration needed here */
    {
        case( ISI_MODE_MIPI ):
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: mode not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* field-selection */
    switch ( pConfig->FieldSelection )  /* only ISI_FIELDSEL_BOTH supported, no configuration needed */
    {
        case ISI_FIELDSEL_BOTH:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: field selection not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* only Bayer mode is supported by OV8858 sensor, so the YCSequence parameter is not checked */
    switch ( pConfig->YCSequence )
    {
        default:
        {
            break;
        }
    }

    /* 422 conversion */
    switch ( pConfig->Conv422 )         /* only ISI_CONV422_NOCOSITED supported, no configuration needed */
    {
        case ISI_CONV422_NOCOSITED:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: 422 conversion not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* bayer-pattern */
    switch ( pConfig->BPat )            /* only ISI_BPAT_BGBGGRGR supported, no configuration needed */
    {
        case ISI_BPAT_BGBGGRGR:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: bayer pattern not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* horizontal polarity */
    switch ( pConfig->HPol )            /* only ISI_HPOL_REFPOS supported, no configuration needed */
    {
        case ISI_HPOL_REFPOS:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: HPol not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* vertical polarity */
    switch ( pConfig->VPol )            /* only ISI_VPOL_NEG supported, no configuration needed */
    {
        case ISI_VPOL_NEG:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: VPol not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }


    /* edge */
    switch ( pConfig->Edge )            /* only ISI_EDGE_RISING supported, no configuration needed */
    {
        case ISI_EDGE_RISING:
        {
            break;
        }

        case ISI_EDGE_FALLING:          /*TODO for MIPI debug*/
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s:  edge mode not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* gamma */
    switch ( pConfig->Gamma )           /* only ISI_GAMMA_OFF supported, no configuration needed */
    {
        case ISI_GAMMA_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s:  gamma not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    /* color conversion */
    switch ( pConfig->CConv )           /* only ISI_CCONV_OFF supported, no configuration needed */
    {
        case ISI_CCONV_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: color conversion not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->SmiaMode )        /* only ISI_SMIA_OFF supported, no configuration needed */
    {
        case ISI_SMIA_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: SMIA mode not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->MipiMode )        /* only ISI_MIPI_MODE_RAW_12 supported, no configuration needed */
    {
        case ISI_MIPI_MODE_RAW_10:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s%s: MIPI mode not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            return ( RET_NOTSUPP );
        }
    }

    switch ( pConfig->AfpsResolutions ) /* no configuration needed */
    {
        case ISI_AFPS_NOTSUPP:
        {
            break;
        }
        default:
        {
            // don't care about what comes in here
            //TRACE( OV8858_ERROR, "%s%s: AFPS not supported\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"" );
            //return ( RET_NOTSUPP );
        }
    }

    TRACE( OV8858_INFO, "%s%s (exit)\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}

//2400 :real clock/10000
int OV8858_get_PCLK( OV8858_Context_t *pOV8858Ctx, int XVCLK)
{
    // calculate PCLK
    uint32_t SCLK, temp1, temp2, temp3;
	int Pll2_prediv0, Pll2_prediv2x, Pll2_multiplier, Pll2_sys_pre_div, Pll2_sys_divider2x, Sys_pre_div, sclk_pdiv;
    int Pll2_prediv0_map[] = {1, 2};
    int Pll2_prediv2x_map[] = {2, 3, 4, 5, 6, 8, 12, 16};
    int Pll2_sys_divider2x_map[] = {2, 3, 4, 5, 6, 7, 8, 10};
    int Sys_pre_div_map[] = {1, 2, 4, 1};

    
    //temp1 = ReadSCCB(0x6c, 0x3007);
    OV8858_IsiRegReadIss(  pOV8858Ctx, 0x0312, &temp1 );
    temp2 = (temp1>>4) & 0x01;
    Pll2_prediv0 = Pll2_prediv0_map[temp2];

	OV8858_IsiRegReadIss(  pOV8858Ctx, 0x030b, &temp1 );
	temp2 = temp1 & 0x07;
	Pll2_prediv2x = Pll2_prediv2x_map[temp2];

	OV8858_IsiRegReadIss(  pOV8858Ctx, 0x030c, &temp1 );
	OV8858_IsiRegReadIss(  pOV8858Ctx, 0x030d, &temp3 );
	temp1 = temp1 & 0x03;
	temp2 = (temp1<<8) + temp3;
	if(!temp2) {
 		Pll2_multiplier = 1;
 	}
	else {
 	Pll2_multiplier = temp2;
	}
	
    OV8858_IsiRegReadIss(  pOV8858Ctx, 0x030f, &temp1 );
	temp1 = temp1 & 0x0f;
	Pll2_sys_pre_div = temp1 + 1;
	OV8858_IsiRegReadIss(  pOV8858Ctx, 0x030e, &temp1 );
	temp1 = temp1 & 0x07;
	Pll2_sys_divider2x = Pll2_sys_divider2x_map[temp1];

	OV8858_IsiRegReadIss(  pOV8858Ctx, 0x3106, &temp1 );
	temp2 = (temp1>>2) & 0x03;
	Sys_pre_div = Sys_pre_div_map[temp2];
    
    temp2 = (temp1>>4) & 0x0f;
	 if(!temp2) {
 		sclk_pdiv = 1;
 	}
	 else {
 		sclk_pdiv = temp2;
 	}
  	 SCLK = XVCLK * 4 / Pll2_prediv0 / Pll2_prediv2x * Pll2_multiplier /Pll2_sys_pre_div / Pll2_sys_divider2x / Sys_pre_div / sclk_pdiv;
	
	temp1 = SCLK>>8;
	OV8858_IsiRegWriteIss(pOV8858Ctx, 0x350b, temp1);
	temp1 = SCLK & 0xff;
	OV8858_IsiRegWriteIss(pOV8858Ctx, 0x350a, temp1);
	return SCLK*10000;
}

/*****************************************************************************/
/**
 *          OV8858_SetupOutputWindow
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV8858 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * hkw fix
 *****************************************************************************/

static RESULT OV8858_SetupOutputWindowInternal
(
    OV8858_Context_t        *pOV8858Ctx,
    const IsiSensorConfig_t *pConfig,
    bool_t set2Sensor,
    bool_t res_no_chg
)
{
    RESULT result     = RET_SUCCESS;
    uint16_t usFrameLengthLines = 0;
    uint16_t usLineLengthPck    = 0;
	uint16_t usTimeHts;
	uint16_t usTimeVts;
    float    rVtPixClkFreq      = 0.0f;
    int xclk = 2400;
    
	TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);
	
	if(pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_ONE_LANE){
	
		pOV8858Ctx->IsiSensorMipiInfo.ulMipiFreq = 720;
		switch ( pConfig->Resolution )
		{
			case ISI_RES_1632_1224P15:
			{				
				if (set2Sensor == BOOL_TRUE) {
				    TRACE( OV8858_NOTICE1, "%s(%d): Resolution 1632x1224\n", __FUNCTION__,__LINE__ );
    				result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224_onelane);
    				if ( result != RET_SUCCESS )
    				{
    					return ( result );
    				}
    		    }
				usTimeHts = 0x0f10; //hkw
				usTimeVts = 0x04dc;
				/* sleep a while, that sensor can take over new default values */
				osSleep( 10 );
				break;
				
			}
			
			case ISI_RES_3264_2448P7:
			{				
				if (set2Sensor == BOOL_TRUE) {
				    TRACE( OV8858_NOTICE1, "%s(%d): Resolution 3264x2448\n", __FUNCTION__,__LINE__ );
    				result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448_onelane);
    				if ( result != RET_SUCCESS )
    				{
    					return ( result );
    				}
                }
				usTimeHts = 0x0f28;
				usTimeVts = 0x09aa;
				/* sleep a while, that sensor can take over new default values */
				osSleep( 10 );
				break;
				
			}
	
			default:
			{
				TRACE( OV8858_ERROR, "%s: Resolution not supported\n", __FUNCTION__ );
				return ( RET_NOTSUPP );
			}
		}
	} else if(pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_TWO_LANE){

        pOV8858Ctx->IsiSensorMipiInfo.ulMipiFreq = 720;
    	switch ( pConfig->Resolution )
        {
            case ISI_RES_1632_1224P30:
            case ISI_RES_1632_1224P25:
            case ISI_RES_1632_1224P20:
            case ISI_RES_1632_1224P15:
            case ISI_RES_1632_1224P10:            
            {
                if (set2Sensor == BOOL_TRUE) {                    
                    if (res_no_chg == BOOL_FALSE) {
						if(g_sensor_version == OV8858_R2A)
                        	result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224_twolane_R2A);
						else
							result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224_twolane);
                    }
     
                    if (pConfig->Resolution == ISI_RES_1632_1224P30) {                        
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P30_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_1632_1224P25) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P25_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_1632_1224P20) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P20_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_1632_1224P15) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P15_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_1632_1224P10) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P10_twolane_fpschg);
                    }
        		}

    			if(g_sensor_version == OV8858_R2A)
    			    usTimeHts = 0x0f10; 
    			else
                    usTimeHts = 0x0788; 
                    
                if (pConfig->Resolution == ISI_RES_1632_1224P30) {
                    usTimeVts = 0x04dc;
                } else if (pConfig->Resolution == ISI_RES_1632_1224P25) {
                    usTimeVts = 0x5d4;
                } else if (pConfig->Resolution == ISI_RES_1632_1224P20) {
                    usTimeVts = 0x074a;
                } else if (pConfig->Resolution == ISI_RES_1632_1224P15) {
                    usTimeVts = 0x9b8;
                } else if (pConfig->Resolution == ISI_RES_1632_1224P10) {
                    usTimeVts = 0x0e94;
                }
                
    		    /* sleep a while, that sensor can take over new default values */
    		    osSleep( 10 );
    			break;
                
            }

            case ISI_RES_3264_2448P7:
            case ISI_RES_3264_2448P15:
            {
                if (set2Sensor == BOOL_TRUE) {
                    if (res_no_chg == BOOL_FALSE) {
						if(g_sensor_version == OV8858_R2A)
        			    	result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448_twolane_R2A);
						else
							result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448_twolane);
        		    }

                    if (pConfig->Resolution == ISI_RES_3264_2448P15) {                        
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448P15_twolane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_3264_2448P7) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448P7_twolane_fpschg);
                    }
        		    
        		}
        		
    			usTimeHts = 0x0794;                
                if (pConfig->Resolution == ISI_RES_3264_2448P15) {                        
                    usTimeVts = 0x09aa;
                } else if (pConfig->Resolution == ISI_RES_3264_2448P7) {
                    usTimeVts = 0x1354;
                }
    		    /* sleep a while, that sensor can take over new default values */
    		    osSleep( 10 );
    			break;
                
            }

            default:
            {
                TRACE( OV8858_ERROR, "%s: Resolution(0x%x) not supported\n", __FUNCTION__, pConfig->Resolution);
                return ( RET_NOTSUPP );
            }
    	}
    } else if(pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_FOUR_LANE) {
    	pOV8858Ctx->IsiSensorMipiInfo.ulMipiFreq = 720;

        switch ( pConfig->Resolution )
        {
            case ISI_RES_1632_1224P30:
            case ISI_RES_1632_1224P25:
            case ISI_RES_1632_1224P20:
            case ISI_RES_1632_1224P15:
            case ISI_RES_1632_1224P10:            
            {
                if (set2Sensor == BOOL_TRUE) {                    
                    if (res_no_chg == BOOL_FALSE) {
						if(g_sensor_version == OV8858_R2A)
	                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224_fourlane_R2A);
						else
							result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224_fourlane);
                    }
     
                    if (pConfig->Resolution == ISI_RES_1632_1224P30) {                        
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P30_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_1632_1224P25) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P25_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_1632_1224P20) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P20_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_1632_1224P15) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P15_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_1632_1224P10) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_1632x1224P10_fourlane_fpschg);
                    }
        		}
    			usTimeHts = 0x0788; 
                if (pConfig->Resolution == ISI_RES_1632_1224P30) {
                    usTimeVts = 0x04dc;
                } else if (pConfig->Resolution == ISI_RES_1632_1224P25) {
                    usTimeVts = 0x5d4;
                } else if (pConfig->Resolution == ISI_RES_1632_1224P20) {
                    usTimeVts = 0x074a;
                } else if (pConfig->Resolution == ISI_RES_1632_1224P15) {
                    usTimeVts = 0x9b8;
                } else if (pConfig->Resolution == ISI_RES_1632_1224P10) {
                    usTimeVts = 0x0e94;
                }
                
    		    /* sleep a while, that sensor can take over new default values */
    		    osSleep( 10 );
    			break;
                
            }

            case ISI_RES_3264_2448P7:
            case ISI_RES_3264_2448P10:
            case ISI_RES_3264_2448P15:
            case ISI_RES_3264_2448P20:
            case ISI_RES_3264_2448P25:
            case ISI_RES_3264_2448P30:
            {
                if (set2Sensor == BOOL_TRUE) {
                    if (res_no_chg == BOOL_FALSE) {
						if(g_sensor_version == OV8858_R2A)
        			    	result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448_fourlane_R2A);
						else
							result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448_fourlane);
        		    }

                    if (pConfig->Resolution == ISI_RES_3264_2448P30) {                        
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448P30_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_3264_2448P25) {                        
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448P25_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_3264_2448P20) {                        
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448P20_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_3264_2448P15) {                        
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448P15_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_3264_2448P10) {                        
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448P10_fourlane_fpschg);
                    } else if (pConfig->Resolution == ISI_RES_3264_2448P7) {
                        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_3264x2448P7_fourlane_fpschg);
                    }
        		    
        		}
        		
    			usTimeHts = 0x0794;                
                if (pConfig->Resolution == ISI_RES_3264_2448P30) {                        
                    usTimeVts = 0x09aa;
                } else if (pConfig->Resolution == ISI_RES_3264_2448P25) {                        
                    usTimeVts = 0x0b98;
                } else if (pConfig->Resolution == ISI_RES_3264_2448P20) {                        
                    usTimeVts = 0x0e7f;
                } else if (pConfig->Resolution == ISI_RES_3264_2448P15) {                        
                    usTimeVts = 0x1354;
                } else if (pConfig->Resolution == ISI_RES_3264_2448P10) {                        
                    usTimeVts = 0x1cfe;
                } else if (pConfig->Resolution == ISI_RES_3264_2448P7) {
                    usTimeVts = 0x26a8;
                }
    		    /* sleep a while, that sensor can take over new default values */
    		    osSleep( 10 );
    			break;
                
            }
        }        

    }
    
	
/* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */
    
	usLineLengthPck = usTimeHts;
    usFrameLengthLines = usTimeVts;
	rVtPixClkFreq = OV8858_get_PCLK(pOV8858Ctx, xclk);
    
    // store frame timing for later use in AEC module
    pOV8858Ctx->VtPixClkFreq     = rVtPixClkFreq;
    pOV8858Ctx->LineLengthPck    = usLineLengthPck;
    pOV8858Ctx->FrameLengthLines = usFrameLengthLines;

    TRACE( OV8858_INFO, "%s  (exit): Resolution %dx%d@%dfps  MIPI %dlanes  res_no_chg: %d   rVtPixClkFreq: %f\n", __FUNCTION__,
                        ISI_RES_W_GET(pConfig->Resolution),ISI_RES_H_GET(pConfig->Resolution),
                        ISI_FPS_GET(pConfig->Resolution),
                        pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes,
                        res_no_chg,rVtPixClkFreq);
    
    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_SetupImageControl
 *
 * @brief   Sets the image control functions (BLC, AGC, AWB, AEC, DPCC ...)
 *
 * @param   handle      OV8858 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @don't fix hkw
 *****************************************************************************/
RESULT OV8858_SetupImageControl
(
    OV8858_Context_t        *pOV8858Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0U;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    switch ( pConfig->Bls )      /* only ISI_BLS_OFF supported, no configuration needed */
    {
        case ISI_BLS_OFF:
        {
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s: Black level not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* black level compensation */
    switch ( pConfig->BLC )
    {
        case ISI_BLC_OFF:
        {
            /* turn off black level correction (clear bit 0) */
            //result = OV8858_IsiRegReadIss(  pOV8858Ctx, OV8858_BLC_CTRL00, &RegValue );
            //result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_BLC_CTRL00, RegValue & 0x7F);
            break;
        }

        case ISI_BLC_AUTO:
        {
            /* turn on black level correction (set bit 0)
             * (0x331E[7] is assumed to be already setup to 'auto' by static configration) */
            //result = OV8858_IsiRegReadIss(  pOV8858Ctx, OV8858_BLC_CTRL00, &RegValue );
            //result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_BLC_CTRL00, RegValue | 0x80 );
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s: BLC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* automatic gain control */
    switch ( pConfig->AGC )
    {
        case ISI_AGC_OFF:
        {
            // manual gain (appropriate for AEC with Marvin)
            //result = OV8858_IsiRegReadIss(  pOV8858Ctx, OV8858_AEC_MANUAL, &RegValue );
            //result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_AEC_MANUAL, RegValue | 0x02 );
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s: AGC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    /* automatic white balance */
    switch( pConfig->AWB )
    {
        case ISI_AWB_OFF:
        {
            //result = OV8858_IsiRegReadIss(  pOV8858Ctx, OV8858_ISP_CTRL01, &RegValue );
            //result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_ISP_CTRL01, RegValue | 0x01 );
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s: AWB not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }

    switch( pConfig->AEC )
    {
        case ISI_AEC_OFF:
        {
            //result = OV8858_IsiRegReadIss(  pOV8858Ctx, OV8858_AEC_MANUAL, &RegValue );
            //result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_AEC_MANUAL, RegValue | 0x01 );
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s: AEC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }


    switch( pConfig->DPCC )
    {
        case ISI_DPCC_OFF:
        {
            // disable white and black pixel cancellation (clear bit 6 and 7)
            //result = OV8858_IsiRegReadIss( pOV8858Ctx, OV8858_ISP_CTRL00, &RegValue );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            //result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_ISP_CTRL00, (RegValue &0x7c) );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            break;
        }

        case ISI_DPCC_AUTO:
        {
            // enable white and black pixel cancellation (set bit 6 and 7)
            //result = OV8858_IsiRegReadIss( pOV8858Ctx, OV8858_ISP_CTRL00, &RegValue );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            //result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_ISP_CTRL00, (RegValue | 0x83) );
            //RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
            break;
        }

        default:
        {
            TRACE( OV8858_ERROR, "%s: DPCC not supported\n", __FUNCTION__ );
            return ( RET_NOTSUPP );
        }
    }// I have not update this commented part yet, as I did not find DPCC setting in the current 8810 driver of Trillian board. - SRJ

    return ( result );
}
static RESULT OV8858_SetupOutputWindow
(
    OV8858_Context_t        *pOV8858Ctx,
    const IsiSensorConfig_t *pConfig    
)
{
    bool_t res_no_chg;

    if ((ISI_RES_W_GET(pConfig->Resolution)==ISI_RES_W_GET(pOV8858Ctx->Config.Resolution)) && 
        (ISI_RES_W_GET(pConfig->Resolution)==ISI_RES_W_GET(pOV8858Ctx->Config.Resolution))) {
        res_no_chg = BOOL_TRUE;
        
    } else {
        res_no_chg = BOOL_FALSE;
    }

    return OV8858_SetupOutputWindowInternal(pOV8858Ctx,pConfig,BOOL_TRUE, BOOL_FALSE);
}

/*****************************************************************************/
/**
 *          OV8858_AecSetModeParameters
 *
 * @brief   This function fills in the correct parameters in OV8858-Instances
 *          according to AEC mode selection in IsiSensorConfig_t.
 *
 * @note    It is assumed that IsiSetupOutputWindow has been called before
 *          to fill in correct values in instance structure.
 *
 * @param   handle      OV8858 context
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 不用改
 *****************************************************************************/
static RESULT OV8858_AecSetModeParameters
(
    OV8858_Context_t       *pOV8858Ctx,
    const IsiSensorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s%s (enter)  Res: 0x%x  0x%x\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"",
        pOV8858Ctx->Config.Resolution, pConfig->Resolution);

    if ( (pOV8858Ctx->VtPixClkFreq == 0.0f) )
    {
        TRACE( OV8858_ERROR, "%s%s: Division by zero!\n", __FUNCTION__  );
        return ( RET_OUTOFRANGE );
    }

    //as of mail from Omnivision FAE the limit is VTS - 6 (above that we observed a frame
    //exposed way too dark from time to time)
    // (formula is usually MaxIntTime = (CoarseMax * LineLength + FineMax) / Clk
    //                     MinIntTime = (CoarseMin * LineLength + FineMin) / Clk )
    pOV8858Ctx->AecMaxIntegrationTime = ( ((float)(pOV8858Ctx->FrameLengthLines - 4)) * ((float)pOV8858Ctx->LineLengthPck) ) / pOV8858Ctx->VtPixClkFreq;
    pOV8858Ctx->AecMinIntegrationTime = 0.0001f;

    TRACE( OV8858_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"", pOV8858Ctx->AecMaxIntegrationTime  );

    pOV8858Ctx->AecMaxGain = OV8858_MAX_GAIN_AEC;
    pOV8858Ctx->AecMinGain = 1.0f; //as of sensor datasheet 32/(32-6)

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    pOV8858Ctx->AecIntegrationTimeIncrement = ((float)pOV8858Ctx->LineLengthPck) / pOV8858Ctx->VtPixClkFreq;
    pOV8858Ctx->AecGainIncrement = OV8858_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOV8858Ctx->AecCurGain               = pOV8858Ctx->AecMinGain;
    pOV8858Ctx->AecCurIntegrationTime    = 0.0f;
    pOV8858Ctx->OldCoarseIntegrationTime = 0;
    pOV8858Ctx->OldFineIntegrationTime   = 0;
    //pOV8858Ctx->GroupHold                = true; //must be true (for unknown reason) to correctly set gain the first time

    TRACE( OV8858_INFO, "%s%s (exit)\n", __FUNCTION__, pOV8858Ctx->isAfpsRun?"(AFPS)":"");

    return ( result );
}

/*****************************************************************************/
/**
 *          OV8858_IsiSetupSensorIss
 *
 * @brief   Setup of the image sensor considering the given configuration.
 *
 * @param   handle      OV8858 sensor instance handle
 * @param   pConfig     pointer to sensor configuration structure
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8858_IsiSetupSensorIss
(
    IsiSensorHandle_t       handle,
    const IsiSensorConfig_t *pConfig
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid configuration (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    if ( pOV8858Ctx->Streaming != BOOL_FALSE )
    {
        return RET_WRONG_STATE;
    }

    MEMCPY( &pOV8858Ctx->Config, pConfig, sizeof( IsiSensorConfig_t ) );

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    result = OV8858_IsiRegWriteIss ( pOV8858Ctx, OV8858_SOFTWARE_RST, OV8858_SOFTWARE_RST_VALUE );//宏定义 hkw；
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    osSleep( 10 );

    TRACE( OV8858_DEBUG, "%s: OV8858 System-Reset executed\n", __FUNCTION__);
    // disable streaming during sensor setup
    // (this seems not to be necessary, however Omnivision is doing it in their
    // reference settings, simply overwrite upper bits since setup takes care
    // of 'em later on anyway)
    result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_MODE_SELECT, OV8858_MODE_SELECT_OFF );//OV8858_MODE_SELECT,stream off; hkw
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: Can't write OV8858 Image System Register (disable streaming failed)\n", __FUNCTION__ );
        return ( result );
    }
    
    /* 2.) write default values derived from datasheet and evaluation kit (static setup altered by dynamic setup further below) */
    //result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_aRegDescription );
    if(g_sensor_version == OV8858_R2A){
	    if(pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_FOUR_LANE){
	        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_aRegDescription_fourlane_R2A);
        }
		else if(pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_TWO_LANE)
	        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_aRegDescription_twolane_R2A);
	}else if(g_sensor_version == OV8858_R1A){
	    if(pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_FOUR_LANE){
	        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_aRegDescription_fourlane);
        }
		else if(pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_TWO_LANE)
	        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_aRegDescription_twolane);
		else if(pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes == SUPPORT_MIPI_ONE_LANE)
	        result = IsiRegDefaultsApply( pOV8858Ctx, OV8858_g_aRegDescription_onelane);
	}
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* sleep a while, that sensor can take over new default values */
    osSleep( 10 );


    /* 3.) verify default values to make sure everything has been written correctly as expected */
	#if 0
	result = IsiRegDefaultsVerify( pOV8858Ctx, OV8858_g_aRegDescription );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
	#endif
	
    #if 0
    // output of pclk for measurement (only debugging)
    result = OV8858_IsiRegWriteIss( pOV8858Ctx, 0x3009U, 0x10U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    #endif

    /* 4.) setup output format (RAW10|RAW12) */
    result = OV8858_SetupOutputFormat( pOV8858Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: SetupOutputFormat failed.\n", __FUNCTION__);
        return ( result );
    }

    /* 5.) setup output window */
    result = OV8858_SetupOutputWindow( pOV8858Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV8858_SetupImageControl( pOV8858Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: SetupImageControl failed.\n", __FUNCTION__);
        return ( result );
    }

    result = OV8858_AecSetModeParameters( pOV8858Ctx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
        return ( result );
    }
    if (result == RET_SUCCESS)
    {
        pOV8858Ctx->Configured = BOOL_TRUE;
    }

    //set OTP info

    result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_MODE_SELECT, OV8858_MODE_SELECT_ON );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: Can't write OV8858 Image System Register (disable streaming failed)\n", __FUNCTION__ );
        return ( result );
    }

    //set OTP
    {
        //struct otp_struct  otp_ptr;
        //read_otp(handle,&otp_ptr);
        if(g_sensor_version == OV8858_R1A){
	        if(g_otp_info_R1A.flag != 0){
	            TRACE( OV8858_NOTICE0, "%s: apply OTP info !!\n", __FUNCTION__);
				apply_otp_R1A(handle,&g_otp_info_R1A);
	        }
    	}else if(g_sensor_version == OV8858_R2A){
	        if(g_otp_info_R2A.flag != 0){
	            TRACE( OV8858_NOTICE0, "%s: apply OTP info !!\n", __FUNCTION__);
	            apply_otp_R2A(handle,&g_otp_info_R2A);
	        }
		}
    }
    
    result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_MODE_SELECT, OV8858_MODE_SELECT_OFF );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: Can't write OV8858 Image System Register (disable streaming failed)\n", __FUNCTION__ );
        return ( result );
    }
   

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiChangeSensorResolutionIss
 *
 * @brief   Change image sensor resolution while keeping all other static settings.
 *          Dynamic settings like current gain & integration time are kept as
 *          close as possible. Sensor needs 2 frames to engage (first 2 frames
 *          are not correctly exposed!).
 *
 * @note    Re-read current & min/max values as they will probably have changed!
 *
 * @param   handle                  Sensor instance handle
 * @param   Resolution              new resolution ID
 * @param   pNumberOfFramesToSkip   reference to storage for number of frames to skip
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 * @retval  RET_OUTOFRANGE
 * 不用改
 *****************************************************************************/
static RESULT OV8858_IsiChangeSensorResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s (enter)  Resolution: %dx%d@%dfps\n", __FUNCTION__,
        ISI_RES_W_GET(Resolution),ISI_RES_H_GET(Resolution), ISI_FPS_GET(Resolution));

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (pNumberOfFramesToSkip == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pOV8858Ctx->Configured != BOOL_TRUE) )
    {
        return RET_WRONG_STATE;
    }

    IsiSensorCaps_t Caps;
    
    Caps.Index = 0;
    Caps.Resolution = 0;
    while (OV8858_IsiGetCapsIss( handle, &Caps) == RET_SUCCESS) {
        if (Resolution == Caps.Resolution) {            
            break;
        }
        Caps.Index++;
    }

    if (Resolution != Caps.Resolution) {
        return RET_OUTOFRANGE;
    }

    if ( Resolution == pOV8858Ctx->Config.Resolution )
    {
        // well, no need to worry
        *pNumberOfFramesToSkip = 0;
    }
    else
    {
        // change resolution
        char *szResName = NULL;
        bool_t res_no_chg;

        if (!((ISI_RES_W_GET(Resolution)==ISI_RES_W_GET(pOV8858Ctx->Config.Resolution)) && 
            (ISI_RES_W_GET(Resolution)==ISI_RES_W_GET(pOV8858Ctx->Config.Resolution))) ) {

            if (pOV8858Ctx->Streaming != BOOL_FALSE) {
                TRACE( OV8858_ERROR, "%s: Sensor is streaming, Change resolution is not allow\n",__FUNCTION__);
                return RET_WRONG_STATE;
            }
            res_no_chg = BOOL_FALSE;
        } else {
            res_no_chg = BOOL_TRUE;
        }
        
        result = IsiGetResolutionName( Resolution, &szResName );
        TRACE( OV8858_DEBUG, "%s: NewRes=0x%08x (%s)\n", __FUNCTION__, Resolution, szResName);

        // update resolution in copy of config in context        
        pOV8858Ctx->Config.Resolution = Resolution;

        // tell sensor about that
        result = OV8858_SetupOutputWindowInternal( pOV8858Ctx, &pOV8858Ctx->Config, BOOL_TRUE, res_no_chg);
        if ( result != RET_SUCCESS )
        {
            TRACE( OV8858_ERROR, "%s: SetupOutputWindow failed.\n", __FUNCTION__);
            return ( result );
        }

        // remember old exposure values
        float OldGain = pOV8858Ctx->AecCurGain;
        float OldIntegrationTime = pOV8858Ctx->AecCurIntegrationTime;

        // update limits & stuff (reset current & old settings)
        result = OV8858_AecSetModeParameters( pOV8858Ctx, &pOV8858Ctx->Config );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV8858_ERROR, "%s: AecSetModeParameters failed.\n", __FUNCTION__);
            return ( result );
        }

        // restore old exposure values (at least within new exposure values' limits)
        uint8_t NumberOfFramesToSkip;
        float   DummySetGain;
        float   DummySetIntegrationTime;
        result = OV8858_IsiExposureControlIss( handle, OldGain, OldIntegrationTime, &NumberOfFramesToSkip, &DummySetGain, &DummySetIntegrationTime );
        if ( result != RET_SUCCESS )
        {
            TRACE( OV8858_ERROR, "%s: OV8858_IsiExposureControlIss failed.\n", __FUNCTION__);
            return ( result );
        }

        // return number of frames that aren't exposed correctly
        if (res_no_chg == BOOL_TRUE)
            *pNumberOfFramesToSkip = 0;
        else 
            *pNumberOfFramesToSkip = NumberOfFramesToSkip + 1;
        
    }

    TRACE( OV8858_INFO, "%s (exit)  result: 0x%x   pNumberOfFramesToSkip: %d \n", __FUNCTION__, result,
        *pNumberOfFramesToSkip);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV8858_IsiSensorSetStreamingIss
 *
 * @brief   Enables/disables streaming of sensor data, if possible.
 *
 * @param   handle      Sensor instance handle
 * @param   on          new streaming state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
static RESULT OV8858_IsiSensorSetStreamingIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    uint32_t RegValue = 0;

    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s (enter)  on = %d\n", __FUNCTION__,on);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pOV8858Ctx->Configured != BOOL_TRUE) || (pOV8858Ctx->Streaming == on) )
    {
        return RET_WRONG_STATE;
    }

    if (on == BOOL_TRUE)
    {
        /* enable streaming */
        result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_MODE_SELECT, &RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8858_IsiRegWriteIss ( pOV8858Ctx, OV8858_MODE_SELECT, (RegValue | OV8858_MODE_SELECT_ON) );//OV8858_MODE_SELECT,stream on; hkw
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }
    else
    {
        /* disable streaming */
        result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_MODE_SELECT, &RegValue);
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8858_IsiRegWriteIss ( pOV8858Ctx, OV8858_MODE_SELECT, (RegValue & ~OV8858_MODE_SELECT_ON) );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }

    if (result == RET_SUCCESS)
    {
        pOV8858Ctx->Streaming = on;
    }

    TRACE( OV8858_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiSensorSetPowerIss
 *
 * @brief   Performs the power-up/power-down sequence of the camera, if possible.
 *
 * @param   handle      OV8858 sensor instance handle
 * @param   on          new power state (BOOL_TRUE=on, BOOL_FALSE=off)
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 不用改
 *****************************************************************************/
static RESULT OV8858_IsiSensorSetPowerIss
(
    IsiSensorHandle_t   handle,
    bool_t              on
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pOV8858Ctx->Configured = BOOL_FALSE;
    pOV8858Ctx->Streaming  = BOOL_FALSE;

    TRACE( OV8858_DEBUG, "%s power off \n", __FUNCTION__);
    result = HalSetPower( pOV8858Ctx->IsiCtx.HalHandle, pOV8858Ctx->IsiCtx.HalDevID, false );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    TRACE( OV8858_DEBUG, "%s reset on\n", __FUNCTION__);
    result = HalSetReset( pOV8858Ctx->IsiCtx.HalHandle, pOV8858Ctx->IsiCtx.HalDevID, true );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    if (on == BOOL_TRUE)
    { //power on seq; hkw
        TRACE( OV8858_DEBUG, "%s power on \n", __FUNCTION__);
        result = HalSetPower( pOV8858Ctx->IsiCtx.HalHandle, pOV8858Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV8858_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pOV8858Ctx->IsiCtx.HalHandle, pOV8858Ctx->IsiCtx.HalDevID, false );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV8858_DEBUG, "%s reset on \n", __FUNCTION__);
        result = HalSetReset( pOV8858Ctx->IsiCtx.HalHandle, pOV8858Ctx->IsiCtx.HalDevID, true );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        osSleep( 10 );

        TRACE( OV8858_DEBUG, "%s reset off \n", __FUNCTION__);
        result = HalSetReset( pOV8858Ctx->IsiCtx.HalHandle, pOV8858Ctx->IsiCtx.HalDevID, false );

        osSleep( 50 );
    }

    TRACE( OV8858_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiCheckSensorConnectionIss
 *
 * @brief   Checks the I2C-Connection to sensor by reading sensor revision id.
 *
 * @param   handle      OV8858 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 读pid;2或3个寄存器；
 *****************************************************************************/
static RESULT OV8858_IsiCheckSensorConnectionIss
(
    IsiSensorHandle_t   handle
)
{
    uint32_t RevId;
    uint32_t value;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    RevId = OV8858_CHIP_ID_HIGH_BYTE_DEFAULT;
    RevId = (RevId << 16U) | (OV8858_CHIP_ID_MIDDLE_BYTE_DEFAULT<<8U);
    RevId = RevId | OV8858_CHIP_ID_LOW_BYTE_DEFAULT;

    result = OV8858_IsiGetSensorRevisionIss( handle, &value );
    if ( (result != RET_SUCCESS) || (RevId != value) )
    {
        TRACE( OV8858_ERROR, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );
        return ( RET_FAILURE );
    }

    TRACE( OV8858_DEBUG, "%s RevId = 0x%08x, value = 0x%08x \n", __FUNCTION__, RevId, value );

    TRACE( OV8858_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetSensorRevisionIss
 *
 * @brief   reads the sensor revision register and returns this value
 *
 * @param   handle      pointer to sensor description struct
 * @param   p_value     pointer to storage value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
static RESULT OV8858_IsiGetSensorRevisionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

    uint32_t data;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *p_value = 0U;
    result = OV8858_IsiRegReadIss ( handle, OV8858_CHIP_ID_HIGH_BYTE, &data );
    *p_value = ( (data & 0xFF) << 16U );
    result = OV8858_IsiRegReadIss ( handle, OV8858_CHIP_ID_MIDDLE_BYTE, &data );
    *p_value |= ( (data & 0xFF) << 8U );
    result = OV8858_IsiRegReadIss ( handle, OV8858_CHIP_ID_LOW_BYTE, &data );
    *p_value |= ( (data & 0xFF));

    TRACE( OV8858_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiRegReadIss
 *
 * @brief   grants user read access to the camera register
 *
 * @param   handle      pointer to sensor description struct
 * @param   address     sensor register to write
 * @param   p_value     pointer to value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改
 *****************************************************************************/
static RESULT OV8858_IsiRegReadIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    uint32_t            *p_value
)
{
    RESULT result = RET_SUCCESS;

  //  TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( p_value == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        uint8_t NrOfBytes = IsiGetNrDatBytesIss( address, OV8858_g_aRegDescription_twolane );
        if ( !NrOfBytes )
        {
            NrOfBytes = 1;
        }

        *p_value = 0;

        IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;        
        result = IsiI2cReadSensorRegister( handle, address, (uint8_t *)p_value, NrOfBytes, BOOL_TRUE );
    }

  //  TRACE( OV8858_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, *p_value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiRegWriteIss
 *
 * @brief   grants user write access to the camera register
 *
 * @param   handle      pointer to sensor description struct
 * @param   address     sensor register to write
 * @param   value       value to write
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * 不用改
 *****************************************************************************/
static RESULT OV8858_IsiRegWriteIss
(
    IsiSensorHandle_t   handle,
    const uint32_t      address,
    const uint32_t      value
)
{
    RESULT result = RET_SUCCESS;

    uint8_t NrOfBytes;

  //  TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    NrOfBytes = IsiGetNrDatBytesIss( address, OV8858_g_aRegDescription_twolane );
    if ( !NrOfBytes )
    {
        NrOfBytes = 1;
    }

    result = IsiI2cWriteSensorRegister( handle, address, (uint8_t *)(&value), NrOfBytes, BOOL_TRUE );

//    TRACE( OV8858_INFO, "%s (exit: 0x%08x 0x%08x)\n", __FUNCTION__, address, value);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetGainLimitsIss
 *
 * @brief   Returns the exposure minimal and maximal values of an
 *          OV8858 instance
 *
 * @param   handle       OV8858 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 不用改；获得增益限制
 *****************************************************************************/
static RESULT OV8858_IsiGetGainLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinGain,
    float               *pMaxGain
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinGain == NULL) || (pMaxGain == NULL) )
    {
        TRACE( OV8858_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinGain = pOV8858Ctx->AecMinGain;
    *pMaxGain = pOV8858Ctx->AecMaxGain;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetIntegrationTimeLimitsIss
 *
 * @brief   Returns the minimal and maximal integration time values of an
 *          OV8858 instance
 *
 * @param   handle       OV8858 sensor instance handle
 * @param   pMinExposure Pointer to a variable receiving minimal exposure value
 * @param   pMaxExposure Pointer to a variable receiving maximal exposure value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 不用改；获得曝光限制；
 *****************************************************************************/
static RESULT OV8858_IsiGetIntegrationTimeLimitsIss
(
    IsiSensorHandle_t   handle,
    float               *pMinIntegrationTime,
    float               *pMaxIntegrationTime
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL) )
    {
        TRACE( OV8858_ERROR, "%s: NULL pointer received!!\n" );
        return ( RET_NULL_POINTER );
    }

    *pMinIntegrationTime = pOV8858Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pOV8858Ctx->AecMaxIntegrationTime;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV8858_IsiGetGainIss
 *
 * @brief   Reads gain values from the image sensor module.
 *
 * @param   handle                  OV8858 sensor instance handle
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 不用改；获得GAIN值
 *****************************************************************************/
RESULT OV8858_IsiGetGainIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain
)
{
	uint32_t data= 0;
	uint32_t result_gain= 0;
	
	OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        return ( RET_NULL_POINTER );
    }

	
	result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_AEC_AGC_ADJ_H, &data);
	TRACE( OV8858_INFO, " -------reg3508:%x-------\n",data );
	result_gain = (data & 0x07) ;
	result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_AEC_AGC_ADJ_L, &data);
	TRACE( OV8858_INFO, " -------reg3509:%x-------\n",data );
	result_gain = (result_gain<<8) + data;
	*pSetGain = ( (float)result_gain ) / OV8858_MAXN_GAIN;
	
    //*pSetGain = pOV8858Ctx->AecCurGain;
    

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetGainIncrementIss
 *
 * @brief   Get smallest possible gain increment.
 *
 * @param   handle                  OV8858 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 不用改；获得GAIN最小值
 *****************************************************************************/
RESULT OV8858_IsiGetGainIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL)
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV8858Ctx->AecGainIncrement;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiSetGainIss
 *
 * @brief   Writes gain values to the image sensor module.
 *          Updates current gain and exposure in sensor struct/state.
 *
 * @param   handle                  OV8858 sensor instance handle
 * @param   NewGain                 gain to be set
 * @param   pSetGain                set gain
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_FAILURE
 * 不用改；设置gain值
 *****************************************************************************/
RESULT OV8858_IsiSetGainIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               *pSetGain
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint16_t usGain = 0;
	uint32_t data= 0;
	uint32_t result_gain= 0;

    TRACE( OV8858_INFO, "%s: (enter) pOV8858Ctx->AecMaxGain(%f) \n", __FUNCTION__,pOV8858Ctx->AecMaxGain);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetGain == NULL)
    {
        TRACE( OV8858_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

  
    if( NewGain < pOV8858Ctx->AecMinGain ) NewGain = pOV8858Ctx->AecMinGain;
    if( NewGain > pOV8858Ctx->AecMaxGain ) NewGain = pOV8858Ctx->AecMaxGain;

    usGain = (uint16_t)(NewGain * OV8858_MAXN_GAIN+0.5); //大概加0.5 hkw

    // write new gain into sensor registers, do not write if nothing has changed
    if( (usGain != pOV8858Ctx->OldGain) )
    {
        result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_AEC_AGC_ADJ_H, (usGain>>8)&0x07); //fix by ov8858 datasheet
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_AEC_AGC_ADJ_L, (usGain&0xff));
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        pOV8858Ctx->OldGain = usGain;

		/*osSleep(30);
		result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_AEC_AGC_ADJ_H, &data);
		TRACE( OV8858_ERROR, " -------reg35088888888:%x-------\n",data );
		result_gain = (data & 0x07) ;
		result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_AEC_AGC_ADJ_L, &data);
		TRACE( OV8858_ERROR, " -------reg35099999999:%x-------\n",data );
		result_gain = (result_gain<<8) + data;*/
		
    }

    //calculate gain actually set
    pOV8858Ctx->AecCurGain = ( (float)usGain ) / OV8858_MAXN_GAIN;

    //return current state
    *pSetGain = pOV8858Ctx->AecCurGain;
    TRACE( OV8858_INFO, "-----------%s: psetgain=%f, NewGain=%f,result_gain=%x\n", __FUNCTION__, *pSetGain, NewGain,result_gain);

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV8858_IsiGetIntegrationTimeIss
 *
 * @brief   Reads integration time values from the image sensor module.
 *
 * @param   handle                  OV8858 sensor instance handle
 * @param   pSetIntegrationTime     set integration time
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 获得曝光时间 不用改
 *****************************************************************************/
RESULT OV8858_IsiGetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               *pSetIntegrationTime
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetIntegrationTime == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetIntegrationTime = pOV8858Ctx->AecCurIntegrationTime;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetIntegrationTimeIncrementIss
 *
 * @brief   Get smallest possible integration time increment.
 *
 * @param   handle                  OV8858 sensor instance handle
 * @param   pIncr                   increment
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * 获得曝光时间的step 不用改
 *****************************************************************************/
RESULT OV8858_IsiGetIntegrationTimeIncrementIss
(
    IsiSensorHandle_t   handle,
    float               *pIncr
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pIncr == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOV8858Ctx->AecIntegrationTimeIncrement;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiSetIntegrationTimeIss
 *
 * @brief   Writes gain and integration time values to the image sensor module.
 *          Updates current integration time and exposure in sensor
 *          struct/state.
 *
 * @param   handle                  OV8858 sensor instance handle
 * @param   NewIntegrationTime      integration time to be set
 * @param   pSetIntegrationTime     set integration time
 * @param   pNumberOfFramesToSkip   number of frames to skip until AE is
 *                                  executed again
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_FAILURE
 * @retval  RET_DIVISION_BY_ZERO
 *设置曝光时间；根据应用手册修改寄存器宏
 *****************************************************************************/
RESULT OV8858_IsiSetIntegrationTimeIss
(
    IsiSensorHandle_t   handle,
    float               NewIntegrationTime,
    float               *pSetIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t CoarseIntegrationTime = 0;
	uint32_t data= 0;
	uint32_t result_intertime= 0;
	
    //uint32_t FineIntegrationTime   = 0; //not supported by OV8858

    float ShutterWidthPck = 0.0f; //shutter width in pixel clock periods

    TRACE( OV8858_INFO, "%s: (enter) NewIntegrationTime: %f (min: %f   max: %f)\n", __FUNCTION__,
        NewIntegrationTime,
        pOV8858Ctx->AecMinIntegrationTime,
        pOV8858Ctx->AecMaxIntegrationTime);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetIntegrationTime == NULL) || (pNumberOfFramesToSkip == NULL) )
    {
        TRACE( OV8858_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    //maximum and minimum integration time is limited by the sensor, if this limit is not
    //considered, the exposure control loop needs lots of time to return to a new state
    //so limit to allowed range
    if ( NewIntegrationTime > pOV8858Ctx->AecMaxIntegrationTime ) NewIntegrationTime = pOV8858Ctx->AecMaxIntegrationTime;
    if ( NewIntegrationTime < pOV8858Ctx->AecMinIntegrationTime ) NewIntegrationTime = pOV8858Ctx->AecMinIntegrationTime;

    //the actual integration time is given by
    //integration_time = ( coarse_integration_time * line_length_pck + fine_integration_time ) / vt_pix_clk_freq
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck )
    //fine_integration_time   = integration_time * vt_pix_clk_freq - coarse_integration_time * line_length_pck
    //
    //fine integration is not supported by OV8858
    //=>
    //coarse_integration_time = (int)( integration_time * vt_pix_clk_freq  / line_length_pck + 0.5 )

    ShutterWidthPck = NewIntegrationTime * ( (float)pOV8858Ctx->VtPixClkFreq );

    // avoid division by zero
    if ( pOV8858Ctx->LineLengthPck == 0 )
    {
        TRACE( OV8858_ERROR, "%s: Division by zero!\n", __FUNCTION__ );
        return ( RET_DIVISION_BY_ZERO );
    }

    //calculate the integer part of the integration time in units of line length
    //calculate the fractional part of the integration time in units of pixel clocks
    //CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV8858Ctx->LineLengthPck) );
    //FineIntegrationTime   = ( (uint32_t)ShutterWidthPck ) - ( CoarseIntegrationTime * pOV8858Ctx->LineLengthPck );
    CoarseIntegrationTime = (uint32_t)( ShutterWidthPck / ((float)pOV8858Ctx->LineLengthPck) + 0.5f );

    // write new integration time into sensor registers
    // do not write if nothing has changed
    if( CoarseIntegrationTime != pOV8858Ctx->OldCoarseIntegrationTime )
    {//
        result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_AEC_EXPO_H, (CoarseIntegrationTime & 0x0000F000U) >> 12U );//fix by ov8858 datasheet
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_AEC_EXPO_M, (CoarseIntegrationTime & 0x00000FF0U) >> 4U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
        result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_AEC_EXPO_L, (CoarseIntegrationTime & 0x0000000FU) << 4U );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );


        pOV8858Ctx->OldCoarseIntegrationTime = CoarseIntegrationTime;   // remember current integration time
        *pNumberOfFramesToSkip = 1U; //skip 1 frame
        
		/*osSleep(30);
		result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_AEC_EXPO_H, &data);
		TRACE( OV8858_ERROR, " -------reg3500:%x-------\n",data );
		result_intertime = (data & 0x0f) << 8;
		result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_AEC_EXPO_M, &data);
		TRACE( OV8858_ERROR, " -------reg3501:%x-------\n",data );
		result_intertime = result_intertime + data;
		result = OV8858_IsiRegReadIss ( pOV8858Ctx, OV8858_AEC_EXPO_L, &data);
		TRACE( OV8858_ERROR, " -------reg3502:%x-------\n",data );
		result_intertime = (result_intertime << 4) + (data >> 4);*/
		
    }
    else
    {
        *pNumberOfFramesToSkip = 0U; //no frame skip
    }

    //if( FineIntegrationTime != pOV8858Ctx->OldFineIntegrationTime )
    //{
    //    result = OV8858_IsiRegWriteIss( pOV8858Ctx, ... , FineIntegrationTime );
    //    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    //    pOV8858Ctx->OldFineIntegrationTime = FineIntegrationTime; //remember current integration time
    //    *pNumberOfFramesToSkip = 1U; //skip 1 frame
    //}

    //calculate integration time actually set
    //pOV8858Ctx->AecCurIntegrationTime = ( ((float)CoarseIntegrationTime) * ((float)pOV8858Ctx->LineLengthPck) + ((float)FineIntegrationTime) ) / pOV8858Ctx->VtPixClkFreq;
    pOV8858Ctx->AecCurIntegrationTime = ((float)CoarseIntegrationTime) * ((float)pOV8858Ctx->LineLengthPck) / pOV8858Ctx->VtPixClkFreq;

    //return current state
    *pSetIntegrationTime = pOV8858Ctx->AecCurIntegrationTime;

    TRACE( OV8858_DEBUG, "%s:\n"
         "pOV8858Ctx->VtPixClkFreq:%f pOV8858Ctx->LineLengthPck:%x \n"
         "SetTi=%f    NewTi=%f  CoarseIntegrationTime=%x\n"
         "result_intertime = %x\n H:%x\n M:%x\n L:%x\n", __FUNCTION__, 
         pOV8858Ctx->VtPixClkFreq,pOV8858Ctx->LineLengthPck,
         *pSetIntegrationTime,NewIntegrationTime,CoarseIntegrationTime,
         result_intertime,
         (CoarseIntegrationTime & 0x0000F000U) >> 12U ,
         (CoarseIntegrationTime & 0x00000FF0U) >> 4U,
         (CoarseIntegrationTime & 0x0000000FU) << 4U);
    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}




/*****************************************************************************/
/**
 *          OV8858_IsiExposureControlIss
 *
 * @brief   Camera hardware dependent part of the exposure control loop.
 *          Calculates appropriate register settings from the new exposure
 *          values and writes them to the image sensor module.
 *
 * @param   handle                  OV8858 sensor instance handle
 * @param   NewGain                 newly calculated gain to be set
 * @param   NewIntegrationTime      newly calculated integration time to be set
 * @param   pNumberOfFramesToSkip   number of frames to skip until AE is
 *                                  executed again
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_FAILURE
 * @retval  RET_DIVISION_BY_ZERO
 * 不用改，设置整个曝光；
 *****************************************************************************/
RESULT OV8858_IsiExposureControlIss
(
    IsiSensorHandle_t   handle,
    float               NewGain,
    float               NewIntegrationTime,
    uint8_t             *pNumberOfFramesToSkip,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pNumberOfFramesToSkip == NULL)
            || (pSetGain == NULL)
            || (pSetIntegrationTime == NULL) )
    {
        TRACE( OV8858_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_NULL_POINTER );
    }

    TRACE( OV8858_INFO, "%s: g=%f, Ti=%f\n", __FUNCTION__, NewGain, NewIntegrationTime );


    result = OV8858_IsiSetIntegrationTimeIss( handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip );
    result = OV8858_IsiSetGainIss( handle, NewGain, pSetGain );

    TRACE( OV8858_INFO, "%s: set: g=%f, Ti=%f, skip=%d\n", __FUNCTION__, *pSetGain, *pSetIntegrationTime, *pNumberOfFramesToSkip );
    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetCurrentExposureIss
 *
 * @brief   Returns the currently adjusted AE values
 *
 * @param   handle                  OV8858 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *不用改，获取gain和exposure 时间
 *****************************************************************************/
RESULT OV8858_IsiGetCurrentExposureIss
(
    IsiSensorHandle_t   handle,
    float               *pSetGain,
    float               *pSetIntegrationTime
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( (pSetGain == NULL) || (pSetIntegrationTime == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetGain            = pOV8858Ctx->AecCurGain;
    *pSetIntegrationTime = pOV8858Ctx->AecCurIntegrationTime;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetResolutionIss
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
 * 不用改；
 *****************************************************************************/
RESULT OV8858_IsiGetResolutionIss
(
    IsiSensorHandle_t   handle,
    uint32_t            *pSetResolution
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pSetResolution == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    *pSetResolution = pOV8858Ctx->Config.Resolution;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetAfpsInfoHelperIss
 *
 * @brief   Calc AFPS sub resolution settings for the given resolution
 *
 * @param   pOV8858Ctx             OV8858 sensor instance (dummy!) context
 * @param   Resolution              Any supported resolution to query AFPS params for
 * @param   pAfpsInfo               Reference of AFPS info structure to write the results to
 * @param   AfpsStageIdx            Index of current AFPS stage to use
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * 不用改；没用；
 *****************************************************************************/
static RESULT OV8858_IsiGetAfpsInfoHelperIss(
    OV8858_Context_t   *pOV8858Ctx,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo,
    uint32_t            AfpsStageIdx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    DCT_ASSERT(pOV8858Ctx != NULL);
    DCT_ASSERT(pAfpsInfo != NULL);
    DCT_ASSERT(AfpsStageIdx <= ISI_NUM_AFPS_STAGES);

    // update resolution in copy of config in context
    pOV8858Ctx->Config.Resolution = Resolution;

    // tell sensor about that
    result = OV8858_SetupOutputWindowInternal( pOV8858Ctx, &pOV8858Ctx->Config,BOOL_FALSE,BOOL_FALSE );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: SetupOutputWindow failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // update limits & stuff (reset current & old settings)
    result = OV8858_AecSetModeParameters( pOV8858Ctx, &pOV8858Ctx->Config );
    if ( result != RET_SUCCESS )
    {
        TRACE( OV8858_ERROR, "%s: AecSetModeParameters failed for resolution ID %08x.\n", __FUNCTION__, Resolution);
        return ( result );
    }

    // take over params
    pAfpsInfo->Stage[AfpsStageIdx].Resolution = Resolution;
    pAfpsInfo->Stage[AfpsStageIdx].MaxIntTime = pOV8858Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecMinGain           = pOV8858Ctx->AecMinGain;
    pAfpsInfo->AecMaxGain           = pOV8858Ctx->AecMaxGain;
    pAfpsInfo->AecMinIntTime        = pOV8858Ctx->AecMinIntegrationTime;
    pAfpsInfo->AecMaxIntTime        = pOV8858Ctx->AecMaxIntegrationTime;
    pAfpsInfo->AecSlowestResolution = Resolution;
    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV8858_IsiGetAfpsInfoIss
 *
 * @brief   Returns the possible AFPS sub resolution settings for the given resolution series
 *
 * @param   handle                  OV8858 sensor instance handle
 * @param   Resolution              Any resolution within the AFPS group to query;
 *                                  0 (zero) to use the currently configured resolution
 * @param   pAfpsInfo               Reference of AFPS info structure to store the results
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * @retval  RET_NOTSUPP
 * 不用改；没用；
 *****************************************************************************/
RESULT OV8858_IsiGetAfpsInfoIss(
    IsiSensorHandle_t   handle,
    uint32_t            Resolution,
    IsiAfpsInfo_t*      pAfpsInfo
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t RegValue = 0;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        TRACE( OV8858_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __FUNCTION__ );
        return ( RET_WRONG_HANDLE );
    }

    if ( pAfpsInfo == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    // use currently configured resolution?
    if (Resolution == 0)
    {
        Resolution = pOV8858Ctx->Config.Resolution;
    }

    // prepare index
    uint32_t idx = 0;

    // set current resolution data in info struct
    pAfpsInfo->CurrResolution = pOV8858Ctx->Config.Resolution;
    pAfpsInfo->CurrMinIntTime = pOV8858Ctx->AecMinIntegrationTime;
    pAfpsInfo->CurrMaxIntTime = pOV8858Ctx->AecMaxIntegrationTime;

    // allocate dummy context used for Afps parameter calculation as a copy of current context
    OV8858_Context_t *pDummyCtx = (OV8858_Context_t*) malloc( sizeof(OV8858_Context_t) );
    if ( pDummyCtx == NULL )
    {
        TRACE( OV8858_ERROR,  "%s: Can't allocate dummy OV8858 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    *pDummyCtx = *pOV8858Ctx;

    // set AFPS mode in dummy context
    pDummyCtx->isAfpsRun = BOOL_TRUE;

#define AFPSCHECKANDADD(_res_) \
    { \
        RESULT lres = OV8858_IsiGetAfpsInfoHelperIss( pDummyCtx, _res_, pAfpsInfo, idx); \
        if ( lres == RET_SUCCESS ) \
        { \
            ++idx; \
        } \
        else \
        { \
            UPDATE_RESULT( result, lres ); \
        } \
    }

    // check which AFPS series is requested and build its params list for the enabled AFPS resolutions
    switch (pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes)
    {
        case SUPPORT_MIPI_ONE_LANE:
        {

            break;
        }

        case SUPPORT_MIPI_TWO_LANE:
        {
            switch(Resolution)
            {
                default:
                    TRACE( OV8858_DEBUG,  "%s: Resolution %08x not supported by AFPS\n",  __FUNCTION__, Resolution );
                    result = RET_NOTSUPP;
                    break;
                   
                case ISI_RES_1632_1224P30:
                case ISI_RES_1632_1224P25:
                case ISI_RES_1632_1224P20:
                case ISI_RES_1632_1224P15:
                case ISI_RES_1632_1224P10:
                    AFPSCHECKANDADD( ISI_RES_1632_1224P30 );
                    AFPSCHECKANDADD( ISI_RES_1632_1224P25 );
                    AFPSCHECKANDADD( ISI_RES_1632_1224P20 );
                    AFPSCHECKANDADD( ISI_RES_1632_1224P15 );
                    AFPSCHECKANDADD( ISI_RES_1632_1224P10 );
                    break;

                case ISI_RES_3264_2448P15:
                case ISI_RES_3264_2448P7:
                    AFPSCHECKANDADD( ISI_RES_3264_2448P15 );
                    AFPSCHECKANDADD( ISI_RES_3264_2448P7 );
                    break;

                // check next series here...
            }
        

            break;
        }

        case SUPPORT_MIPI_FOUR_LANE:
        {
            switch(Resolution)
            {
                default:
                    TRACE( OV8858_DEBUG,  "%s: Resolution %08x not supported by AFPS\n",  __FUNCTION__, Resolution );
                    result = RET_NOTSUPP;
                    break;
                   
                case ISI_RES_1632_1224P30:
                case ISI_RES_1632_1224P25:
                case ISI_RES_1632_1224P20:
                case ISI_RES_1632_1224P15:
                case ISI_RES_1632_1224P10:
                    AFPSCHECKANDADD( ISI_RES_1632_1224P30 );
                    AFPSCHECKANDADD( ISI_RES_1632_1224P25 );
                    AFPSCHECKANDADD( ISI_RES_1632_1224P20 );
                    AFPSCHECKANDADD( ISI_RES_1632_1224P15 );
                    AFPSCHECKANDADD( ISI_RES_1632_1224P10 );
                    break;
                    
                case ISI_RES_3264_2448P30:
                case ISI_RES_3264_2448P25:
                case ISI_RES_3264_2448P20:
                case ISI_RES_3264_2448P15:
                case ISI_RES_3264_2448P10:
                case ISI_RES_3264_2448P7:
                    AFPSCHECKANDADD( ISI_RES_3264_2448P30 );
                    AFPSCHECKANDADD( ISI_RES_3264_2448P25 );
                    AFPSCHECKANDADD( ISI_RES_3264_2448P20 );
                    AFPSCHECKANDADD( ISI_RES_3264_2448P15 );
                    AFPSCHECKANDADD( ISI_RES_3264_2448P10 );
                    AFPSCHECKANDADD( ISI_RES_3264_2448P7 );
                    break;

                // check next series here...
            }
        

            break;
        }

        default:
            TRACE( OV8858_ERROR,  "%s: pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes(0x%x) is invalidate!\n", 
                __FUNCTION__, pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes );
            result = RET_FAILURE;
            break;

    }

    // release dummy context again
    free(pDummyCtx);

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetCalibKFactor
 *
 * @brief   Returns the OV8858 specific K-Factor
 *
 * @param   handle       OV8858 sensor instance handle
 * @param   pIsiKFactor  Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；没用；
 *****************************************************************************/
static RESULT OV8858_IsiGetCalibKFactor
(
    IsiSensorHandle_t   handle,
    Isi1x1FloatMatrix_t **pIsiKFactor
)
{
	return ( RET_SUCCESS );
	OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiKFactor == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*pIsiKFactor = (Isi1x1FloatMatrix_t *)&OV8858_KFactor;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/*****************************************************************************/
/**
 *          OV8858_IsiGetCalibPcaMatrix
 *
 * @brief   Returns the OV8858 specific PCA-Matrix
 *
 * @param   handle          OV8858 sensor instance handle
 * @param   pIsiPcaMatrix   Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；没用；
 *****************************************************************************/
static RESULT OV8858_IsiGetCalibPcaMatrix
(
    IsiSensorHandle_t   handle,
    Isi3x2FloatMatrix_t **pIsiPcaMatrix
)
{
	return ( RET_SUCCESS );
	OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiPcaMatrix == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*pIsiPcaMatrix = (Isi3x2FloatMatrix_t *)&OV8858_PCAMatrix;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns the sensor specific SvdMean-Vector
 *
 * @param   handle              OV8858 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；没用；return success;
 *****************************************************************************/
static RESULT OV8858_IsiGetCalibSvdMeanValue
(
    IsiSensorHandle_t   handle,
    Isi3x1FloatMatrix_t **pIsiSvdMeanValue
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiSvdMeanValue == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*pIsiSvdMeanValue = (Isi3x1FloatMatrix_t *)&OV8858_SVDMeanValue;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetCalibSvdMeanValue
 *
 * @brief   Returns a pointer to the sensor specific centerline, a straight
 *          line in Hesse normal form in Rg/Bg colorspace
 *
 * @param   handle              OV8858 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；没用；return success;
 *****************************************************************************/
static RESULT OV8858_IsiGetCalibCenterLine
(
    IsiSensorHandle_t   handle,
    IsiLine_t           **ptIsiCenterLine
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiCenterLine == NULL )
    {
        return ( RET_NULL_POINTER );
    }

   // *ptIsiCenterLine = (IsiLine_t*)&OV8858_CenterLine;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetCalibClipParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for Rg/Bg color
 *          space clipping
 *
 * @param   handle              OV8858 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；没用；return success;
 *****************************************************************************/
static RESULT OV8858_IsiGetCalibClipParam
(
    IsiSensorHandle_t   handle,
    IsiAwbClipParm_t    **pIsiClipParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsiClipParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*pIsiClipParam = (IsiAwbClipParm_t *)&OV8858_AwbClipParm;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetCalibGlobalFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for AWB out of
 *          range handling
 *
 * @param   handle              OV8858 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；没用；return success;
 *****************************************************************************/
static RESULT OV8858_IsiGetCalibGlobalFadeParam
(
    IsiSensorHandle_t       handle,
    IsiAwbGlobalFadeParm_t  **ptIsiGlobalFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiGlobalFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //*ptIsiGlobalFadeParam = (IsiAwbGlobalFadeParm_t *)&OV8858_AwbGlobalFadeParm;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetCalibFadeParam
 *
 * @brief   Returns a pointer to the sensor specific arrays for near white
 *          pixel parameter calculations
 *
 * @param   handle              OV8858 sensor instance handle
 * @param   pIsiSvdMeanValue    Pointer to Pointer receiving the memory address
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；没用；return success;
 *****************************************************************************/
static RESULT OV8858_IsiGetCalibFadeParam
(
    IsiSensorHandle_t   handle,
    IsiAwbFade2Parm_t   **ptIsiFadeParam
)
{
    IsiSensorContext_t *pSensorCtx = (IsiSensorContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pSensorCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiFadeParam == NULL )
    {
        return ( RET_NULL_POINTER );
    }

   // *ptIsiFadeParam = (IsiAwbFade2Parm_t *)&OV8858_AwbFade2Parm;

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/*****************************************************************************/
/**
 *          OV8858_IsiGetIlluProfile
 *
 * @brief   Returns a pointer to illumination profile idetified by CieProfile
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
 * 不用改；没用；return success;
 *****************************************************************************/
static RESULT OV8858_IsiGetIlluProfile
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiIlluProfile_t    **ptIsiIlluProfile
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;
	return ( result );
	#if 0
    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ptIsiIlluProfile == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        uint16_t i;

        *ptIsiIlluProfile = NULL;

        /* check if we've a default profile */
        for ( i=0U; i<OV8858_ISIILLUPROFILES_DEFAULT; i++ )
        {
            if ( OV8858_IlluProfileDefault[i].id == CieProfile )
            {
                *ptIsiIlluProfile = &OV8858_IlluProfileDefault[i];
                break;
            }
        }

       // result = ( *ptIsiIlluProfile != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
    }

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
	#endif
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetLscMatrixTable
 *
 * @brief   Returns a pointer to illumination profile idetified by CieProfile
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
 * 不用改；没用；return success;
 *****************************************************************************/
static RESULT OV8858_IsiGetLscMatrixTable
(
    IsiSensorHandle_t   handle,
    const uint32_t      CieProfile,
    IsiLscMatrixTable_t **pLscMatrixTable
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;
	return ( result );
	
	#if 0
    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pLscMatrixTable == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        uint16_t i;


        switch ( CieProfile )
        {
            case ISI_CIEPROF_A:
            {
                if ( ( pOV8858Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_A_1920x1080;
                }
                #if 0
                else if ( pOV8858Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_A_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8858_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F2:
            {
                if ( ( pOV8858Ctx->Config.Resolution == ISI_RES_TV1080P30 ) )
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_F2_1920x1080;
                }
                #if 0
                else if ( pOV8858Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_F2_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8858_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D50:
            {
                if ( ( pOV8858Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_D50_1920x1080;
                }
                #if 0
                else if ( pOV8858Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_D50_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8858_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_D65:
            case ISI_CIEPROF_D75:
            {
                if ( ( pOV8858Ctx->Config.Resolution == ISI_RES_TV1080P30 ) )
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_D65_1920x1080;
                }
                #if 0
                else if ( pOV8858Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_D65_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8858_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            case ISI_CIEPROF_F11:
            {
                if ( ( pOV8858Ctx->Config.Resolution == ISI_RES_TV1080P30 ))
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_F11_1920x1080;
                }
                #if 0
                else if ( pOV8858Ctx->Config.Resolution == ISI_RES_4416_3312 )
                {
                    *pLscMatrixTable = &OV8858_LscMatrixTable_CIE_F11_4416x3312;
                }
                #endif
                else
                {
                    TRACE( OV8858_ERROR, "%s: Resolution (%08x) not supported\n", __FUNCTION__, CieProfile );
                    *pLscMatrixTable = NULL;
                }

                break;
            }

            default:
            {
                TRACE( OV8858_ERROR, "%s: Illumination not supported\n", __FUNCTION__ );
                *pLscMatrixTable = NULL;
                break;
            }
        }

        result = ( *pLscMatrixTable != NULL ) ?  RET_SUCCESS : RET_NOTAVAILABLE;
    }

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
	#endif
}


/*****************************************************************************/
/**
 *          OV8858_IsiMdiInitMotoDriveMds
 *
 * @brief   General initialisation tasks like I/O initialisation.
 *
 * @param   handle              OV8858 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；
 *****************************************************************************/
static RESULT OV8858_IsiMdiInitMotoDriveMds
(
    IsiSensorHandle_t   handle
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiMdiSetupMotoDrive
 *
 * @brief   Setup of the MotoDrive and return possible max step.
 *
 * @param   handle          OV8858 sensor instance handle
 *          pMaxStep        pointer to variable to receive the maximum
 *                          possible focus step
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；
 *****************************************************************************/
static RESULT OV8858_IsiMdiSetupMotoDrive
(
    IsiSensorHandle_t   handle,
    uint32_t            *pMaxStep
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;
	uint32_t vcm_movefull_t;
    RESULT result = RET_SUCCESS;

    //TRACE( OV8858_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMaxStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }
 if ((pOV8858Ctx->VcmInfo.StepMode & 0x0c) != 0) {
 	vcm_movefull_t = 64* (1<<(pOV8858Ctx->VcmInfo.StepMode & 0x03)) *1024/((1 << (((pOV8858Ctx->VcmInfo.StepMode & 0x0c)>>2)-1))*1000);
 }else{
 	vcm_movefull_t =64*1023/1000;
   TRACE( OV8858_ERROR, "%s: (---NO SRC---)\n", __FUNCTION__);
 }
 
	  *pMaxStep = (MAX_LOG|(vcm_movefull_t<<16));
   // *pMaxStep = MAX_LOG;

    result = OV8858_IsiMdiFocusSet( handle, MAX_LOG );

    //TRACE( OV8858_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiMdiFocusSet
 *
 * @brief   Drives the lens system to a certain focus point.
 *
 * @param   handle          OV8858 sensor instance handle
 *          AbsStep         absolute focus point to apply
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 参考14825；外置马达；
 *****************************************************************************/
static RESULT OV8858_IsiMdiFocusSet
(
    IsiSensorHandle_t   handle,
    const uint32_t      Position
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    uint32_t nPosition;
    uint8_t  data[2] = { 0, 0 };

    //TRACE( OV8858_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* map 64 to 0 -> infinity */
    //nPosition = ( Position >= MAX_LOG ) ? 0 : ( MAX_REG - (Position * 16U) );
	if( Position > MAX_LOG ){
		TRACE( OV8858_ERROR, "%s: pOV8858Ctx Position (%d) max_position(%d)\n", __FUNCTION__,Position, MAX_LOG);
		//Position = MAX_LOG;
	}	
    /* ddl@rock-chips.com: v0.3.0 */
    if ( Position >= MAX_LOG )
        nPosition = pOV8858Ctx->VcmInfo.StartCurrent;
    else 
        nPosition = pOV8858Ctx->VcmInfo.StartCurrent + (pOV8858Ctx->VcmInfo.Step*(MAX_LOG-Position));
    /* ddl@rock-chips.com: v0.6.0 */
    if (nPosition > MAX_VCMDRV_REG)  
        nPosition = MAX_VCMDRV_REG;

    TRACE( OV8858_INFO, "%s: focus set position_reg_value(%d) position(%d) \n", __FUNCTION__, nPosition, Position);
    data[0] = (uint8_t)(0x00U | (( nPosition & 0x3F0U ) >> 4U));                 // PD,  1, D9..D4, see AD5820 datasheet
    //data[1] = (uint8_t)( ((nPosition & 0x0FU) << 4U) | MDI_SLEW_RATE_CTRL );    // D3..D0, S3..S0
	data[1] = (uint8_t)( ((nPosition & 0x0FU) << 4U) | pOV8858Ctx->VcmInfo.StepMode );
	
    //TRACE( OV8858_ERROR, "%s: value = %d, 0x%02x 0x%02x\n", __FUNCTION__, nPosition, data[0], data[1] );

    result = HalWriteI2CMem( pOV8858Ctx->IsiCtx.HalHandle,
                             pOV8858Ctx->IsiCtx.I2cAfBusNum,
                             pOV8858Ctx->IsiCtx.SlaveAfAddress,
                             0,
                             pOV8858Ctx->IsiCtx.NrOfAfAddressBytes,
                             data,
                             2U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    //TRACE( OV8858_ERROR, "%s: (exit)\n", __FUNCTION__);
    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiMdiFocusGet
 *
 * @brief   Retrieves the currently applied focus point.
 *
 * @param   handle          OV8858 sensor instance handle
 *          pAbsStep        pointer to a variable to receive the current
 *                          focus point
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 参考14825；外置马达；
 *****************************************************************************/
static RESULT OV8858_IsiMdiFocusGet
(
    IsiSensorHandle_t   handle,
    uint32_t            *pAbsStep
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;
    uint8_t  data[2] = { 0, 0 };

    //TRACE( OV8858_ERROR, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pAbsStep == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    result = HalReadI2CMem( pOV8858Ctx->IsiCtx.HalHandle,
                            pOV8858Ctx->IsiCtx.I2cAfBusNum,
                            pOV8858Ctx->IsiCtx.SlaveAfAddress,
                            0,
                            pOV8858Ctx->IsiCtx.NrOfAfAddressBytes,
                            data,
                            2U );
    RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

    //TRACE( OV8858_ERROR, "%s: value = 0x%02x 0x%02x\n", __FUNCTION__, data[0], data[1] );

    /* Data[0] = PD,  1, D9..D4, see VM149C datasheet */
    /* Data[1] = D3..D0, S3..S0 */
    *pAbsStep = ( ((uint32_t)(data[0] & 0x3FU)) << 4U ) | ( ((uint32_t)data[1]) >> 4U );

    /*  //map 0 to 64 -> infinity 
    if( *pAbsStep == 0 )
    {
        *pAbsStep = MAX_LOG;
    }
    else
    {
        *pAbsStep = ( MAX_REG - *pAbsStep ) / 16U;
    }*/
	if( *pAbsStep <= pOV8858Ctx->VcmInfo.StartCurrent)
    {
        *pAbsStep = MAX_LOG;
    }
    else if((*pAbsStep>pOV8858Ctx->VcmInfo.StartCurrent) && (*pAbsStep<=pOV8858Ctx->VcmInfo.RatedCurrent))
    {
        *pAbsStep = (pOV8858Ctx->VcmInfo.RatedCurrent - *pAbsStep ) / pOV8858Ctx->VcmInfo.Step;
    }
	else
	{
		*pAbsStep = 0;
	}
   // TRACE( OV8858_ERROR, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiMdiFocusCalibrate
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV8858 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改；没用；
 *****************************************************************************/
static RESULT OV8858_IsiMdiFocusCalibrate
(
    IsiSensorHandle_t   handle
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/*****************************************************************************/
/**
 *          OV8858_IsiActivateTestPattern
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV8858 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 *不用改，没用，return；
 ******************************************************************************/
static RESULT OV8858_IsiActivateTestPattern
(
    IsiSensorHandle_t   handle,
    const bool_t        enable
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;
	return ( result );

	#if 0
    uint32_t ulRegValue = 0UL;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( BOOL_TRUE == enable )
    {
        /* enable test-pattern */
        result = OV8858_IsiRegReadIss( pOV8858Ctx, OV8858_PRE_ISP_CTRL00, &ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        ulRegValue |= ( 0x80U );

        result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_PRE_ISP_CTRL00, ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }
    else
    {
        /* disable test-pattern */
        result = OV8858_IsiRegReadIss( pOV8858Ctx, OV8858_PRE_ISP_CTRL00, &ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );

        ulRegValue &= ~( 0x80 );

        result = OV8858_IsiRegWriteIss( pOV8858Ctx, OV8858_PRE_ISP_CTRL00, ulRegValue );
        RETURN_RESULT_IF_DIFFERENT( RET_SUCCESS, result );
    }

     pOV8858Ctx->TestPattern = enable;
    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
	#endif
}



/*****************************************************************************/
/**
 *          OV8858_IsiGetSensorMipiInfoIss
 *
 * @brief   Triggers a forced calibration of the focus hardware.
 *
 * @param   handle          OV8858 sensor instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_NULL_POINTER
 * 不用改
 ******************************************************************************/
static RESULT OV8858_IsiGetSensorMipiInfoIss
(
    IsiSensorHandle_t   handle,
    IsiSensorMipiInfo   *ptIsiSensorMipiInfo
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }


    if ( ptIsiSensorMipiInfo == NULL )
    {
        return ( result );
    }

	ptIsiSensorMipiInfo->ucMipiLanes = pOV8858Ctx->IsiSensorMipiInfo.ucMipiLanes;
    ptIsiSensorMipiInfo->ulMipiFreq= pOV8858Ctx->IsiSensorMipiInfo.ulMipiFreq;
    ptIsiSensorMipiInfo->sensorHalDevID = pOV8858Ctx->IsiSensorMipiInfo.sensorHalDevID;
    TRACE( OV8858_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

static RESULT OV8858_IsiGetSensorIsiVersion
(  IsiSensorHandle_t   handle,
   unsigned int*     pVersion
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
    	TRACE( OV8858_ERROR, "%s: pOV8858Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pVersion == NULL)
	{
		TRACE( OV8858_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pVersion = CONFIG_ISI_VERSION;
	return result;
}

static RESULT OV8858_IsiGetSensorTuningXmlVersion
(  IsiSensorHandle_t   handle,
   char**     pTuningXmlVersion
)
{
    OV8858_Context_t *pOV8858Ctx = (OV8858_Context_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( OV8858_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pOV8858Ctx == NULL )
    {
    	TRACE( OV8858_ERROR, "%s: pOV8858Ctx IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

	if(pTuningXmlVersion == NULL)
	{
		TRACE( OV8858_ERROR, "%s: pVersion IS NULL\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
	}

	*pTuningXmlVersion = OV8858_NEWEST_TUNING_XML;
	return result;
}


/*****************************************************************************/
/**
 *          OV8858_IsiGetSensorIss
 *
 * @brief   fills in the correct pointers for the sensor description struct
 *
 * @param   param1      pointer to sensor description struct
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
RESULT OV8858_IsiGetSensorIss
(
    IsiSensor_t *pIsiSensor
)
{
    RESULT result = RET_SUCCESS;

    TRACE( OV8858_INFO, "%s (enter)\n", __FUNCTION__);

    if ( pIsiSensor != NULL )
    {
        pIsiSensor->pszName                             = OV8858_g_acName;
        pIsiSensor->pRegisterTable                      = OV8858_g_aRegDescription_twolane;
        pIsiSensor->pIsiSensorCaps                      = &OV8858_g_IsiSensorDefaultConfig;
		pIsiSensor->pIsiGetSensorIsiVer					= OV8858_IsiGetSensorIsiVersion;//oyyf
		pIsiSensor->pIsiGetSensorTuningXmlVersion		= OV8858_IsiGetSensorTuningXmlVersion;//oyyf
		pIsiSensor->pIsiCheckOTPInfo                    = check_read_otp;//zyc
        pIsiSensor->pIsiCreateSensorIss                 = OV8858_IsiCreateSensorIss;
        pIsiSensor->pIsiReleaseSensorIss                = OV8858_IsiReleaseSensorIss;
        pIsiSensor->pIsiGetCapsIss                      = OV8858_IsiGetCapsIss;
        pIsiSensor->pIsiSetupSensorIss                  = OV8858_IsiSetupSensorIss;
        pIsiSensor->pIsiChangeSensorResolutionIss       = OV8858_IsiChangeSensorResolutionIss;
        pIsiSensor->pIsiSensorSetStreamingIss           = OV8858_IsiSensorSetStreamingIss;
        pIsiSensor->pIsiSensorSetPowerIss               = OV8858_IsiSensorSetPowerIss;
        pIsiSensor->pIsiCheckSensorConnectionIss        = OV8858_IsiCheckSensorConnectionIss;
        pIsiSensor->pIsiGetSensorRevisionIss            = OV8858_IsiGetSensorRevisionIss;
        pIsiSensor->pIsiRegisterReadIss                 = OV8858_IsiRegReadIss;
        pIsiSensor->pIsiRegisterWriteIss                = OV8858_IsiRegWriteIss;

        /* AEC functions */
        pIsiSensor->pIsiExposureControlIss              = OV8858_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = OV8858_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = OV8858_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetCurrentExposureIss           = OV8858_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetGainIss                      = OV8858_IsiGetGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = OV8858_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = OV8858_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = OV8858_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = OV8858_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = OV8858_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetResolutionIss                = OV8858_IsiGetResolutionIss;
        pIsiSensor->pIsiGetAfpsInfoIss                  = OV8858_IsiGetAfpsInfoIss;

        /* AWB specific functions */
        pIsiSensor->pIsiGetCalibKFactor                 = OV8858_IsiGetCalibKFactor;
        pIsiSensor->pIsiGetCalibPcaMatrix               = OV8858_IsiGetCalibPcaMatrix;
        pIsiSensor->pIsiGetCalibSvdMeanValue            = OV8858_IsiGetCalibSvdMeanValue;
        pIsiSensor->pIsiGetCalibCenterLine              = OV8858_IsiGetCalibCenterLine;
        pIsiSensor->pIsiGetCalibClipParam               = OV8858_IsiGetCalibClipParam;
        pIsiSensor->pIsiGetCalibGlobalFadeParam         = OV8858_IsiGetCalibGlobalFadeParam;
        pIsiSensor->pIsiGetCalibFadeParam               = OV8858_IsiGetCalibFadeParam;
        pIsiSensor->pIsiGetIlluProfile                  = OV8858_IsiGetIlluProfile;
        pIsiSensor->pIsiGetLscMatrixTable               = OV8858_IsiGetLscMatrixTable;

        /* AF functions */
        pIsiSensor->pIsiMdiInitMotoDriveMds             = OV8858_IsiMdiInitMotoDriveMds;
        pIsiSensor->pIsiMdiSetupMotoDrive               = OV8858_IsiMdiSetupMotoDrive;
        pIsiSensor->pIsiMdiFocusSet                     = OV8858_IsiMdiFocusSet;
        pIsiSensor->pIsiMdiFocusGet                     = OV8858_IsiMdiFocusGet;
        pIsiSensor->pIsiMdiFocusCalibrate               = OV8858_IsiMdiFocusCalibrate;

        /* MIPI */
        pIsiSensor->pIsiGetSensorMipiInfoIss            = OV8858_IsiGetSensorMipiInfoIss;

        /* Testpattern */
        pIsiSensor->pIsiActivateTestPattern             = OV8858_IsiActivateTestPattern;
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( OV8858_INFO, "%s (exit)\n", __FUNCTION__);

    return ( result );
}

//fix;hkw 14825
static RESULT OV8858_IsiGetSensorI2cInfo(sensor_i2c_info_t** pdata)
{
    sensor_i2c_info_t* pSensorI2cInfo;

    pSensorI2cInfo = ( sensor_i2c_info_t * )malloc ( sizeof (sensor_i2c_info_t) );

    if ( pSensorI2cInfo == NULL )
    {
        TRACE( OV8858_ERROR,  "%s: Can't allocate OV8858 context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pSensorI2cInfo, 0, sizeof( sensor_i2c_info_t ) );

    
    pSensorI2cInfo->i2c_addr = OV8858_SLAVE_ADDR;
    pSensorI2cInfo->i2c_addr2 = OV8858_SLAVE_ADDR2;
    pSensorI2cInfo->soft_reg_addr = OV8858_SOFTWARE_RST;
    pSensorI2cInfo->soft_reg_value = OV8858_SOFTWARE_RST_VALUE;
    pSensorI2cInfo->reg_size = 2;
    pSensorI2cInfo->value_size = 1;

    {
        IsiSensorCaps_t Caps;
        sensor_caps_t *pCaps;
        uint32_t lanes,i;        

        for (i=0; i<3; i++) {
            lanes = (1<<i);
            ListInit(&pSensorI2cInfo->lane_res[i]);
            if (g_suppoted_mipi_lanenum_type & lanes) {
                Caps.Index = 0;            
                while(OV8858_IsiGetCapsIssInternal(&Caps,lanes)==RET_SUCCESS) {
                    pCaps = malloc(sizeof(sensor_caps_t));
                    if (pCaps != NULL) {
                        memcpy(&pCaps->caps,&Caps,sizeof(IsiSensorCaps_t));
                        ListPrepareItem(pCaps);
                        ListAddTail(&pSensorI2cInfo->lane_res[i], pCaps);
                    }
                    Caps.Index++;
                }
            }
        }
    }
    
    ListInit(&pSensorI2cInfo->chipid_info);

    sensor_chipid_info_t* pChipIDInfo_H = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_H )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_H, 0, sizeof(*pChipIDInfo_H) );    
    pChipIDInfo_H->chipid_reg_addr = OV8858_CHIP_ID_HIGH_BYTE;  
    pChipIDInfo_H->chipid_reg_value = OV8858_CHIP_ID_HIGH_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_H );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_H );

    sensor_chipid_info_t* pChipIDInfo_M = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_M )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_M, 0, sizeof(*pChipIDInfo_M) ); 
    pChipIDInfo_M->chipid_reg_addr = OV8858_CHIP_ID_MIDDLE_BYTE;
    pChipIDInfo_M->chipid_reg_value = OV8858_CHIP_ID_MIDDLE_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_M );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_M );
    
    sensor_chipid_info_t* pChipIDInfo_L = (sensor_chipid_info_t *) malloc( sizeof(sensor_chipid_info_t) );
    if ( !pChipIDInfo_L )
    {
        return RET_OUTOFMEM;
    }
    MEMSET( pChipIDInfo_L, 0, sizeof(*pChipIDInfo_L) ); 
    pChipIDInfo_L->chipid_reg_addr = OV8858_CHIP_ID_LOW_BYTE;
    pChipIDInfo_L->chipid_reg_value = OV8858_CHIP_ID_LOW_BYTE_DEFAULT;
    ListPrepareItem( pChipIDInfo_L );
    ListAddTail( &pSensorI2cInfo->chipid_info, pChipIDInfo_L );

	//oyyf sensor drv version
	pSensorI2cInfo->sensor_drv_version = CONFIG_SENSOR_DRV_VERSION;
	
    *pdata = pSensorI2cInfo;
    return RET_SUCCESS;
}

/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/*****************************************************************************/
/**
 */
/*****************************************************************************/
IsiCamDrvConfig_t IsiCamDrvConfig =
{
    0,
    OV8858_IsiGetSensorIss,
    {
        0,                      /**< IsiSensor_t.pszName */
        0,                      /**< IsiSensor_t.pRegisterTable */
        0,                      /**< IsiSensor_t.pIsiSensorCaps */
        0,						/**< IsiSensor_t.pIsiGetSensorIsiVer_t>*/   //oyyf add
        0,                      /**< IsiSensor_t.pIsiGetSensorTuningXmlVersion_t>*/   //oyyf add
        0,                      /**< IsiSensor_t.pIsiWhiteBalanceIlluminationChk>*/   //ddl@rock-chips.com 
        0,                      /**< IsiSensor_t.pIsiWhiteBalanceIlluminationSet>*/   //ddl@rock-chips.com
        0,                      /**< IsiSensor_t.pIsiCheckOTPInfo>*/  //zyc 
        0,                      /**< IsiSensor_t.pIsiCreateSensorIss */
        0,                      /**< IsiSensor_t.pIsiReleaseSensorIss */
        0,                      /**< IsiSensor_t.pIsiGetCapsIss */
        0,                      /**< IsiSensor_t.pIsiSetupSensorIss */
        0,                      /**< IsiSensor_t.pIsiChangeSensorResolutionIss */
        0,                      /**< IsiSensor_t.pIsiSensorSetStreamingIss */
        0,                      /**< IsiSensor_t.pIsiSensorSetPowerIss */
        0,                      /**< IsiSensor_t.pIsiCheckSensorConnectionIss */
        0,                      /**< IsiSensor_t.pIsiGetSensorRevisionIss */
        0,                      /**< IsiSensor_t.pIsiRegisterReadIss */
        0,                      /**< IsiSensor_t.pIsiRegisterWriteIss */

        0,                      /**< IsiSensor_t.pIsiExposureControlIss */
        0,                      /**< IsiSensor_t.pIsiGetGainLimitsIss */
        0,                      /**< IsiSensor_t.pIsiGetIntegrationTimeLimitsIss */
        0,                      /**< IsiSensor_t.pIsiGetCurrentExposureIss */
        0,                      /**< IsiSensor_t.pIsiGetGainIss */
        0,                      /**< IsiSensor_t.pIsiGetGainIncrementIss */
        0,                      /**< IsiSensor_t.pIsiSetGainIss */
        0,                      /**< IsiSensor_t.pIsiGetIntegrationTimeIss */
        0,                      /**< IsiSensor_t.pIsiGetIntegrationTimeIncrementIss */
        0,                      /**< IsiSensor_t.pIsiSetIntegrationTimeIss */
        0,                      /**< IsiSensor_t.pIsiGetResolutionIss */
        0,                      /**< IsiSensor_t.pIsiGetAfpsInfoIss */

        0,                      /**< IsiSensor_t.pIsiGetCalibKFactor */
        0,                      /**< IsiSensor_t.pIsiGetCalibPcaMatrix */
        0,                      /**< IsiSensor_t.pIsiGetCalibSvdMeanValue */
        0,                      /**< IsiSensor_t.pIsiGetCalibCenterLine */
        0,                      /**< IsiSensor_t.pIsiGetCalibClipParam */
        0,                      /**< IsiSensor_t.pIsiGetCalibGlobalFadeParam */
        0,                      /**< IsiSensor_t.pIsiGetCalibFadeParam */
        0,                      /**< IsiSensor_t.pIsiGetIlluProfile */
        0,                      /**< IsiSensor_t.pIsiGetLscMatrixTable */

        0,                      /**< IsiSensor_t.pIsiMdiInitMotoDriveMds */
        0,                      /**< IsiSensor_t.pIsiMdiSetupMotoDrive */
        0,                      /**< IsiSensor_t.pIsiMdiFocusSet */
        0,                      /**< IsiSensor_t.pIsiMdiFocusGet */
        0,                      /**< IsiSensor_t.pIsiMdiFocusCalibrate */

        0,                      /**< IsiSensor_t.pIsiGetSensorMipiInfoIss */

        0,                      /**< IsiSensor_t.pIsiActivateTestPattern */
    },
    OV8858_IsiGetSensorI2cInfo,
};


