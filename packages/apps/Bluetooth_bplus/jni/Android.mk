LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_android_bluetooth_btservice_AdapterService.cpp \
    com_android_bluetooth_hfp.cpp \
    com_android_bluetooth_a2dp.cpp \
    com_android_bluetooth_avrcp.cpp \
    com_android_bluetooth_hid.cpp \
    com_android_bluetooth_hdp.cpp \
    com_android_bluetooth_pan.cpp \
    com_android_bluetooth_gatt.cpp \
    com_broadcom_fm_service.cpp

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE) \

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libcutils \
    libutils \
    liblog \
    libhardware

#LOCAL_CFLAGS += -O0 -g


# Define FM_AUDIO_PATH for configuring FM audio path.
# Valid values includes 0(ROUTE_NONE),1(ROUTE_DAC),2(ROUTE_I2S).
# If the flag is not defined here 1(ROUTE_DAC) will be taken as default.
LOCAL_CFLAGS += -DFM_AUDIO_PATH=1

LOCAL_MODULE := libbluetooth_jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
