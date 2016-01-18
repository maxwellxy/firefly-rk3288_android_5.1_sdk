#include "CameraHal.h"
namespace android{

#define DISPLAY_FORMAT CAMERA_DISPLAY_FORMAT_YUV420SP/*CAMERA_DISPLAY_FORMAT_YUV420P*/

DisplayAdapter::DisplayAdapter()
              :displayThreadCommandQ("displayCmdQ")
{
    LOG_FUNCTION_NAME
//	strcpy(mDisplayFormat,CAMERA_DISPLAY_FORMAT_YUV420SP/*CAMERA_DISPLAY_FORMAT_YUV420SP*/);
    strcpy(mDisplayFormat,DISPLAY_FORMAT);
    mFrameProvider =  NULL;
    mDisplayRuning = -1;
    mDislayBufNum = 0;
    mDisplayWidth = 0;
    mDisplayHeight = 0;
    mDispBufUndqueueMin = 0;
    mANativeWindow = NULL;
    mDisplayBufInfo =NULL;
    mDisplayState = 0;
    //create display thread

    mDisplayThread = new DisplayThread(this);
    mDisplayThread->run("DisplayThread",ANDROID_PRIORITY_DISPLAY);
    LOG_FUNCTION_NAME_EXIT
}
DisplayAdapter::~DisplayAdapter()
{
    LOG_FUNCTION_NAME

    if(mDisplayThread != NULL){
        //stop thread and exit
        if(mDisplayRuning != STA_DISPLAY_STOP)
            stopDisplay();
        mDisplayThread->requestExitAndWait();
        mDisplayThread.clear();
    }
    LOG_FUNCTION_NAME_EXIT
}
void DisplayAdapter::dump()
{

}

void DisplayAdapter::setDisplayState(int state)
{
	mDisplayState = state;
}

bool DisplayAdapter::isNeedSendToDisplay()
{
    Mutex::Autolock lock(mDisplayLock);

    if((mDisplayRuning == STA_DISPLAY_PAUSE) || (mDisplayRuning == STA_DISPLAY_STOP)
        ||(mDisplayState == CMD_DISPLAY_PAUSE_PREPARE)
        ||(mDisplayState == CMD_DISPLAY_PAUSE_DONE)
        ||(mDisplayState == CMD_DISPLAY_STOP_PREPARE)
        ||(mDisplayState == CMD_DISPLAY_STOP_DONE))
        return false;
    else{
        LOG2("need to display this frame");
        return true;
    }
}
void DisplayAdapter::notifyNewFrame(FramInfo_s* frame)
{
    mDisplayLock.lock();
    //send a frame to display
    if((mDisplayRuning == STA_DISPLAY_RUNNING)  
        &&(mDisplayState != CMD_DISPLAY_PAUSE_PREPARE)
        &&(mDisplayState != CMD_DISPLAY_PAUSE_DONE)
        &&(mDisplayState != CMD_DISPLAY_STOP_PREPARE)
        &&(mDisplayState != CMD_DISPLAY_STOP_DONE))
   {
        Message_cam msg;
        msg.command = CMD_DISPLAY_FRAME;
        msg.arg1 = NULL;
        msg.arg2 = (void*)frame;
        msg.arg3 = (void*)(frame->used_flag);
        displayThreadCommandQ.put(&msg);
        mDisplayCond.signal();
    }else{
    //must return frame if failed to send display
        if(mFrameProvider)
            mFrameProvider->returnFrame(frame->frame_index,frame->used_flag);
    }
    mDisplayLock.unlock();
}


int DisplayAdapter::startDisplay(int width, int height)
{
    int err = NO_ERROR;
    Message_cam msg;
    Semaphore sem;
    LOG_FUNCTION_NAME
    mDisplayLock.lock();
    #if 0
        if (mDisplayRuning == STA_DISPLAY_RUNNING) {
            LOGD("%s(%d): display thread is already run",__FUNCTION__,__LINE__);
            goto cameraDisplayThreadStart_end;
        }
    #endif
    mDisplayWidth = width;
    mDisplayHeight = height;
    setDisplayState(CMD_DISPLAY_START_PREPARE);
    msg.command = CMD_DISPLAY_START;
    sem.Create();
    msg.arg1 = (void*)(&sem);
    displayThreadCommandQ.put(&msg);    
    mDisplayCond.signal();
cameraDisplayThreadStart_end:
    mDisplayLock.unlock(); 
    if(msg.arg1){
        sem.Wait();
		if(mDisplayState != CMD_DISPLAY_START_DONE)
			err = -1;
    }
    LOG_FUNCTION_NAME_EXIT
    return err;
}
//exit display
int DisplayAdapter::stopDisplay()
{
    int err = NO_ERROR;
    Message_cam msg;
    Semaphore sem;
    LOG_FUNCTION_NAME
    mDisplayLock.lock();
    if (mDisplayRuning == STA_DISPLAY_STOP) {
        LOGD("%s(%d): display thread is already pause",__FUNCTION__,__LINE__);
        goto cameraDisplayThreadPause_end;
    }
    setDisplayState(CMD_DISPLAY_STOP_PREPARE);
    msg.command = CMD_DISPLAY_STOP ;
    sem.Create();
    msg.arg1 = (void*)(&sem);
    displayThreadCommandQ.put(&msg);
    mDisplayCond.signal();
cameraDisplayThreadPause_end:
    mDisplayLock.unlock(); 
    if(msg.arg1){
        sem.Wait();
		if(mDisplayState != CMD_DISPLAY_STOP_DONE)
			err = -1;		
    }
    LOG_FUNCTION_NAME_EXIT
    return err;

}
int DisplayAdapter::pauseDisplay()
{
    int err = NO_ERROR;
    Message_cam msg;
    Semaphore sem;
    mDisplayLock.lock();
    LOG_FUNCTION_NAME
    if (mDisplayRuning == STA_DISPLAY_PAUSE) {
        LOGD("%s(%d): display thread is already stop",__FUNCTION__,__LINE__);
        goto cameraDisplayThreadStop_end;
    }
    setDisplayState(CMD_DISPLAY_PAUSE_PREPARE);
    msg.command = CMD_DISPLAY_PAUSE;
    sem.Create();
    msg.arg1 = (void*)(&sem);
    displayThreadCommandQ.put(&msg);
	mDisplayCond.signal();
cameraDisplayThreadStop_end:
    mDisplayLock.unlock(); 
    if(msg.arg1){
        sem.Wait();
		if(mDisplayState != CMD_DISPLAY_PAUSE_DONE)
			err = -1;		
    }
    LOG_FUNCTION_NAME_EXIT
    return err;
}

int DisplayAdapter::setPreviewWindow(struct preview_stream_ops* window)
{
    //mDisplayRuning status
    //mANativeWindow null?
    //window null ?
    LOG_FUNCTION_NAME
    if(window == mANativeWindow){
        return 0;
    }
    if(mANativeWindow){
        pauseDisplay();
    }
    mANativeWindow = window;
    LOG_FUNCTION_NAME_EXIT
    return 0;
}

int DisplayAdapter::getDisplayStatus(void)
{
    Mutex::Autolock lock(mDisplayLock);

    return mDisplayRuning;
}

void DisplayAdapter::setFrameProvider(FrameProvider* framePro)
{
    mFrameProvider = framePro;
}


struct preview_stream_ops* DisplayAdapter::getPreviewWindow()
{
    return mANativeWindow;
}

//internal func
int DisplayAdapter::cameraDisplayBufferCreate(int width, int height, const char *fmt,int numBufs)
{
    int err = NO_ERROR,undequeued = 0;
    int i, total=0;
    buffer_handle_t* hnd = NULL;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    Rect bounds;  
    int stride; 
    
    LOG_FUNCTION_NAME
    if(!mANativeWindow){
        LOGE("%s(%d): nativewindow is null!",__FUNCTION__,__LINE__);
    }
    if(mDisplayBufInfo){
        //destroy buffer first
        cameraDisplayBufferDestory();
    }
    mDislayBufNum = CONFIG_CAMERA_DISPLAY_BUF_CNT;

    // Set gralloc usage bits for window.
    err = mANativeWindow->set_usage(mANativeWindow, CAMHAL_GRALLOC_USAGE);
    if (err != 0) {
        LOGE("%s(%d): %s(err:%d) native_window_set_usage failed", __FUNCTION__,__LINE__, strerror(-err), -err);

        if ( ENODEV == err ) {
            LOGE("%s(%d): Preview surface abandoned !",__FUNCTION__,__LINE__);
            mANativeWindow = NULL;
        }

        goto fail;
    }
    /* ddl@rock-chips.com: NativeWindow switch to async mode after v0.1.3 */
    /*
    if (mANativeWindow->set_swap_interval(mANativeWindow, 1) != 0) {
        LOGE("%s(%d): set mANativeWindow run in synchronous mode failed",__FUNCTION__,__LINE__);
    }
    */
    err = mANativeWindow->get_min_undequeued_buffer_count(mANativeWindow, &undequeued);
    if (err != 0) {

        if ( -ENODEV == err ) {
            mANativeWindow = NULL;
        }
        goto fail;
    }
    mDispBufUndqueueMin = undequeued;
    ///Set the number of buffers needed for camera preview
    
    //total = numBufs+undequeued;
    total = numBufs;
    LOG1("%s(%d): min_undequeued:0x%x total:0x%x",__FUNCTION__,__LINE__, undequeued, total);
    err = mANativeWindow->set_buffer_count(mANativeWindow, total);
    if (err != 0) {
        LOGE("%s(%d): %s(err:%d) native_window_set_buffer_count(%d+%d) failed", __FUNCTION__,__LINE__,strerror(-err), -err,numBufs,undequeued);

        if ( -ENODEV == err ) {
            LOGE("%s(%d): Preview surface abandoned !",__FUNCTION__,__LINE__);
            mANativeWindow = NULL;
        }
        goto fail;
    }
    

    // Set window geometry
    err = mANativeWindow->set_buffers_geometry(
            mANativeWindow,
            width,
            height,
            cameraPixFmt2HalPixFmt(fmt)); 
            

    if (err != 0) {
        LOGE("%s(%d): %s(err:%d) native_window_set_buffers_geometry failed", __FUNCTION__,__LINE__, strerror(-err), -err);

        if ( ENODEV == err ) {
            LOGE("%s(%d): Preview surface abandoned !",__FUNCTION__,__LINE__);
            mANativeWindow = NULL;
        }

        goto fail;
    }    
    mDisplayBufInfo = (rk_displaybuf_info_t*)malloc(sizeof(rk_displaybuf_info_t)*total);
    if(!mDisplayBufInfo){
        LOGE("%s(%d): malloc diaplay buffer structue failed!",__FUNCTION__,__LINE__);
        err = -1;
        goto fail;
    }
    for ( i=0; i < total; i++ ) {         
        err = mANativeWindow->dequeue_buffer(mANativeWindow, (buffer_handle_t**)&hnd, &stride);

        if (err != 0) {
            LOGE("%s(%d): %s(err:%d) dequeueBuffer failed", __FUNCTION__,__LINE__, strerror(-err), -err);

            if ( ENODEV == err ) {
                LOGE("%s(%d): Preview surface abandoned !",__FUNCTION__,__LINE__);
                mANativeWindow = NULL;
            }

            goto fail;
        }
        mDisplayBufInfo[i].lock = new Mutex();
        mDisplayBufInfo[i].buffer_hnd = hnd;
        mDisplayBufInfo[i].priv_hnd= (NATIVE_HANDLE_TYPE*)(*hnd);
        mDisplayBufInfo[i].stride = stride;
    #if defined(TARGET_RK29) 
        struct pmem_region sub;
    
        if (ioctl(mDisplayBufInfo[i].priv_hnd->fd,PMEM_GET_PHYS,&sub) == 0) {                    
            mDisplayBufInfo[i].phy_addr = sub.offset + mDisplayBufInfo[i].priv_hnd->offset;    /* phy address */ 
        } else {   
            /* ddl@rock-chips.com: gralloc buffer is not continuous in phy */
            mDisplayBufInfo[i].phy_addr = 0x00;
        }        
    #elif defined(TARGET_RK3188)
		mDisplayBufInfo[i].phy_addr = mDisplayBufInfo[i].priv_hnd->phy_addr;
	#else
		mDisplayBufInfo[i].phy_addr = 0x00;  
	#endif
        
    }
    // lock the initial queueable buffers
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = width;
    bounds.bottom = height;
    for( i = 0;  i < total; i++ ) {
        void* y_uv[3];
        
        mANativeWindow->lock_buffer(mANativeWindow, (buffer_handle_t*)mDisplayBufInfo[i].buffer_hnd);
        mapper.lock((buffer_handle_t)mDisplayBufInfo[i].priv_hnd, CAMHAL_GRALLOC_USAGE, bounds, y_uv);
        #if defined(TARGET_BOARD_PLATFORM_RK30XX) || defined(TARGET_RK29) || defined(TARGET_BOARD_PLATFORM_RK2928)
            mDisplayBufInfo[i].vir_addr = (long)mDisplayBufInfo[i].priv_hnd->base;
        #elif defined(TARGET_BOARD_PLATFORM_RK30XXB) || defined(TARGET_RK3368)
            mDisplayBufInfo[i].vir_addr = (long)y_uv[0];
        #endif
        setBufferState(i,0);
        LOG1("%s(%d): mGrallocBufferMap[%d] phy_addr: 0x%x  vir_dir: 0x%x",
            __FUNCTION__,__LINE__, i, mDisplayBufInfo[i].phy_addr,mDisplayBufInfo[i].vir_addr);
    }

    LOG_FUNCTION_NAME_EXIT    
    return err; 
 fail:
        for (i = 0; i<total; i++) {
            if (mDisplayBufInfo && mDisplayBufInfo[i].buffer_hnd) {
                err = mANativeWindow->cancel_buffer(mANativeWindow, (buffer_handle_t*)mDisplayBufInfo[i].buffer_hnd);
                if (err != 0) {
                  LOGE("%s(%d): cancelBuffer failed w/ error 0x%08x",__FUNCTION__,__LINE__, err);                  
                }
            }
        }
    
    LOGE("%s(%d): exit with error(%d)!",__FUNCTION__,__LINE__,err);
    return err;
}

int DisplayAdapter::cameraDisplayBufferDestory(void)
{
    int ret = NO_ERROR,i;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    LOG_FUNCTION_NAME
    //Give the buffers back to display here -  sort of free it
    if (mANativeWindow && mDisplayBufInfo) {
        for(i = 0; i < mDislayBufNum; i++) {
            // unlock buffer before giving it up
            if (mDisplayBufInfo[i].priv_hnd && (mDisplayBufInfo[i].buf_state == 0) ) {
                mapper.unlock((buffer_handle_t)mDisplayBufInfo[i].priv_hnd);
                mANativeWindow->cancel_buffer(mANativeWindow, (buffer_handle_t*)mDisplayBufInfo[i].buffer_hnd);
            }
            mDisplayBufInfo[i].buffer_hnd = NULL;
            mDisplayBufInfo[i].priv_hnd = NULL;
            delete mDisplayBufInfo[i].lock;
        }
        if(mDisplayBufInfo){
            free(mDisplayBufInfo);
            mDisplayBufInfo = NULL;
            mDislayBufNum = 0;
            //mANativeWindow = NULL;//video may lock
        }
    } else {
        LOGD("%s(%d): mANativeWindow is NULL, destory is ignore",__FUNCTION__,__LINE__);
    }

    LOG_FUNCTION_NAME_EXIT
cameraDisplayBufferDestory_end:
    return ret;    
}
void DisplayAdapter::setBufferState(int index,int status)
{
    rk_displaybuf_info_t* buf_hnd = NULL;
    if(mDisplayBufInfo){
        buf_hnd = mDisplayBufInfo+index;
        buf_hnd->buf_state = status;
    }else{
        LOGE("%s(%d):display buffer is null.",__FUNCTION__,__LINE__);
    }
}

extern "C" void arm_yuyv_to_nv12(int src_w, int src_h,char *srcbuf, char *dstbuf){

    char *srcbuf_begin;
    int *dstint_y, *dstint_uv, *srcint;
    int i = 0,j = 0;
    int y_size = 0;

	y_size = src_w*src_h;
	dstint_y = (int*)dstbuf;
	srcint = (int*)srcbuf;
	dstint_uv =  (int*)(dstbuf + y_size);
	//LOGE("-----------%s----------------zyh",__FUNCTION__);
	/*
	 * author :zyh
	 * neon code for YUYV to NV12
	 */
#if HAVE_ARM_NEON
	for(i=0;i<src_h;i++) {
         int n = src_w;
		 char tmp = i%2;//get uv only when in even row
		 asm volatile (
			"   pld [%[src], %[src_stride], lsl #2]                         \n\t"
			"   cmp %[n], #16                                               \n\t"
			"   blt 5f                                                      \n\t"
			"0: @ 16 pixel swap                                             \n\t"
			"   vld2.8  {q0,q1} , [%[src]]!  @ q0 = y q1 = uv               \n\t"
			"   vst1.16 {q0},[%[dst_y]]!     @ now q0  -> dst               \n\t"
			"   cmp %[tmp], #1                                              \n\t"
			"   bge 1f                                                      \n\t"
			"   vst1.16 {q1},[%[dst_uv]]!    @ now q1  -> dst   	    	\n\t"
			"1: @ don't need get uv in odd row                              \n\t"
			"   sub %[n], %[n], #16                                         \n\t"
			"   cmp %[n], #16                                               \n\t"
			"   bge 0b                                                      \n\t"
			"5: @ end                                                       \n\t"
			: [dst_y] "+r" (dstint_y), [dst_uv] "+r" (dstint_uv),[src] "+r" (srcint), [n] "+r" (n),[tmp] "+r" (tmp)
			: [src_stride] "r" (src_w)
			: "cc", "memory", "q0", "q1", "q2"
			);
	 }
	 //LOGE("---------------neon code arm_yuyv_to_nv12-----------------------------");
	 /*
	  * C code YUYV to YUV420
	  */
#else
    for(i=0;i<src_h; i++) {
		for (j=0; j<(src_w>>2); j++) {
			if(i%2 == 0){
			    *dstint_uv++ = (*(srcint+1)&0xff000000)|((*(srcint+1)&0x0000ff00)<<8)
				        |((*srcint&0xff000000)>>16)|((*srcint&0x0000ff00)>>8);
			 }
			 *dstint_y++ = ((*(srcint+1)&0x00ff0000)<<8)|((*(srcint+1)&0x000000ff)<<16)
			            |((*srcint&0x00ff0000)>>8)|(*srcint&0x000000ff);
		     srcint += 2;
		 }
	 }
	 //LOGE("---------------c code arm_yuyv_to_nv12-----------------------------");
#endif
}

extern "C" void arm_yuyv_to_yv12(int src_w, int src_h,char *srcbuf, char *dstbuf){

    char *srcbuf_begin;
    int *dst_y, *dst_uv, *src;
    short int *dst_u, *dst_v;
    int i = 0,j = 0;
    int y_size = 0;
    y_size = src_w*src_h;
    dst_y  = (int*)dstbuf;
    src = (int*)srcbuf;
    dst_u  = (short int*)(dstbuf + y_size);
    dst_v  = (short int*)((char*)dst_u + (y_size >> 2));
	//LOGE("-----------%s----------------zyh",__FUNCTION__);
	/*
	 * author :zyh
	 * neon code for YUYV to YV12
	 */
#if HAVE_ARM_NEON
	for(i=0;i<src_h;i++) {
        int n = src_w;
        char tmp = i%2;//get uv only when in even row
		asm volatile (
				"   pld [%[src], %[src_stride], lsl #2]                         \n\t"
				"   cmp %[n], #16                                               \n\t"
				"   blt 5f                                                      \n\t"
				"0: @ 16 pixel swap                                             \n\t"
				"   vld2.8  {q0,q1} , [%[src]]!  @ q0 = y q1 = uv               \n\t"
				"   vuzp.8 q1, q2                @ d1 = u d5 = v                \n\t"
				"   vst1.16 {q0},[%[dst_y]]!     @ now q0   -> dst              \n\t"
				"   cmp %[tmp], #1                                              \n\t"
				"   bge 1f                                                      \n\t"
				"   vst1.8  {d4},[%[dst_u]]!     @ now d0  -> dst   	    	\n\t"
				"   vst1.8  {d2},[%[dst_v]]!     @ now d1  -> dst   	    	\n\t"
				"1: @ don't need get uv in odd row                              \n\t"
				"   sub %[n], %[n], #16                                         \n\t"
				"   cmp %[n], #16                                               \n\t"
				"   bge 0b                                                      \n\t"
				"5: @ end                                                       \n\t"
				: [dst_y] "+r" (dst_y), [dst_u] "+r" (dst_u),[dst_v] "+r" (dst_v),[src] "+r" (src), [n] "+r" (n),[tmp] "+r" (tmp)
				: [src_stride] "r" (src_w)
				: "cc", "memory", "q0", "q1", "q2"
				);
	}
    //LOGE("---------------neon code arm_yuyv_to_yv12-----------------------------");
	/*
	 * C code YUYV to YUV420
	 */
#else
	for(i=0;i<src_h; i++) {
        for (j=0; j<(src_w>>2); j++) {
             if(i%2 == 0){
                 *dst_v++ = (((*src&0x0000ff00)>>8) | ((*(src+1)&0x0000ff00)));
                 *dst_u++ = (((*src&0xff000000)>>24) | ((*(src+1)&0xff000000)>>16));
             }
             *dst_y++ = ((*(src+1)&0x00ff0000)<<8)|((*(src+1)&0x000000ff)<<16)
                                    |((*src&0x00ff0000)>>8)|(*src&0x000000ff);
             src += 2;
        }
	 }
    //LOGE("---------------c code arm_yuyv_to_yv12-----------------------------");
#endif
}
//for soc camera test
extern "C" void arm_yuyv_to_nv12_soc_ex(int src_w, int src_h,char *srcbuf, char *dstbuf){
    char *srcbuf_begin;
    char  *srcint;
    char *dstint_y; 
    char *dstint_uv;
    int i = 0,j = 0;
    int y_size = 0;


    y_size = src_w*src_h;
    dstint_y = (char*)dstbuf;
    srcint = (char*)srcbuf;
    for(i=0;i<(y_size);i++) {

       *dstint_y++ = ((*(srcint+1)&0x3f) << 2) | ((*(srcint+0)>> 6) & 0x03);

       srcint += 4;

    }
    #if 1
    dstint_uv =  (char*)(dstbuf + y_size);
    srcint = (char*)srcbuf;
    for(i=0;i<src_h/2; i++) {
        for (j=0; j<(src_w >> 1 ); j++) {
			*dstint_uv++ = (((*(srcint+3) &0x3f ) << 2) | ((*(srcint+2) >> 6 ) & 0x03 ));
			*dstint_uv++ = (((*(srcint+7) &0x3f ) << 2) | ((*(srcint+6) >> 6 ) & 0x03 ));
            srcint += 8;
        }
        srcint += (src_w<<2);
    }
    #endif

}

void DisplayAdapter::displayThread()
{
    int err,stride,i,queue_cnt;
    long dequeue_buf_index,queue_buf_index,queue_display_index;
    buffer_handle_t *hnd = NULL; 
    NATIVE_HANDLE_TYPE *phnd;
    GraphicBufferMapper& mapper = GraphicBufferMapper::get();
    Message_cam msg;
    void *y_uv[3];
    long frame_used_flag = -1;
    Rect bounds;
    
    LOG_FUNCTION_NAME    
    while (mDisplayRuning != STA_DISPLAY_STOP) {
display_receive_cmd:        
        if (displayThreadCommandQ.isEmpty() == false ) {
            displayThreadCommandQ.get(&msg);         

            switch (msg.command)
            {
                case CMD_DISPLAY_START:
                {
                    LOGD("%s(%d): receive CMD_DISPLAY_START", __FUNCTION__,__LINE__);
                    cameraDisplayBufferDestory(); 
                    err = cameraDisplayBufferCreate(mDisplayWidth, mDisplayHeight,mDisplayFormat,CONFIG_CAMERA_DISPLAY_BUF_CNT);
					if (err == 0){	
                    mDisplayRuning = STA_DISPLAY_RUNNING;
					setDisplayState(CMD_DISPLAY_START_DONE);
					}
                    if(msg.arg1)
                        ((Semaphore*)msg.arg1)->Signal();
                    break;
                }

                case CMD_DISPLAY_PAUSE:
                {
                    LOGD("%s(%d): receive CMD_DISPLAY_PAUSE", __FUNCTION__,__LINE__);

                    cameraDisplayBufferDestory();
                    mDisplayRuning = STA_DISPLAY_PAUSE;
					setDisplayState(CMD_DISPLAY_PAUSE_DONE);
                    if(msg.arg1)
                        ((Semaphore*)msg.arg1)->Signal();
                    break;
                }
                
                case CMD_DISPLAY_STOP:
                {
                    LOGD("%s(%d): receive CMD_DISPLAY_STOP", __FUNCTION__,__LINE__);
					cameraDisplayBufferDestory();
                    mDisplayRuning = STA_DISPLAY_STOP;
					setDisplayState(CMD_DISPLAY_STOP_DONE);
                    if(msg.arg1)
                        ((Semaphore*)msg.arg1)->Signal();
                    continue;
                }

                case CMD_DISPLAY_FRAME:
                {  
                    if(msg.arg1)
                        ((Semaphore*)msg.arg1)->Signal();

                    if (mDisplayRuning != STA_DISPLAY_RUNNING) 
                        goto display_receive_cmd;
                        
                   
                    if (mANativeWindow == NULL) {
                        LOGE("%s(%d): thread exit, because mANativeWindow is NULL", __FUNCTION__,__LINE__);
                        mDisplayRuning = STA_DISPLAY_STOP;  
                        continue;
                    }

                    FramInfo_s* frame = (FramInfo_s*)msg.arg2;
                    frame_used_flag = (long)msg.arg3;


                    
                    queue_buf_index = (long)msg.arg1;                    
                    queue_display_index = CONFIG_CAMERA_DISPLAY_BUF_CNT;
                    //get a free buffer                        
                        for (i=0; i<CONFIG_CAMERA_DISPLAY_BUF_CNT; i++) {
                            if (mDisplayBufInfo[i].buf_state == 0) 
                                break;
                        }
                        if (i<CONFIG_CAMERA_DISPLAY_BUF_CNT) {
                            queue_display_index = i;
                        } else {
                            err = 0x01;
                            while (err != 0) {
                                err = mANativeWindow->dequeue_buffer(mANativeWindow, (buffer_handle_t**)&hnd, &stride);
                                if (err == 0) {
                                    // lock the initial queueable buffers
                                    bounds.left = 0;
                                    bounds.top = 0;
                                    bounds.right = mDisplayWidth;
                                    bounds.bottom = mDisplayHeight;
                                    mANativeWindow->lock_buffer(mANativeWindow, (buffer_handle_t*)hnd);
                                    mapper.lock((buffer_handle_t)(*hnd), CAMHAL_GRALLOC_USAGE, bounds, y_uv);

                                    phnd = (NATIVE_HANDLE_TYPE*)*hnd;
                                    for (i=0; i<CONFIG_CAMERA_DISPLAY_BUF_CNT; i++) {
                                        if (phnd == mDisplayBufInfo[i].priv_hnd) {  
                                            queue_display_index = i;
                                            break;
                                        }
                                    }
                                    if(i == CONFIG_CAMERA_DISPLAY_BUF_CNT){
                                        err = mANativeWindow->cancel_buffer(mANativeWindow, (buffer_handle_t*)hnd);

                                        //receive another msg
                                         continue;
                                    }
                                    //set buffer status,dequed,but unused
                                    setBufferState(queue_display_index, 0);
                                } else {
                                    LOGD("%s(%d): %s(err:%d) dequeueBuffer failed, so pause here", __FUNCTION__,__LINE__, strerror(-err), -err);

                                    mDisplayLock.lock();
                                    if (displayThreadCommandQ.isEmpty() == false ) {
                                        //return this frame to frame provider
                                        if(mFrameProvider)
                                            mFrameProvider->returnFrame(frame->frame_index,frame_used_flag);
                                        mDisplayLock.unlock(); 
                                        goto display_receive_cmd;
                                    }
						                                    
                                    mDisplayCond.wait(mDisplayLock); 
                                    mDisplayLock.unlock();
                                    LOG2("%s(%d): wake up...", __FUNCTION__,__LINE__);
                                }
                            }
                        } 
                        //fill display buffer
                   
#if 1
                 //   rga_nv12torgb565(frame->frame_width, frame->frame_height, 
                  //                  (char*)(frame->vir_addr), (short int*)mDisplayBufInfo[queue_display_index].vir_addr, 
                   //                 mDisplayWidth,mDisplayWidth,mDisplayHeight);

            //        if((frame->frame_fmt == V4L2_PIX_FMT_YUYV) /*&& (strcmp((mDisplayFormat),CAMERA_DISPLAY_FORMAT_YUV420SP)==0)*/)
                    if((frame->frame_fmt == V4L2_PIX_FMT_YUYV) && (strcmp((mDisplayFormat),CAMERA_DISPLAY_FORMAT_YUV420P)==0))
                    {
                        if((frame->frame_width == mDisplayWidth) && (frame->frame_height== mDisplayHeight))
						arm_yuyv_to_yv12(frame->frame_width, frame->frame_height,
                         (char*)(frame->vir_addr), (char*)mDisplayBufInfo[queue_display_index].vir_addr);
					}else if((frame->frame_fmt == V4L2_PIX_FMT_YUYV) && (strcmp((mDisplayFormat),CAMERA_DISPLAY_FORMAT_YUV420SP)==0))
                    {
                        if((frame->frame_width == mDisplayWidth) && (frame->frame_height== mDisplayHeight))
						arm_yuyv_to_nv12(frame->frame_width, frame->frame_height,
                         (char*)(frame->vir_addr), (char*)mDisplayBufInfo[queue_display_index].vir_addr);
                        //LOGD("display got a frame");
                    }
                    else if((frame->frame_fmt == V4L2_PIX_FMT_NV12) && (strcmp((mDisplayFormat),CAMERA_DISPLAY_FORMAT_RGB565)==0))
                    {
                       arm_nv12torgb565(frame->frame_width, frame->frame_height,
                						(char*)(frame->vir_addr), (short int*)mDisplayBufInfo[queue_display_index].vir_addr,
                                         mDisplayWidth);
                    }else if((frame->frame_fmt == V4L2_PIX_FMT_NV12) && (strcmp((mDisplayFormat),CAMERA_DISPLAY_FORMAT_YUV420SP)==0)){
                    #if 0
                        arm_camera_yuv420_scale_arm(V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV12, 
							(char*)(frame->vir_addr), (char*)mDisplayBufInfo[queue_display_index].vir_addr,
							frame->frame_width, frame->frame_height,
							mDisplayWidth, mDisplayHeight,
							false,frame->zoom_value);
                    #else
						#if defined(TARGET_RK3188)
							rk_camera_zoom_ipp(V4L2_PIX_FMT_NV12, (int)(frame->phy_addr), frame->frame_width, frame->frame_height,(int)(mDisplayBufInfo[queue_display_index].phy_addr),frame->zoom_value);
						#else
                        	rga_nv12_scale_crop(frame->frame_width, frame->frame_height, 
                                            (char*)(frame->vir_addr), (short int *)(mDisplayBufInfo[queue_display_index].vir_addr), 
                                            mDisplayWidth,mDisplayHeight,frame->zoom_value,true,true,false);
						#endif

                    #endif
                    }
#endif
                    setBufferState(queue_display_index, 1);
                    mapper.unlock((buffer_handle_t)mDisplayBufInfo[queue_display_index].priv_hnd);
                    err = mANativeWindow->enqueue_buffer(mANativeWindow, (buffer_handle_t*)mDisplayBufInfo[queue_display_index].buffer_hnd);                    
                    if (err != 0){
                                                
                        bounds.left = 0;
                        bounds.top = 0;
                        bounds.right = mDisplayWidth ;
                        bounds.bottom = mDisplayHeight;
                        mANativeWindow->lock_buffer(mANativeWindow, (buffer_handle_t*)mDisplayBufInfo[queue_display_index].buffer_hnd);
                        mapper.lock((buffer_handle_t)(mDisplayBufInfo[queue_display_index].priv_hnd), CAMHAL_GRALLOC_USAGE, bounds, y_uv);

                        mDisplayRuning = STA_DISPLAY_PAUSE;
                        LOGE("%s(%d): enqueue buffer %d to mANativeWindow failed(%d),so display pause", __FUNCTION__,__LINE__,queue_display_index,err);
                    }                

                    //return this frame to frame provider
                    if(mFrameProvider)
                        mFrameProvider->returnFrame(frame->frame_index,frame_used_flag);
                    
                    //deque a display buffer

                        queue_cnt = 0;
                        for (i=0; i<mDislayBufNum; i++) {
                            if (mDisplayBufInfo[i].buf_state == 1) 
                                queue_cnt++;
                        }
						
                        if (queue_cnt > mDispBufUndqueueMin) {
							err = mANativeWindow->dequeue_buffer(mANativeWindow, (buffer_handle_t**)&hnd, &stride);
							if (err == 0) {                                    
                                // lock the initial queueable buffers
                                bounds.left = 0;
                                bounds.top = 0;
                                bounds.right = mDisplayWidth;
                                bounds.bottom = mDisplayHeight;
                                mANativeWindow->lock_buffer(mANativeWindow, (buffer_handle_t*)hnd);
                                mapper.lock((buffer_handle_t)(*hnd), CAMHAL_GRALLOC_USAGE, bounds, y_uv);

                                phnd = (NATIVE_HANDLE_TYPE*)*hnd;
                                for (i=0; i<mDislayBufNum; i++) {
                                    if (phnd == mDisplayBufInfo[i].priv_hnd) {
                                        dequeue_buf_index = i;
                                        break;
                                    }
                                }
                                
                                if (i >= mDislayBufNum) {                    
                                    LOGE("%s(%d): dequeue buffer(0x%x ) don't find in mDisplayBufferMap", __FUNCTION__,__LINE__,(long)phnd);                    
                                    continue;
                                } else {
                                    setBufferState(dequeue_buf_index, 0);
                                }
                                
                            } else {
                                /* ddl@rock-chips.com: dequeueBuffer isn't block, when ANativeWindow in asynchronous mode */
                                LOG2("%s(%d): %s(err:%d) dequeueBuffer failed, so pause here", __FUNCTION__,__LINE__, strerror(-err), -err);
                                mDisplayLock.lock();
                                if (displayThreadCommandQ.isEmpty() == false ) {
                                    mDisplayLock.unlock(); 
                                    goto display_receive_cmd;
                                }                  
                                mDisplayCond.wait(mDisplayLock); 
                                mDisplayLock.unlock();
                                LOG2("%s(%d): wake up...", __FUNCTION__,__LINE__);
                            }
                        }
                    }                    
                    break;
        
                default:
                {
                    LOGE("%s(%d): receive unknow command(0x%x)!", __FUNCTION__,__LINE__,msg.command);
                    break;
                }
            }
        }
        
        mDisplayLock.lock();
        if (displayThreadCommandQ.isEmpty() == false ) {
            mDisplayLock.unlock(); 
            goto display_receive_cmd;
        }        	
        LOG2("%s(%d): display thread pause here... ", __FUNCTION__,__LINE__);
        mDisplayCond.wait(mDisplayLock);  
        mDisplayLock.unlock(); 
        LOG2("%s(%d): display thread wake up... ", __FUNCTION__,__LINE__);
        goto display_receive_cmd;        
        
    }
    LOG_FUNCTION_NAME_EXIT
}

} // namespace android

