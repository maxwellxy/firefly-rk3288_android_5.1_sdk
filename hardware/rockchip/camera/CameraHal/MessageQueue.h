#ifndef __MESSAGEQUEUE_H__
#define __MESSAGEQUEUE_H__

#include "CameraHal_Tracer.h"
namespace android {
struct Message_cam
{
    unsigned int command;
    void*        arg1;
    void*        arg2;
    void*        arg3;
    void*        arg4;
    Message_cam(){
        arg1 = NULL;
        arg2 = NULL;
        arg3 = NULL;
        arg4 = NULL;
    }
};
class MessageQueue
{
public:
    MessageQueue();
    MessageQueue(const char *name);
	~MessageQueue();			/* ddl@rock-chips.com */
    int get(Message_cam*);
	int get(Message_cam*, int);		/* ddl@rock-chips.com : timeout interface */
    int put(Message_cam*);
    bool isEmpty();
    int dump();
private:
    char MsgQueName[30];
    int fd_read;
    int fd_write;
};
}
#endif

