#include<stdio.h>
#include <stdlib.h>

#include"common.h"
#include"extra-functions.h"

#include"udisk_test.h"
#include"test_case.h"
#include "language.h"

#define SCAN_RESULT_LENGTH 128
#define SCAN_RESULT_FILE "/data/udisk_capacity.txt"

#define LOG(x...)   printf("[UDISK_TEST] "x)


void * udisk_test(void * argv)
{
	
	struct testcase_info *tc_info = (struct testcase_info*)argv;
	int ret,y;
	double cap;
	FILE *fp;
	char results[SCAN_RESULT_LENGTH];
	
	/*remind ddr test*/
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	y = tc_info->y;
#ifdef SOFIA3GR_PCBA
	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s..] \n",PCBA_UCARD,PCBA_TESTING);
#else
	ui_print_xy_rgba(0,y,255,255,0,255,"%s \n",PCBA_UCARD);
#endif


	
	#ifdef SOFIA3GR_PCBA
		ret =  __system("busybox chmod 777 /system/bin/udisktester.sh");
	#else
		ret =  __system("busybox chmod 777 /res/udisktester.sh");

	if(ret)
		printf("chmod udisktester.sh failed :%d\n",ret);
	#endif

	#ifdef SOFIA3GR_PCBA
		//ret = __system("/system/bin/udisktester.sh");
			int testCounts =0;
			while(1)
			{
				fp = NULL;
				LOG("%s::wait for insert udisk card...\n", __FUNCTION__);
				__system("/system/bin/udisktester.sh");

				fp = fopen(SCAN_RESULT_FILE, "r");
				if(fp != NULL)
				{
					LOG("%s line=%d find result file! \n", __FUNCTION__, __LINE__);
					break;
				}

				LOG("%s line=%d can't find result file! continue ... \n", __FUNCTION__, __LINE__);
				
				if(testCounts++ > 3)
				{
					LOG("can not open %s.\n", SCAN_RESULT_FILE);
					ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_UCARD,PCBA_FAILED);
					tc_info->result = -1;
					return argv;
				}
				sleep(1);
			}

			//disable by wjh
			/*
			memset(results, 0, SCAN_RESULT_LENGTH);
			//fread(results, 1, SCAN_RESULT_LENGTH, fp);
			fgets(results,50,fp);
			//fgets(wlan_msg->ssid,50,fp); //we assume tha a AP's name is less of 50 charactes
			
			//LOG("%s.\n", results);

			cap = strtod(results,NULL);
			if(cap) {
				ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] { %2fG } \n",PCBA_UCARD,PCBA_SECCESS,cap*1.0);
				tc_info->result = 0;
			}*/

			ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s]\n",PCBA_UCARD,PCBA_SECCESS);
				tc_info->result = 0;
		    fclose(fp);

			return argv;
	#else
		ret = __system("/res/udisktester.sh");
	if(ret < 0) {
		printf("udisk test failed.\n");
		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_UCARD,PCBA_FAILED);
		tc_info->result = -1;
		return argv;
	}
	#endif
	
	
	fp = fopen(SCAN_RESULT_FILE, "r");
	if(fp == NULL) {
		printf("can not open %s.\n", SCAN_RESULT_FILE);
		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_UCARD,PCBA_FAILED);
		tc_info->result = -1;
		return argv;
	}

	
  	memset(results, 0, SCAN_RESULT_LENGTH);
	fgets(results,50,fp);
	
	cap = strtod(results,NULL);
    printf("capacity : %s\n", results);
	if(cap > 0) {
		ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] { %2fG }\n",PCBA_UCARD,PCBA_SECCESS,cap*1.0/1024/1024);
		tc_info->result = 0;
	}
    else {
        ui_print_xy_rgba(0,y,0,0,255,255,"%s:[%s]\n",PCBA_UCARD,PCBA_FAILED);
		tc_info->result = -1;
    }

        fclose(fp);
	
	return argv;
	
}
