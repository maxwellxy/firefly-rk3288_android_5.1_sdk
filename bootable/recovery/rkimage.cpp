/*
 * Copyright (C) 2011 Rockchip Open Source Project
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
#define	_FILE_OFFSET_BITS   64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fs_mgr.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include "cutils/properties.h"

#include "common.h"
#include "install.h"
#include "mincrypt/rsa.h"
#include "minui/minui.h"
#include "minzip/SysUtil.h"
#include "minzip/Zip.h"
#include "minzip/DirUtil.h"
#include "mtdutils/mounts.h"
#include "mtdutils/mtdutils.h"
#include "mincrypt/sha.h"
#include "ui.h"
#include "screen_ui.h"
#include "roots.h"
#include "bootloader.h"
#include "rkimage.h"
#include "mtdutils/rk29.h"
#include "cutils/android_reboot.h"
extern "C" {
#include "edify/expr.h"
#include "applypatch/applypatch.h"
#include "verifier.h"
#include "board_id/custom.h"
#include "board_id/restore.h"
}

#ifdef USE_RADICAL_UPDATE
#include "radical_update.h"
#endif

extern bool gIfBoardIdCustom;


extern bool bIfUpdateLoader;
extern RecoveryUI* ui;
extern struct selabel_handle *sehandle;

extern "C" int yyparse(Expr** root, int* error_count);
extern "C" void* yy_scan_string (const char *yy_str  );


//The origin is the partition, for example recover from backup
static int read_partition(int fd, off_t offset, char* data, int size);
// The origin is the file, for example recover from udisk or sdcard
static int read_file(int fd, off_t offset, char* data, int size);

#define STEP_SIZE 1024*1024
#define MY_READ(fd, offset, data, size)\
    (g_src_isFile?\
    read_file(fd, offset, data, size):\
    read_partition(fd, offset, data, size) )

//the update package path
char g_package_target[128] = {0};   // /sdcard/update.img
char g_package_root_path[128] = {0};
bool g_src_isFile = false;
RKIMAGE_HDR g_imagehdr;
unsigned int gFwOffset = 0;

extern int dirCreateHierarchy(const char *path, int mode,
        const struct utimbuf *timestamp, bool stripFileName,
        struct selabel_handle *sehnd);

extern int property_get(const char *key, char *value, const char *default_value);
extern FILE* fopen_root_path(const char *root_path, const char *mode);
extern int wipe_data(int wipe_flags);


//===================== check usbboot mode ==========================
int check_usbboot()
{
    char param[1024];
    int fd, ret;
    char *s=NULL;
    
    memset(param,0,1024);
    fd= open("/proc/cmdline", O_RDONLY);
    ret = read(fd, (char*)param, 1024);
    printf("cmdline=%s\n",param);
    s = strstr(param,"usbfwupdate");
    if(s!= NULL)
        return 0;
    else
        return -1;
}

//===================== check sdboot mode ==========================
int check_sdboot(void)
{
    char param[1024];
    int fd, ret;
    char *s=NULL;
    printf("read cmdline\n");
    memset(param,0,1024);
    fd= open("/proc/cmdline", O_RDONLY);
    ret = read(fd, (char*)param, 1024);
    s = strstr(param,"sdfwupdate");
    if(s!= NULL)
	return 0;
    else
	return -1;
}

//====================update rkimage loader ==========================

static void dump_data(const char *data, int len) {
    int pos;
    for (pos = 0; pos < len; ) {
        LOGW("%05x: %02x", pos, data[pos]);
        for (++pos; pos < len && (pos % 24) != 0; ++pos) {
            LOGW(" %02x", data[pos]);
        }
        LOGW("\n");
    }
}

#define BOOTSIGN		"RK28@Copyright2008Rockchip"
#define BOOTSIGN_SIZE	32
#define CHECK_SIZE		16
#define HEADINFO_SIZE	512
#define BCD2INT(num) (((((num)>>4)&0x0F)*10)+((num)&0x0F))

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

	unsigned int        uiFlashBootOffset;
	unsigned int		uiFlashBootLen;
	unsigned char       ucRc4Flag;

	unsigned int		MergerVersion;		// Generate Boot file Merger tools used the version number of the high 16 bytes (mainly low 16 byte acted as the version number of the version number)
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

#pragma pack(1)
typedef struct  
{
	unsigned short usYear;
	unsigned char  ucMonth;
	unsigned char  ucDay;
	unsigned char  ucHour;
	unsigned char  ucMinute;
	unsigned char  ucSecond;
}STRUCT_RKTIME,*PSTRUCT_RKTIME;
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
}STRUCT_RKBOOT_HEAD,*PSTRUCT_RKBOOT_HEAD;
typedef struct  
{
	unsigned char ucSize;
	ENUM_RKBOOTENTRY emType;
	unsigned char szName[40];
	unsigned int dwDataOffset;
	unsigned int dwDataSize;
	unsigned int dwDataDelay;//in seconds
}STRUCT_RKBOOT_ENTRY,*PSTRUCT_RKBOOT_ENTRY;
#pragma pack()

int make_loader_data(const char* old_loader, char* new_loader, int *new_loader_size)//path, RKIMAGE_HDR *hdr)
{
    int i,j;
    PSTRUCT_RKBOOT_ENTRY pFlashDataEntry = NULL;
    PSTRUCT_RKBOOT_ENTRY pFlashBootEntry = NULL;
    STRUCT_RKBOOT_HEAD *boot_hdr = NULL;
    RK28BOOT_HEAD *new_hdr = NULL;

    boot_hdr = (STRUCT_RKBOOT_HEAD*)old_loader;

// get the data block information of FlashData/FlashBoot
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

// Construct a new Loader data, to local Loader to upgrade
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

//=======================================================

RKIMAGE_ITEM* FindItem(RKIMAGE_HDR* prkimage, const char* name)
{
	int i=0;

	for(i=0; i<prkimage->item_count; i++)
		if( !strcmp(prkimage->item[i].name, name) )
			return prkimage->item+i;

	return NULL;
}

extern unsigned long CRC_32_NEW( unsigned char * aData, unsigned long aSize, unsigned long prev_crc );
extern "C" int check_image_rsa(const char* imageFilePath, unsigned int fwOffset, unsigned int fwsize);
/*
    success return 0
    error return -1
 */
int check_image_crc(const char* mtddevname, unsigned long image_size)
{
	int size = 32<<9;
	char buffer[16*1024] = "";
	unsigned long crc = 0;
	int remain = image_size;
	int read_count = 0;
	int r=0;
    int file_offset = gFwOffset;

	int fdread = open(mtddevname, O_RDONLY);
	if(fdread < 0)
	{
        LOGE("Can't open part(%s)\n(%s)\n", mtddevname, strerror(errno));
        return -1;
	}

    lseek(fdread, gFwOffset, SEEK_SET);

	while(remain > 0)
	{
		read_count = remain>size?size:remain;

        if( 0 != MY_READ(fdread, g_src_isFile?-1:file_offset, buffer, read_count) )
		{
			LOGE("Can't read (%s)\n(%s)\n", mtddevname, strerror(errno));
			close(fdread);
			return -1;
		}
        file_offset += read_count;
        
		crc = CRC_32_NEW((unsigned char*)buffer, read_count, crc);
		remain -= read_count;
	}

	// Read in the end of the file additional CRC32 check code
    if( 0 != MY_READ(fdread, g_src_isFile?-1:file_offset, buffer, 4) )
	{
		LOGE("Can't read (%s)\n(%s)\n", mtddevname, strerror(errno));
		close(fdread);
		return -1;
	}
    file_offset += read_count;
    
	close(fdread);
	if( crc != *(unsigned long*)buffer )
	{
		LOGE("Check failed\n");
        	LOGI("crc = %04lx  buffer=%04lx \n", crc, *(unsigned long*)buffer);
		return -1;
	}

	LOGI("Check crc okay.\n");

	return 0;
}


void adjustFileOffset(RKIMAGE_HDR* hdr, int offset)
{
	int i=0;

	for(i=0; i<hdr->item_count; i++)
        hdr->item[i].offset += offset;

    return;
}

/*
    from fd offset position, Continuous read size byte of data store in a data
    offset < 0 : Not seek fd operation
    success return 0
    fail return -1
 */
static int read_file(int fd, off_t offset, char* data, int size)
{
    ssize_t r = 0;
    if( offset >= 0 )
    {
        lseek(fd, offset, SEEK_SET);
    }
    
    r = read(fd, (char*)data, size);
    if (r != size)
	{
//		LOGE("Read failed: (%s)\n", strerror(errno));
		return -1;
    }

    return 0;
}

/*
    from fd offset position, Continuous read size byte of data store in a data
    offset < 0 : Not seek fd operation
    success return 0
    fail return -1
 */
static int read_partition(int fd, off_t offset, char* data, int size)
{
    char buf[512];
    int m,n;
    int remain = size;

    if( offset >= 0 )
    {
        m = offset/512;
        n = offset%512;
        lseek(fd, m*512, SEEK_SET);
        if(n != 0)
        {
            read(fd, buf, 512);
            memcpy(data, buf+n, 512-n);
            data += 512-n;
            remain -= 512-n;
        }
    }

	while(remain > 0)
	{
		if( read(fd, buf, 512) != 512 )
		{
//			LOGE("read error: (%s)\n", strerror(errno));
			return -1;
		}
        if( remain > 512 )
        {
            memcpy(data, buf, 512);
            data += 512;
    		remain -= 512;
        }
        else
        {
            memcpy(data, buf, remain);
            data += remain;
    		remain = 0;
        }
//        LOGI("remain=%d\n", remain);
	}
    
    return 0;
}


/*
    success return 0
    fail return other
 */
static int CheckImageFile(const char* path, RKIMAGE_HDR* hdr)
{
    /* Try to open the image.
     */
	int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOGE("Can't open %s\n", path);
        return -1;
    }

/* Need to read the documents before 512 bytes,
 * to determine whether the new way of packing update.
 * If not be, according to the way to handle before then
 * If the new packaging mode, the firmware of the offset each file to adjust accordingly
 *
 */
    gFwOffset = 0;
    char buf[512] = "";
    unsigned int fwSize = 0;
    if( 0 != MY_READ(fd, 0, buf, 512) )
	{
		LOGE("Can't read %s\n(%s)\n", path, strerror(errno));
		close(fd);
		return -2;
    }
    // Confirm whether the new packaging format
    if( *((unsigned int*)buf) == 0x57464B52 )
    {
        gFwOffset = *(unsigned int*)(buf+0x21);
        fwSize = *(unsigned int *)(buf + 0x25);
    }

    if(0 != MY_READ(fd, gFwOffset, (char*)hdr, sizeof(RKIMAGE_HDR)))
	{
		LOGE("Can't read %s\n(%s)\n", path, strerror(errno));
		close(fd);
		return -2;
    }
	close(fd);

	if(hdr->tag != RKIMAGE_TAG)
	{
	    LOGI("tag: %x\n", hdr->tag);
		LOGE("Invalid image\n");
		return -3;
	}

/* check product model */
    char model[256] = {0};
    if( property_get("ro.product.model", model, NULL) <= 0)
    {
        LOGE("Not found local model!\n");
        return -4;
    }
    LOGI("Local model: %s\nUpdate model: %s\n", model, hdr->machine_model);
    if(strcmp(model, hdr->machine_model))
    {
        LOGE("Not support firmware\n");
        return -5;
    }

	LOGI("Checking...\n");

#ifdef USE_RSA_CHECK
	if(check_image_rsa(path, gFwOffset, fwSize))
	    return -6;
#else
	if(check_image_crc(path, hdr->size))
		return -6;
#endif

    if(gFwOffset)
        adjustFileOffset(hdr, gFwOffset);
    
	return 0;
}

int open_file_path(const char *path, int mode) {
    if (ensure_path_mounted(path) != 0) {
        LOGE("Can't mount %s\n", path);
        return -1;
    }

    // When writing, try to create the containing directory, if necessary.
    // Use generous permissions, the system (init.rc) will reset them.
    dirCreateHierarchy(path, 0777, NULL, 1, sehandle);

    int fd = open(path, mode);
    if (fd < 0)
    {
        LOGE("Can't open %s\n", path);
        return -3;
    }
    
    return fd;
}

// open_partition_path("BACKUP:", O_RDWR, path)
int open_partition_path(const char *part_name, int mode, char* path) {
    if (ensure_path_unmounted(part_name) != 0) {
        LOGE("Can't unmount %s\n", part_name);
        return -1;
    }

    char *p;
	char mtdname[32];
    Volume* v = volume_for_path(part_name);
    if(!strcmp(v->fs_type, "mtd")) {
#ifdef USE_OLD_NAND_DRIVER
		if(p = strstr(v->blk_device,"/dev/block/mtd/by-name/"))
    		strcpy(mtdname,p+23);
		else
			strcpy(mtdname,v->blk_device);
		const MtdPartition* partition = mtd_find_partition_by_name(mtdname);
		if (partition == NULL) {
			LOGE("failed to find \"%s\" partition to mount at \"%s\"\n",
				 mtdname, v->mount_point);
			return -1;
		}

		sprintf(path, "/dev/mtd/mtd%d", mtd_get_partition_index((MtdPartition*)partition));
#else
		if(p = strstr(v->blk_device,"/dev/block/rknand_"))
		    strcpy(path, v->blk_device);
		else
			sprintf(path, "/dev/block/rknand_%s", v->blk_device);
#endif
    }else {
    	strcpy(path, v->blk_device);
    }

    int fd = open(path, mode);
    if (fd < 0){
        LOGE("Can't open %s\n", path);
        return -3;
    }
    return fd;
}

#define MAX_LOADER_LEN      1024*1024

int write_loader(const char* src, const char* dest, int woffset)
{
    RKIMAGE_HDR *hdr = &g_imagehdr;
    int fd_src, fd_dest;
    bool dest_is_file = false;
    char destpath[PATH_MAX];
    char old_loader[MAX_LOADER_LEN] = "";
    char new_loader[MAX_LOADER_LEN] = "";
    char *buff;
    int new_loader_size = 0;

    LOGI("src=%s  dest=%s  offset=%d\n", src, dest, woffset);
    
// get loader data
    RKIMAGE_ITEM* pItem = (RKIMAGE_ITEM*)FindItem(hdr, src);
    if(pItem == NULL)
    {
		LOGE("Can't find %s \n", src);
		return -2;
    }
    if(pItem->offset > MAX_LOADER_LEN)
    {
		LOGE("Loader too large\n");
		return -2;
    }

    fd_src = open(g_package_target, O_RDONLY);
    if (fd_src <= 0) {
        LOGE("Can't open file: %s\n", g_package_target);
        return -3;
    }

    lseek(fd_src, pItem->offset, SEEK_SET);

    if( read(fd_src, old_loader, pItem->size) != pItem->size)
    {
        close(fd_src);
        LOGE("Read failed(%s)\n", strerror(errno));
        return -4;
    }
    close(fd_src);

// create new loader data
	if ( (*(unsigned int*)old_loader)==0x544F4F42 )
    {// new Loader
        if( make_loader_data(old_loader, new_loader, &new_loader_size) )
        {
            LOGE("Invalid loader file\n");
            return -5;
        }
	    buff = new_loader;
    }
    else
	{// old loader
	    buff = old_loader;
        new_loader_size = pItem->size;
	}

    if( !strcmp(dest, "/backup") )
	dest_is_file = false;
	
    LOGI("dest isFile: %d\n", dest_is_file);
    if(dest_is_file)
        fd_dest = open_file_path(dest, O_RDWR|O_CREAT);
    else
        fd_dest = open_partition_path(dest, O_RDWR,destpath);
    
    if (fd_dest < 0) {
        LOGE("Bad dest path %s\n", dest);
        return -6;
    }
    lseek(fd_dest, woffset, SEEK_SET);

	if( write(fd_dest, buff, new_loader_size) != new_loader_size )
	{
        close(fd_dest);
		LOGE("Write failed(%s)\n", strerror(errno));
		return -7;
	}
    sync();
    
// Read the writing of data and check
    lseek(fd_dest, woffset, SEEK_SET);
	if( read(fd_dest, old_loader, new_loader_size) != new_loader_size )
	{
        close(fd_dest);
		LOGE("Read failed(%s)\n", strerror(errno));
		return -8;
	}
    close(fd_dest);
    
    if( memcmp(old_loader, buff, new_loader_size) )
    {
        LOGE("Check failed!\n");
        return -9;
    }
    
	return 0;
}

int write_image_from_file(const char* src, const char* dest, int woffset)
{
    bool dest_is_file = false;
    char destpath[PATH_MAX];
    int fd_src, fd_dest;
    off64_t dest_offset, image_length;
    char data_buf[STEP_SIZE] = {0};
    off64_t src_remain, dest_remain;
    int src_step, dest_step;
    int count = 1;
    off64_t src_file_offset = 0;

    LOGI("src=%s  dest=%s  offset=%d\n", src, dest, woffset);

    fd_src = open(src, O_RDONLY);
    if (fd_src == 0) {
        LOGE("Can't open file: %s\n", g_package_target);
        return -5;
    }

    image_length = lseek64(fd_src, 0, SEEK_END);

	src_remain = image_length;
	src_step = STEP_SIZE;
    src_file_offset = 0;

    LOGI("size of off_t %d\n", sizeof(off64_t));
    LOGI("img length is %llu\n", src_remain);

    dest_offset = (off64_t)woffset;
    // The target is parameter zoning, fixed read write 32 sector
    dest_remain = image_length;
    dest_step = STEP_SIZE;

    if(dest_is_file)
        fd_dest = open_file_path(dest, O_RDWR|O_CREAT);
    else
        fd_dest = open_partition_path(dest, O_RDWR,destpath);
    if (fd_dest < 0) {
        close(fd_src);
        LOGE("Bad dest path %s\n", dest);
        return -6;
    }
    lseek64(fd_dest, dest_offset, SEEK_SET);

// From source data read and write goal
    off64_t read_count, write_count, real_count;
    bool bWrited = false;
//    LOGI("pItem->name=%s\n", pItem->name);

	while(src_remain>0 && dest_remain > 0)
	{
		memset(data_buf, 0, STEP_SIZE);
		// read data
		read_count = src_remain>src_step?src_step:src_remain;

		real_count = pread64(fd_src, data_buf, read_count, src_file_offset);
		printf("real_count= %llu ; read_count= %llu \n", real_count, read_count);
        if(real_count != read_count)
        {
            close(fd_src);
            close(fd_dest);
            LOGE("Read failed(%s)\n", strerror(errno));
            return bWrited?-1:-2;
        }
        src_remain -= read_count;
        src_file_offset += read_count;

        write_count = (src_remain == 0)?dest_remain:dest_step;
    	bWrited=true;

		if(write(fd_dest, data_buf, dest_is_file?write_count:dest_step) != (dest_is_file?write_count:dest_step) ) {
			close(fd_src);
			close(fd_dest);
			LOGE("Write failed(%s)\n", strerror(errno));
			return -1;
		}

		dest_remain -= write_count;
//		LOGI("src: read=%d remain=%d   dest: write=%d remain=%d\n", read_count, src_remain, write_count, dest_remain);
	}

    	close(fd_src);
    	close(fd_dest);
	return 0;
}

// PACKAGE:system, "SYSTEM:", offset            
// PACKAGE:system, "USERDATA:aaa.img", 0        
// PACKAGE  == SDCARD:update.img                OK
// PACKAGE  == BACKUP:                          OK
/*
 * success                   0
 * read or write fail        -1
 * read fail before write    -2
 * others                    <-2
 */
int write_image(const char* src, const char* dest, int woffset)
{
    RKIMAGE_HDR *hdr = &g_imagehdr;
    bool dest_is_parameter = false;
    bool dest_is_file = false;
    char destpath[PATH_MAX];
    int fd_src, fd_dest;
    int src_offset, dest_offset;
    char data_buf[16*1024] = {0};
    int src_remain, dest_remain;
    int src_step, dest_step;
    int count = 1;
    int src_file_offset = 0;

    LOGI("src=%s  dest=%s  offset=%d\n", src, dest, woffset);
    
// Source for the necessary information

    RKIMAGE_ITEM* pItem = (RKIMAGE_ITEM*)FindItem(hdr, src);
    if(pItem == NULL)
    {
		LOGE("Can't find %s \n", src);
		return -4;
    }
    
    fd_src = open(g_package_target, O_RDONLY);
    if (fd_src == 0) {
        LOGE("Can't open file: %s\n", g_package_target);
        return -5;
    }

	src_offset = pItem->offset;
	src_remain = pItem->size;
	src_step = 16*1024;
    lseek(fd_src, src_offset, SEEK_SET);
    src_file_offset = src_offset;

// Get target necessary information
    if( !strcmp(dest, "/backup") )
	dest_is_file = false;
	
    LOGI("dest isFile: %d\n", dest_is_file);
	
    if( !strcmp(dest, "/parameter") )
    {
        dest_is_parameter = true;
    }

    dest_offset = woffset;
    // The target is parameter zoning, fixed read write 32 sector
    dest_remain = dest_is_parameter?16*1024:pItem->size;
    dest_step = 16*1024;
    
    if(dest_is_file)
        fd_dest = open_file_path(dest, O_RDWR|O_CREAT);
    else
        fd_dest = open_partition_path(dest, O_RDWR,destpath);
    if (fd_dest < 0) {
        close(fd_src);
        LOGE("Bad dest path %s\n", dest);
        return -6;
    }
    lseek(fd_dest, dest_offset, SEEK_SET);

// From source data read and write goal
    int read_count, write_count;
    bool bWrited = false;
    
    bool bSystemEncrypted = false;
    int remain_encrypt = 128*1024;
    long long sys_key = 0x8399190660919938;
    unsigned char plain[16*1024]={0};
    bool bFirst = true;
//    LOGI("pItem->name=%s\n", pItem->name);
    
	while(src_remain>0 && dest_remain > 0)
	{
		memset(data_buf, 0, 16*1024);
		// read data
		read_count = src_remain>src_step?src_step:src_remain;

//        if( read(fd_src, data_buf, read_count) != read_count)
        if( MY_READ(fd_src, src_file_offset, data_buf, read_count) != 0)
        {
            close(fd_src);
            close(fd_dest);
            LOGE("Read failed(%s)\n", strerror(errno));
            return bWrited?-1:-2;
        }
        src_remain -= read_count;
        src_file_offset += read_count;

#if 0
        if( !strcmp(pItem->name, "system") )// write system
        {
            if( bFirst && (*(unsigned int*)data_buf)!=0x28cd3d45 )
            {// system.img encrypted
//                LOGI("encrypt!!!\n");
                bSystemEncrypted = true;
            }
            if( bSystemEncrypted && remain_encrypt>0 )
            {
                // To decrypt data
//                LOGI("remain_decrypt=%d\n", remain_encrypt);
                DES_decrypt(data_buf, plain, 16*1024, sys_key);
                remain_encrypt -= 16*1024;
                memcpy(data_buf, plain, 16*1024);
            }
        }
#endif
        bFirst = false;

        write_count = (src_remain == 0)?dest_remain:dest_step;
    	bWrited=true;

    	if(dest_is_parameter) {
    		printf("write_image: target is parameter!\n");
    		int i = 0;
    		for(i = 0; i <= 3; i++) {
    			printf("write the %d times!\n", i);
    			lseek(fd_dest, 512*1024*i, SEEK_SET);
				if(write(fd_dest, data_buf, dest_is_file?write_count:dest_step) != (dest_is_file?write_count:dest_step) ) {
					close(fd_src);
					close(fd_dest);
					LOGE("Write failed(%s)\n", strerror(errno));
					return -1;
				}
    		}
    	}else {
    		if(write(fd_dest, data_buf, dest_is_file?write_count:dest_step) != (dest_is_file?write_count:dest_step) ) {
    			close(fd_src);
    			close(fd_dest);
    			LOGE("Write failed(%s)\n", strerror(errno));
    			return -1;
    		}
    	}

		dest_remain -= write_count;
//		LOGI("src: read=%d remain=%d   dest: write=%d remain=%d\n", read_count, src_remain, write_count, dest_remain);
	}
	
    	close(fd_src);
    	close(fd_dest);
	return 0;
}

int copy_file_from_image(const char* src, const char* dest, int woffset) {
	RKIMAGE_HDR *hdr = &g_imagehdr;
	bool dest_is_parameter = false;
	bool dest_is_file = true;
	char destpath[PATH_MAX];
	int fd_src, fd_dest;
	int src_offset, dest_offset;
	char data_buf[16*1024] = {0};
	int src_remain, dest_remain;
	int src_step, dest_step;
	int count = 1;
	int src_file_offset = 0;

	LOGI("src=%s  dest=%s  offset=%d\n", src, dest, woffset);

	RKIMAGE_ITEM* pItem = (RKIMAGE_ITEM*)FindItem(hdr, src);
	if(pItem == NULL)
	{
		LOGE("Can't find %s \n", src);
		return -4;
	}

	fd_src = open(g_package_target, O_RDONLY);
	if (fd_src == 0) {
		LOGE("Can't open file: %s\n", g_package_target);
		return -5;
	}

	src_offset = pItem->offset;
	src_remain = pItem->size;
	src_step = 16*1024;
	lseek(fd_src, src_offset, SEEK_SET);
	src_file_offset = src_offset;

	dest_offset = woffset;

	dest_remain = dest_is_parameter?16*1024:pItem->size;
	dest_step = 16*1024;

	if(dest_is_file)
		fd_dest = open_file_path(dest, O_RDWR|O_CREAT);
	else
		fd_dest = open_partition_path(dest, O_RDWR,destpath);
	if (fd_dest < 0) {
		close(fd_src);
		LOGE("Bad dest path %s\n", dest);
		return -6;
	}
	lseek(fd_dest, dest_offset, SEEK_SET);

	int read_count, write_count;
	bool bWrited = false;

	bool bSystemEncrypted = false;
	int remain_encrypt = 128*1024;
	long long sys_key = 0x8399190660919938;
	unsigned char plain[16*1024]={0};
	bool bFirst = true;
//    LOGI("pItem->name=%s\n", pItem->name);

	while(src_remain>0 && dest_remain > 0)
	{
		memset(data_buf, 0, 16*1024);
		read_count = src_remain>src_step?src_step:src_remain;

//        if( read(fd_src, data_buf, read_count) != read_count)
		if( read_file(fd_src, src_file_offset, data_buf, read_count) != 0)
		{
			close(fd_src);
			close(fd_dest);
			LOGE("Read failed(%s)\n", strerror(errno));
			return bWrited?-1:-2;
		}
		src_remain -= read_count;
		src_file_offset += read_count;
		bFirst = false;

		write_count = (src_remain == 0)?dest_remain:dest_step;

		int i=0;
		for(i=0; i<count; i++)
		{
			bWrited=true;

			if( write(fd_dest, data_buf, dest_is_file?write_count:dest_step) != (dest_is_file?write_count:dest_step) )
			{
				close(fd_src);
				close(fd_dest);
				LOGE("Write failed(%s)\n", strerror(errno));
				return -1;
			}
		}
		dest_remain -= write_count;
//		LOGI("src: read=%d remain=%d   dest: write=%d remain=%d\n", read_count, src_remain, write_count, dest_remain);
	}

	close(fd_src);
	close(fd_dest);
	return 0;
}

int my_memcmp(void *_a, void *_b, unsigned len, int *index)
{
    char *a = (char *)_a;
    char *b = (char *)_b;

    while(len-- > 0) {
        if(*a++ != *b++)
        {
            if(index != NULL) *index = (a-(char*)_a-1);
            return 1;
        }
    }
    return 0;
}

// PACKAGE:system, "SYSTEM:", offset            
// PACKAGE:system, "USERDATA:aaa.img", 0        
// PACKAGE  == SDCARD:update.img                OK
// PACKAGE  == BACKUP:                          OK
/*
 * success                   0
 * read or write fail        -1
 * read fail before write    -2
 * others                    <-2
 */
#define COMPARE_BUFFER_SIZE ((int)(1024*1024))
int image_compare(const char* src, const char* dest, int woffset)
{
    RKIMAGE_HDR *hdr = &g_imagehdr;
    bool dest_is_parameter = false;
    bool dest_is_file = false;
    char destpath[PATH_MAX];
    int fd_src, fd_dest;
    int src_offset, dest_offset;
    char *src_buf;//[16*1024] = {0};
    char *dest_buf;//[16*1024] = {0};
    int src_remain, dest_remain;
    int src_step, dest_step;
    int count = 1;
    int src_file_offset = 0;

    LOGI("src=%s  dest=%s  offset=%d\n", src, dest, woffset);

    RKIMAGE_ITEM* pItem = (RKIMAGE_ITEM*)FindItem(hdr, src);
    if(pItem == NULL)
    {
		LOGE("Can't find %s \n", src);
		return -4;
    }
    
    fd_src = open(g_package_target, O_RDONLY);
    if (fd_src == 0) {
        LOGE("Can't open file: %s\n", g_package_target);
        return -5;
    }

	src_offset = pItem->offset;
	src_remain = pItem->size;
	src_step = COMPARE_BUFFER_SIZE;
    lseek(fd_src, src_offset, SEEK_SET);
    src_file_offset = src_offset;

//    dest_is_file = !root_is_partition(dest);
//    LOGI("DEST isFile: %d\n", dest_is_file);
    if( !strcmp(dest, "/parameter") )
    {
        dest_is_parameter = true;
    }

    dest_offset = woffset;

    dest_remain = dest_is_parameter?16*1024:pItem->size;
    dest_step = COMPARE_BUFFER_SIZE;
    
    if(dest_is_file)
        fd_dest = open_file_path(dest, O_RDWR|O_CREAT);
    else
        fd_dest = open_partition_path(dest, O_RDWR,destpath);
    if (fd_dest < 0) {
        close(fd_src);
        LOGE("Bad dest path %s\n", dest);
        return -6;
    }
    lseek(fd_dest, dest_offset, SEEK_SET);

#if 0
    LOGI("=== SRC ===\n");
    LOGI("size: %d\n", pItem->size);
    LOGI("usespace: %d\n", pItem->usespace);
    LOGI("offset: %d\n", src_offset);
    LOGI("src_remain: %d\n", src_remain);

    LOGI("=== DEST ===\n");
    LOGI("isFile: %d\n", dest_is_file);
    LOGI("offset: %d\n", dest_offset);
    LOGI("dest_remain: %d\n", dest_remain);
#endif

    int src_read_count, dest_read_count;

    bool bSystemEncrypted = false;
    int remain_encrypt = 128*1024;
    long long sys_key = 0x8399190660919938;
    //unsigned char plain[16*1024]={0};
    bool bFirst = true;
//    LOGI("pItem->name=%s\n", pItem->name);

    src_buf = malloc(COMPARE_BUFFER_SIZE);
    if (src_buf == 0) {
        LOGE("malloc memory error\n");
        close(fd_src);
        close(fd_dest);
        return -7;
    }

    dest_buf = malloc(COMPARE_BUFFER_SIZE);
    if (src_buf == 0) {
       LOGE("malloc memory error\n");
       close(fd_src);
        close(fd_dest);
        free(src_buf);
        return -7;
    }
    
	while(src_remain>0 && dest_remain > 0)
	{
		src_read_count = src_remain>src_step?src_step:src_remain;

		//memset(src_buf, 0, 16*1024);
//        if( read(fd_src, src_buf, src_read_count) != src_read_count)
        if( MY_READ(fd_src, src_file_offset, src_buf, src_read_count) != 0)
        {
            close(fd_src);
            close(fd_dest);
            free(src_buf);
            free(dest_buf);
            LOGE("Read failed(%s)\n", strerror(errno));
            return -2;
        }
        src_remain -= src_read_count;
        src_file_offset += src_read_count;

#if 0
        if( !strcmp(pItem->name, "system") )
        {
            if( bFirst && (*(unsigned int*)src_buf)!=0x28cd3d45 )
            {
//                LOGI("encrypt!!!\n");
                bSystemEncrypted = true;
            }
            if( bSystemEncrypted && remain_encrypt>0 )
            {
                //
//                LOGI("remain_decrypt=%d\n", remain_encrypt);
                DES_decrypt(src_buf, plain, 16*1024, sys_key);
                remain_encrypt -= 16*1024;
                memcpy(src_buf, plain, 16*1024);
            }
        }
#endif
        bFirst = false;

        dest_read_count = (src_remain == 0)?dest_remain:dest_step;
		int i=0;
		int diff_index = 0;

		//memset(dest_buf, 0, 16*1024);
		if( read(fd_dest, dest_buf, dest_is_file?dest_read_count:dest_step) != (dest_is_file?dest_read_count:dest_step) )
		{
			close(fd_src);
			close(fd_dest);
			free(src_buf);
			free(dest_buf);
			LOGE("Read failed(%s)\n", strerror(errno));
			return -2;
		}

		if( my_memcmp(src_buf, dest_buf, src_read_count, &diff_index) )
		{
			LOGE("Check failed at: 0x%X\n", diff_index);
			diff_index = (diff_index>>3)<<3;
			int j=0, k=0;
			for(k=0; k<8; k++)
			{
				LOGI("%08X: ", diff_index+k*16);
				for(j=0; j<16; j++)
					LOGI("%02X ", src_buf[diff_index+k*16+j]);
				LOGI("\n");

				LOGI("%08X: ", diff_index+k*16);
				for(j=0; j<16; j++)
					LOGI("%02X ", dest_buf[diff_index+k*16+j]);
				LOGI("\n");
				LOGI("---\n");

				if( (diff_index+k*16+j) > COMPARE_BUFFER_SIZE ) break;
			}
			close(fd_src);
			close(fd_dest);
			return -1;
		}
	}
		dest_remain -= dest_read_count;
//		LOGI("src: read=%d remain=%d   dest: read=%d remain=%d\n", src_read_count, src_remain, dest_read_count, dest_remain);
    close(fd_src);
    close(fd_dest);
    free(src_buf);
    free(dest_buf);
	return 0;
}

int cmd_write_blcmd(const char *blcmd)
{
       ui->Print("Writing bootloader command...\n");
	struct bootloader_message boot;
	memset(&boot, 0, sizeof(boot));
	if(blcmd!=NULL)
	{
    		sprintf(boot.command, "bootloader");
    		sprintf(boot.recovery,"%s" ,blcmd);
	}
	set_bootloader_message(&boot);

// avoid deleting command in misc when exit recovery
	bIfUpdateLoader = true;

    return 0;
}

int delete_image(const char *rkimagefile)
{
	// delete update image
    ui->Print("delete file: %s...\n", rkimagefile);
    if( remove(rkimagefile) ) 
	ui->Print("Delete failed!\n");

   return 0;
}

int find_update_img(const char *path, RKIMAGE_HDR* hdr)
{
    LOGI("Update location: %s\n", path);

    if (ensure_path_mounted(path) != 0) {
        LOGE("Can't mount %s\n", path);
        return -1;
    }

    LOGI("Update file path: %s\n", path);

    if( CheckImageFile(path, hdr) )
        return -2;

	return 0;
}


static int read_data_from_image(const char* path, RKIMAGE_ITEM* pItem, char* script_data, int *script_len)
{
	int offset = 0;
	int len;

	offset = pItem->offset;
	len = pItem->size;
	*script_len = pItem->size;

    int fdread = open(path, O_RDONLY);
    if (fdread == 0) {
        LOGE("Can't open file: %s\n", path);
        return -2;
    }

    if(0 != MY_READ(fdread, offset, script_data, len))
	{
		LOGE("Read failed(%s)\n", strerror(errno));
        close(fdread);
		return -3;
	}

    close(fdread);
    
	return 0;
}

Value* WriteImageFn(const char* name, State* state, int argc, Expr* argv[]) {
	char* result = NULL;
    if (argc != 3) {
        return ErrorAbort(state, "%s() expects 3 args, got %d", name, argc);
    }
    char* 	itemname;
    char* 	partition;
    char*	offset;
	
    if (ReadArgs(state, argv, 3, &itemname, &partition, &offset) < 0) {
        return NULL;
    	}

    if (write_image(itemname,partition,0) < 0 ) { 
	return NULL;
	}
    else
    	{
    	result = strdup("");
        goto done;
	}

done:
    free(itemname);
    free(partition);
    return StringValue(result);
}

Value* CheckImageFn(const char* name, State* state, int argc, Expr* argv[]) {
	char* result = NULL;
    if (argc != 3) {
        return ErrorAbort(state, "%s() expects 3 args, got %d", name, argc);
    }
    char* 	itemname;
    char* 	partition;
    char*	offset;
	
    if (ReadArgs(state, argv, 3, &itemname, &partition, &offset) < 0) {
        return NULL;
    	}

    if (image_compare(itemname,partition,0) < 0 ) { 
	return NULL;
	}
    else
    	{
    	result = strdup("");
        goto done;
	}

done:
    free(itemname);
    free(partition);
    return StringValue(result);
}

Value* WriteLoaderFn(const char* name, State* state, int argc, Expr* argv[]) {
	char* result = NULL;
    if (argc != 3) {
        return ErrorAbort(state, "%s() expects 3 args, got %d", name, argc);
    }
    char* 	itemname;
    char* 	partition;
    char*	offset;
	
    if (ReadArgs(state, argv, 3, &itemname, &partition, &offset) < 0) {
        return NULL;
    	}

    if (write_loader(itemname,partition,(int)offset) < 0 ) { 
	return NULL;
	}
    else
    	{
    	result = strdup("");
        goto done;
	}

done:
    free(itemname);
    free(partition);
    return StringValue(result);
}

Value* WriteBlcmdFn(const char* name, State* state, int argc, Expr* argv[]) {
	char* result = NULL;
    if (argc != 1) {
        return ErrorAbort(state, "%s() expects 1 args, got %d", name, argc);
    }
    char* 	command;
	
    if (ReadArgs(state, argv, 1, &command) < 0) {
        return NULL;
    	}

    if (cmd_write_blcmd(command) < 0 ) { 
	return NULL;
	}
    else
    	{
    	result = strdup("");
        goto done;
	}

done:
    free(command);
    return StringValue(result);
}

Value* ExtractFileFn(const char* name, State* state, int argc, Expr* argv[]) {
	char* result = NULL;
	    if (argc != 2) {
	        return ErrorAbort(state, "%s() expects 3 args, got %d", name, argc);
	    }
	    char* 	itemname;
	    char* 	targetPath;
	    RKIMAGE_HDR *pHdr = &g_imagehdr;

	    if (ReadArgs(state, argv, 2, &itemname, &targetPath) < 0) {
	        return NULL;
	    }

	    if(copy_file_from_image(itemname, targetPath, 0)) {
	    	return NULL;
	    }else {
	    	result = strdup("");
	    }

	done:
	    free(itemname);
	    free(targetPath);
	    return StringValue(result);
}

char* EvaluateScript(State* state, Expr* expr) {
    Value* v = expr->fn(expr->name, state, expr->argc, expr->argv);
    if (v == NULL) return NULL;
    if (v->type != VAL_STRING) {
        free(v->data);
        free(v);
        return NULL;
    }
    char* result = v->data;
    free(v);
    return result;
}

/*
 * 0 success
 * 1 parse script failed, default update
 * -1 doing script failed, 
 */
static int
handle_update_script(const char* path, RKIMAGE_ITEM* pItem)
{
    /* Read the entire script into a buffer.
     */
    int script_len;
    char script_data[128*1024];
    Expr* root;
    int error_count = 0;

    State state;
    state.cookie = NULL;
    state.script = strdup(script_data);
    state.errmsg = NULL;

    LOGI("Read script\n");
    if( read_data_from_image(path, pItem, script_data, &script_len) < 0 )
    {
        LOGE("Can't read update script\n");
        return -1;
    }

// earlier update script has no this line
    if( strncmp(script_data, "#!enable_script", strlen("#!enable_script")) )
    {
        LOGE("Invalid update script\n");
        return -1;
    }

    /* Parse the script.  Note that the script and parse tree are never freed.
     */
    LOGI("Parse the script\n");

    ::yy_scan_string(script_data);

    int error = yyparse(&root, &error_count);
	
    if (error != 0 || error_count > 0) {
        LOGE("%d parse errors\n", error_count);
        return -1;
    }

    char* result = EvaluateScript(&state,root);
    if (result == NULL) {
    	LOGE("EvaluateScript return null\n");
		return -1;        
    } 
    free(result);

    ui->Print("Installation complete.\n");
    
    return 0;
}

void RegisterInstallRKimageFunctions(void) 
{
    RegisterFunction("write_image", WriteImageFn);
    //RegisterFunction("write_loader", WriteLoaderFn);
    RegisterFunction("check_image", CheckImageFn);
    RegisterFunction("extract_file", ExtractFileFn);
    //RegisterFunction("write_blcmd", WriteBlcmdFn);
}

//==============================================================
// mount(fs_type, partition_type, location, mount_point)
//
//    fs_type="yaffs2" partition_type="MTD"     location=partition
//    fs_type="ext4"   partition_type="EMMC"    location=device
Value* MountFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* result = NULL;
    if (argc != 4) {
        return ErrorAbort(state, "%s() expects 4 args, got %d", name, argc);
    }
    char* fs_type;
    char* partition_type;
    char* location;
    char* mount_point;
    if (ReadArgs(state, argv, 4, &fs_type, &partition_type,
                 &location, &mount_point) < 0) {
        return NULL;
    }

    if (strlen(fs_type) == 0) {
        ErrorAbort(state, "fs_type argument to %s() can't be empty", name);
        goto done;
    }
    if (strlen(partition_type) == 0) {
        ErrorAbort(state, "partition_type argument to %s() can't be empty",
                   name);
        goto done;
    }
    if (strlen(location) == 0) {
        ErrorAbort(state, "location argument to %s() can't be empty", name);
        goto done;
    }
    if (strlen(mount_point) == 0) {
        ErrorAbort(state, "mount_point argument to %s() can't be empty", name);
        goto done;
    }

    mkdir(mount_point, 0755);

    if (strcmp(partition_type, "MTD") == 0) {
        mtd_scan_partitions();
        const MtdPartition* mtd;
        mtd = mtd_find_partition_by_name(location);
        if (mtd == NULL) {
            fprintf(stderr, "%s: no mtd partition named \"%s\"",
                    name, location);
            result = strdup("");
            goto done;
        }
        if (mtd_mount_partition(mtd, mount_point, fs_type, 0 /* rw */) != 0) {
            fprintf(stderr, "mtd mount of %s failed: %s\n",
                    location, strerror(errno));
            result = strdup("");
            goto done;
        }
        result = mount_point;
    } else {
        if (mount(location, mount_point, fs_type,
                  MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0) {
            fprintf(stderr, "%s: failed to mount %s at %s: %s\n",
                    name, location, mount_point, strerror(errno));
            result = strdup("");
        } else {
            result = mount_point;
        }
    }

done:
    free(fs_type);
    free(partition_type);
    free(location);
    if (result != mount_point) free(mount_point);
    return StringValue(result);
}


Value* UnmountFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* result = NULL;
    const MountedVolume* vol;

    if (argc != 1) {
        return ErrorAbort(state, "%s() expects 1 arg, got %d", name, argc);
    }
    char* mount_point;
    if (ReadArgs(state, argv, 1, &mount_point) < 0) {
        return NULL;
    }
    if (strlen(mount_point) == 0) {
        ErrorAbort(state, "mount_point argument to unmount() can't be empty");
        goto done;
    }

    scan_mounted_volumes();
    vol = find_mounted_volume_by_mount_point(mount_point);
    if (vol == NULL) {
        fprintf(stderr, "unmount of %s failed; no such volume\n", mount_point);
        result = strdup("");
    } else {
        unmount_mounted_volume(vol);
        result = mount_point;
    }

done:
    if (result != mount_point) free(mount_point);
    return StringValue(result);
}


// format(fs_type, partition_type, location, fs_size, mount_point)
//
//    fs_type="yaffs2" partition_type="MTD"     location=partition fs_size=<bytes> mount_point=<location>
//    fs_type="ext4"   partition_type="EMMC"    location=device    fs_size=<bytes> mount_point=<location>
//    if fs_size == 0, then make_ext4fs uses the entire partition.
//    if fs_size > 0, that is the size to use
//    if fs_size < 0, then reserve that many bytes at the end of the partition
Value* FormatFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* result = NULL;
    if (argc != 5) {
        return ErrorAbort(state, "%s() expects 5 args, got %d", name, argc);
    }
    char* fs_type;
    char* partition_type;
    char* location;
    char* fs_size;
    char* mount_point;

    if (ReadArgs(state, argv, 5, &fs_type, &partition_type, &location, &fs_size, &mount_point) < 0) {
        return NULL;
    }

    if (strlen(fs_type) == 0) {
        ErrorAbort(state, "fs_type argument to %s() can't be empty", name);
        goto done;
    }
    if (strlen(partition_type) == 0) {
        ErrorAbort(state, "partition_type argument to %s() can't be empty",
                   name);
        goto done;
    }
    if (strlen(location) == 0) {
        ErrorAbort(state, "location argument to %s() can't be empty", name);
        goto done;
    }

    if (strlen(mount_point) == 0) {
        ErrorAbort(state, "mount_point argument to %s() can't be empty", name);
        goto done;
    }

    if (strcmp(partition_type, "MTD") == 0) {
        mtd_scan_partitions();
        const MtdPartition* mtd = mtd_find_partition_by_name(location);
        if (mtd == NULL) {
            fprintf(stderr, "%s: no mtd partition named \"%s\"",
                    name, location);
            result = strdup("");
            goto done;
        }
        MtdWriteContext* ctx = mtd_write_partition(mtd);
        if (ctx == NULL) {
            fprintf(stderr, "%s: can't write \"%s\"", name, location);
            result = strdup("");
            goto done;
        }
        if (mtd_erase_blocks(ctx, -1) == -1) {
            mtd_write_close(ctx);
            fprintf(stderr, "%s: failed to erase \"%s\"", name, location);
            result = strdup("");
            goto done;
        }
        if (mtd_write_close(ctx) != 0) {
            fprintf(stderr, "%s: failed to close \"%s\"", name, location);
            result = strdup("");
            goto done;
        }
        result = location;
#ifdef USE_EXT4
    } else if (strcmp(fs_type, "ext4") == 0) {
        int status = rk_make_ext4fs(location, atoll(fs_size), mount_point);
        if (status != 0) {
            fprintf(stderr, "%s: make_ext4fs failed (%d) on %s",
                    name, status, location);
            result = strdup("");
            goto done;
        }
        result = location;
#endif
    } else if (strcmp(fs_type, "vfat") == 0) {
		char volume_label[256] = "\0";
		property_get("UserVolumeLabel", volume_label, "");
		LOGI("VolumeLabel: %s\n", volume_label);
		int status = make_vfat(location, volume_label);
		if (status != 0) {
			LOGE("format_volume: make_vfat failed on %s\n", location);
			goto done;
		 }
		result = location;
    } else if (strcmp(fs_type, "ntfs") == 0) {
    	ensure_path_mounted("/system");
		char volume_label[256] = "\0";
		property_get("UserVolumeLabel", volume_label, "");
		LOGI("VolumeLabel: %s\n", volume_label);
		int status = make_ntfs(location, volume_label);
		if (status != 0) {
			LOGE("format_volume: make_ntfs failed on %s\n", location);
			goto done;
		 }
		result = location;
    } else {
        fprintf(stderr, "%s: unsupported fs_type \"%s\" partition_type \"%s\"",
                name, fs_type, partition_type);
    }

done:
    free(fs_type);
    free(partition_type);
    if (result != location) free(location);
    return StringValue(result);
}

Value* UIPrintFn(const char* name, State* state, int argc, Expr* argv[]) {
	if (argc != 1) {
		return ErrorAbort(state, "%s() expects 1 arg, got %d", name, argc);
	}

	char* content;
	if (ReadArgs(state, argv, 1, &content) < 0) return NULL;

	ui->Print("%s\n", content);

	return StringValue(strdup(content));
}

Value* RunProgramFn(const char* name, State* state, int argc, Expr* argv[]) {
    if (argc < 1) {
        return ErrorAbort(state, "%s() expects at least 1 arg", name);
    }
    char** args = ReadVarArgs(state, argc, argv);
    if (args == NULL) {
        return NULL;
    }

    char** args2 = malloc(sizeof(char*) * (argc+1));
    memcpy(args2, args, sizeof(char*) * argc);
    args2[argc] = NULL;

    fprintf(stderr, "about to run program [%s] with %d args\n", args2[0], argc);

    pid_t child = fork();
    if (child == 0) {
        execv(args2[0], args2);
        fprintf(stderr, "run_program: execv failed: %s\n", strerror(errno));
        _exit(1);
    }
    int status;
    waitpid(child, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            fprintf(stderr, "run_program: child exited with status %d\n",
                    WEXITSTATUS(status));
        }
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "run_program: child terminated by signal %d\n",
                WTERMSIG(status));
    }

    int i;
    for (i = 0; i < argc; ++i) {
        free(args[i]);
    }
    free(args);
    free(args2);

    char buffer[20];
    sprintf(buffer, "%d", status);

    return StringValue(strdup(buffer));
}

// Take a sha-1 digest and return it as a newly-allocated hex string.
static char* PrintSha1(uint8_t* digest) {
    char* buffer = malloc(SHA_DIGEST_SIZE*2 + 1);
    int i;
    const char* alphabet = "0123456789abcdef";
    for (i = 0; i < SHA_DIGEST_SIZE; ++i) {
        buffer[i*2] = alphabet[(digest[i] >> 4) & 0xf];
        buffer[i*2+1] = alphabet[digest[i] & 0xf];
    }
    buffer[i*2] = '\0';
    return buffer;
}


// Read a local file and return its contents (the Value* returned
// is actually a FileContents*).
Value* ReadFileFn(const char* name, State* state, int argc, Expr* argv[]) {
    if (argc != 1) {
        return ErrorAbort(state, "%s() expects 1 arg, got %d", name, argc);
    }
    char* filename;
    if (ReadArgs(state, argv, 1, &filename) < 0) return NULL;

    Value* v = malloc(sizeof(Value));
    v->type = VAL_BLOB;

    FileContents fc;
    if (LoadFileContents(filename, &fc) != 0) {
        ErrorAbort(state, "%s() loading \"%s\" failed: %s",
                   name, filename, strerror(errno));
        free(filename);
        free(v);
        free(fc.data);
        return NULL;
    }

    v->size = fc.size;
    v->data = (char*)fc.data;

    free(filename);
    return v;
}

void RegisterInstallFunctions() {
    RegisterFunction("mount", MountFn);
    RegisterFunction("unmount", UnmountFn);
    RegisterFunction("format", FormatFn);
    RegisterFunction("read_file", ReadFileFn);
    RegisterFunction("ui_print", UIPrintFn);
    RegisterFunction("run_program", RunProgramFn);
}
//================================================================
/*  升级/还原系统的来源有两种:
       文件系统，如uDisk:update.img，SDCARD:update.img
       分区，如BACKUP:update.img

    PACKAGE maybe 2 kinds: g_package_target
       /flash/update.img
       /dev/block/mtdblockX

    g_src_isFile:
       filesystem    	1
       patition         0

 */
// update_file = SDCARD:update.img
/*
 * All kinds of case analysis:
 *    1 No problem                        0
 *    2 have problem,but can update continue
 *    3 have problem, can not update continue
          a Restart after the system can be normal operation    1
          b Restart after the system does not work normally (keep order, after the reset back up)   -1
 */
int install_rkimage(const char* update_file) {
    int result = 0;
    RKIMAGE_ITEM* pItem;

    ui->SetBackground(RecoveryUI::INSTALLING_UPDATE);
    ui->Print("Finding update package...\n");
    ui->ShowProgress(0, 0);
	
    memset(&g_imagehdr, 0, sizeof(g_imagehdr));
    RKIMAGE_HDR *hdr = &g_imagehdr;
	
    ui->Print("=== UPDATE RKIMAGE===\n");

    ui->Print("Find and check firmware...\n");

    g_src_isFile = true;
    
    ui->ShowProgress(0.2, 30);
    result = find_update_img(update_file, hdr);
    if(result) {
    	ui->SetBackground(RecoveryUI::ERROR);
           ui->Print("Can not found firmware image or invalid image.\n");
	    return 1;
	}

    strcpy(g_package_target, update_file);
	
    LOGI("g_package_target=%s\n", g_package_target);
	

#if 1
    ui->Print("!!! Please KEEP your USB cable or DC-in connected !!!\n");
   	if( !strncmp( update_file, "/sdcard", strlen("/sdcard")) )
        	ui->Print("!!! Do NOT remove SD card form the device !!!\n");

#if 1

#ifdef USE_BOARD_ID
    ensure_path_mounted("/cust");
    ensure_path_mounted("/system");
    restore();

    gIfBoardIdCustom = true;
#endif

#ifdef USE_RADICAL_UPDATE
    ensure_path_mounted("RU_PARTITION_MOUNT_PATH");
    ensure_path_mounted("SYSTEM_PARTITION_MOUNT_PATH");
    if ( RadicalUpdate_isApplied() )
    {
        I("a ru_pkg is applied, to reset the flag 'ru_is_applied' befor install rk_img");
        RadicalUpdate_restoreFirmwaresInOtaVer();
    }
    ensure_path_unmounted(RU_PARTITION_MOUNT_PATH);
    ensure_path_unmounted(SYSTEM_PARTITION_MOUNT_PATH);
#endif

	if( !strncmp(update_file, "/mnt/external_sd", 16) ) {
		ui->Print("Check parameter...\n");
   	    if(image_compare("parameter", "/parameter", 0)) {
   	    	ui->Print("Update parameter...\n");
   	      	result = write_image("parameter", "/parameter", 0);
   	        if(result) {
   	     	    ui->Print("Update parameter Failed(%d)\n", result);
   	       		goto update_error;
   	       	}

   	       	ui->Print("Check parameter after Update...\n");
   	       	result = image_compare("parameter", "/parameter", 0);
   	        if(result) {
   	       	    ui->Print("Check parameter Failed(%d)\n", result);
   	       		goto update_error;
   	       	}

   	        struct bootloader_message boot;
   	        memset(&boot, 0, sizeof(boot));
   	        strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
   	        char cmd[100] = "recovery\n--update_rkimage=";
   	        strcat(cmd, update_file);
   	        strlcpy(boot.recovery, cmd, sizeof(boot.recovery));
   	        set_bootloader_message(&boot);

   	        LOGI("update parameter finish...\n");

   	        android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
   	        return 0;
   	    }
   	}
#endif

	ui->ShowProgress(0.1, 12);
	ui->Print("Update boot.img...\n");
	result = write_image("boot", "/boot", 0);
    if(result == -4) {
    	//some times, we not want to boot every time;
	    ui->Print("no boot.img so ignore\n");
	}else if(result == 0) {
		ui->Print("Check boot.img...\n");
		result = image_compare("boot", "/boot", 0);
		if(result) {
			ui->Print("Failed(%d)\n", result);
			goto update_error;
		}
	}else {
		ui->Print("Failed(%d)\n", result);
		goto update_error;
	}

	ui->Print("Update uboot.img...\n");
	result = write_image("uboot", "/uboot", 0);
	if(result == -4) {
		ui->Print("no uboot.img so ignore\n");
	}else if(result == 0) {
		ui->Print("Check uboot.img...\n");
		result = image_compare("uboot", "/uboot", 0);
		if(result) {
			ui->Print("Failed(%d)\n", result);
			goto update_error;
		}
	}else {
		ui->Print("Failed(%d)\n", result);
		goto update_error;
	}

	ui->Print("Update trust.img...\n");
	result = write_image("trust", "/trust", 0);
	if(result == -4) {
		ui->Print("no trust.img so ignore\n");
	}else if(result == 0) {
		ui->Print("Check trust.img...\n");
		result = image_compare("trust", "/trust", 0);
		if(result) {
			ui->Print("Failed(%d)\n", result);
			goto update_error;
		}
	}else {
		ui->Print("Failed(%d)\n", result);
		goto update_error;
	}

    ui->ShowProgress(0.3, 100);
	ui->Print("Update system...\n");
	result = write_image("system", "/system", 0);
    if(result == -4) {
    	//some times, we not want to system every time, because system.img is so large;
    	ui->Print("no boot.img so ignore\n");
	}else if(result == 0) {
		//write success, now is to check.
		ui->Print("Check system...\n");
		result = image_compare("system", "/system", 0);
		if(result) {
			ui->Print("Failed(%d)\n", result);
			goto update_error;
		}

		//try to e2fsck check and resize system partition
		Volume* v = volume_for_path("/system");
		result = rk_check_and_resizefs(v->blk_device);
		if(result) {
			ui->Print("Failed(%d)\n", result);
			goto update_error;
		}
	}else {
		//write error
		ui->Print("Failed(%d)\n", result);
		goto update_error;
	}

// cmy: Should make sure whether to ask and write backup
    if( FindItem(hdr, "backup") != NULL) {
    	ui->Print("Update backup...\n");
    	result = write_image("backup", "/backup", 0);
        if(result) {
    	    ui->Print("Failed(%d)\n", result);
    		goto update_error;
    	}

    	ui->Print("Check backup...\n");
    	result = image_compare("backup", "/backup", 0);
        if(result) {
    	    ui->Print("Failed(%d)\n", result);
    		goto update_error;
    	}
    }

#if 1
    ui->ShowProgress(0.1, 6);
	ui->Print("Update recovery.img...\n");
	result = write_image("recovery", "/recovery", 0);
    if(result) {
	    ui->Print("Failed(%d)\n", result);
		goto update_error;
	}

    ui->ShowProgress(0.1, 6);
	ui->Print("Check recovery...\n");
	result = image_compare("recovery", "/recovery", 0);
    if(result) {
	    ui->Print("Failed(%d)\n", result);
		goto update_error;
	}
#endif

#if 1
    // execute the addition script of "update-script"
	pItem = (RKIMAGE_ITEM*)FindItem(hdr, RK_UPDATE_SCRIPT);
	if(pItem != NULL) {
	     //register the script command
		RegisterBuiltins();
    	RegisterInstallRKimageFunctions();
		RegisterInstallFunctions();
   		FinishRegistration();

		result = handle_update_script(update_file, pItem);
		if(result == -1) {
		    ui->Print("handle script error! ignore...\n");
		}else {
			ui->Print("handle script success!\n");
		}
	}
#endif

#if 1
	ui->SetProgress(0.9);
// Write to misc command, makes the loader after launching the upgrade their own, new bootloader data in order to behind
    ui->Print("copy Bootloader...\n");
    struct bootloader_message boot;
	memset(&boot, 0, sizeof(boot));
	sprintf(boot.command, "bootloader");
	sprintf(boot.recovery, "update-bootloader");
	result = set_bootloader_message(&boot);
	if(result) goto update_error;

	result = write_loader("bootloader", "/misc", 3*16*1024);
    if( result ) {
	    ui->Print("Failed(%d)\n", result);
		goto update_error;
	}
// when install rkimage, rk29 can be to update loader,misc must be written before install rkimage finish 
    bIfUpdateLoader = true;
	
#endif

#ifdef USE_BOARD_ID
    if(gIfBoardIdCustom) {
        ensure_path_mounted("/cust");
        ensure_path_mounted("/system");
        custom();
    }
#endif

	ui->SetProgress(1.0);
    ui->Print("Installation complete.\n");
    
	return 0;

update_error:
    ui->Print("Update failed, please reboot and update again!\n");
    return -1;
#endif
}

// root_path - BACKUP partition name
// cmy: only update from backup
/*
 * All kinds of case analysis:
 *    1 No problem                        0
 *    2 have problem,but can update continue
 *    3 have problem, can not update continue
          a Restart after the system can be normal operation    1
          b Restart after the system does not work normally (keep order, after the reset back up)   -1
 */
int recover_backup(const char *root_path)
{
// find backup partition
// crc check
// recover system.img
// wipe cache and userdata
//psע: Normal use, the user should can't through the other tools to update parameter/kernel/boot/recovery alone
//  if Can enter the kernel parameters that normal, bootloader can able to inspection/reduction kernel/boot/recovery
//  so we only recover system.img, and wipe cache/userdata
	int result = 0;
	ui->SetBackground(RecoveryUI::INSTALLING_UPDATE);

    ui->Print("=== RECOVER ===\n");

    size_t write_size;

    	Volume* v = volume_for_path(root_path);
	const MtdPartition* partition= mtd_find_partition_by_name(v->blk_device);
    	if (partition == NULL) {
        	LOGE("Can't find %s\n", root_path);
        	return 1;
    	}
    
    	g_src_isFile = false;

	// crc check
    char mtddevname[32]="";
	sprintf(mtddevname, "/dev/mtd/mtd%d", mtd_get_partition_index((MtdPartition*)partition));

	char data[2048] = "\0";
    	RKIMAGE_HDR *hdr = &g_imagehdr;//(RKIMAGE_HDR*)data;

	ui->Print("Checking firmware...\n");

	result = CheckImageFile(mtddevname, hdr);
	if( result )
		return 1;

    	strcpy(g_package_root_path, root_path);
    	strcpy(g_package_target, mtddevname);


#if 1
// recover form backup patition
	ui->Print("=== Default recover ===\n");

    	ui->Print("!!! Please KEEP your USB cable or DC-in connected !!!\n");
    
	ui->Print("Restore system...\n");
	result = write_image("system", "/system", 0);
    if( result )
   	{
	    ui->Print("Failed(%d)\n", result);
		goto recover_error;
   	}

	ui->Print("Check system...\n");
	result = image_compare("system", "/system", 0);
    if( result )
	{
	    ui->Print("Failed(%d)\n", result);
		goto recover_error;
	}


    ui->Print("Installation complete.\n");
	return 0;

recover_error:
    ui->Print("Recover failed, please reboot and recover again!\n");
    return -1;
#endif    
}

