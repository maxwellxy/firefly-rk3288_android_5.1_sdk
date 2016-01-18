ifneq ($(TARGET_SIMULATOR),true)

#ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
NO_CODEC_TEST_BIN=true
else
NO_CODEC_TEST_BIN=false
endif
ifneq ($(NO_CODEC_TEST_BIN), true)
include $(CLEAR_VARS)
LOCAL_MODULE := codec_test
LOCAL_FORCE_STATIC_EXECUTABLE := true
#LOCAL_MODULE_PATH:=$(TARGET_ROOT_OUT_SBIN)
LOCAL_C_INCLUDES += bionic external/stlport/stlport $(LOCAL_PATH)/Language external/libpng/

LOCAL_SRC_FILES := \
    alsa_pcm.c     \
    alsa_mixer.c   \
    codec_test.c   \
    codec_main.c

LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libc libcutils liblog
LOCAL_SHARED_LIBRARIES := 


include $(BUILD_EXECUTABLE)
endif

include $(CLEAR_VARS)

commands_recovery_local_path := $(LOCAL_PATH)

TARGET_RECOVERY_GUI := true
BOARD_HAS_NO_REAL_SDCARD := true
TW_INTERNAL_STORAGE_PATH := "/mnt/sdcard"
TW_INTERNAL_STORAGE_MOUNT_POINT := "/mnt/sdcard"
LOCAL_MODULE := pcba_core
LOCAL_FORCE_STATIC_EXECUTABLE := true
#LOCAL_MODULE_PATH:=$(TARGET_ROOT_OUT_SBIN)
LOCAL_C_INCLUDES += bionic external/stlport/stlport $(LOCAL_PATH)/Language

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk3368)
LOCAL_CFLAGS += -DRK3368_PCBA
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
LOCAL_CFLAGS += -DSOFIA3GR_PCBA -DDB_DEBUG

endif

ifeq ($(strip $(BOARD_SENSOR_MPU)), true)
LOCAL_CFLAGS += -DSOFIA3GR_SENSOR_MPU

endif

ifeq ($(strip $(AUD_WITHOUT_EARPIECE)), true)
LOCAL_CFLAGS += -DSOFIA3GR_AUD_WITHOUT_EARPIEC

endif

LOCAL_SRC_FILES := \
    pre_test.c\
    ui.c \
    extra-functions.c \
    ddftw.c \
    themes.c \
    default_recovery_ui.c \
    reboot.c			\
    data.cpp			\
    script.c			\
    script_parser.c		\
    screen_test.c		\
    rtc_test.c			\
    key_test.c			\
    wlan_test.c			\
    bt_test.c			\
    sdcard_test.c		\
    udisk_test.c        \
    gsensor_test.c 		\
    gsensor_test_mpu.c 		\
    hdmi_test.c       \
    sim_test.c \
    battery_test.c\
	ddr_test.c \
	cpu_test.c	\
	lightsensor_test.c \
	gnss_test.c \
	psensor_test.c \
	fm_test.c \
	compass_test.c \
	vibrator.c \
	flashlight_test.c \
	lan_test.c \
	nand_test.c

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk3288)
	LOCAL_CFLAGS += -DRK3288_PCBA
	LOCAL_SRC_FILES += 	rk32_codec/alsa_mixer.c \
						rk32_codec/alsa_pcm.c \
						rk32_codec/alsa_route.c \
						rk32_codec/codec_test.c 
						#rk3288-camera/camera_test.c  				
else
	ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk312x)
	LOCAL_CFLAGS += -DRK312X_PCBA
	LOCAL_SRC_FILES += alsa_mixer.c alsa_pcm.c codec_test.c rk312x-camera/camera_test.c
 	else
 		ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
 			LOCAL_SRC_FILES += 	sofia_codec/alsa_mixer.c \
 						sofia_codec/alsa_pcm.c \
                        sofia_codec/alsa_route.c \
						sofia_codec/codec_test.c \
						sofia_camera/camera_test.c \
						at_util_extern.c
 		else
 			LOCAL_SRC_FILES += alsa_mixer.c alsa_pcm.c codec_test.c camera_test.c
 		endif
 	endif
endif




RECOVERY_API_VERSION := 2
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

ifeq ($(BOARD_HAS_NO_REAL_SDCARD), true)
    LOCAL_CFLAGS += -DBOARD_HAS_NO_REAL_SDCARD
endif

ifneq ($(SP1_NAME),)
	LOCAL_CFLAGS += -DSP1_NAME=$(SP1_NAME) -DSP1_BACKUP_METHOD=$(SP1_BACKUP_METHOD) -DSP1_MOUNTABLE=$(SP1_MOUNTABLE)
endif
ifneq ($(SP1_DISPLAY_NAME),)
	LOCAL_CFLAGS += -DSP1_DISPLAY_NAME=$(SP1_DISPLAY_NAME)
endif
ifneq ($(SP2_NAME),)
	LOCAL_CFLAGS += -DSP2_NAME=$(SP2_NAME) -DSP2_BACKUP_METHOD=$(SP2_BACKUP_METHOD) -DSP2_MOUNTABLE=$(SP2_MOUNTABLE)
endif
ifneq ($(SP2_DISPLAY_NAME),)
	LOCAL_CFLAGS += -DSP2_DISPLAY_NAME=$(SP2_DISPLAY_NAME)
endif
ifneq ($(SP3_NAME),)
	LOCAL_CFLAGS += -DSP3_NAME=$(SP3_NAME) -DSP3_BACKUP_METHOD=$(SP3_BACKUP_METHOD) -DSP3_MOUNTABLE=$(SP3_MOUNTABLE)
endif
ifneq ($(SP3_DISPLAY_NAME),)
	LOCAL_CFLAGS += -DSP3_DISPLAY_NAME=$(SP3_DISPLAY_NAME)
endif
ifneq ($(RECOVERY_SDCARD_ON_DATA),)
	LOCAL_CFLAGS += -DRECOVERY_SDCARD_ON_DATA
endif
ifneq ($(TW_INCLUDE_DUMLOCK),)
	LOCAL_CFLAGS += -DTW_INCLUDE_DUMLOCK
endif
ifneq ($(TW_INTERNAL_STORAGE_PATH),)
	LOCAL_CFLAGS += -DTW_INTERNAL_STORAGE_PATH=$(TW_INTERNAL_STORAGE_PATH)
endif
ifneq ($(TW_INTERNAL_STORAGE_MOUNT_POINT),)
	LOCAL_CFLAGS += -DTW_INTERNAL_STORAGE_MOUNT_POINT=$(TW_INTERNAL_STORAGE_MOUNT_POINT)
endif
ifneq ($(TW_EXTERNAL_STORAGE_PATH),)
	LOCAL_CFLAGS += -DTW_EXTERNAL_STORAGE_PATH=$(TW_EXTERNAL_STORAGE_PATH)
endif
ifneq ($(TW_EXTERNAL_STORAGE_MOUNT_POINT),)
	LOCAL_CFLAGS += -DTW_EXTERNAL_STORAGE_MOUNT_POINT=$(TW_EXTERNAL_STORAGE_MOUNT_POINT)
endif
ifeq ($(TW_HAS_NO_RECOVERY_PARTITION), true)
    LOCAL_CFLAGS += -DTW_HAS_NO_RECOVERY_PARTITION
endif
ifeq ($(TW_NO_REBOOT_BOOTLOADER), true)
    LOCAL_CFLAGS += -DTW_NO_REBOOT_BOOTLOADER
endif
ifeq ($(TW_NO_REBOOT_RECOVERY), true)
    LOCAL_CFLAGS += -DTW_NO_REBOOT_RECOVERY
endif
ifeq ($(TW_NO_BATT_PERCENT), true)
    LOCAL_CFLAGS += -DTW_NO_BATT_PERCENT
endif
ifneq ($(TW_CUSTOM_POWER_BUTTON),)
	LOCAL_CFLAGS += -DTW_CUSTOM_POWER_BUTTON=$(TW_CUSTOM_POWER_BUTTON)
endif
ifeq ($(TW_ALWAYS_RMRF), true)
    LOCAL_CFLAGS += -DTW_ALWAYS_RMRF
endif
ifeq ($(TW_NEVER_UNMOUNT_SYSTEM), true)
    LOCAL_CFLAGS += -DTW_NEVER_UNMOUNT_SYSTEM
endif
ifeq ($(TW_NO_USB_STORAGE), true)
    LOCAL_CFLAGS += -DTW_NO_USB_STORAGE
endif
ifeq ($(TW_INCLUDE_INJECTTWRP), true)
    LOCAL_CFLAGS += -DTW_INCLUDE_INJECTTWRP
endif
ifeq ($(TW_INCLUDE_BLOBPACK), true)
    LOCAL_CFLAGS += -DTW_INCLUDE_BLOBPACK
endif
ifeq ($(TW_DEFAULT_EXTERNAL_STORAGE), true)
    LOCAL_CFLAGS += -DTW_DEFAULT_EXTERNAL_STORAGE
endif
ifneq ($(TARGET_USE_CUSTOM_LUN_FILE_PATH),)
    LOCAL_CFLAGS += -DCUSTOM_LUN_FILE=\"$(TARGET_USE_CUSTOM_LUN_FILE_PATH)\"
endif
ifneq ($(BOARD_UMS_LUNFILE),)
    LOCAL_CFLAGS += -DCUSTOM_LUN_FILE=\"$(BOARD_UMS_LUNFILE)\"
endif
#ifeq ($(TW_FLASH_FROM_STORAGE), true) Making this the default behavior
    LOCAL_CFLAGS += -DTW_FLASH_FROM_STORAGE
#endif
ifeq ($(TW_HAS_DOWNLOAD_MODE), true)
    LOCAL_CFLAGS += -DTW_HAS_DOWNLOAD_MODE
endif
ifeq ($(TW_SDEXT_NO_EXT4), true)
    LOCAL_CFLAGS += -DTW_SDEXT_NO_EXT4
endif
ifeq ($(TW_FORCE_CPUINFO_FOR_DEVICE_ID), true)
    LOCAL_CFLAGS += -DTW_FORCE_CPUINFO_FOR_DEVICE_ID
endif

# This binary is in the recovery ramdisk, which is otherwise a copy of root.
# It gets copied there in config/Makefile.  LOCAL_MODULE_TAGS suppresses
# a (redundant) copy of the binary in /system/bin for user builds.
# TODO: Build the ramdisk image in a more principled way.
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := 

ifeq ($(TARGET_RECOVERY_GUI),true)
  LOCAL_STATIC_LIBRARIES += libtwgui
else
  LOCAL_SRC_FILES += gui_stub.c
endif
LOCAL_STATIC_LIBRARIES += libm
LOCAL_STATIC_LIBRARIES += libminziptwrp libunz libmincrypt
LOCAL_STATIC_LIBRARIES += libminuitwrp libpixelflinger_static libpng libjpegtwrp libbluetooth
LOCAL_STATIC_LIBRARIES += libz libc libstlport_static libcutils libstdc++
LOCAL_STATIC_LIBRARIES += libmtdutils liblog



include $(BUILD_EXECUTABLE)

#added for CM busybox
#BUSYBOX_LINKS := $(shell cat external/busybox/busybox-minimal.links)
#exclude := tune2fs mke2fs
#RECOVERY_BUSYBOX_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(filter-out $(exclude),$(notdir $(BUSYBOX_LINKS))))
#$(RECOVERY_BUSYBOX_SYMLINKS): BUSYBOX_BINARY := busybox
#$(RECOVERY_BUSYBOX_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
#@echo "Symlink: $@ -> $(BUSYBOX_BINARY)"
#@mkdir -p $(dir $@)
#@rm -rf $@
#$(hide) ln -sf $(BUSYBOX_BINARY) $@
#$(shell cp -rf $(commands_recovery_local_path)/res/* $(TARGET_ROOT_OUT_SBIN)/)
#ALL_DEFAULT_INSTALLED_MODULES += $(RECOVERY_BUSYBOX_SYMLINKS) 



include $(commands_recovery_local_path)/minuitwrp/Android.mk
include $(commands_recovery_local_path)/gui/Android.mk
include $(commands_recovery_local_path)/libjpegtwrp/Android.mk
include $(commands_recovery_local_path)/minziptwrp/Android.mk
include $(commands_recovery_local_path)/libbluetooth/Android.mk
commands_recovery_local_path :=

#endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR
