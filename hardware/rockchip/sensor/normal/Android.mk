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

ifeq ($(BOARD_SENSOR_NORMAL),true)

ifneq ($(TARGET_SIMULATOR),true)

# HAL module implemenation, not prelinked, and stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

LOCAL_MODULE := sensors.$(TARGET_BOARD_HARDWARE)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"SensorsHal\" -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_SRC_FILES := 						\
				sensors.c 				\
				nusensors.cpp 			\
				InputEventReader.cpp	\
				SensorBase.cpp			\
				AkmSensor.cpp			\
				MmaSensor.cpp	\
				LightSensor.cpp	\
				ProximitySensor.cpp		\
				
LOCAL_SHARED_LIBRARIES := liblog \
	libcutils \
	libutils

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/akm8975/Android.mk

endif # !TARGET_SIMULATOR
endif 
