LOCAL_PATH := $(call my-dir)

#ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)),\
#      $(filter $(BOARD_CONNECTIVITY_MODULE), rtl8723au rtl8723bu rtl8761auv))

include $(CLEAR_VARS)

BDROID_DIR := $(TOP_DIR)external/bluetooth/bluedroid

LOCAL_SRC_FILES := \
        src/bt_vendor_rtk.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include

LOCAL_SHARED_LIBRARIES := \
        libcutils

LOCAL_MODULE := libbt-vendor-rtl8723bu
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := realtek

ifneq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
 LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)
else
ifneq ($(strip $(TARGET_2ND_ARCH)), )
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_RELATIVE_PATH :=
endif

#include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

#endif # BOARD_CONNECTIVITY_MODULE
