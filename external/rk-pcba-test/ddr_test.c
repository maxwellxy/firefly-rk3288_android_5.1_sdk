#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "test_case.h"
#include "extra-functions.h"
#include "language.h"

#define MAX_TEST 255
#define MAX_USE_RATE 0.850000
#define DDR_TXT "/data/get_ddr_msg.txt"
#define DDR_NOTE "/proc/driver/ddr_ts"

#define GET_TOTAL_SIZE  0x01
#define GET_USE_RATE	0x02
#define GET_FREE_RATE	0x03

static float total_size = 0;

int get_data(char *buf,char *data)
{
	char *p1 =NULL,* p2=NULL;
	int num =0;
	
        p1 = buf;
        p2 = strstr(p1,data);
        if(p2 == NULL)
        {
                printf("%s no find  %s --- %s\r\n",__FUNCTION__ ,buf,data);
                return 0;
        }

        p1 = p2+strlen(data);
        while(1)
        {
                if(p1[0] != 0x20)
                        break;

                p1++;
        }

        num = atoi(p1);

	return num; 
}

float get_ddr_msg(int state)
{
	FILE *fp = NULL;
        char command[1024],buf[1024];
        int num=0,len=0,ddr_free = 0,ddr_used = 0;
	float result;

        memset(command,0,sizeof(command));
        sprintf(command,"busybox top -n1 | busybox grep Mem: > %s" , DDR_TXT);        
	__system(command);

        fp = fopen(DDR_TXT,"r");
        if(fp == NULL)
        {
                printf("%s open err\r\n",__FUNCTION__);
                return -1;
        }

        memset(buf,0,sizeof(buf));
        len = fread(buf, 1,sizeof(buf), fp);
        if(len<=0)
        {
                printf("%s read err\r\n",__FUNCTION__);
                fclose(fp);
                return -1;
        }
	
	ddr_used = get_data(buf,"Mem:");
	ddr_free = get_data(buf,"used,");
	
	if(ddr_free == 0 || ddr_used ==0) {
                fclose(fp);
		return -1;
        }
  
        fclose(fp);

	switch(state)
	{
		case GET_TOTAL_SIZE:
			return((float)ddr_used + (float)ddr_free); 
		
		case GET_USE_RATE:
        		return ((float)ddr_used/((float)ddr_used+(float)ddr_free));

		case GET_FREE_RATE:
			break;

		default:
			break;
	}

        return -1;
}


int get_block_size(int total_size)
{
	if(total_size < 256*1024)
		return -1;

	return ((total_size/256)*5);
}


int ddr_check_test()
{
	char *ddr_data1[MAX_TEST];
	char *ddr_data2[MAX_TEST];
	int val = 0, num =0,result = 0,i = 0,block_size = 0,y=0;
	float use_rate = 0;

	/*According to the different size calculation for each application space*/
	block_size = get_block_size((int)total_size); 
	if(block_size <= 0)
		return -1;
	
	while(1)
	{	
		if(num >= MAX_TEST)
			break;

		use_rate = get_ddr_msg(GET_USE_RATE);
		if(use_rate >= MAX_USE_RATE)
		{
			printf("%s : test ok :%f\r\n",__FUNCTION__,use_rate);
			break;
		}

		ddr_data1[num] = calloc(1,block_size*1024);
		if(ddr_data1[num] == NULL)
			break;
		
		ddr_data2[num] = calloc(1,block_size*1024);
		if(ddr_data2[num] == NULL)
			break;

		memset(ddr_data1[num],num,block_size*1024);
		memset(ddr_data2[num],num,block_size*1024);

		if(strcmp(ddr_data1[num],ddr_data2[num]) != 0)
		{	
			result = -1;
			break;
		}	
		num++;
	}

	for(i = 0; i< num; i++)
	{	
		if(ddr_data1[i])
			free(ddr_data1[i]);

		if(ddr_data2[i])
			free(ddr_data2[i]);
	}
		
	return result;
}

int ddr_freq_test()
{	
	char cmd[512];
	int max_freq=0,min_freq=0,ret=0,freq_test=0;

	if(script_fetch("ddr", "freq_test", &freq_test, 1) != 0){
		printf("%s: freq test err : %d !!!\r\n",__FUNCTION__,min_freq);
		return -1;
	}	
		
	if(freq_test>0)
	{
		//check node
		ret = access(DDR_NOTE,F_OK);
		if(ret != 0){	
			printf("%s:note err !!!\r\n",__FUNCTION__);
			return -1;
		}

		if(script_fetch("ddr", "max_freq", &max_freq, 1) != 0){
			printf("%s: max freq err : %d !!!\r\n",__FUNCTION__,max_freq);
			return -1;
		}

		if(script_fetch("ddr", "min_freq", &min_freq, 1) != 0){
			printf("%s: min freq err : %d !!!\r\n",__FUNCTION__,min_freq);
			return -1;
		}
		
		if(min_freq<=0 || max_freq<=0){
			printf("%s: parameter error err : %d,%s !!!\r\n",__FUNCTION__,min_freq,max_freq);
			return -1;
		}
		else{	
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 'a:%dM-%dM-1000T' > %s &",min_freq,max_freq,DDR_NOTE);
			printf("%s: set ddr freq cmd: %s \r\n",__FUNCTION__,cmd);
			__system(cmd);
		}
	}

	return 0;
}

void *ddr_test(void *argv)
{
	int ret = 0;
	struct testcase_info *tc_info = (struct testcase_info *)argv;

#ifndef SOFIA3GR_PCBA

	/*remind ddr test*/
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	ui_print_xy_rgba(0,tc_info->y,255,255,0,255,"%s:[%s..] \n",PCBA_DDR,PCBA_TESTING);

	/*Get ddr total size*/
	total_size = get_ddr_msg(GET_TOTAL_SIZE);

	//check ddr test	
	ret = ddr_check_test();
	if(ret < 0){
		ui_print_xy_rgba(0,tc_info->y,255,0,0,255,"%s:[%s] {%s}\n",PCBA_DDR,PCBA_FAILED,PCBA_ERR_CHECK);	
		goto ERR;
	}

	//ddr freq test
	ret = ddr_freq_test();	
	if(ret < 0){
		ui_print_xy_rgba(0,tc_info->y,255,0,0,255,"%s:[%s] {%s}\n",PCBA_DDR,PCBA_FAILED,PCBA_ERR_FREQ);	
		goto ERR;
	}
	
	tc_info->result = 1;	
	ui_print_xy_rgba(0,tc_info->y,0,255,0,255,"%s:[%s] { %d MB }\n",PCBA_DDR,PCBA_SECCESS,((int)total_size)/1024);
	return argv;

ERR:	
	tc_info->result =-1;
#endif
	return argv;
}

