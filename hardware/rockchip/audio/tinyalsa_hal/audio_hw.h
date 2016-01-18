/*
 * Copyright (C) 2012 The Android Open Source Project
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

/**
 * @file audio_hw.h
 * @brief 
 *                 ALSA Audio Git Log
 * - V0.1.0:add alsa audio hal,just support 312x now.
 * - V0.2.0:remove unused variable.
 * - V0.3.0:turn off device when do_standby.
 * - V0.4.0:turn off device before open pcm.
 * - V0.4.1:Need to re-open the control to fix no sound when suspend.
 * - V0.5.0:Merge the mixer operation from legacy_alsa.
 * - V0.6.0:Merge speex denoise from legacy_alsa.
 * - V0.7.0:add copyright.
 * - V0.7.1:add support for box audio
 * - V0.7.2:add support for dircet output
 * - V0.8.0:update the direct output for box, add the DVI mode
 * - V1.0.0:stable version
 *
 * @author  RkAudio
 * @version 1.0.8
 * @date 2015-08-24
 */

#ifndef AUIDO_HW_H
#define AUIDO_HW_H
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
     
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>
     
#include <hardware/audio.h>
#include <hardware/hardware.h>
     
#include <linux/videodev2.h>
     
#include <system/audio.h>
     
#include <tinyalsa/asoundlib.h>
     
#include <audio_utils/resampler.h>
#include <audio_route/audio_route.h>


#include <speex/speex.h>
#include <speex/speex_preprocess.h>


#include <poll.h>
#include <linux/fb.h>
#include <hardware_legacy/uevent.h>

#define AUDIO_HAL_VERSION "ALSA Audio Version: V1.0.9"


#ifdef BOX_HAL
int PCM_CARD = 0;
int PCM_CARD_HDMI = 0;
int PCM_CARD_SPDIF = 1;
/*#define PCM_CARD 0
#define PCM_CARD_HDMI 0
#define PCM_CARD_SPDIF 1
#define PCM_CARD_USB 3*/
#define PCM_TOTAL 4
#else
#define PCM_CARD 0
#define PCM_CARD_HDMI 1
#define PCM_CARD_SPDIF 2
#define PCM_CARD_USB 3
#define PCM_TOTAL 4 
#endif
#define PCM_DEVICE 0
#define PCM_DEVICE_SCO 1
#define PCM_DEVICE_VOICE 2
#define PCM_DEVICE_DEEP 3 

#define MIXER_CARD 0

/* duration in ms of volume ramp applied when starting capture to remove plop */
#define CAPTURE_START_RAMP_MS 100

/* default sampling for HDMI multichannel output */
#define HDMI_MULTI_DEFAULT_SAMPLING_RATE  44100
/* maximum number of channel mask configurations supported. Currently the primary
 * output only supports 1 (stereo) and the multi channel HDMI output 2 (5.1 and 7.1) */
#define MAX_SUPPORTED_CHANNEL_MASKS 2


#define SPEEX_DENOISE_ENABLE


#define HW_PARAMS_FLAG_LPCM 0
#define HW_PARAMS_FLAG_NLPCM 1

#define HDMI_AUIOINFO_NODE      "/sys/class/display/HDMI/audioinfo"
#define HDMI_CONNECTION_NODE    "/sys/class/display/HDMI/connect"
#define SND_CARD0_NODE          "/proc/asound/card0/id"
#define SND_CARD1_NODE          "/proc/asound/card1/id"
#define SND_CARD2_NODE          "/proc/asound/card2/id"
#define MEDIA_SINK_AUDIO        "media.sink.audio"
#define MEDIA_CFG_AUDIO_BYPASS  "media.cfg.audio.bypass"
#define MEDIA_CFG_AUDIO_MUL     "media.cfg.audio.mul"

/* b, p, c, u, v, 0, 0, 0*/
#define B_BIT_SHIFT    7
#define P_BIT_SHIFT    6
#define C_BIT_SHIFT    5
#define U_BIT_SHIFT    4
#define V_BIT_SHIFT    3
#define C_BIT_SET      (0x1<<C_BIT_SHIFT)
#define C_BIT_UNSET    (~(0x1<<C_BIT_SHIFT))

#define CHASTA_NUM     192
#define CHASTA_SUB_NUM (CHASTA_NUM*2)
#define CHASTA_BIT0    0
#define CHASTA_BIT1    1
/* bit 24~27  sampling frequency */
#define CHASTA_BIT24   24
#define CHASTA_BIT25   25
#define CHASTA_BIT26   26
#define CHASTA_BIT27   27
/* bit 32~35 sample word lentgh */
#define CHASTA_BIT32   32
#define CHASTA_BIT33   33
#define CHASTA_BIT34   34
#define CHASTA_BIT35   35
/* bit 36~39 original samplling frequency */
#define CHASTA_BIT36   36
#define CHASTA_BIT37   37
#define CHASTA_BIT38   38
#define CHASTA_BIT39   39

#ifdef BOX_HAL
struct pcm_config pcm_config = {
    .channels = 2,
    .rate = 44100,
    .period_size = 2048,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_in = {
    .channels = 2,
    .rate = 44100,
    .period_size = 1024,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

#else
struct pcm_config pcm_config = {
    .channels = 2,
    .rate = 44100,
    .period_size = 1024,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_in = {
    .channels = 2,
    .rate = 44100,
    .period_size = 1024, 
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};
#endif

struct pcm_config pcm_config_in_low_latency = {
    .channels = 2,
    .rate = 44100,
    .period_size = 256,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_sco = {
    .channels = 1,
    .rate = 8000,
    .period_size = 128,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_deep = {
    .channels = 2,
    .rate = 44100,
    /* FIXME This is an arbitrary number, may change.
     * Dynamic configuration based on screen on/off is not implemented;
     * let's see what power consumption is first to see if necessary.
     */
    .period_size = 8192,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_hdmi_multi = {
    .channels = 6, /* changed when the stream is opened */
    .rate = HDMI_MULTI_DEFAULT_SAMPLING_RATE,
    .period_size = 1024,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_direct = {
    .channels = 2,
    .rate = 48000,
    .period_size = 1024*8,
    .period_count = 3,
    .format = PCM_FORMAT_S16_LE,
};

enum output_type {
    OUTPUT_DEEP_BUF,      // deep PCM buffers output stream
    OUTPUT_LOW_LATENCY,   // low latency output stream
    OUTPUT_HDMI_MULTI,          // HDMI multi channel
    OUTPUT_DIRECT,
    OUTPUT_TOTAL
};

struct direct_mode_t {
    int output_mode;
    char* hbr_Buf;
};

struct audio_device {
    struct audio_hw_device hw_device;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    audio_devices_t out_device; /* "or" of stream_out.device for all active output streams */
    audio_devices_t in_device;
    bool mic_mute;
    struct audio_route *ar;
    audio_source_t input_source;
    int cur_route_id;     /* current route ID: combination of input source
                           * and output device IDs */
    struct pcm *pcm_voice_out;
    struct pcm *pcm_sco_out;
    struct pcm *pcm_voice_in;
    struct pcm *pcm_sco_in;
    int hdmi_drv_fd;    /* either an fd >= 0 or -1 */
    audio_channel_mask_t in_channel_mask;
    unsigned int sco_on_count;

    struct stream_out *outputs[OUTPUT_TOTAL];
    pthread_mutex_t lock_outputs; /* see note below on mutex acquisition order */
    int pre_output_device_id;
    int pre_input_source_id;

};

struct stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm *pcm[PCM_TOTAL];
    struct pcm_config config;
    unsigned int pcm_device;
    bool standby; /* true if all PCMs are inactive */
    audio_devices_t device;
    /* FIXME: when HDMI multichannel output is active, other outputs must be disabled as
     * HDMI and WM1811 share the same I2S. This means that notifications and other sounds are
     * silent when watching a 5.1 movie. */
    bool disabled;
    audio_channel_mask_t channel_mask;
    /* Array of supported channel mask configurations. +1 so that the last entry is always 0 */
    audio_channel_mask_t supported_channel_masks[MAX_SUPPORTED_CHANNEL_MASKS + 1];
    bool muted;
    uint64_t written; /* total frames written, not cleared when entering standby */
    uint64_t nframes;
    bool output_direct;

    struct audio_device *dev;
};

struct stream_in {
    struct audio_stream_in stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm *pcm;
    bool standby;

    unsigned int requested_rate;
    struct resampler_itfe *resampler;
    struct resampler_buffer_provider buf_provider;
    int16_t *buffer;
    size_t frames_in;
    int read_status;
    audio_source_t input_source;
    audio_io_handle_t io_handle;
    audio_devices_t device;
    uint16_t ramp_vol;
    uint16_t ramp_step;
    size_t  ramp_frames;
    audio_channel_mask_t channel_mask;
    audio_input_flags_t flags;
    struct pcm_config *config;

    struct audio_device *dev;
#ifdef SPEEX_DENOISE_ENABLE
    SpeexPreprocessState* mSpeexState;
    int mSpeexFrameSize;
    int16_t *mSpeexPcmIn;
#endif
};

#define STRING_TO_ENUM(string) { #string, string }

struct string_to_enum {
    const char *name;
    uint32_t value;
};

const struct string_to_enum out_channels_name_to_enum_table[] = {
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_STEREO),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_5POINT1),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_7POINT1),
};

enum {
    OUT_DEVICE_SPEAKER,
    OUT_DEVICE_HEADSET,
    OUT_DEVICE_HEADPHONES,
    OUT_DEVICE_BT_SCO,
    OUT_DEVICE_SPEAKER_AND_HEADSET,
    OUT_DEVICE_OFF,
    OUT_DEVICE_TAB_SIZE,           /* number of rows in route_configs[][] */
    OUT_DEVICE_NONE,
    OUT_DEVICE_CNT
};

enum {
    IN_SOURCE_MIC,
    IN_SOURCE_CAMCORDER,
    IN_SOURCE_VOICE_RECOGNITION,
    IN_SOURCE_VOICE_COMMUNICATION,
    IN_SOURCE_OFF,
    IN_SOURCE_TAB_SIZE,            /* number of lines in route_configs[][] */
    IN_SOURCE_NONE,
    IN_SOURCE_CNT
};

struct route_config {
    const char * const output_route;
    const char * const input_route;
    const char * const output_off;
    const char * const input_off;
};

const struct route_config media_speaker = {
    "media-speaker",
    "media-main-mic",
    "playback-off",
    "capture-off",
};

const struct route_config media_headphones = {
    "media-headphones",
    "media-main-mic",
    "playback-off",
    "capture-off",
};

const struct route_config media_headset = {
    "media-headphones",
    "media-headset-mic",
    "playback-off",
    "capture-off",
};

const struct route_config camcorder_speaker = {
    "media-speaker",
    "media-second-mic",
    "playback-off",
    "capture-off",
};

const struct route_config camcorder_headphones = {
    "media-headphones",
    "media-second-mic",
    "playback-off",
    "capture-off",
};

const struct route_config voice_rec_speaker = {
    "voice-rec-speaker",
    "voice-rec-main-mic",
    "incall-off",
    "incall-off",
};

const struct route_config voice_rec_headphones = {
    "voice-rec-headphones",
    "voice-rec-main-mic",
    "incall-off",
    "incall-off",
};

const struct route_config voice_rec_headset = {
    "voice-rec-headphones",
    "voice-rec-headset-mic",
    "incall-off",
    "incall-off",
};

const struct route_config communication_speaker = {
    "communication-speaker",
    "communication-main-mic",
    "voip-off",
    "voip-off",
};

const struct route_config communication_headphones = {
    "communication-headphones",
    "communication-main-mic",
    "voip-off",
    "voip-off",
};

const struct route_config communication_headset = {
    "communication-headphones",
    "communication-headset-mic",
    "voip-off",
    "voip-off",
};

const struct route_config speaker_and_headphones = {
    "speaker-and-headphones",
    "main-mic",
    "playback-off",
    "capture-off",
};

const struct route_config bluetooth_sco = {
    "bt-sco-headset",
    "bt-sco-mic",
    "playback-off",
    "capture-off",
};

const struct route_config * const route_configs[IN_SOURCE_TAB_SIZE]
                                               [OUT_DEVICE_TAB_SIZE] = {
    {   /* IN_SOURCE_MIC */
        &media_speaker,             /* OUT_DEVICE_SPEAKER */
        &media_headset,             /* OUT_DEVICE_HEADSET */
        &media_headphones,          /* OUT_DEVICE_HEADPHONES */
        &bluetooth_sco,             /* OUT_DEVICE_BT_SCO */
        &speaker_and_headphones     /* OUT_DEVICE_SPEAKER_AND_HEADSET */
    },
    {   /* IN_SOURCE_CAMCORDER */
        &camcorder_speaker,         /* OUT_DEVICE_SPEAKER */
        &camcorder_headphones,      /* OUT_DEVICE_HEADSET */
        &camcorder_headphones,      /* OUT_DEVICE_HEADPHONES */
        &bluetooth_sco,             /* OUT_DEVICE_BT_SCO */
        &speaker_and_headphones     /* OUT_DEVICE_SPEAKER_AND_HEADSET */
    },
    {   /* IN_SOURCE_VOICE_RECOGNITION */
        &voice_rec_speaker,         /* OUT_DEVICE_SPEAKER */
        &voice_rec_headset,         /* OUT_DEVICE_HEADSET */
        &voice_rec_headphones,      /* OUT_DEVICE_HEADPHONES */
        &bluetooth_sco,             /* OUT_DEVICE_BT_SCO */
        &speaker_and_headphones     /* OUT_DEVICE_SPEAKER_AND_HEADSET */
    },
    {   /* IN_SOURCE_VOICE_COMMUNICATION */
        &communication_speaker,     /* OUT_DEVICE_SPEAKER */
        &communication_headset,     /* OUT_DEVICE_HEADSET */
        &communication_headphones,  /* OUT_DEVICE_HEADPHONES */
        &bluetooth_sco,             /* OUT_DEVICE_BT_SCO */
        &speaker_and_headphones     /* OUT_DEVICE_SPEAKER_AND_HEADSET */
    }
};


struct mixer* pre_mixer;
struct direct_mode_t direct_mode = {HW_PARAMS_FLAG_LPCM, NULL};
unsigned char channel_status[CHASTA_SUB_NUM];
static int scount = 0;
pthread_t hdmi_uevent_t = NULL;
int prop_pcm;//for debug

static void do_out_standby(struct stream_out *out);

/**
 * @brief initchnsta 
 */
void initchnsta(void)
{   
    memset(channel_status, 0x0, CHASTA_SUB_NUM);
    channel_status[CHASTA_BIT1*2] = 1;
    channel_status[CHASTA_BIT1*2+1] = 1;
    /* word length default 24 bit*/
    channel_status[CHASTA_BIT32*2] = 1;
    channel_status[CHASTA_BIT32*2+1] = 1;
    channel_status[CHASTA_BIT33*2] = 1;
    channel_status[CHASTA_BIT33*2+1] = 1;
    channel_status[CHASTA_BIT35*2] = 1;
    channel_status[CHASTA_BIT35*2+1] = 1;
    /* sampling frequency default 48k */
    channel_status[CHASTA_BIT25*2] = 1;
    channel_status[CHASTA_BIT25*2+1] = 1;
    /* original sampling frequency default 48k */
    channel_status[CHASTA_BIT36*2] = 1;
    channel_status[CHASTA_BIT36*2+1] = 1;
    channel_status[CHASTA_BIT38*2] = 1;
    channel_status[CHASTA_BIT38*2+1] = 1;
    channel_status[CHASTA_BIT39*2] = 1;
    channel_status[CHASTA_BIT39*2+1] = 1;

    /* b, p, c, u, v, 0, 0, 0*/
    int i = 0;
    for (i=0; i<CHASTA_SUB_NUM; i++)
       channel_status[i] = (channel_status[i]<<C_BIT_SHIFT) | (0x1<<V_BIT_SHIFT);

    /* B bit */
    channel_status[CHASTA_BIT0*2] |= (0X1<<B_BIT_SHIFT);
    channel_status[CHASTA_BIT0*2+1] |= (0X1<<B_BIT_SHIFT);
}

/**
 * @brief setnlpcmchnsta 
 */
void setnlpcmchnsta(void)
{
       /* sampling frequency default 48k */
       channel_status[CHASTA_BIT24*2] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT24*2+1] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT25*2] |= C_BIT_SET;
       channel_status[CHASTA_BIT25*2+1] |= C_BIT_SET;
       channel_status[CHASTA_BIT26*2] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT26*2+1] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT27*2] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT27*2+1] &= C_BIT_UNSET;
       /* original sampling frequency */
       channel_status[CHASTA_BIT36*2] |= C_BIT_SET;
       channel_status[CHASTA_BIT36*2+1] |= C_BIT_SET;
       channel_status[CHASTA_BIT37*2] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT37*2+1] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT38*2] |= C_BIT_SET;
       channel_status[CHASTA_BIT38*2+1] |= C_BIT_SET;
       channel_status[CHASTA_BIT39*2] |= C_BIT_SET;
       channel_status[CHASTA_BIT39*2+1] |= C_BIT_SET;
}

/**
 * @brief sethbrchnsta 
 */
void sethbrchnsta(void)
{
       /* sampling frequency 768k */
       channel_status[CHASTA_BIT24*2] |= C_BIT_SET;
       channel_status[CHASTA_BIT24*2+1] |= C_BIT_SET;
       channel_status[CHASTA_BIT25*2] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT25*2+1] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT26*2] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT26*2+1] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT27*2] |= C_BIT_SET;
       channel_status[CHASTA_BIT27*2+1] |= C_BIT_SET;
       /* original sampling frequency */
       channel_status[CHASTA_BIT36*2] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT36*2+1] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT37*2] |= C_BIT_SET;
       channel_status[CHASTA_BIT37*2+1] |= C_BIT_SET;
       channel_status[CHASTA_BIT38*2] |= C_BIT_SET;
       channel_status[CHASTA_BIT38*2+1] |= C_BIT_SET;
       channel_status[CHASTA_BIT39*2] &= C_BIT_UNSET;
       channel_status[CHASTA_BIT39*2+1] &= C_BIT_UNSET;
}

/**
 * @brief dumpchnsta 
 */
void dumpchnsta()
{
    int i= 0;
    for (i=0; i<CHASTA_SUB_NUM; i+=16) {
        ALOGI("%02d: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "
        "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
        i/16, channel_status[i], channel_status[i+1], channel_status[i+2], channel_status[i+3],
        channel_status[i+4], channel_status[i+5], channel_status[i+6], channel_status[i+7],
        channel_status[i+8], channel_status[i+9], channel_status[i+10], channel_status[i+11],
        channel_status[i+12], channel_status[i+13], channel_status[i+14], channel_status[i+15]);
    }
}


#endif

