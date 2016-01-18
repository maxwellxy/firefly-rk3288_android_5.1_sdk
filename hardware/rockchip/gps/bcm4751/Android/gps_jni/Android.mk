#
# Copyright 2011 Broadcom Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the following terms: 
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
#  Redistributions of source code must retain the above copyright notice, this 
#  list of conditions and the following disclaimer.
#  Redistributions in binary form must reproduce the above copyright notice, 
#  this list of conditions and the following disclaimer in the documentation 
#  and/or other materials provided with the distribution.
#  Neither the name of Broadcom nor the names of its contributors may be used 
#  to endorse or promote products derived from this software without specific 
#  prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM 
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES;LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
# LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Use hardware GPS implementation if available.
#
LOCAL_PATH:= $(call my-dir)

ifeq ($(BOARD_GPS_BCM4751), true)
#CONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS=true
#CONFIG_LCS_ALL_CONSTELLATIONS_IN_USE_MASK=true


ifneq ($(wildcard $(LOCAL_PATH)/../../lcsapi/src/brcm_marshall.c),)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS:=optional

ifneq ($(wildcard hardware/libhardware/include/hardware/gps.h),)
LOCAL_CFLAGS += -DANDROID_23
endif

ifeq ($(CONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS), true)
LOCAL_CFLAGS += -DCONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS
endif

LOCAL_SRC_FILES += ../../lcsapi/src/brcm_marshall.c \
	../../lcsapi/src/gps_api.c \
    ../../lcsapi/src/brcmipc_unixsocket.c \
    ../../lcsapi/src/lbs.c \
    ../../lcsapi/src/supl_api.c \
    ../../lcsapi/src/ril_api.c \
    ../../lcsapi/src/ril_lcsapi.c 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../lcsapi/include 
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := brcmgpslcsapi

include $(BUILD_STATIC_LIBRARY)
else
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_LIBS := brcmgpslcsapi.a
include $(BUILD_MULTI_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS:=eng
LOCAL_SRC_FILES += gps_lcsapi.c 

ifeq ($(CONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS), true)
LOCAL_CFLAGS += -DCONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS
endif

ifeq ($(CONFIG_LCS_ALL_CONSTELLATIONS_IN_USE_MASK), true)
LOCAL_CFLAGS += -DCONFIG_LCS_ALL_CONSTELLATIONS_IN_USE_MASK
endif

ifeq ($(CONFIG_HAL_SUPLLOG), yes)
LOCAL_CFLAGS += -DCONFIG_HAL_SUPLLOG
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../lcsapi/include 

ifneq ($(wildcard hardware/libhardware/include/hardware/gps.h),)
LOCAL_CFLAGS += -DGPS_H_IN_HARDWARE
ifeq ($(TARGET_BOARD_PLATFORM),)
LOCAL_MODULE := gps.default
else
LOCAL_MODULE := gps.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
endif
LOCAL_PRELINK_MODULE := false
else
LOCAL_MODULE := libgps
endif

LOCAL_SHARED_LIBRARIES := liblog 
LOCAL_STATIC_LIBRARIES += brcmgpslcsapi

ifeq ($(TARGET_SIMULATOR),true)
LOCAL_LDFLAGS += -lrt
endif

include $(BUILD_SHARED_LIBRARY)

endif
