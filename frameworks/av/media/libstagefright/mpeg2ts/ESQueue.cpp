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
#define LOG_TAG "ESQueue"
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaBuffer.h>

#include "ESQueue.h"
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/foundation/ABitReader.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

#include "include/avc_utils.h"

#include <netinet/in.h>
#include <dlfcn.h>  // for dlopen/dlclose
#include"vpu_api.h"
static void *gHevcParserLibHandle = NULL;

#define LATM_AAC_DEBUG 0
#if LATM_AAC_DEBUG
static FILE *fp = NULL;
void* point = NULL;
#endif
namespace android {
int TS_MPASampleRateTable[4][4] = {{11025, 12000,  8000,   0},     /* MPEG2.5 */
                                   {0,     0,      0,      0},     /* Reserved */
                                   {22050, 24000,  16000,  0},     /* MPEG2 */
                                   {44100, 48000,  32000,  0}};    /* MPEG1 */

int TS_MPABitsRateTable[16][6] = {{0   , 0   , 0   , 0   , 0   , 0  },  //   0000
                                  {32  , 32  , 32  , 32  , 32  , 8  },  //   0001
                                  {64  , 48  , 40  , 64  , 48  , 16 },  //   0010
                                  {96  , 56  , 48  , 96  , 56  , 24 },  //   0011
                                  {128 , 64  , 56  , 128 , 64  , 32 },  //   0100
                                  {160 , 80  , 64  , 160 , 80  , 64 },  //   0101
                                  {192 , 96  , 80  , 192 , 96  , 80 },  //   0110
                                  {224 , 112 , 96  , 224 , 112 , 56 },  //   0111
                                  {256 , 128 , 112 , 256 , 128 , 64 },  //   1000
                                  {288 , 160 , 128 , 288 , 160 , 128},  //   1001
                                  {320 , 192 , 160 , 320 , 192 , 160},  //   1010
                                  {352 , 224 , 192 , 352 , 224 , 112},  //   1011
                                  {384 , 256 , 224 , 384 , 256 , 128},  //   1100
                                  {416 , 320 , 256 , 416 , 320 , 256},  //   1101
                                  {448 , 384 , 320 , 448 , 384 , 320},  //   1110
                                  {-1  , -1  , -1  , -1  , -1  , -1 }}; //   1111
int samplingFreqTable[] =
{
    96000, 88200, 64000, 48000, 44100,
    32000, 24000, 22050, 16000, 12000,
    11025, 8000, 7350
};
int mvl[4/*version*/][4/*layer*/] = {
    {255, 255, 255, 255},
    {255, 255, 255, 255},
    {255, 4, 4, 3},
    {255, 2, 1, 0}
};
int mbitrate[16/*bitrate index*/][5/*mvl*/] = {
    {0, 0, 0, 0, 0},
    {32, 32, 32, 32, 8},
    {64, 48, 40, 48, 16},
    {96, 56, 48, 56, 24},
    {128, 64, 56, 64, 32},
    {160, 80, 64, 80, 40},
    {192, 96, 80, 96, 48},
    {224, 112, 96, 112, 56},
    {256, 128, 112, 128, 64},
    {288, 160, 128, 144, 80},
    {320, 192, 160, 160, 96},
    {352, 224, 192, 176, 112},
    {384, 256, 224, 192, 128},
    {416, 320, 256, 224, 144},
    {448, 384, 320, 256, 160},
    {255, 255, 255, 255, 255}
};
int msamplerate[4/*samplerate index*/][4/*version*/] = {
    {11205, 255, 22050, 44100},
    {12000, 255, 24000, 48000},
    {8000, 255, 16000, 32000},
    {255, 255, 255, 255}
};
//only for latm frame parse
typedef struct AACStruct
{
	uint8_t 		initialized;
	uint8_t 		audio_mux_version_A;
	uint8_t 		frameLengthType;
	int32_t 		frameLength; // faad(may be useful)
} AACStruct;

ElementaryStreamQueue::ElementaryStreamQueue(Mode mode, uint32_t flags)
    : mMode(mode),
	 mBuffer(NULL),
     mFormat(NULL),
     HevcParser_api(NULL),
     hevcparser_handle(NULL),
     mFlags(flags) {

	mFormat_flag = 0;
    lastTimeus = 0;
    appendlastTimeus = 0;
    seekFlag = false;
    FirstIDRFlag = false;
    pktStart = 0;
	startoffset = -1;
    Nextsize = 0;
    Vc1InterlaceFlag = false;
    player_type = 0;
    spsFlag = false;
    ppsFlag = false;
    spsSize = 0;
    SpsPpsBuf = NULL;
    if(mMode == HEVC){
        if(gHevcParserLibHandle == NULL){
            gHevcParserLibHandle = dlopen("/system/lib/librk_hevcdec.so", RTLD_LAZY);
            if (gHevcParserLibHandle == NULL) {
                ALOGI("dlopen hevc_hwdec library fail\n");
            }
        }
        if(gHevcParserLibHandle != NULL){
            HevcParser_api = (RK_HEVC_PAESER_S*)malloc(sizeof(RK_HEVC_PAESER_S));
            if(HevcParser_api != NULL){
                HevcParser_api->init = (void* (*)())dlsym(gHevcParserLibHandle, "libHevcParserInit");

                HevcParser_api->parser = (int (*)(void *hevcparserHandle,
                                void *packet,void *outpacket))dlsym(gHevcParserLibHandle, "libHevcParser");

                HevcParser_api->close = (void (*)(void *hevcparserHandle))dlsym(gHevcParserLibHandle, "libHevcParserClose");
                HevcParser_api->flush = (void (*)(void *hevcparserHandle))dlsym(gHevcParserLibHandle, "libHevcParserflush");
           }
           if((HevcParser_api != NULL) && (HevcParser_api->init != NULL)){
                hevcparser_handle = HevcParser_api->init();
           }

        }

    }

#ifdef ES_DEBUG
    fp = NULL;
   switch (mMode) {
        case H264:
        {
            fp = fopen("/sdcard/264es.data","wb+");
            break;
        }
        case HEVC:{
            fp = fopen("/data/video/hevc_ts.bin","wb+");
            break;
        }
        case AAC_LATM:
        {


			break;
        }

        case AAC_ADTS:
        {
            break;
        }

        case MP3:
        {
            break;
        }
        case MPEG2:
        {
             break;
        }
        case AC3:
        {

            break;
        }
		case VC1:
        {
            break;
        }
        case PCM:
        {
            break;
        }
        default:
            break;
       }
#endif
}

sp<MetaData> ElementaryStreamQueue::getFormat() {
    return mFormat;
}
sp<AnotherPacketSource> ElementaryStreamQueue::getSource() {
    return mSource;
}
void ElementaryStreamQueue::set_player_type(int type)
{
	ALOGV("ElementaryStreamQueue::set_player_type %d",type);
	player_type = type;
	return;
}

void ElementaryStreamQueue::clear(bool clearFormat) {
    if (mBuffer != NULL) {
        mBuffer->setRange(0, 0);
    }
	mTimestamps.clear();
    mRangeInfos.clear();

    if (clearFormat) {
        mFormat.clear();
        if(mMode == H264){
        spsFlag = false;
        ppsFlag = false;
        spsSize = 0;
        }
    }
}
void ElementaryStreamQueue::seekflush() {
    if(mBuffer != NULL)
        mBuffer->setRange(0, 0);
    mTimestamps.clear();
    mRangeInfos.clear();
    seekFlag = true;
    Nextsize  = 0;
    startoffset = -1;
    pktStart = 0;
    lastTimeus = 0;
	appendlastTimeus = 0;
    if(hevcparser_handle != NULL){
        HevcParser_api->flush(hevcparser_handle);
    }
}

static bool IsSeeminglyValidADTSHeader(const uint8_t *ptr, size_t size) {
    if (size < 3) {
        // Not enough data to verify header.
        return false;
    }

    if (ptr[0] != 0xff || (ptr[1] >> 4) != 0x0f) {
        return false;
    }

    unsigned layer = (ptr[1] >> 1) & 3;

    if (layer != 0) {
        return false;
    }

    unsigned ID = (ptr[1] >> 3) & 1;
    unsigned profile_ObjectType = ptr[2] >> 6;

    if (ID == 1 && profile_ObjectType == 3) {
        // MPEG-2 profile 3 is reserved.
        return false;
    }

    return true;
}

static bool IsSeeminglyValidMPEGAudioHeader(const uint8_t *ptr, size_t size) {
    if (size < 3) {
        // Not enough data to verify header.
        return false;
    }

    if (ptr[0] != 0xff || (ptr[1] >> 5) != 0x07) {
        return false;
    }

    unsigned ID = (ptr[1] >> 3) & 3;

    if (ID == 1) {
        return false;  // reserved
    }

    unsigned layer = (ptr[1] >> 1) & 3;

    if (layer == 0) {
        return false;  // reserved
    }

    unsigned bitrateIndex = (ptr[2] >> 4);

    if (bitrateIndex == 0x0f) {
        return false;  // reserved
    }

    unsigned samplingRateIndex = (ptr[2] >> 2) & 3;

    if (samplingRateIndex == 3) {
        return false;  // reserved
    }

    return true;
}

status_t ElementaryStreamQueue::appendData(
        const void *data, size_t size, int64_t timeUs) {
    if (mBuffer == NULL || mBuffer->size() == 0) {
        switch (mMode) {
            case H264:
            {

                break;
            }
            case HEVC:
            {
                break;
            }

            case AAC_LATM:
            {

                ALOGV("AAC_LATM append the data \n");
                uint8_t *ptr = (uint8_t *)data;

                break;
            }

            case AAC_ADTS:
            {
                uint8_t *ptr = (uint8_t *)data;

#if 0
                if (size < 2 || ptr[0] != 0xff || (ptr[1] >> 4) != 0x0f) {
                    return ERROR_MALFORMED;
                }
#else
                ssize_t startOffset = -1;
                for (size_t i = 0; i < size; ++i) {
                    if (IsSeeminglyValidADTSHeader(&ptr[i], size - i)) {
                        startOffset = i;
                        break;
                    }
                }

                if (startOffset < 0) {
                    return ERROR_MALFORMED;
                }

                if (startOffset > 0) {
                    ALOGI("found something resembling an AAC syncword at offset %ld",
                         startOffset);
                }

                data = &ptr[startOffset];
                size -= startOffset;
#endif
                break;
            }

            case MP3:
            {
                break;
            }
            case MPEG2:
            {
                ALOGV("MPEG2 append the data \n");
                break;
            }

            case AC3:
            {
                ALOGV("AC3 append the data \n");

                        break;
            }
			case VC1:
            {
                ALOGV(" VC1 append the data size %d\n",size);
                break;
            }
            case PCM_AUDIO:
            {
                break;
            }

            default:
                return OK;
                break;
        }
    }

    size_t neededSize = (mBuffer == NULL ? 0 : mBuffer->size()) + size;
    if (mBuffer == NULL || neededSize > mBuffer->capacity()) {
        neededSize = (neededSize + 65535) & ~65535;

        ALOGV("resizing buffer to size %d", neededSize);

        sp<ABuffer> buffer = new ABuffer(neededSize);
        if (mBuffer != NULL) {
            memcpy(buffer->data(), mBuffer->data(), mBuffer->size());
            buffer->setRange(0, mBuffer->size());
        } else {
            buffer->setRange(0, 0);
        }

        mBuffer = buffer;
    }

    memcpy(mBuffer->data() + mBuffer->size(), data, size);
    mBuffer->setRange(0, mBuffer->size() + size);
#ifdef ES_DEBUG
 /* if(mMode == H264)
  {
     fwrite(data,1,size,fp);
     fflush(fp);
  }*/
#endif
    //when the mode is H264 the timestampe must use mTimestamps vendor case
    if(mMode == MPEG2 ||mMode == VC1 ||mMode == PCM_AUDIO ||mMode == HEVC || (mMode == H264 && player_type != 4)
            || mMode == AAC_ADTS)
    {
      if(timeUs < 0)
        {
            timeUs = appendlastTimeus;
        }
        appendlastTimeus = timeUs;
        RangeInfo info;
        info.mLength = size;
        info.mTimestampUs = timeUs;
        mRangeInfos.push_back(info);
   }
   else if (timeUs >= 0)
    {
        if (timeUs >0) {
            appendlastTimeus = timeUs;
        }

        if ((appendlastTimeus >0) && (timeUs ==0)) {
            mTimestamps.push_back(appendlastTimeus);
        } else {
            mTimestamps.push_back(timeUs);
        }
    }

    return OK;

}

MediaBuffer * ElementaryStreamQueue::dequeueAccessUnit() {
#if 0
    if ((mFlags & kFlag_AlignedData) && mMode == H264) {
        if (mRangeInfos.empty()) {
            return NULL;
        }

        RangeInfo info = *mRangeInfos.begin();
        mRangeInfos.erase(mRangeInfos.begin());

        sp<ABuffer> accessUnit = new ABuffer(info.mLength);
        memcpy(accessUnit->data(), mBuffer->data(), info.mLength);
        accessUnit->meta()->setInt64("timeUs", info.mTimestampUs);

        memmove(mBuffer->data(),
                mBuffer->data() + info.mLength,
                mBuffer->size() - info.mLength);

        mBuffer->setRange(0, mBuffer->size() - info.mLength);

        if (mFormat == NULL) {
            mFormat = MakeAVCCodecSpecificData(accessUnit);
        }

        return accessUnit;
    }
#endif
    switch (mMode) {
        case H264:
			if(player_type == 3)
				return dequeueAccessUnitH264_Wireless();
			else
            return dequeueAccessUnitH264();
        case HEVC:
            return dequeueAccessUnitHEVC();
        case MPEG2:

            return dequeueAccessUnitMPEG2();
		case VC1:

			 return dequeueAccessUnitVC1();
        case AC3:

            return dequeueAccessUnitAC3();
		case MP3:

			return dequeueAccessUnitMP3();
		case AAC_ADTS:

			return dequeueAccessUnitAAC_ADTS();
        case PCM_AUDIO:
            return dequeueAccessUnitPCMAudio();
        default:
            return NULL;
    }
}


int64_t ElementaryStreamQueue::fetchTimestamp(size_t size) {
    int64_t timeUs = -1;
    bool first = true;

    while (size > 0) {
        if(mRangeInfos.empty()){
            return -1;
        }

        RangeInfo *info = &*mRangeInfos.begin();

        if (first) {
            timeUs = info->mTimestampUs;
            first = false;
        }

        if (info->mLength > size) {
            info->mLength -= size;

            if (first) {
                info->mTimestampUs = -1;
            }

            size = 0;
        } else {
            size -= info->mLength;

            mRangeInfos.erase(mRangeInfos.begin());
            info = NULL;
        }
    }

    if (timeUs == 0ll) {
        ALOGV("Returning 0 timestamp");
    }

    return timeUs;
}

struct NALPosition {
    size_t nalOffset;
    size_t nalSize;
};

MediaBuffer *ElementaryStreamQueue::dequeueAccessUnitH264() {
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();
    Vector<NALPosition> nals;

    size_t totalSize = 0;

    status_t err;
    const uint8_t *nalStart;
    size_t nalSize;
    bool foundSlice = false;
    while ((err = getNextNALUnit(&data, &size, &nalStart, &nalSize)) == OK) {
        //CHECK_GT(nalSize, 0u);
        if(nalSize <= 0)
        {
            continue;
        }
        unsigned nalType = nalStart[0] & 0x1f;
        if((nalType== 7)&& !spsFlag)
        {
        	if(nalSize + 4 > 1024)
			{
				ALOGD("sps is boo big,may be something wrong");
				continue;
			}
            SpsPpsBuf = (uint8_t*)malloc(1024);
            memcpy(SpsPpsBuf,(uint8_t*)nalStart-4,nalSize+4);
            spsSize = nalSize+4;
            spsFlag = true;
        }
        if(spsFlag &&(nalType==8)&& !ppsFlag)
        {
            if(nalSize + spsSize + 4 + 4 > 1024)
			{
				continue;
			}
             memcpy(SpsPpsBuf+spsSize,(uint8_t*)nalStart-4,nalSize+4);
             spsSize += nalSize+4;
             ppsFlag = true;
        }
        if(spsFlag&&ppsFlag&&(mFormat == NULL))
        {
             sp<ABuffer> accessdata = new ABuffer(spsSize);
             if(SpsPpsBuf != NULL) {
             memcpy(accessdata->data(),SpsPpsBuf,spsSize);
             }else{
                continue;
             }
             mFormat = MakeAVCCodecSpecificData(accessdata);
             if(mFormat != NULL){
                mFormat->setInt32(kKeyisTs, 1);
             }
             if(mFormat != NULL && mSource == NULL)
             {
                mSource = new AnotherPacketSource(mFormat);
             }
             if(mFormat != NULL && mSource != NULL)
             {
                mSource->setFormat(mFormat);
             }
             if(SpsPpsBuf)
             {
                free(SpsPpsBuf);
                SpsPpsBuf = NULL;
                if(mFormat == NULL ){
                    spsSize = 0;
                    spsFlag = false;
                    ppsFlag = false;
                }
             }
        }
        bool flush = false;
        if (nalType == 1 || nalType == 5) {
            if (foundSlice) {
                ABitReader br(nalStart + 1, nalSize);
                unsigned first_mb_in_slice = parseUE(&br);

                if (first_mb_in_slice == 0) {
                    // This slice starts a new frame.

                    flush = true;
                }
            }

            foundSlice = true;
        } else if ((nalType == 9 || nalType == 7) && foundSlice) {
            // Access unit delimiter and SPS will be associated with the
            // next frame.

            flush = true;
        }

        if (flush) {
            // The access unit will contain all nal units up to, but excluding
            // the current one, separated by 0x00 0x00 0x00 0x01 startcodes.

            size_t auSize = 4 * nals.size() + totalSize;
            MediaBuffer * accessUnit = new MediaBuffer(auSize);

#if !LOG_NDEBUG
            AString out;
#endif

            size_t dstOffset = 0;
            for (size_t i = 0; i < nals.size(); ++i) {
                const NALPosition &pos = nals.itemAt(i);

                unsigned nalType = mBuffer->data()[pos.nalOffset] & 0x1f;

#if !LOG_NDEBUG
                char tmp[128];
                sprintf(tmp, "0x%02x", nalType);
                if (i > 0) {
                    out.append(", ");
                }
                out.append(tmp);
#endif
                memcpy(accessUnit->data() + dstOffset, "\x00\x00\x00\x01", 4);

                memcpy(accessUnit->data() + dstOffset + 4,
                       mBuffer->data() + pos.nalOffset,
                       pos.nalSize);

                dstOffset += pos.nalSize + 4;
            }
            size_t nextScan = 0;

            const NALPosition &pos = nals.itemAt(nals.size() - 1);
            nextScan = pos.nalOffset + pos.nalSize;
            //remove the zero at the end of frame add by csy
            {
                int32_t endOffset = accessUnit->range_length();
                uint8_t *data1 = (uint8_t*) accessUnit->data();

                while (endOffset > 1 && data1[endOffset - 1] == 0x00) {
                    --endOffset;
                }
                accessUnit->set_range(0,endOffset);
            }
            memmove(mBuffer->data(),
                    mBuffer->data() + nextScan,
                    mBuffer->size() - nextScan);

            mBuffer->setRange(0, mBuffer->size() - nextScan);
            int64_t timeUs = 0;


			if(player_type == 4/*live_tv*/)
			{

				if(mTimestamps.size() == 0)
				{
					timeUs = lastTimeus;
					ALOGV("no timestampe in quen");
				}
				else
				{
					timeUs = *mTimestamps.begin();
					mTimestamps.erase(mTimestamps.begin());
					lastTimeus	= timeUs;
				}
			}
			else
			{
				timeUs = fetchTimestamp(nextScan);

	            if(timeUs < 0){
	                ALOGE("fetch timeUs fail \n");
	                timeUs = 0;
	            }
			}
            accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
            if (mFormat == NULL) {
#ifdef ES_DEBUG
                fwrite(accessUnit->data(),1,accessUnit->range_length(),fp);
                fflush(fp);
#endif
                sp<ABuffer> accessdata = new ABuffer(accessUnit->range_length());
                memcpy(accessdata->data(),(uint8_t *)accessUnit->data(),accessUnit->range_length());
                mFormat = MakeAVCCodecSpecificData(accessdata);
                 if(mFormat != NULL && mSource == NULL)
                 {
                    mSource = new AnotherPacketSource(mFormat);
                 }

                 if(mFormat != NULL && mSource != NULL)
                 {
                    mSource->setFormat(mFormat);
                 }
                 if(mFormat == NULL){
                    accessUnit->release();
                    return NULL;
                 }
            }
            return accessUnit;
        }

        NALPosition pos;
        pos.nalOffset = nalStart - mBuffer->data();
        pos.nalSize = nalSize;
        nals.push(pos);
        totalSize += nalSize;
    }
    if(err != (status_t)-EAGAIN)
    {
        ALOGV("no nal header in this slice");
        mBuffer->setRange(0,0);
    }
    return NULL;
}
MediaBuffer *ElementaryStreamQueue::dequeueAccessUnitHEVC() {
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();
    Vector<NALPosition> nals;
    size_t totalSize = 0;
    status_t err;
    const uint8_t *nalStart;
    size_t nalSize;
    bool foundSlice = false;
    int nextScan = 0;
    if(hevcparser_handle != NULL){
       VideoPacket_t packet;
       ParserOut_t   parserpacket;
       uint8_t *outbuff = NULL;
       int len = 0;
       memset(&packet,0,sizeof(packet));
       memset(&parserpacket,0,sizeof(parserpacket));
       packet.data = mBuffer->data();
       packet.size = size;
       do{
           len = HevcParser_api->parser(hevcparser_handle,&packet,&parserpacket);
           if (mFormat == NULL) {
                if(parserpacket.width > 0){
                    sp<MetaData> meta = new MetaData;
                    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_HEVC);
                   	meta->setInt32(kKeyWidth,parserpacket.width);
            		meta->setInt32(kKeyHeight,parserpacket.height);
                    mFormat = meta;
                    mSource = new AnotherPacketSource(mFormat);
                }
           }
           if(parserpacket.size > 0 && seekFlag){
                if(parserpacket.nFlags != 1){
                   fetchTimestamp(parserpacket.size);
                   parserpacket.size = 0;
                }else{
                    seekFlag = false;
                }
           }
           if(parserpacket.size > 0){
                MediaBuffer * accessUnit = new MediaBuffer(parserpacket.size);
                memcpy(accessUnit->data(),parserpacket.data,parserpacket.size);
                int64_t timeUs = 0;
                timeUs = fetchTimestamp(parserpacket.size);
                accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
                if(mSource != NULL){
                    mSource->queueAccessUnit(accessUnit);
                }else{
                    accessUnit->release();
                    accessUnit = NULL;
                }
           }
           packet.data += len;
           packet.size -= len;
       }while(packet.size);
       mBuffer->setRange(0,0);
       return NULL;
    }else{
        if (mFormat == NULL) {
            ALOGI("creat mFormat");
            sp<MetaData> meta = new MetaData;
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_HEVC);
           	meta->setInt32(kKeyWidth,1280);
    		meta->setInt32(kKeyHeight,720);
            mFormat = meta;
            mSource = new AnotherPacketSource(mFormat);
        }
        while ((err = getNextNALUnit(&data, &size, &nalStart, &nalSize)) == OK) {
            CHECK_GT(nalSize, 0u);
            unsigned nalType = (nalStart[0]>> 1) & 0x3f;
            MediaBuffer * accessUnit = new MediaBuffer(nalSize);
            memcpy(accessUnit->data(),nalStart,nalSize);
            int64_t timeUs = 0;
            accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
            mSource->queueAccessUnit(accessUnit);
            nextScan = nalStart - mBuffer->data() + nalSize;
        }
        if(nextScan){
            memmove(mBuffer->data(),
                            mBuffer->data() + nextScan,
                            mBuffer->size() - nextScan);
            mBuffer->setRange(0, mBuffer->size() - nextScan);
        }
    }
    return NULL;
}
#if 0
MediaBuffer *ElementaryStreamQueue::dequeueAccessUnitH264_Wireless() {
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();
    Vector<NALPosition> nals;

    size_t totalSize = 0;

    status_t err;
    const uint8_t *nalStart;
    size_t nalSize;
    bool foundSlice = false;
	if(mFormat == NULL)
	{
		sp<MetaData> meta = new MetaData;
		meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);

		mFormat = meta;
		mFormat->setInt32(kKeyWidth,1280);
		mFormat->setInt32(kKeyHeight,720);
		MediaBuffer * accessUnit = new MediaBuffer(0);
		return accessUnit;
	}
    while ((err = getNextNALUnit(&data, &size, &nalStart, &nalSize)) == OK) {
        CHECK_GT(nalSize, 0u);
        unsigned nalType = nalStart[0] & 0x1f;
        if((nalType== 7)&& !spsFlag)
        {
        	if(nalSize + 4 > 1024)
			{
				ALOGD("sps is boo big,may be something wrong");
				continue;
			}
            SpsPpsBuf = (uint8_t*)malloc(1024);
            memcpy(SpsPpsBuf,(uint8_t*)nalStart-4,nalSize+4);
            spsSize = nalSize+4;
            spsFlag = true;
        }
        if(spsFlag &&(nalType==8)&& !ppsFlag)
        {
             memcpy(SpsPpsBuf+spsSize,(uint8_t*)nalStart-4,nalSize+4);
             spsSize += nalSize+4;
             ppsFlag = true;
        }
        if(spsFlag&&ppsFlag&&(mFormat == NULL))
        {
             sp<ABuffer> accessdata = new ABuffer(spsSize);
             memcpy(accessdata->data(),SpsPpsBuf,spsSize);
             mFormat = MakeAVCCodecSpecificData(accessdata);
             if(mFormat != NULL && mSource == NULL)
             {
                mSource = new AnotherPacketSource(mFormat);
             }
             if(mFormat != NULL && mSource != NULL)
             {
                mSource->setFormat(mFormat);
             }
             if(SpsPpsBuf)
             {
                free(SpsPpsBuf);
                SpsPpsBuf = NULL;
             }
        }
        bool flush = false;
        if (nalType == 1 || nalType == 5) {
            if (1){//foundSlice) {
                ABitReader br(nalStart + 1, nalSize);
                unsigned first_mb_in_slice = parseUE(&br);

                if (first_mb_in_slice == 0) {
                    // This slice starts a new frame.

                    flush = true;
                }
            }

            foundSlice = true;
        } else if ((nalType == 9 || nalType == 7) && foundSlice) {
            // Access unit delimiter and SPS will be associated with the
            // next frame.

            flush = true;
        }
	  else if(nalType != 9 && nalType != 7 && nalType != 1 && nalType != 5 && nalType != 8   )
	  {
		ALOGV("naltype %d size %d mTimestamps.size() %d",nalType,nalSize,mTimestamps.size());
		if(nalType == 0xe && mTimestamps.size() > 0)
		{
                mTimestamps.erase(mTimestamps.begin());
		}
	 	continue;
	  }
        NALPosition pos;
        pos.nalOffset = nalStart - mBuffer->data();
        pos.nalSize = nalSize;
        nals.push(pos);
        totalSize += nalSize;

        if (flush) {
            // The access unit will contain all nal units up to, but excluding
            // the current one, separated by 0x00 0x00 0x00 0x01 startcodes.

            size_t auSize = 4 * nals.size() + totalSize;
            MediaBuffer * accessUnit = new MediaBuffer(auSize);

#if !LOG_NDEBUG
            AString out;
#endif

            size_t dstOffset = 0;
            for (size_t i = 0; i < nals.size(); ++i) {
                const NALPosition &pos = nals.itemAt(i);

                unsigned nalType = mBuffer->data()[pos.nalOffset] & 0x1f;

#if !LOG_NDEBUG
                char tmp[128];
                sprintf(tmp, "0x%02x", nalType);
                if (i > 0) {
                    out.append(", ");
                }
                out.append(tmp);
#endif
                memcpy(accessUnit->data() + dstOffset, "\x00\x00\x00\x01", 4);

                memcpy(accessUnit->data() + dstOffset + 4,
                       mBuffer->data() + pos.nalOffset,
                       pos.nalSize);

                dstOffset += pos.nalSize + 4;
            }
            size_t nextScan = 0;

            const NALPosition &pos = nals.itemAt(nals.size() - 1);
            nextScan = pos.nalOffset + pos.nalSize;

            memmove(mBuffer->data(),
                    mBuffer->data() + nextScan,
                    mBuffer->size() - nextScan);

            mBuffer->setRange(0, mBuffer->size() - nextScan);
            int64_t timeUs = 0;
            if(mTimestamps.size() == 0)
            {
                timeUs = lastTimeus;
                ALOGV("no timestampe in quen");
            }
            else
            {
                timeUs = *mTimestamps.begin();
                mTimestamps.erase(mTimestamps.begin());
                lastTimeus  = timeUs;
            }

            accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
		#if 0
            if (mFormat == NULL) {

                sp<ABuffer> accessdata = new ABuffer(accessUnit->range_length());
                memcpy(accessdata->data(),(uint8_t *)accessUnit->data(),accessUnit->range_length());
                mFormat = MakeAVCCodecSpecificData(accessdata);
            }
		#else
		if(mFormat_flag==0)
		{
			sp<ABuffer> accessdata = new ABuffer(accessUnit->range_length());
                	memcpy(accessdata->data(),(uint8_t *)accessUnit->data(),accessUnit->range_length());
			mFormat_flag = MakeAVCCodecSpecificData_Wimo(accessdata,mFormat);
                 if(mFormat != NULL && mSource == NULL)
                 {
                    mSource = new AnotherPacketSource(mFormat);
                 }

                 if(mFormat != NULL && mSource != NULL)
                 {
                    mSource->setFormat(mFormat);
                 }
			ALOGD("dequeueAccess 1280 720");
			//mFormat->setInt32(kKeyWidth,1280);
		//	mFormat->setInt32(kKeyHeight,720);
		}
		#endif
            return accessUnit;
        }

    }
    if(err != (status_t)-EAGAIN)
    {
        ALOGE("no nal header in this slice");
        mBuffer->setRange(0,0);
    }
    return NULL;
}
#else
MediaBuffer *ElementaryStreamQueue::dequeueAccessUnitH264_Wireless()

{
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();
    Vector<NALPosition> nals;

    size_t totalSize = 0;

    status_t err;
    const uint8_t *nalStart;
    size_t nalSize;
    bool foundSlice = false;

    while ((err = getNextNALUnit(&data, &size, &nalStart, &nalSize)) == OK) {
 //       CHECK_GT(nalSize, 0u);
        if(nalSize <= 0)
        {
            continue;
        }

        unsigned nalType = nalStart[0] & 0x1f;
        if((nalType== 7)&& !spsFlag)
        {
        	if(nalSize + 4 > 1024)
			{
				ALOGD("sps is boo big,may be something wrong");
				continue;
			}
            SpsPpsBuf = (uint8_t*)malloc(1024);
            memcpy(SpsPpsBuf,(uint8_t*)nalStart-4,nalSize+4);
            spsSize = nalSize+4;
            spsFlag = true;
        }
        if(spsFlag &&(nalType==8)&& !ppsFlag)
        {
            if(nalSize + spsSize + 4 + 4 > 1024)
			{
				continue;
			}
             memcpy(SpsPpsBuf+spsSize,(uint8_t*)nalStart-4,nalSize+4);
             spsSize += nalSize+4;
             ppsFlag = true;
        }
        if(spsFlag&&ppsFlag&&(mFormat == NULL))
        {
             sp<ABuffer> accessdata = new ABuffer(spsSize);

             if(SpsPpsBuf != NULL) {
             memcpy(accessdata->data(),SpsPpsBuf,spsSize);
             }else{
                continue;
             }
             mFormat = MakeAVCCodecSpecificData(accessdata);
             if(mFormat != NULL){
                mFormat->setInt32(kKeyisTs, 1);
             }
             if(mFormat != NULL && mSource == NULL)
             {
                mSource = new AnotherPacketSource(mFormat);
             }
             if(mFormat != NULL && mSource != NULL)
             {
                mSource->setFormat(mFormat);
             }
             if(SpsPpsBuf)
             {
                free(SpsPpsBuf);
                SpsPpsBuf = NULL;
                if(mFormat == NULL ){
                    spsSize = 0;
                    spsFlag = false;
                    ppsFlag = false;
                }
             }
        }
        bool flush = false;
        if (nalType == 1 || nalType == 5) {
            if (foundSlice) {
                ABitReader br(nalStart + 1, nalSize);
                unsigned first_mb_in_slice = parseUE(&br);

                if (first_mb_in_slice == 0) {
                    // This slice starts a new frame.

                    flush = true;
                }
            }

            foundSlice = true;
        } else if ((nalType == 9 || nalType == 7||nalType==8) && foundSlice) {
            // Access unit delimiter and SPS will be associated with the
            // next frame.

            flush = true;
        }
	  else if(0)//nalType != 9 && nalType != 7 && nalType != 1 && nalType != 5 && nalType != 8   )
	  {
		ALOGV("naltype %d size %d mTimestamps.size() %d",nalType,nalSize,mTimestamps.size());
		if(nalType == 0xe && mTimestamps.size() > 0)
		{
                mTimestamps.erase(mTimestamps.begin());
		}
	 	continue;
	  }


        if (flush) {
            // The access unit will contain all nal units up to, but excluding
            // the current one, separated by 0x00 0x00 0x00 0x01 startcodes.

            size_t auSize = 4 * nals.size() + totalSize;
            MediaBuffer * accessUnit = new MediaBuffer(auSize);

#if !LOG_NDEBUG
            AString out;
#endif

            size_t dstOffset = 0;
            for (size_t i = 0; i < nals.size(); ++i) {
                const NALPosition &pos = nals.itemAt(i);

                unsigned nalType = mBuffer->data()[pos.nalOffset] & 0x1f;

#if !LOG_NDEBUG
                char tmp[128];
                sprintf(tmp, "0x%02x", nalType);
                if (i > 0) {
                    out.append(", ");
                }
                out.append(tmp);
#endif
                memcpy(accessUnit->data() + dstOffset, "\x00\x00\x00\x01", 4);

                memcpy(accessUnit->data() + dstOffset + 4,
                       mBuffer->data() + pos.nalOffset,
                       pos.nalSize);

                dstOffset += pos.nalSize + 4;
            }
            size_t nextScan = 0;

            const NALPosition &pos = nals.itemAt(nals.size() - 1);
            nextScan = pos.nalOffset + pos.nalSize;

            memmove(mBuffer->data(),
                    mBuffer->data() + nextScan,
                    mBuffer->size() - nextScan);

            mBuffer->setRange(0, mBuffer->size() - nextScan);
            int64_t timeUs = 0;
			timeUs = fetchTimestamp(nextScan);

	        if(timeUs < 0){
	            ALOGE("fetch timeUs fail \n");
	            timeUs = 0;
			}
            accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
            if (mFormat == NULL) {

                sp<ABuffer> accessdata = new ABuffer(accessUnit->range_length());
                memcpy(accessdata->data(),(uint8_t *)accessUnit->data(),accessUnit->range_length());
                mFormat = MakeAVCCodecSpecificData(accessdata);
                 if(mFormat != NULL && mSource == NULL)
                 {
                    mSource = new AnotherPacketSource(mFormat);
                 }

                 if(mFormat != NULL && mSource != NULL)
                 {
                    mSource->setFormat(mFormat);
                 }
                 if(mFormat == NULL){
                    accessUnit->release();
                    return NULL;
                 }
            }

            return accessUnit;
        }
        NALPosition pos;
        pos.nalOffset = nalStart - mBuffer->data();
        pos.nalSize = nalSize;
        nals.push(pos);
        totalSize += nalSize;

    }
    if(err != (status_t)-EAGAIN)
    {
        ALOGE("no nal header in this slice");
        mBuffer->setRange(0,0);
    }
    return NULL;
}



#endif

MediaBuffer * ElementaryStreamQueue::dequeueAccessUnitMPEG2() {
    const uint8_t *data = mBuffer->data() + Nextsize;
    size_t size = mBuffer->size()- Nextsize;
    size_t auSize = 0;
    size_t offset = 0;
    bool skipFlag = false;
    if(seekFlag){
        for (;;) {

            while (offset < size && data[offset] != 0x01) {
                ++offset;
            }
            if (offset == size) {
                mBuffer->setRange(0,0);
                int64_t timeUs = fetchTimestamp(size);
                ALOGV("seek skip current pes ");
                return NULL;
            }
            if(data[offset - 1] == 0x00 && data[offset - 2] == 0x00&&data[offset+1] == 0x00) {
                ALOGV("seek PICTURE_START_CODE \n");
                size_t tempoffset = offset + 2;
                if(mBuffer->size() - tempoffset > 2)
                {
                    ABitReader bits(mBuffer->data() + tempoffset, mBuffer->size() - tempoffset);
                    bits.skipBits(10);
                    int32_t codeType = bits.getBits(3);
                    if(codeType == 1)
                    {
                        offset -= 2;
                        memmove(mBuffer->data(), mBuffer->data() + offset,
                        mBuffer->size() - offset);
                        mBuffer->setRange(0, mBuffer->size() - offset);
                        seekFlag = false;
                        ALOGV("set seek flag ok");
                        int64_t timeUs = fetchTimestamp(offset);
                        return NULL;
                     }

                }

            }
            ++offset;
        }
    }

    for (;;) {
        while (offset < (size -1) && data[offset] != 0x01) {
            ++offset;
        }
        if(offset < 2)
        {
            ++offset;
            continue;
        }
        if (offset == (size - 1)) {
            if(startoffset >= 0)
            {
                if(startoffset > 0)
                {
                    uint8_t *buf = (uint8_t *)malloc(mBuffer->size() - startoffset);
                    if(buf)
                    {
                         memcpy(buf, mBuffer->data() + startoffset,mBuffer->size() - startoffset);
                         memcpy(mBuffer->data(),buf,mBuffer->size() - startoffset);
                         free(buf);
                    }
                    mBuffer->setRange(0, mBuffer->size() - startoffset);
                    Nextsize += offset - startoffset - 2;
                    fetchTimestamp(startoffset);
                    startoffset = 0;
                }
                else
                {
                     Nextsize += offset - 2;
                }
            }
            return NULL;
            // seqHeader without/with extension
        }
        if (data[offset - 1] == 0x00 && data[offset - 2] == 0x00 && data[offset+1] == 0xB3) {
            ALOGV("SEQUENCE_HEADER_CODE found \n");
            if (mFormat == NULL) {
                size_t tempoffset = offset + Nextsize + 2;
                ALOGV("tempoffset = %d \n",tempoffset);
                ABitReader bits(mBuffer->data() + tempoffset, mBuffer->size() - tempoffset);


                sp<MetaData> meta = new MetaData;
                uint32_t width = bits.getBits(12);
                uint32_t height = bits.getBits(12);
                ALOGD("width = %d,height = %d \n",width,height);
                meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_MPEG2);
                meta->setInt32(kKeyWidth, width);
                meta->setInt32(kKeyHeight, height);
                mFormat = meta;
                if (pktStart != 0)
                {
                    skipFlag = true;
                    offset -= 2;
                    break;
                }
            }
            if (pktStart == 0) {
                startoffset = offset - 2;
			}
            else
            {
                offset -= 2;
                break;
            }
        } else if(data[offset] == 0x01 && data[offset - 1] == 0x00 && data[offset - 2] == 0x00&&data[offset+1] == 0x00) {
            ALOGV("PICTURE_START_CODE \n");
            if (pktStart == 0) {
					pktStart = 1;
					if (startoffset == -1) {
						startoffset = offset - 2;
					}
			}
            else
            {
                offset -= 2;
                break;
            }
        }
        ++offset;
    }
    offset += Nextsize;
    auSize = offset - startoffset;
    Nextsize  = 0;
    startoffset = -1;
    pktStart = 0;
    int64_t timeUs = 0;

    if((mFormat == NULL) || skipFlag ||!auSize)
    {
        uint8_t *buf = (uint8_t *)malloc(mBuffer->size() - offset);
        if(buf)
        {
             memcpy(buf, mBuffer->data() + offset,mBuffer->size() - offset);
             memcpy(mBuffer->data(),buf,mBuffer->size() - offset);
             free(buf);
        }
        ALOGV("skip the data before SEQUENCE_HEADER_CODE found \n");
                mBuffer->setRange(0, mBuffer->size() - offset);
        fetchTimestamp(offset);
                // hexdump(csd->data(), csd->size());


                return NULL;
            }
    timeUs = fetchTimestamp(auSize);
    ALOGV("timeUs input = %lld",timeUs);
    uint32_t temptimeUs = (uint32_t)(timeUs/1000);
    MediaBuffer *accessUnit = new MediaBuffer(auSize+sizeof(TsBitsHeader));
    TsBitsHeader *h = (TsBitsHeader *)accessUnit->data();
    h->start_code = XMEDIA_BITSTREAM_START_CODE;
	h->size = auSize;
	h->time.low_part = (uint32_t)temptimeUs;
	h->time.high_part = 0;
	h->type = 0;
	h->pic_num = 0;
	h->reserved[0] = 0;
	h->reserved[1] = 0;
    memcpy(accessUnit->data()+sizeof(TsBitsHeader),(uint8_t*)mBuffer->data(),auSize);
#ifdef ES_DEBUG
#endif
    {
        uint8_t *buf = (uint8_t *)malloc(mBuffer->size() - offset);
        if(buf)
        {
             memcpy(buf, mBuffer->data() + offset,mBuffer->size() - offset);
             memcpy(mBuffer->data(),buf,mBuffer->size() - offset);
             free(buf);
        }
    }
            // Picture start



                mBuffer->setRange(0, mBuffer->size() - offset);



    accessUnit->meta_data()->setInt64(kKeyTime, timeUs);


                // hexdump(accessUnit->data(), accessUnit->size());

                return accessUnit;
            }
MediaBuffer *ElementaryStreamQueue::dequeueAccessUnitAC3() {
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();
    size_t auSize = size;
    if(!size)
    {
        return NULL;
    }
    if (mFormat == NULL) {
        int32_t kSamplingFreq = 0;
        int32_t channel_configuration = 1;
        uint16_t code = 0xFFFF;
        int TS_AC3SampleRateTable[4] = {48000, 44100, 32000, 0};
        ABitReader bits(mBuffer->data(), mBuffer->size());
        do {
            if(bits.numBitsLeft()){
                code = (code << 8) | bits.getBits(8);
            }
        } while (code != AC3SYNCWORD);
        bits.skipBits(16);
        uint8_t fscod;
        fscod = bits.getBits(8);
        if ((fscod >> 6) == 3) {
            return NULL;
        }
        kSamplingFreq= TS_AC3SampleRateTable[fscod >> 6];
        sp<MetaData> meta = new MetaData;
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AC3);
        meta->setInt32(kKeySampleRate, kSamplingFreq);
        meta->setInt32(kKeyChannelCount, channel_configuration);
        mFormat = meta;
        mSource = new AnotherPacketSource(meta);
    }
    MediaBuffer  *accessUnit = new MediaBuffer(auSize);
    memcpy(accessUnit->data(),(uint8_t *)mBuffer->data(),auSize);
#ifdef ES_DEBUG
#endif
    int64_t timeUs = 0;
    if(mTimestamps.size() > 0)
    {
        timeUs = *mTimestamps.begin();
        lastTimeus = timeUs;
        mTimestamps.erase(mTimestamps.begin());
        }
    else
    {
        timeUs = lastTimeus;
    }
    mBuffer->setRange(0,0);
    accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
    mSource->queueAccessUnit(accessUnit);
    return NULL;
}
MediaBuffer * ElementaryStreamQueue::dequeueAccessUnitVC1() {
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();
    size_t auSize = 0;
    size_t offset = Nextsize;
    bool skipFlag = false;
    uint8_t *tempdata = mBuffer->data();
    if(seekFlag)
    {
        for (;;) {
             while (offset < size && data[offset] != 0x01) {
                ++offset;
            }
            if (offset == size) {
                mBuffer->setRange(0,0);
                int64_t timeUs = fetchTimestamp(size);
                ALOGV("seek skip current pes ");
                return NULL;
            }
            if(data[offset - 1] == 0x00 && data[offset - 2] == 0x00&&data[offset+1] == 0x0D) {
                ALOGV("seek SC_FRAME found \n");
                size_t tempoffset = offset + 2;
                bool IPicTypeFlag = false;
                if(mBuffer->size() - tempoffset > 2)
                {
                    ABitReader bits(mBuffer->data() + tempoffset, mBuffer->size() - tempoffset);
                    int filed_interlace = 0;
                    if(Vc1InterlaceFlag)
                    {
                        if(bits.getBits(1))
                        {
                            filed_interlace = bits.getBits(1);
                        }
                    }
                    if(filed_interlace)
                    {
                        int fieldPicType = bits.getBits(3);
                        if(fieldPicType == 0|| fieldPicType == 1)
                        {
                            IPicTypeFlag = true;
                        }
                    }
                    else
                    {
                        int tmp = bits.getBits(3);
                        if (tmp == 6) {   /* 110b */
                            IPicTypeFlag = true;
                        }
                    }
                    if(IPicTypeFlag)
                    {
                        offset -= 2;
                        memmove(mBuffer->data(), mBuffer->data() + offset,
                        mBuffer->size() - offset);
                        mBuffer->setRange(0, mBuffer->size() - offset);
                        seekFlag = false;
                        int64_t timeUs = fetchTimestamp(offset);
                        return NULL;
                    }
                }
            }
            ++offset;
       }
     }
repe:
    for (;;) {
        while (offset < (size -1) && data[offset] != 0x01) {
            ++offset;
        }
        if(offset < 2)
        {
            ++offset;
            continue;
        }
        if (offset == (size - 1)) {
            if(startoffset >= 0)
            {
                if(startoffset > 0)
                {
                    uint8_t *buf = (uint8_t *)malloc(mBuffer->size() - startoffset);
                    if(buf)
                    {
                         memcpy(buf, mBuffer->data() + startoffset,mBuffer->size() - startoffset);
                         memcpy(mBuffer->data(),buf,mBuffer->size() - startoffset);
                         free(buf);
                    }
                    mBuffer->setRange(0, mBuffer->size() - startoffset);
                    Nextsize += offset - startoffset - 2;
                    startoffset = 0;
                }
                else
                {
                    Nextsize += offset - 2;
                }
            }
            return NULL;
        }
        if (data[offset - 1] == 0x00 && data[offset - 2] == 0x00 && data[offset+1] == 0x0F) {
            ALOGV("SC_SEQ found");
            if (mFormat == NULL) {
                size_t tempoffset = offset + Nextsize + 2;
                ABitReader bits(mBuffer->data() + tempoffset, mBuffer->size() - tempoffset);
                sp<MetaData> meta = new MetaData;
                bits.skipBits(16);
                uint32_t width = bits.getBits(12)*2 + 2;
                uint32_t height = bits.getBits(12)*2 + 2;
                bits.skipBits(1);
                Vc1InterlaceFlag = bits.getBits(1);
                ALOGD("width = %d,height = %d \n",width,height);
                meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_VC1);
                meta->setInt32(kKeyWidth, width);
                meta->setInt32(kKeyHeight, height);
                mFormat = meta;
                mSource = new AnotherPacketSource(meta);
                if (pktStart != 0)
                {
                    skipFlag = true;
                    offset -= 2;
                    break;
                }
            }
            if (pktStart == 0) {
                startoffset = offset - 2;
			}
            else
            {
                offset -= 2;
                break;
            }
        }
        else if(data[offset] == 0x01 && data[offset - 1] == 0x00 && data[offset - 2] == 0x00&&data[offset+1] == 0x0D) {
            ALOGV("SC_FRAME");
            if (pktStart == 0) {
					pktStart = 1;
					if (startoffset == -1) {
						startoffset = offset - 2;
					}
			}
            else
            {
                offset -= 2;
                break;
            }
        }
        ++offset;
    }
    auSize = offset - startoffset;
    Nextsize  = 0;
    startoffset = -1;
    pktStart = 0;
    int64_t timeUs = 0;
    if((mFormat == NULL) || skipFlag ||!auSize)
    {
        uint8_t *buf = (uint8_t *)malloc(mBuffer->size() - offset);
        if(buf)
        {
             memcpy(buf, mBuffer->data() + offset,mBuffer->size() - offset);
             memcpy(mBuffer->data(),buf,mBuffer->size() - offset);
             free(buf);
        }
        ALOGV("skip the data before SEQUENCE_HEADER_CODE found \n");
        mBuffer->setRange(0, mBuffer->size() - offset);
        fetchTimestamp(offset);
        return NULL;
    }
    timeUs = fetchTimestamp(auSize);
    ALOGV("timeUs input = %lld",timeUs);
    MediaBuffer *accessUnit = new MediaBuffer(auSize);
    memcpy(accessUnit->data(),(uint8_t*)tempdata,auSize);
    accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
    mSource->queueAccessUnit(accessUnit);
#ifdef ES_DEBUG
  // fwrite(accessUnit->data(),1,auSize,fp1);
 //  fflush(fp1);
#endif
    tempdata += auSize;
    goto repe;
    return NULL;
}

#if 0
MediaBuffer * ElementaryStreamQueue::dequeueAccessUnitPCM() {
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();
    size_t auSize = size;
    if(!size)
    {
        return NULL;
    }
    int32_t kSamplingFreq = 0;
    int32_t channel_configuration = 0;
    int32_t bits_per_sample = 0;
    if (mFormat == NULL) {
        uint8_t* header = (uint8_t *)mBuffer->data();
        uint8_t channel_layout = header[2] >> 4;
        int32_t sample_rate = 0;
        ALOGV("pcm_bluray_parse_header: header = %02x%02x%02x%02x\n",
                header[0], header[1], header[2], header[3]);
         int32_t bits_per_coded_sample = bits_per_samples[header[3] >> 6];
         if (!(bits_per_coded_sample == 16 || bits_per_coded_sample == 24)) {
            ALOGE("unsupported sample depth (%d)\n", bits_per_coded_sample);
            return NULL;
         }
        switch (header[2] & 0x0f) {
            case 1:
                sample_rate = 48000;
                break;
            case 4:
                sample_rate = 96000;
                break;
            case 5:
                sample_rate = 192000;
                break;
            default:
            sample_rate = 0;
            ALOGE("unsupported sample rate (%d)\n",
                   header[2] & 0x0f);
            return NULL;
        }
        int32_t channelcount = channels[channel_layout];
        if (!channelcount) {
            ALOGE("unsupported channel configuration (%d)\n",channelcount);
            return NULL;
        }
        int32_t bit_rate = channelcount * sample_rate *bits_per_coded_sample;
        sp<MetaData> meta = new MetaData;
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_WAV);
        meta->setInt32(kKeySampleRate, sample_rate);
        meta->setInt32(kKeyBitRate, bits_per_coded_sample);
        meta->setInt32(kKeyChannelCount, channelcount);
        mFormat = meta;
    }
#ifdef ES_DEBUG
#endif
    MediaBuffer * accessUnit = new MediaBuffer(auSize);
    alternateCode((uint16_t *)mBuffer->data(),auSize);
    memcpy(accessUnit->data(),(uint8_t *)mBuffer->data(),auSize);
    int64_t timeUs = 0;
    if(mTimestamps.size() > 0)
    {
        timeUs = *mTimestamps.begin();
        lastTimeus = timeUs;
        mTimestamps.erase(mTimestamps.begin());
    }
    else
    {
        timeUs = lastTimeus;
    }
    accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
    mBuffer->setRange(0,0);
    return accessUnit;
}
#endif

MediaBuffer *ElementaryStreamQueue::dequeueAccessUnitPCMAudio() {
#if 1
	int loop_time = 0;
    unsigned numAUs;
	unsigned quantization_word_length ;
    unsigned audio_sampling_frequency ;
    unsigned num_channels ;
	do
	{
		if (mBuffer->size() < loop_time * 4 + 4 ) {

			if(loop_time > 0)
			{
				memmove(
		            mBuffer->data(),
		            mBuffer->data() + loop_time * 4,
		            mBuffer->size() - loop_time * 4);
		    	mBuffer->setRange(0, mBuffer->size() - loop_time * 4);

			}
        	return NULL;
    	}
		ABitReader bits(mBuffer->data() + loop_time * 4, 4);
	        char pcm_sign = bits.getBits(8);
	    numAUs = bits.getBits(8);
	    bits.skipBits(8);
	    quantization_word_length = bits.getBits(2);
	    audio_sampling_frequency = bits.getBits(3);
	    num_channels = bits.getBits(3);

		static const size_t kFramesPerAU = 80;
		size_t frameSize = 2 /* numChannels */ * sizeof(int16_t);
		#if 0
		CHECK_EQ(pcm_sign, 0xa0);


		CHECK_EQ(audio_sampling_frequency, 2);	// 48kHz
		CHECK_EQ(num_channels, 1u);  // stereo!
		#endif
		if(pcm_sign!=0xa0 || audio_sampling_frequency ==3 || num_channels != 1)
		{

		   loop_time++;
                   ALOGE("malformed PCM Audio AU, loop_time = %d", loop_time);
		   continue;
		}
		else
		{
			if(loop_time!=0)
			{
				memmove(
		            mBuffer->data(),
		            mBuffer->data() + loop_time * 4,
		            mBuffer->size() - loop_time * 4);
	    		mBuffer->setRange(0, mBuffer->size() - loop_time * 4);
			}
			break;
		}
	}while(1);

    if (mFormat == NULL) {
        mFormat = new MetaData;
        mFormat->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
        mFormat->setInt32(kKeyChannelCount, 2);
		if(audio_sampling_frequency == 2)
        mFormat->setInt32(kKeySampleRate, 48000);//8000);
        else
			mFormat->setInt32(kKeySampleRate, 44100);//8000);
    }
    static const size_t kFramesPerAU = 80;
    size_t frameSize = 2 /* numChannels */ * sizeof(int16_t);

    size_t payloadSize = numAUs * frameSize * kFramesPerAU;

    if (mBuffer->size() < 4 + payloadSize) {
        return NULL;
    }

    MediaBuffer * accessUnit = new MediaBuffer(payloadSize);
    memcpy(accessUnit->data(), mBuffer->data() + 4, payloadSize);

    int64_t timeUs = fetchTimestamp(payloadSize + 4);
    CHECK_GE(timeUs, 0ll);
    accessUnit->meta_data()->setInt64(kKeyTime, timeUs);

    int16_t *ptr = (int16_t *)accessUnit->data();
    for (size_t i = 0; i < payloadSize / sizeof(int16_t); ++i) {
        ptr[i] = ntohs(ptr[i]);
    }

    memmove(
            mBuffer->data(),
            mBuffer->data() + 4 + payloadSize,
            mBuffer->size() - 4 - payloadSize);

    mBuffer->setRange(0, mBuffer->size() - 4 - payloadSize);

    return accessUnit;

#else
    if (mBuffer->size() < 4) {
        return NULL;
    }

    ABitReader bits(mBuffer->data(), 4);
    CHECK_EQ(bits.getBits(8), 0xa0);
    unsigned numAUs = bits.getBits(8);
    bits.skipBits(8);
    unsigned quantization_word_length = bits.getBits(2);
    unsigned audio_sampling_frequency = bits.getBits(3);
    unsigned num_channels = bits.getBits(3);

    CHECK_EQ(audio_sampling_frequency, 2);  // 48kHz
    CHECK_EQ(num_channels, 1u);  // stereo!
    if (mFormat == NULL) {
        mFormat = new MetaData;
        mFormat->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
        mFormat->setInt32(kKeyChannelCount, 2);
        mFormat->setInt32(kKeySampleRate, 48000);
    }

    static const size_t kFramesPerAU = 80;
    size_t frameSize = 2 /* numChannels */ * sizeof(int16_t);

    size_t payloadSize = numAUs * frameSize * kFramesPerAU;

    if (mBuffer->size() < 4 + payloadSize) {
        return NULL;
    }

    MediaBuffer * accessUnit = new MediaBuffer(payloadSize);
    memcpy(accessUnit->data(), mBuffer->data() + 4, payloadSize);

    int64_t timeUs = fetchTimestamp(payloadSize + 4);
    CHECK_GE(timeUs, 0ll);
    accessUnit->meta_data()->setInt64(kKeyTime, timeUs);

    int16_t *ptr = (int16_t *)accessUnit->data();
    for (size_t i = 0; i < payloadSize / sizeof(int16_t); ++i) {
        ptr[i] = ntohs(ptr[i]);
    }

    memmove(
            mBuffer->data(),
            mBuffer->data() + 4 + payloadSize,
            mBuffer->size() - 4 - payloadSize);

    mBuffer->setRange(0, mBuffer->size() - 4 - payloadSize);

    return accessUnit;
#endif
}
MediaBuffer * ElementaryStreamQueue::dequeueAccessUnitMP3() {
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();
    size_t auSize = size;
	size_t offset = 0;
    if(size < 2048)
    {

        if(!size)
            {
            return NULL;
        }
        if(mTimestamps.size()> 1)
        {
            int64_t timeUs = *mTimestamps.begin();
            mTimestamps.erase(mTimestamps.begin());
            lastTimeus = timeUs;
        }
        return NULL;
                }
    if (mFormat == NULL) {
        int32_t kSamplingFreq = 0;
        int32_t channel_configuration = 1;
        uint16_t code = 0;
        uint8_t samplerate;
        int version;
        int layer;
        int error_protection;
        int bitsrate;
        int chnmode;
        while (offset + 8 <= mBuffer->size()) {
	        ABitReader bits(mBuffer->data() + offset, mBuffer->size() - offset);
	        code = 0;
	        do {
	            code = (code << 8) | bits.getBits(8);
				offset+=1;
            ALOGV("code11 = 0x%x \n",code);
        } while ((code&0xFFE0) != MPASYNCWORD);
        ALOGV("code = 0x%x \n",code);
        version = (code >> 3) & 0x03;
        layer = (code >> 1) & 0x03;
        error_protection = code & 1;
        bitsrate = bits.getBits(4);
        samplerate = bits.getBits(2);
        bits.skipBits(2); // padding bit, private bit
        chnmode = bits.getBits(2);
        bits.skipBits(6); // mode extension 2 bits, copyright bit, original bit, emphasis 2 bits.
			offset+=2;
	        switch(chnmode){
        	case 0:			//stereo, just set to 2
        		channel_configuration =2;
    		break;

        	case 1:			//joint stereo, just set to 2
        		channel_configuration =2;
        		break;

        	case 2:
        		channel_configuration =2;
        		break;

        	case 3:
        		channel_configuration = 1;
        		break;

    	    default:
        		channel_configuration = 2;
    		break;
        }
        kSamplingFreq = TS_MPASampleRateTable[version][samplerate];
        ALOGV("version::%d,layer::%d samplerate:%d",version,layer,samplerate);
        ALOGV("channel_configuration %d,kSamplingFreq %d",channel_configuration,kSamplingFreq);
			if(kSamplingFreq == 0)
			{
				continue;
			}
			else
			{
        sp<MetaData> meta = new MetaData;
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MPEG);
        meta->setInt32(kKeySampleRate, kSamplingFreq);
        meta->setInt32(kKeyChannelCount, channel_configuration);
        mFormat = meta;
		mFormat_flag = 1;
				break;
            }
	       }
          }
		  if(mFormat==NULL)
   		  {
   		  	memmove(mBuffer->data(), mBuffer->data() + offset,mBuffer->size() - offset);
    		mBuffer->setRange(0, mBuffer->size() - offset);
			return NULL;
   		  }
#ifdef ES_DEBUG
#endif
	MediaBuffer *accessUnit;
	if(mFormat_flag == false)
	{
	    accessUnit = new MediaBuffer(mBuffer->size() - offset + 4);
	    memcpy(accessUnit->data(),mBuffer->data() + offset - 4,mBuffer->size() - offset + 4);
	}
	else
	{
		accessUnit = new MediaBuffer(mBuffer->size());
	    memcpy(accessUnit->data(),mBuffer->data() ,mBuffer->size() );
	}
    int64_t timeUs = 0;
    if(mTimestamps.size() > 0)
            {
        timeUs = *mTimestamps.begin();
        lastTimeus = timeUs;
        mTimestamps.erase(mTimestamps.begin());
            }
    else
            {
        timeUs = lastTimeus;
    }
    mBuffer->setRange(0,0);
    accessUnit->meta_data()->setInt64(kKeyTime, timeUs);
    return accessUnit;
            }


MediaBuffer *ElementaryStreamQueue::dequeueAccessUnitAAC_ADTS() {
    if (mBuffer->size() == 0) {
        return NULL;
    }

    CHECK(!mRangeInfos.empty());

    const RangeInfo &info = *mRangeInfos.begin();
    if (mBuffer->size() < info.mLength) {
        return NULL;
    }

    CHECK_GE(info.mTimestampUs, 0ll);

    // The idea here is consume all AAC frames starting at offsets before
    // info.mLength so we can assign a meaningful timestamp without
    // having to interpolate.
    // The final AAC frame may well extend into the next RangeInfo but
    // that's ok.
    // TODO: the logic commented above is skipped because codec cannot take
    // arbitrary sized input buffers;
    size_t offset = 0;
    while (offset < info.mLength) {
        if (offset + 7 > mBuffer->size()) {
            return NULL;
        }

        ABitReader bits(mBuffer->data() + offset, mBuffer->size() - offset);

        // adts_fixed_header

        CHECK_EQ(bits.getBits(12), 0xfffu);
        bits.skipBits(3);  // ID, layer
        bool protection_absent = bits.getBits(1) != 0;

        if (mFormat == NULL) {
            unsigned profile = bits.getBits(2);
            CHECK_NE(profile, 3u);
            unsigned sampling_freq_index = bits.getBits(4);
            bits.getBits(1);  // private_bit
            unsigned channel_configuration = bits.getBits(3);
            CHECK_NE(channel_configuration, 0u);
            bits.skipBits(2);  // original_copy, home

            mFormat = MakeAACCodecSpecificData(
                    profile, sampling_freq_index, channel_configuration);

            mFormat->setInt32(kKeyIsADTS, true);

            int32_t sampleRate;
            int32_t numChannels;
            CHECK(mFormat->findInt32(kKeySampleRate, &sampleRate));
            CHECK(mFormat->findInt32(kKeyChannelCount, &numChannels));

            ALOGI("found AAC codec config (%d Hz, %d channels)",
                 sampleRate, numChannels);
        } else {
            // profile_ObjectType, sampling_frequency_index, private_bits,
            // channel_configuration, original_copy, home
            bits.skipBits(12);
        }

        // adts_variable_header

        // copyright_identification_bit, copyright_identification_start
        bits.skipBits(2);

        unsigned aac_frame_length = bits.getBits(13);

        bits.skipBits(11);  // adts_buffer_fullness

        unsigned number_of_raw_data_blocks_in_frame = bits.getBits(2);

        if (number_of_raw_data_blocks_in_frame != 0) {
            // To be implemented.
            TRESPASS();
        }

        if (offset + aac_frame_length > mBuffer->size()) {
            return NULL;
        }

        size_t headerSize = protection_absent ? 7 : 9;

        offset += aac_frame_length;
        // TODO: move back to concatenation when codec can support arbitrary input buffers.
        // For now only queue a single buffer
        break;
    }

    int64_t timeUs = fetchTimestampAAC(offset);

    MediaBuffer *accessUnit = new MediaBuffer(offset);
    memcpy(accessUnit->data(), mBuffer->data(), offset);

    memmove(mBuffer->data(), mBuffer->data() + offset,
            mBuffer->size() - offset);
    mBuffer->setRange(0, mBuffer->size() - offset);

    accessUnit->meta_data()->setInt64(kKeyTime, timeUs);

    return accessUnit;
}

//TODO: avoid interpolating timestamps once codec supports arbitrary sized input buffers
int64_t ElementaryStreamQueue::fetchTimestampAAC(size_t size) {
     int64_t timeUs = -1;
     bool first = true;

     size_t samplesize = size;
     while (size > 0) {
         CHECK(!mRangeInfos.empty());

         RangeInfo *info = &*mRangeInfos.begin();

         if (first) {
             timeUs = info->mTimestampUs;
             first = false;
         }

         if (info->mLength > size) {
             int32_t sampleRate;
             CHECK(mFormat->findInt32(kKeySampleRate, &sampleRate));
             info->mLength -= size;
             size_t numSamples = 1024 * size / samplesize;
             info->mTimestampUs += numSamples * 1000000ll / sampleRate;
             size = 0;
         } else {
             size -= info->mLength;

             mRangeInfos.erase(mRangeInfos.begin());
             info = NULL;
         }

     }

     if (timeUs == 0ll) {
         ALOGV("Returning 0 timestamp");
     }

     return timeUs;
 }
ElementaryStreamQueue:: ~ElementaryStreamQueue()
{
    if(SpsPpsBuf)
    {
        free(SpsPpsBuf);
        SpsPpsBuf = NULL;
    }
    mTimestamps.clear();
    mRangeInfos.clear();

    if(HevcParser_api != NULL){
        HevcParser_api->close(hevcparser_handle);
        free(HevcParser_api);
        HevcParser_api = NULL;
        hevcparser_handle = NULL;
    }
}
}  // namespace android
