#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"
#include "OV5647_priv.h"


/*****************************************************************************
 * DEFINES
 *****************************************************************************/


/*****************************************************************************
 * GLOBALS
 *****************************************************************************/

// Image sensor register settings default values taken from data sheet OV8810_DS_1.1_SiliconImage.pdf.
// The settings may be altered by the code in IsiSetupSensor.
const IsiRegDescription_t OV5647_g_aRegDescription[] =
{
    {0x370c,0x03,"",eReadWrite},
    {0x5000,0x06,"",eReadWrite},
    {0x5003,0x08,"",eReadWrite},
    {0x5a00,0x08,"",eReadWrite},
    {0x3000,0xff,"",eReadWrite},
    {0x3001,0xff,"",eReadWrite},
    {0x3002,0xff,"",eReadWrite},
    {0x301d,0xf0,"",eReadWrite},
    {0x3a18,0x00,"",eReadWrite},
    {0x3a19,0xf8,"",eReadWrite},
    {0x3c01,0x80,"",eReadWrite},
    {0x3b07,0x0c,"",eReadWrite},
    {0x3630,0x2e,"",eReadWrite},
    {0x3632,0xe2,"",eReadWrite},
    {0x3633,0x23,"",eReadWrite},
    {0x3634,0x44,"",eReadWrite},
    {0x3620,0x64,"",eReadWrite},
    {0x3621,0xe0,"",eReadWrite},
    {0x3600,0x37,"",eReadWrite},
    {0x3704,0xa0,"",eReadWrite},
    {0x3703,0x5a,"",eReadWrite},
    {0x3715,0x78,"",eReadWrite},
    {0x3717,0x01,"",eReadWrite},
    {0x3731,0x02,"",eReadWrite},
    {0x370b,0x60,"",eReadWrite},
    {0x3705,0x1a,"",eReadWrite},
    {0x3f05,0x02,"",eReadWrite},
    {0x3f06,0x10,"",eReadWrite},
    {0x3f01,0x0a,"",eReadWrite},
    {0x3a0f,0x58,"",eReadWrite},
    {0x3a10,0x50,"",eReadWrite},
    {0x3a1b,0x58,"",eReadWrite},
    {0x3a1e,0x50,"",eReadWrite},
    {0x3a11,0x60,"",eReadWrite},
    {0x3a1f,0x28,"",eReadWrite},
    {0x4001,0x02,"",eReadWrite},
    {0x4000,0x09,"",eReadWrite},
    {0x3503,0x03,"",eReadWrite},//manual,0xAE
    {0x3501,0x6f,"",eReadWrite},//
    {0x3502,0x00,"",eReadWrite},//
    {0x350a,0x00,"",eReadWrite},//
    {0x350b,0x6f,"",eReadWrite},//
    {0x5001,0x01,"",eReadWrite},//manual,0xAWB
    {0x5180,0x08,"",eReadWrite},//
    {0x5186,0x06,"",eReadWrite},//
    {0x5187,0x00,"",eReadWrite},//
    {0x5188,0x04,"",eReadWrite},//
    {0x5189,0x00,"",eReadWrite},//
    {0x518a,0x04,"",eReadWrite},//
    {0x518b,0x00,"",eReadWrite},//
    {0x5000,0x00,"",eReadWrite},//lenc WBC on  
    {0x3011,0x62,"",eReadWrite},	
    {0x0100,0x00,"",eReadWrite},// software standby
    {0x3035,0x11,"",eReadWrite},// PLL
    {0x3036,0x46,"",eReadWrite},// PLL
    {0x303c,0x11,"",eReadWrite},// PLL
    {0x3821,0x07,"",eReadWrite},// ISP mirror on, Sensor mirror on, H bin on
    {0x3820,0x41,"",eReadWrite},// ISP flip off, Sensor flip off, V bin on
    {0x3612,0x59,"",eReadWrite},// analog control
    {0x3618,0x00,"",eReadWrite},// analog control
    {0x380c,0x07,"",eReadWrite},// HTS = 1896
    {0x380d,0x68,"",eReadWrite},// HTS
    {0x380e,0x07,"",eReadWrite},// VTS = 1968
    {0x380f,0xb0,"",eReadWrite},// VTS
    {0x3814,0x31,"",eReadWrite},// X INC
    {0x3815,0x31,"",eReadWrite},// X INC
    {0x3708,0x64,"",eReadWrite},// analog control
    {0x3709,0x52,"",eReadWrite},// analog control
    {0x3808,0x05,"",eReadWrite},// DVPHO = 1296
    {0x3809,0x10,"",eReadWrite},// DVPHO
    {0x380a,0x03,"",eReadWrite},// DVPVO = 972
    {0x380b,0xcc,"",eReadWrite},// DVPVO
    {0x3800,0x00,"",eReadWrite},// X Start
    {0x3801,0x08,"",eReadWrite},// X Start
    {0x3802,0x00,"",eReadWrite},// Y Start
    {0x3803,0x02,"",eReadWrite},// Y Start
    {0x3804,0x0a,"",eReadWrite},// X End
    {0x3805,0x37,"",eReadWrite},// X End
    {0x3806,0x07,"",eReadWrite},// Y End
    {0x3807,0xa1,"",eReadWrite},// Y End
    /* banding filter*/
    {0x3a08,0x01,"",eReadWrite},// B50
    {0x3a09,0x27,"",eReadWrite},// B50
    {0x3a0a,0x00,"",eReadWrite},// B60
    {0x3a0b,0xf6,"",eReadWrite},// B60
    {0x3a0d,0x04,"",eReadWrite},// B60 max
    {0x3a0e,0x03,"",eReadWrite},// B50 max
    {0x4004,0x02,"",eReadWrite},// black line number
    {0x4837,0x24,"",eReadWrite},// MIPI pclk period
    {0x0100,0x00,"",eReadWrite},// wake up from software standby
    {0,0,"",eTableEnd}
};

const IsiRegDescription_t OV5647_g_2592_1944[] =
{
    {0x370c,0x03,"",eReadWrite},
    {0x5000,0x06,"",eReadWrite},
    {0x5003,0x08,"",eReadWrite},
    {0x5a00,0x08,"",eReadWrite},
    {0x3000,0xff,"",eReadWrite},
    {0x3001,0xff,"",eReadWrite},
    {0x3002,0xff,"",eReadWrite},
    {0x301d,0xf0,"",eReadWrite},
    {0x3a18,0x00,"",eReadWrite},
    {0x3a19,0xf8,"",eReadWrite},
    {0x3c01,0x80,"",eReadWrite},
    {0x3b07,0x0c,"",eReadWrite},
    {0x3630,0x2e,"",eReadWrite},
    {0x3632,0xe2,"",eReadWrite},
    {0x3633,0x23,"",eReadWrite},
    {0x3634,0x44,"",eReadWrite},
    {0x3620,0x64,"",eReadWrite},
    {0x3621,0xe0,"",eReadWrite},
    {0x3600,0x37,"",eReadWrite},
    {0x3704,0xa0,"",eReadWrite},
    {0x3703,0x5a,"",eReadWrite},
    {0x3715,0x78,"",eReadWrite},
    {0x3717,0x01,"",eReadWrite},
    {0x3731,0x02,"",eReadWrite},
    {0x370b,0x60,"",eReadWrite},
    {0x3705,0x1a,"",eReadWrite},
    {0x3f05,0x02,"",eReadWrite},
    {0x3f06,0x10,"",eReadWrite},
    {0x3f01,0x0a,"",eReadWrite},
    {0x3a0f,0x58,"",eReadWrite},
    {0x3a10,0x50,"",eReadWrite},
    {0x3a1b,0x58,"",eReadWrite},
    {0x3a1e,0x50,"",eReadWrite},
    {0x3a11,0x60,"",eReadWrite},
    {0x3a1f,0x28,"",eReadWrite},
    {0x4001,0x02,"",eReadWrite},
    {0x4000,0x09,"",eReadWrite},
    {0x3503,0x03,"",eReadWrite},//manual,0xAE
    {0x3501,0x6f,"",eReadWrite},//
    {0x3502,0x00,"",eReadWrite},//
    {0x350a,0x00,"",eReadWrite},//
    {0x350b,0x6f,"",eReadWrite},//
    {0x5001,0x01,"",eReadWrite},//manual,0xAWB
    {0x5180,0x08,"",eReadWrite},//
    {0x5186,0x06,"",eReadWrite},//
    {0x5187,0x00,"",eReadWrite},//
    {0x5188,0x04,"",eReadWrite},//
    {0x5189,0x00,"",eReadWrite},//
    {0x518a,0x04,"",eReadWrite},//
    {0x518b,0x00,"",eReadWrite},//
    {0x5000,0x00,"",eReadWrite},//lenc WBC on  
    {0x3011,0x62,"",eReadWrite},
	{0x3035,0x11,"",eReadWrite},
	{0x3036,0x64,"",eReadWrite},
	{0x303c,0x11,"",eReadWrite},
	{0x3821,0x06,"",eReadWrite},
	{0x3820,0x00,"",eReadWrite},
	{0x3612,0x5b,"",eReadWrite},
	{0x3618,0x04,"",eReadWrite},
	{0x380c,0x0a,"",eReadWrite},
	{0x380d,0x8c,"",eReadWrite},
	{0x380e,0x07,"",eReadWrite},
	{0x380f,0xb6,"",eReadWrite},
	{0x3814,0x11,"",eReadWrite},
	{0x3815,0x11,"",eReadWrite},
	{0x3708,0x64,"",eReadWrite},
	{0x3709,0x12,"",eReadWrite},
	{0x3808,0x0a,"",eReadWrite},
	{0x3809,0x20,"",eReadWrite},
	{0x380a,0x07,"",eReadWrite},
	{0x380b,0x98,"",eReadWrite},
	{0x3800,0x00,"",eReadWrite},
	{0x3801,0x0c,"",eReadWrite},
	{0x3802,0x00,"",eReadWrite},
	{0x3803,0x04,"",eReadWrite},
	{0x3804,0x0a,"",eReadWrite},
	{0x3805,0x33,"",eReadWrite},
	{0x3806,0x07,"",eReadWrite},
	{0x3807,0xa3,"",eReadWrite},
	{0x3a08,0x01,"",eReadWrite},
	{0x3a09,0x28,"",eReadWrite},
	{0x3a0a,0x00,"",eReadWrite},
	{0x3a0b,0xf6,"",eReadWrite},
	{0x3a0d,0x08,"",eReadWrite},
	{0x3a0e,0x06,"",eReadWrite},
	{0x4004,0x04,"",eReadWrite},
	{0x4837,0x19,"",eReadWrite},
	{0x0100,0x00,"",eReadWrite},
    {0,0,"",eTableEnd}
};

/*****************************************************************************
 * AWB-Calibration data
 *****************************************************************************/

// Calibration (e.g. Matlab) is done in double precision. Parameters are then stored and handle as float
// types here in the software. Finally these parameters are written to hardware registers with fixed
// point precision.
// Some thoughts about the error between a real value, rounded to a constant with a finite number of
// fractional digits, and the resulting binary fixed point value:
// The total absolute error is the absolute error of the conversion real to constant plus the absolute
// error of the conversion from the constant to fixed point.
// For example the distance between two figures of a a fixed point value with 8 bit fractional part
// is 1/256. The max. absolute error is half of that, thus 1/512. So 3 digits fractional part could
// be chosen for the constant with an absolut error of 1/2000. The total absolute error would then be
// 1/2000 + 1/512.
// To avoid any problems we take one more digit. And another one to account for error propagation in
// the calculations of the SLS algorithms. Finally we end up with reasonable 5 digits fractional part.

/*****************************************************************************
 *
 *****************************************************************************/

// K-Factor
// calibration factor to map exposure of current sensor to the exposure of the
// reference sensor
//
// Important: This value is determinde for OV5630_1_CLKIN = 10000000 MHz and
//            need to be changed for other values
const Isi1x1FloatMatrix_t OV5647_KFactor =
{
    { 6.838349f }   // or 3.94f (to be checked)
};


// PCA matrix
const Isi3x2FloatMatrix_t OV5647_PCAMatrix =
{
    {
        -0.62791f, -0.13803f,  0.76595f,
        -0.52191f,  0.80474f, -0.28283f
    }
};


// mean values from SVD
const Isi3x1FloatMatrix_t OV5647_SVDMeanValue =
{
    {
        0.34165f,  0.37876f,  0.27959f
    }
};



/*****************************************************************************
 * Rg/Bg color space (clipping and out of range)
 *****************************************************************************/
// Center line of polygons {f_N0_Rg, f_N0_Bg, f_d}
const IsiLine_t OV5647_CenterLine =
{
//    .f_N0_Rg    = -0.6611259877402997f,
//    .f_N0_Bg    = -0.7502749018422601f,
//    .f_d        = -2.0771246154391578f
    .f_N0_Rg    = -0.6965157268655040f,
    .f_N0_Bg    = -0.7175415264840207f,
    .f_d        = -2.0547830071543265f
};



/* parameter arrays for Rg/Bg color space clipping */
#define AWB_CLIP_PARM_ARRAY_SIZE_1 16
#define AWB_CLIP_PARM_ARRAY_SIZE_2 16

// bottom left (clipping area)
float afRg1[AWB_CLIP_PARM_ARRAY_SIZE_1]      = { 0.68560f, 0.82556f,   0.89599f,   0.96987f,   1.03512f,   1.11087f,   1.18302f,   1.25407f,   1.32402f,   1.39534f,   1.46604f,   1.54006f,   1.61483f,   1.68898f,   1.76107f,   1.83316f};
float afMaxDist1[AWB_CLIP_PARM_ARRAY_SIZE_1] = { -0.06477f,    -0.02014f,   0.00013f,   0.01861f,   0.03144f,   0.04451f,   0.05378f,   0.05876f,   0.05946f,   0.05541f,   0.04642f,   0.03005f,   0.00400f,  -0.01372f,  -0.05657f,  -0.10927f};

// top right (clipping area)
float afRg2[AWB_CLIP_PARM_ARRAY_SIZE_2]      = { 0.68884f, 0.82280f,   0.90309f,   0.91978f,   0.93757f,   1.02093f,   1.18619f,   1.25186f,   1.32429f,   1.39057f,   1.46432f,   1.54130f,   1.61242f,   1.68898f,   1.76107f,   1.83316f};
float afMaxDist2[AWB_CLIP_PARM_ARRAY_SIZE_2] = { 0.07493f,  0.03084f,   0.26486f,   0.31978f,   0.37305f,   0.27866f,  -0.00038f,  -0.05020f,  -0.05042f,  -0.04452f,  -0.03720f,  -0.01971f,   0.01295f,   0.02372f,   0.06657f,   0.11927f};

#if 0
// bottom left (clipping area)
float afRg1[AWB_CLIP_PARM_ARRAY_SIZE_1]         =
{
    0.68560f, 0.82556f, 0.89599f, 0.96987f,
    1.03512f, 1.11087f, 1.18302f, 1.25407f,
    1.32402f, 1.39534f, 1.46604f, 1.54006f,
    1.61483f, 1.68898f, 1.76107f, 1.83316f
};

float afMaxDist1[AWB_CLIP_PARM_ARRAY_SIZE_1]    =
{
    -0.06477f, -0.02014f,  0.00013f,  0.01861f,
     0.03144f,  0.04451f,  0.05378f,  0.05876f,
     0.05946f,  0.05541f,  0.04642f,  0.03005f,
     0.00400f, -0.01372f, -0.05657f, -0.10927f
};

// top right (clipping area)
float afRg2[AWB_CLIP_PARM_ARRAY_SIZE_2]         =
{
    0.68884f, 0.82280f, 0.90309f, 0.91978f,
    0.93757f, 1.02093f, 1.18619f, 1.25186f,
    1.32429f, 1.39057f, 1.46432f, 1.54130f,
    1.61242f, 1.68898f, 1.76107f, 1.83316f
};

float afMaxDist2[AWB_CLIP_PARM_ARRAY_SIZE_2]    =
{
     0.07493f,  0.03084f,  0.26486f,  0.31978f,
     0.37305f,  0.27866f, -0.00038f, -0.05020f,
    -0.05042f, -0.04452f, -0.03720f, -0.01971f,
     0.01295f,  0.02372f,  0.06657f,  0.11927f
};
#endif

// structure holding pointers to above arrays
// and their sizes
const IsiAwbClipParm_t OV5647_AwbClipParm =
{
    .pRg1       = &afRg1[0],
    .pMaxDist1  = &afMaxDist1[0],
    .ArraySize1 = AWB_CLIP_PARM_ARRAY_SIZE_1,
    .pRg2       = &afRg2[0],
    .pMaxDist2  = &afMaxDist2[0],
    .ArraySize2 = AWB_CLIP_PARM_ARRAY_SIZE_2
};



/* parameter arrays for AWB out of range handling */
#define AWB_GLOBAL_FADE1_ARRAY_SIZE 16
#define AWB_GLOBAL_FADE2_ARRAY_SIZE 16

float afGlobalFade1[AWB_GLOBAL_FADE1_ARRAY_SIZE]         = { 0.50566f,  0.82658f,   0.90134f,   0.97609f,   1.05085f,   1.12560f,   1.20036f,   1.27512f,   1.34987f,   1.42463f,   1.49938f,   1.57414f,   1.64889f,   1.72365f,   1.79840f,   1.87316f};
float afGlobalGainDistance1[AWB_GLOBAL_FADE1_ARRAY_SIZE] = { -0.07183f,  0.06684f,   0.09091f,   0.11286f,   0.13161f,   0.14731f,   0.15911f,   0.16626f,   0.16816f,   0.16378f,   0.15189f,   0.13131f,   0.10173f,   0.06157f,   0.01270f,  -0.04571f};

float afGlobalFade2[AWB_GLOBAL_FADE2_ARRAY_SIZE]         = { 0.51621f,  0.73291f,   0.83798f,   0.88724f,   0.96854f,   1.07438f,   1.20953f,   1.26501f,   1.33392f,   1.40777f,   1.47237f,   1.55365f,   1.64889f,   1.72365f,   1.79840f,   1.87316f};
float afGlobalGainDistance2[AWB_GLOBAL_FADE2_ARRAY_SIZE] = { 0.39624f,  0.32108f,   0.37367f,   0.45523f,   0.46804f,   0.34652f,   0.17707f,   0.12084f,   0.09216f,   0.09031f,   0.09848f,   0.10563f,   0.09827f,   0.13843f,   0.18730f,   0.24571f};

#if 0
//bottom left
float afGlobalFade1[AWB_GLOBAL_FADE1_ARRAY_SIZE]         =
{
     0.50566f,  0.82658f,  0.90134f,  0.97609f,
     1.05085f,  1.12560f,  1.20036f,  1.27512f,
     1.34987f,  1.42463f,  1.49938f,  1.57414f,
     1.64889f,  1.72365f,  1.79840f,  1.87316f
};

float afGlobalGainDistance1[AWB_GLOBAL_FADE1_ARRAY_SIZE] =
{
    -0.07183f,  0.06684f,  0.09091f,  0.11286f,
     0.13161f,  0.14731f,  0.15911f,  0.16626f,
     0.16816f,  0.16378f,  0.15189f,  0.13131f,
     0.10173f,  0.06157f,  0.01270f, -0.04571f
};

//top right
float afGlobalFade2[AWB_GLOBAL_FADE2_ARRAY_SIZE]         =
{
     0.51621f,  0.73291f,  0.83798f,  0.88724f,
     0.96854f,  1.07438f,  1.20489f,  1.27083f,
     1.33644f,  1.40777f,  1.47237f,  1.55365f,
     1.64889f,  1.72365f,  1.79840f,  1.87316f
};

float afGlobalGainDistance2[AWB_GLOBAL_FADE2_ARRAY_SIZE] =
{
     0.39624f,  0.32108f,  0.37367f,  0.45523f,
     0.46804f,  0.34652f,  0.11493f,  0.07960f,
     0.07718f,  0.09031f,  0.09848f,  0.10563f,
     0.09827f,  0.13843f,  0.18730f,  0.24571f
};
#endif

// structure holding pointers to above arrays and their sizes
const IsiAwbGlobalFadeParm_t OV5647_AwbGlobalFadeParm =
{
    .pGlobalFade1           = &afGlobalFade1[0],
    .pGlobalGainDistance1   = &afGlobalGainDistance1[0],
    .ArraySize1             = AWB_GLOBAL_FADE1_ARRAY_SIZE,
    .pGlobalFade2           = &afGlobalFade2[0],
    .pGlobalGainDistance2   = &afGlobalGainDistance2[0],
    .ArraySize2             = AWB_GLOBAL_FADE2_ARRAY_SIZE
};


/*****************************************************************************
 * Near white pixel discrimination
 *****************************************************************************/
// parameter arrays for near white pixel parameter calculations
#define AWB_FADE2_ARRAY_SIZE 6

float afFade2[AWB_FADE2_ARRAY_SIZE]   =    {0.50, 0.75, 0.85, 1.35, 1.4, 1.5};

float afCbMinRegionMax[AWB_FADE2_ARRAY_SIZE]   = { 114, 114, 110, 85, 80,  80 };
float afCrMinRegionMax[AWB_FADE2_ARRAY_SIZE]   = {  89, 89, 90, 110, 122, 122 };
float afMaxCSumRegionMax[AWB_FADE2_ARRAY_SIZE] = {  25, 25, 30, 30, 30, 30 };

float afCbMinRegionMin[AWB_FADE2_ARRAY_SIZE]   = { 125, 125, 125, 125, 125, 120 };
float afCrMinRegionMin[AWB_FADE2_ARRAY_SIZE]   = { 125, 125, 125, 125, 125, 126 };
float afMaxCSumRegionMin[AWB_FADE2_ARRAY_SIZE] = {   5.000f,   5.000f,   5.0000f,   5.000f,   5.000f,   5.000f };

#if 0
float afFade2[AWB_FADE2_ARRAY_SIZE] =
{
    0.50000f,   1.12500f,   1.481500f,
    1.63100f,   1.74500f,   1.900000f
};

float afCbMinRegionMax[AWB_FADE2_ARRAY_SIZE] =
{
    114.000f,   114.000f,   110.000f,
    110.000f,    95.000f,    90.000f
};

float afCrMinRegionMax[AWB_FADE2_ARRAY_SIZE] =
{
     90.000f,    90.000f,   110.000f,
    110.000f,   122.000f,   128.000f
};

float afMaxCSumRegionMax[AWB_FADE2_ARRAY_SIZE] =
{
     19.000f,    19.000f,    18.000f,
     16.000f,     9.000f,     9.000f
};

float afCbMinRegionMin[AWB_FADE2_ARRAY_SIZE] =
{
    125.000f,   125.000f,   125.000f,
    125.000f,   125.000f,   120.000f
};

float afCrMinRegionMin[AWB_FADE2_ARRAY_SIZE] =
{
    125.000f,   125.000f,   125.000f,
    125.000f,   125.000f,   126.000f
};

float afMaxCSumRegionMin[AWB_FADE2_ARRAY_SIZE] =
{
      5.000f,     5.000f,     5.000f,
      5.000f,     5.000f,     5.000f
};
#endif


// structure holding pointers to above arrays and their sizes
const IsiAwbFade2Parm_t OV5647_AwbFade2Parm =
{
    .pFade              = &afFade2[0],
    .pCbMinRegionMax    = &afCbMinRegionMax[0],
    .pCrMinRegionMax    = &afCrMinRegionMax[0],
    .pMaxCSumRegionMax  = &afMaxCSumRegionMax[0],
    .pCbMinRegionMin    = &afCbMinRegionMin[0],
    .pCrMinRegionMin    = &afCrMinRegionMin[0],
    .pMaxCSumRegionMin  = &afMaxCSumRegionMin[0],
    .ArraySize          = AWB_FADE2_ARRAY_SIZE
};


