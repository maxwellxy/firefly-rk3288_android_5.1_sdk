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
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>



#define BOOTSIGN		"RK28@Copyright2008Rockchip"
#define BOOTSIGN_SIZE	32
#define CHECK_SIZE		16
#define HEADINFO_SIZE	512
#define BCD2INT(num) (((((num)>>4)&0x0F)*10)+((num)&0x0F))

#define FILE_OPEN_RO        "rb"
#define FILE_OPEN_RW        "r+b"
#define FILE_OPEN_RW_CREATE "w+b"

#define MAX_LOADER_LEN      1024*1024


typedef struct  _rk_time {
	unsigned short		usYear;
	unsigned short		usMonth;
	unsigned short		usDate;
	unsigned short		usHour;
	unsigned short		usMinute;
	unsigned short		usSecond;
}RK_TIME;

typedef struct _RK28BOOT_HEAD{
	char				szSign[BOOTSIGN_SIZE];
	unsigned char		bMD5Check[CHECK_SIZE];
	RK_TIME				tmCreateTime;

	unsigned int		uiMajorVersion;
	unsigned int		uiMinorVersion;

	unsigned int		uiUsbDataOffset;
	unsigned int		uiUsbDataLen;

	unsigned int		uiUsbBootOffset;
	unsigned int		uiUsbBootLen;
	
	unsigned int		uiFlashDataOffset;
	unsigned int		uiFlashDataLen;

	unsigned int        	uiFlashBootOffset;
	unsigned int		uiFlashBootLen;
	unsigned char       ucRc4Flag;

	unsigned int		MergerVersion;		// 生成Boot文件所用Merger工具的版本号(高16字节为主版本号、低16字节为副版本号)
}RK28BOOT_HEAD, *PRK28BOOT_HEAD;


#define  BOOT_RESERVED_SIZE 57

typedef enum
{
		RK27_DEVICE=1,
		RK28_DEVICE=2,
		RKNANO_DEVICE=4
}ENUM_RKDEVICE_TYPE;
typedef enum
{
	ENTRY471=1,
	ENTRY472=2,
	ENTRYLOADER=4
}ENUM_RKBOOTENTRY;

//#pragma pack(1)
typedef struct  
{
	unsigned short usYear;
	unsigned char  ucMonth;
	unsigned char  ucDay;
	unsigned char  ucHour;
	unsigned char  ucMinute;
	unsigned char  ucSecond;
}__attribute__ ((packed)) STRUCT_RKTIME,*PSTRUCT_RKTIME;

typedef struct  
{
	unsigned int uiTag;
	unsigned short usSize;
	unsigned int  dwVersion;
	unsigned int  dwMergeVersion;
	STRUCT_RKTIME stReleaseTime;
	ENUM_RKDEVICE_TYPE emSupportChip;
    	unsigned char temp[12];
	unsigned char ucLoaderEntryCount;
	unsigned int dwLoaderEntryOffset;
	unsigned char ucLoaderEntrySize;
	unsigned char ucSignFlag;
	unsigned char ucRc4Flag;
	unsigned char reserved[BOOT_RESERVED_SIZE];
}__attribute__ ((packed)) STRUCT_RKBOOT_HEAD,*PSTRUCT_RKBOOT_HEAD;

typedef struct  
{
	unsigned char ucSize;
	ENUM_RKBOOTENTRY emType;
	unsigned char szName[40];
	unsigned int dwDataOffset;
	unsigned int dwDataSize;
	unsigned int dwDataDelay;//以秒为单位
}__attribute__ ((packed)) STRUCT_RKBOOT_ENTRY,*PSTRUCT_RKBOOT_ENTRY;

//#pragma pack()

int make_loader_data(const char* old_loader, char* new_loader, int *new_loader_size)//path, RKIMAGE_HDR *hdr)
{
    int i,j;
    PSTRUCT_RKBOOT_ENTRY pFlashDataEntry = NULL;
    PSTRUCT_RKBOOT_ENTRY pFlashBootEntry = NULL;
    STRUCT_RKBOOT_HEAD *boot_hdr = NULL;
    RK28BOOT_HEAD *new_hdr = NULL;

    boot_hdr = (STRUCT_RKBOOT_HEAD*)old_loader;

// 得到FlashData/FlashBoot数据块的信息
	for (i=0;i<boot_hdr->ucLoaderEntryCount;i++)
	{
		PSTRUCT_RKBOOT_ENTRY pEntry;
		pEntry = (PSTRUCT_RKBOOT_ENTRY)(old_loader+boot_hdr->dwLoaderEntryOffset+(boot_hdr->ucLoaderEntrySize*i));
        char name[10] = "";
        for(j=0; j<20 && pEntry->szName[j]; j+=2)
            name[j/2] = pEntry->szName[j];
        if( !strcmp( name, "FlashData" ) )
            pFlashDataEntry = pEntry;
        else if( !strcmp( name, "FlashBoot" ) )
            pFlashBootEntry = pEntry;
	}

    if(pFlashDataEntry == NULL || pFlashBootEntry == NULL)
        return -1;

// 构造新的Loader数据，以传给Loader进行本地升级
    new_hdr = (RK28BOOT_HEAD*)new_loader;
    memset(new_hdr, 0, HEADINFO_SIZE);
    strcpy(new_hdr->szSign, BOOTSIGN);
    new_hdr->tmCreateTime.usYear = boot_hdr->stReleaseTime.usYear;
    new_hdr->tmCreateTime.usMonth= boot_hdr->stReleaseTime.ucMonth;
    new_hdr->tmCreateTime.usDate= boot_hdr->stReleaseTime.ucDay;
    new_hdr->tmCreateTime.usHour = boot_hdr->stReleaseTime.ucHour;
    new_hdr->tmCreateTime.usMinute = boot_hdr->stReleaseTime.ucMinute;
    new_hdr->tmCreateTime.usSecond = boot_hdr->stReleaseTime.ucSecond;
    new_hdr->uiMajorVersion = (boot_hdr->dwVersion&0x0000FF00)>>8;
    new_hdr->uiMajorVersion = BCD2INT(new_hdr->uiMajorVersion);
    new_hdr->uiMinorVersion = boot_hdr->dwVersion&0x000000FF;
    new_hdr->uiMinorVersion = BCD2INT(new_hdr->uiMinorVersion);
    new_hdr->uiFlashDataOffset = HEADINFO_SIZE;
    new_hdr->uiFlashDataLen = pFlashDataEntry->dwDataSize;
    new_hdr->uiFlashBootOffset = new_hdr->uiFlashDataOffset+new_hdr->uiFlashDataLen;
    new_hdr->uiFlashBootLen = pFlashBootEntry->dwDataSize;
	new_hdr->ucRc4Flag = boot_hdr->ucRc4Flag;
	memcpy(new_loader+new_hdr->uiFlashDataOffset, old_loader+pFlashDataEntry->dwDataOffset, pFlashDataEntry->dwDataSize);
	memcpy(new_loader+new_hdr->uiFlashBootOffset, old_loader+pFlashBootEntry->dwDataOffset, pFlashBootEntry->dwDataSize);
	*new_loader_size = new_hdr->uiFlashBootOffset+new_hdr->uiFlashBootLen;
//    dump_data(new_loader, HEADINFO_SIZE);
    
    return 0;
}

int main(int argc, char **argv)
{  
    char* oldloaderfilename = argv[1];
    char* newloaderfilename = argv[2];;
	
    size_t oldloaderfilesize,newloaderfilesize;	
	
    FILE *fp;

    struct stat buf;
	
    char oldloaderbuf[MAX_LOADER_LEN] = "";
    char newloaderbuf[MAX_LOADER_LEN] = "";
	
   // oldloaderfilesize = atoi(argv[2]);
    stat(oldloaderfilename, &buf);

    oldloaderfilesize = buf.st_size;
	
    printf("oldloaderfilename=%s,oldloaderfilesize=%d,newloaderfilename=%s\n",oldloaderfilename,oldloaderfilesize,newloaderfilename);

    fp = fopen(oldloaderfilename, FILE_OPEN_RO);
    
    if(!fp)
    	{
    		printf("Can not open file %s !\n",oldloaderfilename);	
    		return -1;
    	}

    if( fread(oldloaderbuf,1,oldloaderfilesize,fp)== oldloaderfilesize)
   	{
	    make_loader_data(oldloaderbuf, newloaderbuf, &newloaderfilesize);
   	}
    else
   	{
		printf("read old loader file fail!\n");
   	}

     fclose(fp);

	  
     fp = fopen(newloaderfilename, FILE_OPEN_RW_CREATE);

     if(fwrite(newloaderbuf,1,newloaderfilesize,fp)== newloaderfilesize)
     	{
          printf("write new loader file success,size=%d \n",newloaderfilesize);
	 }
     else
	{
          printf("write new loader file fail!\n");
	 }

     fclose(fp);
    
    return 0;
}
