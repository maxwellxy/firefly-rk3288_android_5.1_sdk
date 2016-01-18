

#ifndef APE_INCLUDE
#define APE_INCLUDE

	 enum APE_DECOMPRESS_FIELDS 
	{
    APE_INFO_FILE_VERSION = 1000,               // version of the APE file * 1000 (3.93 = 3930) [ignored, ignored]
    APE_INFO_COMPRESSION_LEVEL = 1001,          // compression level of the APE file [ignored, ignored]
    APE_INFO_FORMAT_FLAGS = 1002,               // format flags of the APE file [ignored, ignored]
    APE_INFO_SAMPLE_RATE = 1003,                // sample rate (Hz) [ignored, ignored]
    APE_INFO_BITS_PER_SAMPLE = 1004,            // bits per sample [ignored, ignored]
    APE_INFO_BYTES_PER_SAMPLE = 1005,           // number of bytes per sample [ignored, ignored]
    APE_INFO_CHANNELS = 1006,                   // channels [ignored, ignored]
    APE_INFO_BLOCK_ALIGN = 1007,                // block alignment [ignored, ignored]
    APE_INFO_BLOCKS_PER_FRAME = 1008,           // number of blocks in a frame (frames are used internally)  [ignored, ignored]
    APE_INFO_FINAL_FRAME_BLOCKS = 1009,         // blocks in the final frame (frames are used internally) [ignored, ignored]
    APE_INFO_TOTAL_FRAMES = 1010,               // total number frames (frames are used internally) [ignored, ignored]
    APE_INFO_WAV_HEADER_BYTES = 1011,           // header bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_TERMINATING_BYTES = 1012,      // terminating bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_DATA_BYTES = 1013,             // data bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_TOTAL_BYTES = 1014,            // total bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_APE_TOTAL_BYTES = 1015,            // total bytes of the APE file [ignored, ignored]
    APE_INFO_TOTAL_BLOCKS = 1016,               // total blocks of audio data [ignored, ignored]
    APE_INFO_LENGTH_MS = 1017,                  // length in ms (1 sec = 1000 ms) [ignored, ignored]
    APE_INFO_AVERAGE_BITRATE = 1018,            // average bitrate of the APE [ignored, ignored]
    APE_INFO_FRAME_BITRATE = 1019,              // bitrate of specified APE frame [frame index, ignored]
    APE_INFO_DECOMPRESSED_BITRATE = 1020,       // bitrate of the decompressed WAV [ignored, ignored]
    APE_INFO_PEAK_LEVEL = 1021,                 // peak audio level (obsolete) (-1 is unknown) [ignored, ignored]
    APE_INFO_SEEK_BIT = 1022,                   // bit offset [frame index, ignored]
    APE_INFO_SEEK_BYTE = 1023,                  // byte offset [frame index, ignored]
    APE_INFO_WAV_HEADER_DATA = 1024,            // error code [buffer *, max bytes]
    APE_INFO_WAV_TERMINATING_DATA = 1025,       // error code [buffer *, max bytes]
    APE_INFO_WAVEFORMATEX = 1026,               // error code [waveformatex *, ignored]
    APE_INFO_IO_SOURCE = 1027,                  // I/O source (CIO *) [ignored, ignored]
    APE_INFO_FRAME_BYTES = 1028,                // bytes (compressed) of the frame [frame index, ignored]
    APE_INFO_FRAME_BLOCKS = 1029,               // blocks in a given frame [frame index, ignored]
    APE_INFO_TAG = 1030,                        // point to tag (CAPETag *) [ignored, ignored]
    
    APE_DECOMPRESS_CURRENT_BLOCK = 2000,        // current block location [ignored, ignored]
    APE_DECOMPRESS_CURRENT_MS = 2001,           // current millisecond location [ignored, ignored]
    APE_DECOMPRESS_TOTAL_BLOCKS = 2002,         // total blocks in the decompressors range [ignored, ignored]
    APE_DECOMPRESS_LENGTH_MS = 2003,            // total blocks in the decompressors range [ignored, ignored]
    APE_DECOMPRESS_CURRENT_BITRATE = 2004,      // current bitrate [ignored, ignored]
    APE_DECOMPRESS_AVERAGE_BITRATE = 2005,      // average bitrate (works with ranges) [ignored, ignored]

    APE_INTERNAL_INFO = 3000                    // for internal use -- don't use (returns APE_FILE_INFO *) [ignored, ignored]
};
 	class IAPEDecompress
{
public:

    // destructor (needed so implementation's destructor will be called)
    virtual ~IAPEDecompress() {}
    
    /*********************************************************************************************
    * Decompress / Seek
    *********************************************************************************************/
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // GetData(...) - gets raw decompressed audio
    // 
    // Parameters:
    //    char * pBuffer
    //        a pointer to a buffer to put the data into
    //    int nBlocks
    //        the number of audio blocks desired (see note at intro about blocks vs. samples)
    //    int * pBlocksRetrieved
    //        the number of blocks actually retrieved (could be less at end of file or on critical failure)
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int GetData(char * pBuffer, int nBlocks, int * pBlocksRetrieved) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Seek(...) - seeks
    // 
    // Parameters:
    //    int nBlockOffset
    //        the block to seek to (see note at intro about blocks vs. samples)
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual int Seek(int nBlockOffset) = 0;

    /*********************************************************************************************
    * Get Information
    *********************************************************************************************/

    //////////////////////////////////////////////////////////////////////////////////////////////
    // GetInfo(...) - get information about the APE file or the state of the decompressor
    // 
    // Parameters:
    //    APE_DECOMPRESS_FIELDS Field
    //        the field we're querying (see APE_DECOMPRESS_FIELDS above for more info)
    //    int nParam1
    //        generic parameter... usage is listed in APE_DECOMPRESS_FIELDS
    //    int nParam2
    //        generic parameter... usage is listed in APE_DECOMPRESS_FIELDS
    //////////////////////////////////////////////////////////////////////////////////////////////
    virtual intptr_t GetInfo(APE_DECOMPRESS_FIELDS Field, intptr_t nParam1 = 0, intptr_t nParam2 = 0) = 0;
};

	extern "C" {
	 IAPEDecompress *  CreateIAPEDecompress(char * pFilename, int * pErrorCode,int flag);
	 IAPEDecompress *  CreateIAPEDecompressFd(int fd,int64_t mOffset,int64_t mLength, int * pErrorCode,int flag);
	}
	#define COMPRESSION_LEVEL_FAST          1000
	#define COMPRESSION_LEVEL_NORMAL        2000
	#define COMPRESSION_LEVEL_HIGH          3000
	#define COMPRESSION_LEVEL_EXTRA_HIGH    4000

#endif
