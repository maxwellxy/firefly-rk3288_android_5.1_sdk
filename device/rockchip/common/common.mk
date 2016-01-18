include device/rockchip/common/BoardConfig.mk
$(call inherit-product, device/rockchip/common/device.mk)

PRODUCT_BRAND := rockchip
PRODUCT_DEVICE := rksdk
PRODUCT_NAME := rksdk
PRODUCT_MODEL := rksdk
PRODUCT_MANUFACTURER := rockchip


PRODUCT_PROPERTY_OVERRIDES += \
			ro.product.version = 1.0.0 \
			ro.product.ota.host = www.rockchip.com:2300
