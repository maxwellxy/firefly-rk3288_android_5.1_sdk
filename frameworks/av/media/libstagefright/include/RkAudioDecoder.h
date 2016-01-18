#ifndef RK_AUDIO_DECODER_H_
#define RK_AUDIO_DECODER_H_

namespace android {

class MediaSource;

 sp<MediaSource> CreateRkAudioDecoderFactory(const sp<MediaSource> &source, const char* decoderType);

}  // namespace android

#endif //RK_AUDIO_DECODER_H_
