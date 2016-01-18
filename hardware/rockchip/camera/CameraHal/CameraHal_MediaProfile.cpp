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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/version.h>

#include "CameraHal.h"
#include "CameraHal_Module.h"

namespace android {

#define VIDEO_DEV_NAME   "/dev/video0"
#define VIDEO_DEV_NAME_1   "/dev/video1"

#define RK29_CAM_VERSION_CODE_1 KERNEL_VERSION(0, 0, 1)
#define RK29_CAM_VERSION_CODE_2 KERNEL_VERSION(0, 0, 2)

static uint resolution[][2] = {{176,144},{240,160},{320,240},{352,288},
                     {640,480},{720,480},{800,600}, {1280,720},{1920,1080},
                     {0,0}};

#define RK30_PLAT 1
#define RK29_PLAT 0

#define XML_FILE_DEFAULT "/etc/media_profiles_default.xml"
#define XML_FILE_TMP "/data/media_profiles_tmp.xml"



static char fmt_name[][10]={
{"qcif"},
{"160p"},
{"qvga"},
{"cif"},
{"480p"},
{"480p"},
{"svga"},
{"720p"},
{"1080p"}
};

struct xml_video_element{
	int n_cameraId;
	char str_quality[10];
	int n_width;
	int n_height;
	int n_frameRate;
	int isAddMark;
};

struct xml_fp_position{
	int camid;
	long camid_start;
	long camid_end;
};

struct xml_video_name{
	int camid;
	char camera_name[30];
};


int CameraGroupFound_new(struct xml_video_name* video_name)
{
    int media_profiles_id=0,i,len,front_id = 0,back_id = 0;
    struct xml_video_name* pst_video_name = video_name;
    char camera_link[30];
    char *camera_name = pst_video_name->camera_name;
    char *end;

    for (i=0; i<2; i++) {
        pst_video_name->camid = i;
        sprintf(camera_link,"/sys/dev/char/81:%d/device",i);
        memset(camera_name,0x00,sizeof(pst_video_name->camera_name));
        len = readlink(camera_link,camera_name, sizeof(pst_video_name->camera_name));
        if (len > 0) {
            if (strstr(camera_name,"front")) {
                camera_name = strstr(camera_name,"front");
                camera_name = strstr(camera_name,"_");
                if (camera_name != NULL)
                    front_id = strtol(camera_name+1,&end,10);
                else
                    front_id = 0;
            } else if (strstr(camera_name,"back")) {
                camera_name = strstr(camera_name,"back");
                camera_name = strstr(camera_name,"_");
                if (camera_name != NULL)
                    back_id = strtol(camera_name+1,&end,10);
                else 
                    back_id = 0;
            }

            media_profiles_id = ((back_id<<8) | front_id);
        }
        pst_video_name++;
        camera_name = pst_video_name->camera_name;
    }
    return media_profiles_id;
}

int isSame_camera(struct xml_video_name* device_videoname, struct xml_video_name* xml_videoname)
{
	int ret = -1;
	struct xml_video_name* pst_device_name = device_videoname;
	struct xml_video_name* pst_xml_name = xml_videoname;
	int i;

	ret = strcmp(pst_device_name->camera_name, pst_xml_name->camera_name);
	pst_device_name++;
	pst_xml_name++;
	ret |= strcmp(pst_device_name->camera_name, pst_xml_name->camera_name);

	return ret;
}


int find_resolution_index(int w, int h)
{
	int list_w, list_h;
	int i=0;
	int index=-1;
	
	do{
		list_w = resolution[i][0];
		list_h = resolution[i][1];
		
		if(list_w==w && list_h==h){
			index = i;
			break;
		}
		i++;
	}while(list_w!=0 && list_h!=0);

	return index;
}

int find_frameRate(struct xml_video_element* pst_element, int element_count, struct xml_video_element* find_element)
{
	int i;
	int ret = -1;
	struct xml_video_element* element = pst_element;
	
	for(i=0; i<element_count; i++, element++){
		if(find_element->n_cameraId != element->n_cameraId){
			continue;
		}
		ret = strcmp(element->str_quality, find_element->str_quality);
		//printf("strcmp(%d) %d==%d\n", ret, element->n_width, find_element->n_width);
		if(!ret && (element->n_width==find_element->n_width))
		{
			find_element->n_height = element->n_height;
			find_element->n_frameRate = element->n_frameRate;
			find_element->isAddMark = element->isAddMark;
			break;
		}
	}
	
	if(i==element_count)
		return -1;
	else
		return 0;
}

int find_camId_fseek(FILE* fp, struct xml_fp_position* fp_pos){
	char one_line_buf[256];
	int find_fmt_sign=0;
	char *leave_line, *leave_line1, *leave_line2;
	char str_camId[4];
	const char *equal_sign = "="; 
	int count=0;
	
	if(fp==NULL)
		return -1;
	
	if(fp_pos==NULL)
		return -2;
		
	memset(str_camId, 0x00, sizeof(str_camId));
	sprintf(str_camId, "%d", fp_pos->camid);	
	fseek(fp,0,SEEK_SET); 
	while(fgets(one_line_buf,256,fp) != NULL) 
	{
		if(strlen(one_line_buf) < 3) //line is NULL
		{ 
			continue; 
		} 
		
		if(find_fmt_sign==0)
		{
			leave_line = NULL; 
			leave_line = strstr(one_line_buf, "<CamcorderProfiles"); 
			if(leave_line == NULL) //no "<CamcorderProfiles" 
			{ 
				continue; 
			} 
			
			leave_line1 = NULL; 
			leave_line1 = strstr(leave_line,equal_sign); 
			if(leave_line1 == NULL) //no "="
			{ 
				continue; 
			}
			
			leave_line2 = NULL; 
			leave_line2 = strstr(leave_line1,str_camId); 
			if(leave_line2 == NULL) //no "0/1"
			{ 
				continue; 
			}else{
				fp_pos->camid_start = ftell(fp);
				find_fmt_sign=1;
				continue;
			}
		}else{
			leave_line = NULL; 
			leave_line = strstr(one_line_buf, "</CamcorderProfiles>"); 
			if(leave_line == NULL) //no 
			{ 
				continue; 
			}else{
				fp_pos->camid_end = ftell(fp);
				break;
			}
		}
			
		if(fgetc(fp)==EOF) 
		{ 
			break; 
		} 
		fseek(fp,-1,SEEK_CUR); 
		memset(one_line_buf,0,sizeof(one_line_buf));
	}
	
	return 0;
}
int xml_version_check(struct v4l2_capability* CamDriverCapability)
{
	return (CamDriverCapability->version > 0x000300)?1:0;
}

int xml_read_camname(FILE* fp, struct xml_video_name* video_name, int video_count)
{
	char one_line_buf[256];
	char *leave_line0, *leave_line1, *leave_line2;
	int leave_num;
	const char* equal_sign = "=";
	const char* mark_sign_start = "<!--";
	const char* mark_sign_end = "-->";
	const char* videoname_sign = "videoname";
	struct xml_video_name* pst_video_name = video_name;
	int count = 0;
	
	fseek(fp,0,SEEK_SET);
	
	while(fgets(one_line_buf,256,fp) != NULL) 
	{ 
		if(strlen(one_line_buf) < 3) //line is NULL
		{ 
			continue; 
		} 
		leave_line0 = NULL;
		leave_line0 = strstr(one_line_buf, mark_sign_start);
		if(leave_line0==NULL)
		{
			continue;
		}
		leave_line1 = NULL;
		leave_line1 = strstr(one_line_buf, mark_sign_end);
		if(leave_line1==NULL)
		{
			continue;
		}

		leave_line0 = NULL;
		leave_line0 = strstr(one_line_buf, videoname_sign);
		if(leave_line0==NULL)
		{
			continue;
		}	

		leave_line1 = NULL;
		leave_line1 = strstr(leave_line0, equal_sign);
		if(leave_line1==NULL)
		{
			continue;
		}	

		sscanf(leave_line0, "videoname%d=\"%[^\"]", &pst_video_name->camid, pst_video_name->camera_name);
		count++;
		if(count==video_count){
			break;
		}
		pst_video_name++;
		
		if(fgetc(fp)==EOF) 
		{ 
			break; 
		} 
		fseek(fp,-1,SEEK_CUR);		 
	}

	return count;
}


int xml_copy_and_write_camname(const char* src_file, char* dst_file, struct xml_video_name* video_name, int video_count)
{
	FILE *fpsrc, *fpdst; 
	char one_line_buf[256]; 
	char *leave_line0, *leave_line1, *leave_line2;
	struct xml_video_name* pst_video_name = video_name;
	int isWrite=0;
	int i;
	
	fpsrc = fopen(src_file,"r"); 
	if(fpsrc == NULL) 
	{ 
		LOGE("%s 111OPEN '%s' FALID, r \n", __FUNCTION__, src_file); 
		return -1; 
	} 
	
	fpdst = fopen(dst_file,"w"); 
	if(fpdst == NULL) 
	{ 
		LOGE("%s 222OPEN %s TEMP FALID w \n",__FUNCTION__, dst_file); 
		return -2; 
	} 

	fseek(fpsrc,0,SEEK_SET); 
	fseek(fpdst,0,SEEK_SET);	
	while(fgets(one_line_buf,256,fpsrc) != NULL) 
	{ 
		
		fputs(one_line_buf, fpdst);

		if(isWrite==0){
			leave_line0 = NULL;
			leave_line0 = strstr(one_line_buf, "<?");
			if(leave_line0==NULL){
				continue;
			}

			leave_line0 = NULL;
			leave_line0 = strstr(one_line_buf, "?>");
			if(leave_line0==NULL){
				continue;
			}

			for(i=0; i<video_count; i++){
				fprintf(fpdst, "<!--  videoname%d=\"%s\"  -->  \n", pst_video_name->camid, pst_video_name->camera_name);
				pst_video_name++;
			}
			isWrite=1;	
		}
		
		if(fgetc(fpsrc)==EOF) 
		{ 
			break; 
		} 
		fseek(fpsrc,-1,SEEK_CUR); 		 
	}

	memset(one_line_buf,0,sizeof(one_line_buf));
	fclose(fpsrc);                 
	fclose(fpdst);  

	return 0;
}

int xml_alter(char* src_xml_file, const char* dst_xml_file, struct xml_video_element* pst_element, int element_count)
{
	int ret = 0, err=0;
	int alter_sign = 0;
	int find_fmt_sign=0;
	int leave_num=0;
	long now_fp_pos;
	const char *equal_sign = "="; 
	FILE *src_fp = NULL, *dst_fp = NULL;
	char one_line_buf[256];
	char frontpart_line[50];
	long front_fptmp = 0,back_fptmp = 0;
	struct xml_fp_position fp_pos[2];
	struct xml_video_element find_element;
	char *leave_line, *leave_line1, *leave_line2;
	struct xml_video_element* element =pst_element ;

	memset(&find_element, 0x00, sizeof(struct xml_video_element));
	src_fp = fopen(src_xml_file, "r");
	if(src_fp==NULL){
		err = -1;
		LOGD("open file '%s' failed!!! (r)\n", src_xml_file);
		goto alter_exit;
	}
	
	dst_fp = fopen(dst_xml_file, "w");
	if(dst_fp==NULL){
		err = -2;
		LOGD("open file '%s' failed!!! (r)\n", dst_xml_file);
		goto alter_exit;
	}
	
	memset(&fp_pos, 0x00, 2*sizeof(struct xml_fp_position));
	fp_pos[0].camid = 0;
	ret = find_camId_fseek(src_fp, &fp_pos[0]);
	if(ret < 0 || fp_pos[0].camid_end <= fp_pos[0].camid_start){
		LOGD("find camid(%d) failed\n", fp_pos[0].camid);
		err = -3;
		goto alter_exit;	
	}
	
	fp_pos[1].camid = 1;
	ret = find_camId_fseek(src_fp, &fp_pos[1]);
	if(ret < 0 || fp_pos[1].camid_end <= fp_pos[1].camid_start){
		LOGD("find camid(%d) failed\n", fp_pos[1].camid);
		err = -3;
		goto alter_exit;	
	}
	
	if(fp_pos[0].camid_end>0 && fp_pos[0].camid_start>0 && fp_pos[1].camid_end>0 && fp_pos[1].camid_start>0){
		fseek(src_fp,0,SEEK_SET); 
		fseek(dst_fp,0,SEEK_SET);
			
		while(fgets(one_line_buf,256,src_fp) != NULL) 
		{ 
			if(strlen(one_line_buf) < 3) //line is NULL
			{ 
				fputs(one_line_buf, dst_fp);
				continue; 
			} 
							
			if(find_fmt_sign==0)
			{		
				leave_line = NULL; 
				leave_line = strstr(one_line_buf,equal_sign); 
				if(leave_line == NULL) //no "="
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
				leave_line1 = NULL; 
				leave_line1 = strstr(one_line_buf, "<EncoderProfile"); 
				if(leave_line1 == NULL) 
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
							
				leave_line2 = NULL; 
				leave_line2 = strstr(leave_line1, "timelapse"); 
				if(leave_line2 == NULL) 
				{ 
					memset(find_element.str_quality, 0x00, sizeof(find_element.str_quality));
					sscanf(leave_line, "%*[^\"]\"%[^\"]", find_element.str_quality);
				}else{
					memset(find_element.str_quality, 0x00, sizeof(find_element.str_quality));
					sscanf(leave_line, "%*[^\"]\"timelapse%[^\"]", find_element.str_quality);
				} 
								
				//printf("quality %s\n", find_element.str_quality);			
				find_fmt_sign = 1;
				front_fptmp = ftell(dst_fp);
				fprintf(dst_fp, "     \n");	
				fputs(one_line_buf, dst_fp);
				continue; 
			}
			else if(find_fmt_sign==1)
			{		
				leave_line = NULL; 
				leave_line = strstr(one_line_buf,equal_sign); 
				if(leave_line == NULL) //no "="
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				}
							
				leave_line1 = NULL; 
				leave_line1 = strstr(one_line_buf,"width"); 
				if(leave_line1 == NULL) //no "width"
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
		 		sscanf(leave_line, "%*[^1-9]%d\"", &find_element.n_width);
				//printf("%d\n", find_element.n_width);
				find_fmt_sign=2;
				fputs(one_line_buf, dst_fp);
				continue; 	 
			}
			else if(find_fmt_sign==2)
			{		
				leave_line = NULL; 
				leave_line = strstr(one_line_buf,equal_sign); 
				if(leave_line == NULL) //no "="
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				}
				leave_line1 = NULL; 
				leave_line1 = strstr(one_line_buf, "frameRate"); 
				if(leave_line1 == NULL) //no "framRate"
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
			
				now_fp_pos = ftell(src_fp);
				if(now_fp_pos>fp_pos[0].camid_start && now_fp_pos<fp_pos[0].camid_end)
					find_element.n_cameraId = 0;
				else if(now_fp_pos>fp_pos[1].camid_start && now_fp_pos<fp_pos[1].camid_end)
					find_element.n_cameraId = 1;
				else
					find_element.n_cameraId = -1;
				
				if(find_element.n_cameraId != -1){
					ret = find_frameRate(element, element_count, &find_element);
					if(ret==0){
						leave_num = leave_line - one_line_buf;
						memset(frontpart_line,0,sizeof(frontpart_line)); 
						strncpy(frontpart_line,one_line_buf,leave_num);  
						fputs(frontpart_line,dst_fp);
											
						//printf("new frameRate %d  isaddmark(%d)\n", find_element.n_frameRate, find_element.isAddMark);
						fprintf(dst_fp,"=\"%d\" /> \n", (find_element.n_frameRate)); 
						alter_sign++; 
						find_fmt_sign = 3;	
						LOGD("XML modify: camID(%d) resolution:%s(%dx%d) fps(%d) isaddmark(%d)\n",find_element.n_cameraId,find_element.str_quality, 
							find_element.n_width, find_element.n_height, find_element.n_frameRate, find_element.isAddMark);
					}else{
						LOGD("WARNING: can't find camID(%d) resolution:%s(%dx), addmark!!!\n", find_element.n_cameraId,find_element.str_quality, find_element.n_width);
						find_element.isAddMark=1;
						find_fmt_sign = 3;
						fputs(one_line_buf, dst_fp);
						//continue;
					}
				}else{
					find_fmt_sign = 3;
					fputs(one_line_buf, dst_fp);
					continue;
				}
			}else if(find_fmt_sign==3){
				leave_line = NULL; 
				leave_line = strstr(one_line_buf,"</EncoderProfile>"); 
				if(leave_line == NULL) //no "framRate"
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
				fputs(one_line_buf, dst_fp);	
				if(find_element.isAddMark){
					back_fptmp = ftell(dst_fp);
					fseek(dst_fp,front_fptmp,SEEK_SET); 
					fprintf(dst_fp, "<!--  \n");
					fseek(dst_fp,back_fptmp,SEEK_SET); 
					fprintf(dst_fp, "-->  \n");
					find_element.isAddMark=0;
				}
				find_fmt_sign=0;
			}
		
			if(fgetc(src_fp)==EOF) 
			{ 
				break; 
			} 
			fseek(src_fp,-1,SEEK_CUR); 
			memset(one_line_buf,0,sizeof(one_line_buf)); 
		} 
	}
	
alter_exit:
	fclose(src_fp);                 
	fclose(dst_fp);

	if(err==0){
		remove(src_xml_file);    
		rename(dst_xml_file,src_xml_file); 
		chmod(src_xml_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	}
	return ret;
}


int camera_request_framerate(const char* dev_path, int camid, struct xml_video_element* pst_element, int element_count)
{
    int err,i;
    int nSizeBytes;
	int ret;
	int crop_w, crop_h;
	int fps;
	int width, height;
	int iCamFd=-1;
	int ver=0;
	unsigned int pix_format_tmp;
	struct v4l2_format format;
	struct v4l2_frmivalenum fival;
	struct v4l2_capability CamDriverCapability;
	struct xml_video_element* element=pst_element;
	struct xml_video_element* element_tmp;
	int resolution_index;
	struct v4l2_format fmt; 
    int sensor_resolution_w=0,sensor_resolution_h=0;
    
    iCamFd = open(dev_path, O_RDWR);

    if (iCamFd < 0) {
        LOGE("%s.%d  Could not open the camera device %s \n", 
        	__FUNCTION__,__LINE__,dev_path);
        err = -1;
        goto exit;
    }
	
    memset(&CamDriverCapability, 0, sizeof(struct v4l2_capability));
    err = ioctl(iCamFd, VIDIOC_QUERYCAP, &CamDriverCapability);
    if (err < 0) {
        LOGE("%s.%d  Error opening device unable to query device.\n",__FUNCTION__,__LINE__);
        goto exit;
    } 

    if (CamDriverCapability.version == RK29_CAM_VERSION_CODE_1) {
        pix_format_tmp = V4L2_PIX_FMT_YUV420;
        LOGD("%s.%d  Current camera driver version: 0.0.1 \n",__FUNCTION__,__LINE__);    
    } else { 
        pix_format_tmp = V4L2_PIX_FMT_NV12;        
    }
    /* oyyf@rock-chips.com: v0.4.0x15 */
    if(strcmp((char*)&CamDriverCapability.driver[0],"uvcvideo") == 0){
        i=0;        
        sensor_resolution_w = 640;        /* ddl@rock-chips.com: uvc camera resolution fix vga */
        sensor_resolution_h = 480;        
        
        for(i=0; i<element_count; i++, element++){
            width = resolution[i][0];
            height = resolution[i][1];				
            element->n_cameraId = camid;
            element->n_width = width;
            element->n_height = height;
            strcat(element->str_quality, fmt_name[i]); 
            element->n_frameRate = 15;
            element->isAddMark = 0;
            if ((width>sensor_resolution_w) || (height>sensor_resolution_h)) {
                element->isAddMark = 1;
                LOGD("USB-CAMERA: CameraId:%d  %dx%d fps: %d isAddMark(%d)\n",camid,width,height,element->n_frameRate,element->isAddMark);
                continue;
            }
            LOGD("USB-CAMERA: CameraId:%d  %dx%d fps: %d isAddMark(%d)\n",camid,width,height,element->n_frameRate,element->isAddMark);
       }

        goto exit;
    }
    
	ver = xml_version_check(&CamDriverCapability);
	if(ver){
        i=0;

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat= pix_format_tmp;
        fmt.fmt.pix.field = V4L2_FIELD_NONE;

        /*picture size setting*/
        fmt.fmt.pix.width = 10000;
        fmt.fmt.pix.height = 10000;
        ret = ioctl(iCamFd, VIDIOC_TRY_FMT, &fmt);

        sensor_resolution_w = fmt.fmt.pix.width;
        sensor_resolution_h = fmt.fmt.pix.height;  /* ddl@rock-chips.com: v0.4.e */
        
        for(i=0; i<element_count; i++, element++){
            width = resolution[i][0];
            height = resolution[i][1];				
            memset(&fival, 0, sizeof(fival));
            fival.index = 0;
            fival.pixel_format = pix_format_tmp;
            fival.width = width;
            fival.height = height;
            fival.reserved[1] = 0x00;
            element->n_cameraId = camid;
            element->n_width = width;
            element->n_height = height;
            strcat(element->str_quality, fmt_name[i]); 
            element->n_frameRate = 0;
            element->isAddMark = 0;

            /* ddl@rock-chips.com: v0.4.e */
            if ((width>sensor_resolution_w) || (height>sensor_resolution_h)) {
                element->isAddMark = 1;
                continue;
            }
            
            if ((ret = ioctl(iCamFd, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0) {
                if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {			
                    fps = (fival.discrete.denominator/fival.discrete.numerator);
                    crop_w = (fival.reserved[1]&0xffff0000)>>16;
                    crop_h = (fival.reserved[1]&0xffff);

                    element->n_frameRate = fps;

                    if ((crop_w!= width) || (crop_h != height)) {
                        if(width==1280 && height==720 ) {
                            element->isAddMark = 1;
                        }
                    }

                    if(width==720 && height==480){

                        if ((crop_w>800) || (crop_h>600)) {    /* ddl@rock-chips.com: v0.4.e */
                            element->isAddMark = 1;    
                        } else {                       
                            resolution_index = find_resolution_index(640,480);
                            element_tmp = pst_element + resolution_index;
                            element_tmp->isAddMark = 1;
                        }
                    }

                    LOGD("CameraId:%d  %dx%d(%dx%d) fps: %d\n",camid,width,height,crop_w,crop_h, fps);
                    
                } else {
					LOGE("fival.type != V4L2_FRMIVAL_TYPE_DISCRETE\n");
				}
			} else {
				element->isAddMark = 1;
				LOGE("find frame intervals failed ret(%d)\n", ret);
	 		}
	 	}
    } else {
        LOGE("%s.%d  Current camera driver version: %d.%d.%d, It is not support CameraHal_MediaProfile\n",
            __FUNCTION__,__LINE__,
            (CamDriverCapability.version>>16) & 0xff,(CamDriverCapability.version>>8) & 0xff,
            CamDriverCapability.version & 0xff); 
        err = -1;
    }

exit:
    if(iCamFd>0){
        close(iCamFd);
        iCamFd=-1;
    }
    return err;
}



int media_profiles_xml_control(void)
{	
	int ver=0;
	int ret=0,ret1,ret2;
	int i;
	int element_count;
	struct xml_video_element *pst_element, *pst_element1;
	struct timeval t0, t1, t2, t3;
	int media_profiles_id;
	struct xml_video_name *pst_device_video_name;
	struct xml_video_name *pst_xml_video_name;
	FILE* client_fp;
	char clientXmlFile[30];
	char defaultXmlFile[30];
	int count;
	FILE* fp=NULL;

	pst_xml_video_name = (struct xml_video_name*)malloc(2*sizeof(struct xml_video_name));
	memset(pst_xml_video_name, 0x00, 2*sizeof(struct xml_video_name));
	pst_device_video_name = (struct xml_video_name*)malloc(2*sizeof(struct xml_video_name));
	memset(pst_device_video_name, 0x00, 2*sizeof(struct xml_video_name));
	
	media_profiles_id = CameraGroupFound_new(pst_device_video_name);
	if(media_profiles_id==0){
		clientXmlFile[0]=0x00;
		strcat(clientXmlFile,"/etc/media_profiles.xml");
		printf("client media_profiles.xml file name %s\n", clientXmlFile);
	}else{
		sprintf(clientXmlFile,"/etc/media_profiles%d%d.xml",(media_profiles_id&0xff00)>>8,media_profiles_id&0xff);
		printf("client media_profiles.xml file name %s\n", clientXmlFile);
	}
	
	client_fp = fopen(clientXmlFile, "r");
	if(client_fp != NULL){
		LOGD("client have %s file, so we use client file first!\n", clientXmlFile);
		fclose(client_fp);
		goto xml_exit;	
	}
	
	defaultXmlFile[0] = 0x00;
	strcat(defaultXmlFile,"/data/media_profiles.xml");
	printf("media_profiles.xml file name %s\n", defaultXmlFile);
	
	fp = fopen(defaultXmlFile, "r");
	if(fp!=NULL){
		count = xml_read_camname(fp, pst_xml_video_name, 2);
		//if camera dev name is same ,then, wo don't need to create new xml file
		ret = isSame_camera(pst_device_video_name, pst_xml_video_name);
		fclose(fp);
		if(ret==0){
			LOGE("%s is exist, and camera device is same!\n", defaultXmlFile);
			goto xml_exit;
		}
		LOGD("%s is exist, but camera device is not same!\n", defaultXmlFile);
		remove(defaultXmlFile);
	}
	ret = xml_copy_and_write_camname(XML_FILE_DEFAULT, defaultXmlFile, pst_device_video_name, 2);
	if(ret<0){
		LOGD("copy file failed\n");
		goto xml_exit;
	} else {
        LOGD("copy file %s from %s, and alter its configuration\n", defaultXmlFile,XML_FILE_DEFAULT);
	}

	i=0;
	element_count=0;
	while(resolution[i][0]>0 && resolution[i][1]>0){
		element_count++;
		i++;
	}
	pst_element = (struct xml_video_element*)malloc(2*element_count*sizeof(struct xml_video_element));
	memset(pst_element, 0x00, 2*element_count*sizeof(struct xml_video_element));
	pst_element1 = pst_element + element_count;

	//dev:  /dev/video0
	ret1 = camera_request_framerate(VIDEO_DEV_NAME, 0, pst_element, element_count);
	//dev:  /dev/video1
	ret2 = camera_request_framerate(VIDEO_DEV_NAME_1, 1, pst_element1, element_count);

    if ((ret1>=0) || (ret2>=0)) {
	    ret = xml_alter(defaultXmlFile, XML_FILE_TMP, pst_element, 2*element_count);
        LOGD("/data/media_profiles.xml is validate!");
    } else {
        LOGD("/etc/media_profiles.xml is validate!");
    }

	free(pst_element);
	pst_element=NULL;
	
xml_exit:
	free(pst_xml_video_name);
	pst_xml_video_name=NULL;
	free(pst_device_video_name);
	pst_device_video_name=NULL;
	
	return ret;
}

}

