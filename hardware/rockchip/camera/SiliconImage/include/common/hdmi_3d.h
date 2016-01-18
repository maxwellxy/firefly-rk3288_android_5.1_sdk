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
 * @file    hdmi_3d.h
 *
 * @brief   Defines HDMI 3D style video format stuff.
 *
 *****************************************************************************/
#ifndef __HDMI_3D_H__
#define __HDMI_3D_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cea_861.h>

#if defined (__cplusplus)
extern "C" {
#endif

typedef enum Hdmi3DVideoFormat_e
{
    HDMI_3D_VIDEOFORMAT_FRAME_PACKING       = 0x00, //!< 0000 Frame packing
    HDMI_3D_VIDEOFORMAT_FIELD_ALTERNATIVE   = 0x01, //!< 0001 Field alternative
    HDMI_3D_VIDEOFORMAT_LINE_ALTERNATIVE    = 0x02, //!< 0010 Line alternative
    HDMI_3D_VIDEOFORMAT_SIDE_BY_SIDE_FULL   = 0x03, //!< 0011 Side-by-Side (Full)
    HDMI_3D_VIDEOFORMAT_L_DEPTH             = 0x04, //!< 0100 L + depth
    HDMI_3D_VIDEOFORMAT_L_DEPTH_GRFX_DEPTH  = 0x05, //!< 0101 L + depth + graphics + graphics-depth
    HDMI_3D_VIDEOFORMAT_TOP_AND_BOTTOM      = 0x06, //!< 0110 Top-and-Bottom
                                                    //   0111 Reserved for future use.
    HDMI_3D_VIDEOFORMAT_SIDE_BY_SIDE_HALF   = 0x08, //!< 1000 Side-by-Side (Half) (See Table H-3)
                                                    //   1001 ~ 1110 Reserved for future use.
    HDMI_3D_VIDEOFORMAT_INVALID             = 0x0f  //!< 1111 Not in use.
} Hdmi3DVideoFormat_t; //!< @note The names and numbers intentionally match the HDMI Specification; Version 1.4a; March 4, 2010; chapter "Extraction of 3D Signaling Portion".

typedef struct Hdmi3DVideoFormatDetails_s
{
    Hdmi3DVideoFormat_t FormatID;       //!< HDMI 3D format ID.
    char                *szName;        //!< Format description.
    struct Progressive_s
    {
        bool_t              Supported;  //!< Can be used with progressive video.
        uint16_t            SubImages;  //!< Number of sub images expected per 3D image.
    } Progressive;
    struct Interlaced_s
    {
        bool_t              Supported;  //!< Can be used with progressive video.
        uint16_t            SubImages;  //!< Number of sub images expected per 3D image.
    } Interlaced;
} Hdmi3DVideoFormatDetails_t;

#define Hdmi3DHasExtData(_format) ((_format&0x8)!=0)

extern const Hdmi3DVideoFormatDetails_t* Hdmi3DGet3DFormatDetails
(
    Hdmi3DVideoFormat_t FormatID                                //!< ID of 3D format to get details for.
);

extern uint16_t Hdmi3DGetNumSubImages
(
    const Hdmi3DVideoFormat_t        FormatID,                  //!< ID of 3D format to use.
    const Cea861VideoFormatDetails_t *pBaseCeaFormatDetails     //!< Reference to base CEA 861 timing details.
);

extern RESULT Hdmi3DGetVideoFormatDetails
(
    const Hdmi3DVideoFormat_t        FormatID,                  //!< ID of 3D format to use.
    const uint16_t                   SubImageNum,               //!< Number of sub image for which to calc timing details; 0 = first image.
    const Cea861VideoFormatDetails_t *pBaseCeaFormatDetails,    //!< Reference to base CEA 861 timing details.
    Cea861VideoFormatDetails_t       *pSubCeaFormatDetails      //!< Reference to storage space for calculated sub image timing details.
);

// NOTE: The value must match the highest SubImages number in Hdmi3DVideoFormats table in implementation.
#define HDMI_3D_MAX_NUM_SUBIMAGES 4 //!< Max number of sub images required for supported 3D formats.

#if defined (__cplusplus)
}
#endif

#endif /* __HDMI_3D_H__*/
