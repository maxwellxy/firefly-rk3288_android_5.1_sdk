# Copyright (C) 2007 The Android Open Source Project
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


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    test_lib_radical_update.c \

LOCAL_MODULE := test_lib_radical_update

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_STATIC_LIBRARIES := \
	libext4_utils_static \
    libsparse_static \
    libminzip \
    libz \
    libmtdutils \
    libmincrypt \
    libminadbd \
    libminui \
    libpng \
    libfs_mgr \
    libcutils \
    liblog \
    libselinux \
    libstdc++ \
    libm \
    libc \
    libedify \
    libapplypatch \
    librsa \
    libcrc32 \
    librk_emmcutils  

LOCAL_STATIC_LIBRARIES += libradical_update_recovery libxml2_recovery

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc \
	$(LOCAL_PATH)/../.. 

LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)

