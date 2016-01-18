#include "FakeCameraAdapter.h"

#include <cutils/properties.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkDevice.h>
#include <SkData.h>
#include <SkStream.h>

namespace android{
static unsigned char* mDataSource;

CameraFakeAdapter::CameraFakeAdapter(int cameraId)
                   :CameraAdapter(cameraId)
{
    mCamDriverV4l2MemType = V4L2_MEMORY_OVERLAY;

}

CameraFakeAdapter::~CameraFakeAdapter()
{
    free(mDataSource);
}

int createSourceImage(unsigned char* output, int width, int height)  
{
    int srcW = 240;
    int srcH = 240;

    char value[PROPERTY_VALUE_MAX];
    property_get(CAMERAHAL_FAKECAMERA_WIDTH_KEY, value, "240");
    srcW = atoi(value);
    property_get(CAMERAHAL_FAKECAMERA_HEIGHT_KEY, value, "240");
    srcH = atoi(value);
    property_get(CAMERAHAL_FAKECAMERA_DIR_KEY, value, CAMERAHAL_FAKECAMERA_DIR_VALUE);

    FILE* ifp;
    ifp = fopen(value, "rb");
    if (ifp == NULL) {
        ALOGD("FakeCamera, fail to open source file.");
        return -1;
    }

    int size = width * height * 4;
    unsigned char *RGBBuffer = new unsigned char[size];
    
    fread(RGBBuffer, sizeof(char), size, ifp);

    SkBitmap bitmap;
    //bitmap.setConfig(SkBitmap::kARGB_8888_Config, srcW, srcH, 0);
    bitmap.setPixels(RGBBuffer);

    SkBitmap newbmp;
    //newbmp.setConfig(SkBitmap::kARGB_8888_Config, width, height, 0);
    newbmp.allocPixels();
    newbmp.eraseColor(SK_ColorWHITE);

    SkCanvas* canvas = new SkCanvas(newbmp);
    SkIRect srcR = { 0, 0, SkIntToScalar(srcW), SkIntToScalar(srcH) };
    SkRect  dstR = { SkIntToScalar((width-srcW) / 2), SkIntToScalar((height - srcH) / 2),
                                           SkIntToScalar((width + srcW) / 2), SkIntToScalar(height + srcH) / 2 };

    canvas->drawBitmapRect(bitmap, /*&srcR*/ NULL, dstR, NULL);

    //RGBA8888 -> RGB888
    int r, g, b, a;
    SkAutoLockPixels alp(newbmp);
    void* pixels = newbmp.getPixels();

    for (int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
             memcpy(output + i*width*3 + j*3, pixels+((width)*(i + 1) - j)*4, sizeof(char)*3);
        }
    }

    bitmap.setPixels(NULL);
    newbmp.setPixels(NULL);
    fclose(ifp);

    return 0;
}  

bool RGB2YUV420SPFast(void* RgbBuf, void* yuvBuf, int nWidth, int nHeight)
{

    int i, j; 
	unsigned char*bufY, *bufUV, *bufRGB, *bufYuv; 
	bufY = yuvBuf; 
	bufUV = (unsigned char*)((long)yuvBuf + nWidth * nHeight); 
	unsigned char y, u, v, r, g, b; 
	for (j = 0; j<nHeight; j++)
	{
		bufRGB = RgbBuf + nWidth * (nHeight - 1 - j) * 3 ; 
		for (i = 0;i < nWidth; i++)
		{
			int pos = nWidth * i + j;
			r = *(bufRGB++);
			g = *(bufRGB++);
			b = *(bufRGB++);

			y = (unsigned char)( ( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16  ;           
			u = (unsigned char)( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128 ;           
			v = (unsigned char)( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128 ;
			*(bufY++) = (y<0) ? 0 : ((y>255) ? 255 : y);

            if (j%2 == 1)
            {
                if (i%2 == 0)
                {
                    *(bufUV++) = (u<0) ? 0 : ((u>255) ? 255 : u);
                } else {
                    *(bufUV++) = (v<0) ? 0 : ((v>255) ? 255 : v);
                }
            }

		}

	}

	return true;
}

int CameraFakeAdapter::setParameters(const CameraParameters &params_set)
{
    mParameters = params_set;
    return 0;
}

void CameraFakeAdapter::initDefaultParameters(int camFd)
{
    CameraParameters params;
    String8 parameterString;
    /*preview size setting*/
    parameterString.append("176x144,320x240,352x288,640x480,800x600,1280x720,1920x1080,1600x1200,2592x1944");
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, parameterString.string());
    params.setPreviewSize(640,480);
    /*picture size setting*/      
    params.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, parameterString.string());        
    params.setPictureSize(640,480); 

    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, "yuv420sp,yuv420p");
    params.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,CameraParameters::PIXEL_FORMAT_YUV420SP);
    params.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP);
    params.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,CameraParameters::PIXEL_FORMAT_YUV420SP);

 	params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"0");
    parameterString = CameraParameters::FOCUS_MODE_FIXED;
    params.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
	params.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, parameterString.string());
    
    /*picture format setting*/
    params.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);
    params.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
    /*jpeg quality setting*/
    params.set(CameraParameters::KEY_JPEG_QUALITY, "70");
    /*rotation setting*/
    params.set(CameraParameters::KEY_ROTATION, "0");

    /*lzg@rockchip.com :add some settings to pass cts*/	
    /*focus distance setting ,no much meaning ,only for passing cts */
    parameterString = "0.3,50,Infinity";
    params.set(CameraParameters::KEY_FOCUS_DISTANCES, parameterString.string());
    /*focus length setting ,no much meaning ,only for passing cts */
    parameterString = "35";
    params.set(CameraParameters::KEY_FOCAL_LENGTH, parameterString.string());
    /*horizontal angle of view setting ,no much meaning ,only for passing cts */
    parameterString = "60";
    params.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, parameterString.string());
    /*vertical angle of view setting ,no much meaning ,only for passing cts */
    parameterString = "28.9";
    params.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, parameterString.string());

    /*quality of the EXIF thumbnail in Jpeg picture setting */
    parameterString = "50";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, parameterString.string());
    /*supported size of the EXIF thumbnail in Jpeg picture setting */
    parameterString = "0x0,160x128,160x96";
    params.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, parameterString.string());
    parameterString = "160";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, parameterString.string());
    parameterString = "128";
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, parameterString.string()); 
    /* zyc@rock-chips.com: for cts ,KEY_MAX_NUM_DETECTED_FACES_HW should not be 0 */
    params.set(CameraParameters::KEY_RECORDING_HINT,"false");
    params.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED,"false");
    params.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED,"true");
    params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "false");
    params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "false");
    //params.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
    //params.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,CameraParameters::FOCUS_MODE_FIXED);

    // params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"0");
    params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
    params.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "1");
    params.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "0");
    params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "1");
    params.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, "false");
    params.set(CameraParameters::KEY_SUPPORTED_EFFECTS, "false");
    params.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, "false");
    params.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, "false");

    //for video test
    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "3000,30000");
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(3000,30000)");
    params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, "10,15,30");  
	
    mParameters = params;
    LOGD("%s",__func__);
    
}


int CameraFakeAdapter::getFrame(FramInfo_s** tmpFrame)
{
    long buf_phy, buf_vir;
    int index = -1;
    LOG1("GET ONE FRAME ");
    index = mPreviewBufProvider->getOneAvailableBuffer(&buf_phy,&buf_vir);
    if(index < 0){
        //LOGE("%s:no available buffer found",__FUNCTION__);
        return -1;
    }
 //   mPreviewBufProvider->setBufferStatus(index, 1, (PreviewBufferProvider::CMD_PREVIEWBUF_WRITING));
    // fill frame info:w,h,phy,vir
    mPreviewFrameInfos[index].frame_fmt=  mCamDriverPreviewFmt;
    mPreviewFrameInfos[index].frame_height = mCamPreviewH;
    mPreviewFrameInfos[index].frame_width = mCamPreviewW;
    mPreviewFrameInfos[index].frame_index = index;
    mPreviewFrameInfos[index].phy_addr = mPreviewBufProvider->getBufPhyAddr(index);
    mPreviewFrameInfos[index].vir_addr = (long)mCamDriverV4l2Buffer[index];
    //get zoom_value
    mPreviewFrameInfos[index].zoom_value = 100;
    mPreviewFrameInfos[index].used_flag = 0;
    mPreviewFrameInfos[index].frame_size = 0;

    //memset((void*)buf_vir, 'z', mCamPreviewW*mCamPreviewH/2);
    //memset((void*)buf_vir+mCamPreviewW*mCamPreviewH/2, 'a', mCamPreviewW*mCamPreviewH/2);

    /*
    //FAKE CAMERA TODO:
    //copy image data to preview buffer
    buffer info:
    output frame format: yuv420sp(NV12)
    output frame res   :mCamPreviewW x mCamPreviewH
    outpurt frame buffer: buf_vir
    
    */

    memcpy((void*)buf_vir, mDataSource, mCamPreviewW*mCamPreviewH*3/2);
    usleep(10000);

    *tmpFrame = &(mPreviewFrameInfos[index]);
    mPreviewFrameIndex++;
    return 0;
}

int CameraFakeAdapter::adapterReturnFrame(long index,int cmd)
{
    mCamDriverStreamLock.lock();
    LOG1("RETURN FRAME ");
    if (!mCamDriverStream) {
        LOGD("%s(%d): preview thread is pause, so buffer %d isn't enqueue to camera",__FUNCTION__,__LINE__,index);
        mCamDriverStreamLock.unlock();
        return 0;
    }
	mPreviewBufProvider->setBufferStatus(index,0, cmd); 
    mCamDriverStreamLock.unlock();
    return 0;
}


int CameraFakeAdapter::cameraStream(bool on)
{
    mCamDriverStreamLock.lock();
    mCamDriverStream = on;

	mCamDriverStreamLock.unlock();
    return 0;
}

int CameraFakeAdapter::cameraStart()
{
    int buffer_count;
    int previewBufStatus = ((PreviewBufferProvider::CMD_PREVIEWBUF_WRITING) | (PreviewBufferProvider::CMD_PREVIEWBUF_DISPING)
                            |(PreviewBufferProvider::CMD_PREVIEWBUF_VIDEO_ENCING) |(PreviewBufferProvider::CMD_PREVIEWBUF_SNAPSHOT_ENCING)
                            | (PreviewBufferProvider::CMD_PREVIEWBUF_DATACB));
    buffer_count = mPreviewBufProvider->getBufCount();
    for (int i = 0; i < buffer_count; i++) {
        mCamDriverV4l2Buffer[i] = (char*)mPreviewBufProvider->getBufVirAddr(i);
        mPreviewBufProvider->setBufferStatus(i, 0,previewBufStatus);
    }
    
    mPreviewErrorFrameCount = 0;
    mPreviewFrameIndex = 0;
    cameraStream(true);

    unsigned char *RGBBuf = (unsigned char*)malloc(mCamPreviewW*mCamPreviewH*3);
    mDataSource = (unsigned char*)malloc(mCamPreviewW*mCamPreviewH*3 / 2);
    createSourceImage(RGBBuf, mCamPreviewW, mCamPreviewH);
    RGB2YUV420SPFast(RGBBuf, mDataSource, mCamPreviewW, mCamPreviewH);
    free(RGBBuf);

    return 0;
}

int CameraFakeAdapter::cameraSetSize(int w, int h, int fmt, bool is_capture)
{
    return 0;
}
int CameraFakeAdapter::cameraStop()
{
    return 0;
}


void CameraFakeAdapter::dump(int cameraId)
{
    return;
}

int CameraFakeAdapter::cameraCreate(int cameraId)
{
    mCamDriverPreviewFmt = V4L2_PIX_FMT_NV12;
    return 0;
}

int CameraFakeAdapter::cameraDestroy()
{
    return 0;
}

}
