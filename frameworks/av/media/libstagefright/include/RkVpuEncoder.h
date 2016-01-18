/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef RKVPU_ENCODER_H_

#define RKVPU_ENCODER_H_

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>
#include "vpu_api.h"


namespace android {

struct MediaBuffer;
struct MediaBufferGroup;

struct RkVpuEncoder : public MediaSource,
                    public MediaBufferObserver {
    RkVpuEncoder(const sp<MediaSource> &source,
            const sp<MetaData>& meta);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

	status_t get_encoder_param(void *param);
	status_t set_encoder_param(void *param);

	void Set_IDR_Frame();
	void Set_IDR_Interval(int64_t interval_time);
	void Set_Flag(int flag,int cabac_flag);

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options);

    virtual void signalBufferReturned(MediaBuffer *buffer);

protected:
    virtual ~RkVpuEncoder();

private:
    sp<MediaSource> mSource;
    sp<MetaData>    mFormat;
    sp<MetaData>    mMeta;

    OMX_RK_VIDEO_CODINGTYPE mCodecId;

    VpuCodecContext_t *mVpuCtx;

    int32_t  wimo_flag;
    int32_t  mVideoWidth;
    int32_t  mVideoHeight;
    int32_t  mVideoFrameRate;
    int32_t  mVideoBitRate;
    int32_t  mVideoColorFormat;
	int32_t	 mCabac_flag;       //0=OFF, 1=ON [0]\n"
    int32_t  mCabacInitIdc;      //0..2\n");
    int64_t  mNumInputFrames;
    int64_t  mPrevTimestampUs;
    int64_t  mPrev_IDR_TimestampUs;
    status_t mInitCheck;
    bool     mStarted;
    bool     mSpsPpsHeaderReceived;
    bool     mReadyForNextFrame;
    int32_t  mIsIDRFrame;  // for set kKeyIsSyncFrame
    bool     skipFlag;
    int64_t  totaldealt;
	int64_t  mIDR_Interval_time;
    int32_t  mFramesIntervalSec;
    int32_t  mLevel;
    int32_t  mProfile;

    MediaBuffer           *mInputBuffer;
    uint8_t               *mInputFrameData;
    MediaBufferGroup      *mGroup;
    Vector<MediaBuffer *> mOutputBuffers;


    status_t initCheck(const sp<MetaData>& meta);
    void releaseOutputBuffers();

    RkVpuEncoder(const RkVpuEncoder &);
    RkVpuEncoder &operator=(const RkVpuEncoder &);

};

}  // namespace android

#endif  // RKVPU_ENCODER_H_
