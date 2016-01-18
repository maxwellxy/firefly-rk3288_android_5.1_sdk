/*
 * Copyright (C) 2011 The RockChip Android Source Project
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

#ifndef SkHWJpegUtility_DEFINED
#define SkHWJpegUtility_DEFINED

#include "SkImageDecoder.h"
#include "SkStream.h"

extern "C" {
	#include "vpu_mem.h"
	#include "hw_jpegdecapi.h"	
    //#include "jpeglib.h"
    //#include "jerror.h"
}

//#include <setjmp.h>

/* Our error-handling struct.
 *
*/
//struct skjpeg_error_mgr : jpeg_error_mgr {
//    jmp_buf fJmpBuf;
//};


//void skjpeg_error_exit(j_common_ptr cinfo);

///////////////////////////////////////////////////////////////////////////
/* Our source struct for directing jpeg to our stream object.
*/
struct sk_hw_jpeg_source_mgr:hw_jpeg_source_mgr {
    sk_hw_jpeg_source_mgr(SkStream* stream, SkImageDecoder* decoder, HwJpegInputInfo* hwInfo, int vpuMem);
    ~sk_hw_jpeg_source_mgr();

    SkStream*   fStream;
    //void*       fMemoryBase;
    //size_t      fMemoryBaseSize;
    //bool        fUnrefStream;
    SkImageDecoder* fDecoder;
	//const void* fBaseAddr;//if stream is memoryStream, fBaseAddr is not null
    enum {
        kBufferSize = 1024//4096
    };
    unsigned char    fBuffer[kBufferSize];
};
int sk_fill_thumb(HwJpegInputInfo* hwInfo, void * thumbBuf);

class SkJpegVPUMemStream : public SkMemoryStream {
public:
	size_t bytesInStream;//if equal -1 , means the data in this stream is error
	SkStream *baseStream;
	SkJpegVPUMemStream(SkStream* stream, size_t* len);
	virtual ~SkJpegVPUMemStream();
	VPUMemLinear_t* getVpuMemInst();
	void setNewMemory(VPUMemLinear_t* src, size_t size);
private:
	VPUMemLinear_t vpuMem;
};

class AutoScaleBitmap : public SkBitmap{
public:
	char reusebm;
	uint32_t finalWidth;
	uint32_t finalHeight;
	SkBitmap* bm;
	SkImageDecoder* fDecoder;
	AutoScaleBitmap();
	~AutoScaleBitmap();
	void setSourceAndFinalWH(char reuse, uint32_t w, uint32_t h,	SkBitmap* bitmap, SkImageDecoder* decoder);
};

#endif
