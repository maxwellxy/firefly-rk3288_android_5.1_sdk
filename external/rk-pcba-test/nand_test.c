
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "extra-functions.h"
#include "test_case.h"
#include "language.h"
#include "nand_test.h"
#define LOG(x...) printf("[Wifi_TEST] "x)
void *nand_test(void *argv)
{
	FILE *stream;
	char msg[1024], *buf, *target = "Device Capacity:";
	int y;
	struct testcase_info *tc_info = (struct testcase_info *)argv;

	if (tc_info->y <= 0)
		tc_info->y = get_cur_print_y();

	y = tc_info->y;

	ui_print_xy_rgba(0, y, 255, 255, 0, 255, "%s:[%s..]\n", PCBA_NAND,
			 PCBA_TESTING);

	stream = fopen("proc/rknand", "r+");/*open nandflash message*/
	if (!stream) {
		perror("fopen");
		return argv;
	}

	do {
		if (fgets(msg, 1024, stream) == NULL)

			goto err;

		buf = msg;

	} while (strncmp(target, buf, 16) != 0);

	ui_print_xy_rgba(0, y, 0, 255, 0, 255, "%s:[%s] %s\n", PCBA_NAND,
			 PCBA_SECCESS, msg);

	fclose(stream);

	return argv;

err:
	ui_print_xy_rgba(0, y, 0, 255, 0, 255, "%s:[%s] %s\n", PCBA_NAND,
			 PCBA_FAILED, msg);

	fclose(stream);

	return argv;
}