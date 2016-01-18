/*
 * Copyright 2013 Rockchip Electronics Co. LTD
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


#ifndef _APE_GET_FILE_INFO_
#define _APE_GET_FILE_INFO_

typedef unsigned short   ape_uint16;
typedef short            ape_int16;
typedef unsigned long    ape_uint32;
typedef long             ape_int32;
typedef unsigned char    ape_uchar;
typedef char             ape_char;
typedef long             ape_BOOL;

#define			TRUE			1
#define			FALSE			0

/*****************************************************************************************
Error Codes
*****************************************************************************************/

// success
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS                                   0
#endif

// file and i/o errors (1000's)
#define ERROR_IO_READ                                   1000
#define ERROR_IO_WRITE                                  1001
#define ERROR_INVALID_INPUT_FILE                        1002
#define ERROR_INVALID_OUTPUT_FILE                       1003
#define ERROR_INPUT_FILE_TOO_LARGE                      1004
#define ERROR_INPUT_FILE_UNSUPPORTED_BIT_DEPTH          1005
#define ERROR_INPUT_FILE_UNSUPPORTED_SAMPLE_RATE        1006
#define ERROR_INPUT_FILE_UNSUPPORTED_CHANNEL_COUNT      1007
#define ERROR_INPUT_FILE_TOO_SMALL                      1008
#define ERROR_INVALID_CHECKSUM                          1009
#define ERROR_DECOMPRESSING_FRAME                       1010
#define ERROR_INITIALIZING_UNMAC                        1011
#define ERROR_INVALID_FUNCTION_PARAMETER                1012
#define ERROR_UNSUPPORTED_FILE_TYPE                     1013
#define ERROR_UPSUPPORTED_FILE_VERSION                  1014

// memory errors (2000's)
#define ERROR_INSUFFICIENT_MEMORY                       2000

// dll errors (3000's)
#define ERROR_LOADINGAPE_DLL                            3000
#define ERROR_LOADINGAPE_INFO_DLL                       3001
#define ERROR_LOADING_UNMAC_DLL                         3002

// general and misc errors
#define ERROR_USER_STOPPED_PROCESSING                   4000
#define ERROR_SKIPPED                                   4001

// programmer errors
#define ERROR_BAD_PARAMETER                             5000

// IAPECompress errors
#define ERROR_APE_COMPRESS_TOO_MUCH_DATA                6000

// unknown error
#define ERROR_UNDEFINED                                -1


/*****************************************************************************************
Defines
*****************************************************************************************/


#define MAC_FORMAT_FLAG_8_BIT                 1    // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2    // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4    // ape_uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8    // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16    // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32    // create the wave header on decompression (not stored)

#define CREATE_WAV_HEADER_ON_DECOMPRESSION    -1
#define MAX_AUDIO_BYTES_UNKNOWN -1

#define COMPRESSION_LEVEL_FAST          1000
#define COMPRESSION_LEVEL_NORMAL        2000
#define COMPRESSION_LEVEL_HIGH          3000
#define COMPRESSION_LEVEL_EXTRA_HIGH    4000
#define COMPRESSION_LEVEL_INSANE        5000

/*****************************************************************************************
WAV header structure
*****************************************************************************************/
struct WAVE_HEADER
{
  // RIFF header
  ape_char cRIFFHeader[4];//20070528 fixed
  ape_uint32 nRIFFBytes;

  // data type
  ape_char cDataTypeID[4];//20070528 fixed

  // wave format
  ape_char cFormatHeader[4];//20070528 fixed
  ape_uint32 nFormatBytes;

  ape_uint16 nFormatTag;
  ape_uint16 nChannels;
  ape_uint32 nSamplesPerSec;
  ape_uint32 nAvgBytesPerSec;
  ape_uint16 nBlockAlign;
  ape_uint16 nBitsPerSample;

  // data chunk header
  ape_char cDataHeader[4];//20070528 fixed
  ape_uint32 nDataBytes;
};


/*****************************************************************************************
APE header that all APE files have in common (old and new)
*****************************************************************************************/
struct APE_COMMON_HEADER
{
    ape_char cID[4];                            // should equal 'MAC '//20070528 fixed
    ape_uint16 nVersion;                        // version number * 1000 (3.81 = 3810)
};

/*****************************************************************************************
APE header structure for old APE files (3.97 and earlier)
*****************************************************************************************/
struct APE_HEADER_OLD
{
    ape_char cID[4];                            // should equal 'MAC '
    ape_uint16 nVersion;                        // version number * 1000 (3.81 = 3810)
    ape_uint16 nCompressionLevel;               // the compression level
    ape_uint16 nFormatFlags;                    // any format flags (for future use)
    ape_uint16 nChannels;                       // the number of channels (1 or 2)
    ape_uint32 nSampleRate;                     // the sample rate (typically 44100)
    ape_uint32 nHeaderBytes;                    // the bytes after the MAC header that compose the WAV header
    ape_uint32 nTerminatingBytes;               // the bytes after that raw data (for extended info)
    ape_uint32 nTotalFrames;                    // the number of frames in the file
    ape_uint32 nFinalFrameBlocks;               // the number of samples in the final frame
};

/*****************************************************************************************
APE_DESCRIPTOR structure (file header that describes lengths, offsets, etc.)
*****************************************************************************************/
struct APE_DESCRIPTOR
{
  ape_char    cID[4];                             // should equal 'MAC '//20070528 fixed
  ape_uint16  nVersion;                           // version number * 1000 (3.81 = 3810)
  ape_uint16  tmp;

  ape_uint32  nDescriptorBytes;                   // the number of descriptor bytes (allows later expansion of this header)
  ape_uint32  nHeaderBytes;                       // the number of header APE_HEADER bytes
  ape_uint32  nSeekTableBytes;                    // the number of bytes of the seek table
  ape_uint32  nHeaderDataBytes;                   // the number of header data bytes (from original file)
  ape_uint32  nAPEFrameDataBytes;                 // the number of bytes of APE frame data
  ape_uint32  nAPEFrameDataBytesHigh;             // the high order number of APE frame data bytes
  ape_uint32  nTerminatingDataBytes;              // the terminating data of the file (not including tag data)

  //uint8   cFileMD5[16];                       // the MD5 hash of the file (see notes for usage... it's a littly tricky)
  ape_uchar cFileMD5[16];//20070528 fixed
  //ape_uchar cFileMD5[8];//20070528 fixed
};


/*****************************************************************************************
APE_FILE_INFO - structure which describes most aspects of an APE file
(used internally for speed and ease)
*****************************************************************************************/
struct APE_FILE_INFO
{
  ape_int32 nVersion;                                   // file version number * 1000 (3.93 = 3930)
  ape_int32 nCompressionLevel;                          // the compression level
  ape_int32 nFormatFlags;                               // format flags
  ape_int32 nTotalFrames;                               // the total number frames (frames are used internally)
  ape_int32 nBlocksPerFrame;                            // the samples in a frame (frames are used internally)
  ape_int32 nFinalFrameBlocks;                          // the number of samples in the final frame
  ape_int32 nChannels;                                  // audio channels
  ape_int32 nSampleRate;                                // audio samples per second
  ape_int32 nBitsPerSample;                             // audio bits per sample
  ape_int32 nBytesPerSample;                            // audio bytes per sample
  ape_int32 nBlockAlign;                                // audio block align (channels * bytes per sample)
  ape_int32 nWAVHeaderBytes;                            // header bytes of the original WAV
  ape_int32 nWAVDataBytes;                              // data bytes of the original WAV
  ape_int32 nWAVTerminatingBytes;                       // terminating bytes of the original WAV
  ape_int32 nWAVTotalBytes;                             // total bytes of the original WAV
  ape_int32 nAPETotalBytes;                             // total bytes of the APE file
  ape_int32 nTotalBlocks;                               // the total number audio blocks
  ape_int32 nLengthMS;                                  // the length in milliseconds
  ape_int32 nAverageBitrate;                            // the kbps (i.e. 637 kpbs)
  ape_int32 nDecompressedBitrate;                       // the kbps of the decompressed audio (i.e. 1440 kpbs for CD audio)
  ape_int32 nJunkHeaderBytes;                           // used for ID3v2, etc.
  ape_int32 nSeekTableElements;                         // the number of elements in the seek table(s)

  /*Mod by Wei.Hisung 2007.03.06*/
  ape_uint32* spSeekByteTable;              // the seek table (byte)
  ape_uchar* spSeekBitTable;        // the seek table (bits -- legacy)
  ape_uchar* spWaveHeaderData;      // the pre-audio header data
  struct APE_DESCRIPTOR* spAPEDescriptor;      // the descriptor (only with newer files)
};

/*****************************************************************************************
APE_HEADER structure (describes the format, duration, etc. of the APE file)
*****************************************************************************************/
struct APE_HEADER
{
	ape_uint16    nCompressionLevel;                 // the compression level (see defines I.E. COMPRESSION_LEVEL_FAST)
	ape_uint16    nFormatFlags;                      // any format flags (for future use)

	ape_uint32    nBlocksPerFrame;                   // the number of audio blocks in one frame
	ape_uint32    nFinalFrameBlocks;                 // the number of audio blocks in the final frame
	ape_uint32    nTotalFrames;                      // the total number of frames

	ape_uint16    nBitsPerSample;                    // the bits per sample (typically 16)
	ape_uint16    nChannels;                         // the number of channels (1 or 2)
	ape_uint32    nSampleRate;                       // the sample rate (typically 44100)
};

ape_int32 ApeHeaderAnalyze(void *pHandle, struct APE_FILE_INFO *pInfo);

#endif

























