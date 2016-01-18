LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := vpu_mem_pool.c vpu_dma_buf.c
LOCAL_SHARED_LIBRARIES := libutils libion 
LOCAL_MODULE := libvpu_mem_pool
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS:= -DLOG_TAG=\"VPU_MEM_POOL\"
#include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

#
# vpu_mem_pool_test
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vpu_mem_observer.c

LOCAL_SHARED_LIBRARIES := \
	libcutils

LOCAL_MODULE := vpu_mem_observer
LOCAL_MODULE_TAGS := optional tests
LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS += -DLOG_TAG=\"VPU_MEM_OBSERVER\"

LOCAL_C_INCLUDES := 

include $(BUILD_EXECUTABLE)

