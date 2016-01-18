# Copyright 2008 The Android Open Source Project
#
# Android.mk for remkloader
#

LOCAL_PATH:= $(call my-dir)

# re-make-loader host tool
# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := remkloader.c

LOCAL_CFLAGS += -O2 -Wall -Wno-unused-parameter
LOCAL_MODULE := remkloader
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_EXECUTABLE)
