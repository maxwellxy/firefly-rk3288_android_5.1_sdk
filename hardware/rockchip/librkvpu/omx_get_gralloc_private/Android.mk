LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := gralloc_priv_omx.cpp
LOCAL_SHARED_LIBRARIES := liblog libutils 
LOCAL_MODULE := libgralloc_priv_omx
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(TOP)/hardware/rockchip/libgralloc \
	$(TOP)/hardware/libhardware/include 

ifeq ($(strip $(GRAPHIC_MEMORY_PROVIDER)),dma_buf)
	LOCAL_CFLAGS += -DUSE_DMA_BUF
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM_GPU)),G6110)
	LOCAL_CFLAGS += -DGPU_G6110
endif

include $(BUILD_SHARED_LIBRARY)


