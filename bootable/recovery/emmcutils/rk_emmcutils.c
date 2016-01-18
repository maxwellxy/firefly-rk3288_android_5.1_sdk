/*
 * emmcutils.c
 *
 *  Created on: 2013-7-30
 *      Author: mmk@rock-chips.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "rk_emmcutils.h"
#include "cutils/properties.h"

int getEmmcState() {
	char bootmode[256];
	int result = 0;

	property_get("ro.bootmode", bootmode, "unknown");
	printf("bootmode = %s \n", bootmode);
	if(!strcmp(bootmode, "emmc")) {
		result = 1;
	}else {
		result = 0;
	}

	return result;
}


int transformPath(const char *in, char *out) {
	if(in == NULL || out == NULL) {
		printf("transformPath argument can't be NULL\n");
		return -1;
	}

	printf("transformPath in: %s\n", in);
	strcpy(out, "dev/block/platform/ff0f0000.rksdmmc/by-name/");
	strcat(out, in);

	return 0;
}




