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
#define LOG_TAG "StagefrightMediaScanner"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <media/stagefright/StagefrightMediaScanner.h>

#include <media/IMediaHTTPService.h>
#include <media/mediametadataretriever.h>
#include <private/media/VideoFrame.h>

// Sonivox includes
#include <libsonivox/eas.h>
#define GETAPETAG
#ifdef GETAPETAG
#include "ApeGetFileInfo.h"
#include "get_ape_id3.h"
#endif
#include <utils/String8.h>

namespace android {

StagefrightMediaScanner::StagefrightMediaScanner() {}

StagefrightMediaScanner::~StagefrightMediaScanner() {}

static bool FileHasAcceptableExtension(const char *extension) {
    static const char *kValidExtensions[] = {
        ".mp3", ".mp4",".mov",".m4a", ".3gp", ".3gpp", ".3g2", ".3gpp2",
        ".mpeg", ".ogg", ".mid", ".smf", ".imy", ".wma", ".aac",
        ".wav", ".amr", ".midi", ".xmf", ".rtttl", ".rtx", ".ota",
        ".mkv", ".mka", ".webm", ".ts",".avi",".flv",".wmv", ".asf", ".mpg",
        ".vob",".dat",".flac",".ape",".mpga",".ts",".tp",
        ".trp",".m2ts", ".mxmf",".mp2",".mp1",".mxmf", ".f4v", ".mts", ".divx", ".dcf"
    };
    static const size_t kNumValidExtensions =
        sizeof(kValidExtensions) / sizeof(kValidExtensions[0]);

    for (size_t i = 0; i < kNumValidExtensions; ++i) {
        if (!strcasecmp(extension, kValidExtensions[i])) {
            return true;
        }
    }

    return false;
}

static MediaScanResult HandleMIDI(
        const char *filename, MediaScannerClient *client) {
    // get the library configuration and do sanity check
    const S_EAS_LIB_CONFIG* pLibConfig = EAS_Config();
    if ((pLibConfig == NULL) || (LIB_VERSION != pLibConfig->libVersion)) {
        ALOGE("EAS library/header mismatch\n");
        return MEDIA_SCAN_RESULT_ERROR;
    }
    EAS_I32 temp;

    // spin up a new EAS engine
    EAS_DATA_HANDLE easData = NULL;
    EAS_HANDLE easHandle = NULL;
    EAS_RESULT result = EAS_Init(&easData);
    if (result == EAS_SUCCESS) {
        EAS_FILE file;
        file.path = filename;
        file.fd = 0;
        file.offset = 0;
        file.length = 0;
        result = EAS_OpenFile(easData, &file, &easHandle);
    }
    if (result == EAS_SUCCESS) {
        result = EAS_Prepare(easData, easHandle);
    }
    if (result == EAS_SUCCESS) {
        result = EAS_ParseMetaData(easData, easHandle, &temp);
    }
    if (easHandle) {
        EAS_CloseFile(easData, easHandle);
    }
    if (easData) {
        EAS_Shutdown(easData);
    }

    if (result != EAS_SUCCESS) {
        return MEDIA_SCAN_RESULT_SKIPPED;
    }

    char buffer[20];
    sprintf(buffer, "%ld", temp);
    status_t status = client->addStringTag("duration", buffer);
    if (status != OK) {
        return MEDIA_SCAN_RESULT_ERROR;
    }
    return MEDIA_SCAN_RESULT_OK;
}

#ifdef GETAPETAG
static MediaScanResult parseAPE(const char *filename, MediaScannerClient& client)
{
	FILE* apeFile = fopen(filename,"r");
	ALOGD("parseAPE filename = %s",filename);
	if (!apeFile)
        return MEDIA_SCAN_RESULT_ERROR;
	struct APE_FILE_INFO ape_file_info;

    if(ERROR_SUCCESS == ApeHeaderAnalyze(apeFile, &ape_file_info))
    {
		ALOGD("parseAPE ApeHeaderAnalyze success");
		char buffer[20];
		//int bitrate = 0;
		int samplerate = 0;

		//get duration
		ALOGD("duration =%d bitrate= %d",ape_file_info.nLengthMS,ape_file_info.nAverageBitrate);
	  	if(ape_file_info.nLengthMS > 0)
	  	{

			sprintf(buffer, "%d", ape_file_info.nLengthMS);
	        if (!client.addStringTag("duration", buffer)) goto failure;
	  	}

		//get bitrate
		if (ape_file_info.nAverageBitrate > 0)
		{
			sprintf(buffer,"%d",ape_file_info.nAverageBitrate);
			if(!client.addStringTag("bitrate", buffer)) goto failure;
		}
		//get SampleRate
		if(ape_file_info.nSampleRate > 0)
		{
			sprintf(buffer,"%d",ape_file_info.nSampleRate);
			if(!client.addStringTag("samplerate", buffer)) goto failure;
		}
    }
	else
    {
		ALOGE("now this format ape will didnit support");
		goto failure;
    }

	ALOGD("ape get ID3 in");
	ApeId3 apeId3;
	if (!apeId3.getapetagex(apeFile))
	{

		ALOGD("ape get ID3 ");
		if(strlen(apeId3.apetagex.Artist))
		{
			if(!client.addStringTag("artist", apeId3.apetagex.Artist)) goto failure;
		}
		if(strlen(apeId3.apetagex.Album))
		{
			if(!client.addStringTag("album", apeId3.apetagex.Album)) goto failure;
		}
		if(strlen(apeId3.apetagex.Year))
		{
			if(!client.addStringTag("year", apeId3.apetagex.Year)) goto failure;
		}
		if(strlen(apeId3.apetagex.Genre))
		{
			if(!client.addStringTag("genre", apeId3.apetagex.Genre)) goto failure;
		}
	}
	ALOGD("ape get ID3 out");

	//the following code will be parse the ID3 info
	fclose(apeFile);
	return MEDIA_SCAN_RESULT_OK;

failure:
	fclose(apeFile);
    return MEDIA_SCAN_RESULT_ERROR;
}

#endif
MediaScanResult StagefrightMediaScanner::processFile(
        const char *path, const char *mimeType,
        MediaScannerClient &client) {
    ALOGV("processFile '%s'.", path);

    client.setLocale(locale());
    client.beginFile();
    MediaScanResult result = processFileInternal(path, mimeType, client);
    client.endFile();
    return result;
}

MediaScanResult StagefrightMediaScanner::processFileInternal(
        const char *path, const char * /* mimeType */,
        MediaScannerClient &client) {
    const char *extension = strrchr(path, '.');

    if (!extension) {
        return MEDIA_SCAN_RESULT_SKIPPED;
    }

    if (!FileHasAcceptableExtension(extension)) {
        return MEDIA_SCAN_RESULT_SKIPPED;
    }

    if (!strcasecmp(extension, ".mid")
            || !strcasecmp(extension, ".smf")
            || !strcasecmp(extension, ".imy")
            || !strcasecmp(extension, ".midi")
            || !strcasecmp(extension, ".xmf")
            || !strcasecmp(extension, ".rtttl")
            || !strcasecmp(extension, ".rtx")
            || !strcasecmp(extension, ".ota")
            || !strcasecmp(extension, ".mxmf")) {
        return HandleMIDI(path, &client);
    }
#ifdef GETAPETAG
	  if (!strcasecmp(extension, ".ape")) {
	    return parseAPE(path, client);
	}
#endif//GETAPETAG
    sp<MediaMetadataRetriever> mRetriever(new MediaMetadataRetriever);

    int fd = open(path, O_RDONLY | O_LARGEFILE);
    status_t status;
    if (fd < 0) {
        // couldn't open it locally, maybe the media server can?
        status = mRetriever->setDataSource(NULL /* httpService */, path);
    } else {
        status = mRetriever->setDataSource(fd, 0, 0x7ffffffffffffffL);
        close(fd);
    }

    if (status) {
        return MEDIA_SCAN_RESULT_ERROR;
    }

    const char *value;
    if ((value = mRetriever->extractMetadata(
                    METADATA_KEY_MIMETYPE)) != NULL) {
        status = client.setMimeType(value);
        if (status) {
            return MEDIA_SCAN_RESULT_ERROR;
        }
    }

    struct KeyMap {
        const char *tag;
        int key;
    };
    static const KeyMap kKeyMap[] = {
        { "tracknumber", METADATA_KEY_CD_TRACK_NUMBER },
        { "discnumber", METADATA_KEY_DISC_NUMBER },
        { "album", METADATA_KEY_ALBUM },
        { "artist", METADATA_KEY_ARTIST },
        { "albumartist", METADATA_KEY_ALBUMARTIST },
        { "composer", METADATA_KEY_COMPOSER },
        { "genre", METADATA_KEY_GENRE },
        { "title", METADATA_KEY_TITLE },
        { "year", METADATA_KEY_YEAR },
        { "duration", METADATA_KEY_DURATION },
        { "writer", METADATA_KEY_WRITER },
        { "compilation", METADATA_KEY_COMPILATION },
        { "isdrm", METADATA_KEY_IS_DRM },
        { "width", METADATA_KEY_VIDEO_WIDTH },
        { "height", METADATA_KEY_VIDEO_HEIGHT },
    };
    static const size_t kNumEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

    for (size_t i = 0; i < kNumEntries; ++i) {
        const char *value;
        if ((value = mRetriever->extractMetadata(kKeyMap[i].key)) != NULL) {
			String8 rkTag(value);
			
			if(rkTag.find("rkutf8") == 0)
			{
				//LOGI("-->key %s value %s",kKeyMap[i].tag,value);
				status = client.handleStringTag(kKeyMap[i].tag, value+6);
			}	
			else
            status = client.addStringTag(kKeyMap[i].tag, value);
            if (status != OK) {
                return MEDIA_SCAN_RESULT_ERROR;
            }
        }
    }

    return MEDIA_SCAN_RESULT_OK;
}

MediaAlbumArt *StagefrightMediaScanner::extractAlbumArt(int fd) {
    ALOGV("extractAlbumArt %d", fd);

    off64_t size = lseek64(fd, 0, SEEK_END);
    if (size < 0) {
        return NULL;
    }
    lseek64(fd, 0, SEEK_SET);

    sp<MediaMetadataRetriever> mRetriever(new MediaMetadataRetriever);
    if (mRetriever->setDataSource(fd, 0, size) == OK) {
        sp<IMemory> mem = mRetriever->extractAlbumArt();
        if (mem != NULL) {
            MediaAlbumArt *art = static_cast<MediaAlbumArt *>(mem->pointer());
            return art->clone();
        }
    }

    return NULL;
}

}  // namespace android
