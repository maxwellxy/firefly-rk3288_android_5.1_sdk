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
 * @defgroup ov5630_D75   Illumination Profile D65
 * @{
 *
 */
#ifndef __OV14825_D75_H__
#define __OV14825_D75_H__

#ifdef __cplusplus
extern "C"
{
#endif


#define AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75  2
#define AWB_LSCMATRIX_ARRAY_SIZE_CIE_D75    1

#define AWB_SATURATION_ARRAY_SIZE_CIE_D75   4
#define AWB_VIGNETTING_ARRAY_SIZE_CIE_D75   2


/*****************************************************************************/
/*!
 * CIE D75:
 *  overcast daylight, 7500K
 * This illumination is not tuned for this sensor correctly! This color profile
 * might not yield satisfying results.
 */
/*****************************************************************************/

// crosstalk matrix
const Isi3x3FloatMatrix_t  OV14825_XTalkCoeff_D75 =
{
    {
        1.69160f,  -0.42523f,  -0.26637f, 
       -0.23301f,   1.80211f,  -0.56910f, 
       -0.05856f,  -1.06792f,   2.12648f  
    }
};

// crosstalk offset matrix
const IsiXTalkFloatOffset_t OV14825_XTalkOffset_D75 =
{
    .fCtOffsetRed      = (-132.6875f / CC_OFFSET_SCALING),
    .fCtOffsetGreen    = (-133.1250f / CC_OFFSET_SCALING),
    .fCtOffsetBlue     = (-146.6875f / CC_OFFSET_SCALING)
};

// gain matrix
const IsiComponentGain_t OV14825_CompGain_D75 =
{
    .fRed      = 1.46495f,
    .fGreenR   = 1.00000f,
    .fGreenB   = 1.00000f,
    .fBlue     = 1.45640f
};

// mean value of gaussian mixture model
const Isi2x1FloatMatrix_t OV14825_GaussMeanValue_D75 =
{
    {
        0.03355f,  0.06111f
    }
};

// inverse covariance matrix
const Isi2x2FloatMatrix_t OV14825_CovarianceMatrix_D75 =
{
    {
        505.02062f,  -83.78852f, 
        -83.78852f,  2113.43624f 
    }
};

// factor in gaussian mixture model
const IsiGaussFactor_t OV14825_GaussFactor_D75 =
{
    .fGaussFactor = 163.88388f
};

// thresholds for switching between MAP classification and interpolation
const Isi2x1FloatMatrix_t OV14825_Threshold_D75 =
{
    {
        1.00000f,  1.00000f
    }
};

// saturation curve
float afSaturationSensorGain_D75[AWB_SATURATION_ARRAY_SIZE_CIE_D75] =
{
    1.0f, 2.0f, 4.0f, 8.0f
};

float afSaturation_D75[AWB_SATURATION_ARRAY_SIZE_CIE_D75] =
{
    100.0f, 100.0f, 90.0f, 74.0f
};

const IsiSaturationCurve_t OV14825_SaturationCurve_D75 =
{
    .ArraySize      = AWB_SATURATION_ARRAY_SIZE_CIE_D75,
    .pSensorGain    = &afSaturationSensorGain_D75[0],
    .pSaturation    = &afSaturation_D75[0]
};

// saturation depended color conversion matrices
IsiSatCcMatrix_t OV14825_SatCcMatrix_D75[AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75] =
{
    {
        .fSaturation    = 74.0f,
        .XTalkCoeff     =
        {
            {
                1.51023f,  -0.31492f,  -0.19531f, 
                0.13488f,   1.27669f,  -0.41157f, 
                0.25817f,  -0.79601f,   1.53784f  
            }
        }
    },
    {
        .fSaturation    = 100.0f,
        .XTalkCoeff     =
        {
            {
                1.69160f,  -0.42523f,  -0.26637f, 
               -0.23301f,   1.80211f,  -0.56910f, 
               -0.05856f,  -1.06792f,   2.12648f  
            }
        }
    }
 };

const IsiCcMatrixTable_t OV14825_CcMatrixTable_D75 =
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75,
    .pIsiSatCcMatrix    = &OV14825_SatCcMatrix_D75[0]
};

// saturation depended color conversion offset vectors
IsiSatCcOffset_t OV14825_SatCcOffset_D75[AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75] =
{
    {
        .fSaturation    = 74.0f,
        .CcOffset       =
        {
            .fCtOffsetRed   = 0.0f,
            .fCtOffsetGreen = 0.0f,
            .fCtOffsetBlue  = 0.0f
        }
    },
    {
        .fSaturation    = 100.0f,
        .CcOffset       =
        {
            .fCtOffsetRed      = (-132.6875f / CC_OFFSET_SCALING),
            .fCtOffsetGreen    = (-133.1250f / CC_OFFSET_SCALING),
            .fCtOffsetBlue     = (-146.6875f / CC_OFFSET_SCALING)
        }
    }
};

const IsiCcOffsetTable_t OV14825_CcOffsetTable_D75=
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75,
    .pIsiSatCcOffset    = &OV14825_SatCcOffset_D75[0]
};

// vignetting curve
float afVignettingSensorGain_D75[AWB_VIGNETTING_ARRAY_SIZE_CIE_D75] =
{
    1.0f, 8.0f
};

float afVignetting_D75[AWB_VIGNETTING_ARRAY_SIZE_CIE_D75] =
{
    100.0f, 100.0f
};

const IsiVignettingCurve_t OV14825_VignettingCurve_D75 =
{
    .ArraySize      = AWB_VIGNETTING_ARRAY_SIZE_CIE_D75,
    .pSensorGain    = &afVignettingSensorGain_D75[0],
    .pVignetting    = &afVignetting_D75[0]
};



#ifdef __cplusplus
}
#endif

/* @} ov5630_D75 */

#endif /* __OV14825_D75_H__ */

