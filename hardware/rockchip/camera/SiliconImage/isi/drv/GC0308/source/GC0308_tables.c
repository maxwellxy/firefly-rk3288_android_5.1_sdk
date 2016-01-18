#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "isi.h"
#include "isi_iss.h"
#include "isi_priv.h"
#include "GC0308_priv.h"


/*****************************************************************************
 * DEFINES
 *****************************************************************************/


/*****************************************************************************
 * GLOBALS
 *****************************************************************************/

// Image sensor register settings default values taken from data sheet OV8810_DS_1.1_SiliconImage.pdf.
// The settings may be altered by the code in IsiSetupSensor.
const IsiRegDescription_t GC0308_g_aRegDescription[] =
{
#if 0		
    {0xfe , 0x80,"",eReadWrite},  	
		        
	{0xfe , 0x00,"",eReadWrite},   // set page0
	           
	{0xd2 , 0x10,"",eReadWrite},   // close AEC
	{0x22 , 0x55,"",eReadWrite},   // close AWB
              
	{0x03 , 0x01,"",eReadWrite},                                  
	{0x04 , 0x2c,"",eReadWrite},                                  
	{0x5a , 0x56,"",eReadWrite},
	{0x5b , 0x40,"",eReadWrite},
	{0x5c , 0x4a,"",eReadWrite},			
             
	{0x22 , 0x57,"",eReadWrite},   // Open AWB
               
	{0x01 , 0xfa ,"",eReadWrite },                                    
	{0x02 , 0x70,"",eReadWrite},                                  
	{0x0f , 0x01,"",eReadWrite},                                  
                                                                   
                                                                   
	{0xe2 , 0x00,"",eReadWrite},   //anti-flicker step [11:8]     
	{0xe3 , 0x64,"",eReadWrite},   //anti-flicker step [7:0]      
		                                                               
	{0xe4 , 0x02,"",eReadWrite},   //exp level 1  16.67fps        
	{0xe5 , 0x58,"",eReadWrite},                                  
	{0xe6 , 0x03,"",eReadWrite},   //exp level 2  12.5fps         
	{0xe7 , 0x20,"",eReadWrite},                                  
	{0xe8 , 0x04,"",eReadWrite},   //exp level 3  8.33fps         
	{0xe9 , 0xb0,"",eReadWrite},                                  
	{0xea , 0x09,"",eReadWrite},   //exp level 4  4.00fps         
	{0xeb , 0xc4,"",eReadWrite},                                  

	//{0xec , 0x20},
	   
	{0x05 , 0x00,"",eReadWrite},                                  
	{0x06 , 0x00,"",eReadWrite},                                  
	{0x07 , 0x00,"",eReadWrite},                                  
	{0x08 , 0x00,"",eReadWrite},                                  
	{0x09 , 0x01,"",eReadWrite},                                  
	{0x0a , 0xe8,"",eReadWrite},                                  
	{0x0b , 0x02,"",eReadWrite},                                  
	{0x0c , 0x88,"",eReadWrite},                                  
	{0x0d , 0x02,"",eReadWrite},                                  
	{0x0e , 0x02,"",eReadWrite},                                  
	{0x10 , 0x22,"",eReadWrite},                                  
	{0x11 , 0xfd,"",eReadWrite},                                  
	{0x12 , 0x2a,"",eReadWrite},                                  
	{0x13 , 0x00,"",eReadWrite},                                  
	//{0x14 , 0x10},                                
	{0x15 , 0x0a,"",eReadWrite},                                  
	{0x16 , 0x05,"",eReadWrite},                                  
	{0x17 , 0x01,"",eReadWrite},                                  
	{0x18 , 0x44,"",eReadWrite},                                  
	{0x19 , 0x44,"",eReadWrite},                                  
	{0x1a , 0x1e,"",eReadWrite},                                  
	{0x1b , 0x00,"",eReadWrite},                                  
	{0x1c , 0xc1,"",eReadWrite},                                  
	{0x1d , 0x08,"",eReadWrite},                                  
	{0x1e , 0x60,"",eReadWrite},                                  
	{0x1f , 0x16,"",eReadWrite},                                  
                                                                   
	                                                                 
	{0x20 , 0xff,"",eReadWrite},                                  
	{0x21 , 0xf8,"",eReadWrite},                                  
	{0x22 , 0x57,"",eReadWrite},                                  
	{0x24 , 0xa2,"",eReadWrite},                                  
	{0x25 , 0x0f,"",eReadWrite},                                  
	                                                                 
	//output sync_mode                                               
	{0x26 , 0x02,"",eReadWrite},   //0x03  20101016 zhj                                 
	{0x2f , 0x01,"",eReadWrite},                                  
	{0x30 , 0xf7,"",eReadWrite},                                  
	{0x31 , 0x50,"",eReadWrite},
	{0x32 , 0x00,"",eReadWrite},
	{0x39 , 0x04,"",eReadWrite},
	{0x3a , 0x18,"",eReadWrite},
	{0x3b , 0x20,"",eReadWrite},                                  
	{0x3c , 0x00,"",eReadWrite},                                  
	{0x3d , 0x00,"",eReadWrite},                                  
	{0x3e , 0x00,"",eReadWrite},                                  
	{0x3f , 0x00,"",eReadWrite},                                  
	{0x50 , 0x10,"",eReadWrite},                                  
	{0x53 , 0x82,"",eReadWrite},                                  
	{0x54 , 0x80,"",eReadWrite},                                  
	{0x55 , 0x80,"",eReadWrite},                                  
	{0x56 , 0x82,"",eReadWrite},                                  
	{0x8b , 0x40,"",eReadWrite},                                  
	{0x8c , 0x40,"",eReadWrite},                                  
	{0x8d , 0x40,"",eReadWrite},                                  
	{0x8e , 0x2e,"",eReadWrite},                                  
	{0x8f , 0x2e,"",eReadWrite},                                  
	{0x90 , 0x2e,"",eReadWrite},                                  
	{0x91 , 0x3c,"",eReadWrite},                                  
	{0x92 , 0x50,"",eReadWrite},                                  
	{0x5d , 0x12,"",eReadWrite},                                  
	{0x5e , 0x1a,"",eReadWrite},                                  
	{0x5f , 0x24,"",eReadWrite},                                  
	{0x60 , 0x07,"",eReadWrite},                                  
	{0x61 , 0x15,"",eReadWrite},                                  
	{0x62 , 0x08,"",eReadWrite},                                  
	{0x64 , 0x03,"",eReadWrite},                                  
	{0x66 , 0xe8,"",eReadWrite},                                  
	{0x67 , 0x86,"",eReadWrite},                                  
	{0x68 , 0xa2,"",eReadWrite},                                  
	{0x69 , 0x18,"",eReadWrite},                                  
	{0x6a , 0x0f,"",eReadWrite},                                  
	{0x6b , 0x00,"",eReadWrite},                                  
	{0x6c , 0x5f,"",eReadWrite},                                  
	{0x6d , 0x8f,"",eReadWrite},                                  
	{0x6e , 0x55,"",eReadWrite},                                  
	{0x6f , 0x38,"",eReadWrite},                                  
	{0x70 , 0x15,"",eReadWrite},                                  
	{0x71 , 0x33,"",eReadWrite},                                  
	{0x72 , 0xdc,"",eReadWrite},                                  
	{0x73 , 0x80,"",eReadWrite},                                  
	{0x74 , 0x02,"",eReadWrite},                                  
	{0x75 , 0x3f,"",eReadWrite},                                  
	{0x76 , 0x02,"",eReadWrite},                                  
	{0x77 , 0x36,"",eReadWrite},                                  
	{0x78 , 0x88,"",eReadWrite},                                  
	{0x79 , 0x81,"",eReadWrite},                                  
	{0x7a , 0x81,"",eReadWrite},                                  
	{0x7b , 0x22,"",eReadWrite},                                  
	{0x7c , 0xff,"",eReadWrite},                                  
	{0x93 , 0x48,"",eReadWrite},                                  
	{0x94 , 0x00,"",eReadWrite},                                  
	{0x95 , 0x05,"",eReadWrite},                                  
	{0x96 , 0xe8,"",eReadWrite},                                  
	{0x97 , 0x40,"",eReadWrite},                                  
	{0x98 , 0xf0,"",eReadWrite},                                  
	{0xb1 , 0x38,"",eReadWrite},                                  
	{0xb2 , 0x38,"",eReadWrite},                                  
	{0xbd , 0x38,"",eReadWrite},                                  
	{0xbe , 0x36,"",eReadWrite},                                  
	{0xd0 , 0xc9,"",eReadWrite},                                  
	{0xd1 , 0x10,"",eReadWrite},                                  
	//{0xd2 , 0x90},                                
	{0xd3 , 0x80,"",eReadWrite},                                  
	{0xd5 , 0xf2,"",eReadWrite},                                  
	{0xd6 , 0x16,"",eReadWrite},                                  
	{0xdb , 0x92,"",eReadWrite},                                  
	{0xdc , 0xa5,"",eReadWrite},                                  
	{0xdf , 0x23,"",eReadWrite},                                  
	{0xd9 , 0x00,"",eReadWrite},                                  
	{0xda , 0x00,"",eReadWrite},                                  
	{0xe0 , 0x09,"",eReadWrite},                                  
                               
	{0xed , 0x04,"",eReadWrite},                                  
	{0xee , 0xa0,"",eReadWrite},                                  
	{0xef , 0x40,"",eReadWrite},                                  
	{0x80 , 0x03,"",eReadWrite},                                  
	{0x80 , 0x03,"",eReadWrite},                                  
	{0x9F , 0x10,"",eReadWrite},                                  
	{0xA0 , 0x20,"",eReadWrite},                                  
	{0xA1 , 0x38,"",eReadWrite},                                  
	{0xA2 , 0x4E,"",eReadWrite},                                  
	{0xA3 , 0x63,"",eReadWrite},                                  
	{0xA4 , 0x76,"",eReadWrite},                                  
	{0xA5 , 0x87,"",eReadWrite},                                  
	{0xA6 , 0xA2,"",eReadWrite},                                  
	{0xA7 , 0xB8,"",eReadWrite},                                  
	{0xA8 , 0xCA,"",eReadWrite},                                  
	{0xA9 , 0xD8,"",eReadWrite},                                  
	{0xAA , 0xE3,"",eReadWrite},                                  
	{0xAB , 0xEB,"",eReadWrite},                                  
	{0xAC , 0xF0,"",eReadWrite},                                  
	{0xAD , 0xF8,"",eReadWrite},                                  
	{0xAE , 0xFD,"",eReadWrite},                                  
	{0xAF , 0xFF,"",eReadWrite},                                  
	{0xc0 , 0x00,"",eReadWrite},                                  
	{0xc1 , 0x10,"",eReadWrite},                                  
	{0xc2 , 0x1C,"",eReadWrite},                                  
	{0xc3 , 0x30,"",eReadWrite},                                  
	{0xc4 , 0x43,"",eReadWrite},                                  
	{0xc5 , 0x54,"",eReadWrite},                                  
	{0xc6 , 0x65,"",eReadWrite},                                  
	{0xc7 , 0x75,"",eReadWrite},                                  
	{0xc8 , 0x93,"",eReadWrite},                                  
	{0xc9 , 0xB0,"",eReadWrite},                                  
	{0xca , 0xCB,"",eReadWrite},                                  
	{0xcb , 0xE6,"",eReadWrite},                                  
	{0xcc , 0xFF,"",eReadWrite},                                  
	{0xf0 , 0x02,"",eReadWrite},                                  
	{0xf1 , 0x01,"",eReadWrite},                                  
	{0xf2 , 0x01,"",eReadWrite},                                  
	{0xf3 , 0x30,"",eReadWrite},                                  
	{0xf9 , 0x9f,"",eReadWrite},                                  
	{0xfa , 0x78,"",eReadWrite},                                  
                                                                   
	//---------------------------------------------------------------
	{0xfe , 0x01,"",eReadWrite},// set page1                                            
                                                                   
	{0x00 , 0xf5,"",eReadWrite},                                  
	{0x02 , 0x1a,"",eReadWrite},                                  
	{0x0a , 0xa0,"",eReadWrite},                                  
	{0x0b , 0x60,"",eReadWrite},                                  
	{0x0c , 0x08,"",eReadWrite},                                  
	{0x0e , 0x4c,"",eReadWrite},                                  
	{0x0f , 0x39,"",eReadWrite},                                  
	{0x11 , 0x3f,"",eReadWrite},                                  
	{0x12 , 0x72,"",eReadWrite},                                  
	{0x13 , 0x13,"",eReadWrite},                                  
	{0x14 , 0x42,"",eReadWrite},                                  
	{0x15 , 0x43,"",eReadWrite},                                  
	{0x16 , 0xc2,"",eReadWrite},                                  
	{0x17 , 0xa8,"",eReadWrite},                                  
	{0x18 , 0x18,"",eReadWrite},                                  
	{0x19 , 0x40,"",eReadWrite},                                  
	{0x1a , 0xd0,"",eReadWrite},                                  
	{0x1b , 0xf5,"",eReadWrite},                                  
	{0x70 , 0x40,"",eReadWrite},                                  
	{0x71 , 0x58,"",eReadWrite},                                  
	{0x72 , 0x30,"",eReadWrite},                                  
	{0x73 , 0x48,"",eReadWrite},                                  
	{0x74 , 0x20,"",eReadWrite},                                  
	{0x75 , 0x60,"",eReadWrite},                                  
	{0x77 , 0x20,"",eReadWrite},                                  
	{0x78 , 0x32,"",eReadWrite},                                  
	{0x30 , 0x03,"",eReadWrite},                                  
	{0x31 , 0x40,"",eReadWrite},                                  
	{0x32 , 0xe0,"",eReadWrite},                                  
	{0x33 , 0xe0,"",eReadWrite},                                  
	{0x34 , 0xe0,"",eReadWrite},                                  
	{0x35 , 0xb0,"",eReadWrite},                                  
	{0x36 , 0xc0,"",eReadWrite},                                  
	{0x37 , 0xc0,"",eReadWrite},                                  
	{0x38 , 0x04,"",eReadWrite},                                  
	{0x39 , 0x09,"",eReadWrite},                                  
	{0x3a , 0x12,"",eReadWrite},                                  
	{0x3b , 0x1C,"",eReadWrite},                                  
	{0x3c , 0x28,"",eReadWrite},                                  
	{0x3d , 0x31,"",eReadWrite},                                  
	{0x3e , 0x44,"",eReadWrite},                                  
	{0x3f , 0x57,"",eReadWrite},                                  
	{0x40 , 0x6C,"",eReadWrite},                                  
	{0x41 , 0x81,"",eReadWrite},                                  
	{0x42 , 0x94,"",eReadWrite},                                  
	{0x43 , 0xA7,"",eReadWrite},                                  
	{0x44 , 0xB8,"",eReadWrite},                                  
	{0x45 , 0xD6,"",eReadWrite},                                  
	{0x46 , 0xEE,"",eReadWrite},                                  
	{0x47 , 0x0d,"",eReadWrite},                                  
	{0xfe , 0x00,"",eReadWrite}, // set page0
	            
	//-----------Update the registers 2010/07/06-------------//
	//Registers of Page0
	{0xfe , 0x00,"",eReadWrite}, // set page0
	{0x10 , 0x26,"",eReadWrite},                                 
	{0x11 , 0x0d,"",eReadWrite},  // fd,modified by mormo 2010/07/06                               
	{0x1a , 0x2a,"",eReadWrite},  // 1e,modified by mormo 2010/07/06                                  
                
	{0x1c , 0x49,"",eReadWrite}, // c1,modified by mormo 2010/07/06                                 
	{0x1d , 0x9a,"",eReadWrite}, // 08,modified by mormo 2010/07/06                                 
	{0x1e , 0x61,"",eReadWrite}, // 60,modified by mormo 2010/07/06                                 
                
	{0x3a , 0x20,"",eReadWrite},
               
	{0x50 , 0x14,"",eReadWrite},  // 10,modified by mormo 2010/07/06                               
	{0x53 , 0x80,"",eReadWrite},                                  
	{0x56 , 0x80,"",eReadWrite},
	           
	{0x8b , 0x20,"",eReadWrite}, //LSC                                 
	{0x8c , 0x20,"",eReadWrite},                                  
	{0x8d , 0x20,"",eReadWrite},                                  
	{0x8e , 0x14,"",eReadWrite},                                  
	{0x8f , 0x10,"",eReadWrite},                                  
	{0x90 , 0x14,"",eReadWrite},                                  
                
	{0x94 , 0x02,"",eReadWrite},                                  
	{0x95 , 0x07,"",eReadWrite},                                  
	{0x96 , 0xe0,"",eReadWrite},                                  
                
	{0xb1 , 0x40,"",eReadWrite}, // YCPT                                 
	{0xb2 , 0x40,"",eReadWrite},                                  
	{0xb3 , 0x40,"",eReadWrite},
	{0xb6 , 0xe0,"",eReadWrite},
                
	{0xd0 , 0xcb,"",eReadWrite}, // AECT  c9,modifed by mormo 2010/07/06                                
	{0xd3 , 0x48,"",eReadWrite}, // 80,modified by mormor 2010/07/06                           
                
	{0xf2 , 0x02,"",eReadWrite},                                  
	{0xf7 , 0x12,"",eReadWrite},
	{0xf8 , 0x0a,"",eReadWrite},
                
	//Registers of Page1
	{0xfe , 0x01,"",eReadWrite},// set page1    
	{0x02 , 0x20,"",eReadWrite},
	{0x04 , 0x10,"",eReadWrite},
	{0x05 , 0x08,"",eReadWrite},
	{0x06 , 0x20,"",eReadWrite},
	{0x08 , 0x0a,"",eReadWrite},
                
	{0x0e , 0x44,"",eReadWrite},                                  
	{0x0f , 0x32,"",eReadWrite},
	{0x10 , 0x41,"",eReadWrite},                                  
	{0x11 , 0x37,"",eReadWrite},                                  
	{0x12 , 0x22,"",eReadWrite},                                  
	{0x13 , 0x19,"",eReadWrite},                                  
	{0x14 , 0x44,"",eReadWrite},                                  
	{0x15 , 0x44,"",eReadWrite},  
	            
	{0x19 , 0x50,"",eReadWrite},                                  
	{0x1a , 0xd8,"",eReadWrite}, 
	            
	{0x32 , 0x10,"",eReadWrite}, 
	            
	{0x35 , 0x00,"",eReadWrite},                                  
	{0x36 , 0x80,"",eReadWrite},                                  
	{0x37 , 0x00,"",eReadWrite}, 
	//-----------Update the registers end---------//
                
                
	{0xfe , 0x00,"",eReadWrite}, // set page0
	{0xd2 , 0x90,"",eReadWrite},
                

	//-----------GAMMA Select(3)---------------//
	{0x9F , 0x10,"",eReadWrite},
	{0xA0 , 0x20,"",eReadWrite},
	{0xA1 , 0x38,"",eReadWrite},
	{0xA2 , 0x4E,"",eReadWrite},
	{0xA3 , 0x63,"",eReadWrite},
	{0xA4 , 0x76,"",eReadWrite},
	{0xA5 , 0x87,"",eReadWrite},
	{0xA6 , 0xA2,"",eReadWrite},
	{0xA7 , 0xB8,"",eReadWrite},
	{0xA8 , 0xCA,"",eReadWrite},
	{0xA9 , 0xD8,"",eReadWrite},
	{0xAA , 0xE3,"",eReadWrite},
	{0xAB , 0xEB,"",eReadWrite},
	{0xAC , 0xF0,"",eReadWrite},
	{0xAD , 0xF8,"",eReadWrite},
	{0xAE , 0xFD,"",eReadWrite},
	{0xAF , 0xFF,"",eReadWrite},

	 /*GC0308_GAMMA_Select,
		1:                                             //smallest gamma curve
			{0x9F , 0x0B},
			{0xA0 , 0x16},
			{0xA1 , 0x29},
			{0xA2 , 0x3C},
			{0xA3 , 0x4F},
			{0xA4 , 0x5F},
			{0xA5 , 0x6F},
			{0xA6 , 0x8A},
			{0xA7 , 0x9F},
			{0xA8 , 0xB4}, 
			{0xA9 , 0xC6},
			{0xAA , 0xD3},
			{0xAB , 0xDD},
			{0xAC , 0xE5},
			{0xAD , 0xF1},
			{0xAE , 0xFA},
			{0xAF , 0xFF},	
			
		2:			
			{0x9F , 0x0E},
			{0xA0 , 0x1C},
			{0xA1 , 0x34},
			{0xA2 , 0x48},
			{0xA3 , 0x5A},
			{0xA4 , 0x6B},
			{0xA5 , 0x7B},
			{0xA6 , 0x95},
			{0xA7 , 0xAB},
			{0xA8 , 0xBF},
			{0xA9 , 0xCE},
			{0xAA , 0xD9},
			{0xAB , 0xE4},
			{0xAC , 0xEC},
			{0xAD , 0xF7},
			{0xAE , 0xFD},
			{0xAF , 0xFF},
			
		3:
			{0x9F , 0x10},
			{0xA0 , 0x20},
			{0xA1 , 0x38},
			{0xA2 , 0x4E},
			{0xA3 , 0x63},
			{0xA4 , 0x76},
			{0xA5 , 0x87},
			{0xA6 , 0xA2},
			{0xA7 , 0xB8},
			{0xA8 , 0xCA},
			{0xA9 , 0xD8},
			{0xAA , 0xE3},
			{0xAB , 0xEB},
			{0xAC , 0xF0},
			{0xAD , 0xF8},
			{0xAE , 0xFD},
			{0xAF , 0xFF},

		4:
			{0x9F , 0x14},
			{0xA0 , 0x28},
			{0xA1 , 0x44},
			{0xA2 , 0x5D},
			{0xA3 , 0x72},
			{0xA4 , 0x86},
			{0xA5 , 0x95},
			{0xA6 , 0xB1},
			{0xA7 , 0xC6},
			{0xA8 , 0xD5},
			{0xA9 , 0xE1},
			{0xAA , 0xEA},
			{0xAB , 0xF1},
			{0xAC , 0xF5},
			{0xAD , 0xFB},
			{0xAE , 0xFE},
			{0xAF , 0xFF},
			
		5:								//largest gamma curve
			{0x9F , 0x15},
			{0xA0 , 0x2A},
			{0xA1 , 0x4A},
			{0xA2 , 0x67},
			{0xA3 , 0x79},
			{0xA4 , 0x8C},
			{0xA5 , 0x9A},
			{0xA6 , 0xB3},
			{0xA7 , 0xC5},
			{0xA8 , 0xD5},
			{0xA9 , 0xDF},
			{0xAA , 0xE8},
			{0xAB , 0xEE},
			{0xAC , 0xF3},
			{0xAD , 0xFA},
			{0xAE , 0xFD},
			{0xAF , 0xFF}, */
	//-----------GAMMA Select End--------------//




	//-------------H_V_Switch(4)---------------//
			{0x14 , 0x11,"",eReadWrite},  //0x10

	 /*GC0308_H_V_Switch,

		1:  // normal
	    		{0x14 , 0x10},
	    		
		2:  // IMAGE_H_MIRROR
	    		{0x14 , 0x11},
	    		
		3:  // IMAGE_V_MIRROR
	    		{0x14 , 0x12},
	    		
		4:  // IMAGE_HV_MIRROR
	    		{0x14 , 0x13},
	*/		    
	//-------------H_V_Select End--------------//

#endif
#if 1
		{0xfe , 0x80,"",eReadWrite},		
			      
		{0xfe , 0x00,"",eReadWrite},	 // set page0
		          
		{0xd2 , 0x10,"",eReadWrite},	 // close AEC
		{0x22 , 0x55,"",eReadWrite},	 // close AWB
	              
		{0x03 , 0x01,"",eReadWrite},									
		{0x04 , 0x2c,"",eReadWrite},									
#if 1
		{0x5a , 0x56,"",eReadWrite},
		{0x5b , 0x40,"",eReadWrite},
		{0x5c , 0x4a,"",eReadWrite},			
	              
		{0x22 , 0x57,"",eReadWrite},	 // Open AWB
#endif 
#if 0 
		{0x22 , 0x55,"",eReadWrite},
		{0x5a , 0x50,"",eReadWrite},
		{0x5b , 0x45,"",eReadWrite},			
	              
		{0x5c , 0x40,"",eReadWrite},	              
#endif
		{0x01 , 0xfa,"",eReadWrite},									  
		{0x02 , 0x70,"",eReadWrite},									
		{0x0f , 0x01,"",eReadWrite},									
																	   
																	   
		{0xe2 , 0x00,"",eReadWrite},	 //anti-flicker step [11:8] 	
		{0xe3 , 0x64,"",eReadWrite},	 //anti-flicker step [7:0]		
																		   
		{0xe4 , 0x02,"",eReadWrite},	 //exp level 1	16.67fps		
		{0xe5 , 0x58,"",eReadWrite},									
		{0xe6 , 0x03,"",eReadWrite},	 //exp level 2	12.5fps 		
		{0xe7 , 0x20,"",eReadWrite},									
		{0xe8 , 0x04,"",eReadWrite},	 //exp level 3	8.33fps 		
		{0xe9 , 0xb0,"",eReadWrite},									
		{0xea , 0x09,"",eReadWrite},	 //exp level 4	4.00fps 		
		{0xeb , 0xc4,"",eReadWrite},									
	              
		//{0xec , 0x20},
		          
		{0x05 , 0x00,"",eReadWrite},									
		{0x06 , 0x00,"",eReadWrite},									
		{0x07 , 0x00,"",eReadWrite},									
		{0x08 , 0x00,"",eReadWrite},									
		{0x09 , 0x01,"",eReadWrite},									
		{0x0a , 0xe8,"",eReadWrite},									
		{0x0b , 0x02,"",eReadWrite},									
		{0x0c , 0x88,"",eReadWrite},									
		{0x0d , 0x02,"",eReadWrite},									
		{0x0e , 0x02,"",eReadWrite},									
		{0x10 , 0x22,"",eReadWrite},									
		{0x11 , 0xfd,"",eReadWrite},									
		{0x12 , 0x2a,"",eReadWrite},									
		{0x13 , 0x00,"",eReadWrite},									
		{0x14 , 0x10,"",eReadWrite},	//0x10
		//-------------H_V_Switch(4)---------------//
		/*	1:	// normal
					{0x14 , 0x10},			
			2:	// IMAGE_H_MIRROR
					{0x14 , 0x11},
					
			3:	// IMAGE_V_MIRROR
					{0x14 , 0x12},
					
			4:	// IMAGE_HV_MIRROR
					{0x14 , 0x13},*/										
		{0x15 , 0x0a,"",eReadWrite},									
		{0x16 , 0x05,"",eReadWrite},									
		{0x17 , 0x01,"",eReadWrite},									
		{0x18 , 0x44,"",eReadWrite},									
		{0x19 , 0x44,"",eReadWrite},									
		{0x1a , 0x1e,"",eReadWrite},									
		{0x1b , 0x00,"",eReadWrite},									
		{0x1c , 0xc1,"",eReadWrite},									
		{0x1d , 0x08,"",eReadWrite},									
		{0x1e , 0x60,"",eReadWrite},									
		{0x1f , 0x17,"",eReadWrite},									
																	   
																		 
		{0x20 , 0xff,"",eReadWrite},									
		{0x21 , 0xf8,"",eReadWrite},									
		{0x22 , 0x57,"",eReadWrite},									
		{0x24 , 0xa0,"",eReadWrite},									
		{0x25 , 0x0f,"",eReadWrite},									
																		 
		//output sync_mode												 
		{0x26 , 0x02,"",eReadWrite},	 //0x03  20101016 zhj								  
		{0x2f , 0x01,"",eReadWrite},									
		{0x30 , 0xf7,"",eReadWrite},									
		{0x31 , 0x50,"",eReadWrite},
		{0x32 , 0x00,"",eReadWrite},
		{0x39 , 0x04,"",eReadWrite},
		{0x3a , 0x18,"",eReadWrite},
		{0x3b , 0x20,"",eReadWrite},									
		{0x3c , 0x00,"",eReadWrite},									
		{0x3d , 0x00,"",eReadWrite},									
		{0x3e , 0x00,"",eReadWrite},									
		{0x3f , 0x00,"",eReadWrite},									
		{0x50 , 0x10,"",eReadWrite},									
		{0x53 , 0x82,"",eReadWrite},									
		{0x54 , 0x80,"",eReadWrite},									
		{0x55 , 0x80,"",eReadWrite},									
		{0x56 , 0x82,"",eReadWrite},									
		{0x8b , 0x40,"",eReadWrite},									
		{0x8c , 0x40,"",eReadWrite},									
		{0x8d , 0x40,"",eReadWrite},									
		{0x8e , 0x2e,"",eReadWrite},									
		{0x8f , 0x2e,"",eReadWrite},									
		{0x90 , 0x2e,"",eReadWrite},									
		{0x91 , 0x3c,"",eReadWrite},									
		{0x92 , 0x50,"",eReadWrite},									
		{0x5d , 0x12,"",eReadWrite},									
		{0x5e , 0x1a,"",eReadWrite},									
		{0x5f , 0x24,"",eReadWrite},									
		{0x60 , 0x07,"",eReadWrite},									
		{0x61 , 0x15,"",eReadWrite},									
		{0x62 , 0x08,"",eReadWrite},									
		{0x64 , 0x03,"",eReadWrite},									
		{0x66 , 0xe8,"",eReadWrite},									
		{0x67 , 0x86,"",eReadWrite},									
		{0x68 , 0xa2,"",eReadWrite},									
		{0x69 , 0x18,"",eReadWrite},									
		{0x6a , 0x0f,"",eReadWrite},									
		{0x6b , 0x00,"",eReadWrite},									
		{0x6c , 0x5f,"",eReadWrite},									
		{0x6d , 0x8f,"",eReadWrite},									
		{0x6e , 0x55,"",eReadWrite},									
		{0x6f , 0x38,"",eReadWrite},									
		{0x70 , 0x15,"",eReadWrite},									
		{0x71 , 0x33,"",eReadWrite},									
		{0x72 , 0xdc,"",eReadWrite},									
		{0x73 , 0x80,"",eReadWrite},									
		{0x74 , 0x02,"",eReadWrite},									
		{0x75 , 0x3f,"",eReadWrite},									
		{0x76 , 0x02,"",eReadWrite},									
		{0x77 , 0x36,"",eReadWrite},									
		{0x78 , 0x88,"",eReadWrite},									
		{0x79 , 0x81,"",eReadWrite},									
		{0x7a , 0x81,"",eReadWrite},									
		{0x7b , 0x22,"",eReadWrite},									
		{0x7c , 0xff,"",eReadWrite},									
		{0x93 , 0x48,"",eReadWrite},									
		{0x94 , 0x00,"",eReadWrite},									
		{0x95 , 0x05,"",eReadWrite},									
		{0x96 , 0xe8,"",eReadWrite},									
		{0x97 , 0x40,"",eReadWrite},									
		{0x98 , 0xf0,"",eReadWrite},									
		{0xb1 , 0x38,"",eReadWrite},									
		{0xb2 , 0x38,"",eReadWrite},									
		{0xbd , 0x38,"",eReadWrite},									
		{0xbe , 0x36,"",eReadWrite},									
		{0xd0 , 0xc9,"",eReadWrite},									
		{0xd1 , 0x10,"",eReadWrite},									
		//{0xd2 , 0x90},								
		{0xd3 , 0x80,"",eReadWrite},									
		{0xd5 , 0xf2,"",eReadWrite},									
		{0xd6 , 0x16,"",eReadWrite},									
		{0xdb , 0x92,"",eReadWrite},									
		{0xdc , 0xa5,"",eReadWrite},									
		{0xdf , 0x23,"",eReadWrite},									
		{0xd9 , 0x00,"",eReadWrite},									
		{0xda , 0x00,"",eReadWrite},									
		{0xe0 , 0x09,"",eReadWrite},									
								   
		{0xed , 0x04,"",eReadWrite},									
		{0xee , 0xa0,"",eReadWrite},									
		{0xef , 0x40,"",eReadWrite},									
		{0x80 , 0x03,"",eReadWrite},									
		{0x80 , 0x03,"",eReadWrite},									
		{0x9F , 0x10,"",eReadWrite},									
		{0xA0 , 0x20,"",eReadWrite},									
		{0xA1 , 0x38,"",eReadWrite},									
		{0xA2 , 0x4E,"",eReadWrite},									
		{0xA3 , 0x63,"",eReadWrite},									
		{0xA4 , 0x76,"",eReadWrite},									
		{0xA5 , 0x87,"",eReadWrite},									
		{0xA6 , 0xA2,"",eReadWrite},									
		{0xA7 , 0xB8,"",eReadWrite},									
		{0xA8 , 0xCA,"",eReadWrite},									
		{0xA9 , 0xD8,"",eReadWrite},									
		{0xAA , 0xE3,"",eReadWrite},									
		{0xAB , 0xEB,"",eReadWrite},									
		{0xAC , 0xF0,"",eReadWrite},									
		{0xAD , 0xF8,"",eReadWrite},									
		{0xAE , 0xFD,"",eReadWrite},									
		{0xAF , 0xFF,"",eReadWrite},									
		 /*GC0308_GAMMA_Select,
			1:											   //smallest gamma curve
				{0x9F , 0x0B},
				{0xA0 , 0x16},
				{0xA1 , 0x29},
				{0xA2 , 0x3C},
				{0xA3 , 0x4F},
				{0xA4 , 0x5F},
				{0xA5 , 0x6F},
				{0xA6 , 0x8A},
				{0xA7 , 0x9F},
				{0xA8 , 0xB4}, 
				{0xA9 , 0xC6},
				{0xAA , 0xD3},
				{0xAB , 0xDD},
				{0xAC , 0xE5},
				{0xAD , 0xF1},
				{0xAE , 0xFA},
				{0xAF , 0xFF},	
				
			2:			
				{0x9F , 0x0E},
				{0xA0 , 0x1C},
				{0xA1 , 0x34},
				{0xA2 , 0x48},
				{0xA3 , 0x5A},
				{0xA4 , 0x6B},
				{0xA5 , 0x7B},
				{0xA6 , 0x95},
				{0xA7 , 0xAB},
				{0xA8 , 0xBF},
				{0xA9 , 0xCE},
				{0xAA , 0xD9},
				{0xAB , 0xE4},
				{0xAC , 0xEC},
				{0xAD , 0xF7},
				{0xAE , 0xFD},
				{0xAF , 0xFF},
				
			3:
				{0x9F , 0x10},
				{0xA0 , 0x20},
				{0xA1 , 0x38},
				{0xA2 , 0x4E},
				{0xA3 , 0x63},
				{0xA4 , 0x76},
				{0xA5 , 0x87},
				{0xA6 , 0xA2},
				{0xA7 , 0xB8},
				{0xA8 , 0xCA},
				{0xA9 , 0xD8},
				{0xAA , 0xE3},
				{0xAB , 0xEB},
				{0xAC , 0xF0},
				{0xAD , 0xF8},
				{0xAE , 0xFD},
				{0xAF , 0xFF},
	
			4:
				{0x9F , 0x14},
				{0xA0 , 0x28},
				{0xA1 , 0x44},
				{0xA2 , 0x5D},
				{0xA3 , 0x72},
				{0xA4 , 0x86},
				{0xA5 , 0x95},
				{0xA6 , 0xB1},
				{0xA7 , 0xC6},
				{0xA8 , 0xD5},
				{0xA9 , 0xE1},
				{0xAA , 0xEA},
				{0xAB , 0xF1},
				{0xAC , 0xF5},
				{0xAD , 0xFB},
				{0xAE , 0xFE},
				{0xAF , 0xFF},
				
			5:								//largest gamma curve
				{0x9F , 0x15},
				{0xA0 , 0x2A},
				{0xA1 , 0x4A},
				{0xA2 , 0x67},
				{0xA3 , 0x79},
				{0xA4 , 0x8C},
				{0xA5 , 0x9A},
				{0xA6 , 0xB3},
				{0xA7 , 0xC5},
				{0xA8 , 0xD5},
				{0xA9 , 0xDF},
				{0xAA , 0xE8},
				{0xAB , 0xEE},
				{0xAC , 0xF3},
				{0xAD , 0xFA},
				{0xAE , 0xFD},
				{0xAF , 0xFF}, */
		//-----------GAMMA Select End--------------//
									
		{0xc0 , 0x00,"",eReadWrite},									
		{0xc1 , 0x10,"",eReadWrite},									
		{0xc2 , 0x1C,"",eReadWrite},									
		{0xc3 , 0x30,"",eReadWrite},									
		{0xc4 , 0x43,"",eReadWrite},									
		{0xc5 , 0x54,"",eReadWrite},									
		{0xc6 , 0x65,"",eReadWrite},									
		{0xc7 , 0x75,"",eReadWrite},									
		{0xc8 , 0x93,"",eReadWrite},									
		{0xc9 , 0xB0,"",eReadWrite},									
		{0xca , 0xCB,"",eReadWrite},									
		{0xcb , 0xE6,"",eReadWrite},									
		{0xcc , 0xFF,"",eReadWrite},									
		{0xf0 , 0x02,"",eReadWrite},									
		{0xf1 , 0x01,"",eReadWrite},									
		{0xf2 , 0x01,"",eReadWrite},									
		{0xf3 , 0x30,"",eReadWrite},									
		{0xf9 , 0x9f,"",eReadWrite},									
		{0xfa , 0x78,"",eReadWrite},									
																	   
		//---------------------------------------------------------------
		{0xfe , 0x01,"",eReadWrite},// set page1											  
																	   
		{0x00 , 0xf5,"",eReadWrite},									
		{0x02 , 0x1a,"",eReadWrite},									
		{0x0a , 0xa0,"",eReadWrite},									
		{0x0b , 0x60,"",eReadWrite},									
		{0x0c , 0x08,"",eReadWrite},									
		{0x0e , 0x4c,"",eReadWrite},									
		{0x0f , 0x39,"",eReadWrite},									
		{0x11 , 0x3f,"",eReadWrite},									
		{0x12 , 0x72,"",eReadWrite},									
		{0x13 , 0x13,"",eReadWrite},									
		{0x14 , 0x42,"",eReadWrite},									
		{0x15 , 0x43,"",eReadWrite},									
		{0x16 , 0xc2,"",eReadWrite},									
		{0x17 , 0xa8,"",eReadWrite},									
		{0x18 , 0x18,"",eReadWrite},									
		{0x19 , 0x40,"",eReadWrite},									
		{0x1a , 0xd0,"",eReadWrite},									
		{0x1b , 0xf5,"",eReadWrite},									
		{0x70 , 0x40,"",eReadWrite},									
		{0x71 , 0x58,"",eReadWrite},									
		{0x72 , 0x30,"",eReadWrite},									
		{0x73 , 0x48,"",eReadWrite},									
		{0x74 , 0x20,"",eReadWrite},									
		{0x75 , 0x60,"",eReadWrite},									
		{0x77 , 0x20,"",eReadWrite},									
		{0x78 , 0x32,"",eReadWrite},									
		{0x30 , 0x03,"",eReadWrite},									
		{0x31 , 0x40,"",eReadWrite},									
		{0x32 , 0xe0,"",eReadWrite},									
		{0x33 , 0xe0,"",eReadWrite},									
		{0x34 , 0xe0,"",eReadWrite},									
		{0x35 , 0xb0,"",eReadWrite},									
		{0x36 , 0xc0,"",eReadWrite},									
		{0x37 , 0xc0,"",eReadWrite},									
		{0x38 , 0x04,"",eReadWrite},									
		{0x39 , 0x09,"",eReadWrite},									
		{0x3a , 0x12,"",eReadWrite},									
		{0x3b , 0x1C,"",eReadWrite},									
		{0x3c , 0x28,"",eReadWrite},									
		{0x3d , 0x31,"",eReadWrite},									
		{0x3e , 0x44,"",eReadWrite},									
		{0x3f , 0x57,"",eReadWrite},									
		{0x40 , 0x6C,"",eReadWrite},									
		{0x41 , 0x81,"",eReadWrite},									
		{0x42 , 0x94,"",eReadWrite},									
		{0x43 , 0xA7,"",eReadWrite},									
		{0x44 , 0xB8,"",eReadWrite},									
		{0x45 , 0xD6,"",eReadWrite},									
		{0x46 , 0xEE,"",eReadWrite},									
		{0x47 , 0x0d,"",eReadWrite},									
		{0xfe , 0x00,"",eReadWrite}, // set page0
		 
		//-----------Update the registers 2010/07/06-------------//
		//Registers of Page0
		{0xfe , 0x00,"",eReadWrite}, // set page0
		{0x10 , 0x26,"",eReadWrite},								   
		{0x11 , 0x0d,"",eReadWrite},	// fd,modified by mormo 2010/07/06								 
		{0x1a , 0x2a,"",eReadWrite},	// 1e,modified by mormo 2010/07/06									
	
		{0x1c , 0x49,"",eReadWrite}, // c1,modified by mormo 2010/07/06								  
		{0x1d , 0x9a,"",eReadWrite}, // 08,modified by mormo 2010/07/06								  
		{0x1e , 0x61,"",eReadWrite}, // 60,modified by mormo 2010/07/06								  
	
		{0x3a , 0x20,"",eReadWrite},
	
		{0x50 , 0x14,"",eReadWrite},	// 10,modified by mormo 2010/07/06								 
		{0x53 , 0x80,"",eReadWrite},									
		{0x56 , 0x80,"",eReadWrite},
		
		{0x8b , 0x20,"",eReadWrite}, //LSC								 
		{0x8c , 0x20,"",eReadWrite},									
		{0x8d , 0x20,"",eReadWrite},									
		{0x8e , 0x14,"",eReadWrite},									
		{0x8f , 0x10,"",eReadWrite},									
		{0x90 , 0x14,"",eReadWrite},									
	
		{0x94 , 0x02,"",eReadWrite},									
		{0x95 , 0x07,"",eReadWrite},									
		{0x96 , 0xe0,"",eReadWrite},									
	
		{0xb1 , 0x40,"",eReadWrite}, // YCPT								   
		{0xb2 , 0x40,"",eReadWrite},									
		{0xb3 , 0x40,"",eReadWrite},
		{0xb6 , 0xe0,"",eReadWrite},
	
		{0xd0 , 0xcb,"",eReadWrite}, // AECT	c9,modifed by mormo 2010/07/06								  
		{0xd3 , 0x48,"",eReadWrite}, // 80,modified by mormor 2010/07/06							 
	              
		{0xf2 , 0x02,"",eReadWrite},									
		{0xf7 , 0x12,"",eReadWrite},
		{0xf8 , 0x0a,"",eReadWrite},
	
		//Registers of Page1
		{0xfe , 0x01,"",eReadWrite},// set page1	  
		{0x02 , 0x20,"",eReadWrite},
		{0x04 , 0x10,"",eReadWrite},
		{0x05 , 0x08,"",eReadWrite},
		{0x06 , 0x20,"",eReadWrite},
		{0x08 , 0x0a,"",eReadWrite},
	
		{0x0e , 0x44,"",eReadWrite},									
		{0x0f , 0x32,"",eReadWrite},
		{0x10 , 0x41,"",eReadWrite},									
		{0x11 , 0x37,"",eReadWrite},									
		{0x12 , 0x22,"",eReadWrite},									
		{0x13 , 0x19,"",eReadWrite},									
		{0x14 , 0x44,"",eReadWrite},									
		{0x15 , 0x44,"",eReadWrite},	
		          
		{0x19 , 0x50,"",eReadWrite},									
		{0x1a , 0xd8,"",eReadWrite}, 
		          
		{0x32 , 0x10,"",eReadWrite}, 
		          
		{0x35 , 0x00,"",eReadWrite},									
		{0x36 , 0x80,"",eReadWrite},									
		{0x37 , 0x00,"",eReadWrite}, 
		//-----------Update the registers end---------//
	              
	              
		{0xfe , 0x00,"",eReadWrite}, // set page0
		{0xd2 , 0x90,"",eReadWrite},	
#endif
		{0x0000 ,0x00,"eTableEnd",eTableEnd}

};

const IsiRegDescription_t GC0308_g_vga[] =
{
    {0x0000 ,0x00,"eTableEnd",eTableEnd}
};

const IsiRegDescription_t GC0308_g_1600x1200[] =
{
	           
    {0x0000 ,0x00,"eTableEnd",eTableEnd}

};


