LOCAL_PATH := $(my-dir)
####################### priv-app ######################################

include $(CLEAR_VARS)
LOCAL_MODULE := hdmiin
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(LOCAL_MODULE).apk
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
include $(BUILD_PREBUILT)

