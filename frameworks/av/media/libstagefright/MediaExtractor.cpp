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
#define LOG_TAG "MediaExtractor"
#include <utils/Log.h>

#include "include/AMRExtractor.h"
#include "include/MP3Extractor.h"
#include "include/MPEG4Extractor.h"
#include "include/WAVExtractor.h"
#include "include/OggExtractor.h"
#include "include/MPEG2TSExtractor.h"
#include "include/DRMExtractor.h"
#include "include/WVMExtractor.h"
#include "include/FLACExtractor.h"
#include "include/AACExtractor.h"

#include "include/ExtendedExtractor.h"

#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>
#include <cutils/properties.h>

namespace android {

sp<MetaData> MediaExtractor::getMetaData() {
    return new MetaData;
}

uint32_t MediaExtractor::flags() const {
    return CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_PAUSE | CAN_SEEK;
}

// static
sp<MediaExtractor> MediaExtractor::Create(
        const sp<DataSource> &source, const char *mime) {
    sp<AMessage> meta;

    String8 tmp;
    float confidence = 0;
    if (mime == NULL) {
        float confidence;
        if (!source->sniff(&tmp, &confidence, &meta)) {
            ALOGV("FAILED to autodetect media content.");

            return NULL;
        }

        mime = tmp.string();
        ALOGV("Autodetected media content as '%s' with confidence %.2f",
             mime, confidence);
    }

    bool isDrm = false;
    // DRM MIME type syntax is "drm+type+original" where
    // type is "es_based" or "container_based" and
    // original is the content's cleartext MIME type
    if (!strncmp(mime, "drm+", 4)) {
        const char *originalMime = strchr(mime+4, '+');
        if (originalMime == NULL) {
            // second + not found
            return NULL;
        }
        ++originalMime;
        if (!strncmp(mime, "drm+es_based+", 13)) {
            // DRMExtractor sets container metadata kKeyIsDRM to 1
            return new DRMExtractor(source, originalMime);
        } else if (!strncmp(mime, "drm+container_based+", 20)) {
            mime = originalMime;
            isDrm = true;
        } else {
            return NULL;
        }
    }

    MediaExtractor *ret = NULL;
    char value[PROPERTY_VALUE_MAX];
    if(property_get("media.demux.cfg", value, NULL)){
        ALOGV("read demux cfg, value: %s", value);
    if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
                || !strcasecmp(mime, "audio/mp4")
                || (isDrm && (strstr(mime, "audio/m4a")))
                || (isDrm && (strstr(mime, "audio/3gpp")))
                || (isDrm && (strstr(mime, "video/mp4")))
                || (isDrm && (strstr(mime, "video/m4v")))
                || (isDrm && (strstr(mime, "video/3gpp")))) {
            if (strstr(value, "Mov")) {
        ret = new MPEG4Extractor(source);
            }
        }else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)
                || (isDrm && (strstr(mime, "audio/mp3")))) {
            if (strstr(value, "Mp3")) {
                ret = new MP3Extractor(source, meta);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
                || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)
                || (isDrm && (strstr(mime, "audio/amr"))
                || (isDrm && (strstr(mime, "audio/3gpp"))))) {
            if (strstr(value, "Amr")) {
                ret = new AMRExtractor(source);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
            ret = new FLACExtractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
            if (strstr(value, "Wav")) {
                ret = new WAVExtractor(source);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
            if (strstr(value, "Ogg")) {
                ret = new OggExtractor(source);
            }
        }else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) {
            if (strstr(value, "Mpeg2ts")) {
                ret = new MPEG2TSExtractor(source);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WVM)) {
            ret = new WVMExtractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS) ||
        	!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_ADIF) ||
        	!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_ADTS) ||
        	(isDrm && (strstr(mime, "audio/aac")))) {
    	    if (strstr(value, "Aac")) {
                ret = new AACExtractor(source, meta);
            }
        }
    } else {
        if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
                || !strcasecmp(mime, "audio/mp4")
                || (isDrm && (strstr(mime, "audio/m4a")))
                || (isDrm && (strstr(mime, "audio/3gpp")))
                || (isDrm && (strstr(mime, "video/mp4")))
                || (isDrm && (strstr(mime, "video/m4v")))
                || (isDrm && (strstr(mime, "video/3gpp")))) {
            ret = new MPEG4Extractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)
                || (isDrm && (strstr(mime, "audio/mp3")))) {
        ret = new MP3Extractor(source, meta);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
                || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)
                || (isDrm && (strstr(mime, "audio/amr"))
                || (isDrm && (strstr(mime, "audio/3gpp"))))) {
        ret = new AMRExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
        ret = new FLACExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
        ret = new WAVExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
        ret = new OggExtractor(source);
    }else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) {
        ret = new MPEG2TSExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WVM)) {
        // Return now.  WVExtractor should not have the DrmFlag set in the block below.
            ret = new WVMExtractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS) ||
        	!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_ADIF) ||
            	!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_ADTS)) {
        ret = new AACExtractor(source, meta);
        }
    }

    if (ret != NULL && isDrm) {
			ret->setDrmFlag(true);
           /* <2012050301388 wanghao 20120503 begin */
           ret->getMetaData()->setInt32(kKeyIsDRM, 1);
           /* 2012050301388 wanghao 20120503 end> */
    } else if (ret != NULL) {
        ret->setDrmFlag(false);
    }
	if(ret == NULL){
        ALOGI(" Using ExtendedExtractor mime = %s \n",mime);
   		sp<MediaExtractor> retextParser =  ExtendedExtractor::CreateExtractor(source, mime);
    	if (retextParser != NULL){
        	return retextParser;
    	}
    }

    return ret;
}

sp<MediaExtractor> MediaExtractor::Create(
        const sp<DataSource> &source, const char *mime, const bool bSetDataSourceCall,const char *path ){
    sp<AMessage> meta;

    String8 tmp;
        float confidence = 0;
    if (mime == NULL) {
        float confidence;
        if (!source->sniff(&tmp, &confidence, &meta)) {
            ALOGV("FAILED to autodetect media content.");

            return NULL;
        }

        mime = tmp.string();
        ALOGV("Autodetected media content as '%s' with confidence %.2f",
             mime, confidence);
    } else {
	     tmp = mime;
	        if (!source->sniff(&tmp, &confidence, &meta)) {
	            ALOGV("FAILED to autodetect media content.");
	            return NULL;
	        }
	        ALOGV("Autodetected media content as '%s' with confidence %.2f",
	             mime, confidence);
		 mime = tmp.string();
    }

    bool isDrm = false;
    // DRM MIME type syntax is "drm+type+original" where
    // type is "es_based" or "container_based" and
    // original is the content's cleartext MIME type
    if (!strncmp(mime, "drm+", 4)) {
        const char *originalMime = strchr(mime+4, '+');
        if (originalMime == NULL) {
            // second + not found
            return NULL;
        }
        ++originalMime;
        if (!strncmp(mime, "drm+es_based+", 13)) {
            // DRMExtractor sets container metadata kKeyIsDRM to 1
            return new DRMExtractor(source, originalMime);
        } else if (!strncmp(mime, "drm+container_based+", 20)) {
            mime = originalMime;
            isDrm = true;
        } else {
            return NULL;
        }
    }
    MediaExtractor *ret = NULL;
    char value[PROPERTY_VALUE_MAX];
    if (property_get("media.demux.cfg", value, NULL)) {

        ALOGV("create 2nd, read demux cfg, value: %s", value);

        if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
                || !strcasecmp(mime, "audio/mp4")
                || (isDrm && (strstr(mime, "audio/m4a")))
                || (isDrm && (strstr(mime, "audio/3gpp")))
                || (isDrm && (strstr(mime, "video/mp4")))
                || (isDrm && (strstr(mime, "video/m4v")))
                || (isDrm && (strstr(mime, "video/3gpp")))) {
            if (strstr(value, "Mov")) {
                ret = new MPEG4Extractor(source);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)
                || (isDrm && (strstr(mime, "audio/mp3")))) {
			if (strstr(value, "Mp3")) {
                ret = new MP3Extractor(source, meta);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
                || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)
                || (isDrm && (strstr(mime, "audio/amr"))
                || (isDrm && (strstr(mime, "audio/3gpp"))))) {
            if (strstr(value, "Amr")) {
                ret = new AMRExtractor(source);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
            ret = new FLACExtractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
            if (strstr(value, "Wav")) {
                ret = new WAVExtractor(source);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
            if (strstr(value, "Ogg")) {
                ret = new OggExtractor(source);
            }
        }else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) {
            if(!strncasecmp("http://",path, 7)) {
                if (strstr(value, "Mpeg2ts")) {
                    ret = new MPEG2TSExtractor(source);
                }
            }
            else {
                if (strstr(value, "Mpeg2ts")) {
                    ret = new MPEG2TSExtractor(source);
                }
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WVM)) {
            ret = new WVMExtractor(source);
        }else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS) ||
        	!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_ADIF) ||
        	!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_ADTS) ||
        	(isDrm && (strstr(mime, "audio/aac")))) {
            if (strstr(value, "Aac")) {
                ret = new AACExtractor(source, meta);
            }
        }
    } else {
        if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
                || !strcasecmp(mime, "audio/mp4")
                || (isDrm && (strstr(mime, "audio/m4a")))
                || (isDrm && (strstr(mime, "audio/3gpp")))
                || (isDrm && (strstr(mime, "video/mp4")))
                || (isDrm && (strstr(mime, "video/m4v")))
                || (isDrm && (strstr(mime, "video/3gpp")))) {
            ret = new MPEG4Extractor(source);
        }else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)
                || (isDrm && (strstr(mime, "audio/mp3")))) {
            ret = new MP3Extractor(source, meta);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
                || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)
                || (isDrm && (strstr(mime, "audio/amr"))
                || (isDrm && (strstr(mime, "audio/3gpp"))))) {
            ret = new AMRExtractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
            ret = new FLACExtractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
            ret = new WAVExtractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
            ret = new OggExtractor(source);
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) {
            if (!strncasecmp("http://",path, 7)) {
                ret = new MPEG2TSExtractor(source);
            } else {
                ret = new MPEG2TSExtractor(source);
            }
        } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WVM)) {
            ret = new WVMExtractor(source);
        }else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS) ||
        	!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_ADIF) ||
        	!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_ADTS) ||
        	(isDrm && (strstr(mime, "audio/aac")))) {
            ret = new AACExtractor(source, meta);
        }
    }

    if (ret != NULL && isDrm) {
		ret->setDrmFlag(true);
       /* <2012050301388 wanghao 20120503 begin */
       ret->getMetaData()->setInt32(kKeyIsDRM, 1);
       /* 2012050301388 wanghao 20120503 end> */
    } else if (ret != NULL) {
        ret->setDrmFlag(false);
    }
    if(ret == NULL){
    	ALOGI(" Using ExtendedExtractor mime = %s \n",mime);
    	sp<MediaExtractor> retextParser =  ExtendedExtractor::CreateExtractor(source, mime);
    	if (retextParser != NULL){
        	return retextParser;
    	}
    }
    return ret;
}

}  // namespace android
