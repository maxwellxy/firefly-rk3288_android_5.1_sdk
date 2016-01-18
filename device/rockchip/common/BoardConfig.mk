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
-include vendor/rockchip/common/BoardConfigVendor.mk

TARGET_NO_KERNEL ?= false
TARGET_PREBUILT_KERNEL ?= kernel/arch/arm/boot/zImage
ifneq ($(TARGET_PRODUCT),rk3188)
 TARGET_PREBUILT_RESOURCE ?= kernel/resource.img
endif

TARGET_BOARD_PLATFORM ?= rk3288
TARGET_BOARD_HARDWARE ?= rk30board

# CPU feature configration
ifeq ($(strip $(TARGET_BOARD_HARDWARE)), rk30board)
TARGET_ARCH ?= arm
TARGET_ARCH_VARIANT ?= armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER ?= true
TARGET_CPU_ABI ?= armeabi-v7a
TARGET_CPU_ABI2 ?= armeabi
TARGET_CPU_VARIANT ?= cortex-a9
TARGET_CPU_SMP ?= true
else
TARGET_ARCH ?= x86
TARGET_ARCH_VARIANT ?= silvermont
TARGET_CPU_ABI ?= x86
TARGET_CPU_ABI2 ?= 
TARGET_CPU_SMP ?= true
endif


# GPU configration
TARGET_BOARD_PLATFORM_GPU ?= mali-t760
BOARD_USE_LCDC_COMPOSER ?= false
GRAPHIC_MEMORY_PROVIDER ?= ump
USE_OPENGL_RENDERER ?= true
TARGET_DISABLE_TRIPLE_BUFFERING ?= false
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK ?= false

DEVICE_HAVE_LIBRKVPU ?= true

ifeq ($(strip $(TARGET_BOARD_PLATFORM_GPU)), mali400)
BOARD_EGL_CFG := vendor/rockchip/common/gpu/Mali400/lib/arm/egl.cfg
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM_GPU)), mali450)
BOARD_EGL_CFG := vendor/rockchip/common/gpu/Mali450/lib/x86/egl.cfg
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM_GPU)), mali-t760)
BOARD_EGL_CFG := vendor/rockchip/common/gpu/MaliT760/etc/egl.cfg
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM_GPU)), PVR540)
BOARD_EGL_CFG ?= vendor/rockchip/common/gpu/PVR540/egl.cfg
endif

TARGET_BOOTLOADER_BOARD_NAME ?= rk30sdk
TARGET_NO_BOOTLOADER ?= true
BOARD_USE_LOW_MEM ?= false
DEVICE_PACKAGE_OVERLAYS += device/rockchip/common/overlay
TARGET_RELEASETOOLS_EXTENSIONS := device/rockchip/common
TARGET_PROVIDES_INIT_RC ?= false
BOARD_HAL_STATIC_LIBRARIES ?= libdumpstate.$(TARGET_PRODUCT) libhealthd.$(TARGET_PRODUCT)

//MAX-SIZE=512M, for generate out/.../system.img
BOARD_SYSTEMIMAGE_PARTITION_SIZE ?= 1073741824
BOARD_FLASH_BLOCK_SIZE ?= 131072

# Enable dex-preoptimization to speed up first boot sequence
ifeq ($(HOST_OS),linux)
  ifeq ($(TARGET_BUILD_VARIANT), user)
    ifeq ($(WITH_DEXPREOPT),)
      WITH_DEXPREOPT ?= true
      WITH_DEXPREOPT_PIC := true
    endif
  endif
endif

ART_USE_HSPACE_COMPACT ?= true

TARGET_USES_LOGD ?= true

# Sepolicy
BOARD_SEPOLICY_DIRS ?= device/rockchip/common/sepolicy
BOARD_SEPOLICY_REPLACE := \
    domain.te
BOARD_SEPOLICY_UNION ?=     \
        akmd.te             \
        app.te              \
        device.te           \
        bluetooth.te        \
        drmserver.te        \
        file.te             \
        file_contexts       \
        genfs_contexts      \
        gpsd.te             \
        init.te             \
        kernel.te           \
        mediaserver.te      \
        netd.te             \
        platform_app.te     \
        recovery.te         \
        rild.te             \
        shell.te            \
        surfaceflinger.te   \
        system_app.te       \
        system_server.te    \
        uncrypt.te          \
        vold.te             \
        apk_logfs.te        \
        crashlogd.te        \
        dhcp.te             \
        fg_conf.te          \
        fmd.te              \
        bootanim.te         \
	 init_shell.te       \
	 install_recovery.te \
	 keystore.te         \
	 lbsd.te             \
	 logconfig.te        \
	 nvm.te              \
	 pekallfmrserver.te  \
	 rpc.te              \
	 service_contexts    \
	 servicemanager.te   \
	 setup_fs_nvm.te     \
	 ueventd.te          \
	 untrusted_app.te    \
	 wpa.te              \
	 zygote.te           \
         rtl_wpa.te          \
         esp_wpa.te          \
         rftest.te


# Recovery
TARGET_NO_RECOVERY ?= false
TARGET_ROCHCHIP_RECOVERY ?= true

# to flip screen in recovery 
BOARD_HAS_FLIPPED_SCREEN ?= false

# Auto update package from USB
RECOVERY_AUTO_USB_UPDATE ?= false

# To use bmp as kernel logo, uncomment the line below to use bgra 8888 in recovery
#TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"
TARGET_ROCKCHIP_PCBATEST ?= false
TARGET_RECOVERY_UI_LIB ?= librecovery_ui_$(TARGET_PRODUCT)
TARGET_USERIMAGES_USE_EXT4 ?= true
TARGET_USERIMAGES_USE_F2FS ?= false
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE ?= ext4
RECOVERY_UPDATEIMG_RSA_CHECK ?= false

RECOVERY_BOARD_ID ?= false
# RECOVERY_BOARD_ID ?= true

# for widevine drm
BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 3

# for drmservice
BUILD_WITH_DRMSERVICE :=true

# for tablet encryption
BUILD_WITH_CRYPTO := false

# Audio
BOARD_USES_GENERIC_AUDIO ?= true

# Wifi&Bluetooth
BOARD_HAVE_BLUETOOTH ?= true
BLUETOOTH_USE_BPLUS ?= false
BOARD_HAVE_BLUETOOTH_BCM ?= false
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR ?= device/rockchip/$(TARGET_PRODUCT)/bluetooth
-include device/rockchip/$(TARGET_PRODUCT)/wifi_bt.mk
include device/rockchip/common/wifi_bt_common.mk

# google apps
BUILD_WITH_GOOGLE_MARKET ?= false
BUILD_WITH_GOOGLE_MARKET_ALL ?= false

# face lock
BUILD_WITH_FACELOCK ?= false

# ebook
BUILD_WITH_RK_EBOOK ?= false

# Sensors
BOARD_SENSOR_ST ?= true
# if use akm8963
#BOARD_SENSOR_COMPASS_AK8963 ?= true
# if need calculation angle between two gsensors
#BOARD_SENSOR_ANGLE ?= true
# if need calibration
#BOARD_SENSOR_CALIBRATION ?= true
# if use mpu
#BOARD_SENSOR_MPU ?= true
#BOARD_USES_GENERIC_INVENSENSE ?= false

# readahead files to improve boot time
# BOARD_BOOT_READAHEAD ?= true

BOARD_BP_AUTO ?= true

# phone pad codec list
BOARD_CODEC_WM8994 ?= false
BOARD_CODEC_RT5625_SPK_FROM_SPKOUT ?= false
BOARD_CODEC_RT5625_SPK_FROM_HPOUT ?= false
BOARD_CODEC_RT3261 ?= false
BOARD_CODEC_RT3224 ?= true
BOARD_CODEC_RT5631 ?= false
BOARD_CODEC_RK616 ?= false

# Vold configrations
# if set to true m-user would be disabled and UMS enabled, if set to disable UMS would be disabled and m-user enabled
BUILD_WITH_UMS ?= true
# if set to true BUILD_WITH_UMS must be false.
BUILD_WITH_CDROM ?= false
BUILD_WITH_CDROM_PATH ?= /system/etc/cd.iso
# multi usb partitions
BUILD_WITH_MULTI_USB_PARTITIONS ?= false
# define tablet support NTFS
BOARD_IS_SUPPORT_NTFS ?= true

# product has GPS or not
BOARD_HAS_GPS ?= false

# ethernet
BOARD_HS_ETHERNET ?= true

# manifest
SYSTEM_WITH_MANIFEST ?= true

# no battery
BUILD_WITHOUT_BATTERY ?= false

BOARD_CHARGER_ENABLE_SUSPEND ?= true
CHARGER_ENABLE_SUSPEND ?= true
CHARGER_DISABLE_INIT_BLANK ?= true
BOARD_CHARGER_DISABLE_INIT_BLANK ?= true

#stress test
BOARD_HAS_STRESSTEST_APP ?= true

#remotecontrol by phone apk
BOARD_HAS_REMOTECONTROL ?= false

#Board use IOMMU
BOARD_WITH_IOMMU ?= true

#boot optimization
BOARD_WITH_BOOT_BOOST ?= false
