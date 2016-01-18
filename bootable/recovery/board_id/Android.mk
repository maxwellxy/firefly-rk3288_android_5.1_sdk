LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := 	\
	board_id_ctrl.c	\
	custom.c		\
	parse_cust_xml.c	\
	parse_device_xml.c	\
	restore.c		
	
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../libxml2/include \
	$(LOCAL_PATH)/..
	
LOCAL_MODULE := libboard_id_recovery

# LOCAL_CFLAGS += -E
include $(BUILD_STATIC_LIBRARY)
