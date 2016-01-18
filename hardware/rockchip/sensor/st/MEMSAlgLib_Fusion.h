/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name		: MEMSAlgLib_Fusion.h
* Authors		: ST Motion Sensors & iNEMO 
*			    : Travis Tu (travis.tu@st.com)
* Version		: V 1.43
* Date			: 2011/11/08
* Description   : iNEMO 9-axis Fusion Algorithm Library. For the Release Version,
*               : Pls contact with Vincent Xu (vincent.xu@st.com)
*******************************************************************************/
/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __MEMSALGLIB_FUSION_H
#define __MEMSALGLIB_FUSION_H
typedef struct
{
  long ax;  
  long ay;  
  long az;  
  long mx;  
  long my;  
  long mz;  
  long gx;  
  long gy;
  long gz;
  long Azimuth;
  long pitch;
  long roll;
  long time; //system time in ms
}NineAxisTypeDef; 

typedef struct
{
  float a;  
  float b;
  float c;
  float d;
}QuaternionTypeDef; 

typedef struct
{
  float x;  
  float y;
  float z;
}GravityTypeDef; 

typedef struct
{
  float x;  
  float y;
  float z;
}LinearAccTypeDef; 

typedef struct
{
  float x;  
  float y;
  float z;
}AccTypeDef; 

typedef struct
{
	QuaternionTypeDef q;
	GravityTypeDef	  g;
	LinearAccTypeDef  l;
}FusionTypeDef;


typedef struct
{
  float pitch;  
  float roll;
  float yaw;
}PRYTypeDef; 


//void MEMSAlgLib_Fusion_Init(long oneGravityValue, long oneGaussValue, float oneDSPValue);

//gyro in ST's LSB short to long
//acc in mGravity
//mag in mGauss
FusionTypeDef MEMSAlgLib_Fusion_Update(NineAxisTypeDef v);

//to get the version of this algorithm 
float MEMSAlgLib_Fusion_Get_Version(void);

//to get the filted ACC value after LPF. 
AccTypeDef MEMSAlgLib_Fusion_Get_ACC(void);

//as an Air Mouse Mode in arc 
//Picth: Vertical move, up Positive
//Roll: Scroll move, anti-clockwise Positive
//Yaw: Horizontal move, right Positive 
PRYTypeDef GetAirMouseMove(void);

//to get gyro 0-rate offset 
void MEMSAlgLib_Fusion_Get_GyroOffset(float *x, float *y, float *z); //unit: same LSB as input structure

//to get the record data
int  MEMSAlgLib_Fusion_GetStoreValuesLength(void);
void MEMSAlgLib_Fusion_GetStoreValues(unsigned char* buf);
void MEMSAlgLib_Fusion_SetStoreValues(unsigned char* buf);



//property API
//algorithm customization API
void MEMSAlgLib_Fusion_SetMode(int mode); //default 9-axis fusion, //0-GyroMode, 1-AccMagMode, 2-AccMagGyroMode, 3-AccGyroMode
void MEMSAlgLib_FuSION_LPF(int active); //default on
void MEMSAlgLib_FuSION_AntiInterfere(int active); //default off
void MEMSAlgLib_Fusion_SelfLearning(int acitve);  //default on
char MEMSAlgLib_Fusion_SelfLenrning_DoneON_XYZ(int idXYZ); //whether self learning is done on axis x 0 / y 1 /z 2

//Supplier property API
void MEMSAlgLib_Config(int id, float value); //only for professional.
long MEMSAlgLib_Get(int id); //only for professional. 
QuaternionTypeDef MEMSAlgLib_GetQ3(void);
QuaternionTypeDef MEMSAlgLib_GetQ6(void);
#endif


