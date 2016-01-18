#include <stdio.h>
#include <stdlib.h>
#include "extra-functions.h"
#include "common.h"
#include "flashlight_test.h"
#include "test_case.h"
#include "language.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/videodev2.h>

#define LOG(x...)   printf("[FLASHLIGHT_TEST] "x)

#define CAMERA_OVERLAY_DEV_NAME   "/dev/video0"

static int m_flash_on;
static int m_cam_fd_overlay;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond;
extern int lock;

static int fimc_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.value = value;
    ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOG("ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, value, ret);
    }

    return ret;
}

static int fimc_v4l2_s_flash_mode(int fp, enum xgold_flash_enable_t flash_enable_cmd)
{
   int ret = 0;

   LOG("%s 0x%x %d",__func__,V4L2_CID_FLASH_LED_MODE, flash_enable_cmd);

   if (flash_enable_cmd == FLASH_OFF)
      ret = fimc_v4l2_s_ctrl(fp, V4L2_CID_FLASH_LED_MODE, V4L2_FLASH_LED_MODE_NONE);
   else if (flash_enable_cmd == FLASH_ON)
      ret = fimc_v4l2_s_ctrl(fp, V4L2_CID_FLASH_LED_MODE, V4L2_FLASH_LED_MODE_FLASH);
   else if (flash_enable_cmd == FLASH_TORCH)
      ret = fimc_v4l2_s_ctrl(fp, V4L2_CID_FLASH_LED_MODE, V4L2_FLASH_LED_MODE_TORCH);
   else
      ret = -EINVAL;

   if (ret < 0)
      LOG("ERR(%s):Set Flash Mode failed\n", __func__);

   return ret;
}

int triggerFlash(int enable)
{
    int ret = 0;

    if(m_flash_on != enable)
    {
        if (enable)
            ret = fimc_v4l2_s_flash_mode(m_cam_fd_overlay, FLASH_ON);
        else
            ret = fimc_v4l2_s_flash_mode(m_cam_fd_overlay, FLASH_OFF);

        if(ret == 0)
        {
            m_flash_on = enable;
        }
    }

    return ret;
}

int initFlashlightFd()
{
	m_cam_fd_overlay = open(CAMERA_OVERLAY_DEV_NAME, O_RDWR | O_NONBLOCK);
    LOG("%s :m_cam_fd_overlay %d \n", __func__, m_cam_fd_overlay);
    if (m_cam_fd_overlay < 0) {
        LOG("ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_OVERLAY_DEV_NAME, strerror(errno));
        return -1;
    }

	return 0;
}

void * flashlight_test(void * argv)
{
	int test_count = 1;
	int y;
	int flashlightTurnoffcounts = 0;
	struct testcase_info *tc_info = (struct testcase_info*)argv;
	
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	y = tc_info->y;

	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s..] \n",PCBA_FLASHLIGHT,PCBA_TESTING);
	pthread_mutex_lock(&mutex);  
	while(lock)  
	   pthread_cond_wait(&cond,&mutex);  
	pthread_mutex_unlock(&mutex);

	LOG("flashlight start testing...");
	while(1) {
		if(test_count++ > 3) {
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_FLASHLIGHT,PCBA_FAILED);
			tc_info->result = -1;
			return argv;
		}
		
		if(initFlashlightFd() < 0) {
			continue;
		}

		if(triggerFlash(1) >= 0) {
			sleep(2);
			break;
		}
	}

	while(triggerFlash(0) < 0) {
		flashlightTurnoffcounts++;
		
		if(flashlightTurnoffcounts >= 3) {
			LOG("Turn off flashlight faile.\n");
			break;
		}
		
	}
	
	if (m_cam_fd_overlay > -1) {
         close(m_cam_fd_overlay);
         m_cam_fd_overlay = -1;
    }
	ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] \n", PCBA_FLASHLIGHT, PCBA_SECCESS);
	tc_info->result = 0;

	return argv;
}
