#ifndef __FLASHLIGHT_TEST_H
#define __FLASHLIGHT_TEST_H
void * flashlight_test(void * argv);
enum xgold_flash_enable_t {
    FLASH_OFF = 0, /* flash mode settings */
    FLASH_ON,
    FLASH_TORCH,
    FLASH_AUTO
};

struct flashlight_msg {
	int result;
	int x;
	int y;
	int w;
	int h;
};

#endif
