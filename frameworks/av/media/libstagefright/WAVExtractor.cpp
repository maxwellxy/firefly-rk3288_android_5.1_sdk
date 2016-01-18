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

//#define LOG_NDEBUG 0
#define LOG_TAG "WAVExtractor"
#include <utils/Log.h>

#include "include/WAVExtractor.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>
#include <cutils/bitops.h>

#define CHANNEL_MASK_USE_CHANNEL_ORDER 0

namespace android {

enum {
    WAVE_FORMAT_PCM        = 0x0001,
	WAVE_FORMAT_ADPCM		= 0x0002,
    WAVE_FORMAT_ALAW       = 0x0006,
    WAVE_FORMAT_MULAW      = 0x0007,
    WAVE_FORMAT_DVI_ADPCM  = 0x0011,
    WAVE_FORMAT_MSGSM      = 0x0031,
    WAVE_FORMAT_EXTENSIBLE = 0xFFFE
};

#define SUPPORT_ADPCM 1

#if SUPPORT_ADPCM
#ifndef WaveFormatExStruct
typedef struct 
{
   	uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t	nBlockAlign;
    uint16_t	wBitsPerSample;
    uint16_t	cbSize;
    uint16_t	wSamplesPerBlock;
} WaveFormatExStruct;
#define MSADPCM_MAX_PCM_LENGTH          2048
#define IMAADPCM_MAX_PCM_LENGTH         4096

#endif//PCMWAVEFORMAT
#endif//#if SUPPORT_ADPCM


static const char* WAVEEXT_SUBFORMAT = "\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71";


static uint32_t U32_LE_AT(const uint8_t *ptr) {
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

static uint16_t U16_LE_AT(const uint8_t *ptr) {
    return ptr[1] << 8 | ptr[0];
}

struct WAVSource : public MediaSource {
    WAVSource(
            const sp<DataSource> &dataSource,
            const sp<MetaData> &meta,
            uint16_t waveFormat,
            int32_t bitsPerSample,
            off64_t offset, size_t size);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~WAVSource();

private:
    static const size_t kMaxFrameSize;

    sp<DataSource> mDataSource;
    sp<MetaData> mMeta;
    uint16_t mWaveFormat;
    int32_t mSampleRate;
    int32_t mNumChannels;
    int32_t mBitsPerSample;
    off64_t mOffset;
    size_t mSize;
    bool mStarted;
    MediaBufferGroup *mGroup;
    off64_t mCurrentPos;
#if SUPPORT_ADPCM
	WaveFormatExStruct *mWavExt;
#endif//SUPPORT_ADPCM
    WAVSource(const WAVSource &);
    WAVSource &operator=(const WAVSource &);
};

WAVExtractor::WAVExtractor(const sp<DataSource> &source)
    : mDataSource(source),
      mValidFormat(false),
      mChannelMask(CHANNEL_MASK_USE_CHANNEL_ORDER) {
    mInitCheck = init();
}

WAVExtractor::~WAVExtractor() {
}

sp<MetaData> WAVExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;

    if (mInitCheck != OK) {
        return meta;
    }

    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_WAV);

    return meta;
}

size_t WAVExtractor::countTracks() {
    return mInitCheck == OK ? 1 : 0;
}

sp<MediaSource> WAVExtractor::getTrack(size_t index) {
    if (mInitCheck != OK || index > 0) {
        return NULL;
    }

    return new WAVSource(
            mDataSource, mTrackMeta,
            mWaveFormat, mBitsPerSample, mDataOffset, mDataSize);
}

sp<MetaData> WAVExtractor::getTrackMetaData(
        size_t index, uint32_t flags) {
    if (mInitCheck != OK || index > 0) {
        return NULL;
    }

    return mTrackMeta;
}

status_t WAVExtractor::init() {
    uint8_t header[12];
    if (mDataSource->readAt(
                0, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        return NO_INIT;
    }

    if (memcmp(header, "RIFF", 4) || memcmp(&header[8], "WAVE", 4)) {
        return NO_INIT;
    }
	    // Get file size at the very first time
	if(mDataSource->getSize((off64_t *)&mDataSize)!= OK)
		return NO_INIT;
	
    size_t totalSize = U32_LE_AT(&header[4]);

	//if the file end lose some data ,we must check total size again.
	if(totalSize + 8 > mDataSize)
		totalSize = mDataSize -8;

    off64_t offset = 12;
    size_t remainingSize = totalSize;
#if SUPPORT_ADPCM
	WaveFormatExStruct sWaveFormat;
	/* clear the sWaveFormat structure */
	memset((void *)&sWaveFormat,0, sizeof(sWaveFormat));
#endif//SUPPORT_ADPCM
    while (remainingSize >= 8) {
        uint8_t chunkHeader[8];
        if (mDataSource->readAt(offset, chunkHeader, 8) < 8) {
            return NO_INIT;
        }

        remainingSize -= 8;
        offset += 8;
		
        uint32_t chunkSize = U32_LE_AT(&chunkHeader[4]);

        if (chunkSize > remainingSize) {
			if(!memcmp(chunkHeader, "data", 4)){
				//if the data chunk have wrong chunksize ,maybe the file is broken ,we just give one chance
				chunkSize = remainingSize;
			}
			else
			{
				chunkHeader[4] = '\0';
				ALOGE("chunk %s have wrong chunkSize = %d ,the remainingSize is %d",chunkHeader,chunkSize,remainingSize);
	            return NO_INIT;
			}
        }

        if (!memcmp(chunkHeader, "fmt ", 4)) {
            if (chunkSize < 16) {
                return NO_INIT;
            }
					
            uint8_t formatSpec[40];
            if (mDataSource->readAt(offset, formatSpec, 2) < 2) {
                return NO_INIT;
            }

            mWaveFormat = U16_LE_AT(formatSpec);
            if (mWaveFormat != WAVE_FORMAT_PCM
                    && mWaveFormat != WAVE_FORMAT_ALAW
                    && mWaveFormat != WAVE_FORMAT_MULAW
                    && mWaveFormat != WAVE_FORMAT_MSGSM
#if SUPPORT_ADPCM
                    && mWaveFormat != WAVE_FORMAT_ADPCM
                    && mWaveFormat != WAVE_FORMAT_DVI_ADPCM
#endif
                    && mWaveFormat != WAVE_FORMAT_EXTENSIBLE) {
                return ERROR_UNSUPPORTED;
            }

            uint8_t fmtSize = 16;
            if (mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
                fmtSize = 40;
            }
#if SUPPORT_ADPCM
			else if(mWaveFormat == WAVE_FORMAT_ADPCM){
				fmtSize = sizeof(sWaveFormat);//have more data like coef ,but I don't want it
			}else if(mWaveFormat == WAVE_FORMAT_DVI_ADPCM){
				fmtSize = 20;
			}
#endif
			
            if (mDataSource->readAt(offset, formatSpec, fmtSize) < fmtSize) {
                return NO_INIT;
            }
#if SUPPORT_ADPCM
			if(mWaveFormat == WAVE_FORMAT_ADPCM || mWaveFormat == WAVE_FORMAT_DVI_ADPCM){
				
				memcpy((void *)&sWaveFormat, formatSpec, fmtSize);

				mNumChannels = sWaveFormat.nChannels;
				mSampleRate =  sWaveFormat.nSamplesPerSec;
				mBitsPerSample = sWaveFormat.wBitsPerSample;
				
				
				if(mWaveFormat == WAVE_FORMAT_ADPCM){
						/* sanity check */
						if ((sWaveFormat.nChannels > 2) ||
							(sWaveFormat.nBlockAlign > 4096) ||
							(sWaveFormat.wBitsPerSample != 4) ||
							(sWaveFormat.cbSize != 32) ||
							(sWaveFormat.wSamplesPerBlock > MSADPCM_MAX_PCM_LENGTH))
							{
								//
								// The wave format does not pass the sanity checks, so we can
								// not decode this file.
								//
								ALOGE("WAVE_FORMAT_ADPCM para check error");
								return NO_INIT;
							}
				}else{
					/* sanity check */
						if ((sWaveFormat.nChannels > 2) ||
								(sWaveFormat.nBlockAlign > 4096) ||
								(sWaveFormat.cbSize != 2) ||
								(sWaveFormat.wSamplesPerBlock > IMAADPCM_MAX_PCM_LENGTH))
								{
									//
									// The wave format does not pass the sanity checks, so we can
									// not decode this file.
									//
									ALOGE("WAVE_FORMAT_DVI_ADPCM para check error");
									return NO_INIT;
								}
				}
					
			}else{
#endif//SUPPORT_ADPCM			
	            mNumChannels = U16_LE_AT(&formatSpec[2]);
	            if (mWaveFormat != WAVE_FORMAT_EXTENSIBLE) {
	                if (mNumChannels != 1 && mNumChannels != 2) {
	                    ALOGW("More than 2 channels (%d) in non-WAVE_EXT, unknown channel mask",
	                            mNumChannels);
	                }
	            } else {
	                if (mNumChannels < 1 && mNumChannels > 8) {
	                    return ERROR_UNSUPPORTED;
	                }
	            }

	            mSampleRate = U32_LE_AT(&formatSpec[4]);

	            if (mSampleRate == 0) {
	                return ERROR_MALFORMED;
	            }

	            mBitsPerSample = U16_LE_AT(&formatSpec[14]);

	            if (mWaveFormat == WAVE_FORMAT_PCM
	                    || mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
	                if (mBitsPerSample != 8 && mBitsPerSample != 16
	                    && mBitsPerSample != 24) {
	                    return ERROR_UNSUPPORTED;
	                }
	            } else if (mWaveFormat == WAVE_FORMAT_MSGSM) {
	                if (mBitsPerSample != 0) {
	                    return ERROR_UNSUPPORTED;
	                }
	            } else {
	                CHECK(mWaveFormat == WAVE_FORMAT_MULAW
	                        || mWaveFormat == WAVE_FORMAT_ALAW);
	                if (mBitsPerSample != 8) {
	                    return ERROR_UNSUPPORTED;
	                }
	            }

	            if (mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
	                uint16_t validBitsPerSample = U16_LE_AT(&formatSpec[18]);
	                if (validBitsPerSample != mBitsPerSample) {
	                    if (validBitsPerSample != 0) {
	                        ALOGE("validBits(%d) != bitsPerSample(%d) are not supported",
	                                validBitsPerSample, mBitsPerSample);
	                        return ERROR_UNSUPPORTED;
	                    } else {
	                        // we only support valitBitsPerSample == bitsPerSample but some WAV_EXT
	                        // writers don't correctly set the valid bits value, and leave it at 0.
	                        ALOGW("WAVE_EXT has 0 valid bits per sample, ignoring");
	                    }
	                }

	                mChannelMask = U32_LE_AT(&formatSpec[20]);
	                ALOGV("numChannels=%d channelMask=0x%x", mNumChannels, mChannelMask);
	                if ((mChannelMask >> 18) != 0) {
	                    ALOGE("invalid channel mask 0x%x", mChannelMask);
	                    return ERROR_MALFORMED;
	                }

	                if ((mChannelMask != CHANNEL_MASK_USE_CHANNEL_ORDER)
	                        && (popcount(mChannelMask) != mNumChannels)) {
	                    ALOGE("invalid number of channels (%d) in channel mask (0x%x)",
	                            popcount(mChannelMask), mChannelMask);
	                    return ERROR_MALFORMED;
	                }

	                // In a WAVE_EXT header, the first two bytes of the GUID stored at byte 24 contain
	                // the sample format, using the same definitions as a regular WAV header
	                mWaveFormat = U16_LE_AT(&formatSpec[24]);
	                if (mWaveFormat != WAVE_FORMAT_PCM
	                        && mWaveFormat != WAVE_FORMAT_ALAW
	                        && mWaveFormat != WAVE_FORMAT_MULAW) {
	                    return ERROR_UNSUPPORTED;
	                }
	                if (memcmp(&formatSpec[26], WAVEEXT_SUBFORMAT, 14)) {
	                    ALOGE("unsupported GUID");
	                    return ERROR_UNSUPPORTED;
	                }
	            }
#if SUPPORT_ADPCM
			}
#endif//SUPPORT_ADPCM
            mValidFormat = true;
        } else if (!memcmp(chunkHeader, "data", 4)) {
            if (mValidFormat) {
				
                mDataOffset = offset;
                mDataSize = chunkSize;

                mTrackMeta = new MetaData;

                switch (mWaveFormat) {
                    case WAVE_FORMAT_PCM:
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
                        break;
                    case WAVE_FORMAT_ALAW:
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_G711_ALAW);
                        break;
                    case WAVE_FORMAT_MSGSM:
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MSGSM);
                        break;
#if SUPPORT_ADPCM
					case WAVE_FORMAT_ADPCM:
					case WAVE_FORMAT_DVI_ADPCM:
						mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_WAV);
						mTrackMeta->setData(kKeyWavExtInfo, 0, &sWaveFormat, sizeof(sWaveFormat));
						break;
#endif//SUPPORT_ADPCM
                    default:
                        CHECK_EQ(mWaveFormat, (uint16_t)WAVE_FORMAT_MULAW);
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_G711_MLAW);
                        break;
                }

                mTrackMeta->setInt32(kKeyChannelCount, mNumChannels);
                mTrackMeta->setInt32(kKeyChannelMask, mChannelMask);
                mTrackMeta->setInt32(kKeySampleRate, mSampleRate);

                int64_t durationUs = 0;
                if (mWaveFormat == WAVE_FORMAT_MSGSM) {
                    // 65 bytes decode to 320 8kHz samples
                    durationUs =
                        1000000LL * (mDataSize / 65 * 320) / 8000;
#if SUPPORT_ADPCM
                } else if(mWaveFormat == WAVE_FORMAT_ADPCM || mWaveFormat == WAVE_FORMAT_DVI_ADPCM){
                	//reset the mNumSamples value
					uint32_t totalSamples = (mDataSize/sWaveFormat.nBlockAlign)*sWaveFormat.wSamplesPerBlock;//
					uint32_t duration_sec = totalSamples / mSampleRate;
				    uint32_t duration_msec = totalSamples % mSampleRate;
				    durationUs = (duration_msec * 1000000LL) / mSampleRate + duration_sec * 1000000LL ;
					mTrackMeta->setInt32(kKeySampleRate, mSampleRate);
#endif//SUPPORT_ADPCM				                	
                }else{
                    size_t bytesPerSample = mBitsPerSample >> 3;
                    durationUs =
                        1000000LL * (mDataSize / (mNumChannels * bytesPerSample))
                            / mSampleRate;
                }

                mTrackMeta->setInt64(kKeyDuration, durationUs);

                return OK;
            }
        }

        offset += chunkSize;
		remainingSize -= chunkSize;
    }

    return NO_INIT;
}

const size_t WAVSource::kMaxFrameSize = 32768;

WAVSource::WAVSource(
        const sp<DataSource> &dataSource,
        const sp<MetaData> &meta,
        uint16_t waveFormat,
        int32_t bitsPerSample,
        off64_t offset, size_t size)
    : mDataSource(dataSource),
      mMeta(meta),
      mWaveFormat(waveFormat),
      mSampleRate(0),
      mNumChannels(0),
      mBitsPerSample(bitsPerSample),
      mOffset(offset),
      mSize(size),
      mStarted(false),
      mGroup(NULL) {
    CHECK(mMeta->findInt32(kKeySampleRate, &mSampleRate));
    CHECK(mMeta->findInt32(kKeyChannelCount, &mNumChannels));
#if SUPPORT_ADPCM
	mWavExt = NULL;
	const void * tmpData = NULL;
	size_t tmpSize = 0;
	uint32_t tmpType = 0;
	if(mMeta->findData(kKeyWavExtInfo,&tmpType,&tmpData,&tmpSize)){
		mWavExt = new WaveFormatExStruct;
		if(mWavExt)
			memcpy((void *)mWavExt,tmpData,sizeof(WaveFormatExStruct));
	}
#endif//SUPPORT_ADPCM
    mMeta->setInt32(kKeyMaxInputSize, kMaxFrameSize);
}

WAVSource::~WAVSource() {
    if (mStarted) {
        stop();
    }
}

status_t WAVSource::start(MetaData *params) {
    ALOGV("WAVSource::start");

    CHECK(!mStarted);
#if SUPPORT_ADPCM
	if((mWaveFormat == WAVE_FORMAT_ADPCM || mWaveFormat == WAVE_FORMAT_DVI_ADPCM) && mWavExt == NULL){
		ALOGI("ADPCM have none info, must be error");
		return ERROR_UNSUPPORTED;
			
	}
#endif//SUPPORT_ADPCM
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(new MediaBuffer(kMaxFrameSize));

    if (mBitsPerSample == 8) {
        // As a temporary buffer for 8->16 bit conversion.
        mGroup->add_buffer(new MediaBuffer(kMaxFrameSize));
    }

    mCurrentPos = mOffset;

    mStarted = true;
	
	
    return OK;
}

status_t WAVSource::stop() {
    ALOGV("WAVSource::stop");

    CHECK(mStarted);

    delete mGroup;
    mGroup = NULL;
#if SUPPORT_ADPCM
	if(mWavExt){
		delete mWavExt;
		mWavExt = NULL;
	}
#endif//SUPPORT_ADPCM
    mStarted = false;
	
    return OK;
}

sp<MetaData> WAVSource::getFormat() {
    ALOGV("WAVSource::getFormat");

    return mMeta;
}

status_t WAVSource::read(
        MediaBuffer **out, const ReadOptions *options) {
    *out = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    if (options != NULL && options->getSeekTo(&seekTimeUs, &mode)) {
        int64_t pos = 0;

        if (mWaveFormat == WAVE_FORMAT_MSGSM) {
            // 65 bytes decode to 320 8kHz samples
            int64_t samplenumber = (seekTimeUs * mSampleRate) / 1000000;
            int64_t framenumber = samplenumber / 320;
            pos = framenumber * 65;
#if SUPPORT_ADPCM
		} else if(mWaveFormat == WAVE_FORMAT_ADPCM|| mWaveFormat == WAVE_FORMAT_DVI_ADPCM){
			int64_t numSamples = (seekTimeUs * mSampleRate) / 1000000LL;
			int64_t blockCounter = numSamples/mWavExt->wSamplesPerBlock;
			blockCounter += ((numSamples%mWavExt->wSamplesPerBlock) > (mWavExt->wSamplesPerBlock >> 1))?1:0;
			pos = blockCounter * mWavExt->nBlockAlign;
#endif
		}else{
            pos = (seekTimeUs * mSampleRate) / 1000000 * mNumChannels * (mBitsPerSample >> 3);
        }
        if (pos > mSize) {
            pos = mSize;
        }
        mCurrentPos = pos + mOffset;
    }

    MediaBuffer *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }

#if SUPPORT_ADPCM
	if(mWaveFormat == WAVE_FORMAT_ADPCM || mWaveFormat == WAVE_FORMAT_DVI_ADPCM)
	{
		//whether reach end of the file
		if(mWavExt->nBlockAlign + mCurrentPos - mOffset > mSize)
		{
			buffer->release();
			buffer = NULL;
			return ERROR_END_OF_STREAM;
		}

		size_t n = mDataSource->readAt(mCurrentPos, buffer->data(), mWavExt->nBlockAlign);
		if (n <= 0)
		{
			buffer->release();
			buffer = NULL;
			return ERROR_END_OF_STREAM;
		}
		
		//update the file offset
		mCurrentPos += mWavExt->nBlockAlign;

		 buffer->meta_data()->setInt64(kKeyTime,(1000000LL*(mCurrentPos - mOffset)*mWavExt->wSamplesPerBlock)/(mWavExt->nBlockAlign*mSampleRate));
		
		 buffer->set_range(0, mWavExt->nBlockAlign);

	}else{
#endif
	    // make sure that maxBytesToRead is multiple of 3, in 24-bit case
	    size_t maxBytesToRead =
	        mBitsPerSample == 8 ? kMaxFrameSize / 2 : 
	        (mBitsPerSample == 24 ? 3*(kMaxFrameSize/3): kMaxFrameSize);

	    size_t maxBytesAvailable =
	        (mCurrentPos - mOffset >= (off64_t)mSize)
	            ? 0 : mSize - (mCurrentPos - mOffset);

	    if (maxBytesToRead > maxBytesAvailable) {
	        maxBytesToRead = maxBytesAvailable;
	    }

	    if (mWaveFormat == WAVE_FORMAT_MSGSM) {
	        // Microsoft packs 2 frames into 65 bytes, rather than using separate 33-byte frames,
	        // so read multiples of 65, and use smaller buffers to account for ~10:1 expansion ratio
	        if (maxBytesToRead > 1024) {
	            maxBytesToRead = 1024;
	        }
	        maxBytesToRead = (maxBytesToRead / 65) * 65;
	    }

	    ssize_t n = mDataSource->readAt(
	            mCurrentPos, buffer->data(),
	            maxBytesToRead);

	    if (n <= 0) {
	        buffer->release();
	        buffer = NULL;

	        return ERROR_END_OF_STREAM;
	    }

	    buffer->set_range(0, n);

	    if (mWaveFormat == WAVE_FORMAT_PCM || mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
	        if (mBitsPerSample == 8) {
	            // Convert 8-bit unsigned samples to 16-bit signed.

	            MediaBuffer *tmp;
	            CHECK_EQ(mGroup->acquire_buffer(&tmp), (status_t)OK);

	            // The new buffer holds the sample number of samples, but each
	            // one is 2 bytes wide.
	            tmp->set_range(0, 2 * n);

	            int16_t *dst = (int16_t *)tmp->data();
	            const uint8_t *src = (const uint8_t *)buffer->data();
	            ssize_t numBytes = n;

	            while (numBytes-- > 0) {
	                *dst++ = ((int16_t)(*src) - 128) * 256;
	                ++src;
	            }

	            buffer->release();
	            buffer = tmp;
	        } else if (mBitsPerSample == 24) {
	            // Convert 24-bit signed samples to 16-bit signed.

	            const uint8_t *src =
	                (const uint8_t *)buffer->data() + buffer->range_offset();
	            int16_t *dst = (int16_t *)src;

	            size_t numSamples = buffer->range_length() / 3;
	            for (size_t i = 0; i < numSamples; ++i) {
	                int32_t x = (int32_t)(src[0] | src[1] << 8 | src[2] << 16);
	                x = (x << 8) >> 8;  // sign extension

	                x = x >> 8;
	                *dst++ = (int16_t)x;
	                src += 3;
	            }

	            buffer->set_range(buffer->range_offset(), 2 * numSamples);
	        }
	    }

	    int64_t timeStampUs = 0;

	    if (mWaveFormat == WAVE_FORMAT_MSGSM) {
	        timeStampUs = 1000000LL * (mCurrentPos - mOffset) * 320 / 65 / mSampleRate;
	    } else {
	        size_t bytesPerSample = mBitsPerSample >> 3;
	        timeStampUs = 1000000LL * (mCurrentPos - mOffset)
	                / (mNumChannels * bytesPerSample) / mSampleRate;
	    }

	    buffer->meta_data()->setInt64(kKeyTime, timeStampUs);
		 mCurrentPos += n;
#if SUPPORT_ADPCM
	}
#endif//SUPPORT_ADPCM
    buffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);
   
    *out = buffer;

    return OK;
}

////////////////////////////////////////////////////////////////////////////////

bool SniffWAV(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *) {
    char header[12];
    if (source->readAt(0, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        return false;
    }

    if (memcmp(header, "RIFF", 4) || memcmp(&header[8], "WAVE", 4)) {
        return false;
    }

    sp<MediaExtractor> extractor = new WAVExtractor(source);
    if (extractor->countTracks() == 0) {
        return false;
    }

    *mimeType = MEDIA_MIMETYPE_CONTAINER_WAV;
    *confidence = WAV_CONTAINER_CONFIDENCE;

    return true;
}

}  // namespace android
