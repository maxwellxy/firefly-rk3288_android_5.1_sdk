#define LOG_TAG "FrameQueueManage"

#include "FrameQueueManage.h"
#include "vob_blend_cmd.h"
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MetaData.h>
#include "include/AwesomePlayer.h"
#include <sys/prctl.h>
#include <dlfcn.h>  // for dlopen/dlclose

#include <dlfcn.h>
#include <cutils/properties.h>

namespace android {

FrameQueue::FrameQueue(MediaBuffer *data, int64_t tr)
{
	info = data;
	time = tr;
	next = NULL;
    isSwDec = false;
}

FrameQueue::~FrameQueue()
{
    if(!isSwDec){
		info->releaseframe();
    }else{
        info->release();
    }
	info = NULL;
	next = NULL;
}

FrameQueueManage::FrameQueueManage()
	:mThread_sistarted(false),
	mIPPThread_sistarted(false),
	stopFlag(false),
	mIppStopFlag(false),
	mIppRun(false),
	deintFlag(0),
#if USE_DEINTERLACE_DEV
    deint(NULL)
#else
    ipp_fd(-1)
#endif
{
	pstart = NULL;
	pend = NULL;
	pdisplay = NULL;
	plastdisplay = NULL;
    pdeInterlaceBuffer = NULL;
    deintpollFlag = false;
    OrignMediaBuffer = NULL;
	num = 0;
    numdeintlace = 0;
	run = false;
    cacheNum = CACHE_NUM;

    mCurrentSubIsVobSub = 0;
    mHaveReadOneFrm = false;
    isSwDecFlag = false;
    memset(&mPlayerExtCfg, 0, sizeof(AwesomePlayerExt_t));
    memset(&mVobBlend, 0, sizeof(VobBlend_t));
}

FrameQueueManage::~FrameQueueManage()
{
	Mutex::Autolock autoLock(mLock);
	FrameQueue	*ptmp;
	VPU_FRAME	*frame;


	ptmp = pstart;
	while(ptmp)
	{
		frame = (VPU_FRAME*)ptmp->info->data();
		//LOGI("vpumem:ph:0x%08x,vir:0x%08x,size:0x%08x\n",frame->vpumem.phy_addr,(unsigned int)frame->vpumem.vir_addr,frame->vpumem.size);
		ptmp = ptmp->next;
	}

	while(pstart)
	{
		ptmp = pstart;
		pstart = pstart->next;
		ALOGI("relese frame start ! num:%d\n",num--);
		delete ptmp;
	}
    while(!DeInterlaceFrame.isEmpty())
    {
       MediaBuffer *mediaBuffer = DeInterlaceFrame.editItemAt(0);
       DeInterlaceFrame.removeAt(0);
       mediaBuffer->releaseframe();
    }
    if (deintpollFlag)
    {
#if USE_DEINTERLACE_DEV
        if (deint) deint->sync();
#else
        int ret = -1,result = -1;
        if(poll(&fd,1,-1)>0)
        {
            ret = ioctl(ipp_fd, IPP_GET_RESULT, &result);
            if(ret)
            {
                ALOGE("ioctl:IPP_GET_RESULT faild!");
                return;
            }
        }
        else
        {
            ALOGE("poll timeout");
        }
#endif
        deintpollFlag = false;
    }
    if(OrignMediaBuffer)
    {
        OrignMediaBuffer->releaseframe();
        OrignMediaBuffer = NULL;
    }
    if(pdeInterlaceBuffer)
    {
        pdeInterlaceBuffer->releaseframe();
        pdeInterlaceBuffer = NULL;
    }
	pstart = NULL;
	pend = NULL;
	pdisplay = NULL;
	plastdisplay = NULL;
    pdeInterlaceBuffer = NULL;
    deintpollFlag = false;
	num = 0;
	run = false;
#if USE_DEINTERLACE_DEV
    if (deint) delete deint;
#else
    if(deintFlag)
        close(ipp_fd);
#endif
    deintFlag = 0;

    /* release vobsub blender */
    VobBlend_t* p = &mVobBlend;
    if (p->blender && p->ctrl_fun) {
        p->ctrl_fun(VOB_BLEND_CTRL_DESTROY, &p->blender, NULL);

        p->blender = NULL;
        p->ctrl_fun = NULL;
    }

	ALOGI("FrameQueueManage::~FrameQueueManage end!\n");
}

void FrameQueueManage::start(void *pram)
{
	Mutex::Autolock autoLock(mLock);

	if (mThread_sistarted)
		return;
    prctl(PR_SET_NAME, (unsigned long)"DISPLAY_MANAGE", 0, 0, 0);
	mThread_sistarted = true;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&mThread, &attr, Threadproc, pram);
	pthread_attr_destroy(&attr);
}

void FrameQueueManage::startIppThread(){
   if (mIPPThread_sistarted)
       return;
   mIPPThread_sistarted = true;
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_create(&mIPPThread, &attr, ThreadIPP, this);
   pthread_attr_destroy(&attr);
}
void FrameQueueManage::stop()
{
	if (!mThread_sistarted)
		return;
	{
		Mutex::Autolock autoLock(mLock);
		run = false;
		stopFlag = true;
	}
    {
        Mutex::Autolock autoLock(mIppLock);
        mIppRun = false;
        mIppStopFlag = true;
    }
	void *dummy;
    pthread_join(mThread, &dummy);
	mThread_sistarted = false;
    if (!mIPPThread_sistarted)
		return;
    pthread_join(mIPPThread, &dummy);
	mIPPThread_sistarted = false;
}

int FrameQueueManage::pushframe(FrameQueue * frame)
    {
    	Mutex::Autolock autoLock(mLock);
        if (pend)
        {
            pend->next = frame;
            pend = frame;
        }
        else
        {
            pstart = frame;
            pend = frame;
            pdisplay = frame;
        }
        num++;

	return true;
}

int FrameQueueManage::pushDeInterlaceframe(MediaBuffer* frame)
{
	Mutex::Autolock autoLock(mLock);
    DeInterlaceFrame.push(frame);
    numdeintlace++;
	return true;
}
MediaBuffer* FrameQueueManage::getNextFrameDinterlace()
{
	Mutex::Autolock autoLock(mLock);
    if (!DeInterlaceFrame.isEmpty())
    {
       MediaBuffer *mediaBuffer = DeInterlaceFrame.editItemAt(0);
       DeInterlaceFrame.removeAt(0);
       numdeintlace--;
       return mediaBuffer;
    }
    else
    {
        return NULL;
    }
}
int FrameQueueManage::deleteframe(int64_t curtime)
{
	//Mutex::Autolock autoLock(mLock);

	FrameQueue	*ptmp;
	if (num <= 0)
		return false;
	while(pstart != pdisplay)
	{
		ptmp = pstart;
		pstart = pstart->next;
		if(ptmp)
			delete ptmp;
		num--;
	}

	return true;
}

FrameQueue* FrameQueueManage::popframe()
{
	Mutex::Autolock autoLock(mLock);

	return pstart;
}

FrameQueue* FrameQueueManage::getNextDiplayframe()
{
	if (pdisplay)
		return pdisplay->next;
	else
		return NULL;
}

void FrameQueueManage::flushframes(bool end)
{
	Mutex::Autolock autoLock(mLock);

	FrameQueue	*ptmp = NULL;

	while(pstart)
	{
		ptmp = pstart;
		pstart = pstart->next;
		if (ptmp != pdisplay)
			delete ptmp;
	}

    {
        Mutex::Autolock autoLock(mIppLock);
        while(!DeInterlaceFrame.isEmpty())
        {
    	       MediaBuffer *mediaBuffer = DeInterlaceFrame.editItemAt(0);
    	       DeInterlaceFrame.removeAt(0);
    	       mediaBuffer->releaseframe();
        }
        if(deintpollFlag)
        {
#if USE_DEINTERLACE_DEV
            if (deint) deint->sync();
#else
            int ret = -1,result = -1;
            if(poll(&fd,1,-1)>0)
            {
                ret = ioctl(ipp_fd, IPP_GET_RESULT, &result);
                if(ret)
                {
                    ALOGE("ioctl:IPP_GET_RESULT faild!");
                    return;
                }
            }
            else
            {
                ALOGE("poll timeout");
            }
#endif
            deintpollFlag = false;
        }
        if(OrignMediaBuffer)
        {
            OrignMediaBuffer->releaseframe();
            OrignMediaBuffer = NULL;
        }
    	if(end)
    	{
    		if(pdisplay)
    			delete pdisplay;
            if(pdeInterlaceBuffer)
            {
                pdeInterlaceBuffer->releaseframe();
                pdeInterlaceBuffer = NULL;
            }
    		pstart = NULL;
    		pend = NULL;
    		pdisplay = NULL;
    		plastdisplay = NULL;
            pdeInterlaceBuffer = NULL;
            deintpollFlag = false;
    		num = 0;
            numdeintlace = 0;
    	}
    	else
    	{
    		num = 0;
    		if (pdisplay)
    		{
    			pdisplay->next = NULL;
    			num = 1;
    		}
            if(pdeInterlaceBuffer)
            {
                pdeInterlaceBuffer->releaseframe();
                pdeInterlaceBuffer = NULL;
            }
            numdeintlace = 0;
    		pstart = pdisplay;
    		pend = pdisplay;
    		plastdisplay = NULL;
            pdeInterlaceBuffer = NULL;
            deintpollFlag = false;
    	}
    }
}


void FrameQueueManage::updateDisplayframe()
{
	pdisplay = pdisplay->next;
}

void FrameQueueManage::play()
{
    {
	    Mutex::Autolock autoLock(mLock);
	    run = true;
    }
    {
        Mutex::Autolock autoLock(mIppLock);
         mIppRun = true;
    }
}

void FrameQueueManage::pause()
{
    {
	    Mutex::Autolock autoLock(mLock);
	    run = false;
    }
    {
        Mutex::Autolock autoLock(mIppLock);
        mIppRun = false;
    }
}

bool FrameQueueManage::runstate()
{
	return run;
}

int FrameQueueManage::Displayframe(void *ptr)
{
	Mutex::Autolock autoLock(mLock);
	AwesomePlayer *ap = (AwesomePlayer*)ptr;
	int	sleeptime = 0;
	{

		if (stopFlag)
		{
			ALOGD("FrameQueueManage::Displayframe stoped");
			return -1;
		}
		if (run)
		{
			//sleeptime = ap->onDisplayEvent();
                    ALOGV("onDisplayEvent out");
		}else{
            return 8000l;
		}
		//else
		//	LOGD("FrameQueueManage::Displayframe pauseed");
	}

	return sleeptime;
}

void* FrameQueueManage::Threadproc(void *me)
{
	AwesomePlayer *ap = (AwesomePlayer*)me;

	while(1)
	{
		int	sleeptime = ap->pfrmanager->Displayframe(ap);
		if(sleeptime<0)
			break;
		usleep((int64_t)sleeptime);
	}

	return NULL;
}
void* FrameQueueManage::ThreadIPP(void *me)
{
    prctl(PR_SET_NAME, (unsigned long)"IppProc", 0, 0, 0);
	FrameQueueManage *ap = (FrameQueueManage*)me;
	while(1)
	{
        int64_t timeUs = ap->IppProc();
        if(timeUs > 0){
            usleep(timeUs);
        }else{
            break;
        }
	}
	return NULL;
}
int64_t FrameQueueManage::IppProc(){
    Mutex::Autolock autoLock(mIppLock);
    if (mIppStopFlag)
    {
        return -1;
    }
    if(deintFlag && mIppRun){
        if (DeinterlaceProc() <0) {
            return -1;
        }

        if(deintpollFlag){
            DeinterlacePoll();
        }else{
           return 4000;
        }
    }else{
        return 8000;
    }
    return 8000;
}

void FrameQueueManage::subtitleNotify(int msg, void* obj) {
    int32_t ret = 0;
    VobBlend_t* p = &mVobBlend;

    switch(msg) {
        case SUBTITLE_MSG_VOBSUB_GET:
            if ((p->blender !=NULL) && (p->ctrl_fun !=NULL)) {
                /* Queue sub picture to vob_blender */
                ret = p->ctrl_fun(VOB_BLEND_CTRL_VOB_SUB_QUEUE, &p->blender, obj);
            }
            break;

        case SUBTITLE_MSG_VOBSUB_FLAG:
            mCurrentSubIsVobSub = *((uint32_t*)obj);
            if ((p->blender == NULL) && mCurrentSubIsVobSub) {
               ret = createvobBlender();
            }
            break;

        default:
            break;
    }
}


int FrameQueueManage::DeinterlaceProc()
{
    pdeInterlaceBuffer = getNextFrameDinterlace();
	if (NULL != pdeInterlaceBuffer) {
        VPU_FRAME *vpu_frame = (VPU_FRAME*)pdeInterlaceBuffer->data();
#if USE_DEINTERLACE_DEV
        MediaBuffer* orig = new MediaBuffer(sizeof(VPU_FRAME));
        memcpy(orig->data(),pdeInterlaceBuffer->data(),sizeof(VPU_FRAME));
        status_t ret = deint->perform(vpu_frame, mCurrentSubIsVobSub);
        if (ret) {
            ALOGE("perform deinterlace failed");
            orig->release();
            orig = NULL;
            mPlayerExtCfg.ionMemAllocFail = true;
            return -1;
        } else {
            OrignMediaBuffer = orig;
            deintpollFlag = true;
        }
#else
        ALOGV("DeinterlaceProc get frame for process");
        struct rk29_ipp_req ipp_req;
        memset(&ipp_req,0,sizeof(rk29_ipp_req));
        VPUMemLinear_t deInterlaceFrame;
        ipp_req.src0.YrgbMst = vpu_frame->FrameBusAddr[0];
        ipp_req.src0.CbrMst = vpu_frame->FrameBusAddr[1];
        ipp_req.src0.w = vpu_frame->DisplayWidth;
        ipp_req.src0.h = vpu_frame->DisplayHeight;
        ipp_req.src0.fmt = IPP_Y_CBCR_H2V2;
        ipp_req.dst0.w = vpu_frame->DisplayWidth;
        ipp_req.dst0.h = vpu_frame->DisplayHeight;
        ipp_req.src_vir_w = vpu_frame->FrameWidth;
        ipp_req.dst_vir_w = vpu_frame->FrameWidth;
        ipp_req.timeout = 100;
        ipp_req.flag = IPP_ROT_0;
        ipp_req.deinterlace_enable = mCurrentSubIsVobSub ? 0 : 1;
	    ipp_req.deinterlace_para0 = 8;
	    ipp_req.deinterlace_para1 = 16;
	    ipp_req.deinterlace_para2 = 8;
        ALOGV("vpu_frame->FrameHeight %d,vpu_frame->FrameWidth %d",vpu_frame->FrameHeight,vpu_frame->FrameWidth);
        int err = VPUMallocLinear(&deInterlaceFrame, vpu_frame->FrameHeight*vpu_frame->FrameWidth*3/2);
        if(err)
        {
            mPlayerExtCfg.ionMemAllocFail = true;
            return -1;
        }
        vpu_frame->FrameBusAddr[0] = deInterlaceFrame.phy_addr;
        vpu_frame->FrameBusAddr[1] = vpu_frame->FrameBusAddr[0] + vpu_frame->FrameHeight*vpu_frame->FrameWidth;
        ipp_req.dst0.YrgbMst =  vpu_frame->FrameBusAddr[0];
        ipp_req.dst0.CbrMst =  vpu_frame->FrameBusAddr[1];
        int ret = ioctl(ipp_fd, IPP_BLIT_ASYNC, &ipp_req);
       	if(ret)
        {
            ALOGE("ioctl: IPP_BLIT_ASYNC faild!");
            return -1;
        }
        OrignMediaBuffer = new MediaBuffer(sizeof(VPU_FRAME));
        memcpy(OrignMediaBuffer->data(),pdeInterlaceBuffer->data(),sizeof(VPU_FRAME));
        deintpollFlag = true;
        VPUMemDuplicate(&vpu_frame->vpumem,&deInterlaceFrame);
        VPUFreeLinear(&deInterlaceFrame);
#endif
    }

    return 0;
}

int32_t FrameQueueManage::createvobBlender()
{
    int32_t ret =0;
    void* pRkDemuxLib = NULL;
    VobBlend_t* p = &mVobBlend;
    if (p->ctrl_fun == NULL) {
        pRkDemuxLib = RkDemuxLib();
        if (pRkDemuxLib == NULL) {
            ALOGE("RkDemuxLib get instance fail");
            return -1;
        }
        p->ctrl_fun =
            (vobBlendCtrlFunc)dlsym(pRkDemuxLib, "_ZN7android8VobBlend13vobBlendCtrolEiPPvS1_");
        if (p->ctrl_fun == NULL) {
            ALOGE("dlsym vobBlendCtrol function fail");
            return -1;
        } else {
            ret = p->ctrl_fun(VOB_BLEND_CTRL_CREATE, &p->blender, NULL);
            if (p->blender ==NULL) {
                ALOGE("create vob blender fail");
            }
        }
    }

    return ret;
}

void FrameQueueManage::flushVobSubQueue() {
    Mutex::Autolock autoLock(mIppLock);
    ALOGV("flushVobSubQueue in");

    VobBlend_t* p = &mVobBlend;
    if (p->blender && p->ctrl_fun) {
        p->ctrl_fun(VOB_BLEND_CTRL_FLUSH, &p->blender, NULL);
    }
}

void FrameQueueManage::DeinterlacePoll()
{
    VPU_FRAME *vpu_frame = NULL;
    int result = -1,ret = -1;
    int64_t timeUs = 0;
    int32_t vobSubQueSize = 0;
    VobBlend_t* p = &mVobBlend;

    if(deintpollFlag) {
#if USE_DEINTERLACE_DEV
        if (deint->sync())
            return;
#else
		if(poll(&fd,1,-1)>0) {
			ret = ioctl(ipp_fd, IPP_GET_RESULT, &result);
			if(ret) {
				ALOGE("ioctl:IPP_GET_RESULT faild!");
				return;
			}
		} else {
			ALOGE("IPP poll faild!");
			return;
		}
#endif

		if(OrignMediaBuffer) {
            OrignMediaBuffer->releaseframe();
            OrignMediaBuffer = NULL;
		}

        if (p->blender && p->ctrl_fun) {
            vobSubQueSize = p->ctrl_fun(
                VOB_BLEND_CTRL_GET_QUE_SIZE, &p->blender, NULL);

            if (vobSubQueSize >0) {
                p->ctrl_fun(VOB_BLEND_CTRL_VOB_SUB_BLEND,
                        &p->blender, pdeInterlaceBuffer);
            }
        }

        FrameQueue *frame = new FrameQueue(pdeInterlaceBuffer);
        frame->isSwDec = isSwDecFlag;
		pushframe(frame);
		pdeInterlaceBuffer = NULL;
		deintpollFlag = false;
		ALOGV("DeinterlaceProc poll out");
    }
}

#include "vpu.h"
#include <cutils/properties.h> // for property_get

deinterlace_dev::deinterlace_dev(int size)
	:dev_status(USING_NULL),dev_fd(-1),
	pool(NULL),
	priv_data(NULL)
{
    dev_fd = open("/dev/rk29-ipp", O_RDWR, 0);
    if (dev_fd > 0) {
        dev_status = USING_IPP;
    } else {
        if (access("/dev/iep", 06) == 0) {
            iep_lib_handle = dlopen("/system/lib/libiep.so", RTLD_LAZY);
            if (iep_lib_handle == NULL) {
                ALOGE("dlopen iep library failure\n");
                return;
            }

            ops.claim = (void* (*)())dlsym(iep_lib_handle, "iep_ops_claim");
            ops.reclaim = (void* (*)(void *iep_obj))dlsym(iep_lib_handle, "iep_ops_reclaim");
            ops.init_discrete = (int (*)(void *iep_obj,
                          int src_act_w, int src_act_h,
                          int src_x_off, int src_y_off,
                          int src_vir_w, int src_vir_h,
                          int src_format,
                          int src_mem_addr, int src_uv_addr, int src_v_addr,
                          int dst_act_w, int dst_act_h,
                          int dst_x_off, int dst_y_off,
                          int dst_vir_w, int dst_vir_h,
                          int dst_format,
                          int dst_mem_addr, int dst_uv_addr, int dst_v_addr))dlsym(iep_lib_handle, "iep_ops_init_discrete");
            ops.config_yuv_deinterlace = (int (*)(void *iep_obj))dlsym(iep_lib_handle, "iep_ops_config_yuv_deinterlace");
            ops.run_async_ncb = (int (*)(void *iep_obj))dlsym(iep_lib_handle, "iep_ops_run_async_ncb");
            ops.poll = (int (*)(void *iep_obj))dlsym(iep_lib_handle, "iep_ops_poll");
            if (ops.claim == NULL || ops.reclaim == NULL || ops.init_discrete == NULL
                || ops.config_yuv_deinterlace == NULL || ops.run_async_ncb == NULL || ops.poll == NULL) {
                ALOGE("dlsym iep library failure\n");
                dlclose(iep_lib_handle);
                return;
            }

            api = (void*)ops.claim();
            if (api == NULL) {
                ALOGE("iep api claim failure\n");
                dlclose(iep_lib_handle);
                return;
            }

            dev_status = USING_IEP;
            ALOGD("Deinterlace Using IEP\n");
        } else {
            char prop_value[PROPERTY_VALUE_MAX];
            if (property_get("sys.sf.pp_deinterlace", prop_value, NULL) && atoi(prop_value) > 0) {
                dev_fd = VPUClientInit(VPU_PP);
                if (dev_fd > 0) {
                    dev_status = USING_PP;
                    ALOGI("try to use PP ok");
                } else {
                    ALOGW("found no hardware for deinterlace just skip!!");
                }
            } else {
                ALOGW("no ipp to do deinterlace but pp is disabled");
                dev_fd = -1;
            }
        }
    }
   if (0 > create_vpu_memory_pool_allocator(&pool, 2, size)) {
       ALOGE("Create vpu memory pool for deinterlace failed\n");
       pool = NULL;
   }
}

deinterlace_dev::~deinterlace_dev()
{
    switch (dev_status) {
    case USING_IEP : {
        ops.reclaim(api);
        dlclose(iep_lib_handle);
    } break;
    case USING_IPP : {
        close(dev_fd);
    } break;
    default : {
    } break;
    }
    if (pool) {
        release_vpu_memory_pool_allocator(pool);
    }
}

status_t deinterlace_dev::perform(VPU_FRAME *frm, uint32_t bypass)
{
    status_t ret = NO_INIT;
    VPUMemLinear_t deInterlaceFrame;
    ret = VPUMallocLinearFromRender(&deInterlaceFrame, frm->FrameHeight*frm->FrameWidth*3/2, (void*)pool);
    if (!ret) {
        uint32_t width    = frm->FrameWidth;
        uint32_t height   = frm->FrameHeight;
        uint32_t srcYAddr = frm->FrameBusAddr[0];
        uint32_t srcCAddr = frm->FrameBusAddr[1];
        uint32_t dstYAddr = deInterlaceFrame.phy_addr;
        uint32_t dstCAddr = deInterlaceFrame.phy_addr + width*height;
        frm->FrameBusAddr[0] = dstYAddr;
        frm->FrameBusAddr[1] = dstCAddr;
        switch (dev_status) {
        case USING_IPP : {
            struct rk29_ipp_req ipp_req;
            memset(&ipp_req,0,sizeof(rk29_ipp_req));
            ipp_req.src0.YrgbMst = srcYAddr;
            ipp_req.src0.CbrMst  = srcCAddr;
            ipp_req.src0.w = frm->DisplayWidth;
            ipp_req.src0.h = frm->DisplayHeight;
            ipp_req.src0.fmt = IPP_Y_CBCR_H2V2;
            ipp_req.dst0.w = frm->DisplayWidth;
            ipp_req.dst0.h = frm->DisplayHeight;
            ipp_req.src_vir_w = width;
            ipp_req.dst_vir_w = width;
            ipp_req.timeout = 100;
            ipp_req.flag = IPP_ROT_0;
            ipp_req.deinterlace_enable = bypass ? 0 : 1;
            ipp_req.deinterlace_para0 = 8;
            ipp_req.deinterlace_para1 = 16;
            ipp_req.deinterlace_para2 = 8;
            ipp_req.dst0.YrgbMst = dstYAddr;
            ipp_req.dst0.CbrMst  = dstCAddr;
            ret = ioctl(dev_fd, IPP_BLIT_ASYNC, &ipp_req);
        } break;
        case USING_IEP : {
            /**
            IEP_FORMAT_ARGB_8888    = 0x0,
            IEP_FORMAT_ABGR_8888    = 0x1,
            IEP_FORMAT_RGBA_8888    = 0x2,
            IEP_FORMAT_BGRA_8888    = 0x3,
            IEP_FORMAT_RGB_565      = 0x4,
            IEP_FORMAT_BGR_565      = 0x5,

            IEP_FORMAT_YCbCr_422_SP = 0x10,
            IEP_FORMAT_YCbCr_422_P  = 0x11,
            IEP_FORMAT_YCbCr_420_SP = 0x12,
            IEP_FORMAT_YCbCr_420_P  = 0x13,
            IEP_FORMAT_YCrCb_422_SP = 0x14,
            IEP_FORMAT_YCrCb_422_P  = 0x15,
            IEP_FORMAT_YCrCb_420_SP = 0x16,
            IEP_FORMAT_YCrCb_420_P  = 0x17
            */

            ops.init_discrete(api, width, height, 0, 0, width, height, 0x12, srcYAddr, srcCAddr, 0,
                                  width, height, 0, 0, width, height, 0x12, dstYAddr, dstCAddr, 0);

            if (!bypass) {
                if (0 > ops.config_yuv_deinterlace(api)) {
                    ALOGE("Failure to Configure YUV DEINTERLACE\n");
                }
            }

            ops.run_async_ncb(api);

        } break;
        default : {
            ret = BAD_VALUE;
        } break;
        }

        if (!ret) {
            VPUMemDuplicate(&frm->vpumem,&deInterlaceFrame);
        } else {
            ALOGE("ioctl: IPP_BLIT_ASYNC faild!");
        }
        VPUFreeLinear(&deInterlaceFrame);
    }
    return ret;
}

status_t deinterlace_dev::sync()
{
    status_t ret = NO_INIT;
    switch (dev_status) {
    case USING_IPP : {
        int result;
        struct pollfd fd;
        fd.fd = dev_fd;
        fd.events = POLLIN;
        if (poll(&fd, 1, -1) > 0) {
            ret = ioctl(dev_fd, IPP_GET_RESULT, &result);
            if (ret) {
                ALOGE("ioctl:IPP_GET_RESULT faild!");
            }
        } else {
            ALOGE("IPP poll faild!");
            ret = -ETIMEDOUT;
        }
    } break;
    case USING_IEP : {
        ret = ops.poll(api);
        if (ret != 0) {
            ALOGD("iep poll failure, return %d\n", ret);
        }
        ret = 0;
    } break;

    default : {
    } break;
    }
    return ret;
}

status_t deinterlace_dev::status()
{
    return dev_status;
}

status_t deinterlace_dev::test()
{
    status_t ret = NO_INIT;
    switch (dev_status) {
    case USING_IPP : {
        struct rk29_ipp_req ipp_req;
        memset(&ipp_req, 0, sizeof(rk29_ipp_req));
        ipp_req.deinterlace_enable =2;
        ALOGI("test ipp is support or not");
        ret = ioctl(dev_fd, IPP_BLIT_ASYNC, &ipp_req);
        if (ret) {
            close(dev_fd);
            dev_status = USING_NULL;
        } else {
            ALOGI("test ipp ok");
        }
    } break;
    case USING_IEP: {
        ret = OK;
    } break;
    case USING_PP : {
        //VPUClientRelease(dev_fd);
        ret = OK;
    } break;
    default : {
    } break;
    }
    return ret;
}

}
