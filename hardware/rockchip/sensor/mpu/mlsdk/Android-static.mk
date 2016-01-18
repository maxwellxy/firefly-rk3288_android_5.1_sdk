SHELL=/bin/bash

MLSDK_ROOT   = $(CURDIR)
TARGET = android

ifeq ($(VERBOSE),1)
	DUMP=1>/dev/stdout
else
	DUMP=1>/dev/null
endif

include Android-common.mk

#############################################################################
## targets

LIB_FOLDERS  = platform/linux
LIB_FOLDERS += mllite/mpl/$(TARGET)
LIB_FOLDERS += mldmp/mpl/$(TARGET)
LIB_FOLDERS += external/aichi/mpl/$(TARGET)
LIB_FOLDERS += external/akmd/mpl/$(TARGET)
LIB_FOLDERS += external/akm8963/mpl/$(TARGET)
LIB_FOLDERS += external/memsic/mpl/$(TARGET)

APP_FOLDERS += mlapps/DemoAppPoll/PollUnix
ifeq ($(MPU_NAME),MPU3050)
	APP_FOLDERS += mltools/driver_selftest
endif

INSTALL_DIR = $(CURDIR)

####################################################################################################
## macros

ifndef echo_in_colors
define echo_in_colors
	echo -ne "\e[1;34m"$(1)"\e[0m"
endef
endif

define maker_libs
	echo "MLPLATFORM_LIB_NAME = $(MLPLATFORM_LIB_NAME)"
	echo "MLLITE_LIB_NAME     = $(MLLITE_LIB_NAME)"
	echo "AICHI_LIB_NAME      = $(AICHI_LIB_NAME)"
	echo "MEMSIC_LIB_NAME     = $(MEMSIC_LIB_NAME)"
	echo "MPL_LIB_NAME        = $(MPL_LIB_NAME)"

	$(call echo_in_colors, "\n<making '$(1)' in folder 'platform/linux'>\n"); \
	make MLSDK_ROOT=$(MLSDK_ROOT) MLPLATFORM_LIB_NAME=$(MLPLATFORM_LIB_NAME) ANDROID_ROOT=$(ANDROID_ROOT) KERNEL_ROOT=$(KERNEL_ROOT) PRODUCT=$(PRODUCT) -C platform/linux -f Android-static.mk $@ $(DUMP)

	$(call echo_in_colors, "\n<making '$(1)' in folder 'mllite/mpl/$(TARGET)'>\n"); \
	make MLSDK_ROOT=$(MLSDK_ROOT) MLLITE_LIB_NAME=$(MLLITE_LIB_NAME) ANDROID_ROOT=$(ANDROID_ROOT) KERNEL_ROOT=$(KERNEL_ROOT) PRODUCT=$(PRODUCT) -C mllite/mpl/$(TARGET) -f static.mk $@ $(DUMP)

	if test -f external/aichi/mpl/$(TARGET)/static.mk; then \
		$(call echo_in_colors, "\n<making '$(1)' in folder 'external/aichi/mpl/$(TARGET)'>\n"); \
		make MLSDK_ROOT=$(MLSDK_ROOT) AICHI_LIB_NAME=$(AICHI_LIB_NAME) ANDROID_ROOT=$(ANDROID_ROOT) KERNEL_ROOT=$(KERNEL_ROOT) PRODUCT=$(PRODUCT) -C external/aichi/mpl/$(TARGET) -f static.mk $@ $(DUMP); \
	fi

	if test -f external/akmd/mpl/$(TARGET)/static.mk; then \
		$(call echo_in_colors, "\n<making '$(1)' in folder 'external/akmd/mpl/$(TARGET)'>\n"); \
		make MLSDK_ROOT=$(MLSDK_ROOT) AKM_LIB_NAME=$(AKM_LIB_NAME) ANDROID_ROOT=$(ANDROID_ROOT) KERNEL_ROOT=$(KERNEL_ROOT) PRODUCT=$(PRODUCT) -C external/akmd/mpl/$(TARGET) -f static.mk $@ $(DUMP); \
	fi

	if test -f external/akm8963/mpl/$(TARGET)/static.mk; then \
		$(call echo_in_colors, "\n<making '$(1)' in folder 'external/akm8963/mpl/$(TARGET)'>\n"); \
		make MLSDK_ROOT=$(MLSDK_ROOT) AKM8963_LIB_NAME=$(AKM8963_LIB_NAME) ANDROID_ROOT=$(ANDROID_ROOT) KERNEL_ROOT=$(KERNEL_ROOT) PRODUCT=$(PRODUCT) -C external/akm8963/mpl/$(TARGET) -f static.mk $@ $(DUMP); \
	fi

	if test -f external/memsic/mpl/$(TARGET)/static.mk; then \
		$(call echo_in_colors, "\n<making '$(1)' in folder 'external/memsic/mpl/$(TARGET)'>\n"); \
		make MLSDK_ROOT=$(MLSDK_ROOT) MEMSIC_LIB_NAME=$(MEMSIC_LIB_NAME) ANDROID_ROOT=$(ANDROID_ROOT) KERNEL_ROOT=$(KERNEL_ROOT) PRODUCT=$(PRODUCT) -C external/memsic/mpl/$(TARGET) -f static.mk $@ $(DUMP); \
	fi

	if test -f mldmp/mpl/$(TARGET)/Android-shared.mk; then \
		$(call echo_in_colors, "\n<making '$(1)' in folder 'mldmp/mpl/$(TARGET)'>\n"); \
	        make MLSDK_ROOT=$(MLSDK_ROOT) MPL_LIB_NAME=$(MPL_LIB_NAME) ANDROID_ROOT=$(ANDROID_ROOT) KERNEL_ROOT=$(KERNEL_ROOT) PRODUCT=$(PRODUCT) -C mldmp/mpl/$(TARGET) -f static.mk $@ $(DUMP); \
	fi
endef

define maker_apps
	for dir in $(APP_FOLDERS); do \
		$(call echo_in_colors, "\n<making '$(1)' in folder $$dir>\n"); \
		make MLSDK_ROOT=$(MLSDK_ROOT) -C $$dir TARGET=$(TARGET) -f Android-static.mk $@ $(DUMP); \
	done
endef 

#############################################################################
## rules

.PHONY : all $(LIB_FOLDERS) $(APP_FOLDERS) clean cleanall install

all : 
	@$(call maker_libs,$@)
	@$(call maker_apps,$@)

clean : 
	@$(call maker_libs,$@)
	@$(call maker_apps,$@)

cleanall : 
	@$(call maker_libs,$@)
	@$(call maker_apps,$@)

install:
	for dir in $(APP_FOLDERS); do \
		$(call echo_in_colors, "\n<making '$(1)' in folder $$dir>\n"); \
		make INSTALL_DIR=$(INSTALL_DIR) -C $$dir TARGET=$(TARGET) $@ $(DUMP); \
	done
