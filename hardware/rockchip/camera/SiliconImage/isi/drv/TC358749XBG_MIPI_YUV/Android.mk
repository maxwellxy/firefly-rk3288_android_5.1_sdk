#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	source/TC358749XBG_PARREL.c\
	source/TC358749XBG_tables.c\
	

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include_priv\
	$(LOCAL_PATH)/../../include\
	$(LOCAL_PATH)/../../include_priv\
	$(LOCAL_PATH)/../../../include\
	
	

LOCAL_CFLAGS := -Wall -Wextra -std=c99 -Wno-unused -Wformat-nonliteral -g -O0 -DDEBUG -pedantic
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H
#LOCAL_STATIC_LIBRARIES := libisp_ebase libisp_oslayer libisp_common libisp_hal libisp_cameric_reg_drv libisp_cameric_drv libisp_isi
LOCAL_SHARED_LIBRARIES := libutils libcutils libion libisp_silicomimageisp_api
LOCAL_MODULE:= libisp_isi_drv_TC358749XBG
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS:= optional
include $(BUILD_SHARED_LIBRARY)

