/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef TIMED_TEXT_MATROSKA_SOURCE_H_
#define TIMED_TEXT_MATROSKA_SOURCE_H_

#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>

#include "TimedTextSource.h"
#include "TimedTextPlayer.h"

namespace android {

/* must sync with FrameQueueManage.h*/
typedef enum MkvTimedMsgType {
    // 0xx
    MKV_TIMED_MSG_UNKNOWN = -1,
    // 1xx
    MKV_TIMED_MSG_VOBSUB_FLAG = 1,
    // 2xx
    MKV_TIMED_MSG_VOBSUB_GET = 2,
}MkvTimedMsgType;

typedef enum {
    MKV_TIMED_SRC_MIME_NONE,
    MKV_TIMED_SRC_MIME_UTF8,
    MKV_TIMED_SRC_MIME_SSA,
    MKV_TIMED_SRC_MIME_VOBSUB

} MkvTimedSrcMime;

typedef struct TimedMkvSource{
    MkvTimedSrcMime srcMimeType;

}TimedMkvSource;

class MediaBuffer;
class Parcel;

class TimedTextMatroskaSource : public TimedTextSource {
public:
    TimedTextMatroskaSource(const sp<MediaSource>& mediaSource);
    virtual status_t start() { return mSource->start(); }
    virtual status_t stop() { return mSource->stop(); }
    virtual status_t read(
            int64_t *startTimeUs,
            int64_t *endTimeUs,
            Parcel *parcel,
            const MediaSource::ReadOptions *options = NULL);
    virtual status_t extractGlobalDescriptions(Parcel *parcel);
    virtual sp<MetaData> getFormat();

    void setTimedTextSourceObserver(void* obServer);

protected:
    virtual ~TimedTextMatroskaSource();

private:
    sp<MediaSource> mSource;
    int64_t mPreGetFrmTimeUs;

    TimedTextPlayer* mObserver;

    TimedMkvSource  mTimedMkvSource;

    status_t extractAndAppendLocalDescriptions(
            int64_t timeUs, const MediaBuffer *textBuffer, Parcel *parcel);

    DISALLOW_EVIL_CONSTRUCTORS(TimedTextMatroskaSource);
};

}  // namespace android

#endif  // TIMED_TEXT_MATROSKA_SOURCE_H_
