#include "CameraHal.h"
namespace android{


CameraAdapter::CameraAdapter(int cameraId):mPreviewRunning(0),
                                           mCamId(cameraId)
{
    LOG_FUNCTION_NAME
    mRefDisplayAdapter = NULL;
    mRefEventNotifier = NULL;
    mCameraPreviewThread = NULL;
    mPreviewRunning = 0;
	mPictureRunning = 0;
    mPreviewBufProvider = NULL;
    mCamDrvWidth = 0;
    mCamDrvHeight = 0;
    mVideoWidth = 0;
    mVideoHeight = 0;
    mCamDriverStream = false;
    camera_device_error = false;
    mPreviewFrameIndex = 0;
    mPreviewErrorFrameCount = 0;
    mCamFd = -1;
    mCamDriverPreviewFmt = 0;
    mZoomVal = 100;
	mLibstageLibHandle = NULL;

    CameraHal_SupportFmt[0] = V4L2_PIX_FMT_NV12;
    CameraHal_SupportFmt[1] = V4L2_PIX_FMT_NV16;
    CameraHal_SupportFmt[2] = V4L2_PIX_FMT_YUYV;
    CameraHal_SupportFmt[3] = V4L2_PIX_FMT_RGB565;
    CameraHal_SupportFmt[4] = 0x00;
    CameraHal_SupportFmt[5] = 0x00;

	cif_driver_iommu = false;
   LOG_FUNCTION_NAME_EXIT   
}

int CameraAdapter::initialize()
{
	int ret = -1;
    //create focus thread
    LOG_FUNCTION_NAME
    
	if((ret = cameraCreate(mCamId)) < 0)
		return ret;
	
	initDefaultParameters(mCamId);
    LOG_FUNCTION_NAME_EXIT
	return ret;
}
CameraAdapter::~CameraAdapter()
{
    LOG_FUNCTION_NAME
		
	
	if(mAutoFocusThread != NULL){
    	mAutoFocusLock.lock();
    	mExitAutoFocusThread = true;
    	mAutoFocusLock.unlock();
    	mAutoFocusCond.signal();
		mAutoFocusThread->requestExitAndWait();
		mAutoFocusThread.clear();
        mAutoFocusThread = NULL;
	}

    if(mPreviewBufProvider)
        mPreviewBufProvider->freeBuffer();
    
    if (mCamFd > 0) {
        close(mCamFd);
        mCamFd = -1;
    }  

	this->cameraDestroy();
    LOG_FUNCTION_NAME_EXIT
}

void CameraAdapter::setDisplayAdapterRef(DisplayAdapter& refDisplayAdap)
{
    mRefDisplayAdapter =  &refDisplayAdap;
}
void CameraAdapter::setEventNotifierRef(AppMsgNotifier& refEventNotify)
{
    mRefEventNotifier = &refEventNotify;
}

int CameraAdapter::faceNotify(struct RectFace* faces, int* num)
{    
    return 0;
}
void CameraAdapter::setPreviewBufProvider(BufferProvider* bufprovider)
{
    mPreviewBufProvider = bufprovider;
}
CameraParameters & CameraAdapter::getParameters()
{
    return mParameters;
}
int CameraAdapter::getCurPreviewState(int *drv_w,int *drv_h)
{
	*drv_w = mCamDrvWidth;
	*drv_h = mCamDrvHeight;
    return mPreviewRunning;
}
int CameraAdapter::getCurVideoSize(int *video_w, int *video_h)
{
	*video_w = mVideoWidth;
	*video_h = mVideoHeight;
    return mPreviewRunning;

}

bool CameraAdapter::isNeedToRestartPreview()
{
    int preferPreviewW=0,preferPreviewH=0;
	int previewFrame2AppW=0,previewFrame2AppH=0;
	bool ret = false;

    mParameters.getPreviewSize(&previewFrame2AppW, &previewFrame2AppH);
    //case 1: when setVideoSize is suported

	if(mPreviewRunning && (mParameters.get(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES) != NULL)) {
    	mParameters.getVideoSize(&mVideoWidth,&mVideoHeight);
	}else{
		mVideoWidth = -1;
		mVideoHeight = -1;
	}	

	if(previewFrame2AppW >= mVideoWidth){
		preferPreviewW = previewFrame2AppW;
		preferPreviewH = previewFrame2AppH;
	} else {
		preferPreviewW = mVideoWidth;
		preferPreviewH = mVideoHeight;	
	}
    //not support setvideosize
    if(mVideoWidth == -1){
        mVideoWidth = preferPreviewW;
        mVideoHeight = preferPreviewH;
    }
	
    LOG1("mPreviewFrame2AppW (%dx%d)",previewFrame2AppW,previewFrame2AppH);
    LOG1("mCamPreviewW (%dx%d)",mCamPreviewW,mCamPreviewH);
    LOG1("video width (%dx%d)",mVideoWidth,mVideoHeight);

	if(mPreviewRunning && ((preferPreviewW != mCamPreviewW) || (preferPreviewH != mCamPreviewH)) && (mVideoWidth != -1))
	{
      ret = true;
	}
    return ret;
}

void CameraAdapter::dump(int cameraId)
{
	LOG2("%s CameraAdapter dump cameraId(%d)\n", __FUNCTION__,cameraId);
	
}

void CameraAdapter::getCameraParamInfo(cameraparam_info_s &paraminfo)
{

}

bool CameraAdapter::getFlashStatus()
{
	return false;
}
status_t CameraAdapter::startPreview(int preview_w,int preview_h,int w, int h, int fmt,bool is_capture)
{
    
    //create buffer
	LOG_FUNCTION_NAME
    unsigned int frame_size = 0,i;
    struct bufferinfo_s previewbuf;
    int ret = 0,buf_count = CONFIG_CAMERA_PREVIEW_BUF_CNT;
    LOGD("%s%d:preview_w = %d,preview_h = %d,drv_w = %d,drv_h = %d",__FUNCTION__,__LINE__,preview_w,preview_h,w,h);
   mPreviewFrameIndex = 0;
   mPreviewErrorFrameCount = 0;
    switch (mCamDriverPreviewFmt)
    {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_YUV420:
			frame_size = ((w+15)&(~15))*((h+15)&(~15))*3/2;
            break;
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_YUV422P:
        default:
            frame_size = w*h*2;
            break;            
    }

    //for test capture
    if(is_capture){
        buf_count = 1;
    }
    if(mPreviewBufProvider->createBuffer(buf_count,frame_size,PREVIEWBUFFER,mPreviewBufProvider->is_cif_driver) < 0)
    {
        LOGE("%s%d:create preview buffer failed",__FUNCTION__,__LINE__);
        return -1;
    }
    //set size
    if(cameraSetSize(w, h, mCamDriverPreviewFmt,is_capture)<0){
        ret = -1;
        goto start_preview_end;
    }
    mCamDrvWidth = w;
    mCamDrvHeight = h;
    mCamPreviewH = preview_h;
    mCamPreviewW = preview_w;

    memset(mPreviewFrameInfos,0,sizeof(mPreviewFrameInfos));
    //camera start
    if(cameraStart() < 0){
        ret = -1;
        goto start_preview_end;
    }
    //new preview thread
    mCameraPreviewThread = new CameraPreviewThread(this);
	mCameraPreviewThread->run("CameraPreviewThread",ANDROID_PRIORITY_DISPLAY);
    
    mPreviewRunning = 1;
    LOGD("%s(%d):OUT",__FUNCTION__,__LINE__);
    return 0;
start_preview_end:
    mCamDrvWidth = 0;
    mCamDrvHeight = 0;
    mCamPreviewH = 0;
    mCamPreviewW = 0;
    return ret;
    
}
status_t CameraAdapter::stopPreview()
{
    LOGD("%s(%d):IN",__FUNCTION__,__LINE__);
    if(mPreviewRunning == 1){
        //camera stop
        cameraStream(false);
        
        //quit preview thread
    	if(mCameraPreviewThread != NULL){
        	mCameraPreviewThread->requestExitAndWait();
        	mCameraPreviewThread.clear();
            mCameraPreviewThread = NULL;
    	}
		
    	cameraStop();
        //destroy preview buffer
        if(mPreviewBufProvider)
            mPreviewBufProvider->freeBuffer();
        mPreviewRunning = 0;
    }
    mCamDrvWidth = 0;
    mCamDrvHeight = 0;
    LOGD("%s(%d):OUT",__FUNCTION__,__LINE__);
    return 0;
}
int CameraAdapter::setParameters(const CameraParameters &params_set,bool &isRestartValue)
{
    
    return 0;
}
void CameraAdapter::initDefaultParameters(int camFd)
{

}
status_t CameraAdapter::autoFocus()
{
	
	mAutoFocusCond.signal();
    return 0;
}
status_t CameraAdapter::cancelAutoFocus()
{
    return 0;
}
int CameraAdapter::getCameraFd()
{
    return mCamFd;
}
int CameraAdapter::flashcontrol()
{
    return 0;
}

//talk to driver
//open camera
int CameraAdapter::cameraCreate(int cameraId)
{
    int err = 0,iCamFd;
    int pmem_fd,i,j,cameraCnt;
    char cam_path[20];
    char cam_num[3];
    char *ptr_tmp;
    struct v4l2_fmtdesc fmtdesc;
    char *cameraDevicePathCur = NULL;
    char decode_name[50];
    
    LOG_FUNCTION_NAME
        
    memset(decode_name,0x00,sizeof(decode_name));
    mLibstageLibHandle = dlopen("libstagefright.so", RTLD_NOW);    
    if (mLibstageLibHandle == NULL) {
        LOGE("%s(%d): open libstagefright.so fail",__FUNCTION__,__LINE__);
    } else {
        mMjpegDecoder.get = (getMjpegDecoderFun)dlsym(mLibstageLibHandle, "get_class_On2JpegDecoder");         
    }

    if (mMjpegDecoder.get == NULL) {
        if (mLibstageLibHandle != NULL)
            dlclose(mLibstageLibHandle);            /* ddl@rock-chips.com: v0.4.0x27 */
        mLibstageLibHandle = dlopen("librk_vpuapi.so", RTLD_NOW);    
        if (mLibstageLibHandle == NULL) {
            LOGE("%s(%d): open librk_vpuapi.so fail",__FUNCTION__,__LINE__);
        } else {
            mMjpegDecoder.get = (getMjpegDecoderFun)dlsym(mLibstageLibHandle, "get_class_RkJpegDecoder");
            if (mMjpegDecoder.get == NULL) {
                LOGE("%s(%d): dlsym get_class_RkJpegDecoder fail",__FUNCTION__,__LINE__);
            } else {
                strcat(decode_name,"dec_oneframe_RkJpegDecoder");
            }
        }
    } else {
        strcat(decode_name,"dec_oneframe_class_RkJpegDecoder");
    }


    if (mMjpegDecoder.get != NULL) {
        mMjpegDecoder.decoder = mMjpegDecoder.get();
        if (mMjpegDecoder.decoder==NULL) {
            LOGE("%s(%d): get mjpeg decoder failed",__FUNCTION__,__LINE__);
        } else {
            mMjpegDecoder.destroy =(destroyMjpegDecoderFun)dlsym(mLibstageLibHandle, "destroy_class_RkJpegDecoder");
            if (mMjpegDecoder.destroy == NULL)
                LOGE("%s(%d): dlsym destroy_class_RkJpegDecoder fail",__FUNCTION__,__LINE__);

            mMjpegDecoder.init = (initMjpegDecoderFun)dlsym(mLibstageLibHandle, "init_class_RkJpegDecoder");
            if (mMjpegDecoder.init == NULL)
                LOGE("%s(%d): dlsym init_class_RkJpegDecoder fail",__FUNCTION__,__LINE__);

            mMjpegDecoder.deInit =(deInitMjpegDecoderFun)dlsym(mLibstageLibHandle, "deinit_class_RkJpegDecoder");
            if (mMjpegDecoder.deInit == NULL)
                LOGE("%s(%d): dlsym deinit_class_RkJpegDecoder fail",__FUNCTION__,__LINE__);

            mMjpegDecoder.decode =(mjpegDecodeOneFrameFun)dlsym(mLibstageLibHandle, decode_name);
            if (mMjpegDecoder.decode == NULL)
                LOGE("%s(%d): dlsym %s fail",__FUNCTION__,__LINE__,decode_name); 

            if ((mMjpegDecoder.deInit != NULL) && (mMjpegDecoder.init != NULL) &&
                (mMjpegDecoder.destroy != NULL) && (mMjpegDecoder.decode != NULL)) {
                mMjpegDecoder.state = mMjpegDecoder.init(mMjpegDecoder.decoder);
            }
        }
    }

	
    cameraDevicePathCur = (char*)&gCamInfos[cameraId].device_path[0];
    iCamFd = open(cameraDevicePathCur, O_RDWR);
    if (iCamFd < 0) {
        LOGE("%s(%d): open camera%d(%s) is failed",__FUNCTION__,__LINE__,cameraId,cameraDevicePathCur);
        goto exit;
    }

    memset(&mCamDriverCapability, 0, sizeof(struct v4l2_capability));
    err = ioctl(iCamFd, VIDIOC_QUERYCAP, &mCamDriverCapability);
    if (err < 0) {
    	LOGE("%s(%d): %s query device's capability failed.\n",__FUNCTION__,__LINE__,cam_path);
	    goto exit1;
    }
    
    LOGD("Camera driver: %s   Driver version: %d.%d.%d  CameraHal version: %d.%d.%d ",mCamDriverCapability.driver,
        (mCamDriverCapability.version>>16) & 0xff,(mCamDriverCapability.version>>8) & 0xff,
        mCamDriverCapability.version & 0xff,(CONFIG_CAMERAHAL_VERSION>>16) & 0xff,(CONFIG_CAMERAHAL_VERSION>>8) & 0xff,
        CONFIG_CAMERAHAL_VERSION & 0xff);

    memset(&fmtdesc, 0, sizeof(fmtdesc));    
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;    
	while (ioctl(iCamFd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        mCamDriverSupportFmt[fmtdesc.index] = fmtdesc.pixelformat;
        LOGD("mCamDriverSupportFmt: fmt = %d,index = %d",fmtdesc.pixelformat,fmtdesc.index);

		fmtdesc.index++;
	}

    
    i = 0;    
    while (CameraHal_SupportFmt[i]) {
        LOG1("CameraHal_SupportFmt:fmt = %d,index = %d",CameraHal_SupportFmt[i],i);
        j = 0;
        while (mCamDriverSupportFmt[j]) {
            if (mCamDriverSupportFmt[j] == CameraHal_SupportFmt[i]) {
                if ((mCamDriverSupportFmt[j] == V4L2_PIX_FMT_MJPEG) && (mMjpegDecoder.state == -1))
                    continue;
                break;
            }
            j++;
        }
        if (mCamDriverSupportFmt[j] == CameraHal_SupportFmt[i]) {
            break;
        }
        i++;
    }

    if (CameraHal_SupportFmt[i] == 0x00) {
        LOGE("%s(%d): all camera driver support format is not supported in CameraHal!!",__FUNCTION__,__LINE__);
        j = 0;
        while (mCamDriverSupportFmt[j]) {
            LOG1("pixelformat = '%c%c%c%c'",
				mCamDriverSupportFmt[j] & 0xFF, (mCamDriverSupportFmt[j] >> 8) & 0xFF,
				(mCamDriverSupportFmt[j] >> 16) & 0xFF, (mCamDriverSupportFmt[j] >> 24) & 0xFF);
            j++;
        }
        goto exit1;
    } else {  
        mCamDriverPreviewFmt = CameraHal_SupportFmt[i];
        LOGD("%s(%d): mCamDriverPreviewFmt(%c%c%c%c) is cameraHal and camera driver is also supported!!",__FUNCTION__,__LINE__,
            mCamDriverPreviewFmt & 0xFF, (mCamDriverPreviewFmt >> 8) & 0xFF,
			(mCamDriverPreviewFmt >> 16) & 0xFF, (mCamDriverPreviewFmt >> 24) & 0xFF);

        LOGD("mCamDriverPreviewFmt  = %d",mCamDriverPreviewFmt);
        
    }

    
    LOGD("%s(%d): Current driver is %s, v4l2 memory is %s",__FUNCTION__,__LINE__,mCamDriverCapability.driver, 
        (mCamDriverV4l2MemType==V4L2_MEMORY_MMAP)?"V4L2_MEMORY_MMAP":"V4L2_MEMORY_OVERLAY");
   mCamFd = iCamFd;

    //create focus thread for soc or usb camera.
    mAutoFocusThread = new AutoFocusThread(this);
	mAutoFocusThread->run("AutoFocusThread", ANDROID_PRIORITY_URGENT_DISPLAY);	
    mExitAutoFocusThread = false;


    LOG_FUNCTION_NAME_EXIT 
    return 0;

exit1:
    if (iCamFd > 0) {
        close(iCamFd);
        iCamFd = -1;
    }  

exit:
    LOGE("%s(%d): exit with error -1",__FUNCTION__,__LINE__);
    return -1;
}
int CameraAdapter::cameraDestroy()
{
    int err,i;

    LOG_FUNCTION_NAME
    if (mLibstageLibHandle && (mMjpegDecoder.state == 0)) {
        mMjpegDecoder.deInit(mMjpegDecoder.decoder);
        mMjpegDecoder.destroy(mMjpegDecoder.decoder);
        mMjpegDecoder.decoder = NULL;

        dlclose(mLibstageLibHandle);
        mLibstageLibHandle = NULL;
    }    LOG_FUNCTION_NAME_EXIT
    return 0;
}

int CameraAdapter::cameraSetSize(int w, int h, int fmt, bool is_capture)
{
    int err=0;
    struct v4l2_format format;

	/* Set preview format */
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = w;
	format.fmt.pix.height = h;
	format.fmt.pix.pixelformat = fmt;
	format.fmt.pix.field = V4L2_FIELD_NONE;		/* ddl@rock-chips.com : field must be initialised for Linux kernel in 2.6.32  */
    LOGD("%s(%d):IN, w = %d,h = %d",__FUNCTION__,__LINE__,w,h);
    if (is_capture) {                           /* ddl@rock-chips.com: v0.4.1 add capture and preview check */
        format.fmt.pix.priv = 0xfefe5a5a;    
    } else {
        format.fmt.pix.priv = 0x5a5afefe;  
    }
    
	err = ioctl(mCamFd, VIDIOC_S_FMT, &format);
	if ( err < 0 ){
		LOGE("%s(%d): VIDIOC_S_FMT failed",__FUNCTION__,__LINE__);		
	} else {
	    LOG1("%s(%d): VIDIOC_S_FMT %dx%d '%c%c%c%c'",__FUNCTION__,__LINE__,format.fmt.pix.width, format.fmt.pix.height,
				fmt & 0xFF, (fmt >> 8) & 0xFF,(fmt >> 16) & 0xFF, (fmt >> 24) & 0xFF);
	}

    return err;
}
int CameraAdapter::cameraStream(bool on)
{
    int err = 0;
    int cmd ;
    enum v4l2_buf_type type;
    LOGD("%s(%d):on = %d",__FUNCTION__,__LINE__,on);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cmd = (on)?VIDIOC_STREAMON:VIDIOC_STREAMOFF;

    mCamDriverStreamLock.lock();
    err = ioctl(mCamFd, cmd, &type);
    if (err < 0) {
        LOGE("%s(%d): %s Failed",__FUNCTION__,__LINE__,((on)?"VIDIOC_STREAMON":"VIDIOC_STREAMOFF"));
        goto cameraStream_end;
    }
    mCamDriverStream = on;

cameraStream_end:
	mCamDriverStreamLock.unlock();
    return err;
}
// query buffer ,qbuf , stream on
int CameraAdapter::cameraStart()
{
    int preview_size,i;
    int err;
    int nSizeBytes;
    int buffer_count;
    struct v4l2_format format;
    enum v4l2_buf_type type;
    struct v4l2_requestbuffers creqbuf;
    struct v4l2_buffer buffer;
    CameraParameters  tmpparams;
    
    LOG_FUNCTION_NAME

    buffer_count = mPreviewBufProvider->getBufCount();
    creqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;    
    creqbuf.memory = mCamDriverV4l2MemType;
    creqbuf.count  = buffer_count;
    if (ioctl(mCamFd, VIDIOC_REQBUFS, &creqbuf) < 0) {
        LOGE ("%s(%d): VIDIOC_REQBUFS Failed. %s",__FUNCTION__,__LINE__, strerror(errno));
        goto fail_reqbufs;
    }   
    if (buffer_count == 0) {
        LOGE("%s(%d): preview buffer havn't alloced",__FUNCTION__,__LINE__);
        goto fail_reqbufs;
    }
    memset(mCamDriverV4l2Buffer, 0x00, sizeof(mCamDriverV4l2Buffer));
    for (int i = 0; i < buffer_count; i++) {
            memset(&buffer, 0, sizeof(struct v4l2_buffer));        
            buffer.type = creqbuf.type;
            buffer.memory = creqbuf.memory;
            buffer.flags = 0;
            buffer.index = i;

            if (ioctl(mCamFd, VIDIOC_QUERYBUF, &buffer) < 0) {
                LOGE("%s(%d): VIDIOC_QUERYBUF Failed",__FUNCTION__,__LINE__);
                goto fail_bufalloc;
            }

            if (buffer.memory == V4L2_MEMORY_OVERLAY) {  

                buffer.m.offset = mPreviewBufProvider->getBufPhyAddr(i);
                mCamDriverV4l2Buffer[i] = (char*)mPreviewBufProvider->getBufVirAddr(i);
            } else if (buffer.memory == V4L2_MEMORY_MMAP) {
                mCamDriverV4l2Buffer[i] = (char*)mmap(0 /* start anywhere */ ,
                                    buffer.length, PROT_READ, MAP_SHARED, mCamFd,
                                    buffer.m.offset);
                if (mCamDriverV4l2Buffer[i] == MAP_FAILED) {
                    LOGE("%s(%d): Unable to map buffer(length:0x%x offset:0x%x) %s(err:%d)\n",__FUNCTION__,__LINE__, buffer.length,buffer.m.offset,strerror(errno),errno);
                    goto fail_bufalloc;
                } 
            }
            mCamDriverV4l2BufferLen = buffer.length;
            
            mPreviewBufProvider->setBufferStatus(i, 1,PreviewBufferProvider::CMD_PREVIEWBUF_WRITING);
            err = ioctl(mCamFd, VIDIOC_QBUF, &buffer);
            if (err < 0) {
                LOGE("%s(%d): VIDIOC_QBUF Failed,err=%d[%s]\n",__FUNCTION__,__LINE__,err, strerror(errno));
                mPreviewBufProvider->setBufferStatus(i, 0,PreviewBufferProvider::CMD_PREVIEWBUF_WRITING);
                goto fail_bufalloc;
            }   
    }
	

    mPreviewErrorFrameCount = 0;
    mPreviewFrameIndex = 0;
    cameraStream(true);
    LOG_FUNCTION_NAME_EXIT
    return 0;

fail_bufalloc:
    mPreviewBufProvider->freeBuffer();
fail_reqbufs:
    LOGE("%s(%d): exit with error(%d)",__FUNCTION__,__LINE__,-1);
    return -1;
}
int CameraAdapter::cameraStop()
{
    return 0;
}
int CameraAdapter::cameraAutoFocus(bool auto_trig_only)
{
    return 0;
}

//dqbuf
int CameraAdapter::getFrame(FramInfo_s** tmpFrame){

   struct v4l2_buffer cfilledbuffer1;
   int ret = 0;
    
    cfilledbuffer1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cfilledbuffer1.memory = mCamDriverV4l2MemType;
    cfilledbuffer1.reserved = 0;

    FILTER_FRAMES:
    /* De-queue the next avaliable buffer */            
    LOG2("%s(%d): get frame in",__FUNCTION__,__LINE__);
    if (ioctl(mCamFd, VIDIOC_DQBUF, &cfilledbuffer1) < 0) {
        LOGE("%s(%d): VIDIOC_DQBUF Failed!!! err[%s] \n",__FUNCTION__,__LINE__,strerror(errno));
        if (errno == EIO) {
            mCamDriverStreamLock.lock();
            if (mCamDriverStream) {
                camera_device_error = true;  
                LOGE("%s(%d): camera driver or device may be error, so notify CAMERA_MSG_ERROR",
                        __FUNCTION__,__LINE__);
            }
            mCamDriverStreamLock.unlock();
        } else {
            mCamDriverStreamLock.lock();
            if (mCamDriverStream) {
                mPreviewErrorFrameCount++;
                if (mPreviewErrorFrameCount >= 2) {  
                    mCamDriverStreamLock.unlock();
                    camera_device_error = true;   
                    LOGE("%s(%d): mPreviewErrorFrameCount is %d, camera driver or device may be error, so notify CAMERA_MSG_ERROR",
                        __FUNCTION__,__LINE__,mPreviewErrorFrameCount);
                } else {
                    mCamDriverStreamLock.unlock();
                }
            } else {
                mCamDriverStreamLock.unlock();  
            }
        }
        ret = -1;
        goto getFrame_out;
    } else {
        mPreviewErrorFrameCount = 0;
    }
    LOG2("%s(%d): deque a  frame %d success",__FUNCTION__,__LINE__,cfilledbuffer1.index);

    if(mPreviewFrameIndex++ < FILTER_FRAME_NUMBER)
    {
        LOG2("%s:filter frame %d",__FUNCTION__,mPreviewFrameIndex);
    	mCamDriverStreamLock.lock();
		if(mCamDriverStream)
        ioctl(mCamFd, VIDIOC_QBUF, &cfilledbuffer1);
        mCamDriverStreamLock.unlock();
        goto FILTER_FRAMES; 
    }
    // fill frame info:w,h,phy,vir
    mPreviewFrameInfos[cfilledbuffer1.index].frame_fmt=  mCamDriverPreviewFmt;
    mPreviewFrameInfos[cfilledbuffer1.index].frame_height = mCamDrvHeight;
    mPreviewFrameInfos[cfilledbuffer1.index].frame_width = mCamDrvWidth;
    mPreviewFrameInfos[cfilledbuffer1.index].frame_index = cfilledbuffer1.index;
    if(mCamDriverV4l2MemType == V4L2_MEMORY_OVERLAY){
		if(cif_driver_iommu){
			mPreviewFrameInfos[cfilledbuffer1.index].phy_addr = mPreviewBufProvider->getBufShareFd(cfilledbuffer1.index);
		}else
        	mPreviewFrameInfos[cfilledbuffer1.index].phy_addr = mPreviewBufProvider->getBufPhyAddr(cfilledbuffer1.index);
	}else
        mPreviewFrameInfos[cfilledbuffer1.index].phy_addr = 0;
    mPreviewFrameInfos[cfilledbuffer1.index].vir_addr = (unsigned long)mCamDriverV4l2Buffer[cfilledbuffer1.index];
    //get zoom_value
    mPreviewFrameInfos[cfilledbuffer1.index].zoom_value = mZoomVal;
    mPreviewFrameInfos[cfilledbuffer1.index].used_flag = 0;
    mPreviewFrameInfos[cfilledbuffer1.index].frame_size = cfilledbuffer1.bytesused;
    mPreviewFrameInfos[cfilledbuffer1.index].res        = NULL;
        
    *tmpFrame = &(mPreviewFrameInfos[cfilledbuffer1.index]);
    LOG2("%s(%d): fill  frame info success",__FUNCTION__,__LINE__);
 //   if (gLogLevel == 2)
  //      debugShowFPS();
getFrame_out:
    if(camera_device_error)
        mRefEventNotifier->notifyCbMsg(CAMERA_MSG_ERROR, CAMERA_ERROR_SERVER_DIED);
    return ret ;
}
int CameraAdapter::adapterReturnFrame(long index,int cmd){
    int buf_state = -1;
   struct v4l2_buffer vb;
    mCamDriverStreamLock.lock();

    if (!mCamDriverStream) {
        LOGD("%s(%d): preview thread is pause, so buffer %d isn't enqueue to camera",__FUNCTION__,__LINE__,index);
        mCamDriverStreamLock.unlock();
        return 0;
    }
	mPreviewBufProvider->setBufferStatus(index,0, cmd); 
    buf_state = mPreviewBufProvider->getBufferStatus(index);
    LOG2("%s(%d):index(%d),cmd(%d),buf_state(%d)",__FUNCTION__,__LINE__,index,cmd,buf_state);
    if (buf_state == 0) {
        vb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vb.memory = mCamDriverV4l2MemType;
        vb.index = index;                        
        vb.reserved = 0;
        vb.m.offset = mPreviewBufProvider->getBufPhyAddr(index); 
        
        mPreviewBufProvider->setBufferStatus(index,1,PreviewBufferProvider::CMD_PREVIEWBUF_WRITING); 
        if (ioctl(mCamFd, VIDIOC_QBUF, &vb) < 0) {
            LOGE("%s(%d): VIDIOC_QBUF %d Failed!!! err[%s]", __FUNCTION__,__LINE__,index, strerror(errno));
            mPreviewBufProvider->setBufferStatus(index,0,PreviewBufferProvider::CMD_PREVIEWBUF_WRITING); 
        } 

    } else {
        LOG2("%s(%d): buffer %d state 0x%x",__FUNCTION__,__LINE__, index, buf_state);
    }
    mCamDriverStreamLock.unlock();
    return 0;
}

int CameraAdapter::returnFrame(long index,int cmd)
{
    return adapterReturnFrame(index,cmd);
}

//define  the frame info ,such as w, h ,fmt 
int CameraAdapter::reprocessFrame(FramInfo_s* frame)
{
    //  usb camera may do something
    return 0;
}
void CameraAdapter::previewThread(){
    bool loop = true;
    FramInfo_s* tmpFrame = NULL;
    int buffer_log = 0;
    int ret = 0;
    while(loop){

        //get a frame
        //fill frame info 

        //dispatch a frame
            tmpFrame = NULL;
            mCamDriverStreamLock.lock();
            if (mCamDriverStream == false) {
                mCamDriverStreamLock.unlock();
                break;
            }
            mCamDriverStreamLock.unlock();
            
            ret = getFrame(&tmpFrame);

//            LOG2("%s(%d),frame addr = %p,%dx%d,index(%d)",__FUNCTION__,__LINE__,tmpFrame,tmpFrame->frame_width,tmpFrame->frame_height,tmpFrame->frame_index);
            if((ret!=-1) && (!camera_device_error)){
            	mPreviewBufProvider->setBufferStatus(tmpFrame->frame_index, 0,PreviewBufferProvider::CMD_PREVIEWBUF_WRITING);
                //set preview buffer status
                ret = reprocessFrame(tmpFrame);
                if(ret < 0){
                    returnFrame(tmpFrame->frame_index,buffer_log);
                    continue;
                }

                buffer_log = 0;
                //display ?
                if(mRefDisplayAdapter->isNeedSendToDisplay())
                    buffer_log |= PreviewBufferProvider::CMD_PREVIEWBUF_DISPING;
                //video enc ?
                if(mRefEventNotifier->isNeedSendToVideo())
                    buffer_log |= PreviewBufferProvider::CMD_PREVIEWBUF_VIDEO_ENCING;
                //picture ?
                if(mRefEventNotifier->isNeedSendToPicture())
    				buffer_log |= PreviewBufferProvider::CMD_PREVIEWBUF_SNAPSHOT_ENCING;
                //preview data callback ?
                if(mRefEventNotifier->isNeedSendToDataCB())
    				buffer_log |= PreviewBufferProvider::CMD_PREVIEWBUF_DATACB;
                
                mPreviewBufProvider->setBufferStatus(tmpFrame->frame_index,1,buffer_log);

                if(buffer_log & PreviewBufferProvider::CMD_PREVIEWBUF_DISPING){
                      tmpFrame->used_flag = PreviewBufferProvider::CMD_PREVIEWBUF_DISPING;
                      mRefDisplayAdapter->notifyNewFrame(tmpFrame);
                }
                if(buffer_log & (PreviewBufferProvider::CMD_PREVIEWBUF_SNAPSHOT_ENCING)){
                      tmpFrame->used_flag = PreviewBufferProvider::CMD_PREVIEWBUF_SNAPSHOT_ENCING;
                      mRefEventNotifier->notifyNewPicFrame(tmpFrame);
                }
                if(buffer_log & (PreviewBufferProvider::CMD_PREVIEWBUF_VIDEO_ENCING)){
                      tmpFrame->used_flag = PreviewBufferProvider::CMD_PREVIEWBUF_VIDEO_ENCING;
                      mRefEventNotifier->notifyNewVideoFrame(tmpFrame);
                }
                if(buffer_log & (PreviewBufferProvider::CMD_PREVIEWBUF_DATACB)){
                      tmpFrame->used_flag = PreviewBufferProvider::CMD_PREVIEWBUF_DATACB;
                      mRefEventNotifier->notifyNewPreviewCbFrame(tmpFrame);
                }
                
                if(buffer_log == 0)
                    returnFrame(tmpFrame->frame_index,buffer_log);
            }else if(camera_device_error){
                //notify app erro
                break;
            }else if((ret==-1) && (!camera_device_error)){

                    if(tmpFrame)
                        returnFrame(tmpFrame->frame_index,buffer_log);
            }
    }
    LOG_FUNCTION_NAME_EXIT
    return;
}


void CameraAdapter::autofocusThread()
{
    int err;
	LOG_FUNCTION_NAME
    while (1) {
        mAutoFocusLock.lock();
        /* check early exit request */
        if (mExitAutoFocusThread) {
            mAutoFocusLock.unlock();
            LOG1("%s(%d) exit", __FUNCTION__,__LINE__);
            goto autofocusThread_end;
        }
        mAutoFocusCond.wait(mAutoFocusLock);
		LOGD("wait out\n");
        /* check early exit request */
        if (mExitAutoFocusThread) {
            mAutoFocusLock.unlock();
            LOG1("%s(%d) exit", __FUNCTION__,__LINE__);
            goto autofocusThread_end;
        }
        mAutoFocusLock.unlock();

       	//const char *mfocusMode = params.get(CameraParameters::KEY_FOCUS_MODE);
		//if(mfocusMode) 
        	//err = cameraAutoFocus(mfocusMode/*CameraParameters::FOCUS_MODE_AUTO*/,false);
			err = cameraAutoFocus(false);
		if(mRefEventNotifier){
			mRefEventNotifier->notifyCbMsg(CAMERA_MSG_FOCUS, err);		
		}
        
    }

autofocusThread_end: 
    LOG_FUNCTION_NAME_EXIT
    return;
}

}

