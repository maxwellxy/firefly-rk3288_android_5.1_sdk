#ifndef VOB_BLEND_CMD_H
#define VOB_BLEND_CMD_H

namespace android {

enum vob_blend_ctrl_type {
    // 0xx
    VOB_BLEND_CTRL_UNKNOWN = -1,

    VOB_BLEND_CTRL_CREATE           = 1,
    VOB_BLEND_CTRL_GET_QUE_SIZE     = 2,
    VOB_BLEND_CTRL_VOB_SUB_QUEUE    = 3,
    VOB_BLEND_CTRL_FLUSH            = 4,

    VOB_BLEND_CTRL_VOB_SUB_BLEND    = 10,
    VOB_BLEND_CTRL_DESTROY          = 20,
};

}
#endif  //VOB_BLEND_H
