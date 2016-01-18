LOCAL_PATH:= $(call my-dir)

# the library
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)
            
LOCAL_MODULE := android.policy
LOCAL_STATIC_JAVA_LIBRARIES := multiwindowpolicy

include $(BUILD_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := multiwindowpolicy:./../multiwindow/multiwindowpolicy.jar
include $(BUILD_MULTI_PREBUILT)

# additionally, build unit tests in a separate .apk
include $(call all-makefiles-under,$(LOCAL_PATH))
