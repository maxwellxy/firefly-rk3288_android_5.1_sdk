#ifndef ANDROID_HARDWARE_COMMONTYPE_H
#define ANDROID_HARDWARE_COMMONTYPE_H
//目前只有CameraAdapter为frame provider，display及event类消费完frame后，可通过该类
//将buffer返回给CameraAdapter,CameraAdapter实现该接口。

//描述帧信息，如width，height，bufaddr，fmt，便于帧消费类收到帧后做后续处理。
//包括zoom的信息
typedef struct FramInfo
{
    ulong_t phy_addr;
    ulong_t vir_addr;
    int frame_width;
    int frame_height;
    ulong_t frame_index;
    int frame_fmt;
    int zoom_value;
    ulong_t used_flag;
    int frame_size;
    void* res;
}FramInfo_s;

typedef int (*func_displayCBForIsp)(void* frameinfo,void* cookie);

#endif
