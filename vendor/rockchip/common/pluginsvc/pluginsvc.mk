
ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sofia3gr)
PRODUCT_PACKAGES += \
	pluginservice \
	libpluginservice \
	librkplugin

endif
