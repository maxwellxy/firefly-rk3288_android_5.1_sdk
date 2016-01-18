#include "CameraIspTunning.h"
#include <libexpat/expat.h>
#include "CameraHal_Tracer.h"


namespace android{

CameraIspTunning::CameraIspTunning(){

}

CameraIspTunning::~CameraIspTunning(){
    int nCamTuneTask = mTuneInfoVector.size();
    while(--nCamTuneTask >= 0)
        delete mTuneInfoVector[nCamTuneTask];
}

CameraIspTunning* CameraIspTunning::createInstance()
{
	FILE *fp = NULL;

    CameraIspTunning *profiles = NULL;
    const int BUFF_SIZE = 1024;
    
    fp = fopen(RK_ISP_TUNNING_FILE_PATH, "r");
    if(!fp){
  	    LOGE("%s:open %s failed!!\n",__func__,RK_ISP_TUNNING_FILE_PATH);
        return profiles;
    }
    
    LOGD("open xml file(%s) success\n", RK_ISP_TUNNING_FILE_PATH);

    profiles = new CameraIspTunning();
    
    XML_Parser parser = XML_ParserCreate(NULL);
    if(parser==NULL){
        LOGE("XML_ParserCreate failed\n");
        goto fail;
    }
    
    XML_SetUserData(parser, profiles);
    XML_SetElementHandler(parser, StartElementHandler, NULL);

    for (;;) {
        void *buff = ::XML_GetBuffer(parser, BUFF_SIZE);
        if (buff == NULL) {
            LOGE("failed to in call to XML_GetBuffer()");
            goto fail;
        }

        int bytes_read = ::fread(buff, 1, BUFF_SIZE, fp);
        if (bytes_read < 0) {
            LOGE("failed in call to read");
            goto fail;
        }

        int res = XML_ParseBuffer(parser, bytes_read, bytes_read == 0);
        if(res!=1){
            LOGE("XML_ParseBuffer error or susppend (%d)\n", res);
        }

        if (bytes_read == 0) break;  // done parsing the xml file
    }
    //delete disabled task
    {
        int nCamTuneTask = profiles->mTuneInfoVector.size();
        while(--nCamTuneTask >= 0){
            if(profiles->mTuneInfoVector[nCamTuneTask]->mTuneEnable == false){
                delete profiles->mTuneInfoVector[nCamTuneTask];
                profiles->mTuneInfoVector.removeAt(nCamTuneTask);
                LOGD("%s:delete this %d cap task from queue!",__func__,nCamTuneTask);
            }
        }
        profiles->mTuneTaskcount = profiles->mTuneInfoVector.size();
        if(profiles->mTuneTaskcount <= 0){
            LOGD("%s: WARNING:all tasks are disabled !!",__func__);
            goto  fail;
        }
        profiles->mCurTunIndex    =   0; 
        profiles->mCurTuneTask = profiles->mTuneInfoVector[0];   
    }

    LOGD("%s:task count is %d \n",__func__,profiles->mTuneTaskcount);
    goto exit;
fail:
    if(profiles){
        delete profiles;
        profiles = NULL;
    }
exit:
    if(parser)
        XML_ParserFree(parser);
    fclose(fp);

    return profiles;

}
void CameraIspTunning::StartElementHandler(void *userData, const char *name, const char **atts){
	CameraIspTunning *pCamIspTunning = (CameraIspTunning *) userData;
	ispTuneTaskInfo_s *pCamTuneTaskInfo = pCamIspTunning->mCurTuneTask;
    if(strcmp(name,"Capture")==0){
        ispTuneTaskInfo_s* pNewCaptureTask = (ispTuneTaskInfo_s*)malloc(sizeof(ispTuneTaskInfo_s));
        if(pNewCaptureTask){
            pCamIspTunning->mTuneInfoVector.add(pNewCaptureTask);
            pCamTuneTaskInfo = pCamIspTunning->mCurTuneTask = pNewCaptureTask;
            memset(pCamTuneTaskInfo,0,sizeof(ispTuneTaskInfo_s));
            pCamTuneTaskInfo->mWhiteBalance.whiteBalanceMode = WHITEBALANCE_MODE_INVALID;
            pCamTuneTaskInfo->mExpose.exposuseMode  =   EXPOSUSE_MODE_INVALID;
            //get capture info
            if(strcmp(atts[1],"CamSys_Fmt_Raw_12b") == 0)
                pCamTuneTaskInfo->mTuneFmt = CAMERIC_MI_DATAMODE_RAW12;
            else if(strcmp(atts[1],"CamSys_Fmt_Yuv422_8b") == 0)
                pCamTuneTaskInfo->mTuneFmt = CAMERIC_MI_DATAMODE_YUV422;
            else
                TRACE_E("%s:not suppot this format %s now !",__func__,atts[1]);
            pCamTuneTaskInfo->mTunePicNum = atoi(atts[3]);
            pCamTuneTaskInfo->mTuneEnable = (atoi(atts[5]) == 0 ) ? false:true;

        }else{
            TRACE_E("%s:alloc pNewCaptureTask failed !",__func__);
            return;
        }
    }else if(strcmp(name,"Resolution")==0){
        pCamTuneTaskInfo->mTuneWidth = atoi(atts[1]);
        pCamTuneTaskInfo->mTuneHeight= atoi(atts[3]);
        LOGD("parser:resolution(%dx%d)",pCamTuneTaskInfo->mTuneWidth,pCamTuneTaskInfo->mTuneHeight)

    }else if(strcmp(name,"Exposure")==0){
        if(strcmp(atts[1],"manual") == 0)
            pCamTuneTaskInfo->mExpose.exposuseMode = EXPOSUSE_MODE_MANUAL;
        else if(strcmp(atts[1],"auto") == 0)
            pCamTuneTaskInfo->mExpose.exposuseMode = EXPOSUSE_MODE_AUTO;
        else
            pCamTuneTaskInfo->mExpose.exposuseMode = EXPOSUSE_MODE_INVALID;

    }else if(strcmp(name,"Mec")==0){
        pCamTuneTaskInfo->mExpose.integrationTime = atof(atts[1]) / 1000;
        pCamTuneTaskInfo->mExpose.gain = atof(atts[3]);             
        pCamTuneTaskInfo->mExpose.integrationTimeStep = atof(atts[5]) / 1000;
        pCamTuneTaskInfo->mExpose.gainStep = atof(atts[7]);
        pCamTuneTaskInfo->mExpose.minRaw = atoi(atts[9]);
        pCamTuneTaskInfo->mExpose.maxRaw = atoi(atts[11]);
        pCamTuneTaskInfo->mExpose.threshold = atoi(atts[13]);
        pCamTuneTaskInfo->mExpose.aeRound = (strcmp(atts[15],"true") == 0) ? true:false;
        pCamTuneTaskInfo->mExpose.number = atoi(atts[17]);
    }else if(strcmp(name,"Wdr")==0){
        pCamTuneTaskInfo->mWdrEnable = (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"Cac")==0){
        pCamTuneTaskInfo->mCacEnable= (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"Gamma")==0){
        pCamTuneTaskInfo->mGammarEnable= (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"Lsc")==0){
        pCamTuneTaskInfo->mLscEnable= (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"Dpcc")==0){
        pCamTuneTaskInfo->mDpccEnable= (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"Bls")==0){
        pCamTuneTaskInfo->mBlsEnable= (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"Adpf")==0){
        pCamTuneTaskInfo->mAdpfEnable= (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"Avs")==0){
        pCamTuneTaskInfo->mAvsEnable= (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"Af")==0){
        pCamTuneTaskInfo->mAfEnable= (strcmp(atts[1],"enable") == 0)? true:false;
    }else if(strcmp(name,"WhiteBalance")==0){
        memset(&pCamTuneTaskInfo->mWhiteBalance,0,sizeof(pCamTuneTaskInfo->mWhiteBalance));
        if(strcmp(atts[1],"manual") == 0)
            pCamTuneTaskInfo->mWhiteBalance.whiteBalanceMode = WHITEBALANCE_MODE_MANUAL;
        else if(strcmp(atts[1],"auto") == 0)
            pCamTuneTaskInfo->mWhiteBalance.whiteBalanceMode = WHITEBALANCE_MODE_AUTO;
        else
            pCamTuneTaskInfo->mWhiteBalance.whiteBalanceMode = WHITEBALANCE_MODE_INVALID;
        
    }else if(strcmp(name,"Mwb")==0){
        strncpy(pCamTuneTaskInfo->mWhiteBalance.illumination, atts[1], strlen(atts[1]));
        strncpy(pCamTuneTaskInfo->mWhiteBalance.cc_matrix, atts[3], strlen(atts[3]));
        strncpy(pCamTuneTaskInfo->mWhiteBalance.cc_offset, atts[5], strlen(atts[5]));
        strncpy(pCamTuneTaskInfo->mWhiteBalance.rggb_gain, atts[7], strlen(atts[7]));
    }

}


int CameraIspTunning::ispTuneStoreBufferRAW
(
    ispTuneTaskInfo_s    *pIspTuneTaskInfo,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool              putHeader,
    bool              is16bit
)
{
    int result = 0;
    TRACE_D(1, "%s (enter)\n", __FUNCTION__);

    if (!pIspTuneTaskInfo)
    {
        return -1;
    }

    if (!pFile)
    {
        return -1;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return -1;
    }
    // get base address & size of local plane
    uint32_t RawPlaneSize = pPicBufMetaData->Data.raw.PicWidthBytes * pPicBufMetaData->Data.raw.PicHeightPixel;
    uint8_t *pRawTmp, *pRawBase;
    pRawTmp = pRawBase = (uint8_t *) pIspTuneTaskInfo->y_addr;

    if ( 1)
    {
        // write out raw image; no matter what pSomContext->ForceRGBOut requests

        // write pgm header
        fprintf( pFile,
                "%sP5\n%d %d\n#####<DCT Raw>\n#<Type>%u</Type>\n#<Layout>%u</Layout>\n#<TimeStampUs>%lli</TimeStampUs>\n#####</DCT Raw>\n%d\n",
                putHeader ? "" : "\n", pPicBufMetaData->Data.raw.PicWidthPixel, pPicBufMetaData->Data.raw.PicHeightPixel,
                        pPicBufMetaData->Type, pPicBufMetaData->Layout, pPicBufMetaData->TimeStampUs, is16bit ? 65535 : 255 );

        // write raw plane to file
        if ( (pPicBufMetaData->Data.raw.PicWidthPixel * (is16bit ? 2 : 1)) == pPicBufMetaData->Data.raw.PicWidthBytes )
        {
            // a single write will do
            if ( 1 != fwrite( pRawBase, RawPlaneSize, 1, pFile ) )
            {
                result = -1;
            }
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t y;
            for (y=0; y < pPicBufMetaData->Data.raw.PicHeightPixel; y++)
            {
                if ( 1 != fwrite( pRawTmp, (pPicBufMetaData->Data.raw.PicWidthPixel * (is16bit ? 2 : 1)), 1, pFile ) )
                {
                    result = -1;
                    break; // for loop
                }
                pRawTmp += pPicBufMetaData->Data.raw.PicWidthBytes;
            }
        }
    }

    TRACE_D(1, "%s (exit)\n", __FUNCTION__);

    return result;
}


void CameraIspTunning::ConvertYCbCr444combToRGBcomb
(
    uint8_t     *pYCbCr444,
    uint32_t    PlaneSizePixel
)
{
    uint32_t pix;
    for (pix = 0; pix < PlaneSizePixel; pix++)
    {
        // where to put the RGB data
        uint8_t *pPix = pYCbCr444;

        // get YCbCr pixel data
        int32_t Y  = *pYCbCr444++;
        int32_t Cb = *pYCbCr444++; // TODO: order in marvin output is CrCb and not CbCr as expected
        int32_t Cr = *pYCbCr444++; //       s. above

        // remove offset as in VideoDemystified 3; page 18f; YCbCr to RGB(0..255)
        Y  -=  16;
        Cr -= 128;
        Cb -= 128;

        // convert to RGB
////#define USE_FLOAT
#if (1)
        // Standard Definition TV (BT.601) as in VideoDemystified 3; page 18f; YCbCr to RGB(0..255)
    #ifdef USE_FLOAT
        float R = 1.164*Y + 1.596*Cr;
        float G = 1.164*Y - 0.813*Cr - 0.391*Cb;
        float B = 1.164*Y + 2.018*Cb;
    #else
        int32_t R = ( ((int32_t)(1.164*1024))*Y + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
        int32_t G = ( ((int32_t)(1.164*1024))*Y - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
        int32_t B = ( ((int32_t)(1.164*1024))*Y + ((int32_t)(2.018*1024))*Cb                              ) >> 10;
    #endif
#else
        // High Definition TV (BT.709) as in VideoDemystified 3; page 19; YCbCr to RGB(0..255)
    #ifdef USE_FLOAT
        float R = 1.164*Y + 1.793*Cr;
        float G = 1.164*Y - 0.534*Cr - 0.213*Cb;
        float B = 1.164*Y + 2.115*Cb;
    #else
        int32_t R = ( ((int32_t)(1.164*1024))*Y + ((int32_t)(1.793*1024))*Cr                              ) >> 10;
        int32_t G = ( ((int32_t)(1.164*1024))*Y - ((int32_t)(0.534*1024))*Cr - ((int32_t)(0.213*1024))*Cb ) >> 10;
        int32_t B = ( ((int32_t)(1.164*1024))*Y + ((int32_t)(2.115*1024))*Cb                              ) >> 10;
    #endif
#endif
        // clip
        if (R<0) R=0; else if (R>255) R=255;
        if (G<0) G=0; else if (G>255) G=255;
        if (B<0) B=0; else if (B>255) B=255;

        // write back RGB data
        *pPix++ = (uint8_t) R;
        *pPix++ = (uint8_t) G;
        *pPix++ = (uint8_t) B;
    }
}

int CameraIspTunning::ispTuneStoreBufferYUV422Semi
(
    ispTuneTaskInfo_s    *pIspTuneTaskInfo,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool              putHeader
)
{
    int result = 0;

    TRACE_D(0, "%s (enter)\n", __FUNCTION__);

    if (!pIspTuneTaskInfo)
    {
        return -1;
    }

    if (!pFile)
    {
        return -1;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return -1;
    }

    // get base addresses & sizes of local planes
    uint32_t YCPlaneSize = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;
    uint8_t *pYTmp, *pYBase, *pCbCrTmp, *pCbCrBase;
    pYTmp    = pYBase    = (uint8_t *) pIspTuneTaskInfo->y_addr; // pPicBufMetaData->Data.YCbCr.semiplanar.Y.pBuffer;
    pCbCrTmp = pCbCrBase = (uint8_t *) pIspTuneTaskInfo->uv_addr; // pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.pBuffer;


    // write out raw or RGB image?
    if (!pIspTuneTaskInfo->mForceRGBOut)
    {
        // write pgm header
        fprintf( pFile, "%sP5\n%d %d\n255\n", putHeader ? "" : "\n", pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, 2 * pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel );

        // write luma plane to file
        if ( pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel == pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes )
        {
            // a single write will do
            if ( 1 != fwrite( pYBase, YCPlaneSize, 1, pFile ) )
            {
                result = -1;
            }
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
            {
                if ( 1 != fwrite( pYTmp, pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, 1, pFile ) )
                {
                    result = -1;
                    break; // for loop
                }
                pYTmp += pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes;
            }
        }

        // write combined chroma plane to file
        if ( pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel == pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes )
        {
            // a single write will do
            if ( 1 != fwrite( pCbCrBase, YCPlaneSize, 1, pFile ) )
            {
                result = -1;
            }
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicHeightPixel; y++)
            {
                if ( 1 != fwrite( pCbCrTmp, pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel, 1, pFile ) )
                {
                    result = -1;
                    break; // for loop
                }
                pCbCrTmp += pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
            }
        }
    }
    else
    {
        // we need a temporary helper buffer capable of holding 3 times the YPlane size (upscaled to 4:4:4 by pixel replication)
        uint8_t *pYCbCr444 = (uint8_t *)malloc(3*YCPlaneSize);
        if (pYCbCr444 == NULL)
        {
                result = -1;
        }
        else
        {
            // upscale and combine each 4:2:2 pixel to 4:4:4 while removing any gaps at line ends as well
            uint8_t *pYCbCr444Tmp = pYCbCr444;
            uint32_t x,y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
            {
                // get line starts
                uint8_t *pY = pYTmp;
                uint8_t *pC = pCbCrTmp;

                // walk through line
                for (x=0; x < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel; x+=2)
                {
                    uint8_t Cb, Cr;
                    *pYCbCr444Tmp++ = *pY++;
                    *pYCbCr444Tmp++ = Cb = *pC++;
                    *pYCbCr444Tmp++ = Cr = *pC++;
                    *pYCbCr444Tmp++ = *pY++;
                    *pYCbCr444Tmp++ = Cb;
                    *pYCbCr444Tmp++ = Cr;
                }

                // update line starts
                pYTmp    += pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes;
                pCbCrTmp += pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
            }

            // inplace convert consecutive YCbCr444 to RGB; both are combined color component planes
            ConvertYCbCr444combToRGBcomb( pYCbCr444, YCPlaneSize );

            // write ppm header
            fprintf( pFile, "%sP6\n%d %d\n255\n", putHeader ? "" : "", pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel );

            // finally write result
            if ( 1 != fwrite( pYCbCr444, 3 * YCPlaneSize, 1, pFile ) )
            {
                result = -1;
            }

            // release helper buffer
            free(pYCbCr444);
        }
    }


    TRACE_D(0, "%s (exit)\n", __FUNCTION__);

    return result;
}

int CameraIspTunning::ispTuneStoreBuffer
(
    ispTuneTaskInfo_s    *pIspTuneTaskInfo,
    MediaBuffer_t       *pBuffer,
    char     *szNmae,
    int      index     
)
{
    int result = 0;
    FILE* pStoreFile = NULL;

    char szFileName[FILENAME_MAX] = "";
    char szFileNameRight[FILENAME_MAX] = "";
    uint32_t width  = 0;
    uint32_t height = 0;
    bool   isLeftRight = false;
    bool   isNewFile = false;

    TRACE_D(1, "%s (enter)\n", __FUNCTION__);

    if ( (pIspTuneTaskInfo == NULL) || (pBuffer == NULL) )
    {
        return -1;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return -1;
    }
    
    // get dimensions for filename creation
    switch ( pPicBufMetaData->Type )
    {
        case PIC_BUF_TYPE_RAW8:
            width  = pPicBufMetaData->Data.raw.PicWidthPixel;
            height = pPicBufMetaData->Data.raw.PicHeightPixel;
            break;
        case PIC_BUF_TYPE_RAW16:
            width  = pPicBufMetaData->Data.raw.PicWidthPixel;
            height = pPicBufMetaData->Data.raw.PicHeightPixel;
            break;
        case PIC_BUF_TYPE_YCbCr422:
            switch ( pPicBufMetaData->Layout )
            {
                case PIC_BUF_LAYOUT_COMBINED:
                    width  = pPicBufMetaData->Data.YCbCr.combined.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.combined.PicHeightPixel;
                    break;
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                    width  = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;
                    break;
                case PIC_BUF_LAYOUT_PLANAR:
                    width  = pPicBufMetaData->Data.YCbCr.planar.Y.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.planar.Y.PicHeightPixel;
                    break;
                default:
                result = -1;
            }
            isLeftRight = true;
            break;
        default:
            result = -1;
    }
    if (result != 0)
    {
        return result;
    }


    // open new file?
    if (1)
    {
        isNewFile = true;

        // build filename first:
        // ...get suitable file extension (.raw, .jpg, .yuv, .rgb)
        // ...get data type & layout string
        const char *szFileExt = "";
        bool isData = false;
        bool isJpeg = false;
        const char *szTypeLayout = ""; // not used for DATA or JPEG
        switch ( pPicBufMetaData->Type )
        {
            case PIC_BUF_TYPE_RAW8:
                szFileExt    = ".pgm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
                        szTypeLayout = "_raw8_RGGB";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
                        szTypeLayout = "_raw8_GRBG";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
                        szTypeLayout = "_raw8_GBRG";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
                        szTypeLayout = "_raw8_BGGR";
                        break;
                    case PIC_BUF_LAYOUT_COMBINED:
                        szTypeLayout = "_raw8";
                        break;
                    default:
                        result = -1;
                }
                break;
            case PIC_BUF_TYPE_RAW16:
                szFileExt    = ".pgm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
                        szTypeLayout = "_raw16_RGGB";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
                        szTypeLayout = "_raw16_GRBG";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
                        szTypeLayout = "_raw16_GBRG";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
                        szTypeLayout = "_raw16_BGGR";
                        break;
                    case PIC_BUF_LAYOUT_COMBINED:
                        szTypeLayout = "_raw16";
                        break;
                    default:
                        result = -1;
                }
                break;
            case PIC_BUF_TYPE_YCbCr422:
                szFileExt = pIspTuneTaskInfo->mForceRGBOut ? ".ppm" : ".pgm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_SEMIPLANAR:
                        szTypeLayout = pIspTuneTaskInfo->mForceRGBOut ? "" : "_yuv422_semi";
                        break;
                    default:
                        result = -1;
                }
                break;
            default:
                result = -1;
        }
        if (result != 0)
        {
            return result;
        }

        // ...create image dimension string
        char szDimensions[20] = "";
        if (!isData && !isJpeg) // but neither for DATA nor for JPEG
        {
            snprintf( szDimensions, sizeof(szDimensions)-1, "_%dx%d", width, height );
        }

        // ...create date/time string
        char szDateTime[20] = "";
        if (!isJpeg) // but not for JPEG
        {
                time_t t;
                t = time( NULL );

            // always use creation time of first file for file name in a sequence of files
            strftime( szDateTime, sizeof(szDateTime), "_%Y%m%d_%H%M%S", localtime(&t) );
        }

        // ...create sequence number string
        char szNumber[20] = "";
        snprintf( szNumber, sizeof(szNumber)-1, "_%04d", index);

        // ...combine all parts
        uint32_t combLen;
        if(strstr(szFileExt,"ppm"))
            combLen = strlen(szNmae) + strlen(szDimensions)  + strlen(szTypeLayout)+ strlen(szDateTime)+strlen(pIspTuneTaskInfo->mWhiteBalance.illumination) + strlen(szFileExt);
        else
            combLen = strlen(szNmae) + strlen(szDimensions)  + strlen(szTypeLayout)+ strlen(szDateTime)/*+strlen(szNumber)*/ + strlen(szFileExt);
        
        if ( combLen >= FILENAME_MAX)
        {
            TRACE_E( "%s Max filename length exceeded.\n"
                                  " len(BaseFileName) = %3d\n"
                                  " len(Dimensions)   = %3d\n"
                                  " len(szTypeLayout)   = %3d\n"
                                  " len(szDateTime)   = %3d\n"
                                  " len(FileExt)      = %3d\n"
                                  " --------------------------\n"
                                  " combLen        >= %3d\n",
                                  __FUNCTION__,
                                  strlen(szNmae), strlen(szDimensions),strlen(szTypeLayout),
                                  strlen(szDateTime), strlen(szFileExt), combLen);
            return -1;
        }
        if(strstr(szFileExt,"ppm") && strlen(pIspTuneTaskInfo->mWhiteBalance.illumination))
            snprintf( szFileName, FILENAME_MAX, "%s_%s%s%s%s%s", szNmae, pIspTuneTaskInfo->mWhiteBalance.illumination,szDimensions,szTypeLayout,szDateTime, szFileExt );
        else
            snprintf( szFileName, FILENAME_MAX, "%s%s%s%s%s", szNmae, szDimensions, szTypeLayout,szDateTime, szFileExt );
        szFileName[FILENAME_MAX-1] = '\0';

        // then open file
        pStoreFile = fopen( szFileName, "wb" );
        if (pStoreFile == NULL)
        {
            TRACE_E( "%s Couldn't open file '%s'.\n", __FUNCTION__, szFileName);
            return -1;
        }

    }

    // depending on data format, layout & size call subroutines to:
    // ...get data into local buffer(s)
    // ...write local buffer(s) to file while removing any trailing stuffing from lines where applicable
    // ...averaging pixel data is handled by @ref somCtrlStoreBufferRAW() function internally
    switch (pPicBufMetaData->Type)
    {
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
                case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
                case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
                case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
                case PIC_BUF_LAYOUT_COMBINED:
                {
                    result = ispTuneStoreBufferRAW( pIspTuneTaskInfo, pStoreFile, pBuffer, isNewFile, (pPicBufMetaData->Type == PIC_BUF_TYPE_RAW16) );
                    break;
                }
                default:
                    result = -1;
            }
            break;
        case PIC_BUF_TYPE_YCbCr422:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                {
                    result = ispTuneStoreBufferYUV422Semi( pIspTuneTaskInfo, pStoreFile, pBuffer, isNewFile );
                    break;
                }
                default:
                    result = -1;
            }
            break;
        case PIC_BUF_TYPE_DPCC:
            TRACE_D(0,"DPCC ------------------------------------------------------ \n");
            break;
        default:
            result = -1;
    }
    if(pStoreFile){
        // close file
        fclose( pStoreFile );
    }
    if (result != 0)
    {
        return result;
    }

    TRACE_D(1, "%s (exit)\n", __FUNCTION__);

    return result;
}

int  CameraIspTunning::ispTuneDesiredExp(long raw_ddr,int width,int height,int min_raw,int max_raw,int threshold){
	int max_raw_num = 0,min_raw_num = 0;
	int num,value,result = 0;
	int proportion;
	short unsigned int *p= NULL;
	p = (short unsigned int*)raw_ddr;
	//TRACE_D(0, "%s @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@(enter)\n", __FUNCTION__);

    min_raw <<= 8;
    max_raw <<= 8;
	
	for(num = 0; num < width*height; num++)
		{
			value = *p++;
			if(value > max_raw)
				max_raw_num++;
			if(value < min_raw)
				min_raw_num++;
		}
	
	//TRACE_D(0, "min_raw = %d \n", min_raw);
	//TRACE_D(0, "max_raw = %d \n", max_raw);
	//TRACE_D(0, "threshold = %d \n", threshold);
	//TRACE_D(0, "width*height = %d \n", width*height);

	//TRACE_D(0, "min_raw_num = %.1f \n", min_raw_num);
	//TRACE_D(0, "max_raw_num = %.1f \n", max_raw_num);
	
	proportion = min_raw_num*100/(width*height);
	if(proportion > threshold){
        result |= 0x1;
	}
    TRACE_D(0, "min_raw %d !!!!!!!!!!!\n", proportion);
	proportion = max_raw_num*100/(width*height);
	if(proportion > threshold){
        result |= 0x2;
		//TRACE_D(0, "%.5f ~~~~~~~~~~~\n", proportion);
        
	}
	TRACE_D(0, "max_raw %d ~~~~~~~~~~~\n", proportion);

	//TRACE_D(0, "%s @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@(exit)\n", __FUNCTION__);
    return result;
} 
};
