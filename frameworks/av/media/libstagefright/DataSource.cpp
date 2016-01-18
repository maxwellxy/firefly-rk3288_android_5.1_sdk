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
#define LOG_TAG "DataSource"

#include "include/AMRExtractor.h"

#include "include/AACExtractor.h"
#include "include/DRMExtractor.h"
#include "include/FLACExtractor.h"
#include "include/HTTPBase.h"
#include "include/MP3Extractor.h"
#include "include/MPEG2PSExtractor.h"
#include "include/MPEG2TSExtractor.h"
#include "include/MPEG4Extractor.h"
#include "include/NuCachedSource2.h"
#include "include/OggExtractor.h"
#include "include/WAVExtractor.h"
#include "include/WVMExtractor.h"

#if 0
#include "matroska/MatroskaExtractor.h"
#endif

#include "include/ExtendedExtractor.h"

#include <media/IMediaHTTPConnection.h>
#include <media/IMediaHTTPService.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/DataURISource.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaHTTP.h>
#include <utils/String8.h>

#include <cutils/properties.h>

namespace android {

bool DataSource::getUInt16(off64_t offset, uint16_t *x) {
    *x = 0;

    uint8_t byte[2];
    if (readAt(offset, byte, 2) != 2) {
        return false;
    }

    *x = (byte[0] << 8) | byte[1];

    return true;
}

bool DataSource::getUInt24(off64_t offset, uint32_t *x) {
    *x = 0;

    uint8_t byte[3];
    if (readAt(offset, byte, 3) != 3) {
        return false;
    }

    *x = (byte[0] << 16) | (byte[1] << 8) | byte[2];

    return true;
}

bool DataSource::getUInt32(off64_t offset, uint32_t *x) {
    *x = 0;

    uint32_t tmp;
    if (readAt(offset, &tmp, 4) != 4) {
        return false;
    }

    *x = ntohl(tmp);

    return true;
}

bool DataSource::getUInt64(off64_t offset, uint64_t *x) {
    *x = 0;

    uint64_t tmp;
    if (readAt(offset, &tmp, 8) != 8) {
        return false;
    }

    *x = ntoh64(tmp);

    return true;
}

status_t DataSource::getSize(off64_t *size) {
    *size = 0;

    return ERROR_UNSUPPORTED;
}

////////////////////////////////////////////////////////////////////////////////

Mutex DataSource::gSnifferMutex;
List<DataSource::SnifferFunc> DataSource::gSniffers;
bool DataSource::gSniffersRegistered = false;

bool DataSource::sniff(
        String8 *mimeType, float *confidence, sp<AMessage> *meta) {
    *mimeType = "";
    *confidence = 0.0f;
    meta->clear();
    String8 newMimeType;
    float newConfidence;
    sp<AMessage> newMeta;
   
   {
        Mutex::Autolock autoLock(gSnifferMutex);
        if (!gSniffersRegistered) {
            return false;
        }
    }

    if (gSniffers.empty() == true) {
       ALOGI("have not register any extractor now, register.");
       gSnifferMutex.unlock();
       RegisterDefaultSniffers();
       gSnifferMutex.lock();
    }
    if ((mimeType == NULL) || (mimeType->string() == NULL)) {
    for (List<SnifferFunc>::iterator it = gSniffers.begin();
         it != gSniffers.end(); ++it) {
            if ((*it)(this, &newMimeType, &newConfidence, &newMeta)) {
                if (newConfidence > *confidence) {
                    *mimeType = newMimeType;
                    *confidence = newConfidence;
                    *meta = newMeta;
                }
            }
        }
    } else {
        const char* mime = mimeType->string();
        SnifferFunc  func[10];
        int func_count =0;
        char mime_lower[5] = {'\0'};
		/*@jh: convert possible BIG character to SMALL one*/
		for (int id = 0; id < 5; id++)
		{
			if ((mime[id] >= 'A') && (mime[id] <= 'Z'))
				mime_lower[id] = mime[id] - 'A' + 'a';
			else
				mime_lower[id] = mime[id];
		}

		//LOGE("mime_lowerType->string()=%c%c%c%c%c",mime_lower[0],mime_lower[1],mime_lower[2],mime_lower[3],mime_lower[4]);
 		if(mime_lower[0] =='m' && mime_lower[1]=='p' && mime_lower[2]=='3' && mime_lower[3]=='\0')
		{
		 	func[0] = SniffMP3;
			func_count = 1;

		}
		else if(mime_lower[0] =='a' && mime_lower[1]=='a' && mime_lower[2]=='c' && mime_lower[3]=='\0')
		{
			func[0] = SniffAAC;
			func_count = 1;

		}
		else if(mime_lower[0] =='w' && mime_lower[1]=='a' && mime_lower[2]=='v' && mime_lower[3]=='\0')
		{
			func[0] = SniffWAV;
			func_count = 1;

		}
		else if(mime_lower[0] =='o' && mime_lower[1]=='g' && mime_lower[2]=='g' && mime_lower[3]=='\0')
		{
			func[0] = SniffOgg;
			func_count = 1;

		}
#if 0
		else if(mime_lower[0] =='m' && mime_lower[1]=='k' && mime_lower[2]=='v' && mime_lower[3]=='\0')
		{
			func[0] = SniffMatroska;
			func_count = 1;

		}
#endif
		else if((mime_lower[0] =='m' && mime_lower[1]=='p' && mime_lower[2]=='4' && mime_lower[3]=='\0' )
				||(mime_lower[0] =='m' && mime_lower[1]=='o' && mime_lower[2]=='v' && mime_lower[3]=='\0' )
				||(mime_lower[0] =='3' && mime_lower[1]=='g' && mime_lower[2]=='p' && mime_lower[3]=='\0' ))
		{
			func[0] = SniffMPEG4;

			func_count = 1;

		}
		else if((mime_lower[0] =='t' && mime_lower[1]=='s' && mime_lower[2]=='\0')
				||(mime_lower[0] == 't' && mime_lower[1] == 'p' && mime_lower[2] == '\0')
				||(mime_lower[0] == 't' && mime_lower[1] == 'r' && mime_lower[2] == 'p' && mime_lower[3] == '\0')
				||(mime_lower[0] == 'm' && mime_lower[1] == '2' && mime_lower[2] == 't' && mime_lower[3] == 's' && mime_lower[4] == '\0'))
		{
			func[0] = SniffMPEG2TS;
			func_count = 1;

		}
		else if(mime_lower[0] =='f' && mime_lower[1]=='l' && mime_lower[2]=='a' && mime_lower[3]=='c' &&mime_lower[4]=='\0')
		{
			ALOGE("herer \n");
			func[0] = SniffFLAC;
			func_count = 1;
		}
		else
		{
			;
		}
		//gettimeofday(&timeFirst, NULL);
		for(int i = 0;i < func_count;i++)
		{
			if(func[i](this, &newMimeType, &newConfidence, &newMeta))
			{
				if (newConfidence > *confidence) {
	                *mimeType = newMimeType;
	                *confidence = newConfidence;
	                *meta = newMeta;
	            }
			}
		}
		//gettimeofday(&timeSec, NULL);
		//LOGE("func time  is  %d\n", (timeSec.tv_sec - timeFirst.tv_sec) * 1000 + (timeSec.tv_usec - timeFirst.tv_usec) / 1000);

		if(*confidence == 0)
		{
			for (List<SnifferFunc>::iterator it = gSniffers.begin();
	         it != gSniffers.end(); ++it) {

	        	if ((*it)(this, &newMimeType, &newConfidence, &newMeta)) {
	            	if (newConfidence > *confidence) {
	                	*mimeType = newMimeType;
	                	*confidence = newConfidence;
	                	*meta = newMeta;
	            	}
	        	}
	    	}
		}
    }

    return *confidence > 0.0;
}

// static
void DataSource::RegisterSniffer_l(SnifferFunc func) {
    for (List<SnifferFunc>::iterator it = gSniffers.begin();
         it != gSniffers.end(); ++it) {
        if (*it == func) {
            return;
        }
    }

    gSniffers.push_back(func);
}

// static
void DataSource::RegisterDefaultSniffers() {
    Mutex::Autolock autoLock(gSnifferMutex);
    if (gSniffersRegistered) {
        return;
    }

    RegisterSniffer_l(SniffMPEG4);
#if 0
    RegisterSniffer_l(SniffMatroska);
#endif
    RegisterSniffer_l(SniffOgg);
    RegisterSniffer_l(SniffWAV);
    RegisterSniffer_l(SniffFLAC);
    RegisterSniffer_l(SniffAMR);
    RegisterSniffer_l(SniffMPEG2TS);
    RegisterSniffer_l(SniffMP3);
    RegisterSniffer_l(SniffAAC);
    ExtendedExtractor::RegisterSniffers();
    RegisterSniffer_l(SniffWVM);

    char value[PROPERTY_VALUE_MAX];
    if (property_get("drm.service.enabled", value, NULL)
            && (!strcmp(value, "1") || !strcasecmp(value, "true"))) {
        RegisterSniffer_l(SniffDRM);
    }
    gSniffersRegistered = true;
}

// static
sp<DataSource> DataSource::CreateFromURI(
        const sp<IMediaHTTPService> &httpService,
        const char *uri,
        const KeyedVector<String8, String8> *headers,
        String8 *contentType,
        HTTPBase *httpSource) {
    if (contentType != NULL) {
        *contentType = "";
    }

    bool isWidevine = !strncasecmp("widevine://", uri, 11);

    sp<DataSource> source;
    if (!strncasecmp("file://", uri, 7)) {
        source = new FileSource(uri + 7);
    } else if (!strncasecmp("http://", uri, 7)
            || !strncasecmp("https://", uri, 8)
            || isWidevine) {
        if (httpService == NULL) {
            ALOGE("Invalid http service!");
            return NULL;
        }

        if (httpSource == NULL) {
            sp<IMediaHTTPConnection> conn = httpService->makeHTTPConnection();
            if (conn == NULL) {
                ALOGE("Failed to make http connection from http service!");
                return NULL;
            }
            httpSource = new MediaHTTP(conn);
        }

        String8 tmp;
        if (isWidevine) {
            tmp = String8("http://");
            tmp.append(uri + 11);

            uri = tmp.string();
        }

        String8 cacheConfig;
        bool disconnectAtHighwatermark;
        KeyedVector<String8, String8> nonCacheSpecificHeaders;
        if (headers != NULL) {
            nonCacheSpecificHeaders = *headers;
            NuCachedSource2::RemoveCacheSpecificHeaders(
                    &nonCacheSpecificHeaders,
                    &cacheConfig,
                    &disconnectAtHighwatermark);
        }

        if (httpSource->connect(uri, &nonCacheSpecificHeaders) != OK) {
            ALOGE("Failed to connect http source!");
            return NULL;
        }

        if (!isWidevine) {
            if (contentType != NULL) {
                *contentType = httpSource->getMIMEType();
            }

            source = new NuCachedSource2(
                    httpSource,
                    cacheConfig.isEmpty() ? NULL : cacheConfig.string(),
                    disconnectAtHighwatermark);
        } else {
            // We do not want that prefetching, caching, datasource wrapper
            // in the widevine:// case.
            source = httpSource;
        }
    } else if (!strncasecmp("data:", uri, 5)) {
        source = DataURISource::Create(uri);
    } else {
        // Assume it's a filename.
        source = new FileSource(uri);
    }

    if (source == NULL || source->initCheck() != OK) {
        return NULL;
    }

    return source;
}

sp<DataSource> DataSource::CreateMediaHTTP(const sp<IMediaHTTPService> &httpService) {
    if (httpService == NULL) {
        return NULL;
    }

    sp<IMediaHTTPConnection> conn = httpService->makeHTTPConnection();
    if (conn == NULL) {
        return NULL;
    } else {
        return new MediaHTTP(conn);
    }
}

String8 DataSource::getMIMEType() const {
    return String8("application/octet-stream");
}

}  // namespace android
