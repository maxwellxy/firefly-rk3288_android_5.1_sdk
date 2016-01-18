PRODUCT_COPY_FILES += \
        vendor/rockchip/common/security/optee/optee.ko:system/lib/modules/optee.ko \
	vendor/rockchip/common/security/optee/optee_armtz.ko:system/lib/modules/optee_armtz.ko
#LOCAL_PATH := $(call my-dir)
#OPTEE_KO_FILES := $(shell ls $(LOCAL_PATH)/*.ko)
#PRODUCT_COPY_FILES += \
#    $(foreach file, $(OPTEE_KO_FILES), $(LOCAL_PATH)/$(file):system/lib/modules/$(file))
