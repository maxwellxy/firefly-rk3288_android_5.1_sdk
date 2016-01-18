LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        src/bt_hci_bdroid.c \
        src/lpm.c \
        src/bt_hw.c \
        src/btsnoop.c \
        src/utils.c

ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), mt5931_6622)
LOCAL_CFLAGS := -DMTK_MT6622
endif

ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), mt6622)
LOCAL_CFLAGS := -DMTK_MT6622
endif

ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), esp8089_bk3515)
LOCAL_CFLAGS := -DBT_BK3515A
endif

ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), rda587x)
    LOCAL_CFLAGS += -DRDA587X_BLUETOOTH
endif 

ifeq ($(BLUETOOTH_HCI_USE_MCT),true)

LOCAL_CFLAGS := -DHCI_USE_MCT

LOCAL_SRC_FILES += \
        src/hci_mct.c \
        src/userial_mct.c

else
ifeq ($(BLUETOOTH_HCI_USE_RTK_H5),true)    

LOCAL_CFLAGS := -DHCI_USE_RTK_H5

LOCAL_SRC_FILES += \
       src/hci_h5.c \
       src/userial.c \
	src/bt_skbuff.c \
	src/bt_list.c

else
LOCAL_SRC_FILES += \
        src/hci_h4.c \
        src/userial.c

endif

endif


LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/../utils/include

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libdl \
        libbt-utils

LOCAL_MODULE := libbt-hci
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)
