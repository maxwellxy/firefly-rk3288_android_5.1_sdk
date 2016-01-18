MLPLATFORM_LIB_NAME = mlplatform
LIBRARY = $(LIB_PREFIX)$(MLPLATFORM_LIB_NAME).$(STATIC_LIB_EXT)

MK_NAME = $(notdir $(CURDIR)/$(firstword $(MAKEFILE_LIST)))

OBJFOLDER = $(CURDIR)/obj

CROSS = arm-none-linux-gnueabi-
COMP= $(CROSS)gcc
LINK= $(CROSS)ar cr

MLSDK_ROOT = ../..
MLPLATFORM_DIR = .

include $(MLSDK_ROOT)/Android-common.mk

CFLAGS += $(CMDLINE_CFLAGS)
CFLAGS +=  -Wall -fpic
CFLAGS += -DNDEBUG
CFLAGS += -mthumb-interwork 
CFLAGS += -fno-exceptions -ffunction-sections -funwind-tables 
CFLAGS += -fstack-protector -fmessage-length=0 -fno-short-enums
CFLAGS += -D_REENTRANT -DLINUX -DANDROID
CFLAGS += -I$(MLPLATFORM_DIR)/../include -I$(MLPLATFORM_DIR) -I$(MLPLATFORM_DIR)/kernel 
CFLAGS += -I$(MLSDK_ROOT)/mllite 

VPATH += $(MLPLATFORM_DIR)

####################################################################################################
## sources

ML_SOURCES = \
	$(MLPLATFORM_DIR)/int_linux.c \
	$(MLPLATFORM_DIR)/mlos_linux.c \
	$(MLPLATFORM_DIR)/mlsl_linux_mpu.c

ML_OBJS := $(addsuffix .o,$(ML_SOURCES))
ML_OBJS_DST = $(addprefix $(OBJFOLDER)/,$(addsuffix .o, $(notdir $(ML_SOURCES))))

####################################################################################################
## rules

.PHONY: all clean cleanall

all: $(LIBRARY) $(MK_NAME)

$(LIBRARY) : $(OBJFOLDER) $(ML_OBJS_DST) $(MK_NAME)
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
