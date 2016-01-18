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

//#define LOG_DEBUG
//#define LOG_NDEBUG 0

#define LOG_TAG "MPEG2TSExtractor"
#ifdef SEEK_LOG_DEBUG
    #define LOGSEEK ALOGD
#else
    #define LOGSEEK
#endif
#ifdef MY_LOG_DEBUG
    #define MY_LOGD ALOGD
#else
    #define MY_LOGD
#endif
#include <utils/Log.h>

#include "include/MPEG2TSExtractor.h"
#include "include/NuCachedSource2.h"

#include <media/stagefright/DataSource.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBuffer.h>
#include <utils/String8.h>

#include "AnotherPacketSource.h"
#include "ATSParser.h"
#include <dlfcn.h>

#define MAX_AUDIO_TRACK 10

#if DVB_ENABLE
//Function name for extractor dvb function. Extended extractor must export a function with this name.
#define DVB_LIB_NAME  "libdvbstream.so"
#define DVB_GET_DATA  "dvb_get_ts_stream_data"
#define DVB_GET_INFO  "dvb_get_ts_stream_info"
#define DVB_FREE_INFO  "dvb_free_ts_stream_info"
#define DVB_GET_EXIT   "dvb_get_exit_flag_status"
#endif//DVB_ENABLE

namespace android {

typedef struct
{
    uint16_t    FormatTag;
    uint16_t    Channels;
    uint32_t    SamplesPerSec;
    uint32_t    AvgBytesPerSec;
    uint16_t    BlockAlign;
    uint16_t    BitsPerSample;
    uint16_t    Size;
    uint16_t    SamplesPerBlock;

}WaveFormatExStruct;

static void total_analyze(const uint8_t *buf, int size, int *score_0, int *score_1, int *score_2){
    int i;
    int stats[TS_MAX_PACKET_SIZE*3];
    int *stat_0         = &stats[0];
    int *stat_1         = &stats[TS_MAX_PACKET_SIZE];
    int *stat_2         = &stats[TS_MAX_PACKET_SIZE*2];
    int x_0             = 0;
    int x_1             = 0;
    int x_2             = 0;
    int best_score_0    = 0;
    int best_score_1    = 0;
    int best_score_2    = 0;
    int size_0          = TS_PACKET_SIZE;
    int size_1          = TS_DVHS_PACKET_SIZE;
    int size_2          = TS_FEC_PACKET_SIZE;
    memset(stats, 0, sizeof(stats));
    for(x_0=x_1=x_2=i=0; i<size-3; i++){
        if(buf[i] == 0x47 && !(buf[i+1] & 0x80) && (buf[i+3] & 0x30)){
            stat_0[x_0]++;
            stat_1[x_1]++;
            stat_2[x_2]++;
            if(stat_0[x_0] > best_score_0){
                best_score_0 = stat_0[x_0];
            }
            if(stat_1[x_1] > best_score_1){
                best_score_1 = stat_1[x_1];
            }
            if(stat_2[x_2] > best_score_2){
                best_score_2 = stat_2[x_2];
            }
        }
        x_0++;
        if(x_0 == size_0) x_0= 0;
        x_1++;
        if(x_1 == size_1) x_1= 0;
        x_2++;
        if(x_2 == size_2) x_2= 0;
    }
    *score_0 = best_score_0;
    *score_1 = best_score_1;
    *score_2 = best_score_2;
}
static int get_packet_size(const uint8_t *buf, int size)
{
    int score, fec_score, dvhs_score;
    int recsocre = 0;
    if (size < (TS_FEC_PACKET_SIZE * 5 + 1))
        return -1;
    total_analyze(buf, size, &score, &dvhs_score, &fec_score);
    MY_LOGD("score: %d, dvhs_score: %d, fec_score: %d \n", score, dvhs_score, fec_score);
    if(score > fec_score && score > dvhs_score && score > 20)
    {
        return TS_PACKET_SIZE;
    }
    else if(dvhs_score > score && dvhs_score > fec_score && dvhs_score > 20)
    {
        return TS_DVHS_PACKET_SIZE;
    }
    else if(score < fec_score && dvhs_score < fec_score && fec_score > 20)
    {
        return TS_FEC_PACKET_SIZE;
    }
    else
    {
        return -1;
    }
}

struct MPEG2TSSource : public MediaSource {
    MPEG2TSSource(
            const sp<MPEG2TSExtractor> &extractor,
            const sp<AnotherPacketSource> &impl,
            bool seekable);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);

private:
    sp<MPEG2TSExtractor> mExtractor;
    sp<AnotherPacketSource> mImpl;
    status_t AudioTrackSelectRead(MediaBuffer **out,const ReadOptions *options);
    status_t WavAddHeader(MediaBuffer **out,MediaBuffer *mBuffer);
    status_t MpegtsSeekProcess(const ReadOptions *options,status_t *finalResult);
    status_t AudioSourceRead(MediaBuffer **out,const ReadOptions *options);
    bool isPCM;
    bool isFristFrame;

    // If there are both audio and video streams, only the video stream
    // will be seekable, otherwise the single stream will be seekable.
    bool mSeekable;

    DISALLOW_EVIL_CONSTRUCTORS(MPEG2TSSource);
};

struct MPEG2TSDataSource : public DataSource {
    MPEG2TSDataSource(const sp<DataSource> &source);
    virtual status_t initCheck() const;
    virtual ssize_t readAt(off64_t offset, void *data, size_t size);
    virtual status_t getSize(off64_t *size);
    virtual void updatecache(off64_t offset);
protected:
    virtual ~MPEG2TSDataSource();
private:
    Mutex mLock;
    sp<DataSource> mSource;
    MPEG2TSDataSource(const MPEG2TSDataSource &);
    MPEG2TSDataSource &operator=(const MPEG2TSDataSource &);
};
MPEG2TSDataSource::MPEG2TSDataSource(const sp<DataSource> &source)
    : mSource(source){
}
void MPEG2TSDataSource::updatecache(off64_t offset)
{
    mSource->updatecache(offset);
}
status_t MPEG2TSDataSource::getSize(off64_t *size)
{
    return mSource->getSize(size);
}
status_t MPEG2TSDataSource::initCheck() const {
    return mSource->initCheck();
}
ssize_t MPEG2TSDataSource::readAt(off64_t offset, void *data, size_t size) {
    return mSource->readAt(offset, data, size);
}
MPEG2TSDataSource::~MPEG2TSDataSource() {
}
MPEG2TSSource::MPEG2TSSource(
        const sp<MPEG2TSExtractor> &extractor,
        const sp<AnotherPacketSource> &impl,
        bool seekable)
    : mExtractor(extractor),
      mImpl(impl),
      isPCM(false),
      isFristFrame(false),
      mSeekable(seekable) {
      const char *mime;
      sp<MetaData> meta = impl->getFormat();
      CHECK(meta->findCString(kKeyMIMEType, &mime));
      if (!strcasecmp("audio/wav", mime)) {
          ALOGE("set pcm audio");
          isPCM = true;
      }
}

status_t MPEG2TSSource::start(MetaData *params) {
    mExtractor->mParser->Start(mExtractor->mAudioPID,mExtractor->mVideoPID);
    if(mImpl->mIsVideo && !mExtractor->mType)
    {
        mExtractor->start();
        if(!mExtractor->run)
        {
            mExtractor->run = true;
            int32_t minCount = 0;
            int32_t maxCount = 0;
            while(minCount < 24)
            {
                int Count = 25;
                int memUsed = 0;
                if(mExtractor->endEos){
                    break;
                }
                for(int i = 0; i < mExtractor->mSourceImpls.size(); i++){
                    int num = mExtractor->mSourceImpls.editItemAt(i)->numBufferAvailable(&memUsed);
                    if(memUsed > 8*1024*1024){
                        break;
                    }
                    int32_t mCurrentPID = mExtractor->mSourceImpls.editItemAt(i)->mElementaryPID;
                    if((mExtractor->mAudioPID == mCurrentPID)||(mExtractor->mVideoPID == mCurrentPID)){
                     if(num < Count){
                        Count = num;
                        }
                    }
                }
                minCount = Count;
            }

            for(int i = 0; i < mExtractor->mSourceImpls.size(); i++){
                 int num = mExtractor->mSourceImpls.editItemAt(i)->numBufferAvailable();
                int32_t mCurrentPID = mExtractor->mSourceImpls.editItemAt(i)->mElementaryPID;
                if((mExtractor->mAudioPID == mCurrentPID)||(mExtractor->mVideoPID == mCurrentPID)){
                 if(num > maxCount){
                    maxCount = num;
                    }
                }
            }
            if((maxCount - minCount) > minCount){
                mExtractor->mWaitFlag = true;
            }
        }
    }
    if(!mImpl->mIsVideo)
    {
        if(mExtractor->mAudioPID != mImpl->mElementaryPID)
        {
            mExtractor->mAudioPID = mImpl->mElementaryPID;
            mExtractor->mAudioSelect = true;
        }
        mExtractor->hasAudioPlayFlag = true;
    }
    return mImpl->start(params);
}

status_t MPEG2TSSource::stop() {
     if(mImpl->mIsVideo)
     {
        if(mExtractor->run && !mExtractor->mType)
        {
             mExtractor->stopflag = true;
             mExtractor->run = false;
             while(!mExtractor->threadout)
             {
                usleep(2000);
             }
             mExtractor->stop();
        }

		if(mExtractor->mType == MPEG2TSExtractor::LIVE_TV)
		{
			mExtractor->stopflag = true;
			mExtractor->run = false;
		}
     }
     return OK;
}

sp<MetaData> MPEG2TSSource::getFormat() {
    sp<MetaData> meta = mImpl->getFormat();
    return meta;
}
status_t MPEG2TSSource::AudioTrackSelectRead(MediaBuffer **out,const ReadOptions *options){
    ALOGV("mAudioSelect found it");
    int64_t seekTimeUs;
    ReadOptions::SeekMode seekMode;
    int64_t timeUs  = 0;
    status_t finalResult;
    if(options && options->getSeekTo(&seekTimeUs, &seekMode))
    {
        ALOGV("seekTimeUs = %lld",seekTimeUs);
        while(mImpl->hasBufferAvailable(&finalResult))
        {
            MediaBuffer *mSteambuf = NULL;
            mImpl->read(&mSteambuf, options);
            if(mSteambuf->meta_data()->findInt64(kKeyTime, &timeUs))
            {
                ALOGV("timeUs = %lld",timeUs);
                if(timeUs < seekTimeUs){
                    mSteambuf->release();
                    mSteambuf = NULL;
                }else{
                    mExtractor->mAudioSelect = false;
                    if(isPCM && !isFristFrame)
                        WavAddHeader(out,mSteambuf);
                    else
                        *out = mSteambuf;
                    return OK;
                }
            }
        }
    }
    return OK;
}
status_t MPEG2TSSource::AudioSourceRead(MediaBuffer **out,const ReadOptions *options){
    MediaBuffer *mBuffer = NULL;
    status_t finalResult;
    int64_t mAudioTimeUs = 0;
    mImpl->read(&mBuffer, options);
    mBuffer->meta_data()->findInt64(kKeyTime, &mAudioTimeUs);
    for(int i = 0; i < mExtractor->mSourceImpls.size(); i++){
        sp<AnotherPacketSource> mTImpl = mExtractor->mSourceImpls.editItemAt(i);
        if(mTImpl->mElementaryPID !=mExtractor->mAudioPID && mTImpl->mIsAudio){
            if(mTImpl->hasBufferAvailable(&finalResult)){
                MediaBuffer *mSteambuf = NULL;
                int64_t mCurremtTime = mTImpl->getCurrentPackTime();
                if(mCurremtTime < mAudioTimeUs)
                {
                    mTImpl->read(&mSteambuf, options);
                    if(mSteambuf){
                        mSteambuf->release();
                        mSteambuf = NULL;
                    }
                }
            }
        }
    }
    if(isPCM && !isFristFrame)
        WavAddHeader(out,mBuffer);
    else
        *out = mBuffer;
    return OK;
}

status_t MPEG2TSSource::WavAddHeader(MediaBuffer **out,MediaBuffer *mBuffer){
    MediaBuffer *mTempBuffer = NULL;
    mTempBuffer = new MediaBuffer(sizeof(WaveFormatExStruct)+mBuffer->range_length());
    WaveFormatExStruct *externData =(WaveFormatExStruct *)mTempBuffer->data();
    memset(externData,0,sizeof(WaveFormatExStruct));
    externData->FormatTag = 0x0001;
    sp<MetaData> meta = mImpl->getFormat();
    meta->findInt32(kKeyChannelCount, (int32_t*)&externData->Channels);
    meta->findInt32(kKeyBitRate, (int32_t*)&externData->BitsPerSample);
    meta->findInt32(kKeySampleRate, (int32_t*)&externData->SamplesPerSec);
    uint32_t bytesPerSec = externData->BitsPerSample*externData->SamplesPerSec / 8;
    uint32_t nBlockAlign = externData->Channels * externData->BitsPerSample / 8;
    externData->BlockAlign = nBlockAlign;
    externData->SamplesPerBlock = (((nBlockAlign - (7 * externData->Channels)) * 8) / (externData->BitsPerSample *  externData->Channels)) + 2;
    externData->Size = 20; // ignore for WAVE_FORMAT_PCM
    memcpy(mTempBuffer->data()+sizeof(WaveFormatExStruct),mBuffer->data(),mBuffer->range_length());
    isFristFrame = true;
    *out = mTempBuffer;
    mBuffer->release();
    return OK;
}
status_t MPEG2TSSource::MpegtsSeekProcess(const ReadOptions *options,status_t *finalResult){
    int64_t seekTimeUs;
    ReadOptions::SeekMode seekMode;
    int64_t timeUs  = 0;
    if (mSeekable && options && options->getSeekTo(&seekTimeUs, &seekMode)) {
        LOGSEEK("MPEG2TSSource seek in \n");
        mExtractor->lastseektimeus = seekTimeUs;
        sp<MetaData> meta = mImpl->getFormat();
        int64_t durationUs;
        meta->findInt64(kKeyDuration,&durationUs);
        if(mExtractor->run)
            mExtractor->run = false;
        while(!mExtractor->startSeek)
        {
            LOGSEEK("waiting startSeek become true");
            if(mExtractor->stopflag)
            {
                return ERROR_END_OF_STREAM;
            }
            usleep(5000);
        }
        LOGSEEK("waiting startSeek is ok");
        mExtractor->seekTo(seekTimeUs);
        uint64_t calSeekSize = 0;
        off64_t Totalsize = 0;
        uint32_t seek_count = 0;
        uint32_t num_packet = 0;
        mExtractor->mDataSource->getSize(&Totalsize);
        while(1)
        {
            mExtractor->seekFlag = 1;
            status_t err = mExtractor->feedMore();
            if (err != OK) {
                ALOGE("seek fail in feedMore");
                mImpl->signalEOS(err);
                break;
            }
            num_packet++;
            if(num_packet > 3000)
            {
                break;
            }
            timeUs = mExtractor->mParser->getTimeus(mImpl->mProgramID,mImpl->mElementaryPID);
            if(timeUs)
            {
                LOGSEEK("timeUs seek out %lld",timeUs);
                break;
            }
        }
    }
    if(mImpl->mIsVideo && !mExtractor->mType)
    {
        if(!mExtractor->run)
        {
           if(mExtractor->seekFlag)
           {
                LOGSEEK("MPEG2TSSource seek out");
                mExtractor->seekFlag = 0;
                LOGSEEK("MPEG2TSSource feedMore ok start read thead %lld",timeUs);
                for(int i = 0; i < mExtractor->mSourceImpls.size(); i++)
                {
                    if(!mExtractor->mSourceImpls.editItemAt(i)->mIsVideo)
                    {
                        mExtractor->mSourceImpls.editItemAt(i)->setLastTime(timeUs);
                        mExtractor->mSourceImpls.editItemAt(i)->setLastTime(timeUs);
                    }
                }
                if(mExtractor->mWaitFlag){
                    int32_t minCount = 0;
                    while(minCount < 24)
                    {
                        status_t err = mExtractor->feedMore();
                        if (err != OK) {
                            break;
                        }
                        int Count = 25;
                        int memUsed = 0;
                        for(int i = 0; i < mExtractor->mSourceImpls.size(); i++){
                             int num = mExtractor->mSourceImpls.editItemAt(i)->numBufferAvailable(&memUsed);
                             if(memUsed > 5*1024*1024){
                                  break;
                             }
                             int32_t mCurrentPID = mExtractor->mSourceImpls.editItemAt(i)->mElementaryPID;
                             if((mExtractor->mAudioPID == mCurrentPID)||(mExtractor->mVideoPID == mCurrentPID)){
                                 if(num < Count){
                                    Count = num;
                                }
                            }
                        }
                        minCount = Count;
                    }
                }else{
                    while(!mImpl->hasBufferAvailable(finalResult))
                    {
                        status_t err = mExtractor->feedMore();
                        if (err != OK) {
                                mImpl->signalEOS(err);
                                break;
                            }
                        }
                    }
                mExtractor->run = true;
           }
        }
    }else if(!mExtractor->mType){
        while(!mExtractor->run)
        {
            if(mImpl->hasBufferAvailable(finalResult))
                break;
            if(mExtractor->stopflag)
            {
                return ERROR_END_OF_STREAM;
            }
            usleep(5000);
        }
    }
    return OK;
}


status_t MPEG2TSSource::read(
        MediaBuffer **out, const ReadOptions *options) {
    *out = NULL;
    status_t finalResult;
    int64_t timeUs  = 0;
    struct timeval timeFirst,timesec,timeThird,timeFourth;
    gettimeofday(&timeFirst, NULL);
    if(!mSeekable && mExtractor->mAudioSelect){
        AudioTrackSelectRead(out,options);
        if(*out != NULL){
            return OK;
        }
    }
    if(MpegtsSeekProcess(options,&finalResult)!= OK){
        return ERROR_END_OF_STREAM;
    }

	int flag = 0;
	int read_time = 0;
    bool Eos = false;
    while (!mImpl->hasBufferAvailable(&finalResult)) {
        if (finalResult != OK) {
            return ERROR_END_OF_STREAM;
        }
    	const char *mime;

    	sp<MetaData> meta = mImpl->getFormat();

        if(meta != NULL){
    	    meta->findCString(kKeyMIMEType,&mime);
        }
          //  bool Eos = false;
        switch(mExtractor->mType){
            case MPEG2TSExtractor::WIMO:
            {
                status_t err = mExtractor->feedMore();
                if(err==-2111)
                {
                    return INFO_TIME_OUT;
                }
                if (err != OK && err !=-2111) {
                    ALOGD("MPEG2TSSource::read err %d",err);
                    mImpl->signalEOS(err);
                    Eos = true;
                }
                break;
            }
            case MPEG2TSExtractor::NONE:
            {
                if(mExtractor->stopflag)
                {
                    return ERROR_END_OF_STREAM;
                }
                usleep(5000);
                break;
            }
    	    case MPEG2TSExtractor::TVPAD:
    	    {
    		    status_t err = mExtractor->feedMore();
        		if (err == -2111) {
    				if(read_time>=5)
    				{
    					if(!strncasecmp( "video/avc",mime ,6))
    						ALOGV("mpeg2tsextractor time out %d INFO_TIME_OUT %d mime %s",read_time,INFO_TIME_OUT,mime);
    					return INFO_TIME_OUT;
    				}
    				usleep(10000);
    				read_time++;
        		}
    		    if (err != OK && err != -2111) {
        		    ALOGE("MPEG2TSSource::read err %d is_audio %d",err,mImpl->mIsAudio);
        			mImpl->signalEOS(err);
        			Eos = true;
    		    }
    		    break;
    	     }
			case MPEG2TSExtractor::GPU_STRM:
			{
				status_t err = mExtractor->feedMore();
				if(err==-2111)
				{
					return INFO_TIME_OUT;
				}
				if (err != OK && err !=-2111) {
				    ALOGD("MPEG2TSSource::read err %d",err);
				    mImpl->signalEOS(err);
				    Eos = true;
				}
				break;
			}
             default:
                ALOGV("feed more on mLiveTv");
                status_t err = mExtractor->feedMore();
				if(mExtractor->mType == MPEG2TSExtractor::LIVE_TV && mExtractor->stopflag)
                {
                    return ERROR_END_OF_STREAM;
                }
                if (err != OK) {
    		        ALOGE("MPEG2TSSource::read err %d",err);
                    mImpl->signalEOS(err);
                    Eos = true;
					if(mExtractor->mType == MPEG2TSExtractor::LIVE_TV)
                    {
						ALOGE("MPEG2TSSource::read err send ERROR_END_OF_STREAM");
						return ERROR_END_OF_STREAM;
                    }
                }
                break;
        }
        if(Eos)
            break;
    }
    gettimeofday(&timesec, NULL);
    if(mImpl->mIsAudio&&Eos == false){
        AudioSourceRead(out,options);
        return OK;
    }else if(Eos==true)
		ALOGD("Eos true is_audio %d" , mImpl->mIsAudio);
    return mImpl->read(out, options);
}

////////////////////////////////////////////////////////////////////////////////

MPEG2TSExtractor::MPEG2TSExtractor(const sp<DataSource> &source,AppType type)
    : mDataSource(new MPEG2TSDataSource(source)),
      mParser(new ATSParser),
      mOffset(0),
      FirstPackoffset(0),
      mType(type),
      mWaitFlag(false),
      kTSPacketSize(188){
        seekFlag = 0;
	    init_flag = 0;
        mAudioSelect = false;
        run = false;
        stopflag = false;
        mThread_sistarted = false;
        _success = false;
        startSeek = false;
        mAudioPID = 0;
        mVideoPID = 0;
        mSeekSize = 0;
        Totalsize = 0;
        packetNum = 0;
        Lastpackt = 0;
        endEos = false;
        threadout = false;
        packet = NULL;
#if DVB_ENABLE
		mDvbLibHandle = NULL;
		dvbGetData = NULL;
		dvbGetInfo = NULL;
		dvbFreeInfo = NULL;
		dvbGetExit= NULL;
#endif //DVB_ENABLE
    	mDataSource->initCheck();
	init();
        lastseektimeus = 0;
}

MPEG2TSExtractor::~MPEG2TSExtractor(){
    for(int i = 0; i < mSourceImpls.size(); i++){
        sp<AnotherPacketSource> mTImpl = mSourceImpls.editItemAt(i);
        mTImpl->stop();
    }
#if DVB_ENABLE

    if(mType == LIVE_TV)
    {
		ALOGE("--->MPEG2TSExtractor::~MPEG2TSExtractor() in 2");
		if(dvbFreeInfo)
		{
			ALOGE("--->MPEG2TSExtractor::~MPEG2TSExtractor() in 3");
			dvbFreeInfo();
		}


		if(mDvbLibHandle)
		{
			dlclose(mDvbLibHandle);
        	mDvbLibHandle = NULL;
		}

	}
#endif//DVB_ENABLE
}

size_t MPEG2TSExtractor::countTracks() {
    if(!_success)
    {
        return 0;
    }
    return mSourceImpls.size();
}

sp<MediaSource> MPEG2TSExtractor::getTrack(size_t index) {
    if (index >= mSourceImpls.size()) {
        return NULL;
    }

    bool seekable = true;
    if (mSourceImpls.size()) {
        const char *mime;

        sp<MetaData> meta = mSourceImpls.editItemAt(index)->getFormat();
        CHECK(meta->findCString(kKeyMIMEType, &mime));
        if (!strncasecmp("audio/", mime, 6)) {
               MY_LOGD("seekable set false \n");
               seekable = false;
        }else if(!strncasecmp("video/", mime, 6)){
            switch(mType){
                case LIVE_TV:
                {
                     int TvFlag = 1;
                     meta->setInt32(kKeyTvFlag, TvFlag);
                     seekable = false;
                     break;
        }
                case HTTP_LIVE:
        {
            seekable = false;
                    break;
        }
                default:
                    break;
    }
        }
    }
    return new MPEG2TSSource(this, mSourceImpls.editItemAt(index), seekable);
}

sp<MetaData> MPEG2TSExtractor::getTrackMetaData(
        size_t index, uint32_t flags) {
    return index < mSourceImpls.size()
        ? mSourceImpls.editItemAt(index)->getFormat() : NULL;
}

sp<MetaData> MPEG2TSExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_MPEG2TS);

    return meta;
}
off64_t MPEG2TSExtractor::FirstPackfound(off64_t mOffset)
{
    int mpegtsFlag = 1;
    char header;
    off64_t packoffset = 0;
    for (int i = 0; i < 5; ++i)
    {
        if (mDataSource->readAt(mOffset + kTSPacketSize * i, &header, 1) != 1
            || header != 0x47) {
            mpegtsFlag = 0;
            break;
        }
    }
    if(!mpegtsFlag)
    {
        for(int i = 0; i < kTSPacketSize; i++)
        {
            if (mDataSource->readAt(mOffset + i, &header, 1) != 1)
            {
                ALOGV("found first packert read error");
                return -1;
            }
            else
            {
                if(header == 0x47)
                {
                    mpegtsFlag = 1;
                    for(int j = 1; j < 5; j++)
                    {
                        if (mDataSource->readAt(mOffset + j*kTSPacketSize + i, &header, 1) != 1)
                        {
                            break;
                        }
                        if(header != 0x47)
                        {
                            mpegtsFlag = 0;
                            break;
                        }
                    }
                    if(mpegtsFlag)
                    {
                        packoffset = i;
                        break;
                    }
                }
            }
        }
    }
    return packoffset;
}

void MPEG2TSExtractor::init() {
    bool haveAudio = false;
    bool haveVideo = false;
    int numPacketsParsed = 0;
    hasAudioPlayFlag = false;
    char tmpFileHead[12];
    if(!mType)
        {
        if (mDataSource->readAt(0, &tmpFileHead[0], 11) != 11)
        {
            return;
        }
        tmpFileHead[11]='\0';
        if(!strcmp(tmpFileHead, TV_TAG))
        {
            ALOGV("INIT FOUND IS LIVE TV");
    	            mType = LIVE_TV;
        }
        else if(!strcmp(tmpFileHead, WIMO_TAG))
        {
    		ALOGD("INIT FOUND IS WIMO");
    	    		mType = WIMO;
        }
		else if (!strcmp(tmpFileHead, TVPAD_TAG)) {
    	    mType = TVPAD;
        }
		else if(!strcmp(tmpFileHead, GPU_STRM_TAG))
        {
    		ALOGD("INIT FOUND IS GPU_TRANSPORT");
			mType = GPU_STRM;
        }
    }
    int player_type = 1;
	mParser->set_player_type(player_type);
    switch(mType){
        case NONE:
    {
          uint8_t buf[8*1024];
          if(mDataSource->readAt(0, buf, 8*1024) != 8*1024)
          {
               return ;
          }
          kTSPacketSize = get_packet_size(buf,sizeof(buf));
          if(kTSPacketSize == -1)
          {
              return;
          }

          FirstPackoffset = FirstPackfound(mOffset);
          if(FirstPackoffset < 0){
             return;
          }
          mOffset += FirstPackoffset;
            break;
        }
        case LIVE_TV:
        {
#if DVB_ENABLE
						//unsigned mAudioPID = 0x14,mVideoPID = 0x11;
            unsigned mAudioType = 0x81,mVideoType = 0x02;
            struct timeval starttime,midtime,endtime;

			uint32_t mVideoProgramID = 0;
			unsigned mVideoElementaryPID = 0;
		    uint32_t mAudioProgramID = 0;
		    unsigned mAudioElementaryPID = 0;
		    uint32_t timeout_ms = 12000;//0x3FFFFFFF;
		    struct timeval outtime;
		    uint32_t timeout_flag = 0;

			mParser->set_player_type(LIVE_TV);
			mDvbLibHandle = dlopen(DVB_LIB_NAME, RTLD_NOW);

			if(mDvbLibHandle == NULL)
			{
				 ALOGE("Unable to locate libdvbstream.so");
				 return;
			}
			else
			{
			    //dvbGetData
				dvbGetData = (DvbGetDataFunc)dlsym(mDvbLibHandle, DVB_GET_DATA);

				if(dvbGetData == NULL)
				{
					 ALOGE("Unable to find symbol %s",DVB_GET_DATA);
					 return;
				}

				dvbGetInfo = (DvbGetInfoFunc)dlsym(mDvbLibHandle, DVB_GET_INFO);

				if(dvbGetInfo == NULL)
				{
					 ALOGE("Unable to find symbol %s",DVB_GET_INFO);
					 return;
				}

				dvbFreeInfo = (DvbFreeInfoFunc)dlsym(mDvbLibHandle, DVB_FREE_INFO);

				if(dvbFreeInfo == NULL)
				{
					 ALOGE("Unable to find symbol %s",DVB_FREE_INFO);
					 return;
				}

				dvbGetExit = (DvbGetExitFunc)dlsym(mDvbLibHandle, DVB_GET_EXIT);

				if(dvbGetExit == NULL)
				{
					 ALOGE("Unable to find symbol %s",DVB_GET_EXIT);
					 return;
				}

			}

            gettimeofday(&starttime, NULL);
	    	kTSPacketSize = 188;
	   		dvbGetInfo((int *)&mVideoPID, (int *)&mAudioPID, (int *)&mVideoType, (int *)&mAudioType);
			ALOGI("mvpid 0x%04x mapid 0x%04x mvtype %d matype %d",mVideoPID,mAudioPID,mVideoType,mAudioType);
			//mAudioPID = 0;
            mParser->createLiveProgramID(mAudioPID,mAudioType,mVideoPID,mVideoType);
            mParser->Start(mAudioPID,mVideoPID);
            mParser->mPIDbuffer.push(mAudioPID);
            mParser->mPIDbuffer.push(mVideoPID);



		    gettimeofday(&outtime, NULL);
		    outtime.tv_sec += timeout_ms / 1000;
		    outtime.tv_usec += timeout_ms % 1000;
            gettimeofday(&midtime, NULL);
            while (feedMore() == OK) {
			if((mAudioPID == 0) && haveVideo) {//video only
			    init_flag = 1;
			    _success = true;
			    ALOGE("init out, video only ");
			    return;
			}

			if((mVideoPID == 0) && haveAudio) {//audio broadcast
			    init_flag = 1;
			    _success = true;
			    ALOGE("init out ");
			    return;
			}

			if((mVideoPID != 0) && haveVideo && haveAudio) { //video broadcast
			    init_flag = 1;
			    _success = true;
			    gettimeofday(&endtime, NULL);
			    ALOGE("TV TS interface init use time: %d ms!!!!!!!!!!!!!!!!!!!!!!!!!!!", (midtime.tv_sec - starttime.tv_sec)*1000 + (midtime.tv_usec - starttime.tv_usec)/1000);
			    ALOGE("TV SF init use time: %d ms!!!!!!!!!!!!!!!!!!!!!!!!!!!", (endtime.tv_sec - midtime.tv_sec)*1000 + (endtime.tv_usec - midtime.tv_usec)/1000);
			    ALOGE("init out ");
			    return;
			}

			/* check for timeout */
			if (timeout_ms > 0) {
				struct timeval curtime;
				gettimeofday(&curtime, NULL);
				if ((curtime.tv_sec > outtime.tv_sec) ||
				    ((curtime.tv_sec == outtime.tv_sec) && (curtime.tv_usec >= outtime.tv_usec))) {
					timeout_flag = 1;
				}
			}

			if (timeout_flag != 0){
			    ALOGE("init out: time out ");
				if((mVideoPID != 0) && haveVideo) { //video broadcast
				    init_flag = 1;
				    _success = true;
				    ALOGE("video only ");
				}
                return;
             }

			//ALOGE("while init ing!!!");
			if(dvbGetExit()){
				init_flag = 0;
				_success = false;
			    ALOGE("We receive gDvbPlayControlExit socket message:init#########################################!!!!!!!!!!!!!!!!! \n");
			    return;
			}

		        if ((!haveVideo) && (mVideoPID != 0)) {
		            sp<AnotherPacketSource> impl =
		                (AnotherPacketSource *)mParser->getSource(
		                        ATSParser::VIDEO,mVideoProgramID,mVideoElementaryPID).get();

		            if (impl != NULL) {
		                haveVideo = true;
		                impl->mElementaryPID = mVideoElementaryPID;
		                impl->mProgramID = 0xff;
		                impl->mIsVideo = true;
		                ALOGD("mVideoProgramID = %d,mVideoElementaryPID = 0x%x",mVideoProgramID,mVideoElementaryPID);
		                mSourceImpls.push(impl);
		            }
		        }

		        if ((!haveAudio) && (mAudioPID != 0)) {
		            sp<AnotherPacketSource> impl =
		                (AnotherPacketSource *)mParser->getSource(
		                        ATSParser::AUDIO,mAudioProgramID,mAudioElementaryPID).get();
		            if (impl != NULL) {
		                haveAudio = true;
		                impl->mElementaryPID = mAudioElementaryPID;
		                impl->mProgramID = 0xff;
		                ALOGD("mAudioProgramID = %d,mAudioElementaryPID = 0x%x",mAudioProgramID,mAudioElementaryPID);
		                mSourceImpls.push(impl);
		            }
		        }
                 }
			return; //live tv case
#endif//DVB_ENABLE
            break;
        }
        case WIMO:
        {
			player_type = WIMO;
		mParser->set_player_type(player_type);
            break;
        }
		case TVPAD:
		{
			player_type = TVPAD;
			mParser->set_player_type(player_type);
            break;
        }
		case GPU_STRM:
		{
			player_type = GPU_STRM;
			mParser->set_player_type(player_type);
            break;
        }
        default:
         break;
    }
    mDataSource->getSize(&Totalsize);
    if(Totalsize == 0){
        mType = HTTP_LIVE;
    }
    uint32_t mAudioProgramID = 0;
    while (feedMore() == OK) {
        ATSParser::SourceType type;
        if(mType){
        if (haveAudio && haveVideo) {
            break;
            }
        }
        if (!haveVideo) {
            uint32_t mVideoProgramID = 0;
            unsigned mVideoElementaryPID = 0;
            sp<AnotherPacketSource> impl =
                (AnotherPacketSource *)mParser->getSource(
                        ATSParser::VIDEO,mVideoProgramID,mVideoElementaryPID).get();

            if (impl != NULL && mVideoProgramID == 0) {
                haveVideo = true;
                impl->mElementaryPID = mVideoElementaryPID;
                impl->mProgramID = mVideoProgramID;
                mAudioProgramID = mVideoProgramID;
                impl->mIsVideo = true;
                mVideoPID = mVideoElementaryPID;
                MY_LOGD("mVideoProgramID = %d,mVideoElementaryPID = 0x%x",mVideoProgramID,mVideoElementaryPID);
                mSourceImpls.push(impl);
                mParser->mPIDbuffer.push(mVideoElementaryPID);
            }
        }

        if ((!haveAudio && mType == GPU_STRM) || (haveVideo && mType)) {
            unsigned mAudioElementaryPID = 0;
            sp<AnotherPacketSource> impl =
                (AnotherPacketSource *)mParser->getSource(
                        ATSParser::AUDIO,mAudioProgramID,mAudioElementaryPID).get();

            if (impl != NULL) {
                impl->mElementaryPID = mAudioElementaryPID;
                impl->mProgramID = mAudioProgramID;
                if(!haveAudio)
                {
                    mAudioPID = mAudioElementaryPID;
                    haveAudio = true;
                    MY_LOGD("mAudioProgramID = %d,mAudioElementaryPID = 0x%x",mAudioProgramID,mAudioElementaryPID);
                }
                mParser->mPIDbuffer.push(mAudioElementaryPID);
                mSourceImpls.push(impl);
            }
        }

        if ((++numPacketsParsed > 50000 && haveVideo) || (numPacketsParsed > 80000)||(GPU_STRM == mType && haveAudio )) {
            break;
        }
    }
    if(!haveVideo && mType != GPU_STRM)
    {
        ALOGI("u ha in 50000 ts packet can't found program 0 video we try to search all program");
        uint32_t mVideoProgramID = 0;
        unsigned mVideoElementaryPID = 0;
        sp<AnotherPacketSource> impl =
            (AnotherPacketSource *)mParser->getSource(
                    ATSParser::VIDEO,mVideoProgramID,mVideoElementaryPID).get();
        if (impl != NULL) {
            haveVideo = true;
            impl->mElementaryPID = mVideoElementaryPID;
            impl->mProgramID = mVideoProgramID;
            mAudioProgramID = mVideoProgramID;
            impl->mIsVideo = true;
            mVideoPID = mVideoElementaryPID;
            MY_LOGD("mVideoProgramID = %d,mVideoElementaryPID = 0x%x",mVideoProgramID,mVideoElementaryPID);
            mSourceImpls.push(impl);
            mParser->mPIDbuffer.push(mVideoElementaryPID);
        }
        if(!haveVideo) {
            unsigned mAudioElementaryPID = 0;
            sp<AnotherPacketSource> impl =
                (AnotherPacketSource *)mParser->getSource(
                        ATSParser::AUDIO,mAudioProgramID,mAudioElementaryPID).get();

            if (impl != NULL) {
                impl->mElementaryPID = mAudioElementaryPID;
                impl->mProgramID = mAudioProgramID;
                if(!haveAudio)
                {
                    mAudioPID = mAudioElementaryPID;
                    haveAudio = true;
                    MY_LOGD("mAudioProgramID = %d,mAudioElementaryPID = 0x%x",mAudioProgramID,mAudioElementaryPID);
                }
                mParser->mPIDbuffer.push(mAudioElementaryPID);
                mSourceImpls.push(impl);
            mType = GPU_STRM;
            }else{
                return;
            }
        }
    }
    if(!mType)
    {
        for(int i = 0;i< MAX_AUDIO_TRACK; i++){
            unsigned mAudioElementaryPID = 0;
            sp<AnotherPacketSource> impl =
                (AnotherPacketSource *)mParser->getSource(
                        ATSParser::AUDIO,mAudioProgramID,mAudioElementaryPID).get();

            if (impl != NULL) {
                impl->mElementaryPID = mAudioElementaryPID;
                impl->mProgramID = mAudioProgramID;
                if(!haveAudio)
                {
                    mAudioPID = mAudioElementaryPID;
                    haveAudio = true;
                    MY_LOGD("mAudioProgramID = %d,mAudioElementaryPID = 0x%x",mAudioProgramID,mAudioElementaryPID);
                }
                mParser->mPIDbuffer.push(mAudioElementaryPID);
                mSourceImpls.push(impl);
            }
        }
        off_t startOffset;
        mDataSource->getSize(&Totalsize);
        startOffset = mOffset;
        int64_t timeUs = 0,timeUsEnd = 0;
        int videoIndex = -1;
        for(int i = 0; i < mSourceImpls.size(); i++)
        {
            if(mSourceImpls.editItemAt(i)->mIsVideo)
            {
                videoIndex = i;
            }
        }
        mParser->signalSeek();
        if(Totalsize > 20000*188)
        {
            mOffset = Totalsize - 20000*188;
            seekFlag = 1;
        }
        else
        {
            mOffset = 0;
            seekFlag = 1;
        }
        while(feedMore() == OK)
        {
        }
        if(videoIndex >= 0)
        {
            timeUsEnd = mParser->getTimeus(mSourceImpls.editItemAt(videoIndex)->mProgramID,mSourceImpls.editItemAt(videoIndex)->mElementaryPID);
            ALOGV("timeUsEnd = %lld \n",timeUsEnd);
            for(int i = 0; i < mSourceImpls.size(); i++)
            {
                sp<MetaData> meta = mSourceImpls.editItemAt(i)->getFormat();
                meta->setInt64(kKeyDuration,timeUsEnd);
            }
        }
        seekFlag = 0;
        if(timeUsEnd)
           mSeekSize = Totalsize /(timeUsEnd/1000);
        mOffset = startOffset;
        MY_LOGD("haveAudio=%d, haveVideo=%d", haveAudio, haveVideo);
    }
    init_flag = 1;
    MY_LOGD("MPEG2TSExtractor init out ");
    _success = true;
}

void* MPEG2TSExtractor::Threadproc(void *me)
{
	MPEG2TSExtractor *ap = (MPEG2TSExtractor*)me;
	while(1)
	{
        int32_t sleeptime = ap->FileProcess();
        if(sleeptime < 0)
        {
           break;
        }
        else if(sleeptime > 0)
        {
            usleep(sleeptime);
        }
    }
    ap->threadout = true;
    return NULL;
}
int32_t MPEG2TSExtractor::FileProcess()
{
    if(!run)
    {
        startSeek = true;
        if(stopflag)
        {
            run = true;
            return -1;
        }
        packetNum = 0;
        return 2000;
    }
    startSeek = false;
    if(packetNum == 0)
    {
        int minCount = 100;
        for(int i = 0; i < mSourceImpls.size(); i++){
            int num = mSourceImpls.editItemAt(i)->numBufferAvailable();
            if(num < minCount){
               minCount = num;
            }
        }
        if(minCount > 25)
          return 1000;
        if(mOffset + kTSPacketSize*TS_MAX_PACKET < Totalsize)
        {
            if(kTSPacketSize*TS_MAX_PACKET != mDataSource->readAt(mOffset, packet, kTSPacketSize*TS_MAX_PACKET))
            {
                endEos = true;
                goto Endout;
            }
            mOffset += kTSPacketSize*TS_MAX_PACKET;
        }
        else
        {
            if((Totalsize - mOffset) != mDataSource->readAt(mOffset, packet, Totalsize - mOffset)){
                packetNum = 0;
                Lastpackt = 0;
                endEos = true;
                goto Endout;
            }
            endEos = true;
            if((Totalsize - mOffset) <= 188){
                 Lastpackt = 0;
                 goto Endout;
            }
            Lastpackt = (Totalsize - mOffset)/kTSPacketSize;
            mOffset = Totalsize;
        }
    }
    if(packet[kTSPacketSize*packetNum] != 0x47)
    {
        if(mOffset == Totalsize){
            goto Endout;
        }
        packetNum = 0;
        mOffset -=(TS_MAX_PACKET - kTSPacketSize*packetNum);
        status_t err = feedMore();
        if (err != OK) {
            for(int i = 0; i < mSourceImpls.size(); i++)
            {
                mSourceImpls.editItemAt(i)->signalEOS(ERROR_END_OF_STREAM);
            }
            Lastpackt = 0;
            endEos = true;
            goto Endout;
        }
        if(mOffset + kTSPacketSize*TS_MAX_PACKET < Totalsize)
        {
            if(kTSPacketSize*TS_MAX_PACKET != mDataSource->readAt(mOffset, packet, kTSPacketSize*TS_MAX_PACKET))
            {
                endEos = true;
                goto Endout;
            }
            mOffset += kTSPacketSize*TS_MAX_PACKET;
        }
        else
        {
            if((Totalsize - mOffset) != mDataSource->readAt(mOffset, packet, Totalsize - mOffset)){
                packetNum = 0;
                Lastpackt = 0;
                endEos = true;
                goto Endout;
            }
            endEos = true;
            Lastpackt = (Totalsize - mOffset)/kTSPacketSize;
            mOffset = Totalsize;
        }
    }
    if(TS_DVHS_PACKET_SIZE == kTSPacketSize)
    {
       mParser->feedTSPacket(&packet[kTSPacketSize*packetNum], kTSPacketSize - 4,seekFlag);
    }
    else if(TS_FEC_PACKET_SIZE == kTSPacketSize)
    {
        mParser->feedTSPacket(&packet[kTSPacketSize*packetNum], kTSPacketSize - 16,seekFlag);
    }
    else
    {
        mParser->feedTSPacket(&packet[kTSPacketSize*packetNum], kTSPacketSize,seekFlag);
    }
Endout:
    packetNum++;
    if (endEos) {
        if(packetNum > Lastpackt)
        {
            for(int i = 0; i < mSourceImpls.size(); i++)
            {
                 mSourceImpls.editItemAt(i)->signalEOS(ERROR_END_OF_STREAM);
            }
            run = false;
            stopflag = true;
        }
    }
    if(packetNum >= TS_MAX_PACKET)
    {
        int minCount = 25;
        for(int i = 0; i < mSourceImpls.size(); i++){
            int num = mSourceImpls.editItemAt(i)->numBufferAvailable();
            if(num < minCount){
               minCount = num;
            }
        }
        packetNum = 0;
        mDataSource->updatecache(mOffset);
        if(minCount > 25)
           return 1000;
    }
    return 0;
}
void MPEG2TSExtractor::start()
{
     if (mThread_sistarted)
         return;
     packet = (uint8_t*)malloc(kTSPacketSize*TS_MAX_PACKET);
     ALOGV("packet 0x%x \n",packet);
     if(!packet)
     {
        return;
     }
     pthread_attr_t attr;
     pthread_attr_init(&attr);
     pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
     pthread_create(&mThread, &attr, Threadproc, (void*)this);
     pthread_attr_destroy(&attr);
     mThread_sistarted = true;
}
void MPEG2TSExtractor::stop()
{
    if (!mThread_sistarted)
		return;
	void *dummy;
    pthread_join(mThread, &dummy);
	mThread_sistarted = false;
    if(packet)
    {
        ALOGV("free packet packet = 0x%x",packet);
        free(packet);
        packet = NULL;
    }
}

#if DVB_ENABLE
static uint16_t ts_pid(uint8_t *buf)
{
	return (((uint16_t)buf[1] & 0x1f) << 8) + buf[2];
}
#endif
status_t MPEG2TSExtractor::feedMore() {
    Mutex::Autolock autoLock(mLock);

    uint8_t packet[kTSPacketSize];
#if DVB_ENABLE
   if(mType == LIVE_TV){

	    #define DVB_FECTH_PACKET_NUM   50
	    static uint8_t ts_packets[188*DVB_FECTH_PACKET_NUM];
  		int ret = -1;

		if(dvbGetData)
		{
			//return value 0: need wait for data,-1: mean the device error must report this error ; >0 mean read the data len
		 	ret = dvbGetData(ts_packets,kTSPacketSize*DVB_FECTH_PACKET_NUM);

		}
		//ALOGI("--->feedmore ret =%d",ret);
		if(ret > 0)
		{
			for(int i =0; i< DVB_FECTH_PACKET_NUM; i++){
		   	    if((ts_pid(ts_packets+i*kTSPacketSize) == mVideoPID)||(ts_pid(ts_packets+i*kTSPacketSize) == mAudioPID))
		   	    {
		            mParser->feedTSPacket(ts_packets+i*kTSPacketSize, kTSPacketSize,0);
		   	    }
			 }
		}
		else if(ret < 0)
			return !OK;

		if(stopflag){
		    ALOGE("We have get stop message in fetch no data#########################################!!!!!!!!!!!!!!!!! \n");
		    return !OK;
		}
	    return OK;

   }
#endif
retry:
    ssize_t n = mDataSource->readAt(mOffset, packet, kTSPacketSize);
    switch(mType){
        case WIMO:
    {
	    if(n==-2111 )
	    {
			  ALOGV("anothor thread has already got the data init_flag %d %2x%2x%2x%2x%2x%2x%2x%2x",init_flag,
					packet[0],packet[1],packet[2],packet[3]
					,packet[4],packet[5],packet[6],packet[7]);
			if(init_flag==1)
				return -2111;
			else
			{
				usleep(10000);
				return OK;
			}
	    }
            else	if(n == -1111)
	        {
		      ALOGD("MPEG2TSExtractor::feedMore");
		        return -1111;
	        }
	    if(packet[0] != 0x47)
	    {
	    	   ALOGD("packet[0]!=47 init_flag %d %2x%2x%2x%2x%2x%2x%2x%2x n %d",init_flag,packet[0],packet[1],packet[2],packet[3]
					,packet[4],packet[5],packet[6],packet[7],n);
				if(init_flag==1)
				    return -2111;
			    else
			    {
				    usleep(10000);
				    return OK;
			    }
		return -2111;
	    }
                break;
       }
	case TVPAD:
	{
		if (n == -1111) {
		{
			ALOGD("TVPad err = -1111 ");
			return -1111;
		}
		}
		if (n == -2111) {
			if (init_flag == 1)
				return -2111;
			else {
				usleep(10000);
				return OK;
			}
	    }
                break;
       }
		case GPU_STRM:
		{
		    if(n==-2111 )
		    {
				  ALOGV("anothor thread has already got the data init_flag %d %2x%2x%2x%2x%2x%2x%2x%2x",init_flag,
						packet[0],packet[1],packet[2],packet[3]
						,packet[4],packet[5],packet[6],packet[7]);
				if(init_flag==1)
					return -2111;
				else
				{
					usleep(10000);
					return OK;
				}
		    }
            else	if(n == -1111)
	        {
		      ALOGD("MPEG2TSExtractor::feedMore");
		        return -1111;
	        }
		    if(packet[0] != 0x47)
		    {
		    	   ALOGD("GPU_STRM packet[0]!=47 init_flag %d %2x%2x%2x%2x%2x%2x%2x%2x n %d",init_flag,packet[0],packet[1],packet[2],packet[3]
						,packet[4],packet[5],packet[6],packet[7],n);
					if(init_flag==1)
					    return -2111;
				    else
				    {
					    usleep(10000);
					    return OK;
				    }
			return -2111;
		    }
	                break;
		}
       default:
          break;
    }
    if(packet[0] != 0x47)
    {
        off64_t mToffset = FirstPackfound(mOffset);
        if(mToffset > 0)
        {
            mOffset += mToffset;
            goto retry;
        }
        else if(mToffset == 0)
        {
            mOffset += kTSPacketSize;
            return INFO_TIME_OUT;
        }else{
            ALOGI("set ERROR_END_OF_STREAM");
            return ERROR_END_OF_STREAM;
        }
    }
    if (n < (ssize_t)kTSPacketSize) {
        return (n < 0) ? (status_t)n : ERROR_END_OF_STREAM;
    } else {
        if(TS_DVHS_PACKET_SIZE == kTSPacketSize)
        {
            mParser->feedTSPacket(packet, kTSPacketSize-4,seekFlag);
        }
        else if(TS_FEC_PACKET_SIZE == kTSPacketSize)
        {
            mParser->feedTSPacket(packet, kTSPacketSize-16,seekFlag);
        }
        else{
            mParser->feedTSPacket(packet, kTSPacketSize,seekFlag);
        }
    }

    mOffset += n;
    if(mType != WIMO && Totalsize){
        if(mOffset >= Totalsize){
            return ERROR_END_OF_STREAM;
        }
    }
    mDataSource->updatecache(mOffset);
    return OK;
}

void MPEG2TSExtractor::seekTo(int64_t seekTimeUs) {
    Mutex::Autolock autoLock(mLock);

    mParser->signalDiscontinuity(ATSParser::DISCONTINUITY_SEEK,NULL);
    mParser->signalSeek();
    off64_t startOffset;
    startOffset = seekTimeUs/1000*mSeekSize;
    if(startOffset > 2*1024*1024)
    {
        startOffset = startOffset - 2*1024*1024;
    }
    else
    {
        startOffset = FirstPackoffset;
    }
    mOffset = startOffset;
    return;

}

uint32_t MPEG2TSExtractor::flags() const {
    Mutex::Autolock autoLock(mLock);

    uint32_t flags = CAN_PAUSE;

   // if (mLiveSource != NULL && mLiveSource->isSeekable())
    {
        flags |= CAN_SEEK_FORWARD | CAN_SEEK_BACKWARD | CAN_SEEK;
    }

    return flags;
}

////////////////////////////////////////////////////////////////////////////////

bool SniffMPEG2TS(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *) {
        uint8_t buf[8*1024];
        int32_t raw_packet_size = 0;
        char tmpFileHead[12];
        if (source->readAt(0, &tmpFileHead[0], 11) != 11)
        {
            return false;
        }
        tmpFileHead[11]='\0';
        if(!strcmp(tmpFileHead, TV_TAG))
        {
			*confidence = 0.5f;
            mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_MPEG2TS);
            return true;
        } else if (!strcmp(tmpFileHead, TVPAD_TAG)) {
		*confidence = MPEG2TS_CONTAINER_CONFIDENCE;
		mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_MPEG2TS);
		return true;
    }
        if (source->readAt(0, buf, 8*1024) != 8*1024)
        {

            return false;
        }
        raw_packet_size = get_packet_size(buf,sizeof(buf));
        if(raw_packet_size > 0)
        {
            *confidence = MPEG2TS_CONTAINER_CONFIDENCE;
    mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_MPEG2TS);

    return true;
        }
        *confidence = 0;
        return false;
}

}  // namespace android
