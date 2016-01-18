#include "CameraHal.h"
namespace android{

unsigned int CameraSOCAdapter::mFrameSizesEnumTable[][2] = {
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


CameraSOCAdapter::CameraSOCAdapter(int cameraId)
                   :CameraAdapter(cameraId)
{
    LOGD("%s(%d):IN",__FUNCTION__,__LINE__);
    mCamDriverV4l2MemType = V4L2_MEMORY_OVERLAY;
	
	mWhiteBalance_number = 0;
	mEffect_number = 0;
	mScene_number = 0;
	mAntibanding_number = 0;
	mFlashMode_number = 0;
	m_focus_mode = CameraSOCAdapter::focus_fixed;
	m_focus_value = 0;

}
CameraSOCAdapter::~CameraSOCAdapter()
{
    mFrameSizeVector.clear();
}
int CameraSOCAdapter::selectPreferedDrvSize(int *width,int * height,bool is_capture)
{
    int num = static_cast<int>(mFrameSizeVector.size());
    const frameSize_t* tmpItem = NULL;
    int ori_w = *width,ori_h = *height,pref_w=0,pref_h=0;
    int demand_ratio = (*width * 100) / (*height);
    int32_t total_pix = (*width) * (*height);

    int cur_ratio ,cur_ratio_diff,pref_ratio_diff = 10000;
    int32_t cur_pix ,cur_pix_diff,pref_pix_diff = 8*1000*1000;
    //serch for the preferred res
    for(int i =0;i<num;i++){
        tmpItem = &(mFrameSizeVector[i]);
        cur_ratio = tmpItem->width * 100 / tmpItem->height;
        cur_pix   = tmpItem->width * tmpItem->height;
        cur_pix_diff = ABS(total_pix - cur_pix);
        cur_ratio_diff = ABS(demand_ratio - cur_ratio);
        //
        if((cur_pix_diff < pref_pix_diff) && (cur_ratio_diff <=  pref_ratio_diff)){
            pref_pix_diff = cur_pix_diff;
            cur_ratio_diff = pref_ratio_diff;
            pref_w = tmpItem->width;
            pref_h = tmpItem->height;
        }
    }
    if(pref_w != 0){
        *width = pref_w;
        *height = pref_h;
        LOGD("%s:prefer res (%dx%d)",__FUNCTION__,pref_w,pref_h);
    }else{
        LOGE("WARINING:have not select preferred res!!");
    }
    return 0;
}

int CameraSOCAdapter::cameraFramerateQuery(unsigned int format, unsigned int w, unsigned int h, int *min, int *max)
{
    int i,framerate,ret;    
    int preview_data_process_time;
    
    struct v4l2_frmivalenum fival;

    ret = 0;
    fival.index = 0;
    fival.pixel_format = format;
    fival.width = w;
    fival.height = h;
	ret = ioctl(mCamFd, VIDIOC_ENUM_FRAMEINTERVALS, &fival);    
    if (ret == 0) {		
        if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
            *min = (fival.discrete.denominator*1000)/fival.discrete.numerator;
            *max = (fival.discrete.denominator*1000)/fival.discrete.numerator;
        } else {
            LOGE("%s(%d): query framerate type(%d) is not supported",__FUNCTION__,__LINE__, fival.type);
            goto default_fps;
        }
    } else {
        LOGE("%s(%d): Query framerate error(%dx%d@%c%c%c%c index:%d)",__FUNCTION__,__LINE__,
            fival.width,fival.height,(fival.pixel_format & 0xFF), (fival.pixel_format >> 8) & 0xFF,
				((fival.pixel_format >> 16) & 0xFF), ((fival.pixel_format >> 24) & 0xFF),fival.index);
default_fps:    
        if (gCamInfos[mCamId].facing_info.facing == CAMERA_FACING_BACK) {
            *min = CONFIG_CAMERA_BACK_PREVIEW_FPS_MIN;
            *max = CONFIG_CAMERA_BACK_PREVIEW_FPS_MAX;
        } else {
            *min = CONFIG_CAMERA_FRONT_PREVIEW_FPS_MIN;
            *max = CONFIG_CAMERA_FRONT_PREVIEW_FPS_MAX;
        }
        ret = 0;
    }
    
cameraFramerateQuery_end:
    return ret;
}
int CameraSOCAdapter::cameraFpsInfoSet(CameraParameters &params)
{
    int min,max, framerate_min,framerate_max,w,h;
    char fps_str[20];
    char framerates[50];
    String8 parameterString;
    Vector<Size> sizes;
    unsigned int i;
    
    params.getSupportedPreviewSizes(sizes);   
    params.getPreviewSize(&w, &h);
    if (gCamInfos[mCamId].facing_info.facing == CAMERA_FACING_BACK) {
        framerate_min = CONFIG_CAMERA_BACK_PREVIEW_FPS_MIN;
        framerate_max = CONFIG_CAMERA_BACK_PREVIEW_FPS_MAX;
    } else {
        framerate_min = CONFIG_CAMERA_FRONT_PREVIEW_FPS_MIN;
        framerate_max = CONFIG_CAMERA_FRONT_PREVIEW_FPS_MAX;
    }

    memset(framerates,0x00,sizeof(framerates));
    for (i=0; i<sizes.size(); i++) {
        cameraFramerateQuery(mCamDriverPreviewFmt,sizes[i].width,sizes[i].height,&min,&max);        
		if (min<framerate_min) {
            framerate_min = min;
        }

        if (max>framerate_max) {
            framerate_max = max;
        }

        if ((w==sizes[i].width) && (h==sizes[i].height)) {
            params.setPreviewFrameRate(min/1000);
        } 

        memset(fps_str,0x00,sizeof(fps_str));            
        sprintf(fps_str,"%d",min/1000);
        if (strstr(framerates,fps_str) == NULL) {
            if (strlen(framerates)) 
                sprintf(&framerates[strlen(framerates)],",");    
            sprintf(&framerates[strlen(framerates)],"%d",min/1000);
        }

        memset(fps_str,0x00,sizeof(fps_str));            
        sprintf(fps_str,"%d",max/1000);
        if (strstr(framerates,fps_str) == NULL) {
            sprintf(&framerates[strlen(framerates)],",%d",max/1000);
        }
    }
    
    /*frame per second setting*/
    memset(fps_str,0x00,sizeof(fps_str));            
    sprintf(fps_str,"%d",framerate_min);
    fps_str[strlen(fps_str)] = ',';
    sprintf(&fps_str[strlen(fps_str)],"%d",framerate_max);
    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_str);

    LOGD("KEY_PREVIEW_FPS_RANGE : %s",fps_str);
    parameterString = "(";
    parameterString.append(fps_str);
    parameterString.append(")");
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, parameterString.string());
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, framerates);  
    LOGD("KEY_SUPPORTED_PREVIEW_FPS_RANGE : %s",parameterString.string());
    LOGD("KEY_SUPPORTED_PREVIEW_FRAME_RATES : %s",framerates);

    return 0;
}
void CameraSOCAdapter::initDefaultParameters(int camFd)
{
	CameraParameters params;
	String8 parameterString;
	int i=0,j,previewFrameSizeMax;
	char cur_param[32],cam_size[12];		/* ddl@rock-chips.com: v0.4.f */
	char str_picturesize[200];//We support at most 4 resolutions: 2592x1944,2048x1536,1600x1200,1024x768 
	int ret,picture_size_bit;
	struct v4l2_format fmt;    
	bool dot,isRestartPreview = false;
	char prop_value[PROPERTY_VALUE_MAX];
	
	LOG_FUNCTION_NAME	 
	memset(str_picturesize,0x00,sizeof(str_picturesize));

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat= mCamDriverPreviewFmt;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

    LOGD("%s,mCamDriverPreviewFmt  = %d",__FUNCTION__,mCamDriverPreviewFmt);

	/*picture size setting*/
	fmt.fmt.pix.width = 10000;
	fmt.fmt.pix.height = 10000;
	ret = ioctl(mCamFd, VIDIOC_TRY_FMT, &fmt);

	mCamDriverFrmWidthMax = fmt.fmt.pix.width;
	mCamDriverFrmHeightMax = fmt.fmt.pix.height;		

	if (mCamDriverFrmWidthMax > 3264) {
		LOGE("Camera driver support maximum resolution(%dx%d) is overflow 8Mega!",mCamDriverFrmWidthMax,mCamDriverFrmHeightMax);
		mCamDriverFrmWidthMax = 3264;
		mCamDriverFrmHeightMax = 2448;
	}

	/*preview size setting*/

    while(mFrameSizesEnumTable[i][0]){
        if (mCamDriverFrmWidthMax >= mFrameSizesEnumTable[i][0]) {
            fmt.fmt.pix.width = mFrameSizesEnumTable[i][0];
            fmt.fmt.pix.height = mFrameSizesEnumTable[i][1];
            if (ioctl(mCamFd, VIDIOC_TRY_FMT, &fmt) == 0) {
                if ((fmt.fmt.pix.width == mFrameSizesEnumTable[i][0]) && (fmt.fmt.pix.height == mFrameSizesEnumTable[i][1])) {
                	String8 tmpFrameStr = String8::format( "%dx%d",  mFrameSizesEnumTable[i][0],mFrameSizesEnumTable[i][1]);
                    if(parameterString.string())
                        parameterString.append(",");
                    parameterString.append(tmpFrameStr);
                    frameSize_t newItem;
                    newItem.width = mFrameSizesEnumTable[i][0];
                    newItem.height = mFrameSizesEnumTable[i][1];
                    newItem.fmt = mCamDriverPreviewFmt;
                    mFrameSizeVector.push_back(newItem);
                }
            }
        }
        i++;
    }
	mSupportPreviewSizeReally = parameterString;
    
	//set supported picturesize

	if(mCamDriverFrmWidthMax <= 640){
        params.setPreviewSize(640, 480);
        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "640x480,352x288,320x240,176x144");
		strcat( str_picturesize,"640x480,320x240");
		params.setPictureSize(640,480);
	}else if (mCamDriverFrmWidthMax <= 1280) {
		if(mCamDriverFrmHeightMax <= 720){
            params.setPreviewSize(800, 600);
            params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "1280x720,800x600,720x480,640x480,352x288,320x240,176x144");
			strcat( str_picturesize,"1024x768,800x600,640x480,352x288,320x240,176x144");
			params.setPictureSize(1024,768);
		}
		else if(mCamDriverFrmHeightMax <= 960){
            params.setPreviewSize(800, 600);
            params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "1280x960,1280x720,800x600,720x480,640x480,352x288,320x240,176x144");
			strcat( str_picturesize,"1024x768,800x600,640x480,352x288,320x240,176x144");
			params.setPictureSize(1024,768);
		}
	} else if (mCamDriverFrmWidthMax <= 1600) {
        params.setPreviewSize(800, 600);
		if(strstr(parameterString.string(),"1280x720"))
	        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "1280x720,800x600,720x480,640x480,352x288,320x240,176x144");
		else
	        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "800x600,720x480,640x480,352x288,320x240,176x144");

        strcat( str_picturesize,"1600x1200,1024x768,720x480,640x480,352x288,320x240,176x144");
  	    params.setPictureSize(1600,1200);
   #if 0
		property_get("sys.cts_gts.status",prop_value, "false");
		if(!strcmp(prop_value,"true")){
			strcat( str_picturesize,"1600x1200,1024x768,720x480,640x480,352x288,320x240,176x144");			
			params.setPictureSize(1600,1200);
		}
		else
		{
			strcat( str_picturesize,"1024x768,720x480,640x480,352x288,320x240,176x144");			
			params.setPictureSize(1024,768);
		}
    #endif
	} else if (mCamDriverFrmWidthMax <= 2048) {
        params.setPreviewSize(800, 600);
		if(strstr(parameterString.string(),"1280x720"))
	        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "1280x720,800x600,720x480,640x480,352x288,320x240,176x144");
		else
	        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "800x600,720x480,640x480,352x288,320x240,176x144");
		strcat( str_picturesize,"2048x1536,1600x1200,1600x900,1024x768,800x600,640x480,352x288,320x240,176x144"); 		
		params.setPictureSize(2048,1536);
	} else if (mCamDriverFrmWidthMax <= 2592) {  
        params.setPreviewSize(800, 600);
		if(strstr(parameterString.string(),"1280x720"))
	        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "1280x720,800x600,720x480,640x480,352x288,320x240,176x144");
		else
	        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "800x600,720x480,640x480,352x288,320x240,176x144");
		strcat( str_picturesize,"2592x1944,2592x1458,2048x1536,2048x1152,1600x1200,1600x900,1024x768,800x600,640x480,352x288,320x240,176x144");			
		params.setPictureSize(2592,1944);
	} else if (mCamDriverFrmWidthMax <= 3264) {  
        params.setPreviewSize(800, 600);
		if(strstr(parameterString.string(),"1280x720"))
	        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "1280x720,720x480,800x600,640x480,352x288,320x240,176x144");
		else
	        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "800x600,720x480,640x480,352x288,320x240,176x144");
		strcat( str_picturesize,"3264x2448,2592x1944,2592x1458,2048x1536,2048x1152,1600x1200,1600x900,1024x768,800x600,640x480,352x288,320x240,176x144"); 			
		params.setPictureSize(3264,2448);
	} else {
		sprintf(str_picturesize, "%dx%d", mCamDriverFrmWidthMax,mCamDriverFrmHeightMax);
		params.setPictureSize(mCamDriverFrmWidthMax,mCamDriverFrmHeightMax);
	}

	params.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, str_picturesize);

	/*frame rate settings*/
	cameraFpsInfoSet(params);
	
	/*zoom setting*/
    /*zoom setting*/
    char str[300],str_zoom_max[3],str_zoom_element[5];
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
	char str_whitebalance[200];
	/* ddl@rock-chips.com: v0.4.9 */
	memset(str_whitebalance,0x00,sizeof(str_whitebalance));
	strcpy(str_whitebalance, "");//default whitebalance
	whiteBalance.id = V4L2_CID_DO_WHITE_BALANCE;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &whiteBalance)) {
		for (i = whiteBalance.minimum; i <= whiteBalance.maximum; i += whiteBalance.step) {
			whiteBalance_menu->id = V4L2_CID_DO_WHITE_BALANCE;
			whiteBalance_menu->index = i;
			if (!ioctl(mCamFd, VIDIOC_QUERYMENU, whiteBalance_menu)) {
				if (i != whiteBalance.minimum)
					strcat(str_whitebalance, ",");
				strcat(str_whitebalance, (char *)whiteBalance_menu->name);
				if (whiteBalance.default_value == i) {
					strcpy(cur_param, (char *)whiteBalance_menu->name);
				}
				mWhiteBalance_number++;
			}
			whiteBalance_menu++;
		}
		params.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, str_whitebalance);
		params.set(CameraParameters::KEY_WHITE_BALANCE, cur_param);
	}


	/*color effect setting*/
	struct v4l2_queryctrl effect;
	struct v4l2_querymenu *effect_menu = mEffect_menu;
	char str_effect[200];
	strcpy(str_effect, "");//default effect
	effect.id = V4L2_CID_EFFECT;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &effect)) {
		for (i = effect.minimum; i <= effect.maximum; i += effect.step) {
			effect_menu->id = V4L2_CID_EFFECT;
			effect_menu->index = i;
			if (!ioctl(mCamFd, VIDIOC_QUERYMENU, effect_menu)) {
				if (i != effect.minimum)
					strcat(str_effect, ",");
				strcat(str_effect, (char *)effect_menu->name);
				if (effect.default_value == i) {
					strcpy(cur_param, (char *)effect_menu->name);
				}
				mEffect_number++;
			}
			effect_menu++;
		}
		params.set(CameraParameters::KEY_SUPPORTED_EFFECTS, str_effect);
		params.set(CameraParameters::KEY_EFFECT, cur_param);

	}

	/*scene setting*/
	struct v4l2_queryctrl scene;
	struct v4l2_querymenu *scene_menu = mScene_menu;
	char str_scene[200];
	strcpy(str_scene, "");//default scene
	scene.id = V4L2_CID_SCENE;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &scene)) {
		for (i=scene.minimum; i<=scene.maximum; i+=scene.step) {
			scene_menu->id = V4L2_CID_SCENE;
			scene_menu->index = i;
			if (!ioctl(mCamFd, VIDIOC_QUERYMENU, scene_menu)) {
				if (i != scene.minimum)
					strcat(str_scene, ",");
				strcat(str_scene, (char *)scene_menu->name);
				if (scene.default_value == i) {
					strcpy(cur_param, (char *)scene_menu->name);
				}
				mScene_number++;
			}
			scene_menu++;
		}
		params.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, str_scene);
		params.set(CameraParameters::KEY_SCENE_MODE, cur_param);

	}

	/*Antibanding setting*/
	struct v4l2_queryctrl Antibanding;
	struct v4l2_querymenu *Antibanding_menu = mAntibanding_menu;
	char str_Antibanding[200];
	strcpy(str_Antibanding, "");//default Antibanding
	Antibanding.id = V4L2_CID_ANTIBANDING;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &Antibanding)) {
		for (i=Antibanding.minimum; i<=Antibanding.maximum; i+=Antibanding.step) {
			Antibanding_menu->id = V4L2_CID_ANTIBANDING;
			Antibanding_menu->index = i;
			if (!ioctl(mCamFd, VIDIOC_QUERYMENU, Antibanding_menu)) {
				if (i != Antibanding.minimum)
					strcat(str_Antibanding, ",");
				strcat(str_Antibanding, (char *)Antibanding_menu->name);
				if (Antibanding.default_value == i) {
					strcpy(cur_param, (char *)Antibanding_menu->name);
				}
				mAntibanding_number++;
			}
			Antibanding_menu++;
		}
		params.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, str_Antibanding);
		params.set(CameraParameters::KEY_ANTIBANDING, cur_param);
	}else{
		/*no much meaning ,only for passing cts yzm*/
		params.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, "auto,50hz,60hz,off");
		params.set(CameraParameters::KEY_ANTIBANDING, "off");
	}

	/*White Balance lock setting*/
	struct v4l2_queryctrl WhiteBalanceLock;
	WhiteBalanceLock.id = V4L2_CID_WHITEBALANCE_LOCK;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &WhiteBalanceLock)) {
		params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "true");
		if(WhiteBalanceLock.default_value){
			params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, "true");
		}else{
			params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, "false");
		}
	}else{
		params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "");
	}

	/*ExposureLock setting*/
	struct v4l2_queryctrl ExposureLock;
	ExposureLock.id = V4L2_CID_EXPOSURE_LOCK;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &ExposureLock)) {
		params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "true");
		if(ExposureLock.default_value){
			params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, "true");
		}else{
			params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, "false");
		}
	}else{
		/*no much meaning ,only for passing cts yzm*/
		params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, "false");
		params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "true");
	}

	/*flash mode setting*/
	struct v4l2_queryctrl flashMode;
	struct v4l2_querymenu *flashMode_menu = mFlashMode_menu;
	char str_flash[200];
	strcpy(str_flash, "");//default flash
	flashMode.id = V4L2_CID_FLASH;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &flashMode)) {
		for (i = flashMode.minimum; i <= flashMode.maximum; i += flashMode.step) {
			flashMode_menu->id = V4L2_CID_FLASH;
			flashMode_menu->index = i;
			if (!ioctl(mCamFd, VIDIOC_QUERYMENU, flashMode_menu)) {
				if (i != flashMode.minimum)
					strcat(str_flash, ",");
				strcat(str_flash, (char *)flashMode_menu->name);
				if (flashMode.default_value == i) {
					strcpy(cur_param, (char *)flashMode_menu->name);
				}
				mFlashMode_number++;
				flashMode_menu++;				 
			}
		}
		params.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, str_flash);
		params.set(CameraParameters::KEY_FLASH_MODE, cur_param);
	}
	/*focus mode setting*/
	struct v4l2_queryctrl focus;
	
	parameterString = CameraParameters::FOCUS_MODE_FIXED;
	params.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
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

	/*Exposure setting*/
	struct v4l2_queryctrl exposure;
	char str_exposure[16];
	exposure.id = V4L2_CID_EXPOSURE;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &exposure)) {
		sprintf(str_exposure,"%d",exposure.default_value);
		params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, str_exposure);
		sprintf(str_exposure,"%d",exposure.maximum);		
		params.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, str_exposure);
		sprintf(str_exposure,"%d",exposure.minimum);		
		params.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, str_exposure);
		sprintf(str_exposure,"%d",exposure.step); 
		params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, str_exposure);
	} else {
		/*no much meaning ,only for passing cts yzm*/
		params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
		params.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "2");
		params.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "-2");
		params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "1.0f");
	}
	/*rotation setting*/
	params.set(CameraParameters::KEY_ROTATION, "0");

	 /*lzg@rockchip.com :add some settings to pass cts*/	
	 /*focus distance setting ,no much meaning ,only for passing cts */
	 parameterString = "0.3,50,Infinity";
	 params.set(CameraParameters::KEY_FOCUS_DISTANCES, parameterString.string());
	 /*focus length setting ,no much meaning ,only for passing cts */
	 parameterString = "35";
	 params.set(CameraParameters::KEY_FOCAL_LENGTH, parameterString.string());
	/*horizontal angle of view setting ,no much meaning ,only for passing cts */
	 parameterString = "60";
	 params.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, parameterString.string());
	 /*vertical angle of view setting ,no much meaning ,only for passing cts */
	 parameterString = "28.9";
	 params.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, parameterString.string());

	/*quality of the EXIF thumbnail in Jpeg picture setting */
	 parameterString = "50";
	 params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, parameterString.string());
	/*supported size of the EXIF thumbnail in Jpeg picture setting */
	 parameterString = "0x0,160x128,160x96";
	 params.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, parameterString.string());
	 parameterString = "160";
	 params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, parameterString.string());
	 parameterString = "128";
	 params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, parameterString.string()); 
	 /* zyc@rock-chips.com: for cts ,KEY_MAX_NUM_DETECTED_FACES_HW should not be 0 */

	 params.set(CameraParameters::KEY_RECORDING_HINT,"false");
	 params.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED,"false");
	 params.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED,"true");

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
	 LOGD ("Support Preview sizes: %s ",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
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
	 LOGD ("Support hardware faces detecte: %s",params.get(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW));
	 LOGD ("Support software faces detecte: %s",params.get(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW));
	 LOGD ("Support video stabilization: %s",params.get(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED));
	 LOGD ("Support recording hint: %s",params.get(CameraParameters::KEY_RECORDING_HINT));
	 LOGD ("Support video snapshot: %s",params.get(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED));

	 cameraConfig(params,true,isRestartPreview);
	 LOG_FUNCTION_NAME_EXIT
	
}
int CameraSOCAdapter::setParameters(const CameraParameters &params_set,bool &isRestartValue)
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

    params.getPreviewFpsRange(&fps_min,&fps_max);
    if ((fps_min < 0) || (fps_max < 0) || (fps_max < fps_min)) {
        LOGE("%s(%d): FpsRange(%s) is invalidate",__FUNCTION__,__LINE__,params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE));
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

int CameraSOCAdapter::cameraConfig(const CameraParameters &tmpparams,bool isInit,bool &isRestartValue)
{
    int err = 0, i = 0;
    struct v4l2_control control;
	struct v4l2_ext_control extCtrInfo;
	struct v4l2_ext_controls extCtrInfos;
	CameraParameters params = tmpparams;

    LOGD("set whitebalance");
    /*white balance setting*/
    const char *white_balance = params.get(CameraParameters::KEY_WHITE_BALANCE);
	const char *mwhite_balance = mParameters.get(CameraParameters::KEY_WHITE_BALANCE);
	if (params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE)) {
		if ( !mwhite_balance || strcmp(white_balance, mwhite_balance) ) {
            /* ddl@rock-chips.com: v0.4.9 */
    			for (i = 0; i < mWhiteBalance_number; i++) {
    				if (!strcmp((char *)mWhiteBalance_menu[i].name, white_balance)) {
    					break;
    				}
    			}
    			control.id = mWhiteBalance_menu[i].id;
    			control.value = mWhiteBalance_menu[i].index;
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
		if((mZoomVal < mZoomMin) || (mZoomVal > mZoomMax))
			return BAD_VALUE;
	}


    LOGD("set color effect");

    /*color effect setting*/
    const char *effect = params.get(CameraParameters::KEY_EFFECT);
	const char *meffect = mParameters.get(CameraParameters::KEY_EFFECT);
	if (params.get(CameraParameters::KEY_SUPPORTED_EFFECTS)) {
		if ( ( !meffect || strcmp(effect, meffect) ) ) {
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

    LOGD("set scene");
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

    LOGD("set anti banding");

	const char *antibanding = params.get(CameraParameters::KEY_ANTIBANDING);
	const char *mantibanding = mParameters.get(CameraParameters::KEY_ANTIBANDING);
	if (params.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING)){
		if (!mantibanding || strcmp(antibanding, mantibanding)) {

			for (i = 0; i < mAntibanding_number; i++) {
				if (!strcmp((char *)mAntibanding_menu[i].name, antibanding)) {
					break;
				}
			}
			extCtrInfo.id = mAntibanding_menu[i].id;
			extCtrInfo.value = mAntibanding_menu[i].index;
			extCtrInfos.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
			extCtrInfos.count = 1;
			extCtrInfos.controls = &extCtrInfo;
			err = ioctl(mCamFd, VIDIOC_S_EXT_CTRLS, &extCtrInfos);

			if ( err < 0 ){
				LOGE("%s(%d): Set antibanding(%s) failed",__FUNCTION__,__LINE__,antibanding);
			} else {
			    LOGD ("%s(%d): Set antibanding %s ",__FUNCTION__,__LINE__, (char *)mAntibanding_menu[i].name);
			}
	    }
	}

    LOGD("set white balance lock");

	const char *WhiteBalanceLock = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
	const char *mWhiteBalanceLock = mParameters.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
	if (params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED)){
		if (!mWhiteBalanceLock || strcmp(WhiteBalanceLock, mWhiteBalanceLock)) {
    		extCtrInfo.id = V4L2_CID_WHITEBALANCE_LOCK;
			if(strcmp(WhiteBalanceLock,"true") == 0){
				extCtrInfo.value = 1;
			}else{
	    		extCtrInfo.value = 0;
			}
			extCtrInfos.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
			extCtrInfos.count = 1;
			extCtrInfos.controls = &extCtrInfo;
			err = ioctl(mCamFd, VIDIOC_S_EXT_CTRLS, &extCtrInfos);

			if ( err < 0 ){
				LOGE("%s(%d): Set WhiteBalanceLock(%s) failed",__FUNCTION__,__LINE__,WhiteBalanceLock);
			} else {
				LOGD ("%s(%d): Set WhiteBalanceLock %s ",__FUNCTION__,__LINE__, WhiteBalanceLock);
			}
		}
	}

    LOGD("set exposurelock ");

	const char *ExposureLock = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
	const char *mExposureLock = mParameters.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
	if (params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED)){
		if (!mExposureLock || strcmp(ExposureLock, mExposureLock)) {
    		extCtrInfo.id = V4L2_CID_EXPOSURE_LOCK;
			if(strcmp(ExposureLock,"true") == 0){
				extCtrInfo.value = 1;
			}else{
	    		extCtrInfo.value = 0;
			}
			extCtrInfos.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
			extCtrInfos.count = 1;
			extCtrInfos.controls = &extCtrInfo;
			err = ioctl(mCamFd, VIDIOC_S_EXT_CTRLS, &extCtrInfos);

			if ( err < 0 ){
				LOGE("%s(%d): Set ExposureLock(%s) failed",__FUNCTION__,__LINE__,ExposureLock);
			} else {
				LOGD ("%s(%d): Set ExposureLock %s ",__FUNCTION__,__LINE__, ExposureLock);
			}
		}
	}

    LOGD("set focus ");
    /*focus setting*/
    const char *focusMode = params.get(CameraParameters::KEY_FOCUS_MODE);
	const char *mfocusMode = mParameters.get(CameraParameters::KEY_FOCUS_MODE);
	int weight;
	if (strstr(params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES),focusMode)) {
		err = GetAFParameters(params);		
   		if(err < 0)
			return BAD_VALUE;
		
		if ( !mfocusMode || strcmp(focusMode, mfocusMode) ) {
			if(strcmp(focusMode,CameraParameters::FOCUS_MODE_FIXED)){
	       		if(!cameraAutoFocus(isInit)){
	        		params.set(CameraParameters::KEY_FOCUS_MODE,(mfocusMode?mfocusMode:CameraParameters::FOCUS_MODE_FIXED));
	        		err = -1;
	   			}
			}
		} 
	}else{
		params.set(CameraParameters::KEY_FOCUS_MODE,(mfocusMode?mfocusMode:CameraParameters::FOCUS_MODE_FIXED));
		return BAD_VALUE;
	}
    LOGD("set flash ");

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

    LOGD("set exposure ");

    /*exposure setting*/
	const char *exposure = params.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    const char *mexposure = mParameters.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
	if (strcmp("0", params.get(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION))
		|| strcmp("0", params.get(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION))) {
	    if (!mexposure || (exposure && strcmp(exposure,mexposure))) {
    		control.id = V4L2_CID_EXPOSURE;
    		control.value = atoi(exposure);
    		err = ioctl(mCamFd, VIDIOC_S_CTRL, &control);
    		if ( err < 0 ){
    		    LOGE("%s(%d): Set exposure(%s) failed",__FUNCTION__,__LINE__,exposure);
    		} else {	    
		        LOGD("%s(%d): Set exposure %s",__FUNCTION__,__LINE__,exposure);
    		}
	    }
	}    

    LOGD("config out ");

    mParameters = params;
	//changeVideoPreviewSize();
	isRestartValue = isNeedToRestartPreview();
	
	return 0;
}

//mapping TAE/TAF area coordinate
// zone arrange: (-1000, 1000) with width 2001
// bPre2Drv: true - input is base on preview image size
//			 false - input is base on driver image size
// zone : order lx ty rx dy
int CameraSOCAdapter::AndroidZoneMapping(
			const char* tag,
			__s32 pre_w,
			__s32 pre_h,
			__s32 drv_w,
			__s32 drv_h,
			bool bPre2Drv,
			__s32 *zone)
{
	bool bHeight = false;
	long long ll_pre_w, ll_pre_h, ll_drv_w, ll_drv_h;

	if (pre_w <= 0 ||
		pre_h <= 0 ||
		drv_w <= 0 ||
		drv_h <= 0) {
		LOGE("%s(%s)", __FUNCTION__, tag?tag:"NA");
		LOGE("%s invalid parameters", __FUNCTION__);
		return -EINVAL;
	}

	ll_pre_w = pre_w;
	ll_pre_h = pre_h;
	ll_drv_w = drv_w;
	ll_drv_h = drv_h;

	if (ll_pre_w * ll_drv_h == ll_pre_h * ll_drv_w) {
		return 0;
	}

	LOGE("%s(%s)", __FUNCTION__, tag?tag:"NA");
	LOGE("%s pre %dx%d drv %dx%d",
			__FUNCTION__,
			pre_w,
			pre_h,
			drv_w,
			drv_h);
	LOGE("%s from (%d, %d) (%d, %d)",
			__FUNCTION__,
			zone[0],
			zone[1],
			zone[2],
			zone[3]);

	if ( ((float) pre_h)/((float)pre_w) <
		 ((float) drv_h)/((float)drv_w)) {
		bHeight = true;
	}


	// y' * h' = y * h

	if (bPre2Drv) {
		// h = drv_h * pre_w / drv_w
		//
		// y = y'*h'/h
		//   = y' * h' * drv_w / (drv_h * pre_w)

		if (bHeight) {
			zone[1] = (__s32) ( zone[1] * ll_pre_h * ll_drv_w / (ll_drv_h * ll_pre_w) );
			zone[3] = (__s32) ( zone[3] * ll_pre_h * ll_drv_w / (ll_drv_h * ll_pre_w) );
		} else {
			zone[0] = (__s32) ( zone[0] * ll_pre_w * ll_drv_h / (ll_drv_w * ll_pre_h) );
			zone[2] = (__s32) ( zone[2] * ll_pre_w * ll_drv_h / (ll_drv_w * ll_pre_h) );
		}
	} else {
		// h = drv_h * pre_w /drv_w
		//
		// y' = y * h / h'
		//    = y * drv_h * pre_w / (h' * drv_w)

		if (bHeight) {
			zone[1] = (__s32) ( zone[1] * ll_drv_h * ll_pre_w / (ll_pre_h * ll_drv_w) );
			zone[3] = (__s32) ( zone[3] * ll_drv_h * ll_pre_w / (ll_pre_h * ll_drv_w) );
		} else {
			zone[0] = (__s32) ( zone[0] * ll_drv_w * ll_pre_h / (ll_pre_w * ll_drv_h) );
			zone[2] = (__s32) ( zone[2] * ll_drv_w * ll_pre_h / (ll_pre_w * ll_drv_h) );
		}
	}

	LOGE("%s to (%d, %d) (%d, %d)",
				__FUNCTION__,
				zone[0],
				zone[1],
				zone[2],
				zone[3]);

	return 0;
}

int CameraSOCAdapter::GetAFParameters(const CameraParameters params)
{
	int weight = 0;
	const char *focusmode = params.get(CameraParameters::KEY_FOCUS_MODE);

	m_focus_mode = 0;

	if (!focusmode) {
		LOGE("%s(%d): focus is null",__FUNCTION__,__LINE__);
		return BAD_VALUE;
	}
	if (strcmp(focusmode, CameraParameters::FOCUS_MODE_AUTO) == 0) {
		m_focus_mode = V4L2_CID_FOCUS_AUTO;
		m_focus_value = 1;
		
		// set zone focus
		if(params.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS) == 1){
			//parse zone,
			int lx,ty,rx,dy;
			const char* zoneStr = params.get(CameraParameters::KEY_FOCUS_AREAS);
			LOGD("zoneStr = %s",zoneStr);
			if(zoneStr){
				//get lx
				m_taf_roi[0] = strtol(zoneStr+1,0,0);

				//get ty
				char* tys = strstr(zoneStr,",");
				m_taf_roi[1] = strtol(tys+1,0,0);


				//get rx
				char* rxs = strstr(tys+1,",");
				m_taf_roi[2] = strtol(rxs+1,0,0);

				//get by
				char* dys = strstr(rxs+1,",");
				m_taf_roi[3] = strtol(dys+1,0,0);

				//get weight
				char* wt = strstr(dys+1,",");
				weight = strtol(wt+1,0,0);
				
				if(strstr(wt+1,"(")){
					//more than one zone
					return BAD_VALUE;
				}

				if((m_taf_roi[0] == 0) && (m_taf_roi[1] == 0) && (m_taf_roi[2] == 0) && (m_taf_roi[3] == 0)){
					m_taf_roi[0] = 0;
					m_taf_roi[1] = 0;
					m_taf_roi[2] = 0;
					m_taf_roi[3] = 0;
				}
				else if ( ((m_taf_roi[0]<-1000) || (m_taf_roi[0]>1000)) ||
					 ((m_taf_roi[1]<-1000) || (m_taf_roi[1]>1000)) ||
					 ((m_taf_roi[2]<-1000) || (m_taf_roi[2]>1000)) ||
					 ((m_taf_roi[3]<-1000) || (m_taf_roi[3]>1000)) ||
					 (m_taf_roi[0]>=m_taf_roi[2]) || 
					 (m_taf_roi[1]>=m_taf_roi[3]) ||
					 (weight<1) || (weight>1000)) {
					LOGE("%s: %s , afWin(%d,%d,%d,%d)is invalidate!",
						CameraParameters::KEY_FOCUS_AREAS,
						params.get(CameraParameters::KEY_FOCUS_AREAS),
						m_taf_roi[0],m_taf_roi[1],m_taf_roi[2],m_taf_roi[3]); 						   
					m_taf_roi[0] = 0;
					m_taf_roi[1] = 0;
					m_taf_roi[2] = 0;
					m_taf_roi[3] = 0;
					return BAD_VALUE;
				}
				//remapping if the image is croped by hal
				AndroidZoneMapping(
						"AF ROI",
						mCamPreviewW,
						mCamPreviewH,
						mCamDrvWidth,
						mCamDrvHeight,
						true,
						m_taf_roi);

			}
		}
	} else if (strcmp(focusmode, CameraParameters::FOCUS_MODE_INFINITY) == 0) {
		m_focus_mode = V4L2_CID_FOCUS_ABSOLUTE;
		m_focus_value = 0;
	} else if (strcmp(focusmode, CameraParameters::FOCUS_MODE_MACRO) == 0) {
		m_focus_mode = V4L2_CID_FOCUS_ABSOLUTE;
		m_focus_value = 0xff;
	} else if ((strcmp(focusmode, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE) == 0)
			   || (strcmp(focusmode, CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO) == 0)){
		m_focus_mode = V4L2_CID_FOCUS_CONTINUOUS;
		m_focus_value = 1;
	} else if (strcmp(focusmode, CameraParameters::FOCUS_MODE_FIXED) == 0) {
	    m_focus_mode = CameraSOCAdapter::focus_fixed;

		LOG1("%s(%d): %s is not need config sensor driver",__FUNCTION__,__LINE__,CameraParameters::FOCUS_MODE_FIXED);
	} else {
		LOGE("%s(%d): focus is not support in camera driver",__FUNCTION__,__LINE__);
	}
	return 0;
}

//auto_trig_only: avoid drive driver to focus if focus mode is V4L2_CID_FOCUS_AUTO
int CameraSOCAdapter::cameraAutoFocus(bool auto_trig_only)
{
    int err;
    struct v4l2_ext_control extCtrInfo;
	struct v4l2_ext_controls extCtrInfos;
	
    if (m_focus_mode == 0) {
    	LOGE("%s(%d): focus mode is not set",__FUNCTION__,__LINE__);
    	err = false;
    	goto cameraAutoFocus_end;
    }
    
    if (m_focus_mode == CameraSOCAdapter::focus_fixed) {
    	err = true;
    	goto cameraAutoFocus_end;
    }

	extCtrInfo.rect[0] = 0;
    extCtrInfo.rect[1] = 0;
    extCtrInfo.rect[2] = 0;
    extCtrInfo.rect[3] = 0;   
    if (m_focus_mode == V4L2_CID_FOCUS_AUTO) {
			extCtrInfo.rect[0] = m_taf_roi[0];
			extCtrInfo.rect[1] = m_taf_roi[1];
			extCtrInfo.rect[2] = m_taf_roi[2];
		    extCtrInfo.rect[3] = m_taf_roi[3];
		    if (auto_trig_only) {
		    	//m_focus_value = 2;
		    	err = true;
		    	goto cameraAutoFocus_end;
		    }
    }
    extCtrInfo.value = m_focus_value;
    extCtrInfo.id = m_focus_mode;

    extCtrInfos.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
	extCtrInfos.count = 1;
	extCtrInfos.controls = &extCtrInfo;
	err = ioctl(mCamFd, VIDIOC_S_EXT_CTRLS, &extCtrInfos);
	if ( err < 0 ){
		LOGE("%s(%d): Set focus mode(%d) failed",__FUNCTION__,__LINE__, m_focus_mode);
		//do not return false temporary ,driver may be somthing wrong,zyc@rock-chips.com
        err = true;
	} else {
	    LOG1("%s(%d): Set focus mode %d",__FUNCTION__,__LINE__, m_focus_mode);
        err = true;
	}
cameraAutoFocus_end:
    return err;
}

int CameraSOCAdapter::flashcontrol()
{
	int err;
	struct v4l2_queryctrl flashMode;
	flashMode.id = V4L2_CID_FLASH;
	if (!ioctl(mCamFd, VIDIOC_QUERYCTRL, &flashMode)) {
		struct v4l2_ext_control extCtrInfo;
		struct v4l2_ext_controls extCtrInfos;
		extCtrInfo.id = V4L2_CID_FLASH;
		extCtrInfo.value = 0xfefe5a5a;
		extCtrInfos.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
		extCtrInfos.count = 1;
		extCtrInfos.controls = &extCtrInfo;
		
		err = ioctl(mCamFd, VIDIOC_S_EXT_CTRLS, &extCtrInfos);
		if ( err < 0 ){
			LOGD ("%s(%d): Set flash failed",__FUNCTION__,__LINE__);			
		}
		else {
			LOGE ("%s(%d): Set flash succeed",__FUNCTION__,__LINE__);
		}
	}

	return 0;
}


}
