#ifndef __at_util_extern_H_
#define __at_util_extern_H_
extern int test_simcard();

extern int change_bootmode(int values);

extern int commit_pcba_test_value(int values);

extern void* getImei_testresult(void *argc);

#endif
