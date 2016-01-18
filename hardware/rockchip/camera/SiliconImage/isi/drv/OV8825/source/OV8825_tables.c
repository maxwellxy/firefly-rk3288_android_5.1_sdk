//ov8825_tables.c
/*****************************************************************************/
/*!
 *  \file        OV8810_tables.c \n
 *  \version     1.0 \n
 *  \author      Meinicke \n
 *  \brief       Image-sensor-specific tables and other
 *               constant values/structures for OV8810. \n
 *
 *  \revision    $Revision: 803 $ \n
 *               $Author: $ \n
 *               $Date: 2010-02-26 16:35:22 +0100 (Fr, 26 Feb 2010) $ \n
 *               $Id: OV8810_tables.c 803 2010-02-26 15:35:22Z  $ \n
 */
/*  This is an unpublished work, the copyright in which vests in Silicon Image
 *  GmbH. The information contained herein is the property of Silicon Image GmbH
 *  and is supplied without liability for errors or omissions. No part may be
 *  reproduced or used expect as authorized by contract or other written
 *  permission. Copyright(c) Silicon Image GmbH, 2009, all rights reserved.
 */
/*****************************************************************************/
/*
#include "stdinc.h"

#if( OV8810_DRIVER_USAGE == USE_CAM_DRV_EN )
*/


#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"
#include "OV8825_MIPI_priv.h"


/*****************************************************************************
 * DEFINES
 *****************************************************************************/


/*****************************************************************************
 * GLOBALS
 *****************************************************************************/

// Image sensor register settings default values taken from data sheet OV8810_DS_1.1_SiliconImage.pdf.
// The settings may be altered by the code in IsiSetupSensor.
const IsiRegDescription_t OV8825_g_aRegDescription_onelane[] =
{
    //@@5.1.1.1 Initialization (Global Setting)
	//; Slave_ID=0x6c;
//{0x0103 ,0x01 ,"0x0100",eReadWrite}, software reset
	//; delay(5ms)
{0x3000 ,0x16,"0x0100",eReadWrite},
{0x3001 ,0x00,"0x0100",eReadWrite},
{0x3002 ,0x6c,"0x0100",eReadWrite},
{0x3003 ,0xce,"0x0100",eReadWrite},
{0x3004 ,0xd4,"0x0100",eReadWrite},
{0x3005 ,0x00,"0x0100",eReadWrite},
{0x3006 ,0x10,"0x0100",eReadWrite},
{0x3007 ,0x3b,"0x0100",eReadWrite},
{0x300d ,0x00,"0x0100",eReadWrite},
{0x301f ,0x09,"0x0100",eReadWrite},
{0x3020 ,0x01,"0x0100",eReadWrite},
{0x3010 ,0x00,"0x0100",eReadWrite},
{0x3011 ,0x00,"0x0100",eReadWrite},
{0x3012 ,0x80,"0x0100",eReadWrite},
{0x3013 ,0x39,"0x0100",eReadWrite},
{0x3018 ,0x00,"0x0100",eReadWrite},
{0x3104 ,0x20,"0x0100",eReadWrite},
{0x3106 ,0x15,"0x0100",eReadWrite},
{0x3300 ,0x00,"0x0100",eReadWrite},
{0x3500 ,0x00,"0x0100",eReadWrite},
{0x3501 ,0x00,"0x0100",eReadWrite},
{0x3502 ,0x00,"0x0100",eReadWrite},
{0x3503 ,0x07,"0x0100",eReadWrite},
{0x3509 ,0x10,"0x0100",eReadWrite},
{0x350b ,0x10,"0x0100",eReadWrite},
{0x3600 ,0x06,"0x0100",eReadWrite},
{0x3601 ,0x34,"0x0100",eReadWrite},
{0x3602 ,0x42,"0x0100",eReadWrite},
{0x3603 ,0x5c,"0x0100",eReadWrite},
{0x3604 ,0x98,"0x0100",eReadWrite},
{0x3605 ,0xf5,"0x0100",eReadWrite},
{0x3609 ,0xb4,"0x0100",eReadWrite},
{0x360a ,0x7c,"0x0100",eReadWrite},
{0x360b ,0xc9,"0x0100",eReadWrite},
{0x360c ,0x0b,"0x0100",eReadWrite},
{0x3612 ,0x00,"0x0100",eReadWrite},
{0x3613 ,0x02,"0x0100",eReadWrite},
{0x3614 ,0x0f,"0x0100",eReadWrite},
{0x3615 ,0x00,"0x0100",eReadWrite},
{0x3616 ,0x03,"0x0100",eReadWrite},
{0x3617 ,0xa1,"0x0100",eReadWrite},
{0x3618 ,0x00,"0x0100",eReadWrite},//vcm
{0x3619 ,0x00,"0x0100",eReadWrite},
{0x361a ,0xb0,"0x0100",eReadWrite},
{0x361b ,0x04,"0x0100",eReadWrite},
{0x361c ,0x07,"0x0100",eReadWrite},//vcm
{0x3700 ,0x20,"0x0100",eReadWrite},
{0x3701 ,0x44,"0x0100",eReadWrite},
{0x3702 ,0x50,"0x0100",eReadWrite},
{0x3703 ,0xcc,"0x0100",eReadWrite},
{0x3704 ,0x19,"0x0100",eReadWrite},
{0x3705 ,0x32,"0x0100",eReadWrite},
{0x3706 ,0x4b,"0x0100",eReadWrite},
{0x3707 ,0x63,"0x0100",eReadWrite},
{0x3708 ,0x84,"0x0100",eReadWrite},
{0x3709 ,0x40,"0x0100",eReadWrite},
{0x370a ,0x33,"0x0100",eReadWrite},
{0x370b ,0x01,"0x0100",eReadWrite},
{0x370c ,0x50,"0x0100",eReadWrite},
{0x370d ,0x00,"0x0100",eReadWrite},
{0x370e ,0x00,"0x0100",eReadWrite},
{0x3711 ,0x0f,"0x0100",eReadWrite},
{0x3712 ,0x9c,"0x0100",eReadWrite},
{0x3724 ,0x01,"0x0100",eReadWrite},
{0x3725 ,0x92,"0x0100",eReadWrite},
{0x3726 ,0x01,"0x0100",eReadWrite},
{0x3727 ,0xc7,"0x0100",eReadWrite},
{0x3800 ,0x00,"0x0100",eReadWrite},
{0x3801 ,0x00,"0x0100",eReadWrite},
{0x3802 ,0x00,"0x0100",eReadWrite},
{0x3803 ,0x00,"0x0100",eReadWrite},
{0x3804 ,0x0c,"0x0100",eReadWrite},
{0x3805 ,0xdf,"0x0100",eReadWrite},
{0x3806 ,0x09,"0x0100",eReadWrite},
{0x3807 ,0x9b,"0x0100",eReadWrite},
{0x3808 ,0x06,"0x0100",eReadWrite},
{0x3809 ,0x60,"0x0100",eReadWrite},
{0x380a ,0x04,"0x0100",eReadWrite},
{0x380b ,0xc8,"0x0100",eReadWrite},
{0x380c ,0x0d,"0x0100",eReadWrite},
{0x380d ,0xbc,"0x0100",eReadWrite},
{0x380e ,0x04,"0x0100",eReadWrite},
{0x380f ,0xf0,"0x0100",eReadWrite},
{0x3810 ,0x00,"0x0100",eReadWrite},
{0x3811 ,0x08,"0x0100",eReadWrite},
{0x3812 ,0x00,"0x0100",eReadWrite},
{0x3813 ,0x04,"0x0100",eReadWrite},
{0x3814 ,0x31,"0x0100",eReadWrite},
{0x3815 ,0x31,"0x0100",eReadWrite},
{0x3816 ,0x02,"0x0100",eReadWrite},
{0x3817 ,0x40,"0x0100",eReadWrite},
{0x3818 ,0x00,"0x0100",eReadWrite},
{0x3819 ,0x40,"0x0100",eReadWrite},
{0x3820 ,0x81,"0x0100",eReadWrite},
{0x3821 ,0x17,"0x0100",eReadWrite},
{0x3b1f ,0x00,"0x0100",eReadWrite},
{0x3d00 ,0x00,"0x0100",eReadWrite},
{0x3d01 ,0x00,"0x0100",eReadWrite},
{0x3d02 ,0x00,"0x0100",eReadWrite},
{0x3d03 ,0x00,"0x0100",eReadWrite},
{0x3d04 ,0x00,"0x0100",eReadWrite},
{0x3d05 ,0x00,"0x0100",eReadWrite},
{0x3d06 ,0x00,"0x0100",eReadWrite},
{0x3d07 ,0x00,"0x0100",eReadWrite},
{0x3d08 ,0x00,"0x0100",eReadWrite},
{0x3d09 ,0x00,"0x0100",eReadWrite},
{0x3d0a ,0x00,"0x0100",eReadWrite},
{0x3d0b ,0x00,"0x0100",eReadWrite},
{0x3d0c ,0x00,"0x0100",eReadWrite},
{0x3d0d ,0x00,"0x0100",eReadWrite},
{0x3d0e ,0x00,"0x0100",eReadWrite},
{0x3d0f ,0x00,"0x0100",eReadWrite},
{0x3d10 ,0x00,"0x0100",eReadWrite},
{0x3d11 ,0x00,"0x0100",eReadWrite},
{0x3d12 ,0x00,"0x0100",eReadWrite},
{0x3d13 ,0x00,"0x0100",eReadWrite},
{0x3d14 ,0x00,"0x0100",eReadWrite},
{0x3d15 ,0x00,"0x0100",eReadWrite},
{0x3d16 ,0x00,"0x0100",eReadWrite},
{0x3d17 ,0x00,"0x0100",eReadWrite},
{0x3d18 ,0x00,"0x0100",eReadWrite},
{0x3d19 ,0x00,"0x0100",eReadWrite},
{0x3d1a ,0x00,"0x0100",eReadWrite},
{0x3d1b ,0x00,"0x0100",eReadWrite},
{0x3d1c ,0x00,"0x0100",eReadWrite},
{0x3d1d ,0x00,"0x0100",eReadWrite},
{0x3d1e ,0x00,"0x0100",eReadWrite},
{0x3d1f ,0x00,"0x0100",eReadWrite},
{0x3d80 ,0x00,"0x0100",eReadWrite},
{0x3d81 ,0x00,"0x0100",eReadWrite},
{0x3d84 ,0x00,"0x0100",eReadWrite},
{0x3f00 ,0x00,"0x0100",eReadWrite},
{0x3f01 ,0xfc,"0x0100",eReadWrite},
{0x3f05 ,0x10,"0x0100",eReadWrite},
{0x3f06 ,0x00,"0x0100",eReadWrite},
{0x3f07 ,0x00,"0x0100",eReadWrite},
{0x4000 ,0x29,"0x0100",eReadWrite},
{0x4001 ,0x02,"0x0100",eReadWrite},
{0x4002 ,0x45,"0x0100",eReadWrite},
{0x4003 ,0x08,"0x0100",eReadWrite},
{0x4004 ,0x04,"0x0100",eReadWrite},
{0x4005 ,0x18,"0x0100",eReadWrite},
{0x404e ,0x37,"0x0100",eReadWrite},
{0x404f ,0x8f,"0x0100",eReadWrite},
{0x4300 ,0xff,"0x0100",eReadWrite},
{0x4303 ,0x00,"0x0100",eReadWrite},
{0x4304 ,0x08,"0x0100",eReadWrite},
{0x4307 ,0x00,"0x0100",eReadWrite},
{0x4600 ,0x04,"0x0100",eReadWrite},
{0x4601 ,0x00,"0x0100",eReadWrite},
{0x4602 ,0x30,"0x0100",eReadWrite},
{0x4800 ,0x04,"0x0100",eReadWrite},
{0x4801 ,0x0f,"0x0100",eReadWrite},
{0x4837 ,0x28,"0x0100",eReadWrite},
{0x4843 ,0x02,"0x0100",eReadWrite},
{0x5000 ,0x06,"0x0100",eReadWrite},
{0x5001 ,0x00,"0x0100",eReadWrite},
{0x5002 ,0x00,"0x0100",eReadWrite},
{0x5068 ,0x00,"0x0100",eReadWrite},
{0x506a ,0x00,"0x0100",eReadWrite},
{0x501f ,0x00,"0x0100",eReadWrite},
{0x5780 ,0xfc,"0x0100",eReadWrite},
{0x5c00 ,0x80,"0x0100",eReadWrite},
{0x5c01 ,0x00,"0x0100",eReadWrite},
{0x5c02 ,0x00,"0x0100",eReadWrite},
{0x5c03 ,0x00,"0x0100",eReadWrite},
{0x5c04 ,0x00,"0x0100",eReadWrite},
{0x5c05 ,0x00,"0x0100",eReadWrite},
{0x5c06 ,0x00,"0x0100",eReadWrite},
{0x5c07 ,0x80,"0x0100",eReadWrite},
{0x5c08 ,0x10,"0x0100",eReadWrite},
{0x6700 ,0x05,"0x0100",eReadWrite},
{0x6701 ,0x19,"0x0100",eReadWrite},
{0x6702 ,0xfd,"0x0100",eReadWrite},
{0x6703 ,0xd7,"0x0100",eReadWrite},
{0x6704 ,0xff,"0x0100",eReadWrite},
{0x6705 ,0xff,"0x0100",eReadWrite},
{0x6800 ,0x10,"0x0100",eReadWrite},
{0x6801 ,0x02,"0x0100",eReadWrite},
{0x6802 ,0x90,"0x0100",eReadWrite},
{0x6803 ,0x10,"0x0100",eReadWrite},
{0x6804 ,0x59,"0x0100",eReadWrite},
{0x6900 ,0x60,"0x0100",eReadWrite},
{0x6901 ,0x04,"0x0100",eReadWrite},
//{0x0100 ,0x01,"0x0100",eReadWrite},
{0x5800 ,0x0f,"0x0100",eReadWrite},
{0x5801 ,0x0d,"0x0100",eReadWrite},
{0x5802 ,0x09,"0x0100",eReadWrite},
{0x5803 ,0x0a,"0x0100",eReadWrite},
{0x5804 ,0x0d,"0x0100",eReadWrite},
{0x5805 ,0x14,"0x0100",eReadWrite},
{0x5806 ,0x0a,"0x0100",eReadWrite},
{0x5807 ,0x04,"0x0100",eReadWrite},
{0x5808 ,0x03,"0x0100",eReadWrite},
{0x5809 ,0x03,"0x0100",eReadWrite},
{0x580a ,0x05,"0x0100",eReadWrite},
{0x580b ,0x0a,"0x0100",eReadWrite},
{0x580c ,0x05,"0x0100",eReadWrite},
{0x580d ,0x02,"0x0100",eReadWrite},
{0x580e ,0x00,"0x0100",eReadWrite},
{0x580f ,0x00,"0x0100",eReadWrite},
{0x5810 ,0x03,"0x0100",eReadWrite},
{0x5811 ,0x05,"0x0100",eReadWrite},
{0x5812 ,0x09,"0x0100",eReadWrite},
{0x5813 ,0x03,"0x0100",eReadWrite},
{0x5814 ,0x01,"0x0100",eReadWrite},
{0x5815 ,0x01,"0x0100",eReadWrite},
{0x5816 ,0x04,"0x0100",eReadWrite},
{0x5817 ,0x09,"0x0100",eReadWrite},
{0x5818 ,0x09,"0x0100",eReadWrite},
{0x5819 ,0x08,"0x0100",eReadWrite},
{0x581a ,0x06,"0x0100",eReadWrite},
{0x581b ,0x06,"0x0100",eReadWrite},
{0x581c ,0x08,"0x0100",eReadWrite},
{0x581d ,0x06,"0x0100",eReadWrite},
{0x581e ,0x33,"0x0100",eReadWrite},
{0x581f ,0x11,"0x0100",eReadWrite},
{0x5820 ,0x0e,"0x0100",eReadWrite},
{0x5821 ,0x0f,"0x0100",eReadWrite},
{0x5822 ,0x11,"0x0100",eReadWrite},
{0x5823 ,0x3f,"0x0100",eReadWrite},
{0x5824 ,0x08,"0x0100",eReadWrite},
{0x5825 ,0x46,"0x0100",eReadWrite},
{0x5826 ,0x46,"0x0100",eReadWrite},
{0x5827 ,0x46,"0x0100",eReadWrite},
{0x5828 ,0x46,"0x0100",eReadWrite},
{0x5829 ,0x46,"0x0100",eReadWrite},
{0x582a ,0x42,"0x0100",eReadWrite},
{0x582b ,0x42,"0x0100",eReadWrite},
{0x582c ,0x44,"0x0100",eReadWrite},
{0x582d ,0x46,"0x0100",eReadWrite},
{0x582e ,0x46,"0x0100",eReadWrite},
{0x582f ,0x60,"0x0100",eReadWrite},
{0x5830 ,0x62,"0x0100",eReadWrite},
{0x5831 ,0x42,"0x0100",eReadWrite},
{0x5832 ,0x46,"0x0100",eReadWrite},
{0x5833 ,0x46,"0x0100",eReadWrite},
{0x5834 ,0x44,"0x0100",eReadWrite},
{0x5835 ,0x44,"0x0100",eReadWrite},
{0x5836 ,0x44,"0x0100",eReadWrite},
{0x5837 ,0x48,"0x0100",eReadWrite},
{0x5838 ,0x28,"0x0100",eReadWrite},
{0x5839 ,0x46,"0x0100",eReadWrite},
{0x583a ,0x48,"0x0100",eReadWrite},
{0x583b ,0x68,"0x0100",eReadWrite},
{0x583c ,0x28,"0x0100",eReadWrite},
{0x583d ,0xae,"0x0100",eReadWrite},
{0x5842 ,0x00,"0x0100",eReadWrite},
{0x5843 ,0xef,"0x0100",eReadWrite},
{0x5844 ,0x01,"0x0100",eReadWrite},
{0x5845 ,0x3f,"0x0100",eReadWrite},
{0x5846 ,0x01,"0x0100",eReadWrite},
{0x5847 ,0x3f,"0x0100",eReadWrite},
{0x5848 ,0x00,"0x0100",eReadWrite},
{0x5849 ,0xd5,"0x0100",eReadWrite},
//; MB
{0x3400 ,0x04,"0x0100",eReadWrite}, //; red h
{0x3401 ,0x00,"0x0100",eReadWrite}, //; red l
{0x3402 ,0x04,"0x0100",eReadWrite}, //; green h
{0x3403 ,0x00,"0x0100",eReadWrite}, //; green l
{0x3404 ,0x04,"0x0100",eReadWrite}, //; blue h
{0x3405 ,0x00,"0x0100",eReadWrite}, //; blue l
{0x3406 ,0x01,"0x0100",eReadWrite}, //; MWB manual
//; ISP
{0x5001 ,0x01,"0x0100",eReadWrite}, //; MWB on
{0x5000 ,0x06,"0x0100",eReadWrite}, //; LENC on, BPC on, WPC on
{0x301a ,0x71,"0x0100",eReadWrite},// ; MIPI stream off
{0x301c ,0xf4,"0x0100",eReadWrite},// ; clock in LP11 mode
//{0x0100 ,0x01,"0x0100",eReadWrite},// ; wake up from standby
{0x0000 ,0x00,"eTableEnd",eTableEnd}
};
const IsiRegDescription_t OV8825_g_aRegDescription_twolane[] =
{
    {0x3000,0x16,"0x3000",eReadWrite},
    {0x3001,0x00,"0x3001",eReadWrite},
    {0x3002,0x6c,"0x3002",eReadWrite},
    {0x3003,0xce,"0x3003",eReadWrite},
    {0x3004,0xd4,"0x3004",eReadWrite},
    {0x3005,0x00,"0x3005",eReadWrite},
    {0x3006,0x10,"0x3006",eReadWrite},
    {0x3007,0x3b,"0x3007",eReadWrite},
    {0x300d,0x00,"0x300d",eReadWrite},
    {0x301f,0x09,"0x301f",eReadWrite},
    {0x3020,0x01,"0x3020",eReadWrite},
    {0x3010,0x00,"0x3010",eReadWrite},
    {0x3011,0x01,"0x3011",eReadWrite},
    {0x3012,0x80,"0x3012",eReadWrite},
    {0x3013,0x39,"0x3013",eReadWrite},
    {0x3018,0x00,"0x3018",eReadWrite},
    {0x3104,0x20,"0x3104",eReadWrite},
    {0x3106,0x15,"0x3106",eReadWrite},
    {0x3300,0x00,"0x3300",eReadWrite},
    {0x3500,0x00,"0x3500",eReadWrite},
    {0x3501,0x4e,"0x3501",eReadWrite},
    {0x3502,0xa0,"0x3502",eReadWrite},
    {0x3503,0x07,"0x3503",eReadWrite},
    {0x3509,0x10,"0x3509",eReadWrite},
    {0x350b,0x1f,"0x350b",eReadWrite},
    {0x3600,0x06,"0x3600",eReadWrite},
    {0x3601,0x34,"0x3601",eReadWrite},
    {0x3602,0x42,"0x3602",eReadWrite},
    {0x3603,0x5c,"0x3603",eReadWrite},
    {0x3604,0x98,"0x3604",eReadWrite},
    {0x3605,0xf5,"0x3605",eReadWrite},
    {0x3609,0xb4,"0x3609",eReadWrite},
    {0x360a,0x7c,"0x360a",eReadWrite},
    {0x360b,0xc9,"0x360b",eReadWrite},
    {0x360c,0x0b,"0x360c",eReadWrite},
    {0x3612,0x00,"0x3612",eReadWrite},
    {0x3613,0x02,"0x3613",eReadWrite},
    {0x3614,0x0f,"0x3614",eReadWrite},
    {0x3615,0x00,"0x3615",eReadWrite},
    {0x3616,0x03,"0x3616",eReadWrite},
    {0x3617,0xa1,"0x3617",eReadWrite},
    
    {0x3618,0x00,"0x3618",eReadWrite}, //vcm 
    {0x3619,0x00,"0x3619",eReadWrite},
    {0x361a,0xb0,"0x361a",eReadWrite},
    {0x361b,0x04,"0x361b",eReadWrite},//vcm
    {0x361c,0x07,"0x361b",eReadWrite},//vcm

    {0x3700,0x20,"0x3700",eReadWrite},
    {0x3701,0x44,"0x3701",eReadWrite},
    {0x3702,0x50,"0x3702",eReadWrite},
    {0x3703,0xcc,"0x3703",eReadWrite},
    {0x3704,0x19,"0x3704",eReadWrite},
    {0x3705,0x32,"0x3705",eReadWrite},
    {0x3706,0x4b,"0x3706",eReadWrite},
    {0x3707,0x63,"0x3707",eReadWrite},
    {0x3708,0x84,"0x3708",eReadWrite},
    {0x3709,0x40,"0x3709",eReadWrite},
    {0x370a,0x33,"0x370a",eReadWrite},
    {0x370b,0x01,"0x370b",eReadWrite},
    {0x370c,0x50,"0x370c",eReadWrite},
    {0x370d,0x00,"0x370d",eReadWrite},
    {0x370e,0x00,"0x370e",eReadWrite},
    {0x3711,0x0f,"0x3711",eReadWrite},
    {0x3712,0x9c,"0x3712",eReadWrite},
    {0x3724,0x01,"0x3724",eReadWrite},
    {0x3725,0x92,"0x3725",eReadWrite},
    {0x3726,0x01,"0x3726",eReadWrite},
    {0x3727,0xc7,"0x3727",eReadWrite},
    {0x3800,0x00,"0x3800",eReadWrite},
    {0x3801,0x00,"0x3801",eReadWrite},
    {0x3802,0x00,"0x3802",eReadWrite},
    {0x3803,0x00,"0x3803",eReadWrite},
    {0x3804,0x0c,"0x3804",eReadWrite},
    {0x3805,0xdf,"0x3805",eReadWrite},
    {0x3806,0x09,"0x3806",eReadWrite},
    {0x3807,0x9b,"0x3807",eReadWrite},
    {0x3808,0x06,"0x3808",eReadWrite},
    {0x3809,0x60,"0x3809",eReadWrite},
    {0x380a,0x04,"0x380a",eReadWrite},
    {0x380b,0xc8,"0x380b",eReadWrite},
    {0x380c,0x0d,"0x380c",eReadWrite},
    {0x380d,0xbc,"0x380d",eReadWrite},
    {0x380e,0x04,"0x380e",eReadWrite},
    {0x380f,0xf0,"0x380f",eReadWrite},
    {0x3810,0x00,"0x3810",eReadWrite},
    {0x3811,0x08,"0x3811",eReadWrite},
    {0x3812,0x00,"0x3812",eReadWrite},
    {0x3813,0x04,"0x3813",eReadWrite},
    {0x3814,0x31,"0x3814",eReadWrite},
    {0x3815,0x31,"0x3815",eReadWrite},
    {0x3816,0x02,"0x3816",eReadWrite},
    {0x3817,0x40,"0x3817",eReadWrite},
    {0x3818,0x00,"0x3818",eReadWrite},
    {0x3819,0x40,"0x3819",eReadWrite},
    {0x3820,0x81,"0x3820",eReadWrite},
    {0x3821,0x17,"0x3821",eReadWrite},
    {0x3b1f,0x00,"0x3b1f",eReadWrite},
    {0x3d00,0x00,"0x3d00",eReadWrite},
    {0x3d01,0x00,"0x3d01",eReadWrite},
    {0x3d02,0x00,"0x3d02",eReadWrite},
    {0x3d03,0x00,"0x3d03",eReadWrite},
    {0x3d04,0x00,"0x3d04",eReadWrite},
    {0x3d05,0x00,"0x3d05",eReadWrite},
    {0x3d06,0x00,"0x3d06",eReadWrite},
    {0x3d07,0x00,"0x3d07",eReadWrite},
    {0x3d08,0x00,"0x3d08",eReadWrite},
    {0x3d09,0x00,"0x3d09",eReadWrite},
    {0x3d0a,0x00,"0x3d0a",eReadWrite},
    {0x3d0b,0x00,"0x3d0b",eReadWrite},
    {0x3d0c,0x00,"0x3d0c",eReadWrite},
    {0x3d0d,0x00,"0x3d0d",eReadWrite},
    {0x3d0e,0x00,"0x3d0e",eReadWrite},
    {0x3d0f,0x00,"0x3d0f",eReadWrite},
    {0x3d10,0x00,"0x3d10",eReadWrite},
    {0x3d11,0x00,"0x3d11",eReadWrite},
    {0x3d12,0x00,"0x3d12",eReadWrite},
    {0x3d13,0x00,"0x3d13",eReadWrite},
    {0x3d14,0x00,"0x3d14",eReadWrite},
    {0x3d15,0x00,"0x3d15",eReadWrite},
    {0x3d16,0x00,"0x3d16",eReadWrite},
    {0x3d17,0x00,"0x3d17",eReadWrite},
    {0x3d18,0x00,"0x3d18",eReadWrite},
    {0x3d19,0x00,"0x3d19",eReadWrite},
    {0x3d1a,0x00,"0x3d1a",eReadWrite},
    {0x3d1b,0x00,"0x3d1b",eReadWrite},
    {0x3d1c,0x00,"0x3d1c",eReadWrite},
    {0x3d1d,0x00,"0x3d1d",eReadWrite},
    {0x3d1e,0x00,"0x3d1e",eReadWrite},
    {0x3d1f,0x00,"0x3d1f",eReadWrite},
    {0x3d80,0x00,"0x3d80",eReadWrite},
    {0x3d81,0x00,"0x3d81",eReadWrite},
    {0x3d84,0x00,"0x3d84",eReadWrite},
    {0x3f00,0x00,"0x3f00",eReadWrite},
    {0x3f01,0xfc,"0x3f01",eReadWrite},
    {0x3f05,0x10,"0x3f05",eReadWrite},
    {0x3f06,0x00,"0x3f06",eReadWrite},
    {0x3f07,0x00,"0x3f07",eReadWrite},
    {0x4000,0x29,"0x4000",eReadWrite},
    {0x4001,0x02,"0x4001",eReadWrite},
    {0x4002,0x45,"0x4002",eReadWrite},
    {0x4003,0x08,"0x4003",eReadWrite},
    {0x4004,0x04,"0x4004",eReadWrite},
    {0x4005,0x18,"0x4005",eReadWrite},
    {0x404e,0x37,"0x404e",eReadWrite},
    {0x404f,0x8f,"0x404f",eReadWrite},
    {0x4300,0xff,"0x4300",eReadWrite},
    {0x4303,0x00,"0x4303",eReadWrite},
    {0x4304,0x08,"0x4304",eReadWrite},
    {0x4307,0x00,"0x4307",eReadWrite},
    {0x4600,0x04,"0x4600",eReadWrite},
    {0x4601,0x00,"0x4601",eReadWrite},
    {0x4602,0x30,"0x4602",eReadWrite},
    {0x4800,0x04,"0x4800",eReadWrite},
    {0x4801,0x0f,"0x4801",eReadWrite},
    {0x4837,0x28,"0x4837",eReadWrite},
    {0x4843,0x02,"0x4843",eReadWrite},
    {0x5000,0x06,"0x5000",eReadWrite},
    {0x5001,0x00,"0x5001",eReadWrite},
    {0x5002,0x00,"0x5002",eReadWrite},
    {0x5068,0x00,"0x5068",eReadWrite},
    {0x506a,0x00,"0x506a",eReadWrite},
    {0x501f,0x00,"0x501f",eReadWrite},
    {0x5780,0xfc,"0x5780",eReadWrite},
    {0x5c00,0x80,"0x5c00",eReadWrite},
    {0x5c01,0x00,"0x5c01",eReadWrite},
    {0x5c02,0x00,"0x5c02",eReadWrite},
    {0x5c03,0x00,"0x5c03",eReadWrite},
    {0x5c04,0x00,"0x5c04",eReadWrite},
    {0x5c05,0x00,"0x5c05",eReadWrite},
    {0x5c06,0x00,"0x5c06",eReadWrite},
    {0x5c07,0x80,"0x5c07",eReadWrite},
    {0x5c08,0x10,"0x5c08",eReadWrite},
    {0x6700,0x05,"0x6700",eReadWrite},
    {0x6701,0x19,"0x6701",eReadWrite},
    {0x6702,0xfd,"0x6702",eReadWrite},
    {0x6703,0xd7,"0x6703",eReadWrite},
    {0x6704,0xff,"0x6704",eReadWrite},
    {0x6705,0xff,"0x6705",eReadWrite},
    {0x6800,0x10,"0x6800",eReadWrite},
    {0x6801,0x02,"0x6801",eReadWrite},
    {0x6802,0x90,"0x6802",eReadWrite},
    {0x6803,0x10,"0x6803",eReadWrite},
    {0x6804,0x59,"0x6804",eReadWrite},
    {0x6900,0x60,"0x6900",eReadWrite},
    {0x6901,0x04,"0x6901",eReadWrite},
    {0x5800,0x0f,"0x5800",eReadWrite},
    {0x5801,0x0d,"0x5801",eReadWrite},
    {0x5802,0x09,"0x5802",eReadWrite},
    {0x5803,0x0a,"0x5803",eReadWrite},
    {0x5804,0x0d,"0x5804",eReadWrite},
    {0x5805,0x14,"0x5805",eReadWrite},
    {0x5806,0x0a,"0x5806",eReadWrite},
    {0x5807,0x04,"0x5807",eReadWrite},
    {0x5808,0x03,"0x5808",eReadWrite},
    {0x5809,0x03,"0x5809",eReadWrite},
    {0x580a,0x05,"0x580a",eReadWrite},
    {0x580b,0x0a,"0x580b",eReadWrite},
    {0x580c,0x05,"0x580c",eReadWrite},
    {0x580d,0x02,"0x580d",eReadWrite},
    {0x580e,0x00,"0x580e",eReadWrite},
    {0x580f,0x00,"0x580f",eReadWrite},
    {0x5810,0x03,"0x5810",eReadWrite},
    {0x5811,0x05,"0x5811",eReadWrite},
    {0x5812,0x09,"0x5812",eReadWrite},
    {0x5813,0x03,"0x5813",eReadWrite},
    {0x5814,0x01,"0x5814",eReadWrite},
    {0x5815,0x01,"0x5815",eReadWrite},
    {0x5816,0x04,"0x5816",eReadWrite},
    {0x5817,0x09,"0x5817",eReadWrite},
    {0x5818,0x09,"0x5818",eReadWrite},
    {0x5819,0x08,"0x5819",eReadWrite},
    {0x581a,0x06,"0x581a",eReadWrite},
    {0x581b,0x06,"0x581b",eReadWrite},
    {0x581c,0x08,"0x581c",eReadWrite},
    {0x581d,0x06,"0x581d",eReadWrite},
    {0x581e,0x33,"0x581e",eReadWrite},
    {0x581f,0x11,"0x581f",eReadWrite},
    {0x5820,0x0e,"0x5820",eReadWrite},
    {0x5821,0x0f,"0x5821",eReadWrite},
    {0x5822,0x11,"0x5822",eReadWrite},
    {0x5823,0x3f,"0x5823",eReadWrite},
    {0x5824,0x08,"0x5824",eReadWrite},
    {0x5825,0x46,"0x5825",eReadWrite},
    {0x5826,0x46,"0x5826",eReadWrite},
    {0x5827,0x46,"0x5827",eReadWrite},
    {0x5828,0x46,"0x5828",eReadWrite},
    {0x5829,0x46,"0x5829",eReadWrite},
    {0x582a,0x42,"0x582a",eReadWrite},
    {0x582b,0x42,"0x582b",eReadWrite},
    {0x582c,0x44,"0x582c",eReadWrite},
    {0x582d,0x46,"0x582d",eReadWrite},
    {0x582e,0x46,"0x582e",eReadWrite},
    {0x582f,0x60,"0x582f",eReadWrite},
    {0x5830,0x62,"0x5830",eReadWrite},
    {0x5831,0x42,"0x5831",eReadWrite},
    {0x5832,0x46,"0x5832",eReadWrite},
    {0x5833,0x46,"0x5833",eReadWrite},
    {0x5834,0x44,"0x5834",eReadWrite},
    {0x5835,0x44,"0x5835",eReadWrite},
    {0x5836,0x44,"0x5836",eReadWrite},
    {0x5837,0x48,"0x5837",eReadWrite},
    {0x5838,0x28,"0x5838",eReadWrite},
    {0x5839,0x46,"0x5839",eReadWrite},
    {0x583a,0x48,"0x583a",eReadWrite},
    {0x583b,0x68,"0x583b",eReadWrite},
    {0x583c,0x28,"0x583c",eReadWrite},
    {0x583d,0xae,"0x583d",eReadWrite},
    {0x5842,0x00,"0x5842",eReadWrite},
    {0x5843,0xef,"0x5843",eReadWrite},
    {0x5844,0x01,"0x5844",eReadWrite},
    {0x5845,0x3f,"0x5845",eReadWrite},
    {0x5846,0x01,"0x5846",eReadWrite},
    {0x5847,0x3f,"0x5847",eReadWrite},
    {0x5848,0x00,"0x5848",eReadWrite},
    {0x5849,0xd5,"0x5849",eReadWrite},    
    
    {0x3400,0x04,"0x3400",eReadWrite},//red,0xh,
    {0x3401,0x00,"0x3401",eReadWrite},//red,0xl,
    {0x3402,0x04,"0x3402",eReadWrite},//green,0xh
    {0x3403,0x00,"0x3403",eReadWrite},//green,0xl
    {0x3404,0x04,"0x3404",eReadWrite},//blue,0xh
    {0x3405,0x00,"0x3405",eReadWrite},//blue,0xl
    {0x3406,0x01,"0x3406",eReadWrite},//MWB,0xmanual
    {0x5001,0x01,"0x5001",eReadWrite},//MWB,0xon
    {0x5000,0x06,"0x5000",eReadWrite},//LENC,0xon,,0xBPC,0xon,,0xWPC,0xon
    {0x301a,0x71,"0x301a",eReadWrite},//MIPI,0xstream,0xof
    {0x301c,0xf4,"0x301c",eReadWrite},//clock,0xin,0xLP11,0xmode
    //{0x0100,0x01,"0x0100",eReadWrite},//wake,0xup,0xfrom,0xstandby
    {0x0000,0x00,"end",eTableEnd}
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
const Isi1x1FloatMatrix_t OV8825_KFactor =
{
    { 6.838349f }   // or 3.94f (to be checked)
};


// PCA matrix
const Isi3x2FloatMatrix_t OV8825_PCAMatrix =
{
    {
        -0.62791f, -0.13803f,  0.76595f,
        -0.52191f,  0.80474f, -0.28283f
    }
};


// mean values from SVD
const Isi3x1FloatMatrix_t OV8825_SVDMeanValue =
{
    {
        0.34165f,  0.37876f,  0.27959f
    }
};



/*****************************************************************************
 * Rg/Bg color space (clipping and out of range)
 *****************************************************************************/
// Center line of polygons {f_N0_Rg, f_N0_Bg, f_d}
const IsiLine_t OV8825_CenterLine =
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
const IsiAwbClipParm_t OV8825_AwbClipParm =
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
const IsiAwbGlobalFadeParm_t OV8825_AwbGlobalFadeParm =
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
const IsiAwbFade2Parm_t OV8825_AwbFade2Parm =
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

