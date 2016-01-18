/*
*Author: zyc@rock-chips.co
*/
#include <sys/stat.h>
#include <unistd.h>
#include <utils/CallStack.h>
#include "CameraHal.h"


namespace android {


/******************ION BUFFER START*******************/
MemManagerBase::MemManagerBase()
{
	mPreviewBufferInfo = NULL;
	mRawBufferInfo = NULL;
	mJpegBufferInfo = NULL;
	mJpegBufferInfo = NULL;
}
MemManagerBase::~MemManagerBase()
{
	mPreviewBufferInfo = NULL;
	mRawBufferInfo = NULL;
	mJpegBufferInfo = NULL;
	mJpegBufferInfo = NULL;
}

unsigned int MemManagerBase::getBufferAddr(enum buffer_type_enum buf_type, unsigned int buf_idx, buffer_addr_t addr_type)
{
    unsigned long addr = 0x00;
    struct bufferinfo_s *buf_info;
   
    switch(buf_type)
    {
		case PREVIEWBUFFER:			
			buf_info = mPreviewBufferInfo;
			break;
		case RAWBUFFER:
			buf_info = mRawBufferInfo;
			break;
		case JPEGBUFFER:
			buf_info = mJpegBufferInfo;
			break;
		case VIDEOENCBUFFER:
			buf_info = mVideoEncBufferInfo;
			break;
        default:
            LOGE("Buffer type(0x%x) is invaildate",buf_type);
            goto getVirAddr_end;
    }

    if (buf_idx > buf_info->mNumBffers) {
        LOGE("Buffer index(0x%x) is invalidate, Total buffer is 0x%x",
            buf_idx,buf_info->mNumBffers);
        goto getVirAddr_end;
    }

    if (addr_type == buffer_addr_vir) {
        addr = (buf_info+buf_idx)->mVirBaseAddr;
    } else if (addr_type == buffer_addr_phy) {
        addr = (buf_info+buf_idx)->mPhyBaseAddr;
    } else if(addr_type == buffer_sharre_fd){
		addr = (buf_info+buf_idx)->mShareFd;
	}
getVirAddr_end:
    return addr;
}

int MemManagerBase::dump()
{
    
    return 0;
}

#if (CONFIG_CAMERA_MEM == CAMERA_MEM_ION)

IonMemManager::IonMemManager()
			     :MemManagerBase(),
			      mPreviewData(NULL),
			      mRawData(NULL),
			      mJpegData(NULL),
				mVideoEncData(NULL),
				mIonMemMgr(NULL)
{
	mIonMemMgr = new IonAlloc(PAGE_SIZE, ION_MODULE_CAM);
}

IonMemManager::~IonMemManager()
{
	if (mPreviewData) {
		destroyPreviewBuffer();
		free(mPreviewData);
        mPreviewData = NULL;
	}
	if(mRawData) {
		destroyRawBuffer();
		free(mRawData);
        mRawData = NULL;
	}
	if(mJpegData) {
		destroyJpegBuffer();
		free(mJpegData);
        mJpegData = NULL;
	}
	if(mVideoEncData) {
		destroyVideoEncBuffer();
		free(mVideoEncData);
		mVideoEncData = NULL;
	}
	if(mIonMemMgr)
		delete mIonMemMgr;
	mIonMemMgr = NULL;

}

int IonMemManager::createIonBuffer(struct bufferinfo_s* ionbuf)
{
	int ret =0;
	int numBufs;
	int frame_size;
	struct ion_buffer_t* tmpalloc = NULL;
	struct bufferinfo_s* tmp_buf = NULL;
    
	if (!ionbuf || !mIonMemMgr < 0) {
		LOGE("ion_alloc malloc buffer failed");
		goto null_fail;
	}
    
	numBufs = ionbuf->mNumBffers;
	frame_size = ionbuf->mPerBuffersize;
	ionbuf->mBufferSizes = numBufs*PAGE_ALIGN(frame_size);
	switch(ionbuf->mBufType)
	{
		case PREVIEWBUFFER:
			tmpalloc = mPreviewData;
			tmp_buf = mPreviewBufferInfo;
			break;
		case RAWBUFFER:
			tmpalloc = mRawData;
			tmp_buf = mRawBufferInfo;
			break;
		case JPEGBUFFER:
			tmpalloc = mJpegData;
			tmp_buf = mJpegBufferInfo;
			break;
		case VIDEOENCBUFFER:
			tmpalloc = mVideoEncData;
			tmp_buf = mVideoEncBufferInfo;
			break;
        default:
            goto null_fail;
    }
    
	memset(tmpalloc,0,sizeof(tmpalloc));
	ret = mIonMemMgr->alloc(ionbuf->mBufferSizes, _ION_HEAP_RESERVE, tmpalloc);
	if(ret != 0) {
		LOGE("ion_alloc malloc buffer failed , type is = %d ",ionbuf->mBufType);
		goto alloc_fail;
	}
    
	ionbuf->mPhyBaseAddr = (unsigned long)tmpalloc->phys;
	ionbuf->mVirBaseAddr = (unsigned long)tmpalloc->virt;
	ionbuf->mPerBuffersize = PAGE_ALIGN(frame_size);
	*tmp_buf = *ionbuf;

    return 0;

alloc_fail:
null_fail:
    memset(tmp_buf,0,sizeof(struct bufferinfo_s));
    memset(tmpalloc,0,sizeof(ion_buffer_t));
    return ret;
}

void IonMemManager::destroyIonBuffer(buffer_type_enum buftype)
{
	struct ion_buffer_t* tmpalloc = NULL;
    
	switch(buftype)
	{
		case PREVIEWBUFFER:
			tmpalloc = mPreviewData;
			if(tmpalloc && tmpalloc->virt) {
				LOG1("free preview buffer success!");
				mIonMemMgr->free(*tmpalloc);
				memset(tmpalloc,0,sizeof(ion_buffer_t));
 			} else {
 			    if (mPreviewData == NULL) {
				  //  LOGE("%s(%d): mPreviewData is NULL",__FUNCTION__,__LINE__);
                } else {
				    LOGE("mPreviewData->virt:0x%x mPreviewBufferInfo.mVirBaseAddr:0x%x",(long)mPreviewData->virt,mPreviewBufferInfo->mVirBaseAddr);
                }
			}
			memset(&mPreviewBufferInfo,0,sizeof(mPreviewBufferInfo));
			break;
		case RAWBUFFER:
			tmpalloc = mRawData;
			if(tmpalloc && tmpalloc->virt) {
				LOG1("free RAWBUFFER buffer success!");
				mIonMemMgr->free(*tmpalloc);
				memset(tmpalloc,0,sizeof(ion_buffer_t));
			} else {
				if (mRawData == NULL) {
				 //   LOGE("%s(%d): mRawData is NULL",__FUNCTION__,__LINE__);
                } else {
				    LOGE("mRawData->virt:0x%x mRawBufferInfo.mVirBaseAddr:0x%x",(long)mRawData->virt,mRawBufferInfo->mVirBaseAddr);
                }
			}
			memset(&mRawBufferInfo,0,sizeof(mRawBufferInfo));
			break;
		case JPEGBUFFER:
			tmpalloc = mJpegData;
			if(tmpalloc && tmpalloc->virt) {
				LOG1("free RAWBUFFER buffer success!");
				mIonMemMgr->free(*tmpalloc);
				memset(tmpalloc,0,sizeof(ion_buffer_t));
            } else {
				if (mJpegData == NULL) {
				//    LOGE("%s(%d): mJpegData is NULL",__FUNCTION__,__LINE__);
                } else {
				    LOGE("mJpegData->virt:0x%x mRawBufferInfo.mVirBaseAddr:0x%x",(long)mJpegData->virt,mJpegBufferInfo->mVirBaseAddr);
                }
			}
			memset(&mJpegBufferInfo,0,sizeof(mJpegBufferInfo));
			break;
		case VIDEOENCBUFFER:
			tmpalloc = mVideoEncData;
			if(tmpalloc && tmpalloc->virt) {
				LOG1("free VIDEOENCBUFFER buffer success!");
				mIonMemMgr->free(*tmpalloc);
				memset(tmpalloc,0,sizeof(ion_buffer_t));
			} else {
				if (mVideoEncData == NULL) {
				//	LOGE("%s(%d): mVideoEncData is NULL",__FUNCTION__,__LINE__);
				} else {
					LOGE("mVideoEncData->virt:0x%x mVideoEncBufferInfo.mVirBaseAddr:0x%x",(long)mVideoEncData->virt,mVideoEncBufferInfo->mVirBaseAddr);
				}
			}
			memset(&mVideoEncBufferInfo,0,sizeof(mVideoEncBufferInfo));
			break;

        default:
		   	LOGE("buffer type is wrong !");
            break;
	}
}

int IonMemManager::createVideoEncBuffer(struct bufferinfo_s* videoencbuf)
{
	LOG_FUNCTION_NAME
	int ret;
	Mutex::Autolock lock(mLock);
	
	if(videoencbuf->mBufType != VIDEOENCBUFFER)
		LOGE("the type is not VIDEOENCBUFFER");
	
	if(!mVideoEncData) {
		mVideoEncData = (ion_buffer_t*)malloc(sizeof(ion_buffer_t));
	} else if(mVideoEncData->virt) {
		LOG1("FREE the video buffer alloced before firstly");
		destroyVideoEncBuffer();
	}
	
	memset(mVideoEncData,0,sizeof(ion_buffer_t));
	
	ret = createIonBuffer(videoencbuf);
	if (ret == 0) {
		LOG1("Video buffer information(phy:0x%x vir:0x%x size:0x%x)",
			mVideoEncBufferInfo->mPhyBaseAddr,mVideoEncBufferInfo->mVirBaseAddr,mVideoEncBufferInfo->mBufferSizes);
	} else {
		LOGE("Video buffer alloc failed");
	}
	LOG_FUNCTION_NAME_EXIT
	return ret;
}
int IonMemManager::destroyVideoEncBuffer()
{
	LOG_FUNCTION_NAME
	Mutex::Autolock lock(mLock);

	destroyIonBuffer(VIDEOENCBUFFER);

	LOG_FUNCTION_NAME_EXIT
	return 0;

}

int IonMemManager::createPreviewBuffer(struct bufferinfo_s* previewbuf)
{
	LOG_FUNCTION_NAME
    int ret;
	Mutex::Autolock lock(mLock);
    
	if(previewbuf->mBufType != PREVIEWBUFFER)
		LOGE("the type is not PREVIEWBUFFER");
    
	if(!mPreviewData) {
		mPreviewData = (ion_buffer_t*)malloc(sizeof(ion_buffer_t));
	} else if(mPreviewData->virt) {
		LOG1("FREE the preview buffer alloced before firstly");
		destroyPreviewBuffer();
	}
    
	memset(mPreviewData,0,sizeof(ion_buffer_t));
    
    ret = createIonBuffer(previewbuf);
    if (ret == 0) {
        LOG1("Preview buffer information(phy:0x%x vir:0x%x size:0x%x)",
            mPreviewBufferInfo->mPhyBaseAddr,mPreviewBufferInfo->mVirBaseAddr,mPreviewBufferInfo->mBufferSizes);
    } else {
        LOGE("%s(%d): Preview buffer alloc failed",__FUNCTION__,__LINE__);
    }
    LOG_FUNCTION_NAME_EXIT
	return ret;
}
int IonMemManager::destroyPreviewBuffer()
{
	LOG_FUNCTION_NAME
	Mutex::Autolock lock(mLock);

	destroyIonBuffer(PREVIEWBUFFER);

	LOG_FUNCTION_NAME_EXIT
	return 0;

}
int IonMemManager::createRawBuffer(struct bufferinfo_s* rawbuf)
{
	LOG_FUNCTION_NAME
    int ret;
	Mutex::Autolock lock(mLock);
    
	if (rawbuf->mBufType != RAWBUFFER)
		LOGE("%s(%d): the type is not RAWBUFFER",__FUNCTION__,__LINE__);

    if (!mRawData) {
		mRawData = (ion_buffer_t*)malloc(sizeof(ion_buffer_t));
	} else if(mRawData->virt) {
		LOG1("FREE the raw buffer alloced before firstly");
		destroyRawBuffer();
	}
	memset(mRawData,0,sizeof(ion_buffer_t));

    ret = createIonBuffer(rawbuf);
    if (ret == 0) {
        LOG1("Raw buffer information(phy:0x%x vir:0x%x size:0x%x)",
            mRawBufferInfo->mPhyBaseAddr,mRawBufferInfo->mVirBaseAddr,mRawBufferInfo->mBufferSizes);
    } else {
        LOGE("Raw buffer alloc failed");
    }
    LOG_FUNCTION_NAME_EXIT
	return ret;

}
int IonMemManager::destroyRawBuffer()
{
	LOG_FUNCTION_NAME
	Mutex::Autolock lock(mLock);
	destroyIonBuffer(RAWBUFFER);
	LOG_FUNCTION_NAME_EXIT
	return 0;
}
 int IonMemManager::createJpegBuffer(struct bufferinfo_s* jpegbuf)
 {
    LOG_FUNCTION_NAME
    int ret;
    Mutex::Autolock lock(mLock);

    if(jpegbuf->mBufType != JPEGBUFFER)
        LOGE("the type is not JPEGBUFFER");

    if(!mJpegData) {
        mJpegData = (ion_buffer_t*)malloc(sizeof(ion_buffer_t));
    } else if(mJpegData->virt) {
        LOG1("FREE the jpeg buffer alloced before firstly");
        destroyJpegBuffer();
    }
    memset(mJpegData,0,sizeof(ion_buffer_t));
    
    ret = createIonBuffer(jpegbuf);
    if (ret == 0) {
        LOG1("Jpeg buffer information(phy:0x%x vir:0x%x size:0x%x)",
            mJpegBufferInfo->mPhyBaseAddr,mJpegBufferInfo->mVirBaseAddr,mJpegBufferInfo->mBufferSizes);
    } else {
        LOGE("Jpeg buffer alloc failed");
    }
    LOG_FUNCTION_NAME_EXIT
	return ret;

 }
int IonMemManager::destroyJpegBuffer()
{
	 LOG_FUNCTION_NAME
	 Mutex::Autolock lock(mLock);
	 destroyIonBuffer(JPEGBUFFER);
	 LOG_FUNCTION_NAME_EXIT
	 return 0;

}
int IonMemManager::flushCacheMem(buffer_type_enum buftype,unsigned int offset, unsigned int len)
{
    Mutex::Autolock lock(mLock);
    ion_buffer_t data;
    
    switch(buftype)
	{
		case PREVIEWBUFFER:
			data = *mPreviewData;
			break;
		case RAWBUFFER:
			data = *mRawData;
            
			break;
		case JPEGBUFFER:
			data = *mJpegData;
			break;
		case VIDEOENCBUFFER:
			data = *mVideoEncData;
			break;
		default:
			break;
	}
    
    mIonMemMgr->cache_op(data, ION_FLUSH_CACHE);

    return 0;
}
#endif

#if (CONFIG_CAMERA_MEM == CAMERA_MEM_IONDMA)
IonDmaMemManager::IonDmaMemManager(bool iommuEnabled)
			     :MemManagerBase(),
			      mPreviewData(NULL),
			      mRawData(NULL),
			      mJpegData(NULL),
				mVideoEncData(NULL),
				client_fd(-1),
				mIommuEnabled(iommuEnabled)
{
	client_fd = ion_open();
}

IonDmaMemManager::~IonDmaMemManager()
{
	if (mPreviewData) {
		destroyPreviewBuffer();
		free(mPreviewData);
        mPreviewData = NULL;
	}
	if(mRawData) {
		destroyRawBuffer();
		free(mRawData);
        mRawData = NULL;
	}
	if(mJpegData) {
		destroyJpegBuffer();
		free(mJpegData);
        mJpegData = NULL;
	}
	if(mVideoEncData) {
		destroyVideoEncBuffer();
		free(mVideoEncData);
		mVideoEncData = NULL;
	}
	if(client_fd != -1)
         ion_close(client_fd);
		

}

int IonDmaMemManager::createIonBuffer(struct bufferinfo_s* ionbuf)
{
	int ret =0,i = 0;
	int numBufs;
	int frame_size;
	camera_ionbuf_t* tmpalloc = NULL;
	struct bufferinfo_s* tmp_buf = NULL;
	#ifdef ROCKCHIP_ION_VERSION
    ion_user_handle_t handle = 0;
	#else
	struct ion_handle* handle = NULL;
	#endif
    int map_fd;
	long temp_handle = 0;
    unsigned long vir_addr = 0;

    
	if (!ionbuf) {
		LOGE("ion_alloc malloc buffer failed");
		return -1;
	}
    
	numBufs = ionbuf->mNumBffers;
	frame_size = ionbuf->mPerBuffersize;
	ionbuf->mBufferSizes = numBufs*PAGE_ALIGN(frame_size);
	switch(ionbuf->mBufType)
	{
		case PREVIEWBUFFER:
            tmpalloc = mPreviewData ;
			if((tmp_buf  = (struct bufferinfo_s*)malloc(numBufs*sizeof(struct bufferinfo_s))) != NULL){
                mPreviewBufferInfo = tmp_buf;
            }else{
        		LOGE("ion_alloc malloc buffer failed");
        		return -1;
            }
			break;
		case RAWBUFFER:
            tmpalloc =  mRawData;
               
			if((tmp_buf = (struct bufferinfo_s*)malloc(numBufs*sizeof(struct bufferinfo_s))) != NULL){
                mRawBufferInfo = tmp_buf;
            }else{
        		LOGE("ion_alloc malloc buffer failed");
        		return -1;
            }
			break;
		case JPEGBUFFER:
            tmpalloc = mJpegData;
            
			if((tmp_buf  = (struct bufferinfo_s*)malloc(numBufs*sizeof(struct bufferinfo_s))) != NULL ){
                mJpegBufferInfo = tmp_buf;
            }else{
        		LOGE("ion_alloc malloc buffer failed");
        		return -1;
            }
			break;
		case VIDEOENCBUFFER:
            tmpalloc =  mVideoEncData ;

            if((tmp_buf = (struct bufferinfo_s*)malloc(numBufs*sizeof(struct bufferinfo_s))) != NULL){
                mVideoEncBufferInfo = tmp_buf;
            }else{
        		LOGE("ion_alloc malloc buffer failed");
        		return -1;
            }
			break;
        default:
            return -1;
    }

    for(i = 0;i < numBufs;i++){
    	memset(tmpalloc,0,sizeof(struct camera_ionbuf_s));

        if((!mIommuEnabled) || (!ionbuf->mIsForceIommuBuf)){
			#if defined(TARGET_RK3188)
            ret = ion_alloc(client_fd, ionbuf->mPerBuffersize, PAGE_SIZE, ION_HEAP(ION_CARVEOUT_HEAP_ID), 0, &handle);
			#else
			ret = ion_alloc(client_fd, ionbuf->mPerBuffersize, PAGE_SIZE, ION_HEAP(ION_CMA_HEAP_ID), 0, &handle);
			#endif
		}else{
            ret = ion_alloc(client_fd, ionbuf->mPerBuffersize, PAGE_SIZE, ION_HEAP(ION_VMALLOC_HEAP_ID), 0, &handle);
        	}
		if (ret) {
            LOGE("ion alloc failed\n");
            break;
        }

        LOG1("handle %d\n", handle);

        ret = ion_share(client_fd,handle,&map_fd);
        if (ret) {
            LOGE("ion map failed\n");
            ion_free(client_fd,handle);
            break;
        }

        vir_addr = (unsigned long )mmap(NULL, ionbuf->mPerBuffersize, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
        if (vir_addr == 0) {
            LOGE("ion mmap failed\n");
            ret = -1;
            ion_free(client_fd,handle);
            break;
        }
        
        if((!mIommuEnabled) || (!ionbuf->mIsForceIommuBuf)){
            ret=ion_get_phys(client_fd,handle,&(tmpalloc->phy_addr));
			if(ret<0)
				LOGE("ion_get_phys failed\n");
        }else{
				tmpalloc->phy_addr = map_fd;
        }
		tmpalloc->size = ionbuf->mPerBuffersize;
        tmpalloc->vir_addr = vir_addr;
		temp_handle = handle;
        tmpalloc->ion_hdl = (void*)temp_handle;
        tmpalloc->map_fd    =   map_fd;

        
    	ionbuf->mPhyBaseAddr = (unsigned long)tmpalloc->phy_addr;
    	ionbuf->mVirBaseAddr = (unsigned long)tmpalloc->vir_addr;
    	ionbuf->mPerBuffersize = PAGE_ALIGN(frame_size);
    	ionbuf->mShareFd     = (unsigned int)tmpalloc->map_fd;
		*tmp_buf = *ionbuf;
		tmp_buf++;
        tmpalloc++;
        
    }
    if(ret < 0){
        LOGE(" failed !");
        while(--i >= 0){
            --tmpalloc;
            --tmp_buf;
            munmap((void *)tmpalloc->vir_addr, tmpalloc->size);
            ion_free(client_fd, tmpalloc->ion_hdl);
        }
        free(tmpalloc);
        free(tmp_buf);
    }
    return ret;
}

void IonDmaMemManager::destroyIonBuffer(buffer_type_enum buftype)
{
	camera_ionbuf_t* tmpalloc = NULL;
    int err = 0;
	struct bufferinfo_s* tmp_buf = NULL;

   
	switch(buftype)
	{
		case PREVIEWBUFFER:
			tmpalloc = mPreviewData;
            tmp_buf = mPreviewBufferInfo;
			break;
		case RAWBUFFER:
			tmpalloc = mRawData;
            tmp_buf = mRawBufferInfo;
			break;
		case JPEGBUFFER:
			tmpalloc = mJpegData;
            tmp_buf = mJpegBufferInfo;
			break;
		case VIDEOENCBUFFER:
			tmpalloc = mVideoEncData;
            tmp_buf = mVideoEncBufferInfo;
			break;

        default:
		   	LOGE("buffer type is wrong !");
            break;
	}


    for(unsigned int i = 0;(tmp_buf && (i < tmp_buf->mNumBffers));i++){
    	if(tmpalloc && tmpalloc->vir_addr) {
                err = munmap((void *)tmpalloc->vir_addr, tmpalloc->size);
            if (err) {
                LOGE("munmap failed\n");
                    return;
            }

            close(tmpalloc->map_fd);
            err = ion_free(client_fd, tmpalloc->ion_hdl);
        }
        tmpalloc++;
    }

	switch(buftype)
	{
		case PREVIEWBUFFER:
			free(mPreviewData);
			mPreviewData = NULL;
            free(mPreviewBufferInfo);
            mPreviewBufferInfo = NULL;
			break;
		case RAWBUFFER:
			free(mRawData);
            mRawData = NULL;
            free(mRawBufferInfo);
            mRawBufferInfo = NULL;
			break;
		case JPEGBUFFER:
			free(mJpegData);
            mJpegData = NULL;
            free(mJpegBufferInfo);
            mJpegBufferInfo = NULL;
			break;
		case VIDEOENCBUFFER:
			free(mVideoEncData);
            mVideoEncData = NULL;
            free(mVideoEncBufferInfo);
            mVideoEncBufferInfo = NULL;
			break;

        default:
		   	LOGE("buffer type is wrong !");
            break;
	}

    
}


int IonDmaMemManager::createVideoEncBuffer(struct bufferinfo_s* videoencbuf)
{
	LOG_FUNCTION_NAME
	int ret;
	Mutex::Autolock lock(mLock);
	
	if(videoencbuf->mBufType != VIDEOENCBUFFER)
		LOGE("the type is not VIDEOENCBUFFER");
	
	if(!mVideoEncData) {
		mVideoEncData = (camera_ionbuf_t*)malloc(sizeof(camera_ionbuf_t) * videoencbuf->mNumBffers);
	} else if(mVideoEncData->vir_addr) {
		LOG1("FREE the video buffer alloced before firstly");
		destroyVideoEncBuffer();
	}
	
	memset(mVideoEncData,0,sizeof(camera_ionbuf_t)* videoencbuf->mNumBffers);
	
	ret = createIonBuffer(videoencbuf);
	if (ret == 0) {
		LOG1("Video buffer information(phy:0x%x vir:0x%x size:0x%x)",
			mVideoEncBufferInfo->mPhyBaseAddr,mVideoEncBufferInfo->mVirBaseAddr,mVideoEncBufferInfo->mBufferSizes);
	} else {
		LOGE("Video buffer alloc failed");
	}
	LOG_FUNCTION_NAME_EXIT
	return ret;
}
int IonDmaMemManager::destroyVideoEncBuffer()
{
	LOG_FUNCTION_NAME
	Mutex::Autolock lock(mLock);

	destroyIonBuffer(VIDEOENCBUFFER);

	LOG_FUNCTION_NAME_EXIT
	return 0;

}

int IonDmaMemManager::createPreviewBuffer(struct bufferinfo_s* previewbuf)
{
	LOG_FUNCTION_NAME
    int ret;
	Mutex::Autolock lock(mLock);
    
	if(previewbuf->mBufType != PREVIEWBUFFER)
		LOGE("the type is not PREVIEWBUFFER");
    
	if(!mPreviewData) {
		mPreviewData = (camera_ionbuf_t*)malloc(sizeof(camera_ionbuf_t) * previewbuf->mNumBffers);
		if(!mPreviewData){
			LOGE("malloc mPreviewData failed!");
			ret = -1;
			return ret;
		}
	} else if(mPreviewData->vir_addr) {
		LOG1("FREE the preview buffer alloced before firstly");
		destroyPreviewBuffer();
	}
    
	memset(mPreviewData,0,sizeof(camera_ionbuf_t)* previewbuf->mNumBffers);
    
    ret = createIonBuffer(previewbuf);
    if (ret == 0) {
        LOG1("Preview buffer information(phy:0x%x vir:0x%x size:0x%x)",
            mPreviewBufferInfo->mPhyBaseAddr,mPreviewBufferInfo->mVirBaseAddr,mPreviewBufferInfo->mBufferSizes);
    } else {
        LOGE("Preview buffer alloc failed");
    }
    LOG_FUNCTION_NAME_EXIT
	return ret;
}
int IonDmaMemManager::destroyPreviewBuffer()
{
	LOG_FUNCTION_NAME
	Mutex::Autolock lock(mLock);

	destroyIonBuffer(PREVIEWBUFFER);

	LOG_FUNCTION_NAME_EXIT
	return 0;

}
int IonDmaMemManager::createRawBuffer(struct bufferinfo_s* rawbuf)
{
	LOG_FUNCTION_NAME
    int ret;
	Mutex::Autolock lock(mLock);
    
	if (rawbuf->mBufType != RAWBUFFER)
		LOGE("the type is not RAWBUFFER");

    if (!mRawData) {
		mRawData = (camera_ionbuf_t*)malloc(sizeof(camera_ionbuf_t) * rawbuf->mNumBffers);
	} else if(mRawData->vir_addr) {
		LOG1("FREE the raw buffer alloced before firstly");
		destroyRawBuffer();
	}
	memset(mRawData,0,sizeof(camera_ionbuf_t)* rawbuf->mNumBffers);

    ret = createIonBuffer(rawbuf);
    if (ret == 0) {
        LOG1("Raw buffer information(phy:0x%x vir:0x%x size:0x%x)",
            mRawBufferInfo->mPhyBaseAddr,mRawBufferInfo->mVirBaseAddr,mRawBufferInfo->mBufferSizes);
    } else {
        LOGE("Raw buffer alloc failed");
    }
    LOG_FUNCTION_NAME_EXIT
	return ret;

}
int IonDmaMemManager::destroyRawBuffer()
{
	LOG_FUNCTION_NAME
	Mutex::Autolock lock(mLock);
	destroyIonBuffer(RAWBUFFER);
	LOG_FUNCTION_NAME_EXIT
	return 0;
}

 int IonDmaMemManager::createJpegBuffer(struct bufferinfo_s* jpegbuf)
 {
    LOG_FUNCTION_NAME
    int ret;
    Mutex::Autolock lock(mLock);

    if(jpegbuf->mBufType != JPEGBUFFER)
        LOGE("the type is not JPEGBUFFER");

    if(!mJpegData) {
        mJpegData = (camera_ionbuf_t*)malloc(sizeof(camera_ionbuf_t) * jpegbuf->mNumBffers);
    } else if(mJpegData->vir_addr) {
        LOG1("FREE the jpeg buffer alloced before firstly");
        destroyJpegBuffer();
    }
    memset(mJpegData,0,sizeof(camera_ionbuf_t)* jpegbuf->mNumBffers);
    
    ret = createIonBuffer(jpegbuf);
    if (ret == 0) {
        LOG1("Jpeg buffer information(phy:0x%x vir:0x%x size:0x%x)",
            mJpegBufferInfo->mPhyBaseAddr,mJpegBufferInfo->mVirBaseAddr,mJpegBufferInfo->mBufferSizes);
    } else {
        LOGE("Jpeg buffer alloc failed");
    }
    LOG_FUNCTION_NAME_EXIT
	return ret;

 }
int IonDmaMemManager::destroyJpegBuffer()
{
	 LOG_FUNCTION_NAME
	 Mutex::Autolock lock(mLock);
	 destroyIonBuffer(JPEGBUFFER);
	 LOG_FUNCTION_NAME_EXIT
	 return 0;

}
int IonDmaMemManager::flushCacheMem(buffer_type_enum buftype,unsigned int offset, unsigned int len)
{
    Mutex::Autolock lock(mLock);

    return 0;
}


#endif


/******************ION BUFFER END*******************/

/*****************pmem buffer start*******************/
#if (CONFIG_CAMERA_MEM == CAMERA_MEM_PMEM)
PmemManager::PmemManager(char* devpath)
			:MemManagerBase(),
			mPmemFd(-1),
			mPmemSize(0),
			mPmemHeapPhyBase(0),
			mMemHeap(NULL),
			mMemHeapPmem(NULL),
			mJpegBuffer(NULL),
			mRawBuffer(NULL),
			mPreviewBuffer(NULL)
{
    initPmem(devpath);
}
PmemManager::~PmemManager()
{
	deinitPmem();
}
int PmemManager::initPmem(char* devpath)
{
	int pmem_fd;
	struct pmem_region sub;
	int err = 0;
	Mutex::Autolock lock(mLock);
    
	if(mPmemFd > 0) {
		LOGE(" PMEM has been initialized");
		return err;
	}
    
	pmem_fd = open(devpath, O_RDWR);
	if (pmem_fd < 0) {
		LOGE("Open the PMEM device(%s): %s", devpath, strerror(errno));
        err = -1;
        goto exit;
	}

	ioctl(pmem_fd, PMEM_GET_TOTAL_SIZE, &sub);
	mPmemSize = sub.len;

	if (pmem_fd > 0) {
		close(pmem_fd);
		pmem_fd = -1;
	}

	mMemHeap = new MemoryHeapBase(devpath,mPmemSize,0);
	mPmemFd = mMemHeap->getHeapID();
	if (mPmemFd < 0) {
		LOGE("allocate mMemHeap from %s failed",devpath);
		err = -1;		
		goto exit;
	}

	if (ioctl(mPmemFd,PMEM_GET_PHYS, &sub)) {
		LOGE("Obtain %s physical address failed",devpath);
		err = -1;
        goto exit;
	} else {
		mPmemHeapPhyBase = sub.offset;
	}

	mMemHeapPmem = new MemoryHeapPmem(mMemHeap,0);

exit:
    if (err < 0) {
        if (mMemHeapPmem != NULL) {
            mMemHeapPmem.clear();
            mMemHeapPmem = NULL;
        }
        
        if (mMemHeap != NULL) {			
			mMemHeap.clear();
			mMemHeap = NULL;
		}
    }
    if (pmem_fd > 0) {
		close(pmem_fd);
		pmem_fd = -1;
	} 
    return err;
}	

int PmemManager::deinitPmem()
{
	Mutex::Autolock lock(mLock);

	if (mMemHeapPmem != NULL) { 		
		mMemHeapPmem.clear();
		mMemHeapPmem = NULL;
	}
	if (mMemHeap != NULL) {		
		mMemHeap.clear();
		mMemHeap = NULL;
	}
	mPmemHeapPhyBase = 0;

	if( mPmemFd > 0 ) {
		close(mPmemFd);
		mPmemFd = -1;
	}
	return 0;
}

int PmemManager::createPreviewBuffer(struct bufferinfo_s* previewbuf)
{
    LOG_FUNCTION_NAME
	int ret =0,i ;
	struct bufferinfo_s* tmp_buf = NULL;
	void * viraddress = NULL;
	int numBufs;
	int frame_size;
	Mutex::Autolock lock(mLock);
    
	if(!previewbuf || mMemHeapPmem == NULL){
		LOGE("Pmem malloc preview buffer failed");
		ret = -1;
		goto null_fail;
	}
    
	numBufs = previewbuf->mNumBffers;
	frame_size = previewbuf->mPerBuffersize;
	previewbuf->mBufferSizes = numBufs*PAGE_ALIGN(frame_size);

    if( mPmemSize < previewbuf->mBufferSizes+mJpegBufferInfo.mBufferSizes+mRawBufferInfo.mBufferSizes){
        LOGE("Pmem is not enough for 0x%x bytes preview buffer!(Pmem:0x%x, Raw:0x%x, Jpeg:0x%x)",
            previewbuf->mBufferSizes,mPmemSize,mRawBufferInfo.mBufferSizes,mJpegBufferInfo.mBufferSizes);
        ret = -1;
        goto null_fail;
    }
    
    mPreviewBuffer = (sp<IMemory>**)malloc(sizeof(sp<IMemory>*)*numBufs);
    if (mPreviewBuffer == NULL) {
        LOGE("mPreviewBuffer malloc failed");
        ret = -1;
        goto null_fail;
    }
    
	for(i = 0; i < numBufs; i++){        
        mPreviewBuffer[i] = (sp<IMemory>*)new (sp<IMemory>);
		*mPreviewBuffer[i] = (static_cast<MemoryHeapPmem*>(mMemHeapPmem.get()))->mapMemory(PAGE_ALIGN(frame_size)*i,PAGE_ALIGN(frame_size));
	}
    
	previewbuf->mPhyBaseAddr = mPmemHeapPhyBase + (*mPreviewBuffer[0])->offset();
	previewbuf->mVirBaseAddr= (unsigned long)(*mPreviewBuffer[0])->pointer();
	previewbuf->mPerBuffersize = PAGE_ALIGN(frame_size);
	mPreviewBufferInfo = *previewbuf;
    LOG1("Preview buffer information(phy:0x%x vir:0x%x size:0x%x)",
        mPreviewBufferInfo.mPhyBaseAddr,mPreviewBufferInfo.mVirBaseAddr,mPreviewBufferInfo.mBufferSizes);

    LOG_FUNCTION_NAME_EXIT
null_fail:        
	return ret;
}
int PmemManager::destroyPreviewBuffer()
{
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    unsigned int i;

    for(i = 0; i < mPreviewBufferInfo.mNumBffers; i++) {
        if (mPreviewBuffer[i] != NULL) {
            (*mPreviewBuffer[i]).clear();
            delete mPreviewBuffer[i];
            mPreviewBuffer[i] = NULL;
        }
    }
    free((char*)mPreviewBuffer);
    mPreviewBuffer = NULL;
    memset(&mPreviewBufferInfo,0,sizeof(mPreviewBufferInfo));
    LOG_FUNCTION_NAME_EXIT
    return 0;
}
int PmemManager::createRawBuffer(struct bufferinfo_s* rawbuf)
{
    LOG_FUNCTION_NAME
    int ret =0;
    int numBufs;
    int frame_size;
    int map_start;
    Mutex::Autolock lock(mLock);

    if(!rawbuf || mMemHeapPmem == NULL ){
        LOGE("Pmem malloc raw buffer failed");
        ret = -1;
        goto null_fail;
    }
    
    numBufs = rawbuf->mNumBffers;
    frame_size = rawbuf->mPerBuffersize;
    rawbuf->mBufferSizes = numBufs*PAGE_ALIGN(frame_size);
    //compute the start address of map
    if( mPmemSize < mPreviewBufferInfo.mBufferSizes + mJpegBufferInfo.mBufferSizes +rawbuf->mBufferSizes){
        LOGE("Pmem is not enough for 0x%x bytes raw buffer!(Pmem:0x%x, Preview:0x%x, Jpeg:0x%x)",
            rawbuf->mBufferSizes,mPmemSize,mPreviewBufferInfo.mBufferSizes,mJpegBufferInfo.mBufferSizes);
        ret = -1;
        goto null_fail;
    } else {
        map_start = mPreviewBufferInfo.mBufferSizes;
    }
    
    mRawBuffer = (static_cast<MemoryHeapPmem*>(mMemHeapPmem.get()))->mapMemory(map_start,PAGE_ALIGN(frame_size)*numBufs);
    rawbuf->mPhyBaseAddr = mPmemHeapPhyBase + mRawBuffer->offset();
    rawbuf->mVirBaseAddr= (unsigned long)mRawBuffer->pointer();
    rawbuf->mPerBuffersize = PAGE_ALIGN(frame_size);
    mRawBufferInfo= *rawbuf;

    LOG1("Raw buffer information(phy:0x%x vir:0x%x size:0x%x)",
        mRawBufferInfo.mPhyBaseAddr,mRawBufferInfo.mVirBaseAddr,mRawBufferInfo.mBufferSizes);

    LOG_FUNCTION_NAME_EXIT
    return ret;
null_fail:
    memset(&mRawBufferInfo,0,sizeof(mRawBufferInfo));
    return ret;
}
int PmemManager::destroyRawBuffer()
{
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);

    if (mRawBuffer!= NULL) {        
        mRawBuffer.clear();
        mRawBuffer = NULL;
    }
    memset(&mRawBufferInfo,0,sizeof(mRawBufferInfo));
    LOG_FUNCTION_NAME_EXIT
    return 0;
}
int PmemManager::createJpegBuffer(struct bufferinfo_s* jpegbuf)
{
    LOG_FUNCTION_NAME
    int ret =0;
    int numBufs;
    int frame_size;
    int map_start;

    Mutex::Autolock lock(mLock);
    if(!jpegbuf || mMemHeapPmem == NULL ){
        LOGE("Pmem malloc jpeg buffer failed");
        ret = -1;
        goto null_fail;
    }
    
    numBufs = jpegbuf->mNumBffers;
    frame_size = jpegbuf->mPerBuffersize;
    jpegbuf->mBufferSizes = numBufs*PAGE_ALIGN(frame_size);
    
    //compute the start address of map
    if ( mPmemSize < mPreviewBufferInfo.mBufferSizes + mRawBufferInfo.mBufferSizes +jpegbuf->mBufferSizes) {
        LOGE("Pmem is not enough for 0x%x bytes jpeg buffer!(Pmem:0x%x, Preview:0x%x, Raw:0x%x)",
            jpegbuf->mBufferSizes,mPmemSize,mPreviewBufferInfo.mBufferSizes,mRawBufferInfo.mBufferSizes);
        ret = -1;
        goto null_fail;
    } else {
        map_start = mPmemSize - jpegbuf->mBufferSizes;
    }
    mJpegBuffer = (static_cast<MemoryHeapPmem*>(mMemHeapPmem.get()))->mapMemory(map_start,PAGE_ALIGN(frame_size)*numBufs);
    jpegbuf->mPhyBaseAddr = mPmemHeapPhyBase + mJpegBuffer->offset();
    jpegbuf->mVirBaseAddr= (unsigned long)mJpegBuffer->pointer();
    jpegbuf->mPerBuffersize = PAGE_ALIGN(frame_size);
    mJpegBufferInfo= *jpegbuf;

    LOG1("Jpeg buffer information(phy:0x%x vir:0x%x size:0x%x)",
        mJpegBufferInfo.mPhyBaseAddr,mJpegBufferInfo.mVirBaseAddr,mJpegBufferInfo.mBufferSizes);
    LOG_FUNCTION_NAME_EXIT
    return ret;
    
null_fail:
    memset(&mJpegBufferInfo,0,sizeof(mJpegBufferInfo));
    return ret;
}
int PmemManager::destroyJpegBuffer()
{
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);

    if (mJpegBuffer!= NULL) {
       mJpegBuffer.clear();
       mJpegBuffer = NULL;
    }
    memset(&mJpegBufferInfo,0,sizeof(mJpegBufferInfo));
    LOG_FUNCTION_NAME_EXIT
    return 0;
}
int PmemManager::flushCacheMem(buffer_type_enum buftype,unsigned int offset, unsigned int len)
{
	struct bufferinfo_s* tmpbuf =NULL;
	int ret = 0;
	struct pmem_region region;
	sp<IMemory> tmpmem;
    unsigned int i;
	Mutex::Autolock lock(mLock);    

	switch(buftype)
	{
		case PREVIEWBUFFER:
			tmpmem = (*mPreviewBuffer[0]);
			break;
		case RAWBUFFER:
			tmpmem =mRawBuffer;
            
			break;
		case JPEGBUFFER:
			tmpmem =mJpegBuffer;
			break;
		default:
			break;
	}
    region.offset = tmpmem->offset()+offset;
	region.len = len;
	ret = ioctl(mPmemFd,PMEM_CACHE_FLUSH, &region);
    
    return ret;
}
/******************* pmem buffer end*****************/
#endif
}
