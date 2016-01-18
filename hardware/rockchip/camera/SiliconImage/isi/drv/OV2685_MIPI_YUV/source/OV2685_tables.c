#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"
#include "OV2685_priv.h"


/*****************************************************************************
 * DEFINES
 *****************************************************************************/


/*****************************************************************************
 * GLOBALS
 *****************************************************************************/

// Image sensor register settings default values taken from data sheet OV8810_DS_1.1_SiliconImage.pdf.
// The settings may be altered by the code in IsiSetupSensor.
const IsiRegDescription_t OV2685_g_aRegDescription[] =
{
	
	
	//{0x0103,0x01,"",eReadWrite}, //software reset																				  
	{0x3002,0x00,"",eReadWrite}, //gpio input, vsync input, fsin input															  
	{0x3016,0x1c,"",eReadWrite}, //drive strength of low speed = 1x, bypass latch of hs_enable									  
	{0x3018,0x40,"",eReadWrite}, //0x44:MIPI 1 lane, 10-bit mode 																	  
	{0x301d,0xf0,"",eReadWrite}, //enable clocks																				  
	{0x3020,0x20,"",eReadWrite}, //output raw																					  
	{0x3082,0x2c,"",eReadWrite}, //PLL																							  
	{0x3083,0x03,"",eReadWrite}, //PLL																							  
	{0x3084,0x0f,"",eReadWrite}, //PLL																							  
	{0x3085,0x03,"",eReadWrite}, //PLL																							  
	{0x3086,0x00,"",eReadWrite}, //PLL																							  
	{0x3087,0x00,"",eReadWrite}, //PLL																							  
	{0x3501,0x26,"",eReadWrite}, //exposure M																					  
	{0x3502,0x40,"",eReadWrite}, //exposure L																					  
	{0x3503,0x03,"",eReadWrite}, //AGC manual, AEC manual																		  
	{0x350b,0x36,"",eReadWrite}, //Gain L																						  
	{0x3600,0xb4,"",eReadWrite}, //analog conrtol																				  
	{0x3603,0x35,"",eReadWrite}, // 																							  
	{0x3604,0x24,"",eReadWrite}, // 																							  
	{0x3605,0x00,"",eReadWrite}, // 																							  
	{0x3620,0x24,"",eReadWrite}, //26 ; 																						  
	{0x3621,0x34,"",eReadWrite}, //37 ; 																						  
	{0x3622,0x03,"",eReadWrite}, //04 ; 																						  
	{0x3628,0x10,"",eReadWrite}, //analog control																				  
	{0x3705,0x3c,"",eReadWrite}, //sensor control																				  
	{0x370a,0x23,"",eReadWrite}, // 																							  
	{0x370c,0x50,"",eReadWrite}, // 																							  
	{0x370d,0x00,"",eReadWrite}, //c0 ; 																						  
	{0x3717,0x58,"",eReadWrite}, // 																							  
	{0x3718,0x80,"",eReadWrite}, //88 ; 																						  
	{0x3720,0x00,"",eReadWrite}, // 																							  
	{0x3721,0x09,"",eReadWrite}, //00 ; 																						  
	{0x3722,0x0b,"",eReadWrite}, //00 ; 																						  
	{0x3723,0x48,"",eReadWrite}, //00 ; 																						  
	{0x3738,0x99,"",eReadWrite}, //00 ; sensor control																			  
	{0x3781,0x80,"",eReadWrite}, //PSRAM																						  
	{0x3784,0x0c,"",eReadWrite}, // 																							  
	{0x3789,0x60,"",eReadWrite}, //PSRAM																						  
	{0x3800,0x00,"",eReadWrite}, //x start H																					  
	{0x3801,0x00,"",eReadWrite}, //x start L																					  
	{0x3802,0x00,"",eReadWrite}, //y start H																					  
	{0x3803,0x00,"",eReadWrite}, //y start L																					  
	{0x3804,0x06,"",eReadWrite}, //x end H																						  
	{0x3805,0x4f,"",eReadWrite}, //x end L																						  
	{0x3806,0x04,"",eReadWrite}, //y end H																						  
	{0x3807,0xbf,"",eReadWrite}, //y end L																						  
	{0x3808,0x03,"",eReadWrite}, //x output size H																				  
	{0x3809,0x20,"",eReadWrite}, //x output size L																				  
	{0x380a,0x02,"",eReadWrite}, //y output size H																				  
	{0x380b,0x58,"",eReadWrite}, //y output size L																				  
	{0x380c,0x06,"",eReadWrite}, //HTS H																						  
	{0x380d,0xac,"",eReadWrite}, //HTS L																						  
	{0x380e,0x02,"",eReadWrite}, //VTS H																						  
	{0x380f,0x84,"",eReadWrite}, //VTS L																						  
	{0x3810,0x00,"",eReadWrite}, //ISP x win H																					  
	{0x3811,0x04,"",eReadWrite}, //ISP x win L																					  
	{0x3812,0x00,"",eReadWrite}, //ISP y win H																					  
	{0x3813,0x04,"",eReadWrite}, //ISP y win L																					  
	{0x3814,0x31,"",eReadWrite}, //x inc																						  
	{0x3815,0x31,"",eReadWrite}, //y inc																						  
	{0x3819,0x04,"",eReadWrite}, //Vsync end row L																				  
	{0x3820,0xc2,"",eReadWrite}, //vsub48_blc on, vflip_blc on, vbinf on,														  
	{0x3821,0x01,"",eReadWrite}, //hbin on																						  
	{0x3820,0xc2,"",eReadWrite}, //vsub48_blc on, vflip_blc on, vbinf on,														  
	{0x3821,0x05,"",eReadWrite}, //Mirror on, hbin on																			  
	{0x3820,0xc6,"",eReadWrite}, //vsub48_blc on, vflip_blc on, Flip on, vbinf on,												  
	{0x3821,0x01,"",eReadWrite}, //hbin on																						  
	{0x3820,0xc6,"",eReadWrite}, //vsub48_blc on, vflip_blc on, Flip on, vbinf on,												  
	{0x3821,0x05,"",eReadWrite}, //Mirror on, hbin on																			  
	{0x382a,0x08,"",eReadWrite}, //auto VTS 																					  
																																  
	{0x3a00,0x41,"",eReadWrite}, //night mode off, band enable																	  
	{0x3a02,0x90,"",eReadWrite}, //50Hz 																						  
	{0x3a03,0x4e,"",eReadWrite}, //AEC target H 																				  
	{0x3a04,0x40,"",eReadWrite}, //AEC target L 																				  
	{0x3a06,0x00,"",eReadWrite}, //B50 H																						  
	{0x3a07,0xc1,"",eReadWrite}, //B50 L																						  
	{0x3a08,0x00,"",eReadWrite}, //B60 H																						  
	{0x3a09,0xa1,"",eReadWrite}, //B60 L																						  
	{0x3a0a,0x07,"",eReadWrite}, //max exp 50 H 																				  
	{0x3a0b,0x8a,"",eReadWrite}, //max exp 50 L, 10 band																		  
	{0x3a0c,0x07,"",eReadWrite}, //max exp 60 H 																				  
	{0x3a0d,0x8c,"",eReadWrite}, //max exp 60 L, 12 band																		  
	{0x3a0e,0x02,"",eReadWrite}, //VTS band 50 H																				  
	{0x3a0f,0x43,"",eReadWrite}, //VTS band 50 L																				  
	{0x3a10,0x02,"",eReadWrite}, //VTS band 60 H																				  
	{0x3a11,0x84,"",eReadWrite}, //VTS band 60 L																				  
	{0x3a13,0xf8,"",eReadWrite}, //gain ceiling = 15.5x 																		  
	{0x4000,0x81,"",eReadWrite}, //avg_weight = 8, mf_en on 																	  
	{0x4001,0x40,"",eReadWrite}, //format_trig_beh on																			  
	{0x4002,0x00,"",eReadWrite}, //blc target																					  
	{0x4003,0x10,"",eReadWrite}, //blc target																					  
	{0x4008,0x00,"",eReadWrite}, //bl_start 																					  
	{0x4009,0x03,"",eReadWrite}, //bl_end																						  
	{0x4300,0x30,"",eReadWrite}, //YUV 422																						  
	{0x430e,0x00,"",eReadWrite}, // 																							  
	{0x4602,0x02,"",eReadWrite}, //VFIFO R2, frame reset enable 																  
	{0x4837,0x1e,"",eReadWrite}, //MIPI global timing																			  
	{0x5000,0xff,"",eReadWrite}, //lenc_en, awb_gain_en, lcd_en, avg_en, bc_en, WC_en, blc_en									  
	{0x5001,0x05,"",eReadWrite}, //auto blc offset, avg after LCD																  
	{0x5002,0x32,"",eReadWrite}, //dpc_href_s, sof_sel, bias_plus																  
	{0x5003,0x10,"",eReadWrite}, //bias_man 																					  
	{0x5004,0xff,"",eReadWrite}, //uv_dsn_en, rgb_dns_en, gamma_en, cmx_en, cip_en, raw_dns_en, strech_en, awb_en				  
	{0x5005,0x12,"",eReadWrite}, //sde_en, rgb2yuv_en																			  
	{0x0100,0x00,"",eReadWrite}, //wake up																						  
	{0x0101,0x01,"",eReadWrite}, // 																							  
	{0x1000,0x01,"",eReadWrite}, // 																							  
	{0x0129,0x10,"",eReadWrite}, // 																							  
																																  
	{0x5180,0xf4,"",eReadWrite}, //AWB																							  
	{0x5181,0x11,"",eReadWrite}, // 																							  
	{0x5182,0x41,"",eReadWrite}, // 																							  
	{0x5183,0x42,"",eReadWrite}, // 																							  
	{0x5184,0x82,"",eReadWrite}, //78; cwf_x																					  
	{0x5185,0x62,"",eReadWrite}, //cwf_y																						  
	{0x5186,0x86,"",eReadWrite}, //kx(cwf 2 a)x2y																				  
	{0x5187,0xd0,"",eReadWrite}, //Ky(cwf 2 day)y2x 																			  
	{0x5188,0x10,"",eReadWrite}, //cwf range																					  
	{0x5189,0x0e,"",eReadWrite}, //a range																						  
	{0x518a,0x20,"",eReadWrite}, //day range																					  
	{0x518b,0x4f,"",eReadWrite}, //day limit																					  
	{0x518c,0x3c,"",eReadWrite}, //a limit																						  
	{0x518d,0xf8,"",eReadWrite}, // 																							  
	{0x518e,0x04,"",eReadWrite}, // 																							  
	{0x518f,0x7f,"",eReadWrite}, // 																							  
	{0x5190,0x40,"",eReadWrite}, // 																							  
	{0x5191,0x5f,"",eReadWrite}, // 																							  
	{0x5192,0x40,"",eReadWrite}, // 																							  
	{0x5193,0xff,"",eReadWrite}, // 																							  
	{0x5194,0x40,"",eReadWrite}, // 																							  
	{0x5195,0x07,"",eReadWrite}, // 																							  
	{0x5196,0x99,"",eReadWrite}, // 																							  
	{0x5197,0x04,"",eReadWrite}, // 																							  
	{0x5198,0x00,"",eReadWrite}, // 																							  
	{0x5199,0x06,"",eReadWrite}, // 																							  
	{0x519a,0x61,"",eReadWrite}, // 																							  
	{0x519b,0x04,"",eReadWrite}, //AWB																							  
	{0x5200,0x09,"",eReadWrite}, //stretch minimum = 3096, auto enable															  
	{0x5201,0x00,"",eReadWrite}, //stretch min low level																		  
	{0x5202,0x06,"",eReadWrite}, //stretch min low level																		  
	{0x5203,0x20,"",eReadWrite}, //stretch min high level																		  
	{0x5204,0x41,"",eReadWrite}, //stretch step2, step1 																		  
	{0x5205,0x16,"",eReadWrite}, //stretch current low level																	  
	{0x5206,0x00,"",eReadWrite}, //stretch current high level L 																  
	{0x5207,0x05,"",eReadWrite}, //stretch current high level H 																  
	{0x520b,0x30,"",eReadWrite}, //stretch_thres1 L 																			  
	{0x520c,0x75,"",eReadWrite}, //stretch_thres1 M 																			  
	{0x520d,0x00,"",eReadWrite}, //stretch_thres1 H 																			  
	{0x520e,0x30,"",eReadWrite}, //stretch_thres2 L 																			  
	{0x520f,0x75,"",eReadWrite}, //stretch_thres2 M 																			  
	{0x5210,0x00,"",eReadWrite}, //stretch_thres2 H 																			  
																																  
	{0x5280,0x15,"",eReadWrite}, //m_nNoise YSlop = 5, Parameter noise and edgethre calculated by noise list					  
	{0x5281,0x06,"",eReadWrite}, //m_nNoiseList[0]																				  
	{0x5282,0x06,"",eReadWrite}, //m_nNoiseList[1]																				  
	{0x5283,0x08,"",eReadWrite}, //m_nNoiseList[2]																				  
	{0x5284,0x1c,"",eReadWrite}, //m_nNoiseList[3]																				  
	{0x5285,0x1c,"",eReadWrite}, //m_nNoiseList[4]																				  
	{0x5286,0x20,"",eReadWrite}, //m_nNoiseList[5]																				  
	{0x5287,0x10,"",eReadWrite}, //m_nMaxEdgeGThre																				  
																																  
	{0x5300,0xc5,"",eReadWrite}, //m_bColorEdgeEnable, m_bAntiAlasing on, m_nNoise YSlop = 5									  
	{0x5301,0xa0,"",eReadWrite}, //m_nSharpenSlop = 1																			  
	{0x5302,0x06,"",eReadWrite}, //m_nNoiseList[0]																				  
	{0x5303,0x08,"",eReadWrite}, //m_nNoiseList[1]																				  
	{0x5304,0x10,"",eReadWrite}, //m_nNoiseList[2]																				  
	{0x5305,0x20,"",eReadWrite}, //m_nNoiseList[3]																				  
	{0x5306,0x30,"",eReadWrite}, //m_nNoiseList[4]																				  
	{0x5307,0x60,"",eReadWrite}, //m_nNoiseList[5]																				  
																																  
	{0x5308,0x32,"",eReadWrite}, //m_nMaxSarpenGain, m_nMinSharpenGain															  
	{0x5309,0x00,"",eReadWrite}, //m_nMinSharpen																				  
	{0x530a,0x2a,"",eReadWrite}, //m_nMaxSharpen																				  
	{0x530b,0x02,"",eReadWrite}, //m_nMinDetail 																				  
	{0x530c,0x02,"",eReadWrite}, //m_nMaxDetail 																				  
	{0x530d,0x00,"",eReadWrite}, //m_nDetailRatioList[0]																		  
	{0x530e,0x0c,"",eReadWrite}, //m_nDetailRatioList[1]																		  
	{0x530f,0x14,"",eReadWrite}, //m_nDetailRatioList[2]																		  
	{0x5310,0x1a,"",eReadWrite}, //m_nSharpenNegEdgeRatio																		  
	{0x5311,0x20,"",eReadWrite}, //m_nClrEdgeShT1																				  
	{0x5312,0x80,"",eReadWrite}, //m_nClrEdgeShT2																				  
	{0x5313,0x4b,"",eReadWrite}, //m_nClrEdgeShpSlop																			  
																																  
	{0x5380,0x01,"",eReadWrite}, //nCCM_D[0][0] H																				  
	{0x5381,0x83,"",eReadWrite}, //nCCM_D[0][0] L																				  
	{0x5382,0x00,"",eReadWrite}, //nCCM_D[0][1] H																				  
	{0x5383,0x1f,"",eReadWrite}, //nCCM_D[0][1] L																				  
	{0x5384,0x00,"",eReadWrite}, //nCCM_D[1][0] H																				  
	{0x5385,0x88,"",eReadWrite}, //nCCM_D[1][0] L																				  
	{0x5386,0x00,"",eReadWrite}, //nCCM_D[1][1] H																				  
	{0x5387,0x82,"",eReadWrite}, //nCCM_D[1][1] L																				  
	{0x5388,0x00,"",eReadWrite}, //nCCM_D[2][0] H																				  
	{0x5389,0x40,"",eReadWrite}, //nCCM_D[2][0] L																				  
	{0x538a,0x01,"",eReadWrite}, //nCCM_D[2][1] H																				  
	{0x538b,0xb9,"",eReadWrite}, //nCCM_D[2][1] L																				  
	{0x538c,0x10,"",eReadWrite}, //Sing bit [2][1], [2][0], [1][1], [1][0], [0][1], [0][0]										  
																																  
	{0x5400,0x0d,"",eReadWrite}, //m_pCurveYList[0] 																			  
	{0x5401,0x1a,"",eReadWrite}, //m_pCurveYList[1] 																			  
	{0x5402,0x32,"",eReadWrite}, //m_pCurveYList[2] 																			  
	{0x5403,0x59,"",eReadWrite}, //m_pCurveYList[3] 																			  
	{0x5404,0x68,"",eReadWrite}, //m_pCurveYList[4] 																			  
	{0x5405,0x76,"",eReadWrite}, //m_pCurveYList[5] 																			  
	{0x5406,0x82,"",eReadWrite}, //m_pCurveYList[6] 																			  
	{0x5407,0x8c,"",eReadWrite}, //m_pCurveYList[7] 																			  
	{0x5408,0x94,"",eReadWrite}, //m_pCurveYList[8] 																			  
	{0x5409,0x9c,"",eReadWrite}, //m_pCurveYList[9] 																			  
	{0x540a,0xa9,"",eReadWrite}, //m_pCurveYList[10]																			  
	{0x540b,0xb6,"",eReadWrite}, //m_pCurveYList[11]																			  
	{0x540c,0xcc,"",eReadWrite}, //m_pCurveYList[12]																			  
	{0x540d,0xdd,"",eReadWrite}, //m_pCurveYList[13]																			  
	{0x540e,0xeb,"",eReadWrite}, //m_pCurveYList[14]																			  
	{0x540f,0xa0,"",eReadWrite}, //m_nMaxShadowHGain																			  
	{0x5410,0x6e,"",eReadWrite}, //m_nMidTongHGain																				  
	{0x5411,0x06,"",eReadWrite}, //m_nHighLightHGain																			  
																																  
	{0x5480,0x19,"",eReadWrite}, //m_nShadowExtraNoise = 12, m_bSmoothYEnable													  
	{0x5481,0x00,"",eReadWrite}, //m_nNoiseYList[1], m_nNoiseYList[0]															  
	{0x5482,0x09,"",eReadWrite}, //m_nNoiseYList[3], m_nNoiseYList[2]															  
	{0x5483,0x12,"",eReadWrite}, //m_nNoiseYList[5], m_nNoiseYList[4]															  
	{0x5484,0x04,"",eReadWrite}, //m_nNoiseUVList[0]																			  
	{0x5485,0x06,"",eReadWrite}, //m_nNoiseUVList[1]																			  
	{0x5486,0x08,"",eReadWrite}, //m_nNoiseUVList[2]																			  
	{0x5487,0x0c,"",eReadWrite}, //m_nNoiseUVList[3]																			  
	{0x5488,0x10,"",eReadWrite}, //m_nNoiseUVList[4]																			  
	{0x5489,0x18,"",eReadWrite}, //m_nNoiseUVList[5]																			  
																																  
	{0x5500,0x00,"",eReadWrite}, //m_nNoiseList[0]																				  
	{0x5501,0x01,"",eReadWrite}, //m_nNoiseList[1]																				  
	{0x5502,0x02,"",eReadWrite}, //m_nNoiseList[2]																				  
	{0x5503,0x03,"",eReadWrite}, //m_nNoiseList[3]																				  
	{0x5504,0x04,"",eReadWrite}, //m_nNoiseList[4]																				  
	{0x5505,0x05,"",eReadWrite}, //m_nNoiseList[5]																				  
	{0x5506,0x00,"",eReadWrite}, //uuv_dns_psra_man dis, m_nShadowExtraNoise = 0												  
																																  
	{0x5600,0x06,"",eReadWrite}, //fixy off, neg off, gray off, fix_v off, fix_u off, contrast_en, saturation on				  
	{0x5603,0x40,"",eReadWrite}, //sat U																						  
	{0x5604,0x20,"",eReadWrite}, //sat V																						  
	{0x5609,0x10,"",eReadWrite}, //uvadj_th1																					  
	{0x560a,0x40,"",eReadWrite}, //uvadj_th2																					  
	{0x560b,0x00,"",eReadWrite}, // 																							  
																																  
	{0x5800,0x03,"",eReadWrite}, //red x0 H 																					  
	{0x5801,0x10,"",eReadWrite}, //red x0 L 																					  
	{0x5802,0x02,"",eReadWrite}, //red y0 H 																					  
	{0x5803,0x68,"",eReadWrite}, //red y0 L 																					  
	{0x5804,0x2a,"",eReadWrite}, //red a1																						  
	{0x5805,0x05,"",eReadWrite}, //red a2																						  
	{0x5806,0x12,"",eReadWrite}, //red b1																						  
	{0x5807,0x05,"",eReadWrite}, //red b2																						  
	{0x5808,0x03,"",eReadWrite}, //green x0 H																					  
	{0x5809,0x38,"",eReadWrite}, //green x0 L																					  
	{0x580a,0x02,"",eReadWrite}, //green y0 H																					  
	{0x580b,0x68,"",eReadWrite}, //green y0 L																					  
	{0x580c,0x20,"",eReadWrite}, //green a1 																					  
	{0x580d,0x05,"",eReadWrite}, //green a2 																					  
	{0x580e,0x52,"",eReadWrite}, //green b1 																					  
	{0x580f,0x06,"",eReadWrite}, //green b2 																					  
	{0x5810,0x03,"",eReadWrite}, //blue x0 H																					  
	{0x5811,0x10,"",eReadWrite}, //blue x0 L																					  
	{0x5812,0x02,"",eReadWrite}, //blue y0 H																					  
	{0x5813,0x7c,"",eReadWrite}, //blue y0 L																					  
	{0x5814,0x1c,"",eReadWrite}, //bule a1																						  
	{0x5815,0x05,"",eReadWrite}, //blue a2																						  
	{0x5816,0x42,"",eReadWrite}, //blue b1																						  
	{0x5817,0x06,"",eReadWrite}, //blue b2																						  
	{0x5818,0x0d,"",eReadWrite}, //rst_seed on, md_en, coef_m off, gcoef_en 													  
	{0x5819,0x10,"",eReadWrite}, //lenc_coef_th 																				  
	{0x581a,0x04,"",eReadWrite}, //lenc_gain_thre1																				  
	{0x581b,0x08,"",eReadWrite}, //lenc_gain_thre2																				  
	{0x3503,0x00,"",eReadWrite}, //AEC auto, AGC auto																			  
    {0x0000 ,0x00,"eTableEnd",eTableEnd}

};

const IsiRegDescription_t OV2685_g_svga[] =
{

	#if 1  //15fps
	{0x4200,0x0f,"",eReadWrite}, // stream off											
	{0x3501,0x26,"",eReadWrite}, // exposure M											
	{0x3502,0x40,"",eReadWrite}, // exposure L											
	{0x3620,0x24,"",eReadWrite}, // analog control										
	{0x3621,0x34,"",eReadWrite}, // 													
	{0x3622,0x03,"",eReadWrite}, // analog control										
	{0x370a,0x23,"",eReadWrite}, // sensor control										
	{0x370d,0x00,"",eReadWrite}, // 													
	{0x3718,0x88,"",eReadWrite}, // 													
	{0x3721,0x09,"",eReadWrite}, // 													
	{0x3722,0x0b,"",eReadWrite}, // 													
	{0x3723,0x48,"",eReadWrite}, // 													
	{0x3738,0x99,"",eReadWrite}, // sensor control										
	{0x3801,0x00,"",eReadWrite}, // x start L											
	{0x3803,0x00,"",eReadWrite}, // y start L											
	{0x3804,0x06,"",eReadWrite}, // x end H 											
	{0x3805,0x4f,"",eReadWrite}, // x end L 											
	{0x3806,0x04,"",eReadWrite}, // y end H 											
	{0x3807,0xbf,"",eReadWrite}, // y end L 											
	{0x3808,0x03,"",eReadWrite}, // x output size H 									
	{0x3809,0x20,"",eReadWrite}, // x output size L 									
	{0x380a,0x02,"",eReadWrite}, // y output size H 									
	{0x380b,0x58,"",eReadWrite}, // y output size L 									
	{0x380c,0x0d,"",eReadWrite}, // HTS H												
	{0x380d,0x58,"",eReadWrite}, // HTS L												
	{0x380e,0x02,"",eReadWrite}, // VTS H												
	{0x380f,0x84,"",eReadWrite}, // VTS L												
	{0x3811,0x04,"",eReadWrite}, // ISP x win L 										
	{0x3813,0x04,"",eReadWrite}, // ISP y win L 										
	{0x3814,0x31,"",eReadWrite}, // x inc												
	{0x3815,0x31,"",eReadWrite}, // y inc												
	{0x3820,0xc2,"",eReadWrite}, // vsub48_blc on, vflip_blc on, vbinf on,				
	{0x3821,0x01,"",eReadWrite}, // hbin on 											
	{0x3820,0xc2,"",eReadWrite}, // vsub48_blc on, vflip_blc on, vbinf on,				
	{0x3821,0x05,"",eReadWrite}, // Mirror on, hbin on									
	{0x3820,0xc6,"",eReadWrite}, // vsub48_blc on, vflip_blc on, Flip on, vbinf on, 	
	{0x3821,0x01,"",eReadWrite}, // hbin on 											
	{0x3820,0xc6,"",eReadWrite}, // vsub48_blc on, vflip_blc on, Flip on, vbinf on, 	
	{0x3821,0x05,"",eReadWrite}, // Mirror on, hbin on									
	{0x382a,0x00,"",eReadWrite}, // fixed VTS											
	{0x3a00,0x41,"",eReadWrite}, // night mode off, band on 							
	{0x3a07,0xc1,"",eReadWrite}, // B50 L												
	{0x3a09,0xa1,"",eReadWrite}, // B60 L												
	{0x3a0a,0x12,"",eReadWrite}, // max exp 50 H										
	{0x3a0b,0x18,"",eReadWrite}, // max exp 50 L										
	{0x3a0c,0x14,"",eReadWrite}, // max exp 60 H										
	{0x3a0d,0x20,"",eReadWrite}, // max exp 60 L										
	{0x3a0e,0x02,"",eReadWrite}, // VTS band 50 H										
	{0x3a0f,0x43,"",eReadWrite}, // VTS band 50 L										
	{0x3a10,0x02,"",eReadWrite}, // VTS band 60 H										
	{0x3a11,0x84,"",eReadWrite}, // VTS band 60 L										
	{0x3a13,0x80,"",eReadWrite}, // gain ceiling = 8x									
	{0x4008,0x00,"",eReadWrite}, // bl_start											
	{0x4009,0x03,"",eReadWrite}, // bl_end												
	{0x5003,0x0c,"",eReadWrite}, // manual blc offset									
	//{0x4200,0x00,"",eReadWrite}, // stream on											
#else //30~10fps
	{0x4202,0x0f,"",eReadWrite}, // stream off												  
	{0x3501,0x26,"",eReadWrite}, // exposure M												  
	{0x3502,0x40,"",eReadWrite}, // exposure L												  
	{0x3620,0x24,"",eReadWrite}, // analog control											  
	{0x3621,0x34,"",eReadWrite}, // 														  
	{0x3622,0x03,"",eReadWrite}, // analog control											  
	{0x370a,0x23,"",eReadWrite}, // sensor control											  
	{0x370d,0x00,"",eReadWrite}, // 														  
	{0x3718,0x80,"",eReadWrite}, // 														  
	{0x3721,0x09,"",eReadWrite}, // 														  
	{0x3722,0x0b,"",eReadWrite}, // 														  
	{0x3723,0x48,"",eReadWrite}, // 														  
	{0x3738,0x99,"",eReadWrite}, // sensor control											  
	{0x3801,0x00,"",eReadWrite}, // x start L												  
	{0x3803,0x00,"",eReadWrite}, // y start L												  
	{0x3804,0x06,"",eReadWrite}, // x end H 												  
	{0x3805,0x4f,"",eReadWrite}, // x end L 												  
	{0x3806,0x04,"",eReadWrite}, // y end H 												  
	{0x3807,0xbf,"",eReadWrite}, // y end L 												  
	{0x3808,0x03,"",eReadWrite}, // x output size H 										  
	{0x3809,0x20,"",eReadWrite}, // x output size L 										  
	{0x380a,0x02,"",eReadWrite}, // y output size H 										  
	{0x380b,0x58,"",eReadWrite}, // y output size L 										  
	{0x380c,0x06,"",eReadWrite}, // HTS H													  
	{0x380d,0xac,"",eReadWrite}, // HTS L													  
	{0x380e,0x02,"",eReadWrite}, // VTS H													  
	{0x380f,0x84,"",eReadWrite}, // VTS L													  
	{0x3811,0x04,"",eReadWrite}, // ISP x win L 											  
	{0x3813,0x04,"",eReadWrite}, // ISP y win L 											  
	{0x3814,0x31,"",eReadWrite}, // x inc													  
	{0x3815,0x31,"",eReadWrite}, // y inc													  
	{0x3820,0xc2,"",eReadWrite}, // vsub48_blc on, vflip_blc on, vbinf on,					  
	{0x3821,0x01,"",eReadWrite}, // hbin on 												  
	{0x3820,0xc2,"",eReadWrite}, // vsub48_blc on, vflip_blc on, vbinf on,					  
	{0x3821,0x05,"",eReadWrite}, // Mirror on, hbin on										  
	{0x3820,0xc6,"",eReadWrite}, // vsub48_blc on, vflip_blc on, Flip on, vbinf on, 		  
	{0x3821,0x01,"",eReadWrite}, // hbin on 												  
	{0x3820,0xc6,"",eReadWrite}, // vsub48_blc on, vflip_blc on, Flip on, vbinf on, 		  
	{0x3821,0x05,"",eReadWrite}, // Mirror on, hbin on										  
	{0x382a,0x08,"",eReadWrite}, // auto VTS												  
	{0x3a00,0x43,"",eReadWrite}, // night mode on, band on									  
	{0x3a07,0xc1,"",eReadWrite}, // B50 L													  
	{0x3a09,0xa1,"",eReadWrite}, // B60 L													  
	{0x3a0a,0x07,"",eReadWrite}, // max exp 50 H											  
	{0x3a0b,0x8a,"",eReadWrite}, // max exp 50 L											  
	{0x3a0c,0x07,"",eReadWrite}, // max exp 60 H											  
	{0x3a0d,0x8c,"",eReadWrite}, // max exp 60 L											  
	{0x3a0e,0x02,"",eReadWrite}, // VTS band 50 H											  
	{0x3a0f,0x43,"",eReadWrite}, // VTS band 50 L											  
	{0x3a10,0x02,"",eReadWrite}, // VTS band 60 H											  
	{0x3a11,0x84,"",eReadWrite}, // VTS band 60 L											  
	{0x3a13,0x80,"",eReadWrite}, // gain ceiling = 8x										  
	{0x4008,0x00,"",eReadWrite}, // bl_start												  
	{0x4009,0x03,"",eReadWrite}, // bl_end													  
	{0x5003,0x0c,"",eReadWrite}, // manual blc offset										  
	{0x4200,0x00,"",eReadWrite}, // stream on			
	#endif
    {0x0000 ,0x00,"eTableEnd",eTableEnd}
};

const IsiRegDescription_t OV2685_g_1600x1200[] =
{
	
#if 0 //7.5fps
	{0x4202,0x0f,"",eReadWrite}, // stream off												  
	{0x3503,0x03,"",eReadWrite}, // AGC manual, AEC manual									  
	{0x3501,0x4e,"",eReadWrite}, // exposure M												  
	{0x3502,0xe0,"",eReadWrite}, // exposure L												  
	{0x3620,0x24,"",eReadWrite}, // analog control											  
	{0x3621,0x34,"",eReadWrite}, // 														  
	{0x3622,0x03,"",eReadWrite}, // analog control											  
	{0x370a,0x21,"",eReadWrite}, // sensor control											  
	{0x370d,0xc0,"",eReadWrite}, // 														  
	{0x3718,0x80,"",eReadWrite}, // 														  
	{0x3721,0x09,"",eReadWrite}, // 														  
	{0x3722,0x06,"",eReadWrite}, // 														  
	{0x3723,0x59,"",eReadWrite}, // 														  
	{0x3738,0x99,"",eReadWrite}, // sensor control											  
	{0x3801,0x00,"",eReadWrite}, // x start L												  
	{0x3803,0x00,"",eReadWrite}, // y start L												  
	{0x3804,0x06,"",eReadWrite}, // x end H 												  
	{0x3805,0x4f,"",eReadWrite}, // x end L 												  
	{0x3806,0x04,"",eReadWrite}, // y end H 												  
	{0x3807,0xbf,"",eReadWrite}, // y end L 												  
	{0x3808,0x06,"",eReadWrite}, // x output size H 										  
	{0x3809,0x40,"",eReadWrite}, // x output size L 										  
	{0x380a,0x04,"",eReadWrite}, // y output size H 										  
	{0x380b,0xb0,"",eReadWrite}, // y output size L 										  
	{0x380c,0x06,"",eReadWrite}, // HTS H													  
	{0x380d,0xa4,"",eReadWrite}, // HTS L													  
	{0x380e,0x0a,"",eReadWrite}, // VTS H													  
	{0x380f,0x1c,"",eReadWrite}, // VTS L													  
	{0x3811,0x08,"",eReadWrite}, // ISP x win L 											  
	{0x3813,0x08,"",eReadWrite}, // ISP y win L 											  
	{0x3814,0x11,"",eReadWrite}, // x inc													  
	{0x3815,0x11,"",eReadWrite}, // y inc													  
	{0x3820,0xc0,"",eReadWrite}, // vsub48_blc on, vflip_blc on, vbinf off, 				  
	{0x3821,0x00,"",eReadWrite}, // hbin off												  
	{0x3820,0xc0,"",eReadWrite}, // vsub48_blc on, vflip_blc on, vbinf off, 				  
	{0x3821,0x04,"",eReadWrite}, // Mirror on, hbin off 									  
	{0x3820,0xc4,"",eReadWrite}, // vsub48_blc on, vflip_blc on, Flip on, vbinf off,		  
	{0x3821,0x00,"",eReadWrite}, // hbin off												  
	{0x3820,0xc4,"",eReadWrite}, // vsub48_blc on, vflip_blc on, Flip on, vbinf off,		  
	{0x3821,0x04,"",eReadWrite}, // Mirror on, hbin off 									  
	{0x382a,0x00,"",eReadWrite}, // fixed VTS												  
	{0x3a00,0x41,"",eReadWrite}, // night mode off, band on 								  
	{0x3a07,0xc2,"",eReadWrite}, // B50 L													  
	{0x3a09,0xa1,"",eReadWrite}, // B60 L													  
	{0x3a0a,0x09,"",eReadWrite}, // max exp 50 H											  
	{0x3a0b,0xda,"",eReadWrite}, // max exp 50 L											  
	{0x3a0c,0x0a,"",eReadWrite}, // max exp 60 H											  
	{0x3a0d,0x10,"",eReadWrite}, // max exp 60 L											  
	{0x3a0e,0x09,"",eReadWrite}, // VTS band 50 H											  
	{0x3a0f,0xda,"",eReadWrite}, // VTS band 50 L											  
	{0x3a10,0x0a,"",eReadWrite}, // VTS band 60 H											  
	{0x3a11,0x10,"",eReadWrite}, // VTS band 60 L											  
	{0x3a13,0x80,"",eReadWrite}, // gain ceilling = 8x										  
	{0x4008,0x02,"",eReadWrite}, // bl_start												  
	{0x4009,0x09,"",eReadWrite}, // bl_end													  
	{0x4202,0x00,"",eReadWrite}, // stream on												  
	
#else //7.5-15fps
	{0x4202,0x0f,"",eReadWrite}, // stream off											   
	{0x3503,0x03,"",eReadWrite}, // AGC manual, AEC manual								   
	{0x3501,0x4e,"",eReadWrite}, // exposure M											   
	{0x3502,0xe0,"",eReadWrite}, // exposure L											   
	{0x3620,0x24,"",eReadWrite}, // analog control										   
	{0x3621,0x34,"",eReadWrite}, // 													   
	{0x3622,0x03,"",eReadWrite}, // analog control										   
	{0x370a,0x21,"",eReadWrite}, // sensor control										   
	{0x370d,0xc0,"",eReadWrite}, // 													   
	{0x3718,0x80,"",eReadWrite}, // 													   
	{0x3721,0x09,"",eReadWrite}, // 													   
	{0x3722,0x06,"",eReadWrite}, // 													   
	{0x3723,0x59,"",eReadWrite}, // 													   
	{0x3738,0x99,"",eReadWrite}, // sensor control										   
	{0x3801,0x00,"",eReadWrite}, // x start L											   
	{0x3803,0x00,"",eReadWrite}, // y start L											   
	{0x3804,0x06,"",eReadWrite}, // x end H 											   
	{0x3805,0x4f,"",eReadWrite}, // x end L 											   
	{0x3806,0x04,"",eReadWrite}, // y end H 											   
	{0x3807,0xbf,"",eReadWrite}, // y end L 											   
	{0x3808,0x06,"",eReadWrite}, // x output size H 									   
	{0x3809,0x40,"",eReadWrite}, // x output size L 									   
	{0x380a,0x04,"",eReadWrite}, // y output size H 									   
	{0x380b,0xb0,"",eReadWrite}, // y output size L 									   
	{0x380c,0x06,"",eReadWrite}, // HTS H												   
	{0x380d,0xa4,"",eReadWrite}, // HTS L												   
	{0x380e,0x05,"",eReadWrite}, // VTS H												   
	{0x380f,0x0e,"",eReadWrite}, // VTS L												   
	{0x3811,0x08,"",eReadWrite}, // ISP x win L 										   
	{0x3813,0x08,"",eReadWrite}, // ISP y win L 										   
	{0x3814,0x11,"",eReadWrite}, // x inc												   
	{0x3815,0x11,"",eReadWrite}, // y inc												   
	{0x3820,0xc0,"",eReadWrite}, // vsub48_blc on, vflip_blc on, vbinf off, 			   
	{0x3821,0x00,"",eReadWrite}, // hbin off											   
	{0x3820,0xc0,"",eReadWrite}, // vsub48_blc on, vflip_blc on, vbinf off, 			   
	{0x3821,0x04,"",eReadWrite}, // Mirror on, hbin off 								   
	{0x3820,0xc4,"",eReadWrite}, // vsub48_blc on, vflip_blc on, Flip on, vbinf off,	   
	{0x3821,0x00,"",eReadWrite}, // hbin off											   
	{0x3820,0xc4,"",eReadWrite}, // vsub48_blc on, vflip_blc on, Flip on, vbinf off,	   
	{0x3821,0x04,"",eReadWrite}, // Mirror on, hbin off 								   
	{0x382a,0x08,"",eReadWrite}, // auto VTS											   
	{0x3a00,0x41,"",eReadWrite}, // night mode off, band on 							   
	{0x3a07,0xc2,"",eReadWrite}, // B50 L												   
	{0x3a09,0xa1,"",eReadWrite}, // B60 L												   
	{0x3a0a,0x09,"",eReadWrite}, // max exp 50 H										   
	{0x3a0b,0xda,"",eReadWrite}, // max exp 50 L										   
	{0x3a0c,0x0a,"",eReadWrite}, // max exp 60 H										   
	{0x3a0d,0x10,"",eReadWrite}, // max exp 60 L										   
	{0x3a0e,0x04,"",eReadWrite}, // VTS band 50 H										   
	{0x3a0f,0x8c,"",eReadWrite}, // VTS band 50 L										   
	{0x3a10,0x05,"",eReadWrite}, // VTS band 60 H										   
	{0x3a11,0x08,"",eReadWrite}, // VTS band 60 L										   
	{0x4008,0x02,"",eReadWrite}, // bl_start											   
	{0x4009,0x09,"",eReadWrite}, // bl_end												   
	{0x4202,0x00,"",eReadWrite}, // stream on
#endif
    {0x0000 ,0x00,"eTableEnd",eTableEnd}

};


