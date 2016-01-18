ifeq ($(strip $(TARGET_BOARD_PLATFORM_GPU)), mali-t760)
ifeq ($(strip $(TARGET_ARCH)), arm)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/gpu/MaliT760/lib/$(TARGET_ARCH)/libGLES_mali.so:system/vendor/lib/egl/libGLES_mali.so \
    vendor/rockchip/common/gpu/MaliT760/modules/$(TARGET_ARCH)/mali_kbase.ko:system/lib/modules/mali_kbase.ko \
    vendor/rockchip/common/gpu/gpu_performance/bin/$(TARGET_ARCH)/performance:system/bin/performance \
    vendor/rockchip/common/gpu/gpu_performance/lib/$(TARGET_ARCH)/libperformance_runtime.so:system/lib/libperformance_runtime.so \
    vendor/rockchip/common/gpu/gpu_performance/lib/$(TARGET_ARCH)/gpu.$(TARGET_BOARD_HARDWARE).so:system/lib/hw/gpu.$(TARGET_BOARD_HARDWARE).so \
    vendor/rockchip/common/gpu/gpu_performance/etc/performance_info.xml:system/etc/performance_info.xml \
    vendor/rockchip/common/gpu/gpu_performance/etc/packages-compat.xml:system/etc/packages-compat.xml \
    vendor/rockchip/common/gpu/gpu_performance/lib/$(TARGET_ARCH)/libface_detection_native.so:system/lib/libface_detection_native.so \
    vendor/rockchip/common/gpu/gpu_performance/lib/$(TARGET_ARCH)/libopencv_java.so:system/lib/libopencv_java.so

endif
endif

ifeq ($(strip $(ENABLE_STEREO_DEFORM)), true)
PRODUCT_COPY_FILES += \
	vendor/rockchip/common/gpu/libs/libGLES.so:system/lib/egl/libGLES.so
endif