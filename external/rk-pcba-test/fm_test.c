#include<stdio.h>
#include <stdlib.h>

#include"extra-functions.h"
#include"common.h"
#include"fm_test.h"
#include "test_case.h"
#include "language.h"

#define LOG(x...)   				printf("[FM_TEST] "x)
#define SCAN_CHANNEL_COUNTS_FILE 	"/data/fm_channel_counts"
#define SCAN_CHANNEL_RESULT_FILE	"/data/fm_info"
#define SCAN_RESULTS_LENGTH			1024
#define SCAN_RESULT_LENGTH			256


void * fm_test(void * argv)
{
	
	struct testcase_info *tc_info = (struct testcase_info*)argv;
	int ret,y,channelCounts=0,testCounts =-1;;
	int cap;
	FILE *fp;
	char result[SCAN_RESULT_LENGTH];
	
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	y = tc_info->y;

	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s..] \n",PCBA_FM,PCBA_TESTING);
	
	while(1)
	{
		testCounts++;
		if(testCounts > 3)
		{
			LOG("can not open %s.\n", SCAN_CHANNEL_COUNTS_FILE);
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_FM,PCBA_FAILED);
			tc_info->result = -1;
			return argv;
		}
		sleep(1);
		
		fp = NULL;
		LOG("%s::start test fm...\n", __FUNCTION__);
		//__system("busybox chmod 777 /system/bin/fmtester.sh");
		__system("/system/bin/fmtester.sh");
		fp = fopen(SCAN_CHANNEL_RESULT_FILE, "r");
		
		if(fp != NULL)
		{
			LOG("open /data/fm_info sucess.");
			char *temp;			
			char *channel_counts = NULL;
			char *channel_fq = NULL;
			int channelCounts = NULL;			
			char delims[] = "OK\n";
			char results[SCAN_RESULTS_LENGTH];
			char results2[SCAN_RESULTS_LENGTH];
			
			memset(results, 0, SCAN_RESULTS_LENGTH);
			memset(results2, 0, SCAN_RESULTS_LENGTH);
			//fgets(results,SCAN_RESULT_LENGTH-1,fp);
			fread(results, SCAN_RESULTS_LENGTH-1, 1, fp);
			//LOG("results=%s", results);
			strcpy(results2, results);

		    temp = strstr(results,delims);
		    if(temp == NULL) {
				continue;
			}
			
			channel_counts = strtok(results, delims);
			channel_counts = strtok(NULL, delims);
			channel_counts = strtok(channel_counts, " {");
			LOG("result counts=%s", channel_counts);
			channelCounts = strtod(channel_counts,NULL);

			//LOG("results2=%s", results2);
			channel_fq = strtok(results2, "{");
			channel_fq = strtok(NULL, "{");
			channel_fq = strtok(channel_fq, ",");
			LOG("result cap=%s", channel_fq);
			cap = strtod(channel_fq,NULL);
			if(channelCounts > 0)
			{
				fclose(fp);
				fp = NULL;
				break;
			}
		}
	}

	if(cap) {
		char *temp_level;
		char cmd[200];
		char *channelLevel = NULL;
		memset(cmd, 0, 200);
		
		sprintf(cmd, "/system/bin/at_cli_client \"at@audapp:fmrx_set_station(0,%d,0,0)\"", cap);
		//LOG("%s.\n", cmd);
		__system(cmd);
		__system("echo "" > /data/fm_info");
		__system("/system/bin/at_cli_client \"at@audapp:fmrx_get_dynamic_data()\" outfile=/data/fm_info");

		fp = fopen(SCAN_CHANNEL_RESULT_FILE, "r");
		memset(result, 0, SCAN_RESULT_LENGTH);
		fread(result, SCAN_RESULT_LENGTH-1, 1, fp);
		fclose(fp);
		fp = NULL;
		
		//LOG("exect at@audapp:fmrx_get_dynamic_data() return is %s.\n", result);
		
		temp_level = strstr(result,"OK\n");
	    if(temp_level == NULL) {
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_FM,PCBA_FAILED);
			tc_info->result = -1;
			return argv;
		}
			
		channelLevel = strtok(result, "OK\n");
		channelLevel = strtok(NULL, "OK\n");
		//LOG("channelLevel is %s\n", channelLevel);
		strcpy(result, channelLevel);
		channelLevel = strtok(result, " ");
		
		int i=0;
		for(; i<4 && channelLevel!=NULL; i++) {
			//LOG("channelLevel is %s\n", channelLevel);
			channelLevel = strtok(NULL, " ");
		}

		LOG("channelLevel is %s\n", channelLevel);
		int level = strtod(channelLevel, NULL);
		if(level > 3)
		{
			ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] { channel = %d level = %ddBuV} \n", PCBA_FM, PCBA_SECCESS, cap, level);
			tc_info->result = 0;

			return argv;
		}
	}

	ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_FM,PCBA_FAILED);
	tc_info->result = -1;
	return argv;
}
