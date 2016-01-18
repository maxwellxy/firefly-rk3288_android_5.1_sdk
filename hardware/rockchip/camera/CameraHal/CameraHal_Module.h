#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_MODULE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_MODULE_H
#include <linux/videodev2.h>
#include <utils/threads.h>
#include "CameraHal_board_xml_parse.h"

using namespace android;

#define CONFIG_AUTO_DETECT_FRAMERATE    0
#if CONFIG_AUTO_DETECT_FRAMERATE
#define CAMERA_DEFAULT_PREVIEW_FPS_MIN    8000        //8 fps
#define CAMERA_DEFAULT_PREVIEW_FPS_MAX    15000
#endif
#define CAMERAS_SUPPORT_MAX             3
#define CAMERAS_SUPPORTED_SIMUL_MAX     1
#define CAMERA_DEVICE_NAME              "/dev/video"
#define CAMERA_MODULE_NAME              "RK29_ICS_CameraHal_Module"

typedef struct rk_cam_info_s {
    char device_path[30];
    char driver[16];
    unsigned int version;
    struct camera_info facing_info;
    struct v4l2_frmivalenum fival_list[10];   // default preview framerate, dc preview framerate, dv preview framerate(highe quality/low quality)   
    struct rk_cam_total_info *pcam_total_info;
}rk_cam_info_t;


typedef struct rk_camera_device {
    camera_device_t base;   
    int cameraid;
} rk_camera_device_t;

#if CONFIG_AUTO_DETECT_FRAMERATE 
int camera_famerate_detect_loop(void);

class CameraFpsDetectThread : public Thread {        
public:
    CameraFpsDetectThread()
        : Thread(false){ }

    virtual bool threadLoop() {
        camera_famerate_detect_loop();
        return false;
    }
};
#endif

#endif
