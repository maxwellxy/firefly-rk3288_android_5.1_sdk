#ifndef _RKIMAGE_H_
#define _RKIMAGE_H_

#include "minzip/Zip.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PART_NAME				32

#define RELATIVE_PATH				64

#define MAX_PARTS				20

#define MAX_MACHINE_MODEL		64

//#define MAX_MANUFACTURER		64
#define MAX_MANUFACTURER		60

#define MAX_PACKAGE_FILES			16

typedef struct tagRKIMAGE_ITEM
{
	char name[PART_NAME];// �������
	char file[RELATIVE_PATH];// ���·������ȡ�ļ�ʱ�õ�
	unsigned int offset;// �ļ���Image�е�ƫ��
	unsigned int flash_offset;// ��д��Flash�е�λ��(��sectorΪ��λ)
	unsigned int usespace;// �ļ�ռ�ÿռ䣨��PAGE����)
	unsigned int size;// �ֽ���ʵ���ļ���С
}RKIMAGE_ITEM;

typedef struct tagRKIMAGE_HDR
{
	unsigned int tag;
	unsigned int size;// �ļ���С������ĩβ��CRCУ����
	char machine_model[MAX_MACHINE_MODEL];
	char manufacturer[MAX_MANUFACTURER];
	unsigned int version;
	int item_count;
	RKIMAGE_ITEM item[MAX_PACKAGE_FILES];
}RKIMAGE_HDR;

//copy from updater.h
typedef struct {
    FILE* cmd_pipe;
    ZipArchive* package_zip;
    int version;
} UpdaterInfo;

#define RKIMAGE_TAG				0x46414B52

//#define PAGESIZE				2048

#define BOOTLOADER				"Rock28Boot(L).bin"
#define PARTNAME_BOOTLOADER		"bootloader"

#define RK_UPDATE_SCRIPT			"update-script"
#define RK_RECOVER_SCRIPT			"recover-script"


int install_rkimage(const char* update_file);
int write_image_from_file(const char* src, const char* dest, int woffset);
int recover_backup(const char *root_path);
int check_sdboot(void);
int check_usbboot(void);

#ifdef __cplusplus
}
#endif

#endif
