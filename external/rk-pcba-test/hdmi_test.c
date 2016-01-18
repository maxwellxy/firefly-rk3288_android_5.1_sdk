/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "hdmitest"

#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>  // for utimes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utime.h>
#include <fcntl.h>

#include <cutils/log.h>
#include "common.h"
#include "screen_test.h"
#include "alsa_audio.h"
#include "extra-functions.h"
#include "script.h"

#ifdef DEBUG
#define LOG(x...) printf(x)
#else
#define LOG(x...)
#endif

#define AUDIO_HW_OUT_PERIOD_MULT 8 // (8 * 128 = 1024 frames)
#define AUDIO_HW_OUT_PERIOD_CNT 4

#define HDMI_SWITCH_FILE		"/sys/class/switch/hdmi/state"

#define HDMI_INSERT			1
#define HDMI_REMOVE			0

// ---------------------------------------------------------------------------
static int hdmi_poll_status(void)
{
	FILE *fp;
	char buf[5];
	unsigned char state = 0;
	
	memset(buf, 0, 5);
	fp = fopen(HDMI_SWITCH_FILE, "r");
	if(fp == NULL) {
		LOG("HDMI is not exist\n");
		return -1;
	}
	fgets(buf, 5, fp);
	fclose(fp);
	
	state = atoi(buf);
	return (state);
}

static void hdmi_audio_test(char *filename)
{
	struct pcm* pcmOut = NULL;
	FILE *fp = NULL;
	unsigned int bufsize, num_read;
	char *buffer;

	unsigned inFlags = PCM_IN;
	unsigned outFlags = PCM_OUT;
	unsigned flags =  PCM_STEREO;
	
	LOG("[%s] file %s\n", __FUNCTION__, filename);
	
	if(filename == NULL) {
		LOG("[%s] audio filename is NULL\n", __FUNCTION__);
		return;
	}
	
	flags |= (AUDIO_HW_OUT_PERIOD_MULT - 1) << PCM_PERIOD_SZ_SHIFT;
	flags |= (AUDIO_HW_OUT_PERIOD_CNT - PCM_PERIOD_CNT_MIN)<< PCM_PERIOD_CNT_SHIFT;

	flags |= PCM_CARD1;

	inFlags |= flags;
	outFlags |= flags;
	
	fp = fopen(filename, "rb");
	if(NULL == fp) {
		LOG("[%s] open file %s error\n", __FUNCTION__, filename);
		return;
	}
	fseek(fp, 0, SEEK_SET);

	pcmOut = pcm_open(outFlags);
	if (!pcm_ready(pcmOut)) {
		pcm_close(pcmOut);
		pcmOut = NULL;
                fclose(fp);
		return;
	}

	bufsize = pcm_buffer_size(pcmOut);
	buffer = malloc(bufsize);
    if (!buffer) {
        LOG("Unable to allocate %d bytes\n", bufsize);
        free(buffer);
        pcm_close(pcmOut);
        fclose(fp);
        return;
    }

	do {
        num_read = fread(buffer, 1, bufsize, fp);
        if (num_read > 0) {
            if (pcm_write(pcmOut, buffer, num_read)) {
                LOG("Error playing sample\n");
                break;
            }
        }
    } while (num_read > 0);

    pcm_close(pcmOut);
    free(buffer);
    fclose(fp);
    LOG("[%s] done\n", __FUNCTION__);
}

void* hdmi_test(void* argv)
{	
	int hpdstate, hpdstate_last;
	char sound[256];
	int ret;
	
	hpdstate = HDMI_REMOVE;
	hpdstate_last = hpdstate;

	memset(sound, 0, 256);
	ret = script_fetch("hdmi", "sound_file", (int *)sound, 256/4);
	
	LOG("[%s] sound file is %s ret is %d\n", __FUNCTION__, sound, ret);
	while(1) {
		sleep(1);
		hpdstate = hdmi_poll_status();
		LOG("[%s] hdmi state is %d last state %d\n", __FUNCTION__, hpdstate, hpdstate_last);
		sleep(3);
		if(hpdstate != hpdstate_last) {
			hpdstate_last = hpdstate;
			if(hpdstate == HDMI_INSERT) {
				// Test FB
				LOG("[%s] start test screen\n", __FUNCTION__);
				screen_test(NULL);
				// Test audio
				LOG("[%s] start test audio\n", __FUNCTION__);
				hdmi_audio_test(sound);
			}
		}
	}
    return 0;
}
