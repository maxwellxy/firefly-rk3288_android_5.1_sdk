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
 * @file ibd_common.h
 *
 * @brief
 *   Common stuff used by ibd API & implementation.
 *
 *****************************************************************************/
/**
 * @page ibd_page IBD Ctrl
 * The In-Buffer Display module allows to perform some simple graphics stuff on image buffers.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref ibd_api
 * - @ref ibd_common
 * - @ref ibd
 *
 * @defgroup ibd_common IBD Common
 * @{
 *
 */

#ifndef __IBD_COMMON_H__
#define __IBD_COMMON_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief   handle to ibd instance
 */
typedef struct ibdContext_s *ibdHandle_t;

/**
 * @brief   IDs of supported commands.
 */
typedef enum ibdCmdId_e
{
    IBD_NOOP = 0,   //!< No-op.
    IBD_DRAW_PIXEL, //!< Single pixel.
    IBD_DRAW_LINE,  //!< Straight line.
    IBD_DRAW_BOX,   //!< Empty rectangle build with 4 lines.
    IBD_DRAW_RECT,  //!< Filled box.
    IBD_DRAW_TEXT   //!< Single line of text.
} ibdCmdId_t;

/**
 * @brief   Color spec. union
 *
 *          Members overlap, so that writing seperated color components
 *          and reading back combined color components is possible.
 *
 * @note    No color space conversion between RGB and YUV is performed!
 */
typedef union ibdColor_u
{
    struct
    {
        uint8_t B;  //!< Blue  (0..255)
        uint8_t G;  //!< Green (0..255)
        uint8_t R;  //!< Red   (0..255)
        uint8_t A;  //!< Alpha (0..255 = transparent..opaque)
    }           compARGB;   //!< Split R G B A value

    uint32_t    ARGB;       //!< Combined RGBA value.

    struct
    {
        uint8_t Cr; //!< Cr  (0..255)
        uint8_t Cb; //!< Cb (0..255)
        uint8_t Y;  //!< Y   (0..255)
        uint8_t A;  //!< Alpha (0..255 = transparent..opaque)
    }           compAYCbCr; //!< Split Y Cb Cr A value

    uint32_t    AYCbCr;     //!< Combined AYCbCr value.
} ibdColor_t;

/**
 * @brief   IDs of supported fonts.
 *
 * @note    The exact fonts used depend on the underlying graphics implementation.
 */
typedef enum ibdFontId_e
{
    IBD_FONT_PROP_XXSMALL,  //!< Proportional width font, extra extra small.
    IBD_FONT_PROP_XSMALL,   //!< Proportional width font, extra small.
    IBD_FONT_PROP_SMALL,    //!< Proportional width font, small.
    IBD_FONT_PROP_MEDIUM,   //!< Proportional width font, medium.
    IBD_FONT_PROP_LARGE,    //!< Proportional width font, large.
    IBD_FONT_PROP_XLARGE,   //!< Proportional width font, extra large.
} ibdFontId_t;

/**
 * @brief   The parameter structure type used for @ref IBD_DRAW_PIXEL cmd.
 */
typedef struct ibdPixelParam_s
{
    int32_t     x;      //!< Point x.
    int32_t     y;      //!< Point y.
    ibdColor_t  color;  //!< Point color.
} ibdPixelParam_t;

/**
 * @brief   The parameter structure type used for @ref IBD_DRAW_LINE cmd.
 */
typedef struct ibdLineParam_s
{
    int32_t     x;      //!< Starting point left.
    int32_t     y;      //!< Starting point top.
    int32_t     x2;     //!< Ending point right.
    int32_t     y2;     //!< Ending point bottom.
    ibdColor_t  color;  //!< Line color.
} ibdLineParam_t;

/**
 * @brief   The parameter structure type used for @ref IBD_DRAW_BOX cmd.
 */
typedef struct ibdBoxParam_s
{
    int32_t     x;      //!< Bounding box left.
    int32_t     y;      //!< Bounding box top.
    int32_t     x2;     //!< Bounding box right.
    int32_t     y2;     //!< Bounding box bottom.
    ibdColor_t  color;  //!< Box color.
} ibdBoxParam_t;

/**
 * @brief   The parameter structure type used for @ref IBD_DRAW_RECT cmd.
 */
typedef struct ibdRectParam_s
{
    int32_t     x;      //!< Bounding box left.
    int32_t     y;      //!< Bounding box top.
    int32_t     x2;     //!< Bounding box right.
    int32_t     y2;     //!< Bounding box bottom.
    ibdColor_t  color;  //!< Rect color.
} ibdRectParam_t;

/**
 * @brief   The parameter structure type used for @ref IBD_DRAW_TEXT cmd.
 */
typedef struct ibdTextParam_s
{
    int32_t     x;      //!< Bounding box left.
    int32_t     y;      //!< Bounding box top.
    int32_t     x2;     //!< Bounding box right.
    int32_t     y2;     //!< Bounding box bottom.
    ibdColor_t  color;  //!< Text color.
    ibdColor_t  colorB; //!< Background color.
    char        *pcText;//!< The text to be drawn.
    int32_t     len;    //!< The length (in chars) of the text to be draw.
    ibdFontId_t fontID; //!< The font to use for drawing.
} ibdTextParam_t;

/**
 * @brief   The common data type used for commands (@ref ibdCmdId_t).
 */
typedef struct ibdCmd_s
{
    ibdCmdId_t cmdId;   //!< ID of command.

    union param_u
    {
        ibdPixelParam_t pixel;
        ibdLineParam_t  line;
        ibdBoxParam_t   box;
        ibdRectParam_t  rect;
        ibdTextParam_t  text;
    } params;           //!< Params union for commands.
} ibdCmd_t;

/* @} ibd_common */

#ifdef __cplusplus
}
#endif

#endif /* __IBD_COMMON_H__ */
