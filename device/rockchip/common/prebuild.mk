#GENERATE MANIFEST
$(shell test -d .repo && .repo/repo/repo manifest -r -o manifest.xml)

-include $(TARGET_DEVICE_DIR)/prebuild.mk

