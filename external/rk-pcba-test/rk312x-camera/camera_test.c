#include <hardware/rga.h>
#include "camera_test.h"
#include "../minuitwrp/minui.h"
#include "../test_case.h"
#define VIDEO_DEV_NAME   "/dev/video0"
#define PMEM_DEV_NAME    "/dev/pmem_cam"
#define DISP_DEV_NAME    "/dev/graphics/fb1"
#define ION_DEVICE          "/dev/ion"

#define FBIOSET_ENABLE			0x5019	


#define CAM_OVERLAY_BUF_NEW  1
#define RK29_CAM_VERSION_CODE_1 KERNEL_VERSION(0, 0, 1)
#define RK29_CAM_VERSION_CODE_2 KERNEL_VERSION(0, 0, 2)

static void *m_v4l2Buffer[4];
static void *m_v4l2buffer_display[4];
static int v4l2Buffer_phy_addr[4] = {0};
static int v4l2Buffer_phy_addr_display;
static int SharedFd_display[4];

static int iCamFd = 0, iDispFd =-1;
static int preview_w,preview_h;

static char videodevice[20] ={0};
static struct v4l2_capability mCamDriverCapability;
static unsigned int pix_format;

static void* vaddr = NULL;
static volatile int isstoped = 0;
static int hasstoped = 1;
enum {
	FD_INIT = -1,
};

static int iIonFd = -1;
struct ion_allocation_data ionAllocData;
struct ion_fd_data fd_data;
struct ion_handle_data handle_data;

static int iIonFd_display = -1;
struct ion_allocation_data ionAllocData_display;//yzm
struct ion_fd_data fd_data_display;
struct ion_handle_data handle_data_display;

struct ion_phys_data  phys_data;
struct ion_custom_data data;

struct ion_phys_data  phys_data_display;
struct ion_custom_data data_display;

#define RK30_PLAT 1
#define RK29_PLAT 0
static int is_rk30_plat = RK30_PLAT;
#define  FB_NONSTAND ((is_rk30_plat == RK29_PLAT)?0x2:0x20)
static int cam_id = 0;

static int camera_x=0,camera_y=0,camera_w=0,camera_h=0,camera_num=0;
static struct testcase_info *tc_info = NULL;

pthread_t camera_tid;

/*add yzm*/
static unsigned int mFrameSizesEnumTable[][2] = {
    {176,144},
    {320,240},
    {352,288},
    {640,480},
    {720,480},
    {800,600},
    {1024,768},
    {1280,720},
    {1280,960},
    {1600,1200},
    {2048,1536},
    {2592,1944},
    {0,0}
};
static unsigned int driver_support_size[12][2] = {0x0};
#define false 0
#define true 1
int get_camera_size();
static int arm_camera_yuv420_scale_arm(int v4l2_fmt_src, int v4l2_fmt_dst, 
									char *srcbuf, char *dstbuf,int src_w, int src_h,int dst_w, int dst_h,int mirror,int zoom_val);

static int rga_nv12_scale_crop(int src_width, int src_height, char *src, short int *dst, int dstbuf_width,int dst_width,int dst_height,int zoom_val,int mirror,int isNeedCrop,int isDstNV21);
 
int Camera_Click_Event(int x,int y)
{	
	struct list_head *pos;
	int x_start,x_end;
	int y_start,y_end;
	int err;

	if(tc_info == NULL)
		return -1;		

	if(camera_num < 2)
		return -1;
	
	get_camera_size();

	x_start = camera_x;
	x_end = x_start + camera_w;
	y_start = camera_y;
	y_end = y_start + camera_h;
	if( (x >= x_start) && (x <= x_end) && (y >= y_start) && (y <= y_end))
	{
		
		printf("Camera_Click_Event : change \r\n");	
		if(isstoped != 0)
			return 0;
		stopCameraTest();
		usleep(100000);
		pthread_create(&camera_tid, NULL, startCameraTest, NULL); 
	}
		
	return 0;
	
}


int CameraCreate(void)
{
    int err,size,i;
	struct v4l2_format format;

    if (iCamFd == 0) {
        iCamFd = open(videodevice, O_RDWR|O_CLOEXEC);
       
        if (iCamFd < 0) {
            printf(" Could not open the camera  device:%s\n",videodevice);
    		err = -1;
            goto exit;
        }
		printf("open the camera	device:%s success\n",videodevice);

        memset(&mCamDriverCapability, 0, sizeof(struct v4l2_capability));
        err = ioctl(iCamFd, VIDIOC_QUERYCAP, &mCamDriverCapability);
        if (err < 0) {
        	printf("Error opening device unable to query device.\n");
    	    goto exit;
        } 
		
		if(strstr((char*)&mCamDriverCapability.card[0], "front") != NULL){
			printf("it is a front camera \n!");
			}
		else if(strstr((char*)&mCamDriverCapability.card[0], "back") != NULL){
			printf("it is a back camera \n!"); 
		}
		else{
			printf("it is a usb camera \n!");
			}
		
        if (mCamDriverCapability.version == RK29_CAM_VERSION_CODE_1) {
            pix_format = V4L2_PIX_FMT_YUV420;
            printf("Current camera driver version: 0.0.1 \n");    
        } else 
        { 
            pix_format = V4L2_PIX_FMT_NV12;
            printf("Current camera driver version: %d.%d.%d \n",                
                (mCamDriverCapability.version>>16) & 0xff,(mCamDriverCapability.version>>8) & 0xff,
                mCamDriverCapability.version & 0xff); 
        }
        
    }
    if(access("/sys/module/rk29_camera_oneframe", O_RDWR) >=0 ){
        is_rk30_plat =  RK29_PLAT;
        printf("it is rk29 platform!\n");
    }else if(access("/sys/module/rk30_camera_oneframe", O_RDWR) >=0){
        printf("it is rk30 platform!\n");
    }else{
        printf("default as rk30 platform\n");
    }
    if(v4l2Buffer_phy_addr[0] !=0 || v4l2Buffer_phy_addr[1] !=0 || v4l2Buffer_phy_addr[2] !=0 || v4l2Buffer_phy_addr[3] !=0 )
		goto suc_alloc;
    if(access(PMEM_DEV_NAME, O_RDWR) < 0) {
        iIonFd = open(ION_DEVICE, O_RDONLY|O_CLOEXEC);

        if(iIonFd < 0 ) {
            printf("%s: Failed to open ion device - %s",
                    __FUNCTION__, strerror(errno));
            iIonFd = -1;
    		err = -1;
            goto exit1;
        }
		for(i = 0;i < 4;i++)
		{
	        ionAllocData.len = 0x100000;
	        ionAllocData.align = 4*1024;
	        ionAllocData.heap_id_mask = ION_HEAP(ION_CMA_HEAP_ID);
			ionAllocData.flags = 0;

	          err = ioctl(iIonFd, ION_IOC_ALLOC, &ionAllocData);
	        if(err) {
	            printf("%s: ION_IOC_ALLOC failed to alloc 0x%x bytes with error - %s\n", 
	    			__FUNCTION__, ionAllocData.len, strerror(errno));
	            
	    		err = -errno;
	            goto exit2;
	        }

	        fd_data.handle = ionAllocData.handle;
	        handle_data.handle = ionAllocData.handle;

	        err = ioctl(iIonFd, ION_IOC_MAP, &fd_data);
	        if(err) {
	            printf("%s: ION_IOC_MAP failed with error - %s",
	                    __FUNCTION__, strerror(errno));
	            ioctl(iIonFd, ION_IOC_FREE, &handle_data);
	    		err = -errno;
	           goto exit2;
	        }
				
	        m_v4l2Buffer[i] = mmap(0, ionAllocData.len, PROT_READ|PROT_WRITE,
	                MAP_SHARED, fd_data.fd, 0);
	        if(m_v4l2Buffer[i] == MAP_FAILED) {
	            printf("%s: Failed to map the allocated memory: %s",
	                    __FUNCTION__, strerror(errno));
	            err = -errno;
	            ioctl(iIonFd, ION_IOC_FREE, &handle_data);
	            goto exit2;
	        }
			memset(m_v4l2Buffer[i], 0x00, ionAllocData.len);

	    	//err = ioctl(fd_data.fd, PMEM_GET_PHYS, &sub);yzm
	    	phys_data.handle = ionAllocData.handle;
			phys_data.phys = 0;
	    	data.cmd = ION_IOC_GET_PHYS;
			data.arg = (unsigned long)&phys_data;
	    	err = ioctl(iIonFd, ION_IOC_CUSTOM, &data);
	    	if (err < 0) {
	        	printf(" ion get phys_data fail !!!!\n");
	            ioctl(iIonFd, ION_IOC_FREE, &handle_data);
	            goto exit2;
	    	}
	          err = ioctl(iIonFd, ION_IOC_FREE, &handle_data);
	    	if(err){
	    		printf("%s: ION_IOC_FREE failed with error - %s",
	                    __FUNCTION__, strerror(errno));
	    		err = -errno;
	    	}else
	        	printf("%s: Successfully allocated 0x%x bytes, mIonFd=%d, SharedFd=%d\n",
	        			__FUNCTION__,ionAllocData.len, iIonFd, fd_data.fd);

			v4l2Buffer_phy_addr[i] = phys_data.phys;
		}
		/*display_buf*************yzm*/
		for(i = 0 ;i < 4 ;i++)
		{
			ionAllocData_display.len = 0x100000;
			ionAllocData_display.align = 4*1024;
			ionAllocData_display.heap_id_mask = ION_HEAP(ION_CMA_HEAP_ID);
			ionAllocData_display.flags = 0;
			
			  err = ioctl(iIonFd, ION_IOC_ALLOC, &ionAllocData_display);
			if(err) {
				printf("%s: ION_IOC_ALLOC failed to alloc 0x%x bytes with error - %s\n", 
					__FUNCTION__, ionAllocData_display.len, strerror(errno));
				
				err = -errno;
				goto exit2;
			}
			
			fd_data_display.handle = ionAllocData_display.handle;
			handle_data_display.handle = ionAllocData_display.handle;
			
			err = ioctl(iIonFd, ION_IOC_MAP, &fd_data_display);
			if(err) {
				printf("%s: ION_IOC_MAP failed with error - %s",
						__FUNCTION__, strerror(errno));
				ioctl(iIonFd, ION_IOC_FREE, &handle_data_display);
				err = -errno;
			   goto exit2;
			}
		
			m_v4l2buffer_display[i] = mmap(0, ionAllocData_display.len, PROT_READ|PROT_WRITE,
					MAP_SHARED, fd_data_display.fd, 0);
			if(m_v4l2buffer_display[i] == MAP_FAILED) {
				printf("%s: Failed to map the allocated memory: %s",
						__FUNCTION__, strerror(errno));
				err = -errno;
				ioctl(iIonFd, ION_IOC_FREE, &handle_data_display);
				goto exit2;
			}
			memset(m_v4l2buffer_display[i], 0x00, ionAllocData_display.len);
			SharedFd_display[i] = fd_data_display.fd;
			/*//err = ioctl(fd_data.fd, PMEM_GET_PHYS, &sub);yzm
			phys_data_display.handle = ionAllocData_display.handle;
			phys_data_display.phys = 0;
			data_display.cmd = ION_IOC_GET_PHYS;
			data_display.arg = (unsigned long)&phys_data_display;
			err = ioctl(iIonFd, ION_IOC_CUSTOM, &data_display);
			if (err < 0) {
				printf(" ion get phys_data_display fail !!!!\n");
				ioctl(iIonFd, ION_IOC_FREE, &handle_data_display);
				goto exit2;
			}*/
			err = ioctl(iIonFd, ION_IOC_FREE, &handle_data_display);
			if(err){
				printf("%s: ION_IOC_FREE failed with error - %s",
						__FUNCTION__, strerror(errno));
				err = -errno;
			}else
				printf("%s: Successfully allocated 0x%x bytes, mIonFd=%d, SharedFd_display=%d\n",
						__FUNCTION__,ionAllocData_display.len, iIonFd, fd_data_display.fd);
		
		}
	}

	//v4l2Buffer_phy_addr = phys_data.phys;
	//v4l2Buffer_phy_addr_display = phys_data_display.phys;
suc_alloc:   
          err = ioctl(iCamFd, VIDIOC_QUERYCAP, &mCamDriverCapability);
        if (err < 0) {
        	printf("Error opening device unable to query device.\n");
    	    goto exit;
        }  
    return 0;

exit3:
	
	for(i=0;i<4;i++){
		munmap(m_v4l2Buffer[i], ionAllocData.len);
		munmap(m_v4l2buffer_display[i], ionAllocData_display.len);
	}
exit2:

    if(iIonFd > 0){
    	close(iIonFd);
    	iIonFd = -1;
        }
exit1:
exit:
    return err;
}

int selectPreferedDrvSize(int *width,int * height,int driver_support_fmt_num)
{
    int num = driver_support_fmt_num;
    int ori_w = *width,ori_h = *height,pref_w=0,pref_h=0;
    int demand_ratio = (*width * 100) / (*height);
    int32_t total_pix = (*width) * (*height);

    int cur_ratio ,cur_ratio_diff,pref_ratio_diff = 10000;
    int32_t cur_pix ,cur_pix_diff,pref_pix_diff = 8*1000*1000;
	int i = 0;
    //serch for the preferred res
    for(i =0;i<num;i++){
        cur_ratio = driver_support_size[i][0] * 100 / driver_support_size[i][1];
        cur_pix   = driver_support_size[i][0] * driver_support_size[i][1];
        cur_pix_diff = ((total_pix - cur_pix)>0)?(total_pix - cur_pix):(cur_pix - total_pix);
        cur_ratio_diff = ((demand_ratio - cur_ratio)>0)?(demand_ratio - cur_ratio):(cur_ratio - demand_ratio);
        //
        if((cur_pix_diff < pref_pix_diff) && (cur_ratio_diff <=  pref_ratio_diff)){
            pref_pix_diff = cur_pix_diff;
            cur_ratio_diff = pref_ratio_diff;
            pref_w = driver_support_size[i][0];
            pref_h = driver_support_size[i][1];
        }
    }
    if(pref_w != 0){
        *width = pref_w;
        *height = pref_h;
        printf("%s:prefer res (%dx%d)",__FUNCTION__,pref_w,pref_h);
    }else{
        printf("WARINING:have not select preferred res!!");
    }
    return 0;
}


int CameraStart(int phy_addr[4], int buffer_count, int w, int h)
{
    int err,i = 0;
    int nSizeBytes;
    struct v4l2_format format;
    enum v4l2_buf_type type;
    struct v4l2_requestbuffers creqbuf;
	/*add yzm*/
	struct v4l2_format fmt;
	unsigned int mCamDriverFrmWidthMax = 0,mCamDriverFrmHeightMax = 0;
	int driver_support_fmt_num = 0;
		
	//buffer_count = 2;
	if( phy_addr == 0 || buffer_count == 0  ) {
    	printf(" Video Buf is NULL\n");
		goto  fail_bufalloc;
    }


	/* Try preview format */		
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat= pix_format;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;


	/*picture size setting*/
	fmt.fmt.pix.width = 10000;
	fmt.fmt.pix.height = 10000;
	err = ioctl(iCamFd, VIDIOC_TRY_FMT, &fmt);

	mCamDriverFrmWidthMax = fmt.fmt.pix.width;
	mCamDriverFrmHeightMax = fmt.fmt.pix.height;		

	if (mCamDriverFrmWidthMax > 3264) {
		printf("Camera driver support maximum resolution(%dx%d) is overflow 8Mega!",mCamDriverFrmWidthMax,mCamDriverFrmHeightMax);
		mCamDriverFrmWidthMax = 3264;
		mCamDriverFrmHeightMax = 2448;
	}

	/*preview size setting*/
    while(mFrameSizesEnumTable[i][0]){
        if (mCamDriverFrmWidthMax >= mFrameSizesEnumTable[i][0]) {
            fmt.fmt.pix.width = mFrameSizesEnumTable[i][0];
            fmt.fmt.pix.height = mFrameSizesEnumTable[i][1];
            if (ioctl(iCamFd, VIDIOC_TRY_FMT, &fmt) == 0) {
                if ((fmt.fmt.pix.width == mFrameSizesEnumTable[i][0]) && (fmt.fmt.pix.height == mFrameSizesEnumTable[i][1])) {
                    driver_support_size[driver_support_fmt_num][0] = mFrameSizesEnumTable[i][0];
                    driver_support_size[driver_support_fmt_num][1] = mFrameSizesEnumTable[i][1];
					printf("wXy = %dX%d\n",driver_support_size[driver_support_fmt_num][0],driver_support_size[driver_support_fmt_num][1]);
					driver_support_fmt_num++;
                }
            }
        }
        i++;
    }
	err = selectPreferedDrvSize(&w,&h,driver_support_fmt_num);
	

	/* Set preview format */
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = w;
	format.fmt.pix.height = h;
	format.fmt.pix.pixelformat = pix_format;
	format.fmt.pix.field = V4L2_FIELD_NONE;	
	err = ioctl(iCamFd, VIDIOC_S_FMT, &format);
	if ( err < 0 ){
		printf(" Failed to set VIDIOC_S_FMT\n");
		goto exit1;
	}

	preview_w = format.fmt.pix.width;
	preview_h = format.fmt.pix.height;	
	creqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    creqbuf.memory = V4L2_MEMORY_OVERLAY;
    creqbuf.count  =  buffer_count /*- 1*/ ; //We will use the last buffer for snapshots.
    if (ioctl(iCamFd, VIDIOC_REQBUFS, &creqbuf) < 0) {
        printf("%s VIDIOC_REQBUFS Failed\n",__FUNCTION__);
        goto fail_reqbufs;
    }
	printf("creqbuf.count = %d\n",creqbuf.count);
    for (i = 0; i < (int)creqbuf.count; i++) {

        struct v4l2_buffer buffer;
        buffer.type = creqbuf.type;
        buffer.memory = creqbuf.memory;
        buffer.index = i;

        if (ioctl(iCamFd, VIDIOC_QUERYBUF, &buffer) < 0) {
            printf("%s VIDIOC_QUERYBUF Failed\n",__FUNCTION__);
            goto fail_loop;
        }

        #if CAM_OVERLAY_BUF_NEW
        //buffer.m.offset = phy_addr + i*buffer.length;
		buffer.m.offset = phy_addr[i];
        #else
        buffer.m.offset = phy_addr;
        #endif

        err = ioctl(iCamFd, VIDIOC_QBUF, &buffer);
        if (err < 0) {
            printf("%s CameraStart VIDIOC_QBUF Failed\n",__FUNCTION__);
            goto fail_loop;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    err = ioctl(iCamFd, VIDIOC_STREAMON, &type);
    if ( err < 0) {
        printf("%s VIDIOC_STREAMON Failed\n",__FUNCTION__);
        goto fail_loop;
    }

    return 0;

fail_bufalloc:
fail_loop:
fail_reqbufs:

exit1:
    close(iCamFd);
	iCamFd = -1;
exit:
    return -1;
}



int DispCreate(int corx ,int cory,int preview_w,int preview_h )
{
	int err = 0;
	struct fb_var_screeninfo var;
	unsigned int panelsize[2];
	int x_phy,y_phy,w_phy,h_phy;
	int x_visual,y_visual,w_visual,h_visual;
	struct fb_fix_screeninfo finfo;
	struct color_key_cfg clr_key_cfg;

	int data[2];
	
	if(iDispFd !=-1)
		goto exit;
	iDispFd = open(DISP_DEV_NAME,O_RDWR, 0);
	if (iDispFd < 0) {
		printf("%s Could not open display device\n",__FUNCTION__);
		err = -1;
		goto exit;
	}
	if(ioctl(iDispFd, 0x5001, panelsize) < 0)
	{
		printf("%s Failed to get panel size\n",__FUNCTION__);
		err = -1;
		goto exit1;
	}
	if(panelsize[0] == 0 || panelsize[1] ==0)
	{
		panelsize[0] = preview_w;
		panelsize[1] = preview_h;
	}
	#if 0
	data[0] = v4l2Buffer_phy_addr;
	data[1] = (int)(data[0] + preview_w *preview_h);
	if (ioctl(iDispFd, 0x5002, data) == -1) 
	{
		printf("%s ioctl fb1 queuebuf fail!\n",__FUNCTION__);
		err = -1;
		goto exit;
	}
	#endif
	if (ioctl(iDispFd, FBIOGET_VSCREENINFO, &var) == -1) {
		printf("%s ioctl fb1 FBIOPUT_VSCREENINFO fail!\n",__FUNCTION__);
		err = -1;
		goto exit;
	}
	var.xoffset = 0;   //win0 start x in memery
	var.yoffset = 0;   //win0 start y in memery
	var.nonstd = ((cory<<20)&0xfff00000) + ((corx<<8)&0xfff00) +FB_NONSTAND; //win0 ypos & xpos & format (ypos<<20 + xpos<<8 + format)
	var.grayscale = ((preview_h<<20)&0xfff00000) + (( preview_w<<8)&0xfff00) + 0;	//win0 xsize & ysize
	var.xres = preview_w;	 //win0 show x size
	var.yres = preview_h;	 //win0 show y size
	var.xres_virtual = preview_w;
	var.yres_virtual = preview_h;
	var.bits_per_pixel = 16;
	var.activate = FB_ACTIVATE_FORCE;
	if (ioctl(iDispFd, FBIOPUT_VSCREENINFO, &var) == -1) {
		printf("%s ioctl fb1 FBIOPUT_VSCREENINFO fail!\n",__FUNCTION__);
		err = -1;
		goto exit;
	}
	
	clr_key_cfg.win0_color_key_cfg = 0;		//win0 color key disable
	clr_key_cfg.win1_color_key_cfg = 0x01000000; 	// win1 color key enable
	clr_key_cfg.win2_color_key_cfg = 0;  
	if (ioctl(iDispFd,RK_FBIOPUT_COLOR_KEY_CFG, &clr_key_cfg) == -1) {
                printf("%s set fb color key failed!\n",__FUNCTION__);
                err = -1;
        }

	return 0;
exit1:
	if (iDispFd > 0)
	{
		close(iDispFd);
		iDispFd = -1;
	}
exit:
	return err;
}
int TaskStop(void)
{
	int i = 0;
	struct v4l2_requestbuffers creqbuf;
    creqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    isstoped =1;
    while(!hasstoped){
    	sleep(1);
    	}
	
	if (iCamFd > 0) {
	    if (ioctl(iCamFd, VIDIOC_STREAMOFF, &creqbuf.type) == -1) {
	        printf("%s VIDIOC_STREAMOFF Failed\n", __FUNCTION__);
	   //     return -1;
	    }		
	}
	if (iDispFd > 0) {
		int disable = 0;
		printf("Close disp\n");
		ioctl(iDispFd, FBIOSET_ENABLE,&disable);
		close(iDispFd);
		iDispFd = -1;
	}
	if (iCamFd > 0) {
		close(iCamFd);
		iCamFd = 0;
	}
	for(i = 0; i < 4; i++){
		memset(m_v4l2buffer_display[i], 0x00, camera_w*camera_h);
		//memset(m_v4l2buffer_display[i]+camera_w*camera_h, 0xff,
			//ionAllocData_display.len - camera_w*camera_h);
	}
	printf("\n%s: stop ok!\n",__func__);
	return 0;
}
int TaskRuning(int fps_total,int corx,int cory)
{
	int err,fps;
	int data[2];
	struct v4l2_buffer cfilledbuffer1;
	int i ;
	struct fb_var_screeninfo var ;
	int fb_offset = 0;
	cfilledbuffer1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cfilledbuffer1.memory = V4L2_MEMORY_OVERLAY;
	cfilledbuffer1.reserved = 0;
	fps = 0;

	while (!isstoped)
	{
		if (ioctl(iCamFd, VIDIOC_DQBUF, &cfilledbuffer1) < 0)
		{
			printf("%s VIDIOC_DQBUF Failed!!! \n",__FUNCTION__);
			err = -1;
			goto exit;
		}	

#if 1		
		err = arm_camera_yuv420_scale_arm(V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV12, 
			(char*)(m_v4l2Buffer[cfilledbuffer1.index]), (char*)m_v4l2buffer_display[cfilledbuffer1.index],
			preview_w, preview_h,
			camera_w, camera_h,
			false,0);
#else
		err = rga_nv12_scale_crop(preview_w, preview_h, 
			(char*)(m_v4l2Buffer[cfilledbuffer1.index]), (char*)m_v4l2buffer_display[cfilledbuffer1.index],
			camera_w,
			camera_w, camera_h,
			0,false,true,false);
#endif
#if 0 
		{
			static int writeframe = 0;
			//write file
			if((writeframe++ > 5) && (writeframe%3 == 0) && (writeframe < 100)){
				FILE* fp =NULL;
				char filename[40];
			
				filename[0] = 0x00;
				//sprintf(filename, "/raw8_%dx%d_%d.raw",camera_w,camera_h,writeframe);
				sprintf(filename, "/raw8_%dx%d_%d.raw",preview_w,preview_h,writeframe);
				fp = fopen(filename, "wb+");
				if (fp > 0) {
					fwrite((char*)m_v4l2Buffer[cfilledbuffer1.index], 1,preview_w*preview_h*3/2,fp);
					//fwrite((char*)m_v4l2buffer_display[cfilledbuffer1.index], 1,camera_w*camera_h*3/2,fp);
					//	fwrite((char*)uv_addr_vir, 1,width*height*3/2,fp); //yuv422
			
					fclose(fp);
					printf("Write success yuv data to %s\n",filename);
				} else {
					printf("Create %s failed(%d, %s)",filename,fp, strerror(errno));
				}
			}
		}
#endif
		if (iDispFd > 0) 
		{	
			#if CAM_OVERLAY_BUF_NEW
			//data[0] = (int)fd_data_display.fd;
			data[0] = SharedFd_display[cfilledbuffer1.index];
			//printf("fd_data_display.fd[%d] = %d\n",cfilledbuffer1.index,SharedFd_display[cfilledbuffer1.index]);
			#else
			data[0] = (int)cfilledbuffer1.m.offset + cfilledbuffer1.index * cfilledbuffer1.length;
			#endif
			data[1] = (int)(data[0] + camera_w *camera_h);
			if (ioctl(iDispFd, 0x5002, data) == -1) {
			       printf("%s ioctl fb1 queuebuf fail!\n",__FUNCTION__);
			       err = -1;
				goto exit;
			}
			if (ioctl(iDispFd, FBIOGET_VSCREENINFO, &var) == -1) {
				printf("%s ioctl fb1 FBIOPUT_VSCREENINFO fail!\n",__FUNCTION__);
				err = -1;
				goto exit;
			}
			
			
			var.xres_virtual = camera_w;	//win0 memery x size
			var.yres_virtual = camera_h;	 //win0 memery y size
			var.xoffset = 0;   //win0 start x in memery
			var.yoffset = 0;   //win0 start y in memery
			var.nonstd = ((cory<<20)&0xfff00000) + (( corx<<8)&0xfff00) +FB_NONSTAND;
			
			var.grayscale = ((camera_h<<20)&0xfff00000) + (( camera_w<<8)&0xfff00) + 0;   //win0 xsize & ysize
			var.xres = camera_w;	 //win0 show x size
			var.yres = camera_h;	 //win0 show y size
			var.bits_per_pixel = 16;
			var.activate = FB_ACTIVATE_FORCE;
			if (ioctl(iDispFd, FBIOPUT_VSCREENINFO, &var) == -1) {
				printf("%s ioctl fb1 FBIOPUT_VSCREENINFO fail!\n",__FUNCTION__);
				err = -1;
				goto exit;
			}
			
			if (ioctl(iDispFd,RK_FBIOSET_CONFIG_DONE, NULL) < 0) {
        			perror("set config done failed");
    			}
			

		}
		
		
	if (ioctl(iCamFd, VIDIOC_QBUF, &cfilledbuffer1) < 0) {
		printf("%s VIDIOC_QBUF Failed!!!\n",__FUNCTION__);
		err = -1;
		goto exit;
	}

	    fps++;
	}
//	hasstoped = 1;

exit:
	return err;
}
// the func is a while loop func , MUST  run in a single thread.
int startCameraTest(){
	int ret = 0;
	int cameraId = 0;
	int preWidth;
	int preHeight;
	int corx ;
	int cory;

	get_camera_size();
	
	if(iCamFd > 0){
		printf(" %s has been opened! can't switch camera!\n",videodevice);
		return -1;
	}

	isstoped = 0;
	hasstoped = 0;
	cameraId = cam_id%2;
	cam_id++;
	preWidth = camera_w;
	preHeight = camera_h;
	corx = camera_x;
	cory = camera_y;
	sprintf(videodevice,"/dev/video%d",cameraId);
	preview_w = preWidth;
	preview_h = preHeight;
	printf("start test camera %d ....\n",cameraId);
	
    if(access(videodevice, O_RDWR) <0 ){
	   printf("access %s failed\n",videodevice);
	   hasstoped = 1;
	   return -1;
     }
	  
	if (CameraCreate() == 0)
	{
		if (CameraStart(v4l2Buffer_phy_addr, 4, preview_w,preview_h) == 0)
		{
			if (DispCreate(corx ,cory,preWidth,preHeight) == 0)
			{
				TaskRuning(1,corx,cory);
			}
			else
			{
				tc_info->result = -1;
				printf("%s display create wrong!\n",__FUNCTION__);
			}
		}
		else
		{
			tc_info->result = -1;
			printf("%s camera start erro\n",__FUNCTION__);
		}
	}
	else
	{
		tc_info->result = -1;
		printf("%s camera create erro\n",__FUNCTION__);
	}
	//isstoped = 1;
	hasstoped = 1;
	printf("camrea%d test over\n",cameraId);
	return 0;
}

int stopCameraTest(){
	
	sprintf(videodevice,"/dev/video%d",(cam_id%2));
	if(access(videodevice, O_RDWR) <0 ){
	   printf("access %s failed,so dont't switch to camera %d\n",videodevice,(cam_id%2));
	   //recover videodevice
	   sprintf(videodevice,"/dev/video%d",(1-(cam_id%2)));
	   return 0;
	 }
	printf("%s enter stop -----\n",__func__);
	return TaskStop();
}
void finishCameraTest(){
		int i = 0;
		TaskStop();
	
		if(iIonFd > 0){
			
			for(i=0;i<4;i++){
				munmap(m_v4l2Buffer[i], ionAllocData.len);
				munmap(m_v4l2buffer_display[i], ionAllocData_display.len);
			}
			close(iIonFd);
			iIonFd = -1;
			}
		if (iDispFd > 0) {
			int disable = 0;
			printf("Close disp\n");
			ioctl(iDispFd, FBIOSET_ENABLE,&disable);
			close(iDispFd);
			iDispFd = -1;
		}
		printf("%s finishCameraTest out -----\n",__func__);
		
}

int get_camera_size()
{
	if(camera_x>0 && camera_y>0 && camera_w>0 && camera_h >0)
		return 0;	

	if(gr_fb_width() > gr_fb_height()){
		camera_w = ((gr_fb_width() >> 1) & ~0x03);//camera_msg->w;
		camera_h = ((gr_fb_height()*2/3) & ~0x03);// camera_msg->h;
	}
	else{
		camera_h = ((gr_fb_width() >> 1) & ~0x03);//camera_msg->w;
		camera_w = ((gr_fb_height()*2/3) & ~0x03);// camera_msg->h;
	}

	if(camera_w > 640)
		camera_w = 640;
	if(camera_h > 480)
		camera_h=480;			
	
	camera_x = gr_fb_width() >> 1; 	
	camera_y = 0;

	printf("camera_w = %d,camera_h = %d\ncamera_x = %d,camera_y = %d\n"
		,camera_w,camera_h,camera_x,camera_y);
	return 0;
}


void * camera_test(void *argc)
{
	int ret,num;

	tc_info = (struct testcase_info *)argc; 

	if (script_fetch("camera", "number",&num, 1) == 0) {
		printf("camera_test num:%d\r\n",num);
		camera_num = num;	
	}

	pthread_create(&camera_tid, NULL, startCameraTest, NULL); 
		
	return argc;
}



static int arm_camera_yuv420_scale_arm(int v4l2_fmt_src, int v4l2_fmt_dst, 
									char *srcbuf, char *dstbuf,int src_w, int src_h,int dst_w, int dst_h,int mirror,int zoom_val)
{
	unsigned char *psY,*pdY,*psUV,*pdUV; 
	unsigned char *src,*dst;
	int srcW,srcH,cropW,cropH,dstW,dstH;
	long zoomindstxIntInv,zoomindstyIntInv;
	long x,y;
	long yCoeff00,yCoeff01,xCoeff00,xCoeff01;
	long sX,sY;
	long r0,r1,a,b,c,d;
	int ret = 0;
	//bool nv21DstFmt = false;
	int nv21DstFmt = false;
	int ratio = 0;
	int top_offset=0,left_offset=0;
	if((v4l2_fmt_src != V4L2_PIX_FMT_NV12) ||
		((v4l2_fmt_dst != V4L2_PIX_FMT_NV12) && (v4l2_fmt_dst != V4L2_PIX_FMT_NV21) )){
		printf("%s:%d,not suppport this format ",__FUNCTION__,__LINE__);
		return -1;
	}

    //just copy ?
    if((v4l2_fmt_src == v4l2_fmt_dst) && (mirror == false)
        &&(src_w == dst_w) && (src_h == dst_h) && (zoom_val == 100)){
        memcpy(dstbuf,srcbuf,src_w*src_h*3/2);
        return 0;
    }/*else if((v4l2_fmt_dst == V4L2_PIX_FMT_NV21) 
            && (src_w == dst_w) && (src_h == dst_h) 
            && (mirror == false) && (zoom_val == 100)){
    //just convert fmt

        cameraFormatConvert(V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV21, NULL, 
    					    srcbuf, dstbuf,0,0,src_w*src_h*3/2,
    					    src_w, src_h,src_w,
    					    dst_w, dst_h,dst_w,
    						mirror);
        return 0;

    }*/

	if ((v4l2_fmt_dst == V4L2_PIX_FMT_NV21)){
		nv21DstFmt = true;
		
	}

	//need crop ?
	if((src_w*100/src_h) != (dst_w*100/dst_h)){
		ratio = ((src_w*100/dst_w) >= (src_h*100/dst_h))?(src_h*100/dst_h):(src_w*100/dst_w);
		cropW = ratio*dst_w/100;
		cropH = ratio*dst_h/100;
		
		left_offset=((src_w-cropW)>>1) & (~0x01);
		top_offset=((src_h-cropH)>>1) & (~0x01);
	}else{
		cropW = src_w;
		cropH = src_h;
		top_offset=0;
		left_offset=0;
	}

    //zoom ?
    if(zoom_val > 100){
        cropW = cropW*100/zoom_val;
        cropH = cropH*100/zoom_val;
		left_offset=((src_w-cropW)>>1) & (~0x01);
		top_offset=((src_h-cropH)>>1) & (~0x01);
    }

	src = psY = (unsigned char*)(srcbuf)+top_offset*src_w+left_offset;
	//psUV = psY +src_w*src_h+top_offset*src_w/2+left_offset;
	psUV = (unsigned char*)(srcbuf) +src_w*src_h+top_offset*src_w/2+left_offset;

	
	srcW =src_w;
	srcH = src_h;
//	cropW = src_w;
//	cropH = src_h;

	
	dst = pdY = (unsigned char*)dstbuf; 
	pdUV = pdY + dst_w*dst_h;
	dstW = dst_w;
	dstH = dst_h;

	zoomindstxIntInv = ((unsigned long)(cropW)<<16)/dstW + 1;
	zoomindstyIntInv = ((unsigned long)(cropH)<<16)/dstH + 1;
	//y
	//for(y = 0; y<dstH - 1 ; y++ ) {	
	for(y = 0; y<dstH; y++ ) {	 
		yCoeff00 = (y*zoomindstyIntInv)&0xffff;
		yCoeff01 = 0xffff - yCoeff00; 
		sY = (y*zoomindstyIntInv >> 16);
		sY = (sY >= srcH - 1)? (srcH - 2) : sY; 	 
		for(x = 0; x<dstW; x++ ) {
			xCoeff00 = (x*zoomindstxIntInv)&0xffff;
			xCoeff01 = 0xffff - xCoeff00;	
			sX = (x*zoomindstxIntInv >> 16);
			sX = (sX >= srcW -1)?(srcW- 2) : sX;
			a = psY[sY*srcW + sX];
			b = psY[sY*srcW + sX + 1];
			c = psY[(sY+1)*srcW + sX];
			d = psY[(sY+1)*srcW + sX + 1];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;
			
			if(mirror)
				pdY[dstW -1 - x] = r0;
			else
				pdY[x] = r0;
		}
		pdY += dstW;
	}

	dstW /= 2;
	dstH /= 2;
	srcW /= 2;
	srcH /= 2;

	//UV
	//for(y = 0; y<dstH - 1 ; y++ ) {
	for(y = 0; y<dstH; y++ ) {
		yCoeff00 = (y*zoomindstyIntInv)&0xffff;
		yCoeff01 = 0xffff - yCoeff00; 
		sY = (y*zoomindstyIntInv >> 16);
		sY = (sY >= srcH -1)? (srcH - 2) : sY;		
		for(x = 0; x<dstW; x++ ) {
			xCoeff00 = (x*zoomindstxIntInv)&0xffff;
			xCoeff01 = 0xffff - xCoeff00;	
			sX = (x*zoomindstxIntInv >> 16);
			sX = (sX >= srcW -1)?(srcW- 2) : sX;
			//U
			a = psUV[(sY*srcW + sX)*2];
			b = psUV[(sY*srcW + sX + 1)*2];
			c = psUV[((sY+1)*srcW + sX)*2];
			d = psUV[((sY+1)*srcW + sX + 1)*2];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;
		
			if(mirror && nv21DstFmt)
				pdUV[dstW*2-1- (x*2)] = r0;
			else if(mirror)
				pdUV[dstW*2-1-(x*2+1)] = r0;
			else if(nv21DstFmt)
				pdUV[x*2 + 1] = r0;
			else
				pdUV[x*2] = r0;
			//V
			a = psUV[(sY*srcW + sX)*2 + 1];
			b = psUV[(sY*srcW + sX + 1)*2 + 1];
			c = psUV[((sY+1)*srcW + sX)*2 + 1];
			d = psUV[((sY+1)*srcW + sX + 1)*2 + 1];

			r0 = (a * xCoeff01 + b * xCoeff00)>>16 ;
			r1 = (c * xCoeff01 + d * xCoeff00)>>16 ;
			r0 = (r0 * yCoeff01 + r1 * yCoeff00)>>16;

			if(mirror && nv21DstFmt)
				pdUV[dstW*2-1- (x*2+1) ] = r0;
			else if(mirror)
				pdUV[dstW*2-1-(x*2)] = r0;
			else if(nv21DstFmt)
				pdUV[x*2] = r0;
			else
				pdUV[x*2 + 1] = r0;
		}
		pdUV += dstW*2;
	}
	return 0;
}	

static int rga_nv12_scale_crop(int src_width, int src_height, char *src, short int *dst, int dstbuf_width,int dst_width,int dst_height,int zoom_val,int mirror,int isNeedCrop,int isDstNV21)
{
    int rgafd = -1,ret = -1;
	/*has something wrong with rga of rk312x mirror operation*/
//#if defined(TARGET_RK312x)
		if(mirror){
			return arm_camera_yuv420_scale_arm(V4L2_PIX_FMT_NV12, (isDstNV21 ? V4L2_PIX_FMT_NV21:V4L2_PIX_FMT_NV12), 
												src, (char *)dst,src_width, src_height,dst_width, dst_height,
												true,zoom_val);
		}
//#endif

    if((rgafd = open("/dev/rga",O_RDWR)) < 0) {
    	printf("%s(%d):open rga device failed!!",__FUNCTION__,__LINE__);
        ret = -1;
    	return ret;
	}

    struct rga_req  Rga_Request;
    int err = 0;

    memset(&Rga_Request,0x0,sizeof(Rga_Request));

	unsigned char *psY, *psUV;
	int srcW,srcH,cropW,cropH;
	int ratio = 0;
	int top_offset=0,left_offset=0;
	//need crop ? when cts FOV,don't crop
	if(isNeedCrop && (src_width*100/src_height) != (dst_width*100/dst_height)){
		ratio = ((src_width*100/dst_width) >= (src_height*100/dst_height))?(src_height*100/dst_height):(src_width*100/dst_width);
		cropW = ratio*dst_width/100;
		cropH = ratio*dst_height/100;
		
		left_offset=((src_width-cropW)>>1) & (~0x01);
		top_offset=((src_height-cropH)>>1) & (~0x01);
	}else{
		cropW = src_width;
		cropH = src_height;
		top_offset=0;
		left_offset=0;
	}

    //zoom ?
    if(zoom_val > 100){
        cropW = cropW*100/zoom_val;
        cropH = cropH*100/zoom_val;
		left_offset=((src_width-cropW)>>1) & (~0x01);
		top_offset=((src_height-cropH)>>1) & (~0x01);
    }
    

	psY = (unsigned char*)(src)/*+top_offset*src_width+left_offset*/;
	//psUV = (unsigned char*)(src) +src_width*src_height+top_offset*src_width/2+left_offset;
	
	Rga_Request.src.yrgb_addr =  0;
    Rga_Request.src.uv_addr  = (int)psY;
    Rga_Request.src.v_addr   =  0;
    Rga_Request.src.vir_w =  src_width;
    Rga_Request.src.vir_h = src_height;
    Rga_Request.src.format = RK_FORMAT_YCbCr_420_SP;
    Rga_Request.src.act_w = cropW;
    Rga_Request.src.act_h = cropH;
    Rga_Request.src.x_offset = left_offset;
    Rga_Request.src.y_offset = top_offset;

    Rga_Request.dst.yrgb_addr = 0;
    Rga_Request.dst.uv_addr  = (int)dst;
    Rga_Request.dst.v_addr   = 0;
    Rga_Request.dst.vir_w = dstbuf_width;
    Rga_Request.dst.vir_h = dst_height;
    if(isDstNV21) 
        Rga_Request.dst.format = RK_FORMAT_YCrCb_420_SP;
    else 
        Rga_Request.dst.format = RK_FORMAT_YCbCr_420_SP;
    Rga_Request.clip.xmin = 0;
    Rga_Request.clip.xmax = dst_width - 1;
    Rga_Request.clip.ymin = 0;
    Rga_Request.clip.ymax = dst_height - 1;
    Rga_Request.dst.act_w = dst_width;
    Rga_Request.dst.act_h = dst_height;
    Rga_Request.dst.x_offset = 0;
    Rga_Request.dst.y_offset = 0;
    Rga_Request.mmu_info.mmu_en    = 1;
    Rga_Request.mmu_info.mmu_flag  = ((2 & 0x3) << 4) | 1 | (1 << 8) | (1 << 10);
    Rga_Request.alpha_rop_flag |= (1 << 5);             /* ddl@rock-chips.com: v0.4.3 */
    
#if defined(TARGET_RK312x)
    /* wrong operation of nv12 to nv21 ,not scale */
	if(1/*(cropW != dst_width) || ( cropH != dst_height)*/){
#else
	if((cropW != dst_width) || ( cropH != dst_height)){
#endif
		Rga_Request.sina = 0;
		Rga_Request.cosa = 0x10000;
		Rga_Request.scale_mode = 1;
    	Rga_Request.rotate_mode = mirror ? 2:1;
	}else{
		Rga_Request.sina = 0;
		Rga_Request.cosa =  0;
		Rga_Request.scale_mode = 0;
    	Rga_Request.rotate_mode = mirror ? 2:0;
		Rga_Request.render_mode = pre_scaling_mode;
	}
	

	if(ioctl(rgafd, RGA_BLIT_SYNC, &Rga_Request) != 0) {
		printf("%s(%d):  RGA_BLIT_ASYNC Failed", __FUNCTION__, __LINE__);
		err = -1;
	}

	close(rgafd);
	return err;
}

