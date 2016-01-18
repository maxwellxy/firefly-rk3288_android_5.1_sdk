#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <linux/input.h>

#include "codec_test.h"
#include "../test_case.h"
#include "alsa_audio.h"
#include "../Language/language.h"
#include "../common.h"
#include "../extra-functions.h"

#define LOG(x...) printf("[Codec_TEST] "x)

pthread_t rec_vol_up_tid;
pthread_t rec_vol_down_tid;
pthread_t rec_power_tid;
static int y = 0;
static int isTesting = 0;

void IntMic_LS_test()
{
	if(isTesting)
	{
		return;
	}
	isTesting = 1;
	//ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_CODEC_VOL_UP);
	int ret = system("/system/bin/audio_pcb 1 /data/test.wav -D 0 -d 0 -c 2 -r 48000 -b 16 -p 240 -n 8 -t 0 -u 3");
	if(ret < 0) {
		LOG("IntMic_LS_test:: cmd: ./audio_pcb error: %s", strerror(errno));
	}

	ret = system("/system/bin/alc /data/test.wav");
	if(ret < 0) {
		LOG("IntMic_LS_test:: cmd: ./alc /data/test.wav error: %s", strerror(errno));
	}
	
	ret = system("/system/bin/audio_pcb 2 /data/test.wav -D 0 -d 0 -p 240 -n 16 -t 1 -u 3");
	if(ret < 0) {
		LOG("IntMic_LS_test:: play ./audio_pcb error: %s", strerror(errno));
	}
	//ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_CODEC_INFO);
	isTesting = 0;
}

void ExtMic_HS_test()
{
	if(isTesting)
	{
		return;
	}
	isTesting = 1;
	//ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_CODEC_VOL_DOWN);
	int ret = system("/system/bin/audio_pcb 1 /data/test.wav -D 0 -d 0 -c 2 -r 48000 -b 16 -p 240 -n 8 -t 1 -u 6");
	if(ret < 0) {
		LOG("ExtMic_HS_test:: record error: %s", strerror(errno));
	}

	ret = system("/system/bin/alc /data/test.wav");
	if(ret < 0) {
		LOG("IntMic_LS_test:: cmd: ./alc /data/test.wav error: %s", strerror(errno));
	}
	
	ret = system("/system/bin/audio_pcb 2 /data/test.wav -D 0 -d 0 -p 240 -n 16 -t 2 -u 6");
	if(ret < 0) {
		LOG("ExtMic_HS_test:: playing error: %s", strerror(errno));
	}
	//ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_CODEC_INFO);
	isTesting = 0;
}

void EP_test()
{
	if(isTesting)
	{
		return;
	}
	isTesting = 1;
	//ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_CODEC_VOL_POWER);
	system("tinymix \"Earpiece Gain\" 8");
	int ret = system("/system/bin/audio_pcb 2 /system/etc/test.wav -D 0 -d 0 -p 240 -n 16 -t 0 -u 10");
	if(ret < 0) {
		LOG("EP_test:: playing earphone error: %s", strerror(errno));
	}
	//ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_CODEC_INFO);
	isTesting = 0;
}

void* codec_test(void *argc)
{
    int ret = -1;
    char dt[32] = {0};
	static int flag = 0;

	struct testcase_info *tc_info = (struct testcase_info*)argc;
			
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	y = tc_info->y;
	tc_info->result = -1;

#ifdef SOFIA3GR_AUD_WITHOUT_EARPIEC
	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_WITHOUT_EP_CODEC_INFO);
#else
	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_CODEC_INFO);
#endif

	int key_code, key_count = 0, key_power = 0, key_vol_plus = 0, key_vol_cut = 0, codec_err=0;
	while(key_code = ui_wait_key()) {
		if(isTesting)
		{
			continue;
		}
		switch(key_code) {
			#ifndef SOFIA3GR_AUD_WITHOUT_EARPIEC
			case KEY_POWER:
				LOG("codec_test::Power key is press! \n");
				key_power++;
				codec_err = pthread_create(&rec_power_tid, NULL, EP_test,NULL); //
				if(codec_err != 0)
				{  
				   LOG("codec_test::create rec_power_tid thread error: %s/n",strerror(codec_err));  
				} 
				break;
			#endif
			case KEY_VOLUMEUP:
				LOG("codec_test::Volueme up key is press! \n");
				key_vol_plus++;
				codec_err = pthread_create(&rec_vol_up_tid, NULL, IntMic_LS_test,NULL); //
				if(codec_err != 0)
				{  
				   LOG("codec_test::create rec_vol_up_tid thread error: %s/n",strerror(codec_err));  
				} 
				break;
			case KEY_VOLUMEDOWN:
				LOG("codec_test::Voluem down key is press! \n");
				key_vol_cut++;
				codec_err = pthread_create(&rec_vol_down_tid, NULL, ExtMic_HS_test,NULL); //
				if(codec_err != 0)
				{  
				   LOG("codec_test::create rec_vol_down_tid thread error: %s/n",strerror(codec_err));  
				} 
				break;
			default:
				key_count = 0;
				break;
		}
		#ifdef SOFIA3GR_AUD_WITHOUT_EARPIEC
			if(key_vol_plus >= 1 && key_vol_cut >= 1 && !flag) {
				ptest_set_key_wait_status(1);
				flag = 1;
				sleep(5);
				ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_WITHOUT_EP_CODEC_INFO);
				tc_info->result = 0;
			}
		#else
			if(key_power >= 1 && key_vol_plus >= 1 && key_vol_cut >= 1 && !flag) {
				ptest_set_key_wait_status(1);
				flag = 1;
				sleep(5);
				ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] \n",PCBA_CODEC, PCBA_CODEC_INFO);
				tc_info->result = 0;
			}
		#endif
	}
	
    return NULL;
}
