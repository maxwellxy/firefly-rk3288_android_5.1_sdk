#///============================================================================
#/// Copyright 2011 Broadcom Corporation -- http://www.broadcom.com
#/// This program is the proprietary software of Broadcom Corporation and/or
#/// its licensors, and may only be used, duplicated, modified or distributed
#/// pursuant to the terms and conditions of a separate, written license
#/// agreement executed between you and Broadcom (an "Authorized License").
#/// Except as set forth in an Authorized License, Broadcom grants no license
#/// (express or implied), right to use, or waiver of any kind with respect to
#/// the Software, and Broadcom expressly reserves all rights in and to the
#/// Software and all intellectual property rights therein. IF YOU HAVE NO
#/// AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
#/// WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
#/// THE SOFTWARE.
#/// ---------------------------------------------------------------------------
#/// \file
#/// \brief Unix libgps for broadcom build script
#///============================================================================

SOURCE_TOPDIR=../../../../../..
TARGET=$(CUST)$(PLATF)
ifeq "$(CUST)" ""
else
TARGET_DIR=$(SOURCE_TOPDIR)/customers/$(CUST)
endif

ifeq ($(TARGET_BOARD_PLATFORM),)
TARGET_BOARD_PLATFORM = default
endif

ifeq "$(TOOLCHAIN)" ""
endif

ifeq "$(CUST)" ""
LIBGPS = libgps
else
LIBGPS = libgps_$(TARGET)
endif

ifeq "$(OUT_DIR)" ""
OUT_DIR:=$(shell pwd)/$(dir $(lastword $(MAKEFILE_LIST)))/build_out
#OUT_DIR:=~/build_out
endif

ifeq "$(CUST)" ""
else
CMF = $(TARGET_DIR)/Makefile.$(TARGET)
ifneq ($(wildcard $(CMF).mak),)
CUSTMAKEFILE = $(CMF).mak
else
CUSTMAKEFILE = $(CMF)
endif
include $(CUSTMAKEFILE)
endif

export OUT_DIR TARGET_BOARD_PLATFORM CONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS

$(warning TOOLCHAIN is "$(TOOLCHAIN)")
$(warning ANDROIDPATH is "$(ANDROIDPATH)")
$(warning CUST is "$(CUST)")
$(warning PLATF is "$(PLATF)")
$(warning CUSTMAKEFILE is "$(CUSTMAKEFILE)")
$(warning OUT_DIR is "$(OUT_DIR)")

all:
ifeq (,$(filter $(TOOLCHAIN),"2_3", "4_0"))	
	@rm -f $(shell pwd)/../../../$(LIBGPS).so
else
	@rm -f $(CUSTOMER_TOP_DIR)/$(LIBGPS).so
endif
	$(MAKE) -C $(ANDROIDPATH) libxml2
	$(MAKE) -C $(ANDROIDPATH) liblog	
	$(MAKE) -C $(ANDROIDPATH) ONE_SHOT_MAKEFILE=$(shell pwd)/Android.mk files 
ifeq (,$(filter $(TOOLCHAIN),"2_3", "4_0"))	
	@echo "**************************** Android 2.3 or 4.0 ******************************"
	#@cp $(OUT_DIR)/target/product/generic/system/lib/hw/gps.$(TARGET_BOARD_PLATFORM) $(shell pwd)/../../../$(LIBGPS).so 2>/dev/null
else
	@echo "**************************** Android 2.2 or 2.1  ******************************"
	@cp $(OUT_DIR)/target/product/generic/system/lib/libgps.so $(shell pwd)/../../../$(LIBGPS).so
endif
    
clean:
ifeq (,$(filter $(TOOLCHAIN),"2_3", "4_0"))	
	@rm -f $(OUT_DIR)/target/product/generic/system/lib/hw/gps.$(TARGET_BOARD_PLATFORM)
else
	@rm -f $(OUT_DIR)/target/product/generic/system/lib/$(LIBGPS).so
endif
	@rm -f $(OUT_DIR)/target/product/generic/obj/SHARED_LIBRARIES/libgps_intermediates/gps_lcsapi.o
	@rm -f $(OUT_DIR)/target/product/generic/obj/SHARED_LIBRARIES/libgps_intermediates/gps_lcsapi.P
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/brcm_marshall.o
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/brcm_marshall.P
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/brcmipc_unixsocket.o
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/brcmipc_unixsocket.P
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/gps_api.o
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/gps_api.P
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/lbs.o
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/lbs.P
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/supl_api.o
	@rm -f $(OUT_DIR)/target/product/generic/obj/lcsapi/src/supl_api.P
