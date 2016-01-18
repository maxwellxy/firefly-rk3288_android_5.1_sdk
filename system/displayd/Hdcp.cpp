#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <termio.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <cutils/misc.h>
#define LOG_TAG "HDCP"

#include <cutils/log.h>
#include "Hdcp.h"

#define RKNAND_GET_SN_SECTOR       _IOW('d', 3, unsigned int) 
#define SN_SECTOR_OP_TAG            0x41444E53 // "SNDA"

#define RKNAND_SYS_STORGAE_DATA_LEN 512

#define ERROR	ALOGE
typedef		unsigned short	    uint16;
typedef		unsigned int	    uint32;
typedef		unsigned char	    uint8;

typedef struct tagRKNAND_SYS_STORGAE
{
    uint32  tag;
    uint32  len;
    uint8   data[RKNAND_SYS_STORGAE_DATA_LEN];
}RKNAND_SYS_STORGAE;

#ifdef __cplusplus
extern "C" {
#endif 
extern int init_module(void *, unsigned long, const char *);
extern int delete_module(const char *, unsigned int);
#ifdef __cplusplus
}
#endif
void rknand_print_hex_data(uint32 * buf,uint32 len)
{
	uint32 i,j,count;
	
	for(i=0;i<len;i+=4)
		ALOGD("%08x %08x %08x %08x",buf[i],buf[i+1],buf[i+2],buf[i+3]);
}

static int hdcp_enable = 0;

int hdcp_read_key_from_idb(char **buf)
{
    uint32 i;
    int ret ;
	uint8 *data;
    RKNAND_SYS_STORGAE sysData;
    
    ALOGD("hdcp_read_idb start\n");
    *buf = NULL;
    
    int sys_fd = open("/dev/rknand_sys_storage",O_RDWR,0);
    if(sys_fd < 0){
        ALOGE("rknand_sys_storage open fail\n");
        return -1;
    }
    memset(&sysData, 0, sizeof(RKNAND_SYS_STORGAE));
    //sn
    sysData.tag = SN_SECTOR_OP_TAG;
    sysData.len = RKNAND_SYS_STORGAE_DATA_LEN;
    ret = ioctl(sys_fd, RKNAND_GET_SN_SECTOR, &sysData);
	//rknand_print_hex_data((uint32*)sysData.data,RKNAND_SYS_STORGAE_DATA_LEN);
    if(ret){
        ALOGE("get sn error %d\n", errno);
        return -1;
    }

	data = sysData.data + HDCP_KEY_IDB_OFFSET;
	if (data[0] == 0 &&
	    data[1] == 0 &&
	    data[2] == 0 &&
	    data[3] == 0 &&
	    data[4] == 0) {
	    	ALOGE("It is not a valid HDCP Key, We disable HDCP\n");
	    	return -1;
	}
	#if HDCP_KEY_ENCRYPTED
	// If HDCP key is encrypted, add decryption at here.
	// Decryption Code:
	
	#else
	*buf = (char*) malloc(HDCP_KEY_SIZE + HDCP_SEED_SIZE);
	if (*buf) {
		memcpy(*buf, sysData.data + HDCP_KEY_IDB_OFFSET, HDCP_KEY_SIZE + HDCP_SEED_SIZE);
	} else {
		ALOGE("malloc error\n");
		return -1;
	}
#endif
    return 0;
}

static int firmware_loading_enable(void)
{
	FILE *fd = NULL;
	
	fd = fopen(HDCP_FIRMWARE_LOADING, "w");
	if(fd == NULL)
		return -1;
		
	fputc('1', fd);
	fclose(fd);
	
	return 0;
}

static int firmware_loading_disable(void)
{
	FILE *fd = NULL;
	
	fd = fopen(HDCP_FIRMWARE_LOADING, "w");
	if(fd == NULL)
		return -1;
		
	fputc('0', fd);
	fclose(fd);
	
	return 0;
}

static int firmware_loading_cancel(void)
{
	FILE *fd = NULL;
	
	fd = fopen(HDCP_FIRMWARE_LOADING, "w");
	if(fd == NULL)
		return -1;
		
	fputs("-1", fd);
	fclose(fd);
	
	return 0;
}

static int hdcp_read_key(char **buf, int* size)
{
#ifdef HDCP_KEY_READ_FROM_FILE
	FILE *fd = NULL;	
	fd = fopen(KEY_FILE, "r");
	if(fd == NULL) {
		*buf = NULL;
		*size = 0;
		ALOGE("[%s] cant not open file %s\n", __FUNCTION__, KEY_FILE);
		return -1;
	}
	fseek(fd, 0, SEEK_END);
	*size = ftell(fd);
	
	*buf = (char*) malloc(*size);
	if(*buf) {
		fseek(fd, 0, SEEK_SET);
		fread(*buf, 1, *size, fd);
	}
	
	fclose(fd);
#else
	#ifdef HDCP_KEY_READ_FROM_IDB
	if(hdcp_read_key_from_idb(buf))
	{
		*buf = NULL;
		*size = 0;
	}
	else
		*size = HDCP_KEY_SIZE + HDCP_SEED_SIZE;
	#endif
#endif
	return 0;
}

static int hdcp_read_srm(char **buf, int* size)
{
	FILE *fd = NULL;
	int invalid_key_size, filesize;
	int temp, vrl_length, i;
	unsigned char data, generation;
	
	fd = fopen(SRM_FILE, "r");
	if(fd == NULL) {
		ALOGE("[%s] can not open file %s\n", __FUNCTION__, SRM_FILE);
		goto failed;
	}
	
	fseek(fd, 0, SEEK_END);
	filesize = ftell(fd);
	ALOGD("[%s] SRM file size is %d", __FUNCTION__, filesize);
	fseek(fd, 0, SEEK_SET);	
	*buf = (char*) malloc(filesize);
	if(*buf == NULL) {
		ALOGE("[%s] no enough memory to malloc srm data.\n", __FUNCTION__);
		goto failed;
	}
	memset(*buf, 0, filesize);
	
	temp = 0;
	fread(&temp, 1, 2, fd);
	if(temp != 0x0080) {
		ALOGE("[%s] invalid srm file %s\n", __FUNCTION__, SRM_FILE);
		goto invalid;
	}
	
	temp = 0;
	fread(&temp, 1, 2, fd);
	ALOGD("[%s] SRM Version 0x%04x", __FUNCTION__, data);
	
	data = 0;
	fread(&generation, 1, 1, fd);
	ALOGD("[%s] SRM Generation %d", __FUNCTION__, generation);
	
	//VRL Length
	vrl_length = 0;
	for(i = 0; i < 3; i++)
	{
		data = 0;
		fread(&data, 1, 1, fd);
		vrl_length |= data << ((2 - i) * 8);
	}	
	vrl_length -= 40 + 3;	
	ALOGD("[%s] SRM 1st block vrl size is %d", __FUNCTION__, vrl_length);
	if(!vrl_length) {
		ALOGW("[%s] There is no invalid key in SRM file.\n", __FUNCTION__);
		goto invalid;
	}
	invalid_key_size = vrl_length - 1;
	
	data = 0;
	fread(&data, 1, 1, fd);
	ALOGD("[%s] SRM 1st block invalid key number is %d", __FUNCTION__, data);
	if((invalid_key_size/5) != data) {
		ALOGE("[%s] invalid key number incorrect.\n", __FUNCTION__);
		goto invalid;
	}
	fread(*buf, 1, invalid_key_size , fd);
	fseek(fd, 40, SEEK_CUR);
	
	temp = ftell(fd);
	while(temp < filesize)
	{
		//VRL Length
		vrl_length = 0;
		for(i = 0; i < 2; i++)
		{
			data = 0;
			fread(&data, 1, 1, fd);
			vrl_length |= data << ((1 - i) * 8);
		}
		if(vrl_length > 42) {
			vrl_length -= 42;
			data = 0;
			fread(&data, 1, 1, fd);
			if(data*5 != vrl_length - 1) {
				ALOGE("[%s] invalid key number incorrect.\n", __FUNCTION__);
				goto invalid;
			}
			ALOGD("[%s] SRM next block invalid key number is %d", __FUNCTION__, data);
			fread(*buf, 1, data * 5, fd);
			invalid_key_size += data*5;
			fseek(fd, 40, SEEK_CUR);
		}
		temp = ftell(fd);
	}
	
	fclose(fd);
	*size = invalid_key_size;
	return 0;
	
invalid:
	if(*buf) free(*buf);
	fclose(fd);
failed:
	*buf = NULL;
	*size = 0;
	return -1;
}

static int hdcp_write_fireware(char *key, int keysize, char *srm, int srmsize)
{
	FILE *fd = NULL;
	
	if(key == NULL || keysize == 0)
		return -1;
	
	fd = fopen(HDCP_FIRMWARE_DATA, "w");
	if(fd == NULL) 
		return -1;
	
	fwrite(key, 1, keysize, fd);
	if(srmsize && srm) {
		fwrite(srm, 1, srmsize, fd);
	}
	
	fclose(fd);
	return 0;
}

/* 	Load hdcp key and srm file to kernel. Key and SRM are combined with following mode:
	struct firmware {
		char	key[keysize];
		char	srm[srmsize];
	}
 */
static int hdcp_load_firmware(void)
{
	char *key = NULL, *srm = NULL;
	int keysize = 0, srmsize, rc = -1, i;
	
	ALOGD("%s start\n", __FUNCTION__);
		
	hdcp_read_key(&key, &keysize);
	hdcp_read_srm(&srm, &srmsize);
	if(keysize) {
		rc = firmware_loading_enable();
		if(!rc) {
			hdcp_write_fireware(key, keysize, srm, srmsize);
			rc = firmware_loading_disable();
		}
	}
	if(key)	free(key);
	if(srm) free(srm);
		
	return rc;
}

static void hdcp_set_trytimes(int times)
{
	FILE *fd = NULL;
	char  buf[5];
	
	fd = fopen(HDCP_TRYTIMES, "w");
	if(fd == NULL)	return;
	
	memset(buf, 0, 5);
	sprintf(buf, "%d", times);	
	fputs(buf, fd);
	fclose(fd);
}

void Hdcp_enable(void)
{
	FILE *fd = NULL;

	if (!hdcp_enable)
		return;
	ALOGD("%s", __func__);
	fd = fopen(HDCP_ENABLE, "w");
	if(fd == NULL)	return;
		
	fputc('1', fd);
	fclose(fd);
}

static int insmod(const char *filename)
{
	void *module = NULL;
	unsigned int size;
	int ret;
	struct utsname name;
	char filename_release[PATH_MAX];

	ALOGD("insmod %s", filename);
	memset(&name, 0, sizeof(name));
	ret = uname(&name);
	if (ret == 0 && name.release) {
		// try insmod filename.x.x.x 
		strncat(filename_release, filename, sizeof(filename_release) - 1);
		strncat(filename_release, ".", sizeof(filename_release) - 1);
		strncat(filename_release, name.release, sizeof(filename_release) - 1);
		module = load_file(filename_release, &size);
	}
	if (!module)
		module = load_file(filename, &size);
	if (!module)
		return -1;
	ret = init_module(module, size, "");
	free(module);
	return ret;
}

static int rmmod(const char *modname)
{
	int ret = -1;
	int maxtry = 10;

	while (maxtry-- > 0)
	{
		ret = delete_module(modname, O_NONBLOCK | O_EXCL);
		if (ret < 0 && errno == EAGAIN)
			usleep(500000);
		else
			break;
	}
	if (ret != 0)
		SLOGE("Unable to unload driver module \"%s\": %s\n", modname, strerror(errno));
	return ret;
}

static void hdcp2_init(void)
{
	ALOGI("HDCP2 starting");
	if (access(HDCP2_MODULE, R_OK) == 0 &&
	    access(HDCP2_FW_NAME, R_OK) == 0) {
	    	if (insmod(HDCP2_MODULE) == 0)
	    		usleep(150000);
			rk_hdmi_hdcp2_init(HDCP2_FW_NAME, NULL);
	}
}

void Hdcp_init()
{
	
	ALOGI("HDCP starting");
	hdcp_enable = 0;
	if (access(HDCP_ENABLE, W_OK) == 0) {
		if( (access(HDCP_FIRMWARE_LOADING, W_OK) == 0) &&
			(access(HDCP_FIRMWARE_DATA, W_OK) == 0) ) {
			if (access("/drmboot.ko", R_OK) == 0) {
				insmod("/drmboot.ko");
				usleep(150000);
				if (hdcp_load_firmware() == 0) {
					hdcp_set_trytimes(HDCP_AUTH_RETRY_TIMES);
					hdcp2_init();
					hdcp_enable = 1;
				} else
					firmware_loading_cancel();
			} else
				firmware_loading_cancel();
		}
		else
			ALOGW("Device not support load HDCP firmware, just exit.\n");
	}
	else
		ALOGW("Device not support HDCP, just exit.\n");
	
    ALOGI("HDCP exiting");
}