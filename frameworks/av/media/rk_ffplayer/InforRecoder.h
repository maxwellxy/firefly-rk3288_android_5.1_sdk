#ifndef _FFMPEG_INFORMATION_RECODER_
#define _FFMPEG_INFORMATION_RECODER_

extern "C"
{
#include <stdio.h>
}

#include <utils/threads.h>

/*
* this file is defined by hh@rock-chips.com for record playing status for shanghai CMCC
*/


// 卡顿开始
#define LAG_START  701

// 卡顿结束
#define LAG_STOP 702

// 获取不到切片
#define GET_DATA_FAIL 703

// 播放器报错
#define PLAYER_ERROR 100



#define MAX_URL_LENGTH 4096


namespace android 
{
    
class InforRecoder
{
public:
    InforRecoder();
    ~InforRecoder();

  
  int getOpenFileName();
	
    /*
    * 创建存放LOG文件的目录
    * path:创建的文件夹目录，上海移动视频基地要求目录固定为 /tmp/playLog/
    */
    int createDirectory(char* path);

    /*
    * 根据当前系统时间(年月日时分)，作为文件名字，比如:20140626195600.txt
    * time: 用于保存文件名
    * length: 存储区的最大长度
    */
    void getSystemTime(char* time,int length,int needSecond);

    /*
    * 创建文件，根据getFileName获取到的文件名创建文件
    * 上海移动视频基地要求每隔15分钟创建一个新文件，将后续15分钟中出现的播放卡顿信息写入新文件中
    */
    void openFile();

    /*
    * 关闭文件
    */
    void closeFile();

    /*
    *将播放器异常状态的信息写入文件
    * url: 当前播放的片源url
    * playtime: 当前的播放时间
    * status: 错误码，异常码
    * des: 错误码的文字描述
    */
    void writeFile(char* url,int playtime,int status,char* des);


	
	void realWrite(char* url,int playTime,int status,char* des);

    /*
    * 删除特定目录(/tmp/playLog/)下的文件，只保留最后一个15分钟的文件
    */
    void deleteDirectory();

   /*
     *  delete all files in this directory
     */
   void deleteAllFileInDirectory();

    /*
    * 线程回调函数
    */
    static void* threadCallBack(void * recoder);
private:
    // 打开的文件句柄
    FILE*      mLogFile;

    // 打开的文件路径，用于删除没有写入LOG的文件
    char*      mFilePath;

    // 删除文件的时间
    int64_t    mDeleteFileTime;

    // 创建文件的时间
    int64_t    mCreateFileTime;

    // 线程控制变量，用于控制线程的进入退出
    int        mThreadExit;

    // 用于表示线程是否有存在
    int        mThreadStatus;

    // 线程句柄
    pthread_t  mThread;

    // 线程同步锁. 写文件和文件的创建、删除不在同一个线程中。
    // 使用Mutex来防止同一时间，多个线程同时访问文件。
    Mutex      mLock;

	  // 用于保存写入的URL
	  char* mURL;
};
}

#endif
