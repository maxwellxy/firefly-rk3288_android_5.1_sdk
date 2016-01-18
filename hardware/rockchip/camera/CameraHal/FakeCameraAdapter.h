#ifndef ANDROID_HARDWARE_CAMERA_FAKE_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_FAKE_HARDWARE_H

//usb camera adapter
#include "CameraHal.h"

#define CAMERAHAL_FAKECAMERA_WIDTH_KEY "sys_graphic.cam_hal.fakewidth"
#define CAMERAHAL_FAKECAMERA_HEIGHT_KEY "sys_graphic.cam_hal.fakeheight"
#define CAMERAHAL_FAKECAMERA_DIR_KEY "sys_graphic.cam_hal.fakedir"
#define CAMERAHAL_FAKECAMERA_DIR_VALUE "/mnt/sdcard/Screencapture/camera"

namespace android{


class CameraFakeAdapter: public CameraAdapter
{
public:
    CameraFakeAdapter(int cameraId);
    virtual ~CameraFakeAdapter();
//    virtual status_t startPreview(int preview_w,int preview_h,int w, int h, int fmt,bool is_capture);
//    virtual status_t stopPreview();
//    virtual int getCurPreviewState(int *drv_w,int *drv_h);

    virtual int setParameters(const CameraParameters &params_set);
    virtual void initDefaultParameters(int camFd);
    virtual int getFrame(FramInfo_s** frame); 
	virtual void dump(int cameraId);
    
private:
    //talk to driver
    virtual int cameraCreate(int cameraId);
    virtual int cameraDestroy();
    virtual int cameraSetSize(int w, int h, int fmt, bool is_capture); 
    virtual int adapterReturnFrame(long index,int cmd);
    virtual int cameraStream(bool on);
    virtual int cameraStart();
    virtual int cameraStop();
    
};

}
#endif

