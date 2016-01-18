/*
 * custom.c
 *
 *  Created on: 2013-4-27
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
        E("cannot find '%s'", filename);
        return -1;
    }

    pid = fork();
    if (pid == 0) {
        setpgid(0, getpid());
        /* execute */
        execv(filename, argv);
        E("can't execv %s (%s)", filename, strerror(errno) );
        /* exit */
        _exit(0);
    }

    if (pid < 0) {
        E("failed to fork and start '%s'", filename);
        return -1;
    }

    if (-1 == waitpid(pid, &status, WCONTINUED | WUNTRACED)) {
        E("wait for child error");
        return -1;
    }

    D("executed '%s' return %d", filename, WEXITSTATUS(status) );
    return 0;
}

static int setProp(char *name, char *value) {
	char buf[128];
	char *cmd[6];
	uint length;

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/%s/d", name);            // "/<pattern>/d" : sed 的查找删除行 命令.  
	cmd[0] = "/sbin/busybox";
	cmd[1] = "sed";
	cmd[2] = "-i";
	cmd[3] = buf;
	cmd[4] = "system/build.prop";
	cmd[5] = NULL;
	D("%s %s %s %s %s ", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
	run(cmd[0], cmd);

	FILE *f = fopen("system/build.prop", "a");      // "a" : append.
	if(f == NULL) {
		E("fial to open system/build.prop, err : %s.", strerror(errno) ); 
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "\n%s=%s", name, value);
	length = strlen(name) + strlen(value) + 2;
	//fseek(f, 0, SEEK_END);
	if(length != fwrite(buf, 1, length, f)) {
		E("write prop error =============");
		fclose(f);
		return -1;
	}

	I("success to set property.");
	fclose(f);
	return 0;
}

void customHandler(char *command, int argc, char **argv) {
	char *cmd[10];
	char *str;
	char *str1;
	char str3[128];

	if(!strcmp(command, "cp") || !strcmp(command, "CP")) {
		str = strrchr(argv[1], '/');
		str1 = strndup(argv[1], str - argv[1]);
		strcpy(str3, "cust/backup/");
		strcat(str3, str1);
		D("to mkdir '%s'.", str3);
		//mkdir des target
		cmd[0] = "/sbin/busybox";
		cmd[1] = "mkdir";
		cmd[2] = "-p";
		cmd[3] = str3;
		cmd[4] = NULL;
		run(cmd[0], cmd);

		//backup des target. 实际上备份 fw_in_cur_ota_ver. 
		cmd[1] = "cp";
		cmd[2] = "-a";      // .! : -a, --archive : same as -dR --preserve=all  
		cmd[3] = argv[1];
		cmd[4] = str3;
		cmd[5] = NULL;
		run(cmd[0], cmd);

		//cp src to des
		cmd[3] = argv[0];
		cmd[4] = argv[1];
		cmd[5] = NULL;
		run(cmd[0], cmd);

		//chmod des
		cmd[1] = "chmod";
		cmd[2] = argv[2];
		cmd[3] = argv[1];
		cmd[4] = NULL;
		run(cmd[0], cmd);

		free(str1);
	}else if(!strcmp(command, "rm") || !strcmp(command, "RM")) {
		str = strrchr(argv[0], '/');
		str1 = strndup(argv[0], str - argv[0]);
		strcpy(str3, "cust/backup/");
		strcat(str3, str1);
		D("to mkdir '%s'.", str3);
		//mkdir des target
		cmd[0] = "/sbin/busybox";
		cmd[1] = "mkdir";
		cmd[2] = "-p";
		cmd[3] = str3;
		cmd[4] = NULL;
		run(cmd[0], cmd);

		//backup src target
		cmd[1] = "cp";
		cmd[2] = "-a";
		cmd[3] = argv[0];
		cmd[4] = str3;
		cmd[5] = NULL;
		run(cmd[0], cmd);

		//rm src
		cmd[1] = "rm";
		cmd[2] = argv[0];
		cmd[3] = NULL;
		run(cmd[0], cmd);

		free(str1);
	}else if(!strcmp(command, "set") || !strcmp(command, "SET")) {
		setProp(argv[0], argv[1]);
	}
}


int saveBoardIdToFile(char *board_id) {
	FILE *fp_id;
	fp_id = fopen("cust/last_board_id", "w");
	if(fp_id == NULL) {
		E("fial to open file last_board_id; err : %s.", strerror(errno) );
		return -1;
	}

	uint length = DEVICE_NUM_TYPES;
	D("length:%d", length);
	uint i;
	for(i = 0; i < length; i++) {
		D("board-id: %d.", *(board_id+i));
	}

	if(1 != fwrite(&length, 4, 1, fp_id)) {
		E("fail to write length, err : %s.", strerror(errno) );
		fclose(fp_id);
		return -1;
	}

	if(length != fwrite(board_id, 1, length, fp_id)) {
		E("fail to write board id, err = %s.", strerror(errno) );
		fclose(fp_id);
		return -1;
	}

	I("save board id success!");
	fclose(fp_id);
	return 0;
}

int backupProp() {
	char *cmd[10];

	cmd[0] = "/sbin/busybox";
	cmd[1] = "mkdir";
	cmd[2] = "-p";
	cmd[3] = "cust/backup/system";
	cmd[4] = NULL;
	run(cmd[0], cmd);

	cmd[1] = "cp";
	cmd[2] = "-a";
	cmd[3] = "system/build.prop";
	cmd[4] = "cust/backup/system/build.prop";
	cmd[5] = NULL;
	run(cmd[0], cmd);

	I("backup build.prop complete!");
	return 0;
}

int custom() {

	char area[32];
	char language[32];
	char local[32];	
	char geo[32];	
	char timezone[32];	
	char user_define[32];
	
	char operator[32];
	char reserve[32];
	FILE *fp_area;

	I("*********** start custom ***************");
	board_id_open_device();

	//get language from ioctrl
	memset(area, 0, sizeof(area));
	memset(language, 0, sizeof(language));
	memset(local, 0, sizeof(local));	
	memset(geo, 0, sizeof(geo));
	memset(timezone, 0, sizeof(timezone));
	memset(user_define, 0, sizeof(user_define));
	
	memset(operator, 0, sizeof(operator));
	memset(reserve, 0, sizeof(reserve));

	board_id_get_operator_name(DEVICE_TYPE_OPERATOR, local, operator);
	board_id_get_reserve_name(DEVICE_TYPE_RESERVE, local, reserve);
	board_id_get_locale_region(DEVICE_TYPE_AREA, area, language, local, geo, timezone, user_define);
	D("get area form ioctrl: area=%s, language=%s, local=%s, timezone=%s, operator=%s, reserve=%s",
	    area,
        language,
        local,
        timezone,
        operator,
        reserve);

	fp_area = fopen("/cust/cust.xml", "r");
	if(fp_area == NULL) {
		E("fial to open area.xml, err : %s!", strerror(errno) );
		board_id_close_device();
		return -1;
	}

	//must backup build.prop first.  .R : 定制操作中, /system/build.prop 将以 line 为单位被修改. 
	backupProp();

    // .KP : 在解析 /cust/cust.xml 的流程中, 同时完成对应的 定制化 (custom) 处理. 这里使用策略回调的形式. 
	if(parse_area(fp_area, area, local, language, operator, reserve, customHandler)) {
		E("=============> parse area.xml error <===========");
		fclose(fp_area);
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
		E("fail to open device.xml, err : %s!", strerror(errno) );
		board_id_close_device();
		return -1;
	}

    /* 循环地, 从内核获取 所有 设备的 字串形态的 type 和 dev_name. */
	for(i = DEVICE_TYPE_TP; i < DEVICE_NUM_TYPES; i++) {
		memset(type, 0, sizeof(type));
		memset(dev, 0, sizeof(dev));
		if(board_id_get_device_name(i, type, dev)) {
			E("===========> get device info error <===========");
			fclose(fp_device);
			board_id_close_device();
			return -1;
		}

		D("get device info from ioctrl: type=%s, dev=%s ", type, dev);

        // .KP : 在解析 /cust/devicie.xml 的流程中, 同时完成对应的 定制化 (custom) 处理, 
		if(parse_device(fp_device, dev, type, customHandler)) {
			E("===========> parse device.xml error <==========");
			fclose(fp_device);
			board_id_close_device();
			return -1;
		}
	}

	char board_id[DEVICE_NUM_TYPES];

	memset(board_id, 0, sizeof(board_id));
	board_id_get(board_id);
	if(saveBoardIdToFile(board_id)) {
		E("save board id to file error! ");
		fclose(fp_device);
		board_id_close_device();
		return -1;
	}

	fclose(fp_device);
	board_id_close_device();
	return 0;
}



