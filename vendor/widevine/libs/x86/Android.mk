LOCAL_PATH := $(my-dir)

#########################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES_32 := libwvdrmengine.so

LOCAL_MODULE := libwvdrmengine

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/mediadrm

include $(BUILD_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES_32 := libdrmwvmplugin.so

LOCAL_MODULE := libdrmwvmplugin

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/drm

include $(BUILD_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES_32 := libwvdrm_L3.so

LOCAL_MODULE := libwvdrm_L3

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

include $(BUILD_PREBUILT)

#########################################
include $(CLEAR_VARS)


LOCAL_SRC_FILES_32 := libWVStreamControlAPI_L3.so
                     
LOCAL_MODULE := libWVStreamControlAPI_L3

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

include $(BUILD_PREBUILT)
#########################################
include $(CLEAR_VARS)
  
LOCAL_SRC_FILES_32 := libwvm.so

LOCAL_MODULE := libwvm

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

include $(BUILD_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES_32 := libdrmdecrypt.so

LOCAL_MODULE := libdrmdecrypt

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)

include $(BUILD_PREBUILT)
