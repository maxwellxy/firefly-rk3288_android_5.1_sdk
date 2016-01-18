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

#ifndef MPEG2_TS_EXTRACTOR_H_

#define MPEG2_TS_EXTRACTOR_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/MediaExtractor.h>
#include <utils/threads.h>
#include <utils/Vector.h>

#define DVB_ENABLE 1
#if DVB_ENABLE
typedef int (*DvbGetDataFunc)(void * buffer,size_t requestLen);
typedef int (*DvbGetInfoFunc)(int *video_data_pid, int *audio_data_pid, int *video_stream_type, int *audio_stream_type);
typedef int (*DvbFreeInfoFunc)();
typedef bool(*DvbGetExitFunc)();
#endif//DVB_ENABLE


namespace android {

#define TS_FEC_PACKET_SIZE 204
#define TS_DVHS_PACKET_SIZE 192
#define TS_PACKET_SIZE 188
#define TS_MAX_PACKET_SIZE 204
#define TS_MAX_PACKET 5000
#define TV_TAG "ROCKCHIP_TV"
#define WIMO_TAG "RK_WIMO"
#define GPU_STRM_TAG "RK_GPU_STRM"
#define TVPAD_TAG "TVPAD"
struct AMessage;
struct AnotherPacketSource;
struct ATSParser;
struct DataSource;
struct MPEG2TSSource;
struct String8;
struct MPEG2TSDataSource;


struct MPEG2TSExtractor : public MediaExtractor {
    enum AppType {
        NONE               = 0,
        HTTP_LIVE         = 1,
        M3U8               = 2,
        WIMO               = 3,
        LIVE_TV           = 4,
		TVPAD		        = 5,
		GPU_STRM			= 6,
    };
    MPEG2TSExtractor(const sp<DataSource> &source,AppType type = NONE);

    virtual size_t countTracks();
    virtual sp<MediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

    virtual uint32_t flags() const;

    void seekTo(int64_t seekTimeUs);

protected:
   virtual ~MPEG2TSExtractor();
private:
    friend struct MPEG2TSSource;

    mutable Mutex mLock;

    sp<MPEG2TSDataSource> mDataSource;

    sp<ATSParser> mParser;

    Vector<sp<AnotherPacketSource> > mSourceImpls;

    pthread_t	mThread;
	bool		mThread_sistarted;
    bool run;
    bool stopflag;
    bool startSeek;
    off64_t mOffset;
    AppType mType;
    bool mWaitFlag;
    unsigned init_flag;
    off64_t FirstPackoffset;
    size_t kTSPacketSize;
    uint64_t mSeekSize;
    off64_t Totalsize;
    int32_t packetNum;
    int32_t Lastpackt;
    bool   endEos;
    bool  threadout;
    bool mAudioSelect;
    bool hasAudioPlayFlag;
    bool _success;
    uint8_t *packet;
    void init();
    void start();
    void stop();
    unsigned mAudioPID;
    unsigned mVideoPID;
    off64_t FirstPackfound(off64_t mOffset);
    int64_t lastseektimeus;
    status_t feedMore();
    uint32_t seekFlag;
    static void* Threadproc(void *me);
    int32_t FileProcess();
#if DVB_ENABLE
	void * mDvbLibHandle;
	DvbGetDataFunc dvbGetData;
	DvbGetInfoFunc dvbGetInfo;
	DvbFreeInfoFunc dvbFreeInfo;
	DvbGetExitFunc dvbGetExit;
#endif//DVB_ENABLE
    DISALLOW_EVIL_CONSTRUCTORS(MPEG2TSExtractor);
};

bool SniffMPEG2TS(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *);

}  // namespace android

#endif  // MPEG2_TS_EXTRACTOR_H_
