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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fnmatch.h>
#include <libgen.h>

#include <linux/kdev_t.h>

#define LOG_TAG "DirectVolume"

#include <cutils/log.h>
#include <sysutils/NetlinkEvent.h>

#include "DirectVolume.h"
#include "VolumeManager.h"
#include "ResponseCode.h"
#include "cryptfs.h"

#define PARTITION_DEBUG

PathInfo::PathInfo(const char *p)
{
    warned = false;
    pattern = strdup(p);
    if (!strchr(pattern, '*')) {
        patternType = prefix;
    } else {
        patternType = wildcard;
    }
}

PathInfo::~PathInfo()
{
    free(pattern);
}

bool PathInfo::match(const char *path)
{
    switch (patternType) {
	    case prefix:
	    {
	        bool ret = (strncmp(path, pattern, strlen(pattern)) == 0);
	        if (!warned && ret && (strlen(pattern) != strlen(path))) {
	            SLOGW("Deprecated implied prefix pattern detected, please use '%s*' instead", pattern);
	            warned = true;
	        }
	        return ret;
	    }
		
	    case wildcard:
	        return fnmatch(pattern, path, 0) == 0;
    }
    SLOGE("Bad matching type");
    return false;
}

DirectVolume::DirectVolume(VolumeManager *vm, const fstab_rec* rec, int flags) :
        Volume(vm, rec, flags) {

	mPaths = new PathCollection();

	for (int i = 0; i < MAX_PARTITIONS; i++)
        mPartMinors[i] = -1;
	
    mPendingPartCount = 0;
    mDiskMajor = -1;
    mDiskMinor = -1;
    mDiskNumParts = 0;
    mIsDecrypted = 0;

    if (strcmp(rec->mount_point, "auto") != 0) {
        ALOGE("Vold managed volumes must have auto mount point; ignoring %s",
              rec->mount_point);
    }

    char mount[PATH_MAX];
	char usb_mount[PATH_MAX];
#ifndef VOLD_BOX
    snprintf(mount, PATH_MAX, "%s/%s", Volume::MEDIA_DIR, rec->label);
    mMountpoint = strdup(mount);
    snprintf(mount, PATH_MAX, "%s/%s", Volume::FUSE_DIR, rec->label);
    mFuseMountpoint = strdup(mount);
#endif

#ifdef VOLD_BOX	
    snprintf(mount, PATH_MAX, "%s/%s", Volume::MEDIA_DIR, rec->label);/* mount = /mnt/usb_storage */
/* $_rbox_$_modify_$_begin_huangyonglin*/	
    //mMountpoint = strdup(rec->label);
	if(strncmp(rec->label,"usb_storage",strlen("usb_storage"))==0)
	{
		snprintf(usb_mount, PATH_MAX, "%s/%s", mount, mDiskVolumeLabel);
    }
	else
	{
	    snprintf(usb_mount, PATH_MAX, "%s/%s", Volume::MEDIA_DIR, rec->label);
	}
    mMountpoint = strdup(usb_mount);/* /mnt/usb_storage/UDISKX */
    snprintf(mount, PATH_MAX, "%s/%s", Volume::FUSE_DIR, rec->label);
    mFuseMountpoint = strdup(mount);
	ALOGE("DirectVolume mFuseMountpoint:%s,mMountpoint:%s.",mFuseMountpoint,mMountpoint);
/* $_rbox_$_modify_$_end*/	
#endif
    setState(Volume::State_NoMedia);
}

DirectVolume::~DirectVolume() {
    PathCollection::iterator it;

    for (it = mPaths->begin(); it != mPaths->end(); ++it)
        delete *it;
    delete mPaths;
}

int DirectVolume::addPath(const char *path) {
    mPaths->push_back(new PathInfo(path));
    return 0;
}

dev_t DirectVolume::getDiskDevice() {
    return MKDEV(mDiskMajor, mDiskMinor);
}

dev_t DirectVolume::getShareDevice() {
    if (mPartIdx != -1) {
        return MKDEV(mDiskMajor, mPartIdx);
    } else {
        return MKDEV(mDiskMajor, mDiskMinor);
    }
}

void DirectVolume::handleVolumeShared() {
    setState(Volume::State_Shared);
}

void DirectVolume::handleVolumeUnshared() {
    setState(Volume::State_Idle);
}
/* $_rbox_$_modify_$_huangyonglin: added by huangyonglin for adding the funtion to support the mass storage.*/
void DirectVolume::handleDiskForDuoPartitionRemoved(const char *devpath, NetlinkEvent *evt) {
    int major = atoi(evt->findParam("MAJOR"));
    int minor = atoi(evt->findParam("MINOR"));
    char msg[255];
    char devicePath[255];

    //setState(Volume::State_Unmounting);
    //usleep(1000 * 1000); // Give the framework some time to react	
    
    /*
    sprintf(devicePath, "/dev/block/vold/%d:%d", major,
            minor+mDiskNumParts);
    
    SLOGD("handleDiskForDuoPartitionRemoved Volume %s %s disk %d:%d removed\n", getLabel(), getMountpoint(), major, minor);
    snprintf(msg, sizeof(msg), "Volume %s %s disk removed (%d:%d)",
             getLabel(), getMountpoint(), major, minor);
    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeDiskRemoved,
                                             msg, false);
    */
	//to remove disk mount file.
	//maybe it's a ntfs udisk, in other words, the whole disk is the unique partition,not any logical partitions.
	//just remove any partitions in the disk.
	#if 0
	handleAllUdiskPartitionRemoved();
    CHANGE_ANDROIDFILESYSTEM_TO_READWRITE;
	if(rmdir(mDiskMountFilePathName)){
		SLOGE("RemoveUdiskMountFile %s Failed! errno = %s", mDiskMountFilePathName, strerror(errno));
	}
	CHANGE_ANDROIDFILESYSTEM_TO_READONLY;
    sprintf(devicePath, "/dev/block/vold/%d:%d", major,
            minor);
    SLOGE("handleDiskRemoved,ready to unlink: %s",devicePath);
    if ( 0 != unlink(devicePath) ) {
        SLOGE("Failed to unlink %s or it has been unlinked!",devicePath);
    }
	#endif 

    setState(Volume::State_Idle);

    sprintf(devicePath, "/dev/block/vold/%d:%d", major,
            minor+mDiskNumParts);
    
    SLOGD("handleDiskForDuoPartitionRemoved Volume %s %s disk %d:%d removed\n", getLabel(), getMountpoint(), major, minor);
    snprintf(msg, sizeof(msg), "Volume %s %s disk removed (%d:%d)",
             getLabel(), getMountpoint(), major, minor);
    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeDiskRemoved,
                                             msg, false);

    setState(Volume::State_NoMedia);
    //setDevPath(NULL);
}
void DirectVolume::handleUdiskPartitionRemoved(const char *devpath, NetlinkEvent *evt)
{
    int major = atoi(evt->findParam("MAJOR"));
    int minor = atoi(evt->findParam("MINOR"));
	char devicePath[255];
    const char *pMountpoint =NULL;
    int istate;
    UDISK_PARTITION_CONFIG *pUdiskPartiton;
#if 0
    pUdiskPartiton =getPartitionState(major,minor);
    if(pUdiskPartiton ==NULL)
        return  ;
    pMountpoint =pUdiskPartiton->ucMountPoint;
    SLOGD("Volume  %s partition %d:%d removed", pUdiskPartiton->ucMountPoint, major, minor);

    /*
    * Finally, unmount the actual block device from the staging dir
    */
    if (0 !=doUnmount(pUdiskPartiton->ucFilePathName, true)) {
        SLOGE("handleUdiskPartitionRemoved Failed to unmount %s (%s)", pUdiskPartiton->ucFilePathName, strerror(errno));
    }
    else
        SLOGI("handleUdiskPartitionRemoved %s unmounted sucessfully ", pUdiskPartiton->ucFilePathName);
    
	CHANGE_ANDROIDFILESYSTEM_TO_READWRITE;
    if(pUdiskPartiton->ucFilePathName)
        rmdir(pUdiskPartiton->ucFilePathName);
	CHANGE_ANDROIDFILESYSTEM_TO_READONLY;
    RemoveUdiskPartition(pMountpoint);
    sprintf(devicePath, "/dev/block/vold/%d:%d", major,
            minor);
    SLOGE("handleUdiskPartitionRemoved handlePartitionRemoved,ready to unlink: %s",devicePath);
    if ( 0 != unlink(devicePath) ) {
        SLOGE("handleUdiskPartitionRemoved Failed to unlink %s",devicePath);
    }
#endif
}
void DirectVolume::handleAllUdiskPartitionRemoved() {
	if(!mUdiskPartition || mUdiskPartition->empty())
		return;

	char devicePath[255];
	const char *pMountpoint =NULL;
	UDISK_PARTITION_CONFIG *pUdiskPartiton;
	UDisk_Partition_Collection::iterator it;
	int major;
	int minor;
	while(!mUdiskPartition->empty()) {
		SLOGD("there are %d partitions in the disk still", mUdiskPartition->size());
		it = mUdiskPartition->begin();
		pUdiskPartiton = *it;
		major = pUdiskPartiton->imajor;
		minor = pUdiskPartiton->iminor;
		pMountpoint = pUdiskPartiton->ucMountPoint;
		SLOGD("Volume  %s partition %d:%d removed", pUdiskPartiton->ucMountPoint, major, minor);

		/*
		* Finally, unmount the actual block device from the staging dir
		*/
		if (0 !=doUnmount(pUdiskPartiton->ucFilePathName, true)) {
			SLOGE("handleUdiskPartitionRemoved Failed to unmount %s (%s)", pUdiskPartiton->ucFilePathName, strerror(errno));
		}
		else
			SLOGI("handleUdiskPartitionRemoved %s unmounted sucessfully ", pUdiskPartiton->ucFilePathName);
		
		CHANGE_ANDROIDFILESYSTEM_TO_READWRITE;
		if(pUdiskPartiton->ucFilePathName)
			rmdir(pUdiskPartiton->ucFilePathName);
		CHANGE_ANDROIDFILESYSTEM_TO_READONLY;
		//RemoveUdiskPartition(pMountpoint);
		sprintf(devicePath, "/dev/block/vold/%d:%d", major,
				minor);
		SLOGE("handleUdiskPartitionRemoved handlePartitionRemoved,ready to unlink: %s",devicePath);
		if ( 0 != unlink(devicePath) ) {
			SLOGE("handleUdiskPartitionRemoved Failed to unlink %s",devicePath);
		}
		mUdiskPartition->erase(it);
		
	}
}


void DirectVolume::handleUdiskDiskAdded(const char *devpath, NetlinkEvent *evt) {
    mDiskMajor = atoi(evt->findParam("MAJOR"));
    mDiskMinor = atoi(evt->findParam("MINOR"));

    const char *tmp = evt->findParam("NPARTS");
	SLOGD("handleUdiskDiskAdded NPARTS = %s.",tmp);
	
    if (tmp) {
        mDiskNumParts = atoi(tmp);
    } else {
        SLOGW("Kernel block uevent missing 'NPARTS'");
        mDiskNumParts = 1;
    }
/*ifdef DISABLE_INTERNAL_DISK	  
		if (!strcmp(getLabel(),"udiskint"))
			handleToWriteLunfile();
endif DISABLE_INTERNAL_DISK*/
    char msg[255];
    int iPartNum;
    int partmask = 0;
    int i;
    for (i = 1; i <= mDiskNumParts; i++) {
        partmask |= (1 << i);
    }
    mPendingPartMap = partmask;
	SLOGD("handleUdiskDiskAdded mPendingPartMap = 0x%08x.",partmask);
    
    if (mDiskNumParts == 0) {
        mPartMinors[0] = mDiskMinor;
#ifdef PARTITION_DEBUG
        SLOGD("Dv::handleUdiskDiskAdded - No partitions - good to go son!");
#endif
        setState(Volume::State_Idle);

        snprintf(msg, sizeof(msg), "Volume %s %s disk inserted (%d:%d) state %d",
                 getLabel(), getMountpoint(), mDiskMajor, mDiskMinor, Volume::State_Idle);
        mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeDiskInserted,
                                                 msg, false);
         iPartNum =addUdiskPartition(mDiskMajor,mDiskMinor);
        if( -1 ==iPartNum)
            return ;
    } else {
#ifdef PARTITION_DEBUG
        SLOGD("Dv::diskIns - waiting for %d partitions (mask 0x%x)",
             mDiskNumParts, mPendingPartMap);
#endif
        setState(Volume::State_Pending);
		mIndexPartition =0;
		mDiskVolumelMinors[0] = mDiskMinor;
	
        snprintf(msg, sizeof(msg), "Volume %s %s disk inserted (%d:%d) state %d",
                 getLabel(), getMountpoint(), mDiskMajor, mDiskMinor, Volume::State_Pending);
        mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeDiskInserted,
                                                 msg, false);
    }
}

void DirectVolume::handleUdiskPartitionAdded(const char *devpath, NetlinkEvent *evt) {   //add to process usb partition add message by cc
    int major = atoi(evt->findParam("MAJOR"));
    int minor = atoi(evt->findParam("MINOR"));

    int part_num;
    char msg[255];
    int iPartNum,ii =0;

    const char *tmp = evt->findParam("PARTN");
	SLOGD("handleUdiskPartitionAdded NPARTS = part_num = %s.",tmp);
    if (tmp) {
        part_num = atoi(tmp);
    } else {
        SLOGW("Kernel block uevent missing 'PARTN'");
        part_num = 1;
    }
	
    mIndexPartition++;
 	#ifdef PARTITION_DEBUG   
    SLOGW("##################handleUdiskPartitionAdded part_num =%d    mDiskNumParts =%d   mIndexPartition =%d",
    	part_num,mDiskNumParts,mIndexPartition);
	#endif
    /*part_num = 1;//only mount partition 1 add by cc
    if (part_num > mDiskNumParts) {
        mDiskNumParts = part_num;
    }*/

	SLOGD("handleUdiskPartitionAdded major = %d mDiskMajor = %d.",major,mDiskMajor);
    if (major != mDiskMajor) {
        SLOGE("Partition '%s' has a different major than its disk!", devpath);
        return;
    }
    mPartMinors[part_num -1] = minor;
	SLOGE("handleUdiskPartitionAdded mPartMinors[0]= %d,mPartMinors[1]= %d,mPartMinors[2]= %d,mPartMinors[3]= %d",
		mPartMinors[0],mPartMinors[1],mPartMinors[2],mPartMinors[3]);
    
    //mPendingPartMap &= ~(1 << part_num);    
    mPendingPartMap &= ~(1 << mIndexPartition);    
 #ifdef PARTITION_DEBUG   
     SLOGD("Dv:partAdd: part_num = %d, minor = %d,mPendingPartMap =%x,mDiskNumParts=%d", part_num, minor,mPendingPartMap,mDiskNumParts);
 #endif
    iPartNum =addUdiskPartition(major,minor);
    if( -1 ==iPartNum)
    return ;

	if (mRetryMount == true) 
		SLOGD("mRetryMount = true.");
	else
		SLOGD("mRetryMount = false.");	

    if (!mPendingPartMap) {        //do not care about the mask ,just send PatritionAdd message to mountservice add by cc
#ifdef PARTITION_DEBUG
        SLOGD("Dv:partAdd: Got all partitions - ready to rock!  curstate =%d",getState());
#endif

     if (getState() != Volume::State_Formatting)
        {
            setState(Volume::State_Idle);
//$_rbox_$_modify_$_lijiehong: change to avoid udisk mount failure.
            if (mRetryMount == true) {
                SLOGD("!!!! mount failed,and need to retry mountVol()!!!!");
                mRetryMount = false;
                mountVol();
            }else{
                snprintf(msg, sizeof(msg), "Volume %s %s partition added (%d:%d)",
                       getLabel(), getMountpoint(), major, minor);
                SLOGD(" ---------!!!! no retry ,send msg(%s) --------!!!!",msg);
                mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumePartitionAdded,
                                                msg, false);
            }
//$_rbox_$_modify_$ end
        }
    } else {
#ifdef PARTITION_DEBUG
        SLOGD("Dv:partAdd: pending mask now = 0x%x", mPendingPartMap);
#endif
    }
}
/* $_rbox_$_modify_$ end */

int DirectVolume::handleBlockEvent(NetlinkEvent *evt) {
    const char *dp = evt->findParam("DEVPATH");
	SLOGD("directvolume DEVPATH = %s.",dp);

    PathCollection::iterator  it;
    for (it = mPaths->begin(); it != mPaths->end(); ++it) {
        if ((*it)->match(dp)) {
            /* We can handle this disk */
            int action = evt->getAction();
            const char *devtype = evt->findParam("DEVTYPE");
			SLOGD("DEVTYPE = %s.",devtype);

            if (action == NetlinkEvent::NlActionAdd) {
                int major = atoi(evt->findParam("MAJOR"));
                int minor = atoi(evt->findParam("MINOR"));
                char nodepath[255];

                snprintf(nodepath,
                         sizeof(nodepath), "/dev/block/vold/%d:%d",
                         major, minor);
                if (createDeviceNode(nodepath, major, minor)) {
                    SLOGE("Error making device node '%s' (%s)", nodepath,
                                                               strerror(errno));
                }

#ifdef VOLD_BOX		
/* $_FOR_ROCKCHIP_RBOX_$  added by huangyonglin for supporting the mass storage*/
				SLOGD("getLabel = %s,devtype = %s,getDevPath = %s.",getLabel(),devtype,getDevPath());
                if (strncmp(getLabel(),"usb_storage",strlen("usb_storage"))==0){
                    if (!strcmp(devtype, "disk")) 
                    {
                        if(getDevPath() == NULL)
                        {
                            setDevPath(dp);
                            handleUdiskDiskAdded(dp, evt);
                        }
                        else
                        {
                            return -1;
                        }
                    } 
                    else 
                    {
                        char *pDevPah =(char *)getDevPath();
						if(pDevPah){
							SLOGE("#########NetlinkEvent::NlActionAdd  Partition  pDevPah=%s dp",pDevPah,dp);
							}
						else{
								return -1;
							}
                        if (!strncmp(dp, pDevPah, strlen(pDevPah)))
                        {
                            handleUdiskPartitionAdded(dp,evt);
                        }
                        else
                        {
                            return -1;
                        }
                    }
                }
/* $_rbox_$_modify_$ end */
#endif				
				else if (!strcmp(devtype, "disk")) {
					char *p1 = basename(dp);
					if (strstr(p1,"boot0") != 0 || strstr(p1,"boot1") != 0) {
						SLOGD("skia mmc boot disk ! path : %s",dp);
						continue;
					}
                    handleDiskAdded(dp, evt);
                } else {
                    handlePartitionAdded(dp, evt);
                }
                /* Send notification iff disk is ready (ie all partitions found) */
                if (getState() == Volume::State_Idle) {
                    char msg[255];

                    snprintf(msg, sizeof(msg),
                             "Volume %s %s disk inserted (%d:%d)", getLabel(),
                             getFuseMountpoint(), mDiskMajor, mDiskMinor);
                    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeDiskInserted,
                                                         msg, false);
                }
            } else if (action == NetlinkEvent::NlActionRemove) {
#ifdef VOLD_BOX	            
           	 	/* $_FOR_ROCKCHIP_RBOX_$  added by huangyonglin for supporting the mass storage*/
                if (strncmp(getLabel(),"usb_storage",strlen("usb_storage"))==0)
                {
                    char *pDevPah =(char *)getDevPath();
                    if(pDevPah ==NULL)
                        return -1;
                    else if(strncmp(dp, pDevPah, strlen(pDevPah)))
                        return -1;
        	           SLOGE("#######################################NlActionRemove pDevPah=%s,devtype=%s",pDevPah,devtype);
                    if (!strcmp(devtype, "disk")) {
						//setState(Volume::State_Unmounting);
                        //handleDiskForDuoPartitionRemoved(dp, evt);
                         handleDiskRemoved(dp, evt);
						//setDevPath(NULL);
                    } else {
                    //setState(Volume::State_Unmounting);
                	/*if(getState() >= Volume::State_Idle){
                		SLOGI("#####22Volume NlActionRemove sleep 5 sec");
						usleep(1000*300);
                	}
					while(getState() == Volume::State_Checking || getState() == Volume::State_Pending){
						SLOGI("#### wait for mount successfully");
						usleep(1000*200);
					}
					handleUdiskPartitionRemoved(dp, evt);*/
					handlePartitionRemoved(dp, evt);
                    }            	    
            	}
				/* $_rbox_$_modify_$ end */
#endif				
                if (!strcmp(devtype, "disk")) {					
                    handleDiskRemoved(dp, evt);
                } else {
                    handlePartitionRemoved(dp, evt);
                }
            } else if (action == NetlinkEvent::NlActionChange) {
                if (!strcmp(devtype, "disk")) {
                    handleDiskChanged(dp, evt);
                } else {
                    handlePartitionChanged(dp, evt);
                }
            } else {
                    SLOGW("Ignoring non add/remove/change event");
            }

            return 0;
        }
    }
    errno = ENODEV;
    return -1;
}

void DirectVolume::handleDiskAdded(const char * /*devpath*/,
                                   NetlinkEvent *evt) {
    mDiskMajor = atoi(evt->findParam("MAJOR"));
    mDiskMinor = atoi(evt->findParam("MINOR"));

    const char *tmp = evt->findParam("NPARTS");
    if (tmp) {
        mDiskNumParts = atoi(tmp);
    } else {
        SLOGW("Kernel block uevent missing 'NPARTS'");
        mDiskNumParts = 1;
    }

 
#ifdef PARTITION_DEBUG
       SLOGD("----handleDiskAdded,mDiskNumParts =%d,mDiskMajor=%d,mDiskMinor=%d",mDiskNumParts,mDiskMajor,mDiskMinor);
#endif

    char msg[255];

    mPendingPartCount = mDiskNumParts;
    for (int i = 0; i < MAX_PARTITIONS; i++)
        mPartMinors[i] = -1;

    if (mDiskNumParts == 0) {
#ifdef PARTITION_DEBUG
        SLOGD("Dv::diskIns - No partitions - good to go son!");
#endif
        setState(Volume::State_Idle);
        snprintf(msg, sizeof(msg), "Volume %s %s disk inserted (%d:%d)",
                    getLabel(), getMountpoint(), mDiskMajor, mDiskMinor);
        mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeDiskInserted,msg, false);
    } else {
#ifdef PARTITION_DEBUG
        SLOGD("Dv::diskIns - waiting for %d pending partitions", mPendingPartCount);
#endif
        setState(Volume::State_Pending);
    }
}

void DirectVolume::handlePartitionAdded(const char *devpath, NetlinkEvent *evt) {
    int major = atoi(evt->findParam("MAJOR"));
    int minor = atoi(evt->findParam("MINOR"));

    int part_num;

    const char *tmp = evt->findParam("PARTN");/*这个主分区里面子分区索引，类似次设备号*/

    if (tmp) {
        part_num = atoi(tmp);
    } else {
        SLOGW("Kernel block uevent missing 'PARTN'");
        part_num = 1;
    }

    if (part_num > MAX_PARTITIONS || part_num < 1) {
        SLOGE("Invalid 'PARTN' value");
        return;
    }

    if (part_num > mDiskNumParts) {
        if (mDiskNumParts == 0)
            mDiskNumParts = part_num;
        else
            part_num = mDiskNumParts;
    }

#ifdef PARTITION_DEBUG
       SLOGD("---handlePartitionAdded,part_num=%d,major=%d,minor=%d,mDiskMajor=%d",part_num,major,minor,mDiskMajor);
#endif
    if (major != mDiskMajor) {
        SLOGE("Partition '%s' has a different major than its disk!", devpath);
        return;
    }
#ifdef PARTITION_DEBUG
    SLOGD("Dv:partAdd: part_num = %d, minor = %d\n", part_num, minor);
#endif
    if (part_num >= MAX_PARTITIONS) {
        SLOGE("Dv:partAdd: ignoring part_num = %d (max: %d)\n", part_num, MAX_PARTITIONS-1);
    } else {
        if ((mPartMinors[part_num - 1] == -1) && mPendingPartCount)
            mPendingPartCount--;
        mPartMinors[part_num -1] = minor;
    }

    if (!mPendingPartCount) {
#ifdef PARTITION_DEBUG
        SLOGD("Dv:partAdd: Got all partitions - ready to rock!");
#endif
        if (getState() != Volume::State_Formatting) {
            setState(Volume::State_Idle);
            if (mRetryMount == true) {
                mRetryMount = false;
                mountVol();
            }
        }
    } else {
#ifdef PARTITION_DEBUG
        SLOGD("Dv:partAdd: pending %d disk", mPendingPartCount);
#endif
    }
}

void DirectVolume::handleDiskChanged(const char * /*devpath*/,
                                     NetlinkEvent *evt) {
    int major = atoi(evt->findParam("MAJOR"));
    int minor = atoi(evt->findParam("MINOR"));

    if ((major != mDiskMajor) || (minor != mDiskMinor)) {
        return;
    }

    SLOGI("Volume %s disk has changed", getLabel());
    const char *tmp = evt->findParam("NPARTS");
    if (tmp) {
        mDiskNumParts = atoi(tmp);
    } else {
        SLOGW("Kernel block uevent missing 'NPARTS'");
        mDiskNumParts = 1;
    }

    mPendingPartCount = mDiskNumParts;
    for (int i = 0; i < MAX_PARTITIONS; i++)
        mPartMinors[i] = -1;

    if (getState() != Volume::State_Formatting) {
        if (mDiskNumParts == 0) {
            setState(Volume::State_Idle);
        } else {
            setState(Volume::State_Pending);
        }
    }
}

void DirectVolume::handlePartitionChanged(const char * /*devpath*/,
                                          NetlinkEvent *evt) {
    int major = atoi(evt->findParam("MAJOR"));
    int minor = atoi(evt->findParam("MINOR"));
    SLOGD("Volume %s %s partition %d:%d changed\n", getLabel(), getMountpoint(), major, minor);
}

void DirectVolume::handleDiskRemoved(const char * /*devpath*/,
                                     NetlinkEvent *evt) {
    int major = atoi(evt->findParam("MAJOR"));
    int minor = atoi(evt->findParam("MINOR"));
    char msg[255];
    char devicePath[255];
    bool enabled;

    if (mVm->shareEnabled(getLabel(), "ums", &enabled) == 0 && enabled) {
        mVm->unshareVolume(getLabel(), "ums");
    }

	if ( 0 == mDiskNumParts ) {
			SLOGD("####################################Volume %s %s disk %d:%d removed\n", getLabel(), getMountpoint(), major, minor);
			snprintf(msg, sizeof(msg), "Volume %s %s bad removal (%d:%d)",
					getLabel(), getMountpoint(), major, minor);
			mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeBadRemoval,
					msg, false);
		}

   #if 0
    sprintf(devicePath, "/dev/block/vold/%d:%d", major,minor);

    if (access(devicePath, R_OK) == 0) {
        SLOGD("current mounted dev exist,access devicePath: %s ;partitionNum: %d", devicePath, mDiskNumParts);

        /*
         * Confirm partition removed.
         */
         if (Volume::unmountVol(true, false)) {
            SLOGE("Failed to unmount volume on bad removal (%s)",
                 strerror(errno));
             // XXX: At this point we're screwed for now
         } else {
             SLOGD("Crisis averted");
         }

         SLOGE("handlePartitionRemoved,ready to unlink: %s",devicePath);
         if ( 0 != unlink(devicePath) ) {
             SLOGE("Failed to unlink %s",devicePath);
         }
    }
    SLOGD("Volume %s %s disk %d:%d removed\n", getLabel(), getMountpoint(), major, minor);
    snprintf(msg, sizeof(msg), "Volume %s %s disk removed (%d:%d)",
             getLabel(), getFuseMountpoint(), major, minor);
    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeDiskRemoved,
                                             msg, false);
    setState(Volume::State_NoMedia);
	#endif 
}

void DirectVolume::handlePartitionRemoved(const char * /*devpath*/,
                                          NetlinkEvent *evt) {
    int major = atoi(evt->findParam("MAJOR"));
    int minor = atoi(evt->findParam("MINOR"));
    char msg[255];
    int state;

    SLOGD("Volume %s %s partition %d:%d removed\n", getLabel(), getMountpoint(), major, minor);

    /*
     * The framework doesn't need to get notified of
     * partition removal unless it's mounted. Otherwise
     * the removal notification will be sent on the Disk
     * itself
     */
    state = getState();
    /*if (state != Volume::State_Mounted && state != Volume::State_Shared) {
        return;
    }*/
    #if 0
    if ((dev_t) MKDEV(major, minor) == mCurrentlyMountedKdev) {
        /*
         * Yikes, our mounted partition is going away!
         */

        bool providesAsec = (getFlags() & VOL_PROVIDES_ASEC) != 0;
        if (providesAsec && mVm->cleanupAsec(this, true)) {
            SLOGE("Failed to cleanup ASEC - unmount will probably fail!");
        }

        snprintf(msg, sizeof(msg), "Volume %s %s bad removal (%d:%d)",
                 getLabel(), getFuseMountpoint(), major, minor);
        mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeBadRemoval,
                                             msg, false);

        if (Volume::unmountVol(true, false)) {
            SLOGE("Failed to unmount volume on bad removal (%s)", 
                 strerror(errno));
            // XXX: At this point we're screwed for now
        } else {
            SLOGD("Crisis averted");
        }

    } else if (state == Volume::State_Shared) {
        /* removed during mass storage */
         snprintf(msg, sizeof(msg), "Volume %s %s bad removal (%d:%d)",
                 getLabel(), getFuseMountpoint(), major, minor);
        mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeBadRemoval,
                                             msg, false);

        if (mVm->unshareVolume(getLabel(), "ums")) {
            SLOGE("Failed to unshare volume on bad removal (%s)",
                strerror(errno));
        } else {
            SLOGD("Crisis averted");
        }
    }
	#endif
	if (state == Volume::State_Shared) {
        /* removed during mass storage */
         snprintf(msg, sizeof(msg), "Volume %s %s bad removal (%d:%d)",
                 getLabel(), getFuseMountpoint(), major, minor);
        mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeBadRemoval,
                                             msg, false);

        if (mVm->unshareVolume(getLabel(), "ums")) {
            SLOGE("Failed to unshare volume on bad removal (%s)",
                strerror(errno));
        } else {
            SLOGD("Crisis averted");
        }
    }else{
	     bool providesAsec = (getFlags() & VOL_PROVIDES_ASEC) != 0;
        if (providesAsec && mVm->cleanupAsec(this, true)) {
            SLOGE("Failed to cleanup ASEC - unmount will probably fail!");
        }

		SLOGE("################sendbroadcast---------");
        snprintf(msg, sizeof(msg), "Volume %s %s bad removal (%d:%d)",
                 getLabel(), getFuseMountpoint(), major, minor);
        mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeBadRemoval,
                                             msg, false);
    }
}

/*
 * Called from base to get a list of devicenodes for mounting
 */
int DirectVolume::getDeviceNodes(dev_t *devs, int max) {
	SLOGD("getDeviceNodes mPartIdx = %d, mDiskNumParts = %d.",mPartIdx,mDiskNumParts);
    if (mPartIdx == -1) {
        // If the disk has no partitions, try the disk itself
        if (!mDiskNumParts) {
            devs[0] = MKDEV(mDiskMajor, mDiskMinor);
            return 1;
        }

        int i;
        for (i = 0; i < mDiskNumParts; i++) {
            if (i == max)
                break;
            devs[i] = MKDEV(mDiskMajor, mPartMinors[i]);
        }
        return mDiskNumParts;
    }
    devs[0] = MKDEV(mDiskMajor, mPartMinors[mPartIdx -1]);
    return 1;
}

/*
 * Called from base to update device info,
 * e.g. When setting up an dm-crypt mapping for the sd card.
 */
int DirectVolume::updateDeviceInfo(char *new_path, int new_major, int new_minor)
{
    PathCollection::iterator it;

    if (mPartIdx == -1) {
        SLOGE("Can only change device info on a partition\n");
        return -1;
    }

    /*
     * This is to change the sysfs path associated with a partition, in particular,
     * for an internal SD card partition that is encrypted.  Thus, the list is
     * expected to be only 1 entry long.  Check that and bail if not.
     */
    if (mPaths->size() != 1) {
        SLOGE("Cannot change path if there are more than one for a volume\n");
        return -1;
    }

    it = mPaths->begin();
    delete *it; /* Free the string storage */
    mPaths->erase(it); /* Remove it from the list */
    addPath(new_path); /* Put the new path on the list */

    /* Save away original info so we can restore it when doing factory reset.
     * Then, when doing the format, it will format the original device in the
     * clear, otherwise it just formats the encrypted device which is not
     * readable when the device boots unencrypted after the reset.
     */
    mOrigDiskMajor = mDiskMajor;
    mOrigDiskMinor = mDiskMinor;
    mOrigPartIdx = mPartIdx;
    memcpy(mOrigPartMinors, mPartMinors, sizeof(mPartMinors));

    mDiskMajor = new_major;
    mDiskMinor = new_minor;
    /* Ugh, virual block devices don't use minor 0 for whole disk and minor > 0 for
     * partition number.  They don't have partitions, they are just virtual block
     * devices, and minor number 0 is the first dm-crypt device.  Luckily the first
     * dm-crypt device is for the userdata partition, which gets minor number 0, and
     * it is not managed by vold.  So the next device is minor number one, which we
     * will call partition one.
     */
    mPartIdx = new_minor;
    mPartMinors[new_minor-1] = new_minor;

    mIsDecrypted = 1;

    return 0;
}

/*
 * Called from base to revert device info to the way it was before a
 * crypto mapping was created for it.
 */
void DirectVolume::revertDeviceInfo(void)
{
    if (mIsDecrypted) {
        mDiskMajor = mOrigDiskMajor;
        mDiskMinor = mOrigDiskMinor;
        mPartIdx = mOrigPartIdx;
        memcpy(mPartMinors, mOrigPartMinors, sizeof(mPartMinors));

        mIsDecrypted = 0;
    }

    return;
}

/*
 * Called from base to give cryptfs all the info it needs to encrypt eligible volumes
 */
int DirectVolume::getVolInfo(struct volume_info *v)
{
    strcpy(v->label, mLabel);
    strcpy(v->mnt_point, mMountpoint);
    v->flags = getFlags();
    /* Other fields of struct volume_info are filled in by the caller or cryptfs.c */

    return 0;
}
