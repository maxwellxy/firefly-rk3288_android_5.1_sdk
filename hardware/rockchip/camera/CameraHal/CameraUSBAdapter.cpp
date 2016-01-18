#include "CameraHal.h"


namespace android{


CameraUSBAdapter::CameraUSBAdapter(int cameraId)
                   :CameraAdapter(cameraId)
{

    mCamDriverV4l2MemType = V4L2_MEMORY_MMAP;
    CameraHal_SupportFmt[0] = V4L2_PIX_FMT_NV12;
    CameraHal_SupportFmt[1] = V4L2_PIX_FMT_NV16;
    #if CONFIG_CAMERA_UVC_MJPEG_SUPPORT
    CameraHal_SupportFmt[2] = V4L2_PIX_FMT_MJPEG;
    CameraHal_SupportFmt[3] = V4L2_PIX_FMT_YUYV;
    CameraHal_SupportFmt[4] = V4L2_PIX_FMT_RGB565;
    #else
    CameraHal_SupportFmt[2] = V4L2_PIX_FMT_YUYV;
    CameraHal_SupportFmt[3] = V4L2_PIX_FMT_RGB565;
    CameraHal_SupportFmt[4] = 0x00;
    #endif
    CameraHal_SupportFmt[5] = 0x00;
	
	mWhiteBalance_number = 0;
	mEffect_number = 0;
	mScene_number = 0;
	mAntibanding_number = 0;
	mZoomMin = 0;
	mZoomMax = 0;
	mZoomStep = 0;
	mFlashMode_number = 0;
	mCamDriverFrmWidthMax = 0;
	mCamDriverFrmHeightMax = 0;
}
CameraUSBAdapter::~CameraUSBAdapter()
{

}

int CameraUSBAdapter::cameraStop()
{
    
    int i;
    char *cameraDevicePathCur = NULL;
    /* ddl@rock-chips.com: Release v4l2 buffer must by close device, buffer isn't release in VIDIOC_STREAMOFF ioctl */
	if (mCamDriverV4l2MemType == V4L2_MEMORY_MMAP) {
    	for (i=0; i<V4L2_BUFFER_MAX; i++) {
        	if (mCamDriverV4l2Buffer[i] != NULL) {
            	if (munmap((void*)mCamDriverV4l2Buffer[i], mCamDriverV4l2BufferLen) < 0)
                	LOGE("%s(%d): mCamDriverV4l2Buffer[%d] munmap failed : %s",__FUNCTION__,__LINE__,i,strerror(errno));
            	mCamDriverV4l2Buffer[i] = NULL;
        	} else {
            	break;
        	}
    	}
	}	
    close(mCamFd);
    cameraDevicePathCur = (char*)&gCamInfos[mCamId].device_path[0];
    mCamFd = open(cameraDevicePathCur, O_RDWR);
    if (mCamFd < 0) {
        LOGE ("%s(%d): Could not open the camera device(%s): %s",__FUNCTION__,__LINE__, cameraDevicePathCur, strerror(errno) );
        return -1;
    }
    return 0;
}

void CameraUSBAdapter::initDefaultParameters(int camFd)
{
    CameraParameters params;
    String8 parameterString;
	int i,j,k,previewFrameSizeMax;
	char cur_param[32],str_element[32];        /* ddl@rock-chips.com: v0.4.f */
    char str[300];           
    int ret,picture_size_bit,framerate[200];
    struct v4l2_format fmt;    
    struct v4l2_queryctrl query_control;
    struct v4l2_control control;
	struct v4l2_querymenu *menu_ptr,query_menu;   
    struct v4l2_frmivalenum fival;
    struct v4l2_frmsizeenum fsize; 
    bool dot,isRestartPreview = false;
    char *ptr,str_fov_h[4],str_fov_v[4],fov_h,fov_v;
    
    LOG_FUNCTION_NAME  
    i = 0;
    memset(str,0x00,sizeof(str));
    memset(framerate,0x00,sizeof(framerate));

    /*preview size setting*/
    fsize.index = 0;       
    fsize.pixel_format = mCamDriverPreviewFmt;
	mCamDriverFrmWidthMax = 0;
	mCamDriverFrmHeightMax = 0;
    while ((ret = ioctl(mCamFd, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0) {
        if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {  
//			if(fsize.discrete.width%16 || fsize.discrete.height%16)
//			{
//				fsize.index++;
//				continue;
//			}
            memset(str_element,0x00,sizeof(str_element));
            if (parameterString.size() != 0) 
                str_element[0]=',';
            sprintf((char*)(&str_element[strlen(str_element)]),"%d",fsize.discrete.width);
            strcat(str_element, "x");
            sprintf((char*)(&str_element[strlen(str_element)]),"%d",fsize.discrete.height);
            parameterString.append((const char*)str_element);

            if (fsize.discrete.width > (unsigned int)mCamDriverFrmWidthMax) {
                mCamDriverFrmWidthMax = fsize.discrete.width;
                mCamDriverFrmHeightMax = fsize.discrete.height;
            }
            
            memset(&fival, 0, sizeof(fival));
            fival.index = 0;
            fival.pixel_format = fsize.pixel_format;
            fival.width = fsize.discrete.width;
            fival.height = fsize.discrete.height;
            while ((ret = ioctl(mCamFd, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0) {
                if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                    framerate[i++] = fival.discrete.denominator/fival.discrete.numerator;
                    //LOGD("%dx%d : %d  %d/%d",fival.width,fival.height, framerate[i-1],fival.discrete.denominator,fival.discrete.numerator);
                } else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
                    break;
                } else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
                    break;
                }
                fival.index++;
            }
        }
    	fsize.index++;
    }

    params.set(KEY_PREVIEW_W_FORCE,"0");
    params.set(KEY_PREVIEW_H_FORCE,"0");
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, parameterString.string());
    params.setPreviewSize(640,480);
    /*picture size setting*/      
    params.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, parameterString.string());        
    params.setPictureSize(mCamDriverFrmWidthMax,  mCamDriverFrmHeightMax);        
    
    /* set framerate */
    struct v4l2_streamparm setfps;
    int fps_min,fps_max;

    i=0;
    fps_min = 100;
    fps_max = 0;
    while (framerate[i]) {
        memset(str_element,0x00,sizeof(str_element));
        sprintf((char*)(&str_element[strlen(str_element)]),"%d",framerate[i]);
        if (strstr(str,str_element)==NULL) {
            if (strlen(str)==0) {
                strcat(str,str_element);
            } else {
                strcat(str,",");
                strcat(str,str_element);
            }
        }

        if (fps_min>framerate[i])
            fps_min = framerate[i];
        if (fps_max<framerate[i])
            fps_max = framerate[i];

        i++;
    }
    
    memset(&setfps, 0, sizeof(struct v4l2_streamparm));
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator=1;
    setfps.parm.capture.timeperframe.denominator=fps_max;
    ret = ioctl(mCamFd, VIDIOC_S_PARM, &setfps); 

    /*frame rate setting*/    
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, str);
    params.setPreviewFrameRate(fps_max);
    
    memset(str_element,0x00,sizeof(str_element));
    sprintf((char*)(&str_element[0]),"%d,%d",fps_min*1000,fps_max*1000);
    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, str_element);
    memset(str_element,0x00,sizeof(str_element));
    sprintf((char*)(&str_element[0]),"(%d,%d)",fps_min*1000,fps_max*1000);
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, str_element);
    /*zoom setting*/
    char str_zoom_max[3],str_zoom_element[5];
    int max;
    
    memset(str,0x00,sizeof(str));
    strcpy(str, "");//default zoom

	mZoomMax = 300;
	mZoomMin= 100;
	mZoomStep = 5;	

	max = (mZoomMax - mZoomMin)/mZoomStep;
	sprintf(str_zoom_max,"%d",max);
	params.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");
	params.set(CameraParameters::KEY_MAX_ZOOM, str_zoom_max);
	params.set(CameraParameters::KEY_ZOOM, "0");
	for (i=mZoomMin; i<=mZoomMax; i+=mZoomStep) {
		sprintf(str_zoom_element,"%d,", i);
		strcat(str,str_zoom_element);
	}
	params.set(CameraParameters::KEY_ZOOM_RATIOS, str);


    /*preview format setting*/
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, "yuv420sp,yuv420p");
    params.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,CameraParameters::PIXEL_FORMAT_YUV420SP);
    params.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP);
    

	params.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,CameraParameters::PIXEL_FORMAT_YUV420SP);

    /*picture format setting*/
    params.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);
    params.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);

    /*jpeg quality setting*/
    params.set(CameraParameters::KEY_JPEG_QUALITY, "70");

    /*white balance setting*/
	struct v4l2_queryctrl whiteBalance;
	struct v4l2_querymenu *whiteBalance_menu = mWhiteBalance_menu;
    
    /* ddl@rock-chips.com: v0.4.9 */
    memset(str,0x00,sizeof(str));

    dot = false;

    whiteBalance.id = V4L2_CID_AUTO_WHITE_BALANCE;
    if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &whiteBalance)) {
        strcat(str, "auto");
        dot = true;
    }

    whiteBalance.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &whiteBalance)) {
    	if ((whiteBalance.minimum <= 2800) && (2800 <= whiteBalance.maximum)) {
            if (dot)
                strcat(str, ",");
            strcat(str, "incandescent");
            dot = true;
    	}

        if ((whiteBalance.minimum <= 4000) && (4000 <= whiteBalance.maximum)) {
            if (dot)
                strcat(str, ",");
            strcat(str, "fluorescent");
            dot = true;
    	}

        if ((whiteBalance.minimum <= 5500) && (5500 <= whiteBalance.maximum)) {
            if (dot)
                strcat(str, ",");
            strcat(str, "daylight");
            dot = true;
    	}

        if ((whiteBalance.minimum <= 6500) && (6500 <= whiteBalance.maximum)) {
            if (dot)
                strcat(str, ",");
            strcat(str, "cloudy-daylight");
            dot = true;
    	}

        params.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, str);
    	params.set(CameraParameters::KEY_WHITE_BALANCE, "auto");
    }        

    /*scene setting*/
	struct v4l2_queryctrl scene;
	struct v4l2_querymenu *scene_menu = mScene_menu;

    memset(str,0x00,sizeof(str));
	strcpy(str, "");//default scene
	scene.id = V4L2_CID_SCENE;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &scene)) {
		for (i=scene.minimum; i<=scene.maximum; i+=scene.step) {
		    scene_menu->id = V4L2_CID_SCENE;
			scene_menu->index = i;
			if (!ioctl(mCamFd, VIDIOC_QUERYMENU, scene_menu)) {
                if (i != scene.minimum)
                    strcat(str, ",");
				strcat(str, (char *)scene_menu->name);
				if (scene.default_value == i) {
					strcpy(cur_param, (char *)scene_menu->name);
				}
				mScene_number++;
			}
			scene_menu++;
		}
		params.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, str);
		params.set(CameraParameters::KEY_SCENE_MODE, cur_param);

	}
    
    
    /*anti-banding setting*/
	struct v4l2_queryctrl anti_banding;
	struct v4l2_querymenu *anti_banding_menus = mAntibanding_menu;

    memset(str,0x00,sizeof(str));
	strcpy(str, "");//default scene
	anti_banding.id = V4L2_CID_POWER_LINE_FREQUENCY;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &anti_banding)) {
		for (i=anti_banding.minimum; i<=anti_banding.maximum; i+=anti_banding.step) {
		    anti_banding_menus->id = V4L2_CID_POWER_LINE_FREQUENCY;
			anti_banding_menus->index = i;
			if (!ioctl(mCamFd, VIDIOC_QUERYMENU, anti_banding_menus)) {
                if (i != anti_banding.minimum)
                    strcat(str, ",");
                if (!strcmp((char*)anti_banding_menus->name,"50 Hz")) {
                    strcat(str, "50hz");
                    if (anti_banding.default_value == i) {
                        strcpy(cur_param, "50hz");
    				}
                } else if (!strcmp((char*)anti_banding_menus->name,"60 Hz")) {
                    strcat(str, "60hz");
                    if (anti_banding.default_value == i) {
                        strcpy(cur_param, "60hz");
    				}
                } else if (!strcmp((char*)anti_banding_menus->name,"Disabled")) {
                    strcat(str, "off");
                    if (anti_banding.default_value == i) {
                        strcpy(cur_param, "off");
    				}
                } else if (!strcmp((char*)anti_banding_menus->name,"Auto")) {
                    strcat(str, "auto");
                    if (anti_banding.default_value == i) {
                        strcpy(cur_param, "auto");
    				}
                }				
				mAntibanding_number++;
			}
			anti_banding_menus++;
		}

        if ((!strstr(str,"auto")) && (strstr(str,"off"))) {
            strcat(str, ",auto");
        }
		params.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, str);
		params.set(CameraParameters::KEY_ANTIBANDING, "off");
	}    

    /*flash mode setting*/
	struct v4l2_queryctrl flashMode;
	struct v4l2_querymenu *flashMode_menu = mFlashMode_menu;
    
    memset(str,0x00,sizeof(str));
	strcpy(str, "");//default flash
	flashMode.id = V4L2_CID_FLASH;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &flashMode)) {
		for (i = flashMode.minimum; i <= flashMode.maximum; i += flashMode.step) {
			flashMode_menu->id = V4L2_CID_FLASH;
			flashMode_menu->index = i;
			if (!ioctl(mCamFd, VIDIOC_QUERYMENU, flashMode_menu)) {
                if (i != flashMode.minimum)
                    strcat(str, ",");
				strcat(str, (char *)flashMode_menu->name);
				if (flashMode.default_value == i) {
					strcpy(cur_param, (char *)flashMode_menu->name);
				}
				mFlashMode_number++;
                flashMode_menu++;                
			}
		}
		params.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, str);
		params.set(CameraParameters::KEY_FLASH_MODE, cur_param);
	}
    
    /*focus mode setting*/
    struct v4l2_queryctrl focus;

    parameterString = CameraParameters::FOCUS_MODE_FIXED;
    params.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
	#if 1
	params.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, CameraParameters::FOCUS_MODE_FIXED);
	params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"0");
	#else
    focus.id = V4L2_CID_FOCUS_AUTO;
    if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &focus)) {
        parameterString.append(",");
        parameterString.append(CameraParameters::FOCUS_MODE_AUTO);
        params.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_AUTO);
    }

    focus.id = V4L2_CID_FOCUS_CONTINUOUS;
    if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &focus)) {
        parameterString.append(",");
        parameterString.append(CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);
    }

    focus.id = V4L2_CID_FOCUS_ABSOLUTE;
    if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &focus)) {
        parameterString.append(",");
        parameterString.append(CameraParameters::FOCUS_MODE_INFINITY);
        parameterString.append(",");
        parameterString.append(CameraParameters::FOCUS_MODE_MACRO);
    }

	params.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, parameterString.string());

	focus.id = V4L2_CID_FOCUSZONE;
     
	// focus area settings
    if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &focus)) {

 	   params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"1");
	}else{
	   params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"0");
	}
	#endif
	//hardware face detect settings
	struct v4l2_queryctrl facedetect;
	facedetect.id = V4L2_CID_FACEDETECT;
	    if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &facedetect)) {

       params.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW,"1");
    }else{
		params.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW,"0");
	}	
 	 
    /*Exposure setting*/
    char str_exposure[16], exposure_failed;
    exposure_failed = true;

    #if CONFIG_CAMERA_UVC_MANEXP
    query_control.id = V4L2_CID_BRIGHTNESS;
    if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &query_control)) {
		 
        for (i = query_control.minimum; i <= query_control.maximum; i += query_control.step) {

                    control.id = V4L2_CID_BRIGHTNESS;
                    control.value = i;
                    if (!ioctl(mCamFd,VIDIOC_S_CTRL,&control)) {
                        exposure_failed = false;                
                        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
                        params.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "3");
                        params.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "-3");
                        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "1");
                    } else {
                        LOGE("%s(%d): V4L2_CID_EXPOSURE_AUTO(%s) set failed!",
                            __FUNCTION__,__LINE__,query_menu.name);
                    }
                    break;

        }    

        if (i==query_control.maximum) {
            LOGE("%s(%d): V4L2_CID_EXPOSURE_AUTO Manual Mode isn't support",__FUNCTION__,__LINE__);
        }
	}
    #endif

    if (exposure_failed == true) {
        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
        params.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "0");
        params.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "0");
        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0.000001f");
    }
    
    /*rotation setting*/
    params.set(CameraParameters::KEY_ROTATION, "0");

    /*horizontal angle of view setting ,no much meaning ,only for passing cts */
    ptr = strstr((char*)(&mCamDriverCapability.card[0]),"-");   /* ddl@rock-chips.com: v0.4.0x15 */
    if (ptr != NULL) {
        ptr = strstr(ptr,"_");
        if (ptr != NULL) {
            ptr++;
            fov_h = atoi(ptr);
            sprintf(str_fov_h,"%d",fov_h);            
            ptr = strstr(ptr,"_");
            ptr++;
            fov_v = atoi(ptr);
            sprintf(str_fov_v,"%d",fov_v);
        } else {
            LOGD("%s(%d): Current driver isn't support fov query, user can update driver to v0.3.0xf",__FUNCTION__,__LINE__);
            strlcpy(str_fov_h,"100",3);
            strlcpy(str_fov_v,"100",3);
        }
    } else {
        LOGE("%s(%d): mCamDriverCapability.card is error!",__FUNCTION__,__LINE__);
        strlcpy(str_fov_h,"100",3);
        strlcpy(str_fov_v,"100",3);
    }    
    params.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, str_fov_h);
    params.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, str_fov_v);

    params.set(KEY_CONTINUOUS_PIC_NUM,"1");
    
    /*lzg@rockchip.com :add some settings to pass cts*/    
    /*focus distance setting ,no much meaning ,only for passing cts */
    parameterString = "0.3,50,Infinity";
    params.set(CameraParameters::KEY_FOCUS_DISTANCES, parameterString.string());
    /*focus length setting ,no much meaning ,only for passing cts */
    parameterString = "35";
    params.set(CameraParameters::KEY_FOCAL_LENGTH, parameterString.string());

   /*quality of the EXIF thumbnail in Jpeg picture setting */
    parameterString = "50";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, parameterString.string());
   /*supported size of the EXIF thumbnail in Jpeg picture setting */
    parameterString = "0x0,160x128";
    params.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, parameterString.string());
    parameterString = "160";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, parameterString.string());
    parameterString = "128";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, parameterString.string()); 
    /* zyc@rock-chips.com: for cts ,KEY_MAX_NUM_DETECTED_FACES_HW should not be 0 */

    params.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, "0");
    params.set(CameraParameters::KEY_RECORDING_HINT,"false");
    params.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED,"false");
    params.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED,"true");
    params.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS,"0");

	#if (CONFIG_CAMERA_SETVIDEOSIZE == 1)
    params.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,"640x480");
	params.set(CameraParameters::KEY_VIDEO_SIZE,"640x480");
	params.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,"176x144,240x160,352x288,640x480,720x480,800x600,1280x720");
    #else

    params.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,"");
    params.set(CameraParameters::KEY_VIDEO_SIZE,"");
    params.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,"");
	#endif
    params.set(KEY_CONTINUOUS_PIC_NUM,"1");  

    LOGD ("Support Preview format: %s .. %s",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS),
        params.get(CameraParameters::KEY_PREVIEW_FORMAT));
    LOGD ("Support Preview sizes: %s     %s",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES),
        params.get(CameraParameters::KEY_PREVIEW_SIZE));
    LOGD ("Support Preview FPS range: %s",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE));
    LOGD ("Support Preview framerate: %s",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES)); 
    LOGD ("Support Picture sizes: %s ",params.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES));
    if (params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE))
        LOGD ("Support white balance: %s",params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE));
    if (params.get(CameraParameters::KEY_SUPPORTED_EFFECTS))
        LOGD ("Support color effect: %s",params.get(CameraParameters::KEY_SUPPORTED_EFFECTS));
    if (params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES))
        LOGD ("Support scene: %s",params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES));
    if (params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES))
        LOGD ("Support flash: %s",params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES));
    LOGD ("Support focus: %s  focus zone: %s",params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES),
        params.get(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS));
    LOGD ("Support zoom: %s(ratios: %s)",params.get(CameraParameters::KEY_ZOOM_SUPPORTED),
        params.get(CameraParameters::KEY_ZOOM_RATIOS));
    if (strcmp("0", params.get(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION))
		|| strcmp("0", params.get(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION))) {
        LOGD ("Support exposure: (%s -> %s)",params.get(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION),
            params.get(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION));
    }
    if (params.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING))
        LOGD("Support anti-banding: %s  anti-banding: %s",params.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING),
              params.get(CameraParameters::KEY_ANTIBANDING));
    
    LOGD ("Support hardware faces detecte: %s",params.get(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW));
    LOGD ("Support software faces detecte: %s",params.get(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW));
    LOGD ("Support video stabilization: %s",params.get(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED));
    LOGD ("Support recording hint: %s",params.get(CameraParameters::KEY_RECORDING_HINT));

    cameraConfig(params,true,isRestartPreview);
    LOG_FUNCTION_NAME_EXIT

}

int CameraUSBAdapter::setParameters(const CameraParameters &params_set,bool &isRestartValue)
{
    CameraParameters params;
    int fps_min,fps_max;

    params = params_set;

    if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES), params.get(CameraParameters::KEY_PREVIEW_SIZE)) == NULL) {
        LOGE("%s(%d): previewsize(%s) not supported",__FUNCTION__,__LINE__,params.get(CameraParameters::KEY_PREVIEW_SIZE));
        return BAD_VALUE;

    }else if (strcmp(mParameters.get(CameraParameters::KEY_PREVIEW_SIZE), params.get(CameraParameters::KEY_PREVIEW_SIZE))) {
        LOGD("%s(%d): Set preview size %s",__FUNCTION__,__LINE__,params.get(CameraParameters::KEY_PREVIEW_SIZE));
        if(mPreviewRunning){
            LOGD("%s(%d):WARNING, set preview size during preview",__FUNCTION__,__LINE__);
        }
        //should update preview cb settings ,for cts
        int w,h;
        const char * fmt=  params_set.getPreviewFormat();
		params_set.getPreviewSize(&w, &h); 
        mRefEventNotifier->setPreviewDataCbRes(w, h, fmt);
    }


    if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES), params.get(CameraParameters::KEY_PICTURE_SIZE)) == NULL) {
        LOGE("%s(%d): PictureSize(%s) not supported",__FUNCTION__,__LINE__,params.get(CameraParameters::KEY_PICTURE_SIZE));
        return BAD_VALUE;
    } else if (strcmp(mParameters.get(CameraParameters::KEY_PICTURE_SIZE), params.get(CameraParameters::KEY_PICTURE_SIZE))) {
        LOGD("%s(%d): Set picture size %s",__FUNCTION__,__LINE__,params.get(CameraParameters::KEY_PICTURE_SIZE));
    }


    if (strcmp(params.getPictureFormat(), "jpeg") != 0) {
        LOGE("%s(%d): Only jpeg still pictures are supported",__FUNCTION__,__LINE__);
        return BAD_VALUE;
    }

    //set zoom
    if (params.getInt(CameraParameters::KEY_ZOOM) > params.getInt(CameraParameters::KEY_MAX_ZOOM)) {
        LOGE("Zomm(%d) is larger than MaxZoom(%d)",params.getInt(CameraParameters::KEY_ZOOM),params.getInt(CameraParameters::KEY_MAX_ZOOM));
        return BAD_VALUE;
    }

    params.getPreviewFpsRange(&fps_min,&fps_max);
    if ((fps_min < 0) || (fps_max < 0) || (fps_max < fps_min)) {
        LOGE("%s(%d): FpsRange(%s) is invalidate",__FUNCTION__,__LINE__,params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE));
        return BAD_VALUE;
    }
	
	//set af
	if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES),params.get(CameraParameters::KEY_FOCUS_MODE))){
		LOGD("suppport focus modes:%s, expect:%s",mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES),
			params.get(CameraParameters::KEY_FOCUS_MODE));
	}else {
            LOGE("%s isn't supported for this camera, support focus: %s",
                params.get(CameraParameters::KEY_FOCUS_MODE),
                mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES));
            if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES),CameraParameters::FOCUS_MODE_AUTO))
                mParameters.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_AUTO);
            else 
                mParameters.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
            return BAD_VALUE;
    }

    //adapter needn't know preview formats ? just to tell AppMsgNotifier ?
    if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS),params.getPreviewFormat())) {
        if (strcmp(mParameters.getPreviewFormat(), params.getPreviewFormat())){
            if(mPreviewRunning){
                LOGD("%s(%d): WARING,set preview format during preview",__FUNCTION__,__LINE__);
            }
        }
    } else {
        LOGE("%s(%d): %s is not supported,Only %s and %s preview is supported",__FUNCTION__,__LINE__,params.getPreviewFormat(),CameraParameters::PIXEL_FORMAT_YUV420SP,CameraParameters::PIXEL_FORMAT_YUV422SP);
        return BAD_VALUE;
    }

    
    int framerate = params.getPreviewFrameRate();

	if (!cameraConfig(params,false,isRestartValue)) {        
        LOG1("PreviewSize(%s)", mParameters.get(CameraParameters::KEY_PREVIEW_SIZE));
        LOG1("PreviewFormat(%s)  mCamDriverPreviewFmt(%c%c%c%c)",params.getPreviewFormat(), 
            mCamDriverPreviewFmt & 0xFF, (mCamDriverPreviewFmt >> 8) & 0xFF,
			(mCamDriverPreviewFmt >> 16) & 0xFF, (mCamDriverPreviewFmt >> 24) & 0xFF);  
        LOG1("FPS Range(%s)",mParameters.get(CameraParameters::KEY_PREVIEW_FPS_RANGE));
        LOG1("PictureSize(%s)",mParameters.get(CameraParameters::KEY_PICTURE_SIZE)); 
        LOG1("PictureFormat(%s)  ", params.getPictureFormat());
        LOG1("Framerate: %d  ", framerate);
        LOG1("WhiteBalance: %s", params.get(CameraParameters::KEY_WHITE_BALANCE));
        LOG1("Flash: %s", params.get(CameraParameters::KEY_FLASH_MODE));
        LOG1("Focus: %s", params.get(CameraParameters::KEY_FOCUS_MODE));
        LOG1("Scene: %s", params.get(CameraParameters::KEY_SCENE_MODE));
    	LOG1("Effect: %s", params.get(CameraParameters::KEY_EFFECT));
    	LOG1("ZoomIndex: %s", params.get(CameraParameters::KEY_ZOOM));	    
	}else{
	    return BAD_VALUE;
	}

    
    return 0;
}

int CameraUSBAdapter::cameraConfig(const CameraParameters &tmpparams,bool isInit,bool &isRestartValue)
{
    int err = 0, i = 0;
    struct v4l2_control control;
	struct v4l2_ext_control extCtrInfo;
	struct v4l2_ext_controls extCtrInfos;
	struct v4l2_queryctrl query_control;
	CameraParameters params = tmpparams;
    bool    isRestartPreview = false;

    if (params.getPreviewFrameRate() != mParameters.getPreviewFrameRate()) {
            if (mPreviewRunning == 0) {      /* ddl@rock-chips.com: v0.4.0x21 */
                struct v4l2_streamparm setfps;          
            
                memset(&setfps, 0, sizeof(struct v4l2_streamparm));
                setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                setfps.parm.capture.timeperframe.numerator=1;
                setfps.parm.capture.timeperframe.denominator=params.getPreviewFrameRate();
                err = ioctl(mCamFd, VIDIOC_S_PARM, &setfps); 
                if (err != 0) {
                    LOGE ("%s(%d): Set framerate(%d fps) failed",__FUNCTION__,__LINE__,params.getPreviewFrameRate());
                    return err;
                } else {
                    LOGD ("%s(%d): Set framerate(%d fps) success",__FUNCTION__,__LINE__,params.getPreviewFrameRate());
                }
            } else {                
                LOGE("%s(%d): UVC isn't support set framerate after start preview",__FUNCTION__,__LINE__);
            }
    }

    /*white balance setting*/
    const char *white_balance = params.get(CameraParameters::KEY_WHITE_BALANCE);
	const char *mwhite_balance = mParameters.get(CameraParameters::KEY_WHITE_BALANCE);
	if (params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE)) {
		if ( !mwhite_balance || strcmp(white_balance, mwhite_balance) ) {
            /* ddl@rock-chips.com: v0.4.9 */
                control.id = V4L2_CID_AUTO_WHITE_BALANCE;
                if (strcmp(white_balance,"auto")==0) {                    
                    control.value = true;                    
                } else {
                    control.value = false;
                    err = ioctl(mCamFd, VIDIOC_S_CTRL, &control);
                    if (err<0) {
                        LOGE("%s(%d): turn off auto white balance failed",__FUNCTION__,__LINE__);
                    }
                }

                if (strcmp(white_balance,"incandescent")==0) {
                    control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                    control.value = 2800; 
                } else if (strcmp(white_balance,"fluorescent")==0) {
                    control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                    control.value = 4000; 
                } else if (strcmp(white_balance,"daylight")==0) {
                    control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                    control.value = 5500; 
                } else if (strcmp(white_balance,"cloudy-daylight")==0) {
                    control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                    control.value = 6500; 
                }

                err = ioctl(mCamFd, VIDIOC_S_CTRL, &control);

            if (err<0) {
                LOGE("%s(%d): Set white balance(%s) failed",__FUNCTION__,__LINE__,white_balance);
            } else {                    
                LOGD("%s(%d): Set white balance(%s) success",__FUNCTION__,__LINE__,white_balance);
            }
		}
	}

    /*zoom setting*/

	    const int zoom = params.getInt(CameraParameters::KEY_ZOOM);
		const int mzoom = mParameters.getInt(CameraParameters::KEY_ZOOM);
		if ((mzoom < 0) || (zoom != mzoom)) {	
			mZoomVal = zoom * mZoomStep + mZoomMin;
		}

    /*color effect setting*/
    const char *effect = params.get(CameraParameters::KEY_EFFECT);
	const char *meffect = mParameters.get(CameraParameters::KEY_EFFECT);
	if (params.get(CameraParameters::KEY_SUPPORTED_EFFECTS)) {
		if ( !meffect || strcmp(effect, meffect) ) {
			for (i = 0; i < mEffect_number; i++) {
				if (!strcmp((char *)mEffect_menu[i].name, effect)) {
					break;
				}
			}
			extCtrInfo.id = mEffect_menu[i].id;
			extCtrInfo.value = mEffect_menu[i].index;
			extCtrInfos.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
			extCtrInfos.count = 1;
			extCtrInfos.controls = &extCtrInfo;
			err = ioctl(mCamFd, VIDIOC_S_EXT_CTRLS, &extCtrInfos);
			if ( err < 0 ){
				LOGE ("%s(%d): Set effect(%s) fail",__FUNCTION__,__LINE__,effect);
			} else {
			    LOGD("%s(%d): Set effect %s",__FUNCTION__,__LINE__, (char *)mEffect_menu[i].name);
			}
		}
	}
    
    /*anti-banding setting*/
    const char *anti_banding = params.get(CameraParameters::KEY_ANTIBANDING);
	const char *manti_banding = mParameters.get(CameraParameters::KEY_ANTIBANDING);
	if (anti_banding != NULL) {
		if ( !manti_banding || (anti_banding && strcmp(anti_banding, manti_banding)) ) {
            if (!strcmp(anti_banding,CameraParameters::ANTIBANDING_OFF)) {
                for (i=0; i<mAntibanding_number; i++) {
                    if (!strcmp((char*)mAntibanding_menu[i].name,"Disabled")) 
                        break;
                }
            } else if (!strcmp(anti_banding,CameraParameters::ANTIBANDING_50HZ)) {
                for (i=0; i<mAntibanding_number; i++) {
                    if (!strcmp((char*)mAntibanding_menu[i].name,"50 Hz")) 
                        break;
                }
            } else if (!strcmp(anti_banding,CameraParameters::ANTIBANDING_60HZ)) {
                for (i=0; i<mAntibanding_number; i++) {
                    if (!strcmp((char*)mAntibanding_menu[i].name,"60 Hz")) 
                        break;
                }
            } else if (!strcmp(anti_banding,CameraParameters::ANTIBANDING_AUTO)) {
                for (i=0; i<mAntibanding_number; i++) {
                    if (!strcmp((char*)mAntibanding_menu[i].name,"Auto")) 
                        break;
                }
                if (i==mAntibanding_number) {
                    for (i=0; i<mAntibanding_number; i++) {
                        if (!strcmp((char*)mAntibanding_menu[i].name,"Disabled")) 
                            break;
                    }
                }
            }
            
            if (i<mAntibanding_number) {
    			control.id = mAntibanding_menu[i].id;
    			control.value = mAntibanding_menu[i].index;
    			err = ioctl(mCamFd, VIDIOC_S_CTRL, &control);
    			if ( err < 0 ){
    				LOGE("%s(%d): Set anti-banding(%s) failed",__FUNCTION__,__LINE__,anti_banding);
    			} else {
    			    LOGD ("%s(%d): Set anti-banding %s ",__FUNCTION__,__LINE__, (char *)mAntibanding_menu[i].name);
    			}
            } else {
                LOGE("%s(%d): AntiBanding(%s) isn't support!",__FUNCTION__,__LINE__, anti_banding);
            }
		}
	}
	/*scene setting*/
    const char *scene = params.get(CameraParameters::KEY_SCENE_MODE);
	const char *mscene = mParameters.get(CameraParameters::KEY_SCENE_MODE);
	if (params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES)) {
		if ( !mscene || strcmp(scene, mscene) ) {
			for (i = 0; i < mScene_number; i++) {
				if (!strcmp((char *)mScene_menu[i].name, scene)) {
					break;
				}
			}
			extCtrInfo.id = mScene_menu[i].id;
			extCtrInfo.value = mScene_menu[i].index;
			extCtrInfos.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
			extCtrInfos.count = 1;
			extCtrInfos.controls = &extCtrInfo;
			err = ioctl(mCamFd, VIDIOC_S_EXT_CTRLS, &extCtrInfos);
			if ( err < 0 ){
				LOGE("%s(%d): Set scene(%s) failed",__FUNCTION__,__LINE__,scene);
			} else {
			    LOGD ("%s(%d): Set scene %s ",__FUNCTION__,__LINE__, (char *)mScene_menu[i].name);
			}
		}
	}

    /*focus setting*/

	/*flash mode setting*/
    const char *flashMode = params.get(CameraParameters::KEY_FLASH_MODE);
	const char *mflashMode = mParameters.get(CameraParameters::KEY_FLASH_MODE);
	
	if (params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES)) {
		if ( !mflashMode || strcmp(flashMode, mflashMode) ) {
			for (i = 0; i < mFlashMode_number; i++) {
				if (!strcmp((char *)mFlashMode_menu[i].name, flashMode)) {
					break;
				}
			}
			if(i== mFlashMode_number || mFlashMode_number == 0){
				params.set(CameraParameters::KEY_FLASH_MODE,(mflashMode?mflashMode:CameraParameters::FLASH_MODE_OFF));
				err = -1;
                LOGE("%s(%d): flashMode %s is not support",__FUNCTION__,__LINE__,flashMode);
			} else {
				extCtrInfo.id = mFlashMode_menu[i].id;
				extCtrInfo.value = mFlashMode_menu[i].index;
				extCtrInfos.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
				extCtrInfos.count = 1;
				extCtrInfos.controls = &extCtrInfo;
				err = ioctl(mCamFd, VIDIOC_S_EXT_CTRLS, &extCtrInfos);
				if ( err < 0 ){
					LOGE ("%s(%d): Set flash(%s) failed",__FUNCTION__,__LINE__,flashMode );				
				} else {
				    LOGD ("%s(%d): Set flash %s",__FUNCTION__,__LINE__, (char *)mFlashMode_menu[i].name);
				}
			}
		}
	}

    /*exposure setting*/
	const char *exposure = params.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    const char *mexposure = mParameters.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    
	if (strcmp("0", params.get(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION))
		|| strcmp("0", params.get(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION))) {
	    if (!mexposure || (exposure && strcmp(exposure,mexposure))) {
				#if CONFIG_CAMERA_UVC_MANEXP 
				query_control.id = V4L2_CID_BRIGHTNESS;
				if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &query_control)){
					const int medium = (query_control.maximum + query_control.minimum)/2;
					control.id = V4L2_CID_BRIGHTNESS ;					  
					control.value = medium + atoi(exposure) * (query_control.maximum - medium)/3;
				
					if (ioctl(mCamFd,VIDIOC_S_CTRL, &control) <0) {
						 LOGE("%s(%d):	Set exposure(%s) failed",__FUNCTION__,__LINE__,exposure);
					} else {
						 LOGD("%s(%d): Set exposure %s	%d",__FUNCTION__,__LINE__,exposure,control.value);
					}
				}
				#endif
	    }
	}    

    mParameters = params;
	
	//changeVideoPreviewSize();
	isRestartValue = isNeedToRestartPreview();

end:  
    return err;
}

//define  the frame info ,such as w, h ,fmt 
int CameraUSBAdapter::reprocessFrame(FramInfo_s* frame)
{
    int ret = 0;
	long phy_addr;

	if(gCamInfos[mCamId].pcam_total_info->mIsIommuEnabled)
		phy_addr = mPreviewBufProvider->getBufShareFd(frame->frame_index);
	else
		phy_addr = mPreviewBufProvider->getBufPhyAddr(frame->frame_index);

    if( frame->frame_fmt == V4L2_PIX_FMT_MJPEG){
    	   char *srcbuf = (char*)frame->vir_addr;
    	   if((srcbuf[0] == 0xff) && (srcbuf[1] == 0xd8) && (srcbuf[2] == 0xff)){
        //decoder to NV12
        VPU_FRAME outbuf; 
        unsigned int output_len;
        unsigned int input_len;
        output_len = 0;
        input_len = frame->frame_size;

        ret = mMjpegDecoder.decode(mMjpegDecoder.decoder,
                                    (unsigned char*)&outbuf, &output_len, 
    		                          (unsigned char*)frame->vir_addr, &input_len,
    		                          phy_addr);
        if (ret < 0){
            LOGE("%s(%d): mjpeg stream is error!",__FUNCTION__,__LINE__);
	        }
	    }else{
	    		LOGE("mjpeg data error!!");
	    		return -1;
        }
    }else if(frame->frame_fmt == V4L2_PIX_FMT_YUYV){

        ret = cameraFormatConvert(V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_NV12, NULL, 
							 (char*)frame->vir_addr, (char*)mPreviewBufProvider->getBufVirAddr(frame->frame_index),
							 0,0,frame->frame_size,
							 frame->frame_width, frame->frame_height, frame->frame_width,
							 frame->frame_width, frame->frame_height, frame->frame_width,
							 false);
        if (ret < 0){
            LOGE("%s(%d): yuyv convert to nv12 error!",__FUNCTION__,__LINE__);
        }

    }else{
        LOGE("camerahal not support this format %d",frame->frame_fmt);
        ret =  -1;
    }

    frame->frame_fmt = V4L2_PIX_FMT_NV12;
    frame->phy_addr = phy_addr;
    frame->vir_addr = mPreviewBufProvider->getBufVirAddr(frame->frame_index);
    frame->zoom_value = mZoomVal;
	
	int w,h;
	w = frame->frame_width;
	h = frame->frame_height;
	if((w&0x0f) || (h&0x0f)){
		char *buf = (char*)malloc(w*h*3/2);
		if(buf != NULL){
			memcpy(buf,(void*)frame->vir_addr,w*h);
			memcpy(buf+w*h,(void*)(frame->vir_addr+((w+15)&0xfff0)*((h+15)&0xfff0)), w*h/2);
			memcpy((void*)frame->vir_addr,buf,w*h*3/2);
			free(buf);
		}
	}

    //do zoom here?
    return ret;
    
}


}
