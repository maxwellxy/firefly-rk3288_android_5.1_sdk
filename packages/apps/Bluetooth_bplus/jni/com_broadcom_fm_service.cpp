/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/


#define LOG_TAG "FmServiceJni"

#define LOG_NDEBUG 0




#define CHECK_CALLBACK_ENV                                                      \
   if (!checkCallbackThread()) {                                                \
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);\
       return;                                                                  \
   }

/* I2C_FM_SEARCH_METHOD */
#define I2C_FM_SEARCH_NORMAL            0x00
#define I2C_FM_SEARCH_PRESET            0x01  /* default: preset scan with CO */
#define I2C_FM_SEARCH_RSSI              0x02
#define I2C_FM_SEARCH_PRESET_SNR        0x03  /* preset scan with SNR */

/* scan mode */
#define BTA_FM_PRESET_SCAN      I2C_FM_SEARCH_PRESET        /* preset scan : bit0 = 1 */
#define BTA_FM_NORMAL_SCAN      I2C_FM_SEARCH_NORMAL        /* normal scan : bit0 = 0 */
typedef unsigned char tBTA_FM_SCAN_METHOD;

/* frequency scanning direction */
#define BTA_FM_SCAN_DOWN        0x00        /* bit7 = 0 scanning toward lower frequency */
#define BTA_FM_SCAN_UP          0x80        /* bit7 = 1 scanning toward higher frequency */
typedef unsigned char tBTA_FM_SCAN_DIR;

#define BTA_FM_SCAN_FULL        (BTA_FM_SCAN_UP | BTA_FM_NORMAL_SCAN|0x02)     /* full band scan */
#define BTA_FM_FAST_SCAN        (BTA_FM_SCAN_UP | BTA_FM_PRESET_SCAN)       /* use preset scan */
#define BTA_FM_SCAN_NONE        0xff

typedef unsigned char tBTA_FM_SCAN_MODE;

#define BTA_FM_SCAN_FULL        (BTA_FM_SCAN_UP | BTA_FM_NORMAL_SCAN|0x02)     /* full band scan */
#define BTA_FM_FAST_SCAN        (BTA_FM_SCAN_UP | BTA_FM_PRESET_SCAN)       /* use preset scan */
#define BTA_FM_SCAN_NONE        0xff




/*Fm App audio config value*/
#define FM_AUDIO_PATH_NONE            0
#define FM_AUDIO_PATH_SPEAKER         1
#define FM_AUDIO_PATH_WIRED_HEADSET   2

#define FM_ROUTE_NONE       0x00    /* No Audio output */
#define FM_ROUTE_DAC        0x01    /* routing over analog output */
#define FM_ROUTE_I2S        0x02    /* routing over digital (I2S) output */

// FM_AUDIO_PATH can be defined in make file.
// If this macro is not defined the default path is DAC
#ifndef FM_AUDIO_PATH
#define FM_AUDIO_PATH FM_ROUTE_DAC
#endif



#include "com_android_bluetooth.h"
#include "hardware/bt_fm.h"
#include "utils/Log.h"
#include "android_runtime/AndroidRuntime.h"

#include <string.h>



namespace android {

static btfm_rds_update_t      previous_rds_update;
static jint rds_type_save;
static jboolean af_on_save;
static jboolean rds_on_save;
static int processing_scan = 0;

/* Local references to java callback event functions. */
static jmethodID method_onRadioOnEvent;
static jmethodID method_onRadioOffEvent;
static jmethodID method_onRadioMuteEvent;
static jmethodID method_onRadioTuneEvent;
static jmethodID method_onRadioSearchEvent;
static jmethodID method_onRadioSearchCompleteEvent;
static jmethodID method_onRadioAfJumpEvent;
static jmethodID method_onRadioAudioModeEvent;
static jmethodID method_onRadioAudioPathEvent;
static jmethodID method_onRadioAudioDataEvent;
static jmethodID method_onRadioRdsModeEvent;
static jmethodID method_onRadioRdsTypeEvent;
static jmethodID method_onRadioRdsUpdateEvent;
static jmethodID method_onRadioDeemphEvent;
static jmethodID method_onRadioScanStepEvent;
static jmethodID method_onRadioRegionEvent;
static jmethodID method_onRadioNflEstimationEvent;
static jmethodID method_onRadioVolumeEvent;

static const btfm_interface_t *sFmIf = NULL;
static jobject mCallbacksObj = NULL;
static JNIEnv *sCallbackEnv = NULL;

static int sFmAppAudioPath = -1;
static int sCurrentBtaPath = -1;

static jboolean setRdsRdbsNative(jint rdsType);

static void resetFmData()
{
    sFmAppAudioPath = -1;
    sCurrentBtaPath = -1;
}

static bool checkCallbackThread() {
    // Always fetch the latest callbackEnv from AdapterService.
    // Caching this could cause this sCallbackEnv to go out-of-sync
    // with the AdapterService's ENV if an ASSOCIATE/DISASSOCIATE event
    // is received
    //if (sCallbackEnv == NULL) {
    sCallbackEnv = getCallbackEnv();
    //}

    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (sCallbackEnv != env || sCallbackEnv == NULL) return false;
    return true;
}


/**
 * Enable fm callback
 */
static void enable_callback(int status)
{
    CHECK_CALLBACK_ENV

    memset(&previous_rds_update, 0, sizeof(btfm_rds_update_t));
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioOnEvent, (jint)status);

}

/**
 * Disable fm callback
 */
static void disable_callback(int status)
{
    CHECK_CALLBACK_ENV

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioOffEvent, (jint)status);
}

/**
 * Fm tune event callback
 */
static void tune_callback(int status, int rssi, int snr, int freq)
{
    CHECK_CALLBACK_ENV

    memset(&previous_rds_update, 0, sizeof(btfm_rds_update_t));
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioTuneEvent,
        (jint)status, (jint)rssi,(jint)snr, (jint)freq);

}


/**
 * Fm mute event callback
 */
static void mute_callback(int status, BOOLEAN isMute)
{
    CHECK_CALLBACK_ENV

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioMuteEvent,
    (jint)status, (jboolean)isMute); 
}

/**
 * Fm search event callback
 */
static void search_callback(int status, int rssi, int snr, int freq)
{
    CHECK_CALLBACK_ENV
    if (processing_scan) {
        ALOGI("[JNI] - BTA_FM_SEARCH_EVT: rssi = %i, freq = %i snr = %i", rssi,
            freq, snr);
        sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioSearchEvent,
            (jint)rssi, (jint)snr, (jint)freq);
    }

}


/**
 * Fm search complete event callback
 */
static void search_complete_callback(int status, int rssi, int snr, int freq)
{
    CHECK_CALLBACK_ENV

    ALOGI("[JNI] - BTA_FM_SEARCH_CMPL_EVT: status = %i rssi = %i freq = %i snr = %i",
        status, rssi, freq, snr);
    memset(&previous_rds_update, 0, sizeof(btfm_rds_update_t));
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioSearchCompleteEvent,
        (jint)status, (jint)rssi, (jint)snr, (jint)freq);
    if (processing_scan)
        processing_scan = 0;

}


/**
 * Fm af jump event  callback
 */
static void af_jump_callback(int status, int rssi, int snr, int freq)
{
    CHECK_CALLBACK_ENV

    memset(&previous_rds_update, 0, sizeof(btfm_rds_update_t));
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioAfJumpEvent,
        (jint)status, (jint)rssi, (jint)freq);
    ALOGI("[JNI] - TRANSMITTING EVENT BTA_FM_AF_JMP_EVT :  status = %i rssi = %i freq = %i",
        status, rssi, freq);
    if (processing_scan)
        processing_scan = 0;

}


/**
 * Fm audio mode  callback
 */
static void audio_mode_callback(int status, int audioMode)
{
    CHECK_CALLBACK_ENV

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioAudioModeEvent,
        (jint)status, (jint)audioMode);

}


/**
 * Fm audio path callback
 */
static void audio_path_callback(int status, int audioPath)
{
    CHECK_CALLBACK_ENV

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioAudioPathEvent,
        (jint)status, (jint)sFmAppAudioPath);

}


/**
 * Fm audio data callback
 */
static void audio_data_callback(int status, int rssi, int snr, int audioMode)
{
    CHECK_CALLBACK_ENV

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioAudioDataEvent,
        (jint)status, (jint)rssi, (jint)audioMode, (jint)snr);

}


/**
 * Fm rds mode callback
 */
static void rds_mode_callback(int status, BOOLEAN rdsOn, BOOLEAN afOn)
{
    CHECK_CALLBACK_ENV

    ALOGI("%s: BTA_FM_RDS_MODE_EVT", __FUNCTION__);
    if(status == BT_STATUS_SUCCESS)
        {
            af_on_save = (jboolean)afOn;
            rds_on_save = (jboolean)rdsOn;
            setRdsRdbsNative(rds_type_save);
        }
    else{
          sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioRdsModeEvent,
        (jint)status, (jboolean)rdsOn,(jboolean)afOn,(jint)rds_type_save);
    }

}


/**
 * Fm rds type callback
 */
static void rds_type_callback(int status, int rdsType)
{
    CHECK_CALLBACK_ENV

    ALOGI("%s: BTA_FM_RDS_TYPE_EVT", __FUNCTION__);

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioRdsModeEvent,
        (jint)status, rds_on_save, af_on_save,(jint)rdsType);

}


/**
 * Fm deempasis param callback
 */
static void deemphasis_callback(int status, int timeConst)
{
    CHECK_CALLBACK_ENV

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioDeemphEvent,
        (jint)status, (jint)timeConst);

}


/**
 * Fm scan step callback
 */
static void scan_step_callback(int status, int scanStep)
{
    CHECK_CALLBACK_ENV

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioScanStepEvent,
        (jint)scanStep);

}


/**
 * Fm region callback
 */
static void region_callback(int status, int region)
{
    CHECK_CALLBACK_ENV

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioRegionEvent,
        (jint)status, (jint)region);

}


/**
 * Fm noise floor level callback
 */
static void nfl_callback(int status, int noiseFloor)
    {
        CHECK_CALLBACK_ENV

        sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioNflEstimationEvent,
            (jint)noiseFloor);

    }


/**
 * Fm volume callback
 */
static void volume_callback(int status, int volume)
{
    CHECK_CALLBACK_ENV

    ALOGI("volume_callback : status = %d", status);

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioVolumeEvent,
        (jint)status, (jint)volume);

}


/* Assumes data exists. Screens the text field for illegal (non ASCII) characters. */
static void screenData(char* text) {
    int i = 0;
    char c = text[i];

    while ((i < 65) && (0 != c)) {
        /* Check legality of all characters. */
        if ((c < 32) || (c > 126)) {
            text[i] = '*';
        }
        /* Look at next character. */
        c = text[++i];
    }
    /* Cap for safety at end of 64 bytes. */
    text[64] = 0;
}

/**
 * Fm rds data callback
 */
static void rds_data_callback(int status, int dataType, int index,
                    char *radioText)
{
    CHECK_CALLBACK_ENV

    ALOGI("%s: BTA_FM_RDS_UPD_EVT", __FUNCTION__);
    if (NULL != radioText) {
        /* Pre-process data. */
        screenData(radioText);
        //not to overload system with useless events check if data has changed
        ALOGI("%s: BTA_FM_RDS_UPD_EVT, previous_rds%s", __FUNCTION__,previous_rds_update.text);
        ALOGI("%s: BTA_FM_RDS_UPD_EVT, new_rds%s", __FUNCTION__,radioText);
        ALOGI("%s: BTA_FM_RDS_UPD_EVT, memcmp 0x%8x", __FUNCTION__,
                        memcmp(&previous_rds_update.text, radioText, 65));
        if(memcmp(&previous_rds_update.text, radioText, 65)) {
            if (memcpy(&previous_rds_update.text, radioText, 65)) {
            /* Transmit upwards. */
            sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRadioRdsUpdateEvent,
                (jint)status, (jint)dataType,(jint)index,sCallbackEnv->NewStringUTF(radioText));
            }
        }

    }
}

/**
 * Fm rtp data callback
 */
static void rtp_data_callback(btfm_rds_rtp_info_t *rtpInfo)
{
    CHECK_CALLBACK_ENV

    ALOGI("%s: BTA_RDS_RTP_EVT", __FUNCTION__);
    if (NULL != rtpInfo) {
        int k = 0;

            ALOGI("%s: bRunning: %d bToggle = %d tag_num - %d",__FUNCTION__,
               rtpInfo->running,rtpInfo->tag_toggle,
               rtpInfo->tag_num);
           k = rtpInfo->tag_num;
           while(k){

                ALOGI("%s: content_type: %d start: %d len: %d",__FUNCTION__,
                   rtpInfo->tag[k].content_type,rtpInfo->tag[k].start,
                   rtpInfo->tag[k].len);
                k--;
           }
           //TODO: this event is not sent to java layer as of now. If requirement arises
           // this event as to passed to app.
    }
}


static btfm_callbacks_t sFmCallbacks = {
    sizeof(btfm_callbacks_t),
    enable_callback,
    disable_callback,
    tune_callback,
    mute_callback,
    search_callback,
    search_complete_callback,
    af_jump_callback,
    audio_mode_callback,
    audio_path_callback,
    audio_data_callback,
    rds_mode_callback,
    rds_type_callback,
    deemphasis_callback,
    scan_step_callback,
    region_callback,
    nfl_callback,
    volume_callback,
    rds_data_callback,
    rtp_data_callback
};


static void classInitNative(JNIEnv* env, jclass clazz) {


    method_onRadioOnEvent =
        env->GetMethodID(clazz, "onRadioOnEvent", "(I)V");
    method_onRadioOffEvent =
        env->GetMethodID(clazz, "onRadioOffEvent", "(I)V");
    method_onRadioMuteEvent =
        env->GetMethodID(clazz, "onRadioMuteEvent", "(IZ)V");
    method_onRadioTuneEvent =
        env->GetMethodID(clazz, "onRadioTuneEvent", "(IIII)V");
    method_onRadioSearchEvent =
        env->GetMethodID(clazz, "onRadioSearchEvent", "(III)V");
    method_onRadioSearchCompleteEvent =
        env->GetMethodID(clazz, "onRadioSearchCompleteEvent", "(IIII)V");
    method_onRadioAfJumpEvent =
        env->GetMethodID(clazz, "onRadioAfJumpEvent", "(III)V");
    method_onRadioAudioPathEvent =
        env->GetMethodID(clazz, "onRadioAudioPathEvent", "(II)V");
    method_onRadioAudioModeEvent =
        env->GetMethodID(clazz, "onRadioAudioModeEvent", "(II)V");
    method_onRadioAudioDataEvent =
        env->GetMethodID(clazz, "onRadioAudioDataEvent", "(IIII)V");
    method_onRadioRdsModeEvent =
        env->GetMethodID(clazz, "onRadioRdsModeEvent", "(IZZI)V");
    method_onRadioRdsTypeEvent =
        env->GetMethodID(clazz, "onRadioRdsTypeEvent", "(II)V");
    method_onRadioRdsUpdateEvent =
        env->GetMethodID(clazz, "onRadioRdsUpdateEvent", "(IIILjava/lang/String;)V");
    method_onRadioDeemphEvent =
        env->GetMethodID(clazz, "onRadioDeemphEvent", "(II)V");
    method_onRadioScanStepEvent =
        env->GetMethodID(clazz, "onRadioScanStepEvent", "(I)V");
    method_onRadioRegionEvent =
        env->GetMethodID(clazz, "onRadioRegionEvent", "(II)V");
    method_onRadioNflEstimationEvent =
        env->GetMethodID(clazz, "onRadioNflEstimationEvent", "(I)V");
    method_onRadioVolumeEvent =
        env->GetMethodID(clazz, "onRadioVolumeEvent", "(II)V");

    ALOGI("%s: succeeds", __FUNCTION__);
}

static const bt_interface_t* btIf;

static void initializeNative(JNIEnv *env, jobject object) {
    ALOGD("fm");

    if ( (btIf = getBluetoothInterface()) == NULL) {
        ALOGE("Fm module is not loaded");
        return;
    }

    if (sFmIf !=NULL) {
         ALOGE("Cleaning up  Fm Interface before initializing...");
         sFmIf->cleanup();
         sFmIf = NULL;
    }

    if (mCallbacksObj != NULL) {
         ALOGE("Cleaning up Fm callback object");
         env->DeleteGlobalRef(mCallbacksObj);
         mCallbacksObj = NULL;
    }

    if ( (sFmIf = (btfm_interface_t*)btIf->get_fm_interface()) == NULL) {
        ALOGE("Failed to get Fm Interface");
        return;
    }

    int status;
    if ( (status = sFmIf->init(&sFmCallbacks)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to initialize Fm, status: %d", status);
        sFmIf = NULL;
        return;
    }

    mCallbacksObj = env->NewGlobalRef(object);
    processing_scan = 0;
}

static void cleanupNative(JNIEnv *env, jobject object) {
    const bt_interface_t* btInf;
    int status;

    ALOGE("Fm cleanupNative");

    if ( (btInf = getBluetoothInterface()) == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (sFmIf !=NULL) {
        sFmIf->cleanup();
        sFmIf = NULL;
        resetFmData();
    }

    if (mCallbacksObj != NULL) {
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }
}


static jboolean enableFmNative(JNIEnv *env, jobject obj, jint functionalityMask)
{
    ALOGI("[JNI] - enableFmNative :    functionalityMask = %i", functionalityMask);
    jboolean ret = JNI_TRUE;
    int status;
    if (!sFmIf) return JNI_FALSE;
    if ((status = sFmIf->enable((int)functionalityMask)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM enable, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;
}

static jboolean disableFmNative(JNIEnv *env, jobject obj, jboolean bForcing)
{
    ALOGI("[JNI] - disableNative :");
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    if ((status = sFmIf->disable()) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM disable, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean tuneNative(JNIEnv *env, jobject obj, jint freq)
{
    ALOGI("[JNI] - tuneNative :    freq = %i", freq);
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    if ((status = sFmIf->tune((int)freq)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM tune, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean muteNative(JNIEnv *env, jobject obj, jboolean toggle)
{
    ALOGI("[JNI] - muteNative :    toggle = %i", toggle);
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    if ((status = sFmIf->mute((BOOLEAN)toggle)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM Mute, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;
}

static jboolean searchNative(JNIEnv *env, jobject obj, jint scanMode, jint rssiThreshold, jint condVal, jint condType)
{
    ALOGI("[JNI] - searchNative :    scanMode = %i  rssiThreshold = %i  condVal = %i  condType = %i",
        scanMode, rssiThreshold, condVal, condType);
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    if (scanMode == BTA_FM_SCAN_FULL || scanMode == BTA_FM_FAST_SCAN)
        processing_scan = 1;

    if ((status = sFmIf->search((int)scanMode, (int)rssiThreshold, (int)condVal,
        (int)condType)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM Tune, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean comboSearchNative(JNIEnv *env, jobject obj, jint startFreq,
    jint endFreq, jint rssiThreshold, jint direction, jint scanMethod,
    jboolean multiChannel, jint rdsType, jint rdsTypeValue)
{
    ALOGI("[JNI] - comboSearchNative");
    jboolean ret = JNI_TRUE;
    int status;
    if (!sFmIf) return JNI_FALSE;

    if ((status = sFmIf->combo_search((int)startFreq, (int)endFreq,
        (int)rssiThreshold, (int)direction, (int)scanMethod,
        (int)multiChannel, (int)rdsType, (int)rdsTypeValue)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM Tune, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean searchAbortNative(JNIEnv *env, jobject obj)
{
    ALOGI("[JNI] - searchAbortNative :");
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    if (processing_scan)
       processing_scan = 0;

    if ((status = sFmIf->search_abort()) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM Search abort, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;
}

static jboolean setRdsModeNative(JNIEnv *env, jobject obj, jboolean rdsOn,
    jboolean afOn, jint rdsType)
{
    ALOGI("[JNI] - setRdsModeNative :");
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    if ((status = sFmIf->set_rds_mode((BOOLEAN)rdsOn, (BOOLEAN)afOn)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM set_rds_mode, status: %d", status);
        ret = JNI_FALSE;
    } else {
        rds_type_save = (int) rdsType;
    }
    return ret;
}

static jboolean setRdsRdbsNative(jint rdsType)
{
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - setRdsRdbsNative :");
    if ((status = sFmIf->set_rds_type((int)rdsType)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM setRdsRdbsNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean setAudioModeNative(JNIEnv *env, jobject obj, jint audioMode)
{
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - setAudioModeNative :");
    if ((status = sFmIf->set_audio_mode((int)audioMode)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM setAudioModeNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}


static jboolean setAudioPathNative(JNIEnv *env, jobject obj, jint audioPath)
{
    jboolean ret = JNI_TRUE;
    int status;
    int desiredBtaAudioPath = -1;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - setAudioPathNative :    audioPath = %i", audioPath);
    sFmAppAudioPath = audioPath;
    switch (audioPath)
    {
        case FM_AUDIO_PATH_NONE:
            desiredBtaAudioPath = FM_ROUTE_NONE;
            break;
        case FM_AUDIO_PATH_SPEAKER:
        case FM_AUDIO_PATH_WIRED_HEADSET:
            desiredBtaAudioPath = FM_AUDIO_PATH;
            break;
        default:
            ALOGE( "Unsupported Audio path: %d!",
                                 audioPath );
            break;
    }

    if ( -1 != desiredBtaAudioPath) {
        if ((status = sFmIf->set_audio_path((int)desiredBtaAudioPath)) != BT_STATUS_SUCCESS) {
            ALOGE("Failed FM setAudioPathNative, status: %d", status);
            ret = JNI_FALSE;
        }
        sCurrentBtaPath = desiredBtaAudioPath;
    }

    return ret;

}


static jboolean setScanStepNative(JNIEnv *env, jobject obj, jint stepSize)
{
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - setScanStepNative :    stepSize = %i", stepSize);
    if ((status = sFmIf->set_scan_step((int)stepSize)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM setScanStepNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean setRegionNative(JNIEnv *env, jobject obj, jint region)
{
    jboolean ret = JNI_TRUE;
    int status;
    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - setRegionNative :    stepSize = %i", region);
    if ((status = sFmIf->set_region((int)region)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM setRegionNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean configureDeemphasisNative(JNIEnv *env, jobject obj, jint time)
{
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - configureDeemphasisNative :    time = %i", time);
    if ((status = sFmIf->config_deemphasis((int)time)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM configureDeemphasisNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean estimateNoiseFloorNative(JNIEnv *env, jobject obj, jint level)
{
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - estimateNoiseFloorNative :    level = %i", level);
    if ((status = sFmIf->estimate_noise_floor((int)time)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM estimateNoiseFloorNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean getAudioQualityNative(JNIEnv *env, jobject obj, jboolean enable)
{
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - estimateNoiseFloorNative :    enable = %i", enable);
    if ((status = sFmIf->read_audio_quality((BOOLEAN)enable)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM estimateNoiseFloorNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static jboolean configureSignalNotificationNative(JNIEnv *env, jobject obj,
    jint pollInterval)
{
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - configureSignalNotificationNative :  pollInterval = %i", pollInterval);
    if ((status = sFmIf->config_signal_notification((int)pollInterval)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM configureSignalNotificationNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}


static jboolean setFMVolumeNative(JNIEnv *env, jobject obj, jint volume)
{
    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - setFMVolumeNative :  volume = %i,bta_path = %d ",
        volume, sCurrentBtaPath);

        if ((status = sFmIf->set_volume((int)volume)) != BT_STATUS_SUCCESS) {
            ALOGE("Failed FM setFMVolumeNative, status: %d", status);
            ret = JNI_FALSE;
        }

    return ret;

}

static jboolean setSnrThresholdNative(JNIEnv *env, jobject obj, jint snr_thres)
{

    jboolean ret = JNI_TRUE;
    int status;

    if (!sFmIf) return JNI_FALSE;

    ALOGI("[JNI] - setSnrThresholdNative :  snr_thres = %i", snr_thres);
    if ((status = sFmIf->set_search_criteria((int)snr_thres)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed FM setSnrThresholdNative, status: %d", status);
        ret = JNI_FALSE;
    }
    return ret;

}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void *) classInitNative},
    {"initializeNative", "()V", (void *) initializeNative},
    {"cleanupNative", "()V", (void *) cleanupNative},
    {"enableFmNative", "(I)Z", (void *)enableFmNative},
    {"disableFmNative", "(Z)Z", (void *)disableFmNative},
    {"tuneNative", "(I)Z", (void *)tuneNative},
    {"muteNative", "(Z)Z", (void *)muteNative},
    {"searchNative", "(IIII)Z", (void *)searchNative},
    {"comboSearchNative", "(IIIIIZII)Z", (void *)comboSearchNative},
    {"searchAbortNative", "()Z", (void *)searchAbortNative},
    {"setRdsModeNative", "(ZZI)Z", (void *) setRdsModeNative},
    {"setAudioModeNative", "(I)Z", (void *)setAudioModeNative},
    {"setAudioPathNative", "(I)Z", (void *)setAudioPathNative},
    {"setScanStepNative", "(I)Z", (void *) setScanStepNative},
    {"setRegionNative", "(I)Z", (void *) setRegionNative},
    {"configureDeemphasisNative", "(I)Z", (void *) configureDeemphasisNative},
    {"estimateNoiseFloorNative", "(I)Z", (void *) estimateNoiseFloorNative},
    {"getAudioQualityNative", "(Z)Z", (void *) getAudioQualityNative},
    {"configureSignalNotificationNative", "(I)Z",
                (void *) configureSignalNotificationNative},
    {"setFMVolumeNative", "(I)Z", (void *) setFMVolumeNative},
    {"setSnrThresholdNative", "(I)Z", (void *) setSnrThresholdNative},
};

int register_com_broadcom_fm_service(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/broadcom/fm/fmreceiver/FmNativehandler",
                                    sMethods, NELEM(sMethods));
}


}
