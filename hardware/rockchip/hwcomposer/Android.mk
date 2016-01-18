#

# rockchip hwcomposer( 2D graphic acceleration unit) .

#

# Copyright (C) 2015 Rockchip Electronics Co., Ltd.
#


LOCAL_PATH := $(call my-dir)
#include $(LOCAL_PATH)/../../Android.mk.def

#
# hwcomposer.default.so
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	rk_hwcomposer.cpp \
	rk_hwc_com.cpp \
	rga_api.cpp \
	rk_hwcomposer_hdmi.cpp \
	hwc_rga.cpp 

ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)),tablet)
LOCAL_SRC_FILES += \
	rk_hwcomposer_blit.cpp
endif

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-Wall \
	-Wextra \
	-DLOG_TAG=\"hwcomposer\"

LOCAL_C_INCLUDES := \
	$(AQROOT)/sdk/inc \
	$(AQROOT)/hal/inc

LOCAL_C_INCLUDES += hardware/rockchip/libgralloc/ump/include

LOCAL_C_INCLUDES += \
	system/core/libion/include \
	system/core/libion/kernel-headers

ifneq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
       LOCAL_C_INCLUDES += hardware/rk29/libgralloc_ump \
       hardware/rk29/libon2
else
       LOCAL_C_INCLUDES += hardware/rockchip/libgralloc \
       hardware/rockchip/librkvpu
       LOCAL_CFLAGS += -DSUPPORT_STEREO
endif

LOCAL_LDFLAGS := \
	-Wl,-z,defs	

LOCAL_SHARED_LIBRARIES := \
	libhardware \
	liblog \
	libui \
	libEGL \
	libcutils \
	libion \
	libhardware_legacy \
	libsync 



#LOCAL_C_INCLUDES := \
#	$(LOCAL_PATH)/inc

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk30xxb)	
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XXB
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3368)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK3368
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)),tablet)
LOCAL_CFLAGS += -DRK3368_MID
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)),box)
LOCAL_CFLAGS += -DRK3368_BOX
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)),phone)
LOCAL_CFLAGS += -DRK3368_PHONE
endif
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3288)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK3288
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)),tablet)
LOCAL_CFLAGS += -DRK3288_MID
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)),box)
LOCAL_CFLAGS += -DRK3288_BOX
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)),phone)
LOCAL_CFLAGS += -DRK3288_PHONE
endif
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM_GPU)),G6110)
        LOCAL_CFLAGS += -DGPU_G6110
endif

ifeq ($(strip $(GRAPHIC_MEMORY_PROVIDER)),dma_buf)
LOCAL_CFLAGS += -DUSE_DMA_BUF
endif

#LOCAL_CFLAGS += -DUSE_LCDC_COMPOSER

ifeq ($(strip $(BOARD_USE_LCDC_COMPOSER)),true)	
LOCAL_CFLAGS += -DUSE_LCDC_COMPOSER
ifeq ($(strip $(BOARD_LCDC_COMPOSER_LANDSCAPE_ONLY)),false)
LOCAL_CFLAGS += -DLCDC_COMPOSER_FULL_ANGLE
endif
endif

LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_HARDWARE)
LOCAL_MODULE_TAGS    := optional
#LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/hw
ifneq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
else
ifneq ($(strip $(TARGET_2ND_ARCH)), )
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_RELATIVE_PATH := hw
endif

LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

