

#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>

#include <linux/input.h>
#include "lightsensor_test.h"
#include "../../hardware/rockchip/sensor/st/isl29028.h"   
#include "common.h"
#include "test_case.h"
#include "language.h"



#define  LSENSOR_CTL_DEV_PATH    "/dev/lightsensor"
#define LSENSOR_INPUT_NAME		"lightsensor-level"


static float lux_value = 0;

static float indexToValue(size_t index)
{
    static const float luxValues[8] = {
            10.0, 160.0, 225.0, 320.0,
            640.0, 1280.0, 2600.0, 10240.0
    };

    const size_t maxIndex = sizeof(luxValues)/sizeof(*luxValues) - 1;
    if (index > maxIndex)
        index = maxIndex;
    return luxValues[index];
}


static int openInput(const char* inputName)
{
    int fd = -1;
    const char *dirname = "/dev/input";
    char devname[512];
    char *filename;
    DIR *dir;
    struct dirent *de;

    //return getInput(inputName);

    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
                (de->d_name[1] == '\0' ||
                        (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        fd = open(devname, O_RDONLY);
        if (fd>=0) {
            char name[80];
            if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                name[0] = '\0';
            }
            if (!strcmp(name, inputName)) {
                break;
            } else {
                close(fd);
                fd = -1;
            }
        }
    }
    closedir(dir);
   
    return fd;
}



 static int readEvents(int fd)
 {

	struct input_event  event;

	ssize_t n = read(fd, &event,sizeof(struct input_event));
	if (n < 0)
	{
		printf("gsensor read fail!\n");
	 	return n;
	}



	 int type = event.type;
	//printf("type:%d\n",type);
	 if (type == EV_ABS)
	 {		 // #define EV_ABS 0x03
	 	 if (event.value != -1) {
                    // FIXME: not sure why we're getting -1 sometimes
                    lux_value = indexToValue(event.value);
                }
	     
	   
	 }
	
 
     return 0;
 }
 


 void* lightsensor_test(void *argv)
 {
 	
	int ret;
	int fd;
 	//struct gsensor_msg *g_msg =  (struct gsensor_msg *)malloc(sizeof(struct gsensor_msg));
        struct lsensor_msg g_msg;
	struct testcase_info *tc_info = (struct testcase_info*)argv;
	int flags = 1;
		
	/*remind ddr test*/
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	g_msg.y = tc_info->y;
	ui_print_xy_rgba(0,g_msg.y,255,255,0,255,"%s:[%s..] \n",PCBA_LSENSOR,PCBA_TESTING);
	tc_info->result = 0;
	
        /*
 	if(!g_msg)
	{
		printf("malloc for wlan_msg fail!\n");
	}
	else
	{
		g_msg->result = -1;
		//g_msg->y = get_cur_print_y();
	}
        */
        
 	fd = openInput(LSENSOR_INPUT_NAME);
	if(fd < 0)
	{
		ui_print_xy_rgba(0,g_msg.y,255,0,0,255,"%s:[%s]\n",PCBA_LSENSOR,PCBA_FAILED);
		g_msg.result = -1;
		tc_info->result = -1;
		return argv;
	}
	
	int fd_dev = open(LSENSOR_CTL_DEV_PATH, O_RDONLY);
    if(fd_dev<0)
    {
     	printf("opne Lsensor demon fail\n");
		ui_print_xy_rgba(0,g_msg.y,255,0,0,255,"%s:[%s]\n",PCBA_LSENSOR,PCBA_FAILED);
		g_msg.result = -1;
		tc_info->result = -1;
                close(fd);
		return argv;
	
    }
    ret = ioctl(fd_dev, LIGHTSENSOR_IOCTL_ENABLE, &flags);
    if(ret < 0)
    {
		printf("start light sensor fail!\n");
		ui_print_xy_rgba(0,g_msg.y,255,0,0,255,"%s:[%s]\n",PCBA_LSENSOR,PCBA_FAILED);
		g_msg.result = -1;
		tc_info->result = -1;
                close(fd_dev);
                close(fd);
		return argv;
    }
	for(;;)
	{
		readEvents(fd);
		//ui_print_xy_rgba(0,g_msg.y,0,255,0,255,"%s:[%s] { %2d,%2d,%2d }\n",PCBA_GSENSOR,PCBA_SECCESS,(int)g_x,(int)g_y,(int)g_z);
		ui_display_sync(0,g_msg.y,0,255,0,255,"%s:[%s] { %5d }\n",PCBA_LSENSOR,PCBA_SECCESS,(int)lux_value);
		//ui_print_xy_rgba(0,g_msg->y,0,0,255,255,"gsensor x:%f y:%f z:%f\n",g_x,g_y,g_z);
		usleep(100000);
	}

    close(fd);
    close(fd_dev);

    ui_print_xy_rgba(0,g_msg.y,0,255,0,255,"%s:[%s]\n",PCBA_LSENSOR,PCBA_SECCESS);
	tc_info->result = 0;
	return argv;
 }
 

