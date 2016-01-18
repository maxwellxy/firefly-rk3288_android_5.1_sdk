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

#ifndef FLAC_DECODER_H_

#define FLAC_DECODER_H_

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>


namespace android {

class  RK_FLACParser;
struct MediaBufferGroup;

struct FLACDecoder : public MediaSource {
   FLACDecoder(const sp<MediaSource> &source);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();
    int32_t checkFlacFrameHeader(uint8_t* buf, uint32_t size);

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options);

    int32_t mNumChannels;
    int32_t mSampleRate;
    int32_t mBitDepth;
    int32_t mMaxBlockSize;
    MediaBuffer *mInputBuffer;
    sp<MediaSource> mSource;

protected:
    virtual ~FLACDecoder();

private:
    sp<MetaData> mMeta;
    bool mStarted;
	uint32_t mInitFlag;
    void* mFLACDecExt;
    int64_t mAnchorTimeUs;
    int64_t mNumFramesOutput;

	MediaBuffer *mFirstOutputBuf;
    int64_t mReAsynTime;
    int64_t mReAsynThreshHold;

    RK_FLACParser* mRkFlacParser;

    void init();

    FLACDecoder(const FLACDecoder &);
    FLACDecoder &operator=(const FLACDecoder &);
};

}  // namespace android

#endif  // FLAC_DECODER_H_
