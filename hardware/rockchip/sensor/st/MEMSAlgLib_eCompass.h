/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : MEMSAlgLib_AirMouse.h
* Department         : APM G.C. FAE TEAM
* Author             : Travis Tu
* Date First Issued  : 2009.11.12
********************************************************************************
* History:
* 2009.7.21  V0.1
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR REFERENCE OR
* EDUCATION. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
/*******************************************************************************
* Copyright (c) 2011 Wind River Systems, Inc.
*
* The right to copy, distribute, modify, or otherwise make use
* of this software may be licensed only pursuant to the terms
* of an applicable Wind River license agreement.
*
*******************************************************************************/



#ifndef __MEMSALGLIB_ECOMPASS_H
#define __MEMSALGLIB_ECOMPASS_H

typedef struct
{
    signed short ax;  //x value
    signed short ay;  //y value
    signed short az;  //z value
    signed short mx;  //x value
    signed short my;  //y value
    signed short mz;  //z value
    signed long time; //system time in ms
}ECOM_ValueTypeDef;

/***********************************************************************/
/**                             e Compass                             **/
/***********************************************************************/
//init the eCompass Algorithm
//Only invoked once
void          MEMSAlgLib_eCompass_Init(signed short oneGravityValue, signed short oneGaussValue);

//Hard Iron Calibration Part
unsigned char  MEMSAlgLib_eCompass_IsCalibrated(void);
void           MEMSAlgLib_eCompass_GetCalibration(signed short * mxOff, signed short * myOff, signed short * mzOff);
void           MEMSAlgLib_eCompass_SetCalibration(signed short mx,signed short my,signed short mz);
void           MEMSAlgLib_eCompass_ForceCalibration(void); //equal to init
unsigned short MEMSAlgLib_eCompass_GalibrationProgress_Percentage(void); // could be >100

//return the calibration quality in a positive value in float
float          MEMSAlgLib_eCompass_Galibration_Quality(void); //<=1 Worst, <=5 Bad, <=10 Good, Others Best could be even > 100

//evolution of hard calibration offset data; default disabled.
void MEMSAlgLib_eCompass_enable_evo(void);  //to enable the evolution of clibration procedure
void MEMSAlgLib_eCompass_disable_evo(void); //to disable the evolution of clibration procedure

//Update the Algorithm
//invoked once every loop interval at 15~100ms for calibration
//interval is not limitated for azimuth polling
void          MEMSAlgLib_eCompass_Update(ECOM_ValueTypeDef* v);

//Get the Azimuth back from algorithm
signed  short MEMSAlgLib_eCompass_Get_Azimuth(void);

//Get Pitch -180 ~ +180
signed  short MEMSAlgLib_eCompass_Get_Pitch(void);

//Get Roll -90 ~ +90
signed short MEMSAlgLib_eCompass_Get_Roll(void);

//Get Version of Library
float MEMSAlgLib_eCompass_Get_Version(void);

//whether a unexpected magnet is in short range
//return 0 none, return 1 yes
unsigned char MEMSAlgLib_eCompass_Magnet_Interfere_Now(void);

#endif


//////////////////////////////////////////////////
///////////////  Example of Lib //////////////////
//////////////////////////////////////////////////
/*
//simple example
{
  MEMSAlgLib_eCompass_Init(1024,1055);
  while(1)
  {
    //25ms as interval before calibration finished.
    __GetACC();//platform  API
    __GetMAG();//platform  API
    __GetSystemTimeMS();//platform  API
    __PutIntoStucture(ECOM_ValueTypeDef v);//platform  API
    //caculate Azimuth
    MEMSAlgLib_eCompass_Update(ECOM_ValueTypeDef* v);
    if(MEMSAlgLib_eCompass_IsCalibrated())
    {
      MEMSAlgLib_eCompass_Get_Azimuth();
      __Show_Azimuth_Angle_On_LCD();//platform  API
    }
    pitchAngle = MEMSAlgLib_eCompass_Get_Pitch();
    rollAngle = MEMSAlgLib_eCompass_Get_Roll();
    //delay
    __Delay(50) //delay 50ms
  }
}

//complex way
{
  MEMSAlgLib_eCompass_Init(1024,1055);

  //if find the last record of calibration data
  if(__FindRecord__()) //platform related API
    MEMSAlgLib_eCompass_SetCalibration(signed short mx,signed short my,signed short mz);

  //if need to open calibraiton evolution function , default disabled, need to take 1/100 workload average
  if(__NeedCalibration_Evolution())
    MEMSAlgLib_eCompass_enable_evo();
  else
    MEMSAlgLib_eCompass_disable_evo();

  //endless while
  while(1)
  {
    //25ms as interval before calibration finished.
    __GetACC();//platform related API
    __GetMAG();//platform related API
    __GetSystemTimeMS();//platform related API
    __PutIntoStucture(ECOM_ValueTypeDef v);//platform related API
    //get old calibration data
    signed short MAGOFFX1,MAGOFFY1,MAGOFFZ1;
    MEMSAlgLib_eCompass_GetCalibration(&MAGOFFX1,&MAGOFFY1,&MAGOFFZ1);
    //Azimuth
    MEMSAlgLib_eCompass_Update(ECOM_ValueTypeDef* v);
    if(MEMSAlgLib_eCompass_IsCalibrated())
    {
      MEMSAlgLib_eCompass_Get_Azimuth();
      __Show_Azimuth_Angle_On_LCD();//platform related API
      if(!__FindRecord__())//platform related API
      {
        signed short MAGOFFX2,MAGOFFY2,MAGOFFZ2;
        MEMSAlgLib_eCompass_GetCalibration(&MAGOFFX2,&MAGOFFY2,&MAGOFFZ2);
        if( (MAGOFFX1!=MAGOFFX2) || (MAGOFFY1!=MAGOFFY2) || (MAGOFFZ1!=MAGOFFZ2) )
        {
          __WriteFileRecord(MAGOFFX2, MAGOFFY2, MAGOFFZ2);//platform related API
        }
      }
    }
    pitchAngle = MEMSAlgLib_eCompass_Get_Pitch();
    rollAngle = MEMSAlgLib_eCompass_Get_Roll();
    __Delay(50) //delay 50ms
  }
}
*/
