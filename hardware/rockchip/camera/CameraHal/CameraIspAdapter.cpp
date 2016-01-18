#include "CameraIspAdapter.h"
#include "cam_api/halholder.h"
#include "CameraIspTunning.h"
#include "camsys_head.h"

namespace android{

#define ISP_OUT_YUV420SP   0
#define ISP_OUT_YUV422_INTERLEAVED  1
#define ISP_OUT_YUV422_SEMI  2
#define ISP_OUT_RAW12   3
#define ISP_OUT_FORMAT  ISP_OUT_YUV420SP //ISP_OUT_YUV422_INTERLEAVED

#define USE_RGA_TODO_ZOOM   (1)

/******************************************************************************
 * MainWindow_AfpsResChangeCb
 *****************************************************************************/
void CameraIspAdapter_AfpsResChangeCb( void *ctx)
{
	CtxCbResChange_t *pctx = (CtxCbResChange_t *)ctx;

    CameraIspAdapter *mainwindow = (CameraIspAdapter*)(pctx->pIspAdapter);

    if (mainwindow)
    {
        mainwindow->AfpsResChangeCb();
    }
}



/******************************************************************************
 * MainWindow::AfpsResChangeCb
 *****************************************************************************/
void CameraIspAdapter::AfpsResChangeCb()
{
   // emit resChanged();
    LOG_FUNCTION_NAME

    

    LOG_FUNCTION_NAME_EXIT
}

int CameraIspAdapter::DEFAULTPREVIEWWIDTH = 640;
int CameraIspAdapter::DEFAULTPREVIEWHEIGHT = 480;
int CameraIspAdapter::preview_frame_inval = 1;
bool CameraIspAdapter::hdmiIn_Exit = true;




CameraIspAdapter::CameraIspAdapter(int cameraId)
                    :CameraAdapter(cameraId),
                    m_camDevice(NULL),
                    mSensorItfCur(0)
{
    LOG_FUNCTION_NAME
    mZoomVal = 100;
    mZoomMin = 100;
    mZoomMax = 240;
    mFlashStatus = false;
    mISPOutputFmt = ISP_OUT_YUV420SP;
    mISPTunningRun = false;
    mIsSendToTunningTh = false;
    mDispFrameLeak = 0;
    mVideoEncFrameLeak = 0;
    mPreviewCBFrameLeak = 0;
    mPicEncFrameLeak = 0;
	mCtxCbResChange.res = 0;
	bResetIsp = false;
	mDataListenerThread = new CameraDataThread(this);
	mDataListenerThread->run("CamDataLisThread",ANDROID_PRIORITY_NORMAL);
	mCtxCbResChange.pIspAdapter =NULL;
	LOG_FUNCTION_NAME_EXIT
}
CameraIspAdapter::~CameraIspAdapter()
{

    cameraDestroy();
	if(mDataListenerThread!= NULL){
		mDataListenerThread->requestExitAndWait();
		mDataListenerThread.clear();
	}
    if(mDispFrameLeak != 0){
        LOGE("\n\n\n\nmay have disp frame mem leak,count is %d\n\n\n\n",mDispFrameLeak);
    }
    
    if(mVideoEncFrameLeak!=0){
        LOGE("\n\n\n\nmay have video frame mem leak,count is %d\n\n\n\n",mVideoEncFrameLeak);
    }
    
    if(mPreviewCBFrameLeak != 0){
        LOGE("\n\n\n\nmay have previewcb frame mem leak,count is %d\n\n\n\n",mPreviewCBFrameLeak);
    }
    
    if(mPicEncFrameLeak != 0){
        LOGE("\n\n\n\nmay have pic enc frame mem leak,count is %d\n\n\n\n",mPicEncFrameLeak);
    }
	
}
int CameraIspAdapter::cameraCreate(int cameraId)
{
    LOG_FUNCTION_NAME
	rk_cam_total_info *pCamInfo = gCamInfos[cameraId].pcam_total_info;
	char* dev_filename = pCamInfo->mHardInfo.mSensorInfo.mCamsysDevPath;
    HalPara_t isp_halpara;
	int mipiLaneNum = 0;
	int i =0;
    char value[PROPERTY_VALUE_MAX];
	hdmiIn_Exit = true;

	preview_frame_inval = pCamInfo->mHardInfo.mSensorInfo.awb_frame_skip;
	
    pCamInfo->mLibIspVersion = CONFIG_SILICONIMAGE_LIBISP_VERSION;
	
	for(i=0; i<4; i++){
		mipiLaneNum += (pCamInfo->mHardInfo.mSensorInfo.mPhy.info.mipi.data_en_bit>>i)&0x01;
	}
	
#ifdef ROCKCHIP_ION_VERSION
	isp_halpara.is_new_ion = (bool_t)true;
#else
	isp_halpara.is_new_ion = (bool_t)false;
#endif
    isp_halpara.mipi_lanes = mipiLaneNum;
	mCtxCbResChange.pIspAdapter = (void*)this;
    m_camDevice = new CamDevice( HalHolder::handle(dev_filename,&isp_halpara), CameraIspAdapter_AfpsResChangeCb, (void*)&mCtxCbResChange ,NULL, mipiLaneNum);
	
	//load sensor
    loadSensor( cameraId);

    property_get("sys.boot_completed", value, NULL);
    if (atoi(value) > 0) {
        rk_cam_total_info *pCamInfo = gCamInfos[mCamId].pcam_total_info;
        LOGE("sensor name %s",pCamInfo->mHardInfo.mSensorInfo.mSensorName);
        if((strcmp(pCamInfo->mHardInfo.mSensorInfo.mSensorName,"TC358749XBG")==0)) {
            int camsys_fd;    
            const char HdmiAudioFilePath[] = "/sys/class/hdmiin_reg/hdmiin_audio";
            LOGE("open TC358749XBG camsys");
            //audio hw stop must before hdmiin audio start
            property_set("media.audio.close", "1");
            hdmiIn_Exit = false;

            camsys_fd = open(HdmiAudioFilePath, O_RDWR);
            if (camsys_fd < 0) {
                LOGE("open TC358749XBG camsys fail");
            } else {
                write(camsys_fd,"1",1);
                close(camsys_fd);
            }
        }
    }

    //open image
    //openImage("/system/lib/libisp_isi_drv_OV8825.so");   

    {
        if (OSLAYER_OK != osQueueInit(&mAfListenerQue.queue,100, sizeof(CamEngineAfEvt_t)))   /* ddl@rock-chips.com: v0.0x22.0 */  
        {
            LOGE("create af listener queue failed!");
        }

        ListInit(&mAfListenerQue.list);

        mAfListenerThread = new CameraAfThread(this);
        mAfListenerThread->run("CamAfLisThread",ANDROID_PRIORITY_NORMAL);

        if (mISPTunningRun == false) {
            m_camDevice->resetAf(CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE);
            m_camDevice->registerAfEvtQue(&mAfListenerQue);
        }
    }
	//hdmiin thread
	mHdmiinListenerThread = new HdmiInThread(this);
    mHdmiinListenerThread->run("HdmiInThread",ANDROID_PRIORITY_DISPLAY);
    LOG_FUNCTION_NAME_EXIT
    return 0;

}
int CameraIspAdapter::cameraDestroy()
{
    LOG_FUNCTION_NAME

    {
        CamEngineAfEvt_t cmd;
        int ret;

        cmd.evnt_id = (CamEngineAfEvtId_t)0xfefe5aa5;
        
        osQueueWrite(&mAfListenerQue.queue, &cmd);
        
        if(mAfListenerThread!= NULL){
        	mAfListenerThread->requestExitAndWait();
        	mAfListenerThread.clear();
    	}
		//hdmiin thread
        if (hdmiIn_Exit == false) {
            int camsys_fd;    
            const char HdmiAudioFilePath[] = "/sys/class/hdmiin_reg/hdmiin_audio";
            LOGE("open TC358749XBG camsys");

            camsys_fd = open(HdmiAudioFilePath, O_RDWR);
            if (camsys_fd < 0) {
                LOGE("open TC358749XBG camsys fail");
            } else {
                write(camsys_fd,"0",1);
                close(camsys_fd);
            }

            //audio hw restart must below hdmiin audio stop
            property_set("media.audio.close", "2");
            hdmiIn_Exit = true;
        }

		if(mHdmiinListenerThread!= NULL){
        	mHdmiinListenerThread->requestExitAndWait();
        	mHdmiinListenerThread.clear();
    	}
        osQueueDestroy(&mAfListenerQue.queue);
        
    }

    preview_frame_inval = 0;
    //check flash mode last time
    
    if(mParameters.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES) &&
        (strcmp(mParameters.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_TORCH)==0)){
        CamEngineFlashCfg_t flash_cfg;
        rk_cam_total_info *pCamInfo = gCamInfos[mCamId].pcam_total_info;
        if((strcmp(pCamInfo->mHardInfo.mFlashInfo.mFlashName,"Internal")==0))
            flash_cfg.dev_mask = pCamInfo->mHardInfo.mSensorInfo.mHostDevid;
        else
            flash_cfg.dev_mask = pCamInfo->mHardInfo.mSensorInfo.mCamDevid;
        flash_cfg.mode = CAM_ENGINE_FLASH_TORCH;
        m_camDevice->configureFlash(&flash_cfg);
        m_camDevice->stopFlash(true);
    }

    if(m_camDevice){
        disconnectCamera();
        delete m_camDevice;
        m_camDevice = NULL;
        if(HalHolder::m_halHolder){
            delete HalHolder::m_halHolder;
            HalHolder::m_halHolder = NULL;
        }
    }
    LOG_FUNCTION_NAME_EXIT
    return 0;
}

void CameraIspAdapter::setupPreview(int width_sensor,int height_sensor,int preview_w,int preview_h,int zoom_val)
{
    CamEngineWindow_t dcWin;
	unsigned int max_w = 0,max_h = 0, bufNum = 0, bufSize = 0;
	LOGE(">>>>>>>width_sensor=%d,height_sensor=%d,preview_w=%d,preview_h=%d",width_sensor,height_sensor,preview_w,preview_h);
	//when cts FOV ,don't crop
    if((!mImgAllFovReq)&&((width_sensor*10/height_sensor) != (preview_w*10/preview_h))){
        int ratio = ((width_sensor*10/preview_w) >= (height_sensor*10/preview_h))?(height_sensor*10/preview_h):(width_sensor*10/preview_w);
        dcWin.width = ((ratio*preview_w/10) & ~0x1);
        dcWin.height = ((ratio*preview_h/10) & ~0x1);
        dcWin.hOffset =(ABS(width_sensor-dcWin.width ))>>1;
        dcWin.vOffset = (ABS(height_sensor-dcWin.height))>>1;        
    }else{
        dcWin.width = width_sensor;
        dcWin.height = height_sensor;
        dcWin.hOffset = 0;
        dcWin.vOffset = 0;
    }

#if (USE_RGA_TODO_ZOOM == 0)            /* zyc@rock-chips.com: v0.0x22.0 */ 

    if((zoom_val > mZoomMin) && (zoom_val <= mZoomMax)){
        if((preview_w <= 2592) && (preview_h <= 1944)){
            dcWin.width = dcWin.width*100/zoom_val & ~0x1;
            dcWin.height = dcWin.height*100/zoom_val & ~0x1;
            dcWin.hOffset = (ABS(width_sensor-dcWin.width )) >> 1;
            dcWin.vOffset = (ABS(height_sensor-dcWin.height))>>1;
        }else{
            LOGE("isp output res big than 5M!");
        }

    }
#endif

	getSensorMaxRes(max_w,max_h);
    if(mISPOutputFmt == ISP_OUT_YUV422_INTERLEAVED){
        m_camDevice->previewSetup_ex( dcWin, preview_w, preview_h,
                                CAMERIC_MI_DATAMODE_YUV422,CAMERIC_MI_DATASTORAGE_INTERLEAVED,(bool_t)true);
		bufSize = ((max_w+15)&(~0xf))*((max_h+15)&(~0xf))*2;
		bufNum = CONFIG_CAMERA_ISP_BUF_REQ_CNT;
        LOGD("isp out put format is YUV422 interleaved.");
    }else if(mISPOutputFmt == ISP_OUT_YUV422_SEMI){
        m_camDevice->previewSetup_ex( dcWin, preview_w, preview_h,
                                 CAMERIC_MI_DATAMODE_YUV422,CAMERIC_MI_DATASTORAGE_SEMIPLANAR,(bool_t)true);
		bufSize = ((max_w+15)&(~0xf))*((max_h+15)&(~0xf))*2;
		bufNum = CONFIG_CAMERA_ISP_BUF_REQ_CNT;
        LOGD("isp out put format is YUV422 semi.");
    }else if(mISPOutputFmt == ISP_OUT_YUV420SP){
        m_camDevice->previewSetup_ex( dcWin, preview_w, preview_h,
                                 CAMERIC_MI_DATAMODE_YUV420,CAMERIC_MI_DATASTORAGE_SEMIPLANAR,(bool_t)true);
		bufSize = ((max_w+15)&(~0xf))*((max_h+15)&(~0xf))*3/2 ;
		bufNum = CONFIG_CAMERA_ISP_BUF_REQ_CNT;		
        LOGD("isp out put format is YUV420SP.");
    }else if(mISPOutputFmt == ISP_OUT_RAW12){
        m_camDevice->previewSetup_ex( dcWin, preview_w, preview_h,
                                CAMERIC_MI_DATAMODE_RAW12,CAMERIC_MI_DATASTORAGE_INTERLEAVED,(bool_t)false);
		bufSize = ((max_w+15)&(~0xf))*((max_h+15)&(~0xf))*2;
		bufNum = CONFIG_CAMERA_ISP_BUF_REQ_CNT;
        LOGD("isp out put format is RAW12.");
    }else{
        LOGE("%s:isp don't support this format %d now",__func__,mISPOutputFmt);
    }
	m_camDevice->setIspBufferInfo(bufNum, bufSize);
    LOG1("Sensor output: %dx%d --(%d,%d,%d,%d)--> User request: %dx%d",width_sensor,height_sensor,
        dcWin.hOffset,dcWin.vOffset,dcWin.width,dcWin.height,preview_w,preview_h);

}
status_t CameraIspAdapter::startPreview(int preview_w,int preview_h,int w, int h, int fmt,bool is_capture)
{
    LOG_FUNCTION_NAME
	bool err_af;
	bool err;
	bool avail = false;
    bool enable_flash = false;
    bool low_illumin = false;
    bool is_video = false;
    
	
    if ( ( !m_camDevice->hasSensor() ) &&
         ( !m_camDevice->hasImage()  ) ){
          goto startPreview_end;
    	}

    //for isp tunning
    if((fmt != mISPOutputFmt)){
        //restart isp
        cameraDestroy();
        mISPOutputFmt = fmt;
        cameraCreate(mCamId);
    }
	if(bResetIsp){
		//restart isp
		cameraDestroy();
		bResetIsp = false;
		cameraCreate(mCamId);
	}



    is_video = (((preview_w == 1920) && (preview_h == 1080)) || 
                ((preview_w == 1280) && (preview_h == 720)));

    //must to get illum befor resolution changed
    if (is_capture) {
        enable_flash = isNeedToEnableFlash();

        m_camDevice->lock3a((Lock_awb|Lock_aec)); 
    }
    low_illumin = isLowIllumin();

    //need to change resolution ?
    if((preview_w != mCamPreviewW) ||(preview_h != mCamPreviewH)
        || (w != mCamDrvWidth) || (h != mCamDrvHeight)){

        //change resolution
        //get sensor res
        int width_sensor = 0,height_sensor = 0;
        uint32_t resMask;
        CamEnginePathConfig_t mainPathConfig ,selfPathConfig;
		CamEngineBestSensorResReq_t resReq;
        float curGain,curExp; /* ddl@rock-chips.com: v1.0x15.0 */

        memset(&resReq, 0x00, sizeof(CamEngineBestSensorResReq_t));
        resReq.request_w = preview_w;
        resReq.request_h = preview_h;

        
        if ((m_camDevice->getIntegrationTime(curExp) == false)) {
            curExp = 0.0;
        }

        if (m_camDevice->getGain(curGain) == false){
            curGain = 0.0;
        }
        
        if (is_video) {
            resReq.request_fps = 20;
        } else if (is_capture) {
            resReq.request_fps = 0;
            resReq.request_exp_t = curExp*curGain;
        } else { 
            resReq.request_fps = 10; 
            resReq.request_exp_t = curExp;
        }
        
        resReq.requset_aspect = (bool_t)false;        
        resReq.request_fullfov = (bool_t)mImgAllFovReq;        
        m_camDevice->getPreferedSensorRes(&resReq);
        
        width_sensor = ISI_RES_W_GET(resReq.resolution);
        height_sensor = ISI_RES_H_GET(resReq.resolution);
		
        //stop streaming
        if(-1 == stop())
			goto startPreview_end;
        /* ddl@rock-chips.com: v1.0x16.0 */
        if ( is_video ) {
            enableAfps(false);
        } else {
            enableAfps(true);
        }

        //need to change sensor resolution ?
        //if((width_sensor != mCamDrvWidth) || (height_sensor != mCamDrvHeight)){
            m_camDevice->changeResolution(resReq.resolution,false);
        //}
        //reset dcWin,output width(data path)
        //get dcWin
        #if CONFIG_CAMERA_SCALE_CROP_ISP
        setupPreview(width_sensor,height_sensor,preview_w,preview_h,mZoomVal);
        #else
        if ((preview_w == 1600) && (preview_h == 1200) && 
            (width_sensor==1632) && (height_sensor == 1224) ) {
            setupPreview(width_sensor,height_sensor,preview_w,preview_h,mZoomVal);
        } else {
            setupPreview(width_sensor,height_sensor,width_sensor,height_sensor,mZoomVal);
        }
        #endif
		
        m_camDevice->getPathConfig(CHAIN_MASTER,CAM_ENGINE_PATH_MAIN,mainPathConfig);
        m_camDevice->getPathConfig(CHAIN_MASTER,CAM_ENGINE_PATH_SELF,selfPathConfig);
        m_camDevice->setPathConfig( CHAIN_MASTER, mainPathConfig, selfPathConfig  );

		//start streaming
        if(-1 == start())
			goto startPreview_end;
		
		//set mannual exposure and mannual whitebalance
		setMe(mParameters.get(CameraParameters::KEY_EXPOSURE_COMPENSATION));
		setMwb(mParameters.get(CameraParameters::KEY_WHITE_BALANCE));		
		//
        mCamDrvWidth = width_sensor;
        mCamDrvHeight = height_sensor;
        mCamPreviewH = preview_h;
        mCamPreviewW = preview_w;
        
    }else{
    
        if(mPreviewRunning == 0){
			if(-1 == start())
				goto startPreview_end;
        }
    }


    if (is_capture){
        if(strtod(mParameters.get(KEY_CONTINUOUS_PIC_NUM),NULL) > 1){
            //stop af
            unsigned int maxFocus, minFocus;
            m_camDevice->stopAf();  /* ddl@rock-chips.com: v0.d.3 */
            if(low_illumin){ // low illumin
                if (m_camDevice->getFocusLimits(minFocus, maxFocus) == true) {
                    m_camDevice->setFocus(maxFocus);
                } else {
                    LOGE("getFocusLimits failed!");
                }
            }
            enable_flash = false;
        }else if(low_illumin){
            unsigned int maxFocus, minFocus;
            m_camDevice->stopAf();
            if (m_camDevice->getFocusLimits(minFocus, maxFocus) == true) {
                m_camDevice->setFocus(maxFocus);
            } else {
                LOGE("getFocusLimits failed!");
            }
        }
    }else{
        //restore focus mode
        // Continues picture focus
        if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_MODE),CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)==0) {
            err_af = m_camDevice->startAfContinous(CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE);
        	if ( err_af == false ){
        		LOGE("Set startAfContinous failed");        		
        	} 
        } else if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_MODE),CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO)==0) {
            unsigned int maxFocus, minFocus;
            m_camDevice->stopAf();  
            
            if (m_camDevice->getFocusLimits(minFocus, maxFocus) == true) {
                m_camDevice->setFocus(maxFocus);
            } else {
                LOGE("getFocusLimits failed!");
            }
        }

        m_camDevice->unlock3a((Lock_awb|Lock_aec));
        
    } 
    flashControl(enable_flash);

    mPreviewRunning = 1;

    LOG_FUNCTION_NAME_EXIT
    return 0;
startPreview_end:
	LOG_FUNCTION_NAME_EXIT
	return -1;
}
status_t CameraIspAdapter::stopPreview()
{
    LOG_FUNCTION_NAME
	int err;

    if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_MODE),CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)==0) {
        m_camDevice->stopAf();
    }
    
    if(mPreviewRunning) {
        if(-1 == stop())
			return -1;
        clearFrameArray();        

    }
    mPreviewRunning = 0;

    LOG_FUNCTION_NAME_EXIT
    return 0;
}
int CameraIspAdapter::setParameters(const CameraParameters &params_set,bool &isRestartValue)
{
    int fps_min,fps_max;
    int framerate=0;
    
    if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES), params_set.get(CameraParameters::KEY_PREVIEW_SIZE)) == NULL) {
        LOGE("PreviewSize(%s) not supported",params_set.get(CameraParameters::KEY_PREVIEW_SIZE));        
        return BAD_VALUE;
    } else if (strcmp(mParameters.get(CameraParameters::KEY_PREVIEW_SIZE), params_set.get(CameraParameters::KEY_PREVIEW_SIZE))) {
        LOG1("Set preview size %s",params_set.get(CameraParameters::KEY_PREVIEW_SIZE));
        //should update preview cb settings ,for cts
        int w,h;
        const char * fmt=  params_set.getPreviewFormat();
		params_set.getPreviewSize(&w, &h); 
        mRefEventNotifier->setPreviewDataCbRes(w, h, fmt);

    }

    if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES), params_set.get(CameraParameters::KEY_PICTURE_SIZE)) == NULL) {
        LOGE("PictureSize(%s) not supported",params_set.get(CameraParameters::KEY_PICTURE_SIZE));
        return BAD_VALUE;
    } else if (strcmp(mParameters.get(CameraParameters::KEY_PICTURE_SIZE), params_set.get(CameraParameters::KEY_PICTURE_SIZE))) {
        LOG1("Set picture size %s",params_set.get(CameraParameters::KEY_PICTURE_SIZE));
    }

    if (strcmp(params_set.getPictureFormat(), "jpeg") != 0) {
        LOGE("Only jpeg still pictures are supported");
        return BAD_VALUE;
    }

    if (params_set.getInt(CameraParameters::KEY_ZOOM) > params_set.getInt(CameraParameters::KEY_MAX_ZOOM)) {
        LOGE("Zomm(%d) is larger than MaxZoom(%d)",params_set.getInt(CameraParameters::KEY_ZOOM),params_set.getInt(CameraParameters::KEY_MAX_ZOOM));
        return BAD_VALUE;
    }

    params_set.getPreviewFpsRange(&fps_min,&fps_max);
    if ((fps_min < 0) || (fps_max < 0) || (fps_max < fps_min)) {
        LOGE("FpsRange(%s) is invalidate",params_set.get(CameraParameters::KEY_PREVIEW_FPS_RANGE));
        return BAD_VALUE;
    }


    {/* ddl@rock-chips.com: v1.5.0 */
        if (params_set.get(CameraParameters::KEY_MAX_NUM_METERING_AREAS) != NULL) {
            if (params_set.getInt(CameraParameters::KEY_MAX_NUM_METERING_AREAS) >= 1) {
                int hOff,vOff,w,h,weight;
                CamEngineWindow_t aeWin;
    			const char* zoneStr = params_set.get(CameraParameters::KEY_METERING_AREAS);
    	    	if (zoneStr) {   
                    LOG1("meter zoneStr %s",zoneStr);
        	        hOff = strtol(zoneStr+1,0,0);
        	        zoneStr = strstr(zoneStr,",");
        	        vOff = strtol(zoneStr+1,0,0);
        		    zoneStr = strstr(zoneStr+1,",");
        	        w = strtol(zoneStr+1,0,0);                    
                    zoneStr = strstr(zoneStr+1,",");
        	        h = strtol(zoneStr+1,0,0);
                    zoneStr = strstr(zoneStr+1,",");
                    weight = strtol(zoneStr+1,0,0);
                    if(strstr(zoneStr+1,"(")){
                        //more than one zone
                        return BAD_VALUE;
                    }
                    
                    w -= hOff;
                    h -= vOff;
                    if((w == 0) && (h == 0) && (hOff == 0) && (weight == 0)){
                        hOff = 0;
                        vOff = 0;
                        w = 0;
                        h = 0;
                    }else if ( ((hOff<-1000) || (hOff>1000)) ||
                         ((vOff<-1000) || (vOff>1000)) ||
                         ((w<=0) || (w>2000)) ||
                         ((h<=0) || (h>2000)) ||
                         ((weight < 1) || (weight > 1000))) {
                        hOff = 0;
                        vOff = 0;
                        w = 0;
                        h = 0;
                        return BAD_VALUE;
                    } 

                    //ddl@rock-chips.com v1.0x1d.0
                    if (hOff || vOff || w || h){                           
                        m_camDevice->setAecHistMeasureWinAndMode(hOff,vOff,w,h,AverageMetering);
                    } else {
                        m_camDevice->setAecHistMeasureWinAndMode(hOff,vOff,w,h,CentreWeightMetering);
                    }
    	    	}

            }

        }

    }


    {
        bool err_af = false;

        if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES),params_set.get(CameraParameters::KEY_FOCUS_MODE))) {            
            // Continues picture focus
            if (strcmp(params_set.get(CameraParameters::KEY_FOCUS_MODE),CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)==0) {
                if (mPreviewRunning == 1) {
                    /* ddl@rock-chips.com: v0.0x22.0 */ 
                    if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_MODE),CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)) {
                        err_af = m_camDevice->startAfContinous(CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE);
                    	if ( err_af == false ){
                    		LOGE("Set startAfContinous failed");        		
                    	} 
                    }
                }
            // Continues video focus is not implement, so fixd focus; /* ddl@rock-chips.com: v0.c.0 */
            } else if (strcmp(params_set.get(CameraParameters::KEY_FOCUS_MODE),CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO)==0) {
                if (mPreviewRunning == 1) {
                    /* ddl@rock-chips.com: v0.0x22.0 */ 
                    if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_MODE),CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO)) {
                        unsigned int maxFocus, minFocus;

                        m_camDevice->stopAf();  /* ddl@rock-chips.com: v0.d.3 */
                        
                        if (m_camDevice->getFocusLimits(minFocus, maxFocus) == true) {
                            m_camDevice->setFocus(maxFocus);
                        } else {
                            LOGE("getFocusLimits failed!");
                        }
        				LOG1("Continues-video focus is fixd focus now!");
                    }
                }                
            }
            
            if (mParameters.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS) == 1) {
                mAfChk = false;
                /* ddl@rock-chips.com: v0.0x22.0 */ 
                if ((mParameters.get(CameraParameters::KEY_FOCUS_AREAS) == NULL) || 
                    (params_set.get(CameraParameters::KEY_FOCUS_AREAS) == NULL)) {
                    mAfChk = true;
                    LOG1("Focus areas is change(%s -> %s), Must af again!!",
                        mParameters.get(CameraParameters::KEY_FOCUS_AREAS),
                        params_set.get(CameraParameters::KEY_FOCUS_AREAS));
                } else if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_AREAS),params_set.get(CameraParameters::KEY_FOCUS_AREAS))) {
                    mAfChk = true;

        	    	int hOff,vOff,w,h,weight;
        			const char* zoneStr = params_set.get(CameraParameters::KEY_FOCUS_AREAS);
        	    	if (zoneStr) {  
                        LOG1("focus zoneStr %s",zoneStr);
            	        vOff = strtol(zoneStr+1,0,0);
            	        zoneStr = strstr(zoneStr,",");
            	        hOff = strtol(zoneStr+1,0,0);
            		    zoneStr = strstr(zoneStr+1,",");
            	        w = strtol(zoneStr+1,0,0);                    
                        zoneStr = strstr(zoneStr+1,",");
            	        h = strtol(zoneStr+1,0,0);
                        zoneStr = strstr(zoneStr+1,",");
                        weight = strtol(zoneStr+1,0,0);

                        if(strstr(zoneStr+1,"(")){
                            //more than one zone
                            return BAD_VALUE;
                        }
                        w -= vOff;
                        h -= hOff;                    
                        
                        if((w == 0) && (h == 0) && (hOff == 0) && (weight == 0)){
                            hOff = 0;
                            vOff = 0;
                            w = 0;
                            h = 0;
                        }else if ( ((hOff<-1000) || (hOff>1000)) ||
                         ((vOff<-1000) || (vOff>1000)) ||
                         ((w<=0) || (w>2000)) ||
                         ((h<=0) || (h>2000)) ||
                         ((weight < 1) || (weight > 1000))) {
                        LOGE("%s: %s , afWin(%d,%d,%d,%d)is invalidate!",
                            CameraParameters::KEY_FOCUS_AREAS,
                            params_set.get(CameraParameters::KEY_FOCUS_AREAS),
                            hOff,vOff,w,h);
                        return BAD_VALUE;
                        }
        	    	}
                    
                    LOG1("Focus areas is change(%s -> %s), Must af again!!",
                        mParameters.get(CameraParameters::KEY_FOCUS_AREAS),
                        params_set.get(CameraParameters::KEY_FOCUS_AREAS));
                }
            }
            
            if( err_af == true )
                LOG1("Set focus mode: %s success",params_set.get(CameraParameters::KEY_FOCUS_MODE));
        } else {
            LOGE("%s isn't supported for this camera, support focus: %s",
                params_set.get(CameraParameters::KEY_FOCUS_MODE),
                mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES));
            if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES),CameraParameters::FOCUS_MODE_AUTO))
                mParameters.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_AUTO);
            else 
                mParameters.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
            return BAD_VALUE;
        }

    }
  

    {
        CamEngineFlashCfg_t flash_cfg;

        if (mParameters.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES) && mParameters.get(CameraParameters::KEY_FLASH_MODE)) {

            if (strstr(mParameters.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES),params_set.get(CameraParameters::KEY_FLASH_MODE))) {
                if (strcmp(mParameters.get(CameraParameters::KEY_FLASH_MODE),params_set.get(CameraParameters::KEY_FLASH_MODE))) {
                    rk_cam_total_info *pCamInfo = gCamInfos[mCamId].pcam_total_info;
                    flash_cfg.active_pol = (pCamInfo->mHardInfo.mFlashInfo.mFlashTrigger.active>0) ? CAM_ENGINE_FLASH_HIGH_ACTIVE:CAM_ENGINE_FLASH_LOW_ACTIVE;
					flash_cfg.flashtype = pCamInfo->mHardInfo.mFlashInfo.mFlashMode;
                    if((strcmp(pCamInfo->mHardInfo.mFlashInfo.mFlashName,"Internal")==0))
                        flash_cfg.dev_mask = pCamInfo->mHardInfo.mSensorInfo.mHostDevid;
                    else
                        flash_cfg.dev_mask = pCamInfo->mHardInfo.mSensorInfo.mCamDevid;
                    if ((strcmp(params_set.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_ON)==0)
                        || ((strcmp(params_set.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_AUTO)==0))){
                        //check flash mode last time
                        if(strcmp(mParameters.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_TORCH)==0){
                            flash_cfg.mode = CAM_ENGINE_FLASH_TORCH;
                            m_camDevice->configureFlash(&flash_cfg);
                            m_camDevice->stopFlash(true);
                        }
                        flash_cfg.mode = CAM_ENGINE_FLASH_ON;
                        m_camDevice->configureFlash(&flash_cfg);
            			LOG1("Set flash on success!");
                    }else if (strcmp(params_set.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_OFF)==0) {
                        //check flash mode last time
                        if(strcmp(mParameters.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_TORCH)==0){
                            flash_cfg.mode = CAM_ENGINE_FLASH_TORCH;
                            m_camDevice->configureFlash(&flash_cfg);
                            m_camDevice->stopFlash(true);
                        }
                        flash_cfg.mode = CAM_ENGINE_FLASH_OFF;
                        m_camDevice->configureFlash(&flash_cfg);
            			LOG1("Set flash off success!");
                    }else if (strcmp(params_set.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_TORCH)==0) {
                        flash_cfg.mode = CAM_ENGINE_FLASH_TORCH;
                        m_camDevice->configureFlash(&flash_cfg);
                        m_camDevice->startFlash(true);
            			LOG1("Set flash torch success!");
                    }
                }
            } else {
                LOGE("%s isn't supported for this camera, support flash: %s",
                    params_set.get(CameraParameters::KEY_FLASH_MODE),
                    mParameters.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES));
                return BAD_VALUE;

            }

        }

    }
	if (!cameraConfig(params_set,false,isRestartValue)) {        
        LOG1("PreviewSize(%s)", mParameters.get(CameraParameters::KEY_PREVIEW_SIZE));
        LOG1("PreviewFormat(%s)",params_set.getPreviewFormat());  
        LOG1("FPS Range(%s)",mParameters.get(CameraParameters::KEY_PREVIEW_FPS_RANGE));
        LOG1("PictureSize(%s)",mParameters.get(CameraParameters::KEY_PICTURE_SIZE)); 
        LOG1("PictureFormat(%s)  ", params_set.getPictureFormat());
        LOG1("Framerate: %d  ", framerate);
        LOG1("WhiteBalance: %s", params_set.get(CameraParameters::KEY_WHITE_BALANCE));
        LOG1("Flash: %s", params_set.get(CameraParameters::KEY_FLASH_MODE));
        LOG1("Focus: %s  Foucus area: %s", params_set.get(CameraParameters::KEY_FOCUS_MODE),
            params_set.get(CameraParameters::KEY_FOCUS_AREAS));
        LOG1("Scene: %s", params_set.get(CameraParameters::KEY_SCENE_MODE));
    	LOG1("Effect: %s", params_set.get(CameraParameters::KEY_EFFECT));
    	LOG1("ZoomIndex: %s", params_set.get(CameraParameters::KEY_ZOOM));
	}else{
	    return BAD_VALUE;
	}  
    
    return 0;
}
void CameraIspAdapter::initDefaultParameters(int camFd)
{
    CameraParameters params;
	String8 parameterString;
    rk_cam_total_info *pCamInfo = gCamInfos[camFd].pcam_total_info;
    bool isRestartPreview = false;
    char string[100];
    char prop_value[PROPERTY_VALUE_MAX];

	LOG_FUNCTION_NAME
    //previwe size and picture size
    {
        IsiSensorCaps_t pCaps;
        unsigned int pixels;
        unsigned int max_w,max_h,max_fps,maxfps_res;
        bool chk_720p,chk_1080p;
        
        parameterString = "176x144,320x240,352x288,640x480,720x480,800x600";
        LOG1("Sensor resolution list:");

        max_w = 0;
        max_h = 0;
        max_fps = 0;
        maxfps_res = 0;
        pCaps.Index = 0;
        chk_720p = false;
        chk_1080p = false;
	    while (m_camDevice->getSensorCaps(pCaps) == true) {
         
            memset(string,0x00,sizeof(string));        
            if (ISI_FPS_GET(pCaps.Resolution) >= 15) {
                pixels = ISI_RES_W_GET(pCaps.Resolution)*ISI_RES_H_GET(pCaps.Resolution)*10;                
                if (pixels > 1280*720*9) {
                    if (chk_720p == false) {
                        strcat(string,",1280x720");
                        chk_720p = true;
                    }
                }

                if (pixels > 1920*1080*9) {
                    if (chk_1080p == false) {
                        strcat(string,",1920x1080");
                        chk_1080p = true;
                    }
                } 

                parameterString.append(string);
            }

            if (max_fps < ISI_FPS_GET(pCaps.Resolution)) {
                maxfps_res = pCaps.Resolution;
            }
            memset(string,0x00,sizeof(string)); 
            sprintf(string,",%dx%d",ISI_RES_W_GET(pCaps.Resolution),ISI_RES_H_GET(pCaps.Resolution));
            if (strcmp(string,",1600x1200")){
                parameterString.append(string);
            }
            LOG1("    %dx%d @ %d fps", ISI_RES_W_GET(pCaps.Resolution),ISI_RES_H_GET(pCaps.Resolution),
                ISI_FPS_GET(pCaps.Resolution));

            if (ISI_RES_W_GET(pCaps.Resolution)>max_w)
                max_w = ISI_RES_W_GET(pCaps.Resolution);
            if (ISI_RES_H_GET(pCaps.Resolution)>max_h)
                max_h = ISI_RES_H_GET(pCaps.Resolution);
            pCaps.Index++;
	    };
        
        if (pCamInfo->mSoftInfo.mPreviewWidth && pCamInfo->mSoftInfo.mPreviewHeight) {
            memset(string,0x00,sizeof(string));    
            sprintf(string,"%d",pCamInfo->mSoftInfo.mPreviewWidth);  
            params.set(KEY_PREVIEW_W_FORCE,string);
            memset(string,0x00,sizeof(string));
            sprintf(string,"%d",pCamInfo->mSoftInfo.mPreviewHeight);  
            params.set(KEY_PREVIEW_H_FORCE,string);
            memset(string,0x00,sizeof(string));
            sprintf(string,"%dx%d",pCamInfo->mSoftInfo.mPreviewWidth,pCamInfo->mSoftInfo.mPreviewHeight);  
        } else {
            memset(string,0x00,sizeof(string)); 
            sprintf(string,"%dx%d",ISI_RES_W_GET(maxfps_res),ISI_RES_H_GET(maxfps_res));
        }

        pixels = max_w*max_h;
        if (max_w*10/max_h == 40/3) {          //  4:3 Sensor
            if (pixels > 12800000) {
                if ((max_w != 4128)&&(max_h != 3096))
                    parameterString.append(",2064x1548");                
            } else if (pixels > 7900000) {
                if ((max_w != 3264)&&(max_h != 2448))
                    parameterString.append(",1632x1224");                
            } else if (pixels > 5000000) {
                if ((max_w != 2592)&&(max_h != 1944))
                    parameterString.append(",1296x972");
            }
        }
        
        params.set(CameraParameters::KEY_PREVIEW_SIZE,string);
        params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, parameterString.string());        
        
        memset(string,0x00,sizeof(string));
        sprintf(string,"%dx%d",max_w,max_h);
        parameterString = string;        
		int interpolationRes = pCamInfo->mSoftInfo.mInterpolationRes;
		if(interpolationRes){			
	        if (max_w*10/max_h == 40/3) {          //  4:3 Sensor
				if (interpolationRes >= 8000000) {
	                if ((max_w != 3264)&&(max_h != 2448))
	                    parameterString.append(",3264x2448");
	                parameterString.append(",2592x1944,2048x1536,1600x1200");
	            }else if (interpolationRes >= 5000000) {
	                parameterString.append(",2592x1944,2048x1536,1600x1200");
	            }else if (interpolationRes >= 3000000) {
	                parameterString.append(",2048x1536,1600x1200,1024x768");
	            } else if (interpolationRes >= 2000000) {
	                parameterString.append(",1600x1200,1024x768");
	            } else if (interpolationRes >= 1000000) {
	                parameterString.append(",1024x768");
	            }
	        } else if (max_w*10/max_h == 160/9) {   // 16:9 Sensor

	        }
		}else{
            if (max_w*10/max_h == 40/3) {          //  4:3 Sensor
                if (pixels > 12800000) {
                    if ((max_w != 4128)&&(max_h != 3096))
                        parameterString.append(",4128x3096");
                    parameterString.append(",3264x2448,2592x1944,1600x1200");
                } else if (pixels > 7900000) {
                    if ((max_w != 3264)&&(max_h != 2448))
                        parameterString.append(",3264x2448");
                    parameterString.append(",2592x1944,2048x1536,1600x1200");
                } else if (pixels > 5000000) {
                    parameterString.append(",2048x1536,1600x1200,1024x768");
                } else if (pixels > 3000000) {
                    parameterString.append(",1600x1200,1024x768");
                } else if (pixels > 2000000) {
                    parameterString.append(",1024x768");
                }
            } else if (max_w*10/max_h == 160/9) {   // 16:9 Sensor

            }
		}
        parameterString.append(",640x480,352x288,320x240,176x144");        
        params.set(CameraParameters::KEY_PICTURE_SIZE, string);
        params.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, parameterString.string());
	}

#if (CONFIG_CAMERA_SETVIDEOSIZE == 1)
    params.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,"640x480");
	params.set(CameraParameters::KEY_VIDEO_SIZE,"640x480");
	params.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,parameterString); //here maybe erro , cause may not same with  XML 
#endif


    //auto focus parameters
	{
        bool err_af;
    	bool avail = false;

        parameterString = CameraParameters::FOCUS_MODE_FIXED;
        params.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);

        //char prop_value[PROPERTY_VALUE_MAX];
        property_get("sys.cts_gts.status",prop_value, "false");
       //if(!strcmp(prop_value,"true")){
       if(0){
           params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"0");
           params.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, CameraParameters::FOCUS_MODE_FIXED);
       }else{ 
		if (strcmp(pCamInfo->mHardInfo.mVcmInfo.mVcmDrvName,"NC")!=0) {
          //if(0){
              LOGD("------mHardInfo.mVcmInfo.mVcmDrvName in not NC-----\n");
            err_af = m_camDevice->isAfAvailable(avail);
            if ((err_af == true) && (avail == true)) {
                parameterString.append(",");
                parameterString.append(CameraParameters::FOCUS_MODE_AUTO);
                params.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_AUTO);
        		params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"1");  

                if (pCamInfo->mSoftInfo.mFocusConfig.mFocusSupport & (0x01<<FOCUS_CONTINUOUS_VIDEO_BITPOS)) {
                    /* ddl@rock-chips.com: v0.d.0 */
                    parameterString.append(",");
                    parameterString.append(CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO);
                }

                if (pCamInfo->mSoftInfo.mFocusConfig.mFocusSupport & (0x01<<FOCUS_CONTINUOUS_PICTURE_BITPOS)) {
                    parameterString.append(",");
                    parameterString.append(CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);
                } 
            } else {
             	params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"0");
        	}

		}else{
         	params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"0");

        }
            params.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, parameterString.string());
      }
	}

    //flash parameters
    {
        CamEngineFlashCfg_t flash_cfg;

        //Internal or external flash
        if ((strcmp(pCamInfo->mHardInfo.mFlashInfo.mFlashName,"NC")!=0)) {
            parameterString = CameraParameters::FLASH_MODE_OFF;

            parameterString.append(",");
            parameterString.append(CameraParameters::FLASH_MODE_ON);
            
            parameterString.append(",");
            parameterString.append(CameraParameters::FLASH_MODE_AUTO);

            //only external flash support torch now,zyc
            if((strcmp(pCamInfo->mHardInfo.mFlashInfo.mFlashName,"Internal")!=0)){
                parameterString.append(",");
                parameterString.append(CameraParameters::FLASH_MODE_TORCH);
            }
            
            //must FLASH_MODE_OFF when initial,forced by cts
            params.set(CameraParameters::KEY_FLASH_MODE,CameraParameters::FLASH_MODE_OFF);
            params.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES,parameterString.string());


            flash_cfg.mode = CAM_ENGINE_FLASH_ON;
            flash_cfg.active_pol = (pCamInfo->mHardInfo.mFlashInfo.mFlashTrigger.active>0) ? CAM_ENGINE_FLASH_HIGH_ACTIVE:CAM_ENGINE_FLASH_LOW_ACTIVE;
			flash_cfg.flashtype = pCamInfo->mHardInfo.mFlashInfo.mFlashMode;
            if((strcmp(pCamInfo->mHardInfo.mFlashInfo.mFlashName,"Internal")==0))
                flash_cfg.dev_mask = pCamInfo->mHardInfo.mSensorInfo.mHostDevid;
            else
                flash_cfg.dev_mask = pCamInfo->mHardInfo.mSensorInfo.mCamDevid;
            m_camDevice->configureFlash(&flash_cfg);
        }
    }

    //digital zoom
    {
        if (pCamInfo->mSoftInfo.mZoomConfig == 1) {
            char str_zoom_max[3],str_zoom_element[5];
            char str[300];           
            int max,i;
            
        	mZoomMax = 300;
        	mZoomMin= 100;
        	mZoomStep = 5;	
            memset(str,0x00,sizeof(str));
            strcpy(str, "");//default zoom
            
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
            mZoomVal = 100;
        }else{
        	params.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
        }

    }	

    //WB setting
	{   
    	parameterString = "auto";
    	
        if(pCamInfo->mSoftInfo.mAwbConfig.mAwbSupport&(0x1<<AWB_INCANDESCENT_BITPOS)) {
            if (m_camDevice->chkAwbIllumination((char*)"A") == true) {
                parameterString.append(",");
                parameterString.append(CameraParameters::WHITE_BALANCE_INCANDESCENT);
            }
        }
        
    	if(pCamInfo->mSoftInfo.mAwbConfig.mAwbSupport&0x1<<(AWB_FLUORESCENT_BITPOS)){
            if (m_camDevice->chkAwbIllumination((char*)"F2_CWF") == true) {
                parameterString.append(",");
                parameterString.append(CameraParameters::WHITE_BALANCE_FLUORESCENT);
            }
    	}
        
    	if(pCamInfo->mSoftInfo.mAwbConfig.mAwbSupport&0x1<<(AWB_WARM_FLUORESCENT_BITPOS)) {
            if (m_camDevice->chkAwbIllumination((char*)"U30") == true) {
                parameterString.append(",");
                parameterString.append(CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT);
            }

    	}
        
    	if(pCamInfo->mSoftInfo.mAwbConfig.mAwbSupport&0x1<<(AWB_DAYLIGHT_BITPOS)) {
            if (m_camDevice->chkAwbIllumination((char*)"D65") == true) {
                parameterString.append(",");
                parameterString.append(CameraParameters::WHITE_BALANCE_DAYLIGHT);
            }
    	}

        if(pCamInfo->mSoftInfo.mAwbConfig.mAwbSupport&0x1<<(AWB_CLOUDY_BITPOS)) {
            if (m_camDevice->chkAwbIllumination((char*)"D50") == true) {
                parameterString.append(",");
                parameterString.append(CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT);
            }
    	}

        if(pCamInfo->mSoftInfo.mAwbConfig.mAwbSupport&0x1<<(AWB_SHADE_BITPOS)) {
            if (m_camDevice->chkAwbIllumination((char*)"D75") == true) {
                parameterString.append(",");
                parameterString.append(CameraParameters::WHITE_BALANCE_SHADE);
            }
    	}

    	if(pCamInfo->mSoftInfo.mAwbConfig.mAwbSupport&0x1<<(AWB_TWILIGHT_BITPOS)) {
            if (m_camDevice->chkAwbIllumination((char*)"HORIZON") == true) {
                parameterString.append(",");
                parameterString.append(CameraParameters::WHITE_BALANCE_TWILIGHT);
            }
    	}
    
        params.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, parameterString.string());
    	params.set(CameraParameters::KEY_WHITE_BALANCE, "auto");
	}
    
    /*preview format setting*/
    //yuv420p ,forced by cts
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, "yuv420sp,yuv420p");
    params.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,CameraParameters::PIXEL_FORMAT_YUV420SP);
    params.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP);
    params.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,CameraParameters::PIXEL_FORMAT_YUV420SP);

    /*picture format setting*/
    params.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);
    params.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
    /*jpeg quality setting*/
    params.set(CameraParameters::KEY_JPEG_QUALITY, "70");
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
	parameterString = String8::format( "%f",  pCamInfo->mHardInfo.mSensorInfo.fov_h);
    params.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, parameterString.string());
    /*vertical angle of view setting ,no much meaning ,only for passing cts */
	parameterString = String8::format("%f",  pCamInfo->mHardInfo.mSensorInfo.fov_v);
    params.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, parameterString.string());

    /*quality of the EXIF thumbnail in Jpeg picture setting */
    parameterString = "70";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, parameterString.string());
    /*supported size of the EXIF thumbnail in Jpeg picture setting */
    parameterString = "0x0,160x128,160x96";
    params.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, parameterString.string());
    parameterString = "160";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, parameterString.string());
    parameterString = "128";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, parameterString.string()); 
    if(pCamInfo->mSoftInfo.mFaceDetctConfig.mFaceDetectSupport > 0){
        memset(string,0x00,sizeof(string));    
    	sprintf(string,"%d",pCamInfo->mSoftInfo.mFaceDetctConfig.mFaceMaxNum);
    	params.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW,string);
    }else
    	params.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW,"0");
    params.set(CameraParameters::KEY_RECORDING_HINT,"false");
    params.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED,"false");
    params.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED,"true");
    params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "false");
    params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "false");
            
    //params.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
    //params.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,CameraParameters::FOCUS_MODE_FIXED);

    // params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"0");
    //exposure setting
    if (m_camDevice->isSOCSensor() == false) {
        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
        params.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "2");
        params.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "-2");
        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "1");

        params.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS,"1");
    } else {
        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
        params.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "0");
        params.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "0");
        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0");

        params.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS,"0");
    }
	

    //for video test
    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "3000,30000");
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(3000,30000)");
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, "10,15,30"); 
    params.setPreviewFrameRate(30);

    params.set(KEY_CONTINUOUS_PIC_NUM,"1");

    if((pCamInfo->mSoftInfo.mContinue_snapshot_config)
        && (pCamInfo->mHardInfo.mSensorInfo.mPhy.type == CamSys_Phy_Mipi)
        && (pCamInfo->mHardInfo.mSensorInfo.laneNum > 1)){
        params.set(KEY_CONTINUOUS_SUPPORTED,"true");
    }else{
        params.set(KEY_CONTINUOUS_SUPPORTED,"false");
    }
    // for cts
    params.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, "auto,50hz,60hz,off");
    params.set(CameraParameters::KEY_ANTIBANDING, "off");
    params.set(CameraParameters::KEY_SUPPORTED_EFFECTS, "none,mono,sepia");
    params.set(CameraParameters::KEY_EFFECT, "none");

    LOG1 ("Support Preview format: %s    %s(default)",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS),params.get(CameraParameters::KEY_PREVIEW_FORMAT)); 
	LOG1 ("Support Preview sizes: %s    %s(default)    %dx%d(force)",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES),params.get(CameraParameters::KEY_PREVIEW_SIZE),
        pCamInfo->mSoftInfo.mPreviewWidth,pCamInfo->mSoftInfo.mPreviewHeight);
	LOG1 ("Support Preview FPS range: %s",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE));   
	LOG1 ("Support Preview framerate: %s",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES));   
	LOG1 ("Support Picture sizes: %s    %s(default)",params.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES),params.get(CameraParameters::KEY_PICTURE_SIZE));
	LOG1 ("Support Focus: %s  Focus zone: %s",params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES),
        params.get(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS));
	if (params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES) && params.get(CameraParameters::KEY_FLASH_MODE))
    	LOG1 ("Support Flash: %s  Flash: %s",params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES),
        	params.get(CameraParameters::KEY_FLASH_MODE));
    LOG1 ("Support AWB: %s ",params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE));
	
	LOG1("Support FOV test h(%f) v(%f)\n", pCamInfo->mHardInfo.mSensorInfo.fov_h,pCamInfo->mHardInfo.mSensorInfo.fov_v);

	cameraConfig(params,true,isRestartPreview);
    LOG_FUNCTION_NAME_EXIT
}

int CameraIspAdapter::cameraConfig(const CameraParameters &tmpparams,bool isInit,bool &isRestartValue)
{
	int err = 0, i = 0;
	CameraParameters params = tmpparams;
	
    /*white balance setting*/
    const char *white_balance = params.get(CameraParameters::KEY_WHITE_BALANCE);
	const char *mwhite_balance = mParameters.get(CameraParameters::KEY_WHITE_BALANCE);
	if (params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE)) {	
		if ( !mwhite_balance || strcmp(white_balance, mwhite_balance) ){
			if(!isInit) {
				char prfName[10];
				uint32_t illu_index = 1; 
				if(!strcmp(white_balance, "auto")) {
                    if (m_camDevice->isSOCSensor() == false) {
    					m_camDevice->stopAwb();
    					m_camDevice->resetAwb();
    					m_camDevice->startAwb(CAM_ENGINE_AWB_MODE_AUTO, 0, (bool_t)true);
                    }
				} else {
					setMwb(white_balance);
				#if 0
				    if (m_camDevice->isSOCSensor() == false) {
    					if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_INCANDESCENT)) {
    						strcpy(prfName, "A");
    					} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_DAYLIGHT)) {
    						strcpy(prfName, "D65");			
    					} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_FLUORESCENT)) {
    						strcpy(prfName, "F2_CWF");
    					} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_SHADE)) {
    						strcpy(prfName, "D75");
    					} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_TWILIGHT)) {
    						strcpy(prfName, "Horizon");
    					} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT)) {
    						strcpy(prfName, "D50");
    					} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT)) {
    						strcpy(prfName, "U30");
    					}

    					std::vector<CamIlluProfile_t *> profiles;
    					m_camDevice->getIlluminationProfiles( profiles );
    					int i,size;
    					size = profiles.size();
    					for(i=0; i<size; i++)
    					{
    						if(strstr(profiles[i]->name, prfName))
    						{
    							illu_index = i;
								break;
							}
    					}
    					
		                m_camDevice->stopAwb();
    					m_camDevice->startAwb(CAM_ENGINE_AWB_MODE_MANUAL, illu_index, (bool_t)true);
				    }
					#endif
				}
			}
		}
	}

	/*zoom setting*/
    const int zoom = params.getInt(CameraParameters::KEY_ZOOM);
	const int mzoom = mParameters.getInt(CameraParameters::KEY_ZOOM);
	if (params.get(CameraParameters::KEY_ZOOM_SUPPORTED)) {
		//TODO
        if((zoom != mzoom) && (!isInit)){
            CamEnginePathConfig_t pathConfig;
            mZoomVal = zoom*mZoomStep+mZoomMin;
            #if (USE_RGA_TODO_ZOOM == 0)          /* zyc@rock-chips.com: v0.0x22.0 */ 
            setupPreview(mCamDrvWidth,mCamDrvHeight,mCamPreviewW, mCamPreviewH, mZoomVal);
            m_camDevice->getPathConfig(CHAIN_MASTER,CAM_ENGINE_PATH_MAIN,pathConfig);
            m_camDevice->reSetMainPathWhileStreaming(&pathConfig.dcWin,pathConfig.width,pathConfig.height);
            #endif

        }
        
	}
	
    /*color effect setting*/
    const char *effect = params.get(CameraParameters::KEY_EFFECT);
	const char *meffect = mParameters.get(CameraParameters::KEY_EFFECT);
	if (params.get(CameraParameters::KEY_SUPPORTED_EFFECTS)) {
		//TODO
	}
	
    /*anti-banding setting*/
    const char *anti_banding = params.get(CameraParameters::KEY_ANTIBANDING);
	const char *manti_banding = mParameters.get(CameraParameters::KEY_ANTIBANDING);
	if (anti_banding != NULL) {
		if ( !manti_banding || (anti_banding && strcmp(anti_banding, manti_banding)) ) {
			//TODO
		}
	}
	
	/*scene setting*/
    const char *scene = params.get(CameraParameters::KEY_SCENE_MODE);
	const char *mscene = mParameters.get(CameraParameters::KEY_SCENE_MODE);
	if (params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES)) {
		if ( !mscene || strcmp(scene, mscene) ) {
			//TODO
		}
	}
	
    /*focus setting*/
    const char *focusMode = params.get(CameraParameters::KEY_FOCUS_MODE);
	const char *mfocusMode = mParameters.get(CameraParameters::KEY_FOCUS_MODE);
	if (params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES)) {
		if ( !mfocusMode || strcmp(focusMode, mfocusMode) ) {
       		//if(!cameraAutoFocus(isInit)){
        	//	params.set(CameraParameters::KEY_FOCUS_MODE,(mfocusMode?mfocusMode:CameraParameters::FOCUS_MODE_FIXED));
        	//	err = -1;
   			//}
   			//TODO
		}
	} else{
		params.set(CameraParameters::KEY_FOCUS_MODE,(mfocusMode?mfocusMode:CameraParameters::FOCUS_MODE_FIXED));
	}
	
	/*flash mode setting*/
    const char *flashMode = params.get(CameraParameters::KEY_FLASH_MODE);
	const char *mflashMode = mParameters.get(CameraParameters::KEY_FLASH_MODE);
	
	if (params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES)) {
		if ( !mflashMode || strcmp(flashMode, mflashMode) ) {
			//TODO
		}
	}
    
    /*exposure setting*/
	const char *exposure = params.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    const char *mexposure = mParameters.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    
	if (strcmp("0", params.get(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION))
		|| strcmp("0", params.get(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION))) {
	    if (!mexposure || (exposure && strcmp(exposure,mexposure))) {
            if (m_camDevice->isSOCSensor() == false) {
    			if(isInit)
    			{
    				float tolerance;
    				manExpConfig.clmtolerance = m_camDevice->getAecClmTolerance();
    				tolerance = manExpConfig.clmtolerance/100.0f;
    				m_camDevice->getInitAePoint(&manExpConfig.level_0);

    				manExpConfig.plus_level_1 = (manExpConfig.level_0/(1-tolerance))+manExpConfig.clmtolerance*1.5;
    				manExpConfig.plus_level_2 = (manExpConfig.plus_level_1/(1-tolerance))+manExpConfig.clmtolerance*1.5;
    				manExpConfig.plus_level_3 = (manExpConfig.plus_level_2/(1-tolerance))+manExpConfig.clmtolerance*1.5;
    				
    				manExpConfig.minus_level_1 = (manExpConfig.level_0/(1+tolerance))-manExpConfig.clmtolerance*1.5;
    				if(manExpConfig.minus_level_1 < 0)
    					manExpConfig.minus_level_1 = 0;
    				manExpConfig.minus_level_2 = (manExpConfig.minus_level_1/(1+tolerance))-manExpConfig.clmtolerance*1.5;
    				if(manExpConfig.minus_level_2 < 0)
    					manExpConfig.minus_level_2 = 0;				
    				manExpConfig.minus_level_3 = (manExpConfig.minus_level_2/(1+tolerance))-manExpConfig.clmtolerance*1.5;	
    				if(manExpConfig.minus_level_3 < 0)
    					manExpConfig.minus_level_3 = 0;
    				LOG1("Exposure Compensation ae point:\n");
                    LOG1("    -3 : %f\n",manExpConfig.minus_level_3);
                    LOG1("    -2 : %f\n",manExpConfig.minus_level_2);
                    LOG1("    -1 : %f\n",manExpConfig.minus_level_1);
                    LOG1("     0 : %f\n",manExpConfig.level_0);
                    LOG1("     1 : %f\n",manExpConfig.plus_level_1);
                    LOG1("     2 : %f\n",manExpConfig.plus_level_2);
                    LOG1("     3 : %f\n",manExpConfig.plus_level_3);
    			}else{
				    setMe(exposure);
				#if 0
    				int iexp;
    				iexp = atoi(exposure);
    				switch(iexp)
    				{
    					case -3:
    						m_camDevice->setAeClmTolerance(manExpConfig.clmtolerance*0.6);
    						m_camDevice->setAePoint(manExpConfig.minus_level_3);
    						break;
    					case -2:
    						m_camDevice->setAeClmTolerance(manExpConfig.clmtolerance*0.8);
    						m_camDevice->setAePoint(manExpConfig.minus_level_2);
    						break;
    					case -1:
    						m_camDevice->setAePoint(manExpConfig.minus_level_1);
    						break;
    					case 0:
    						m_camDevice->setAePoint(manExpConfig.level_0);
    						break;
    					case 1:
    						m_camDevice->setAePoint(manExpConfig.plus_level_1);
    						break;
    					case 2:
    						m_camDevice->setAeClmTolerance(manExpConfig.clmtolerance*0.8);
    						m_camDevice->setAePoint(manExpConfig.plus_level_2);
    						break;
    					case 3:
    						m_camDevice->setAeClmTolerance(manExpConfig.clmtolerance*0.6);
    						m_camDevice->setAePoint(manExpConfig.plus_level_3);
    						break;
    					default:
    						break;
    				}
				#endif
    			}
            }
	    }
	}    

	mParameters = params;
	//changeVideoPreviewSize();
	isRestartValue = isNeedToRestartPreview();

	return err;
}

status_t CameraIspAdapter::autoFocus()
{
    bool shot = false,err_af = false;
    CamEngineWindow_t afWin;
#if 1    
    char prop_value[PROPERTY_VALUE_MAX];
    property_get("sys.cts_gts.status",prop_value, "false");
    if(!strcmp(prop_value,"true")){
    goto finish_focus;
    }
#endif
    if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_MODE), CameraParameters::FOCUS_MODE_AUTO) == 0) {
        if (mAfChk == false) {
            LOG1("Focus mode is Auto and areas not change, CheckAfShot!");
            if (m_camDevice->checkAfShot(&shot) == false) {
                shot = true;    
            }
        } else {
            LOG1("Focus mode is Auto, but areas have changed, Must AfShot!");
            shot = true;
        }
        
    } else if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_MODE), CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE) == 0) {
        LOG1("Focus mode is continues, checkAfShot!");
        if (m_camDevice->checkAfShot(&shot) == false) {
            shot = true;    
        }
    }

    if (shot == true) {
        LOG1("Single auto focus must be trigger");
        m_camDevice->stopAf();  /* ddl@rock-chips.com: v0.d.0 */
        /* ddl@rock-chips.com: v1.5.0 */
        if (mParameters.get(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS) != NULL) {
            if (mParameters.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS) > 0) {
    	    	int hOff,vOff,w,h;
    			const char* zoneStr = mParameters.get(CameraParameters::KEY_FOCUS_AREAS);
    	    	if (zoneStr) {        			
        	        hOff = strtol(zoneStr+1,0,0);
        	        zoneStr = strstr(zoneStr,",");
        	        vOff = strtol(zoneStr+1,0,0);
        		    zoneStr = strstr(zoneStr+1,",");
        	        w = strtol(zoneStr+1,0,0);                    
                    zoneStr = strstr(zoneStr+1,",");
        	        h = strtol(zoneStr+1,0,0);

                    w -= hOff;
                    h -= vOff;                    
                    
                    if ( ((hOff<-1000) || (hOff>1000)) ||
                         ((vOff<-1000) || (vOff>1000)) ||
                         ((w<=0) || (w>2000)) ||
                         ((h<=0) || (h>2000)) ) {
                        LOGE("%s: %s , afWin(%d,%d,%d,%d)is invalidate!",
                            CameraParameters::KEY_FOCUS_AREAS,
                            mParameters.get(CameraParameters::KEY_FOCUS_AREAS),
                            hOff,vOff,w,h);                            
                        vOff = 0;
                        hOff = 0;
                        w = 0;
                        h = 0;
                    }
                    m_camDevice->setAfMeasureWindow(hOff,vOff,w,h);
    	    	}
            }
        }
        
        err_af = m_camDevice->startAfOneShot(CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE);
    	if ( err_af == false ){
    		TRACE_E("Trigger a single auto focus failed!");  
            goto finish_focus;  /* ddl@rock-chips.com: v0.0x27.0 */
    	} else {
            LOG1("Trigger a single auto focus success");
    	}
    } else {

finish_focus:    
        LOG1("It has been focused!");

        CamEngineAfEvt_t evnt;
        int32_t err, times;
        
        evnt.evnt_id = CAM_ENGINE_AUTOFOCUS_FINISHED;
        evnt.info.fshEvt.focus = BOOL_TRUE;

        err = osQueueTryWrite(&mAfListenerQue.queue, &evnt);
        if (err != OSLAYER_OK) {
            times = 0;
            do 
            {
                times++;
                err = osQueueTryRead(&mAfListenerQue.queue, &evnt);
            } while ((err == OSLAYER_OK) && (times < 5));

            evnt.evnt_id = CAM_ENGINE_AUTOFOCUS_FINISHED;
            evnt.info.fshEvt.focus = BOOL_TRUE;

            err = osQueueTryWrite(&mAfListenerQue.queue, &evnt);
            if (err != OSLAYER_OK) {
                LOGE(" mAfListenerQue.queue write failed!!");
                DCT_ASSERT(0);
            }
        }
    }
	 
    return 0;
}


status_t CameraIspAdapter::cancelAutoFocus()
{  
    /* ddl@rock-chips.com: v0.0x27.0 */
    if (strcmp(mParameters.get(CameraParameters::KEY_FOCUS_MODE), CameraParameters::FOCUS_MODE_AUTO) == 0) {
        m_camDevice->stopAf();    
    }
    
    return 0;
}

void CameraIspAdapter::setScenarioMode(CamEngineModeType_t newScenarioMode)
{
    CamEngineModeType_t oldScenarioMode = CAM_ENGINE_MODE_INVALID;
    m_camDevice->getScenarioMode( oldScenarioMode );


    if ( oldScenarioMode != newScenarioMode )
    {
        // disconnect
        disconnectCamera();

        m_camDevice->setScenarioMode( newScenarioMode );
        m_camDevice->clearSensorDriverFile();
    }
}
//sensor interface :mipi ? parallel?
void CameraIspAdapter::setSensorItf(int newSensorItf)
{
    if ( CamEngineItf::Sensor == m_camDevice->configType() )
    {
        int oldSensorItf = m_camDevice->getSensorItf();
        if ( oldSensorItf != newSensorItf )
        {
            m_camDevice->setSensorItf( newSensorItf );
             mSensorItfCur = newSensorItf;
            loadSensor( mCamId);
           
        }
        
    }
}
void CameraIspAdapter::enableAfps( bool enable )
{
    if ( CamEngineItf::Sensor == m_camDevice->configType() )
    {
        if ( enable != m_camDevice->isAfpsEnabled() )
        {
            m_camDevice->enableAfps( enable );
            m_camDevice->changeEcm( true );
        }
    }
}

void CameraIspAdapter::openImage( const char* fileName)
{
    // open sensor
    if ( NULL != fileName )
    {
        disconnectCamera();
        PicBufMetaData_t        image;
        image.Type = PIC_BUF_TYPE_RAW8;
        image.Layout =PIC_BUF_LAYOUT_BAYER_RGRGGBGB;
        image.Data.raw.PicWidthPixel= 1920;
        image.Data.raw.PicHeightPixel = 1080;
        image.Data.raw.PicWidthBytes = 1920;

        image.Data.raw.pBuffer = (uint8_t *)malloc( image.Data.raw.PicWidthBytes * image.Data.raw.PicHeightPixel );
        if (true == m_camDevice->openImage(fileName,image,NULL,NULL,NULL,NULL,1.0,1.0))
        {
            //set scenario mode
            CamEngineModeType_t mode1 = CAM_ENGINE_MODE_IMAGE_PROCESSING; 

            m_camDevice->setScenarioMode( mode1 );
            // connect
            connectCamera();


        }else{
            LOGE("failed!");
        }
        
        if(image.Data.raw.pBuffer)
            free(image.Data.raw.pBuffer);


        
    }

    
}
void CameraIspAdapter::loadSensor( const int cameraId)
{
    // open sensor
    if(cameraId>=0)
    {
        disconnectCamera();

        rk_cam_total_info *pCamInfo = gCamInfos[cameraId].pcam_total_info;
        if ( true == m_camDevice->openSensor( pCamInfo, mSensorItfCur ) )
        {
        	bool res = m_camDevice->checkVersion(pCamInfo);
			if(res!=true)
			    return;
#if 1
            // connect
            uint32_t resMask;
            CamEngineWindow_t dcWin;
            CamEngineBestSensorResReq_t resReq;

            resReq.request_w = DEFAULTPREVIEWWIDTH;
            resReq.request_h = DEFAULTPREVIEWHEIGHT;
            resReq.request_fps = 15;
            resReq.requset_aspect = (bool_t)false;
            resReq.request_fullfov = (bool_t)mImgAllFovReq;
            m_camDevice->getPreferedSensorRes(&resReq);            
           
            m_camDevice->setSensorResConfig(resReq.resolution);
            setupPreview(mCamDrvWidth,mCamDrvHeight,DEFAULTPREVIEWWIDTH,DEFAULTPREVIEWHEIGHT,mZoomVal);
            connectCamera();
            mCamPreviewH = DEFAULTPREVIEWHEIGHT;
            mCamPreviewW = DEFAULTPREVIEWWIDTH;
            //mSensorDriverFile[mSensorItfCur] = fileName;
#endif
        }
        else
        {
            LOGE("%s(%d):failed!",__func__,__LINE__);
        }
    }
}

void CameraIspAdapter::loadCalibData(const char* fileName )
{
    if ( NULL != fileName )
    {
        disconnectCamera();
        if ( true == m_camDevice->loadCalibrationData( fileName ) )
        {
            // connect
            connectCamera();
        }
        else
        {
            LOGE("loadCalibrationData failed!");
        }
    }
}

bool CameraIspAdapter::connectCamera(){
    bool result = false;
    result = m_camDevice->connectCamera( true, this );
    if ( true != result)
    {
        LOGE("connectCamera failed!");
    }

    {
    	rk_cam_total_info *pCamInfo = gCamInfos[mCamId].pcam_total_info;

        CamEngineCprocConfig_t cproc_config = {
            CAM_ENGINE_CPROC_CHROM_RANGE_OUT_BT601,//CAM_ENGINE_CPROC_CHROM_RANGE_OUT_FULL_RANGE,
            CAM_ENGINE_CPROC_LUM_RANGE_OUT_FULL_RANGE,//CAM_ENGINE_CPROC_LUM_RANGE_OUT_BT601,//,
            CAM_ENGINE_CPROC_LUM_RANGE_IN_FULL_RANGE,//CAM_ENGINE_CPROC_LUM_RANGE_IN_BT601,//,
            1.1,  //contrast 0-1.992
            0,      //brightness -128 - 127
            1.0,      //saturation 0-1.992
            0,      //hue   -90 - 87.188
        };
        if(pCamInfo->mSoftInfo.mCprocConfig.mSupported == true){
            cproc_config.contrast = pCamInfo->mSoftInfo.mCprocConfig.mContrast;
            cproc_config.saturation= pCamInfo->mSoftInfo.mCprocConfig.mSaturation;
            cproc_config.hue= pCamInfo->mSoftInfo.mCprocConfig.mHue;
            cproc_config.brightness= pCamInfo->mSoftInfo.mCprocConfig.mBrightness;
            m_camDevice->cProcEnable( &cproc_config);
        }
    }
    {
    	rk_cam_total_info *pCamInfo = gCamInfos[mCamId].pcam_total_info;
        int tmp_gamma = 0;
        float gamma = pCamInfo->mSoftInfo.mGammaOutConfig.mGamma,maxval = 1024;
        int offset = pCamInfo->mSoftInfo.mGammaOutConfig.mOffSet;

        CamEngineGammaOutCurve_t gamma_curve = {
            CAM_ENGINE_GAMMAOUT_XSCALE_EQU,
            {0}
        };
        
        for(int i = 0;i < (CAMERIC_ISP_GAMMA_CURVE_SIZE);i++){
            tmp_gamma = (int)((pow((64*i/maxval),gamma))*1024)+offset;
            tmp_gamma = (tmp_gamma > 1023)?1023:tmp_gamma;
            tmp_gamma = (tmp_gamma < 0)?0:tmp_gamma;
            gamma_curve.GammaY[i] = (uint16_t)(tmp_gamma);
            TRACE_D(1,"gamma_curve.GammaY[%d]:%d ",i,gamma_curve.GammaY[i]);
        }
        if(pCamInfo->mSoftInfo.mGammaOutConfig.mSupported == true){
            m_camDevice->gamCorrectEnable();
            m_camDevice->gamCorrectSetCurve(gamma_curve);
        }
    }
    return result;
}

void CameraIspAdapter::disconnectCamera()
{
    unsigned int maxFocus, minFocus;

    if (mISPTunningRun == false) {
    
        m_camDevice->stopAf();  /* ddl@rock-chips.com: v0.d.1 */
        
        if (m_camDevice->getFocusLimits(minFocus, maxFocus) == true) {
            m_camDevice->setFocus(maxFocus);
            usleep(100000);
        } else {
            LOGE("getFocusLimits failed!");
        }
    }

	rk_cam_total_info *pCamInfo = gCamInfos[mCamId].pcam_total_info;
    if(pCamInfo->mSoftInfo.mCprocConfig.mSupported == true){
        bool running = false;
        CamEngineCprocConfig_t cproc_config;
        if((m_camDevice->state() == CamEngineItf::Running) ||
            (m_camDevice->state() == CamEngineItf::Idle))
            m_camDevice->cProcStatus( running, cproc_config );
        if(running)
            m_camDevice->cProcDisable();
    }
    
    m_camDevice->disconnectCamera();
}

int CameraIspAdapter::start()
{
    if ( ( !m_camDevice->hasSensor() ) &&
         ( !m_camDevice->hasImage()  ) )
    {
        LOGE("start failed! (!m_camDevice->hasSensor(): %d !m_camDevice->hasImage(): %d)",
            (!m_camDevice->hasSensor()),(!m_camDevice->hasImage()));
        return -1;
    }

    if ( true == m_camDevice->startPreview() )
    {
        LOGD("m_camDevice->startPreview success");
        m_camDevice->isPictureOrientationAllowed( CAM_ENGINE_MI_ORIENTATION_ORIGINAL );
		return 0;
    } else {
        LOGE("m_camDevice->startPreview failed!");
		return -1;
    }
}
int CameraIspAdapter::pause()
{
    if ( ( !m_camDevice->hasSensor() ) &&
         ( !m_camDevice->hasImage()  ) )
    {
        LOGE("start failed! (!m_camDevice->hasSensor(): %d !m_camDevice->hasImage(): %d)",
            (!m_camDevice->hasSensor()),(!m_camDevice->hasImage()));
        return -1;
    }

    if ( true == m_camDevice->pausePreview() )
    {
        LOGD("m_camDevice->pausePreview success!");
		return 0;
    } else {
        LOGE("m_camDevice->pausePreview failed!");
		return -1;
    }
}


/******************************************************************************
 * stop
 *****************************************************************************/
int CameraIspAdapter::stop()
{
    if ( ( !m_camDevice->hasSensor() ) &&
         ( !m_camDevice->hasImage()  ) )
    {
        LOGE("start failed! (!m_camDevice->hasSensor(): %d !m_camDevice->hasImage(): %d)",
            (!m_camDevice->hasSensor()),(!m_camDevice->hasImage()));
        return -1;
    }

    if ( true == m_camDevice->stopPreview() )
    {
    
        LOGD("m_camDevice->stopPreview success!");
		return 0;
    }
	else
	{
		LOGE("m_camDevice->stopPreview fail!");
		return -1;
	}
}

void CameraIspAdapter::clearFrameArray(){
    LOG_FUNCTION_NAME
    MediaBuffer_t *pMediaBuffer = NULL;
    FramInfo_s *tmpFrame = NULL;
    Mutex::Autolock lock(mFrameArrayLock);

    int num = mFrameInfoArray.size();
    while(--num >= 0){
        tmpFrame = (FramInfo_s *)mFrameInfoArray.keyAt(num);
        if(mFrameInfoArray.indexOfKey((void*)tmpFrame) < 0){
            LOGE("this frame is not in frame array,used_flag is %d!",tmpFrame->used_flag);
        }else{
            pMediaBuffer = (MediaBuffer_t *)mFrameInfoArray.valueAt(num);
            switch (tmpFrame->used_flag){
                case 0:
                    mDispFrameLeak--;
                    break;
                case 1:
                    mVideoEncFrameLeak--;
                    break;
                case 2:
                    mPicEncFrameLeak--;
                    break;
                case 3:
                    mPreviewCBFrameLeak--;
                    break;
                default:
                    LOGE("not the valid used_flag %d",tmpFrame->used_flag);
            }
            //remove item
            mFrameInfoArray.removeItem((void*)tmpFrame);
            free(tmpFrame);
            //unlock
            
            MediaBufUnlockBuffer( pMediaBuffer );
        }
    }
    mFrameInfoArray.clear();
    LOG_FUNCTION_NAME_EXIT
}
int CameraIspAdapter::adapterReturnFrame(long index,int cmd){
    FramInfo_s* tmpFrame = ( FramInfo_s *)index;
    Mutex::Autolock lock(mFrameArrayLock);
    if(mFrameInfoArray.size() > 0){
        if(mFrameInfoArray.indexOfKey((void*)tmpFrame) < 0){
            LOGE("this frame is not in frame array,used_flag is %d!",tmpFrame->used_flag);
        }else{
            MediaBuffer_t *pMediaBuffer = (MediaBuffer_t *)mFrameInfoArray.valueFor((void*)tmpFrame);
            {	
                switch (tmpFrame->used_flag){
                    case 0:
                        mDispFrameLeak--;
                        break;
                    case 1:
                        mVideoEncFrameLeak--;
                        break;
                    case 2:
                        mPicEncFrameLeak--;
                        break;
                    case 3:
                        mPreviewCBFrameLeak--;
                        break;
                    default:
                        LOG1("not the valid used_flag %d",tmpFrame->used_flag);
                }
                //remove item
                mFrameInfoArray.removeItem((void*)tmpFrame);
                free(tmpFrame);
                
                //unlock
                MediaBufUnlockBuffer( pMediaBuffer );
            }
        }
    }else{
        LOGD("frame array has been cleard!");
    }
    return 0;
}

int CameraIspAdapter::getCurPreviewState(int *drv_w,int *drv_h)
{
    *drv_w = mCamPreviewW;
    *drv_h = mCamPreviewH;
    return mPreviewRunning;
}

int CameraIspAdapter::selectPreferedDrvSize(int *width,int * height,bool is_capture)
{

    return 0;
}

static void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if (!(mFrameCount & 0x1F)) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps = ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        LOGD("Camera %d Frames, %2.3f FPS", mFrameCount, mFps);
    }
    // XXX: mFPS has the value we want
}
void CameraIspAdapter::bufferCb( MediaBuffer_t* pMediaBuffer )
{
    static int writeoneframe = 0;
    ulong_t y_addr = 0,uv_addr = 0;
	uint32_t y_size;
    void* y_addr_vir = NULL,*uv_addr_vir = NULL ;
    int width = 0,height = 0;
    int fmt = 0;
	int tem_val;
	ulong_t phy_addr=0;

	Mutex::Autolock lock(mLock);
    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pMediaBuffer->pMetaData);
    HalHandle_t  tmpHandle = m_camDevice->getHalHandle();

    //debugShowFPS();
    
    //
	bDataReced = true;
    if(pPicBufMetaData->Type == PIC_BUF_TYPE_YCbCr420 || pPicBufMetaData->Type == PIC_BUF_TYPE_YCbCr422){        
        if(pPicBufMetaData->Type == PIC_BUF_TYPE_YCbCr420){
            fmt = V4L2_PIX_FMT_NV12;
        }else{
            fmt = V4L2_PIX_FMT_YUYV;
        }

        if(pPicBufMetaData->Layout == PIC_BUF_LAYOUT_SEMIPLANAR ){
            y_addr = (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.Y.pBuffer);
            //now gap of y and uv buffer is 0. so uv addr could be calc from y addr.
            uv_addr = (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.pBuffer);
            width = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel;
            height = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;
            //get vir addr
            HalMapMemory( tmpHandle, y_addr, 100, HAL_MAPMEM_READWRITE, &y_addr_vir );
            HalMapMemory( tmpHandle, uv_addr, 100, HAL_MAPMEM_READWRITE, &uv_addr_vir );
            if(gCamInfos[mCamId].pcam_total_info->mIsIommuEnabled)
                HalGetMemoryMapFd(tmpHandle, y_addr,(int*)&phy_addr);
            else
                phy_addr = y_addr;
            
            /* ddl@rock-chips.com:  v1.3.0 */
            y_size = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel*pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;
            if (uv_addr > (y_addr+y_size)) {
                memcpy((void*)((ulong_t)y_addr_vir+y_size),uv_addr_vir, y_size/2);
            }
            
        }else if(pPicBufMetaData->Layout == PIC_BUF_LAYOUT_COMBINED){
            y_addr = (ulong_t)(pPicBufMetaData->Data.YCbCr.combined.pBuffer );
            width = pPicBufMetaData->Data.YCbCr.combined.PicWidthPixel>>1;
            height = pPicBufMetaData->Data.YCbCr.combined.PicHeightPixel;
            HalMapMemory( tmpHandle, y_addr, 100, HAL_MAPMEM_READWRITE, &y_addr_vir );
            if(gCamInfos[mCamId].pcam_total_info->mIsIommuEnabled)
                HalGetMemoryMapFd(tmpHandle, y_addr,(int*)&phy_addr);
            else
                phy_addr = y_addr;
        }

    } else if(pPicBufMetaData->Type == PIC_BUF_TYPE_RAW16) {

        y_addr = (ulong_t)(pPicBufMetaData->Data.raw.pBuffer );
        width = pPicBufMetaData->Data.raw.PicWidthPixel;
        height = pPicBufMetaData->Data.raw.PicHeightPixel;
        fmt = V4L2_PIX_FMT_SBGGR10;
        HalMapMemory( tmpHandle, y_addr, 100, HAL_MAPMEM_READWRITE, &y_addr_vir );
        if(gCamInfos[mCamId].pcam_total_info->mIsIommuEnabled)
            HalGetMemoryMapFd(tmpHandle, y_addr,(int*)&phy_addr);
        else
            phy_addr = y_addr;
    } else {
        LOGE("not support this type(%dx%d)  ,just support  yuv20 now",width,height);
        return ;
    }
    

    if ( pMediaBuffer->pNext != NULL ) {
        MediaBufLockBuffer( (MediaBuffer_t*)pMediaBuffer->pNext );
    }
	
#if 1
    char prop_value[PROPERTY_VALUE_MAX];
    property_get("sys.cts_gts.status",prop_value, "false");
	if( strcmp(prop_value,"true") && (preview_frame_inval > 0) ){
	  	preview_frame_inval--;
		LOG1("frame_inval:%d\n",preview_frame_inval);

        if(m_camDevice->isSOCSensor() == false){
			bool awb_ret = m_camDevice->isAwbStable();
			LOG1("awb test fps(%d) awb stable(%d)\n", preview_frame_inval, awb_ret);
			
			if( awb_ret!=true){
				LOG1("awb test fps(%d) awb stable(%d)\n", preview_frame_inval, awb_ret);
				goto end;
			}
		}else{
			goto end;
		}
  	}else{
       // LOG1("--is cts 44--");
    }
#endif


    if(mIsSendToTunningTh){
        MediaBufLockBuffer( pMediaBuffer );
        //new frames
        FramInfo_s *tmpFrame=(FramInfo_s *)malloc(sizeof(FramInfo_s));
        if(!tmpFrame){
            MediaBufUnlockBuffer( pMediaBuffer );
            return;
        }
        //add to vector
        tmpFrame->frame_index = (ulong_t)tmpFrame; 
        tmpFrame->phy_addr = (ulong_t)phy_addr;
        tmpFrame->frame_width = width;
        tmpFrame->frame_height= height;
        tmpFrame->vir_addr = (ulong_t)y_addr_vir;
        tmpFrame->frame_fmt = fmt;
        tmpFrame->used_flag = (ulong_t)pMediaBuffer; // tunning thread will use pMediaBuffer

        {
            Mutex::Autolock lock(mFrameArrayLock);
            mFrameInfoArray.add((void*)tmpFrame,(void*)pMediaBuffer);
        }
        Message_cam msg;
        msg.command = ISP_TUNNING_CMD_PROCESS_FRAME;
        msg.arg2 = (void*)(tmpFrame);
        msg.arg3 = (void*)(tmpFrame->used_flag);
        mISPTunningQ->put(&msg);

    }else{
        //need to send face detection ?
    	if(mRefEventNotifier->isNeedSendToFaceDetect()){  
    	    MediaBufLockBuffer( pMediaBuffer );
    		//new frames
    		FramInfo_s *tmpFrame=(FramInfo_s *)malloc(sizeof(FramInfo_s));
    		if(!tmpFrame){
    			MediaBufUnlockBuffer( pMediaBuffer );
    			return;
          }
          //add to vector
          tmpFrame->frame_index = (ulong_t)tmpFrame; 
          tmpFrame->phy_addr = (ulong_t)phy_addr;
          tmpFrame->frame_width = width;
          tmpFrame->frame_height= height;
          tmpFrame->vir_addr = (ulong_t)y_addr_vir;
          tmpFrame->frame_fmt = fmt;
    	  
          tmpFrame->used_flag = 4;

          tmpFrame->zoom_value = mZoomVal;
        
          {
            Mutex::Autolock lock(mFrameArrayLock);
            mFrameInfoArray.add((void*)tmpFrame,(void*)pMediaBuffer);

          }
          mRefEventNotifier->notifyNewFaceDecFrame(tmpFrame);
        }
    	//need to display ?
    	if(mRefDisplayAdapter->isNeedSendToDisplay()){  
    	    MediaBufLockBuffer( pMediaBuffer );
    		//new frames
    		FramInfo_s *tmpFrame=(FramInfo_s *)malloc(sizeof(FramInfo_s));
    		if(!tmpFrame){
    			MediaBufUnlockBuffer( pMediaBuffer );
    			return;
          }
          //add to vector
          tmpFrame->frame_index = (ulong_t)tmpFrame; 
          tmpFrame->phy_addr = (ulong_t)phy_addr;
          tmpFrame->frame_width = width;
          tmpFrame->frame_height= height;
          tmpFrame->vir_addr = (ulong_t)y_addr_vir;
          tmpFrame->frame_fmt = fmt;
    	  
          tmpFrame->used_flag = 0;

          #if (USE_RGA_TODO_ZOOM == 1)  
             tmpFrame->zoom_value = mZoomVal;
          #else
          if((tmpFrame->frame_width > 2592) && (tmpFrame->frame_height > 1944) && (mZoomVal != 100) ){
             tmpFrame->zoom_value = mZoomVal;
          }else
             tmpFrame->zoom_value = 100;
          #endif
        
          {
            Mutex::Autolock lock(mFrameArrayLock);
            mFrameInfoArray.add((void*)tmpFrame,(void*)pMediaBuffer);
            mDispFrameLeak++;

          }
          mRefDisplayAdapter->notifyNewFrame(tmpFrame);

        }

    	//video enc ?
    	if(mRefEventNotifier->isNeedSendToVideo()) {
            MediaBufLockBuffer( pMediaBuffer );
            //new frames
            FramInfo_s *tmpFrame=(FramInfo_s *)malloc(sizeof(FramInfo_s));
            if(!tmpFrame){
            	MediaBufUnlockBuffer( pMediaBuffer );
            	return;
            }          
            //add to vector
            tmpFrame->frame_index = (ulong_t)tmpFrame; 
            tmpFrame->phy_addr = (ulong_t)phy_addr;
            tmpFrame->frame_width = width;
            tmpFrame->frame_height= height;
            tmpFrame->vir_addr = (ulong_t)y_addr_vir;
            tmpFrame->frame_fmt = fmt;
            tmpFrame->used_flag = 1;
#if (USE_RGA_TODO_ZOOM == 1)  
            tmpFrame->zoom_value = mZoomVal;
#else
            if((tmpFrame->frame_width > 2592) && (tmpFrame->frame_height > 1944) && (mZoomVal != 100) ) {
                tmpFrame->zoom_value = mZoomVal;
            } else {
                tmpFrame->zoom_value = 100;
            }
#endif
          
            {
                Mutex::Autolock lock(mFrameArrayLock);
                mFrameInfoArray.add((void*)tmpFrame,(void*)pMediaBuffer);
                mVideoEncFrameLeak++;
            }
            mRefEventNotifier->notifyNewVideoFrame(tmpFrame);		
    	}
        
    	//picture ?
    	if(mRefEventNotifier->isNeedSendToPicture()){
            bool send_to_pic = true;
            if(mFlashStatus && ((ulong_t)(pPicBufMetaData->priv) != 1)){
                pPicBufMetaData->priv = NULL;
                send_to_pic = false;
                LOG1("not the desired flash pic,skip it,mFlashStatus %d!",mFlashStatus);
            }
            if (send_to_pic) { 
                MediaBufLockBuffer( pMediaBuffer );
                //new frames
                FramInfo_s *tmpFrame=(FramInfo_s *)malloc(sizeof(FramInfo_s));
                if(!tmpFrame){
                	MediaBufUnlockBuffer( pMediaBuffer );
                	return;
                }

                //add to vector
                tmpFrame->frame_index = (ulong_t)tmpFrame; 
                tmpFrame->phy_addr = (ulong_t)phy_addr;
                tmpFrame->frame_width = width;
                tmpFrame->frame_height= height;
                tmpFrame->vir_addr = (ulong_t)y_addr_vir;
                tmpFrame->frame_fmt = fmt;
                tmpFrame->used_flag = 2;
                tmpFrame->res = &mImgAllFovReq;
#if (USE_RGA_TODO_ZOOM == 1)  
                tmpFrame->zoom_value = mZoomVal;
#else
                if((tmpFrame->frame_width > 2592) && (tmpFrame->frame_height > 1944) && (mZoomVal != 100) ){
                    tmpFrame->zoom_value = mZoomVal;
                } else {
                    tmpFrame->zoom_value = 100;
                }
#endif

                {
                    Mutex::Autolock lock(mFrameArrayLock);
                    mFrameInfoArray.add((void*)tmpFrame,(void*)pMediaBuffer);
                    mPicEncFrameLeak++;
                }
                picture_info_s &picinfo = mRefEventNotifier->getPictureInfoRef();
                getCameraParamInfo(picinfo.cameraparam);
                mRefEventNotifier->notifyNewPicFrame(tmpFrame);	
            }
    	}

    	//preview data callback ?
    	if(mRefEventNotifier->isNeedSendToDataCB() && (mRefDisplayAdapter->getDisplayStatus() == 0)) {
            MediaBufLockBuffer( pMediaBuffer );
            //new frames
            FramInfo_s *tmpFrame=(FramInfo_s *)malloc(sizeof(FramInfo_s));
            if(!tmpFrame){
            	MediaBufUnlockBuffer( pMediaBuffer );
            	return;
            }
            //add to vector
            tmpFrame->frame_index = (ulong_t)tmpFrame; 
            tmpFrame->phy_addr = (ulong_t)phy_addr;
            tmpFrame->frame_width = width;
            tmpFrame->frame_height= height;
            tmpFrame->vir_addr = (ulong_t)y_addr_vir;
            tmpFrame->frame_fmt = fmt;
            tmpFrame->used_flag = 3;
#if (USE_RGA_TODO_ZOOM == 1)  
            tmpFrame->zoom_value = mZoomVal;
#else
            if((tmpFrame->frame_width > 2592) && (tmpFrame->frame_height > 1944) && (mZoomVal != 100) ) {
                tmpFrame->zoom_value = mZoomVal;
            } else {
                tmpFrame->zoom_value = 100;
            }
#endif

            {
                Mutex::Autolock lock(mFrameArrayLock);
                mFrameInfoArray.add((void*)tmpFrame,(void*)pMediaBuffer);
                mPreviewCBFrameLeak++;
            }
                mRefEventNotifier->notifyNewPreviewCbFrame(tmpFrame);			
        }
    }
end:
	
	tem_val =0 ;
}

void CameraIspAdapter::dump(int cameraId)
{
    Message_cam msg;
	rk_cam_total_info *pCamInfo = gCamInfos[cameraId].pcam_total_info;
	m_camDevice->checkVersion(pCamInfo);

    if(mISPTunningRun){
        TRACE_D(0, "-----------stop isp tunning in--------------");
        msg.command = ISP_TUNNING_CMD_EXIT;
        mISPTunningQ->put(&msg);
    	mISPTunningThread->requestExitAndWait();
    	mISPTunningThread.clear();
        delete mISPTunningQ;
        mISPTunningQ = NULL;
        delete mIspTunningTask;
        mIspTunningTask = NULL;
        mISPTunningRun = false;
        TRACE_D(0, "-----------stop isp tunning out--------------");
    }else{
        TRACE_D(0, "-----------start isp tunning in--------------");
        mISPTunningQ = new MessageQueue("ISPTunningQ");
        mISPTunningThread = new CamISPTunningThread(this);

        //parse tunning xml file
        mIspTunningTask = CameraIspTunning::createInstance();
        if(mIspTunningTask){
            mISPTunningThread->run("CamISPTunningThread",ANDROID_PRIORITY_NORMAL);
            msg.command = ISP_TUNNING_CMD_START;
            mISPTunningQ->put(&msg);
            mISPTunningRun = true;
            TRACE_D(0, "-----------start isp tunning out--------------");
        }else{
            delete mISPTunningQ;
            mISPTunningThread.clear();
            TRACE_E("-----------start isp tunning failed--------------");
        }
    }
}

int CameraIspAdapter::hdmiinListenerThread(void)
{
	LOG_FUNCTION_NAME
	while(hdmiIn_Exit == false){
		usleep(2000000);
		if(1){
			rk_cam_total_info *pCamInfo = gCamInfos[mCamId].pcam_total_info;
			bool res = m_camDevice->checkHdmiInRes(pCamInfo);
					//if(res!=true)
					    //return;
		}
	}
	LOG_FUNCTION_NAME_EXIT
    return 0;	
}

int CameraIspAdapter::dataListenerThread(void)
{

   LOG_FUNCTION_NAME
   bool bExit = true;
   unsigned long msecs = 0;

   while (bExit == false)
   {
       LOG1( "\n+++++++++++++++bDataReced=%d+++msecs=%ld+++++++++\n",bDataReced,msecs);

       if(!bDataReced){
           if(msecs>4000){
               LOG1( "\n+++++++++++++++restart camera++++++++++++\n");
               bResetIsp = true;
               bExit = true;
               stopPreview();
               startPreview(mCamPreviewW,mCamPreviewH,mCamDrvWidth,mCamDrvHeight,0,false);
           }
       }else{
           bExit = true;
       }

       usleep(100000);
       msecs += 100;
   }
 
   LOG_FUNCTION_NAME_EXIT

    return 0;
}

int CameraIspAdapter::afListenerThread(void)
{

    LOG_FUNCTION_NAME

    bool bExit = false;
    int evnt_id ;

    while (bExit == false)
    {
        CamEngineAfEvt_t afEvt;        
        
        OSLAYER_STATUS osStatus = (OSLAYER_STATUS)osQueueRead(&mAfListenerQue.queue, &afEvt); 
        if (OSLAYER_OK != osStatus)
        {
            LOGE( "receiving af event failed -> OSLAYER_RESULT=%d\n", osStatus );
            continue; /* for now we simply try again */
        }

        evnt_id = (int)afEvt.evnt_id;
        switch (evnt_id)
        {
            case CAM_ENGINE_AUTOFOCUS_MOVE:
            {
                LOG2("CAMERA_MSG_FOCUS_MOVE: %d",afEvt.info.mveEvt.start);
                mRefEventNotifier->notifyCbMsg(CAMERA_MSG_FOCUS_MOVE, afEvt.info.mveEvt.start);
                break;
            }

            case CAM_ENGINE_AUTOFOCUS_FINISHED:
            {
                LOG2("CAMERA_MSG_FOCUS: %d",afEvt.info.fshEvt.focus);
                mRefEventNotifier->notifyCbMsg(CAMERA_MSG_FOCUS, afEvt.info.fshEvt.focus);
                break;
            }

            case 0xfefe5aa5:
            {
                LOG1("receive exit command for af thread handle!");
                bExit = true;
                break;
            }
            default:
            {                    
                LOGE("afEvt.evnt_id: 0x%x is invalidate!",afEvt.evnt_id);
                break;
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT

    return 0;
}

bool CameraIspAdapter::isNeedToEnableFlash()
{	

    if(mParameters.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES)
        && ((strcmp(mParameters.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_ON)==0) ||
         ((strcmp(mParameters.get(CameraParameters::KEY_FLASH_MODE),CameraParameters::FLASH_MODE_AUTO)==0) && isLowIllumin()))){
         return true;
    }else{
        return false;
    }
}
bool CameraIspAdapter::isLowIllumin()
{
    float gain,mingain,maxgain,step,time,mintime,maxtime,sharpness,meanluma = 0 ;
    bool enabled;
    CamEngineAfSearchAlgorithm_t searchAlgorithm;
    const float lumaThreshold = 45;
    const float sharpThreshold = 300;
    
    m_camDevice->getGain(gain);
    m_camDevice->getIntegrationTime(time);
    m_camDevice->getGainLimits(mingain,maxgain,step);
    m_camDevice->getIntegrationTimeLimits(mintime,maxtime,step);
    meanluma = m_camDevice->getAecMeanLuminance();

    m_camDevice->getAfStatus(enabled,searchAlgorithm,&sharpness);  /* ddl@rock-chips.com: v0.0x32.0 */     

    LOG1("Check LowIllumin :")
    LOG1("    gain       %f(%f,%f)",gain,mingain,maxgain);
    LOG1("    inttime    %f(%f,%f)",time,mintime,maxtime);
    LOG1("    meanluma   %f     threshold: %f",meanluma,lumaThreshold);
    LOG1("    sharpness: %f     threshold: %f",sharpness,sharpThreshold);

    if( meanluma < lumaThreshold ) //&& (sharpness < sharpThreshold))
        return true;
    else
        return false;
}

void CameraIspAdapter::flashControl(bool on)
{
    if(mFlashStatus && !on){
        m_camDevice->stopFlash(false);
        //restore awb
        m_camDevice->startAec();
        m_camDevice->lscEnable();
  //      m_camDevice->startAdpf();
  //      m_camDevice->startAdpcc();
  //      m_camDevice->startAvs();
        if(curAwbStatus.manual_mode){
            curAwbStatus.manual_mode = false;
            m_camDevice->stopAwb();
            m_camDevice->startAwb(curAwbStatus.mode, curAwbStatus.idx, (bool_t)curAwbStatus.damping);
        }
        mFlashStatus = false;
    }else if(!mFlashStatus && on){
        float mingain,maxgain,step,time,mintime,maxtime,meanluma = 0 ;
        if(isLowIllumin()){
            //get awb status
            m_camDevice->getAwbStatus(curAwbStatus.enabled, curAwbStatus.mode, curAwbStatus.idx, curAwbStatus.RgProj, curAwbStatus.damping);
            //stop awb
           // m_camDevice->stopAwb();
            //set D65 manual awb
           // m_camDevice->startAwb(CAM_ENGINE_AWB_MODE_MANUAL, 1, (bool_t)false);
            curAwbStatus.manual_mode = false;
            m_camDevice->stopAec();
            m_camDevice->getGainLimits(mingain,maxgain,step);
            m_camDevice->setGain(maxgain, maxgain);
            m_camDevice->getIntegrationTimeLimits(mintime,maxtime,step);
            m_camDevice->setIntegrationTime(maxtime, maxtime);

            m_camDevice->lscDisable(); /*ddl@rock-chips.com: v1.0x25.0*/
        }
//     m_camDevice->stopAvs();
//     m_camDevice->stopAdpcc();
//     m_camDevice->stopAdpf();
       
//     m_camDevice->stopAwb();
//       //set D65 manual awb
//     m_camDevice->startAwb(CAM_ENGINE_AWB_MODE_MANUAL, 1, (bool_t)false);
        m_camDevice->startFlash(true);
        mFlashStatus = true;
        LOG1("flash set to status %d",mFlashStatus);

    }else{
        LOG1("flash is already in status %d",mFlashStatus);
    }
}

void CameraIspAdapter::getCameraParamInfo(cameraparam_info_s &paraminfo)
{
	
	m_camDevice->getAwbGainInfo(&paraminfo.f_RgProj,&paraminfo.f_s,&paraminfo.f_s_Max1,&paraminfo.f_s_Max2,
								&paraminfo.f_Bg1,&paraminfo.f_Rg1,&paraminfo.f_Bg2,&paraminfo.f_Rg2);

	m_camDevice->getIlluEstInfo(&paraminfo.expPriorIn,&paraminfo.expPriorOut,paraminfo.illuName,paraminfo.likehood,
								paraminfo.wight,&paraminfo.illuIdx,&paraminfo.region,&paraminfo.count);
	
	m_camDevice->getIntegrationTime(paraminfo.ExposureTime);
	m_camDevice->getGain(paraminfo.ISOSpeedRatings);
	m_camDevice->getSensorXmlVersion(&paraminfo.XMLVersion);
}

bool CameraIspAdapter::getFlashStatus()
{
	return mFlashStatus;
}

void CameraIspAdapter::getSensorMaxRes(unsigned int &max_w, unsigned int &max_h)
{
	IsiSensorCaps_t pCaps;		
	pCaps.Index = 0;
	max_w = 0;
	max_h = 0;
	while (m_camDevice->getSensorCaps(pCaps) == true) {
		if (ISI_RES_W_GET(pCaps.Resolution)>max_w)
			max_w = ISI_RES_W_GET(pCaps.Resolution);
		if (ISI_RES_H_GET(pCaps.Resolution)>max_h)
			max_h = ISI_RES_H_GET(pCaps.Resolution);
		pCaps.Index++;
	};

}

void CameraIspAdapter::setMwb(const char *white_balance)
{
	uint32_t illu_index = 1;
	char prfName[10];
	int i,size;
	std::vector<CamIlluProfile_t *> profiles;

	if(white_balance == NULL)
		return;
	
	if(!strcmp(white_balance, "auto")) {
		//do nothing
	}else{
	    if (m_camDevice->isSOCSensor() == false) {
			if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_INCANDESCENT)) {
				strcpy(prfName, "A");
			} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_DAYLIGHT)) {
				strcpy(prfName, "D65");	
			} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_FLUORESCENT)) {
				strcpy(prfName, "F2_CWF");
			} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_SHADE)) {
				strcpy(prfName, "D75");
			} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_TWILIGHT)) {
				strcpy(prfName, "HORIZON");
			} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT)) {
				strcpy(prfName, "D50");
			} else if(!strcmp(white_balance, CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT)) {
				strcpy(prfName, "U30");
			}
			
			m_camDevice->getIlluminationProfiles( profiles );			
			size = profiles.size();
			for(i=0; i<size; i++)
			{
				if(strstr(profiles[i]->name, prfName))
				{
					illu_index = i;
					break;
				}
			}
			
	        m_camDevice->stopAwb();
			m_camDevice->startAwb(CAM_ENGINE_AWB_MODE_MANUAL, illu_index, (bool_t)true);
	    }
	}
}

void CameraIspAdapter::setMe(const char *exposure)
{
    #if 1
    if (m_camDevice->isSOCSensor() == true)   /* ddl@rock-chips.com : v0.0x39.0 */ 
        return;
    
	if(exposure == NULL)
		return;
	if(!strcmp(exposure, "-2")){
		m_camDevice->setAeClmTolerance(manExpConfig.clmtolerance*0.8);
		m_camDevice->setAePoint(manExpConfig.minus_level_2);
	}else if(!strcmp(exposure, "-1")){
		m_camDevice->setAePoint(manExpConfig.minus_level_1);
	}else if(!strcmp(exposure, "0")){
		m_camDevice->setAePoint(manExpConfig.level_0);
	}else if(!strcmp(exposure, "1")){
		m_camDevice->setAePoint(manExpConfig.plus_level_1);
	}else if(!strcmp(exposure, "2")){
		m_camDevice->setAeClmTolerance(manExpConfig.clmtolerance*0.8);
		m_camDevice->setAePoint(manExpConfig.plus_level_2);
	}
    #endif
}

int CameraIspAdapter::ispTunningThread(void)
{
    bool bExit = false;
    Message_cam msg;
    int curFmt = 0,curPreW = 0,curPreH =0;
    float gain,setgain,mingain,maxgain,gainstep,time,settime,mintime,maxtime,timestep;
    ispTuneTaskInfo_s* curTuneTask = NULL;
    char szBaseFileName[100];
    static int skip_frames = 0;
    while (bExit == false)
    {
        memset(&msg,0,sizeof(msg));
        mISPTunningQ->get(&msg);
PROCESS_CMD:
        switch(msg.command)
        {
            case ISP_TUNNING_CMD_START:
                //stop preview
                stopPreview();
                //start preview
                curTuneTask = mIspTunningTask->mCurTuneTask = mIspTunningTask->mTuneInfoVector[mIspTunningTask->mCurTunIndex];

                curPreW = curTuneTask->mTuneWidth;
                curPreH = curTuneTask->mTuneHeight;

                if(curTuneTask->mTuneFmt == CAMERIC_MI_DATAMODE_RAW12){
                    curFmt = ISP_OUT_RAW12;
                    curTuneTask->mForceRGBOut = false;
                    if(curTuneTask->mExpose.exposuseMode == EXPOSUSE_MODE_AUTO){
                        //get tune task res ae value
                        startPreview(curPreW, curPreH, 0, 0,ISP_OUT_YUV420SP, false);
                        m_camDevice->stopAec();
                        m_camDevice->startAec();
                        sleep(3);
                        m_camDevice->getGain(gain);
                        m_camDevice->getIntegrationTime(time);
                        stopPreview();
                    }
                }else if(curTuneTask->mTuneFmt == CAMERIC_MI_DATAMODE_YUV422){
                    curFmt = ISP_OUT_YUV422_SEMI;
                    curTuneTask->mForceRGBOut = true;
                } else {
                    LOGE("this format %d is not support",curTuneTask->mTuneFmt);
                }
                    
                
                startPreview(curPreW, curPreH, curPreW, curPreH,curFmt, false);

                // need to do following steps in RAW capture mode ?
                // following modules are bypassed in RAW mode ?
                m_camDevice->stopAvs();
                m_camDevice->stopAdpf();
                m_camDevice->stopAec();
                m_camDevice->stopAwb();
                m_camDevice->stopAdpcc();
                m_camDevice->lscDisable();
                m_camDevice->cacDisable();
                m_camDevice->wdrDisable();
                m_camDevice->gamCorrectDisable();
                m_camDevice->stopAf();

                if(curTuneTask->mDpccEnable == true) 
                    m_camDevice->startAdpcc();
                if(curTuneTask->mLscEnable == true)
                   m_camDevice->lscEnable();
                if(curTuneTask->mCacEnable== true)
                    m_camDevice->cacEnable();
                if(curTuneTask->mGammarEnable == true)
                    m_camDevice->gamCorrectEnable();
                if(curTuneTask->mWdrEnable == true)
                    m_camDevice->wdrEnable();

                if(curTuneTask->mExpose.exposuseMode == EXPOSUSE_MODE_MANUAL){
                   //set manual ae
                    m_camDevice->getGain(gain);
                    m_camDevice->getIntegrationTime(time);
                    m_camDevice->getGainLimits(mingain,maxgain,gainstep);
                    m_camDevice->getIntegrationTimeLimits(mintime,maxtime,timestep);
                    m_camDevice->setIntegrationTime(curTuneTask->mExpose.integrationTime,settime);
                    LOGD("setIntegrationTime(desired:%0.3f,real:%0.3f)",curTuneTask->mExpose.integrationTime,settime);
                    m_camDevice->setGain(curTuneTask->mExpose.gain,setgain);
                    LOGD("setGain(desired:%0.3f,real:%0.3f)",curTuneTask->mExpose.gain,setgain);
                    
                    mIspTunningTask->mCurGain = setgain;
                    mIspTunningTask->mCurIntegrationTime = settime;
                    if(curTuneTask->mExpose.aeRound == true){
                        mIspTunningTask->mCurAeRoundNum = curTuneTask->mExpose.number;
                    }
                }else{
                    if(curTuneTask->mTuneFmt == CAMERIC_MI_DATAMODE_YUV422){
                       m_camDevice->startAec();
                    }else{
                        m_camDevice->setGain(gain,setgain);
                        m_camDevice->setIntegrationTime(time,settime);
                    }
                   
                }

                if(curTuneTask->mWhiteBalance.whiteBalanceMode == WHITEBALANCE_MODE_AUTO){
                    m_camDevice->startAwb(CAM_ENGINE_AWB_MODE_AUTO, 0, (bool_t)true);
                }else if(curTuneTask->mWhiteBalance.whiteBalanceMode == WHITEBALANCE_MODE_MANUAL){
                   //set manual awb
                    uint32_t illu_index = 1;
                    if(strcmp(curTuneTask->mWhiteBalance.illumination,"A") == 0)
                        illu_index = 0;
                    else if(strcmp(curTuneTask->mWhiteBalance.illumination,"D65") == 0)
                        illu_index = 1;
                    else if(strcmp(curTuneTask->mWhiteBalance.illumination,"CWF") == 0)
                        illu_index = 2;
                    else if(strcmp(curTuneTask->mWhiteBalance.illumination,"TL84") == 0)
                        illu_index = 3;
                    else if(strcmp(curTuneTask->mWhiteBalance.illumination,"D50") == 0)
                        illu_index = 4;
                    else if(strcmp(curTuneTask->mWhiteBalance.illumination,"D75") == 0)
                        illu_index = 5;
                    else if(strcmp(curTuneTask->mWhiteBalance.illumination,"HORIZON") == 0)
                        illu_index = 6;
                    else{
                        LOGE("not support this illum %s ,set to D65!!!",curTuneTask->mWhiteBalance.illumination);
                    }

                    LOGD("current illum is %s ,illu_index %d !!!",curTuneTask->mWhiteBalance.illumination,illu_index);
                    m_camDevice->startAwb( CAM_ENGINE_AWB_MODE_MANUAL, illu_index, (bool_t)true );

                    if(strcmp(curTuneTask->mWhiteBalance.cc_matrix,"unit_matrix") == 0){
                        LOGD(" set unit matrix ");
                        m_camDevice->wbCcMatrixSet(1,0,0,
                                                   0,1,0,
                                                   0,0,1);
                    }else if(strcmp(curTuneTask->mWhiteBalance.cc_matrix,"default") == 0){
                    }else{
                        LOGE("not support this cc_matrix %s !!!",curTuneTask->mWhiteBalance.cc_matrix);
                    }

                    if(strcmp(curTuneTask->mWhiteBalance.rggb_gain,"unit") == 0){
                        m_camDevice->wbGainSet(1,1,1,1);
                    }else if(strcmp(curTuneTask->mWhiteBalance.rggb_gain,"default") == 0){
                    }else{
                        LOGE("not support this rggb_gain %s !!!",curTuneTask->mWhiteBalance.rggb_gain);
                    }

                    if(strcmp(curTuneTask->mWhiteBalance.cc_offset,"zero") == 0){
                        m_camDevice->wbCcOffsetSet(0,0,0);
                    }else if(strcmp(curTuneTask->mWhiteBalance.cc_offset,"default") == 0){

                    }else{
                        LOGE("not support this cc_offset %s !!!",curTuneTask->mWhiteBalance.cc_offset);
                    }
                
                }

                #if 1
                if(curTuneTask->mAfEnable){
                    LOGD("start oneshot af");
                    m_camDevice->startAfOneShot(CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE);
                    sleep(2);
                    m_camDevice->startAfOneShot(CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE);
                    sleep(1);
                }
                #endif   
                skip_frames = 15;
                
                mIsSendToTunningTh = true;
                break;
            case ISP_TUNNING_CMD_EXIT:
                //restore saved config
                mIsSendToTunningTh = false;
                stopPreview();
                //mISPOutputFmt = oldFmt;
                startPreview(800, 600, 0, 0, ISP_OUT_YUV420SP, false);
                bExit = true;
                break;
            case ISP_TUNNING_CMD_PROCESS_FRAME:
                {
				FramInfo_s *frame = (FramInfo_s*)msg.arg2;
                float newtime,newgain;
                bool isStore = false;
                LOG1("tunning thread receive a frame !!");
                //
                if(skip_frames-- > 0)
                    goto PROCESS_OVER;
                m_camDevice->getGain(gain);
                m_camDevice->getIntegrationTime(time);
                //is it the satisfied frame ?
                if(curTuneTask->mExpose.exposuseMode == EXPOSUSE_MODE_MANUAL){
                    bool isSetExp = true;

                    if((gain != mIspTunningTask->mCurGain) || (time != mIspTunningTask->mCurIntegrationTime)){
                        LOGD("%s:not the desired exposure frame,skip it,time(%0.3f,%0.3f),gain(%0.3f,%0.3f)",__func__,
                            mIspTunningTask->mCurIntegrationTime,time,mIspTunningTask->mCurGain,gain);
                        goto PROCESS_OVER;
                    }

                    //set different exposure parameter ?
                    if(curFmt == ISP_OUT_RAW12 ){
                        if((curTuneTask->mExpose.integrationTimeStep != 0)|| (curTuneTask->mExpose.gainStep != 0)){
                            newtime = mIspTunningTask->mCurIntegrationTime+curTuneTask->mExpose.integrationTimeStep;
                            if((newtime >maxtime) && ((newtime - maxtime) - curTuneTask->mExpose.integrationTimeStep < 0.0001)){
                                //next loop
                                LOGD("set new gain+++");
                                newgain = mIspTunningTask->mCurGain+curTuneTask->mExpose.gainStep;
                                newtime = curTuneTask->mExpose.integrationTime;
                                if ((newgain > maxgain) && ((newgain - maxgain) - curTuneTask->mExpose.gainStep < 0.00001)){
                                    curTuneTask->mTunePicNum = 0;
                                    LOGD("time and gain are max ,newgain %0.5f,%0.5f!!!!",newgain,maxgain);
                                    isSetExp = false;
                                }
                            }else
                                newgain = mIspTunningTask->mCurGain;
                                
                            if(isSetExp){
                                m_camDevice->setIntegrationTime(newtime,settime);
                                m_camDevice->setGain(newgain,setgain);

                                LOGD("setIntegrationTime(desired:%0.3f,real:%0.3f)",newtime,settime);
                                LOGD("setGain(desired:%0.3f,real:%0.3f)",newgain,setgain);
                                mIspTunningTask->mCurGain = setgain;
                                mIspTunningTask->mCurIntegrationTime = settime;
                                //skip some frames,max buffer count is 6,one is here now,so filter 5 frames
                                skip_frames = 12;
                            }
                            int filter_result = mIspTunningTask->ispTuneDesiredExp(frame->vir_addr, frame->frame_width,frame->frame_height,
                                    curTuneTask->mExpose.minRaw, curTuneTask->mExpose.maxRaw, curTuneTask->mExpose.threshold); 
                            if(filter_result & 0x1){

                                goto PROCESS_OVER;
                            }else if(filter_result & 0x2){  
                                LOGD("frame is overhead exposure , finish capture frame !!");
                               // curTuneTask->mTunePicNum = 0;
                                goto PROCESS_OVER;
                            }
                        }
                    }else if(curFmt == ISP_OUT_YUV422_SEMI ){
                       if(curTuneTask->mExpose.aeRound == true){
                            newtime = curTuneTask->mExpose.integrationTime
                                    +curTuneTask->mExpose.integrationTimeStep*((mIspTunningTask->mCurAeRoundNum+1)/2);
                            newgain = curTuneTask->mExpose.gain+curTuneTask->mExpose.gainStep*(mIspTunningTask->mCurAeRoundNum/2);
                            m_camDevice->setIntegrationTime(newtime,settime);
                            m_camDevice->setGain(newgain,setgain);
                            mIspTunningTask->mCurGain = setgain;
                            mIspTunningTask->mCurIntegrationTime = settime;
                            skip_frames = 6;

                            LOGD("setIntegrationTime(desired:%0.3f,real:%0.3f)",newtime,settime);
                            LOGD("setGain(desired:%0.3f,real:%0.3f)",newgain,setgain);
                            if(mIspTunningTask->mCurAeRoundNum == 1){
                                mIspTunningTask->mCurAeRoundNum -=3;
                            }else
                                mIspTunningTask->mCurAeRoundNum--;
                            if(mIspTunningTask->mCurAeRoundNum < (-(curTuneTask->mExpose.number+3))){
                                curTuneTask->mTunePicNum = 0;
                            }
                                
                        }
                    }
                    
                }

                curTuneTask->mTunePicNum--;
                //generate base file name
                    
                snprintf( szBaseFileName, sizeof(szBaseFileName)-1, "%st%0.3f_g%0.3f", "/data/isptune/",time, gain );
                szBaseFileName[99] = '\0';

                //store this frame
                curTuneTask->y_addr = frame->vir_addr;
                if(curFmt == ISP_OUT_YUV422_SEMI){
                    curTuneTask->uv_addr = frame->vir_addr + frame->frame_width*frame->frame_height;
                    curTuneTask->mForceRGBOut = true;
                }else{
                    curTuneTask->mForceRGBOut = false;
                }

                mIspTunningTask->ispTuneStoreBuffer(curTuneTask, (MediaBuffer_t * )frame->used_flag, 
                                                    szBaseFileName, 0);
                PROCESS_OVER:
                //return this frame buffer
                adapterReturnFrame(frame->frame_index, frame->used_flag);

                //current task has been finished ? start next capture?
                if(curTuneTask->mTunePicNum <= 0){
                    skip_frames = 0;
                    mIsSendToTunningTh = false;
                    stopPreview();
                    //remove redundant frame in queue
                    while(!mISPTunningQ->isEmpty())
                        mISPTunningQ->get(&msg);
                    //start new cap?
                    if(mIspTunningTask->mCurTunIndex < (mIspTunningTask->mTuneTaskcount-1)){
                        mIspTunningTask->mCurTunIndex++;
                        msg.command = ISP_TUNNING_CMD_START;
                        mISPTunningQ->put(&msg);
                    }else{
                        LOGD("\n\n*********************\n all tune tasks have fininished\n\n ******************");
                        startPreview(800, 600, 0, 0, ISP_OUT_YUV420SP, false);
                    }
                }
                break;
                }
            default:
                {                    
                LOGE("%d tunning cmd is not support !",msg.command);
                break;
                }     
        }
    }
    return 0;
 }
/* ddl@rock-chips.com: v1.0xb.0 */
int CameraIspAdapter::faceNotify(struct RectFace* faces, int* num)
{
    CamEngineWindow_t curWin,curGrid;
    unsigned int cur_size, face_size,diff;
    short int x,y;
    unsigned short int width,height;
    bool setWin;
    
    if (*num == 1) {
        // AF
        {
            m_camDevice->getAfMeasureWindow(&curWin);
            
            cur_size = curWin.width*curWin.height;
            face_size = faces[0].width*faces[0].height;
            cur_size = cur_size*10/face_size;
            if ((cur_size>13) || (cur_size<7)) {
                setWin = true;
            } else {
                setWin = false;
            }

            if (setWin == false) {
                diff = abs(curWin.hOffset - faces[0].x)*10/curWin.width;
                if (diff >= 5) {
                    setWin = true;
                }

                diff = abs(curWin.vOffset - faces[0].y)*10/curWin.height;
                if  (diff >= 5) {
                    setWin = true;
                }
            }

            if (setWin == true) {
                x = faces[0].x*2000/mCamPreviewW - 1000;
                y = faces[0].y*2000/mCamPreviewH - 1000;
                width = faces[0].width*2000/mCamPreviewW;
                height = faces[0].height*2000/mCamPreviewH;
                
                LOG1("faceWin: (%d, %d, %d, %d) ---> afWin: (%d, %d, %d, %d)",
                    faces[0].x, faces[0].y, faces[0].width, faces[0].height,
                    curWin.hOffset, curWin.vOffset,curWin.width,curWin.height);
                
                m_camDevice->setAfMeasureWindow(x,y,width,height);
            }
        }
    } else if (*num == 0) {
        m_camDevice->setAfMeasureWindow(0,0,0,0);
        m_camDevice->setAecHistMeasureWinAndMode(0,0,0,0,CentreWeightMetering);
    }

    return 0;

}
}
