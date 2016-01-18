LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  		RkVpuDecoder.cpp \

LOCAL_MODULE := libstagefright_rkvpudec

LOCAL_C_INCLUDES := \
 	$(TOP)/frameworks/av/media/libstagefright/include \
  	$(TOP)/frameworks/native/include/media/openmax \
    $(TOP)/hardware/rockchip/librkvpu \
  
LOCAL_CFLAGS := -DOSCL_IMPORT_REF= -DOSCL_UNUSED_ARG= -DOSCL_EXPORT_REF=

include $(BUILD_STATIC_LIBRARY)
