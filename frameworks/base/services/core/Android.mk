LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := services.core

LOCAL_SRC_FILES += \
    $(call all-java-files-under,java) \
    java/com/android/server/EventLogTags.logtags \
    java/com/android/server/am/EventLogTags.logtags

LOCAL_JAVA_LIBRARIES := android.policy telephony-common
LOCAL_STATIC_JAVA_LIBRARIES := multiwindowservice boot_boost
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := multiwindowservice:./../../multiwindow/multiwindowservice.jar boot_boost:./../../third-party/boot_boost.jar
include $(BUILD_MULTI_PREBUILT)
