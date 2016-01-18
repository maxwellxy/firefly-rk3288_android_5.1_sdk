#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>

#include "brcm_types.h"
#include "brcmipc_unixsocket.h"

#define MAX_IPCOBJ_SZ 400
#define BUF_SZ 3*MAX_IPCOBJ_SZ


int brcmipc_connect(const char* path)
{
    struct sockaddr_un sadr;
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0) {
        perror("ipcclient >> getConnectionTCP - error in socket()");
        goto err;
    }

    sadr.sun_family = AF_UNIX;

    if(sizeof(sadr.sun_path) < strlen(path)+1) {
        perror("ipcclient >> getConnectionTCP - given path is too long");
        goto err;
    }

    strcpy(sadr.sun_path, path);

    if (connect(fd, (struct sockaddr *)&sadr, sizeof(sadr))) {
        perror("ipcclient >> getConnectionTCP - error in connect()");
        goto err;
    }

    return fd;
err:
    if (fd >= 0)
        close(fd);
    return -1;
}

BrcmLbs_Result brcmipc_send(OsHandle lbs, uint8_t *data, size_t datalen)
{
    ssize_t sz;
    LbsHandle hlbs = (LbsHandle) lbs;
    if (!lbs)
        return BRCM_LBS_ERROR_LBS_INVALID;
    if ((sz = write(hlbs->ipc_fd,data,datalen)) < 0)
        return BRCM_LBS_ERROR_FAILED;

    return BRCM_LBS_OK;
}

