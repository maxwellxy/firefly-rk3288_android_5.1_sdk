
# wifi firmware
PRODUCT_COPY_FILES += \
        external/wlan_loader/iwconfig:system/bin/iwconfig \
        external/wlan_loader/iwlist:system/bin/iwlist \

WifiFirmwareFile := $(shell ls $(LOCAL_PATH)/firmware)
PRODUCT_COPY_FILES += \
        $(foreach file, $(WifiFirmwareFile), $(LOCAL_PATH)/firmware/$(file):system/etc/firmware/$(file))

#ifeq ($(WIFI_HAVE_WAPI), true)
#       PRODUCT_COPY_FILES +=  external/wlan_loader/firmware/fw_bcm4329_wapi.bin:system/etc/firmware/fw_bcm4329.bin 
#else
#        PRODUCT_COPY_FILES += external/wlan_loader/firmware/fw_bcm4329.bin:system/etc/firmware/fw_bcm4329.bin
#endif


