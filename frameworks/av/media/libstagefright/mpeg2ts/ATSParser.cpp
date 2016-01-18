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

//#define LOG_NDEBUG 0
#define LOG_TAG "ATSParser"
#include <utils/Log.h>

#include "ATSParser.h"

#include "AnotherPacketSource.h"
#include "ESQueue.h"
#include "include/avc_utils.h"

#include <media/stagefright/foundation/ABitReader.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <media/IStreamSource.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <sys/endian.h>
#if 0
extern "C" {
#include <crc.h>
};
#endif

#ifdef MY_LOG_DEBUG
    #define MY_LOGD ALOGD
#else
    #define MY_LOGD
#endif
#define MAX_PACKET_NUM 40  //use for when audio have MAX_PACKET_NUM PES we also no found video PTS use audio PTS instead
namespace android {

// I want the expression "y" evaluated even if verbose logging is off.
#define MY_LOGV(x, y) \
    do { unsigned tmp = y; ALOGV(x, tmp); } while (0)

static const size_t kTSPacketSize = 188;

struct ATSParser::Program : public RefBase {
    Program(ATSParser *parser, unsigned programNumber, unsigned programMapPID);

    bool parsePSISection(
            unsigned pid, ABitReader *br, status_t *err);

    bool parsePID(
            unsigned pid, unsigned continuity_counter,
            unsigned payload_unit_start_indicator,
            ABitReader *br,uint32_t seekFlag,status_t *err);

    void signalDiscontinuity(
            DiscontinuityType type, const sp<AMessage> &extra);

    void signalEOS(status_t finalResult);
	void signalSeek();
    sp<MediaSource> getSource(SourceType type,unsigned& elementaryPID);

    sp<MediaSource> getSource(SourceType type);
    bool hasSource(SourceType type) const;

    int64_t getTimeus(unsigned elementaryPID);
	void set_player_type(int type);
	int get_player_type(){return player_type;};
    void createLiveStream(unsigned AudioPID,unsigned AudioType,unsigned VideoPID,unsigned VideoType);
    int64_t convertPTSToTimestamp(uint64_t PTS);

    bool PTSTimeDeltaEstablished() const {
		if(mStreams.size() > 1){
            return (hasAudio&&hasVideo&&mFirstPTSValid);
        }
        return mFirstPTSValid;
    }

	uint64_t mFirstPTS;
    uint64_t mLastPTS;
    bool plusTimeFlag;
	bool mrestFlag;
    int64_t mBaseTimeUs;
	bool mFirstPTSValid;
	bool mOnlyAudioFlag;
    bool hasVideo;
    bool hasAudio;
    bool mPlusBaseTimeFlag;
    int32_t noVideoPtsNum;
    unsigned number() const { return mProgramNumber; }

    void updateProgramMapPID(unsigned programMapPID) {
        mProgramMapPID = programMapPID;
    }

    unsigned programMapPID() const {
        return mProgramMapPID;
    }

    uint32_t parserFlags() const {
        return mParser->mFlags;
    }

private:
    ATSParser *mParser;
    unsigned mProgramNumber;
    int player_type;
    unsigned mProgramMapPID;
    KeyedVector<unsigned, sp<Stream> > mStreams;


    status_t parseProgramMap(ABitReader *br);

    DISALLOW_EVIL_CONSTRUCTORS(Program);
};

struct ATSParser::Stream : public RefBase {
    Stream(Program *program,
           unsigned elementaryPID,
           unsigned streamType,
           unsigned PCR_PID);

    unsigned type() const { return mStreamType; }
    unsigned pid() const { return mElementaryPID; }
    void setPID(unsigned pid) { mElementaryPID = pid; }

    status_t parse(
            unsigned continuity_counter,
            unsigned payload_unit_start_indicator,
            ABitReader *br);

    status_t Seekparse(
            unsigned payload_unit_start_indicator,
            ABitReader *br);
    void signalDiscontinuity(
            DiscontinuityType type, const sp<AMessage> &extra);

    void signalEOS(status_t finalResult);

    void signalSeek();
	void set_player_type(int type);
    sp<MediaSource> getSource(SourceType type,unsigned& elementaryPID);
    sp<MediaSource> getSource(SourceType type);

    int64_t getTimeus(SourceType type);
    void setLanguage(const char *language);
    unsigned mElementaryPID;
    int64_t mCurTimeus;
    int64_t mDuration;
    bool is_video;
	int player_type;
    int32_t mCount;
    int32_t mFormatChange;
    int32_t mPes_Length;
    bool mPes_Getlength_Flag;
    bool isAudio() const;
    bool isVideo() const;
protected:
    virtual ~Stream();

private:
    Program *mProgram;
    unsigned mStreamType;
    unsigned mPCR_PID;
    int32_t mExpectedContinuityCounter;

    int64_t lasttimeus;
    sp<ABuffer> mBuffer;
    sp<ABuffer> mSeekBuffer;
    sp<AnotherPacketSource> mSource;
    bool mPayloadStarted;
    bool mSeekPayloadStarted;
    bool isRigster;

    uint64_t mPrevPTS;

    ElementaryStreamQueue *mQueue;
#ifdef PES_DEBUG
    FILE *fp;
#endif
    status_t flush();
    status_t parsePES(ABitReader *br);

    status_t Seekflush();
    status_t SeekparsePES(ABitReader *br);
    void onPayloadData(
            unsigned PTS_DTS_flags, uint64_t PTS, uint64_t DTS,
            const uint8_t *data, size_t size);

    void extractAACFrames(const sp<ABuffer> &buffer);

    DISALLOW_EVIL_CONSTRUCTORS(Stream);
};

struct ATSParser::PSISection : public RefBase {
    PSISection();

    status_t append(const void *data, size_t size);
    void clear();

    bool isComplete() const;
    bool isEmpty() const;

    const uint8_t *data() const;
    size_t size() const;

protected:
    virtual ~PSISection();

private:
    sp<ABuffer> mBuffer;

    DISALLOW_EVIL_CONSTRUCTORS(PSISection);
};

////////////////////////////////////////////////////////////////////////////////

ATSParser::Program::Program(
        ATSParser *parser, unsigned programNumber, unsigned programMapPID)
    : mParser(parser),
      mProgramNumber(programNumber),
      mProgramMapPID(programMapPID),
      mFirstPTSValid(false),
      mrestFlag(false),
      mFirstPTS(0),
      mLastPTS(0),
	  hasVideo(false),
      hasAudio(false),
      player_type(false),
      mOnlyAudioFlag(false),
      plusTimeFlag(false),
      mPlusBaseTimeFlag(false),
      noVideoPtsNum(0),
      mBaseTimeUs(0) {
    ALOGV("new program number %u", programNumber);
}
void ATSParser::Program::createLiveStream(unsigned AudioPID,unsigned AudioType,unsigned VideoPID,unsigned VideoType)
{
    if(VideoPID)
        mStreams.add(VideoPID, new Stream(this, VideoPID, VideoType,0));
    if(AudioPID)
        mStreams.add(AudioPID, new Stream(this, AudioPID, AudioType,0));
    if(!VideoPID)
        mOnlyAudioFlag = true;
	if(player_type == 4 /*live_tv*/){
		for (size_t i = 0; i < mStreams.size(); ++i) {
		  		mStreams.editValueAt(i)->set_player_type(player_type);
		  }
	}
}
void ATSParser::Program::set_player_type(int type){
	ALOGV("ATSParser Program::set_player_type flag %dmStreams.size() %d",type,mStreams.size());
	player_type = type;
	return;
}

bool ATSParser::Program::parsePSISection(
        unsigned pid, ABitReader *br, status_t *err) {
    *err = OK;

    if (pid != mProgramMapPID) {
        return false;
    }

    *err = parseProgramMap(br);

    return true;
}

bool ATSParser::Program::parsePID(
        unsigned pid, unsigned continuity_counter,
        unsigned payload_unit_start_indicator,
        ABitReader *br, uint32_t seekFlag,status_t *err) {
    *err = OK;

    ssize_t index = mStreams.indexOfKey(pid);
    if (index < 0) {
        return false;
    }
    if(seekFlag)
    {
        mStreams.editValueAt(index)->Seekparse(
                payload_unit_start_indicator, br);
    }
    else
    {
    *err = mStreams.editValueAt(index)->parse(
            continuity_counter, payload_unit_start_indicator, br);
    }
    return true;
}

void ATSParser::Program::signalDiscontinuity(
        DiscontinuityType type, const sp<AMessage> &extra) {
    int64_t mediaTimeUs;
    if ((type & DISCONTINUITY_TIME)
            && extra != NULL
            && extra->findInt64(
                IStreamListener::kKeyMediaTimeUs, &mediaTimeUs)) {
        mFirstPTSValid = false;
    }

    for (size_t i = 0; i < mStreams.size(); ++i) {
        mStreams.editValueAt(i)->signalDiscontinuity(type, extra);
    }
}

void ATSParser::Program::signalSeek() {
    for (size_t i = 0; i < mStreams.size(); ++i) {
        mStreams.editValueAt(i)->signalSeek();
    }
}


void ATSParser::Program::signalEOS(status_t finalResult) {
    for (size_t i = 0; i < mStreams.size(); ++i) {
        mStreams.editValueAt(i)->signalEOS(finalResult);
    }
}

struct StreamInfo {
    unsigned mType;
    unsigned mPID;
};

status_t ATSParser::Program::parseProgramMap(ABitReader *br) {
    unsigned char* crc_start_addr = (uint8_t*)(br->data()) ;//+ (br->numBitsLeft()/8));
    unsigned table_id = br->getBits(8);
    ALOGV("  table_id = %u", table_id);

    if (table_id != 0x02u) {
        ALOGE("PMT data error!");
        return ERROR_MALFORMED;
    }
    unsigned section_syntax_indicator = br->getBits(1);
    ALOGV("  section_syntax_indicator = %u", section_syntax_indicator);
    if (section_syntax_indicator != 1u) {
        ALOGE("PMT data error!");
        return ERROR_MALFORMED;
    }

    br->getBits(1);//  CHECK_EQ(br->getBits(1), 0u);
    MY_LOGV("  reserved = %u", br->getBits(2));

    unsigned section_length = br->getBits(12);
    ALOGV("  section_length = %u", section_length);
  //  CHECK_EQ(section_length & 0xc00, 0u);
  //  CHECK_LE(section_length, 1021u);

    MY_LOGV("  program_number = %u", br->getBits(16));
    MY_LOGV("  reserved = %u", br->getBits(2));
    MY_LOGV("  version_number = %u", br->getBits(5));
    MY_LOGV("  current_next_indicator = %u", br->getBits(1));
    MY_LOGV("  section_number = %u", br->getBits(8));
    MY_LOGV("  last_section_number = %u", br->getBits(8));
    MY_LOGV("  reserved = %u", br->getBits(3));

    int PCR_PID = br->getBits(13);

    MY_LOGV("  reserved = %u", br->getBits(4));

    unsigned program_info_length = br->getBits(12);
    ALOGV("  program_info_length = %u", program_info_length);
   // CHECK_EQ(program_info_length & 0xc00, 0u);

    br->skipBits(program_info_length * 8);  // skip descriptors

    Vector<StreamInfo> infos;

    // infoBytesRemaining is the number of bytes that make up the
    // variable length section of ES_infos. It does not include the
    // final CRC.
    size_t infoBytesRemaining = section_length - 9 - program_info_length - 4;
    if(infoBytesRemaining > br->numBitsLeft()/8)
    {
        infoBytesRemaining = br->numBitsLeft()/8 -4;
    }
    while (infoBytesRemaining > 0) {
        ALOGV("infoBytesRemaining  = %d",infoBytesRemaining);
        if(infoBytesRemaining < 5u)
        {
            ALOGD("infoBytesRemaining small then 5u");
            br->skipBits(infoBytesRemaining*8);
            infoBytesRemaining = 0;
            break;
        }

        unsigned streamType = br->getBits(8);
        ALOGV("stream_type = 0x%02x", streamType);

        MY_LOGV("reserved = %u", br->getBits(3));

        unsigned elementaryPID = br->getBits(13);
        ALOGV("elementary_PID = 0x%04x", elementaryPID);

        MY_LOGV("reserved = %u", br->getBits(4));

        unsigned ES_info_length = br->getBits(12);
        ALOGV("ES_info_length = %u", ES_info_length);
     //   CHECK_EQ(ES_info_length & 0xc00, 0u);

  		if(infoBytesRemaining - 5 < ES_info_length)
        {
             br->skipBits((infoBytesRemaining-5)*8);
             infoBytesRemaining = 0;
             break;
        }
        unsigned info_bytes_remaining = ES_info_length;
        char language[252];
        String8 request("");
        while (info_bytes_remaining >= 2) {
            unsigned desc_tag = br->getBits(8);

            unsigned descLength = br->getBits(8);
            ALOGV("desc_tag = 0x%x , len = %u",desc_tag,descLength);

            if(info_bytes_remaining < 2 + descLength)
            {
                return OK;
            }
            switch(desc_tag){
                case 0x05:
                {
                    uint32_t EStag = br->getBits(32);
                    switch(EStag){
                        case FOURCC('A', 'C', '-', '3'):
                        {
                            break;
                        }
                        case FOURCC('D','T','S','1'):
                        case FOURCC('D','T','S','2'):
                        case FOURCC('D','T','S','3'):
                        {
                             streamType = 0x7b;
                             break;
                        }
                        case FOURCC('V','C','-','1'):
                        {
                            streamType = 0xea;
                            break;
                        }
                        case FOURCC('H','E','V','C'):
                        {
                            streamType = STREAMTYPE_HEVC;
                            break;
                        }
                        default:
                            break;
                    }
                    br->skipBits((descLength - 4) * 8);
                    break;
                }
                case 0x6a:
                case 0x7a:
                {
                    streamType = 0x81;
            br->skipBits(descLength * 8);
                    break;
                }
                case 0x7b:
                {
                    streamType = 0x7b;
        		    br->skipBits(descLength * 8);
                    break;
                }
                case 0x56:
                {
                    streamType = 0x56;
        		    br->skipBits(descLength * 8);

                    break;
                }
                case 0x59:
                {
                    streamType = 0x59;
        		    br->skipBits(descLength * 8);
                    break;
                }
                case 0xa:
                {
                    int32_t skiplen = descLength;
                    int i = 0;
                    for (i = 0; i + 4 <= descLength; i += 4) {
                        language[i + 0] = br->getBits(8);
                        language[i + 1] = br->getBits(8);
                        language[i + 2] = br->getBits(8);
                        language[i + 3] = ',';
                        br->getBits(8);
                        skiplen -= 4;
                    }
        		    br->skipBits(skiplen * 8);
                    if (i) {
                        language[i - 1] = 0;
                        skiplen = i;
                    }
                    request.append(String8(language));
                    if(request.size() > 2){
                        ALOGV("info size: %d,request = %s,streamType = %d,elementaryPID = %d", request.size(),request.string(),streamType,elementaryPID);
                        ssize_t index = mStreams.indexOfKey(elementaryPID);
                        if(index > 0){
                            mStreams.editValueAt(index)->setLanguage(request.string());
                        }
                    }
                }
                break;
                default:
        		    br->skipBits(descLength * 8);
                 break;
            }
            info_bytes_remaining -= descLength + 2;
        }

        StreamInfo info;
        info.mType = streamType;
        info.mPID = elementaryPID;
        infos.push(info);

        infoBytesRemaining -= 5 + ES_info_length;
    }

    //CHECK_EQ(infoBytesRemaining, 0u);
    if(player_type == 3||player_type==6)
	{
		#if 0
    int crc;
    int crc_calc;
    unsigned char* crc_end_addr = (uint8_t*)br->data();// + (br->numBitsLeft()/8);
    crc_calc = __swap32(av_crc(av_crc_get_table(AV_CRC_32_IEEE), -1, crc_start_addr, crc_end_addr - crc_start_addr));
    crc = br->getBits(32);
    if(crc != crc_calc)
    {
		ALOGI("  mData %d  bitleft %d CRC = 0x%08x crc_calc %x crc_start_addr %x crc_end_addr %x",	br->data(), br->numBitsLeft(), crc , crc_calc, crc_start_addr, crc_end_addr);
    }
		#endif
	}
	else
    MY_LOGV("  CRC = 0x%08x", br->getBits(32));

    bool PIDsChanged = false;
    for (size_t i = 0; i < infos.size(); ++i) {
        StreamInfo &info = infos.editItemAt(i);

        ssize_t index = mStreams.indexOfKey(info.mPID);

        if (index >= 0 && mStreams.editValueAt(index)->type() != info.mType) {
            ALOGV("uh oh. stream PIDs have changed.");
//            PIDsChanged = true;
            break;
        }
    }

    if (PIDsChanged) {
#if 0
        ALOGI("before:");
        for (size_t i = 0; i < mStreams.size(); ++i) {
            sp<Stream> stream = mStreams.editValueAt(i);

            ALOGI("PID 0x%08x => type 0x%02x", stream->pid(), stream->type());
        }

        ALOGI("after:");
        for (size_t i = 0; i < infos.size(); ++i) {
            StreamInfo &info = infos.editItemAt(i);

            ALOGI("PID 0x%08x => type 0x%02x", info.mPID, info.mType);
        }
#endif

        // The only case we can recover from is if we have two streams
        // and they switched PIDs.

        bool success = false;

        if (mStreams.size() == 2 && infos.size() == 2) {
            const StreamInfo &info1 = infos.itemAt(0);
            const StreamInfo &info2 = infos.itemAt(1);

            sp<Stream> s1 = mStreams.editValueAt(0);
            sp<Stream> s2 = mStreams.editValueAt(1);

            bool caseA =
                info1.mPID == s1->pid() && info1.mType == s2->type()
                    && info2.mPID == s2->pid() && info2.mType == s1->type();

            bool caseB =
                info1.mPID == s2->pid() && info1.mType == s1->type()
                    && info2.mPID == s1->pid() && info2.mType == s2->type();

            if (caseA || caseB) {
                unsigned pid1 = s1->pid();
                unsigned pid2 = s2->pid();
                s1->setPID(pid2);
                s2->setPID(pid1);

                mStreams.clear();
                mStreams.add(s1->pid(), s1);
                mStreams.add(s2->pid(), s2);

                success = true;
            }
        }

        if (!success) {
            ALOGI("Stream PIDs changed and we cannot recover.");
            return ERROR_MALFORMED;
        }
    }
    for (size_t i = 0; i < infos.size(); ++i) {
        StreamInfo &info = infos.editItemAt(i);

        ssize_t index = mStreams.indexOfKey(info.mPID);

        if (index < 0) {
            sp<Stream> stream = new Stream(
                    this, info.mPID, info.mType, PCR_PID);

            mStreams.add(info.mPID, stream);
        }
    }


    bool hasVideo = false;
	for (size_t i = 0; i < mStreams.size(); ++i) {
        hasVideo = mStreams.editValueAt(i)->is_video;
        if(hasVideo){
                    break;
            }
        }

    if(!hasVideo){ //add by csy for check if only have audio
        ALOGI("no have video found hasVideo");
        player_type = 6;
    }
    if(player_type)
    {
      ALOGV(" set_player_type mStreams.size() %d",mStreams.size());
    	for (size_t i = 0; i < mStreams.size(); ++i) {
      mStreams.editValueAt(i)->set_player_type(player_type);
        }
    }

    return OK;
}

sp<MediaSource> ATSParser::Program::getSource(SourceType type) {
    size_t index = (type == AUDIO) ? 0 : 0;

    for (size_t i = 0; i < mStreams.size(); ++i) {
        sp<MediaSource> source = mStreams.editValueAt(i)->getSource(type);
        if (source != NULL) {
            if (index == 0) {
                return source;
            }
            --index;
        }
    }

    return NULL;
}
sp<MediaSource> ATSParser::Program::getSource(SourceType type, unsigned & elementaryPID ) {
    size_t index = (type == AUDIO) ? 0 : 0;

    for (size_t i = 0; i < mStreams.size(); ++i) {
        sp<MediaSource> source = mStreams.editValueAt(i)->getSource(type,elementaryPID);
        if (source != NULL) {
            if (index == 0) {
                return source;
            }
            --index;
        }
    }

    return NULL;
}

int64_t ATSParser::Program::getTimeus(unsigned elementaryPID) {

    ssize_t index = mStreams.indexOfKey(elementaryPID);
    int64_t timeus = mStreams.editValueAt(index)->mCurTimeus;
    return timeus;
}
bool ATSParser::Program::hasSource(SourceType type) const {
    for (size_t i = 0; i < mStreams.size(); ++i) {
        const sp<Stream> &stream = mStreams.valueAt(i);
        if (type == AUDIO && stream->isAudio()) {
            return true;
        } else if (type == VIDEO && stream->isVideo()) {
            return true;
        }
    }

    return false;
}

int64_t ATSParser::Program::convertPTSToTimestamp(uint64_t PTS) {
    if (!(mParser->mFlags & TS_TIMESTAMPS_ARE_ABSOLUTE)) {
   /* if (!mFirstPTSValid) {
        mFirstPTSValid = true;
        mFirstPTS = PTS;
            PTS = 0;
    } else */if (PTS < mFirstPTS) {
            PTS = 0;
        } else {
            PTS -= mFirstPTS;
        }
    }

    int64_t timeUs = (PTS * 100) / 9;

    if (mParser->mAbsoluteTimeAnchorUs >= 0ll) {
        timeUs += mParser->mAbsoluteTimeAnchorUs;
    }

    if (mParser->mTimeOffsetValid) {
        timeUs += mParser->mTimeOffsetUs;
    }

    return timeUs;
}

////////////////////////////////////////////////////////////////////////////////

ATSParser::Stream::Stream(
        Program *program,
        unsigned elementaryPID,
        unsigned streamType,
        unsigned PCR_PID)
    : mProgram(program),
      mElementaryPID(elementaryPID),
      mStreamType(streamType),
      mPCR_PID(PCR_PID),
      mExpectedContinuityCounter(-1),
      mPayloadStarted(false),
      mPrevPTS(0),
	  mSeekPayloadStarted(false),
	  lasttimeus(0),
      mDuration(0),
      mCount(0),
      isRigster(false),
      mFormatChange(false),
      player_type(0),
	  is_video(false),
	  mPes_Length(0),
	  mPes_Getlength_Flag(false),
      mQueue(NULL) {
    switch (mStreamType) {
        case STREAMTYPE_H264:
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::H264,
                    (mProgram->parserFlags() & ALIGNED_VIDEO_DATA)
                        ? ElementaryStreamQueue::kFlag_AlignedData : 0);
					is_video = true;
            break;
        case STREAMTYPE_MPEG2_AUDIO_ADTS:
            mQueue = new ElementaryStreamQueue(ElementaryStreamQueue::AAC_ADTS);
            break;
		case STREAMTYPE_MPEG2_AUDIO_LATM:
            mQueue = new ElementaryStreamQueue(ElementaryStreamQueue::AAC_LATM);
            break;
        case STREAMTYPE_MPEG1_AUDIO:
        case STREAMTYPE_MPEG2_AUDIO:
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::MP3);
            break;

        case STREAMTYPE_MPEG1_VIDEO:
        case STREAMTYPE_MPEG2_VIDEO:
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::MPEG2);
			is_video = true;
            break;


	    case STREAMTYPE_AC3:
        case STREAMTYPE_TruHD:
			if(mStreamType == STREAMTYPE_TruHD && mProgram->get_player_type() == 3)
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::PCM_AUDIO);
			else
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::AC3);
            break;
	   	case STREAMTYPE_HEVC:
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::HEVC);
			is_video = true;
            break;

		case STREAMTYPE_VC1:
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::VC1);
			is_video = true;
            break;
      /*  case STREAMTYPE_PCM:
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::PCM);
            break;*/

       /* case STREAMTYPE_MPEG4_VIDEO:
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::MPEG4_VIDEO);
            break;*/

       /*case STREAMTYPE_PCM_AUDIO:
            mQueue = new ElementaryStreamQueue(
                    ElementaryStreamQueue::PCM_AUDIO);
            break;*/
        default:
            break;
    }

    ALOGV("new stream PID 0x%02x, type 0x%02x", elementaryPID, streamType);

    if (mQueue != NULL) {
        mBuffer = new ABuffer(192 * 1024);
        mBuffer->setRange(0, 0);
		mSeekBuffer = new ABuffer(192 * 1024);
		mSeekBuffer->setRange(0,0);
    }
}
void ATSParser::Stream::set_player_type(int type){
      ALOGV("ATSParser::set_player_type %d",type);
	player_type = type;
    if(mQueue != NULL)
	mQueue->set_player_type(player_type);
	return;
}

ATSParser::Stream::~Stream() {
    delete mQueue;
    mQueue = NULL;
}

status_t ATSParser::Stream::parse(
        unsigned continuity_counter,
        unsigned payload_unit_start_indicator, ABitReader *br) {
    if (mQueue == NULL) {
        return OK;
    }
#if 0
    if (mExpectedContinuityCounter >= 0
            && (unsigned)mExpectedContinuityCounter != continuity_counter) {
        ALOGI("discontinuity on stream pid 0x%04x", mElementaryPID);

        mPayloadStarted = false;
        mBuffer->setRange(0, 0);
        mExpectedContinuityCounter = -1;

#if 0
        // Uncomment this if you'd rather see no corruption whatsoever on
        // screen and suspend updates until we come across another IDR frame.

        if (mStreamType == STREAMTYPE_H264) {
            ALOGI("clearing video queue");
            mQueue->clear(true /* clearFormat */);
        }
#endif

        return OK;
    }
#endif
    mExpectedContinuityCounter = (continuity_counter + 1) & 0x0f;

    if (payload_unit_start_indicator) {
        if (mPayloadStarted) {
            // Otherwise we run the danger of receiving the trailing bytes
            // of a PES packet that we never saw the start of and assuming
            // we have a a complete PES packet.

            status_t err = flush();
            mPes_Length = 0;
            mPes_Getlength_Flag = false;
            if (err != OK) {
                return err;
            }
        }

        mPayloadStarted = true;
    }

    if (!mPayloadStarted) {
        return OK;
    }

    size_t payloadSizeBits = br->numBitsLeft();
    CHECK_EQ(payloadSizeBits % 8, 0u);

    size_t neededSize = mBuffer->size() + payloadSizeBits / 8;
    if (mBuffer->capacity() < neededSize) {
        // Increment in multiples of 64K.
        neededSize = (neededSize + 65535) & ~65535;

        ALOGI("resizing buffer to %d bytes", neededSize);

        sp<ABuffer> newBuffer = new ABuffer(neededSize);
        memcpy(newBuffer->data(), mBuffer->data(), mBuffer->size());
        newBuffer->setRange(0, mBuffer->size());
        mBuffer = newBuffer;
    }

    memcpy(mBuffer->data() + mBuffer->size(), br->data(), payloadSizeBits / 8);
    mBuffer->setRange(0, mBuffer->size() + payloadSizeBits / 8);

    if (mBuffer->size() >= 6 && !mPes_Getlength_Flag) {
        ABitReader brTmp(mBuffer->data(), mBuffer->size());
        brTmp.skipBits(32);
        mPes_Length = brTmp.getBits(16);
        mPes_Getlength_Flag = true;
    }
    if((mPes_Length + 6) == mBuffer->size()){
         status_t err = flush();
         mPes_Length = 0;
         mPes_Getlength_Flag = false;
         if (err != OK) {
                return err;
         }
    }
    return OK;
}

bool ATSParser::Stream::isVideo() const {
    switch (mStreamType) {
        case STREAMTYPE_H264:
        case STREAMTYPE_MPEG1_VIDEO:
        case STREAMTYPE_MPEG2_VIDEO:
        case STREAMTYPE_MPEG4_VIDEO:
        case STREAMTYPE_VC1:
        case STREAMTYPE_HEVC:
            return true;

        default:
            return false;
    }
}
void ATSParser::Stream::setLanguage(const char *language)
{
     if(language && mSource != NULL && isAudio())
     {
        ALOGV("language = %s mStreamType = %d",language,mStreamType);
        sp<MetaData> meta = mSource->getFormat();
        meta->setCString(kKeyMediaLanguage,language);
     }
}

bool ATSParser::Stream::isAudio() const {
    switch (mStreamType) {
        case STREAMTYPE_MPEG1_AUDIO:
        case STREAMTYPE_MPEG2_AUDIO:
        case STREAMTYPE_MPEG2_AUDIO_ADTS:
        case STREAMTYPE_MPEG2_AUDIO_LATM:
	//	case STREAMTYPE_PCM_AUDIO:
        case STREAMTYPE_AC3:
        case STREAMTYPE_TruHD:
            return true;

        default:
            return false;
    }
}
status_t  ATSParser::Stream::Seekparse(
        unsigned payload_unit_start_indicator, ABitReader *br) {
       if (mQueue == NULL) {
            return OK;
        }
        if (payload_unit_start_indicator) {
            ALOGV("payload_unit_start_indicator in \n");
            if (mSeekPayloadStarted) {
                // Otherwise we run the danger of receiving the trailing bytes
                // of a PES packet that we never saw the start of and assuming
                // we have a a complete PES packet.
                Seekflush();
            }

            mSeekPayloadStarted = true;
        }

    if (!mSeekPayloadStarted) {
        return OK;
    }

    size_t payloadSizeBits = br->numBitsLeft();

    CHECK_EQ(payloadSizeBits % 8, 0u);

    size_t neededSize = mSeekBuffer->size() + payloadSizeBits / 8;
    if (mSeekBuffer->capacity() < neededSize) {
        // Increment in multiples of 64K.
        neededSize = (neededSize + 65535) & ~65535;

        ALOGI("resizing buffer to %d bytes", neededSize);

        sp<ABuffer> newBuffer = new ABuffer(neededSize);
        memcpy(newBuffer->data(), mSeekBuffer->data(), mSeekBuffer->size());
        newBuffer->setRange(0, mSeekBuffer->size());
        mSeekBuffer = newBuffer;
    }

    memcpy(mSeekBuffer->data() + mSeekBuffer->size(), br->data(), payloadSizeBits / 8);
    mSeekBuffer->setRange(0, mSeekBuffer->size() + payloadSizeBits / 8);

	return OK;

}
void ATSParser::Stream::signalDiscontinuity(
        DiscontinuityType type, const sp<AMessage> &extra) {
    mExpectedContinuityCounter = -1;

    if (mQueue == NULL) {
        return;
    }
    ALOGV("ATSParser Stream::signalDiscontinuity type = %d",type);
    if(type == DISCONTINUITY_PLUSTIME)
    {
       type = (DiscontinuityType)(type|DISCONTINUITY_VIDEO_FORMAT|DISCONTINUITY_TIME);
        if(is_video&&extra != NULL)
        {
            extra->findInt64("start-at-mediatimeUs",&mProgram->mBaseTimeUs);
            mProgram->plusTimeFlag = true;
        }
    }

    if(type&DISCONTINUITY_SEEK)
    {
        mProgram->mLastPTS = 0;
        if(is_video&&extra != NULL)
        {
            extra->findInt64("start-at-mediatimeUs",&mProgram->mBaseTimeUs);
        }
        if(mQueue != NULL)
            mQueue->seekflush();
        if(mSource != NULL)
            mSource->clear();
    }
    mPayloadStarted = false;
    mSeekPayloadStarted = false;
    mPes_Getlength_Flag = false;
    mPes_Length = 0;
    mBuffer->setRange(0, 0);
    mSeekBuffer->setRange(0, 0);
    lasttimeus = 0;

    bool clearFormat = false;
    if (isAudio()) {
        if (type & DISCONTINUITY_AUDIO_FORMAT) {
            clearFormat = true;
        }
        if(mQueue != NULL)
            mQueue->seekflush();
        if(mSource != NULL)
            mSource->clear();
    } else {
        if (type & DISCONTINUITY_VIDEO_FORMAT) {
            if(type&DISCONTINUITY_PLUSTIME){
                mFormatChange = false;
            }else{
            	mFormatChange = true;
            }
            clearFormat = true;
        }
        if(mQueue != NULL)
            mQueue->seekflush();
        if(mSource != NULL)
            mSource->clear();
    }

    mQueue->clear(clearFormat);

    if (type & DISCONTINUITY_TIME) {
        uint64_t resumeAtPTS;
        if (extra != NULL
                && extra->findInt64(
                    IStreamListener::kKeyResumeAtPTS,
                    (int64_t *)&resumeAtPTS)) {
            int64_t resumeAtMediaTimeUs =
                mProgram->convertPTSToTimestamp(resumeAtPTS);


            extra->setInt64("resume-at-mediaTimeUs", resumeAtMediaTimeUs);
                       type = DISCONTINUITY_FORMATCHANGE;
        }
    }

    if (mSource != NULL) {
        mSource->queueDiscontinuity(type, extra, true);
    }
}

void ATSParser::Stream::signalSeek() {
    mSeekPayloadStarted = false;
    if(mQueue != NULL)
    {
        mSeekBuffer->setRange(0,0);
    }
    mCurTimeus = 0;
}
void ATSParser::Stream::signalEOS(status_t finalResult) {
    if (mSource != NULL) {
        mSource->signalEOS(finalResult);
    }
}

status_t ATSParser::Stream::parsePES(ABitReader *br) {
    if(br->numBitsLeft() < 48)
    {
        return OK;
    }
    unsigned packet_startcode_prefix = br->getBits(24);

    ALOGV("packet_startcode_prefix = 0x%08x", packet_startcode_prefix);

    if(packet_startcode_prefix != 0x000001u)
    {
        ALOGV("packet_startcode_prefix is no equal 1");
        return OK;
    }

    unsigned stream_id = br->getBits(8);
    unsigned PES_packet_length = br->getBits(16);
    unsigned extended_stream_id = 0;
    ALOGV("ATSParser::Stream::parsePES PES_packet_length %d",PES_packet_length);
    if (stream_id != 0xbc  // program_stream_map
            && stream_id != 0xbe  // padding_stream
            && stream_id != 0xbf  // private_stream_2
            && stream_id != 0xf0  // ECM
            && stream_id != 0xf1  // EMM
            && stream_id != 0xff  // program_stream_directory
            && stream_id != 0xf2  // DSMCC
            && stream_id != 0xf8) {  // H.222.1 type E
        if(br->numBitsLeft() < 24)
        {
            return OK;
        }
        br->skipBits(8);
  /*    MY_LOGV("PES_scrambling_control = %u", br->getBits(2));
        MY_LOGV("PES_priority = %u", br->getBits(1));
        MY_LOGV("data_alignment_indicator = %u", br->getBits(1));
        MY_LOGV("copyright = %u", br->getBits(1));
        MY_LOGV("original_or_copy = %u", br->getBits(1));*/
        unsigned PTS_DTS_flags = br->getBits(2);
        ALOGV("PTS_DTS_flags = %u", PTS_DTS_flags);

        unsigned ESCR_flag = br->getBits(1);

        ALOGV("ESCR_flag = %u", ESCR_flag);

        unsigned ES_rate_flag = br->getBits(1);

        ALOGV("ES_rate_flag = %u", ES_rate_flag);

        unsigned DSM_trick_mode_flag = br->getBits(1);

        ALOGV("DSM_trick_mode_flag = %u", DSM_trick_mode_flag);

        unsigned additional_copy_info_flag = br->getBits(1);


        unsigned PES_CRC_flag = br->getBits(1);
        unsigned PES_extension_flag = br->getBits(1);

        unsigned PES_header_data_length = br->getBits(8);
        ALOGV("PES_header_data_length = %u", PES_header_data_length);

        unsigned optional_bytes_remaining = PES_header_data_length;

        if(br->numBitsLeft() < PES_header_data_length*8)
        {
            return OK;
        }

        uint64_t PTS = 0, DTS = 0;

        if (PTS_DTS_flags == 2 || PTS_DTS_flags == 3) {
            CHECK_GE(optional_bytes_remaining, 5u);

            if (br->getBits(4) != PTS_DTS_flags) {
                ALOGE("PES data Error!");
                return ERROR_MALFORMED;
            }
            PTS = ((uint64_t)br->getBits(3)) << 30;
            br->getBits(1);
            PTS |= ((uint64_t)br->getBits(15)) << 15;
            br->getBits(1);
            PTS |= br->getBits(15);
            br->getBits(1);

             ALOGV("PTS = %lld", PTS);

            // LOGI("PTS = %.2f secs", PTS / 90000.0f);

            optional_bytes_remaining -= 5;

            if (PTS_DTS_flags == 3) {
                if(optional_bytes_remaining < 5)
                {
                    return OK;
                }
                br->getBits(4);

                DTS = ((uint64_t)br->getBits(3)) << 30;
                br->getBits(1);
                DTS |= ((uint64_t)br->getBits(15)) << 15;
                br->getBits(1);
                DTS |= br->getBits(15);
                br->getBits(1);
                 if(mStreamType == 0x1b ||mStreamType == 0x2b)
                {
                    int64_t timeUs;
                    timeUs = mProgram->convertPTSToTimestamp(DTS);
 //                   LOGE("DTS timeUs = %lld", timeUs);
                }
                optional_bytes_remaining -= 5;
            }
        }

        if(mStreamType == 0x83 && player_type != 3){
        if (ESCR_flag) {
            if(optional_bytes_remaining < 6)
            {
                return OK;
            }
            br->skipBits(48);

          /*  uint64_t ESCR = ((uint64_t)br->getBits(3)) << 30;
            br->getBits(1);
            ESCR |= ((uint64_t)br->getBits(15)) << 15;
            br->getBits(1);
            ESCR |= br->getBits(15);
            br->getBits(1);

            LOGV("ESCR = %llu", ESCR);
            MY_LOGV("ESCR_extension = %u", br->getBits(9));

            br->getBits(1);

            optional_bytes_remaining -= 6;*/
        }

        if (ES_rate_flag) {
            if(optional_bytes_remaining < 3)
            {
                return OK;
            }
            br->skipBits(24);
           /* MY_LOGV("ES_rate = %u", br->getBits(22));

            br->getBits(1);*/

            optional_bytes_remaining -= 3;
        }
            if(DSM_trick_mode_flag){
                if(optional_bytes_remaining < 1)
                {
                    return OK;
                }
                br->skipBits(8);
                optional_bytes_remaining -= 1;
            }
            if(additional_copy_info_flag){
                if(optional_bytes_remaining < 1)
                {
                    return OK;
                }
                br->skipBits(8);
                optional_bytes_remaining -= 1;
            }
            if(PES_CRC_flag){
                if(optional_bytes_remaining < 2){
                    return OK;
                }
                br->skipBits(16);
                optional_bytes_remaining -= 2;
            }
            if(PES_extension_flag){
                uint8_t pes_ext = br->getBits(8);
                optional_bytes_remaining -= 1;
                uint8_t skip = (pes_ext >> 4) & 0xb;
                skip += skip & 0x9;
               if(skip > 0){
                    if(optional_bytes_remaining < skip){
                        return OK;
                    }
                    br->skipBits(skip*8);
                    optional_bytes_remaining -= skip;
               }
               if(((pes_ext & 0x41) == 0x01) && optional_bytes_remaining >= 2){
                    uint8_t data0 = br->getBits(8);
                    uint8_t data1 = br->getBits(8);
                    if ((data0 & 0x7f) > 0 && (data1 & 0x80) == 0)
                        extended_stream_id = data1;
                    optional_bytes_remaining -= 2;
                }
            }
        }

        if(br->numBitsLeft() < optional_bytes_remaining * 8)
        {
            return OK;
        }
       if(mStreamType == 0x83 && player_type !=3){
            if(extended_stream_id != 0x76){
                return OK;
            }
        }
        br->skipBits(optional_bytes_remaining * 8);
        // ES data follows.
        if (PES_packet_length != 0) {
            if(PES_packet_length < (PES_header_data_length + 3))
            {
                return OK;
            }
            unsigned dataLength =
                PES_packet_length - 3 - PES_header_data_length;

           if(br->numBitsLeft() < dataLength * 8)
           {
               //ALOGI("PES packet does not carry enough data to contain payload. (numBitsLeft = %d, required = %d)",
                                  //br->numBitsLeft(), dataLength * 8);
               return OK;
           }
            onPayloadData(
                    PTS_DTS_flags, PTS, DTS, br->data(), br->numBitsLeft()/8);

            br->skipBits(br->numBitsLeft());
        } else {
            onPayloadData(
                    PTS_DTS_flags, PTS, DTS,
                    br->data(), br->numBitsLeft() / 8);

            size_t payloadSizeBits = br->numBitsLeft();
        }
    } else if (stream_id == 0xbe) {  // padding_stream
        br->skipBits(br->numBitsLeft());
    } else {
        br->skipBits(br->numBitsLeft());
    }

    return OK;
}
status_t ATSParser::Stream::SeekparsePES(ABitReader *br) {
    if(br->numBitsLeft() < 48)
    {
    	return OK;
    }
    unsigned packet_startcode_prefix = br->getBits(24);

    ALOGV("packet_startcode_prefix = 0x%08x", packet_startcode_prefix);

    if(packet_startcode_prefix != 0x000001u)
    {
        return OK;
    }

    unsigned stream_id = br->getBits(8);
    ALOGV("stream_id = 0x%02x", stream_id);

    unsigned PES_packet_length = br->getBits(16);
    ALOGV("PES_packet_length = %u", PES_packet_length);

    if (stream_id != 0xbc  // program_stream_map
            && stream_id != 0xbe  // padding_stream
            && stream_id != 0xbf  // private_stream_2
            && stream_id != 0xf0  // ECM
            && stream_id != 0xf1  // EMM
            && stream_id != 0xff  // program_stream_directory
            && stream_id != 0xf2  // DSMCC
            && stream_id != 0xf8) {  // H.222.1 type E
        if(br->numBitsLeft() < 24)
        {
            return OK;
        }
        br->skipBits(8);
        unsigned PTS_DTS_flags = br->getBits(2);
        ALOGV("PTS_DTS_flags = %u", PTS_DTS_flags);
        br->skipBits(6);
        unsigned PES_header_data_length = br->getBits(8);
        ALOGV("PES_header_data_length = %u", PES_header_data_length);

        unsigned optional_bytes_remaining = PES_header_data_length;

        uint64_t PTS = 0, DTS = 0;
        if(br->numBitsLeft()< optional_bytes_remaining * 8)
        {
            return OK;
        }
        if (PTS_DTS_flags == 2 || PTS_DTS_flags == 3) {
            if(optional_bytes_remaining < 5)
            {
                return OK;
            }
            br->getBits(4);

            PTS = ((uint64_t)br->getBits(3)) << 30;
            br->getBits(1);
            PTS |= ((uint64_t)br->getBits(15)) << 15;
            br->getBits(1);
            PTS |= br->getBits(15);
            br->getBits(1);

            ALOGV("PTS = %llu", PTS);
            // LOGI("PTS = %.2f secs", PTS / 90000.0f);

             optional_bytes_remaining -= 5;
             uint64_t timeUs;
             timeUs = mProgram->convertPTSToTimestamp(PTS);
             mCurTimeus = timeUs;
             if(mDuration < timeUs)
             {
                mDuration = timeUs;
             }
        }
        br->skipBits(br->numBitsLeft());
    } else if (stream_id == 0xbe) {  // padding_stream
        br->skipBits(br->numBitsLeft());
    } else {
        br->skipBits(br->numBitsLeft());
    }
    return OK;
}
status_t ATSParser::Stream::Seekflush() {
    if (mSeekBuffer->size() == 0) {
        return OK;
    }

    ALOGV("Seekflush stream 0x%04x size = %d", mElementaryPID, mBuffer->size());

    ABitReader br(mSeekBuffer->data(), mSeekBuffer->size());
    SeekparsePES(&br);

    mSeekBuffer->setRange(0, 0);
    return OK;
}

status_t ATSParser::Stream::flush() {
    if (mBuffer->size() == 0) {
        return OK;
    }

    ALOGV("flushing stream 0x%04x size = %d", mElementaryPID, mBuffer->size());

    ABitReader br(mBuffer->data(), mBuffer->size());

    status_t err = parsePES(&br);

    mBuffer->setRange(0, 0);

    return err;
}

void ATSParser::Stream::onPayloadData(
        unsigned PTS_DTS_flags, uint64_t PTS, uint64_t DTS,
        const uint8_t *data, size_t size) {
#if 0
    ALOGI("payload streamType 0x%02x, PTS = 0x%016llx, dPTS = %lld",
          mStreamType,
          PTS,
          (int64_t)PTS - mPrevPTS);
    mPrevPTS = PTS;
#endif

    ALOGV("onPayloadData mStreamType=0x%02x", mStreamType);

    int64_t timeUs = 0ll;  // no presentation timestamp available.
  	if(PTS_DTS_flags == 2 || PTS_DTS_flags == 3)
    {
        if(is_video || player_type == 6 || //GPU_STRM
            mProgram->noVideoPtsNum > MAX_PACKET_NUM) //if MAX_PACKET_NUM pes packet no video we used audio PTS as base pts add by csy
        {
            if (!mProgram->mFirstPTSValid)
            {
                mProgram->mFirstPTSValid = true;
                mProgram->mFirstPTS = PTS;
            }
//            LOGE("PTS = %lld",PTS);
        }
		else if(player_type == 4/*LIVE_TV*/)
		{
		    if(mProgram->mOnlyAudioFlag && !mProgram->mFirstPTSValid)
			{
				mProgram->mFirstPTSValid = true;
				mProgram->mFirstPTS = PTS;
			}
		}
        if(!mProgram->mFirstPTSValid)
        {
            mProgram->noVideoPtsNum++;
            return;
        }
        if(PTS <  mProgram->mFirstPTS)
        {
            mCount++;
        }
        else
        {
            mCount = 0;
        }

        if(PTS <= (mProgram->mFirstPTS + 100) && is_video){
            if(PTS >= mProgram->mFirstPTS&&!mProgram->plusTimeFlag){
                mProgram->mBaseTimeUs += mProgram->convertPTSToTimestamp(mProgram->mLastPTS);
            }
        }else if(is_video){
            mProgram->mLastPTS = PTS;
        }

        if(mCount > 5 && is_video)
        {
            mProgram->mFirstPTS = PTS;
            mProgram->mrestFlag = true;
            mCount = 0;
            ALOGI("revet time timestamp set PTS %lld",PTS);
        }
        if(mFormatChange && mProgram->mBaseTimeUs)
        {
            mProgram->mBaseTimeUs -= mProgram->convertPTSToTimestamp(PTS);
            mFormatChange = false;
            if( mProgram->mBaseTimeUs < 10000000){
                 mProgram->mBaseTimeUs = 0;
            }
        }
        timeUs = mProgram->convertPTSToTimestamp(PTS);
        if((timeUs + 20000000) < mProgram->mBaseTimeUs &&mProgram->mBaseTimeUs){
            mProgram->mPlusBaseTimeFlag = true;
        }
		//if the player type is live_tv, it mean not need the follow process.
        if(player_type != 4 && mProgram->mPlusBaseTimeFlag){
            timeUs += mProgram->mBaseTimeUs;
        }
    }
    else
    {
        timeUs = -1;
    }
    if(mCount && !is_video)
        timeUs = lasttimeus;
    if((player_type != 4)&&(lasttimeus > 0) &&(timeUs > lasttimeus + 50000000)&&mProgram->mBaseTimeUs)
    {
        timeUs = lasttimeus + 40000;
    }
    lasttimeus = timeUs;

    if(!mProgram->mFirstPTSValid)
    {
        return;
    }
    if(is_video){
        if(mSource != NULL){
            mProgram->hasVideo = true;
        }
    }else{
        if(mSource != NULL){
            mProgram->hasAudio = true;
        }
    }
    struct timeval tv1, tv2;
    gettimeofday(&tv1,NULL);
#ifdef PES_DEBUG
    if(mStreamType == 0x02)
    {
        fwrite(data,1,size,fp);
        fflush(fp);
    }
#endif

    status_t err = mQueue->appendData(data, size, timeUs);
    gettimeofday(&tv2,NULL);

    if (err != OK) {
        return;
    }

    MediaBuffer *accessUnit = NULL;
	if(player_type == 3||player_type==6)
	{
    	if(mStreamType == 0x1b && mSource !=NULL && mSource->numBufferAvailable() != 0)
    		return;
	}
    while ((accessUnit = mQueue->dequeueAccessUnit()) != NULL) {
        if (mSource == NULL) {
            sp<MetaData> meta = mQueue->getFormat();

            if (meta != NULL) {
                ALOGV("Stream PID 0x%08x of type 0x%02x now has data.",
                     mElementaryPID, mStreamType);

                mSource = mQueue->getSource();
                if(mSource == NULL){
                    mSource = new AnotherPacketSource(meta);
                }
				#if 0
                mSource->queueAccessUnit(accessUnit);
				#else
				if(accessUnit->range_length())
				{
					mSource->queueAccessUnit(accessUnit);
				}
				else
				{
					break;
				}
				#endif
            }
        } else if (mQueue->getFormat() != NULL) {
            // After a discontinuity we invalidate the queue's format
            // and won't enqueue any access units to the source until
            // the queue has reestablished the new format.
			if (mSource->getFormat() == NULL) {
                mSource->setFormat(mQueue->getFormat());
            }
            mSource->queueAccessUnit(accessUnit);
        }
    }
    if (mSource == NULL) {
        mSource = mQueue->getSource();
    }
    gettimeofday(&tv2,NULL);
/*    if(mStreamType == 0x2)
    {
        int32_t deat = tv2.tv_usec - tv1.tv_usec;
        if(deat < 0)
        {
            deat = 1000 + deat/1000;
        }
        else
        {
            deat = deat/1000;
        }
        if(deat > 10)
        LOGV("deat2 = %d \n",deat);
    }*/
}

sp<MediaSource> ATSParser::Stream::getSource(SourceType type,unsigned& elementaryPID) {
    switch (type) {
        case VIDEO:
        {
            if (isVideo()) {
				elementaryPID = mElementaryPID;
                return mSource;
            }
            break;
        }

        case AUDIO:
        {
            if (isAudio() && !isRigster) {
                elementaryPID = mElementaryPID;
                if(mSource != NULL)
                {
                    isRigster = true;
                }

                return mSource;
            }
            break;
        }

        default:
            break;
    }

    return NULL;
}

sp<MediaSource> ATSParser::Stream::getSource(SourceType type) {
    switch (type) {
        case VIDEO:
        {
            if (isVideo()) {
                return mSource;
            }
            break;
        }

        case AUDIO:
        {
            if (isAudio()) {
                return mSource;
            }
            break;
        }

        default:
            break;
    }

    return NULL;
}

int64_t ATSParser::Stream::getTimeus(SourceType type) {
        return mCurTimeus;
}////////////////////////////////////////////////////////////////////////////

ATSParser::ATSParser(uint32_t flags)
    : mFlags(flags),
      mAbsoluteTimeAnchorUs(-1ll),
      mTimeOffsetValid(false),
      mTimeOffsetUs(0ll),
      mNumTSPacketsParsed(0),
      mNumPCRs(0) {
    mPSISections.add(0 /* PID */, new PSISection);
    player_type = 0;
	playStart = false;
#ifdef TS_DEBUG
    fp = fopen("/sdcard/net.ts","wb+");
#endif
}

ATSParser::~ATSParser() {
}
void ATSParser::set_player_type(int type){
	player_type = type;
    ALOGV("ATSParser::set_player_type %d mPrograms.size() %d",type,mPrograms.size());

	return;
}
status_t ATSParser::feedTSPacket(const void *data, size_t size) {
    return feedTSPacket(data, size, 0);
}
status_t ATSParser::feedTSPacket(const void *data, size_t size,uint32_t seekflag) {
    kTSPacketSize = size;
    seekFlag = seekflag;
    ABitReader br((const uint8_t *)data, kTSPacketSize);
#ifdef TS_DEBUG
    fwrite(data,1,kTSPacketSize,fp);
    fflush(fp);
#endif
    return parseTS(&br);
}

void ATSParser::createLiveProgramID(unsigned AudioPID,unsigned AudioType,unsigned VideoPID,unsigned VideoType)
{
    unsigned programMapPID = 0xff; //live ts the programe we have chose;
    mPrograms.push(new Program(this,1,programMapPID));
	//program->updateProgramMapPID(programMapPID);
    for (size_t i = 0; i < mPrograms.size(); ++i) {
		if(player_type == 4/*live_tv*/){
            mPrograms.editItemAt(i)->set_player_type(player_type);
    	}
        mPrograms.editItemAt(i)->createLiveStream(AudioPID,AudioType,VideoPID,VideoType);
    }
}
void ATSParser::signalDiscontinuity(
        DiscontinuityType type, const sp<AMessage> &extra) {
    int64_t mediaTimeUs;
    if ((type & DISCONTINUITY_TIME)
            && extra != NULL
            && extra->findInt64(
                IStreamListener::kKeyMediaTimeUs, &mediaTimeUs)) {
        mAbsoluteTimeAnchorUs = mediaTimeUs;
    } else if (type == DISCONTINUITY_ABSOLUTE_TIME) {
        int64_t timeUs;
        CHECK(extra->findInt64("timeUs", &timeUs));

        CHECK(mPrograms.empty());
        mAbsoluteTimeAnchorUs = timeUs;
        return;
    } else if (type == DISCONTINUITY_TIME_OFFSET) {
        int64_t offset;
        CHECK(extra->findInt64("offset", &offset));

        mTimeOffsetValid = true;
        mTimeOffsetUs = offset;
        return;
    }

    for (size_t i = 0; i < mPrograms.size(); ++i) {
        mPrograms.editItemAt(i)->signalDiscontinuity(type, extra);
    }
}

void ATSParser::signalEOS(status_t finalResult) {
    CHECK_NE(finalResult, (status_t)OK);

    for (size_t i = 0; i < mPrograms.size(); ++i) {
        mPrograms.editItemAt(i)->signalEOS(finalResult);
    }
}
void ATSParser::signalSeek() {
    for (size_t i = 0; i < mPrograms.size(); ++i) {
        mPrograms.editItemAt(i)->signalSeek();
    }
}
void ATSParser::parseProgramAssociationTable(ABitReader *br) {
	unsigned char* crc_start_addr = (uint8_t*)(br->data()) ;//+ (br->numBitsLeft()/8));
    unsigned table_id = br->getBits(8);
    ALOGV("  table_id = %u", table_id);
    if (table_id != 0x00u) {
        ALOGE("PAT data error!");
        return ;
    }
    unsigned section_syntax_indictor = br->getBits(1);
    ALOGV("  section_syntax_indictor = %u", section_syntax_indictor);
    //CHECK_EQ(section_syntax_indictor, 1u);
	br->getBits(1);//
    //CHECK_EQ(br->getBits(1), 0u);
    MY_LOGV("  reserved = %u", br->getBits(2));

    unsigned section_length = br->getBits(12);
    ALOGV("  section_length = %u", section_length);
    //CHECK((section_length & 0xc00) == 0);

 /*  MY_LOGV("  transport_stream_id = %u", br->getBits(16));
    MY_LOGV("  reserved = %u", br->getBits(2));
    MY_LOGV("  version_number = %u", br->getBits(5));
    MY_LOGV("  current_next_indicator = %u", br->getBits(1));
    MY_LOGV("  section_number = %u", br->getBits(8));
    MY_LOGV("  last_section_number = %u", br->getBits(8));*/
    br->skipBits(40);

    size_t numProgramBytes = (section_length - 5 /* header */ - 4 /* crc */);
  //  CHECK_EQ((numProgramBytes % 4), 0u);

    for (size_t i = 0; i < numProgramBytes / 4; ++i) {
        unsigned program_number = br->getBits(16);
        ALOGV("    program_number = %u", program_number);

        MY_LOGV("    reserved = %u", br->getBits(3));

        if (program_number == 0) {
            MY_LOGV("    network_PID = 0x%04x", br->getBits(13));
        } else {
            unsigned programMapPID = br->getBits(13);

            ALOGV("    program_map_PID = 0x%04x", programMapPID);

            bool found = false;
            for (size_t index = 0; index < mPrograms.size(); ++index) {
                const sp<Program> &program = mPrograms.itemAt(index);

                if (program->number() == program_number) {
                    program->updateProgramMapPID(programMapPID);
                    found = true;
                    break;
                }
            }

            if (!found) {
                mPrograms.push(
                        new Program(this, program_number, programMapPID));
            }

            if (mPSISections.indexOfKey(programMapPID) < 0) {
                mPSISections.add(programMapPID, new PSISection);
            }
        }
    }

	if(player_type == 3||player_type ==6)
	{
		#if 0
		int crc;
		int crc_calc;
		unsigned char* crc_end_addr = (uint8_t*)br->data();
		crc_calc = __swap32(av_crc(av_crc_get_table(AV_CRC_32_IEEE), -1, crc_start_addr, crc_end_addr - crc_start_addr));
		crc = br->getBits(32);
		if(crc != crc_calc)
		{
			LOGI("  mData %d  bitleft %d CRC = 0x%08x crc_calc %x crc_start_addr %x crc_end_addr %x",	br->data(), br->numBitsLeft(), crc , crc_calc, crc_start_addr, crc_end_addr);
		}
		#endif
	}
	else
    MY_LOGV("  CRC = 0x%08x", br->getBits(32));
	ALOGV("Pat parser playertype %d",player_type);
    if(player_type)
    {
      ALOGV("  set_player_type  mPrograms.size() %d  ", mPrograms.size());
    	for (size_t i = 0; i < mPrograms.size(); ++i) {
            mPrograms.editItemAt(i)->set_player_type(player_type);
    	}
    }
}

status_t ATSParser::parsePID(
        ABitReader *br, unsigned PID,
        unsigned continuity_counter,
        unsigned payload_unit_start_indicator) {
    ssize_t sectionIndex = mPSISections.indexOfKey(PID);

    if (sectionIndex >= 0) {
        sp<PSISection> section = mPSISections.valueAt(sectionIndex);

        if (payload_unit_start_indicator) {
            if (!section->isEmpty()) {
                return ERROR_UNSUPPORTED;
            }
            unsigned skip = br->getBits(8);
            br->skipBits(skip * 8);
        }

       // CHECK((br->numBitsLeft() % 8) == 0);
        status_t err = section->append(br->data(), br->numBitsLeft() / 8);

        if (err != OK) {
            return err;
        }

        if (!section->isComplete()) {
            return OK;
        }

        ABitReader sectionBits(section->data(), section->size());

        if (PID == 0) {
            parseProgramAssociationTable(&sectionBits);
        } else {
            bool handled = false;
            for (size_t i = 0; i < mPrograms.size(); ++i) {
                status_t err;
                if (!mPrograms.editItemAt(i)->parsePSISection(
                            PID, &sectionBits, &err)) {
                    continue;
                }

                if (err != OK) {
                    return err;
                }

                handled = true;
                break;
            }

            if (!handled) {
                mPSISections.removeItem(PID);
                section.clear();
            }
        }

        if (section != NULL) {
            section->clear();
        }

        return OK;
    }

    bool handled = false;
    for (size_t i = 0; i < mPrograms.size(); ++i) {
        status_t err;
        if (mPrograms.editItemAt(i)->parsePID(
                    PID,  continuity_counter, payload_unit_start_indicator, br,seekFlag,&err)) {
            if (err != OK) {
                return err;
            }

            handled = true;
            break;
        }
    }

    if (!handled) {
        ALOGV("PID 0x%04x not handled.", PID);
    }

    return OK;
}

void ATSParser::parseAdaptationField(ABitReader *br, unsigned PID) {
    unsigned adaptation_field_length = br->getBits(8);

    if (adaptation_field_length > 0) {
        unsigned discontinuity_indicator = br->getBits(1);

        if (discontinuity_indicator) {
            ALOGV("PID 0x%04x: discontinuity_indicator = 1 (!!!)", PID);
        }

        br->skipBits(2);
        unsigned PCR_flag = br->getBits(1);

        size_t numBitsRead = 4;

        if (PCR_flag) {
            br->skipBits(4);
            uint64_t PCR_base = br->getBits(32);
            PCR_base = (PCR_base << 1) | br->getBits(1);

            br->skipBits(6);
            unsigned PCR_ext = br->getBits(9);

            // The number of bytes from the start of the current
            // MPEG2 transport stream packet up and including
            // the final byte of this PCR_ext field.
            size_t byteOffsetFromStartOfTSPacket =
                (188 - br->numBitsLeft() / 8);

            uint64_t PCR = PCR_base * 300 + PCR_ext;

            ALOGV("PID 0x%04x: PCR = 0x%016llx (%.2f)",
                  PID, PCR, PCR / 27E6);

            // The number of bytes received by this parser up to and
            // including the final byte of this PCR_ext field.
            size_t byteOffsetFromStart =
                mNumTSPacketsParsed * 188 + byteOffsetFromStartOfTSPacket;

            for (size_t i = 0; i < mPrograms.size(); ++i) {
                updatePCR(PID, PCR, byteOffsetFromStart);
            }

            numBitsRead += 52;
        }

      //  CHECK_GE(adaptation_field_length * 8, numBitsRead);
        unsigned skipLength = adaptation_field_length * 8 - numBitsRead;
		if( skipLength > 0)
		{
			
			if(skipLength > br->numBitsLeft() )
			{
				//ALOGI("parseAdaptationField in4 %d left %d",adaptation_field_length * 8 - numBitsRead,br->numBitsLeft());
				skipLength = br->numBitsLeft();
			}
				
			
			br->skipBits(skipLength);
		}
    }
}

status_t ATSParser::parseTS(ABitReader *br) {
    ALOGV("---");

    unsigned sync_byte = br->getBits(8);
    if( sync_byte != 0x47u ){
        return OK;
    }
    MY_LOGV("transport_error_indicator = %u", br->getBits(1));

    unsigned payload_unit_start_indicator = br->getBits(1);
    ALOGV("payload_unit_start_indicator = %u", payload_unit_start_indicator);

    MY_LOGV("transport_priority = %u", br->getBits(1));

    unsigned PID = br->getBits(13);
    if(playStart)
    {
        bool pass = false;
        for(int i = 0; i< mPIDbuffer.size();i++)
        {
            if(PID == mPIDbuffer.editItemAt(i))
            {
                pass = true;
                break;
            }
        }
        if(!pass)
        {
            return OK;
        }
    }
    ALOGV("PID = 0x%04x", PID);

    MY_LOGV("transport_scrambling_control = %u", br->getBits(2));

    unsigned adaptation_field_control = br->getBits(2);
    ALOGV("adaptation_field_control = %u", adaptation_field_control);

    unsigned continuity_counter = br->getBits(4);
    ALOGV("PID = 0x%04x, continuity_counter = %u", PID, continuity_counter);

    // ALOGI("PID = 0x%04x, continuity_counter = %u", PID, continuity_counter);

    if (adaptation_field_control == 2 || adaptation_field_control == 3) {
        parseAdaptationField(br, PID);
    }

    status_t err = OK;

    if (adaptation_field_control == 1 || adaptation_field_control == 3) {
        err = parsePID(
                br, PID, continuity_counter, payload_unit_start_indicator);
    }

    ++mNumTSPacketsParsed;

    return err;
}

sp<MediaSource> ATSParser::getSource(SourceType type) {
    int which = -1;  // any

    for (size_t i = 0; i < mPrograms.size(); ++i) {
        const sp<Program> &program = mPrograms.editItemAt(i);

        if (which >= 0 && (int)program->number() != which) {
            continue;
        }
        sp<MediaSource> source = mPrograms.editItemAt(i)->getSource(type);

        if (source != NULL) {
            return source;
        }
    }

    return NULL;
}

sp<MediaSource> ATSParser::getSource(SourceType type,uint32_t& ProgramID,unsigned& elementaryPID) {
    int which = -1;  // any

    if(type == VIDEO || player_type == 6)//GPU_STRM
    {
        for (size_t i = 0; i < mPrograms.size(); ++i) {
            const sp<Program> &program = mPrograms.editItemAt(i);

            if (which >= 0 && (int)program->number() != which) {
                continue;
            }
            sp<MediaSource> source = mPrograms.editItemAt(i)->getSource(type,elementaryPID);

            if (source != NULL) {
    		 	ProgramID = i;
                return source;
            }
        }
    }
    else
    {
        sp<MediaSource> source = mPrograms.editItemAt(ProgramID)->getSource(type,elementaryPID);
        if (source != NULL) {
            return source;
        }
    }

    return NULL;
}
int64_t ATSParser::getTimeus(uint32_t ProgramID,unsigned elementaryPID) {
        int64_t timeUs = mPrograms.editItemAt(ProgramID)->getTimeus(elementaryPID);
        return timeUs;
}

void ATSParser::Start(unsigned AudioPID,unsigned VideoPID) {
    playStart = true;
    mVideoPID = VideoPID;
    mAudioPID = AudioPID;
}


bool ATSParser::hasSource(SourceType type) const {
    for (size_t i = 0; i < mPrograms.size(); ++i) {
        const sp<Program> &program = mPrograms.itemAt(i);
        if (program->hasSource(type)) {
            return true;
        }
    }

    return false;
}

bool ATSParser::PTSTimeDeltaEstablished() {
    if (mPrograms.isEmpty()) {
        return false;
    }

    return mPrograms.editItemAt(0)->PTSTimeDeltaEstablished();
}

void ATSParser::updatePCR(
        unsigned PID, uint64_t PCR, size_t byteOffsetFromStart) {
    ALOGV("PCR 0x%016llx @ %d", PCR, byteOffsetFromStart);

    if (mNumPCRs == 2) {
        mPCR[0] = mPCR[1];
        mPCRBytes[0] = mPCRBytes[1];
        mSystemTimeUs[0] = mSystemTimeUs[1];
        mNumPCRs = 1;
    }

    mPCR[mNumPCRs] = PCR;
    mPCRBytes[mNumPCRs] = byteOffsetFromStart;
    mSystemTimeUs[mNumPCRs] = ALooper::GetNowUs();

    ++mNumPCRs;

    if (mNumPCRs == 2) {
        double transportRate =
            (mPCRBytes[1] - mPCRBytes[0]) * 27E6 / (mPCR[1] - mPCR[0]);

        ALOGV("transportRate = %.2f bytes/sec", transportRate);
    }
}

////////////////////////////////////////////////////////////////////////////////

ATSParser::PSISection::PSISection() {
}

ATSParser::PSISection::~PSISection() {
}

status_t ATSParser::PSISection::append(const void *data, size_t size) {
    if (mBuffer == NULL || mBuffer->size() + size > mBuffer->capacity()) {
        size_t newCapacity =
            (mBuffer == NULL) ? size : mBuffer->capacity() + size;

        newCapacity = (newCapacity + 1023) & ~1023;

        sp<ABuffer> newBuffer = new ABuffer(newCapacity);

        if (mBuffer != NULL) {
            memcpy(newBuffer->data(), mBuffer->data(), mBuffer->size());
            newBuffer->setRange(0, mBuffer->size());
        } else {
            newBuffer->setRange(0, 0);
        }

        mBuffer = newBuffer;
    }

    memcpy(mBuffer->data() + mBuffer->size(), data, size);
    mBuffer->setRange(0, mBuffer->size() + size);

    return OK;
}

void ATSParser::PSISection::clear() {
    if (mBuffer != NULL) {
        mBuffer->setRange(0, 0);
    }
}

bool ATSParser::PSISection::isComplete() const {
    if (mBuffer == NULL || mBuffer->size() < 3) {
        return false;
    }

    unsigned sectionLength = U16_AT(mBuffer->data() + 1) & 0xfff;
    return mBuffer->size() >= sectionLength + 3;
}

bool ATSParser::PSISection::isEmpty() const {
    return mBuffer == NULL || mBuffer->size() == 0;
}

const uint8_t *ATSParser::PSISection::data() const {
    return mBuffer == NULL ? NULL : mBuffer->data();
}

size_t ATSParser::PSISection::size() const {
    return mBuffer == NULL ? 0 : mBuffer->size();
}

}  // namespace android
