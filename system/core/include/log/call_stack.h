

#ifndef __CALL_STACK_H__
#define __CALL_STACK_H__

#include <utils/Log.h>
#include <utils/CallStack.h>

#define DUMP_CALL_STACK() \
{ \
    CallStack stack; \
    stack.update(); \
    stack.log(LOG_TAG); \
}

#endif /* __CALL_STACK_H__ */


