#ifeq ($(BOARD_USE_PPPOE),true)
  SOURCE_PPPOE_CONF_DIR := vendor/rockchip/common/pppoe/configs
  TARGET_PPPOE_CONF_DIR := system/etc/ppp

  PRODUCT_COPY_FILES += $(SOURCE_PPPOE_CONF_DIR)/pap-secrets:$(TARGET_PPPOE_CONF_DIR)/pppoe-secrets \
                        $(SOURCE_PPPOE_CONF_DIR)/pppoe.conf:$(TARGET_PPPOE_CONF_DIR)/pppoe.conf \
                        $(SOURCE_PPPOE_CONF_DIR)/ip-up-pppoe:$(TARGET_PPPOE_CONF_DIR)/ip-up-pppoe \
                        $(SOURCE_PPPOE_CONF_DIR)/ip-down-pppoe:$(TARGET_PPPOE_CONF_DIR)/ip-down-pppoe


  SOURCE_PPPOE_SCRIPT_DIR := vendor/rockchip/common/pppoe/scripts
  TARGET_PPPOE_script_DIR := system/bin

  PRODUCT_COPY_FILES += $(SOURCE_PPPOE_SCRIPT_DIR)/pppoe-stop:$(TARGET_PPPOE_script_DIR)/pppoe-stop \
                        $(SOURCE_PPPOE_SCRIPT_DIR)/pppoe-connect:$(TARGET_PPPOE_script_DIR)/pppoe-connect \
                        $(SOURCE_PPPOE_SCRIPT_DIR)/pppoe-setup:$(TARGET_PPPOE_script_DIR)/pppoe-setup \
                        $(SOURCE_PPPOE_SCRIPT_DIR)/pppoe-start:$(TARGET_PPPOE_script_DIR)/pppoe-start \
                        $(SOURCE_PPPOE_SCRIPT_DIR)/pppoe-status:$(TARGET_PPPOE_script_DIR)/pppoe-status \
                        $(SOURCE_PPPOE_SCRIPT_DIR)/pppd_pppoe:$(TARGET_PPPOE_script_DIR)/pppd_pppoe  
  
  SOURCE_PPPOE_BIN_DIR := vendor/rockchip/common/pppoe/bin
  TARGET_PPPOE_BIN_DIR := system/bin

  PRODUCT_COPY_FILES += $(SOURCE_PPPOE_BIN_DIR)/pppoe:$(TARGET_PPPOE_BIN_DIR)/pppoe \
			$(SOURCE_PPPOE_BIN_DIR)/pppoe-repay:$(TARGET_PPPOE_BIN_DIR)/pppoe-repay \
			$(SOURCE_PPPOE_BIN_DIR)/pppoe-sniff:$(TARGET_PPPOE_BIN_DIR)/pppoe-sniff 

  include $(all-subdir-makefiles)
#endif
