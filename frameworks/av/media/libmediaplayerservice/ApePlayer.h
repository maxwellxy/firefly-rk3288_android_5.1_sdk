/*
 **
 ** Copyright 2008, The Android Open Source Project
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

#ifndef ANDROID_APEPLAYER_H
#define ANDROID_APEPLAYER_H

#include <utils/threads.h>

#include <media/MediaPlayerInterface.h>
#include <media/AudioTrack.h>



#include "APE.h"


#define ANDROID_LOOP_TAG "ANDROID_LOOP"

namespace android {

    class ApePlayer : public MediaPlayerInterface {
        public:
            ApePlayer();
            ~ApePlayer();

            virtual status_t    initCheck();
            virtual status_t    setDataSource(
                    const sp<IMediaHTTPService> &httpService,
                    const char* path, const KeyedVector<String8, String8> *headers);
            virtual status_t    setDataSource(int fd, int64_t offset, int64_t length);
            virtual status_t    setVideoSurfaceTexture(
                    const sp<IGraphicBufferProducer>& bufferProducer)
            { return UNKNOWN_ERROR; }
            virtual status_t    prepare();
            virtual status_t    prepareAsync();
            virtual status_t    start();
            virtual status_t    stop();
            virtual status_t    seekTo(int msec);
            virtual status_t    pause();
            virtual bool        isPlaying();
            virtual status_t    getCurrentPosition(int* msec);
            virtual status_t    getDuration(int* msec);
            virtual status_t    release();
            virtual status_t    reset();
            virtual status_t    setLooping(int loop);
            virtual player_type playerType() { return APE_PLAYER; }
            virtual status_t    invoke(const Parcel& request, Parcel *reply) {
                return INVALID_OPERATION;
            }
            virtual status_t    setParameter(int key, const Parcel &request) {
                return INVALID_OPERATION;
            }
            virtual status_t    getParameter(int key, Parcel *reply) {
                return INVALID_OPERATION;
            }

        private:
            status_t    setdatasource(const char *path, int fd, int64_t offset, int64_t length);
            status_t    reset_nosync();
            status_t    createOutputTrack();
            static  int         renderThread(void*);
            int         render();


            Mutex               mMutex;
            Condition           mCondition;
            int64_t             mOffset;
            int64_t             mLength;
            char*               mAudioBuffer;
            int                 mPlayTime;
            int                 mDuration;
            status_t            mState;
            int                 mStreamType;
            bool                mLoop;
            volatile bool       mExit;
            bool                mPaused;
            volatile bool       mRender;
            pid_t               mRenderTid;
            //modified by HeLun
            IAPEDecompress *mApeDec;
            int mblocks;
            int mLastTime;

    };

}; // namespace android

#endif // ANDROID_APEPLAYER_H

