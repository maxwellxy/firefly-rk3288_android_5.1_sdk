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

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/param.h>

#include <linux/kdev_t.h>

#include <cutils/properties.h>

#include <diskconfig/diskconfig.h>

#include <private/android_filesystem_config.h>

#define LOG_TAG "Vold"

#include <cutils/fs.h>
#include <cutils/log.h>

#include <string>

#include "Volume.h"
#include "VolumeManager.h"
#include "ResponseCode.h"
#include "Fat.h"
#include "Ntfs.h"
#include "Process.h"
#include "cryptfs.h"
#include "Ext4.h"

#include "blkid/blkid.h"  
#include "unicode/ucnv.h"
#define MAX_DEVICE_NODES 32

extern "C" void dos_partition_dec(void const *pp, struct dos_partition *d);
extern "C" void dos_partition_enc(void *pp, struct dos_partition *d);


/*
 * Media directory - stuff that only media_rw user can see
 */
const char *Volume::MEDIA_DIR           = "/mnt";//"/mnt/media_rw";

/*
 * Fuse directory - location where fuse wrapped filesystems go
 */
const char *Volume::FUSE_DIR           = "/storage";

/*
 * Path to external storage where *only* root can access ASEC image files
 */
const char *Volume::SEC_ASECDIR_EXT   = "/mnt/secure/asec";

/*
 * Path to internal storage where *only* root can access ASEC image files
 */
const char *Volume::SEC_ASECDIR_INT   = "/data/app-asec";

/*
 * Path to where secure containers are mounted
 */
const char *Volume::ASECDIR           = "/mnt/asec";

/*
 * Path to where OBBs are mounted
 */
const char *Volume::LOOPDIR           = "/mnt/obb";

/* $_rbox_$_modify_$_huangyonglin: added by huangyonglin for adding the funtion to support the mass storage.*/
/*
 * Path to where udisk partitions are mounted
 */
const char *Volume::SEC_UDISK_PRATITION_DIR          ="/mnt/usb_storage/";
/*
 * Path to where udisk partitions are mounted
 */
const char *Volume::SEC_UDISK_PRATITION_DIR_EXTERN_0           	= "/mnt/usb_storage/udisk0/";
const char *Volume::SEC_UDISK_PRATITION_DIR_EXTERN_1           	= "/mnt/usb_storage/udisk1/";
const char *Volume::SEC_UDISK_PRATITION_DIR_EXTERN_2           	= "/mnt/usb_storage/udisk2/";
const char *Volume::SEC_UDISK_PRATITION_DIR_EXTERN_3           	= "/mnt/usb_storage/udisk3/";
const char *Volume::SEC_UDISK_PRATITION_DIR_EXTERN_4           	= "/mnt/usb_storage/udisk4/";
const char *Volume::SEC_UDISK_PRATITION_DIR_EXTERN_5           	= "/mnt/usb_storage/udisk5/";
const char *Volume::SEC_UDISK_INTERNAL_DIR           			= "/mnt/udiskint";

/*
 *	used to create mDiskVolumeLabel
 */
int Volume::sDiskVolumeLabelNum = 0;//because add a internal harddisk  ,edited by hyl
int Volume::mDiskVolumelNum =0;

/*
 * Property of external SD card's state.
 */
const char *Volume::PROP_EXTERNAL_STORAGE_STATE  =  "EXTERNAL_STORAGE_STATE";
/*# $_rbox_$_modify_$ end */

const char *Volume::BLKID_PATH = "/system/bin/blkid";

static const char *stateToStr(int state) {
    if (state == Volume::State_Init)
        return "Initializing";
    else if (state == Volume::State_NoMedia)
        return "No-Media";
    else if (state == Volume::State_Idle)
        return "Idle-Unmounted";
    else if (state == Volume::State_Pending)
        return "Pending";
    else if (state == Volume::State_Mounted)
        return "Mounted";
    else if (state == Volume::State_Unmounting)
        return "Unmounting";
    else if (state == Volume::State_Checking)
        return "Checking";
    else if (state == Volume::State_Formatting)
        return "Formatting";
    else if (state == Volume::State_Shared)
        return "Shared-Unmounted";
    else if (state == Volume::State_SharedMnt)
        return "Shared-Mounted";
    else
        return "Unknown-Error";
}

Volume::Volume(VolumeManager *vm, const fstab_rec* rec, int flags) {
    mVm = vm;
    mDebug = false;
    mLabel = strdup(rec->label);
    mUuid = NULL;
    mUserLabel = NULL;
    mState = Volume::State_Init;
    mFlags = flags;
    mCurrentlyMountedKdev = -1;
    mPartIdx = rec->partnum;
    mRetryMount = false;
	mSkipAsec = false;

#ifdef VOLD_BOX
	/* $_rbox_$_modify_$_huangyonglin: added by huangyonglin for adding the funtion to support the mass storage.*/
    mDevPath =NULL;
    mUdiskPartition =new UDisk_Partition_Collection();
	//just create disk label for udisk;added by zxg
	memset(mDiskVolumeLabel, 0, sizeof(mDiskVolumeLabel));
	sprintf(mDiskVolumeLabel, "%s%d", UDISK_VOLUME_LABEL_PREFIX, sDiskVolumeLabelNum);
	SLOGE("Volume::Volume mDiskVolumeLabel = %s",mDiskVolumeLabel);
	SLOGE("Volume::Volume mLabel = %s",mLabel);	
	if(strncmp(mLabel,"usb_storage",strlen("usb_storage"))==0){
		++sDiskVolumeLabelNum;
	}
	/*# $_rbox_$_modify_$ end */
#endif
}

Volume::~Volume() {
#ifdef VOLD_BOX
/* $_rbox_$_modify_$_huangyonglin: added by huangyonglin for adding the funtion to support the mass storage.*/
    UDisk_Partition_Collection::iterator it;
	if(strncmp(mLabel,"usb_storage",strlen("usb_storage"))==0){
		--sDiskVolumeLabelNum;
	}
    for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); ++it)
        free(*it);
    delete mUdiskPartition;
    free(mDevPath);
/*# $_rbox_$_modify_$ end */
#endif
	
    free(mLabel);
    free(mUuid);
    free(mUserLabel);
}

void Volume::setDebug(bool enable) {
    mDebug = enable;
}

dev_t Volume::getDiskDevice() {
    return MKDEV(0, 0);
};

dev_t Volume::getShareDevice() {
    return getDiskDevice();
}

void Volume::handleVolumeShared() {
}

void Volume::handleVolumeUnshared() {
}

int Volume::handleBlockEvent(NetlinkEvent * /*evt*/) {
    errno = ENOSYS;
    return -1;
}

void Volume::setUuid(const char* uuid) {
    char msg[256];

    if (mUuid) {
        free(mUuid);
    }

    if (uuid) {
        mUuid = strdup(uuid);
        snprintf(msg, sizeof(msg), "%s %s \"%s\"", getLabel(),
                getFuseMountpoint(), mUuid);
    } else {
        mUuid = NULL;
        snprintf(msg, sizeof(msg), "%s %s", getLabel(), getFuseMountpoint());
    }

    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeUuidChange, msg,
            false);
}

void Volume::setUserLabel(const char* userLabel) {
    char msg[256];

    if (mUserLabel) {
        free(mUserLabel);
    }

    if (userLabel) {
        mUserLabel = strdup(userLabel);
        snprintf(msg, sizeof(msg), "%s %s \"%s\"", getLabel(),
                getFuseMountpoint(), mUserLabel);
    } else {
        mUserLabel = NULL;
        snprintf(msg, sizeof(msg), "%s %s", getLabel(), getFuseMountpoint());
    }

    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeUserLabelChange,
            msg, false);
}

//to inform SDMMC-driver for umounting sdcard. noted by xbw@2011-06-07
void Volume::notifyStateKernel(int number)
{
    if(!strncmp(getLabel(),"external_sd", strlen("external_sd")))
    {
        FILE *fp = fopen("/sys/sd-sdio/rescan","w");
        if(fp){
            char kstate[64] = "sd-";

            if(number == 0) {
                strcat(kstate,"Ready");//strcat(kstate,stateToStr(Volume::State_Ready));
            } else {
                strcat(kstate,stateToStr(mState));
            }
            fputs(kstate,fp);
            SLOGI("Call notifyStateKernel No.%d in the file of Volume.cpp", number);
            fclose(fp);
        } else {
            SLOGI("Error(call No.%d) opening /sys/sd-sdio/rescan in the file of Volume.cpp", number);
        }
    }
}


/* $_rbox_$_modify_$_huangyonglin: added for adding the mass storage funcion*/
bool Volume::addPartitionMountFile(char *FilePath) 
{
    int i =0;
    int iret;
    char szName[200];
    UDisk_Partition_Collection::iterator it;
    
    for(i =0 ;i<MAX_PARTITIONS_FILE_NUM;i++)
    {
         sprintf(szName,"%sudisk%d",SEC_UDISK_PRATITION_DIR,i);
         iret =access(szName, F_OK);
        if (access(szName, F_OK) != -1) 
            continue;
        strcpy(FilePath,szName);
        SLOGE("############ addPartitionMountFile  %s",szName);
        return true;
    }
    SLOGE("############ addPartitionMountFile error ##################");
    return false;
}

bool Volume::addPartitionMountFileSuffix(char *filepath) {
	int i = 0;
	char szName[200];
	UDisk_Partition_Collection::iterator it;
	
	if(access(filepath, F_OK) == -1)
		return true;
	for(i=1; i<MAX_SAME_PATITION_VOLUME_NAME_COUNTS; ++i) {
		sprintf(szName,"%s(%d)", filepath, i);
        if (access(szName, F_OK) != -1) 
            continue;
		sprintf(filepath, "%s", szName);
		SLOGE("############ addPartitionMountFile suffix  %s",filepath);
		return true;
	}
	SLOGE("############ addPartitionMountFile suffix error ##################");
	return false;
}

int Volume::addUdiskPartition(int major,int minor) 
{
    UDISK_PARTITION_CONFIG *partition;
    int i =0;
    UDisk_Partition_Collection::iterator it;
    char szLable[100];
    char *pucMountPoint =NULL;

	SLOGE("addUdiskPartition getLabel = %s.",getLabel());

    if(strcmp(getLabel(),"udisk1")==0)
        pucMountPoint =(char *)SEC_UDISK_PRATITION_DIR_EXTERN_1;
    else if(strcmp(getLabel(),"udisk2") ==0)
        pucMountPoint =(char *)SEC_UDISK_PRATITION_DIR_EXTERN_2;
    else if(strcmp(getLabel(),"udisk3") ==0)
        pucMountPoint =(char *)SEC_UDISK_PRATITION_DIR_EXTERN_3;
    else if(strcmp(getLabel(),"udisk4") ==0)
        pucMountPoint =(char *)SEC_UDISK_PRATITION_DIR_EXTERN_4;     
    else if(strcmp(getLabel(),"udisk5") ==0)
        pucMountPoint =(char *)SEC_UDISK_PRATITION_DIR_EXTERN_5;      
    else if(strcmp(getLabel(),"udiskint") ==0)
        pucMountPoint =(char *)SEC_UDISK_INTERNAL_DIR;      
    else
        pucMountPoint =(char *)SEC_UDISK_PRATITION_DIR_EXTERN_0;

	SLOGE("addUdiskPartition pucMountPoint = %s.",pucMountPoint);
		
    for(i =0 ;i<MAX_UDISK_PARTITIONS;i++)
    {
        sprintf(szLable,"udisk%d",i);

		for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); ++it)
        {
           if (!strncmp(szLable, (*it)->ucLable,strlen((*it)->ucLable)))
                break;
        }
		
        if(it == mUdiskPartition->end())
        {
            partition = (UDISK_PARTITION_CONFIG *)malloc(sizeof(UDISK_PARTITION_CONFIG)+1);
            memset(partition,0,sizeof(sizeof(UDISK_PARTITION_CONFIG)+1));
            strcpy(partition->ucLable,szLable);           
            sprintf(partition->ucMountPoint,"%s%s",pucMountPoint,szLable);
            partition->imajor =major;
            partition->iminor =minor;
            partition->mState =State_Init;
    
            mUdiskPartition->push_back(partition);
            for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); ++it)
                SLOGE("############ addUdiskPartition ucMountPoint=%s mState =%d##################",(*it)->ucMountPoint,(*it)->mState);    
                    
            return i;
        }
    }

    SLOGE("############ addUdiskPartition error ##################");
    return -1;
}

int Volume::RemoveUdiskPartition(const char *Mountpoint) 
{
    UDISK_PARTITION_CONFIG *partition;
    int ii =0;
    UDisk_Partition_Collection::iterator it;
    char szLable[100];    

    for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); ++it)
    {
	   SLOGE("############ RemoveUdiskPartition remove =%s##################",(*it)->ucMountPoint);
       if (!strncmp(Mountpoint, (*it)->ucMountPoint,strlen((*it)->ucMountPoint)))
       {
            mUdiskPartition->erase(it);
            return 0;
       }
    }

    SLOGE("############ RemoveUdiskPartition error no such  Mountpoint =%s##################",Mountpoint);
	return -1;
}
const char *Volume::setDevPath(const char *DevPath)
{
    if(mDevPath)
    {
        free(mDevPath);
    }
    if(DevPath)
        mDevPath = strdup(DevPath);
    else
        mDevPath =NULL;
    return mDevPath;
}
void Volume::displayItem()
{
    UDisk_Partition_Collection::iterator it;

    SLOGE("############ displayItem#########");
    for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); ++it)
        SLOGE(" ucMountPoint=%s szLable =%s##################",(*it)->ucMountPoint,(*it)->ucLable);    
    SLOGE("############ displayItem end#########");
}

UDISK_PARTITION_CONFIG *Volume::getPartitionState(int major,int minor)
{
        UDisk_Partition_Collection::iterator it;
        int i;
        int oldState ;
        for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); ++it)
        {
           if ((*it)->imajor ==major && (*it)->iminor ==minor)
                {
                    return (*it);
                }
        }
	return NULL;	
}
bool Volume::blkid_get_Device_value(const char *tagname,const char *devname,char *pRetType)
{
    bool iRet =false;
    char *pfstype;
    blkid_cache cache = NULL;
    blkid_get_cache(&cache, "/dev/null");
    pfstype = blkid_get_tag_value(cache, tagname,devname);
    if(pfstype)
    {
        iRet =true;
        strcpy(pRetType,pfstype);
        free(pfstype);
        pfstype =NULL;
    }
    return iRet;
}

int Volume::mountUdiskVol() {
    dev_t deviceNodes[4];
    int n, i, rc = 0;
    char errmsg[255];
    int istate ;

    istate =getState();
	SLOGE("mountUdiskVol istate = %d",istate);
    if (istate == Volume::State_NoMedia) {
        snprintf(errmsg, sizeof(errmsg),
                 "Volume %s %s mount failed - no media",
                 getLabel(), getMountpoint());
        mVm->getBroadcaster()->sendBroadcast(
                                         ResponseCode::VolumeMountFailedNoMedia,
                                         errmsg, false);
        errno = ENODEV;
        return -1;
    } 
	else if (istate != Volume::State_Idle) {
        
        errno = EBUSY;
//$_rbox_$_modify_$_lijiehong: change to avoid udisk mount failure.
	 SLOGW("mountVol   errno = EBUSY istate =%d, retry..**************************************",istate);
        if (getState() == Volume::State_Pending) {
            mRetryMount = true;
        }
//$_rbox_$_modify_$ end
        return -1;
    }

        {
            UDisk_Partition_Collection::iterator it;
            char szType[50];
            char szVolume[255];             
            char devicePath[256];
            bool bSucceed =false;
            int iMountRet =false;
            int bUsbDiskMount =false;
            int imajor,iminor;
            char szTempBuf[256];
            mDiskVolumelNum =0;
            setState(Volume::State_Checking);
            CHANGE_ANDROIDFILESYSTEM_TO_READWRITE;
		//to mkdir the disk mount file
		SLOGE("mountUdiskVol getLabel = %s.",getLabel());
		if(!strcmp(getLabel(),"udiskint"))
		{
            snprintf(mDiskMountFilePathName, sizeof(mDiskMountFilePathName), "%s", SEC_UDISK_INTERNAL_DIR);
		}
        else
		{
	        snprintf(mDiskMountFilePathName, sizeof(mDiskMountFilePathName), "%s%s", SEC_UDISK_PRATITION_DIR, mDiskVolumeLabel);
		}
		SLOGE("mountUdiskVol mDiskMountFilePathName = %s.",mDiskMountFilePathName);

		mkdir(mDiskMountFilePathName, 0777);
        chmod(mDiskMountFilePathName,0777);
        bUsbDiskMount =false;
		SLOGW("mountVol mDiskVolumelNum =%s",mDiskMountFilePathName);
		
            for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); ++it)
            {
	            if((*it) == NULL)
		        {
		        SLOGW("POINT IS NULL------------------");
	            	return -1;
				}
                imajor =(*it)->imajor;
                iminor =(*it)->iminor;
                
                sprintf(devicePath, "/dev/block/vold/%d:%d", imajor,iminor);
                
                SLOGE("%s 1111being considered for volume %s %s ", devicePath, (*it)->ucFilePathName, (*it)->ucLable);
                memset(szType,0,sizeof(szType));
                memset(szVolume,0,sizeof(szVolume));
                blkid_get_Device_value("TYPE",devicePath,szType);
				SLOGE("#######mUdiskPartition size: %d  szType =%s",mUdiskPartition->size(),szType);
              //  if(mUdiskPartition->size() >2)
				{

	                if(!strcmp(szType,"ntfs"))
	                {
	                    blkid_get_Device_value("LABEL",devicePath,szVolume);
	                    sprintf((*it)->ucFilePathName, "%s/%s", mDiskMountFilePathName, szVolume);
	                }
	                else
	                    sprintf((*it)->ucFilePathName, "%s/%s", mDiskMountFilePathName, (*it)->ucLable);        
	                SLOGE("ucFilePathName:%s  szType=%s", (*it)->ucFilePathName,szType);
	                
	                //if(addPartitionMountFile((*it)->ucFilePathName) ==false)
	                //    break;
	                //now just try to get ntfs partition label
	
	                if(addPartitionMountFileSuffix((*it)->ucFilePathName) == false)
	                	break;
	                SLOGE("FilePathName:%s  szType=%s", (*it)->ucFilePathName,szType);
	                mkdir((*it)->ucFilePathName, 0700);
        	}
        	//else
        	 //  sprintf((*it)->ucFilePathName, "%s", mDiskMountFilePathName);     
                errno = 0;
                iMountRet =false;
 

                if(!strcmp(szType,"vfat"))
                {
                    if (Fat::doMount(devicePath, (*it)->ucFilePathName, false, false, false,
                         1000, 1015, 0002, true) ==0)
                    {
                            iMountRet =true;
                            bUsbDiskMount =true;
                    }
                }
                else if(!strcmp(szType,"ntfs"))
                {
                    if(Ntfs::doMount(devicePath, (*it)->ucFilePathName, false, 1000,1000)==0)
                    {
                            iMountRet =true;
                            bUsbDiskMount =true;
                    }                       
                }
                else if(!strcmp(szType,"ext2"))
                {
                    if(Ext2::doMount(devicePath, (*it)->ucFilePathName, false, false,false)==0)
                    {
                            iMountRet =true;
                            bUsbDiskMount =true;
                    }  
                                 
                }

                else if(!strcmp(szType,"ext3"))
                {
                    if(Ext3::doMount(devicePath, (*it)->ucFilePathName, false, false,false)==0)
                    {
                            iMountRet =true;
                            bUsbDiskMount =true;
                    }  
                                 
                }
                                
                 else if(!strcmp(szType,"ext4"))
                {
                    if(Ext4::doMount(devicePath, (*it)->ucFilePathName, false, false,false)==0)
                    {
                            iMountRet =true;
                            bUsbDiskMount =true;
                    }                       
                }

               
                if(iMountRet ==false)
                {
                   // if(mUdiskPartition->size() >2)
                    	
                    SLOGE("earse succed!!!");
                    SLOGE("%s failed to mount via %s (%s)",devicePath,szType,strerror(errno));
					umount((*it)->ucFilePathName);
					rmdir((*it)->ucFilePathName);
					//RemoveUdiskPartition((*it)->ucMountPoint);
					/*if(0 == RemoveUdiskPartition((*it)->ucMountPoint)){
						continue;
					}else{
						++it;
					}*/
					//setDevPath(NULL);
					//bSucceed =false;
                }
                else
                {
                    SLOGE("mount vfat %s succed!!!",(*it)->ucFilePathName);
                    bSucceed =true;
                    //mDiskVolumelMinors[mDiskVolumelNum++] =iminor;
                }
            }
            if(bSucceed)
            {
                CHANGE_ANDROIDFILESYSTEM_TO_READONLY;
                setState(Volume::State_Mounted);
                return 0;
             }
            else
            {
                if(bUsbDiskMount==false)
                    rmdir(mDiskMountFilePathName);
                CHANGE_ANDROIDFILESYSTEM_TO_READONLY;
                setState(Volume::State_Idle);
                setDevPath(NULL);
            }
        }
    return -1;
}

int Volume::unmountUdiskVol(const char *label, bool force, bool badremove)
{
    int i;
    char Mountpoint[55];
    int istate ;
    UDisk_Partition_Collection::iterator it;
	char devicePath[255];

    istate =getState();

    if (istate == Volume::State_NoMedia) {
        SLOGW("Attempt to unmount  failed State_NoMedia ");
        errno = ENODEV;
        return -1;
    }

    if (istate != Volume::State_Mounted) {
        SLOGW("Attempt to unmount volume which isn't mounted (%d)\n",istate);
        errno = EBUSY;
        return -1;
    }


    setState(Volume::State_Unmounting);
        CHANGE_ANDROIDFILESYSTEM_TO_READWRITE;
    SLOGE("1********************************");
    /* if(!strcmp(getLabel(),"udiskint"))
    {
        for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); ++it)
        {
            SLOGE("######### del %s", (*it)->ucFilePathName);
            
            //Finally, unmount the actual block device from the staging dir
            
            if (0 !=doUnmount((*it)->ucFilePathName, force)) {
                SLOGE("Failed to unmount %s (%s)", (*it)->ucFilePathName, strerror(errno));
                //goto out_nomedia;
            }
            else
            {
                SLOGI("%s unmounted sucessfully", (*it)->ucFilePathName);
            }
             rmdir((*it)->ucFilePathName);
        }
    }
    else
    {
        for (it = mUdiskPartition->begin(); it != mUdiskPartition->end();)
        {
            SLOGE("######### del %s", (*it)->ucFilePathName);
            
            //Finally, unmount the actual block device from the staging dir
            
            if (0 !=doUnmount((*it)->ucFilePathName, force)) {
                SLOGE("Failed to unmount %s (%s)", (*it)->ucFilePathName, strerror(errno));
            }
            else
            {
                SLOGI("%s unmounted sucessfully", (*it)->ucFilePathName);
            }
            rmdir((*it)->ucFilePathName);

            it =mUdiskPartition->erase(it);

            if(it ==mUdiskPartition->end())
                break;
        }
    }*/
    
    for (it = mUdiskPartition->begin(); it != mUdiskPartition->end(); )
    {
            SLOGE("######### del %s", (*it)->ucFilePathName);
            
            //Finally, unmount the actual block device from the staging dir
            
            if (0 !=doUnmount((*it)->ucFilePathName, force)) {
                SLOGE("Failed to unmount %s (%s)", (*it)->ucFilePathName, strerror(errno));
                //goto out_nomedia;
            }
            else
            {
                SLOGI("%s unmounted sucessfully", (*it)->ucFilePathName);
            }
             rmdir((*it)->ucFilePathName);
			 if(badremove){
			 	sprintf(devicePath, "/dev/block/vold/%d:%d", (*it)->imajor, (*it)->iminor);
				SLOGE("handleUdiskPartitionRemoved handlePartitionRemoved,ready to unlink: %s",devicePath);
				if ( 0 != unlink(devicePath) ) {
					SLOGE("handleUdiskPartitionRemoved Failed to unlink %s",devicePath);
				}
			 	it = mUdiskPartition->erase(it);
				if(it ==mUdiskPartition->end())
	                break;
			 }else{
			  ++it;
			}
	}
	SLOGE("2********************************");

	if(badremove)
		setDevPath(NULL);

	rmdir(mDiskMountFilePathName);
    CHANGE_ANDROIDFILESYSTEM_TO_READONLY;
        system("sync");
    setState(Volume::State_Idle);
    return 0;
}
/* $_rbox_$_modify_$ end */



void Volume::setState(int state) {
    char msg[255];
    int oldState = mState;

    if (oldState == state) {
        SLOGW("Duplicate state (%d)\n", state);
        return;
    }

    if ((oldState == Volume::State_Pending) && (state != Volume::State_Idle)) {
        mRetryMount = false;
    }

    mState = state;
	notifyStateKernel(1);//add by xbw

    SLOGD("Volume %s state changing %d (%s) -> %d (%s)", mLabel,
         oldState, stateToStr(oldState), mState, stateToStr(mState));
    snprintf(msg, sizeof(msg),
             "Volume %s %s state changed from %d (%s) to %d (%s)", getLabel(),
             getFuseMountpoint(), oldState, stateToStr(oldState), mState,
             stateToStr(mState));

    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeStateChange,
                                         msg, false);
}

int Volume::createDeviceNode(const char *path, int major, int minor) {
    mode_t mode = 0660 | S_IFBLK;
    dev_t dev = (major << 8) | minor;
    if (mknod(path, mode, dev) < 0) {
        if (errno != EEXIST) {
            return -1;
        }
    }
    return 0;
}

int Volume::formatVol(bool wipe) {

    if (getState() == Volume::State_NoMedia) {
        errno = ENODEV;
        return -1;
    } else if (getState() != Volume::State_Idle) {
        errno = EBUSY;
        return -1;
    }

    if (isMountpointMounted(getMountpoint())) {
        SLOGW("Volume is idle but appears to be mounted - fixing");
        setState(Volume::State_Mounted);
        // mCurrentlyMountedKdev = XXX
        errno = EBUSY;
        return -1;
    }

    bool formatEntireDevice = (mPartIdx == -1);
    char devicePath[255];
    char label[PROPERTY_VALUE_MAX] = "";
    dev_t diskNode = getDiskDevice();
    dev_t partNode =
        MKDEV(MAJOR(diskNode),
              MINOR(diskNode) + (formatEntireDevice ? 1 : mPartIdx));

    setState(Volume::State_Formatting);

    int ret = -1;
    // Only initialize the MBR if we are formatting the entire device
    if (!strcmp(getLabel(),"internal_sd")) {
          property_get("UserVolumeLabel", label, "");
          formatEntireDevice = false;
    }
    if (formatEntireDevice) {
        sprintf(devicePath, "/dev/block/vold/%d:%d",
                major(diskNode), minor(diskNode));

        if (initializeMbr(devicePath)) {
            SLOGE("Failed to initialize MBR (%s)", strerror(errno));
            goto err;
        }
    }

    if (!strcmp(getLabel(),"internal_sd") && (major(diskNode) != 179)) {
        sprintf(devicePath, "/dev/block/vold/%d:%d",
                major(diskNode), minor(diskNode));
    } else {
        sprintf(devicePath, "/dev/block/vold/%d:%d",
                major(partNode), minor(partNode));
    }

    if (mDebug) {
        SLOGI("Formatting volume %s (%s)", getLabel(), devicePath);
    }

    if (Fat::format(devicePath, 0, wipe,label)) {
        SLOGE("Failed to format (%s)", strerror(errno));
        goto err;
    }
     if (!strcmp(getLabel(),"internal_sd")) {
          system("sync");
    }
    ret = 0;

err:
    setState(Volume::State_Idle);
    return ret;
}

bool Volume::isMountpointMounted(const char *path) {
    char device[256];
    char mount_path[256];
    char rest[256];
    FILE *fp;
    char line[1024];
	//SLOGE("isMountpointMounted path %s.",path);
    if (!(fp = fopen("/proc/mounts", "r"))) {
        SLOGE("Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    while(fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
		//SLOGE("	device=%s,mount_path=%s,rest=%s.",device, mount_path, rest);
#ifndef VOLD_BOX
        if (!strcmp(device, "tmpfs")) {
			continue;
        }
#endif
        if (!strcmp(mount_path, path)) {
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}
int Volume::mountVol() {
    dev_t deviceNodes[4];
    int n, i, rc = 0;
    char errmsg[255];

    int flags = getFlags();
    bool providesAsec = (flags & VOL_PROVIDES_ASEC) != 0;

    // TODO: handle "bind" style mounts, for emulated storage

    char decrypt_state[PROPERTY_VALUE_MAX];
    char crypto_state[PROPERTY_VALUE_MAX];
    char encrypt_progress[PROPERTY_VALUE_MAX];
    char has_ums[PROPERTY_VALUE_MAX];

#ifdef VOLD_BOX
/* $_rbox_$_modify_$_huangyonglin: added for adding the mass storage funcion*/
	if(strncmp(getLabel(),"usb_storage",strlen("usb_storage"))==0)
	{
	    SLOGW("mountVol udisk###############label =%s ",getLabel());
	    return mountUdiskVol();
	}
/* $_rbox_$_modify_$ end */
#endif

    property_get("vold.decrypt", decrypt_state, "");/* getprop vold.decrypt == NULL*/
    property_get("vold.encrypt_progress", encrypt_progress, "");/* getprop vold.encrypt_progress == NULL */
    property_get("ro.factory.hasUMS",has_ums, "false");/* getprop ro.factory.hasUMS == true */

    char getSupNtfs[PROPERTY_VALUE_MAX];
    property_get("ro.factory.storage_suppntfs", getSupNtfs, "true");/* getprop ro.factory.storage_suppntfs == true */
    bool isSupNtfs = !strcmp(getSupNtfs, "true");
    /* Don't try to mount the volumes if we have not yet entered the disk password
     * or are in the process of encrypting.
     */
    /* D/Vold    (  136): Volume internal_sd state changing 2 (Pending) -> 1 (Idle-Unmounted)*/
    if ((getState() == Volume::State_NoMedia) ||
        ((!strcmp(decrypt_state, "1") || (encrypt_progress[0] && encrypt_progress[2] != '0')) && providesAsec)) {
        snprintf(errmsg, sizeof(errmsg),
                 "Volume %s %s mount failed - no media",
                 getLabel(), getFuseMountpoint());
        mVm->getBroadcaster()->sendBroadcast(
                                         ResponseCode::VolumeMountFailedNoMedia,
                                         errmsg, false);
        errno = ENODEV;
        return -1;
    } else if (getState() != Volume::State_Idle) {
        errno = EBUSY;
        if (getState() == Volume::State_Pending) {
            mRetryMount = true;
        }
        return -1;
    }

    if (isMountpointMounted(getMountpoint())) {
        SLOGW("Volume is idle but appears to be mounted - fixing");
        setState(Volume::State_Mounted);
        // mCurrentlyMountedKdev = XXX
        return 0;
    }

    n = getDeviceNodes((dev_t *) &deviceNodes, 4);
    if (!n) {
        SLOGE("Failed to get device nodes (%s)\n", strerror(errno));
        return -1;
    }
	else
	{
		SLOGE("deviceNodes[0]= 0x%08x,[1]= 0x%08x,[2]= 0x%08x,[3]= 0x%08x",deviceNodes[0],deviceNodes[1],deviceNodes[2],deviceNodes[3]);
	}

    /* If we're running encrypted, and the volume is marked as encryptable and nonremovable,
     * and also marked as providing Asec storage, then we need to decrypt
     * that partition, and update the volume object to point to it's new decrypted
     * block device
     */
    property_get("ro.crypto.state", crypto_state, "");/* getprop ro.crypto.state == unencrypted */
    if (providesAsec &&
        ((flags & (VOL_NONREMOVABLE | VOL_ENCRYPTABLE))==(VOL_NONREMOVABLE | VOL_ENCRYPTABLE)) &&
        !strcmp(crypto_state, "encrypted") && !isDecrypted()) {
       char new_sys_path[MAXPATHLEN];
       char nodepath[256];
       int new_major, new_minor;

       if (n != 1) {
           /* We only expect one device node returned when mounting encryptable volumes */
           SLOGE("Too many device nodes returned when mounting %s\n", getMountpoint());
           return -1;
       }

       if (cryptfs_setup_volume(getLabel(), MAJOR(deviceNodes[0]), MINOR(deviceNodes[0]),
                                new_sys_path, sizeof(new_sys_path),
                                &new_major, &new_minor)) {
           SLOGE("Cannot setup encryption mapping for %s\n", getMountpoint());
           return -1;
       }
       /* We now have the new sysfs path for the decrypted block device, and the
        * majore and minor numbers for it.  So, create the device, update the
        * path to the new sysfs path, and continue.
        */
        snprintf(nodepath,
                 sizeof(nodepath), "/dev/block/vold/%d:%d",
                 new_major, new_minor);
        if (createDeviceNode(nodepath, new_major, new_minor)) {
            SLOGE("Error making device node '%s' (%s)", nodepath,
                                                       strerror(errno));
        }

        // Todo: Either create sys filename from nodepath, or pass in bogus path so
        //       vold ignores state changes on this internal device.
        updateDeviceInfo(nodepath, new_major, new_minor);

        /* Get the device nodes again, because they just changed */
        n = getDeviceNodes((dev_t *) &deviceNodes, 4);
        if (!n) {
            SLOGE("Failed to get device nodes (%s)\n", strerror(errno));
            return -1;
        }
		else
		{
			SLOGE("deviceNodes[0]= 0x%08x,[1]= 0x%08x,[2]= 0x%08x,[3]= 0x%08x",deviceNodes[0],deviceNodes[1],deviceNodes[2],deviceNodes[3]);
		}

    }

    for (i = 0; i < n; i++) {
        char devicePath[255];

        sprintf(devicePath, "/dev/block/vold/%d:%d", major(deviceNodes[i]),
                minor(deviceNodes[i]));

		/* /dev/block/vold/179:14 being considered for volume internal_sd,1 */
        SLOGI("%s being considered for volume %s,%d\n ", devicePath, getLabel(),isSupNtfs);

        errno = 0;
        setState(Volume::State_Checking);

        if (Fat::check(devicePath) && !isSupNtfs) {
            if (errno == ENODATA) {
                SLOGW("%s does not contain a FAT filesystem\n", devicePath);
                continue;
            }
            errno = EIO;
            /* Badness - abort the mount */
            SLOGE("%s failed FS checks (%s)", devicePath, strerror(errno));
            setState(Volume::State_Idle);
            return -1;
        }

        errno = 0;
        int gid;

        if(!strcmp("true",has_ums))//has UMS function ,set group to AID_SDCARD_RW
		{
			/* /dev/block/vold/179:14 /mnt/internal_sd vfat 
			 * rw,dirsync,nosuid,nodev,noexec,relatime,uid=1000,gid=1015,fmask=0007,dmask=0007,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
			 * 挂载user分区到/mnt/internal_sd  user -> /dev/block/mmcblk0p14
			 */
           	if (Fat::doMount(devicePath, getMountpoint(), false, false, false,
        	  	AID_SYSTEM,AID_SDCARD_RW, 0007, true)) {
	        	SLOGE("%s failed to mount via VFAT (%s)\n", devicePath, strerror(errno));
	        	if(providesAsec){
			     	mSkipAsec = true;
		             SLOGE("---------set mSkipAsec to disable app2sd because mount Vfat fail for %s, mountpoint =%s",getLabel(),getMountpoint());
				}
	      		if(Ntfs::doMount(devicePath, getMountpoint(), false,AID_SYSTEM,AID_SDCARD_RW)){ 
	            	SLOGE("%s failed to mount via VNTFS (%s)\n", devicePath, strerror(errno));
	            	continue;
	            }
			}
			else   //mount flash as fat succeed
	    	{
		  		mSkipAsec = false;
				// ---------set mSkipAsec to enable app2sd because mount Vfat succeed for internal_sd, mountpoint =/mnt/internal_sd
		  		SLOGE("---------set mSkipAsec to enable app2sd because mount Vfat succeed for %s, mountpoint =%s",getLabel(),getMountpoint());
			}
	    }
	    else //do not has ums,set group to AID_MEDIA_RW
	    {
        	if (Fat::doMount(devicePath, getMountpoint(), false, false, false,
                        AID_SYSTEM,AID_MEDIA_RW, 0002, true)) {
            	        SLOGE("%s failed to mount via VFAT (%s)\n", devicePath, strerror(errno));
    
				if(Ntfs::doMount(devicePath, getMountpoint(), false,AID_SYSTEM,AID_MEDIA_RW)){
               			SLOGE("%s failed to mount via VNTFS (%s)\n", devicePath, strerror(errno));
		       		continue;
		     	}
        	}
	    }   
		//blkid identified as /dev/block/vold/179:14: LABEL="ROCKCHIP" UUID="0FE6-0808" TYPE="vfat"
        extractMetadata(devicePath);

        if (providesAsec&&!mSkipAsec&& mountAsecExternal() != 0) {
            SLOGE("Failed to mount secure area (%s)", strerror(errno));
            umount(getMountpoint());
            setState(Volume::State_Idle);
            return -1;
        }

        char service[64];
        snprintf(service, 64, "fuse_%s", getLabel());
        property_set("ctl.start", service);

        setState(Volume::State_Mounted);
        mCurrentlyMountedKdev = deviceNodes[i];/* 179:14 */
        return 0;
    }

    SLOGE("Volume %s found no suitable devices for mounting :(\n", getLabel());
    setState(Volume::State_Idle);

    return -1;
}

int Volume::mountAsecExternal() {
    char legacy_path[PATH_MAX];
    char secure_path[PATH_MAX];
    char has_ums[PROPERTY_VALUE_MAX];

    property_get("ro.factory.hasUMS",has_ums, "false");


    snprintf(legacy_path, PATH_MAX, "%s/android_secure", getMountpoint());
    snprintf(secure_path, PATH_MAX, "%s/.android_secure", getMountpoint());

    // Recover legacy secure path
    if (!access(legacy_path, R_OK | X_OK) && access(secure_path, R_OK | X_OK)) {
        if (rename(legacy_path, secure_path)) {
            SLOGE("Failed to rename legacy asec dir (%s)", strerror(errno));
        }
    }

    if(!strcmp("true",has_ums))//has UMS function ,set group to AID_SDCARD_RW
    {
    	if (fs_prepare_dir(secure_path, 0770, AID_SYSTEM,AID_SDCARD_RW) != 0) {
        	return -1;
    	}
    }
    else
    {
    	if (fs_prepare_dir(secure_path, 0770, AID_SYSTEM,AID_MEDIA_RW) != 0) {
          SLOGW("fs_prepare_dir failed: %s", strerror(errno));
           return -1;
    	}
    }

    if (mount(secure_path, SEC_ASECDIR_EXT, "", MS_BIND, NULL)) {
        SLOGE("Failed to bind mount points %s -> %s (%s)", secure_path,
                SEC_ASECDIR_EXT, strerror(errno));
        return -1;
    }
	property_set("sys.vold.hasAsec","true"); 
    return 0;
}

int Volume::doUnmount(const char *path, bool force) {
    int retries = 10;

    if (mDebug) {
        SLOGD("Unmounting {%s}, force = %d", path, force);
    }

    while (retries--) {
        if (!umount(path) || errno == EINVAL || errno == ENOENT) {
            SLOGI("%s sucessfully unmounted", path);
			notifyStateKernel(2);
            return 0;
        }

        int action = 0;
		notifyStateKernel(3);
        if (force) {
            if (retries == 1) {
                action = 2; // SIGKILL
            } else if (retries == 2) {
                action = 1; // SIGHUP
            }
        }

        SLOGW("Failed to unmount %s (%s, retries %d, action %d)",
                path, strerror(errno), retries, action);

        Process::killProcessesWithOpenFiles(path, action);
        usleep(1000*30);
    }
    errno = EBUSY;
    SLOGE("Giving up on unmount %s (%s)", path, strerror(errno));
    return -1;
}

int Volume::unmountVol(bool force, bool revert, bool badremove) {
    int i, rc;

    int flags = getFlags();
    bool providesAsec = ((flags & VOL_PROVIDES_ASEC) != 0)&&(!mSkipAsec);
    revert = mPartIdx == -1 ? false : revert;

    if (getState() != Volume::State_Mounted) {
        SLOGE("Volume %s unmount request when not mounted", getLabel());
        errno = EINVAL;
        return UNMOUNT_NOT_MOUNTED_ERR;
    }

    setState(Volume::State_Unmounting);
    usleep(1000 * 1000); // Give the framework some time to react

    char service[64];
    snprintf(service, 64, "fuse_%s", getLabel());
    property_set("ctl.stop", service);
    /* Give it a chance to stop.  I wish we had a synchronous way to determine this... */
    sleep(1);

    // TODO: determine failure mode if FUSE times out

    if (providesAsec) {
		if(doUnmount(Volume::SEC_ASECDIR_EXT, force) != 0)
		{
        	SLOGE("Failed to unmount secure area on %s (%s)", getMountpoint(), strerror(errno));
        	goto out_mounted;
		}
		else
		{
			property_set("sys.vold.hasAsec","false"); 
			SLOGE("Succeed to umount secure area on %s",getMountpoint());
		}
    }

    /* Now that the fuse daemon is dead, unmount it */
    if (doUnmount(getFuseMountpoint(), force) != 0) {
        SLOGE("Failed to unmount %s (%s)", getFuseMountpoint(), strerror(errno));
        //goto fail_remount_secure;
    }

    /* Unmount the real sd card */
    if (doUnmount(getMountpoint(), force) != 0) {
        SLOGE("Failed to unmount %s (%s)", getMountpoint(), strerror(errno));
        goto fail_remount_secure;
    }

    SLOGI("%s unmounted successfully", getMountpoint());

    /* If this is an encrypted volume, and we've been asked to undo
     * the crypto mapping, then revert the dm-crypt mapping, and revert
     * the device info to the original values.
     */
    if (revert && isDecrypted()) {
        cryptfs_revert_volume(getLabel());
        revertDeviceInfo();
        SLOGI("Encrypted volume %s reverted successfully", getMountpoint());
    }

    setUuid(NULL);
    setUserLabel(NULL);
    setState(Volume::State_Idle);
    mCurrentlyMountedKdev = -1;
    return 0;

fail_remount_secure:
    if (providesAsec && mountAsecExternal() != 0) {
        SLOGE("Failed to remount secure area (%s)", strerror(errno));
        goto out_nomedia;
    }

out_mounted:
    setState(Volume::State_Mounted);
    return -1;
	
/*fail_remount_tmpfs:
    if (mount("tmpfs", SEC_STG_SECIMGDIR, "tmpfs", MS_RDONLY, "size=0,mode=0,uid=0,gid=0")) {
        SLOGE("Failed to restore tmpfs after failure! - Storage will appear offline!");
        goto out_nomedia;
    }
fail_republish:
    if (doMoveMount(SEC_STGDIR, getMountpoint(), force)) {
        SLOGE("Failed to republish mount after failure! - Storage will appear offline!");
        goto out_nomedia;
    }*/

out_nomedia:
    setState(Volume::State_NoMedia);
    return -1;
}

int Volume::initializeMbr(const char *deviceNode) {
    struct disk_info dinfo;

    memset(&dinfo, 0, sizeof(dinfo));

    if (!(dinfo.part_lst = (struct part_info *) malloc(MAX_NUM_PARTS * sizeof(struct part_info)))) {
        SLOGE("Failed to malloc prt_lst");
        return -1;
    }

    memset(dinfo.part_lst, 0, MAX_NUM_PARTS * sizeof(struct part_info));
    dinfo.device = strdup(deviceNode);
    dinfo.scheme = PART_SCHEME_MBR;
    dinfo.sect_size = 512;
    dinfo.skip_lba = 2048;
    dinfo.num_lba = 0;
    dinfo.num_parts = 1;

    struct part_info *pinfo = &dinfo.part_lst[0];

    pinfo->name = strdup("android_sdcard");
    pinfo->flags |= PART_ACTIVE_FLAG;
    pinfo->type = PC_PART_TYPE_FAT32;
    pinfo->len_kb = -1;

    int rc = apply_disk_config(&dinfo, 0);

    if (rc) {
        SLOGE("Failed to apply disk configuration (%d)", rc);
        goto out;
    }

 out:
    free(pinfo->name);
    free(dinfo.device);
    free(dinfo.part_lst);

    return rc;
}

/*
 * Use blkid to extract 提取 UUID and label from device, since it handles many
 * obscure edge cases around partition types and formats. Always broadcasts
 * updated metadata values.
 */
int Volume::extractMetadata(const char* devicePath) {
    int res = 0;

    std::string cmd;
    cmd = BLKID_PATH;
    cmd += " -c /dev/null ";
    cmd += devicePath;

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) {
        ALOGE("Failed to run %s: %s", cmd.c_str(), strerror(errno));
        res = -1;
        goto done;
    }

    char line[1024];
    char value[128];
    if (fgets(line, sizeof(line), fp) != NULL) {
		//blkid identified as /dev/block/vold/179:14: LABEL="ROCKCHIP" UUID="0FE6-0808" TYPE="vfat"
        ALOGD("blkid identified as %s", line);
		
        char* start = strstr(line, "UUID=");
        if (start != NULL && sscanf(start + 5, "\"%127[^\"]\"", value) == 1) {
            setUuid(value);
        } else {
            setUuid(NULL);
        }

        start = strstr(line, "LABEL=");
        if (start != NULL && sscanf(start + 6, "\"%127[^\"]\"", value) == 1) {
            setUserLabel(value);
        } else {
            setUserLabel(NULL);
        }
    } else {
        ALOGW("blkid failed to identify %s", devicePath);
        res = -1;
    }

    pclose(fp);

done:
    if (res == -1) {
        setUuid(NULL);
        setUserLabel(NULL);
    }
    return res;
}
