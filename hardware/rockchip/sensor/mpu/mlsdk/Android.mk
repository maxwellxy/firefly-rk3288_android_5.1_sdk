# Android.mk for building InvenSense MPL as part of the Android source tree
LOCAL_PATH := $(call my-dir)

MPL_LIB_NAME = libmplmpu
## Use one of these flags if building from a non-customer 
## release version of the kernel driver and MPL
# DEVICE = MPU3050
# DEVICE = MPU6050A2
DEVICE = MPU6050B1

#### MLPLATFORM build ##########################################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmlplatform

#modify these to point to the mpl source installation
MLSDK_ROOT = .
MLPLATFORM_DIR = $(MLSDK_ROOT)/platform/linux

LOCAL_CFLAGS += -D_REENTRANT -DLINUX -DANDROID
LOCAL_CFLAGS += -DCONFIG_MPU_SENSORS_$(DEVICE)
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/platform/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/platform/include/linux
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLPLATFORM_DIR)
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLPLATFORM_DIR)/kernel
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/mllite

LOCAL_SRC_FILES := $(MLPLATFORM_DIR)/mlos_linux.c
LOCAL_SRC_FILES += $(MLPLATFORM_DIR)/mlsl_linux_mpu.c

LOCAL_SHARED_LIBRARIES := liblog libm libutils libcutils
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

#### MLLITE build ##############################################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmllite

#modify these to point to the mpl source installation
MLSDK_ROOT = .
MLPLATFORM_DIR = $(MLSDK_ROOT)/platform
MLLITE_DIR = $(MLSDK_ROOT)/mllite
MPL_DIR = $(MLSDK_ROOT)/mldmp

LOCAL_CFLAGS += -D_REENTRANT -DLINUX -DANDROID -DINV_CACHE_DMP=1
LOCAL_CFLAGS += -DCONFIG_MPU_SENSORS_$(DEVICE)
LOCAL_CFLAGS += -DUNICODE -D_UNICODE -DSK_RELEASE
LOCAL_CFLAGS += -DI2CDEV=\"/dev/mpu\"
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MPL_DIR) 
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLLITE_DIR) 
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLPLATFORM_DIR)/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/mlutils 
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/mlapps/common
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/platform/include/linux
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/mllite/akmd
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLPLATFORM_DIR)/linux

LOCAL_SRC_FILES += $(MLLITE_DIR)/mldl_cfg_mpu.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mldl_cfg_init_linux.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/accel.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/compass.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/compass_supervisor.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/compass_supervisor_adv_callbacks.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/key0_96.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/pressure.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ml.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ml_invobj.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ml_init.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlarray_lite.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlarray_adv.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlarray_legacy.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlBiasNoMotion.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlFIFO.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlFIFOHW.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlMathFunc.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlcontrol.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mldl.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mldmp.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/dmpDefault.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlstates.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlsupervisor.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ml_stored_data.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ustore_manager.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ustore_mlsl_io.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ustore_adv_fusion_delegate.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ustore_lite_fusion_delegate.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/temp_comp_legacy.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mlSetGyroBias.c
LOCAL_SRC_FILES += $(MLSDK_ROOT)/mlutils/checksum.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/ml_mputest.c
LOCAL_SRC_FILES += $(MLSDK_ROOT)/mlutils/mputest.c
LOCAL_SRC_FILES += $(MLLITE_DIR)/mldl_print_cfg.c
#ifneq ($(DEVICE),MPU3050)
#    LOCAL_SRC_FILES += $(MLLITE_DIR)/accel/mpu6050.c
#endif

LOCAL_SHARED_LIBRARIES := libm libutils libcutils liblog libmlplatform
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(MLSDK_ROOT)/mldmp/mpl/android/$(MPL_LIB_NAME).a
include $(BUILD_MULTI_PREBUILT)
### MPLMPU build ##############################################################
# to make an .so from our .a statically build library

#MPL_LIB_NAME = $(MPL_LIB_NAME)

include $(CLEAR_VARS)

LOCAL_MODULE := $(MPL_LIB_NAME)
LOCAL_MODULE_TAGS := optional

MLSDK_ROOT = .

LOCAL_SRC_FILES := $(MLSDK_ROOT)/mldmp/mpl/android/$(MPL_LIB_NAME).a
LOCAL_SHARED_LIBRARIES := libm libutils libcutils liblog libmlplatform libmllite

LOCAL_WHOLE_STATIC_LIBRARIES := $(MPL_LIB_NAME)


LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)



