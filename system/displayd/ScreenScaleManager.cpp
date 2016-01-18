#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#define LOG_TAG "ScreenScaleManager"
#include <cutils/log.h>
#include "Config.h"
#include "ScreenScaleManager.h"


#define PROPETY_OVERSCAN_MAIN	"persist.sys.overscan.main"
#define PROPETY_OVERSCAN_AUX	"persist.sys.overscan.aux"

#define MAIN_DISPLAY_SCALE_FILE	"/sys/class/graphics/fb0/scale"
#define AUX_DISPLAY_SCALE_FILE	"/sys/class/graphics/fb2/scale"


ScreenScaleManager::ScreenScaleManager() {
	InitSysNode();

	char property[PROPERTY_VALUE_MAX];

	int fd = open(MainDisplaySysNode, O_RDWR, 0);
	if(fd > 0) {
		if (property_get(PROPETY_OVERSCAN_MAIN, property, NULL) > 0)
			sscanf(property, "overscan %d,%d,%d,%d", &overscan_left, &overscan_top, &overscan_right, &overscan_bottom);
		if(overscan_left == 0)
			overscan_left = DEFALUT_SCREEN_SCALE;
		if(overscan_right == 0)
			overscan_right = DEFALUT_SCREEN_SCALE;
		if(overscan_top == 0)
			overscan_top = DEFALUT_SCREEN_SCALE;
		if(overscan_bottom == 0)
			overscan_bottom = DEFALUT_SCREEN_SCALE;
		
		memset(property, 0, PROPERTY_VALUE_MAX);
		sprintf(property, "overscan %d,%d,%d,%d", overscan_left, overscan_top, overscan_right, overscan_bottom);
//		write(fd, property, strlen(property));
		close(fd);
		property_set(PROPETY_OVERSCAN_MAIN, property);
	}
	fd = open(AuxDisplaySysNode, O_RDWR, 0);
	if(fd > 0) {
		if (property_get(PROPETY_OVERSCAN_AUX, property, NULL) > 0)
	    		sscanf(property, "overscan %d,%d,%d,%d", &overscan_left, &overscan_top, &overscan_right, &overscan_bottom);

		if(overscan_left == 0)
			overscan_left = DEFALUT_SCREEN_SCALE;
		if(overscan_right == 0)
			overscan_right = DEFALUT_SCREEN_SCALE;
		if(overscan_top == 0)
			overscan_top = DEFALUT_SCREEN_SCALE;
		if(overscan_bottom == 0)
			overscan_bottom = DEFALUT_SCREEN_SCALE;
		memset(property, 0, PROPERTY_VALUE_MAX);
		sprintf(property, "overscan %d,%d,%d,%d", overscan_left, overscan_top, overscan_right, overscan_bottom);
//		write(fd, property, strlen(property));
		close(fd);
		property_set(PROPETY_OVERSCAN_AUX, property);
	}
}

void ScreenScaleManager::InitSysNode(void) {
	memset(MainDisplaySysNode, 0, 64);
	memset(AuxDisplaySysNode, 0, 64);
	
	char const * const device_template = "/sys/class/graphics/fb%u/lcdcid";
	FILE *fd = NULL;
	int i = 0, id = 0, id_fb0 = -1;
	char name[64];
	
	do
	{
		ALOGD("i is %d", i);
		memset(name, 0, 64);
		snprintf(name, 64, device_template, i);
		fd = fopen(name, "r");
		if(fd != NULL) {
			memset(name, 0, 64);
			fgets(name, 64, fd);
			fclose(fd);
			id = atoi(name);
			if (id_fb0 < 0)
				id_fb0 = id;
			if(id == id_fb0 && strlen(MainDisplaySysNode) == 0) {
				snprintf(MainDisplaySysNode, 64, "/sys/class/graphics/fb%u/scale", i);
			}
			else if(id != id_fb0 && strlen(AuxDisplaySysNode) == 0) {
				snprintf(AuxDisplaySysNode, 64, "/sys/class/graphics/fb%u/scale", i);
			}
		}
		i++;
	} while (fd != NULL);
	
	if(strlen(MainDisplaySysNode) == 0)
		strcpy(MainDisplaySysNode, MAIN_DISPLAY_SCALE_FILE);
//	if(strlen(AuxDisplaySysNode) == 0) {
//		strcpy(AuxDisplaySysNode, AUX_DISPLAY_SCALE_FILE);
//	}
	ALOGD("MainDisplaySysNode is %s", MainDisplaySysNode);
	ALOGD("AuxDisplaySysNode is %s", AuxDisplaySysNode);
}

int ScreenScaleManager::SSMReadCfg() {
	return 0;
}

void ScreenScaleManager::SSMCtrl(int display, int direction, int scalevalue) {
	int fd = -1;
	char property[PROPERTY_VALUE_MAX];
	
	ALOGD("[%s] display %d, direction %d rate %d\n", __FUNCTION__, display, direction, scalevalue);
	if(display == 0)
		fd = open(MainDisplaySysNode, O_RDWR, 0);
	else
		fd = open(AuxDisplaySysNode, O_RDWR, 0);
	if(fd < 0) return;
	
	if (direction == DISPLAY_OVERSCAN_X) {
		overscan_left = scalevalue;
		overscan_right = scalevalue;
	} else if (direction == DISPLAY_OVERSCAN_Y) {
		overscan_top = scalevalue;
		overscan_bottom = scalevalue;
	} else if (direction == DISPLAY_OVERSCAN_LEFT) {
		overscan_left = scalevalue;
	} else if (direction == DISPLAY_OVERSCAN_RIGHT) {
		overscan_right = scalevalue;
	} else if (direction == DISPLAY_OVERSCAN_TOP) {
		overscan_top = scalevalue;
	} else if (direction == DISPLAY_OVERSCAN_BOTTOM) {
		overscan_bottom = scalevalue;
	} else if (direction == DISPLAY_OVERSCAN_ALL) {
		overscan_left = scalevalue;
		overscan_right = scalevalue;
		overscan_top = scalevalue;
		overscan_bottom = scalevalue;
	}
	
	memset(property, 0, PROPERTY_VALUE_MAX);
	sprintf(property, "overscan %d,%d,%d,%d", overscan_left, overscan_top, overscan_right, overscan_bottom);
	write(fd, property, strlen(property));
	close(fd);
	if(display == 0)
		property_set(PROPETY_OVERSCAN_MAIN, property);
	else
		property_set(PROPETY_OVERSCAN_AUX, property);
	system("sync");
	ALOGD("sync property scale\n");
}
