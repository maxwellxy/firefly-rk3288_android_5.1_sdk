

LOCAL_PATH:= $(call my-dir)

VERSION=3.10

################################
#include $(CLEAR_VARS)

#LOCAL_SRC_FILES:= \
#        libevent/event.c \
#        libevent/hash.c \
#        libevent/event_sig.c \
#        libevent/event_tcp.c

#LOCAL_MODULE:=libevent

#include $(BUILD_STATIC_LIBRARY)


################################
include $(CLEAR_VARS)
LOCAL_CFLAGS:= 

LOCAL_SRC_FILES:= \
        discovery.c \
	if.c \
	common.c \
	debug.c \
	plugin.c

LOCAL_MODULE:=librp-pppoe

include $(BUILD_SHARE_LIBRARY)


#
# pppoe-sniff
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        pppoe-sniff.c \
        if.c \
        common.c \
        debug.c
        
LOCAL_CFLAGS:= \
	-DVERSION="$(VERSION)"
       
LOCAL_C_INCLUDES:=\
        $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := \
        

LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=pppoe-sniff

include $(BUILD_EXECUTABLE)

#
# pppoe-server
#
#include $(CLEAR_VARS)

#LOCAL_SRC_FILES:= \
#        pppoe-server.c \
#        if.c \
#        common.c \
#        debug.c \
#        md5.c 
        
#LOCAL_CFLAGS:= \
#	-DVERSION="$(VERSION)"
        
#LOCAL_C_INCLUDES:=\
#        $(LOCAL_PATH) \
#	$(LOCAL_PATH)/libevent

#LOCAL_SHARED_LIBRARIES := \
#        libevent

#LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
#LOCAL_MODULE_TAGS := eng
#LOCAL_MODULE:=pppoe-server

#include $(BUILD_EXECUTABLE)

#
# pppoe
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        pppoe.c \
        if.c \
        common.c \
        debug.c \
        ppp.c \
        discovery.c
        
LOCAL_CFLAGS:= \
	-DVERSION="$(VERSION)"
        
LOCAL_C_INCLUDES:=\
        $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := \


LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=pppoe

include $(BUILD_EXECUTABLE)


#
# pppoe-relay
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        relay.c \
        if.c \
        common.c \
        debug.c 
        
LOCAL_CFLAGS:= \
	-DVERSION="$(VERSION)"
        
LOCAL_C_INCLUDES:=\
        $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := \


LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=pppoe-repay

include $(BUILD_EXECUTABLE)


