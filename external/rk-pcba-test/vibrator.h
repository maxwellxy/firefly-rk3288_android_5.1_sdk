#ifndef __VIBRATOR_TEST_H
#define __VIBRATOR_TEST_H
void * vibrator_test(void * argv);

struct vibrator_msg {
	int result;
	int x;
	int y;
	int w;
	int h;
};

#endif

