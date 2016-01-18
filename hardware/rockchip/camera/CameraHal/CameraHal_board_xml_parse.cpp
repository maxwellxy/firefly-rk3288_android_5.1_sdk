

#include <stdlib.h>
#include <utils/Log.h>
#include <libexpat/expat.h>
#include <dlfcn.h>
#include <isi/isi_iss.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <linux/ion.h>
#if !defined(ANDROID_5_X)
#include <linux/videodev.h>
#include <linux/android_pmem.h>
#endif
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <linux/types.h>
#include <linux/version.h>

#include "CameraHal_board_xml_parse.h"
#include "CameraHal_Tracer.h"

void camera_board_profiles::ParserSensorInfo(const char *name, const  char **atts, void *userData)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
    rk_sensor_info *pSensorInfo = &(pCamInfo->mHardInfo.mSensorInfo);
	int result;    
    if (strcmp(name, "SensorName")==0) {
        strlcpy(pSensorInfo->mSensorName, atts[1], sizeof(pSensorInfo->mSensorName));
        ALOGD("%s(%d): SensorName(%s)\n", __FUNCTION__, __LINE__, pSensorInfo->mSensorName);
    } else if (strcmp(name, "SensorLens")==0) {
        strlcpy(pSensorInfo->mLensName, atts[1], sizeof(pSensorInfo->mLensName));
        ALOGD("%s(%d): lensName(%s)\n", __FUNCTION__, __LINE__, pSensorInfo->mLensName);
    } else if (strcmp(name, "SensorDevID")==0) {
        ALOGD("%s(%d): SensorDevID(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        if(strcmp("CAMSYS_DEVID_SENSOR_1A", atts[1])==0){
            pSensorInfo->mCamDevid = CAMSYS_DEVID_SENSOR_1A;
        }else if(strcmp("CAMSYS_DEVID_SENSOR_1B", atts[1])==0){
            pSensorInfo->mCamDevid = CAMSYS_DEVID_SENSOR_1B; 
        }else if(strcmp("CAMSYS_DEVID_SENSOR_2", atts[1])==0){
            pSensorInfo->mCamDevid = CAMSYS_DEVID_SENSOR_2; 
        }else{
            pSensorInfo->mCamDevid = 0;
            ALOGD("%s(%d): SensorDevID(%s) don't support\n", __FUNCTION__, __LINE__, atts[1]);
        }
        ALOGD("%s(%d): SensorDevID(%d)\n", __FUNCTION__, __LINE__, pSensorInfo->mCamDevid);
    } else if (strcmp(name,"SensorHostDevID")==0){
        ALOGD("%s(%d): SensorHostDevID(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        if(strcmp("CAMSYS_DEVID_MARVIN", atts[1])==0){
            pSensorInfo->mHostDevid= CAMSYS_DEVID_MARVIN;
            strlcpy(pSensorInfo->mCamsysDevPath, "/dev/camsys_marvin", sizeof(pSensorInfo->mCamsysDevPath));
        }else if(strcmp("CAMSYS_DEVID_CIF_0", atts[1])==0){
            pSensorInfo->mHostDevid = CAMSYS_DEVID_CIF_0; 
            strlcpy(pSensorInfo->mCamsysDevPath, "/dev/camsys_cif0", sizeof(pSensorInfo->mCamsysDevPath));
        }else if(strcmp("CAMSYS_DEVID_CIF_1", atts[1])==0){
            pSensorInfo->mHostDevid = CAMSYS_DEVID_CIF_1; 
            strlcpy(pSensorInfo->mCamsysDevPath, "/dev/camsys_cif1", sizeof(pSensorInfo->mCamsysDevPath));
        }else {
            pSensorInfo->mHostDevid = 0;
            ALOGD("%s(%d): SensorDevID(%s) don't support\n", __FUNCTION__, __LINE__, atts[1]);
        }
    } else if (strcmp(name,"SensorI2cBusNum")==0){
        ALOGD("%s(%d): Sensori2cBusNum(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pSensorInfo->mSensorI2cBusNum = atoi(atts[1]);
    } else if (strcmp(name,"SensorI2cAddrByte")==0){
        ALOGD("%s(%d): SensorI2cAddrByte(%s)\n", __FUNCTION__, __LINE__, atts[1]); 
        pSensorInfo->mI2cAddrBytes = atoi(atts[1]);
    } else if (strcmp(name,"SensorI2cRate")==0){
        ALOGD("%s(%d): SensorI2cRate(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pSensorInfo->mSensorI2cRate = atoi(atts[1]);
    } else if (strcmp(name,"SensorMclk")==0){
        ALOGD("%s(%d): SensorMclk(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pSensorInfo->mMclkRate = atoi(atts[1]);
    } else if (strcmp(name,"SensorAvdd")==0){
        ALOGD("%s(%d): SensorAvdd(%s) min(%s) max(%s)\n", __FUNCTION__, __LINE__, atts[1], atts[3], atts[5]);
        strlcpy((char*)pSensorInfo->mAvdd.name, (atts[1]), sizeof(pSensorInfo->mAvdd.name));
        pSensorInfo->mAvdd.min_uv = atoi(atts[3]);
        pSensorInfo->mAvdd.max_uv = atoi(atts[5]);
    } else if (strcmp(name,"SensorDovdd")==0){
        ALOGD("%s(%d): SensorDovdd(%s) min(%s) max(%s)\n", __FUNCTION__, __LINE__, atts[1], atts[3], atts[5]);
        strlcpy((char*)pSensorInfo->mDovdd.name, (atts[1]), sizeof(pSensorInfo->mDovdd.name));
        pSensorInfo->mDovdd.min_uv = atoi(atts[3]);   
        pSensorInfo->mDovdd.max_uv = atoi(atts[5]);
    } else if (strcmp(name,"SensorDvdd")==0){
        ALOGD("%s(%d): SensorDvdd(%s) min(%s) max(%s)\n", __FUNCTION__, __LINE__, atts[1], atts[3], atts[5]);
        strlcpy((char*)pSensorInfo->mDvdd.name, (atts[1]), sizeof(pSensorInfo->mDvdd.name));
        pSensorInfo->mDvdd.min_uv = atoi(atts[3]);  
        pSensorInfo->mDvdd.max_uv = atoi(atts[5]);
    } else if (strcmp(name,"SensorGpioPwdn")==0){
        ALOGD("%s(%d): SensorGpioPwdn(%s) active(%s) \n", __FUNCTION__, __LINE__, atts[1], atts[3]);
        strlcpy((char*)pSensorInfo->mSensorGpioPwdn.name, (atts[1]), sizeof(pSensorInfo->mSensorGpioPwdn.name));
        pSensorInfo->mSensorGpioPwdn.active = atoi(atts[3]);
    } else if (strcmp(name,"SensorGpioRst")==0){
        ALOGD("%s(%d): SensorGpioRst(%s) active(%s) \n", __FUNCTION__, __LINE__, atts[1], atts[3]);
        strlcpy((char*)pSensorInfo->mSensorGpioReset.name, (atts[1]), sizeof(pSensorInfo->mSensorGpioReset.name));
        pSensorInfo->mSensorGpioReset.active = atoi(atts[3]);
    } else if (strcmp(name,"SensorGpioPwen")==0){
        ALOGD("%s(%d): SensorGpioPwen(%s) active(%s) \n", __FUNCTION__, __LINE__, atts[1], atts[3]);
        strlcpy((char*)pSensorInfo->SensorGpioPwen.name, (atts[1]), sizeof(pSensorInfo->SensorGpioPwen.name));
        pSensorInfo->SensorGpioPwen.active = atoi(atts[3]);
    }else if (strcmp(name,"SensorFacing")==0){
        ALOGD("%s(%d): SensorFacing(%s) \n", __FUNCTION__, __LINE__, atts[1]);
        if(strcmp("front", atts[1])==0){
            pSensorInfo->mFacing = RK_CAM_FACING_FRONT;
        }else if(strcmp("back", atts[1])==0){
            pSensorInfo->mFacing = RK_CAM_FACING_BACK;
        }else{
            ALOGD("%s(%d): SensorFacing(%s) is wrong \n", __FUNCTION__, __LINE__, atts[1]);
        }
    } else if (strcmp(name,"SensorInterface")==0){
        ALOGD("%s(%d): SensorInterface(%s) \n", __FUNCTION__, __LINE__, atts[1]);
        if(strcmp("CCIR601", atts[1])==0){
            pSensorInfo->mMode = CCIR601;
        }else if(strcmp("MIPI", atts[1])==0){
            pSensorInfo->mMode = MIPI;
        }else if(strcmp("SMIA", atts[1])==0){
            pSensorInfo->mMode = SMIA;
        }else if(strcmp("CCIR656", atts[1])==0){
            pSensorInfo->mMode = CCIR656;
        }else{
            ALOGD("%s(%d): SensorInterface(%s) don't support \n", __FUNCTION__, __LINE__, atts[1]);
        }
    } else if (strcmp(name,"SensorMirrorFlip")==0){
        ALOGD("%s(%d): SensorMirrorFlip(%s) \n", __FUNCTION__, __LINE__, atts[1]);
        pSensorInfo->mMirrorFilp = atoi(atts[1]);
    } else if (strcmp(name,"SensorPowerupSequence")==0){
        ALOGD("%s(%d): SensorPowerupSequence(%s) \n", __FUNCTION__, __LINE__, atts[1]);
        pSensorInfo->mSensorPowerupSequence = atoi(atts[1]);
    } else if(strcmp(name, "SensorOrientation")==0){
        ALOGD("%s(%d): SensorOrientation(%s) \n", __FUNCTION__, __LINE__, atts[1]);
        pSensorInfo->mOrientation = atoi(atts[1]);
    }else if(strcmp(name, "SensorDriver")==0){
        ALOGD("%s(%d): SensorDriver(%s) \n", __FUNCTION__, __LINE__, atts[1]);
        strlcpy(pSensorInfo->mSensorDriver, atts[1], sizeof(pSensorInfo->mSensorDriver));
    }else if(strcmp(name, "SensorPhy")==0){
        camsys_fmt_t fmt;
        
        if(strcmp(atts[7], "CamSys_Fmt_Yuv420_8b")==0){
            fmt = CamSys_Fmt_Yuv420_8b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Yuv420_10b")==0){
            fmt = CamSys_Fmt_Yuv420_10b;
        }else if(strcmp(atts[7], "CamSys_Fmt_LegacyYuv420_8b")==0){
            fmt = CamSys_Fmt_LegacyYuv420_8b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Yuv422_8b")==0){
            fmt = CamSys_Fmt_Yuv422_8b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Yuv422_10b")==0){
            fmt = CamSys_Fmt_Yuv422_10b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Raw_6b")==0){
            fmt = CamSys_Fmt_Raw_6b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Raw_7b")==0){
            fmt = CamSys_Fmt_Raw_7b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Raw_8b")==0){
            fmt = CamSys_Fmt_Raw_8b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Raw_10b")==0){
            fmt = CamSys_Fmt_Raw_10b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Raw_12b")==0){
            fmt = CamSys_Fmt_Raw_12b;
        }else if(strcmp(atts[7], "CamSys_Fmt_Raw_14b")==0){
            fmt = CamSys_Fmt_Raw_14b;
        }else {
            fmt = CamSys_Fmt_Raw_10b;
           ALOGE("%s(%d):  unknown fmt (%s) \n", __FUNCTION__, __LINE__ , atts[7]); 
        }
    
        if(strcmp(atts[1], "CamSys_Phy_Mipi")==0){
            pSensorInfo->mPhy.type = CamSys_Phy_Mipi;
			int laneNum = atoi(atts[3]);
			if(laneNum<=1 || laneNum>4){		
				ALOGE("%s(%d): SensorPhy laneNum wrong (%s) range[1,4]\n", __FUNCTION__, __LINE__, atts[3]);
				if(laneNum<1)
					laneNum=1;
				if(laneNum>4)
					laneNum=4;
			}
			
			int data_en_bit = 0;
			int i=0;
			while(laneNum){
				data_en_bit |= (1<<i) ;
				laneNum--;
				i++;
			}
			
			int phyIndex = atoi(atts[5]);
			if(phyIndex<0 || phyIndex>1){
				ALOGE("%s(%d): SensorPhy phyIndex wrong (%s) range[0,1]\n", __FUNCTION__, __LINE__, atts[9]);
				if(phyIndex<0)
					phyIndex = 0;
				if(phyIndex>1)
					phyIndex = 1;
			}
			
            pSensorInfo->mPhy.info.mipi.data_en_bit = data_en_bit;
			pSensorInfo->mPhy.info.mipi.phy_index = phyIndex;
			pSensorInfo->laneNum = atoi(atts[3]);
            pSensorInfo->fmt = fmt;
            ALOGD("%s(%d): SensorPhy: MIPI  lane: %d  phyindex: %d  fmt: 0x%x\n",
                __FUNCTION__,__LINE__,pSensorInfo->laneNum, pSensorInfo->mPhy.info.mipi.phy_index , fmt);
        }else if(strcmp(atts[1], "CamSys_Phy_Cif")==0){
            pSensorInfo->mPhy.type = CamSys_Phy_Cif;

            if (strcmp(atts[3], "0") == 0) {
                pSensorInfo->mPhy.info.cif.cifio = CamSys_SensorBit0_CifBit0;
            } else if (strcmp(atts[3], "2") == 0) {
                pSensorInfo->mPhy.info.cif.cifio = CamSys_SensorBit0_CifBit2;
            }else if (strcmp(atts[3], "4") == 0) {
                pSensorInfo->mPhy.info.cif.cifio = CamSys_SensorBit0_CifBit4;
            }
                
            pSensorInfo->mPhy.info.cif.cif_num = atoi(atts[5]);            
            pSensorInfo->mPhy.info.cif.fmt = fmt;

            ALOGD("%s(%d): SensorPhy: CIF sensor_d0_to_cif_d: %s  cifnum: %d  fmt: 0x%x\n",
                __FUNCTION__,__LINE__,atts[3], pSensorInfo->mPhy.info.cif.cif_num , fmt);
            
        }else{
           ALOGE("%s(%d): unknown phy mode(%s) \n" ,__FUNCTION__,__LINE__, atts[1]); 
        }
        strlcpy(pSensorInfo->mSensorDriver, atts[1], sizeof(pSensorInfo->mSensorDriver));
    }
	else if(strcmp(name, "SensorFovParemeter")==0){
		sscanf(atts[1], "%f", &(pSensorInfo->fov_h));
		sscanf(atts[3], "%f", &(pSensorInfo->fov_v));
		ALOGD("%s(%d): SensorFovParemeter fov_h(%s)(%f) fov_v(%s)(%f)  \n", __FUNCTION__, __LINE__, atts[1],pSensorInfo->fov_h, atts[3],pSensorInfo->fov_v);
	}
	else if(strcmp(name, "SensorAWB_Frame_Skip")==0){
    	ALOGD("%s(%d): SensorAWB_Frame_Skip fps(%s) \n", __FUNCTION__, __LINE__, atts[1]);
        pSensorInfo->awb_frame_skip = atoi(atts[1]);
	}
}

void camera_board_profiles::ParserVCMInfo(const char *name, const char **atts, void *userData)
{
    camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	rk_vcm_info *pVcmInfo = &(pCamInfo->mHardInfo.mVcmInfo);

    if (strcmp(name, "VCMDrvName")==0) {
        ALOGD("%s(%d): VCMDrvName(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        strlcpy(pVcmInfo->mVcmDrvName, atts[1], sizeof(pVcmInfo->mVcmDrvName));
    } else if (strcmp(name, "VCMName")==0) {
        ALOGD("%s(%d): VCMName(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        strlcpy(pVcmInfo->mVcmName, atts[1], sizeof(pVcmInfo->mVcmName));
    } else if (strcmp(name, "VCMI2cBusNum")==0) {
        ALOGD("%s(%d): VCMI2cBusNum(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pVcmInfo->mVcmI2cBusNum = atoi(atts[1]);
    } else if (strcmp(name,"VCMI2cAddrByte")==0){
        ALOGD("%s(%d): VCMI2cAddrByte(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pVcmInfo->mI2cAddrBytes = atoi(atts[1]);
    } else if (strcmp(name,"VCMI2cRate")==0){
        ALOGD("%s(%d): VCMI2cRate(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pVcmInfo->mVcmI2cRate = atoi(atts[1]);
    } else if (strcmp(name,"VCMGpioPwdn")==0){
        ALOGD("%s(%d): VCMGpioPwdn(%s) active(%s) \n", __FUNCTION__, __LINE__, atts[1], atts[3]);
        strlcpy((char*)pVcmInfo->mVcmGpioPwdn.name, (atts[1]), sizeof(pVcmInfo->mVcmGpioPwdn.name));
        pVcmInfo->mVcmGpioPwdn.active = atoi(atts[3]);
    } else if (strcmp(name,"VCMGpioPower")==0){
        ALOGD("%s(%d): VCMGpioPower(%s) active(%s) \n", __FUNCTION__, __LINE__, atts[1], atts[3]);
        strlcpy((char*)pVcmInfo->mVcmGpioPower.name, (atts[1]), sizeof(pVcmInfo->mVcmGpioPower.name));
        pVcmInfo->mVcmGpioPower.active = atoi(atts[3]);
    } else if (strcmp(name,"VCMVdd")==0){
        ALOGD("%s(%d): VCMVdd(%s) min(%s) max(%s)\n", __FUNCTION__, __LINE__, atts[1], atts[3], atts[5]);
        strlcpy((char*)pVcmInfo->mVcmVdd.name, (atts[1]), sizeof(pVcmInfo->mVcmVdd.name));       
        pVcmInfo->mVcmVdd.min_uv= atoi(atts[3]);
        pVcmInfo->mVcmVdd.max_uv= atoi(atts[5]);
    } else if (strcmp(name,"VCMCurrent") == 0) {        
        pVcmInfo->mStartCurrent = atoi(atts[1]);
        pVcmInfo->mRatedCurrent = atoi(atts[3]);
        pVcmInfo->mVcmMaxCurrent = atoi(atts[5]);
        pVcmInfo->mStepMode = atoi(atts[7]);
        pVcmInfo->mVcmDrvMaxCurrent = atoi(atts[9]);
        ALOGD("%s(%d): start current(%d) rated current(%d) vcm max(%d)  step mode(%d)  drv max(%d) \n", 
            __FUNCTION__, __LINE__, 
            pVcmInfo->mStartCurrent,
            pVcmInfo->mRatedCurrent,
            pVcmInfo->mVcmMaxCurrent,
            pVcmInfo->mStepMode,
            pVcmInfo->mVcmDrvMaxCurrent);
    }
	
}

void camera_board_profiles::ParserFlashInfo(const char *name, const char **atts, void *userData)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	rk_flash_info *pFlashInfo = &(pCamInfo->mHardInfo.mFlashInfo);
	rk_flash_config *pFlashConfig = &(pCamInfo->mSoftInfo.mFlashConfig);
	int support = 0;

    if (strcmp(name, "FlashName")==0) {
        ALOGD("%s(%d): FlashName(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        strlcpy(pFlashInfo->mFlashName, atts[1], sizeof(pFlashInfo->mFlashName));        
    } else if (strcmp(name, "FlashI2cBusNum")==0) {
        ALOGD("%s(%d): FlashI2cBusNum(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pFlashInfo->mFlashI2cBusNum = atoi(atts[1]);
    } else if (strcmp(name,"FlashI2cAddrByte")==0){
        ALOGD("%s(%d): FlashI2cAddrByte(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pFlashInfo->mI2cAddrBytes = atoi(atts[1]);
    } else if (strcmp(name,"FlashI2cRate")==0){
        ALOGD("%s(%d): FlashI2cRate(%s)\n", __FUNCTION__, __LINE__, atts[1]);
        pFlashInfo->mFlashI2cRate = atoi(atts[1]);
    } else if (strcmp(name,"FlashTrigger")==0){
        ALOGD("%s(%d): FlashTrigger(%s) active(%s) \n", __FUNCTION__, __LINE__, atts[1], atts[3]);
        strlcpy((char*)pFlashInfo->mFlashTrigger.name, (atts[1]), sizeof(pFlashInfo->mFlashTrigger.name));
        pFlashInfo->mFlashTrigger.active = atoi(atts[3]);
    } else if (strcmp(name,"FlashEn")==0){
        ALOGD("%s(%d): FlashEn(%s) active(%s) \n", __FUNCTION__, __LINE__, atts[1], atts[3]);
        strlcpy((char*)pFlashInfo->mFlashEn.name, (atts[1]), sizeof(pFlashInfo->mFlashEn.name));
        pFlashInfo->mFlashEn.active = atoi(atts[3]);     
    }else if(strcmp(name,"FlashModeType")==0){
    	pFlashInfo->mFlashMode = atoi(atts[1]);
    }else if(strcmp(name,"FlashLuminance")==0){
    	pFlashInfo->mLuminance = atof(atts[1]);
		ALOGD("%s(%d): FlashLuminance(%s)\n", __FUNCTION__, __LINE__, atts[1]);
	}else if(strcmp(name,"FlashColorTemp")==0){
    	pFlashInfo->mColorTemperature = atof(atts[1]);
		ALOGD("%s(%d): FlashColorTemp(%s)\n", __FUNCTION__, __LINE__, atts[1]);
	}else if (strcmp(name, "Flash_Mode_Off")==0) {
        support = atoi(atts[1]);
	    if(support==1)
            pFlashConfig->mFlashSupport |= (0x01<<FLASH_MODE_OFF_BITPOS); 
    } else if (strcmp(name, "Flash_Mode_On")==0) {
        support = atoi(atts[1]);
	    if(support==1)
            pFlashConfig->mFlashSupport |= (0x01<<FLASH_MODE_ON_BITPOS); 
    } else if (strcmp(name,"Flash_Mode_Torch")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pFlashConfig->mFlashSupport |= (0x01<<FLASH_MODE_TORCH_BITPOS); 
    } else if (strcmp(name,"Flash_Mode_Auto")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pFlashConfig->mFlashSupport |= (0x01<<FLASH_MODE_AUTO_BITPOS); 
    } else if (strcmp(name,"Flash_Mode_Red_Eye")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pFlashConfig->mFlashSupport |= (0x01<<FLASH_MODE_RED_EYE_BITPOS); 
    }
	
}

void camera_board_profiles::ParserAwbConfig(const char *name, const char **atts, void *userData)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	rk_white_balance_config *pAWBConfig = &(pCamInfo->mSoftInfo.mAwbConfig);
	int support = 0;

    if (strcmp(name, "AWB_Auto")==0) {
	    support = atoi(atts[1]);
	    if(support==1)
            pAWBConfig->mAwbSupport |= (0x01<<AWB_AUTO_BITPOS ); 
    } else if (strcmp(name, "AWB_Incandescent")==0) {
        support = atoi(atts[1]);
	    if(support==1)
            pAWBConfig->mAwbSupport |= (0x01<<AWB_INCANDESCENT_BITPOS); 
    } else if (strcmp(name,"AWB_Fluorescent")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pAWBConfig->mAwbSupport |= (0x01<<AWB_FLUORESCENT_BITPOS); 
    } else if (strcmp(name,"AWB_Warm_Fluorescent")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pAWBConfig->mAwbSupport |= (0x01<<AWB_WARM_FLUORESCENT_BITPOS); 
    } else if (strcmp(name,"AWB_Daylight")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pAWBConfig->mAwbSupport |= (0x01<<AWB_DAYLIGHT_BITPOS); 
    } else if (strcmp(name,"AWB_Cloudy_Daylight")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pAWBConfig->mAwbSupport |= (0x01<<AWB_CLOUDY_BITPOS); 
    } else if (strcmp(name,"AWB_Twilight")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pAWBConfig->mAwbSupport |= (0x01<<AWB_TWILIGHT_BITPOS); 
    } else if (strcmp(name,"AWB_Shade")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pAWBConfig->mAwbSupport |= (0x01<<AWB_SHADE_BITPOS); 
    } 
	
}

void camera_board_profiles::ParserSenceConfig(const char *name, const char **atts, void *userData)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	rk_sence_config *pSenceConfig = &(pCamInfo->mSoftInfo.mSenceConfig);
	int support = 0;

   if (strcmp(name, "Sence_Mode_Auto")==0) {
	    support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_AUTO_BITPOS); 
    } else if (strcmp(name, "Sence_Mode_Action")==0) {
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_ACTION_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Portrait")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_PORTRAIT_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Landscape")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_LANDSCAPE_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Night")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_NIGHT_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Night_Portrait")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_NIGHT_PORTRAIT_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Theatre")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_THEATRE_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Beach")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_BEACH_BITPOS); 
    } else if (strcmp(name, "Sence_Mode_Snow")==0) {
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_SNOW_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Sunset")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_SUNSET_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Steayphoto")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_STEAYPHOTO_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Pireworks")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_PIREWORKS_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Sports")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_SPORTS_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Party")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport|= (0x01<<SENCE_MODE_PARTY_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Candlelight")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_CANDLELIGHT_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_Barcode")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_BARCODE_BITPOS); 
    } else if (strcmp(name,"Sence_Mode_HDR")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pSenceConfig->mSenceSupport |= (0x01<<SENCE_MODE_HDR_BITPOS); 
    }
	
}

void camera_board_profiles::ParserEffectConfig(const char *name, const char **atts, void *userData)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	rk_effect_config *pEffectConfig = &(pCamInfo->mSoftInfo.mEffectConfig);
	int support=0;

    if (strcmp(name, "Effect_None")==0) {
	    support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_NONE_BITPOS); 
    } else if (strcmp(name, "Effect_Mono")==0) {
        support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_MONO_BITPOS); 
    } else if (strcmp(name,"Effect_Solarize")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_SOLARIZE_BITPOS); 
    } else if (strcmp(name,"Effect_Negative")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_NEGATIVE_BITPOS); 
    } else if (strcmp(name,"Effect_Sepia")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_SEPIA_BITPOS); 
    } else if (strcmp(name,"Effect_Posterize")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_POSTERIZE_BITPOS); 
    } else if (strcmp(name,"Effect_Whiteboard")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_WHITEBOARD_BITPOS); 
    } else if (strcmp(name,"Effect_Blackboard")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_BLACKBOARD_BITPOS); 
    } else if (strcmp(name,"Effect_Aqua")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pEffectConfig->mEffectSupport |= (0x01<<EFFECT_AQUE_BITPOS); 
    } 
	
}

void camera_board_profiles::ParserFocusConfig(const char *name, const char **atts, void *userData)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	rk_focus_config *pFocusConfig = &(pCamInfo->mSoftInfo.mFocusConfig);
	int support=0;

    if (strcmp(name, "Focus_Mode_Auto")==0) {
	    support = atoi(atts[1]);
	    if(support==1)
            pFocusConfig->mFocusSupport |= (0x01<<FOCUS_AUTO_BITPOS); 
    } else if (strcmp(name, "Focus_Mode_Infinity")==0) {
        support = atoi(atts[1]);
	    if(support==1)
            pFocusConfig->mFocusSupport |= (0x01<<FOCUS_INFINITY_BITPOS); 
    } else if (strcmp(name,"Focus_Mode_Marco")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pFocusConfig->mFocusSupport |= (0x01<<FOCUS_MARCO_BITPOS); 
    } else if (strcmp(name,"Focus_Mode_Fixed")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pFocusConfig->mFocusSupport |= (0x01<<FOCUS_FIXED_BITPOS); 
    } else if (strcmp(name,"Focus_Mode_Edof")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pFocusConfig->mFocusSupport |= (0x01<<FOCUS_EDOF_BITPOS); 
    } else if (strcmp(name,"Focus_Mode_Continuous_Video")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pFocusConfig->mFocusSupport |= (0x01<<FOCUS_CONTINUOUS_VIDEO_BITPOS); 
    } else if (strcmp(name,"Focus_Mode_Continuous_Picture")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pFocusConfig->mFocusSupport |= (0x01<<FOCUS_CONTINUOUS_PICTURE_BITPOS); 
    } 
	
}

void camera_board_profiles::ParserAntiBandingConfig(const char *name, const char **atts, void *userData)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	rk_anti_banding_config *pAntiBandingConfig = &(pCamInfo->mSoftInfo.mAntiBandingConfig);
	int support = 0;

    if (strcmp(name, "Anti_Banding_Auto")==0) {
	    support = atoi(atts[1]);
	    if(support==1)
            pAntiBandingConfig->mAntiBandingSupport |= (0x01<<ANTI_BANDING_AUTO_BITPOS); 
    } else if (strcmp(name, "Anti_Banding_50HZ")==0) {
        support = atoi(atts[1]);
	    if(support==1)
            pAntiBandingConfig->mAntiBandingSupport |= (0x01<<ANTI_BANDING_50HZ_BITPOS); 
    } else if (strcmp(name,"Anti_Banding_60HZ")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pAntiBandingConfig->mAntiBandingSupport |= (0x01<<ANTI_BANDING_60HZ_BITPOS); 
    } else if (strcmp(name,"Anti_Banding_Off")==0){
        support = atoi(atts[1]);
	    if(support==1)
            pAntiBandingConfig->mAntiBandingSupport |= (0x01<<ANTI_BANDING_OFF_BITPOS); 
    } 
	
}

void camera_board_profiles::ParserDVConfig(const char *name, const char **atts, void *userData)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	rk_DV_info *pDVResolution = NULL;
    
    if (strcmp(name, "DV_QCIF")==0) {
	    ALOGD("%s(%d):  DV_QCIF(%s) resolution(%sx%s) fps(%s) support(%s)\n", __FUNCTION__, __LINE__, atts[1],atts[3], atts[5],atts[7],atts[9]);
	    pDVResolution = new rk_DV_info();
        if(pDVResolution){
            strlcpy(pDVResolution->mName, atts[1], sizeof(pDVResolution->mName));
    	    pDVResolution->mWidth = atoi(atts[3]);
    	    pDVResolution->mHeight = atoi(atts[5]);
    	    pDVResolution->mFps = atoi(atts[7]);
    	    pDVResolution->mIsSupport =  atoi(atts[9]);
            pDVResolution->mResolution = 0x00000000;
            pCamInfo->mSoftInfo.mDV_vector.add(pDVResolution);
        }
    } else if (strcmp(name, "DV_QVGA")==0) {
        ALOGD("%s(%d):  DV_QVGA(%s) resolution(%sx%s) fps(%s) support(%s)\n", __FUNCTION__, __LINE__, atts[1],atts[3], atts[5],atts[7],atts[9]);
	    pDVResolution = new rk_DV_info();
        if(pDVResolution){
            strlcpy(pDVResolution->mName, atts[1], sizeof(pDVResolution->mName));
    	    pDVResolution->mWidth = atoi(atts[3]);
    	    pDVResolution->mHeight = atoi(atts[5]);
    	    pDVResolution->mFps = atoi(atts[7]);
    	    pDVResolution->mIsSupport =  atoi(atts[9]);
            pDVResolution->mResolution = 0x00000000;
            pCamInfo->mSoftInfo.mDV_vector.add(pDVResolution);
        }
    } else if (strcmp(name,"DV_CIF")==0){
        ALOGD("%s(%d):  DV_CIF(%s) resolution(%sx%s) fps(%s) support(%s)\n", __FUNCTION__, __LINE__, atts[1],atts[3], atts[5],atts[7],atts[9]);
	    pDVResolution = new rk_DV_info();
        if(pDVResolution){
            strlcpy(pDVResolution->mName, atts[1], sizeof(pDVResolution->mName));
    	    pDVResolution->mWidth = atoi(atts[3]);
    	    pDVResolution->mHeight = atoi(atts[5]);
    	    pDVResolution->mFps = atoi(atts[7]);
    	    pDVResolution->mIsSupport =  atoi(atts[9]);
            pDVResolution->mResolution = 0x00000000;
            pCamInfo->mSoftInfo.mDV_vector.add(pDVResolution);
        }
    } else if (strcmp(name,"DV_VGA")==0){
        ALOGD("%s(%d):  DV_VGA(%s) resolution(%sx%s) fps(%s) support(%s)\n", __FUNCTION__, __LINE__, atts[1],atts[3], atts[5],atts[7],atts[9]);
	    pDVResolution = new rk_DV_info();
        if(pDVResolution){
            strlcpy(pDVResolution->mName, atts[1], sizeof(pDVResolution->mName));
    	    pDVResolution->mWidth = atoi(atts[3]);
    	    pDVResolution->mHeight = atoi(atts[5]);
    	    pDVResolution->mFps = atoi(atts[7]);
    	    pDVResolution->mIsSupport =  atoi(atts[9]);
            pDVResolution->mResolution = ISI_RES_VGAP15;
            pCamInfo->mSoftInfo.mDV_vector.add(pDVResolution);
        }
    }  else if (strcmp(name, "DV_480P")==0) {
        ALOGD("%s(%d):  DV_480P(%s) resolution(%sx%s) fps(%s) support(%s)\n", __FUNCTION__, __LINE__, atts[1],atts[3], atts[5],atts[7],atts[9]);
	    pDVResolution = new rk_DV_info();
        if(pDVResolution){
            strlcpy(pDVResolution->mName, atts[1], sizeof(pDVResolution->mName));
    	    pDVResolution->mWidth = atoi(atts[3]);
    	    pDVResolution->mHeight = atoi(atts[5]);
    	    pDVResolution->mFps = atoi(atts[7]);
    	    pDVResolution->mIsSupport =  atoi(atts[9]);
            pDVResolution->mResolution = 0x000000000;
            pCamInfo->mSoftInfo.mDV_vector.add(pDVResolution);
        }
    } else if (strcmp(name, "DV_SVGA")==0) {
        ALOGD("%s(%d):  DV_SVGA(%s) resolution(%sx%s) fps(%s) support(%s)\n", __FUNCTION__, __LINE__, atts[1],atts[3], atts[5],atts[7],atts[9]);
	    pDVResolution = new rk_DV_info();
        if(pDVResolution){
            strlcpy(pDVResolution->mName, atts[1], sizeof(pDVResolution->mName));
    	    pDVResolution->mWidth = atoi(atts[3]);
    	    pDVResolution->mHeight = atoi(atts[5]);
    	    pDVResolution->mFps = atoi(atts[7]);
    	    pDVResolution->mIsSupport =  atoi(atts[9]);
            pDVResolution->mResolution = 0x000000000;
            pCamInfo->mSoftInfo.mDV_vector.add(pDVResolution);
        }
    } else if (strcmp(name,"DV_720P")==0){
        ALOGD("%s(%d):  DV_720P(%s) resolution(%sx%s) fps(%s) support(%s)\n", __FUNCTION__, __LINE__, atts[1],atts[3], atts[5],atts[7],atts[9]);
	    pDVResolution = new rk_DV_info();
        if(pDVResolution){
            strlcpy(pDVResolution->mName, atts[1], sizeof(pDVResolution->mName));
    	    pDVResolution->mWidth = atoi(atts[3]);
    	    pDVResolution->mHeight = atoi(atts[5]);
    	    pDVResolution->mFps = atoi(atts[7]);
    	    pDVResolution->mIsSupport =  atoi(atts[9]);
            pDVResolution->mResolution = (ISI_RES_TV720P5 | ISI_RES_TV720P15 |ISI_RES_TV720P30 |ISI_RES_TV720P60);;
            pCamInfo->mSoftInfo.mDV_vector.add(pDVResolution);
        }
    } else if (strcmp(name,"DV_1080P")==0){
        ALOGD("%s(%d):  DV_1080P(%s) resolution(%sx%s) fps(%s) support(%s)\n", __FUNCTION__, __LINE__, atts[1],atts[3], atts[5],atts[7],atts[9]);
	    pDVResolution = new rk_DV_info();
        if(pDVResolution){
            strlcpy(pDVResolution->mName, atts[1], sizeof(pDVResolution->mName));
    	    pDVResolution->mWidth = atoi(atts[3]);
    	    pDVResolution->mHeight = atoi(atts[5]);
    	    pDVResolution->mFps = atoi(atts[7]);
    	    pDVResolution->mIsSupport =  atoi(atts[9]);
            pDVResolution->mResolution = (ISI_RES_TV1080P5 |ISI_RES_TV1080P12 |ISI_RES_TV1080P15 |ISI_RES_TV1080P20 |ISI_RES_TV1080P24 |ISI_RES_TV1080P25 |ISI_RES_TV1080P30 |ISI_RES_TV1080P50 |ISI_RES_TV1080P60);
            pCamInfo->mSoftInfo.mDV_vector.add(pDVResolution);
        }
    } 
	
}

void camera_board_profiles::StartElementHandler(void *userData, const char *name, const char **atts)
{
	camera_board_profiles *pCamInfoProfiles = (camera_board_profiles *) userData;
	rk_cam_total_info *pCamInfo = pCamInfoProfiles->mCurDevice;
	int support = 0;

	if(strcmp(name,"BoardXmlVersion")==0){
		int highBit = 0;
		int middleBit = 0;
		int lowBit = 0;
		sscanf(atts[1], "v%x.%x.%x", &highBit, &middleBit, &lowBit);
		pCamInfoProfiles->mBoardXmlVersion = ( (highBit&0xff)<<16 ) + ( (middleBit&0xff)<<8 ) + (lowBit&0xff) ;
        ALOGD("\n\n\n Cam_board.xml Version Check: \n");
        ALOGD("    /etc/cam_board.xml : %s\n",atts[1]);
        ALOGD("    CameraHal_board_xml_parser: v%d.%d.%d\n",
            (ConfigBoardXmlVersion&0xff0000)>>16,
            (ConfigBoardXmlVersion&0xff00)>>8,
            ConfigBoardXmlVersion&0xff);
	}
    /* ddl@rock-chips.com: v1.3.0 */
    if(pCamInfoProfiles->mBoardXmlVersion != ConfigBoardXmlVersion) {
		ALOGE("cam_board.xml version(v%d.%d.%d) != xml parser version(v%d.%d.%d)\n",
          (pCamInfoProfiles->mBoardXmlVersion&0xff0000)>>16,
          (pCamInfoProfiles->mBoardXmlVersion&0xff00)>>8,
          pCamInfoProfiles->mBoardXmlVersion&0xff,
          (ConfigBoardXmlVersion&0xff0000)>>16,
          (ConfigBoardXmlVersion&0xff00)>>8,
          ConfigBoardXmlVersion&0xff);
        return;
	}

    if(strcmp(name,"CamDevie")==0){
	    rk_cam_total_info* pNewCamInfo = new rk_cam_total_info();
	    if(pNewCamInfo){	        
            pCamInfoProfiles->mCurDevice= pNewCamInfo;
            pCamInfoProfiles->mDevieVector.add(pNewCamInfo);
            pNewCamInfo->mDeviceIndex = (pCamInfoProfiles->mDevieVector.size()) - 1;
            memset(pNewCamInfo->mHardInfo.mSensorInfo.mLensName,0,CAMSYS_NAME_LEN);
	    }else{
            ALOGE("%s(%d): Warnimg camdevice malloc fail! \n", __FUNCTION__,__LINE__);
	    }
	}else if (strstr(name, "Sensor")) {
        ParserSensorInfo(name, atts, userData);
    } else if (strstr(name, "VCM")) {
        ParserVCMInfo(name, atts, userData);
    } else if (strstr(name,"Flash")){
        ParserFlashInfo(name, atts, userData);
    } else if (strstr(name,"AWB")){
        ParserAwbConfig(name, atts, userData);
    } else if (strstr(name,"Sence")){
        ParserSenceConfig(name, atts, userData);
    } else if (strstr(name,"Effect")){
        ParserEffectConfig(name, atts, userData);
    } else if (strstr(name,"Focus")){
        ParserFocusConfig(name, atts, userData);
    } else if (strstr(name,"Anti_Banding")){
        ParserAntiBandingConfig(name, atts, userData);
    } else if (strstr(name,"HDR")){
        support = atoi(atts[1]);
        pCamInfo->mSoftInfo.mHDRConfig = support;
        ALOGD("%s(%d): HDR(%d)! \n", __FUNCTION__,__LINE__,support);
    } else if (strstr(name,"ZSL")){
        support = atoi(atts[1]);
        pCamInfo->mSoftInfo.mZSLConfig= support;
        ALOGD("%s(%d): ZSL(%d)! \n", __FUNCTION__,__LINE__,support);
    } else if (strstr(name,"DigitalZoom")){
        support = atoi(atts[1]);
        pCamInfo->mSoftInfo.mZoomConfig= support;
        ALOGD("%s(%d): zoom(%d)! \n", __FUNCTION__,__LINE__,support);
    } else if (strstr(name,"Cproc")){
        pCamInfo->mSoftInfo.mCprocConfig.mSupported = (atoi(atts[1]) == 1) ? true:false;
        pCamInfo->mSoftInfo.mCprocConfig.mContrast = atof(atts[3]);
        pCamInfo->mSoftInfo.mCprocConfig.mSaturation = atof(atts[5]);
        pCamInfo->mSoftInfo.mCprocConfig.mHue= atof(atts[7]);
        pCamInfo->mSoftInfo.mCprocConfig.mBrightness = atoi(atts[9]);
        ALOGD("%s(%d): cproc support %d(contrast:%f,saturation:%f,hue:%f,brightness:%d)! \n", 
                __FUNCTION__,__LINE__,atoi(atts[1]),atof(atts[3]),atof(atts[5]),atof(atts[7]),atoi(atts[9]));
    } else if (strstr(name,"Gammaout")){
        pCamInfo->mSoftInfo.mGammaOutConfig.mSupported = (atoi(atts[1]) == 1) ? true:false;
        pCamInfo->mSoftInfo.mGammaOutConfig.mGamma= atof(atts[3]);
        pCamInfo->mSoftInfo.mGammaOutConfig.mOffSet= atoi(atts[5]);
        ALOGD("%s(%d): Gammaout support %d(mGamma:%f,mOffSet:%d)! \n", __FUNCTION__,__LINE__,atoi(atts[1]),atof(atts[3]),atoi(atts[5]));
    } else if (strstr(name,"FaceDetect")){
        support = atoi(atts[1]);
        pCamInfo->mSoftInfo.mFaceDetctConfig.mFaceDetectSupport = support;
        pCamInfo->mSoftInfo.mFaceDetctConfig.mFaceMaxNum = atoi(atts[3]);
        ALOGD("%s(%d): face detect config(%d),max face num is (%d)! \n", __FUNCTION__,__LINE__,support,atoi(atts[3]));
    } else if (strstr(name,"PreviewSize")){
        pCamInfo->mSoftInfo.mPreviewWidth = atoi(atts[1]);
        pCamInfo->mSoftInfo.mPreviewHeight = atoi(atts[3]);
        ALOGD("%s(%d): PreviewSize(%dx%d)! \n", __FUNCTION__,__LINE__,pCamInfo->mSoftInfo.mPreviewWidth,pCamInfo->mSoftInfo.mPreviewHeight);
    } else if (strstr(name,"DV")){
        ParserDVConfig(name, atts, userData);
    } else if (strstr(name,"Continue_SnapShot")){
        support = atoi(atts[1]);
        pCamInfo->mSoftInfo.mContinue_snapshot_config = support;
        ALOGD("%s(%d): Continue_SnapShot(%d)! \n", __FUNCTION__,__LINE__,support);
    }else if(strstr(name,"InterpolationRes")){
    	pCamInfo->mSoftInfo.mInterpolationRes = atoi(atts[1]);
		ALOGD("%s(%d): InterpolationRes(%d)! \n", __FUNCTION__,__LINE__,pCamInfo->mSoftInfo.mInterpolationRes);
    }  

    return;
}

camera_board_profiles* camera_board_profiles::createInstance()
{
	FILE *fp = NULL;

    camera_board_profiles *profiles = new camera_board_profiles();
    
    fp = fopen(RK_BOARD_XML_PATH, "r");
    if(!fp){
  	    LOGD("This machine have not dvp/mipi camera!!\n");
        return profiles;
    }
    
    LOGD("open xml file(%s) success\n", RK_BOARD_XML_PATH);
    
    XML_Parser parser = XML_ParserCreate(NULL);
    if(parser==NULL){
        ALOGE("XML_ParserCreate failed\n");
        return NULL;
    }
    
    XML_SetUserData(parser, profiles);
    XML_SetElementHandler(parser, StartElementHandler, NULL);

    const int BUFF_SIZE = 512;
    for (;;) {
        void *buff = ::XML_GetBuffer(parser, BUFF_SIZE);
        if (buff == NULL) {
            ALOGE("failed to in call to XML_GetBuffer()");
            goto exit;
        }

        int bytes_read = ::fread(buff, 1, BUFF_SIZE, fp);
        if (bytes_read < 0) {
            ALOGE("failed in call to read");
            goto exit;
        }

        int res = XML_ParseBuffer(parser, bytes_read, bytes_read == 0);
        if(res!=1){
            ALOGE("XML_ParseBuffer error or susppend (%d)\n", res);
        }

        if (bytes_read == 0) break;  // done parsing the xml file
    }

exit:
    XML_ParserFree(parser);
    fclose(fp);

    size_t nCamDev2 = profiles->mDevieVector.size();
    ALOGD("number of camdevice (%d)\n", nCamDev2);

    if (nCamDev2>0) {
        size_t nDVnum2 = profiles->mCurDevice->mSoftInfo.mDV_vector.size();
        ALOGD("now DV size(%d)\n", nDVnum2);
    }
    return profiles;

}

camera_board_profiles* camera_board_profiles::getInstance()
{
    camera_board_profiles *profiles = createInstance();

    return profiles;
}

bool camera_board_profiles::LoadALLCalibrationData(camera_board_profiles* profiles)
{
    size_t nCamDev2 = profiles->mDevieVector.size();
    unsigned int i=0;
    char filename[50];
    
    
    if(nCamDev2>=1){
        for(i=0; i<nCamDev2; i++)
        {
            rk_sensor_info *pSensorInfo = &(profiles->mDevieVector[i]->mHardInfo.mSensorInfo);
            
            CalibDb *pcalidb = &(profiles->mDevieVector[i]->mLoadSensorInfo.calidb);
            memset(filename, 0x00, 50);
            if(strlen(pSensorInfo->mLensName) == 0)
                sprintf(filename, "%s.xml", pSensorInfo->mSensorName);
            else
                sprintf(filename, "%s_lens_%s.xml", pSensorInfo->mSensorName,pSensorInfo->mLensName);
            bool res = pcalidb->CreateCalibDb(filename);
            if(res){
                ALOGD("load %s success\n", filename);
            }else{
                ALOGE("load %s failed\n", filename);
            }
        }
    }

    return true;
}

void camera_board_profiles::OpenAndRegistALLSensor(camera_board_profiles* profiles)
{
    size_t nCamDev2 = profiles->mDevieVector.size();
    unsigned int i=0;
    char filename[50];
    int err;    

    LOG_FUNCTION_NAME
    if(nCamDev2>=1){
        for(i=0; i<nCamDev2; i++)
        {         
            OpenAndRegistOneSensor(profiles->mDevieVector[i]);
        }
    }
    LOG_FUNCTION_NAME_EXIT
}


int camera_board_profiles::OpenAndRegistOneSensor(rk_cam_total_info *pCamInfo)
{
    rk_sensor_info *pSensorInfo = &(pCamInfo->mHardInfo.mSensorInfo);
    camsys_load_sensor_info* pLoadSensorInfo = &(pCamInfo->mLoadSensorInfo);

    if(!pCamInfo)
        return RK_RET_NULL_POINTER;

    pCamInfo->mIsConnect = 0;
    
    sprintf(pLoadSensorInfo->mSensorLibName, "%s%s.so", RK_SENSOR_LIB_PATH, pSensorInfo->mSensorName);    
    
    void *hSensorLib = dlopen( pLoadSensorInfo->mSensorLibName, RTLD_NOW/*RTLD_LAZY*/ );
    if ( NULL == hSensorLib )
    {
        ALOGE( "%s can't open the specified driver(%s)\n", __FUNCTION__, pLoadSensorInfo->mSensorLibName);
        ALOGE("dlopen err:%s.\n",dlerror()); 
        pLoadSensorInfo->mhSensorLib = NULL;
        pLoadSensorInfo->pCamDrvConfig = NULL;
        return RK_RET_NULL_POINTER;
    }

    IsiCamDrvConfig_t *pIsiCamDrvConfig = (IsiCamDrvConfig_t *)dlsym( hSensorLib, "IsiCamDrvConfig" );
    if ( NULL == pIsiCamDrvConfig )
    {
        ALOGE("%s (can't load sensor driver)\n", __FUNCTION__ );
        ALOGE("dlsym err:%s.\n",dlerror()); 
        if(hSensorLib)
            dlclose( hSensorLib );
        pLoadSensorInfo->mhSensorLib = NULL;
        pLoadSensorInfo->pCamDrvConfig = NULL;
        return RK_RET_NULL_POINTER;
    }

    // initialize function pointer
    if(pIsiCamDrvConfig->pfIsiGetSensorIss){
        if ( RET_SUCCESS != pIsiCamDrvConfig->pfIsiGetSensorIss( &(pIsiCamDrvConfig->IsiSensor) ) )
        {
            ALOGE("%s (IsiGetSensorIss failed)\n", __FUNCTION__ );
            return RK_RET_FUNC_FAILED;              
        }
    }else{
        ALOGE("%s ERROR(driver(%s) don't support IsiGetSensorIss)\n", __FUNCTION__,  pSensorInfo->mSensorName);
        return RK_RET_NULL_POINTER;   
    }

    pLoadSensorInfo->mhSensorLib = hSensorLib;
    pLoadSensorInfo->pCamDrvConfig = pIsiCamDrvConfig;
    
    if(pIsiCamDrvConfig->pfIsiGetSensorI2cInfo){
        sensor_i2c_info_t* pI2cInfo;
        if(RET_SUCCESS != pIsiCamDrvConfig->pfIsiGetSensorI2cInfo(&pI2cInfo)){
        	ALOGE("GET I2C INFO ERRO !!!!!!!!!!!!!!!!");
            return RK_RET_FUNC_FAILED;
        }
        
        pCamInfo->mLoadSensorInfo.mpI2cInfo = pI2cInfo;
        //register i2c device 
        int err = RegisterSensorDevice(pCamInfo);
        if(err==RK_RET_SUCCESS)
        {
        	if(pIsiCamDrvConfig->IsiSensor.pIsiSensorCaps->SensorOutputMode == ISI_SENSOR_OUTPUT_MODE_RAW){
	            CalibDb *pcalidb = &(pCamInfo->mLoadSensorInfo.calidb);
                if(strlen(pSensorInfo->mLensName) == 0)
	                sprintf(pLoadSensorInfo->mSensorXmlFile, "%s%s", RK_SENSOR_XML_PATH, pSensorInfo->mSensorName);
                else
	                sprintf(pLoadSensorInfo->mSensorXmlFile, "%s%s_lens_%s", RK_SENSOR_XML_PATH, pSensorInfo->mSensorName,pSensorInfo->mLensName);
                if(pCamInfo->mHardInfo.mIsOTP == true)
                    strcat(pLoadSensorInfo->mSensorXmlFile,"_OTP.xml");
                else
                    strcat(pLoadSensorInfo->mSensorXmlFile,".xml");
                    
                LOGD("sensor xml file name : %s lens name %s",pLoadSensorInfo->mSensorXmlFile,pSensorInfo->mLensName);
	            bool res = pcalidb->CreateCalibDb(pLoadSensorInfo->mSensorXmlFile);	           	
			    if(res){	                
	                pCamInfo->mIsConnect = 1;
	                return RK_RET_SUCCESS;
	            }else{
	                ALOGE("load %s failed\n", pLoadSensorInfo->mSensorXmlFile);
	                return RK_RET_FUNC_FAILED;
	            }
    		}else{
                pCamInfo->mIsConnect = 1;
                return RK_RET_SUCCESS;
			}
        }else{
            ALOGE("%s device register failed!",pSensorInfo->mSensorName);
            if(hSensorLib)
                dlclose( hSensorLib );
			return RK_RET_NOSETUP;
    	}
    }else{
        ALOGE("sensor(%s)'s driver don't have func pfIsiGetSensorI2cInfo\n", pSensorInfo->mSensorName);
        return RK_RET_NULL_POINTER;
    }
	
	return 0;
}

static int sensor_write_i2c(
    void* context,
    int camsys_fd,    
    const uint32_t      reg_address,
    const uint32_t      value,
    int* i2c_base_info
)
{
    int err = RK_RET_SUCCESS; 
    camsys_i2c_info_t i2cinfo;
    rk_cam_total_info* pCamInfo = (rk_cam_total_info*)context;
    rk_sensor_info *pSensorInfo = &(pCamInfo->mHardInfo.mSensorInfo);
    camsys_load_sensor_info *pLoadInfo = &(pCamInfo->mLoadSensorInfo);

    if(i2c_base_info != NULL && i2c_base_info[0] != 0){
        i2cinfo.bus_num = pSensorInfo->mSensorI2cBusNum;
        i2cinfo.slave_addr = i2c_base_info[0];
        i2cinfo.reg_addr = reg_address; 
        i2cinfo.reg_size = i2c_base_info[1];
        i2cinfo.val = value;
        i2cinfo.val_size = i2c_base_info[2];
        i2cinfo.i2cbuf_directly = 0;
        i2cinfo.speed = pSensorInfo->mSensorI2cRate;
    }else{
        i2cinfo.bus_num = pSensorInfo->mSensorI2cBusNum;
        i2cinfo.slave_addr = pLoadInfo->mpI2cInfo->i2c_addr;
        i2cinfo.reg_addr = reg_address; 
        i2cinfo.reg_size = pLoadInfo->mpI2cInfo->reg_size;
        i2cinfo.val = value;
        i2cinfo.val_size = pLoadInfo->mpI2cInfo->value_size;
        i2cinfo.i2cbuf_directly = 0;
        i2cinfo.speed = pSensorInfo->mSensorI2cRate;

    }
    
	err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
    if (err<0) {
        ALOGE("%s failed\n",__FUNCTION__);
        err = RK_RET_DEVICEERR;
    }
    return err;
}

static int sensor_read_i2c(
    void* context,
    int camsys_fd,    
    const uint32_t      reg_address,
    int* i2c_base_info
)
{
    int err = RK_RET_SUCCESS; 
    camsys_i2c_info_t i2cinfo;
    rk_cam_total_info* pCamInfo = (rk_cam_total_info*)context;
    rk_sensor_info *pSensorInfo = &(pCamInfo->mHardInfo.mSensorInfo);
    camsys_load_sensor_info *pLoadInfo = &(pCamInfo->mLoadSensorInfo);

    if(i2c_base_info != NULL && i2c_base_info[0] != 0){
        i2cinfo.bus_num = pSensorInfo->mSensorI2cBusNum;
        i2cinfo.slave_addr = i2c_base_info[0];
        i2cinfo.reg_addr = reg_address; 
        i2cinfo.reg_size = i2c_base_info[1];
        i2cinfo.val = 0;
        i2cinfo.val_size = i2c_base_info[2];
        i2cinfo.i2cbuf_directly = 0;
        i2cinfo.speed = pSensorInfo->mSensorI2cRate;
    }else{
        i2cinfo.bus_num = pSensorInfo->mSensorI2cBusNum;
        i2cinfo.slave_addr = pLoadInfo->mpI2cInfo->i2c_addr;
        i2cinfo.reg_addr = reg_address; 
        i2cinfo.reg_size = pLoadInfo->mpI2cInfo->reg_size;
        i2cinfo.val = 0;
        i2cinfo.val_size = pLoadInfo->mpI2cInfo->value_size;
        i2cinfo.i2cbuf_directly = 0;
        i2cinfo.speed = pSensorInfo->mSensorI2cRate;
    }
    
	err = ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo);
    if (err<0) {
        ALOGE("%s failed\n",__FUNCTION__);
        err = RK_RET_DEVICEERR;
    }else
        err = i2cinfo.val;
    return err;
}

int camera_board_profiles::RegisterSensorDevice(rk_cam_total_info* pCamInfo)
{
    int err = RK_RET_SUCCESS,i; 
    camsys_sysctrl_t sysctl;
    camsys_devio_name_t extdev;
    camsys_i2c_info_t i2cinfo;
    camsys_querymem_t qmem1, qmem2;
 //   unsigned int *regbase=MAP_FAILED, *i2cbase=MAP_FAILED;
    unsigned int i2cbytes;
    struct rk_sensor_reg *sensor_reg;
    unsigned char *i2cchar;
    int camsys_fd=-1;
    int regist_ret=-1;
	int ret = RK_RET_SUCCESS;

    //for test 
    //return RK_RET_SUCCESS;
    
    rk_sensor_info *pSensorInfo = &(pCamInfo->mHardInfo.mSensorInfo);
    rk_vcm_info *pVcmInfo = &(pCamInfo->mHardInfo.mVcmInfo);
    camsys_load_sensor_info *pLoadInfo = &(pCamInfo->mLoadSensorInfo);
    rk_flash_info *pFlashInfo = &(pCamInfo->mHardInfo.mFlashInfo);
    sensor_i2c_info_t *pI2cInfo = pLoadInfo->mpI2cInfo;
    
    camsys_fd = open(pSensorInfo->mCamsysDevPath, O_RDWR);
    if (camsys_fd < 0) {
        ALOGE("Open (%s) failed, error=(%s)\n", pSensorInfo->mCamsysDevPath,strerror(errno));
        err = RK_RET_NOFILE;
		ret = RK_RET_NOFILE;
        goto end;
    }    

    memset(&extdev,0x00, sizeof(camsys_devio_name_t));
    pCamInfo->mLoadSensorInfo.mCamsysFd = camsys_fd;

    strlcpy((char*)extdev.dev_name,  (char*)pSensorInfo->mSensorName, sizeof(extdev.dev_name));
    extdev.dev_id = pSensorInfo->mCamDevid;
    strlcpy((char*)extdev.avdd.name, (char*)pSensorInfo->mAvdd.name,sizeof(extdev.avdd.name));
    //strlcpy((char*)extdev.avdd.name, pSensorInfo->mAvdd.name,2);
	extdev.avdd.min_uv = pSensorInfo->mAvdd.min_uv;
    extdev.avdd.max_uv = pSensorInfo->mAvdd.max_uv;
    strlcpy((char*)extdev.dovdd.name,(char*)pSensorInfo->mDovdd.name,sizeof(extdev.dovdd.name));
    extdev.dovdd.min_uv = pSensorInfo->mDovdd.min_uv;
    extdev.dovdd.max_uv = pSensorInfo->mDovdd.max_uv;
    strlcpy((char*)extdev.dvdd.name, (char*)pSensorInfo->mDvdd.name,sizeof(extdev.dvdd.name));
    extdev.dvdd.min_uv = pSensorInfo->mDvdd.min_uv;
    extdev.dvdd.max_uv = pSensorInfo->mDvdd.max_uv;
    strlcpy((char*)extdev.afvdd.name, (char*)pVcmInfo->mVcmVdd.name,sizeof(extdev.afvdd.name));
    extdev.afvdd.min_uv = pVcmInfo->mVcmVdd.min_uv;
    extdev.afvdd.max_uv = pVcmInfo->mVcmVdd.max_uv;
    
    strlcpy((char*)extdev.pwrdn.name, (char*)pSensorInfo->mSensorGpioPwdn.name,sizeof(extdev.pwrdn.name));
    extdev.pwrdn.active = pSensorInfo->mSensorGpioPwdn.active;
    strlcpy((char*)extdev.rst.name, (char*)pSensorInfo->mSensorGpioReset.name,sizeof(extdev.rst.name));
    extdev.rst.active = pSensorInfo->mSensorGpioReset.active;
    
    strlcpy((char*)extdev.pwren.name, (char*)pSensorInfo->SensorGpioPwen.name,sizeof(extdev.pwren.name));
    extdev.pwren.active = pSensorInfo->SensorGpioPwen.active;
    
    strlcpy((char*)extdev.afpwrdn.name, (char*)pVcmInfo->mVcmGpioPwdn.name,sizeof(extdev.afpwrdn.name));
    extdev.afpwrdn.active = pVcmInfo->mVcmGpioPwdn.active;
    strlcpy((char*)extdev.afpwr.name, (char*)pVcmInfo->mVcmGpioPower.name,sizeof(extdev.afpwr.name));
    extdev.afpwr.active = pVcmInfo->mVcmGpioPower.active;
    
    if (strcmp("Internal",pFlashInfo->mFlashName) == 0) {
        extdev.dev_cfg |= CAMSYS_DEVCFG_FLASHLIGHT;
		extdev.fl.fl.active = pFlashInfo->mFlashTrigger.active;
		if(pFlashInfo->mFlashMode == 2)
		{
			if (pFlashInfo->mFlashTrigger.active != pFlashInfo->mFlashEn.active) {
			    LOGE("%s:\n"
			        "WARNING: flashen active value is not equal to flashtrigger active value!\n\n\n",  __PRETTY_FUNCTION__);
			}
		    extdev.dev_cfg |= CAMSYS_DEVCFG_PREFLASHLIGHT;
		}
    }else{
       strcpy((char*)extdev.fl.fl_drv_name,pFlashInfo->mFlashName);
       strlcpy((char*)extdev.fl.fl.name, (char*)pFlashInfo->mFlashTrigger.name,sizeof(extdev.fl.fl.name));
	   extdev.fl.fl.active = pFlashInfo->mFlashTrigger.active;
       strlcpy((char*)extdev.fl.fl_en.name, (char*)pFlashInfo->mFlashEn.name,sizeof(extdev.fl.fl_en.name));
	   extdev.fl.fl_en.active = pFlashInfo->mFlashEn.active;
        
    }

    if(pSensorInfo->mPhy.type == CamSys_Phy_Cif){
        extdev.phy.type = CamSys_Phy_Cif;
        extdev.phy.info.cif.fmt = pSensorInfo->mPhy.info.cif.fmt;
        extdev.phy.info.cif.cif_num = pSensorInfo->mPhy.info.cif.cif_num;
    }else if(pSensorInfo->mPhy.type == CamSys_Phy_Mipi){
        extdev.phy.type = CamSys_Phy_Mipi;
        extdev.phy.info.mipi.data_en_bit = pSensorInfo->mPhy.info.mipi.data_en_bit;
        extdev.phy.info.mipi.phy_index = pSensorInfo->mPhy.info.mipi.phy_index;

    }else{
        ALOGE("%s %d: unknow phy type(%d)\n",__func__,__LINE__, pSensorInfo->mPhy.type);
    }
    
    extdev.clk.in_rate = pSensorInfo->mMclkRate;

	//oyyf before register sensor driver, check the kernel camsys version
	ALOGD("\n\n\n CamSys_Head.h Version Check:\n");
	err = ioctl(camsys_fd, CAMSYS_VERCHK, &(pCamInfo->mCamsysVersion));
	if(!err){
        ALOGD("    Kernel camsys_head.h: v%d.%d.%d\n",
            (pCamInfo->mCamsysVersion.head_ver&0xff0000)>>16,
            (pCamInfo->mCamsysVersion.head_ver&0xff00)>>8,
            (pCamInfo->mCamsysVersion.head_ver&0xff));
        ALOGD("    Kernel camsys_drv :   v%d.%d.%d\n",
            (pCamInfo->mCamsysVersion.drv_ver&0xff0000)>>16,
            (pCamInfo->mCamsysVersion.drv_ver&0xff00)>>8,
            (pCamInfo->mCamsysVersion.drv_ver&0xff));

        ALOGD("    CameraHal camsys_head.h : v%d.%d.%d\n",
            (CAMSYS_HEAD_VERSION&0xff0000)>>16,
            (CAMSYS_HEAD_VERSION&0xff00)>>8,
            (CAMSYS_HEAD_VERSION&0xff));
        ALOGD("\n\n\n");

        //just warnning
        if (CAMSYS_HEAD_VERSION != pCamInfo->mCamsysVersion.head_ver)
            ALOGE("%s:\n VERSION-WARNING: camsys_head.h version isn't match in Kernel and CameraHal\n\n\n", __FUNCTION__);
        
        
	}else{
		ALOGE("%s(%d): get camsys head version failed! ---------\n\n\n",__FUNCTION__,__LINE__);
		goto regist_err;
	}
	
    regist_ret = ioctl(camsys_fd, CAMSYS_REGISTER_DEVIO, &extdev);
    if (regist_ret<0) {        
        ALOGE("CAMSYS_REGISTER_DEVIO failed\n");
        ret = RK_RET_DEVICEERR;
        goto regist_err;
    }

    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_Avdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_Avdd on failed!\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }

    sysctl.ops = CamSys_Dvdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_Dvdd on failed!\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }
    
    sysctl.ops = CamSys_Dovdd;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_Dovdd on failed!\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }
    usleep(5000);
    sysctl.dev_mask = (pSensorInfo->mHostDevid|pSensorInfo->mCamDevid); //need modify
    sysctl.ops = CamSys_ClkIn;
    sysctl.on = 1;

    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_ClkIn on failed\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }

    //1)power en
    usleep(1000);
    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_PwrEn;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_PwrDn on failed\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }

    //2)reset 
    usleep(1000);
    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_Rst;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_PwrDn on failed\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }
    //3)power down control
    usleep(1000);
    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_PwrDn;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_PwrDn on failed\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }
    usleep(2000);
    
    i2cinfo.bus_num = pSensorInfo->mSensorI2cBusNum;
    i2cinfo.slave_addr = pLoadInfo->mpI2cInfo->i2c_addr;
    i2cinfo.reg_addr = pLoadInfo->mpI2cInfo->soft_reg_addr; 
    i2cinfo.reg_size = pLoadInfo->mpI2cInfo->reg_size;
    i2cinfo.val = pLoadInfo->mpI2cInfo->soft_reg_value;
    i2cinfo.val_size = pLoadInfo->mpI2cInfo->value_size;
    i2cinfo.i2cbuf_directly = 0;
    i2cinfo.speed = pSensorInfo->mSensorI2cRate;
	err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
    if(err<0) {
        if (pLoadInfo->mpI2cInfo->i2c_addr2 != 0) {
            i2cinfo.slave_addr = pLoadInfo->mpI2cInfo->i2c_addr2;
            err = ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo);
            if (err>=0) {
                pLoadInfo->mpI2cInfo->i2c_addr = pLoadInfo->mpI2cInfo->i2c_addr2;
            }
        }

        if (err<0) {            
            ALOGE("WARNING: %s soft reset by i2c failed!, please check follow information:\n",pSensorInfo->mSensorName);
            ALOGE("    Slave_addr: 0x%x 0x%x\n"
                  "    Soft reset reg: 0x%x  val: 0x%x\n"
                  "    Power/PowerDown/Reset/Mclk/I2cBus\n",
                  pLoadInfo->mpI2cInfo->i2c_addr,pLoadInfo->mpI2cInfo->i2c_addr2,
                  i2cinfo.reg_addr, i2cinfo.val); 
    		ret = RK_RET_DEVICEERR;
            goto power_off;
        }
    }
    
    //query iommu is enabled ?
    {
        int iommu_enabled = 0;
    	err = ioctl(camsys_fd, CAMSYS_QUREYIOMMU, &iommu_enabled);
        if(err<0) {
            ALOGE("CAMSYS_QUREYIOMMU failed !!!!"); 
        }else{
            pCamInfo->mIsIommuEnabled = (iommu_enabled == 1) ? true:false;
        }
    }

    if(!ListEmpty(&(pI2cInfo->chipid_info))){
        List* l = ListHead( &(pI2cInfo->chipid_info) );
        while ( l )
        {
            sensor_chipid_info_t* pChipIDInfo = (sensor_chipid_info_t *)l;
            i2cinfo.reg_addr = pChipIDInfo->chipid_reg_addr; 

            err = ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo);
            if (err<0) {
                ALOGE("%s CAMSYS_I2CRD failed\n",pSensorInfo->mSensorName);
            } else {
                ALOGD("Check %s ID: reg: 0x%x  val: 0x%x default: 0x%x \n",
                    pSensorInfo->mSensorName,
                    i2cinfo.reg_addr, i2cinfo.val, pChipIDInfo->chipid_reg_value);
                if(i2cinfo.val!=pChipIDInfo->chipid_reg_value){
                    ret = RK_RET_DEVICEERR;
                    goto power_off;
                }
            }
            
            l = l->p_next;
        }
    }else{
        ALOGE("ERROR: sensor dirver don't have chip id info\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }

    //check otp
    {
        camsys_load_sensor_info* pLoadSensorInfo = &(pCamInfo->mLoadSensorInfo);

        if(pLoadInfo->pCamDrvConfig->IsiSensor.pIsiCheckOTPInfo){
            ALOGD("%s:check and read otp info!!!!",__FUNCTION__); 
            int tmp = pLoadInfo->pCamDrvConfig->IsiSensor.pIsiCheckOTPInfo(sensor_write_i2c,sensor_read_i2c,pCamInfo,camsys_fd);
            if(tmp == RET_SUCCESS){
                pCamInfo->mHardInfo.mIsOTP = true;
            }
        }
    }

//  power off
power_off:
    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_PwrDn;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_PwrDn off failed\n");
        ret = RK_RET_DEVICEERR;
        
    }

    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_Rst;
    sysctl.on = 1;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_Rst off failed\n");
        ret = RK_RET_DEVICEERR;
        
    }

    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_PwrEn;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_PwrEn off failed\n");
        ret = RK_RET_DEVICEERR;
       
    }
    
    usleep(1000);
    sysctl.dev_mask = (pSensorInfo->mHostDevid|pSensorInfo->mCamDevid);
    sysctl.ops = CamSys_ClkIn;
    sysctl.on = 0;

    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_ClkIn off failed\n");
        ret = RK_RET_DEVICEERR;
        
    }

    usleep(2000);

    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_Dovdd;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_Dovdd off failed!\n");
        ret = RK_RET_DEVICEERR;
       
    }

    sysctl.ops = CamSys_Dvdd;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_Dvdd on failed!\n");
        ret = RK_RET_DEVICEERR;
        goto power_off;
    }
    
    sysctl.dev_mask = pSensorInfo->mCamDevid;
    sysctl.ops = CamSys_Avdd;
    sysctl.on = 0;
    err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
    if (err<0) {
        ALOGE("CamSys_Avdd off failed!\n");
        ret = RK_RET_DEVICEERR;
        
    }


unmap_pos:
    
regist_err: 
    if(regist_ret==0 && ret<0){
        // unregister device  need modify
        err = ioctl(camsys_fd, CAMSYS_DEREGISTER_DEVIO, &sysctl);
        if(err<0){
            ALOGE("CAMSYS_DEREGISTER_DEVIO failed!\n");
            ret = RK_RET_DEVICEERR;
        }   
    }
    
    if(camsys_fd){
        close(camsys_fd);    
        camsys_fd = 0;
    }
   
end:
	return ret;   
}

int camera_board_profiles::CheckSensorSupportDV(rk_cam_total_info* pCamInfo)
{
    size_t nDvVector = pCamInfo->mSoftInfo.mDV_vector.size();
    uint32_t lanes;

    if (pCamInfo->mHardInfo.mSensorInfo.mPhy.type == CamSys_Phy_Mipi) {
        lanes = pCamInfo->mHardInfo.mSensorInfo.laneNum;
        if (lanes == 4)
            lanes = 2;
        else if (lanes == 2)
            lanes = 1;
        else 
            lanes = 0;
    } else { 
        lanes = 0;
    }
    if((int)nDvVector>=1){
        for(int i=0; i<(int)nDvVector; i++){
            rk_DV_info *pDVInfo = pCamInfo->mSoftInfo.mDV_vector[i];
			char *p = pCamInfo->mHardInfo.mSensorInfo.mSensorName;
			if(strcmp(pCamInfo->mHardInfo.mSensorInfo.mSensorName, UVC_CAM_NAME)==0){
				
					if(pDVInfo->mIsSupport)
	                    pDVInfo->mAddMask = 0;
	                else
	                    pDVInfo->mAddMask = 1;
					ALOGD("(%s) UVC camera resolution(%dx%d) is support \n", pCamInfo->mHardInfo.mSensorInfo.mSensorName, pDVInfo->mWidth, pDVInfo->mHeight);
			}
			else if(strcmp(pCamInfo->mHardInfo.mSensorInfo.mSensorName, SOC_CAM_NAME)==0){
				if(pDVInfo->mIsSupport)
					pDVInfo->mAddMask = 0;
				else{
					pDVInfo->mAddMask = 1;
					ALOGD("(%s) SOC camera resolution(%dx%d) is support \n", pCamInfo->mHardInfo.mSensorInfo.mSensorName, pDVInfo->mWidth, pDVInfo->mHeight);
				}
			}
			else{ 
			    pDVInfo->mAddMask = 1;
                if(pDVInfo->mIsSupport) {
                    List *l,*head;
                    sensor_caps_t *pCaps;                        
                    
                    head = &pCamInfo->mLoadSensorInfo.mpI2cInfo->lane_res[lanes];
                    if (!ListEmpty(head)) {
                        l = ListHead( head );
                        while ( l )
                        {
                            pCaps = (sensor_caps_t*)l;                            
                            if (ISI_RES_W_GET(pCaps->caps.Resolution)*ISI_RES_H_GET(pCaps->caps.Resolution)*10 >=
                                pDVInfo->mHeight*pDVInfo->mWidth*9) {
                                if ((unsigned int)(ISI_FPS_GET(pCaps->caps.Resolution)) >= pDVInfo->mFps) {
                                    pDVInfo->mFps = ISI_FPS_GET(pCaps->caps.Resolution);
                                    pDVInfo->mAddMask = 0;
                                }
                            }
                            l = l->p_next;
                        }
                    }
                }  
			}
           
        }
    }else{
        ALOGD("WARNING: sensor(%s) don't support any DV resolution\n", pCamInfo->mHardInfo.mSensorInfo.mSensorName);
    }

    return 0;
}

int camera_board_profiles::WriteDevNameTOXML(camera_board_profiles* profiles, char *SrcFile, char* DstFile)
{   
    FILE *fpsrc, *fpdst; 
	char one_line_buf[256]; 
	char *leave_line0, *leave_line1, *leave_line2;
	int isWrite=0;
	int i;

    size_t nCamNum = profiles->mDevideConnectVector.size();
    
	fpsrc = fopen(SrcFile,"r"); 
	if(fpsrc == NULL) 
	{ 
		ALOGE("%s OPEN SrcMediaProfiles '%s' FALID, mode(read only), error(%s)\n", __FUNCTION__, SrcFile, strerror(errno)); 
		return -1; 
	} 
	
	fpdst = fopen(DstFile,"w"); 
	if(fpdst == NULL) 
	{ 
		ALOGE("%s OPEN DstMediaProfiles %s TEMP FALID, mode(w), error(%s)\n",__FUNCTION__, DstFile, strerror(errno)); 
		return -2; 
	} 

	fseek(fpsrc,0,SEEK_SET); 
	fseek(fpdst,0,SEEK_SET);	
	while(fgets(one_line_buf,256,fpsrc) != NULL) 
	{ 
		
		fputs(one_line_buf, fpdst);

		if(isWrite==0){
			leave_line0 = NULL;
			leave_line0 = strstr(one_line_buf, "<?");
			if(leave_line0==NULL){
				continue;
			}

			leave_line0 = NULL;
			leave_line0 = strstr(one_line_buf, "?>");
			if(leave_line0==NULL){
				continue;
			}

			for(i=0; (i<(int)nCamNum && i<CAM_MAX_SUPPORT); i++){
				fprintf(fpdst, "<!--  videoname%d=\"%s\" index=%d facing=%d -->  \n", 
                    i, profiles->mDevideConnectVector[i]->mHardInfo.mSensorInfo.mSensorName,
                    profiles->mDevideConnectVector[i]->mDeviceIndex, profiles->mDevideConnectVector[i]->mHardInfo.mSensorInfo.mFacing);
			}
			isWrite=1;	
		}
		
		if(fgetc(fpsrc)==EOF) 
		{ 
			break; 
		} 
		fseek(fpsrc,-1,SEEK_CUR); 		 
	}

	memset(one_line_buf,0,sizeof(one_line_buf));
	fclose(fpsrc);                 
	fclose(fpdst);  

	return 0;
}

int camera_board_profiles::ReadDevNameFromXML(FILE* fp, xml_DEV_name_s* video_name)
{
    char one_line_buf[256];
	char *leave_line0, *leave_line1, *leave_line2;
	int leave_num;
	const char* equal_sign = "=";
	const char* mark_sign_start = "<!--";
	const char* mark_sign_end = "-->";
	const char* videoname_sign = "videoname";
	xml_DEV_name_s* pst_video_name = video_name;
	int count = 0;
	
	fseek(fp,0,SEEK_SET);
	
	while(fgets(one_line_buf,256,fp) != NULL) 
	{ 
		if(strlen(one_line_buf) < 3) //line is NULL
		{ 
			continue; 
		} 
		leave_line0 = NULL;
		leave_line0 = strstr(one_line_buf, mark_sign_start);
		if(leave_line0==NULL)
		{
			continue;
		}
		leave_line1 = NULL;
		leave_line1 = strstr(one_line_buf, mark_sign_end);
		if(leave_line1==NULL)
		{
			continue;
		}

		leave_line0 = NULL;
		leave_line0 = strstr(one_line_buf, videoname_sign);
		if(leave_line0==NULL)
		{
			continue;
		}	

		leave_line1 = NULL;
		leave_line1 = strstr(leave_line0, equal_sign);
		if(leave_line1==NULL)
		{
			continue;
		}	

        ALOGD("%s\n", leave_line0);
		sscanf(leave_line0, "videoname%d=\"%[^\"]\" index=%d facing=%d", 
            &(pst_video_name->camid), pst_video_name->camera_name,
            &(pst_video_name->index), &(pst_video_name->facing));
		count++;
        pst_video_name++;
        
		if(count==CAM_MAX_SUPPORT) {
			break;
		}
				
		if(fgetc(fp)==EOF) 
		{ 
			break; 
		} 
		fseek(fp,-1,SEEK_CUR);		 
	}

	return count;
}

int camera_board_profiles::XMLFseekCamIDPos(FILE* fp, xml_fp_pos_s* fp_pos)
{
	char one_line_buf[256];
	int find_fmt_sign=0;
	char *leave_line, *leave_line1, *leave_line2;
	char str_camId[4];
	const char *equal_sign = "="; 
	int count=0;
	
	if(fp==NULL)
		return -1;
	
	if(fp_pos==NULL)
		return -2;
		
	memset(str_camId, 0x00, sizeof(str_camId));
	sprintf(str_camId, "%d", fp_pos->camid);	
	fseek(fp,0,SEEK_SET); 
	while(fgets(one_line_buf,256,fp) != NULL) 
	{
		if(strlen(one_line_buf) < 3) //line is NULL
		{ 
			continue; 
		} 
		
		if(find_fmt_sign==0)
		{
			leave_line = NULL; 
			leave_line = strstr(one_line_buf, "<CamcorderProfiles"); 
			if(leave_line == NULL) //no "<CamcorderProfiles" 
			{ 
				continue; 
			} 
			
			leave_line1 = NULL; 
			leave_line1 = strstr(leave_line,equal_sign); 
			if(leave_line1 == NULL) //no "="
			{ 
				continue; 
			}
			
			leave_line2 = NULL; 
			leave_line2 = strstr(leave_line1,str_camId); 
			if(leave_line2 == NULL) //no "0/1"
			{ 
				continue; 
			}else{
				fp_pos->camid_start = ftell(fp);
				find_fmt_sign=1;
				continue;
			}
		}else{
			leave_line = NULL; 
			leave_line = strstr(one_line_buf, "</CamcorderProfiles>"); 
			if(leave_line == NULL) //no 
			{ 
				continue; 
			}else{
				fp_pos->camid_end = ftell(fp);
				break;
			}
		}
			
		if(fgetc(fp)==EOF) 
		{ 
			break; 
		} 
		fseek(fp,-1,SEEK_CUR); 
		memset(one_line_buf,0,sizeof(one_line_buf));
	}
	
	return 0;
}

int camera_board_profiles::FindResolution(camera_board_profiles* profiles, xml_video_element_s* find_element)
{
	int ret = -1;
    unsigned int j;
	size_t nCamNum = profiles->mDevideConnectVector.size();
    size_t nDVNum = 0; 

    ALOGD("find element camid(%d) quality(%s) width(%d)\n",find_element->n_cameraId, find_element->str_quality, find_element->n_width);
    if(find_element->n_cameraId < (int)nCamNum){
        nDVNum = profiles->mDevideConnectVector[find_element->n_cameraId]->mSoftInfo.mDV_vector.size();
        for(j=0; j<nDVNum; j++){	
            rk_DV_info* DVInfo = profiles->mDevideConnectVector[find_element->n_cameraId]->mSoftInfo.mDV_vector[j];
    		if(DVInfo->mWidth==find_element->n_width)
    		{
    			find_element->n_height = DVInfo->mHeight;
    			find_element->n_frameRate = DVInfo->mFps;
    			find_element->isAddMark = DVInfo->mAddMask;
    			break;
    		}
        }
    }else{
        return -1;
    }
	
	if( j==nDVNum)
		return -1;
	else
		return 0;
}

int camera_board_profiles::ModifyMediaProfileXML( camera_board_profiles* profiles, char* src_xml_file, char* dst_xml_file)
{
	int ret=0, err=0;
	int alter_sign = 0;
	int find_fmt_sign=0;
	int leave_num=0;
	long now_fp_pos;
	const char *equal_sign = "="; 
	FILE *src_fp=NULL, *dst_fp=NULL;
	char one_line_buf[256];
	char frontpart_line[50];
	long front_fptmp = 0,back_fptmp = 0;
    char *leave_line, *leave_line1, *leave_line2;
    int i = 0;
    int camid_found = 0;
    
	xml_fp_pos_s fp_pos[CAM_MAX_SUPPORT];  
	xml_video_element_s find_element;

	src_fp = fopen(src_xml_file, "r");
	if(src_fp==NULL){
		err = -1;
		ALOGE("open file '%s' failed!!! (r)\n", src_xml_file);
		goto alter_exit;
	}
	
	dst_fp = fopen(dst_xml_file, "w");
	if(dst_fp==NULL){
		err = -2;
		ALOGE("open file '%s' failed!!! (r)\n", dst_xml_file);
		goto alter_exit;
	}
	
    for(i=0; i<CAM_MAX_SUPPORT; i++) {
        fp_pos[i].camid = 1;
        fp_pos[i].camid_start = 0;
        fp_pos[i].camid_end = 0;
        ret = XMLFseekCamIDPos(src_fp, &fp_pos[i]);
        if(ret < 0 || fp_pos[i].camid_end <= fp_pos[i].camid_start){
            ALOGE("find camid(%d) failed\n", fp_pos[i].camid);
            err = -3;
            goto alter_exit;	
        }
        if(fp_pos[i].camid_end > 0 && fp_pos[i].camid_end > 0)
            camid_found = 1;
        else
            camid_found = 0;
    }

    find_element.isAddMark = 1;
    find_element.n_cameraId = -1;
    find_element.n_frameRate = 0;
    find_element.n_width = 0;
    find_element.n_height = 0;
	if(camid_found) {
		fseek(src_fp,0,SEEK_SET); 
		fseek(dst_fp,0,SEEK_SET);
			
		while(fgets(one_line_buf,256,src_fp) != NULL) 
		{ 
			if(strlen(one_line_buf) < 3) //line is NULL
			{ 
				fputs(one_line_buf, dst_fp);
				continue; 
			} 
							
			if(find_fmt_sign==0)
			{		
				leave_line = NULL; 
				leave_line = strstr(one_line_buf,equal_sign); 
				if(leave_line == NULL) //no "="
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
				leave_line1 = NULL; 
				leave_line1 = strstr(one_line_buf, "<EncoderProfile"); 
				if(leave_line1 == NULL) 
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
							
				leave_line2 = NULL; 
				leave_line2 = strstr(leave_line1, "timelapse"); 
				if(leave_line2 == NULL) 
				{ 
					memset(find_element.str_quality, 0x00, sizeof(find_element.str_quality));
					sscanf(leave_line, "%*[^\"]\"%[^\"]", find_element.str_quality);
				}else{
					memset(find_element.str_quality, 0x00, sizeof(find_element.str_quality));
					sscanf(leave_line, "%*[^\"]\"timelapse%[^\"]", find_element.str_quality);
				} 
								
				//ALOGD("quality %s\n", find_element.str_quality);			
				find_fmt_sign = 1;
				front_fptmp = ftell(dst_fp);
				fprintf(dst_fp, "     \n");	
				fputs(one_line_buf, dst_fp);
				continue; 
			}
			else if(find_fmt_sign==1)
			{		
				leave_line = NULL; 
				leave_line = strstr(one_line_buf,equal_sign); 
				if(leave_line == NULL) //no "="
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				}
							
				leave_line1 = NULL; 
				leave_line1 = strstr(one_line_buf,"width"); 
				if(leave_line1 == NULL) //no "width"
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
		 		sscanf(leave_line, "%*[^1-9]%d\"", &(find_element.n_width));
				//ALOGD("%d\n", find_element.n_width);
				find_fmt_sign=2;
				fputs(one_line_buf, dst_fp);
				continue; 	 
			}
			else if(find_fmt_sign==2)
			{		
				leave_line = NULL; 
				leave_line = strstr(one_line_buf,equal_sign); 
				if(leave_line == NULL) //no "="
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				}
				leave_line1 = NULL; 
				leave_line1 = strstr(one_line_buf, "frameRate"); 
				if(leave_line1 == NULL) //no "framRate"
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
			
				now_fp_pos = ftell(src_fp);
                find_element.n_cameraId = -1;
                for (i=0; i<CAM_MAX_SUPPORT; i++) {
                    if(now_fp_pos>fp_pos[i].camid_start && now_fp_pos<fp_pos[i].camid_end) {
                        find_element.n_cameraId = i;
                        break;
                    }
                }
				
				if(find_element.n_cameraId != -1){
					ret = FindResolution(profiles, &find_element);
					if(ret==0){
						leave_num = leave_line - one_line_buf;
						memset(frontpart_line,0,sizeof(frontpart_line)); 
						strncpy(frontpart_line,one_line_buf,leave_num);  
						fputs(frontpart_line,dst_fp);
											
						//ALOGD("new frameRate %d  isaddmark(%d)\n", find_element.n_frameRate, find_element.isAddMark);
						fprintf(dst_fp,"=\"%d\" /> \n", (find_element.n_frameRate)); 
						alter_sign++; 
						find_fmt_sign = 3;	
						ALOGD("XML modify: camID(%d) resolution:%s(%dx%d) fps(%d) isaddmark(%d)\n",find_element.n_cameraId,find_element.str_quality, 
							find_element.n_width, find_element.n_height, find_element.n_frameRate, find_element.isAddMark);
					}else{
						ALOGD("WARNING: can't find camID(%d) resolution:%s(%dx), addmark!!!\n", find_element.n_cameraId,find_element.str_quality, find_element.n_width);
						find_element.isAddMark=1;
						find_fmt_sign = 3;
						fputs(one_line_buf, dst_fp);
						//continue;
					}
				}else{
					find_fmt_sign = 3;
					fputs(one_line_buf, dst_fp);
					continue;
				}
			}else if(find_fmt_sign==3){
				leave_line = NULL; 
				leave_line = strstr(one_line_buf,"</EncoderProfile>"); 
				if(leave_line == NULL) //no "framRate"
				{ 
					fputs(one_line_buf, dst_fp);
					continue; 
				} 
				fputs(one_line_buf, dst_fp);	
				if(find_element.isAddMark){
					back_fptmp = ftell(dst_fp);
					fseek(dst_fp,front_fptmp,SEEK_SET); 
					fprintf(dst_fp, "<!--  \n");
					fseek(dst_fp,back_fptmp,SEEK_SET); 
					fprintf(dst_fp, "-->  \n");
					find_element.isAddMark=0;
				}
				find_fmt_sign=0;
			}
		
			if(fgetc(src_fp)==EOF) 
			{ 
				break; 
			} 
			fseek(src_fp,-1,SEEK_CUR); 
			memset(one_line_buf,0,sizeof(one_line_buf)); 
		} 
	}
	
alter_exit:
    if(src_fp)
	    fclose(src_fp); 

    if(dst_fp)
	    fclose(dst_fp);

	if(err==0){
		//remove(src_xml_file);    
		//chmod(src_xml_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        remove(src_xml_file);
		chmod(dst_xml_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	}
	return err;
}


int camera_board_profiles::ProduceNewXml(camera_board_profiles* profiles)
{
    char temp_dst_file[50];
    char dst_file[50];
    char default_file[50];
    int err=0;
	int res=0;

    //CheckSensorSupportDV
    AddConnectSensorToVector(profiles);
	size_t nCamNum =profiles->mDevideConnectVector.size();
	
	//verrify media_xml_device is supported by board xml 
    for(int i=0; (i<profiles->xml_device_count && i<CAM_MAX_SUPPORT); i++)
    {
    	res |= ConnectDevHaveDev(profiles, (profiles->mXmlDevInfo + i));
    }

    if(res == RK_RET_SUCCESS && profiles->xml_device_count==(int)nCamNum){
        return RK_RET_SUCCESS;
    }

    int fileexit = access(RK_DST_MEDIA_PROFILES_XML_PATH, 0);
    ALOGD("judge the media profile xml fileexit = %d\n", fileexit);	
    //if((int)nCamNum>=1){ 
    if((int)nCamNum>=1 && fileexit == -1){
        LOG1("enter produce new xml\n");
        //new xml file name
        strlcpy(default_file, RK_DEFAULT_MEDIA_PROFILES_XML_PATH, sizeof(default_file));
        strlcpy(dst_file, RK_DST_MEDIA_PROFILES_XML_PATH, sizeof(dst_file));
        strlcpy(temp_dst_file, RK_TMP_MEDIA_PROFILES_XML_PATH, sizeof(temp_dst_file));
		
        for(int i=0; i<(int)nCamNum; i++){		
            CheckSensorSupportDV(profiles->mDevideConnectVector[i]);
        }
        
        //write name to xml 
        err = WriteDevNameTOXML(profiles, default_file, temp_dst_file);
        if(err){
            ALOGE("write dev name to xml failed\n");
            goto end;
        }

        //modify xml
        err = ModifyMediaProfileXML( profiles, temp_dst_file, dst_file);
        if(err){
            ALOGE("modify xml failed\n");
            goto end;
        }

        LOG1("exit produce new xml\n");
    }

end:
    return err;
    
}

int camera_board_profiles::LoadSensor(camera_board_profiles* profiles)
{    
    char dst_file[50];
    int err = RK_RET_SUCCESS;
    int count = 0;
	int result= 0;

    LOG_FUNCTION_NAME
        
    strlcpy(dst_file, RK_DST_MEDIA_PROFILES_XML_PATH, sizeof(dst_file));
    ALOGD("read cam name from xml(%s)\n",dst_file );

    FILE* fp = fopen(dst_file, "r");
    if(!fp){
        ALOGE(" is not exist, register all\n");
        goto err_end;
    }

    //read sensor name
    count = ReadDevNameFromXML(fp, profiles->mXmlDevInfo);
	profiles->xml_device_count = count;
    if(count<1){
        ALOGD("media_profiles.xml not have any camera device\n");
        goto err_end;
    }

    ALOGD("find camera count(%d) cam1(%s) cam2(%s)\n", count, profiles->mXmlDevInfo[0].camera_name,  profiles->mXmlDevInfo[1].camera_name);
    //verrify media_xml_device is supported by board xml 
    for(int i=0; (i<count && i<CAM_MAX_SUPPORT); i++)
    {
    	if(strcmp(profiles->mXmlDevInfo[i].camera_name, UVC_CAM_NAME)!= 0){
        	err |= BoardFileHaveDev(profiles, (profiles->mXmlDevInfo+i));
		}
    }

    if(err != RK_RET_SUCCESS){
        goto err_end;
    }

    //register exist sensor
    for(int i=0; (i<count && i<CAM_MAX_SUPPORT); i++){
		if(strcmp(profiles->mXmlDevInfo[i].camera_name, UVC_CAM_NAME)== 0){
			continue;
		}
				
        result = OpenAndRegistOneSensor(profiles->mDevieVector[profiles->mXmlDevInfo[i].index]);
		if(result != 0){
			goto err_end;
		}
       
    }

	if(profiles->mDevieVector.size()>0){
		
		return RK_RET_SUCCESS;
	}else
		return RK_RET_NOSETUP;

err_end:
    OpenAndRegistALLSensor(profiles);
    LOG_FUNCTION_NAME_EXIT
    return err;
    
}

int camera_board_profiles::BoardFileHaveDev(camera_board_profiles* profiles, xml_DEV_name_s* media_xml_device )
{
    size_t nCamNum = profiles->mDevieVector.size();

    if(media_xml_device->index < (int)nCamNum)
    {
        rk_sensor_info *pSensorInfo =  &(profiles->mDevieVector[media_xml_device->index]->mHardInfo.mSensorInfo);
        if(!strcmp(media_xml_device->camera_name, pSensorInfo->mSensorName) 
            && (media_xml_device->facing == pSensorInfo->mFacing))
        {
            return RK_RET_SUCCESS;
        }  
    }
    return RK_RET_NOSETUP;
}

void camera_board_profiles::AddConnectSensorToVector(camera_board_profiles* profiles)
{
    size_t nCamNum = profiles->mDevieVector.size();

    for(int i=0; i<(int)nCamNum; i++)
    {
        if(profiles->mDevieVector[i]->mIsConnect == 1)
        {
            profiles->mDevideConnectVector.add(profiles->mDevieVector[i]);
        }
    }
}

int camera_board_profiles::ConnectDevHaveDev(camera_board_profiles* profiles, xml_DEV_name_s* media_xml_device )
{
    size_t nCamNum = profiles->mDevideConnectVector.size();
    
	for(int i=0; i<(int)nCamNum; i++)
    {
        rk_sensor_info *pSensorInfo =  &(profiles->mDevideConnectVector[i]->mHardInfo.mSensorInfo);
        if(!strcmp(media_xml_device->camera_name, pSensorInfo->mSensorName) 
            && (media_xml_device->facing == pSensorInfo->mFacing))
        {
            return RK_RET_SUCCESS;
        }  
    }
    return RK_RET_NOSETUP;
}



