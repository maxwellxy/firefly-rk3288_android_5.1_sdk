
ifneq ($(filter arm%, $(TARGET_ARCH)), )
ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk3368)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/nand/modules/arm64/drmboot.ko:root/drmboot.ko \
    vendor/rockchip/common/nand/modules/arm64/rk30xxnand_ko.ko.3.10.0:root/rk30xxnand_ko.ko.3.10.0 \
    vendor/rockchip/common/nand/modules/arm64/rk30xxnand_ko.ko.3.10.0:recovery/root/rk30xxnand_ko.ko.3.10.0
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk3036)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/nand/modules/arm/rk3036/rk30xxnand_ko.ko.3.10.0:root/rk30xxnand_ko.ko.3.10.0 \
    vendor/rockchip/common/nand/modules/arm/drmboot.ko:root/drmboot.ko
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk3188)
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/nand/modules/arm/rk3188/rk30xxnand_ko.ko:root/rk30xxnand_ko.ko \
    vendor/rockchip/common/nand/modules/arm/drmboot.ko:root/drmboot.ko
else
PRODUCT_COPY_FILES += \
    vendor/rockchip/common/nand/modules/arm/rk30xxnand_ko.ko.3.10.0:root/rk30xxnand_ko.ko.3.10.0 \
    vendor/rockchip/common/nand/modules/arm/drmboot.ko:root/drmboot.ko
endif
endif
