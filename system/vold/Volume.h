/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _VOLUME_H
#define _VOLUME_H

#include <utils/List.h>
#include <fs_mgr.h>

class NetlinkEvent;
class VolumeManager;

/* $_rbox_$_modify_$_huangyonglin: added for adding a way to support the mass storage*/
#define MAX_UDISK_PARTITIONS 32
#define MAX_PARTITIONS_FILE_NUM 50
#define MAX_SAME_PATITION_VOLUME_NAME_COUNTS	15
#define UDISK_VOLUME_LABEL_PREFIX		"USB_DISK"

//macro to change android filesystem state;added by zxg
#define	CHANGE_ANDROIDFILESYSTEM_TO_READWRITE	
#define CHANGE_ANDROIDFILESYSTEM_TO_READONLY


typedef  struct {
    int imajor;
    int iminor;
    int mState;
    int iPartitionIndex;    
    char ucLable[30];
    char ucMountPoint[50];
    char ucFilePathName[200];
} UDISK_PARTITION_CONFIG;
typedef android::List<UDISK_PARTITION_CONFIG *> UDisk_Partition_Collection;
/* $_rbox_$_modify_$ end */

class Volume {
private:
    int mState;
    int mFlags;

public:
    static const int State_Init       = -1;
    static const int State_NoMedia    = 0;
    static const int State_Idle       = 1;
    static const int State_Pending    = 2;
    static const int State_Checking   = 3;
    static const int State_Mounted    = 4;
    static const int State_Unmounting = 5;
    static const int State_Formatting = 6;
    static const int State_Shared     = 7;
    static const int State_SharedMnt  = 8;

    static const char *MEDIA_DIR;
    static const char *FUSE_DIR;
    static const char *SEC_ASECDIR_EXT;
    static const char *SEC_ASECDIR_INT;
    static const char *ASECDIR;
    static const char *LOOPDIR;

/* $_rbox_$_modify_$_huangyonglin: added for adding a way to support the mass storage*/
    static const char *SEC_UDISK_PRATITION_DIR;
    static const char *SEC_UDISK_PRATITION_DIR_EXTERN_0;
    static const char *SEC_UDISK_PRATITION_DIR_EXTERN_1;
    static const char *SEC_UDISK_PRATITION_DIR_EXTERN_2;
    static const char *SEC_UDISK_PRATITION_DIR_EXTERN_3;
    static const char *SEC_UDISK_PRATITION_DIR_EXTERN_4;
    static const char *SEC_UDISK_PRATITION_DIR_EXTERN_5;
    static const char *SEC_UDISK_INTERNAL_DIR;
    static const char *PROP_EXTERNAL_STORAGE_STATE;
    
    int mDiskVolumelMinors[MAX_SAME_PATITION_VOLUME_NAME_COUNTS];
    static int mDiskVolumelNum;
    
	//used to create mDiskVolumeLabel
	static int sDiskVolumeLabelNum;
/* $_rbox_$_modify_$ end */
    
    static const char *BLKID_PATH;

protected:
    char* mLabel;
    char* mUuid;
    char* mUserLabel;
    VolumeManager *mVm;
    bool mDebug;
	bool mSkipAsec;
    int mPartIdx;
    int mOrigPartIdx;
    bool mRetryMount;

/* $_rbox_$_modify_$_huangyonglin: added for adding a way to support the mass storage*/
    char *mDevPath;
	char mDiskVolumeLabel[20];	//used to show udisk volume label;added by zxg
	char mDiskMountFilePathName[100];
    UDisk_Partition_Collection *mUdiskPartition;
/* $_rbox_$_modify_$ end */	


    /*
     * The major/minor tuple of the currently mounted filesystem.
     */
    dev_t mCurrentlyMountedKdev;

public:
    Volume(VolumeManager *vm, const fstab_rec* rec, int flags);
    virtual ~Volume();

    int mountVol();
    int unmountVol(bool force, bool revert, bool badremove= false);
    int formatVol(bool wipe);

    const char* getLabel() { return mLabel; }
    const char* getUuid() { return mUuid; }
    const char* getUserLabel() { return mUserLabel; }
    int getState() { return mState; }
    int getFlags() { return mFlags; };

    /* Mountpoint of the raw volume */
    virtual const char *getMountpoint() = 0;
    virtual const char *getFuseMountpoint() = 0;


/* $_rbox_$_modify_$_huangyonglin: added for adding a way to support the mass storage*/
    int addUdiskPartition(int major,int minor);
    bool addPartitionMountFile(char *FilePath);
	//to avoid the same partition volume name;added by zxg
	bool addPartitionMountFileSuffix(char *filepath);
    int RemoveUdiskPartition(const char *Mountpoint);
    int mountUdiskVol();
    int unmountUdiskVol(const char *label, bool force, bool badremove = false);

    void displayItem();
    const char *getDevPath() { return mDevPath; }
    const char *setDevPath(const char *DevPath);
    UDISK_PARTITION_CONFIG *getPartitionState(const char *lable) ;
    UDISK_PARTITION_CONFIG *getPartitionState(int major,int minor);
/* $_rbox_$_modify_$ end */

    virtual int handleBlockEvent(NetlinkEvent *evt);
    virtual dev_t getDiskDevice();
    virtual dev_t getShareDevice();
    virtual void handleVolumeShared();
    virtual void handleVolumeUnshared();

    void setDebug(bool enable);
    virtual int getVolInfo(struct volume_info *v) = 0;

protected:
    void setUuid(const char* uuid);
    void setUserLabel(const char* userLabel);
    void setState(int state);

    virtual int getDeviceNodes(dev_t *devs, int max) = 0;
    virtual int updateDeviceInfo(char *new_path, int new_major, int new_minor) = 0;
    virtual void revertDeviceInfo(void) = 0;
    virtual int isDecrypted(void) = 0;

    int createDeviceNode(const char *path, int major, int minor);
/* $_rbox_$_modify_$_huangyonglin: added for adding a way to support the mass storage*/
     void setPartitionState(const char *Mountpoint,int state);
    int doUnmount(const char *path, bool force);
     bool blkid_get_Device_value(const char *tagname,const char *devname,char *pRetType);
/* $_rbox_$_modify_$ end */

private:
    int initializeMbr(const char *deviceNode);
    bool isMountpointMounted(const char *path);
    int mountAsecExternal();
/* $_rbox_$_modify_$_huangyonglin: added ,because the protected funtion had add the method*/
    //int doUnmount(const char *path, bool force);
/* $_rbox_$_modify_$ end */
    int extractMetadata(const char* devicePath);
	void notifyStateKernel(int number);
};

typedef android::List<Volume *> VolumeCollection;

#endif
