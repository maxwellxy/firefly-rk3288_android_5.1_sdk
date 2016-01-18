/*
*Author: zyc@rock-chips.com
*/
#define CAMERA_MEM_PMEM 0
#define CAMERA_MEM_ION  1
#define CAMERA_MEM_IONDMA  2
/* 
*NOTE: 
*   configuration macro 
*      
*/
#if  (defined(TARGET_RK32) || defined(TARGET_RK312x) || defined(TARGET_RK3188))
#define CONFIG_CAMERA_MEM               CAMERA_MEM_IONDMA
#elif	defined(TARGET_RK30)
#define CONFIG_CAMERA_MEM               CAMERA_MEM_ION
#else
#define CONFIG_CAMERA_MEM               CAMERA_MEM_PMEM
#endif
#if (CONFIG_CAMERA_MEM == CAMERA_MEM_ION)
#include <ion/IonAlloc.h>
#elif (CONFIG_CAMERA_MEM == CAMERA_MEM_PMEM)
#ifdef HAVE_ANDROID_OS 
#include <linux/android_pmem.h>
#include <binder/MemoryHeapPmem.h>
#endif

#elif (CONFIG_CAMERA_MEM == CAMERA_MEM_IONDMA)
#include <rockchip_ion.h>
#include <ion.h>
#endif
#include <binder/IMemory.h>
namespace android {

/*****************for unified memory manager  start*******************/
enum buffer_type_enum{
	PREVIEWBUFFER,
	RAWBUFFER,
	JPEGBUFFER,
	VIDEOENCBUFFER,

};
struct bufferinfo_s{
	unsigned int mNumBffers; //invaild if this value is 0
	size_t mPerBuffersize;
	size_t mBufferSizes;
	unsigned long mPhyBaseAddr;
	unsigned long mVirBaseAddr;
	unsigned long mShareFd;
	buffer_type_enum mBufType;
	bool        mIsForceIommuBuf;
};

typedef enum buffer_addr_e {
    buffer_addr_phy,
    buffer_addr_vir,    
	buffer_sharre_fd
}buffer_addr_t;

class MemManagerBase{
public :
	MemManagerBase();
	virtual ~MemManagerBase();
	virtual int createPreviewBuffer(struct bufferinfo_s* previewbuf) = 0;
	virtual int createRawBuffer(struct bufferinfo_s* rawbuf) = 0;
	virtual int createJpegBuffer(struct bufferinfo_s* jpegbuf) = 0;
	virtual int createVideoEncBuffer(struct bufferinfo_s* videoencbuf) = 0;

	virtual int destroyPreviewBuffer() = 0;
	virtual int destroyRawBuffer() = 0;
	virtual int destroyJpegBuffer() = 0;
	virtual int destroyVideoEncBuffer() = 0;
	virtual int flushCacheMem(buffer_type_enum buftype,unsigned int offset, unsigned int len) = 0;
	#if 0
	struct bufferinfo_s& getPreviewBufInfo(){
		return mPreviewBufferInfo;}
	struct bufferinfo_s& getRawBufInfo(){
		return mRawBufferInfo;}
	struct bufferinfo_s& getJpegBufInfo(){
		return mJpegBufferInfo;}
	struct bufferinfo_s& getVideoEncBufInfo(){
		return mVideoEncBufferInfo;}
	#endif
    unsigned int getBufferAddr(enum buffer_type_enum buf_type, unsigned int buf_idx, buffer_addr_t addr_type);
    int dump();
protected:
	struct bufferinfo_s* mPreviewBufferInfo;
	struct bufferinfo_s* mRawBufferInfo;
	struct bufferinfo_s* mJpegBufferInfo;
	struct bufferinfo_s* mVideoEncBufferInfo;
	mutable Mutex mLock;
};
#if (CONFIG_CAMERA_MEM == CAMERA_MEM_PMEM)
class PmemManager:public MemManagerBase{
	public :
		PmemManager(char* devpath);
		~PmemManager();
		virtual int createPreviewBuffer(struct bufferinfo_s* previewbuf);
		virtual int createRawBuffer(struct bufferinfo_s* rawbuf);
		virtual int createJpegBuffer(struct bufferinfo_s* jpegbuf);
		virtual int createVideoEncBuffer(struct bufferinfo_s* videoencbuf) ;

		virtual int destroyPreviewBuffer();
		virtual int destroyRawBuffer();
		virtual int destroyJpegBuffer();
		virtual int destroyVideoEncBuffer();
		virtual int flushCacheMem(buffer_type_enum buftype,unsigned int offset, unsigned int len) ;
		int initPmem(char* devpath);
		int deinitPmem();
	private:
		int mPmemFd;
		unsigned int mPmemSize; 
		unsigned int mPmemHeapPhyBase;
		sp<MemoryHeapBase> mMemHeap;
		sp<MemoryHeapBase> mMemHeapPmem;
		sp<IMemory> mJpegBuffer; 
		sp<IMemory> mRawBuffer;  
        sp<IMemory> **mPreviewBuffer;
};
#endif
#if (CONFIG_CAMERA_MEM == CAMERA_MEM_ION)
class IonMemManager:public MemManagerBase{
	public :
		IonMemManager();
		~IonMemManager();

		virtual int createPreviewBuffer(struct bufferinfo_s* previewbuf);
		virtual int createRawBuffer(struct bufferinfo_s* rawbuf);
		virtual int createJpegBuffer(struct bufferinfo_s* jpegbuf);
		virtual int createVideoEncBuffer(struct bufferinfo_s* videoencbuf) ;

		virtual int destroyPreviewBuffer();
		virtual int destroyRawBuffer();
		virtual int destroyJpegBuffer();
		virtual int destroyVideoEncBuffer();
		virtual int flushCacheMem(buffer_type_enum buftype,unsigned int offset, unsigned int len);
	private:
		int createIonBuffer(struct bufferinfo_s* ionbuf);
		void destroyIonBuffer(buffer_type_enum buftype);
		ion_buffer_t *mPreviewData;
		ion_buffer_t *mRawData;
		ion_buffer_t *mJpegData;
		ion_buffer_t *mVideoEncData;
		IonAlloc *mIonMemMgr;
};
#endif

#if (CONFIG_CAMERA_MEM == CAMERA_MEM_IONDMA)

typedef struct camera_ionbuf_s
{
    void* ion_hdl;
    int   map_fd;
    unsigned long vir_addr;
    unsigned long phy_addr;
    size_t size;
    int share_id;

}camera_ionbuf_t;


class IonDmaMemManager:public MemManagerBase{
	public :
		IonDmaMemManager(bool iommuEnabled);
		~IonDmaMemManager();

		virtual int createPreviewBuffer(struct bufferinfo_s* previewbuf);
		virtual int createRawBuffer(struct bufferinfo_s* rawbuf);
		virtual int createJpegBuffer(struct bufferinfo_s* jpegbuf);
		virtual int createVideoEncBuffer(struct bufferinfo_s* videoencbuf) ;

		virtual int destroyPreviewBuffer();
		virtual int destroyRawBuffer();
		virtual int destroyJpegBuffer();
		virtual int destroyVideoEncBuffer();
		virtual int flushCacheMem(buffer_type_enum buftype,unsigned int offset, unsigned int len);

		//map

		//unmap

		//share
	private:
		int createIonBuffer(struct bufferinfo_s* ionbuf);
		void destroyIonBuffer(buffer_type_enum buftype);
		camera_ionbuf_t* mPreviewData;
		camera_ionbuf_t* mRawData;
		camera_ionbuf_t* mJpegData;
		camera_ionbuf_t* mVideoEncData;
	    int client_fd;
	    bool mIommuEnabled;

};
#endif


/*****************for unified memory manager  end*******************/
}
