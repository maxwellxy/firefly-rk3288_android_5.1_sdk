/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * mkparameter.c
 *
 *  Created on: 2012-6-15
 *      Author: mmk@rock-chips.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "crc.h"

#define PARM_TAG 0x4D524150

#define FILE_OPEN_RO        "rb"
#define FILE_OPEN_RW        "r+b"
#define FILE_OPEN_RW_CREATE "w+b"

/*RK parameter format
typedef struct {
	unsigned int uiParmTag;
	unsigned int uiParmLength;
	unsigned char* pucParmRealData;
	unsigned int uiParmCRC32Data;
}RK_Parameter, *RK_Parameter;
*/

/**************************************************************
 *This is parameter packaging tool, the format of command is:
 *
 *mkparameter [src_path] [target_path]
 */
int main(int argc, char **argv) {
	if(argc != 3) {
		printf("args error! the command is: mkparameter [src_path] [target_path]\n");
		return -1;
	}

	printf("src_path: %s, target_path: %s\n", argv[1], argv[2]);

	char* src_path = argv[1]; //original parameter file
	char* target_path = argv[2];

	FILE* fp_src = fopen(src_path, FILE_OPEN_RO);
	if(!fp_src) {
		printf("can't open file %s!\n", src_path);
		return -1;
	}

	struct stat srcFileInfo;
	stat(src_path, &srcFileInfo);

	unsigned int uiParmLength = srcFileInfo.st_size;
	printf("the original parameter size: %u\n", uiParmLength);

	unsigned char* buffer = malloc(uiParmLength + 20);
	*(unsigned int*)buffer = PARM_TAG;
	*(unsigned int*)(buffer + 4) = uiParmLength;

	unsigned int readCount = fread(buffer + 8, 1, uiParmLength, fp_src);
	if(readCount != uiParmLength) {
		printf("read original parameter error!\n");
		fclose(fp_src);
		free(buffer);
		return -1;
	}

	*(unsigned int*)(buffer + 8 + uiParmLength) = CRC_32(buffer + 8, uiParmLength);

	fclose(fp_src);

	//write buffer to target file
	FILE* fp_target = fopen(target_path, FILE_OPEN_RW_CREATE);
	if(!fp_target) {
		printf("can't open file %s\n", target_path);
		free(buffer);
		return -1;
	}

	if(fwrite(buffer, 1, uiParmLength + 12, fp_target) != uiParmLength + 12) {
		printf("write file fail!");
		fclose(fp_target);
		free(buffer);
		return -1;
	}

	fclose(fp_target);
	free(buffer);
	return 0;
}





