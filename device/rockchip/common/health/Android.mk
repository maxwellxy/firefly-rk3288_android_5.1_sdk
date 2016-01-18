# Copyright (C) 2014 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := system/core/healthd
ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
LOCAL_SRC_FILES := \
    healthd-rockchip.cpp
else
LOCAL_CFLAGS += -DFEATURE_SAVE_CAPACITY
LOCAL_SRC_FILES := \
    healthd-rockchip.cpp \
    bat_cap.cpp
endif

LOCAL_MODULE := libhealthd.$(TARGET_PRODUCT)

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
