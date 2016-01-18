# Use hardware GPS implementation if available.
LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_GPS_GNS7560), true)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := liblog \
	libcutils \
	libutils

ifeq ($(HAVE_BOARD_GPS_LIBRARIES),true)
  LOCAL_CFLAGS           += -DHAVE_GPS_HARDWARE
  LOCAL_SHARED_LIBRARIES += $(BOARD_GPS_LIBRARIES)
endif

# Use emulator GPS implementation if QEMU_HARDWARE is set.
#
USE_QEMU_GPS_HARDWARE := $(QEMU_HARDWARE)

ifeq ($(USE_QEMU_GPS_HARDWARE),true)
    LOCAL_CFLAGS    += -DHAVE_QEMU_GPS_HARDWARE
# LOCAL_SRC_FILES += gps/gps_hardware.c
endif

LOCAL_SRC_FILES += gps.c \
		src/GN_GPS_api_calls_with_NV_UTC.c \
		src/GPS_Task.c \
		src/GN_GPS_DataLogs.c \
		src/API_Interface_new.c  \
		src/UART_Input_Task_new.c \
		src/UART_Output_Task_new.c	\
		gps_hardware.c \
		src/GPS_main.c

#LOCAL_SRC_FILES += gps/gps.c \
		gps/src/GN_GPS_api_calls_with_NV_UTC.c \
		gps/src/GPS_Task.c \
		gps/src/GN_GPS_DataLogs.c \
		gps/src/API_Interface_new.c  \
		gps/src/UART_Input_Task_new.c \
		gps/src/UART_Output_Task_new.c	\
		gps/src/GPS_main.c

LOCAL_CFLAGS += -fno-short-enums 

LOCAL_LDFLAGS = $(LOCAL_PATH)/lib/libgps_4.02.1.a
#LOCAL_LDFLAGS = $(LOCAL_PATH)/gps/lib/libgps_4.02.1.a


LOCAL_MODULE := gps.$(TARGET_BOARD_PLATFORM)
include $(BUILD_SHARED_LIBRARY)

endif  
