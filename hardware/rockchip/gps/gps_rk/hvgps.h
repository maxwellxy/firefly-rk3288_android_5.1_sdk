///////////////////////////////////////////////////////////////////////////////////
//
// Filename:     hvgps.h
// Author:       sjchen
// Copyright: 
// Date: 2012/08/30
// Description:
//		the macor and extern function of GPS interface
//
// Revision:
//		0.0.1
//
///////////////////////////////////////////////////////////////////////////////////
#ifndef _HV_GPS_H__   
#define _HV_GPS_H__


////////////////////////////////////////////////
// macro definition
////////////////////////////////////////////////

//this macro is for debug
//#define DEBUG_LEVEL


#ifndef TRUE
#define TRUE			1
#endif

#ifndef FALSE
#define FALSE			0
#endif

#define  LOG_TAG  "hvgps"
#define  LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__) //
#define  LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__) 
#define  LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__) 

#ifndef LOGD
#define LOGD(...) ALOGD( __VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(...) ALOGE( __VA_ARGS__)
#endif

#ifndef LOGW
#define LOGW(...) ALOGW( __VA_ARGS__)
#endif

///////////////////////////////////////////////////////////////////////////////////
// 
// extern global function declaration
//
///////////////////////////////////////////////////////////////////////////////////
extern void  SetOutputFolder(char* strFolderName);
extern int   LoadParameter();
extern int   GetParameterAge();
extern "C" 
{
int  InitGbc ();
int  DeinitGbc();
int  GPS_ColdStart(void);
int  GPS_WarmStart(void);
int  GPS_HotStart(void);
int  GPS_Stop(void);
int  GPS_ForceResetStart(void);
};

#endif// _HV_GPS_H__
