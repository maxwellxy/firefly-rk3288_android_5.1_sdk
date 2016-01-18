LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	bootanimation_main.cpp \
	AudioPlayer.cpp \
	BootAnimation.cpp

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_C_INCLUDES += external/tinyalsa/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libandroidfw \
	libutils \
	libbinder \
    libui \
	libskia \
    libEGL \
    libGLESv1_CM \
    libmedia \
    libgui \
	libtinyalsa

LOCAL_C_INCLUDES += \
	$(call include-path-for, corecg graphics)

#TARGET_CONTINUOUS_SPLASH_ENABLED := true
ifeq ($(TARGET_CONTINUOUS_SPLASH_ENABLED),true)
    LOCAL_CFLAGS += -DCONTINUOUS_SPLASH
endif

#TARGET_BOOTANIMATION_PRELOAD := true
ifeq ($(TARGET_BOOTANIMATION_PRELOAD),true)
    LOCAL_CFLAGS += -DPRELOAD_BOOTANIMATION
endif

#TARGET_BOOTANIMATION_USE_RGB565 := true
ifeq ($(TARGET_BOOTANIMATION_USE_RGB565),true)
    LOCAL_CFLAGS += -DUSE_565
endif

#TARGET_BOOTRING_ENABLED := true
ifeq ($(strip $(TARGET_BOOTRING_ENABLED)),true)
    LOCAL_CFLAGS += -DBOOTRING_ENABLED
endif

LOCAL_MODULE:= bootanimation

ifdef TARGET_32_BIT_SURFACEFLINGER
LOCAL_32_BIT_ONLY := true
endif

include $(BUILD_EXECUTABLE)
