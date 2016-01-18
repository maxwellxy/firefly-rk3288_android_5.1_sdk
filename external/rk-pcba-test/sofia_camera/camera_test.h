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
#include <linux/videodev2.h>
#include <linux/fb.h>
#include "test_case.h"
#include <linux/ion.h>
#include <sys/poll.h>

extern void* camera_test(void *argc);
//return value: 0 is ok ,-1 erro
extern int Camera_Click_Event(int x,int y);
extern int startCameraTest();
#endif
