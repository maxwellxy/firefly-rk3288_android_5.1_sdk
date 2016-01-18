/******************************************************************************
 *
 *  Copyright (C) 2009-2013 Broadcom Corporation
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

package com.broadcom.fm.fmreceiver;

/**
 * {@hide}
 */
public class FmReceiverServiceState
{
    static final String FM_RECEIVER_PERM = "android.permission.ACCESS_FM_RECEIVER";
    protected static final boolean V = FmReceiverServiceConfig.V;

    /* Main state machine states. */
    static final int RADIO_STATE_OFF = 0;
    static final int RADIO_STATE_STARTING = 1;
    static final int RADIO_STATE_READY_FOR_COMMAND = 2;
    static final int RADIO_STATE_STOPPING = 3;
    static final int RADIO_STATE_BUSY = 4;
    static final int RADIO_STATE_INIT = 5;

    /* The stages in a specific operation before ready for next command. */
    static final int RADIO_OP_STATE_NONE = 0;
    static final int RADIO_OP_STATE_STAGE_1 = 1;
    static final int RADIO_OP_STATE_STAGE_2 = 2;
    static final int RADIO_OP_STATE_STAGE_3 = 3;
    static final int RADIO_OP_STATE_STAGE_4 = 4;
    static final int RADIO_OP_STATE_STAGE_5 = 5;

    /* Handler queue command types. */
    static final int OPERATION_TIMEOUT = 1; // When a timeout event occurs.

    static final int OPERATION_STATUS_EVENT_CALLBACK = 2;
    static final int OPERATION_SEARCH_EVENT_CALLBACK = 3;
    static final int OPERATION_RDS_EVENT_CALLBACK = 4;
    static final int OPERATION_RDS_DATA_EVENT_CALLBACK = 5;
    static final int OPERATION_AUDIO_MODE_EVENT_CALLBACK = 6;
    static final int OPERATION_AUDIO_PATH_EVENT_CALLBACK = 7;
    static final int OPERATION_REGION_EVENT_CALLBACK = 8;
    static final int OPERATION_NFE_EVENT_CALLBACK = 9;
    static final int OPERATION_LIVE_AUDIO_EVENT_CALLBACK = 10;
    static final int OPERATION_VOLUME_EVENT_CALLBACK = 11;

    /* Timeout for various operations to avoid locking state machine. */
    static final int OPERATION_TIMEOUT_STARTUP = 10000; // 10 sec startup
    static final int OPERATION_TIMEOUT_SHUTDOWN = 10000; // 10 sec shutdown
    static final int OPERATION_TIMEOUT_SEARCH = 20000; // 20 sec search whole band
    static final int OPERATION_TIMEOUT_NFL = 20000; // 20 sec busy
    static final int OPERATION_TIMEOUT_GENERIC = 5000; // 5 sec busy

    /* Parameter constraints section. */
    /** Mask over region bits. */
    static final int FUNC_REGION_BITMASK = 0x03;
    /** Mask over RDS or RBDS bits. (also AF) */
    static final int FUNC_RDS_BITMASK = 0x70;
    /** Mask over SOFTMUTE bit */
    static final int FUNC_SOFTMUTE_BITMASK = 0x0100;

    /** Filter for allowable functionality bits. */
    static final int FUNC_BITMASK = FUNC_REGION_BITMASK | FUNC_RDS_BITMASK;

    /** Minimum allowed frequency code. */
    static final int MIN_ALLOWED_FREQUENCY_CODE = 1; // 0.01 MHz
    /** Maximum allowed frequency code. */
    static final int MAX_ALLOWED_FREQUENCY_CODE = 99999; // 999.99 MHz

    /* SCAN_MODE_BITMASK should be 0x83 to support both FULL SCAN and FAST SCAN */
    /** Filter for allowable scan mode bits. */
    static final int SCAN_MODE_BITMASK = 0x00000083;

    /** Minimum allowed signal strength number. */
    static final int MIN_ALLOWED_SIGNAL_STRENGTH_NUMBER = 0;
    /** Maximum allowed signal strength number. */
    static final int MAX_ALLOWED_SIGNAL_STRENGTH_NUMBER = 255;

    /** Minimum allowed RDS condition value. */
    static final int MIN_ALLOWED_RDS_CONDITION_VALUE = 0;
    /** Maximum allowed RDS condition value. */
    static final int MAX_ALLOWED_RDS_CONDITION_VALUE = 255;

    /** Filter for allowable scan mode bits. */
    static final int RDS_FEATURES_BITMASK = 0x0000007c;

    /** RDS event type ID table. */
    static final int RDS_ID_PTY_EVT        = 2;
    static final int RDS_ID_PS_EVT          = 7;
    static final int RDS_ID_PTYN_EVT       = 8;
    static final int RDS_ID_RT_EVT         = 9;

    /** Filter for allowable RDS mode bits. */
    static final int RDS_MODE_BITMASK = 0x00000003;
    static final int RDS_RBDS_BITMASK = 0x00000001;
    /** Filter for allowable AF mode bits. */
    static final int AF_MODE_BITMASK = 0x00000001;
    /** RDS mode native code. */
    static final int RDS_MODE_NATIVE = 0x00000000;
    /** RBDS mode native code. */
    static final int RBDS_MODE_NATIVE = 0x00000001;
    /** Minimum allowed AF jump threshold value. */
    static final int MIN_ALLOWED_AF_JUMP_THRESHOLD = 0;
    /** Maximum allowed AF jump threshold value. */
    static final int MAX_ALLOWED_AF_JUMP_THRESHOLD = 255;

    /** Filter for allowable audio mode bits. */
    static final int AUDIO_MODE_BITMASK = 0x00000003;

    /** Filter for allowable audio path bits. */
    static final int AUDIO_PATH_BITMASK = 0x00000003;

    /** Minimum allowed signal polling time. */
    static final int MIN_ALLOWED_SIGNAL_POLLING_TIME = 10; /* 10 ms. */
    /** Maximum allowed signal polling time. */
    static final int MAX_ALLOWED_SIGNAL_POLLING_TIME = 100000; /* 100 s. */

    /** Minimum allowed SNR Threshold */
    static final int MIN_ALLOWED_SNR_THRESHOLD = 0;
    /** Maximum allowed SNR Threshold */
    static final int MAX_ALLOWED_SNR_THRESHOLD = 31;

    /** Status OK. */
    static final int BTA_FM_OK = 0;
    /** RSSI too low. */
    static final int BTA_FM_SCAN_RSSI_LOW = 1;
    /** Scan failed. */
    static final int BTA_FM_SCAN_FAIL = 2;
    /** Scan was aborted before completion. */
    static final int BTA_FM_SCAN_ABORT = 3;
    /** No resources available. */
    static final int BTA_FM_SCAN_NO_RES = 4;
    /** General error condition. */
    static final int BTA_FM_ERR = 5;
    /** Unsupported functionality error. */
    static final int BTA_FM_UNSPT_ERR = 6;
    /** Flag error. */
    static final int BTA_FM_FLAG_TOUT_ERR = 7;
    /** Frequency error. */
    static final int BTA_FM_FREQ_ERR = 8;
    /** VCMD error. */
    static final int BTA_FM_VCMD_ERR = 9;

    // state variables
    //
    /* Local data cache and associated update freshness. */
    static int mFreq = 0;
    static int mRssi = 127;
    static int mSnr = 127;
    static boolean mRadioIsOn = false;
    static int mRdsProgramType = 0;
    static String mRdsProgramService = "";
    static String mRdsRadioText = "";
    static String mRdsProgramTypeName = "";
    static boolean mIsMute = false;
    static boolean mSeekSuccess = false;
    static boolean mRdsOn = false;
    static boolean mAfOn = false;
    static int mRdsType = 0; // RDS
    static int mAlternateFreqHopThreshold = 0;
    static int mAudioMode = FmProxy.AUDIO_MODE_AUTO;
    static int mAudioPath = FmProxy.AUDIO_PATH_SPEAKER;
    static int mWorldRegion = FmProxy.FUNC_REGION_DEFAULT;
    static int mStepSize = FmProxy.FREQ_STEP_DEFAULT;
    static boolean mLiveAudioQuality = FmProxy.LIVE_AUDIO_QUALITY_DEFAULT;
    static int mEstimatedNoiseFloorLevel = FmProxy.NFL_DEFAULT;
    static int mSignalPollInterval = FmProxy.SIGNAL_POLL_INTERVAL_DEFAULT;
    static int mDeemphasisTime = FmProxy.DEEMPHASIS_TIME_DEFAULT;
    static int radio_state = FmReceiverServiceState.RADIO_STATE_OFF;

    /* The current state of the state machine. */
    static int radio_op_state = FmReceiverServiceState.RADIO_OP_STATE_NONE;
};
