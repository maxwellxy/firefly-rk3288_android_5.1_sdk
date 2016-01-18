CUR_PATH := vendor/widevine


########################################################
#add drm function
#######################################################
ifeq ($(strip $(TARGET_BOARD_HARDWARE)), sofiaboard)
PRODUCT_COPY_FILES += vendor/widevine/libs/x86/libdrmdecrypt.so:system/lib/libdrmdecrypt.so \
                     vendor/widevine/libs/x86/libwvdrmengine.so:system/vendor/lib/mediadrm/libwvdrmengine.so \
                     vendor/widevine/libs/x86/libdrmwvmplugin.so:system/vendor/lib/drm/libdrmwvmplugin.so \
                     vendor/widevine/libs/x86/libwvdrm_L3.so:system/vendor/lib/libwvdrm_L3.so \
                     vendor/widevine/libs/x86/libWVStreamControlAPI_L3.so:system/vendor/lib/libWVStreamControlAPI_L3.so \
                     vendor/widevine/libs/x86/libwvm.so:system/vendor/lib/libwvm.so \
                     vendor/widevine/framework/x86/com.google.widevine.software.drm.jar:system/framework/com.google.widevine.software.drm.jar \
                     vendor/widevine/framework/x86/com.google.widevine.software.drm.xml:system/etc/permissions/com.google.widevine.software.drm.xml
else
PRODUCT_COPY_FILES += vendor/widevine/libs/arm/libdrmdecrypt.so:system/lib/libdrmdecrypt.so \
                     vendor/widevine/libs/arm/libwvdrmengine.so:system/vendor/lib/mediadrm/libwvdrmengine.so \
                     vendor/widevine/libs/arm/libdrmwvmplugin.so:system/vendor/lib/drm/libdrmwvmplugin.so \
                     vendor/widevine/libs/arm/libwvdrm_L3.so:system/vendor/lib/libwvdrm_L3.so \
                     vendor/widevine/libs/arm/libWVStreamControlAPI_L3.so:system/vendor/lib/libWVStreamControlAPI_L3.so \
                     vendor/widevine/libs/arm/libwvm.so:system/vendor/lib/libwvm.so \
                     vendor/widevine/framework/arm/com.google.widevine.software.drm.jar:system/framework/com.google.widevine.software.drm.jar \
                     vendor/widevine/framework/arm/com.google.widevine.software.drm.xml:system/etc/permissions/com.google.widevine.software.drm.xml
endif
