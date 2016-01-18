#include<stdio.h>
#include <stdlib.h>

//#include "../../hardware/libhardware_legacy/include/hardware_legacy/vibrator.h"
#include"extra-functions.h"
#include"common.h"
#include"vibrator.h"
#include "test_case.h"
#include "language.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#define LOG(x...)   	printf("[VIBRATOR_TEST] "x)
#define THE_DEVICE 		"/sys/class/timed_output/vibrator/enable"

int vibrator_exists()
{
    int fd;

#if 0
#ifdef QEMU_HARDWARE
    if (qemu_check()) {
        return 1;
    }
#endif
#endif

    fd = open(THE_DEVICE, O_RDWR);
    if(fd < 0)
        return 0;
    close(fd);
    return 1;
}

static int sendit(int timeout_ms)
{
    int nwr, ret, fd;
    char value[20];

#if 0
#ifdef QEMU_HARDWARE
    if (qemu_check()) {
        return qemu_control_command( "vibrator:%d", timeout_ms );
    }
#endif
#endif

    fd = open(THE_DEVICE, O_RDWR);
    if(fd < 0)
        return errno;

    nwr = sprintf(value, "%d\n", timeout_ms);
    ret = write(fd, value, nwr);

    close(fd);

    return (ret == nwr) ? 0 : -1;
}

int vibrator_on(int timeout_ms)
{
    /* constant on, up to maximum allowed time */
    return sendit(timeout_ms);
}

int vibrator_off()
{
    return sendit(0);
}


void * vibrator_test(void * argv)
{
	int test_count = 1;
	int y;
	struct testcase_info *tc_info = (struct testcase_info*)argv;
	
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	y = tc_info->y;

	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s..] \n",PCBA_VIBRATOR,PCBA_TESTING);
	sleep(2);
	while(1) {
		if(test_count++ > 3) {
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_VIBRATOR,PCBA_FAILED);
			tc_info->result = -1;
			return argv;
		}
		
		if(!vibrator_exists()) {
			continue;
		}

		if(!vibrator_on(3000)) {
			sleep(3);
			break;
		}
	}
	
	vibrator_off();
	ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] \n", PCBA_VIBRATOR, PCBA_SECCESS);
	tc_info->result = 0;

	return argv;
}
