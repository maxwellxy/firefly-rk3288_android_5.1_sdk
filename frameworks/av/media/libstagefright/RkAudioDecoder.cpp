#define LOG_NDEBUG 0
#define LOG_TAG "RKAUDIODECODER"
#include <utils/Log.h>

#include <utils/threads.h>

#include <media/stagefright/MediaSource.h>
#include <dlfcn.h>  // for dlopen/dlclose

#include "include/RkAudioDecoder.h"


namespace android {

static const char* RK_AUDIO_DEC_LIB = "librk_audio.so";

//Prototype for factory function - extended RkAudioDecoder must export a function with this prototype to
//instantiate RkAudioDecoder objects.

typedef sp<MediaSource> (*RkAudioDecoder)(const sp<MediaSource> &source, const char* decoderType);

//Function name for extractor factory function. Extended extractor must export a function with this name.
static const char* CREATE_RK_AUDIO_DECODER = "CreateRkAudioDecoder";

static Mutex    mRkAudioDecoderLock;


sp<MediaSource> CreateRkAudioDecoderFactory(const sp<MediaSource> &source, const char* decoderType){

	static RkAudioDecoder rkAudioDecoder = NULL;
    static bool needToGetDecoder = true;
	{
		//the first time ,the static variables like "needToGetDecoder" must be changed ,so we must lock this code.
		Mutex::Autolock _l(mRkAudioDecoderLock);

		if(needToGetDecoder){
			void* audioLibHandle = NULL;
				
			audioLibHandle = ::dlopen(RK_AUDIO_DEC_LIB,RTLD_NOW|RTLD_GLOBAL); 

			if(audioLibHandle){
				//if the device has the librk_audio.so ,we just dlopen not dlcose,just as the object which I get from dlsym always be used 
				//on Mediaserver process
				rkAudioDecoder = (RkAudioDecoder) dlsym(audioLibHandle, CREATE_RK_AUDIO_DECODER);
				
			}
			else{
				ALOGE("Hi, I can't find the librk_audio.so in your firmware,so the audio can't decoder,pls double check it!");
			}
			needToGetDecoder = false;
		}
	}

	if(rkAudioDecoder == NULL){
		ALOGE("Hi ,I can't find the CreateRkAudioDecoder function in the librk_audio.so, pls double check it");
		return NULL;
	}
	
    sp<MediaSource> decoder = rkAudioDecoder(source, decoderType);
    if(decoder == NULL) {
        ALOGE("Hi, rkAudioDecoderfactory can't  instantiate %s audio decoder,pls check it!",decoderType);
    }
	
    return decoder;
}

}  // namespace android


