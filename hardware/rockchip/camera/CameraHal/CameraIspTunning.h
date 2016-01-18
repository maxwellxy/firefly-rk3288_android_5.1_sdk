#ifndef ANDROID_HARDWARE_CAMERA_ISPTUNNING_H
#define ANDROID_HARDWARE_CAMERA_ISPTUNNING_H
#include <utils/Vector.h>
#include "cam_api/camdevice.h"
#include "oslayer/oslayer.h"

#define RK_ISP_TUNNING_FILE_PATH "/data/capcmd.xml"

namespace android {


typedef enum EXPOSUSE_MODE_ENUM{
    EXPOSUSE_MODE_MANUAL = 1,
    EXPOSUSE_MODE_AUTO  =2,
    EXPOSUSE_MODE_INVALID = 3
}EXPOSUSE_MODE_e;

typedef enum WHITEBALANCE_MODE_ENUM{
    WHITEBALANCE_MODE_MANUAL = 1,
    WHITEBALANCE_MODE_AUTO  =2,
    WHITEBALANCE_MODE_INVALID  =3,
}WHITEBALANCE_MODE_e;

typedef struct ispTuneTaskInfo_t{
//from xml
    bool mTuneEnable;
    int mTuneWidth;
    int mTuneHeight;
    int mTuneFmt;
    int mTunePicNum;
    struct{
        EXPOSUSE_MODE_e exposuseMode;
        float    integrationTime;
        float    gain;
        float    integrationTimeStep;
        float    gainStep;
        int    minRaw;
        int    maxRaw;
        int    threshold;
        bool   aeRound;
        int    number;
    }mExpose;

    struct{
        WHITEBALANCE_MODE_e whiteBalanceMode;
        char     illumination[10];
        char     cc_matrix[15];
        char     cc_offset[10];
        char     rggb_gain[10];
        
    }mWhiteBalance;
    
    bool mWdrEnable;
    bool mCacEnable;
    bool mGammarEnable;
    bool mLscEnable;
    bool mDpccEnable;
    bool mBlsEnable;
    bool mAdpfEnable;
    bool mAvsEnable;
    bool mAfEnable;

//from ..
    bool mForceRGBOut;
    unsigned long y_addr;
    unsigned long uv_addr;
}ispTuneTaskInfo_s;


class CameraIspTunning
{
public:
    CameraIspTunning();
    ~CameraIspTunning();
    static CameraIspTunning* createInstance();
    static void StartElementHandler(void *userData, const char *name, const char **atts);
    static int ispTuneDesiredExp(long raw_ddr,int width,int height,int min_raw,int max_raw,int threshold);

    static int ispTuneStoreBufferRAW
    (
        ispTuneTaskInfo_s    *pIspTuneTaskInfo,
        FILE                *pFile,
        MediaBuffer_t       *pBuffer,
        bool              putHeader,
        bool              is16bit
    );

    static int ispTuneStoreBufferYUV422Semi
    (
        ispTuneTaskInfo_s    *pIspTuneTaskInfo,
        FILE                *pFile,
        MediaBuffer_t       *pBuffer,
        bool              putHeader
    );

    static int ispTuneStoreBuffer
    (
        ispTuneTaskInfo_s    *pIspTuneTaskInfo,
        MediaBuffer_t       *pBuffer,
        char     *szNmae,
        int      index     
    );
private:
    static void ConvertYCbCr444combToRGBcomb
    (
        uint8_t     *pYCbCr444,
        uint32_t    PlaneSizePixel
    );

    
public:
    Vector<ispTuneTaskInfo_s*> mTuneInfoVector;
    ispTuneTaskInfo_s* mCurTuneTask;
	int mTuneTaskcount;
	int mCurTunIndex;
    float mCurIntegrationTime;
    float mCurGain;
    int mCurAeRoundNum;
};

};

#endif

