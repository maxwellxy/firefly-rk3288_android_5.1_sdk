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
#include "SkHwJpegUtility.h"
#include "SkCanvas.h"
#ifdef DEBUG_HW_JPEG
#include "utils/Log.h"
#undef LOG_TAG
#define LOG_TAG "SkHwJpegUtility"
#define WHLOG LOGE
#endif

/////////////////////////////////////////////////////////////////////
static void sk_init_source(HwJpegInputInfo* hwInfo) {
    sk_hw_jpeg_source_mgr*  src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
    src->next_input_byte = (const unsigned char*)src->fBuffer;
    src->bytes_in_buffer = 0;
    src->cur_offset_instream = 0;
    src->fStream->rewind();
}

#define LARGEST_LENGTH (16*1024) //the same to the size of stream limit in BitmapFactory.java

static HW_BOOL sk_skip_input_data(HwJpegInputInfo* hwInfo, long num_bytes) {
    sk_hw_jpeg_source_mgr*  src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
WHLOG("skip num_bytes: %d, cur offset : %d", num_bytes, src->cur_offset_instream);
#if 0
    if (num_bytes > (long)src->bytes_in_buffer) {
        num_bytes = num_bytes - src->bytes_in_buffer;
WHLOG("real skip bytes: %d, bytes_in_buffer: %d", num_bytes, src->bytes_in_buffer);
		long bytesToSkip = sk_hw_jpeg_source_mgr::kBufferSize;
		while(num_bytes > 0) {
            long bytes = (long)src->fStream->skip(bytesToSkip);
            if (bytes <= 0 || bytes > bytesToSkip) {
                WHLOG("Error occure in skip input data.");
                return false;
            }
            src->cur_offset_instream += bytes;
            num_bytes -= bytes;
WHLOG("skip: %d, and remain: %d", bytes, num_bytes);
			if(num_bytes < sk_hw_jpeg_source_mgr::kBufferSize){
				bytesToSkip = num_bytes;
			}
		}
        src->next_input_byte = (const unsigned char*)src->fBuffer;
        src->bytes_in_buffer = 0;
    } else {
        src->next_input_byte += num_bytes;
        src->bytes_in_buffer -= num_bytes;
    }
#else
	if (num_bytes > (long)src->bytes_in_buffer) {
        long bytesToSkip = num_bytes - src->bytes_in_buffer;
#if 1
		if(hwInfo->justcaloutwh && (bytesToSkip + src->cur_offset_instream > LARGEST_LENGTH)){
			WHLOG("skip over Largest length.");
			return false;
		}
#endif
        while (bytesToSkip > 0) {
			long bytes = (long)src->fStream->skip(bytesToSkip);
			
            if (bytes <= 0 || bytes > bytesToSkip) {
				SkDebugf("failure to skip request %d returned %d\n", bytesToSkip, bytes);
                return false;
            }
            src->cur_offset_instream += bytes;
            bytesToSkip -= bytes;
        }
        src->next_input_byte = (const unsigned char*)src->fBuffer;
        src->bytes_in_buffer = 0;
    } else {
        src->next_input_byte += num_bytes;
        src->bytes_in_buffer -= num_bytes;
    }
#endif
WHALLLOG("skip finish, cur offset : %d", src->cur_offset_instream);
	return true;
}

static HW_BOOL sk_seek_input_data(HwJpegInputInfo* hwInfo, long byte_offset) {
    sk_hw_jpeg_source_mgr* src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
#if 0
    if (byte_offset > src->cur_offset_instream) {
		src->bytes_in_buffer = 0;
		src->next_input_byte = (const unsigned char*)src->fBuffer;
		if(! sk_skip_input_data(hwInfo, byte_offset - src->cur_offset_instream)){
			WHLOG("seek error for skip : %d bytes",byte_offset - src->cur_offset_instream);
			return false;		
		}
        //(void)src->fStream->skip(byte_offset - src->cur_offset_instream);
    } else if(byte_offset < src->cur_offset_instream){
		src->bytes_in_buffer = 0;
		src->cur_offset_instream = 0;
        src->fStream->rewind();
		if(! sk_skip_input_data(hwInfo, byte_offset)){
			WHLOG("seek error for skip : %d bytes from start",byte_offset);
			return false;		
		}
        //(void)src->fStream->skip(byte_offset);
    }
#else
	WHALLLOG("seek to : %d, cur offset: %d", byte_offset, src->cur_offset_instream);
	if (byte_offset > src->cur_offset_instream) {
        (void)src->fStream->skip(byte_offset - src->cur_offset_instream);
    } else if(byte_offset < src->cur_offset_instream){
        src->fStream->rewind();
        (void)src->fStream->skip(byte_offset);
    }
#endif
    src->cur_offset_instream = byte_offset;
    src->next_input_byte = (const unsigned char*)src->fBuffer;
    src->bytes_in_buffer = 0;
    return true;
}

static HW_BOOL sk_fill_input_buffer(HwJpegInputInfo* hwInfo) {
    sk_hw_jpeg_source_mgr* src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
WHALLLOG("fill input buffer start cur_offset: %d", src->cur_offset_instream);
	size_t bytes = 0;
#if 1
	if(hwInfo->justcaloutwh && (src->cur_offset_instream + sk_hw_jpeg_source_mgr::kBufferSize > LARGEST_LENGTH)){
		if(LARGEST_LENGTH <= src->cur_offset_instream){
			WHLOG("check here, LARGEST_LENGTH <= src->cur_offset_instream");
			return false;
		}
		bytes = src->fStream->read(src->fBuffer, LARGEST_LENGTH - src->cur_offset_instream);
	}else{
#endif
    	bytes = src->fStream->read(src->fBuffer, sk_hw_jpeg_source_mgr::kBufferSize);
#if 1	
	}
#endif
    // note that JPEG is happy with less than the full read,
    // as long as the result is non-zero
    if (bytes == 0) {
        return false;
    }

    src->cur_offset_instream += bytes;
    src->next_input_byte = (const unsigned char*)src->fBuffer;
    src->bytes_in_buffer = bytes;
WHALLLOG("fill input buffer finish, bytes: %d, cur_offset: %d", bytes, src->cur_offset_instream);
    return true;
}

static HW_BOOL sk_resync_to_restart(HwJpegInputInfo* hwInfo) {
    sk_hw_jpeg_source_mgr*  src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
    if (!src->fStream->rewind()) {
        WHLOG("____________failure to rewind\n");
        return false;
    }
	src->cur_offset_instream = 0;
    src->next_input_byte = (const unsigned char*)src->fBuffer;
    src->bytes_in_buffer = 0;
WHALLLOG("resync to start");
    return true;
}

//static void sk_term_source(HwJpegInputInfo* /*hwInfo*/) {}

static int sk_fill_buffer(HwJpegInputInfo* hwInfo, void * des, VPUMemLinear_t *newVpuMem, int w, int h){
	sk_hw_jpeg_source_mgr*  src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
	if(des == NULL || (src->fDecoder != NULL && src->fDecoder->shouldCancelDecode())) {
		WHLOG("return -1 at start in fill buffer.");
		return -1;
	}
#if 0
	unsigned char * destination = (unsigned char *)des;
	long bytes = hwInfo->streamCtl.wholeStreamLength-src->cur_offset_instream;
	long num = bytes;
	size_t bytesOnce = sk_hw_jpeg_source_mgr::kBufferSize;

	WHLOG("fill buffer: %d bytes; wholeLen:%d,curoffset:%d",bytes,hwInfo->streamCtl.wholeStreamLength,src->cur_offset_instream);

	while(bytes > 0){// fix readlimit in BitmapFactory.java decodeStream in the future
		size_t rdBytes = src->fStream->read(destination, bytesOnce);
    	if (rdBytes <= 0) {
			WHLOG("read fail in fill buffer.");
        	return -1;
    	}
		src->cur_offset_instream += rdBytes;
		destination += rdBytes;
		bytes -= rdBytes;
		if(bytes < sk_hw_jpeg_source_mgr::kBufferSize){
			bytesOnce = bytes;
		}
	}
    src->next_input_byte = (const unsigned char*)src->fBuffer;
    src->bytes_in_buffer = 0;
	return num;
#else
	SkStream *stream = NULL;
	size_t rdBytes = -1;
	long bytes = 0;
	char *destination = (char*)des;
	size_t *bytesHasIn = NULL;
	if(src->isVpuMem){
		stream = ((SkJpegVPUMemStream*)(src->fStream))->baseStream;
		bytesHasIn = &(((SkJpegVPUMemStream*)(src->fStream))->bytesInStream);
		bytes = hwInfo->streamCtl.wholeStreamLength - *bytesHasIn;
WHLOG("bytes: %d, hwInfo->streamCtl.wholeStreamLength: %d,  *bytesHasIn: %d", bytes, hwInfo->streamCtl.wholeStreamLength, *bytesHasIn);
		if(bytes <= 0){
			hwInfo->streamCtl.wholeStreamLength = *bytesHasIn;
			goto HANDLE;
			//return hwInfo->streamCtl.wholeStreamLength;
		} else if(bytes > hwInfo->streamCtl.wholeStreamLength){
			return -1;
		}
		destination += *bytesHasIn;
	} else {
		bytes = hwInfo->streamCtl.wholeStreamLength-src->cur_offset_instream;
		stream = src->fStream;
	}
	rdBytes = stream->read(destination, bytes);
WHLOG("REAL rdbytes: %d", rdBytes);
    if (bytes > 0) {
        if (rdBytes <=0 || rdBytes > bytes) {
            if(NULL != bytesHasIn ){
                //vpumem
                *bytesHasIn = -1;//read error
            }
            return -1;
        }
    }
	src->cur_offset_instream += rdBytes;
	if(rdBytes < bytes){//recompute whole length
		if(src->isVpuMem){
		 	rdBytes = rdBytes + *bytesHasIn;
		 	*bytesHasIn = rdBytes;
		}
		hwInfo->streamCtl.wholeStreamLength = rdBytes;
		WHREDLOG("REAL whole stream length: %d", rdBytes);
	}
HANDLE:
	//if stream getlength is wrong, will go follow codes
	unsigned char* buf = src->fBuffer;
	rdBytes = stream->read(buf, sk_hw_jpeg_source_mgr::kBufferSize);
WHLOG("reReadbyte is : %d", rdBytes);
#ifndef UNLIKELY
#define UNLIKELY(x) __builtin_expect((x), false)
#else 
#error UNLIKELY has been def
#endif
#ifndef LIKELY
#define LIKELY(x) __builtin_expect((x), true)
#else
#error LIKELY has been def
#endif
	if(rdBytes > 0){
		WHREDLOG("all right , we need relloc a vpu mem, first readbyte is : %d", rdBytes);
		VPUMemLinear_t tmpVpuMem;
		tmpVpuMem.vir_addr = NULL;
		int mallocSize = hwInfo->streamCtl.wholeStreamLength + rdBytes;
		if(rdBytes < sk_hw_jpeg_source_mgr::kBufferSize){
			mallocSize = (mallocSize + 255)&(~255);
			if(mallocSize < JPEG_INPUT_BUFFER) {
				mallocSize = JPEG_INPUT_BUFFER;
			}
			hw_jpeg_VPUMallocLinear(&tmpVpuMem, mallocSize);
			if(LIKELY(tmpVpuMem.vir_addr != NULL)){
				memcpy(tmpVpuMem.vir_addr, des, hwInfo->streamCtl.wholeStreamLength);
				memcpy((unsigned char *)tmpVpuMem.vir_addr + hwInfo->streamCtl.wholeStreamLength, buf, rdBytes);
				hwInfo->streamCtl.wholeStreamLength += rdBytes;
				src->cur_offset_instream += rdBytes;
			}else{
				goto ERROR;
			}
		}else{
			size_t tmp = mallocSize;
			WHLOG("11 tmp: %d", tmp);
			WHREDLOG("read is filling buf, and input wh: %d,%d", w , h);
			if(LIKELY(w > 0 && h > 0)){
				if(LIKELY(mallocSize < w*h)){
					mallocSize = w*h;
				}else{
					WHLOG("readed bytes: %d, input w: %d, input h: %d", mallocSize, w, h);
					WHLOG("net stream, now dicide the mallocSize to four times of readed bytes.");
					mallocSize *= 4;
				}
			}else{
				WHLOG("net stream, and not get w or h before, now dicide the mallocSize to four times of readed bytes.");
				mallocSize *= 4;
			}
			mallocSize = (mallocSize + 255)&(~255);
			if(mallocSize < JPEG_INPUT_BUFFER) {
				mallocSize = JPEG_INPUT_BUFFER;
			}
			WHLOG("first real mallocsize: %d", mallocSize);
			hw_jpeg_VPUMallocLinear(&tmpVpuMem, mallocSize);
			if(LIKELY(tmpVpuMem.vir_addr != NULL)){
				unsigned char * mem = (unsigned char*)tmpVpuMem.vir_addr;
				memcpy(mem, des, hwInfo->streamCtl.wholeStreamLength);
				mem += hwInfo->streamCtl.wholeStreamLength;
				memcpy(mem, buf, rdBytes);
				mem += rdBytes;
#if 1
				rdBytes = stream->read(mem, mallocSize - tmp);
				if(rdBytes > 0){
					tmp += rdBytes;
					WHLOG("22 tmp: %d, rdBytes: %d", tmp, rdBytes);
					do{
						if(UNLIKELY(tmp >= mallocSize)){
							WHLOG("unenough space, we need relloc a vpumem to fill data");
							mallocSize *= 2;
							WHLOG("second real mallocsize: %d", mallocSize);
							VPUMemLinear_t tmpVpuMem2;
							tmpVpuMem2.vir_addr = NULL;
							hw_jpeg_VPUMallocLinear(&tmpVpuMem2, mallocSize);
							if(tmpVpuMem2.vir_addr != NULL){
								mem = (unsigned char*)tmpVpuMem2.vir_addr;
								memcpy(mem, tmpVpuMem.vir_addr, tmp);
								mem += tmp;
								hw_jpeg_VPUFreeLinear(&tmpVpuMem);
								tmpVpuMem = tmpVpuMem2;
							}else{
								WHREDLOG("relloc vpumem2 fail.");
								//release vpumem
								hw_jpeg_VPUFreeLinear(&tmpVpuMem);
								goto ERROR;
							}
						}else{
							break;
						}
						rdBytes = stream->read(mem, mallocSize - tmp);
						if(rdBytes <= 0){
							break;
						}
						tmp += rdBytes;
						WHLOG("fuck tmp: %d, rdBytes: %d", tmp, rdBytes);
					}while(true);
				}else{
					goto ERROR;
				}
#else
				do{
					if(UNLIKELY(tmp + sk_hw_jpeg_source_mgr::kBufferSize > mallocSize)){
						WHLOG("unenough space, we need relloc a vpumem to fill data");
						mallocSize *= 2;
						VPUMemLinear_t tmpVpuMem2;
						tmpVpuMem2.vir_addr = NULL;
						hw_jpeg_VPUMallocLinear(&tmpVpuMem2, mallocSize);
						if(tmpVpuMem2.vir_addr != NULL){
							mem = (unsigned char*)tmpVpuMem2.vir_addr;
							memcpy(mem, tmpVpuMem.vir_addr, tmp);
							mem += tmp;
							hw_jpeg_VPUFreeLinear(&tmpVpuMem);
							tmpVpuMem = tmpVpuMem2;
						}else{
							WHREDLOG("relloc vpumem2 fail.");
							//release vpumem
							hw_jpeg_VPUFreeLinear(&tmpVpuMem);
							goto ERROR;
						}
					}
					rdBytes = stream->read(mem, sk_hw_jpeg_source_mgr::kBufferSize);
					if(rdBytes <= 0){
						break;
					}
					mem += rdBytes;
					tmp += rdBytes;
				}while(true);
#endif
				hwInfo->streamCtl.wholeStreamLength = tmp;
			}else{
				goto ERROR;
			}
		}
		*newVpuMem = tmpVpuMem;
		if(tmpVpuMem.vir_addr != NULL && src->isVpuMem){
			((SkJpegVPUMemStream*)(src->fStream))->setNewMemory(newVpuMem, hwInfo->streamCtl.wholeStreamLength);
		}
	}
    src->next_input_byte = (const unsigned char*)src->fBuffer;
    src->bytes_in_buffer = 0;
	return hwInfo->streamCtl.wholeStreamLength;//destination == (char*)des?rdBytes:hwInfo->streamCtl.wholeStreamLength;
ERROR:
	if(src->isVpuMem){
		*bytesHasIn = -1;
	}
	return -1;
#endif
}

static HW_BOOL sk_read_1_byte(HwJpegInputInfo* hwInfo, unsigned char * ch){
	sk_hw_jpeg_source_mgr*  src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
	if(src->bytes_in_buffer == 0){
		if(!sk_fill_input_buffer(hwInfo)) {
			WHLOG("fill_input_buffer fail, may be at the end of the stream.");
			return false;
		}
	}
	src->bytes_in_buffer--;
	*ch = *src->next_input_byte++;
	return true;
}

int sk_fill_thumb(HwJpegInputInfo* hwInfo, void * thumbBuf){
	sk_hw_jpeg_source_mgr*  src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
	if(thumbBuf == NULL || (src->fDecoder != NULL && src->fDecoder->shouldCancelDecode())) 		{
		SkDebugf("return -1 at start in fill thumb.");
		return -1;
	}
	if(!sk_seek_input_data(hwInfo, hwInfo->streamCtl.thumbOffset)){
		SkDebugf("seek fail in fill thumb.");
		return -1;
	}
#if 0
	unsigned char * destination = (unsigned char *)thumbBuf;
	size_t bytes = hwInfo->streamCtl.thumbLength;
	WHLOG("fill thumb buffer: %d bytes",bytes);
	size_t bytesOnce = sk_hw_jpeg_source_mgr::kBufferSize;
	while(bytes > 0){// fix readlimit in BitmapFactory.java decodeStream in the future
		size_t rdBytes = src->fStream->read(destination, bytesOnce);
    	if (rdBytes <= 0) {
			WHLOG("read fail in fill buffer.");
        	return -1;
    	}
		src->cur_offset_instream += rdBytes;
		destination += rdBytes;
		bytes -= rdBytes;
		if(bytes < sk_hw_jpeg_source_mgr::kBufferSize){
			bytesOnce = bytes;
		}
	}
#else
	size_t bytes = src->fStream->read(thumbBuf, hwInfo->streamCtl.thumbLength);
    if (bytes <= 0 || bytes != hwInfo->streamCtl.thumbLength) {
		SkDebugf("read fail in fill thumb.");
        return -1;
    }
	src->cur_offset_instream += bytes;
#endif
    src->next_input_byte = (const unsigned char*)src->fBuffer;
    src->bytes_in_buffer = 0;
    //SkDebugf("%s \n",__FUNCTION__);
	return hwInfo->streamCtl.thumbLength;
}

void sk_get_vpumemInst(HwJpegInputInfo* hwInfo, VPUMemLinear_t* thumbBuf){
	sk_hw_jpeg_source_mgr* src = (sk_hw_jpeg_source_mgr*)hwInfo->streamCtl.inStream;
	if(!src->isVpuMem){
		return;	
	}
	SkJpegVPUMemStream* stream = (SkJpegVPUMemStream*)src->fStream;
	if(thumbBuf != NULL){
		*thumbBuf = *(stream->getVpuMemInst());
	}
}

/*
static HW_BOOL sk_get_ppInfo_msg(HwJpegInputInfo* hwInfo){
	PostProcessInfo * ppInfo = &hwInfo->ppInfo;
	SkJPEGImageDecoder* decoder = src->fDecoder;
	if(decoder == NULL){
		return false;
	}
	
}*/

///////////////////////////////////////////////////////////////////////////////

sk_hw_jpeg_source_mgr::sk_hw_jpeg_source_mgr(SkStream* stream, SkImageDecoder* decoder, HwJpegInputInfo* hwInfo, HW_BOOL vpuMem): fStream(stream) {
    fDecoder = decoder;
    //fBaseAddr = stream->getMemoryBase();
	//WHLOG("BASE ADDR: %x, kBufferSize: %d, pagesize: %d", fBaseAddr, kBufferSize,getpagesize());
	isVpuMem = vpuMem;
    //fMemoryBase = NULL;
    //fUnrefStream = ownStream;
    //fMemoryBaseSize = 0;
	info = hwInfo;
    init_source = sk_init_source;
    fill_input_buffer = sk_fill_input_buffer;
    skip_input_data = sk_skip_input_data;
    resync_to_restart = sk_resync_to_restart;
    //term_source = sk_term_source;
    seek_input_data = sk_seek_input_data;
	fill_buffer = sk_fill_buffer;
	fill_thumb = sk_fill_thumb;
	read_1_byte = sk_read_1_byte;
	get_vpumemInst = sk_get_vpumemInst;
	//*get_ppInfo_msg = sk_get_ppInfo_msg;
}

sk_hw_jpeg_source_mgr::~sk_hw_jpeg_source_mgr() {
    fStream = NULL;
	fDecoder = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
SkJpegVPUMemStream::SkJpegVPUMemStream(SkStream * stream, size_t *len){
	size_t size = *len;
	vpuMem.vir_addr = NULL;
	bytesInStream = 0;
	baseStream = stream;
	int mallocSize = (size+255)&(~255);
	if (mallocSize < LARGEST_LENGTH)
        mallocSize = LARGEST_LENGTH;
	if (mallocSize < JPEG_INPUT_BUFFER) {
		mallocSize = JPEG_INPUT_BUFFER;
	}
	WHLOG("SkJpegVPUMemStream size: %d ", mallocSize);
	hw_jpeg_VPUMallocLinear(&vpuMem, mallocSize);
	if(vpuMem.vir_addr != NULL)
	{
		stream->rewind();
		bool readApp1 = false, readApp0 = false, readSof = false, error = false;
		unsigned short twoBytes = 0;
		unsigned char * mem = (unsigned char *)vpuMem.vir_addr;
		while(bytesInStream < size && error == false){
			if((bytesInStream + 2 > LARGEST_LENGTH) || (bytesInStream + 2 > mallocSize)){
				error = true;
				break;
			}
			size_t readBytes = stream->read(&twoBytes,2);//FIX ME if big edian
			WHLOG("meet mark : 0x%x, readBytes: %d", twoBytes, readBytes);
			error = (readBytes<=0);
			if(error){
				break;
			}
			bytesInStream += readBytes;
			memcpy(mem, &twoBytes, sizeof(unsigned short));
			mem += readBytes;
			switch(twoBytes){
				case 0xD9FF://EOI
					readApp1 = true;
					readApp0 = true;
					readSof = true;
					break;
				case 0xD8FF://SOI
					break;
				case 0xE0FF://APP0
				case 0xE1FF://APP1
				case 0xC0FF://SOF0
				case 0xC2FF://SOF2
					if(twoBytes == 0xC0FF || twoBytes == 0xC2FF){
						readSof = true;
					} else if(twoBytes == 0xE0FF){
						readApp0 = true;
					}else if(twoBytes == 0xE1FF){
						readApp1 = true;
					}
				default:
					if((bytesInStream + 2 > LARGEST_LENGTH) || (bytesInStream + 2 > mallocSize)){
						error = true;
						break;
					}
					readBytes = stream->read(&twoBytes,2);
					error = (readBytes<=0);
					if(error){
						break;
					}
					bytesInStream += readBytes;
					memcpy(mem, &twoBytes, sizeof(unsigned short));
					mem += readBytes;
					twoBytes = ((twoBytes & 0x000000ff) << 8) | ((twoBytes & 0x0000ff00) >> 8);
					if((bytesInStream + twoBytes - 2 > LARGEST_LENGTH) || (bytesInStream + twoBytes - 2 > mallocSize)){
						error = true;
						break;
					}
					readBytes = stream->read(mem,twoBytes - 2);
					error = (readBytes<=0);
					if(error){
						break;
					}
					bytesInStream += readBytes;
					mem += readBytes;
					break;
			}
			if((readApp1 || readApp0) && readSof){
				//just read app0 app1 sofn; optimization for decodeFile() when request size less than thumbnail in file;
				break;			
			}
		}
		//size_t bytes = stream->read(vpuMem->vir_addr,size);//read all to vpumem
		if(error == false && bytesInStream > 0){
			WHLOG("bytesInStream is : %d, size is : %d", bytesInStream, size);
			if(bytesInStream < mallocSize){//!= size){
				mem[0] = 0xFF;mem[1] = 0xD9;//set end
			}
			if(bytesInStream > size){
				*len = bytesInStream;//wrong base stream length, set to bytesInStream
			}
			this->setMemory(vpuMem.vir_addr, *len, false);
			//hw_jpeg_VPUMemFlush(&vpuMem);
		} else {
			WHLOG("read all to vpumemstream fail.");
			this->setMemory(NULL,0);
			hw_jpeg_VPUFreeLinear(&vpuMem);
			vpuMem.vir_addr = NULL;
		}
	} else {
		this->setMemory(NULL,0);
	}
}

SkJpegVPUMemStream::~SkJpegVPUMemStream(){
	if(vpuMem.vir_addr != NULL){
		hw_jpeg_VPUFreeLinear(&vpuMem);
	}
}

VPUMemLinear_t* SkJpegVPUMemStream::getVpuMemInst(){
	return &vpuMem;
}

void SkJpegVPUMemStream::setNewMemory(VPUMemLinear_t *src, size_t size){
	if(vpuMem.vir_addr != NULL){
		hw_jpeg_VPUFreeLinear(&vpuMem);
	}
	vpuMem = *src;
	bytesInStream = size;
	this->setMemory(src->vir_addr, size, false);
}

AutoScaleBitmap::AutoScaleBitmap(){
	finalWidth = -1;
	finalHeight = -1;
	bm = NULL;
	fDecoder = NULL;
}

#ifdef DEBUG_HW_JPEG
#include "SkTime.h"

class AutoTimeMilli {
public:
    AutoTimeMilli(const char label[]) : fLabel(label) {
        if (!fLabel) {
            fLabel = "";
        }
        fNow = SkTime::GetMSecs();
    }
    ~AutoTimeMilli() {
        SkDebugf("%s ---- Time:%d ms\n", fLabel, SkTime::GetMSecs() - fNow);
    }
private:
    const char* fLabel;
    SkMSec      fNow;
};
#endif

AutoScaleBitmap::~AutoScaleBitmap(){
#ifdef DEBUG_HW_JPEG
	 //AutoTimeMilli atm("auto scale bitmap");
#endif
	//scale bitmap to request size
	if(fDecoder != NULL && bm != NULL && finalWidth > 0 && finalHeight > 0){
		if(reusebm == 0)
		{
		
		    const SkColorType colorType = this->colorType();
   			const SkAlphaType alphaType = kAlpha_8_SkColorType == colorType ?
                                      kPremul_SkAlphaType : kOpaque_SkAlphaType;
			bm->setInfo(SkImageInfo::Make(finalWidth, finalHeight,
                                            colorType, alphaType));
			//bm->setConfig(this->getConfig(), finalWidth, finalHeight);
			//bm->setIsOpaque(this->isOpaque());
			bm->allocPixels(fDecoder->getAllocator(),NULL);
		}
		SkCanvas canvas(*bm);
		SkIRect src;
		src.set(0,0,this->width(),this->height());
		SkRect dst;
		dst.set(0,0,SkIntToScalar(finalWidth),SkIntToScalar(finalHeight));
		SkPaint paint;
		paint.setAntiAlias(true);//set AA. Since the config is the same, no need to set dither
		canvas.drawBitmapRect(*this, &src, dst, &paint);
		if(reusebm > 0){
			bm->notifyPixelsChanged();
		}
	}
}

void AutoScaleBitmap::setSourceAndFinalWH(char reuse, uint32_t w, uint32_t h, SkBitmap* bitmap, SkImageDecoder* decoder)
{
	reusebm = reuse;
	finalWidth = w;
	finalHeight = h;
	bm = bitmap;
	fDecoder = decoder;
}

