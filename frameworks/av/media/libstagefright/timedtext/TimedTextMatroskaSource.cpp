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

//#define LOG_NDEBUG 0
#define LOG_TAG "TimedTextMatroskaSource"
#include <utils/Log.h>

#include <binder/Parcel.h>
#include <media/stagefright/foundation/ADebug.h>  // CHECK_XX macro
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaDefs.h>  // for MEDIA_MIMETYPE_xxx
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#include "TimedTextMatroskaSource.h"
#include "TextDescriptions.h"

#define SEND_MKV_TIMED_TEXT_MIN_DELTA_US 500000

/** Returns a time value in milliseconds based on a clock starting at
 *  some arbitrary base. Given a call to GetTime that returns a value
 *  of n a subsequent call to GetTime made m milliseconds later should
 *  return a value of (approximately) (n+m). This method is used, for
 *  instance, to compute the duration of call. */
static int64_t GetSysTimeUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;

}


namespace android {

TimedTextMatroskaSource::TimedTextMatroskaSource(const sp<MediaSource>& mediaSource)
    : mSource(mediaSource),
      mPreGetFrmTimeUs(-1),
      mObserver(NULL){

      memset(&mTimedMkvSource, 0, sizeof(TimedMkvSource));

    const char *mime;
    if((mSource!=NULL) && mSource->getFormat()->findCString(kKeyMIMEType, &mime)) {

        if (strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_UTF8) == 0) {
            mTimedMkvSource.srcMimeType = MKV_TIMED_SRC_MIME_UTF8;
        } else if (strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_SSA) == 0) {
            mTimedMkvSource.srcMimeType = MKV_TIMED_SRC_MIME_SSA;
        } else if (strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_VOBSUB) == 0) {
            mTimedMkvSource.srcMimeType = MKV_TIMED_SRC_MIME_VOBSUB;
        } else {
            mTimedMkvSource.srcMimeType = MKV_TIMED_SRC_MIME_NONE;
        }
    }

}

TimedTextMatroskaSource::~TimedTextMatroskaSource() {
}

status_t TimedTextMatroskaSource::read(
        int64_t *startTimeUs, int64_t *endTimeUs, Parcel *parcel,
        const MediaSource::ReadOptions *options) {
    MediaBuffer *textBuffer = NULL;
    status_t err = mSource->read(&textBuffer, NULL);
    if (err != OK) {
        return err;
    }

    if (textBuffer == NULL) {
        /* let TextPlayer do read after post some time. */
        return WOULD_BLOCK;
    }

    if (textBuffer->range_length() ==0) {
        /* let TextPlayer do read after post some time. */
        textBuffer->release();

        return WOULD_BLOCK;
    }

    int64_t curSysTimeUs = GetSysTimeUs();

    if ((mPreGetFrmTimeUs >0) &&
            abs(curSysTimeUs - mPreGetFrmTimeUs) <SEND_MKV_TIMED_TEXT_MIN_DELTA_US) {

        /* skip this frame */
        textBuffer->release();

        return WOULD_BLOCK;
    }

    if (mPreGetFrmTimeUs == -1) {
        mPreGetFrmTimeUs = curSysTimeUs;
    }

    int32_t durMs = 0;
    *startTimeUs = 0;
    *endTimeUs = 0;

    textBuffer->meta_data()->findInt64(kKeyTime, startTimeUs);
    textBuffer->meta_data()->findInt32(kKeySubtitleDuration, &durMs);
    *endTimeUs = *startTimeUs + durMs*1000;

    CHECK_GE(*startTimeUs, 0);

    if ((mTimedMkvSource.srcMimeType == MKV_TIMED_SRC_MIME_VOBSUB) && mObserver) {
        mObserver->notifyObserver(MKV_TIMED_MSG_VOBSUB_GET, textBuffer);
    } else {
        extractAndAppendLocalDescriptions(*startTimeUs, textBuffer, parcel);
    }

    ALOGV("read one mkv text frame, size: %d, timeUs: %lld, durMs: %d",
        textBuffer->range_length(), *startTimeUs, durMs);
		
    mPreGetFrmTimeUs = curSysTimeUs;

    textBuffer->release();
    // endTimeUs is a dummy parameter for Matroska timed text format.
    // Set a negative value to it to mark it is unavailable.
    return OK;

}

// Each text sample consists of a string of text, optionally with sample
// modifier description. The modifier description could specify a new
// text style for the string of text. These descriptions are present only
// if they are needed. This method is used to extract the modifier
// description and append it at the end of the text.
status_t TimedTextMatroskaSource::extractAndAppendLocalDescriptions(
        int64_t timeUs, const MediaBuffer *textBuffer, Parcel *parcel) {
    const void *data;
    size_t size = 0;
    int32_t flag = TextDescriptions::LOCAL_DESCRIPTIONS;

    const char *mime;
    CHECK(mSource->getFormat()->findCString(kKeyMIMEType, &mime));
    CHECK((strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_UTF8) == 0) ||
			(strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_SSA) == 0) ||
			(strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_VOBSUB) == 0));

    data = textBuffer->data();
    size = textBuffer->size();

    if (size > 0) {
      parcel->freeData();
      flag |= TextDescriptions::IN_BAND_TEXT_MATROSKA;
      return TextDescriptions::getParcelOfDescriptions(
          (const uint8_t *)data, size, flag, timeUs / 1000, parcel);
    }
    return OK;
}

// To extract and send the global text descriptions for all the text samples
// in the text track or text file.
// TODO: send error message to application via notifyListener()...?
status_t TimedTextMatroskaSource::extractGlobalDescriptions(Parcel *parcel) {
    const void *data;
    size_t size = 0;
    int32_t flag = TextDescriptions::GLOBAL_DESCRIPTIONS;

    const char *mime;
    CHECK(mSource->getFormat()->findCString(kKeyMIMEType, &mime));
    CHECK((strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_UTF8) == 0) ||
			(strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_SSA) == 0) ||
			(strcasecmp(mime, MEDIA_MIMETYPE_TEXT_MATROSKA_VOBSUB) == 0));

    return OK;
}

sp<MetaData> TimedTextMatroskaSource::getFormat() {
    return mSource->getFormat();
}

void TimedTextMatroskaSource::setTimedTextSourceObserver(void* obServer) {
    mObserver = (TimedTextPlayer*)obServer;
}

}  // namespace android
