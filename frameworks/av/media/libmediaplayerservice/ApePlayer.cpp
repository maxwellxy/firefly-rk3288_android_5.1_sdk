/*
 ** Copyright 2007, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ApePlayer"
#include "utils/Log.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "ApePlayer.h"

//#define TEST
#define PATH  "/data/pcm.pcm"

#ifdef HAVE_GETTID
static pid_t myTid() { return gettid(); }
#else
static pid_t myTid() { return getpid(); }
#endif

// ----------------------------------------------------------------------------

namespace android {

    // ----------------------------------------------------------------------------

    // TODO: Determine appropriate return codes
    static status_t ERROR_NOT_OPEN = -1;
    static status_t ERROR_OPEN_FAILED = -2;
    static status_t ERROR_ALLOCATE_FAILED = -4;
    static status_t ERROR_NOT_SUPPORTED = -8;
    static status_t ERROR_NOT_READY = -16;
    static status_t STATE_INIT = 0;
    static status_t STATE_ERROR = 1;
    static status_t STATE_OPEN = 2;


    ApePlayer::ApePlayer() :
        mAudioBuffer(NULL), mPlayTime(-1), mDuration(-1), mState(STATE_ERROR),
        mStreamType(AUDIO_STREAM_MUSIC), mLoop(false),mExit(false), mPaused(false), mRender(false), mRenderTid(-1),mApeDec(NULL),mLastTime(-1)
    {
        ALOGV("constructor\n");
        Mutex::Autolock l(mMutex);
        createThreadEtc(renderThread, this, "ape decoder", ANDROID_PRIORITY_AUDIO);
        mCondition.wait(mMutex);
        if (mRenderTid > 0) {
            ALOGE("render thread(%d) started", mRenderTid);
            mState = STATE_INIT;
        }
    }


    status_t ApePlayer::initCheck()
    {
        if (mState != STATE_ERROR) return NO_ERROR;
        return ERROR_NOT_READY;
    }

    ApePlayer::~ApePlayer() {
        ALOGV("ApePlayer destructor\n");
        if(mApeDec)
        {
            delete mApeDec;
            mApeDec = NULL;
        }


        release();
    }

    status_t ApePlayer::setDataSource(
            const sp<IMediaHTTPService> &httpService,
            const char *uri, const KeyedVector<String8, String8> *headers) {
        return setdatasource(uri, -1, 0, 0x7ffffffffffffffLL); // intentionally less than LONG_MAX
    }

    status_t ApePlayer::setDataSource(int fd, int64_t offset, int64_t length)
    {
        return setdatasource(NULL, fd, offset, length);
    }


    char *getpath( long pid, int fd)
    {
        char path[1024];
        char tmp[1024];
        char *res=NULL;

        memset(path,0,sizeof(path));
        memset(tmp,0,sizeof(tmp));  
        strcpy(path,"/proc/");
        sprintf(tmp,"%ld/fd/%d",pid,fd);
        strcat(path,tmp);
        res=(char *)calloc(1,1024);
        if(!res)
            return NULL;
        readlink(path,res,1024);
        return res;

    }

    status_t ApePlayer::setdatasource(const char *path, int fd, int64_t offset, int64_t length)
    {
        ALOGV("setDataSource url=%s, fd=%d, offset=%d, length=%d\n", path, fd, (int)offset, (int)length);

        // file still open?
        Mutex::Autolock l(mMutex);
        if (mState == STATE_OPEN) {
            reset_nosync();
        }

        struct stat sb;
        int ret;
        if (path) {
            ret = stat(path, &sb);
        } else {
            ret = fstat(fd, &sb);
        }
        if (ret != 0) {
            mState = STATE_ERROR;
            return ERROR_OPEN_FAILED;
        }
        if (sb.st_size > (length + offset)) {
            mLength = length;
        } else {
            mLength = sb.st_size - offset;
        }


        mOffset = offset;
        // fseek(mFile, offset, SEEK_SET);
        int nRetVal;
        char *truepath;

        if(path == NULL)
        {
            // ALOGI("setDataSource url=%s, fd=%d, offset=%d, length=%d\n", path, fd, (int)offset, (int)length);
            mApeDec = CreateIAPEDecompressFd(dup(fd),offset,length, &nRetVal,1);
            //ALOGI("setDataSource url=%s, fd=%d, offset=%d, length=%d nRetVal= %d\n", path, fd, (int)offset, (int)length,nRetVal);

        }
        else 
        {
            mApeDec = CreateIAPEDecompress((char *)path, &nRetVal,1);
        }

        if(mApeDec == NULL || nRetVal!=0)
        {
            ALOGE("ApeDec class create failed,nRetVal=%d\n",nRetVal);
            mState = STATE_ERROR;

            if(mApeDec)
            {
                ALOGE("APE_DEC create failed");
                delete mApeDec;
                mApeDec = NULL;
            }
            return ERROR_OPEN_FAILED;
        }


        mState = STATE_OPEN;
        return NO_ERROR;
    }

    status_t ApePlayer::prepare()
    {
        ALOGV("prepare\n");
        if (mState != STATE_OPEN ) {

            return ERROR_NOT_OPEN;
        }

        return NO_ERROR;
    }

    status_t ApePlayer::prepareAsync() {
        ALOGV("prepareAsync\n");
        // can't hold the lock here because of the callback
        // it's safe because we don't change state
        if (mState != STATE_OPEN ) {
            sendEvent(MEDIA_ERROR);

            return NO_ERROR;
        }

        sendEvent(MEDIA_PREPARED);
        return NO_ERROR;
    }

    status_t ApePlayer::start()
    {
        ALOGV("start\n");
        Mutex::Autolock l(mMutex);
        if (mState != STATE_OPEN) {

            return ERROR_NOT_OPEN;
        }


        mPaused = false;
        mRender = true;

        // wake up render thread
        mCondition.signal();
        return NO_ERROR;
    }

    status_t ApePlayer::stop()
    {
        ALOGV("stop\n");
        Mutex::Autolock l(mMutex);
        if (mState != STATE_OPEN) {
            return ERROR_NOT_OPEN;
        }
        mPaused = true;
        mRender = false;
        return NO_ERROR;
    }
    static int NowTime()
    {
        struct timeval tv;

        gettimeofday(&tv, NULL); 

        return tv.tv_sec*1000L+tv.tv_usec/1000L;
    }

    status_t ApePlayer::seekTo(int position)
    {

        Mutex::Autolock l(mMutex);
        if (mState != STATE_OPEN) {
            return ERROR_NOT_OPEN;
        }
        //modifide by HeLun ,avoid the overflows
        int lp = position;
        int ls = mApeDec->GetInfo(APE_INFO_SAMPLE_RATE);
        int  block = lp/1000*ls;


        if(mLastTime == -1)
        {
            mLastTime = position;
        }
        else 
        {
            if( position - mLastTime < 1000 && position -mLastTime > -1000
                    && !(mLastTime == 0 && position == 0))
            {
                sendEvent(MEDIA_SEEK_COMPLETE);
                ALOGI("ape seek return here the time %d",position - mLastTime );
                return NO_ERROR;
            }
            else 
            {
                mLastTime = position;
            }
        }


        ALOGI("seek position is %d ,block is %d\n",position,(int)(block));
        int nowtime = NowTime();
        int result = mApeDec->Seek((int)(block));
        ALOGI("--->seek cost time %d ms",NowTime()-nowtime);
        if (result != 0) {
            ALOGI("ov_time_seek() returned %d\n", result);
            sendEvent(MEDIA_SEEK_COMPLETE);
            return result;
        }
        sendEvent(MEDIA_SEEK_COMPLETE);
        return NO_ERROR;
    }


    status_t ApePlayer::pause()
    {
        ALOGV("pause\n");
        Mutex::Autolock l(mMutex);
        if (mState != STATE_OPEN) {
            return ERROR_NOT_OPEN;
        }
        mPaused = true;
        return NO_ERROR;
    }

    bool ApePlayer::isPlaying()
    {
        ALOGV("isPlaying\n");

        if(mPaused)
            return false;

        if (mState == STATE_OPEN) {
            return mRender;
        }
        return false;
    }

    status_t ApePlayer::getCurrentPosition(int* position)
    {
        ALOGV("getCurrentPosition\n");

        Mutex::Autolock l(mMutex);
        if (mState != STATE_OPEN) {
            ALOGE("getCurrentPosition(): file not open");
            return ERROR_NOT_OPEN;
        }

        *position = mApeDec->GetInfo(APE_DECOMPRESS_CURRENT_MS);
        if (*position < 0) {
            ALOGE("getCurrentPosition(): ov_time_tell returned %d", *position);
            return *position;
        }


        return NO_ERROR;
    }

    status_t ApePlayer::getDuration(int* duration)
    {
        ALOGV("getDuration\n");

        Mutex::Autolock l(mMutex);
        if (mState != STATE_OPEN) {
            return ERROR_NOT_OPEN;
        }

        *duration=mApeDec->GetInfo(APE_DECOMPRESS_LENGTH_MS);
        //*duration=1000*3;
        return NO_ERROR;
    }

    status_t ApePlayer::release()
    {
        ALOGV("release\n");
        Mutex::Autolock l(mMutex);
        reset_nosync();

        // TODO: timeout when thread won't exit
        // wait for render thread to exit
        if (mRenderTid > 0) {
            mExit = true;
            mCondition.signal();
            mCondition.wait(mMutex);
        }
        return NO_ERROR;
    }

    status_t ApePlayer::reset()
    {
        ALOGV("reset\n");
        Mutex::Autolock l(mMutex);
        if (mState != STATE_OPEN) {
            return NO_ERROR;
        }
        return reset_nosync();
    }

    // always call with lock held
    status_t ApePlayer::reset_nosync()
    {
        // close file
        mState = STATE_ERROR;

        mPlayTime = -1;
        mDuration = -1;
        mLoop = false;
        mPaused = false;
        mRender = false;
        return NO_ERROR;
    }

    status_t ApePlayer::setLooping(int loop)
    {
        ALOGV("setLooping\n");
        Mutex::Autolock l(mMutex);
        mLoop = (loop != 0);
        return NO_ERROR;
    }

    status_t ApePlayer::createOutputTrack() {
        // open audio track
        int rate = 44100;
        int channels = 2;

        rate= mApeDec->GetInfo(APE_INFO_SAMPLE_RATE);
        channels=  mApeDec->GetInfo(APE_INFO_CHANNELS);

        if (mAudioSink->open(rate, channels,CHANNEL_MASK_USE_CHANNEL_ORDER, AUDIO_FORMAT_PCM_16_BIT, DEFAULT_AUDIOSINK_BUFFERCOUNT) != NO_ERROR) {
            ALOGE("mAudioSink open failed");
            return ERROR_OPEN_FAILED;
        }
        return NO_ERROR;
    }

    int ApePlayer::renderThread(void* p) {
        return ((ApePlayer*)p)->render();
    }

#define AUDIOBUFFER_SIZE (1152*4)

    int ApePlayer::render() {
        int result = -1;
        int temp;
        int current_section = 0;
        bool audioStarted = false;
        mblocks=-1;




        //int   blocksperframe=mApeDec->GetInfo(APE_INFO_BLOCKS_PER_FRAME);
        int trueblock;
        ALOGV("render\n");
        // allocate render buffer
        mAudioBuffer = new char[1024*256];
        if (!mAudioBuffer) {
            ALOGE("mAudioBuffer allocate failed\n");
            goto threadExit;
        }

        // let main thread know we're ready
        {
            Mutex::Autolock l(mMutex);
            mRenderTid = myTid();
            mCondition.signal();
        }

        while (1) 
        {
            int numread = 0;
            {
                Mutex::Autolock l(mMutex);

                // pausing?
                if (mPaused) {
                    if (mAudioSink->ready()) mAudioSink->pause();
                    mRender = false;
                    audioStarted = false;
                }

                // nothing to render, wait for client thread to wake us up
                if (!mExit && !mRender) {
                    ALOGE("render  %d  signal wait\n",myTid());
                    mCondition.wait(mMutex);
                    ALOGE("render %d signal rx'd\n",myTid());
                    if(!mApeDec)
                    {
                        ALOGI("mApeDec == NULL at loop");
                        goto threadExit;    
                    }
                }

                if (mExit) break;

                // We could end up here if start() is called, and before we get a
                // chance to run, the app calls stop() or reset(). Re-check render
                // flag so we don't try to render in stop or reset state.
                if (!mRender) continue;
                //modified by HeLun , 500ms's  blocks 
                if(mblocks == -1)
                {

                    switch (mApeDec->GetInfo(APE_INFO_COMPRESSION_LEVEL))
                    {
                        case COMPRESSION_LEVEL_FAST: 
                        case COMPRESSION_LEVEL_NORMAL: 
                        case COMPRESSION_LEVEL_HIGH:
                            mblocks = (int)(200 * mApeDec->GetInfo(APE_INFO_SAMPLE_RATE)/1000.0);                               
                            break;

                        case COMPRESSION_LEVEL_EXTRA_HIGH:
                            mblocks = (int)(100 * mApeDec->GetInfo(APE_INFO_SAMPLE_RATE)/1000.0);

                    }
                }

                mApeDec->GetData(mAudioBuffer,mblocks, &trueblock);
                numread = trueblock * mApeDec->GetInfo(APE_INFO_BLOCK_ALIGN);

                if (numread == 0) {
                    // end of file, do we need to loop?
                    // ...
                    if (mLoop) {
                        mApeDec->Seek(0);
                    } else {
                        mAudioSink->stop();
                        audioStarted = false;
                        mRender = false;
                        mPaused = true;
                        //int endpos = ov_time_tell(&mVorbisFile);

                        ALOGI("send MEDIA_PLAYBACK_COMPLETE");
                        sendEvent(MEDIA_PLAYBACK_COMPLETE);

                        // wait until we're started again
                        ALOGI("playback complete - wait for signal");
                        mCondition.wait(mMutex);
                        ALOGI("playback complete - signal rx'd");
                        if (mExit) break;

                    }
                }
            }

            // codec returns negative number on error
            if (numread < 0) {
                ALOGE("Error in Vorbis decoder");
                sendEvent(MEDIA_ERROR);
                break;
            }

            // create audio output track if necessary
            if (!mAudioSink->ready()) {
                ALOGE("render - create output track\n");
                if (createOutputTrack() != NO_ERROR)
                    break;
            }

            // Write data to the audio hardware
#ifdef TEST
            FILE *fp=NULL;
            int res;
            fp=fopen(PATH,"ab+");
            if(!fp)
            {
                ALOGE("helun_open file error \n");
                return -1 ;
            }
            res=fwrite(mAudioBuffer,1,numread,fp);
            if(res!=numread)
            {
                ALOGE("helun_write errorr \n");

            }
            fclose(fp);

#endif 



            if ((temp = mAudioSink->write(mAudioBuffer, numread)) < 0) {
                ALOGE("Error in writing:%d",temp);
                result = temp;
                break;
            }

            // start audio output if necessary
            if (!audioStarted && !mPaused && !mExit) {
                ALOGE("render - starting audio\n");
                mAudioSink->start();
                audioStarted = true;
            }
        }

threadExit:
        mAudioSink.clear();
        if (mAudioBuffer) {
            delete [] mAudioBuffer;
            mAudioBuffer = NULL;
        }

        // tell main thread goodbye
        Mutex::Autolock l(mMutex);
        mRenderTid = -1;
        mCondition.signal();
        return result;
    }

} // end namespace android
