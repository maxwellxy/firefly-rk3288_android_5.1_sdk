/*
 * restore.c
 *
 *  Created on: 2013-5-8
 *      Author: mmk
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parse_xml.h"
#include "board_id_ctrl.h"

#include "common.h"

static int run(const char *filename, char *const argv[])
{
    struct stat s;
    int status;
    pid_t pid;

    if (stat(filename, &s) != 0) {
        E("cannot find '%s', err : %s", filename, strerror(errno) );
        return -1;
    }

    pid = fork();
    if (pid == 0) {
        setpgid(0, getpid());
        /* execute */
        execv(filename, argv);
        LOGE("can't run %s (%s)\n", filename, strerror(errno));
        /* exit */
        _exit(0);
    }

    if (pid < 0) {
        LOGE("failed to fork and start '%s'\n", filename);
        return -1;
    }

    if (-1 == waitpid(pid, &status, WCONTINUED | WUNTRACED)) {
        LOGD("wait for child error\n");
        return -1;
    }

    LOGI("executed '%s' return %d\n", filename, WEXITSTATUS(status));
    return 0;
}


void restoreHandler(char *command, int argc, char **argv) {
	char *cmd[10];
	char *str;
	char *str1;
	char str3[128];

	if(!strcmp(command, "cp") || !strcmp(command, "CP")) {
		strcpy(str3, "cust/backup/");
		strcat(str3, argv[1]);

		//rm des target
		cmd[0] = "/sbin/busybox";
		cmd[1] = "rm";
		cmd[2] = argv[1];
		cmd[3] = NULL;
		run(cmd[0], cmd);

		//recover from backup
		cmd[1] = "cp";
		cmd[2] = "-a";
		cmd[3] = str3;
		cmd[4] = argv[1];
		cmd[5] = NULL;
		run(cmd[0], cmd);

	}else if(!strcmp(command, "rm") || !strcmp(command, "RM")) {
		strcpy(str3, "cust/backup/");
		strcat(str3, argv[0]);

		//recovery from backup
		cmd[0] = "/sbin/busybox";
		cmd[1] = "cp";
		cmd[2] = "-a";
		cmd[3] = str3;
		cmd[4] = argv[0];
		cmd[5] = NULL;
		run(cmd[0], cmd);
	}
}

char *readBoardIdFromFile() {
	FILE *fp_id;
	char *result;
	fp_id = fopen("cust/last_board_id", "r");
	if(fp_id == NULL) {
		E("fial to open last_board_id, err : %s.", strerror(errno) );
		return NULL;
	}

	int length;
	if(1 != fread(&length, 4, 1, fp_id)) {
		E("fail to read board id length, err : %s.", strerror(errno) );
		fclose(fp_id);
		return NULL;
	}

	D("length:%d", length);

	result = malloc(length);
	memset(result, 0, length);
	if(length != fread(result, 1, length, fp_id)) {
		E("fail to read board id, err : %s.", strerror(errno) );
		free(result);
		fclose(fp_id);
		return NULL;
	}


	int i;
	for(i = 0; i < length; i++) {
		D("board-id: %d", *(result+i));
	}

	fclose(fp_id);
	return result;
}

int cleanBackup() {
	char *cmd[10];

	cmd[0] = "/sbin/busybox";
	cmd[1] = "rm";
	cmd[2] = "-rf";
	cmd[3] = "cust/backup/";
	cmd[4] = NULL;
	run(cmd[0], cmd);

	cmd[3] = "cust/last_board_id";
	run(cmd[0], cmd);

	I("clean backup files complete!");
	return 0;
}

int recoverProp() {
	char *cmd[10];

	cmd[0] = "/sbin/busybox";
	cmd[1] = "cp";
	cmd[2] = "-a";
	cmd[3] = "cust/backup/system/build.prop";
	cmd[4] = "system/build.prop";
	cmd[5] = NULL;
	run(cmd[0], cmd);

	I("recover build.prop complete!");
	return 0;
}

int restore() {
	char *board_id;

	printf("*************  BOARD_ID version 0.92 ******************\n");
	printf("************* start restore **************************\n");

	board_id_open_device();

	board_id = readBoardIdFromFile();
	if(board_id == NULL) {
		E("no find last board id, so not restore....");
		board_id_close_device();
		return 0;
	}

	//restore
	char area[32];
	char language[32];
	char local[32];	
	char geo[32];	
	char timezone[32];	
	char user_define[32];
	
	char operator[32];
	char reserve[32];
	FILE *fp_area;

	//get language from ioctrl
	memset(area, 0, sizeof(area));
	memset(language, 0, sizeof(language));
	memset(local, 0, sizeof(local));	
	memset(geo, 0, sizeof(geo));
	memset(timezone, 0, sizeof(timezone));
	memset(user_define, 0, sizeof(user_define));
	
	memset(operator, 0, sizeof(operator));
	memset(reserve, 0, sizeof(reserve));


	board_id_get_operator_name_by_id(DEVICE_TYPE_OPERATOR, board_id, local, operator);
	board_id_get_reserve_name_by_id(DEVICE_TYPE_RESERVE, board_id, local, reserve);
	board_id_get_locale_region_by_id(DEVICE_TYPE_AREA, board_id, area, language, local, geo, timezone, user_define);
	D("get area form ioctrl: area=%s, language=%s, local=%s, timezone=%s, operator=%s, reserve=%s",
	    area,
        language,
        local,
        timezone,
        operator,
        reserve);

	fp_area = fopen("/cust/cust.xml", "r");
	if(fp_area == NULL) {
		E("fail to open area.xml. err : %s.", strerror(errno) );
		free(board_id);
		board_id_close_device();
		return -1;
	}

	if(parse_area(fp_area, area, local, language, operator, reserve, restoreHandler)) {
		E("=============> parse area.xml error <===========");
		fclose(fp_area);
		free(board_id);
		board_id_close_device();
		return -1;
	}

	fclose(fp_area);

	//get devices from ioctrl
	int i;
	char type[32];
	char dev[32];
	FILE *fp_device;

	fp_device = fopen("/cust/device.xml", "r");
	if(fp_device == NULL) {
		E("fail to open device.xml, err : %s.", strerror(errno) );
		free(board_id);
		board_id_close_device();
		return -1;
	}

	for(i = DEVICE_TYPE_TP; i < DEVICE_NUM_TYPES; i++) {
		memset(type, 0, sizeof(type));
		memset(dev, 0, sizeof(dev));
		if(board_id_get_device_name_by_id(i, board_id, type, dev)) {
			E("===========> get device info error <===========");
			fclose(fp_device);
			free(board_id);
			board_id_close_device();
			return -1;
		}

		D("get device info from ioctrl: type=%s, dev=%s ", type, dev);
		if(parse_device(fp_device, dev, type, restoreHandler)) {
			E("===========> parse device.xml error <==========");
			fclose(fp_device);
			free(board_id);
			board_id_close_device();
			return -1;
		}
	}

    /* 恢复之前保存的 /system/build.prop 的 ota 版本. */
	recoverProp();
	cleanBackup();

	free(board_id);
	fclose(fp_device);
	board_id_close_device();
	return 0;
}



