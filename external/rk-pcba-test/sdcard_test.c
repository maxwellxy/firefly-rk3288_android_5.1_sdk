#include <stdio.h>
#include <stdlib.h>
#include "extra-functions.h"
#include "common.h"
#include "sdcard_test.h"
#include "test_case.h"
#include "language.h"

#define LOG_TAG	"[PCBA,SDCARD]: "
#define LOG(x...)	printf(LOG_TAG x)

#define SCAN_RESULT_LENGTH	128
#define SCAN_RESULT_FILE	"/data/sd_capacity"
#define SD_INSERT_RESULT_FILE	"/data/sd_insert_info"

void *sdcard_test(void *argv)
{
	struct testcase_info *tc_info = (struct testcase_info *)argv;
	int ret, y;
	double cap;
	FILE *fp;
	char results[SCAN_RESULT_LENGTH];

	/* remind ddr test */
	if (tc_info->y <= 0)
		tc_info->y = get_cur_print_y();
	y = tc_info->y;

	LOG("start sdcard test.\n");
#ifdef SOFIA3GR_PCBA
	ui_print_xy_rgba(0, y, 255, 255, 0, 255, "%s:[%s..]\n", PCBA_SDCARD,
			 PCBA_TESTING);
#else
	ui_print_xy_rgba(0, y, 255, 255, 0, 255, "%s\n", PCBA_SDCARD);
#endif

#if defined(RK3288_PCBA) || defined(RK3368_PCBA)
	ret = __system("busybox chmod 777 /res/emmctester.sh");
#elif defined(SOFIA3GR_PCBA)
	/* sofia3gr process empty */
#else
	ret = __system("busybox chmod 777 /res/mmctester.sh");
#endif

#ifndef SOFIA3GR_PCBA
	if (ret)
		LOG("chmod mmctester.sh failed :%d\n", ret);
#endif

#if defined(RK3288_PCBA) || defined(RK3368_PCBA)
	ret = __system("/res/emmctester.sh");
#elif defined(SOFIA3GR_PCBA)
	int test_counts = 0;

	while (1) {
		fp = NULL;
		LOG("%s::wait for insert sd card...\n", __func__);
		if (__system("/system/bin/mmctester.sh") > 0)
			fp = fopen(SCAN_RESULT_FILE, "r");

		if (fp != NULL)
			break;

		if (test_counts++ > 3) {
			LOG("can not open %s.\n", SCAN_RESULT_FILE);
			ui_print_xy_rgba(0, y, 255, 0, 0, 255, "%s:[%s]\n",
					 PCBA_SDCARD, PCBA_FAILED);
			tc_info->result = -1;
			return argv;
		}
		sleep(1);
	}
	ui_print_xy_rgba(0, y, 0, 255, 0, 255, "%s:[%s]\n", PCBA_SDCARD,
			 PCBA_SECCESS);
	tc_info->result = 0;
	fclose(fp);

	return argv;
#else
	ret = __system("/res/mmctester.sh");
#endif

	if (ret < 0) {
		LOG("mmc test failed.\n");
		ui_print_xy_rgba(0, y, 255, 0, 0, 255, "%s:[%s]\n", PCBA_SDCARD,
				 PCBA_FAILED);
		tc_info->result = -1;
		return argv;
	}

	fp = fopen(SCAN_RESULT_FILE, "r");
	if (fp == NULL) {
		LOG("can not open %s.\n", SCAN_RESULT_FILE);
		ui_print_xy_rgba(0, y, 255, 0, 0, 255, "%s:[%s]\n", PCBA_SDCARD,
				 PCBA_FAILED);
		tc_info->result = -1;
		return argv;
	}

	memset(results, 0, SCAN_RESULT_LENGTH);
	fgets(results, 50, fp);

	cap = strtod(results, NULL);
	if (cap) {
		ui_print_xy_rgba(0, y, 0, 255, 0, 255, "%s:[%s] { %2fG }\n",
				 PCBA_SDCARD, PCBA_SECCESS,
				 cap * 1.0 / 1024 / 1024);
		tc_info->result = 0;
	}
	fclose(fp);

	return argv;
}
