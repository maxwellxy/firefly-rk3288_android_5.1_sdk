#define LOG_TAG "sf_version"
#include <utils/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include "version.h"
#include <cutils/properties.h>
#include <sys/ioctl.h>
#include <fcntl.h>

sf_info *sf_info::singleton = NULL;

#include "svn_info.h"
static char sf_text_version_info[] = SVN_VERSION_INFO;
static char sf_text_compile_info[] = SF_COMPILE_INFO;

sf_info *sf_info::getInstance()
{
    if (singleton == NULL) {
        singleton = new sf_info();
    }
    return singleton;
}

static RK_CHIP_TYPE chip_version(void)
{
    RK_CHIP_TYPE type = NONE;
    char prop_value[PROPERTY_VALUE_MAX];
    if (property_get("ro.product.board", prop_value, NULL)) {
        if (strstr(prop_value, "rk29")) {
            ALOGI("rk29 board found in board property");
            type = RK29;
        } else if (strstr(prop_value, "rk30")) {
            ALOGI("rk30 board found in board property");
            type = RK30;
        }
    }
    if (NONE == type) {
        if (property_get("ro.board.platform", prop_value, NULL))
        {
            if (strstr(prop_value, "rk29")) {
                ALOGI("rk29 board found in platform property");
                type = RK29;
            } else if (strstr(prop_value, "rk30")) {
                ALOGI("rk30 board found in platform property");
                type = RK30;
            }
        }
    }

    if (NONE == type) {
        ALOGW("can not found matched chip type");
    }
    return type;
}

sf_info::sf_info()
{
    chip_type  = chip_version();
    sf_version = SVN_VERSION;
    sf_version_info = sf_text_version_info;
    sf_compile_info = sf_text_compile_info;
}


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
RK_CHIP_TYPE get_chip_type()
{
    return chip_version();
}
#ifdef __cplusplus
}
#endif


