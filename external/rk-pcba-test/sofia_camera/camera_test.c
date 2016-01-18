#include "camera_test.h"
#include "./minuitwrp/minui.h"
#include "test_case.h"
#include "common.h"
#include "script.h"
#include "test_case.h"
#include "language.h"
#include <linux/videodev2.h>

#define LOG(x...)   printf("[Camera_TEST] "x)

static int fd = -1;
static int rc = -2;
static int mDevFp = -1;
static int y = 0;
int *pBuf;
struct ion_fd_data fd_data;
struct ion_allocation_data alloc_data;
struct v4l2_format v4l2Fmt;
struct v4l2_requestbuffers req;
struct v4l2_buffer v4l2Buf;
struct v4l2_input input;
pthread_t camera_tid;
struct testcase_info *tc_info;

extern pthread_mutex_t mutex;
extern pthread_cond_t cond;
extern int lock;


int pollDataFromCamera(int index)
{
	int poll_counts = 0;
	int ret = -1;
	struct pollfd mEvents;

POLL_WORK:
	poll_counts++;
	mDevFp = open("/dev/video2", O_RDWR);
	if (mDevFp < 0) {
		LOG("startCameraTest::Cannot open camera.\n");
		goto POLL_ERROR;
	}

	fd = open("/dev/ion", O_RDWR);
	if (fd < 0) {
	    LOG("startCameraTest::opening ion device failed with fd = %d\n", fd);
	    goto POLL_ERROR;
	}
	alloc_data.flags = 0;
	alloc_data.heap_id_mask = (1<<4);//camera_id = 4,mask=1<<4
	alloc_data.len = 4194304;//1048576;//128*4096;//854*480;//must page align
	alloc_data.align = 0;//1024*1024;//clip2(alignment);
	rc = ioctl(fd,ION_IOC_ALLOC,&alloc_data);

	if(rc){
        LOG("startCameraTest::ION ALLOC memory failed,rc=%d\n",rc);
        close(fd);
        fd = -1;
        goto POLL_ERROR;
	}

	fd_data.handle = alloc_data.handle;
    rc = -2;
	rc = ioctl(fd,ION_IOC_MAP,&fd_data);
	
	if (rc) {
		LOG("startCameraTest::ION MAP failed,rc=%d\n",rc);
		fd_data.fd =-1;
		close(fd);
		fd = -1;
		goto POLL_ERROR;
	}
	
    LOG("startCameraTest::pBuf=0x%08x,fd_data.fd=%d\n",pBuf,fd_data.fd);
	pBuf = mmap(NULL, 4194304, 3, 1, fd_data.fd, 0);
	if (pBuf == MAP_FAILED) {
        LOG("startCameraTest::mmap failed,pBuf=0x%08x\n",pBuf);
        goto POLL_ERROR;
	}
	memset(pBuf,0x55,1024);
	memset(pBuf+854*480/4,0x55,1024);
	
	input.index = index;
	ret = ioctl(mDevFp, VIDIOC_S_INPUT, &input);
	if (ret < 0) {
		LOG("pollDataFromCamera::channel[%d] set input error,ret=%d\n",input.index,ret);
		goto POLL_ERROR;
	}
	LOG("pollDataFromCamera::set input ok\n");
	
	//设置format
	memset(&v4l2Fmt, 0, sizeof(v4l2Fmt));
	v4l2Fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2Fmt.fmt.pix.width = 320;
	v4l2Fmt.fmt.pix.height = 240;
	v4l2Fmt.fmt.pix.bytesperline = 0;//854*4;//0
	v4l2Fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422P;//V4L2_PIX_FMT_RGB32;//V4L2_PIX_FMT_YUV422P;//v4l2PixFmt;
	v4l2Fmt.fmt.pix.sizeimage = (320 * 240 * 16) / 8;
	
	ret = -1;
	ret = ioctl(mDevFp, VIDIOC_S_FMT, &v4l2Fmt);
	if(ret){
		LOG("pollDataFromCamera::set format error.\n");
		goto POLL_ERROR;
	}
   	
   	LOG("pollDataFromCamera::set format ok\n");
	
	//申请帧缓冲空间
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	
	ret = -1;
	ret = ioctl(mDevFp, VIDIOC_REQBUFS, &req);
	if(ret){
		LOG("pollDataFromCamera::request buffers error,ret=%d\n",ret);
		goto POLL_ERROR;
	}
	
	LOG("pollDataFromCamera::request buffers ok\n");
	
	//把申请到的帧缓冲全部入队列，以便存放采集到的数据
	v4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2Buf.memory = V4L2_MEMORY_USERPTR;
	v4l2Buf.index = 0;//buffer.getIndex();
	v4l2Buf.m.userptr = pBuf;
	v4l2Buf.length = (854*480*16/8);//buffer.getCapacity();
	
	ret = -1;
	ret = ioctl(mDevFp, VIDIOC_QBUF, &v4l2Buf);
	if(ret){
		LOG("pollDataFromCamera::camera qbuf error,ret=%d\n",ret);
		goto POLL_ERROR;
	}
	
	LOG("pollDataFromCamera::queue buffer ok\n");
	
	//开始视频的采集
	ret = -1;
	ret = ioctl(mDevFp, VIDIOC_STREAMON, &v4l2Buf.type);
	if(ret){
		LOG("pollDataFromCamera::camera streamon error,ret=%d\n",ret);
		goto POLL_ERROR;
	}
	
	LOG("pollDataFromCamera::turn on camera ok\n");

	
	//出队列以取得已采集数据的帧缓冲，取得原始采集数据
	ret = -1;
	mEvents.fd = mDevFp;
   	mEvents.revents = 0;
   	mEvents.events = POLLIN | POLLERR;
	ret = poll(&mEvents, 1, 1000);
	if (ret < 0) {
		LOG("pollDataFromCamera::camera dqbuf poll error,ret=%d\n",ret);
		ioctl(mDevFp, VIDIOC_STREAMOFF, &v4l2Buf.type);
		goto POLL_ERROR;
	}
	else if (!ret) {
		LOG("pollDataFromCamera::camera dqbuf poll timeout,ret=%d\n",ret);
		ioctl(mDevFp, VIDIOC_STREAMOFF, &v4l2Buf.type);
		goto POLL_ERROR;
   	}
   	LOG("pollDataFromCamera::camera poll ok,ret=%d\n",ret);

	ret = ioctl(mDevFp, VIDIOC_STREAMOFF, &v4l2Buf.type);
	if (ret < 0) {
        LOG("ERR(%s):VIDIOC_STREAMOFF failed\n", __func__);
        goto POLL_ERROR;
    }
	
	if(mDevFp >= 0) {
		close(mDevFp);
	}

	if(fd >= 0) {
		close(fd);
	}
	return 1;

POLL_ERROR:
	if(mDevFp >= 0) {
		close(mDevFp);
	}

	if(fd >= 0) {
		close(fd);
	}
	
	if(poll_counts >= 1) {
		return -1;
	}
	
	goto POLL_WORK;
}

int startCameraTest()
{
	int ret = -1;

	pthread_mutex_lock(&mutex);  
	  
	mDevFp = open("/dev/video2", O_RDWR);
	if (mDevFp < 0) {
		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] %s:[%s]\n",PCBA_BACK_CAMERA,PCBA_FAILED,PCBA_FRONT_CAMERA,PCBA_FAILED);
		tc_info->result = -1;
		LOG("startCameraTest::Cannot open camera.\n");
		return mDevFp;
	}

	printf("startCameraTest::open camera ok\n");

	ret = -1;
	int index = 0;
	while(1){
		input.index = index;
		ret = ioctl(mDevFp, VIDIOC_ENUMINPUT, &input);
		if(ret != 0){
			//LOG("startCameraTest::channel[%d] enum input error,ret=%d\n",input.index,ret);
			break;//return ret;
		}
		//LOG("startCameraTest::Name of input channel[%d] is %s\n", input.index, input.name);
		index++;
	}

	close(mDevFp);
	
	if(index == 0){
		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] \n",PCBA_CAMERA,PCBA_FAILED);
		tc_info->result = -1;
		LOG("%s line=%d %s:[%s] \n", __FUNCTION__, __LINE__ ,PCBA_CAMERA,PCBA_FAILED);
		return ret;
	}

	if(index == 1)
	{
		if(pollDataFromCamera(0) > 1)
		{
			ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s]\n",PCBA_BACK_CAMERA,PCBA_SECCESS);
			tc_info->result = 0;
		}
		else
		{
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s]\n",PCBA_BACK_CAMERA,PCBA_FAILED);
			tc_info->result = -1;
			LOG("%s line=%d %s:[%s] \n", __FUNCTION__, __LINE__ ,PCBA_BACK_CAMERA,PCBA_FAILED);
		}
	}

	if(index == 2)
	{
		int result_front = pollDataFromCamera(1);
		int result_back = pollDataFromCamera(0);
		LOG("startCameraTest::result_back=%d,result_front=%d\n", result_back, result_front);
		if(result_back == 1 && result_front <= 0)
		{
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] %s:[%s]\n",PCBA_BACK_CAMERA,PCBA_SECCESS,PCBA_FRONT_CAMERA,PCBA_FAILED);
			tc_info->result = -1;
			LOG("%s line=%d %s:[%s] %s:[%s]\n", __FUNCTION__, __LINE__ ,PCBA_BACK_CAMERA,PCBA_SECCESS,PCBA_FRONT_CAMERA,PCBA_FAILED);
		}

		if(result_back <= 0 && result_front == 1)
		{
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] %s:[%s]\n",PCBA_BACK_CAMERA,PCBA_FAILED,PCBA_FRONT_CAMERA,PCBA_SECCESS);
			tc_info->result = -1;
			LOG("%s line=%d %s:[%s] %s:[%s]\n", __FUNCTION__, __LINE__ ,PCBA_BACK_CAMERA,PCBA_FAILED,PCBA_FRONT_CAMERA,PCBA_SECCESS);
		}

		if(result_back <= 0 && result_front <= 0)
		{
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] %s:[%s]\n",PCBA_BACK_CAMERA,PCBA_FAILED,PCBA_FRONT_CAMERA,PCBA_FAILED);
			tc_info->result = -1;
			LOG("%s line=%d %s:[%s] %s:[%s]\n", __FUNCTION__, __LINE__ ,PCBA_BACK_CAMERA,PCBA_FAILED,PCBA_FRONT_CAMERA,PCBA_FAILED);
		}

		if(result_back == 1 && result_front == 1)
		{
			ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] %s:[%s]\n",PCBA_BACK_CAMERA,PCBA_SECCESS,PCBA_FRONT_CAMERA,PCBA_SECCESS);
			tc_info->result = 0;
			LOG("%s line=%d %s:[%s] %s:[%s]\n", __FUNCTION__, __LINE__ ,PCBA_BACK_CAMERA,PCBA_SECCESS,PCBA_FRONT_CAMERA,PCBA_SECCESS);
		}
	}

	lock = 0;
	pthread_cond_signal(&cond);  
    pthread_mutex_unlock(&mutex); 

	return 0;
}

void * camera_test(void *argc)
{ 
	int ret,num;
	tc_info = (struct testcase_info*)argc;
	int arg = 1; 

	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	y = tc_info->y;
	
	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s..] \n",PCBA_CAMERA,PCBA_TESTING);

	pthread_create(&camera_tid, NULL, startCameraTest, NULL); 
		
	return argc;
}

