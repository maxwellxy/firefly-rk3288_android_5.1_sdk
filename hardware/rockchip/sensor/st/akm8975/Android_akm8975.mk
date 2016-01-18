
LOCAL_PATH:= $(call my-dir)

# libAK8975
include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_MODULE := libAK8975
LOCAL_MODULE_TAGS := optional

include $(BUILD_SYSTEM)/binary.mk

$(LOCAL_BUILT_MODULE): $(LOCAL_PATH)/libAK8975/libAK8975.a
	@mkdir -p $(dir $@)
	@echo "target StaticLib: $(PRIVATE_MODULE) ($@)"
	@rm -f $@
	$(hide) cp $^ $@

# akmd2
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/libAK8975 

LOCAL_SRC_FILES:= \
	AK8975Driver.c \
	DispMessage.c \
	FileIO.c \
	Measure.c \
	misc.c \
	main.c \
	Acc_mma8452.c
	# Acc_dummy.c

LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_MODULE := akmd
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := false
LOCAL_STATIC_LIBRARIES := libAK8975
LOCAL_SHARED_LIBRARIES := libc libm libz libutils libcutils
include $(BUILD_EXECUTABLE)

