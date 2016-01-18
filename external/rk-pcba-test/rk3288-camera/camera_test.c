#include "camera_test.h"
#include "../minuitwrp/minui.h"
#include "../test_case.h"
#define VIDEO_DEV_NAME   "/dev/video0"
#define PMEM_DEV_NAME    "/dev/pmem_cam"
#define DISP_DEV_NAME    "/dev/graphics/fb1"
#define ION_DEVICE          "/dev/ion"
#define CAMSYS_DEVNAME   "/dev/camsys_marvin"

#define FBIOSET_ENABLE			0x5019	


#define CAM_OVERLAY_BUF_NEW  1
#define RK29_CAM_VERSION_CODE_1 KERNEL_VERSION(0, 0, 1)
#define RK29_CAM_VERSION_CODE_2 KERNEL_VERSION(0, 0, 2)

static void *m_v4l2Buffer[4];
static int v4l2Buffer_phy_addr = 0;
static int iCamFd, iDispFd =-1;
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
struct ion_phys_data  phys_data;
struct ion_custom_data data;
#define RK30_PLAT 1
#define RK29_PLAT 0
static int is_rk30_plat = RK30_PLAT;
#define  FB_NONSTAND ((is_rk30_plat == RK29_PLAT)?0x2:0x20)
static int cam_id = 0;

//hkw add;
static int camsys_fd;
#define HAL_DEVID_EXTERNAL  CAMSYS_DEVID_EXTERNAL 

static int camera_x=0,camera_y=0,camera_w=0,camera_h=0,camera_num=0;
static struct testcase_info *tc_info = NULL;

pthread_t camera_tid;

//hkw add;
int extdev_register()
{
    int err = 0;
    int numLane = 2;
    camsys_devio_name_t extdev;
    
    extdev.dev_id = CAMSYS_DEVID_SENSOR_1B;
    strlcpy((char*)extdev.avdd.name, "NC",sizeof(extdev.avdd.name));
    extdev.avdd.min_uv = 28000000;
    extdev.avdd.max_uv = 28000000;    
    strlcpy((char*)extdev.dovdd.name,"NC",sizeof(extdev.dovdd.name));
    extdev.dovdd.min_uv = 18000000;
    extdev.dovdd.max_uv = 18000000;
    strlcpy((char*)extdev.dvdd.name, "NC",sizeof(extdev.dvdd.name));
    strlcpy((char*)extdev.afvdd.name, "NC",sizeof(extdev.afvdd.name));
    strlcpy((char*)extdev.pwrdn.name, "RK30_PIN2_PB7",sizeof(extdev.pwrdn.name));
    extdev.pwrdn.active = 0x00;
    strlcpy((char*)extdev.pwren.name, "RK30_PIN0_PC1",sizeof(extdev.pwren.name));
    extdev.pwren.active = 0x01;
    strlcpy((char*)extdev.rst.name,"RK30_PIN2_PB6",sizeof(extdev.rst.name));
    extdev.rst.active = 0x0;
    strlcpy((char*)extdev.afpwrdn.name,"NC",sizeof(extdev.afpwrdn.name));
    strlcpy((char*)extdev.afpwr.name,"NC",sizeof(extdev.afpwr.name));
    extdev.phy.type = CamSys_Phy_Mipi;
    //extdev.phy.info.mipi.phy_index = 1; //MIPI_PHY_INDEX;
    extdev.phy.info.mipi.data_en_bit = 0x01; //?
    
    /*if (numLane == 1) {
        extdev.phy.info.mipi.data_en_bit = 0x01;
        extdev.phy.info.mipi.bit_rate = 656;
    } else if (numLane == 2) {
        extdev.phy.info.mipi.data_en_bit = 0x03;
        extdev.phy.info.mipi.bit_rate = 328;
    } else if (numLane == 4) {
        extdev.phy.info.mipi.data_en_bit = 0x0f;
        extdev.phy.info.mipi.bit_rate = 408;
    }*/
    extdev.clk.in_rate = 24000000;
    
		printf("----CAMSYS_REGISTER_DEVIO----\n");
    err = ioctl(camsys_fd, CAMSYS_REGISTER_DEVIO, &extdev);
    if (err<0) {
        printf("CAMSYS_REGISTER_DEVIO failed\n");
    }

    return err;

}

//int extdev_init(unsigned int *i2cbase)
int extdev_init()
{
    int err,i2cbytes,i;
    struct rk_sensor_reg *sensor_reg;
    unsigned char *i2cchar;
    camsys_sysctrl_t sysctl;    
    camsys_i2c_info_t i2cinfo;
    int id = 0;
    
    if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();
    
    sysctl.dev_mask = (CAMSYS_DEVID_SENSOR_1B & HAL_DEVID_EXTERNAL);
    sysctl.ops = CamSys_Avdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_Avdd on failed!");
    }
    
    sysctl.ops = CamSys_Dvdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_Dvdd on failed!\n");
    }
    
    sysctl.ops = CamSys_Dovdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_Dovdd on failed!");
    }
    usleep(5000);
    		
    sysctl.dev_mask = (CAMSYS_DEVID_SENSOR_1B | CAMSYS_DEVID_MARVIN);
    sysctl.ops = CamSys_ClkIn;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_ClkIn on failed\n");
    }
    
		//1)power en
    usleep(1000);
    sysctl.dev_mask = (CAMSYS_DEVID_SENSOR_1B & HAL_DEVID_EXTERNAL);
    sysctl.ops = CamSys_PwrEn;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_PwrEn on failed");
    }
    
    //2)reset 
    usleep(1000);
    sysctl.ops = CamSys_Rst;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_Rst on failed\n");  
    }
    
    //3)power down control
    usleep(1000);
    //sysctl.dev_mask = CAMSYS_DEVID_SENSOR_1B;
    sysctl.ops = CamSys_PwrDn;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_PwrDn on failed");
    }
    usleep(2000);
    
    i2cinfo.bus_num = 3;
    i2cinfo.slave_addr = 0x6C; //0x6c; //0x20;
    i2cinfo.reg_addr = 0x0103;
    i2cinfo.reg_size = 2; 
    i2cinfo.val = 0x01;
    i2cinfo.val_size = 0x01;
    i2cinfo.i2cbuf_directly = 0;
    i2cinfo.speed = 100000;
       
    err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
    if(err<0) {
        printf("softreset::CAMSYS_I2CWR failed\n");
        i2cinfo.slave_addr = 0x20;
        err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
        if(err<0){
        	printf("softreset  again::CAMSYS_I2CWR failed\n");
        	ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"Back Camera:[%s] { ID:0x%x }\n",PCBA_FAILED,id);
        	goto end;
        }
    }/* else {
        printf("I2c write: 0x%x : 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
    }*/

    usleep(5000);    
    
    i2cinfo.reg_addr = 0x300a;
    i2cinfo.val_size = 0x01;       
    err = ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo);
    if (err<0) {
        printf("CAMSYS_I2CRD failed\n");
        ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"Back Camera:[%s] { ID:0x%x }\n",PCBA_FAILED,id);
        goto end;
    } else {
        printf("I2c read: 0x%x : 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
        id = (i2cinfo.val<<8);
    }
    
    i2cinfo.reg_addr = 0x300b;
    err = ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo);
    if (err<0) {
        printf("CAMSYS_I2CRD failed\n");
        ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"Back Camera:[%s] { ID:0x%x }\n",PCBA_FAILED,id);
        goto end;
    } else {
        printf("I2c read: 0x%x : 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
        id |= i2cinfo.val;
    }

    printf("\n!!!!!!!!!!Back Camera ID: 0x%x; default:0x008858!!!!!!!!!!\n",id);
    if(id == 0x88){
    ui_print_xy_rgba(0,tc_info->y,0,255,0,255,"Back Camera:[%s] { ID:0x%x }\n",PCBA_SECCESS,id);
  	}else{
  	ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"Back Camera:[%s] { ID:0x%x }\n",PCBA_FAILED,id);
  	}
    /*
    i2cinfo.reg_addr = 0x300c;
    err = ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo);
    if (err<0) {
        printf("CAMSYS_I2CRD failed\n");
    } else {
        printf("I2c read: 0x%x : 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
    }
    
    i2cinfo.reg_addr = 0x302a;
    err = ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo);
    if (err<0) {
        printf("CAMSYS_I2CRD failed\n");
    } else {
        printf("I2c read: 0x%x : 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
    }
    */
		/*
    i2cchar = (unsigned char*)i2cbase;
    sensor_reg = sensor_test;
    i2cbytes = 0x00;
    for (i=0; i<sizeof(sensor_test)/sizeof(struct rk_sensor_reg); i++) {
        *i2cchar++ = (sensor_reg->reg&0xff00)>>8; 
        *i2cchar++ = (sensor_reg->reg&0xff);
        *i2cchar++ = (sensor_reg->val&0xff);
        sensor_reg++;
        i2cbytes += 3;
    }

    i2cinfo.bus_num = 3;
    i2cinfo.slave_addr = 0x6c;
    i2cinfo.i2cbuf_directly = 1;
    i2cinfo.i2cbuf_bytes = ((3<<16)|i2cbytes);
    i2cinfo.speed = 100000;
    err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
    if (err<0) {
        printf("CAMSYS_I2CWR buf failed\n"); 
    }
    printf("Sensor init!\n");
    */
    end:
    return 0;
}

int extdev_register_front()
{
    int err = 0;
    int numLane = 2;
    camsys_devio_name_t extdev;
    
    extdev.dev_id = CAMSYS_DEVID_SENSOR_1A;
    strlcpy((char*)extdev.avdd.name, "NC",sizeof(extdev.avdd.name));
    extdev.avdd.min_uv = 28000000;
    extdev.avdd.max_uv = 28000000;    
    strlcpy((char*)extdev.dovdd.name,"NC",sizeof(extdev.dovdd.name));
    extdev.dovdd.min_uv = 18000000;
    extdev.dovdd.max_uv = 18000000;
    strlcpy((char*)extdev.dvdd.name, "NC",sizeof(extdev.dvdd.name));
    strlcpy((char*)extdev.afvdd.name, "NC",sizeof(extdev.afvdd.name));
    strlcpy((char*)extdev.pwrdn.name, "RK30_PIN2_PB6",sizeof(extdev.pwrdn.name));
    extdev.pwrdn.active = 0x01;
    strlcpy((char*)extdev.pwren.name, "RK30_PIN0_PC1",sizeof(extdev.pwren.name));
    extdev.pwren.active = 0x01;
    strlcpy((char*)extdev.rst.name,"NC",sizeof(extdev.rst.name));
    strlcpy((char*)extdev.afpwrdn.name,"NC",sizeof(extdev.afpwrdn.name));
    strlcpy((char*)extdev.afpwr.name,"NC",sizeof(extdev.afpwr.name));
    extdev.phy.type = CamSys_Phy_Mipi;
    //extdev.phy.info.mipi.phy_index = 1; //MIPI_PHY_INDEX;
    extdev.phy.info.mipi.data_en_bit = 0x01; //?
    
    /*if (numLane == 1) {
        extdev.phy.info.mipi.data_en_bit = 0x01;
        extdev.phy.info.mipi.bit_rate = 656;
    } else if (numLane == 2) {
        extdev.phy.info.mipi.data_en_bit = 0x03;
        extdev.phy.info.mipi.bit_rate = 328;
    } else if (numLane == 4) {
        extdev.phy.info.mipi.data_en_bit = 0x0f;
        extdev.phy.info.mipi.bit_rate = 408;
    }*/
    extdev.clk.in_rate = 24000000;
    
		printf("----CAMSYS_REGISTER_FRONT_DEVIO----\n");
    err = ioctl(camsys_fd, CAMSYS_REGISTER_DEVIO, &extdev);
    if (err<0) {
        printf("CAMSYS_REGISTER_DEVIO failed\n");
    }

    return err;

}

int extdev_init_front()
{
    int err,i2cbytes,i;
    struct rk_sensor_reg *sensor_reg;
    unsigned char *i2cchar;
    camsys_sysctrl_t sysctl;    
    camsys_i2c_info_t i2cinfo;
    int id;
    
    //if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();
    
    sysctl.dev_mask = (CAMSYS_DEVID_SENSOR_1A & HAL_DEVID_EXTERNAL);
    sysctl.ops = CamSys_Avdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_Avdd on failed!");
    }
    
    sysctl.ops = CamSys_Dvdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_Dvdd on failed!\n");
    }
    
    sysctl.ops = CamSys_Dovdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_Dovdd on failed!");
    }
    usleep(5000);
    		
    sysctl.dev_mask = (CAMSYS_DEVID_SENSOR_1A | CAMSYS_DEVID_MARVIN);
    sysctl.ops = CamSys_ClkIn;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_ClkIn on failed\n");
    }
    
		//1)power en
    usleep(1000);
    sysctl.dev_mask = (CAMSYS_DEVID_SENSOR_1A & HAL_DEVID_EXTERNAL);
    sysctl.ops = CamSys_PwrEn;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_PwrEn on failed");
    }
    
    //2)reset 
    usleep(1000);
    sysctl.ops = CamSys_Rst;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_Rst on failed\n");  
    }
    
    //3)power down control
    usleep(1000);
    //sysctl.dev_mask = CAMSYS_DEVID_SENSOR_1B;
    sysctl.ops = CamSys_PwrDn;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        printf("CamSys_PwrDn on failed");
    }
    usleep(2000);
    
    i2cinfo.bus_num = 3;
    i2cinfo.slave_addr = 0x6c; 
    i2cinfo.reg_addr = 0x3021;
    i2cinfo.reg_size = 2; 
    i2cinfo.val = 0x61;
    i2cinfo.val_size = 0x01;
    i2cinfo.i2cbuf_directly = 0;
    i2cinfo.speed = 100000;
       
    err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
   /* while (err<0) {
        printf("softreset::CAMSYS_I2CWR failed\n");
        err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
        if(err<0)
        	printf("softreset  again::CAMSYS_I2CWR failed\n");
    }*//* else {
        printf("I2c write: 0x%x : 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
    }*/
    
    if(err<0) {
        printf("softreset::CAMSYS_I2CWR failed\n");
        usleep(5000);
        err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
        if(err<0){
        	printf("softreset  again::CAMSYS_I2CWR failed\n");
        	ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"Front Camera:[%s] { ID:0x%x }\n",PCBA_FAILED,id);
        	goto end;
        }
		}
    usleep(5000);    
    
    i2cinfo.reg_addr = 0x3000;
    i2cinfo.val_size = 0x01;       
    err = ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo);
    if (err<0) {
        printf("CAMSYS_I2CRD failed\n");
        ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"Front Camera:[%s] { ID:0x%x }\n",PCBA_FAILED,id);
        goto end;
    } else {
        printf("I2c read: 0x%x : 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
        id = (i2cinfo.val<<8);
    }
    
    i2cinfo.reg_addr = 0x3001;
    err = ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo);
    if (err<0) {
        printf("CAMSYS_I2CRD failed\n");
        ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"Front Camera:[%s] { ID:0x%x }\n",PCBA_FAILED,id);
        goto end;
    } else {
        printf("I2c read: 0x%x : 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
        id |= i2cinfo.val;
    }

    printf("\n!!!!!!!!!!Front Camera ID: 0x%x;default:0x2520;!!!!!!!!!!\n",id);
    if(id == 0x2520){
    ui_print_xy_rgba(0,tc_info->y,0,255,0,255,"Front Camera:[%s] { ID:0x%x }\n",PCBA_SECCESS,id);
  	}else{
  	ui_print_xy_rgba(0,tc_info->y,0,0,255,255,"Front Camera:[%s] { ID:0x%x }\n",PCBA_FAILED,id);
  	}
    end:
    return 0;
}

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
		stopCameraTest();
		usleep(100000);
		pthread_create(&camera_tid, NULL, startCameraTest, NULL); 
	}
		
	return 0;
	
}


int CameraCreate(void)
{
    int err,size;
	struct v4l2_format format;

    if (iCamFd == 0) {
        iCamFd = open(videodevice, O_RDWR|O_CLOEXEC);
       
        if (iCamFd < 0) {
            printf(" Could not open the camera  device:%s\n",videodevice);
    		err = -1;
            goto exit;
        }

        memset(&mCamDriverCapability, 0, sizeof(struct v4l2_capability));
        err = ioctl(iCamFd, VIDIOC_QUERYCAP, &mCamDriverCapability);
        if (err < 0) {
        	printf("Error opening device unable to query device.\n");
    	    goto exit;
        } 
				if(strstr((char*)&mCamDriverCapability, "front") != NULL){
					printf("it is a front camera \n!");
					}
				else if(strstr((char*)&mCamDriverCapability, "back") != NULL){
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
    if(v4l2Buffer_phy_addr !=0)
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
            ionAllocData.len = 0x200000;
            ionAllocData.align = 4*1024;
	        ionAllocData.heap_id_mask = ION_HEAP(ION_CMA_HEAP_ID);
			ionAllocData.flags = 0;
              err = ioctl(iIonFd, ION_IOC_ALLOC, &ionAllocData);
            if(err) {
                printf("%s: ION_IOC_ALLOC failed to alloc 0x%x bytes with error - %s", 
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
            m_v4l2Buffer[0] = mmap(0, ionAllocData.len, PROT_READ|PROT_WRITE,
                    MAP_SHARED, fd_data.fd, 0);
            if(m_v4l2Buffer[0] == MAP_FAILED) {
                printf("%s: Failed to map the allocated memory: %s",
                        __FUNCTION__, strerror(errno));
                err = -errno;
                ioctl(iIonFd, ION_IOC_FREE, &handle_data);
                goto exit2;
            }
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
            	printf("%s: Successfully allocated 0x%x bytes, mIonFd=%d, SharedFd=%d",
            			__FUNCTION__,ionAllocData.len, iIonFd, fd_data.fd);
			v4l2Buffer_phy_addr = phys_data.phys;
        }
    memset(m_v4l2Buffer[0], 0x00, size);
suc_alloc:   
          err = ioctl(iCamFd, VIDIOC_QUERYCAP, &mCamDriverCapability);
        if (err < 0) {
        	printf("Error opening device unable to query device.\n");
    	    goto exit;
        }  
    return 0;

exit3:
	munmap(m_v4l2Buffer[0], size);
exit2:

    if(iIonFd > 0){
    	close(iIonFd);
    	iIonFd = -1;
        }
exit1:
exit:
    return err;
}

int CameraStart(int phy_addr, int buffer_count, int w, int h)
{
    int err,i;
    int nSizeBytes;
    struct v4l2_format format;
    enum v4l2_buf_type type;
    struct v4l2_requestbuffers creqbuf;
		
	//buffer_count = 2;
	if( phy_addr == 0 || buffer_count == 0  ) {
    	printf(" Video Buf is NULL\n");
		goto  fail_bufalloc;
    }

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
        buffer.m.offset = phy_addr + i*buffer.length;
        #else
        buffer.m.offset = phy_addr;
        #endif

        m_v4l2Buffer[i] =(void*)((int)m_v4l2Buffer[0] + i*buffer.length);
//	memset(m_v4l2Buffer[i],0x0,buffer.length);
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
	//printf("preview_w = %d,preview_h =%d,panelsize[1] = %d,panelsize[0] = %d\n",preview_w,preview_h,panelsize[1],panelsize[0]);
	//var.xres_virtual = preview_w;	//win0 memery x size
	//var.yres_virtual = preview_h;	 //win0 memery y size
	var.xoffset = 0;   //win0 start x in memery
	var.yoffset = 0;   //win0 start y in memery
	var.nonstd = ((cory<<20)&0xfff00000) + ((corx<<8)&0xfff00) +FB_NONSTAND; //win0 ypos & xpos & format (ypos<<20 + xpos<<8 + format)
	var.grayscale = ((preview_h<<20)&0xfff00000) + (( preview_w<<8)&0xfff00) + 0;	//win0 xsize & ysize
	var.xres = preview_w;	 //win0 show x size
	var.yres = preview_h;	 //win0 show y size
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
	struct v4l2_requestbuffers creqbuf;
    creqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    isstoped =1;
    while(!hasstoped){
    	sleep(1);
    	}
    if (ioctl(iCamFd, VIDIOC_STREAMOFF, &creqbuf.type) == -1) {
        printf("%s VIDIOC_STREAMOFF Failed\n", __FUNCTION__);
   //     return -1;
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
		if (iDispFd > 0) 
		{	
#if CAM_OVERLAY_BUF_NEW
			data[0] = (int)cfilledbuffer1.m.offset;  
#else
			data[0] = (int)cfilledbuffer1.m.offset + cfilledbuffer1.index * cfilledbuffer1.length;
#endif
			data[1] = (int)(data[0] + preview_w *preview_h);
			//  		for(i = 0;i < 100;i++){
			// 			printf("0x%x ",*((char*)(m_v4l2Buffer[cfilledbuffer1.index])+i));
			//  			}
			//printf("y_addr = 0x%x,length = %d\n",data[0],cfilledbuffer1.length);
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
			//printf("preview_w = %d,preview_h =%d,panelsize[1] = %d,panelsize[0] = %d\n",preview_w,preview_h,panelsize[1],panelsize[0]);
			var.xres_virtual = preview_w;	//win0 memery x size
			var.yres_virtual = preview_h;	 //win0 memery y size
			var.xoffset = 0;   //win0 start x in memery
			var.yoffset = 0;   //win0 start y in memery
			var.nonstd = ((cory<<20)&0xfff00000) + (( corx<<8)&0xfff00) +FB_NONSTAND;
			var.grayscale = ((preview_h<<20)&0xfff00000) + (( preview_w<<8)&0xfff00) + 0;   //win0 xsize & ysize
			var.xres = preview_w;	 //win0 show x size
			var.yres = preview_h;	 //win0 show y size
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
	
    if(access(CAMSYS_DEVNAME, O_RDWR) <0 ){
	   printf("access %s failed\n",CAMSYS_DEVNAME);
	   hasstoped = 1;
	   return -1;
     }
  camsys_fd = open(CAMSYS_DEVNAME,O_RDWR);
    if (camsys_fd < 0) {
        printf("Open %s  failed\n", CAMSYS_DEVNAME);
        //goto end;
    }
  extdev_register();
  extdev_init();  
  
  extdev_register_front();
  extdev_init_front();
  
  if (camsys_fd > 0) {
  close(camsys_fd);  
	} 
/*	  
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
	*/
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
		TaskStop();
	
		if(iIonFd > 0){
			munmap(m_v4l2Buffer[0], ionAllocData.len);
	
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

	return 0;
}


void * camera_test(void *argc)
{
	int ret,num;
	printf("------------enter camera_test------- \n");
	tc_info = (struct testcase_info *)argc; 

	if (script_fetch("camera", "number",&num, 1) == 0) {
		printf("camera_test num:%d\r\n",num);
		camera_num = num;	
	}

	pthread_create(&camera_tid, NULL, startCameraTest, NULL); 
		
	return argc;
}

