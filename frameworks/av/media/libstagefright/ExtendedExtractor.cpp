#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedExtractor"
#include <utils/Log.h>
//#define DUMP_TO_FILE


#include <media/stagefright/ExtendedExtractorFuncs.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaDefs.h>
#include <utils/String8.h>
#include <dlfcn.h>  // for dlopen/dlclose

#include "include/ExtendedExtractor.h"
#define LOGV ALOGV
#define LOGI ALOGI
#define LOGE ALOGE
#define LOGD ALOGD

static const char* RK_DEMUX_LIB = "librk_demux.so";

namespace android {

void* RkDemuxLib() {
    static void* pRkDemuxLib = NULL;
    static bool alreadyTriedToOpenRkDemuxs = false;

    if(alreadyTriedToOpenRkDemuxs) {
        return pRkDemuxLib;
    }

    alreadyTriedToOpenRkDemuxs = true;

    pRkDemuxLib = ::dlopen(RK_DEMUX_LIB,RTLD_NOW); //RTLD_LAZY);

    return pRkDemuxLib;
}

MediaExtractorFactory MediaExtractorFactoryFunction() {
    static MediaExtractorFactory mediaFactoryFunction = NULL;
    static bool alreadyTriedToFindFactoryFunction = false;

    if(alreadyTriedToFindFactoryFunction) {
        return mediaFactoryFunction;
    }

    void *pRkDemuxLib = RkDemuxLib();
    if (pRkDemuxLib == NULL) {
        return NULL;
    }

    mediaFactoryFunction = (MediaExtractorFactory) dlsym(pRkDemuxLib, MEDIA_CREATE_EXTRACTOR);
    alreadyTriedToFindFactoryFunction = true;

    if(mediaFactoryFunction==NULL) {
        LOGE(" dlsym for ExtendedExtractor factory function failed, dlerror = %s \n", dlerror());
    }

    return mediaFactoryFunction;
}

sp<MediaExtractor> ExtendedExtractor::CreateExtractor(const sp<DataSource> &source, const char* mime) {
    MediaExtractorFactory f = MediaExtractorFactoryFunction();
    if(f==NULL) {
        return NULL;
    }
    sp<MediaExtractor> extractor = f(source, mime);
    if(extractor==NULL) {
        LOGE(" ExtendedExtractor failed to instantiate extractor \n");
    }

    return extractor;
}

void ExtendedExtractor::RegisterSniffers() {
    void *pRkDemuxLib = RkDemuxLib();
    if (pRkDemuxLib == NULL) {
        return;
    }

    SnifferArrayFunc snifferArrayFunc = (SnifferArrayFunc) dlsym(pRkDemuxLib, MEDIA_SNIFFER_ARRAY);
    if(snifferArrayFunc==NULL) {
        LOGE(" Unable to init Extended Sniffers, dlerror = %s \n", dlerror());
        return;
    }

    Vector<DataSource::SnifferFunc> Sniffers;
    int snifferCount = 0;

    //Invoke function in libmmparser to return its array of sniffers.
    snifferArrayFunc(Sniffers, &snifferCount);
    if(snifferCount == 0) {
        LOGE(" snifferArray is NULL \n");
        return;
    }

    bool flag= true;
    //Register the remote sniffers with the DataSource.
    for(int i=0; i<snifferCount; i++) {
          DataSource::RegisterSniffer_l(Sniffers.editItemAt(i));
          flag = false;
    }
    Sniffers.clear();
}

}  // namespace android


