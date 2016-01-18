///============================================================================
/// Copyright 2011 Broadcom Corporation
///
/// This program is free software; you can redistribute it and/or modify
/// it under the following terms: 
///
/// Redistribution and use in source and binary forms, with or without 
/// modification, are permitted provided that the following conditions are met:
///  Redistributions of source code must retain the above copyright notice, this
///  list of conditions and the following disclaimer.
///  Redistributions in binary form must reproduce the above copyright notice, 
///  this list of conditions and the following disclaimer in the documentation 
///  and/or other materials provided with the distribution.
///  Neither the name of Broadcom nor the names of its contributors may be used 
///  to endorse or promote products derived from this software without specific 
///  prior written permission.
///  
///  THIS SOFTWARE IS PROVIDED “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
///  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
///  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
///  BROADCOM BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
///  EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, 
///  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;LOSS OF USE, DATA, OR PROFITS; 
///  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
///  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR 
///  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
///  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/// ---------------------------------------------------------------------------

#ifndef _HARDWARE_GPS_FACT_TEST_H
#define _HARDWARE_GPS_FACT_TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define GPS_FACT_TEST_INTERFACE "fact-test"

typedef uint32_t GpsFactTestMode;
#define GPS_FACT_TEST_ONCE  0 
#define GPS_FACT_TEST_CONT  1

typedef uint32_t GpsFactTestItem;
#define GPS_FACT_TEST_CW    0
#define GPS_FACT_TEST_CN0   1 
#define GPS_FACT_TEST_FRQ   2 
#define GPS_FACT_TEST_WER   3 
#define GPS_FACT_TEST_ACQ   4 

typedef uint32_t GpsCntinStatus;
#define    GPS_CNTIN_NOT_USED   0
#define    GPS_CNTIN_OK 1
#define    GPS_CNTIN_NOK 2
#define    GPS_CNTIN_NO_INPUT 3
#define    GPS_CNTIN_USER_DEAD 4
#define    GPS_CNTIN_USER_CANCEL 5

typedef struct {
    int prn;
    GpsFactTestItem TestItems;
    GpsFactTestItem TestMode;
    int AvgIntrvlSec;
} GpsFactTestParams;

typedef struct {
    GpsFactTestItem TestItem;
    int SVid;
    int EnergyFound;
    int AvgValid;
    double LastSSdBm;
    double AvgSSdBm;

    double LastSNRdbHZ;
    double AvgSNRdBHZ;

    double NoiseFigure;
    int FrequencyMeasured;

    double LastFreqPpu;
    double LastFreqUncPpu;

    int WerMeasured;
    unsigned int GoodWordCnt;
    unsigned int TotalWordCnt;

    GpsCntinStatus CntInStatus;
    int CntinMeasured;
    double CntinOffsetPpu;

    int RtcMeasured;
    double RtcOffsetPpu;

    int RfAgc;

    unsigned int SignalDropCount;

    int ValidCW;
    double DopplerPpuCW;
    double DopplerUncPpuCW;
    double SSdBmCW;

} GpsFactTestResponse;


typedef void (*gps_fact_test_response_callback) (GpsFactTestResponse *result);

typedef struct
{
    gps_fact_test_response_callback result_cb;
} GpsFactTestCallbacks;


typedef struct
{
    void (*init) (GpsFactTestCallbacks *);
    void (*test_start) (GpsFactTestParams *params, int duration_sec);
} GpsFactTestInterface;

#ifdef __cplusplus
}
#endif


#endif

