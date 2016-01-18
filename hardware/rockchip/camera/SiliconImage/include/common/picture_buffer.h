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
 * @file    picture_buffer.h
 *
 * @brief   Defines picture buffer meta data structure including its components
 *          and helper functions around that structure.
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup picture_buffer Picture buffer descriptor
 * @{
 *
 */
#ifndef __PICTURE_BUFFER_H__
#define __PICTURE_BUFFER_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*****************************************************************************/
/**
 *          PicBufType_t
 *
 * @brief   The type of image data a picture buffer holds.
 *
 * @note    MVDU_FXQuad requires PIC_BUF_TYPE_YCbCr422 in PIC_BUF_LAYOUT_SEMIPLANAR mode.
 *
 *****************************************************************************/
typedef enum PicBufType_e
{
	PIC_BUF_TYPE_INVALID  = 0x00,
	PIC_BUF_TYPE_DATA     = 0x08,   // just some sequential data
	PIC_BUF_TYPE_RAW8     = 0x10,
	PIC_BUF_TYPE_RAW16    = 0x11,   // includes: 9..16bits, MSBit aligned, LSByte first!
	PIC_BUF_TYPE_JPEG     = 0x20,
	PIC_BUF_TYPE_YCbCr444 = 0x30,
	PIC_BUF_TYPE_YCbCr422 = 0x31,
	PIC_BUF_TYPE_YCbCr420 = 0x32,
	PIC_BUF_TYPE_YCbCr400 = 0x33,
	PIC_BUF_TYPE_YCbCr32  = 0x3f,
	//PIC_BUF_TYPE_YCbCr400 = 0x33, // "Black&White"
	PIC_BUF_TYPE_RGB888   = 0x40,
	PIC_BUF_TYPE_RGB666   = 0x41, // R, G & B are LSBit aligned!
	PIC_BUF_TYPE_RGB565   = 0x42, // TODO: don't know the memory layout right now, investigate!
	PIC_BUF_TYPE_RGB32    = 0x4f,
	PIC_BUF_TYPE_DPCC     = 0x50,
	_PIC_BUF_TYPE_DUMMY_
} PicBufType_t;

/*****************************************************************************/
/**
 *          PicBufLayout_t
 *
 * @brief   The layout of the image data a picture buffer holds.
 *
 * @note    MVDU_FXQuad requires PIC_BUF_TYPE_YCbCr422 in PIC_BUF_LAYOUT_SEMIPLANAR mode.
 *
 *****************************************************************************/
typedef enum PicBufLayout_e
{
	PIC_BUF_LAYOUT_INVALID         = 0,

	PIC_BUF_LAYOUT_COMBINED        = 0x10,  // PIC_BUF_TYPE_DATA:      Data: D0 D1 D2...
                                            // PIC_BUF_TYPE_RAW8:      Data: D0 D1 D2...
                                            // PIC_BUF_TYPE_RAW16:     Data: D0L D0H D1L D1H...
                                            // PIC_BUF_TYPE_JPEG:      Data: J0 J1 J2...
                                            // PIC_BUF_TYPE_YCbCr444:  Data: Y0 Cb0 Cr0 Y1 Cb1Cr1...
                                            // PIC_BUF_TYPE_YCbCr422:  Data: Y0 Cb0 Y1 Cr0 Y2 Cb1 Y3 Cr1...
                                            // PIC_BUF_TYPE_YCbCr32:   Data: Cr0 Cb0 Y0 A0 Cr1 Cb1 Y1 A1...
                                            // PIC_BUF_TYPE_RGB888:    Data: R0 G0 B0 R1 B2 G1...
                                            // PIC_BUF_TYPE_RGB666:    Data: {00,R0[5:0]} {00,G0[5:0]} {00,B0[5:0]} {00,R1[5:0]} {00,G2[5:0]} {00,B3[5:0]}...
                                            // PIC_BUF_TYPE_RGB565:    Data: {R0[4:0],G0[5:3]} {G0[2:0],B0[4:0]} {R1[4:0],G1[5:3]} {G1[2:0],B1[4:0]}... (is this correct?)
                                            // PIC_BUF_TYPE_RGB32:     Data: B0 G0 R0 A0 B1 G1 R1 A1...
    PIC_BUF_LAYOUT_BAYER_RGRGGBGB  = 0x11,  // 1st line: RGRG... , 2nd line GBGB... , etc.
    PIC_BUF_LAYOUT_BAYER_GRGRBGBG  = 0x12,  // 1st line: GRGR... , 2nd line BGBG... , etc.
    PIC_BUF_LAYOUT_BAYER_GBGBRGRG  = 0x13,  // 1st line: GBGB... , 2nd line RGRG... , etc.
    PIC_BUF_LAYOUT_BAYER_BGBGGRGR  = 0x14,  // 1st line: BGBG... , 2nd line GRGR... , etc.

	PIC_BUF_LAYOUT_SEMIPLANAR      = 0x20,  // PIC_BUF_TYPE_YCbCr422:  Luma:  Y0 Y1 Y2 Y3... ; Chroma: Cb0 Cr0 Cb1 Cr1...
                                            // PIC_BUF_TYPE_YCbCr420:  Luma:  Y0 Y1 Y2 Y3... ; Chroma: Cb0 Cr0 Cb1 Cr1...
                                            // PIC_BUF_TYPE_YCbCr400:  Luma:  Y0 Y1 Y2 Y3... ; Chroma: not used

    PIC_BUF_LAYOUT_PLANAR          = 0x30,  // PIC_BUF_TYPE_YCbCr444:  Y: Y0 Y1 Y2 Y3...;  Cb: Cb0 Cb1 Cb2 Cb3...; Cr: Cr0 Cr1 Cr2 Cr3...
	                                        // PIC_BUF_TYPE_YCbCr422:  Y: Y0 Y1 Y2 Y3...;  Cb: Cb0 Cb1 Cb2 Cb3...; Cr: Cr0 Cr1 Cr2 Cr3...
	                                        // PIC_BUF_TYPE_YCbCr420:  Y: Y0 Y1 Y2 Y3...;  Cb: Cb0 Cb1 Cb2 Cb3...; Cr: Cr0 Cr1 Cr2 Cr3...
                                            // PIC_BUF_TYPE_YCbCr400:  Y: Y0 Y1 Y2 Y3...;  Cb: not used;           Cr: not used...
                                            // PIC_BUF_TYPE_RGB888:    R: R0 R1 R2 R3...;  G:  G0 G1 G2 G3...;     B:  B0 B1 B2 B3...
                                            // PIC_BUF_TYPE_RGB666:    R: {00,R0[5:0]}...; G:  {00,G0[5:0]}...;    B:  {00,B0[5:0]}...
    _PIC_BUF_LAYOUT_DUMMY_
} PicBufLayout_t;

/*****************************************************************************/
/**
 *          PicBufPlane_t
 *
 * @brief   Common information about a color component plane within an image buffer.
 *
 *****************************************************************************/
typedef struct PicBufPlane_s
{
    uint8_t    *pBuffer;
    uint32_t    PicWidthPixel;
    uint32_t    PicWidthBytes;
    uint32_t    PicHeightPixel;
} PicBufPlane_t;

/*****************************************************************************/
/**
 *          PicBufMetaData_t
 *
 * @brief   All the meta data one needs to know about an image buffer.
 *
 *****************************************************************************/
typedef struct PicBufMetaData_s
{
    PicBufType_t            Type;       // type of picture data
    PicBufLayout_t          Layout;     // kind of data layout
    uint32_t                Align;      // min. alignment required for color component planes or sub buffer base adresses for this picture buffer
    int64_t                 TimeStampUs;// timestamp in us
    void                    *priv;      // private pointer
    struct PicBufMetaData_s *pNext3D;   // Reference to PicBufMetaData of the subsequent buffer in a 3D descriptor chain, valid only in 3D mode; set to NULL if last in chain or for 2D mode.
                                        // Note: depending on the 3D format in use, the primary buffer holds left image data while the secondary buffer holds right or depth information.
                                        //       Certain 3D formats require further buffers, in which case the 3D chain consists of more than two descriptors.
    union Data_u                        // the type and layout dependent meta data
    {
        struct data_s       // DATA
        {
            uint8_t    *pData;
            uint32_t    DataSize;
        } data;

        PicBufPlane_t raw;  // RAW8, RAW16

        struct jpeg_s       // JPEG
        {
            uint8_t    *pData;
            uint32_t    DataSize;
            uint32_t    PicWidthPixel;
            uint32_t    PicHeightPixel;
            uint8_t    *pHeader;
            uint32_t    HeaderSize;
        } jpeg;

        union YCbCr_u // YCbCr444, YCbCr422, YCbCr420, YCbCr32
        {
            PicBufPlane_t combined;
            struct semiplanar_s
            {
                PicBufPlane_t Y;
                PicBufPlane_t CbCr;
            } semiplanar;
            struct planar_YUV_s
            {
                PicBufPlane_t Y;
                PicBufPlane_t Cb;
                PicBufPlane_t Cr;
            } planar;
        } YCbCr;

        union RGB_u         // RGB888, RGB32
        {
            PicBufPlane_t combined;
            struct planar_RGB_s
            {
                PicBufPlane_t R;
                PicBufPlane_t G;
                PicBufPlane_t B;
            } planar;
        } RGB;
    } Data;
} PicBufMetaData_t;

/*****************************************************************************/
/**
 *          PicBufIsConfigValid()
 *
 * @brief   Check the given picture buffer meta data structure for valid
 *          type & layout combination.
 *
 * @param   Reference to picture buffer meta data structure.
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS     type & layout combination is valid
 * @retval  RET_NOTSUPP     type & layout combination is invalid
 * @retval  RET_OUTOFRANGE  invalid type and/or layout
 *
 *****************************************************************************/
extern RESULT PicBufIsConfigValid
(
    PicBufMetaData_t *pPicBufMetaData
);

/* @} picture_buffer */

#if defined (__cplusplus)
}
#endif

#endif /* __PICTURE_BUFFER_H__ */
