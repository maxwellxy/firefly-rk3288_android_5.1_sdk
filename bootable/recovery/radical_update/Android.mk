LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := 	\
	radical_update.c	\
	
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/../libxml2/include \
	$(LOCAL_PATH)/..
	
LOCAL_MODULE := libradical_update_recovery

# LOCAL_CFLAGS += -E
include $(BUILD_STATIC_LIBRARY)

# -------- # 

include $(call all-makefiles-under,$(LOCAL_PATH))

