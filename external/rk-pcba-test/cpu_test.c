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
#include <time.h>

#include <cutils/log.h>

#include <linux/input.h>
#include "../../hardware/rockchip/sensor/st/mma8452_kernel.h"             

#include "test_case.h"
#include "common.h"
#include "extra-functions.h"
#include "script.h"
#include "cpu_test.h"
#include "language.h"

//CPU_INFO  cpu;
int cpu_num0 = 0;
CPU_FREQ cpu_0;

int cpu_num1 = 0;
CPU_FREQ cpu_1;

int cpu_num2 = 0;
CPU_FREQ cpu_2;

int cpu_num3 = 0;
CPU_FREQ cpu_3;

int cpu_space= 0;

void Insert_list(CPU_FREQ * head,CPU_FREQ * item)
{
	CPU_FREQ *p1=NULL,*p0=NULL;

	p1= head;
	p0=item;
	if(head==NULL)
	{	
		head=p0;
		p0->next=NULL;
	}
	else
	{
		while(p1&&p1->next)
		{
			p1=p1->next;
		}
		
		if(p1->freq == 0)
			p1->freq = p0->freq;
		else
			p1->next=p0;
	}
	
}



int cpu_set_mode(char *mode)
{
	char command[1024];

	if(cpu_num0 > 0){	
		memset(command,0,sizeof(command));
		sprintf(command,"echo ""%s"" > %s",mode,_CPU_0_FREQ_GOVERNOR);
		//printf("%s,command:%s\r\n",__FUNCTION__,command);
		__system(command);
	}
	
	if(cpu_num1 > 0){
		memset(command,0,sizeof(command));
		sprintf(command,"echo ""%s"" > %s",mode,_CPU_1_FREQ_GOVERNOR);
		//printf("%s,command:%s\r\n",__FUNCTION__,command);
		__system(command);
	}

	
	if(cpu_num2 > 0){
		memset(command,0,sizeof(command));
		sprintf(command,"echo ""%s"" > %s",mode,_CPU_2_FREQ_GOVERNOR);
		//printf("%s,command:%s\r\n",__FUNCTION__,command);
		__system(command);
	}

	
	if(cpu_num3 > 0){
		memset(command,0,sizeof(command));
		sprintf(command,"echo ""%s"" > %s",mode,_CPU_3_FREQ_GOVERNOR);
		//printf("%s,command:%s\r\n",__FUNCTION__,command);
		__system(command);
	}

	return 0;

}

int get_curl_freq(char *patch)
{
	FILE *fp = NULL;
	char command[1024];
	char buf[1024];
	int num=0,len=0;
	
	memset(command,0,sizeof(command));
	sprintf(command,"busybox cat %s > %s",patch,_CPU_FREQ_TXT);
	//printf("%s,command:%s\r\n",__FUNCTION__,command);

	__system(command);
	
	fp = fopen(_CPU_FREQ_TXT,"r");
	if(fp == NULL)
	{
		printf("%s open err\r\n",__FUNCTION__);
		return 0;
	}

	memset(buf,0,sizeof(buf));
	len = fread(buf, 1,sizeof(buf), fp);
	if(len<=0)
	{	
		printf("%s open err\r\n",__FUNCTION__);
                fclose(fp);
		return 0;
	}
	
	 num = atoi(buf);
	//printf("get_curl_freq:%d \r\n",num);
	fclose(fp);
	
	return num;
}



int set_curl_freq(CPU_FREQ *cpu_freq,int num, char *patch)
{
	char command[1024];
	int i =num;
	CPU_FREQ * p = cpu_freq;
	int speed = 0;

	//printf("set_curl_freq:%d\r\n",i);
	
	while(p){
		if((--i)<=0)
			break;	

		if(p->next)
			p=p->next;	 		
	}
	
	speed = p->freq;

	
	memset(command,0,sizeof(command));
	sprintf(command,"echo %d > %s",speed,patch);
	//printf("%s,command:%s\r\n",__FUNCTION__,command);
	__system(command);

	return 0;
}


int get_freq_table(CPU_FREQ *cpu_freq, char *patch)
{
	FILE *fp = NULL;
	char command[1024];
	char buf[1024];
	int len=0;
	char *p1=NULL,*p2=NULL;
	CPU_FREQ *new_freq;
	char  buf_num[32];
	int num=0;
	
	memset(command,0,sizeof(command));
	sprintf(command,"busybox cat %s > %s",patch,_CPU_FREQ_TXT);
	//printf("%s,command:%s\r\n",__FUNCTION__,command);

	__system(command);
	
	fp = fopen(_CPU_FREQ_TXT,"r");
	if(fp == NULL)
	{
		printf("%s open err\r\n",__FUNCTION__);
		return 0;
	}

	memset(buf,0,sizeof(buf));
	len = fread(buf, 1,sizeof(buf), fp);
	if(len<=0)
	{	
		printf("%s read err\r\n",__FUNCTION__);
                fclose(fp);
		return 0;
	}

	//printf("get_freq_table:len:%d buf : %s\r\n",len,buf);

	p1 =buf;
	while(1)
	{
		  p2=strstr(p1," ");
                if(p2==NULL)
                {
                        printf("%s:strstr end\r\n",__FUNCTION__);
                        break;
                }
                
                new_freq= calloc(1,sizeof(CPU_FREQ));
                if(new_freq ==0)
                {
                        printf("%s:calloc err\r\n",__FUNCTION__);
                        break;
                }
		 
                new_freq->freq = atoi(p1);
		  printf("num:%d,%d\r\n",num,new_freq->freq);
                Insert_list(cpu_freq,new_freq);
                p1=p2+1;
		  num++;
	}

	
	fclose(fp);
	
	return num;
}


void* cpufreq_test(void* argv)
{	
	struct testcase_info *tc_info = (struct testcase_info*)argv;
	int num=0,ret=0;
	int cyc = 0,screan = -1;
	unsigned int getdata=0,i=0;
	
	/*check last test ok? */

	//memset(cpu,0,sizeof(cpu));

	/* get all freq*/
	cpu_num0=get_freq_table(&cpu_0,_CPU_0_FREQ_TABLE);
	cpu_num1=get_freq_table(&cpu_1,_CPU_1_FREQ_TABLE);
	cpu_num2=get_freq_table(&cpu_2,_CPU_2_FREQ_TABLE);
	cpu_num3=get_freq_table(&cpu_3,_CPU_3_FREQ_TABLE);

	/*set mode to userspace*/
	cpu_set_mode(_CPU_MODE_USER);
	

	while(1)
	{			
		switch(cyc)
		{
			case 0: /*if first time*/
				if(cpu_num0>0)
					set_curl_freq(&cpu_0,1,_CPU_0_FREQ_SET);			
				if(cpu_num1>0)
					set_curl_freq(&cpu_1,1,_CPU_1_FREQ_SET);
				if(cpu_num2>0)
					set_curl_freq(&cpu_2,1,_CPU_2_FREQ_SET);
				if(cpu_num3>0)
					set_curl_freq(&cpu_3,1,_CPU_3_FREQ_SET);	
				break;

			case 10:/*if end time*/
				if(cpu_num0>0)
					set_curl_freq(&cpu_0,cpu_num0,_CPU_0_FREQ_SET);	
				if(cpu_num1>0)
					set_curl_freq(&cpu_1,cpu_num1,_CPU_1_FREQ_SET);	
				if(cpu_num2>0)
					set_curl_freq(&cpu_2,cpu_num2,_CPU_2_FREQ_SET);	
				if(cpu_num3 >0)
					set_curl_freq(&cpu_3,cpu_num3,_CPU_3_FREQ_SET);	
				break;
				
			default:
			{	
				/*cpu0*/
				/*random number*/
				/*set freq*/
				if(cpu_num0>0)
				{
					num = rand()%cpu_num0;
					ret = set_curl_freq(&cpu_0,num,_CPU_0_FREQ_SET);
				}

				if(cpu_num1>0)
				{
					num = rand()%cpu_num1;
					ret = set_curl_freq(&cpu_1,num,_CPU_1_FREQ_SET);
				}

				if(cpu_num2>0)
				{
					num = rand()%cpu_num2;
					ret = set_curl_freq(&cpu_2,num,_CPU_2_FREQ_SET);
				}

				if(cpu_num3>0)
				{
					num = rand()%cpu_num3;
					ret = set_curl_freq(&cpu_3,num,_CPU_3_FREQ_SET);
				}
			
			}
			break;
		}
		
		if(cyc >= 10)
			cyc = 0;
		else
			cyc++;

		/*get freq*/		
	     //  ui_print_xy_rgba(0,tc_info->y,0,255,0,255,"CPU: [OK]{%d,%d}\n",get_curl_freq(_CPU_0_FREQ_GET),cpu_space);
		//printf("cpufreq_test:%d,%d\r\n",get_curl_freq(_CPU_0_FREQ_GET),cpu_space);
		/*wait time 3 ?*/
		usleep(100000);
		
	}

	return argv;
}


static float angle_pitch_to_angle(float pitch, float accData)
{	
	float angle = 0.0f;
	
	if((pitch > 0) && (accData > 0))
	{
		pitch = pitch;
	}
	else if((pitch > 0) && (accData < 0))
	{
		pitch = 3.14159f - pitch;
	}
	else if((pitch < 0) && (accData > 0))
	{
		pitch = 6.28318f + pitch;
	}
	else if((pitch < 0) && (accData < 0))
	{
		pitch = 3.14159f - pitch;
	}

	angle = pitch * 180 / 3.14159f;	

	return angle;

}


void* add_use_cpu(void)
{
	int ret;
	FILE * fd_mma =NULL;
	struct sensor_axis acc = {90000, 30000, 4000};
	struct sensor_axis angle = {6000, 9000, 20000};
	int sample_rate = MMA8452_RATE_12P5;
	int num =0;
	float angleData[3],accData[3];
	float anglePitch, angleRoll;	
	float accPitch, accRoll;
	int curr_angle[2],old_angle,needcheck=0;

	//printf("cpustress_test start\r\n");

	fd_mma  = open("/dev/accel", O_RDWR);
	if(fd_mma==NULL){
		printf("%s:open:  - err\r\n",__FUNCTION__);
		return 0;
	}
	
	while(1)
	{			
		if (0 > ioctl(fd_mma, MMA_IOCTL_GETDATA, &acc) )
		{
			acc.x =90000;
			acc.y = 30000;
			acc.z = 4000;
		}

		accData[0] = ( (acc.x) * ACCELERATION_RATIO_ANDROID_TO_HW);
		accData[1] = ( (acc.y) * ACCELERATION_RATIO_ANDROID_TO_HW);
		accData[2] = ( (acc.z) * ACCELERATION_RATIO_ANDROID_TO_HW);

		angleData[0] = ( (angle.x) * ACCELERATION_RATIO_ANDROID_TO_HW);
		angleData[1] = ( (angle.y) * ACCELERATION_RATIO_ANDROID_TO_HW);
		angleData[2] = ( (angle.z) * ACCELERATION_RATIO_ANDROID_TO_HW);

		//calculate angle
		anglePitch = atan2(angleData[0], sqrt((int)(angleData[1] * angleData[1]) + (int)(angleData[2] * angleData[2])));
		angleRoll = atan2(angleData[1], sqrt((int)(angleData[0] * angleData[0]) + (int)(angleData[2] * angleData[2])));

		accPitch = atan2(accData[0], sqrt((int)(accData[1] * accData[1]) + (int)(accData[2] * accData[2])));
		accRoll = atan2(accData[1], sqrt((int)(accData[0] * accData[0]) + (int)(accData[2] * accData[2])));

		anglePitch = angle_pitch_to_angle(anglePitch, angleData[2]);
		angleRoll = angle_pitch_to_angle(angleRoll, angleData[2]);

		accPitch = angle_pitch_to_angle(accPitch, accData[2]);
		accRoll = angle_pitch_to_angle(accRoll, accData[2]);

		if(anglePitch > accPitch)
			curr_angle[0] = (int)(anglePitch - accPitch);
		else	
			curr_angle[0] = (int)((anglePitch - accPitch) + 360) % 360;

		if(angleRoll > accRoll)
			curr_angle[1] = (int)(angleRoll - accRoll);
		else
			curr_angle[1] = (int)((angleRoll - accRoll) + 360) % 360;
	}
	
	return 0;
}


int get_cpu_info()
{
	 FILE *fp = NULL;
        char command[1024];
        char buf[1024];
        int num=0,len=0;
        char *p1 =NULL,* p2=NULL;

        memset(command,0,sizeof(command));
        sprintf(command,"busybox top -n1  | busybox grep idle > data/cpu_idle.txt");
        //printf("%s,command:%s\r\n",__FUNCTION__,command);

        __system(command);

        fp = fopen("data/cpu_idle.txt","r");
        if(fp == NULL)
        {
                printf("%s open err\r\n",__FUNCTION__);
                return 0;
        }

        memset(buf,0,sizeof(buf));
        len = fread(buf, 1,sizeof(buf), fp);
        if(len<=0)
        {
                printf("%s open err\r\n",__FUNCTION__);
                fclose(fp);
                return 0;
        }

        p1 = buf;
        p2 = strstr(p1,"nice");
        if(p2 == NULL)
        {
                printf("%s no find  nice\r\n",__FUNCTION__);
                fclose(fp);
                return 0;
        }

        p1 = p2+strlen("nice");

        while(1)
        {
                if(p1[0] != 0x20)
                        break;

                p1++;
        }

        //printf("%s\r\n",p1);
        num = atoi(p1);
        //printf("%s:%d \r\n",__FUNCTION__,num);
        fclose(fp);

        return num;
		
}


int get_boot_mode()
{
	 FILE *fp = NULL;
        char command[1024];
        char buf[1024];
        int num=0,len=0;
        char *p1 =NULL,* p2=NULL;

        memset(command,0,sizeof(command));
        sprintf(command,"busybox dmesg -s 50000 | busybox grep \"Boot mode:\" > data/boot_mode.txt");
        //printf("%s,command:%s\r\n",__FUNCTION__,command);

        __system(command);

        fp = fopen("data/boot_mode.txt","r");
        if(fp == NULL)
        {
                printf("%s open err\r\n",__FUNCTION__);
                return 0;
        }

        memset(buf,0,sizeof(buf));
        len = fread(buf, 1,sizeof(buf), fp);
        if(len<=0)
        {
                printf("%s open err\r\n",__FUNCTION__);
                fclose(fp);
                return 0;
        }

        p1 = buf;
        p2 = strstr(p1,"(");
        if(p2 == NULL)
        {
                printf("%s no find \r\n",__FUNCTION__);
                fclose(fp);
                return 0;
        }

	 p1=p2+strlen("(");
        num = atoi(p1);
        fclose(fp);

        return num;
		
}


static const char *boot_mode_name(int mode)
{
	switch (mode) {
		case BOOT_MODE_NORMAL: return "NORMAL";
		case BOOT_MODE_FACTORY2: return "FACTORY2";
		case BOOT_MODE_RECOVERY: return "RECOVERY";
		case BOOT_MODE_CHARGE: return "CHARGE";
		case BOOT_MODE_POWER_TEST: return "POWER_TEST";
		case BOOT_MODE_OFFMODE_CHARGING: return "OFFMODE_CHARGING";
		case BOOT_MODE_REBOOT: return "REBOOT";
		case BOOT_MODE_PANIC: return "PANIC";
		case BOOT_MODE_WATCHDOG: return "WATCHDOG";
		default: return "";
	}
}


void* cpu_test(void* argv)
{	
	struct testcase_info *tc_info = (struct testcase_info*)argv;
	int i = 0,ret = 0,bootmode =0;
 	pthread_t pid_stress[__MAX],freq_tid;

#ifndef SOFIA3GR_PCBA

	/*remind ddr test*/
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	ui_print_xy_rgba(0,tc_info->y,255,255,0,255,"cpu:[%s..] \n",PCBA_TESTING);

	//check last boot mode
	bootmode = get_boot_mode();
	if(bootmode == BOOT_MODE_PANIC || bootmode == BOOT_MODE_WATCHDOG){
		
		if(tc_info->y == 0)
			tc_info->y = get_cur_print_y();

		tc_info->result = -1;
		 ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"CPU:[%s] { %s }\n",PCBA_FAILED,boot_mode_name(bootmode));
	} 
	else{

		tc_info->result = 1;

		if(tc_info->y > 0)
			ui_print_xy_rgba(0,tc_info->y,0,255,0,255,"CPU:[%s] { %s }\n",PCBA_SECCESS,boot_mode_name(bootmode));
	}
	

	//freq
	pthread_create(&freq_tid, NULL, cpufreq_test, tc_info); 	


	//stress
	while(1)
 	{
		if(i>__MAX)
		{
			usleep(2);
			continue;
		}

		cpu_space = get_cpu_info();
		if(cpu_space > 10)
		{
			printf("i:%d,space:%d\r\n",i,cpu_space);
			pthread_create(&pid_stress[i], NULL, add_use_cpu, (void*)i);
			i++;
		}
		
	 	sleep(2);
 	}
#endif

	return argv;
}


