ifeq ($(strip $(BLUETOOTH_USE_BPLUS)),true)
LOCAL_PATH := $(call my-dir)

# Setup bdroid local make variables for handling configuration
ifneq ($(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR),)
  bdroid_C_INCLUDES := $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
  bdroid_CFLAGS := -DHAS_BDROID_BUILDCFG
else
  bdroid_C_INCLUDES :=
  bdroid_CFLAGS := -DHAS_NO_BDROID_BUILDCFG
endif

BLUEDROID_EXTRA_BPLUS_STATIC_LINKING := true
BPLUS_FM_INCLUDED:= true
#BLUDROID_INTERNAL:=true
include $(call all-subdir-makefiles)

# Cleanup our locals
bdroid_C_INCLUDES :=
bdroid_CFLaGS :=
endif
