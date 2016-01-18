# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.






LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_GPS_RK), true)

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS:=optional
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE :=hvgps
LOCAL_SRC_FILES := $(LOCAL_PATH)/lib/hvgps.a
LOCAL_PREBUILT_LIBS := lib/hvgps.a
include $(BUILD_MULTI_PREBUILT)

 
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS:=optional
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := gps.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_SRC_FILES := gpshal.cpp hvgps.cpp

LOCAL_CFLAGS    := -Wall -fvisibility=default -fsigned-char
#-fvisibility=hidden 
LOCAL_SHARED_LIBRARIES := liblog libc libcutils libm 
#LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -lc -lm -llog -lpthread 
#libcutils
LOCAL_WHOLE_STATIC_LIBRARIES := hvgps
#LOCAL_LDFLAGS += -lhvgps
include $(BUILD_SHARED_LIBRARY)

endif
