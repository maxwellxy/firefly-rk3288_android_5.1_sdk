#ifndef _RK_GSENSOR_H
#define _RK_GSENSOR_H
void* gsensor_test(void *argv);
void* gsensor_test_mpu(void *argv);
void* compass_test_mpu(void *argv);



struct gsensor_msg
{
	int result;
	int y;
};


#endif
