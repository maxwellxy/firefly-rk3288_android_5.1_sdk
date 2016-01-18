ifeq ($(strip $(BOARD_SENSOR_MPU)), true)
include $(call all-subdir-makefiles)
endif
