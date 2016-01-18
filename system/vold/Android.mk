LOCAL_PATH:= $(call my-dir)

common_src_files := \
	VolumeManager.cpp \
	CommandListener.cpp \
	VoldCommand.cpp \
	NetlinkManager.cpp \
	NetlinkHandler.cpp \
	Volume.cpp \
	DirectVolume.cpp \
	Process.cpp \
	Ext4.cpp \
	Ext3.cpp \
	Ext2.cpp \
	Fat.cpp \
	Ntfs.cpp \
	Loop.cpp \
	Devmapper.cpp \
	ResponseCode.cpp \
	CheckBattery.cpp \
	VoldUtil.c \
	fstrim.c \
	cryptfs.c \
	MiscManager.cpp \
	Misc.cpp \
	G3Dev.cpp
	

common_c_includes := \
	system/extras/ext4_utils \
	system/extras/f2fs_utils \
	external/openssl/include \
	external/stlport/stlport \
	bionic \
	external/scrypt/lib/crypto \
	frameworks/native/include \
	system/security/keystore \
	hardware/libhardware/include/hardware \
	system/security/softkeymaster/include/keymaster \
	external/e2fsprogs/lib \
	external/icu4c/common \
	external/icu/icu4c/source/common/

common_shared_libraries := \
	libsparse \
	libsysutils \
	libstlport \
	libbinder \
	libcutils \
	liblog \
	libdiskconfig \
	libhardware_legacy \
	liblogwrap \
	libf2fs_sparseblock \
	libcrypto \
	libselinux \
	libutils \
	libhardware \
	libsoftkeymaster \
	libicuuc 

common_static_libraries := \
	libfs_mgr \
	libscrypt_static \
	libmincrypt \
	libbatteryservice \
	libext4_utils_static \
	libext2_blkid \
	libext2_uuid

include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)), box)
LOCAL_CFLAGS += -DVOLD_BOX
endif


LOCAL_MODULE := libvold

LOCAL_SRC_FILES := $(common_src_files)

LOCAL_C_INCLUDES := $(common_c_includes)

LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)

LOCAL_STATIC_LIBRARIES := $(common_static_libraries)

LOCAL_MODULE_TAGS := eng tests

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE:= vold

LOCAL_SRC_FILES := \
	main.cpp \
	$(common_src_files)

LOCAL_C_INCLUDES := $(common_c_includes)

LOCAL_CFLAGS := -Werror=format
LOCAL_CFLAGS += -DUSE_USB_MODE_SWITCH
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)), box)
LOCAL_CFLAGS += -DVOLD_BOX
endif

ifeq ($(TARGET_HW_DISK_ENCRYPTION),true)
LOCAL_C_INCLUDES += $(TARGET_CRYPTFS_HW_PATH)
common_shared_libraries += libcryptfs_hw
LOCAL_CFLAGS += -DCONFIG_HW_DISK_ENCRYPTION
endif

LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)

LOCAL_STATIC_LIBRARIES := $(common_static_libraries)

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= vdc.c

LOCAL_MODULE:= vdc

LOCAL_C_INCLUDES := 

LOCAL_CFLAGS := 

LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_EXECUTABLE)
