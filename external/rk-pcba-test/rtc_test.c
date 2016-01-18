#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/android_alarm.h>
#include <linux/rtc.h>

#include <stdlib.h>
#include "common.h"
#include "rtc_test.h"
#include "script.h"
#include "test_case.h"
#include "language.h"

int  rtc_xopen(int flags)
{
	int rtc;
	char major_rtc[] = "/dev/rtc";
	char minor_rtc[] = "/dev/rtc0";
	
	rtc = open(major_rtc, flags);
	if (rtc < 0)
	{
		rtc = open(minor_rtc, flags);
		if(rtc >= 0)
		{
		//	printf("open %s\n",minor_rtc);
		}
		else
		{
			printf("open %s failed:%s\n",minor_rtc,strerror(errno));
		}
	}
	else
	{
		printf("open %s\n",major_rtc);
	}

	return rtc;
}

int  rtc_read_tm(struct tm *ptm, int fd)
{
	int ret;
	memset(ptm, 0, sizeof(*ptm));

	ret = ioctl(fd, RTC_RD_TIME, ptm);
	if(ret < 0)
	{
		printf("read rtc failed:%s\n" ,strerror(errno)); 
	}
	else
	{
		ptm->tm_isdst = -1; /* "not known" */
	}

	return ret;
}

static int  read_rtc(time_t *time_p)
{
	struct tm tm_time;
	int fd;
	int ret;

	fd = rtc_xopen(O_RDONLY);
	if(fd < 0)
	{
		return fd;
	}
	else
	{
		ret = rtc_read_tm(&tm_time, fd);
	}

	close(fd);
	if(ret < 0)
		return ret;
	else
		*time_p = mktime(&tm_time);

	return 0;
}

int get_system_time(char *dt)
{    
	int fd;
	time_t t;
	int ret;
	
	#if 1
	time_t timep;
	struct tm *p; 
	ret = read_rtc(&timep);
	if(ret <  0)
		return ret;
	else
		p = localtime(&timep);    
	sprintf(dt,"%04d-%02d-%02d %02d:%02d:%02d",(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec); 
	//printf("time is %s \n",dt);    
	#else
	struct timeval tv;
   	gettimeofday(&tv, NULL);
	printf("%s>>>>>tv.tv_sec:%ld>>tv.tv_usec:%d\n",__func__,tv.tv_sec,tv.tv_usec);
	#endif

	return timep;
	
	
	
	return 0;
}

int set_system_time(struct timeval *tv)
{    
	int ret;
	int fd;
	
	#if 0
	if(settimeofday(tv, NULL) < 0)   
	{        
		printf("Set system datatime error:%s\n" ,strerror(errno));        
		return 0;    
	}    
	else    
	{    
		printf("Set system datatime successfully!\n");       
		return 1;    
	}
	#else

	#ifdef SOFIA3GR_PCBA
	struct rtc_time rtc;   
	struct tm tm, *gmtime_res;    
	//int fd;    
	int res;    
	res = settimeofday(tv, NULL);
	if (res < 0) {        
		printf("settimeofday() failed: %s\n", strerror(errno));        
		return -1;    
	}    
	fd = open("/dev/rtc0", O_RDWR);    
	if (fd < 0) {        
		printf("Unable to open RTC driver: %s\n", strerror(errno));        
		return res;    
	}    
	gmtime_res = gmtime_r(&tv->tv_sec, &tm);    
	if (!gmtime_res) 
	{        
		printf("gmtime_r() failed: %s\n", strerror(errno));        
		res = -1;        
		goto done;   
	}    
	memset(&rtc, 0, sizeof(rtc));   
	rtc.tm_sec = tm.tm_sec;    
	rtc.tm_min = tm.tm_min;   
	rtc.tm_hour = tm.tm_hour;    
	rtc.tm_mday = tm.tm_mday;    
	rtc.tm_mon = tm.tm_mon;    
	rtc.tm_year = tm.tm_year;    
	rtc.tm_wday = tm.tm_wday;    
	rtc.tm_yday = tm.tm_yday;    
	rtc.tm_isdst = tm.tm_isdst;    
	res = ioctl(fd, RTC_SET_TIME, &rtc);    
	if (res < 0)       
		printf("RTC_SET_TIME ioctl failed: %s\n", strerror(errno));

	done:    
		close(fd);    
		return res;
	
	#else
	fd = open("/dev/alarm", O_RDWR);
	if(fd < 0)
	{
		printf("open /dev/alarm failed:%s\n" ,strerror(errno)); 
		return -1;
	}
    	ret = ioctl(fd, ANDROID_ALARM_SET_RTC, tv);
	if(ret < 0)
	{
		printf("set rtc failed:%s\n" ,strerror(errno));
		return -1;
	}
	#endif
	
	#endif
	return 0;
}  

void* rtc_test(void *argc)
{
	struct testcase_info *tc_info = (struct testcase_info*)argc;
	char dt[32]={"20120926.132600"};
	int ret,y;
	struct tm tm;
	struct timeval tv;
	char *s;
	int day,hour;
	time_t t;
	struct tm *p;
	struct timespec ts;
	
	/*remind ddr test*/
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();
	
	y = tc_info->y;	
	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s..] \n",PCBA_RTC,PCBA_TESTING);


	s = malloc(32);
	 if(script_fetch("rtc", "module_args", (int *)dt, 8) == 0)
	 {
	 	//printf("%s>>>args:%s\n",__func__,s);
                strncpy(s, dt, 32);
	}

	//printf("%s>>>%s\n",__func__,s);
	day = atoi(s);

	while (*s && *s != '.')
		s++;

	if (*s)
		s++;

	hour = atoi(s);

	//printf("day:%d>>hour:%d\n",day,hour);
	tm.tm_year = day / 10000 - 1900;
	tm.tm_mon = (day % 10000) / 100 - 1;
	tm.tm_mday = (day % 100);
	tm.tm_hour = hour / 10000;
	tm.tm_min = (hour % 10000) / 100;
	tm.tm_sec = (hour % 100);
	tm.tm_isdst = -1;

    	tv.tv_sec = mktime(&tm);
	tv.tv_usec = 0;
	printf("set rtc time :%lu\n",tv.tv_sec);
	ret = set_system_time(&tv);
	if(ret < 0)
	{
		//rtc_msg->result = -1;
		printf("test rtc failed:set_system_time failed \n");
		ret = -1;
	}
	else
	{
#ifndef SOFIA3GR_PCBA
	    sleep(1);
//	    y=get_cur_print_y();
		while(1)
		{
			t = get_system_time(dt);
			if(t < 0)
			{
			//rtc_msg->result = -1;
				ret = -1;
				break;
			}
			p = localtime(&t);
			//ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] { %04d/%02d/%02d %02d:%02d:%02d }\n",PCBA_RTC,PCBA_SECCESS,(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
			ui_display_sync(0,y,0,255,0,255,"%s:[%s] { %04d/%02d/%02d %02d:%02d:%02d }\n",PCBA_RTC,PCBA_SECCESS,(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
			sleep(1);
		}
#else
		t = get_system_time(dt);
		if(t < 0)
		{
			//rtc_msg->result = -1;
			printf("test rtc failed:get_system_time failed \n");
			ret = -1;
		}
		else
		{
			if((t - tv.tv_sec > 10))
			{
				printf("test rtc failed:settime:%lu>>read time:%lu\n",
					tv.tv_sec,t);
				//rtc_msg->result = -1;
				ret = -1;
			}
			else
			{
				//rtc_msg->result = 0;
				ret = 0;
			}
		}
#endif
	}
	
	if(ret == 0)
	{
		tc_info->result = 0;
	#ifdef SOFIA3GR_PCBA
		p = localtime(&t);
		ui_display_sync(0,y,0,255,0,255,"%s:[%s] { %04d/%02d/%02d %02d:%02d:%02d }\n",PCBA_RTC,PCBA_SECCESS,(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
	#else
	//	ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"rtc: ok!   { %s }\n",dt);
		ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s]\n",PCBA_RTC,PCBA_SECCESS);
	#endif
	}
	else
	{
		tc_info->result = -1;
		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_RTC,PCBA_FAILED);
	}
	
	
	return argc;
}

