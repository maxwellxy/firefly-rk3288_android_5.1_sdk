CUR_PATH := vendor/rockchip/common/bluetooth

ifeq ($(strip $(BLUETOOTH_USE_BPLUS)),true)
PRODUCT_PACKAGES += \
	libbt-client-api \
	com.broadcom.bt \
	com.broadcom.bt.xml
endif

ifeq ($(strip $(MT6622_BT_SUPPORT)),true)
#PRODUCT_COPY_FILES += \
    vendor/rockchip/common/bluetooth/mt6622/libbluetooth_mtk.so:system/lib/libbluetooth_mtk.so \
    vendor/rockchip/common/bluetooth/mt6622/libbt-vendor.so:system/lib/libbt-vendor.so
endif

ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), rtl8723as)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723as/rtl8723a_fw:system/etc/firmware/rtlbt/rtlbt_fw \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723as/rtl8723a_config:system/etc/firmware/rtlbt/rtlbt_config
endif

#ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), rtl8723bs)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723bs/rtl8723b_fw:system/etc/firmware/rtl8723bs_fw
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723bs/rtl8723b_config:system/etc/firmware/rtl8723bs_config
#endif

#ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), rtl8723bs_vq0)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723bs_vq0/rtl8723b_fw:system/etc/firmware/rtl8723bs_VQ0_fw
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723bs_vq0/rtl8723b_VQ0_config:system/etc/firmware/rtl8723bs_VQ0_config
#endif


#ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), rtl8723au)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723au/rtl8723a_fw:system/etc/firmware/rtl8723a_fw \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723au/rtl8723a_config:system/etc/firmware/rtl8723a_config
#endif

#ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), rtl8723bu)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723bu/rtl8723b_fw:system/etc/firmware/rtl8723b_fw \
    vendor/rockchip/common/bluetooth/realtek/bt/firmware/rtl8723bu/rtl8723bu_config:system/etc/firmware/rtl8723bu_config
#endif

BT_FIRMWARE_FILES := $(shell ls $(CUR_PATH)/lib/firmware)
PRODUCT_COPY_FILES += \
    $(foreach file, $(BT_FIRMWARE_FILES), $(CUR_PATH)/lib/firmware/$(file):system/vendor/firmware/$(file))

#include vendor/rockchip/common/bluetooth/console_start_bt/console_start_bt.mk

