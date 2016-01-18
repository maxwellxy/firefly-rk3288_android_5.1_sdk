#include <stdlib.h>
#include <sys/types.h>

#define LOG_TAG "OtgManager"
#include <cutils/log.h>

#include "OtgManager.h"


#define OTG_CTRL_FILE	"/sys/bus/platform/drivers/usb20_otg/force_usb_mode"
#define OTG_CFG_FILE	"/data/otg.cfg"

#define BUFFER_LENGTH	8

OtgManager::OtgManager() {
	int otgstatus;
	
	otgstatus = OtgReadCfg();
	if(otgstatus < 0) {
		ALOGE("OTG configuration file not exist, skip!");
		return;
	}
	
	OtgCtrl(otgstatus);
}

int OtgManager::OtgReadCfg() {
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];
	int otgstatus = -1;
	
	fd = fopen(OTG_CFG_FILE, "r");
	if(fd == NULL ) {
		ALOGE("%s not exist", OTG_CFG_FILE);
		return otgstatus;
	}
	memset(buf, 0, BUFFER_LENGTH);	
	if(fgets(buf, BUFFER_LENGTH, fd) != NULL) {
		otgstatus = atoi(buf);
	}
	fclose(fd);
	
	return otgstatus;
}

void OtgManager::OtgCtrl(int otgstatus) {
	FILE *fd = NULL;
	
	fd = fopen(OTG_CTRL_FILE, "w");
	if(fd == NULL ) {
		ALOGE("%s not exist", OTG_CTRL_FILE);
		return;
	}
	if(otgstatus == 1) {
		ALOGD("set otg to host mode.");
		fputc('1', fd);
	}
	else {
		ALOGD("set otg to slave mode.");
		fputc('2', fd);
	}
	fclose(fd);
}