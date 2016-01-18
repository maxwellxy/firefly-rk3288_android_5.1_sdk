
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>

#include <utils/Log.h>
#include "CameraHal.h"
#include "MessageQueue.h"
namespace android {

static char gDisplayThreadCommands[][30] = {
		{"CMD_DISPLAY_PAUSE"},
        {"CMD_DISPLAY_START"},
        {"CMD_DISPLAY_STOP"},
		{"CMD_DISPLAY_FRAME"}
};
static char gPreviewThreadCommands[][30] = {
        {"CMD_PREVIEW_THREAD_PAUSE"},        
        {"CMD_PREVIEW_THREAD_START"},
        {"CMD_PREVIEW_THREAD_STOP"},
        {"CMD_PREVIEW_VIDEOSNAPSHOT"}
};
static char gSnapshotThreadCommands[][30] = {
        {"CMD_SNAPSHOT_SNAPSHOT"},
        {"CMD_SNAPSHOT_EXIT"}
};
static char gCommandThreadCommands[][30] = {
		// Comands
        {"CMD_PREVIEW_START"},
        {"CMD_PREVIEW_STOP"},
        {"CMD_PREVIEW_CAPTURE"},
        {"CMD_PREVIEW_CAPTURE_CANCEL"},
        {"CMD_PREVIEW_QBUF"},        
        
        {"CMD_AF_START"},
        {"CMD_AF_CANCEL"},
        
        {"CMD_EXIT"}

 };
static char gThreadCmdArgs[][30] = {
        {"CMDARG_ERR"},
        {"CMDARG_OK"},        
        {"CMDARG_ACK"},
        {"CMDARG_NACK"}     
    };
static char gInvalCommands[]={"CMD_UNKNOW"};
static char gInvalArg[]={"CMDARG_UNKNOW"};
static char* MessageCmdConvert(char* msgQ, unsigned int cmd)
{    
    char *cmd_name = gInvalCommands;
    if (strstr(msgQ,"display")) {
        if (cmd < sizeof(gDisplayThreadCommands)/30) 
            cmd_name = (char*)gDisplayThreadCommands[cmd];
    } else if (strstr(msgQ,"preview")) {
        if (cmd < sizeof(gPreviewThreadCommands)/30) 
            cmd_name = (char*)gPreviewThreadCommands[cmd];
    } else if (strstr(msgQ,"command")) {
        if (cmd < sizeof(gCommandThreadCommands)/30) 
            cmd_name = (char*)gCommandThreadCommands[cmd];
    } else if (strstr(msgQ,"snapshot")) {
        if (cmd < sizeof(gSnapshotThreadCommands)/30) 
            cmd_name = (char*)gSnapshotThreadCommands[cmd];
    }
    return cmd_name;
}
static char* MessageArg1Convert(char* msgQ, Message_cam *msg)
{    
    char *arg_name = gInvalArg,*cmd_name=gInvalCommands;
    unsigned long arg_val = (unsigned long)msg->arg1;

    if (strstr(msgQ,"AckQ")) {
        if (arg_val<1)
            arg_name = gThreadCmdArgs[arg_val+1];
    } else {
        if (strstr(msgQ,"command")) {
            if (msg->command < sizeof(gCommandThreadCommands)/30) 
                cmd_name = (char*)gCommandThreadCommands[msg->command];

            if (strcmp(cmd_name,"CMD_PREVIEW_QBUF")==0)
                goto MessageArg1Convert_end;
        }

        if ((arg_val>=1) && ((arg_val+1)<(sizeof(gThreadCmdArgs)/30))) 
            arg_name = gThreadCmdArgs[arg_val+1];
    }
    
MessageArg1Convert_end:
    return arg_name;
}
MessageQueue::MessageQueue()
{
    int fds[2] = {-1,-1};
    
    pipe(fds);
    this->fd_read = fds[0];
    this->fd_write = fds[1];
    MsgQueName[0] = 0;
    strcat(MsgQueName, "CamMsgQue");
}
MessageQueue::MessageQueue(const char *name)
{
    int fds[2] = {-1,-1};

    pipe(fds);

    this->fd_read = fds[0];
    this->fd_write = fds[1];
    MsgQueName[0] = 0;
    strcat(MsgQueName, name);
    LOG1("%s create",name);
}
MessageQueue::~MessageQueue()           /* ddl@rock-chips.com */
{
    LOG1("%s destory",this->MsgQueName);
    close(this->fd_read);
    close(this->fd_write);

    this->fd_read = -1;
    this->fd_write = -1;
}

int MessageQueue::get(Message_cam* msg)
{
    char* p = (char*) msg;
    unsigned int read_bytes = 0;

    while( read_bytes  < sizeof(msg) )
    {
        int err = read(this->fd_read, p, sizeof(*msg) - read_bytes);

        if( err < 0 ) {
            LOGE("%s.get error: %s", this->MsgQueName,strerror(errno));
            return -1;
        }
        else
            read_bytes += err;
    }

    if (((strstr(this->MsgQueName,"display"))&&(strcmp(MessageCmdConvert(this->MsgQueName,msg->command),"CMD_DISPLAY_FRAME")==0)) ||
        ((strstr(this->MsgQueName,"command"))&&(strcmp(MessageCmdConvert(this->MsgQueName,msg->command),"CMD_PREVIEW_QBUF")==0))) {
        LOG2("%s.get(%s,%p,%p,%p,%p)", this->MsgQueName, MessageCmdConvert(this->MsgQueName,msg->command), msg->arg1,msg->arg2,msg->arg3,msg->arg4);
    } else {
        LOG2("%s.get(%s(0x%x),%s(%p),%p,%p,%p)", this->MsgQueName, MessageCmdConvert(this->MsgQueName,msg->command),
            msg->command, MessageArg1Convert(this->MsgQueName,msg),msg->arg1,msg->arg2,msg->arg3,msg->arg4);
    }

    return 0;
}

int MessageQueue::get(Message_cam* msg, int timeout)
{
    char* p = (char*) msg;
    unsigned int read_bytes = 0;
    int err = 0;
    struct pollfd pfd;

    pfd.fd = this->fd_read;
    pfd.events = POLLIN;

    while( read_bytes  < sizeof(msg) )
    {
        pfd.revents = 0;
        err = poll(&pfd,1,timeout);

        if (err == 0) {
            LOGE("%s.get_timeout error: %s", this->MsgQueName,strerror(errno));
            return -1;
        }


        if (pfd.revents & POLLIN) {
            err = read(this->fd_read, p, sizeof(*msg) - read_bytes);

            if( err < 0 ) {
                LOGE("%s.get_timeout error: %s", this->MsgQueName,strerror(errno));
                return -1;
            } else {
                read_bytes += err;
            }
        }
    }

    if (((strstr(this->MsgQueName,"display"))&&(strcmp(MessageCmdConvert(this->MsgQueName,msg->command),"CMD_DISPLAY_FRAME")==0)) ||
        ((strstr(this->MsgQueName,"command"))&&(strcmp(MessageCmdConvert(this->MsgQueName,msg->command),"CMD_PREVIEW_QBUF")==0))) {
        LOG2("%s.get_timeout(%s,%p,%p,%p,%p)",this->MsgQueName,  MessageCmdConvert(this->MsgQueName,msg->command), msg->arg1,msg->arg2,msg->arg3,msg->arg4);
    } else {
        LOG2("%s.get_timeout(%s(0x%x),%s(%p),%p,%p,%p)", this->MsgQueName, MessageCmdConvert(this->MsgQueName,msg->command),
            msg->command, MessageArg1Convert(this->MsgQueName,msg),msg->arg1,msg->arg2,msg->arg3,msg->arg4);
    }

    return 0;
}

int MessageQueue::put(Message_cam* msg)
{
    char* p = (char*) msg;
    unsigned int bytes = 0;

    if (((strstr(this->MsgQueName,"display"))&&(strcmp(MessageCmdConvert(this->MsgQueName,msg->command),"CMD_DISPLAY_FRAME")==0)) ||
        ((strstr(this->MsgQueName,"command"))&&(strcmp(MessageCmdConvert(this->MsgQueName,msg->command),"CMD_PREVIEW_QBUF")==0))) {
        LOG2("%s.put(%s,%p,%p,%p,%p)",this->MsgQueName, MessageCmdConvert(this->MsgQueName,msg->command), msg->arg1,msg->arg2,msg->arg3,msg->arg4);
    } else {
        LOG2("%s.put(%s(0x%x),%s(%p),%p,%p,%p)", this->MsgQueName, MessageCmdConvert(this->MsgQueName,msg->command),
            msg->command, MessageArg1Convert(this->MsgQueName,msg),msg->arg1,msg->arg2,msg->arg3,msg->arg4);
    }

    while( bytes  < sizeof(msg) )
    {
        int err = write(this->fd_write, p, sizeof(*msg) - bytes);

        if( err < 0 ) {
            LOGE("write() error: %s", strerror(errno));
            return -1;
        }
        else
            bytes += err;
    }

    return 0;
}


bool MessageQueue::isEmpty()
{
    struct pollfd pfd;

    pfd.fd = this->fd_read;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if( 1 != poll(&pfd,1,0) ){
        return 1;
    }

    return (pfd.revents & POLLIN) == 0;
}

int MessageQueue::dump()
{
    return 0;
}

}

