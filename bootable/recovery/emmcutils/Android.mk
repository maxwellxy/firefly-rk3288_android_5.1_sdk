LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	rk_emmcutils.c

LOCAL_MODULE := librk_emmcutils
LOCAL_STATIC_LIBRARIES = libcutils

include $(BUILD_STATIC_LIBRARY)
