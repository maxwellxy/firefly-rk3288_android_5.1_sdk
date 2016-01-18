#
# Copyright 2014 The Android Open-Source Project
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
#

# Use the non-open-source parts, if they're present
-include vendor/rockchip/rk3288/BoardConfigVendor.mk
-include device/rockchip/common/BoardConfig.mk

TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a15
TARGET_CPU_SMP := true
PRODUCT_PACKAGE_OVERLAYS += device/rockchip/rk3288/overlay

TARGET_BOARD_PLATFORM := rk3288
TARGET_BOARD_PLATFORM_GPU := mali-t760
#TARGET_BOARD_INFO_FILE := device/rockchip/rk3288/board-info.txt

#TARGET_BUILD_JAVA_SUPPORT_LEVEL := platform

# Include an expanded selection of fonts
EXTENDED_FONT_FOOTPRINT := true

MALLOC_IMPL := dlmalloc

# Sensors
BOARD_SENSOR_ST := false
BOARD_SENSOR_MPU := true
BOARD_USES_GENERIC_INVENSENSE := false

# ------------ #
# radical_update
# $(info to set RECOVERY_WITH_RADICAL_UPDATE to true)
RECOVERY_WITH_RADICAL_UPDATE := true

# cert to verify radical_update_pkg.
RADICAL_UPDATE_CERT := $(TARGET_RELEASETOOLS_EXTENSIONS)/radical_update/certs/radical_update.x509.pem
# ------------ #

# Copy RK3288 own init.rc file
TARGET_PROVIDES_INIT_RC := true

ifneq ($(filter %box, $(TARGET_PRODUCT)), )
TARGET_BOARD_PLATFORM_PRODUCT ?= box
else
TARGET_BOARD_PLATFORM_PRODUCT ?= tablet
endif
#######for target product ########
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)), box)
DEVICE_PACKAGE_OVERLAYS += device/rockchip/rk3288/overlay_screenoff

PRODUCT_PROPERTY_OVERRIDES += \
        ro.target.product=box
else
  PRODUCT_PROPERTY_OVERRIDES += \
        ro.target.product=tablet
endif

