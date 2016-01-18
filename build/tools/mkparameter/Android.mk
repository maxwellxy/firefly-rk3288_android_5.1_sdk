# Copyright 2008 The Android Open Source Project
#
# Android.mk for mkparameter
#

LOCAL_PATH:= $(call my-dir)

# re-make-parameter host tool
# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mkparameter.c \
	crc.c

LOCAL_CFLAGS += -O2 -Wall -Wno-unused-parameter
LOCAL_MODULE := mkparameter
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_EXECUTABLE)
