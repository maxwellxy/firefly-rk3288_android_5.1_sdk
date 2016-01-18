/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "wltest"

#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>		/* for utimes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utime.h>
#include <fcntl.h>

#include <cutils/log.h>
#include "common.h"
#include "extra-functions.h"
#include "lan_test.h"
#include "test_case.h"
#include "script.h"
#include "language.h"

#define LOG(x...) printf(x)

#define MAX_PING_COUNTS (5)
#define PING_RESULT_LENGTH (128*MAX_PING_COUNTS)
#define PING_RESULT_FILE "/data/ether_result.txt"

/*By view local address*/
int process_view_localaddr(char *src)
{
	char *p;

	/*inet6 addr: fe80::7431:a1ff:fe6d:90b6/64 Scope:Link */
	p = strstr(src, "addr:");
	if (p == NULL)
		return 0;
	/* skip "addr:" */
	p += strlen("addr:");
	/* skip " " */
	if (*p == ' ')
		p++;

	char tmp[64];
	int k = 0;

	while (*p != ' ') {
		if ((*p == '\0') || (*p == '\n'))
			return 0;
		if (k >= 64)
			return 0;
		tmp[k++] = *p++;
	}
	/*
	   * Except 127.0.0.1 and ::1/128
	 */
	if (k > 5) {
		char *local1 = strstr(tmp, "127.0.0.1");
		char *local2 = strstr(tmp, "::1/128");
		char *local3 = strstr(tmp, ".");
		char *local4 = strstr(tmp, ":");

		if (local1 == NULL && local2 == NULL &&
		    (local3 != NULL || local4 != NULL)) {
			return 1;
		}
	}
	return 0;
}

/*By ping statistics*/
int process_ping_statistics(char *src)
{
	char *p;
	int losepercent;

	/*5 packets transmitted, 5 packets received, 0% packet loss */
	p = strstr(src, "packets received, ");
	if (p == NULL)
		return 0;
	/* skip "packets received, " */
	p += strlen("packets received, ");
	char tmp[4];
	int k = 0;

	while (*p != '%') {
		if ((*p == '\0') || (*p == '\n'))
			return 0;
		tmp[k++] = *p++;
	}
	losepercent = atoi(tmp);
	if (losepercent < 100)
		return 1;
	else
		return 0;
}

/*By seqno*/
int process_ping_seq(char *src)
{
	char *p;
	int seqno = 0, tseqno = 0, i = 0;
	int presult;

	for (i = 0; i < MAX_PING_COUNTS; i++) {
		/*64 bytes from 172.16.7.1: seq=0 ttl=255 time=7.181 ms */
		p = strstr(src, "seq=");
		if (p == NULL)
			return 0;
		/* skip "seq=" */
		p += strlen("seq=");
		char tmp[4];
		int k = 0;

		while (*p != ' ') {
			if ((*p == '\0') || (*p == '\n'))
				return 0;
			tmp[k++] = *p++;
		}
		tseqno = atoi(tmp);
		if (seqno == 0) {	/*First time */
			seqno = tseqno;
		} else if (seqno + 1 == tseqno) {	/*Continuous seqno */
			seqno = tseqno;
			presult = 1;
		} else {
			return 0;
		}
		src = p;
	}
	return presult;
}

/*
*default use ping to test lan.
*if use local_addr to test,logout " use_ping  = 1 "
*/
void *lan_test(void *argv)
{
	int ret;
	FILE *fp = NULL;
	char *results = NULL;
	struct testcase_info *tc_info = (struct testcase_info *)argv;

	/*remind ddr test */
	if (tc_info->y <= 0)
		tc_info->y = get_cur_print_y();
	ui_print_xy_rgba(0, tc_info->y, 255, 255, 0, 255, "%s:[%s..]\n",
			 PCBA_LAN, PCBA_TESTING);

	char local_addr[32], ping_addr[32];
	int use_ping;

	memset(local_addr, 0, 32);
	memset(ping_addr, 0, 32);
	script_fetch("lan", "local_addr", (int *)local_addr, 32 / 4);
	script_fetch("lan", "ping_addr", (int *)ping_addr, 32 / 4);
	script_fetch("lan", "use_ping", &use_ping, 1);
	use_ping = 1;		/*default use ping to test lan. */

	ret = __system("busybox chmod 777 /res/ethernet.sh");
	if (ret)
		LOG("chmod ethernet.sh failed :%d\n", ret);

	char shpath[128];

	sprintf(shpath, "/res/ethernet.sh %s %s %d", local_addr, ping_addr,
		use_ping);
	LOG("ethernet test cmd %s\n", shpath);
	ret = __system(shpath);
	if (ret <= 0) {
		LOG("ethernet test failed.\n");
		goto error_exit;
	}

	results = malloc(PING_RESULT_LENGTH);
	if (results == NULL) {
		LOG("can malloc results buffer.\n");
		goto error_exit;
	}

	fp = fopen(PING_RESULT_FILE, "r");
	if (fp == NULL) {
		LOG("can not open %s.\n", PING_RESULT_FILE);
		goto error_exit;
	}
	memset(results, 0, PING_RESULT_LENGTH);
	fread(results, PING_RESULT_LENGTH, 1, fp);
	results[PING_RESULT_LENGTH - 1] = '\0';

	int result;

	if (use_ping == 1)
		result = process_ping_statistics(results);
	else
		result = process_view_localaddr(results);
	if (result) {
		ui_print_xy_rgba(0, tc_info->y, 0, 255, 0, 255, "%s:[%s]\n",
				 PCBA_LAN, PCBA_SECCESS);
	} else {
		ui_print_xy_rgba(0, tc_info->y, 255, 0, 0, 255, "%s:[%s]\n",
				 PCBA_LAN, PCBA_FAILED);
	}
	if (fp != NULL)
		fclose(fp);

	if (results != NULL)
		free(results);
	LOG("lan_test success.\n");
	return 0;

error_exit:

	LOG("lan_test failed.\n");

	if (fp != NULL)
		fclose(fp);

	if (results != NULL)
		free(results);

	ui_print_xy_rgba(0, tc_info->y, 255, 0, 0, 255, "%s:[%s]\n", PCBA_LAN,
			 PCBA_FAILED);
	tc_info->result = -1;

	return argv;
}
