#ifndef _SCREEN_TEST_H_
#define _SCREEN_TEST_H_

extern  void* screen_test(void *argc);
struct  screen_msg{
	int result;  //test result
	int x;	     //x,y,w,h
	int y;
	int w;
	int h;
};

#endif
