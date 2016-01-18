#ifndef ANDROID_IONALLOC_PRIV_H
#define ANDROID_IONALLOC_PRIV_H

#include <stdlib.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <errno.h>
#include <linux/ioctl.h>
#include "ionalloc_vpu.h"

#include "linux/ion.h"

#define ION_DEVICE "/dev/ion"
enum {
    FD_INIT = -1,
};

typedef struct private_handle_t {
    struct ion_buffer_t data;
    int fd;
    int pid;
    ion_user_handle_t handle;
#define NUM_INTS    2
#define NUM_FDS     1
#define MAGIC       0x3141592
    int s_num_ints;
    int s_num_fds;
    int s_magic;
}private_handle_t;

typedef struct private_device_t {
    ion_device_t ion;
    int ionfd;
    unsigned long align;
    enum ion_module_id id;
}private_device_t;

#endif /* ANDROID_IONALLOC_PRIV_H */

