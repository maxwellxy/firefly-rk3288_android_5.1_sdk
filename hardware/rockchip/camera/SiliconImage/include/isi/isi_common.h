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
 * @file isi_common.h
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
#ifndef __ISI_COMMON_H__
#define __ISI_COMMON_H__



#ifdef __cplusplus
extern "C"
{
#endif

//oyyf add 
#define ISI_SENSOR_OUTPUT_MODE_RAW	0x00000001U
#define ISI_SENSOR_OUTPUT_MODE_YUV  0x00000002U
#define ISI_SENSOR_OUTPUT_MODE_RGB  0x00000004U

/*****************************************************************************/
/*!
 * interface version
 * =================
 * please increment the version if you add something new to the interface.
 * This helps upper layer software to deal with different interface versions.
 */
/*****************************************************************************/
#define ISI_INTERFACE_VERSION               6



/*****************************************************************************/
/* capabilities / configuration
 *****************************************************************************/

/**<  BusWidth */
#define ISI_BUSWIDTH_8BIT_ZZ                0x00000001U     /**< to expand to a (possibly higher) resolution in marvin, the LSBs will be set to zero */
#define ISI_BUSWIDTH_8BIT_EX                0x00000002U     /**< to expand to a (possibly higher) resolution in marvin, the LSBs will be copied from the MSBs */
#define ISI_BUSWIDTH_10BIT_EX               0x00000004U     /**< /formerly known as ISI_BUSWIDTH_10BIT (at times no marvin derivative was able to process more than 10 bit) */
#define ISI_BUSWIDTH_10BIT_ZZ               0x00000008U
#define ISI_BUSWIDTH_12BIT                  0x00000010U

#define ISI_BUSWIDTH_10BIT      ( ISI_BUSWIDTH_10BIT_EX )


/**< Mode, operating mode of the image sensor in terms of output data format and timing data transmission */
#define ISI_MODE_BT601                      0x00000001      /**< YUV-Data with separate h/v sync lines (ITU-R BT.601) */
#define ISI_MODE_BT656                      0x00000002      /**< YUV-Data with sync words inside the datastream (ITU-R BT.656) */
#define ISI_MODE_BAYER                      0x00000004      /**< Bayer data with separate h/v sync lines */
#define ISI_MODE_DATA                       0x00000008      /**< Any binary data without line/column-structure, (e.g. already JPEG encoded) h/v sync lines act as data valid signals */
#define ISI_MODE_PICT                       0x00000010      /**< RAW picture data with separate h/v sync lines */
#define ISI_MODE_RGB565                     0x00000020      /**< RGB565 data with separate h/v sync lines */
#define ISI_MODE_SMIA                       0x00000040      /**< SMIA conform data stream (see SmiaMode for details) */
#define ISI_MODE_MIPI                       0x00000080      /**< MIPI conform data stream (see MipiMode for details) */
#define ISI_MODE_BAY_BT656                  0x00000100      /**< Bayer data with sync words inside the datastream (similar to ITU-R BT.656) */
#define ISI_MODE_RAW_BT656                  0x00000200      /**< Raw picture data with sync words inside the datastream (similar to ITU-R BT.656) */

/**< SmiaMode */
#define ISI_SMIA_MODE_COMPRESSED            0x00000001      //!< compression mode
#define ISI_SMIA_MODE_RAW_8_TO_10_DECOMP    0x00000002      //!< 8bit to 10 bit decompression
#define ISI_SMIA_MODE_RAW_12                0x00000004      //!< 12 bit RAW Bayer Data
#define ISI_SMIA_MODE_RAW_10                0x00000008      //!< 10 bit RAW Bayer Data
#define ISI_SMIA_MODE_RAW_8                 0x00000010      //!< 8 bit RAW Bayer Data
#define ISI_SMIA_MODE_RAW_7                 0x00000020      //!< 7 bit RAW Bayer Data
#define ISI_SMIA_MODE_RAW_6                 0x00000040      //!< 6 bit RAW Bayer Data
#define ISI_SMIA_MODE_RGB_888               0x00000080      //!< RGB 888 Display ready Data
#define ISI_SMIA_MODE_RGB_565               0x00000100      //!< RGB 565 Display ready Data
#define ISI_SMIA_MODE_RGB_444               0x00000200      //!< RGB 444 Display ready Data
#define ISI_SMIA_MODE_YUV_420               0x00000400      //!< YUV420 Data
#define ISI_SMIA_MODE_YUV_422               0x00000800      //!< YUV422 Data
#define ISI_SMIA_OFF                        0x80000000      //!< SMIA is disabled

/**< MipiMode */
#define ISI_MIPI_MODE_YUV420_8              0x00000001      //!< YUV 420  8-bit
#define ISI_MIPI_MODE_YUV420_10             0x00000002      //!< YUV 420 10-bit
#define ISI_MIPI_MODE_LEGACY_YUV420_8       0x00000004      //!< Legacy YUV 420 8-bit
#define ISI_MIPI_MODE_YUV420_CSPS_8         0x00000008      //!< YUV 420 8-bit (CSPS)
#define ISI_MIPI_MODE_YUV420_CSPS_10        0x00000010      //!< YUV 420 10-bit (CSPS)
#define ISI_MIPI_MODE_YUV422_8              0x00000020      //!< YUV 422 8-bit
#define ISI_MIPI_MODE_YUV422_10             0x00000040      //!< YUV 422 10-bit
#define ISI_MIPI_MODE_RGB444                0x00000080      //!< RGB 444
#define ISI_MIPI_MODE_RGB555                0x00000100      //!< RGB 555
#define ISI_MIPI_MODE_RGB565                0x00000200      //!< RGB 565
#define ISI_MIPI_MODE_RGB666                0x00000400      //!< RGB 666
#define ISI_MIPI_MODE_RGB888                0x00000800      //!< RGB 888
#define ISI_MIPI_MODE_RAW_6                 0x00001000      //!< RAW_6
#define ISI_MIPI_MODE_RAW_7                 0x00002000      //!< RAW_7
#define ISI_MIPI_MODE_RAW_8                 0x00004000      //!< RAW_8
#define ISI_MIPI_MODE_RAW_10                0x00008000      //!< RAW_10
#define ISI_MIPI_MODE_RAW_12                0x00010000      //!< RAW_12
#define ISI_MIPI_OFF                        0x80000000      //!< MIPI is disabled

/**< FieldSelection */
#define ISI_FIELDSEL_BOTH                   0x00000001      /**< sample all field (don't care about fields */
#define ISI_FIELDSEL_EVEN                   0x00000002      /**< sample only even fields */
#define ISI_FIELDSEL_ODD                    0x00000004      /**< sample only odd fields */

/**< YCSeq */
#define ISI_YCSEQ_YCBYCR                    0x00000001
#define ISI_YCSEQ_YCRYCB                    0x00000002
#define ISI_YCSEQ_CBYCRY                    0x00000004
#define ISI_YCSEQ_CRYCBY                    0x00000008

/**< Conv422 */
#define ISI_CONV422_COSITED                 0x00000001
#define ISI_CONV422_INTER                   0x00000002
#define ISI_CONV422_NOCOSITED               0x00000004

/**< BayerPatttern */
#define ISI_BPAT_RGRGGBGB                   0x00000001
#define ISI_BPAT_GRGRBGBG                   0x00000002
#define ISI_BPAT_GBGBRGRG                   0x00000004
#define ISI_BPAT_BGBGGRGR                   0x00000008

/**< HPolarity */
#define ISI_HPOL_SYNCPOS                    0x00000001      /**< sync signal pulses high between lines */
#define ISI_HPOL_SYNCNEG                    0x00000002      /**< sync signal pulses low between lines */
#define ISI_HPOL_REFPOS                     0x00000004      /**< reference signal is high as long as sensor puts out line data */
#define ISI_HPOL_REFNEG                     0x00000008      /**< reference signal is low as long as sensor puts out line data */

/**< VPolarity */
#define ISI_VPOL_POS                        0x00000001
#define ISI_VPOL_NEG                        0x00000002

/**< Edge */
#define ISI_EDGE_RISING                     0x00000001
#define ISI_EDGE_FALLING                    0x00000002

/**< Bls (Black Level Substraction) */
#define ISI_BLS_OFF                         0x00000001      /**< turns on/off additional black lines at frame start */
#define ISI_BLS_TWO_LINES                   0x00000002
#define ISI_BLS_FOUR_LINES                  0x00000004      /**< two lines top and two lines bottom */
#define ISI_BLS_SIX_LINES                   0x00000008      /**< six lines top */

/**< Gamma */
#define ISI_GAMMA_ON                        0x00000001      /**< turns on/off gamma correction in the sensor ISP */
#define ISI_GAMMA_OFF                       0x00000002

/**< ColorConv */
#define ISI_CCONV_ON                        0x00000001      /**< turns on/off color conversion matrix in the sensor ISP */
#define ISI_CCONV_OFF                       0x00000002

/**< Resolution */
#if 0
#define ISI_RES_VGA                         0x00000001      /**<  1  640x480     */
#define ISI_RES_2592_1944                   0x00000002      /**<  2 2592x1944    */
#define ISI_RES_3264_2448                   0x00000004      /**<  3 3264x2448    */
#define ISI_RES_1296_972                    0x00000008      /**<  3 3264x2448    */
#define ISI_RES_4416_3312                   0x00000010      /**<  5 4416x3312    */
#define ISI_RES_1600_1200                   0x00000020      /**<  5 4416x3312    */
#define ISI_RES_2112_1568                   0x00000040      /**<  5 4416x3312    */
#define ISI_RES_4224_3136                   0x00000080      /**<  5 4416x3312    */
#define ISI_RES_1632_1224				    0x00000100 
#define ISI_RES_SVGA30                      0x00001000      /**< 16 1280x720@5   */
#define ISI_RES_TV720P5                     0x00010000      /**< 16 1280x720@5   */
#define ISI_RES_TV720P15                    0x00020000      /**< 17 1280x720@15  */
#define ISI_RES_TV720P30                    0x00040000      /**< 18 1280x720@30  */
#define ISI_RES_TV720P60                    0x00080000      /**< 19 1280x720@60  */
#define ISI_RES_TV1080P5                    0x00200000      /**< 21 1920x1080@5  */
#define ISI_RES_TV1080P6                    0x00400000      /**< 22 1920x1080@6  */
#define ISI_RES_TV1080P10                   0x00800000      /**< 23 1920x1080@10 */
#define ISI_RES_TV1080P12                   0x01000000      /**< 24 1920x1080@12 */
#define ISI_RES_TV1080P15                   0x02000000      /**< 25 1920x1080@15 */
#define ISI_RES_TV1080P20                   0x04000000      /**< 26 1920x1080@20 */
#define ISI_RES_TV1080P24                   0x08000000      /**< 27 1920x1080@24 */
#define ISI_RES_TV1080P25                   0x10000000      /**< 28 1920x1080@25 */
#define ISI_RES_TV1080P30                   0x20000000      /**< 29 1920x1080@30 */
#define ISI_RES_TV1080P50                   0x40000000      /**< 30 1920x1080@50 */
#define ISI_RES_TV1080P60                   0x80000000      /**< 31 1920x1080@60 */
#else

#define ISI_RES_W_GET(v)                ((v&0x1fff000)>>12)
#define ISI_RES_H_GET(v)                (v&0xfff)
#define ISI_FPS_GET(v)                  ((v&0xfe000000)>>25)

#define ISI_RES_VGAP5                          0x0a2801e0        /**<  1  640x480     */
#define ISI_RES_VGAP10                         0x142801e0        /**<  1  640x480     */
#define ISI_RES_VGAP15                         0x1e2801e0        /**<  1  640x480     */
#define ISI_RES_VGAP20                         0x282801e0        /**<  1  640x480     */
#define ISI_RES_VGAP30                         0x3c2801e0        /**<  1  640x480     */
#define ISI_RES_VGAP60                         0x782801e0        /**<  1  640x480     */
#define ISI_RES_VGAP120                        0xf02801e0        /**<  1  640x480     */

#define ISI_RES_SVGAP5                         0x0a320258        /**< 16 1280x720@5   */
#define ISI_RES_SVGAP10                        0x14320258        /**< 16 1280x720@5   */
#define ISI_RES_SVGAP15                        0x1e320258        /**< 16 1280x720@5   */
#define ISI_RES_SVGAP20                        0x28320258        /**< 16 1280x720@5   */
#define ISI_RES_SVGAP30                        0x3c320258        /**< 16 1280x720@5   */
#define ISI_RES_SVGAP60                        0x78320258        /**< 16 1280x720@5   */
#define ISI_RES_SVGAP120                       0xf0320258        /**< 16 1280x720@5   */

#define ISI_RES_2592_1944P5                    0x0aa20798      /**<  2 2592x1944    */
#define ISI_RES_2592_1944P7                    0x0ea20798      /**<  2 2592x1944    */
#define ISI_RES_2592_1944P10                   0x14a20798      /**<  2 2592x1944    */
#define ISI_RES_2592_1944P15                   0x1ea20798      /**<  2 2592x1944    */
#define ISI_RES_2592_1944P20                   0x28a20798      /**<  2 2592x1944    */
#define ISI_RES_2592_1944P25                   0x32a20798      /**<  2 2592x1944    */
#define ISI_RES_2592_1944P30                   0x3ca20798      /**<  2 2592x1944    */

#define ISI_RES_1296_972P7                     0x0e5103cc       /**<  3 3264x2448    */
#define ISI_RES_1296_972P10                    0x145103cc       /**<  3 3264x2448    */
#define ISI_RES_1296_972P15                    0x1e5103cc       /**<  3 3264x2448    */
#define ISI_RES_1296_972P20                    0x285103cc       /**<  3 3264x2448    */
#define ISI_RES_1296_972P25                    0x325103cc       /**<  3 3264x2448    */
#define ISI_RES_1296_972P30                    0x3c5103cc       /**<  3 3264x2448    */

#define ISI_RES_3264_2448P7                    0x0ecc0990      /**<  3 3264x2448    */
#define ISI_RES_3264_2448P10                   0x14cc0990      /**<  3 3264x2448    */
#define ISI_RES_3264_2448P15                   0x1ecc0990      /**<  3 3264x2448    */
#define ISI_RES_3264_2448P20                   0x28cc0990      /**<  3 3264x2448    */
#define ISI_RES_3264_2448P25                   0x32cc0990      /**<  3 3264x2448    */
#define ISI_RES_3264_2448P30                   0x3ccc0990      /**<  3 3264x2448    */

#define ISI_RES_1632_1224P5                    0x0a6604c8                       
#define ISI_RES_1632_1224P7				       0x0e6604c8
#define ISI_RES_1632_1224P10                   0x146604c8
#define ISI_RES_1632_1224P15				   0x1e6604c8
#define ISI_RES_1632_1224P20                   0x286604c8
#define ISI_RES_1632_1224P25                   0x326604c8
#define ISI_RES_1632_1224P30				   0x3c6604c8

#define ISI_RES_4416_3312P7                    0x0f140cf0      /**<  5 4416x3312    */
#define ISI_RES_4416_3312P15                   0x1f140cf0
#define ISI_RES_4416_3312P30                   0x3d140cf0

#define ISI_RES_2208_1656P7                    0x0e8a0678      /**<  5 4416x3312    */
#define ISI_RES_2208_1656P15                   0x1e8a0678      /**<  5 4416x3312    */
#define ISI_RES_2208_1656P30                   0x3c8a0678      /**<  5 4416x3312    */

#define ISI_RES_1600_1200P7                    0x0e6404b0      /**<  5 4416x3312    */
#define ISI_RES_1600_1200P15                   0x1e6404b0       /**<  5 4416x3312    */
#define ISI_RES_1600_1200P30                   0x3c6404b0       /**<  5 4416x3312    */
#define ISI_RES_1600_1200P20                   0x286404b0       /**<  5 4416x3312    */
#define ISI_RES_1600_1200P10                   0x146404b0       /**<  5 4416x3312    */


#define ISI_RES_4208_3120P4                    0x09070c30 
#define ISI_RES_4208_3120P7                    0x0f070c30 
#define ISI_RES_4208_3120P10                   0x15070c30
#define ISI_RES_4208_3120P15                   0x1f070c30
#define ISI_RES_4208_3120P20                   0x29070c30
#define ISI_RES_4208_3120P25                   0x33070c30
#define ISI_RES_4208_3120P30                   0x3d070c30

#define ISI_RES_2104_1560P7                    0x0e838618 
#define ISI_RES_2104_1560P10                   0x14838618
#define ISI_RES_2104_1560P15                   0x1e838618
#define ISI_RES_2104_1560P20                   0x28838618
#define ISI_RES_2104_1560P25                   0x32838618
#define ISI_RES_2104_1560P30                   0x3c838618
#define ISI_RES_2104_1560P40                   0x50838618
#define ISI_RES_2104_1560P50                   0x64838618
#define ISI_RES_2104_1560P60                   0x78838618

#define ISI_RES_4224_3136P4                    0x09080c40
#define ISI_RES_4224_3136P7                    0x0f080c40      /**<  5 4416x3312    */
#define ISI_RES_4224_3136P10                   0x15080c40
#define ISI_RES_4224_3136P15                   0x1f080c40      /**<  5 4416x3312    */
#define ISI_RES_4224_3136P20                   0x29080c40
#define ISI_RES_4224_3136P25                   0x33080c40
#define ISI_RES_4224_3136P30                   0x3d080c40      /**<  5 4416x3312    */

#define ISI_RES_2112_1568P7                    0x0e840620      /**<  5 4416x3312    */
#define ISI_RES_2112_1568P10                   0x14840620      /**<  5 4416x3312    */
#define ISI_RES_2112_1568P15                   0x1e840620      /**<  5 4416x3312    */
#define ISI_RES_2112_1568P20                   0x28840620      /**<  5 4416x3312    */
#define ISI_RES_2112_1568P25                   0x32840620      /**<  5 4416x3312    */
#define ISI_RES_2112_1568P30                   0x3c840620      /**<  5 4416x3312    */
#define ISI_RES_2112_1568P40                   0x50840620      /**<  5 4416x3312    */
#define ISI_RES_2112_1568P50                   0x64840620      /**<  5 4416x3312    */
#define ISI_RES_2112_1568P60                   0x78840620      /**<  5 4416x3312    */

#define ISI_RES_TV720P5                     0x0a5002d0      /**< 16 1280x720@5   */
#define ISI_RES_TV720P15                    0x1e5002d0      /**< 17 1280x720@15  */
#define ISI_RES_TV720P30                    0x3c5002d0      /**< 18 1280x720@30  */
#define ISI_RES_TV720P60                    0x785002d0      /**< 19 1280x720@60  */

#define ISI_RES_TV1080P5                    0x0a780438      /**< 21 1920x1080@5  */
#define ISI_RES_TV1080P6                    0x0c780438      /**< 22 1920x1080@6  */
#define ISI_RES_TV1080P10                   0x14780438      /**< 23 1920x1080@10 */
#define ISI_RES_TV1080P12                   0x18780438
#define ISI_RES_TV1080P15                   0x1e780438
#define ISI_RES_TV1080P20                   0x28780438
#define ISI_RES_TV1080P24                   0x30780438
#define ISI_RES_TV1080P25                   0x32780438
#define ISI_RES_TV1080P30                   0x3c780438
#define ISI_RES_TV1080P50                   0x64780438
#define ISI_RES_TV1080P60                   0x78780438


#endif


/**< DwnSz */
#define ISI_DWNSZ_SUBSMPL                   0x00000001      //!< Use subsampling to downsize output window
#define ISI_DWNSZ_SCAL_BAY                  0x00000002      //!< Use scaling with Bayer sampling to downsize output window
#define ISI_DWNSZ_SCAL_COS                  0x00000004      //!< Use scaling with co-sited sampling to downsize output window

/**< BLC */
#define ISI_BLC_AUTO                        0x00000001      /**< Camera BlackLevelCorrection on */
#define ISI_BLC_OFF                         0x00000002      /**< Camera BlackLevelCorrection off */

/**< AGC */
#define ISI_AGC_AUTO                        0x00000001      /**< Camera AutoGainControl on */
#define ISI_AGC_OFF                         0x00000002      /**< Camera AutoGainControl off */

/**< AWB */
#define ISI_AWB_AUTO                        0x00000001      /**< Camera AutoWhiteBalance on */
#define ISI_AWB_OFF                         0x00000002      /**< Camera AutoWhiteBalance off */

/**< AEC */
#define ISI_AEC_AUTO                        0x00000001      /**< Camera AutoExposureControl on */
#define ISI_AEC_OFF                         0x00000002      /**< Camera AutoExposureControl off */

/**< DPCC */
#define ISI_DPCC_AUTO                       0x00000001      /**< Camera DefectPixelCorrection on */
#define ISI_DPCC_OFF                        0x00000002      /**< Camera DefectPixelCorrection off */

/**< AFPS */
#define ISI_AFPS_NOTSUPP                    0x00000000      /**< Auto FPS mode not supported; or ISI_RES_XXX bitmask of all resolutions being part of any AFPS series */


/**< ulCieProfile */
/* according to http://www.hunterlab.com/appnotes/an05_05.pdf, illuminants
 * A, D65 and F2 are most commonly used and should be selected prior to the
 * others if only a subset is to be supported. Illuminants B and E are mentioned
 * here: http://www.aim-dtp.net/aim/technology/cie_xyz/cie_xyz.htm. */
#define ISI_CIEPROF_A           0x00000001 //!< incandescent/tungsten,                            2856K
#define ISI_CIEPROF_D40         0x00000002 //
#define ISI_CIEPROF_D45         0x00000004 //
#define ISI_CIEPROF_D50         0x00000008 //!< horizon light,                                    5000K
#define ISI_CIEPROF_D55         0x00000010 //!< mid morning daylight,                             5500K
#define ISI_CIEPROF_D60         0x00000020
#define ISI_CIEPROF_D65         0x00000040 //!< indoor D65 daylight from fluorescent lamp,        6504K
#define ISI_CIEPROF_D70         0x00000080
#define ISI_CIEPROF_D75         0x00000100 //!< overcast daylight,                                7500K
#define ISI_CIEPROF_D80         0x00000200
#define ISI_CIEPROF_D85         0x00000400
#define ISI_CIEPROF_D90         0x00000800
#define ISI_CIEPROF_D95         0x00001000
#define ISI_CIEPROF_D100        0x00002000
#define ISI_CIEPROF_D105        0x00004000
#define ISI_CIEPROF_D110        0x00008000
#define ISI_CIEPROF_D115        0x00010000
#define ISI_CIEPROF_D120        0x00020000
#define ISI_CIEPROF_E           0x00040000 //!< Normalized reference source
#define ISI_CIEPROF_F1          0x00080000 //!< daylight flourescent                              6430K
#define ISI_CIEPROF_F2          0x00100000 //!< cool white flourescent CWF                        4230K
#define ISI_CIEPROF_F3          0x00200000 //!< white fluorescent                                 3450K
#define ISI_CIEPROF_F4          0x00400000 //!< warm white flourescent                            2940K
#define ISI_CIEPROF_F5          0x00800000 //!< daylight flourescent                              6350K
#define ISI_CIEPROF_F6          0x01000000 //!< lite white flourescent                            4150K
#define ISI_CIEPROF_F7          0x02000000 //!< similar to D65, daylight flourescent              6500K
#define ISI_CIEPROF_F8          0x04000000 //!< similar to D50, Sylvania F40DSGN50 flourescent    5000K
#define ISI_CIEPROF_F9          0x08000000 //!< cool white deluxe flourescent                     4150K
#define ISI_CIEPROF_F10         0x10000000 //!< TL85, Ultralume 50                                5000K
#define ISI_CIEPROF_F11         0x20000000 //!< TL84, Ultralume 40, SP41                          4000K
#define ISI_CIEPROF_F12         0x40000000 //!< TL83, Ultralume 30                                3000K
#define ISI_CIEPROF_HORIZON     0x80000000 //!< TL83, Ultralume 30



/*****************************************************************************/
/**
 *          IsiDoorType_t
 *
 * @brief   doortype of an illumination profile
 */
/*****************************************************************************/
typedef enum  IsiDoorType_e
{
    ISI_DOOR_TYPE_OUTDOOR = 0,
    ISI_DOOR_TYPE_INDOOR  = ( !ISI_DOOR_TYPE_OUTDOOR )
} IsiDoorType_t;



/*****************************************************************************/
/**
 *          IsiDoorType_t
 *
 * @brief   doortype of an illumination profile
 */
/*****************************************************************************/
typedef enum IsiAwbType_e
{
    ISI_AWB_TYPE_MANUAL  = 0,
    ISI_AWB_TYPE_AUTO    = ( !ISI_AWB_TYPE_MANUAL )
} IsiAwbType_t;



/*****************************************************************************/
/**
 *          IsiColorComponent_t
 *
 * @brief   color components
 */
/*****************************************************************************/
typedef enum IsiColorComponent_e
{
    ISI_COLOR_COMPONENT_RED     = 0,
    ISI_COLOR_COMPONENT_GREENR  = 1,
    ISI_COLOR_COMPONENT_GREENB  = 2,
    ISI_COLOR_COMPONENT_BLUE    = 3,
    ISI_COLOR_COMPONENT_MAX     = 4
} IsiColorComponent_t;



/*****************************************************************************/
/**
 *          Isi1x1FloatMatrix_t
 *
 * @brief   Matrix coefficients
 *
 *          | 0 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct Isi1x1FloatMatrix_s
{
    float fCoeff[1];
} Isi1x1FloatMatrix_t;



/*****************************************************************************/
/**
 *          Isi2x1FloatMatrix_t
 *
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct Isi2x1FloatMatrix
{
    float fCoeff[2];
} Isi2x1FloatMatrix_t;



/*****************************************************************************/
/**
 *          Isi2x1FloatMatrix_t
 *
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |
 *          | 2 | 3 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct Isi2x2FloatMatrix
{
    float fCoeff[4];
} Isi2x2FloatMatrix_t;



/*****************************************************************************/
/**
 *          Isi3x1FloatMatrix_t
 *
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct Isi3x1FloatMatrix
{
    float fCoeff[3];
} Isi3x1FloatMatrix_t;



/*****************************************************************************/
/**
 *          Isi3x2FloatMatrix_t
 *
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *          | 3 | 4 |  5 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct Isi3x2FloatMatrix_s
{
    float fCoeff[6];
} Isi3x2FloatMatrix_t;



/*****************************************************************************/
/**
 *          Isi3x3FloatMatrix_t
 *
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *          | 3 | 4 |  5 |
 *          | 6 | 7 |  8 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct Isi3x3FloatMatrix_s
{
    float fCoeff[9];
} Isi3x3FloatMatrix_t;



/*****************************************************************************/
/**
 *          Isi17x17ShortMatrix_t
 *
 * @brief   Matrix coefficients
 *
 *          |   0 |   1 |   2 |   3 |   4 |   5 |   6 |   7 | ....
 *          |  17 |  18 |  19 |  20 |  21 |  22 |  23 |  24 | ....
 *          |  34 |  35 |  36 |  37 |  38 |  39 |  40 |  41 | ....
 *          ...
 *          ...
 *          ...
 *          | 271 | 272 | 273 | 274 | 275 | 276 | 277 | 278 | .... | 288 |
 *
 * @note    Coefficients are represented as short numbers
 */
/*****************************************************************************/
typedef struct Isi17x17ShortMatrix_s
{
    uint16_t uCoeff[17 * 17];
} Isi17x17ShortMatrix_t;



/*****************************************************************************/
/**
 *          IsiGaussFactor_t
 *
 * @brief
 *
 * @note
 */
/*****************************************************************************/
typedef struct IsiGaussFactor_s
{
    float fGaussFactor;
} IsiGaussFactor_t;



/*****************************************************************************/
/**
 *          IsiComponentGain_t
 *
 * @brief   white balancing gains
 *
 * @note    There are two green gains: One for the green Bayer patterns in
 *          the red and one for the blue line. In the case the used CamerIC
 *          derivative is not able to apply separate green gains the mean
 *          value of both greens will be used for the green gain. The
 *          component gains are represented as signed fixed point numbers
 *          with 12 bit integer and 20 bit fractional part, thus ranging
 *          from -2048.0000000 (0x80000000) to +2047.9999990 (0x7FFFFFFF).
 *
 *          Example: +1.0 is represented by 0x00100000.
 */
/*****************************************************************************/
typedef struct IsiComponentGain_s
{
    float fRed;
    float fGreenR;
    float fGreenB;
    float fBlue;
} IsiComponentGain_t;



/*****************************************************************************/
/**
 *          IsiXTalkFloatOffset_t
 *
 * @brief
 *
 */
/*****************************************************************************/
typedef struct IsiXTalkFloatOffset_s
{
    float fCtOffsetRed;
    float fCtOffsetGreen;
    float fCtOffsetBlue;
} IsiXTalkFloatOffset_t;



/*****************************************************************************/
/**
 *          IsiLine_t
 *
 * @brief   Contains parameters for a straight line in Hesse normal form in
 *          Rg/Bg colorspace
 *
 */
/*****************************************************************************/
typedef struct IsiLine_s
{
    float f_N0_Rg;      /**< Rg component of normal vector */
    float f_N0_Bg;      /**< Bg component of normal vector */
    float f_d;          /**< Distance of normal vector     */
} IsiLine_t;



/*****************************************************************************/
/**
 *          IsiAwbClipParm_t
 *
 * @brief   Contains pointers to parameter arrays for Rg/Bg color space
 *          clipping
 *
 */
/*****************************************************************************/
typedef struct IsiAwbClipParm_s
{
    float*      pRg1;
    float*      pMaxDist1;
    uint16_t    ArraySize1;
    float*      pRg2;
    float*      pMaxDist2;
    uint16_t    ArraySize2;
} IsiAwbClipParm_t;



/*****************************************************************************/
/**
 *          IsiAwbGlobalFadeParm_t
 *
 * @brief   Contains pointers to parameter arrays for AWB out of range
 *          handling
 *
 */
/*****************************************************************************/
typedef struct IsiAwbGlobalFadeParm_s
{
    float*      pGlobalFade1;
    float*      pGlobalGainDistance1;
    uint16_t    ArraySize1;
    float*      pGlobalFade2;
    float*      pGlobalGainDistance2;
    uint16_t    ArraySize2;
} IsiAwbGlobalFadeParm_t;



/*****************************************************************************/
/**
 *          IsiAwbGlobalFadeParm_t
 *
 * @brief   Contains pointers to parameter arrays for near white pixel
 *          parameter calculations
 *
 */
/*****************************************************************************/
typedef struct IsiAwbFade2Parm_s
{
    float*      pFade;
    float*      pCbMinRegionMax;
    float*      pCrMinRegionMax;
    float*      pMaxCSumRegionMax;
    float*      pCbMinRegionMin;
    float*      pCrMinRegionMin;
    float*      pMaxCSumRegionMin;
    uint16_t    ArraySize;
} IsiAwbFade2Parm_t;



/*****************************************************************************/
/**
 *          IsiSaturationCurve_t
 *
 * @brief   parameters for a sensorgain to saturation interpolation
 *
 */
/*****************************************************************************/
typedef struct IsiSaturationCurve_s
{
    uint16_t    ArraySize;
    float       *pSensorGain;
    float       *pSaturation;
} IsiSaturationCurve_t;



/*****************************************************************************/
/**
 *          IsiSatCcMatrix_t
 *
 * @brief   saturated XTalk-Matrix
 *
 */
/*****************************************************************************/
typedef struct IsiSatCcMatrix_s
{
    float               fSaturation;
    Isi3x3FloatMatrix_t XTalkCoeff;
} IsiSatCcMatrix_t;



/*****************************************************************************/
/**
 *          IsiCcMatrixTable_t
 *
 * @brief   Array of saturated XTalk-Matrices
 *
 */
/*****************************************************************************/
typedef struct IsiCcMatrixTable_s
{
    uint16_t            ArraySize;
    IsiSatCcMatrix_t    *pIsiSatCcMatrix;
} IsiCcMatrixTable_t;



/*****************************************************************************/
/**
 *          IIsiSatCcOffset_t
 *
 * @brief   saturated XTalk-Offset
 *
 */
/*****************************************************************************/
typedef struct IsiSatCcOffset_s
{
    float                   fSaturation;
    IsiXTalkFloatOffset_t   CcOffset;
} IsiSatCcOffset_t;



/*****************************************************************************/
/**
 *          IsiCcOffsetTable_t
 *
 * @brief   Array of saturated XTalk-Offset
 *
 */
/*****************************************************************************/
typedef struct IsiCcOffsetTable_s
{
    float               ArraySize;
    IsiSatCcOffset_t    *pIsiSatCcOffset;
} IsiCcOffsetTable_t;



/*****************************************************************************/
/**
 *          IsiVignettingCurve_t
 *
 * @brief   parameters for a sensorgain to vignetting (compensation)
 *          interpolation
 *
 */
/*****************************************************************************/
typedef struct IsiVignettingCurve_s
{
    uint16_t    ArraySize;
    float       *pSensorGain;
    float       *pVignetting;
} IsiVignettingCurve_t;



/*****************************************************************************/
/**
 *          IsiVignettingCurve_t
 *
 * @brief   parameters for a sensorgain to vignetting (compensation)
 *          interpolation
 *
 */
/*****************************************************************************/
typedef struct IsiVignLscMatrix_s
{
    float                   fVignetting;
    Isi17x17ShortMatrix_t   LscMatrix[ISI_COLOR_COMPONENT_MAX];
} IsiVignLscMatrix_t;



/*****************************************************************************/
/**
 *          IsiLscMatrixTable_t
 *
 * @brief   parameters for a sensorgain to vignetting (compensation)
 *          interpolation
 *
 */
/*****************************************************************************/
typedef struct IsiLscMatrixTable_s
{
    uint16_t                ArraySize;
    IsiVignLscMatrix_t      *psIsiVignLscMatrix;
    uint16_t                LscXGradTbl[8];
    uint16_t                LscYGradTbl[8];
    uint16_t                LscXSizeTbl[8];
    uint16_t                LscYSizeTbl[8];
} IsiLscMatrixTable_t;


/*****************************************************************************/
/*!
 *  *  Sensor-specific information structure for MIPI. Is filled in by sensor
 *   *  specific code. Features not supported by the sensor driver code will be
 *   set
 *    *  to NULL.
 *     */
/*****************************************************************************/
typedef struct
{
    uint8_t  ucMipiLanes;             // number of used MIPI lanes by sensor
    uint32_t   ulMipiFreq;
    uint32_t    sensorHalDevID;
} IsiSensorMipiInfo;

/*****************************************************************************/
/**
 *          IsiIlluProfile_t
 *
 * @brief   Illumination specific structure.
 *
 */
/*****************************************************************************/
typedef struct IsiIlluProfile_s
{
    void                            *p_next;                /*!< void pointer for setting up a illumination list */
    const char                      *name;                  /*!< name of the illumination profile (i.e. "D65", "A", ... )*/
    const uint32_t                  id;                     /*!< unique id ( bitmask ) */

    const IsiDoorType_t             DoorType;               /*!< indoor or outdoor profile */
    const IsiAwbType_t              AwbType;                /*!< manual or auto profile */

    const bool_t                    bOutdoorClip;           /*!< outdoor clipping profile (true | false ) */

    const Isi3x3FloatMatrix_t       *pColorConvCoeff;       /*!< ColorConversion  matrix coefficients */
    const Isi3x3FloatMatrix_t       *pCrossTalkCoeff;       /*!< CrossTalk matrix coefficients */
    const IsiXTalkFloatOffset_t     *pCrossTalkOffset;      /*!< CrossTalk offsets */
    const IsiComponentGain_t        *pComponentGain;        /*!< */

    const Isi2x1FloatMatrix_t       *pGaussMeanValue;       /*!< */
    const Isi2x2FloatMatrix_t       *pCovarianceMatrix;     /*!< */
    const IsiGaussFactor_t          *pGaussFactor;          /*!< */
    const Isi2x1FloatMatrix_t       *pThreshold;            /*!< */

    const IsiSaturationCurve_t      *pSaturationCurve;      /*!< sensor-gain to saturation */
    const IsiCcMatrixTable_t        *pCcMatrixTable;
    const IsiCcOffsetTable_t        *pCcOffsetTable;

    const IsiVignettingCurve_t      *pVignettingCurve;      /*!< sensor-gain to vignetting */
} IsiIlluProfile_t;



#ifdef __cplusplus
}
#endif

#endif /* __ISI_COMMON_H__ */
