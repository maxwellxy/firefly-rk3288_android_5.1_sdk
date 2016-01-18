
PRODUCT_PACKAGES += \
    RkApkinstaller  \
    RKUpdateService

ifneq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
PRODUCT_PACKAGES += \
    MediaFloat      \
    GoogleEmail

endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk3288)
PRODUCT_PACKAGES += \
    WinStartService \
    projectX
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk312x)
PRODUCT_PACKAGES += \
    RkVideoPlayer
else
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)), box)
PRODUCT_PACKAGES += \
    RkBoxVideoPlayer
else
ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
PRODUCT_PACKAGES += \
    Rk3grVideoPlayer
else
PRODUCT_PACKAGES += \
    Rk4kVideoPlayer
endif
endif
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
PRODUCT_PACKAGES += \
    Rk3grExplorer
else
PRODUCT_PACKAGES += \
    RkExplorer
endif

ifeq ($(strip $(BOARD_HAS_STRESSTEST_APP)), true)
    PRODUCT_PACKAGES += \
    StressTest \
    DeviceTest
endif


ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
PRODUCT_PACKAGES += \
    ituxd\
    com.rockchip.alarmhelper \
    RFTest

#PRODUCT_COPY_FILES += \
#        $(LOCAL_PATH)/ituxd/lib/x86/libthermalJNI.so:system/lib/libthermalJNI.so
endif


###########for box app ################
ifeq ($(strip $(TARGET_BOARD_PLATFORM_PRODUCT)), box)
PRODUCT_PACKAGES += \
    eHomeMediaCenter_box	\
    Launcher3	\
    RKGameControlSettingV1.0.1	\
    DualScreenApk \
    RkMusic	
endif
