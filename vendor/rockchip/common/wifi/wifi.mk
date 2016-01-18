#WIFI_KO_FILES := $(shell ls $(CUR_PATH)/wifi/modules)
#PRODUCT_COPY_FILES += \
    $(foreach file, $(WIFI_KO_FILES), $(CUR_PATH)/wifi/modules/$(file):system/lib/modules/$(file))
