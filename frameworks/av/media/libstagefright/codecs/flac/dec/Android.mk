LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	FLACDecoder.cpp \

ifeq ($(TARGET_ARCH),arm)
LOCAL_SRC_FILES += \

else
LOCAL_SRC_FILES += \

endif

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
        $(LOCAL_PATH)/src \
        $(LOCAL_PATH)/include \
        $(TOP)/external/flac/include

#LOCAL_STATIC_LIBRARIES := \
#	libFLAC;
				
LOCAL_CFLAGS := \
        -DOSCL_UNUSED_ARG=

LOCAL_MODULE := libstagefright_flacdec

include $(BUILD_STATIC_LIBRARY)

