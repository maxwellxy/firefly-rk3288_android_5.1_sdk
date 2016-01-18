#ifndef __CAMERA_TEST_H__
#define __CAMERA_TEST_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <pthread.h>
//#include <linux/videodev.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <linux/ion.h>
//#include "../ion.h"
#include "../../../system/core/libion/kernel-headers/linux/rockchip_ion.h"

//#include <linux/android_pmem.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <linux/version.h>
#include "../test_case.h"

//hkw add
#include"../../../hardware/rockchip/camera/SiliconImage/include/camsys_head.h"
#include "../Language/language.h"
//#include "../language.h"
// the func is a while loop func , MUST  run in a single thread.
//return value: 0 is ok ,-1 erro

struct camera_msg {
	struct testcase_info *tc_info;
	int result;
	int id;
	int x;
	int y;
	int w;
	int h;
};

extern void* camera_test(void *argc);
//return value: 0 is ok ,-1 erro
extern int stopCameraTest();
extern void finishCameraTest();
extern int Camera_Click_Event(int x,int y);
extern int startCameraTest();
#endif
