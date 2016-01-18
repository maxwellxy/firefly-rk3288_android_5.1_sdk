#ifndef RK_METADATA_RETRIEVER_H
#define RK_METADATA_RETRIEVER_H

#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

#include <media/MediaMetadataRetrieverInterface.h>
#include <utils/KeyedVector.h>
#include "vpu_api.h"
#include "vpu_mem_pool.h"

#ifdef AVS50
#include <media/IMediaHTTPService.h>
#endif

typedef int32_t (*VpuCodecOpenCtxFactory)(VpuCodecContext **ctx);
typedef int32_t (*VpuCodecCloseCtxFactory)(VpuCodecContext **ctx);
typedef struct VpuCodec {
    VpuCodecContext *codec_ctx;
    VpuCodecOpenCtxFactory open_codec;
    VpuCodecCloseCtxFactory close_codec;
}VpuCodec_t;

typedef struct VC1InterLacedCheck {
    bool have_check;
    bool interlaced;
}VC1InterLacedCheck_t;

typedef struct _sw_api {
    int (*init)(void **obj);
    int (*decode)(void *obj,void *vpacket);
    int (*getDecinfo)(void *obj,unsigned int *width,
                   unsigned int *height, unsigned int *stride);
    int (*getFrame)(void *obj,unsigned char **Y,
                    unsigned char **U, unsigned char **V,uint64_t *pts);

    void (*decClose)(void *obj);
    void (*flush)(void *obj);
}sw_api;


namespace android {

class RK_MetadataRetriever: public MediaMetadataRetrieverInterface{
public:
    RK_MetadataRetriever();
    virtual ~RK_MetadataRetriever();
#ifdef AVS50
    virtual status_t setDataSource(
            const sp<IMediaHTTPService> &httpService,
            const char *url,
            const KeyedVector<String8, String8> *headers= NULL);
#else
    virtual status_t setDataSource(
            const char *url,
            const KeyedVector<String8, String8> *headers);
#endif
    virtual status_t setDataSource(int fd, int64_t offset, int64_t length);

    virtual VideoFrame *getFrameAtTime(int64_t timeUs, int option);

    virtual MediaAlbumArt *extractAlbumArt();

    virtual const char *extractMetadata(int keyCode);
private:

    bool                        isRkHwSupport(void *stream);
    int32_t                     parseNALSize(const uint8_t *data) const;
    static  void                ffmpegNotify(void* ptr, int level, const char* fmt, va_list vl);
    int32_t                     mVideo_type;
    VpuCodec_t                  *mVpuCodec;
    void*                       mDec_handle;
    bool                        mHwdecFlag;
    int32_t                     mNALLengthSize;
    int64_t                     mLastTimeUs;
    bool                        mHeaderSendFlag;
    int32_t                     initExtendSoftwareDecoder(void *stream);
    int32_t                     DecoderVideoInit(void* stream);
    status_t                    prepareVideo();
    bool                        hwprepare(void *stream);
    bool                        Sfdec(void *stream,void *packet_t,VideoFrame **frame);
    bool                        Hwdec(void *stream,void *packet_t,VideoFrame **frame);
    int32_t                     checkVc1StreamKeyFrame(void* stream, void* packet);
    void *			            mMovieFile;
    int 						mAudioStreamIndex;
    int 						mVideoStreamIndex;

    int                         mDuration;
    int                         mCurrentPosition;
    int                         mVideoWidth;
    int                         mVideoHeight;
    bool                        noFfmeg_codecsupport;
    void                        *mSwHandle;
    sw_api                      *mSwapi;
    void                        *mSw_lib_handle;
    VC1InterLacedCheck_t        mVc1InterlaceChk;
    Mutex                       mLock;

    bool mParsedMetaData;

    KeyedVector<int, String8> mMetaData;

    MediaAlbumArt *mAlbumArt;

    void* mPrivate;

    void parseMetaData();
    VideoFrame* parseHevcTs(int loopnum);

    RK_MetadataRetriever(const RK_MetadataRetriever &);

    RK_MetadataRetriever &operator=(
            const RK_MetadataRetriever &);
};
}
#endif // RK_METADATA_RETRIEVER_H
