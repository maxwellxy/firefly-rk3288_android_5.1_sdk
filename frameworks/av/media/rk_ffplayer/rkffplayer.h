#ifndef RK_FF_MEDIAPLAYER_H
#define RK_FF_MEDIAPLAYER_H

#include <media/MediaPlayerInterface.h>
#include <utils/Vector.h>

namespace android {

class FFPlayerDelegate;

class FF_MediaPlayer
{
public:
	FF_MediaPlayer();
	virtual             ~FF_MediaPlayer();
	void                setListener(const wp<MediaPlayerBase> &listener);
	status_t            setDataSource(int fd, int64_t offset, int64_t length);
#ifdef AVS50
      status_t             setDataSource(const sp<IMediaHTTPService> &httpService,
                                        const char *url, const KeyedVector<String8, String8> *headers);
#else
    status_t            setDataSource(const char *uri, const KeyedVector<String8, String8> *headers);
#endif

#if defined(AVS44) || defined (AVS50)
	status_t            setSurfaceTexture(const sp<IGraphicBufferProducer> &bufferProducer);
#else
	status_t            setSurfaceTexture(const sp<ISurfaceTexture> &surfaceTexture);
#endif
	void                setAudioSink(const sp<MediaPlayerBase::AudioSink> &audioSink);
	 status_t	        prepare();
	 status_t           prepareAsync();
	 status_t           start();
	 status_t           stop();
	 status_t           pause();
	 status_t           reset();
	 bool               isPlaying();
	 status_t           seekTo(int64_t timeUs);
	 status_t           getDuration(int64_t *durationUs);
	 status_t           getPosition(int64_t *positionUs);
	 status_t           invoke(const Parcel &request, Parcel *reply);
	 status_t           getParameter(int key, Parcel *reply);
	 status_t           setParameter(int key, const Parcel &request);
	 uint32_t           flags() const ;
	 status_t           setLooping(int loop) ;
   
private:
    FFPlayerDelegate* mPlayer;
    
	FF_MediaPlayer(const FF_MediaPlayer &);
	FF_MediaPlayer &operator=(const FF_MediaPlayer &);

};
}
#endif // RK_FF_MEDIAPLAYER_H
