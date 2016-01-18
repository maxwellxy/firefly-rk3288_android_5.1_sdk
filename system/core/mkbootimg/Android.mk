
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := mkbootimg.c
LOCAL_STATIC_LIBRARIES := libmincrypt
LOCAL_CFLAGS := -Werror

LOCAL_MODULE := mkbootimg

ifeq ($(TARGET_BOARD_HARDWARE),rk30board)
LOCAL_CFLAGS := -DTARGET_ROCKCHIP_RECOVERY=true
else
LOCAL_CFLAGS := -DTARGET_ROCKCHIP_RECOVERY=false
endif

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,dist_files,$(LOCAL_BUILT_MODULE))
