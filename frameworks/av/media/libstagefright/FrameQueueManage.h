#ifndef FRAMEQUEUEMANAGE_H_
#define FRAMEQUEUEMANAGE_H_

#include "vpu_global.h"
#include <media/MediaPlayerInterface.h>
#include <utils/threads.h>
#include <utils/List.h>
#include "include/rk29-ipp.h"
#include <fcntl.h>
#include <poll.h>

#include <vpu_mem_pool.h>

#define CACHE_NUM 6

namespace android {

struct MediaBuffer;

typedef int32_t (*vobBlendCtrlFunc)(int32_t ctrl_type, void** me, void* ctrlParam);
extern void* RkDemuxLib();

/*
 ** for support Live WallPaper, embeded subtitle and so on,
 ** must sync with VideoDisplayView.java, also note that
 ** do not conflict with MediaPlayer.h.
*/
enum awesome_player_media_para_keys {
    KEY_PARAMETER_SET_LIVE_WALL_PAPER = 1998,               // set only
	KEY_PARAMETER_TIMED_TEXT_FRAME_DURATION_MS = 1999,      // get only

	KEY_PARAMETER_SET_VIDEO_CODEC_NOT_SUPPORT  = 2100,      // set only
	KEY_PARAMETER_SET_AUDIO_CODEC_NOT_SUPPORT  = 2101,      // set only
	KEY_PARAMETER_SET_BROWSER_REQUEST = 3188,               // set only
};

enum awesome_player_media_config_map {
    LIVE_WALL_PAPER_ENABLE             = 0x01,
    BROWSER_REQUEST_ENABLE             = 0x02,
    VIDEO_CODEC_DISABLE                = 0x04,
    AUDIO_CODEC_DISABLE                = 0x08,
};

typedef struct AwesomePlayerExtion{
    uint32_t flag;
    bool isBuffering;
    bool ionMemAllocFail;
    bool setCacheFlag;
    int32_t cacheNum;
}AwesomePlayerExt_t;


enum subtitle_msg_type {
    // 0xx
    SUBTITLE_MSG_UNKNOWN = -1,
    // 1xx
    SUBTITLE_MSG_VOBSUB_FLAG = 1,
    // 2xx
    SUBTITLE_MSG_VOBSUB_GET = 2,
};

struct FrameQueue
{
	FrameQueue	*next;
	MediaBuffer		*info;
	int64_t		time;
    bool        isSwDec;
	FrameQueue(MediaBuffer *data, int64_t tr = 0);
	~FrameQueue();
};

#define USE_DEINTERLACE_DEV         1

struct iep_ops {
    void* (*claim)();
    void* (*reclaim)(void *iep_obj);
    int (*init_discrete)(void *iep_obj, 
                          int src_act_w, int src_act_h, 
                          int src_x_off, int src_y_off,
                          int src_vir_w, int src_vir_h, 
                          int src_format, 
                          int src_mem_addr, int src_uv_addr, int src_v_addr,
                          int dst_act_w, int dst_act_h, 
                          int dst_x_off, int dst_y_off,
                          int dst_vir_w, int dst_vir_h, 
                          int dst_format, 
                          int dst_mem_addr, int dst_uv_addr, int dst_v_addr);
    int (*config_yuv_deinterlace)(void *iep_obj);
    int (*run_async_ncb)(void *iep_obj);
    int (*poll)(void *iep_obj);
};

struct deinterlace_dev
{
    #define USING_IPP       (0)
    #define USING_PP        (1)
    #define USING_IEP       (2)
    #define USING_NULL      (-1)
    deinterlace_dev(int size);
    ~deinterlace_dev();

    status_t dev_status;
    int dev_fd;
    void *priv_data;
    status_t perform(VPU_FRAME *frm, uint32_t bypass);
    status_t sync();
    status_t status();
    status_t test();
    void *api;
    void *iep_lib_handle;
    struct iep_ops ops;
    vpu_display_mem_pool *pool;
};

typedef struct VobBlend {
    void*   blender;
    vobBlendCtrlFunc ctrl_fun;
}VobBlend_t;

struct FrameQueueManage : public RefBase
{
	FrameQueue	*pstart;
	FrameQueue	*pend;
	FrameQueue	*pdisplay;
	FrameQueue	*plastdisplay;
	MediaBuffer *pdeInterlaceBuffer;
	pthread_t	mThread;
	bool		mThread_sistarted;
    pthread_t                   mIPPThread;
    bool                        mIPPThread_sistarted;
	bool		stopFlag;
	bool		run;
    bool		mIppStopFlag;
	bool		mIppRun;
	bool        deintpollFlag;
	int32_t     deintFlag;
	size_t		num;
    int32_t     cacheNum;
	size_t		numdeintlace;
	Mutex		mLock;
	Mutex		mIppLock;
#if USE_DEINTERLACE_DEV
    deinterlace_dev *deint;
#else
	int ipp_fd;
	struct pollfd fd;
#endif
	MediaBuffer *OrignMediaBuffer;
    uint32_t mCurrentSubIsVobSub;
    bool        mHaveReadOneFrm;
    AwesomePlayerExt_t mPlayerExtCfg;
    VobBlend_t  mVobBlend;

	FrameQueueManage();
	~FrameQueueManage();
	int pushframe(FrameQueue *frame);
	Vector<MediaBuffer *> DeInterlaceFrame;
	int pushDeInterlaceframe(MediaBuffer* frame);
	int deleteframe(int64_t curtime);
    int32_t createvobBlender();
    void flushVobSubQueue();
	void DeinterlacePoll();
	int cache_full()
	{
    	Mutex::Autolock autoLock(mLock);
        if(isSwDecFlag){
            cacheNum = 15;
        }
        if(!mPlayerExtCfg.setCacheFlag){
            cacheNum = mPlayerExtCfg.cacheNum;
            mPlayerExtCfg.setCacheFlag = true;
        }
	    if(deintFlag)
	    {
             return (numdeintlace + num) >= cacheNum;
	    }
		return num >= cacheNum;
	}
	void updateDisplayframe();
	FrameQueue *getNextDiplayframe();
	MediaBuffer *getNextFrameDinterlace();
	FrameQueue *popframe();
	void start(void *pram);
    void startIppThread();
	void stop(void);
	void play();
	void pause();
	bool runstate();
	void flushframes(bool end);
	int Displayframe(void *ptr);
	int DeinterlaceProc();
    bool isSwDecFlag;
	static void* Threadproc(void *me);
    static void* ThreadIPP(void *me);

    int64_t IppProc();
    void subtitleNotify(int msg, void* obj);
};
}
#endif  // FRAMEQUEUEMANAGE_H_

