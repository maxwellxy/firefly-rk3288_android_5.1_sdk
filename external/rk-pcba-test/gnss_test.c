#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include "../../hardware/imc/gnss_drv/gnss_driver/imc_idi_gnss_ioctl.h"  
#include "gnss_test.h"
#include "common.h"
#include "test_case.h"
#include "language.h"
#include"extra-functions.h"
#define GNSS_TTY_DEVICE "/dev/ttyGNSS0"
#define GNSS_RESULT_FILE	"/data/gnss_info"

#define LOG(x...)   				printf("[GPS_TEST] "x)

void* gps_test(void *argv)
{
   
   int ret;
   int fd;
   struct gnss_msg g_msg;
   int chip_id = 0;
   FILE* fp = NULL;
   long file_size= 0;
   struct testcase_info *tc_info = (struct testcase_info*)argv;
	   
   /*remind ddr test*/
   if(tc_info->y <= 0)
	   tc_info->y  = get_cur_print_y();    

   g_msg.y = tc_info->y;
   ui_print_xy_rgba(0,g_msg.y,255,255,0,255,"%s:[%s..] \n",PCBA_GNSS,PCBA_TESTING);


#if 0
	 /*
     * This function should open the tty device. set required properties
     */
    fd = open ( GNSS_TTY_DEVICE, ( O_RDWR | O_NOCTTY | O_NONBLOCK ) );
    if ( fd < 0 )
    {
        ui_print_xy_rgba(0,g_msg.y,255,0,0,255,"%s:[%s]\n",PCBA_GNSS,PCBA_FAILED);
	   g_msg.result = -1;
	   tc_info->result = -1;
	   return argv;
    }

    /* get chip ID */
    ret = ioctl ( fd, GNSS_IOC_GET_CHIPID, &chip_id );
    if ( ret == -1 )
    { 
       close ( fd );
       fd = -1;
       ui_print_xy_rgba(0,g_msg.y,255,0,0,255,"%s:[%s]\n",PCBA_GNSS,PCBA_FAILED);
	   g_msg.result = -1;
	   tc_info->result = -1;
	   return argv;
    }

   close(fd);
   fd = -1;
   printf("%s line=%d chip_id=0x%x \n", __FUNCTION__, __LINE__, chip_id);
#endif

	//__system("busybox chmod 777 /system/bin/gnsstester.sh");
	__system("/system/bin/gnsstester.sh");

	fp = fopen(GNSS_RESULT_FILE, "r");
	if(fp == NULL)
	{
		ui_print_xy_rgba(0,g_msg.y,255,0,0,255,"%s:[%s]\n",PCBA_GNSS,PCBA_FAILED);
		tc_info->result = -1;
		return argv;
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fclose(fp);


	LOG("%s line=%d file_size=%d \n", __FUNCTION__, __LINE__, file_size);

	if(file_size > 1)
	{
		ui_print_xy_rgba(0,g_msg.y,0,255,0,255,"%s:[%s]\n",PCBA_GNSS,PCBA_SECCESS);
		tc_info->result = 0;
		return argv;
	}
	else
	{
		ui_print_xy_rgba(0,g_msg.y,255,0,0,255,"%s:[%s]\n",PCBA_GNSS,PCBA_FAILED);
		tc_info->result = -1;
		return argv;
	}

}



