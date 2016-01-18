/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RKVPU_DECODER_H_

#define RKVPU_DECODER_H_

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>
#include "vpu_api.h"
#include "vpu_mem_pool.h"

typedef enum RkDecoderMap {

    FIRST_FRAME         =   0x01,
    HAVE_SEEK_REQUEST   =   0x02,
    MBAFF_MODE_INFO_CHANGE = 0x04,
};

typedef struct RkDecoderSeekRequest {
    int64_t seekTimeUs;
    android::MediaSource::ReadOptions::SeekMode mode;
}RkDecoderSeekRequest_t;

typedef struct RkDecoderPrivate{
    int32_t flags;
    RkDecoderSeekRequest_t seek_req ;
}RkDecPrivate_t;

namespace android {

struct RkVpuDecoder : public MediaSource,
                    public MediaBufferObserver {
    RkVpuDecoder(const sp<MediaSource> &source);

    virtual status_t start(MetaData *params);

    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options);

    virtual void signalBufferReturned(MediaBuffer *buffer);

    void SetParameterForWimo(const sp<MediaSource> &source);

    int32_t checkVideoInfoChange(void* aFrame);
protected:
    virtual ~RkVpuDecoder();

private:

    int getExtraData(int code_mode);

    status_t keyDataProcess();
    OMX_RK_VIDEO_CODINGTYPE mCodecId;
    VpuCodecContext_t *mVpuCtx;
    sp<MediaSource> mSource;
    bool mStarted;
    sp<MetaData> mFormat;
    MediaBuffer *mInputBuffer;
    int32_t mNumFramesOutput;
    int64_t mPendingSeekTimeUs;
    MediaSource::ReadOptions::SeekMode mPendingSeekMode;
    int64_t mTargetTimeUs;
	int32_t mWidth;
	int32_t mHeight;
    bool _success;

    int32_t mUseDtsTimeFlag;
    uint8_t *mExtraData;
    int32_t  mExtraDataSize;

    uint32_t getwhFlg;
    RkDecPrivate_t mRkDecPrivate;
    vpu_display_mem_pool        *mPool;
    RkVpuDecoder(const RkVpuDecoder &);
    RkVpuDecoder &operator=(const RkVpuDecoder &);
};

}  // namespace android

#endif  //RKVPU_DECODER_H_
