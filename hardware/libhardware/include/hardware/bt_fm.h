/************************************************************************************
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
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/

#ifndef ANDROID_INCLUDE_BT_FM_H
#define ANDROID_INCLUDE_BT_FM_H

__BEGIN_DECLS

/* The input ranges for all the APIs will be as per the BTA FM input ranges and
* these will be defined in FM Proxy in java layer for Fm app usage
*/
typedef unsigned char BOOLEAN;


//These structures are mapped to the corresponding BTA fm stack structures
typedef struct
{
    int          status;
    unsigned char        data;
    unsigned char        index;
    char                 text[65];
} btfm_rds_update_t;


typedef struct
{
    unsigned char         content_type;
    unsigned char         start;
    unsigned char         len;
} btfm_rds_rtp_tag_t;

typedef struct
{
    BOOLEAN                  running;
    unsigned char            tag_toggle;
    unsigned char            tag_num;
    btfm_rds_rtp_tag_t    tag[6];
} btfm_rds_rtp_info_t;


/**
 * Enable fm callback
 */
typedef void (* btfm_enable_callback)(int status);

/**
 * Disable fm callback
 */
typedef void (* btfm_disable_callback)(int status);

/**
 * Fm tune event callback
 */
typedef void (* btfm_tune_callback)(int status, int rssi, int snr, int freq);


/**
 * Fm mute event callback
 */
typedef void (* btfm_mute_callback)(int status, BOOLEAN isMute);

/**
 * Fm search event callback
 */
typedef void (* btfm_search_callback)(int status, int rssi, int snr, int freq);

/**
 * Fm search complete event callback
 */
typedef void (* btfm_search_complete_callback)(int status, int rssi, int snr, int freq);

/**
 * Fm af jump event  callback
 */
typedef void (* btfm_af_jump_callback)(int status, int rssi, int snr, int freq);

/**
 * Fm audio mode  callback
 */
typedef void (* btfm_audio_mode_callback)(int status, int audioMode);

/**
 * Fm audio path callback
 */
typedef void (* btfm_audio_path_callback)(int status, int audioPath);

/**
 * Fm audio data callback
 */
typedef void (* btfm_audio_data_callback)(int status, int rssi, int snr, int audioMode);

/**
 * Fm rds mode callback
 */
typedef void (* btfm_rds_mode_callback)(int status, BOOLEAN rdsOn, BOOLEAN afOn);

/**
 * Fm rds type callback
 */
typedef void (* btfm_rds_type_callback)(int status, int rdsType);

/**
 * Fm deempasis param callback
 */
typedef void (* btfm_deemphasis_callback)(int status, int timeConst);

/**
 * Fm scan step callback
 */
typedef void (* btfm_scan_step_callback)(int status, int scanStep);

/**
 * Fm region callback
 */
typedef void (* btfm_region_callback)(int status, int region);

/**
 * Fm noise floor level callback
 */
typedef void (* btfm_nfl_callback)(int status, int noiseFloor);

/**
 * Fm volume callback
 */
typedef void (* btfm_volume_callback)(int status, int volume);

/**
 * Fm rds data callback
 */
typedef void (* btfm_rds_data_callback)(int status, int dataType, int index,
                    char *radioText);

/**
 * Fm rds data callback
 */
typedef void (* btfm_rtp_data_callback)( btfm_rds_rtp_info_t *rtpInfo);



/** BT-FM callback structure. */
typedef struct {
    /** set to sizeof(BtFmCallbacks) */
    size_t      size;
    btfm_enable_callback            enable_cb;
    btfm_disable_callback           disable_cb;
    btfm_tune_callback              tune_cb;
    btfm_mute_callback              mute_cb;
    btfm_search_callback            search_cb;
    btfm_search_complete_callback   search_complete_cb;
    btfm_af_jump_callback           af_jump_cb;
    btfm_audio_mode_callback        audio_mode_cb;
    btfm_audio_path_callback        audio_path_cb;
    btfm_audio_data_callback        audio_data_cb;
    btfm_rds_mode_callback          rds_mode_cb;
    btfm_rds_type_callback          rds_type_cb;
    btfm_deemphasis_callback        deemphasis_cb;
    btfm_scan_step_callback         scan_step_cb;
    btfm_region_callback            region_cb;
    btfm_nfl_callback               nfl_cb;
    btfm_volume_callback            volume_cb;
    btfm_rds_data_callback          rds_data_cb;
    btfm_rtp_data_callback          rtp_data_cb;
} btfm_callbacks_t;


/** Represents the standard BT-FM interface. */
typedef struct {

/** set to sizeof(bt_interface_t) */
size_t size;
/**
 * Opens the interface and provides the callback routines
 * to the implemenation of this interface.
 */
int (*init)(btfm_callbacks_t* callbacks );

/** Enable Fm. */
int (*enable)(int functionalityMask);

/** Tune Fm. */
int (*tune)(int freq);

/** Mute/unmute Fm. */
int (*mute)(BOOLEAN mute);

/** Search Fm. */
int (*search)(int scanMode, int rssiThresh, int condType, int CondValue);

/** Search Fm.with frequency wrap */
int (*combo_search)(int startFreq, int endFreq, int rssiThresh, int direction, int scanMode,
                        BOOLEAN multiChannel, int condType, int condValue);

/** Abort Search for Fm. */
int (*search_abort)();

/** Enable rds/af for Fm. */
int (*set_rds_mode)(BOOLEAN rdsOn, BOOLEAN afOn);

/** Set Rds type. */
int (*set_rds_type)(int rdsType);

/** Set audio mode */
int (*set_audio_mode)(int audioMode);

/** Set audio path. */
int (*set_audio_path)(int audioPath);

/** Set Fm region */
int (*set_region)(int regionType);

/** Set Fm Scan step */
int (*set_scan_step)(int scanStep);

/** Config de-emphasis param. */
int (*config_deemphasis)(int timeType);

/** Estimate noise floor. */
int (*estimate_noise_floor)(int level);

/** Reads audio quality of current station .Turn audio date live update on/off */
int (*read_audio_quality)(BOOLEAN turnOn);

/** Used to configure RSSI value polling interval */
int (*config_signal_notification)(int time);

/** Set fm volume. */
int (*set_volume)(int volume);

/** Set SNR thresh for search. */
int (*set_search_criteria)(int value);

/** Disable Fm. */
int (*disable)(void);

/** Closes the interface. */
void (*cleanup)(void);

} btfm_interface_t;

__END_DECLS

#endif /* ANDROID_INCLUDE_BT_FM_H */


