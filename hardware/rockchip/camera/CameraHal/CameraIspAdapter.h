#ifndef ANDROID_HARDWARE_CAMERA_ISP_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_ISP_HARDWARE_H

//usb camera adapter
#include "CameraHal.h"
#include "cam_api/camdevice.h"
#include "oslayer/oslayer.h"
#include <string>
#include <utils/KeyedVector.h>



namespace android{

typedef struct awbStatus{
    bool enabled;
    CamEngineAwbMode_t mode;
    uint32_t idx;
    CamEngineAwbRgProj_t RgProj;
    bool  damping;
    bool  manual_mode;
}awbStatus_s;
typedef struct manExpConfig{
	float minus_level_3;
	float minus_level_2;
	float minus_level_1;
	float level_0;
	float plus_level_1;
	float plus_level_2;
	float plus_level_3;
	float clmtolerance;
}manExpConfig_s;

class CameraIspTunning;
class CameraIspAdapter: public CameraAdapter,public BufferCb
{
public:
	static bool hdmiIn_Exit;
	static int preview_frame_inval;
    static int DEFAULTPREVIEWWIDTH;
    static int DEFAULTPREVIEWHEIGHT;
    CameraIspAdapter(int cameraId);
    virtual ~CameraIspAdapter();
    virtual status_t startPreview(int preview_w,int preview_h,int w, int h, int fmt,bool is_capture);
    virtual status_t stopPreview();
    virtual int setParameters(const CameraParameters &params_set,bool &isRestartValue);
    virtual void initDefaultParameters(int camFd);
    virtual status_t autoFocus();
    virtual status_t cancelAutoFocus();
    virtual int getCurPreviewState(int *drv_w,int *drv_h);
    virtual int selectPreferedDrvSize(int *width,int * height,bool is_capture);
    void AfpsResChangeCb();
    virtual void bufferCb( MediaBuffer_t* pMediaBuffer );

    virtual void setupPreview(int width_sensor,int height_sensor,int preview_w,int preview_h,int zoom_value);

	virtual void dump(int cameraId);
    virtual void getCameraParamInfo(cameraparam_info_s &paraminfo);
	virtual bool getFlashStatus();
	virtual void getSensorMaxRes(unsigned int &max_w, unsigned int &max_h);
	virtual int faceNotify(struct RectFace* faces, int* num);
private:
    //talk to driver
    virtual int cameraCreate(int cameraId);
    virtual int cameraDestroy();
    virtual int adapterReturnFrame(long index,int cmd);


    //for isp
    void setScenarioMode(CamEngineModeType_t newScenarioMode);

    void setSensorItf(int newSensorItf);
    void enableAfps( bool enable = false );

    void loadSensor(int cameraId =-1 );
    void loadCalibData(const char* fileName = NULL);
    void openImage( const char* fileName = NULL);

    bool connectCamera();
    void disconnectCamera();

    int start();
    int pause();
    int stop();

	int hdmiinListenerThread(void);
    int afListenerThread(void);
	int dataListenerThread(void);
    int cameraConfig(const CameraParameters &tmpparams,bool isInit,bool &isRestartValue);
    bool isLowIllumin();
    void flashControl(bool on);
    bool isNeedToEnableFlash();
	void setMwb(const char *white_balance);
	void setMe(const char *exposure);

	
protected:
    CamDevice       *m_camDevice;
    KeyedVector<void *, void *> mFrameInfoArray;
    Mutex  mFrameArrayLock;     
    void clearFrameArray();
	mutable Mutex mLock;

    std::string mSensorDriverFile[3];
    int mSensorItfCur;
    bool mFlashStatus;
	CtxCbResChange_t mCtxCbResChange;
    bool mAfChk;
    class CameraAfThread :public Thread
    {
        //deque 到帧后根据需要分发给DisplayAdapter类及EventNotifier类。
        CameraIspAdapter* mCameraAdapter;
    public:
        CameraAfThread(CameraIspAdapter* adapter)
            : Thread(false), mCameraAdapter(adapter) { }

        virtual bool threadLoop() {
            mCameraAdapter->afListenerThread();

            return false;
        }
    };
    
    CamEngineAfEvtQue_t  mAfListenerQue; 
    sp<CameraAfThread>   mAfListenerThread;

	class CameraDataThread :public Thread
	{
	 CameraIspAdapter* mCameraAdapter;
	public:
	 CameraDataThread(CameraIspAdapter* adapter)
		 : Thread(false), mCameraAdapter(adapter) { }

	 virtual bool threadLoop() {
		 mCameraAdapter->dataListenerThread();

		 return false;
	 }
	};
	sp<CameraDataThread>	mDataListenerThread;
	bool bResetIsp;
	bool bDataReced;

	class HdmiInThread : public Thread 
	{
        CameraIspAdapter* mCameraAdapter;        
    public:
        HdmiInThread(CameraIspAdapter* adapter)
            : Thread(false), mCameraAdapter(adapter){}

        virtual bool threadLoop() {
            mCameraAdapter->hdmiinListenerThread();

            return false;
        }
    };
	sp<HdmiInThread>   mHdmiinListenerThread;
	
    enum ISP_TUNNING_THREAD_CMD_e{
       ISP_TUNNING_CMD_START,
       ISP_TUNNING_CMD_EXIT,
       ISP_TUNNING_CMD_PROCESS_FRAME
    };

    class CamISPTunningThread :public Thread
    {
        //deque 到帧后根据需要分发给DisplayAdapter类及EventNotifier类。
        CameraIspAdapter* mCameraAdapter;
    public:
        CamISPTunningThread(CameraIspAdapter* adapter)
            : Thread(false), mCameraAdapter(adapter) { }

        virtual bool threadLoop() {
            mCameraAdapter->ispTunningThread();

            return false;
        }
    };

    int ispTunningThread(void);
    MessageQueue* mISPTunningQ;
    sp<CamISPTunningThread>   mISPTunningThread;
    int mISPOutputFmt;
    bool mISPTunningRun;
    bool mIsSendToTunningTh;    

    int mDispFrameLeak;
    int mVideoEncFrameLeak;
    int mPreviewCBFrameLeak;
    int mPicEncFrameLeak;
private:
    
    awbStatus curAwbStatus;
    CameraIspTunning* mIspTunningTask;
    manExpConfig_s manExpConfig;
    
};

class CameraIspSOCAdapter: public CameraIspAdapter
{
public:

    CameraIspSOCAdapter(int cameraId);
    virtual ~CameraIspSOCAdapter();
#if 0
    virtual int setParameters(const CameraParameters &params_set);
    virtual void initDefaultParameters()
    {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, "10,15,30");  

    };
   virtual status_t autoFocus();
#endif
    virtual void setupPreview(int width_sensor,int height_sensor,int preview_w,int preview_h,int zoom_value);
    virtual void bufferCb( MediaBuffer_t* pMediaBuffer );

private:
    bool    mIs10bit0To0;

};



}
#endif
