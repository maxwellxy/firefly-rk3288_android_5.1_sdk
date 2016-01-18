#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)
include $(call all-subdir-makefiles)

include $(CLEAR_VARS)


LOCAL_SRC_FILES:=\
	CameraHalUtil.cpp\
	MessageQueue.cpp\
	Semaphore.cpp\
	CameraHal_Module.cpp\
	CameraHal_Mem.cpp\
	CameraBuffer.cpp\
	AppMsgNotifier.cpp\
	DisplayAdapter.cpp\
	CameraAdapter.cpp\
	CameraSocAdapter.cpp\
	CameraUSBAdapter.cpp\
	CameraIspAdapter.cpp\
	CameraIspSOCAdapter.cpp\
	FakeCameraAdapter.cpp\
	CameraHal.cpp\
	CameraHal_board_xml_parse.cpp\
	CameraHal_Tracer.c\
	CameraIspTunning.cpp \
	SensorListener.cpp
	
  
ifeq ($(strip $(TARGET_BOARD_HARDWARE)),rk30board)	 
LOCAL_C_INCLUDES += \
	frameworks/base/include/ui \
  external/jpeg \
  external/jhead\
  hardware/rockchip/hwcomposer\
	hardware/rockchip/libgralloc_ump/ump/include\
	hardware/rockchip/librkvpu\
  $(LOCAL_PATH)/../SiliconImage/include\
  $(LOCAL_PATH)/../SiliconImage/include/isp_cam_api\
  bionic\
  external/stlport/stlport\
  external/tinyxml2\
  system/media/camera/include\
  system/core/libion/kernel-headers/linux\
  system/core/libion/include/ion

LOCAL_C_INCLUDES += \
    external/skia/include/core \
    external/skia/include/effects \
    external/skia/include/images \
    external/skia/src/ports \
    external/skia/include/utils

LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libcamera_client \
    libgui\
    libjpeg\
    libjpeghwenc\
	libion\
    libvpu\
    libdl\
	libisp_silicomimageisp_api \
	libstlport\
	libexpat \
	libskia \

#LOCAL_STATIC_LIBRARIES :=  libisp_calibdb libtinyxml2 libisp_cam_calibdb libisp_ebase \
#							libisp_oslayer libisp_common libisp_hal libisp_isi\
#							libisp_cam_engine  libisp_version libisp_cameric_reg_drv  \

#LOCAL_PREBUILT_LIBS := libisp_silicomimageisp_api.so
endif
ifeq ($(strip $(TARGET_BOARD_HARDWARE)),rk2928board)
LOCAL_C_INCLUDES += \
    frameworks/base/include/ui \
  external/jpeg \
  external/jhead\
  hardware/rockchip/hwcomposer_rga\
  hardware/rockchip/librkvpu\
  hardware/rockchip/libgralloc_ump/ump/include

LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libcamera_client \
    libgui\
    libjpeg\
    libjpeghwenc\
    libyuvtorgb\
    libion\
    libvpu\
    libdl
    

endif
ifeq ($(strip $(TARGET_BOARD_HARDWARE)),rk29board)    
LOCAL_C_INCLUDES += \
	#frameworks/base/include/ui \
  frameworks/native/include/media/hardware \
  frameworks/native/include/media/openmax \
  external/jpeg \
  external/jhead	
  
LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libcamera_client \
    libgui\
    libjpeghwenc\
    libjpeg\
    libyuvtorgb
endif

LOCAL_CPPFLAGS := -fpermissive
LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H

ifeq ($(strip $(GRAPHIC_MEMORY_PROVIDER)),dma_buf)
LOCAL_CFLAGS += -DUSE_DMA_BUF
endif

ifeq ($(strip $(TARGET_BOARD_HARDWARE)),rk30board)	
LOCAL_CFLAGS += -DTARGET_RK30
LOCAL_CFLAGS += -DHAL_MOCKUP
endif

ifeq ($(strip $(TARGET_BOARD_HARDWARE)),rk2928board)
LOCAL_CFLAGS += -DTARGET_RK30
endif

ifeq ($(strip $(TARGET_BOARD_HARDWARE)),rk29board) 
LOCAL_CFLAGS += -DTARGET_RK29
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3288)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XX
LOCAL_CFLAGS += -DTARGET_RK32
LOCAL_CFLAGS += -DHAL_MOCKUP
LOCAL_CFLAGS += -DHAVE_ARM_NEON
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3368)
LOCAL_CFLAGS += -DTARGET_RK3368
LOCAL_CFLAGS += -DHAVE_ARM_NEON
LOCAL_CFLAGS += -DTARGET_RK32
LOCAL_CFLAGS += -DHAL_MOCKUP
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3036)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XX
LOCAL_CFLAGS += -DTARGET_RK32
LOCAL_CFLAGS += -DHAL_MOCKUP
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk312x)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XX
LOCAL_CFLAGS += -DTARGET_RK312x
LOCAL_CFLAGS += -DHAL_MOCKUP
LOCAL_CFLAGS += -DHAVE_ARM_NEON
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3188)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XX
LOCAL_CFLAGS += -DTARGET_RK3188
LOCAL_CFLAGS += -DHAL_MOCKUP
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk3026)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XX
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk30xx)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XX
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk319x)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XX
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk2928)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK2928
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),rk30xxb)	
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_RK30XXB
endif

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_CFLAGS += -DANDROID_5_X
endif

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
ifneq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
else
ifneq ($(strip $(TARGET_2ND_ARCH)), )
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_RELATIVE_PATH := hw
endif

LOCAL_MODULE:=camera.rk30board

LOCAL_MODULE_TAGS:= optional
include $(BUILD_SHARED_LIBRARY)


