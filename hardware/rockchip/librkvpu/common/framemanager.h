#ifndef _FRAME_H_
#define _FRAME_H_

#include "vpu_global.h"
#include "vpu_mem.h"


class framemanager
{

public:
    framemanager();
    ~framemanager();

    RK_S32      init(RK_U32 framenum);
    RK_S32      deinit();
    VPU_FRAME*  get_frame(RK_U32 size,void *ctx);
    RK_S32      malloc_frame(VPU_FRAME *frame, RK_U32 size,void *ctx);
    RK_S32      free_frame(VPU_FRAME *frame);
	void		push_empty(VPU_FRAME *frame);
	void		push_display(VPU_FRAME *frame);
	VPU_FRAME* get_display(void);
	void		employ_frame(VPU_FRAME *frame);
private:
    VPU_FRAME   *FrmBufBase;
    RK_U32      frameNum;
	VPU_FRAME *empty_head;
	VPU_FRAME *empty_end;
	RK_U32		empty_cnt;
	VPU_FRAME *display_head;
	VPU_FRAME *display_end;
	RK_U32		display_cnt;
};

#endif
