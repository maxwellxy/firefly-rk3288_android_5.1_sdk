//#define LOG_NDEBUG 0
#define LOG_TAG "RepeaterSource"
#include <utils/Log.h>

#include "RepeaterSource.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MetaData.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <fcntl.h>
#include <inttypes.h>
#include "rga.h"
#include "gralloc_priv.h"

namespace android {
//#define TIMEUS_FOR_TEST 1
//#define RGBA_TEST_FILE
#define ASYNC_RGA 1
#define SURFACE_ORIGINAL_SIZE 1

/* Using for the repeatersource rate statistic*/
//#define REPEATERSOURCE_RATE_STATISTIC
#ifdef REPEATERSOURCE_RATE_STATISTIC
#define STATISTIC_PER_TIME 5  // statistic once per 5s
static int64_t lastRepeaterSourceTime = 0;
static int64_t currentRepeaterSourceTime = 0;
static int32_t lastRepeaterSourceFrameCount = 0;
static int32_t currentRepeaterSourceFrameCount = 0;
#endif

FILE* omx_txt;

RepeaterSource::RepeaterSource(const sp<MediaSource> &source, double rateHz)
    : mStarted(false),
      mSource(source),
      mRateHz(rateHz),
      mBuffer(NULL),
      mResult(OK),
      mLastBufferUpdateUs(-1ll),
      mStartTimeUs(-1ll),
      mFrameCount(0),
      maxbuffercount(1),
      rga_fd(-1),
      vpu_mem_index(0),
      mNumPendingBuffers(0),
      mWidth(0),
      mHeight(0),
      mRGBAFile(NULL) {
#ifdef RGBA_TEST_FILE
      mRGBAFile = fopen("/data/misc/test.rgba", "wb");
#endif
}

RepeaterSource::~RepeaterSource() {
    CHECK(!mStarted);
#ifdef RGBA_TEST_FILE
    if (mRGBAFile != NULL) {
        fclose(mRGBAFile);
        mRGBAFile = NULL;
    }
#endif
}

double RepeaterSource::getFrameRate() const {
    return mRateHz;
}

void RepeaterSource::setFrameRate(double rateHz) {
    Mutex::Autolock autoLock(mLock);

    if (rateHz == mRateHz) {
        return;
    }

    if (mStartTimeUs >= 0ll) {
        int64_t nextTimeUs = mStartTimeUs + (mFrameCount * 1000000ll) / mRateHz;
        mStartTimeUs = nextTimeUs;
        mFrameCount = 0;
    }
    mRateHz = rateHz;
}

status_t RepeaterSource::start(MetaData *params) {
    CHECK(!mStarted);
#ifdef REPEATERSOURCE_RATE_STATISTIC
    lastRepeaterSourceTime = 0;
    currentRepeaterSourceTime = 0;
    lastRepeaterSourceFrameCount = 0;
    currentRepeaterSourceFrameCount = 0;
#endif
    status_t err = mSource->start(params);

    if (err != OK) {
        return err;
    }

#if ASYNC_RGA
    CHECK(mSource->getFormat()->findInt32(kKeyWidth, &mWidth));
    CHECK(mSource->getFormat()->findInt32(kKeyHeight, &mHeight));
    ALOGI("mWidth=%d, mHeight=%d", mWidth, mHeight);
    rga_fd  = open("/dev/rga",O_RDWR,0);
    if(rga_fd < 0)
    {
        ALOGE("Rga device open failed!");
        return rga_fd;
    }

    for(int i = 0; i < maxbuffercount; i++) {
        vpuenc_mem[i] = (VPUMemLinear_t*)malloc(sizeof( VPUMemLinear_t));
        ALOGD("mWidth %d mHeight %d", mWidth, mHeight);
        err = VPUMallocLinear((VPUMemLinear_t*)vpuenc_mem[i], ((mWidth + 15) & 0xfff0) * mHeight * 4);
        if (err) {
            ALOGD("err  %dtemp->phy_addr %x mWidth %d mHeight %d", err, ((VPUMemLinear_t*)vpuenc_mem[i])->phy_addr, mWidth, mHeight);
            return err;
        }
    }
    mUsingTimeUs = mCurTimeUs = 0;
#endif

    mBuffer = NULL;
    mResult = OK;
    mStartTimeUs = -1ll;
    mFrameCount = 0;

    mLooper = new ALooper;
    mLooper->setName("repeater_looper");
    mLooper->start();

    mReflector = new AHandlerReflector<RepeaterSource>(this);
    mLooper->registerHandler(mReflector);

    postRead();

    mStarted = true;

    return OK;
}

status_t RepeaterSource::stop() {
    CHECK(mStarted);

    ALOGV("stopping");

    status_t err = mSource->stop();

    if (mLooper != NULL) {
        mLooper->stop();
        mLooper.clear();

        mReflector.clear();
    }

    if (mBuffer != NULL) {
        ALOGV("releasing mbuf %p", mBuffer);
        mBuffer->release();
        mBuffer = NULL;
    }


    ALOGV("stopped");

    mStarted = false;
    {
        Mutex::Autolock autoLock(mLock);
        while (mNumPendingBuffers > 0) {
            mMediaBuffersAvailableCondition.wait(mLock);
        }
        mMediaBuffersAvailableCondition.broadcast();
    }

#if ASYNC_RGA
    for (int i = 0; i < maxbuffercount; i++) {
        if (vpuenc_mem[i] != NULL) {
            VPUFreeLinear((VPUMemLinear_t*)vpuenc_mem[i]);
            free((VPUMemLinear_t*)vpuenc_mem[i]);
        }
    }
    if (rga_fd > 0) {
        close(rga_fd);
        rga_fd = -1;
    }
#endif

    return err;
}

sp<MetaData> RepeaterSource::getFormat() {
    return mSource->getFormat();
}

status_t RepeaterSource::read(
        MediaBuffer **buffer, const ReadOptions *options) {
    int64_t seekTimeUs;
    ReadOptions::SeekMode seekMode;

    CHECK(options == NULL || !options->getSeekTo(&seekTimeUs, &seekMode));

    for (;;) {
        int64_t bufferTimeUs = -1ll;
#ifdef TIMEUS_FOR_TEST
        int64_t frameBufferTimeUs = -1ll;
#endif
        
        if (mStartTimeUs < 0ll) {
            Mutex::Autolock autoLock(mLock);
            while ((mLastBufferUpdateUs < 0ll || mBuffer == NULL)
                    && mResult == OK) {
                mCondition.wait(mLock);
            }

            ALOGV("now resuming.");
            mStartTimeUs = ALooper::GetNowUs();
            bufferTimeUs = mStartTimeUs;
        } else {
            bufferTimeUs = mStartTimeUs + (mFrameCount * 1000000ll) / mRateHz;

            int64_t nowUs = ALooper::GetNowUs();
            int64_t delayUs = bufferTimeUs - nowUs;

            if (delayUs > 0ll) {
                usleep(delayUs);
            }
        }

        bool stale = false;

        {
            Mutex::Autolock autoLock(mLock);
            if (mResult != OK) {
                CHECK(mBuffer == NULL);
                return mResult;
            }

#if SUSPEND_VIDEO_IF_IDLE
            int64_t nowUs = ALooper::GetNowUs();
            if (nowUs - mLastBufferUpdateUs > 1000000ll) {
                mLastBufferUpdateUs = -1ll;
                stale = true;
            } else
#endif
#if ASYNC_RGA
            {
                mBuffer->add_ref();
                *buffer = new MediaBuffer(24);
                char *data = (char *)(*buffer)->data();
                uint32_t temp = 0x1234;
                buffer_handle_t handle = (buffer_handle_t)*((long*)(mBuffer->data()+4));
				memcpy(data, &mBuffer, 4);
                memcpy(data + 4, &temp , 4);
                memcpy(data + 8, &vpuenc_mem[vpu_mem_index], 4);
                memcpy(data + 12, &rga_fd, 4);
                memcpy(data + 16, &handle, 4);
                private_handle_t *mHandle = (private_handle_t*)handle;
                memcpy(data + 20, &(mHandle->share_fd), 4);
#ifdef RGBA_TEST_FILE
                if(mCurTimeUs != mUsingTimeUs) {
                    const Rect rect(mWidth, mHeight);
                    uint8_t *img=NULL;
                    int res = GraphicBufferMapper::get().lock(handle,
                                GRALLOC_USAGE_SW_READ_MASK,//GRALLOC_USAGE_HW_VIDEO_ENCODER,
                                rect, (void**)&img);

                    if (res != OK) {
                        ALOGE("%s: Unable to lock image buffer %p for access", __FUNCTION__,
                            *((long*)(mBuffer->data()+16)));
                        GraphicBufferMapper::get().unlock(handle);
                        return res;
                    } else {
                        if (mRGBAFile != NULL) {
                            fwrite(img, 1, mWidth * mHeight * 4, mRGBAFile);
                        }
                    }
                    GraphicBufferMapper::get().unlock(handle);
                    mUsingTimeUs = mCurTimeUs;
                }
#endif
                (*buffer)->setObserver(this);
	            (*buffer)->add_ref();
                mNumPendingBuffers++;
                if(mNumPendingBuffers>maxbuffercount)
                    mNumPendingBuffers=maxbuffercount;
                vpu_mem_index++;
                vpu_mem_index %= maxbuffercount;
                (*buffer)->meta_data()->setInt64(kKeyTime, bufferTimeUs);
#ifdef TIMEUS_FOR_TEST
                mBuffer->meta_data()->findInt64(kKeyTime, &frameBufferTimeUs);
                ALOGD("RepeaterSource Video  bufferTimeUs %" PRId64 "  frameBufferTimeUs  %" PRId64 "  delta  %" PRId64,
                      bufferTimeUs, frameBufferTimeUs, (bufferTimeUs - frameBufferTimeUs));
#endif
                ++mFrameCount;
            }
#else
            {
                mBuffer->add_ref();
                *buffer = mBuffer;
                (*buffer)->meta_data()->setInt64(kKeyTime, bufferTimeUs);
                ++mFrameCount;
            }
#endif
        }

        if (!stale) {
#ifdef REPEATERSOURCE_RATE_STATISTIC
            currentRepeaterSourceTime = ALooper::GetNowUs();
            if(lastRepeaterSourceTime != 0) {
                ++currentRepeaterSourceFrameCount;
                if(currentRepeaterSourceTime - lastRepeaterSourceTime >= (STATISTIC_PER_TIME * 1000000)) {
				    ALOGE("Statistic RepeaterSource Rate %d lastEncodeFrameCount %d currentEncodeFrameCount %d", ((currentRepeaterSourceFrameCount - lastRepeaterSourceFrameCount) / STATISTIC_PER_TIME), lastRepeaterSourceFrameCount, currentRepeaterSourceFrameCount);
                    lastRepeaterSourceTime = currentRepeaterSourceTime;
                    lastRepeaterSourceFrameCount = currentRepeaterSourceFrameCount;
                }
            }
            else
                lastRepeaterSourceTime = currentRepeaterSourceTime;
#endif
            break;
        }

        mStartTimeUs = -1ll;
        mFrameCount = 0;
        ALOGV("now dormant");
    }

    return OK;
}

void RepeaterSource::postRead() {
    (new AMessage(kWhatRead, mReflector->id()))->post();
}

void RepeaterSource::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatRead:
        {
            MediaBuffer *buffer;
            status_t err = mSource->read(&buffer);

            ALOGV("read mbuf %p", buffer);

            Mutex::Autolock autoLock(mLock);
            if (mBuffer != NULL) {
                mBuffer->release();
                mBuffer = NULL;
            }
            mBuffer = buffer;
            mResult = err;
            mLastBufferUpdateUs = ALooper::GetNowUs();
#if ASYNC_RGA
            mCurTimeUs = mLastBufferUpdateUs;
#endif

            mCondition.broadcast();

            if (err == OK) {
                postRead();
            }
            break;
        }

        default:
            TRESPASS();
    }
}

void RepeaterSource::signalBufferReturned(MediaBuffer *buffer) {
    Mutex::Autolock autoLock(mLock);

	MediaBuffer *surfaceMediaBuffer;
	memcpy(&surfaceMediaBuffer, (char*)(buffer->data()), sizeof(MediaBuffer*));
	surfaceMediaBuffer->release();
	
    buffer->setObserver(0);
    buffer->release();
    --mNumPendingBuffers;
    mMediaBuffersAvailableCondition.broadcast();
	
    int	retrtptxt;
    if((retrtptxt = access("data/test/omx_txt_file",0)) == 0)
    {
        int64_t	sys_time = systemTime(SYSTEM_TIME_MONOTONIC) / 1000;		
        if(omx_txt == NULL)
            omx_txt = fopen("data/test/omx_txt.txt","ab");
        if(omx_txt != NULL)
        {
            fprintf(omx_txt,"RepeaterSource signalBufferReturned sys_time %lld mNumPendingBuffers %d\n",
                sys_time,mNumPendingBuffers);
            fflush(omx_txt);
        }
    }
}

void RepeaterSource::wakeUp() {
    ALOGV("wakeUp");
    Mutex::Autolock autoLock(mLock);
    if (mLastBufferUpdateUs < 0ll && mBuffer != NULL) {
        mLastBufferUpdateUs = ALooper::GetNowUs();
        mCondition.broadcast();
    }
}

}  // namespace android
