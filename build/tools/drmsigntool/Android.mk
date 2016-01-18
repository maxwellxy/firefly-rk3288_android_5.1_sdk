#==========================================================
# Android.mk for drmsigntool
#
# sign the boot.img and recovery.img for drm secure boot
# =========================================================

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_EXECUTABLES := drmsigntool
include $(BUILD_HOST_PREBUILT)
