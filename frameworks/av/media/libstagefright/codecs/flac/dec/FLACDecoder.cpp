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

#include "FLACDecoder.h"


// libFLAC parser
#include "FLAC/stream_decoder.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>


#undef LOG_TAG
#define LOG_TAG "FLACDecoder"

#define FLAC_DEC_DEBUG 0

#if FLAC_DEC_DEBUG
#define FLAC_LOG ALOGD
#else
#define FLAC_LOG
#endif


#define OUTPUT_BUFFER_SIZE_FLAC (1024*16)

static const int8_t sample_size_table[] = { 0, 8, 12, 0, 16, 20, 24, 0 };

namespace android {

// RK_FLACParser wraps a C libFLAC parser aka stream decoder

class RK_FLACParser : public RefBase {

public:
    RK_FLACParser(
        FLACDecoder* pFlacdec,
        // If metadata pointers aren't provided, we don't fill them
        const sp<MetaData> &fileMetadata = 0,
        const sp<MetaData> &trackMetadata = 0);

    status_t initCheck() const {
        return mInitCheck;
    }

    // stream properties
    unsigned getMaxBlockSize() const {
        //return mStreamInfo.max_blocksize;
        return mFlacDec->mMaxBlockSize;
    }
    unsigned getSampleRate() const {
        //return mStreamInfo.sample_rate;
        return mFlacDec->mSampleRate;
    }
    unsigned getChannels() const {
        //return mStreamInfo.channels;
        return mFlacDec->mNumChannels;
    }
    unsigned getBitsPerSample() const {
        //return mStreamInfo.bits_per_sample;
        return mFlacDec->mBitDepth;
    }
    FLAC__uint64 getTotalSamples() const {
        return mStreamInfo.total_samples;
    }

    // media buffers
    void allocateBuffers();
    void releaseBuffers();
    MediaBuffer *readBuffer() {
        return readBuffer(false, 0LL);
    }
    MediaBuffer *readBuffer(FLAC__uint64 sample) {
        return readBuffer(true, sample);
    }

    void forceSetFirstSampleNumber(FLAC__uint64 sample) {
        mFirstSampleNumber = sample;
    }

protected:
    virtual ~RK_FLACParser();

private:
    FLACDecoder* mFlacDec;
    sp<MetaData> mFileMetadata;
    sp<MetaData> mTrackMetadata;
    bool mInitCheck;

    // media buffers
    size_t mMaxBufferSize;
    MediaBufferGroup *mGroup;
    void (*mCopy)(short *dst, const int *const *src, unsigned nSamples);

    // handle to underlying libFLAC parser
    FLAC__StreamDecoder *mDecoder;

    // current position within the data source
    off_t mCurrentPos;
    bool mEOF;

    // cached when the STREAMINFO metadata is parsed by libFLAC
    FLAC__StreamMetadata_StreamInfo mStreamInfo;
    bool mStreamInfoValid;

    // cached when a decoded PCM block is "written" by libFLAC parser
    bool mWriteRequested;
    bool mWriteCompleted;
    FLAC__FrameHeader mWriteHeader;
    const FLAC__int32 * const *mWriteBuffer;
    FLAC__uint64 mFirstSampleNumber;

    // most recent error reported by libFLAC parser
    FLAC__StreamDecoderErrorStatus mErrorStatus;

    status_t init();
    MediaBuffer *readBuffer(bool doSeek, FLAC__uint64 sample);

    // no copy constructor or assignment
    RK_FLACParser(const RK_FLACParser &);
    RK_FLACParser &operator=(const RK_FLACParser &);

    // FLAC parser callbacks as C++ instance methods
    FLAC__StreamDecoderReadStatus readCallback(
            FLAC__byte buffer[], size_t *bytes);
    FLAC__StreamDecoderSeekStatus seekCallback(
            FLAC__uint64 absolute_byte_offset);
    FLAC__StreamDecoderTellStatus tellCallback(
            FLAC__uint64 *absolute_byte_offset);
    FLAC__StreamDecoderLengthStatus lengthCallback(
            FLAC__uint64 *stream_length);
    FLAC__bool eofCallback();
    FLAC__StreamDecoderWriteStatus writeCallback(
            const FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
    void metadataCallback(const FLAC__StreamMetadata *metadata);
    void errorCallback(FLAC__StreamDecoderErrorStatus status);

    // FLAC parser callbacks as C-callable functions
    static FLAC__StreamDecoderReadStatus read_callback(
            const FLAC__StreamDecoder *decoder,
            FLAC__byte buffer[], size_t *bytes,
            void *client_data);
    static FLAC__StreamDecoderSeekStatus seek_callback(
            const FLAC__StreamDecoder *decoder,
            FLAC__uint64 absolute_byte_offset,
            void *client_data);
    static FLAC__StreamDecoderTellStatus tell_callback(
            const FLAC__StreamDecoder *decoder,
            FLAC__uint64 *absolute_byte_offset,
            void *client_data);
    static FLAC__StreamDecoderLengthStatus length_callback(
            const FLAC__StreamDecoder *decoder,
            FLAC__uint64 *stream_length,
            void *client_data);
    static FLAC__bool eof_callback(
            const FLAC__StreamDecoder *decoder,
            void *client_data);
    static FLAC__StreamDecoderWriteStatus write_callback(
            const FLAC__StreamDecoder *decoder,
            const FLAC__Frame *frame, const FLAC__int32 * const buffer[],
            void *client_data);
    static void metadata_callback(
            const FLAC__StreamDecoder *decoder,
            const FLAC__StreamMetadata *metadata,
            void *client_data);
    static void error_callback(
            const FLAC__StreamDecoder *decoder,
            FLAC__StreamDecoderErrorStatus status,
            void *client_data);

};


// The FLAC parser calls our C++ static callbacks using C calling conventions,
// inside FLAC__stream_decoder_process_until_end_of_metadata
// and FLAC__stream_decoder_process_single.
// We immediately then call our corresponding C++ instance methods
// with the same parameter list, but discard redundant information.

FLAC__StreamDecoderReadStatus RK_FLACParser::read_callback(
        const FLAC__StreamDecoder *decoder, FLAC__byte buffer[],
        size_t *bytes, void *client_data)
{
    return ((RK_FLACParser *) client_data)->readCallback(buffer, bytes);
}

FLAC__StreamDecoderSeekStatus RK_FLACParser::seek_callback(
        const FLAC__StreamDecoder *decoder,
        FLAC__uint64 absolute_byte_offset, void *client_data)
{
    return ((RK_FLACParser *) client_data)->seekCallback(absolute_byte_offset);
}

FLAC__StreamDecoderTellStatus RK_FLACParser::tell_callback(
        const FLAC__StreamDecoder *decoder,
        FLAC__uint64 *absolute_byte_offset, void *client_data)
{
    return ((RK_FLACParser *) client_data)->tellCallback(absolute_byte_offset);
}

FLAC__StreamDecoderLengthStatus RK_FLACParser::length_callback(
        const FLAC__StreamDecoder *decoder,
        FLAC__uint64 *stream_length, void *client_data)
{
    return ((RK_FLACParser *) client_data)->lengthCallback(stream_length);
}

FLAC__bool RK_FLACParser::eof_callback(
        const FLAC__StreamDecoder *decoder, void *client_data)
{
    return ((RK_FLACParser *) client_data)->eofCallback();
}

FLAC__StreamDecoderWriteStatus RK_FLACParser::write_callback(
        const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
        const FLAC__int32 * const buffer[], void *client_data)
{
    return ((RK_FLACParser *) client_data)->writeCallback(frame, buffer);
}

void RK_FLACParser::metadata_callback(
        const FLAC__StreamDecoder *decoder,
        const FLAC__StreamMetadata *metadata, void *client_data)
{
    ((RK_FLACParser *) client_data)->metadataCallback(metadata);
}

void RK_FLACParser::error_callback(
        const FLAC__StreamDecoder *decoder,
        FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    ((RK_FLACParser *) client_data)->errorCallback(status);
}

// These are the corresponding callbacks with C++ calling conventions

FLAC__StreamDecoderReadStatus RK_FLACParser::readCallback(
        FLAC__byte buffer[], size_t *bytes)
{
    uint8_t* pInData = NULL;
    size_t requested = *bytes;
    size_t actual = 0;
    size_t remain = 0;
    status_t err = OK;

    if (mFlacDec->mInputBuffer == NULL) {
        err = mFlacDec->mSource->read(&mFlacDec->mInputBuffer, NULL);
    }

    if (err != OK) {
        if (err == ERROR_END_OF_STREAM) {
            mEOF = true;
            return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        }
        else
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    } else {
        actual = mFlacDec->mInputBuffer->range_length();
        pInData = (uint8_t*)(mFlacDec->mInputBuffer->data()) + mFlacDec->mInputBuffer->range_offset();

        if (actual <=requested) {
            remain = 0;
        } else {
            remain = actual - requested;
        }

        actual = actual >requested ? requested : actual;

        *bytes = actual;
        mCurrentPos += actual;

        if (pInData) {
            memcpy(buffer, pInData, actual);
        }

        if (remain == 0) {
            if (mFlacDec->mInputBuffer) {
                mFlacDec->mInputBuffer->release();
                mFlacDec->mInputBuffer = NULL;
            }
        } else {
            size_t curOff = mFlacDec->mInputBuffer->range_offset();
            curOff +=requested;
            mFlacDec->mInputBuffer->set_range(curOff, remain);
        }

        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }

}

FLAC__StreamDecoderSeekStatus RK_FLACParser::seekCallback(
        FLAC__uint64 absolute_byte_offset)
{
    FLAC_LOG("seekCallback in");
    mCurrentPos = absolute_byte_offset;
    mEOF = false;
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus RK_FLACParser::tellCallback(
        FLAC__uint64 *absolute_byte_offset)
{
    FLAC_LOG("tellCallback in");
    *absolute_byte_offset = mCurrentPos;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus RK_FLACParser::lengthCallback(
        FLAC__uint64 *stream_length)
{
    FLAC_LOG("lengthCallback in");
    *stream_length = 0x1FFFFFFFFFFFFFFF;
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool RK_FLACParser::eofCallback()
{
    return mEOF;
}

FLAC__StreamDecoderWriteStatus RK_FLACParser::writeCallback(
        const FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
    if (mWriteRequested) {
        mWriteRequested = false;
        // FLAC parser doesn't free or realloc buffer until next frame or finish
        mWriteHeader = frame->header;
        mWriteBuffer = buffer;
        mWriteCompleted = true;
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    } else {
        ALOGE("RK_FLACParser::writeCallback unexpected");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
}

void RK_FLACParser::metadataCallback(const FLAC__StreamMetadata *metadata)
{
    FLAC_LOG("metadataCallback in");
    switch (metadata->type) {
    case FLAC__METADATA_TYPE_STREAMINFO:
        if (!mStreamInfoValid) {
            mStreamInfo = metadata->data.stream_info;
            mStreamInfoValid = true;
        } else {
            ALOGE("RK_FLACParser::metadataCallback unexpected STREAMINFO");
        }
        break;
    case FLAC__METADATA_TYPE_VORBIS_COMMENT:
        {
        const FLAC__StreamMetadata_VorbisComment *vc;
        vc = &metadata->data.vorbis_comment;
        for (FLAC__uint32 i = 0; i < vc->num_comments; ++i) {
            FLAC__StreamMetadata_VorbisComment_Entry *vce;
            vce = &vc->comments[i];
            if (mFileMetadata != 0) {
                ;   //do nothing here
            }
        }
        }
        break;
    case FLAC__METADATA_TYPE_PICTURE:
        if (mFileMetadata != 0) {
            const FLAC__StreamMetadata_Picture *p = &metadata->data.picture;
            mFileMetadata->setData(kKeyAlbumArt,
                    MetaData::TYPE_NONE, p->data, p->data_length);
            mFileMetadata->setCString(kKeyAlbumArtMIME, p->mime_type);
        }
        break;
    default:
        ALOGW("RK_FLACParser::metadataCallback unexpected type %u", metadata->type);
        break;
    }
}

void RK_FLACParser::errorCallback(FLAC__StreamDecoderErrorStatus status)
{
    FLAC_LOG("RK_FLACParser::errorCallback status=%d", status);
    mErrorStatus = status;
}

// Copy samples from FLAC native 32-bit non-interleaved to 16-bit interleaved.
// These are candidates for optimization if needed.

static void copyMono8(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i] << 8;
    }
}

static void copyStereo8(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i] << 8;
        *dst++ = src[1][i] << 8;
    }
}

static void copyMultiChn8(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i] << 8;
        *dst++ = src[1][i] << 8;
        *dst++ = src[2][i] << 8;
        *dst++ = src[3][i] << 8;
        *dst++ = src[4][i] << 8;
        *dst++ = src[5][i] << 8;
    }
}


static void copyMono16(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i];
    }
}

static void copyStereo16(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i];
        *dst++ = src[1][i];
    }
}

static void copyMultiChn16(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i];
        *dst++ = src[1][i];
        *dst++ = src[2][i];
        *dst++ = src[3][i];
        *dst++ = src[4][i];
        *dst++ = src[5][i];
    }
}


// 24-bit versions should do dithering or noise-shaping, here or in AudioFlinger

static void copyMono24(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i] >> 8;
    }
}

static void copyStereo24(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i] >> 8;
        *dst++ = src[1][i] >> 8;
    }
}

static void copyMultiChn24(short *dst, const int *const *src, unsigned nSamples)
{
    for (unsigned i = 0; i < nSamples; ++i) {
        *dst++ = src[0][i] >> 8;
        *dst++ = src[1][i] >> 8;
        *dst++ = src[2][i] >> 8;
        *dst++ = src[3][i] >> 8;
        *dst++ = src[4][i] >> 8;
        *dst++ = src[5][i] >> 8;
    }
}


static void copyTrespass(short *dst, const int *const *src, unsigned nSamples)
{
    TRESPASS();
}

// RK_FLACParser

RK_FLACParser::RK_FLACParser(
        FLACDecoder* pFlacdec,
        const sp<MetaData> &fileMetadata,
        const sp<MetaData> &trackMetadata)
    : mFlacDec(pFlacdec),
      mFileMetadata(fileMetadata),
      mTrackMetadata(trackMetadata),
      mInitCheck(false),
      mMaxBufferSize(0),
      mGroup(NULL),
      mCopy(copyTrespass),
      mDecoder(NULL),
      mCurrentPos(0LL),
      mEOF(false),
      mStreamInfoValid(false),
      mWriteRequested(false),
      mWriteCompleted(false),
      mWriteBuffer(NULL),
      mFirstSampleNumber(-1),
      mErrorStatus((FLAC__StreamDecoderErrorStatus) -1)
{
    FLAC_LOG("RK_FLACParser construct in");
    memset(&mStreamInfo, 0, sizeof(mStreamInfo));
    memset(&mWriteHeader, 0, sizeof(mWriteHeader));
    mInitCheck = init();

    if (mInitCheck == OK) {
        FLAC_LOG("RK_FLACParser init OK");
        allocateBuffers();
    } else {
        FLAC_LOG("RK_FLACParser init fail");
    }
}

RK_FLACParser::~RK_FLACParser()
{
    ALOGV("RK_FLACParser::~RK_FLACParser");
    if (mDecoder != NULL) {
        FLAC__stream_decoder_delete(mDecoder);
        mDecoder = NULL;
    }

    releaseBuffers();
}

status_t RK_FLACParser::init()
{
    FLAC_LOG("RK_FLACParser::init in");
    // setup libFLAC parser
    mDecoder = FLAC__stream_decoder_new();
    if (mDecoder == NULL) {
        // The new should succeed, since probably all it does is a malloc
        // that always succeeds in Android.  But to avoid dependence on the
        // libFLAC internals, we check and log here.
        ALOGE("new failed");
        return NO_INIT;
    }

    FLAC__stream_decoder_set_md5_checking(mDecoder, false);
    FLAC__stream_decoder_set_metadata_ignore_all(mDecoder);
    FLAC__stream_decoder_set_metadata_respond(
            mDecoder, FLAC__METADATA_TYPE_STREAMINFO);
    FLAC__stream_decoder_set_metadata_respond(
            mDecoder, FLAC__METADATA_TYPE_PICTURE);
    FLAC__stream_decoder_set_metadata_respond(
            mDecoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
    FLAC__StreamDecoderInitStatus initStatus;

    initStatus = FLAC__stream_decoder_init_stream(
            mDecoder,
            read_callback, seek_callback, tell_callback,
            length_callback, eof_callback, write_callback,
            metadata_callback, error_callback, (void *) this);

    if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        // A failure here probably indicates a programming error and so is
        // unlikely to happen. But we check and log here similarly to above.
        ALOGE("init_stream failed %d", initStatus);
        return NO_INIT;
    }

    /*
     ** we do not need to parse metadata here. so set mStreamInfoValid to true.
    */
    mStreamInfoValid = true;
#if 0
    // parse all metadata
    if (!FLAC__stream_decoder_process_until_end_of_metadata(mDecoder)) {
        ALOGE("end_of_metadata failed");
        return NO_INIT;
    }
#endif

    if (mStreamInfoValid) {
        // check channel count
        switch (getChannels()) {
        case 1:
        case 2:
        case 6:
            break;
        default:
            ALOGE("unsupported channel count %u", getChannels());
            return NO_INIT;
        }
        // check bit depth
        switch (getBitsPerSample()) {
        case 8:
        case 16:
        case 24:
            break;
        default:
            ALOGE("unsupported bits per sample %u", getBitsPerSample());
            return NO_INIT;
        }
        // check sample rate
        switch (getSampleRate()) {
        case  8000:
        case 11025:
        case 12000:
        case 16000:
        case 22050:
        case 24000:
        case 32000:
        case 44100:
        case 48000:
            break;
        default:
            // 96000 would require a proper downsampler in AudioFlinger
            ALOGE("unsupported sample rate %u", getSampleRate());
            return NO_INIT;
        }
        // configure the appropriate copy function, defaulting to trespass
        static const struct {
            unsigned mChannels;
            unsigned mBitsPerSample;
            void (*mCopy)(short *dst, const int *const *src, unsigned nSamples);
        } table[] = {
            { 1,  8, copyMono8    },
            { 2,  8, copyStereo8  },
            { 6,  8, copyMultiChn8},
            { 1, 16, copyMono16   },
            { 2, 16, copyStereo16 },
            { 6, 16, copyMultiChn16},
            { 1, 24, copyMono24   },
            { 2, 24, copyStereo24 },
            { 6, 24, copyMultiChn24 },
        };
        for (unsigned i = 0; i < sizeof(table)/sizeof(table[0]); ++i) {
            if (table[i].mChannels == mFlacDec->mNumChannels &&
                    table[i].mBitsPerSample == mFlacDec->mBitDepth) {
                mCopy = table[i].mCopy;
                break;
            }
        }
        // populate track metadata
        if (mTrackMetadata != 0) {
            mTrackMetadata->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
            mTrackMetadata->setInt32(kKeyChannelCount, getChannels());
            mTrackMetadata->setInt32(kKeySampleRate, getSampleRate());
            // sample rate is non-zero, so division by zero not possible
            mTrackMetadata->setInt64(kKeyDuration,
                    (getTotalSamples() * 1000000LL) / getSampleRate());
        }
    } else {
        ALOGE("missing STREAMINFO");
        return NO_INIT;
    }
    if (mFileMetadata != 0) {
        mFileMetadata->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_FLAC);
    }
    return OK;
}

void RK_FLACParser::allocateBuffers()
{
    FLAC_LOG("allocateBuffers in");
    CHECK(mGroup == NULL);
    mGroup = new MediaBufferGroup;
    mMaxBufferSize = getMaxBlockSize() * getChannels() * sizeof(short);
    mGroup->add_buffer(new MediaBuffer(mMaxBufferSize));
}

void RK_FLACParser::releaseBuffers()
{
    CHECK(mGroup != NULL);
    delete mGroup;
    mGroup = NULL;
}

MediaBuffer *RK_FLACParser::readBuffer(bool doSeek, FLAC__uint64 sample)
{
    mWriteRequested = true;
    mWriteCompleted = false;
    if (doSeek) {
        // We implement the seek callback, so this works without explicit flush
        if (!FLAC__stream_decoder_seek_absolute(mDecoder, sample)) {
            ALOGE("RK_FLACParser::readBuffer seek to sample %llu failed", sample);
            return NULL;
        }
        ALOGV("RK_FLACParser::readBuffer seek to sample %llu succeeded", sample);
    } else {
        if (!FLAC__stream_decoder_process_single(mDecoder)) {
            ALOGE("RK_FLACParser::readBuffer process_single failed");
            return NULL;
        }
    }
    if (!mWriteCompleted) {
        ALOGV("RK_FLACParser::readBuffer write did not complete");
        return NULL;
    }
    // verify that block header keeps the promises made by STREAMINFO
    unsigned blocksize = mWriteHeader.blocksize;
    if (blocksize == 0) {
        ALOGE("RK_FLACParser::readBuffer write invalid blocksize %u", blocksize);
        return NULL;
    }
    if (mWriteHeader.sample_rate != getSampleRate() ||
        mWriteHeader.channels != getChannels() ||
        mWriteHeader.bits_per_sample != getBitsPerSample()) {
        ALOGE("RK_FLACParser::readBuffer write changed parameters mid-stream");
    }

    size_t bufferSize = blocksize * getChannels() * sizeof(short);
    if (bufferSize >mMaxBufferSize) {
        if (mGroup) {
            delete mGroup;
            mGroup = NULL;
        }
        mFlacDec->mMaxBlockSize = blocksize;
        mGroup = new MediaBufferGroup;
        mMaxBufferSize = getMaxBlockSize() * getChannels() * sizeof(short);
        mGroup->add_buffer(new MediaBuffer(mMaxBufferSize));
    }
    // acquire a media buffer
    CHECK(mGroup != NULL);
    MediaBuffer *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return NULL;
    }

    short *data = (short *) buffer->data();
    buffer->set_range(0, bufferSize);
    // copy PCM from FLAC write buffer to our media buffer, with interleaving
    (*mCopy)(data, mWriteBuffer, blocksize);
    // fill in buffer metadata
    CHECK(mWriteHeader.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);
    FLAC__uint64 sampleNumber = mWriteHeader.number.sample_number;
    if (-1 == mFirstSampleNumber) {
        mFirstSampleNumber = sampleNumber;
    }

    if ((sampleNumber >=mFirstSampleNumber) && (mFirstSampleNumber >0)) {
        sampleNumber -=mFirstSampleNumber;
    }

    int64_t timeUs = (1000000LL * sampleNumber) / getSampleRate();
    buffer->meta_data()->setInt64(kKeyTime, timeUs);
    buffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);
    FLAC_LOG("out flac frame, sample num: %lld, ts: %lld", sampleNumber, timeUs);
    return buffer;
}

FLACDecoder::FLACDecoder(const sp<MediaSource> &source)
    : mSource(source),
      mNumChannels(0),
      mSampleRate(0),
      mBitDepth(0),
      mMaxBlockSize(0),
      mStarted(false),
      mInitFlag(0),
      mAnchorTimeUs(0),
      mNumFramesOutput(0),
      mInputBuffer(NULL),
      mFirstOutputBuf(NULL),
      mReAsynTime(0LL),
      mReAsynThreshHold(0LL),
      mRkFlacParser(NULL) {
    init();
}

void FLACDecoder::init() {
    sp<MetaData> srcFormat = mSource->getFormat();

    CHECK(srcFormat->findInt32(kKeyChannelCount, &mNumChannels));
    CHECK(srcFormat->findInt32(kKeySampleRate, &mSampleRate));
    CHECK(srcFormat->findInt32(kKeyBitDepth, &mBitDepth));

    mMeta = new MetaData;
    mMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
    mMeta->setInt32(kKeyChannelCount, mNumChannels);
    mMeta->setInt32(kKeySampleRate, mSampleRate);

    mMaxBlockSize = OUTPUT_BUFFER_SIZE_FLAC;

    int64_t durationUs;
    if (srcFormat->findInt64(kKeyDuration, &durationUs)) {
        mMeta->setInt64(kKeyDuration, durationUs);
    }

    mMeta->setCString(kKeyDecoderComponent, "FLACDecoder");
    FLAC_LOG("init, channel: %d, sampleRate: %d, bitDepth: %d, durationUs: %lld",
        mNumChannels, mSampleRate, mBitDepth, durationUs);
}

FLACDecoder::~FLACDecoder() {
    if (mStarted) {
        stop();
    }
}

status_t FLACDecoder::start(MetaData *params) {
    CHECK(!mStarted);

	mSource->start();
    mAnchorTimeUs = 0;
    mNumFramesOutput = 0;
    mStarted = true;

    /*
     ** check bit depths for we do not support 32 bit depths now.
    */
    if (mBitDepth >=32) {
        status_t err = mSource->read(&mInputBuffer, NULL);
        if (err == OK) {
            uint8_t *pBuf = (uint8_t*)(mInputBuffer->data());
            pBuf +=mInputBuffer->range_offset();
            checkFlacFrameHeader(pBuf, mInputBuffer->range_length());
        }
    }

    if (mRkFlacParser == NULL) {
        mRkFlacParser = new RK_FLACParser(this);
        CHECK(mRkFlacParser != NULL);
    }

    return OK;
}

status_t FLACDecoder::stop() {
    CHECK(mStarted);

    if (mInputBuffer) {
        mInputBuffer->release();
        mInputBuffer = NULL;
    }

    mSource->stop();

    mStarted = false;

    return OK;
}

sp<MetaData> FLACDecoder::getFormat() {
    return mMeta;
}

int32_t FLACDecoder::checkFlacFrameHeader(uint8_t* buf, uint32_t size)
{
    if ((buf == NULL) || (size ==0)) {
        return -1;
    }

    uint8_t *pBuf = buf;
    uint32_t tmp = 0;
    uint32_t bs_code, sr_code, ch_mode, bps_code, bps;
    uint32_t sync_code = (*pBuf++);

    sync_code = ((sync_code <<8) | (*pBuf++));

    FLAC_LOG("sync_code: 0x%X, ((sync_code >>1) & 0x7FFF): 0x%X",
            sync_code, ((sync_code >>1) & 0x7FFF));

    if (((sync_code >>1) & 0x7FFF) != 0x7FFC) {
        FLAC_LOG("invalid flac sync code");
        return -1;
    }

    /* variable block size stream code */
    int32_t is_var_size = (sync_code & 1);

    /* block size and sample rate codes */
    tmp = (*pBuf++);
    bs_code = ((tmp >>4) & 0xF);
    sr_code = ((tmp) & 0xF);

    /* channels and decorrelation */
    tmp = (*pBuf++);
    ch_mode = ((tmp >>4) & 0xF);

    FLAC_LOG("is_var_size: %d, bs_code: %d, sr_code: %d, ch_mode: %d",
            is_var_size, bs_code, sr_code, ch_mode);

    /* bits per sample */
    bps_code = ((tmp & 0xE) >>1);
    if (bps_code == 3 || bps_code == 7) {
        FLAC_LOG("invalid sample size code (%d)",bps_code);
        return -1;
    }

    bps = sample_size_table[bps_code];
    mBitDepth = bps;
    return 0;
}

status_t FLACDecoder::read(
        MediaBuffer **out, const ReadOptions *options) {
    status_t err;

    *out = NULL;
	if(mFirstOutputBuf)
	{
		*out = mFirstOutputBuf;
		mFirstOutputBuf = NULL;
		return OK;
	}
    int64_t seekTimeUs = 0;
    ReadOptions::SeekMode mode;
    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        CHECK(seekTimeUs >= 0);
        FLAC_LOG("flac seek to %lld us", seekTimeUs);
		//when the user seek the video,then will entry into this case,so you must do some reset fun in this case
		//by Charles Chen at Feb,11th ,2011
        mNumFramesOutput = 0;

        /* if we have read one packet while do frame header check, release it at first before do seek */
        if (mInputBuffer) {
            mInputBuffer->release();
            mInputBuffer = NULL;
        }
        /* call extractor to do seek first */
        err = mSource->read(&mInputBuffer, options);
		mAnchorTimeUs = -1LL;//-1 mean after seek ,need Assignment first frame time after seek

    } else {
        seekTimeUs = -1;
    }

    int64_t tmpTimeUs = 0;
    if (mAnchorTimeUs == -1) {
        if (mInputBuffer->meta_data()->findInt64(kKeyTime, &tmpTimeUs)) {
            mAnchorTimeUs = tmpTimeUs;
            mNumFramesOutput = 0;
            if (mRkFlacParser) {
                mRkFlacParser->forceSetFirstSampleNumber(-1);
            }
        }
    }

    /* call RK_FLACParser to decode one frame*/
    MediaBuffer *buffer = NULL;
    if (mRkFlacParser) {
        buffer = mRkFlacParser->readBuffer();
    }

    if (buffer != NULL) {
        mNumFramesOutput += (buffer->range_length()) /mNumChannels;
        if (buffer->meta_data()->findInt64(kKeyTime, &tmpTimeUs)) {
            buffer->meta_data()->setInt64(kKeyTime, tmpTimeUs + mAnchorTimeUs);
        }
    }

    *out = buffer;
    return buffer != NULL ? (status_t) OK : (status_t) ERROR_END_OF_STREAM;
}

}  // namespace android
