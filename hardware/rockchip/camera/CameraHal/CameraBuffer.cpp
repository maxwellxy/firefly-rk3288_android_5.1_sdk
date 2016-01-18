#include "CameraHal.h"
namespace android{
int BufferProvider::getBufferStatus(int bufindex){
    int ret_status;
    mBufInfo[bufindex].lock->lock();
    ret_status = mBufInfo[bufindex].buf_state;
    mBufInfo[bufindex].lock->unlock();
    return ret_status;
    }
int BufferProvider::getBufCount()
{
    return mBufCount;
}
long BufferProvider::getBufPhyAddr(int bufindex)
{
    long phy_addr;
    mBufInfo[bufindex].lock->lock();
    phy_addr = mBufInfo[bufindex].phy_addr;
    mBufInfo[bufindex].lock->unlock();
    return phy_addr;
}
long BufferProvider::getBufVirAddr(int bufindex)
{
    long vir_addr;
    mBufInfo[bufindex].lock->lock();
    vir_addr = mBufInfo[bufindex].vir_addr;
    mBufInfo[bufindex].lock->unlock();
    return vir_addr;
}

int BufferProvider::getBufShareFd(int bufindex)
{
    int share_fd = -1;
    mBufInfo[bufindex].lock->lock();
    share_fd = mBufInfo[bufindex].share_fd;
    mBufInfo[bufindex].lock->unlock();
    return share_fd;
}
int BufferProvider::createBuffer(int count,int perbufsize,buffer_type_enum buftype,bool is_cif_driver)
{
    int ret = 0,i;
    struct bufferinfo_s buf;

	memset(&buf,0,sizeof(struct bufferinfo_s));
    mBufCount = count;
    buf.mNumBffers = count;	
    buf.mPerBuffersize = PAGE_ALIGN(perbufsize);	
    buf.mBufType = (buffer_type_enum)buftype;

    mBufType = (buffer_type_enum)buftype;
    buf.mIsForceIommuBuf = true;

    switch(buftype){
        case PREVIEWBUFFER:
			if(is_cif_driver){//should use cma buffer
				buf.mIsForceIommuBuf = false;
			}
            if(mCamBuffer->createPreviewBuffer(&buf) !=0) {
                LOGE("%s(%d): preview buffer create failed",__FUNCTION__,__LINE__);		
                ret = -1;	
            }
            break;
        case RAWBUFFER:
			#if defined(TARGET_RK3188)//should use cma buffer
				buf.mIsForceIommuBuf = false;
			#endif
            if(mCamBuffer->createRawBuffer(&buf) !=0) {
                LOGE("%s(%d): raw buffer create failed",__FUNCTION__,__LINE__);		
                ret = -1;	
            }
            break;
        case JPEGBUFFER:
            if(mCamBuffer->createJpegBuffer(&buf) !=0) {
                LOGE("%s(%d): jpeg buffer create failed",__FUNCTION__,__LINE__);		
                ret = -1;	
            }
            break;
        case VIDEOENCBUFFER:
            if(mCamBuffer->createVideoEncBuffer(&buf) !=0) {
                LOGE("%s(%d): video buffer create failed",__FUNCTION__,__LINE__);		
                ret = -1;	
            }
            break;
        default :
            ret = -1;


    }
    if(ret == -1) {
        LOGE("%s(%d): buffer create failed",__FUNCTION__,__LINE__);		
        ret = -1;	
        goto createBuffer_end;
    }

    mBufInfo = (rk_buffer_info_t*)malloc(sizeof(rk_buffer_info_t)*count);
    if(!mBufInfo){
        LOGE("%s(%d): buffer create failed",__FUNCTION__,__LINE__);		
        ret = -1;	
        goto createBuffer_end;
    }
    for (i=0; i<count; i++) {
            mBufInfo[i].lock = new Mutex();
            mBufInfo[i].vir_addr = (long)mCamBuffer->getBufferAddr(buftype,i,buffer_addr_vir);
            mBufInfo[i].phy_addr = (long)mCamBuffer->getBufferAddr(buftype,i,buffer_addr_phy);
    		mBufInfo[i].share_fd = (long)mCamBuffer->getBufferAddr(buftype,i,buffer_sharre_fd);
	        mBufInfo[i].buf_state = 0;
    }

createBuffer_end:    
    LOG_FUNCTION_NAME_EXIT
	return ret;
}

int BufferProvider::freeBuffer()
{
    
	LOG_FUNCTION_NAME
     
   	if(mBufInfo != NULL){ 
        for(int i=0; i<mBufCount; i++){
            delete mBufInfo[i].lock;
        }
        free(mBufInfo);
        mBufInfo = NULL;
        mBufCount = 0;
        switch(mBufType){
            case PREVIEWBUFFER:
            	mCamBuffer->destroyPreviewBuffer();
                break;
            case RAWBUFFER:
                mCamBuffer->destroyRawBuffer();
                break;
            case JPEGBUFFER:
                mCamBuffer->destroyJpegBuffer();
                break;
            case VIDEOENCBUFFER:
                mCamBuffer->destroyVideoEncBuffer();
                break;
            default :
                break;

        }
    }
	LOG_FUNCTION_NAME_EXIT
	return 0;
}

int BufferProvider::setBufferStatus(int bufindex,int status,int cmd)
{
    int err = NO_ERROR;   
    rk_buffer_info_t *buf_hnd = NULL;

    if(bufindex >= mBufCount){
        LOGE("%s(%d): Camerahal preview buffer is null, Don't allow set buffer state",__FUNCTION__,__LINE__);
        err = -EINVAL;
        goto setBufferStatus_end;
    }
    if (mBufInfo == NULL) {
        LOGE("%s(%d): buf_hnd is null",__FUNCTION__,__LINE__);
        err = -EINVAL;
        goto setBufferStatus_end;
    }

    buf_hnd = mBufInfo+bufindex;
    
    buf_hnd->lock->lock();

    buf_hnd->buf_state = status;
    buf_hnd->lock->unlock();
setBufferStatus_end:   
    return err;
}

int BufferProvider::getOneAvailableBuffer(long *buf_phy,long *buf_vir)
{
	int i;
	for ( i=0; i < mBufCount; i++) {
			if((mBufInfo[i].buf_state) ==0)
				break;
		}

	if(i == mBufCount)
		return -1;
    else{
        *buf_phy = mBufInfo[i].phy_addr;
        *buf_vir = mBufInfo[i].vir_addr;
    }
	return i;
}

int BufferProvider::flushBuffer(int bufindex)
{
   	if(mBufInfo != NULL){ 
        return mCamBuffer->flushCacheMem(mBufType,0,0);        
    }else{
        return -1;
    }
}

//preview buffer
int PreviewBufferProvider::setBufferStatus(int bufindex,int set,int cmd)
{
    int err = NO_ERROR;   
    rk_buffer_info_t *buf_hnd = NULL;

    if(bufindex >= mBufCount){
        LOGE("%s(%d): Camerahal preview buffer is null, Don't allow set buffer state",__FUNCTION__,__LINE__);
        err = -EINVAL;
        goto setPreviewBufferStatus_end;
    }
    if (mBufInfo == NULL) {
        LOGE("%s(%d): buf_hnd is null",__FUNCTION__,__LINE__);
        err = -EINVAL;
        goto setPreviewBufferStatus_end;
    }

    buf_hnd = mBufInfo+bufindex;
    
    buf_hnd->lock->lock();

    if (cmd & CMD_PREVIEWBUF_DISPING) {
        if (set){
            if (CAMERA_PREVIEWBUF_ALLOW_DISPLAY(buf_hnd->buf_state)==false)
                LOGE("%s(%d): Set buffer displaying, but buffer status(0x%x) is error",__FUNCTION__,__LINE__,buf_hnd->buf_state);
            buf_hnd->buf_state |= CMD_PREVIEWBUF_DISPING;
        } else { 
            buf_hnd->buf_state &= ~CMD_PREVIEWBUF_DISPING;
        }
    }

    if (cmd & CMD_PREVIEWBUF_VIDEO_ENCING) {
        if (set) {
            if (CAMERA_PREVIEWBUF_ALLOW_ENC(buf_hnd->buf_state)==false)
                LOGE("%s(%d): Set buffer encoding,  but buffer status(0x%x) is error",__FUNCTION__,__LINE__,buf_hnd->buf_state);
            buf_hnd->buf_state |= CMD_PREVIEWBUF_VIDEO_ENCING;
        } else {
            buf_hnd->buf_state &= ~CMD_PREVIEWBUF_VIDEO_ENCING;
        }
    }

    if (cmd & CMD_PREVIEWBUF_SNAPSHOT_ENCING) {
        if (set) {
            if (CAMERA_PREVIEWBUF_ALLOW_ENC_PICTURE(buf_hnd->buf_state)==false)
                LOGE("%s(%d): Set buffer snapshot encoding,  but buffer status(0x%x) is error",__FUNCTION__,__LINE__,buf_hnd->buf_state);
            buf_hnd->buf_state |= CMD_PREVIEWBUF_SNAPSHOT_ENCING;
        } else {
            buf_hnd->buf_state &= ~CMD_PREVIEWBUF_SNAPSHOT_ENCING;
        }
    }
    if (cmd & CMD_PREVIEWBUF_DATACB) {
        if (set) {
            if (CAMERA_PREVIEWBUF_ALLOW_DATA_CB(buf_hnd->buf_state)==false)
                LOGE("%s(%d): Set buffer datacb,  but buffer status(0x%x) is error",__FUNCTION__,__LINE__,buf_hnd->buf_state);
            buf_hnd->buf_state |= CMD_PREVIEWBUF_DATACB;
        } else {
            buf_hnd->buf_state &= ~CMD_PREVIEWBUF_DATACB;
        }
    }
    if (cmd & CMD_PREVIEWBUF_WRITING) {
        if (set) {
            if (CAMERA_PREVIEWBUF_ALLOW_WRITE(buf_hnd->buf_state)==false)
                LOGE("%s(%d): Set buffer writing, but buffer status(0x%x) is error",__FUNCTION__,__LINE__,buf_hnd->buf_state);
            buf_hnd->buf_state |= CMD_PREVIEWBUF_WRITING;
        } else { 
            buf_hnd->buf_state &= ~CMD_PREVIEWBUF_WRITING;
        }
    }
    
    buf_hnd->lock->unlock();
  setPreviewBufferStatus_end: 
    return err;
}
}

