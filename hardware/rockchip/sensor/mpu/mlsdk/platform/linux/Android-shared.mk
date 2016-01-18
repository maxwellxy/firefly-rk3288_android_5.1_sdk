MLPLATFORM_LIB_NAME = mlplatform
LIBRARY = $(LIB_PREFIX)$(MLPLATFORM_LIB_NAME).$(SHARED_LIB_EXT)

MK_NAME = $(notdir $(CURDIR)/$(firstword $(MAKEFILE_LIST)))

CROSS = $(ANDROID_ROOT)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
COMP  = $(CROSS)gcc
LINK  = $(CROSS)gcc 

OBJFOLDER = $(CURDIR)/obj

MLSDK_ROOT = ../..
MLLITE_DIR = $(MLSDK_ROOT)/mllite

include $(MLSDK_ROOT)/Android-common.mk

CFLAGS += $(CMDLINE_CFLAGS)
CFLAGS += -Wall 
CFLAGS += -fpic -nostdlib
CFLAGS += -DNDEBUG
CFLAGS += -D_REENTRANT -DLINUX -DANDROID 
CFLAGS += -DUNICODE -D_UNICODE -DSK_RELEASE 
CFLAGS += -DI2CDEV=\"/dev/mpu\" 
CFLAGS += -mthumb-interwork 
CFLAGS += -fno-exceptions -ffunction-sections -funwind-tables 
CFLAGS += -fno-short-enums -fstack-protector -fmessage-length=0
CFLAGS += -I$(MLLITE_DIR) -I$(ML_PLATFORM)/include
CFLAGS += -I$(MLSDK_ROOT)/mlutils
CFLAGS += -I$(MLSDK_ROOT)

LLINK += -lc -lm -lutils -lcutils -lgcc -ldl

LFLAGS += $(CMDLINE_LFLAGS)
LFLAGS += -nostdlib -shared -fpic 
LFLAGS += -Wl,--gc-sections -Wl,--no-whole-archive -Wl,-shared,-Bsymbolic
LFLAGS += -Wl,-T,$(ANDROID_ROOT)/build/core/armelf.xsc 
LFLAGS += -Wl,-soname,$(LIBRARY) 
LFLAGS += $(ANDROID_LINK)
LFLAGS += -Wl,-rpath,$(ANDROID_ROOT)/out/target/product/$(PRODUCT)/obj/lib:$(ANDROID_ROOT)/out/target/product/$(PRODUCT)/system/lib

VPATH += $(MLLITE_DIR)

####################################################################################################
## sources

ML_SOURCES = \
	$(MLLITE_DIR)/int_linux.c \
	$(MLLITE_DIR)/mlos_linux.c \
	$(MLLITE_DIR)/mlsl_linux_mpu.c

#ML_SOURCES += \
	$(MLLITE_DIR)/log_linux.c \
	$(MLLITE_DIR)/log_printf_linux.c \

ML_OBJS := $(addsuffix .o,$(ML_SOURCES))
ML_OBJS_DST = $(addprefix $(OBJFOLDER)/,$(addsuffix .o, $(notdir $(ML_SOURCES))))

####################################################################################################
## rules

.PHONY: all clean cleanall

all: $(LIBRARY) $(MK_NAME)

$(LIBRARY) : $(OBJFOLDER) $(ML_OBJS_DST) $(MK_NAME)
	@$(call echo_in_colors, "\n<linking $(LIBRARY) with objects $(ML_OBJS_DST)\n")
	$(LINK) $(LFLAGS) -o $(LIBRARY) $(ML_OBJS_DST) $(LLINK)

$(OBJFOLDER) : 
	@$(call echo_in_colors, "\n<creating object's folder 'obj/'>\n")
	mkdir obj

$(ML_OBJS_DST) : $(OBJFOLDER)/%.c.o : %.c  $(MK_NAME)
	@$(call echo_in_colors, "\n<compile $< to $(OBJFOLDER)/$(notdir $@)>\n")
	$(COMP) $(ANDROID_INCLUDES) $(KERNEL_INCLUDES) $(MLSDK_INCLUDES) $(CFLAGS) -o $@ -c $<

clean : 
	rm -fR $(OBJFOLDER)

cleanall : 
	rm -fR $(LIBRARY) $(OBJFOLDER)
