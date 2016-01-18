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
 * @file OV14825_tables.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"
#include "OV14825_MIPI_priv.h"


/*****************************************************************************
 * DEFINES
 *****************************************************************************/


/*****************************************************************************
 * GLOBALS
 *****************************************************************************/

// Image sensor register settings default values taken from data sheet OV14825_DS_1.1_SiliconImage.pdf.
// The settings may be altered by the code in IsiSetupSensor.
const IsiRegDescription_t OV14825_g_aRegDescription[] =
{
  //{ulAddr                     , ulDefaultValue                        , pszName                   , ulFlags}

  /* PLL registers and a MIPI register, set these first to get into defined state */
  {OV14825_PLL_CTRL4            , OV14825_PLL_CTRL4_DEFAULT             , "PLL_CTRL4"               , eReadWrite},
  {OV14825_PLL_CTRL3            , OV14825_PLL_CTRL3_DEFAULT             , "PLL_CTRL3"               , eReadWrite},
  {OV14825_PLL_CTRL2            , OV14825_PLL_CTRL2_DEFAULT             , "PLL_CTRL2"               , eReadWrite},
  {OV14825_PLL_CTRL1            , OV14825_PLL_CTRL1_DEFAULT             , "PLL_CTRL1"               , eReadWrite},
  {OV14825_PLL_CTRL0            , OV14825_PLL_CTRL0_DEFAULT             , "PLL_CTRL0"               , eReadWrite},
  {OV14825_SC_MIPI_SC_CTRL      , OV14825_SC_MIPI_SC_CTRL_DEFAULT       , "SC_MIPI_SC_CTRL"         , eReadWrite},

  /* unknown registers but set on evaluation platform, follow the sequence of the reference settings */
  {OV14825_301B                 , OV14825_301B_DEFAULT                  , "OV14825_301B"            , eReadWrite},
  {OV14825_301C                 , OV14825_301C_DEFAULT                  , "OV14825_301C"            , eReadWrite},
  {OV14825_3020                 , OV14825_3020_DEFAULT                  , "OV14825_3020"            , eReadWrite},
  {OV14825_3106                 , OV14825_3106_DEFAULT                  , "OV14825_3106"            , eReadWrite},
  {OV14825_3600                 , OV14825_3600_DEFAULT                  , "OV14825_3600"            , eReadWrite},
  {OV14825_3601                 , OV14825_3601_DEFAULT                  , "OV14825_3601"            , eReadWrite},
  {OV14825_3609                 , OV14825_3609_DEFAULT                  , "OV14825_3609"            , eReadWrite},
  {OV14825_360A                 , OV14825_360A_DEFAULT                  , "OV14825_360A"            , eReadWrite},
  {OV14825_360F                 , OV14825_360F_DEFAULT                  , "OV14825_360F"            , eReadWrite},
  {OV14825_3611                 , OV14825_3611_DEFAULT                  , "OV14825_3611"            , eReadWrite},
  {OV14825_3613                 , OV14825_3613_DEFAULT                  , "OV14825_3613"            , eReadWrite},
  {OV14825_3702                 , OV14825_3702_DEFAULT                  , "OV14825_3702"            , eReadWrite},
  {OV14825_3704                 , OV14825_3704_DEFAULT                  , "OV14825_3704"            , eReadWrite},
  {OV14825_3705                 , OV14825_3705_DEFAULT                  , "OV14825_3705"            , eReadWrite},
  {OV14825_3708                 , OV14825_3708_DEFAULT                  , "OV14825_3708"            , eReadWrite},
  {OV14825_370E                 , OV14825_370E_DEFAULT                  , "OV14825_370E"            , eReadWrite},
  {OV14825_3710                 , OV14825_3710_DEFAULT                  , "OV14825_3710"            , eReadWrite},
  {OV14825_3711                 , OV14825_3711_DEFAULT                  , "OV14825_3711"            , eReadWrite},
  {OV14825_3714                 , OV14825_3714_DEFAULT                  , "OV14825_3714"            , eReadWrite},
  {OV14825_3715                 , OV14825_3715_DEFAULT                  , "OV14825_3715"            , eReadWrite},
  {OV14825_3717                 , OV14825_3717_DEFAULT                  , "OV14825_3717"            , eReadWrite},
  {OV14825_3718                 , OV14825_3718_DEFAULT                  , "OV14825_3718"            , eReadWrite},
  {OV14825_3719                 , OV14825_3719_DEFAULT                  , "OV14825_3719"            , eReadWrite},
  {OV14825_371B                 , OV14825_371B_DEFAULT                  , "OV14825_371B"            , eReadWrite},
  {OV14825_371C                 , OV14825_371C_DEFAULT                  , "OV14825_371C"            , eReadWrite},
  {OV14825_371D                 , OV14825_371D_DEFAULT                  , "OV14825_371D"            , eReadWrite},
  {OV14825_371E                 , OV14825_371E_DEFAULT                  , "OV14825_371E"            , eReadWrite},
  {OV14825_3723                 , OV14825_3723_DEFAULT                  , "OV14825_3723"            , eReadWrite},
  {OV14825_3725                 , OV14825_3725_DEFAULT                  , "OV14825_3725"            , eReadWrite},
  {OV14825_3726                 , OV14825_3726_DEFAULT                  , "OV14825_3726"            , eReadWrite},
  {OV14825_3808                 , OV14825_3808_DEFAULT                  , "OV14825_3808"            , eReadWrite},
  {OV14825_380A                 , OV14825_380A_DEFAULT                  , "OV14825_380A"            , eReadWrite},
  {OV14825_3817                 , OV14825_3817_DEFAULT                  , "OV14825_3817"            , eReadWrite},
  {OV14825_3819                 , OV14825_3819_DEFAULT                  , "OV14825_3819"            , eReadWrite},
  {OV14825_382C                 , OV14825_382C_DEFAULT                  , "OV14825_382C"            , eReadWrite},
  {OV14825_382D                 , OV14825_382D_DEFAULT                  , "OV14825_382D"            , eReadWrite},
  {OV14825_3A1A                 , OV14825_3A1A_DEFAULT                  , "OV14825_3A1A"            , eReadWrite},
  {OV14825_3A25                 , OV14825_3A25_DEFAULT                  , "OV14825_3A25"            , eReadWrite},
  {OV14825_3B09                 , OV14825_3B09_DEFAULT                  , "OV14825_3B09"            , eReadWrite},
  {OV14825_4002                 , OV14825_4002_DEFAULT                  , "OV14825_4002"            , eReadWrite},
  {OV14825_4004                 , OV14825_4004_DEFAULT                  , "OV14825_4004"            , eReadWrite},
  {OV14825_4005                 , OV14825_4005_DEFAULT                  , "OV14825_4005"            , eReadWrite},
  {OV14825_4009                 , OV14825_4009_DEFAULT                  , "OV14825_4009"            , eReadWrite},
  {OV14825_404F                 , OV14825_404F_DEFAULT                  , "OV14825_404F"            , eReadWrite},
  {OV14825_4709                 , OV14825_4709_DEFAULT                  , "OV14825_4709"            , eReadWrite},
  {OV14825_4801                 , OV14825_4801_DEFAULT                  , "OV14825_4801"            , eReadWrite},
  {OV14825_4806                 , OV14825_4806_DEFAULT                  , "OV14825_4806"            , eReadWrite},
  {OV14825_4842                 , OV14825_4842_DEFAULT                  , "OV14825_4842"            , eReadWrite},
  {OV14825_503B                 , OV14825_503B_DEFAULT                  , "OV14825_503B"            , eReadWrite},
  {OV14825_503C                 , OV14825_503C_DEFAULT                  , "OV14825_503C"            , eReadWrite},
  {OV14825_5780                 , OV14825_5780_DEFAULT                  , "OV14825_5780"            , eReadWrite},
  {OV14825_5B00                 , OV14825_5B00_DEFAULT                  , "OV14825_5B00"            , eReadWrite},
  {OV14825_5B01                 , OV14825_5B01_DEFAULT                  , "OV14825_5B01"            , eReadWrite},
  {OV14825_5B03                 , OV14825_5B03_DEFAULT                  , "OV14825_5B03"            , eReadWrite},
  {OV14825_3602                 , OV14825_3602_DEFAULT                  , "OV14825_3602"            , eReadWrite},
  {OV14825_3604                 , OV14825_3604_DEFAULT                  , "OV14825_3604"            , eReadWrite},
  {OV14825_3605                 , OV14825_3605_DEFAULT                  , "OV14825_3605"            , eReadWrite},
  {OV14825_360B                 , OV14825_360B_DEFAULT                  , "OV14825_360B"            , eReadWrite},
  {OV14825_360C                 , OV14825_360C_DEFAULT                  , "OV14825_360C"            , eReadWrite},
  {OV14825_360D                 , OV14825_360D_DEFAULT                  , "OV14825_360D"            , eReadWrite},
  {OV14825_3614                 , OV14825_3614_DEFAULT                  , "OV14825_3614"            , eReadWrite},
  {OV14825_3707                 , OV14825_3707_DEFAULT                  , "OV14825_3707"            , eReadWrite},
  {OV14825_370A                 , OV14825_370A_DEFAULT                  , "OV14825_370A"            , eReadWrite},
  {OV14825_370B                 , OV14825_370B_DEFAULT                  , "OV14825_370B"            , eReadWrite},
  {OV14825_370C                 , OV14825_370C_DEFAULT                  , "OV14825_370C"            , eReadWrite},
  {OV14825_370F                 , OV14825_370F_DEFAULT                  , "OV14825_370F"            , eReadWrite},
  {OV14825_3713                 , OV14825_3713_DEFAULT                  , "OV14825_3713"            , eReadWrite},
  {OV14825_3716                 , OV14825_3716_DEFAULT                  , "OV14825_3716"            , eReadWrite},
  {OV14825_3721                 , OV14825_3721_DEFAULT                  , "OV14825_3721"            , eReadWrite},
  {OV14825_3724                 , OV14825_3724_DEFAULT                  , "OV14825_3724"            , eReadWrite},
  {OV14825_3727                 , OV14825_3727_DEFAULT                  , "OV14825_3727"            , eReadWrite},
  {OV14825_3728                 , OV14825_3728_DEFAULT                  , "OV14825_3728"            , eReadWrite},
  {OV14825_3803                 , OV14825_3803_DEFAULT                  , "OV14825_3803"            , eReadWrite},
  {OV14825_3811                 , OV14825_3811_DEFAULT                  , "OV14825_3811"            , eReadWrite},
  {OV14825_4050                 , OV14825_4050_DEFAULT                  , "OV14825_4050"            , eReadWrite},
  {OV14825_4051                 , OV14825_4051_DEFAULT                  , "OV14825_4051"            , eReadWrite},
  {OV14825_4053                 , OV14825_4053_DEFAULT                  , "OV14825_4053"            , eReadWrite},
  {OV14825_4837                 , OV14825_4837_DEFAULT                  , "OV14825_4837"            , eReadWrite},
  {OV14825_5042                 , OV14825_5042_DEFAULT                  , "OV14825_5042"            , eReadWrite},
  {OV14825_5047                 , OV14825_5047_DEFAULT                  , "OV14825_5047"            , eReadWrite},

  /* system control registers */
  {OV14825_SMIA_R0100           , OV14825_SMIA_R0100_DEFAULT            , "SMIA_R0100"              , eReadWrite},
  {OV14825_SMIA_R0103           , OV14825_SMIA_R0103_DEFAULT            , "SMIA_R0103"              , eReadWrite},
  {OV14825_PAD_OEN2             , OV14825_PAD_OEN2_DEFAULT              , "PAD_OEN2"                , eReadWrite},
  {OV14825_PAD_OUT0             , OV14825_PAD_OUT0_DEFAULT              , "PAD_OUT0"                , eReadWrite},
  {OV14825_SCCB_ID              , OV14825_SCCB_ID_DEFAULT               , "SCCB_ID"                 , eReadWrite},
  {OV14825_SC_PAD_OEN0          , OV14825_SC_PAD_OEN0_DEFAULT           , "SC_PAD_OEN0"             , eReadWrite},
  {OV14825_SC_PAD_OEN1          , OV14825_SC_PAD_OEN1_DEFAULT           , "SC_PAD_OEN1"             , eReadWrite},
  {OV14825_CHIP_ID_HIGH_BYTE    , OV14825_CHIP_ID_HIGH_BYTE_DEFAULT     , "CHIP_ID_HIGH_BYTE"       , eReadOnly},
  {OV14825_CHIP_ID_LOW_BYTE     , OV14825_CHIP_ID_LOW_BYTE_DEFAULT      , "CHIP_ID_LOW_BYTE"        , eReadOnly},
  {OV14825_SC_PAD_OUT1          , OV14825_SC_PAD_OUT1_DEFAULT           , "SC_PAD_OUT1"             , eReadWrite},
  {OV14825_SC_PAD_OUT2          , OV14825_SC_PAD_OUT2_DEFAULT           , "SC_PAD_OUT2"             , eReadWrite},
  {OV14825_SC_PAD_SEL0          , OV14825_SC_PAD_SEL0_DEFAULT           , "SC_PAD_SEL0"             , eReadWrite},
  {OV14825_SC_PAD_SEL1          , OV14825_SC_PAD_SEL1_DEFAULT           , "SC_PAD_SEL1"             , eReadWrite},
  {OV14825_SC_PAD_SEL2          , OV14825_SC_PAD_SEL2_DEFAULT           , "SC_PAD_SEL2"             , eReadWrite},
  {OV14825_SC_PLL_CTRL_S2       , OV14825_SC_PLL_CTRL_S2_DEFAULT        , "SC_PLL_CTRL_S2"          , eReadWrite},
  {OV14825_SC_PLL_CTRL_S1       , OV14825_SC_PLL_CTRL_S1_DEFAULT        , "SC_PLL_CTRL_S1"          , eReadWrite},
  {OV14825_SC_PLL_CTRL_S0       , OV14825_SC_PLL_CTRL_S0_DEFAULT        , "SC_PLL_CTRL_S0"          , eReadWrite},

  /* SRB (for sensor register-group programming) */
  {OV14825_GROUP1_START_ADDRESS , OV14825_GROUP1_START_ADDRESS_DEFAULT  , "GROUP1_START_ADDRESS"    , eReadWrite},
  {OV14825_GROUP2_START_ADDRESS , OV14825_GROUP2_START_ADDRESS_DEFAULT  , "GROUP2_START_ADDRESS"    , eReadWrite},
  {OV14825_GROUP3_START_ADDRESS , OV14825_GROUP3_START_ADDRESS_DEFAULT  , "GROUP3_START_ADDRESS"    , eReadWrite},
  {OV14825_GROUP4_START_ADDRESS , OV14825_GROUP4_START_ADDRESS_DEFAULT  , "GROUP4_START_ADDRESS"    , eReadWrite},
  {OV14825_GROUP_ACCESS         , OV14825_GROUP_ACCESS_DEFAULT          , "GROUP_ACCESS"            , eWriteOnly},
  {OV14825_GROUP_STATUS         , OV14825_GROUP_STATUS_DEFAULT          , "GROUP_STATUS"            , eReadOnly},

  /* manual AEC / AGC */
  {OV14825_AEC_EXPO_2           , OV14825_AEC_EXPO_2_DEFAULT            , "AEC_EXPO_2"              , eReadWrite},
  {OV14825_AEC_EXPO_1           , OV14825_AEC_EXPO_1_DEFAULT            , "AEC_EXPO_1"              , eReadWrite},
  {OV14825_AEC_EXPO_0           , OV14825_AEC_EXPO_0_DEFAULT            , "AEC_EXPO_0"              , eReadWrite},
  {OV14825_AEC_MANUAL           , OV14825_AEC_MANUAL_DEFAULT            , "AEC_MANUAL"              , eReadWrite},
  {OV14825_AEC_AGC_ADJ_2        , OV14825_AEC_AGC_ADJ_2_DEFAULT         , "AEC_AGC_ADJ_2"           , eReadWrite},
  {OV14825_AEC_AGC_ADJ_1        , OV14825_AEC_AGC_ADJ_1_DEFAULT         , "AEC_AGC_ADJ_1"           , eReadWrite},

  /* sensor control */
  {OV14825_ARRAY_CONTROL_0D     , OV14825_ARRAY_CONTROL_0D_DEFAULT      , "ARRAY_CONTROL_0D"        , eReadWrite},

  /* timing control */
  {OV14825_TIMING_HW_2          , OV14825_TIMING_HW_2_DEFAULT           , "TIMING_HW_2"             , eReadWrite},
  {OV14825_TIMING_HW_1          , OV14825_TIMING_HW_1_DEFAULT           , "TIMING_HW_1"             , eReadWrite},
  {OV14825_TIMING_VH_2          , OV14825_TIMING_VH_2_DEFAULT           , "TIMING_VH_2"             , eReadWrite},
  {OV14825_TIMING_VH_1          , OV14825_TIMING_VH_1_DEFAULT           , "TIMING_VH_1"             , eReadWrite},
  {OV14825_TIMING_HOFFS         , OV14825_TIMING_HOFFS_DEFAULT          , "TIMING_HOFFS"            , eReadWrite},
  {OV14825_TIMING_TC_REG18      , OV14825_TIMING_TC_REG18_DEFAULT       , "TIMING_TC_REG18"         , eReadWrite},
  {OV14825_TIMING_HTS_2         , OV14825_TIMING_HTS_2_DEFAULT          , "TIMING_HTS_2"            , eReadWrite},
  {OV14825_TIMING_HTS_1         , OV14825_TIMING_HTS_1_DEFAULT          , "TIMING_HTS_1"            , eReadWrite},
  {OV14825_TIMING_VTS_2         , OV14825_TIMING_VTS_2_DEFAULT          , "TIMING_VTS_2"            , eReadWrite},
  {OV14825_TIMING_VTS_1         , OV14825_TIMING_VTS_1_DEFAULT          , "TIMING_VTS_1"            , eReadWrite},
  {OV14825_TIMING_CONTROL_1C    , OV14825_TIMING_CONTROL_1C_DEFAULT     , "TIMING_CONTROL_1C"       , eReadWrite},
  {OV14825_TIMING_CONTROL_1D    , OV14825_TIMING_CONTROL_1D_DEFAULT     , "TIMING_CONTROL_1D"       , eReadWrite},
  {OV14825_TIMING_CONTROL_1E    , OV14825_TIMING_CONTROL_1E_DEFAULT     , "TIMING_CONTROL_1E"       , eReadWrite},
  {OV14825_TIMING_CONTROL_1F    , OV14825_TIMING_CONTROL_1F_DEFAULT     , "TIMING_CONTROL_1F"       , eReadWrite},
  {OV14825_TIMING_CONTROL_20    , OV14825_TIMING_CONTROL_20_DEFAULT     , "TIMING_CONTROL_20"       , eReadWrite},
  {OV14825_TIMING_CONTROL_21    , OV14825_TIMING_CONTROL_21_DEFAULT     , "TIMING_CONTROL_21"       , eReadWrite},

  /* AEC / AGC */
  {OV14825_AEC_CTRL00           , OV14825_AEC_CTRL00_DEFAULT            , "AEC_CTRL00"              , eReadWrite},
  {OV14825_AEC_CTRL01           , OV14825_AEC_CTRL01_DEFAULT            , "AEC_CTRL01"              , eReadWrite},
  {OV14825_AEC_MAX_EXPO_60_3    , OV14825_AEC_MAX_EXPO_60_3_DEFAULT     , "AEC_MAX_EXPO_60_3"       , eReadWrite},
  {OV14825_AEC_MAX_EXPO_60_2    , OV14825_AEC_MAX_EXPO_60_2_DEFAULT     , "AEC_MAX_EXPO_60_2"       , eReadWrite},
  {OV14825_AEC_MAX_EXPO_60_1    , OV14825_AEC_MAX_EXPO_60_1_DEFAULT     , "AEC_MAX_EXPO_60_1"       , eReadWrite},
  {OV14825_AEC_CTRL05           , OV14825_AEC_CTRL05_DEFAULT            , "AEC_CTRL05"              , eReadWrite},
  {OV14825_AEC_CTRL06           , OV14825_AEC_CTRL06_DEFAULT            , "AEC_CTRL06"              , eReadWrite},
  {OV14825_AEC_CTRL07           , OV14825_AEC_CTRL07_DEFAULT            , "AEC_CTRL07"              , eReadWrite},
  {OV14825_AEC_B50_STEP_2       , OV14825_AEC_B50_STEP_2_DEFAULT        , "AEC_B50_STEP_2"          , eReadWrite},
  {OV14825_AEC_B50_STEP_1       , OV14825_AEC_B50_STEP_1_DEFAULT        , "AEC_B50_STEP_1"          , eReadWrite},
  {OV14825_AEC_B60_STEP_2       , OV14825_AEC_B60_STEP_2_DEFAULT        , "AEC_B60_STEP_2"          , eReadWrite},
  {OV14825_AEC_B60_STEP_1       , OV14825_AEC_B60_STEP_1_DEFAULT        , "AEC_B60_STEP_1"          , eReadWrite},
  {OV14825_AEC_CTRL0D           , OV14825_AEC_CTRL0D_DEFAULT            , "AEC_CTRL0D"              , eReadWrite},
  {OV14825_AEC_CTRL0E           , OV14825_AEC_CTRL0E_DEFAULT            , "AEC_CTRL0E"              , eReadWrite},
  {OV14825_AEC_CTRL0F           , OV14825_AEC_CTRL0F_DEFAULT            , "AEC_CTRL0F"              , eReadWrite},
  {OV14825_AEC_CTRL10           , OV14825_AEC_CTRL10_DEFAULT            , "AEC_CTRL10"              , eReadWrite},
  {OV14825_AEC_CTRL11           , OV14825_AEC_CTRL11_DEFAULT            , "AEC_CTRL11"              , eReadWrite},
  {OV14825_AEC_CTRL12           , OV14825_AEC_CTRL12_DEFAULT            , "AEC_CTRL12"              , eReadWrite},
  {OV14825_AEC_CTRL13           , OV14825_AEC_CTRL13_DEFAULT            , "AEC_CTRL13"              , eReadWrite},
  {OV14825_AEC_MAX_EXPO_50_2    , OV14825_AEC_MAX_EXPO_50_2_DEFAULT     , "AEC_MAX_EXPO_50_2"       , eReadWrite},
  {OV14825_AEC_MAX_EXPO_50_1    , OV14825_AEC_MAX_EXPO_50_1_DEFAULT     , "AEC_MAX_EXPO_50_1"       , eReadWrite},
  {OV14825_AEC_MAX_EXPO_50_0    , OV14825_AEC_MAX_EXPO_50_0_DEFAULT     , "AEC_MAX_EXPO_50_0"       , eReadWrite},
  {OV14825_AEC_CTRL17           , OV14825_AEC_CTRL17_DEFAULT            , "AEC_CTRL17"              , eReadWrite},
  {OV14825_AEC_GAIN_CEILING_2   , OV14825_AEC_GAIN_CEILING_2_DEFAULT    , "AEC_GAIN_CEILING_2"      , eReadWrite},
  {OV14825_AEC_GAIN_CEILING_1   , OV14825_AEC_GAIN_CEILING_1_DEFAULT    , "AEC_GAIN_CEILING_1"      , eReadWrite},
  {OV14825_AEC_CTRL1B           , OV14825_AEC_CTRL1B_DEFAULT            , "AEC_CTRL1B"              , eReadWrite},
  {OV14825_LED_ADD_RW_2         , OV14825_LED_ADD_RW_2_DEFAULT          , "LED_ADD_RW_2"            , eReadWrite},
  {OV14825_LED_ADD_RW_1         , OV14825_LED_ADD_RW_1_DEFAULT          , "LED_ADD_RW_1"            , eReadWrite},
  {OV14825_AEC_CTRL1E           , OV14825_AEC_CTRL1E_DEFAULT            , "AEC_CTRL1E"              , eReadWrite},
  {OV14825_AEC_CTRL1F           , OV14825_AEC_CTRL1F_DEFAULT            , "AEC_CTRL1F"              , eReadWrite},

  /* strobe control */
  {OV14825_STROBE_CTRL00        , OV14825_STROBE_CTRL00_DEFAULT         , "STROBE_CTRL00"           , eReadWrite},
  {OV14825_STROBE_CTRL06        , OV14825_STROBE_CTRL06_DEFAULT         , "STROBE_CTRL06"           , eReadWrite},
  {OV14825_STROBE_CTRL07        , OV14825_STROBE_CTRL07_DEFAULT         , "STROBE_CTRL07"           , eReadWrite},
  {OV14825_STROBE_CTRL0A        , OV14825_STROBE_CTRL0A_DEFAULT         , "STROBE_CTRL0A"           , eReadWrite},
  {OV14825_STROBE_CTRL0B        , OV14825_STROBE_CTRL0B_DEFAULT         , "STROBE_CTRL0B"           , eReadWrite},

  /* auto light frequency detection */
  {OV14825_ALFD_CTRL1           , OV14825_ALFD_CTRL1_DEFAULT            , "ALFD_CTRL1"              , eReadWrite},
  {OV14825_ALFD_CTRLC           , OV14825_ALFD_CTRLC_DEFAULT            , "ALFD_CTRLC"              , eReadOnly},

  /* OTP control */
  {OV14825_OTP_DUMP_PROGRAM_2   , OV14825_OTP_DUMP_PROGRAM_2_DEFAULT    , "OTP_DUMP_PROGRAM_2"      , eReadWrite},
  {OV14825_OTP_DUMP_PROGRAM_1   , OV14825_OTP_DUMP_PROGRAM_1_DEFAULT    , "OTP_DUMP_PROGRAM_1"      , eReadWrite},
  // data 0x3D05 - 0x3D0F

  /* Black level control */
  {OV14825_BLC_CTRL08           , OV14825_BLC_CTRL08_DEFAULT            , "BLC_CTRL08"              , eReadWrite},
  {OV14825_BLC_CTRL09           , OV14825_BLC_CTRL09_DEFAULT            , "BLC_CTRL09"              , eReadWrite},

  /* ISP */
  {OV14825_ISP_CTRL00           , OV14825_ISP_CTRL00_DEFAULT            , "ISP_CTRL00"              , eReadWrite},
  {OV14825_ISP_CTRL01           , OV14825_ISP_CTRL01_DEFAULT            , "ISP_CTRL01"              , eReadWrite},
  {OV14825_ISP_CTRL02           , OV14825_ISP_CTRL02_DEFAULT            , "ISP_CTRL02"              , eReadWrite},
  {OV14825_ISP_CTRL1F           , OV14825_ISP_CTRL1F_DEFAULT            , "ISP_CTRL1F"              , eReadWrite},
  {OV14825_ISP_CTRL3D           , OV14825_ISP_CTRL3D_DEFAULT            , "ISP_CTRL3D"              , eReadWrite},
  {OV14825_ISP_CTRL41           , OV14825_ISP_CTRL41_DEFAULT            , "ISP_CTRL41"              , eReadWrite},
  {OV14825_ISP_RED_GAIN_2       , OV14825_ISP_RED_GAIN_2_DEFAULT        , "ISP_RED_GAIN_2"          , eReadOnly},
  {OV14825_ISP_RED_GAIN_1       , OV14825_ISP_RED_GAIN_1_DEFAULT        , "ISP_RED_GAIN_1"          , eReadOnly},
  {OV14825_ISP_GRN_GAIN_2       , OV14825_ISP_GRN_GAIN_2_DEFAULT        , "ISP_GRN_GAIN_2"          , eReadOnly},
  {OV14825_ISP_GRN_GAIN_1       , OV14825_ISP_GRN_GAIN_1_DEFAULT        , "ISP_GRN_GAIN_1"          , eReadOnly},
  {OV14825_ISP_BLU_GAIN_2       , OV14825_ISP_BLU_GAIN_2_DEFAULT        , "ISP_BLU_GAIN_2"          , eReadOnly},
  {OV14825_ISP_BLU_GAIN_1       , OV14825_ISP_BLU_GAIN_1_DEFAULT        , "ISP_BLU_GAIN_1"          , eReadOnly},

  /* AWB control */
  {OV14825_AWB_CTRL00           , OV14825_AWB_CTRL00_DEFAULT            , "AWB_CTRL00"              , eReadWrite},
  {OV14825_AWB_CTRL06           , OV14825_AWB_CTRL06_DEFAULT            , "AWB_CTRL06"              , eReadWrite},
  {OV14825_AWB_CTRL07           , OV14825_AWB_CTRL07_DEFAULT            , "AWB_CTRL07"              , eReadWrite},
  {OV14825_AWB_CTRL08           , OV14825_AWB_CTRL08_DEFAULT            , "AWB_CTRL08"              , eReadWrite},
  {OV14825_AWB_CTRL09           , OV14825_AWB_CTRL09_DEFAULT            , "AWB_CTRL09"              , eReadWrite},
  {OV14825_AWB_CTRL0A           , OV14825_AWB_CTRL0A_DEFAULT            , "AWB_CTRL0A"              , eReadWrite},
  {OV14825_AWB_CTRL0B           , OV14825_AWB_CTRL0B_DEFAULT            , "AWB_CTRL0B"              , eReadWrite},
  {OV14825_AWB_CTRL0C           , OV14825_AWB_CTRL0C_DEFAULT            , "AWB_CTRL0C"              , eReadWrite},
  {OV14825_AWB_CTRL0D           , OV14825_AWB_CTRL0D_DEFAULT            , "AWB_CTRL0D"              , eReadWrite},
  {OV14825_AWB_CTRL0E           , OV14825_AWB_CTRL0E_DEFAULT            , "AWB_CTRL0E"              , eReadWrite},
  {OV14825_AWB_CTRL0F           , OV14825_AWB_CTRL0F_DEFAULT            , "AWB_CTRL0F"              , eReadWrite},
  {OV14825_AWB_CTRL10           , OV14825_AWB_CTRL10_DEFAULT            , "AWB_CTRL10"              , eReadOnly},
  {OV14825_AWB_CTRL11           , OV14825_AWB_CTRL11_DEFAULT            , "AWB_CTRL11"              , eReadOnly},
  {OV14825_AWB_CTRL12           , OV14825_AWB_CTRL12_DEFAULT            , "AWB_CTRL12"              , eReadOnly},
  {OV14825_AWB_CTRL13           , OV14825_AWB_CTRL13_DEFAULT            , "AWB_CTRL13"              , eReadOnly},
  {OV14825_AWB_CTRL14           , OV14825_AWB_CTRL14_DEFAULT            , "AWB_CTRL14"              , eReadOnly},
  {OV14825_AWB_CTRL15           , OV14825_AWB_CTRL15_DEFAULT            , "AWB_CTRL15"              , eReadOnly},

  /* Average control */
  {OV14825_AVG_CTRL00           , OV14825_AVG_CTRL00_DEFAULT            , "AVG_CTRL00"              , eReadWrite},
  {OV14825_AVG_CTRL01           , OV14825_AVG_CTRL01_DEFAULT            , "AVG_CTRL01"              , eReadWrite},
  {OV14825_AVG_CTRL02           , OV14825_AVG_CTRL02_DEFAULT            , "AVG_CTRL02"              , eReadWrite},
  {OV14825_AVG_CTRL03           , OV14825_AVG_CTRL03_DEFAULT            , "AVG_CTRL03"              , eReadWrite},
  {OV14825_AVG_CTRL04           , OV14825_AVG_CTRL04_DEFAULT            , "AVG_CTRL04"              , eReadWrite},
  {OV14825_AVG_CTRL05           , OV14825_AVG_CTRL05_DEFAULT            , "AVG_CTRL05"              , eReadWrite},
  {OV14825_AVG_CTRL06           , OV14825_AVG_CTRL06_DEFAULT            , "AVG_CTRL06"              , eReadWrite},
  {OV14825_AVG_CTRL07           , OV14825_AVG_CTRL07_DEFAULT            , "AVG_CTRL07"              , eReadWrite},
  {OV14825_AVG_CTRL08           , OV14825_AVG_CTRL08_DEFAULT            , "AVG_CTRL08"              , eReadWrite},
  {OV14825_AVERAGE_CTRL10       , OV14825_AVERAGE_CTRL10_DEFAULT        , "AVERAGE_CTRL10"          , eReadOnly},

  /* lense shade control */
  {OV14825_LENC_CTRL00          , OV14825_LENC_CTRL00_DEFAULT           , "LENC_CTRL00"             , eReadWrite},
  {OV14825_LENC_CTRL01          , OV14825_LENC_CTRL01_DEFAULT           , "LENC_CTRL01"             , eReadWrite},
  {OV14825_LENC_CTRL02          , OV14825_LENC_CTRL02_DEFAULT           , "LENC_CTRL02"             , eReadWrite},
  {OV14825_LENC_CTRL03          , OV14825_LENC_CTRL03_DEFAULT           , "LENC_CTRL03"             , eReadWrite},
  {OV14825_LENC_CTRL04          , OV14825_LENC_CTRL04_DEFAULT           , "LENC_CTRL04"             , eReadWrite},
  {OV14825_LENC_CTRL05          , OV14825_LENC_CTRL05_DEFAULT           , "LENC_CTRL05"             , eReadWrite},
  {OV14825_LENC_CTRL06          , OV14825_LENC_CTRL06_DEFAULT           , "LENC_CTRL06"             , eReadWrite},
  {OV14825_LENC_CTRL07          , OV14825_LENC_CTRL07_DEFAULT           , "LENC_CTRL07"             , eReadWrite},
  {OV14825_LENC_CTRL87          , OV14825_LENC_CTRL87_DEFAULT           , "LENC_CTRL87"             , eReadWrite},

  /* ISP window registers */
  {OV14825_WIN_CTRL00              , OV14825_WIN_CTRL00_DEFAULT          , "WIN_CTRL00"              , eReadWrite},
  {OV14825_WIN_CTRL01              , OV14825_WIN_CTRL01_DEFAULT          , "WIN_CTRL01"              , eReadWrite},
  {OV14825_WIN_CTRL02              , OV14825_WIN_CTRL02_DEFAULT          , "WIN_CTRL02"              , eReadWrite},
  {OV14825_WIN_CTRL03              , OV14825_WIN_CTRL03_DEFAULT          , "WIN_CTRL03"              , eReadWrite},
  {OV14825_WIN_CTRL04              , OV14825_WIN_CTRL04_DEFAULT          , "WIN_CTRL04"              , eReadWrite},
  {OV14825_WIN_CTRL05              , OV14825_WIN_CTRL05_DEFAULT          , "WIN_CTRL05"              , eReadWrite},
  {OV14825_WIN_CTRL06              , OV14825_WIN_CTRL06_DEFAULT          , "WIN_CTRL06"              , eReadWrite},
  {OV14825_WIN_CTRL07              , OV14825_WIN_CTRL07_DEFAULT          , "WIN_CTRL07"              , eReadWrite},
  {OV14825_WIN_CTRL08              , OV14825_WIN_CTRL08_DEFAULT          , "WIN_CTRL08"              , eReadWrite},

  {0x0000                          , 0x00                                , "TableEnd"      , eTableEnd}
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
const Isi1x1FloatMatrix_t OV14825_KFactor =
{
    { 6.838349f }   // or 3.94f (to be checked)
};


// PCA matrix
const Isi3x2FloatMatrix_t OV14825_PCAMatrix =
{
    {
        -0.62791f, -0.13803f,  0.76595f,
        -0.52191f,  0.80474f, -0.28283f
    }
};


// mean values from SVD
const Isi3x1FloatMatrix_t OV14825_SVDMeanValue =
{
    {
        0.34165f,  0.37876f,  0.27959f
    }
};



/*****************************************************************************
 * Rg/Bg color space (clipping and out of range)
 *****************************************************************************/
// Center line of polygons {f_N0_Rg, f_N0_Bg, f_d}
const IsiLine_t OV14825_CenterLine =
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
const IsiAwbClipParm_t OV14825_AwbClipParm =
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
const IsiAwbGlobalFadeParm_t OV14825_AwbGlobalFadeParm =
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
const IsiAwbFade2Parm_t OV14825_AwbFade2Parm =
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

