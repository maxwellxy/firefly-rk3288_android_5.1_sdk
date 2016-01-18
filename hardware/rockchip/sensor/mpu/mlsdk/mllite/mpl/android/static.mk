MLLITE_LIB_NAME = mllite
LIBRARY = $(LIB_PREFIX)$(MLLITE_LIB_NAME).$(STATIC_LIB_EXT)

MK_NAME = $(notdir $(CURDIR)/$(firstword $(MAKEFILE_LIST)))

OBJFOLDER = $(CURDIR)/obj

CROSS = arm-none-linux-gnueabi-
COMP= $(CROSS)gcc
LINK= $(CROSS)ar cr

MLLITE_DIR = $(MLSDK_ROOT)/mllite
MPL_DIR = $(MLSDK_ROOT)/mldmp
MLPLATFORM_DIR = $(MLSDK_ROOT)/platform

include $(MLSDK_ROOT)/Android-common.mk

CFLAGS += $(CMDLINE_CFLAGS)
CFLAGS += -Wall -fpic
CFLAGS += -mthumb-interwork -fno-exceptions -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -fmessage-length=0
CFLAGS += -DNDEBUG
CFLAGS += -D_REENTRANT -DLINUX -DANDROID
CFLAGS += -I$(MLLITE_DIR) 
CFLAGS += -I$(MLPLATFORM_DIR)/include
CFLAGS += -I$(MLSDK_ROOT)/mlutils 
CFLAGS += -I$(MLSDK_ROOT)/mlapps/common
CFLAGS += $(MLSDK_INCLUDES)
CFLAGS += $(MLSDK_DEFINES)

VPATH += $(MLLITE_DIR) 
VPATH += $(MLSDK_ROOT)/mlutils
VPATH += $(MLLITE_DIR)/accel 
VPATH += $(MLLITE_DIR)/compass 
VPATH += $(MLLITE_DIR)/pressure 
VPATH += $(MLLITE_DIR)/mlapps/common

####################################################################################################
## sources

ML_LIBS  = $(MLPLATFORM_DIR)/linux/$(LIB_PREFIX)$(MLPLATFORM_LIB_NAME).$(STATIC_LIB_EXT)

ML_SOURCES += $(MLLITE_DIR)/mldl_cfg_mpu.c
ML_SOURCES += $(MLLITE_DIR)/mldl_cfg_init_linux.c
ML_SOURCES += $(MLLITE_DIR)/accel.c
ML_SOURCES += $(MLLITE_DIR)/compass.c
ML_SOURCES += $(MLLITE_DIR)/compass_supervisor.c
ML_SOURCES += $(MLLITE_DIR)/compass_supervisor_adv_callbacks.c
ML_SOURCES += $(MLLITE_DIR)/key0_96.c
ML_SOURCES += $(MLLITE_DIR)/pressure.c
ML_SOURCES += $(MLLITE_DIR)/ml.c
ML_SOURCES += $(MLLITE_DIR)/ml_invobj.c
ML_SOURCES += $(MLLITE_DIR)/ml_init.c
ML_SOURCES += $(MLLITE_DIR)/mlarray_lite.c
ML_SOURCES += $(MLLITE_DIR)/mlarray_adv.c
ML_SOURCES += $(MLLITE_DIR)/mlarray_legacy.c
ML_SOURCES += $(MLLITE_DIR)/mlBiasNoMotion.c
ML_SOURCES += $(MLLITE_DIR)/mlFIFO.c
ML_SOURCES += $(MLLITE_DIR)/mlFIFOHW.c
ML_SOURCES += $(MLLITE_DIR)/mlMathFunc.c
ML_SOURCES += $(MLLITE_DIR)/mlcontrol.c
ML_SOURCES += $(MLLITE_DIR)/mldl.c
ML_SOURCES += $(MLLITE_DIR)/mldmp.c
ML_SOURCES += $(MLLITE_DIR)/dmpDefault.c
ML_SOURCES += $(MLLITE_DIR)/mlstates.c
ML_SOURCES += $(MLLITE_DIR)/mlsupervisor.c
ML_SOURCES += $(MLLITE_DIR)/ml_stored_data.c
ML_SOURCES += $(MLLITE_DIR)/ustore_manager.c
ML_SOURCES += $(MLLITE_DIR)/ustore_mlsl_io.c
ML_SOURCES += $(MLLITE_DIR)/ustore_adv_fusion_delegate.c
ML_SOURCES += $(MLLITE_DIR)/ustore_lite_fusion_delegate.c
ML_SOURCES += $(MLLITE_DIR)/temp_comp_legacy.c
ML_SOURCES += $(MLLITE_DIR)/mlSetGyroBias.c
ML_SOURCES += $(MLSDK_ROOT)/mlutils/checksum.c
ML_SOURCES += $(MLLITE_DIR)/ml_mputest.c
ML_SOURCES += $(MLSDK_ROOT)/mlutils/mputest.c
ML_SOURCES += $(MLLITE_DIR)/mldl_print_cfg.c

ML_OBJS := $(addsuffix .o,$(ML_SOURCES))
ML_OBJS_DST = $(addprefix $(OBJFOLDER)/,$(addsuffix .o, $(notdir $(ML_SOURCES))))

####################################################################################################
## rules

.PHONY: all clean cleanall

all: $(LIBRARY) $(MK_NAME)

$(LIBRARY) : $(OBJFOLDER) $(ML_OBJS_DST) $(ML_LIBS) $(MK_NAME)
	@$(call echo_in_colors, "\n<linking $(LIBRARY) with objects $(ML_OBJS_DST)\n")
	$(LINK) $(LIBRARY) $(ML_OBJS_DST)
	$(CROSS)ranlib $(LIBRARY)

$(OBJFOLDER) : 
	@$(call echo_in_colors, "\n<creating object's folder 'obj/'>\n")
	mkdir obj

$(ML_OBJS_DST) : $(OBJFOLDER)/%.c.o : %.c  $(MK_NAME)
	@$(call echo_in_colors, "\n<compile $< to $(OBJFOLDER)/$(notdir $@)>\n")
	$(COMP) $(CFLAGS) $(ANDROID_INCLUDES) $(KERNEL_INCLUDES) $(MLSDK_INCLUDES) -o $@ -c $<

clean : 
	rm -fR $(OBJFOLDER)

cleanall : 
	rm -fR $(LIBRARY) $(OBJFOLDER)

