#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <signal.h>
#include <sys/wait.h>
#include <cutils/properties.h>

#define LOG_TAG "Displaymanager"
#include <cutils/log.h>


#include <sysutils/SocketClient.h>
#include "Config.h"
#include "DisplayManager.h"
#include "ResponseCode.h"

#define DISPLAY_SYSFS_NODE	"/sys/class/display/"
#define DISPLAY_TYPE_LCD	"LCD"
#define DISPLAY_TYPE_HDMI	"HDMI"
#define DISPLAY_TYPE_VGA	"VGA"
#define DISPLAY_TYPE_YPbPr	"YPbPr"
#define DISPLAY_TYPE_TV		"TV"

#define DISPLAY_CONFIG_FILE			"/cache/display.cfg"

#define CVBS_MODE_PAL	"720x576i-50"
#define CVBS_MODE_NTSC	"720x480i-60"
enum {
	DISPLAY_INTERFACE_TV = 1,
	DISPLAY_INTERFACE_YPbPr,
	DISPLAY_INTERFACE_VGA,
	DISPLAY_INTERFACE_HDMI,
	DISPLAY_INTERFACE_LCD
};

enum {
	DISPLAY_OPERATE_READ = 0,
	DISPLAY_OPERATE_WRITE
};

#define HDMI_VIDEO_YUV420	(4 << 8)

DisplayManager::DisplayManager() {

	ALOGD("[%s] VER 3.0", __FUNCTION__);
	powerup = 0;
	#if (DISPLAY_UNTIL_WAKEUP == 0)
	init();
	#endif
	ALOGD("[%s] success", __FUNCTION__);
}

const char *DisplayManager::type2string(int type) {

	switch(type)
	{
		case DISPLAY_INTERFACE_TV:
			return DISPLAY_TYPE_TV;
		case DISPLAY_INTERFACE_YPbPr:
			return DISPLAY_TYPE_YPbPr;
		case DISPLAY_INTERFACE_VGA:
			return DISPLAY_TYPE_VGA;
		case DISPLAY_INTERFACE_HDMI:
			return DISPLAY_TYPE_HDMI;
		case DISPLAY_INTERFACE_LCD:
			return DISPLAY_TYPE_LCD;
	}
	return NULL;
}

int DisplayManager::string2type(const char *str) {
	if(!strcmp(str, DISPLAY_TYPE_TV))
		return DISPLAY_INTERFACE_TV;
	else if(!strcmp(str, DISPLAY_TYPE_YPbPr))
		return DISPLAY_INTERFACE_YPbPr;
	else if(!strcmp(str, DISPLAY_TYPE_VGA))
		return DISPLAY_INTERFACE_VGA;
	else if(!strcmp(str, DISPLAY_TYPE_HDMI))
		return DISPLAY_INTERFACE_HDMI;
	else if(!strcmp(str, DISPLAY_TYPE_LCD))
		return DISPLAY_INTERFACE_LCD;
	else
		return 0;
}

void DisplayManager::init() {
	int rc, found = 0, i, enable;
	struct displaynode *head, *node;

	if(powerup)	return;
	powerup = 1;
	main_display_list = NULL;
	aux_display_list = NULL;
	readSysfs();
	rc = readConfig();
	if(rc) {
		ALOGI("Read CFG File error");
		int enabled = 0;
		for(node = main_display_list; node != NULL; node = node->next) {
			if(enabled && node->enable)
				node->enable = 0;
			if(node->connect && !enabled) {
				node->enable = 1;
				enabled = 1;
			}
		}
	}
	for(node = main_display_list; node != NULL; node = node->next) {
		operateIfaceMode(node, DISPLAY_OPERATE_WRITE, node->mode);
		if(node->enable == 1) {
		//	operateIfaceMode(node, DISPLAY_OPERATE_WRITE, node->mode);
			found = 1;
		}
		// HDMI device is always enabled
		if (node->type == DISPLAY_INTERFACE_HDMI)
			node->enable = 1;
		//#ifdef DISPLAY_POLICY_BOX
		//operateIfaceEnable(node, DISPLAY_OPERATE_WRITE);
		//#endif
		updatesinkaudioinfo(node);
	}
	ALOGD("main display enabled interface found %d", found);
	if(!found) {
		//If no display interface is enabled, enable connected interface.
		for(node = main_display_list; node != NULL; node = node->next) {
			readIfaceConnect(node);
			if(node->connect) {
				operateIfaceMode(node, DISPLAY_OPERATE_WRITE, node->mode);
				node->enable = 1;
				#ifdef DISPLAY_POLICY_BOX
				operateIfaceEnable(node, DISPLAY_OPERATE_WRITE);
				#endif
				updatesinkaudioinfo(node);
				break;
			}
		}
		// If no interface is connected, enable first interface.
		if(node == NULL && main_display_list) {
			ALOGD("main display no interface is connected");
			operateIfaceMode(main_display_list, DISPLAY_OPERATE_WRITE, main_display_list->mode);
			main_display_list->enable = 1;
			#ifdef DISPLAY_POLICY_BOX
			operateIfaceEnable(main_display_list, DISPLAY_OPERATE_WRITE);
			#endif
			updatesinkaudioinfo(main_display_list);
		}

	} else {
		for(node = main_display_list; node != NULL; node = node->next) {
			#ifdef DISPLAY_POLICY_BOX
			operateIfaceEnable(node, DISPLAY_OPERATE_WRITE);
			#endif
		}
	}
	found = 0;
	for(node = aux_display_list; node != NULL; node = node->next) {
		if(node->enable == 1) {
			operateIfaceMode(node, DISPLAY_OPERATE_WRITE, node->mode);
			found = 1;
		}
		// HDMI device is always enabled
		if (node->type == DISPLAY_INTERFACE_HDMI)
			node->enable = 1;
		//operateIfaceEnable(node, DISPLAY_OPERATE_WRITE);
	}
	ALOGD("aux display enabled interface found %d", found);
	if(!found) {
		//If no display interface is enabled, enable connected interface.
		for(node = aux_display_list; node != NULL; node = node->next) {
			readIfaceConnect(node);
			if(node->connect) {
				operateIfaceMode(node, DISPLAY_OPERATE_WRITE, node->mode);
				node->enable = 1;
				operateIfaceEnable(node, DISPLAY_OPERATE_WRITE);
				break;
			}
		}
		// If no interface is connected, enable first interface.
		if(node == NULL && aux_display_list) {
			ALOGD("aux display no interface is connected");
			operateIfaceMode(aux_display_list, DISPLAY_OPERATE_WRITE, aux_display_list->mode);
			aux_display_list->enable = 1;
			operateIfaceEnable(aux_display_list, DISPLAY_OPERATE_WRITE);
		}

	} else{
		for(node = aux_display_list; node != NULL; node = node->next) {
			operateIfaceEnable(node, DISPLAY_OPERATE_WRITE);
		}
	}
}

void DisplayManager::led_ctrl(struct displaynode *node) {
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];

	memset(buf, 0, BUFFER_LENGTH);
	sprintf(buf, "/sys/class/leds/%s/brightness", node->name);
	fd = fopen(buf, "w");
	if(fd == NULL) {
		ALOGW("interface %d not support led\n", node->type);
		return;
	}

	if(node->enable)
		fputc('1', fd);
	else
		fputc('0', fd);

	fclose(fd);
}

int DisplayManager::operateIfaceEnable(struct displaynode *node, int operate) {
	FILE *fd = NULL;
	char *buf = NULL;

	buf = (char*)malloc(BUFFER_LENGTH);
	if(buf == NULL)
		return -1;

	// Read enable;
	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, node->path);
	strcat(buf, "/enable");
	if(operate == DISPLAY_OPERATE_READ) {
		fd = fopen(buf, "r");
		memset(buf, 0, BUFFER_LENGTH);
		fgets(buf, BUFFER_LENGTH, fd);
		fclose(fd);
		node->enable = atoi(buf);
	}else if(operate == DISPLAY_OPERATE_WRITE) {
		ALOGD("[%s] property %d iface %d operate %d value %d", __FUNCTION__, node->property, node->type, operate, node->enable);
		fd = fopen(buf, "w");
		if(node->enable == 1)
			fputc('1', fd);
		else if(node->enable == 0)
			fputc('0', fd);
		fclose(fd);
//		if(node->enable == 1)	saveConfig();
	}
	free(buf);
	led_ctrl(node);
	return 0;
}

int	DisplayManager::readIfaceConnect(struct displaynode *node) {
	FILE *fd = NULL;
	char *buf = NULL;
	int connect;

	if(node == NULL)
		return -1;

	buf = (char*)malloc(BUFFER_LENGTH);
	if(buf == NULL)
		return -1;

	// Read enable;
	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, node->path);
	strcat(buf, "/connect");
	fd = fopen(buf, "r");
	memset(buf, 0, BUFFER_LENGTH);
	fgets(buf, BUFFER_LENGTH, fd);
	fclose(fd);
	node->connect = atoi(buf);
	free(buf);
	return node->connect;
}

int	DisplayManager::operateIfaceMode(struct displaynode *node, int type, char *mode){
	FILE *fd = NULL;
	char *buf = NULL;
	int	modelen = 0;

	if(node == NULL || mode == NULL)
		return -1;

	buf = (char*)malloc(BUFFER_LENGTH);
	if(buf == NULL)
		return -1;

	if(type == DISPLAY_OPERATE_READ) {
		memset(buf, 0, BUFFER_LENGTH);
		strcpy(buf, node->path);
		strcat(buf, "/mode");

		fd = fopen(buf, "r");
		memset(buf, 0, BUFFER_LENGTH);
		fgets(buf, MODE_LENGTH, fd);
		fclose(fd);

		//There is a '\n' at last char, should be delelted.
		modelen = strlen(buf);
		if(modelen)
		{
			buf[modelen -1] = 0;
			memset(mode, 0, MODE_LENGTH);
			memcpy(mode, buf, modelen -1);
		}
	}else if(type == DISPLAY_OPERATE_WRITE) {
		ALOGD("[%s] property %d iface %d type %d mode %s\n", __FUNCTION__, node->property, node->type, type, mode);
		// Check the mode inputted is exist in mode list.
		memset(buf, 0, BUFFER_LENGTH);
		strcpy(buf, node->path);
		strcat(buf, "/modes");
		fd = fopen(buf, "r");

		int exist = 0;
		memset(buf, 0, BUFFER_LENGTH);
		while(fgets(buf, BUFFER_LENGTH, fd) != NULL) {
			modelen = strlen(buf);
			if(!modelen) continue;
			//There is a '\n' at last char, should be delelted.
			buf[modelen - 1] = 0;
			if(strcmp(buf, mode) == 0) {
				exist = 1;
				break;
			}
		}
		// If input mode is not exist, just return.
		if(!exist) {
			ALOGV("[%s] Input mode is not exist in mode list, read current mode.", __FUNCTION__);
			if(buf)
				free(buf);
			return 0;
			//operateIfaceMode(node, DISPLAY_OPERATE_READ, mode);
		}
		// Set the mode.
		memset(buf, 0, BUFFER_LENGTH);
		strcpy(buf, node->path);
		strcat(buf, "/mode");
		fd = fopen(buf, "w");
		memset(buf, 0, BUFFER_LENGTH);
		strcpy(buf, mode);
		strcat(buf, "\n");
		fputs(buf, fd);
		fclose(fd);
		if(mode != node->mode) {
			memset(node->mode, 0 ,MODE_LENGTH);
			strcpy(node->mode, mode);
		}
	}

	free(buf);
	return 0;
}

void DisplayManager::display_list_add(struct displaynode *node)
{
	struct displaynode *head = NULL, *pos;

	ALOGD("[%s] display %d iface %s connect %d enable %d mode %s\n", __FUNCTION__,
			node->property, type2string(node->type), node->connect, node->enable, node->mode);

	if(node->property == MAIN_DISPLAY) {
		if(main_display_list == NULL)
			main_display_list = node;
		else
			head = main_display_list;
	}
	else {
		if(aux_display_list == NULL)
			aux_display_list = node;
		else
			head = aux_display_list;
	}
	if(head == NULL) {
		return;
	}
	// input node priority is higher than nodes in list
	if(node->type > head->type)
	{
		node->next = head;
		head->prev = node;
		if(node->property == MAIN_DISPLAY)
			main_display_list = node;
		else
			aux_display_list = node;
		return;
	}

	// input node priority is lower than first node
	for(pos = head; pos->next != NULL; pos = pos->next)
	{
		if(pos->next->type < node->type )
			break;
	}
	node->next = pos->next;
	pos->next = node;
}

int DisplayManager::readSysfs(void) {
	FILE *fd = NULL;
	DIR *dir = NULL;
	struct dirent * dirent = NULL;
	char buf[BUFFER_LENGTH];
	int i, rc;
	struct displaynode *node;

	dir = opendir(DISPLAY_SYSFS_NODE);
	if(dir == NULL)
	{
		ALOGE("[%s] Cannot open sysfs display node", __FUNCTION__);
		return -1;
	}

	while((dirent = readdir(dir)) != NULL)
	{
		if(!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;

		node = (struct displaynode*)malloc(sizeof(struct displaynode));
		if(node == NULL) {
			ALOGE("[%s] Cannot malloc memory for display node", __FUNCTION__);
			return -1;
		}
		memset(node, 0, sizeof(struct displaynode));
		// Get node path
		strcpy(node->path, DISPLAY_SYSFS_NODE);
		strcat(node->path, dirent->d_name);

		// Read node type
		memset(buf, 0, BUFFER_LENGTH);
		strcpy(buf, node->path);
		strcat(buf, "/type");
		fd = fopen(buf, "r");
		if(NULL!=fd){
		memset(buf, 0, BUFFER_LENGTH);
		fgets(buf, BUFFER_LENGTH, fd);
		fclose(fd);
		//There is a '\n' at last char, should be delelted.
		if(strlen(buf)) {
			buf[strlen(buf)-1] = 0;
		}

		if(!strcmp(buf, DISPLAY_TYPE_TV))
			node->type = DISPLAY_INTERFACE_TV;
		else if(!strcmp(buf, DISPLAY_TYPE_YPbPr))
			node->type = DISPLAY_INTERFACE_YPbPr;
		else if(!strcmp(buf, DISPLAY_TYPE_VGA))
			node->type = DISPLAY_INTERFACE_VGA;
		else if(!strcmp(buf, DISPLAY_TYPE_HDMI))
			node->type = DISPLAY_INTERFACE_HDMI;
		else if(!strcmp(buf, DISPLAY_TYPE_LCD))
			node->type = DISPLAY_INTERFACE_LCD;
		}else{
			ALOGE("[%s] [errno=%s] %s", __FUNCTION__, strerror(errno), buf);
		}
		// Read node property;
		memset(buf, 0, BUFFER_LENGTH);
		strcpy(buf, node->path);
		strcat(buf, "/property");
		fd = fopen(buf, "r");
		if(NULL!=fd){
			memset(buf, 0, BUFFER_LENGTH);
			fgets(buf, BUFFER_LENGTH, fd);
			node->property = atoi(buf);
			fclose(fd);
		}else{
		  //node->property = 0;
			ALOGE("[%s] [errno=%s] %s", __FUNCTION__, strerror(errno), buf);
		}
		// Read node name
		memset(buf, 0, BUFFER_LENGTH);
		strcpy(buf, node->path);
		strcat(buf, "/name");
		fd = fopen(buf, "r");
		if(NULL!=fd){
			fgets(node->name, NAME_LENGTH, fd);
			if(node->name[strlen(node->name) - 1] == '\n')
				node->name[strlen(node->name) - 1] = 0;
			fclose(fd);
		}else{
			ALOGE("[%s] [errno=%s] %s", __FUNCTION__, strerror(errno), buf);
		}

		// Read enable;
		operateIfaceEnable(node, DISPLAY_OPERATE_READ);

		// Read connect
		readIfaceConnect(node);

		// When power up, hdmi is enabled whether it is connected or not.
		if(node->connect == 0 && (node->enable == 1) )
			node->enable = 0;

		// Read mode;
		operateIfaceMode(node, DISPLAY_OPERATE_READ, node->mode);

		display_list_add(node);

	}
	closedir(dir);

	return 0;
}

#define BASEPARAMER_FILE  "/dev/block/platform/1021c000.rksdmmc/by-name/baseparamer"
#define BASEPARAMER_FILE_EMMC32  "/dev/block/platform/ff0f0000.rksdmmc/by-name/baseparamer"
#define BASEPARAMER_FILE_NAND  "/dev/block/rknand_baseparamer"
struct HW_BASE_PARAMETER
{
       unsigned int xres;
       unsigned int yres;
       unsigned int interlaced;
       unsigned int type;
       unsigned int refresh;
       unsigned int reserve;
};

struct file_base_paramer
{
	struct HW_BASE_PARAMETER hdmi;
	struct HW_BASE_PARAMETER tve;
};

void DisplayManager::saveConfig(void) {
	FILE *fd = NULL;
	struct displaynode *node;
	char buf[BUFFER_LENGTH];
	struct file_base_paramer base_paramer;

	memset(&base_paramer,0,sizeof(base_paramer));

	fd = fopen(DISPLAY_CONFIG_FILE, "w");

	if(fd != NULL) {
		for(node = main_display_list; node != NULL; node = node->next) {
		       memset(buf, 0 , BUFFER_LENGTH);
		       sprintf(buf, "display=%d,iface=%d,enable=%d,mode=%s\n", node->property, node->type, node->enable, node->mode);
		       fwrite(buf, 1, strlen(buf), fd);
		}
		for(node = aux_display_list; node != NULL; node = node->next) {
		       memset(buf, 0 , BUFFER_LENGTH);
		       sprintf(buf, "display=%d,iface=%d,enable=%d,mode=%s\n", node->property, node->type, node->enable, node->mode);
		       fwrite(buf, 1, strlen(buf), fd);
		}
		fflush(fd);
		fclose(fd);
	}
	sync();
	
	int file;
	file = open(BASEPARAMER_FILE, O_RDWR);
	if (file < 0)
		file = open(BASEPARAMER_FILE_NAND, O_RDWR);
	if (file < 0)
		file = open(BASEPARAMER_FILE_EMMC32, O_RDWR);
	if (file < 0) {
		ALOGW("base paramter file can not be opened");
		return;
	}
	// caculate file's size and read it
	unsigned int length = lseek(file, 0L, SEEK_END);
	lseek(file, 0L, SEEK_SET);
	if(length < sizeof(base_paramer)) {
		ALOGE("BASEPARAME data's length is error\n");
		close(file);
		return;
	}

	read(file, (void*)&base_paramer, sizeof(file_base_paramer));

	for(node = main_display_list; node != NULL; node = node->next) {
		if(node->type == DISPLAY_INTERFACE_HDMI) {
			if(node->mode) {
				base_paramer.hdmi.type = node->type;
				base_paramer.hdmi.interlaced = 0;

				if (strstr(node->mode, "YCbCr420"))
					base_paramer.hdmi.interlaced |= HDMI_VIDEO_YUV420;

				if (strchr(node->mode, 'p')) {
					sscanf(node->mode, "%dx%dp-%d",
					       &base_paramer.hdmi.xres,
					       &base_paramer.hdmi.yres,
					       &base_paramer.hdmi.refresh);
				} else if (strchr(node->mode, 'i')) {
					sscanf(node->mode, "%dx%di-%d",
					       &base_paramer.hdmi.xres,
					       &base_paramer.hdmi.yres,
					       &base_paramer.hdmi.refresh);
					base_paramer.hdmi.interlaced |= 1;
				} else if (strcmp(node->mode,"auto") == 0) {
					base_paramer.hdmi.xres = 0;
					base_paramer.hdmi.yres = 0;
					base_paramer.hdmi.refresh = 0;
				}
				ALOGD("[%s] hdmi_mode %s\n", __FUNCTION__,node->mode);
			}
		} else if(node->type == DISPLAY_INTERFACE_TV) {
			if(node->mode) {
				sscanf(node->mode, "%dx%di-%d",
				       &base_paramer.tve.xres,
				       &base_paramer.tve.yres,
				       &base_paramer.tve.refresh);
				base_paramer.tve.interlaced = 1;
				ALOGD("[%s] tve_mode  %s\n", __FUNCTION__,node->mode);
			}
		}
	}
	lseek(file, 0L, SEEK_SET);
	write(file, (char*)(&base_paramer), sizeof(base_paramer));
	close(file);
	sync();
	
	ALOGD("[%s] hdmi:%d,%d,%d,%d,%d,%d\n", __FUNCTION__,
	base_paramer.hdmi.xres,
	base_paramer.hdmi.yres,
	base_paramer.hdmi.interlaced,
	base_paramer.hdmi.type,
	base_paramer.hdmi.refresh,
	base_paramer.hdmi.reserve);
	
	ALOGD("[%s] tve:%d,%d,%d,%d,%d,%d\n", __FUNCTION__,
	base_paramer.tve.xres,
	base_paramer.tve.yres,
	base_paramer.tve.interlaced,
	base_paramer.tve.type,
	base_paramer.tve.refresh,
	base_paramer.tve.reserve);
}

int DisplayManager::readUbootConfig(char *hdmi_mode, char *tve_mode)
{
	FILE *file = NULL;
	struct displaynode *node;
	char buf[BUFFER_LENGTH];
        struct file_base_paramer base_paramer;
	file = fopen(BASEPARAMER_FILE, "r");
	if(file == NULL ) {
         file = fopen(BASEPARAMER_FILE_NAND, "r");
            if(file == NULL )
             file = fopen(BASEPARAMER_FILE_EMMC32, "r");
             if(file == NULL ) {
		ALOGE("%s not exist", BASEPARAMER_FILE_EMMC32);
		return -1;
             }
	}

	if(file != NULL) {
		// caculate file's size and read it
		fseek(file,0L,SEEK_END);
		unsigned int length = ftell(file);
		fseek(file,0L,SEEK_SET);
		if(length < sizeof(base_paramer))
		{
			ALOGE("getBASEPARAMERValue(), BASEPARAME data's length is error");
			fclose(file);
			file = NULL;
			return -1;
		}

		fread((void*)&base_paramer,sizeof(file_base_paramer),1,file);
		fclose(file);
		if (base_paramer.hdmi.xres &&
		    base_paramer.hdmi.yres &&
		    base_paramer.hdmi.refresh) {
			if(base_paramer.hdmi.interlaced== 0)
			sprintf(hdmi_mode, "%dx%dp-%d", base_paramer.hdmi.xres, base_paramer.hdmi.yres, base_paramer.hdmi.refresh);
			else if(base_paramer.hdmi.interlaced== 1)
			sprintf(hdmi_mode, "%dx%di-%d", base_paramer.hdmi.xres, base_paramer.hdmi.yres, base_paramer.hdmi.refresh);
		}
		if (base_paramer.tve.xres &&
		    base_paramer.tve.yres &&
		    base_paramer.tve.refresh)
			sprintf(tve_mode, "%dx%di-%d", base_paramer.tve.xres, base_paramer.tve.yres, base_paramer.tve.refresh);
        	//sprintf(mode, "%dx%dp-%d", base_paramer.hdmi.xres, base_paramer.hdmi.yres, base_paramer.hdmi.refresh);
        	ALOGD("[%s] hdmi_mode %s tv_mode %s\n", __FUNCTION__,hdmi_mode,tve_mode);
        }
	return 0;
}

int DisplayManager::readConfig(void) {
	FILE *fd = NULL;
	char *buf = (char*) malloc(BUFFER_LENGTH);
	char *ptr, *ptr_space;
	struct displaynode *node, *head;
	int rc = 0, i = 0, display, type, enable;
	int main_enabled = 0, aux_enabled = 0;
	int *enabled;

	if(buf == NULL) {
		ALOGE("setHDMIEnable no memeory malloc buf\n");
		return -1;
	}

	fd = fopen(DISPLAY_CONFIG_FILE, "r");
	if(fd == NULL ) {
		ALOGE("%s not exist", DISPLAY_CONFIG_FILE);
		free(buf);
		return -1;
	}

	memset(buf, 0 , BUFFER_LENGTH);
	while(fgets(buf, BUFFER_LENGTH, fd) != NULL) {
		ALOGD("read cfg: %s", buf);

		// Parse iface property
		ptr = buf;
		if(memcmp(ptr, "display=", 8)) {
			ALOGE("display error");
			rc = -1;
			break;
		}
		ptr += 8;
		ptr_space = strchr(ptr, ',');
		display = atoi(&ptr[0]);

		// Parse iface type.
		ptr = ptr_space + 1;
		if(memcmp(ptr, "iface=", 6)) {
			ALOGE("iface error");
			rc = -1;
			break;
		}
		ptr += 6;
		ptr_space = strchr(ptr, ',');
		type = atoi(&ptr[0]);
		// Check if this type exist in sysfs

		if(display == MAIN_DISPLAY) {
			head = main_display_list;
			enabled = &main_enabled;
		}
		else {
			head = aux_display_list;
			enabled = &aux_enabled;
		}
		for(node = head; node != NULL; node = node->next) {
			if(node->type == type)
				break;
			#if ENABLE_AUTO_SWITCH
			// Sometime display interface listed in sysfs is different in various kernel,
			// when new interface which is not exist in config file appeared, we should
			// check it is connected or not.
			if(*enabled == 0) {
				if(node->connect) {
					node->enable = 1;
					*enabled = 1;
				}
			}
			#endif
		}
		if(node == NULL) {
			ALOGI("[%s] kernel not support iface %d\n", __FUNCTION__, type);
			continue;
		}
		ptr = ptr_space + 1;

		// Parse enable value.
		if(memcmp(ptr, "enable=", 7)) {
			ALOGE("enable error");
			rc = -1;
			break;
		}
		ptr += 7;
		ptr_space = strchr(ptr, ',');
		enable = atoi(&ptr[0]);
		#if ENABLE_AUTO_SWITCH
		// If enable is true in configure file, check if iface is connected or not.
		// Because YPbPr and CVBS is output by RK1000, if mode enabled in configure
		// was YPbPr or CVBS, assume it is connected.
		if(node->type == DISPLAY_INTERFACE_YPbPr || node->type == DISPLAY_INTERFACE_TV) {
			if(*enabled)
				node->connect = 0;
			else if(enable)
				node->connect = 1;
			else
				node->connect = 0;
		}

		if(node->connect && *enabled == 0) {
			node->enable = 1;
			*enabled = 1;
		}
		else
			node->enable = 0;
//		ALOGD("auto switch enabled\n");
		#else
		node->enable = enable;
//		ALOGD("auto switch not enabled\n");
		#endif
		ptr = ptr_space + 1;

		// Parse iface display mode.
		if(memcmp(ptr, "mode=", 5)) {
			ALOGE("mode error");
			rc = -1;
			break;
		}
		ptr += 5;
		ptr_space = strchr(ptr, '\n');
		//make sure wether base_parameter be modified!
		char* hdmi_mode = (char*) malloc(BUFFER_LENGTH);
		char* tve_mode = (char*) malloc(BUFFER_LENGTH);
		memset(hdmi_mode, 0, BUFFER_LENGTH);
		memset(tve_mode, 0, BUFFER_LENGTH);
		if (display == 0)
			int success = readUbootConfig(hdmi_mode, tve_mode);
		if(strlen(hdmi_mode) && (node->type == DISPLAY_INTERFACE_HDMI)){
		        memset(node->mode, 0, MODE_LENGTH);
			memcpy(node->mode, hdmi_mode, MODE_LENGTH);
                       // ALOGD("[%s] iface %d basemode %s\n mode %s\n", __FUNCTION__,node->type,value,node->mode);
		}
		else if(strlen(tve_mode) && (node->type == DISPLAY_INTERFACE_TV)) {
		        memset(node->mode, 0, MODE_LENGTH);
			memcpy(node->mode, tve_mode, MODE_LENGTH);
                       // ALOGD("[%s] iface %d basemode %s\n mode %s\n", __FUNCTION__,node->type,value,node->mode);
		}
		else if(ptr_space > ptr) {
			memset(node->mode, 0, MODE_LENGTH);
			memcpy(node->mode, ptr, ptr_space - ptr);
		}
		ALOGD("[%s] display %d iface %d connect %d enable %d mode %s\n", __FUNCTION__,
			node->property, node->type, node->connect, node->enable, node->mode);
	}
	fclose(fd);
	free(buf);
	return 0;
}

void DisplayManager::getIfaceInfo(SocketClient *cli, int display)
{
	struct displaynode *head, *node;

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	if(head == NULL) {
		cli->sendMsg(ResponseCode::OperationFailed, "Missing display node", false);
	}
	else {
		for(node = head; node != NULL; node = node->next)
			cli->sendMsg(ResponseCode::InterfaceListResult, type2string(node->type), false);
		cli->sendMsg(ResponseCode::CommandOkay, "Interface list completed", false);
	}
}

void DisplayManager::getCurIface(SocketClient *cli, int display) {

	struct displaynode *head, *node;

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next)
	{
		readIfaceConnect(node);
		if(node->enable == 1 && node->connect == 1) {
			cli->sendMsg(ResponseCode::CommandOkay, type2string(node->type), false);
			return;
		}
	}
	cli->sendMsg(ResponseCode::CommandOkay, type2string(head->type), false);
}

int DisplayManager::enableIface(int display, char* iface, int enable) {
	struct displaynode *head, *node;
	int type = string2type(iface);

	if(!powerup)
		return -1;

	ALOGD("[%s] display %d iface %s enable %d", __FUNCTION__, display, iface, enable);

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next)
	{
		if(node->type == type)
		{
			node->enable = enable;
			operateIfaceEnable(node, DISPLAY_OPERATE_WRITE);
			//if(node->enable == 1)	saveConfig();
			break;
		}
	}
	return 0;
}

void DisplayManager::getModeList(SocketClient *cli, int display, char* iface) {
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];
	struct displaynode *head, *node;
	int type = string2type(iface);

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next)
	{
		if(node->type == type)
			break;
	}

	if(node == NULL) {
		cli->sendMsg(ResponseCode::CommandParameterError, "Missing iface", false);
		return;
	}

	// Read modelist;
	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, node->path);
	strcat(buf, "/modes");
	fd = fopen(buf, "r");

	memset(buf, 0, BUFFER_LENGTH);
	while(fgets(buf, BUFFER_LENGTH, fd) != NULL)
	{
		if(strlen(buf)) {
			ALOGD("%s", buf);
			buf[strlen(buf) - 1] = 0;
			cli->sendMsg(ResponseCode::ModeListResult, buf, false);
		}
		memset(buf, 0, BUFFER_LENGTH);
	}
	fclose(fd);
	cli->sendMsg(ResponseCode::CommandOkay, "Mode list completed", false);
}

void DisplayManager::getCurMode(SocketClient *cli, int display, char* iface) {
	struct displaynode *head, *node;
	FILE *fd = NULL;
	int modelen;
	char buf[BUFFER_LENGTH];
	int type = string2type(iface);

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next) {
		if(node->type == type)
			break;
	}

	if(node == NULL) {
		cli->sendMsg(ResponseCode::CommandParameterError, "Missing iface", false);
		return;
	}
	
	if(strlen(node->mode)) {
	    //ALOGD("[%s] %s %s %s %d %d", __FUNCTION__, node->path, node->mode, node->name,strlen(node->name),strcmp(node->name,"firefly_vga"));
	    if(strcmp(node->name,"firefly_vga") == 0) {
	        // Read modelist;
	        memset(buf, 0, BUFFER_LENGTH);
	        strcpy(buf, node->path);
	        strcat(buf, "/mode");
	        fd = fopen(buf, "r");

	        memset(buf, 0, BUFFER_LENGTH);
	        if(fgets(buf, BUFFER_LENGTH, fd) != NULL)
	        {
	        	modelen = strlen(buf);
		        if(modelen)
		        {
		            ALOGD("%s", buf);
			        buf[modelen -1] = 0;
			        memset(node->mode, 0, MODE_LENGTH);
			        memcpy(node->mode, buf, modelen -1);
		        }
	        }		
	        fclose(fd);
	    }
	}

//	if(!strlen(node->mode)) {
	operateIfaceMode(node, DISPLAY_OPERATE_READ, node->mode);
//	}
	cli->sendMsg(ResponseCode::CommandOkay, node->mode, false);

	return;
}

int DisplayManager::setMode(int display, char* iface, char *mode) {
	struct displaynode *head, *node;
	int type = string2type(iface);

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	ALOGD("[%s] display %d iface %s mode %s", __FUNCTION__, display, iface, mode);

	for(node = head; node != NULL; node = node->next) {
		if(node->type == type)
			break;
	}

	if(node == NULL)
		return -1;

	operateIfaceMode(node, DISPLAY_OPERATE_WRITE, mode);

	return 0;
}

void DisplayManager::setHDMIEnable(int display) {
	struct displaynode *head, *node, *iface_hdmi = NULL, *iface_enabled = NULL;
	int enable, count = 0;

	if(!powerup)
		return;

	ALOGD("[%s] display %d", __FUNCTION__, display);

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next) {
		if(node->type == DISPLAY_INTERFACE_HDMI) {
			iface_hdmi = node;
		}
		if(node->enable == 1 && node != iface_hdmi) {
			iface_enabled = node;
		}
		count++;
	}
	//Theres is only one interface in input screen, no need to auto switch.
	if (count == 1)
		return;
	readIfaceConnect(iface_hdmi);
	if(((iface_hdmi == iface_enabled || iface_enabled == NULL)) && iface_hdmi != NULL) {
		operateIfaceMode(iface_hdmi, DISPLAY_OPERATE_WRITE, iface_hdmi->mode);
		iface_hdmi->enable = 1;
		operateIfaceEnable(iface_hdmi, DISPLAY_OPERATE_WRITE);
	}
	else{
		if(ENABLE_AUTO_SWITCH && (iface_hdmi->type > iface_enabled->type))
		{
			iface_enabled->enable = 0;
			operateIfaceEnable(iface_enabled, DISPLAY_OPERATE_WRITE);
//			iface_hdmi->enable = 1;
			operateIfaceMode(iface_hdmi, DISPLAY_OPERATE_WRITE, iface_hdmi->mode);
			operateIfaceEnable(iface_hdmi, DISPLAY_OPERATE_WRITE);
		}
	}
	usleep(500000);
	updatesinkaudioinfo(iface_hdmi);
}

void DisplayManager::setHDMIDisable(int display) {
	struct displaynode *head, *node, *iface_hdmi = NULL, *iface_enabled = NULL;
	int count = 0;
	int i = 0;
	int connect = 0;

	if(!powerup)
		return;

	ALOGD("[%s] display %d", __FUNCTION__, display);

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next) {
		if(node->type == DISPLAY_INTERFACE_HDMI) {
			iface_hdmi = node;
		}
		if(node->enable == 1) {
			iface_enabled = node;
		}
		count++;
	}

	updatesinkaudioinfo(iface_hdmi);

	//Theres is only one interface in input screen, no need to auto switch.
	if(count == 1) return;

	for(i=0; i<30; i++)
	{
		usleep(10000);
		connect = readIfaceConnect(iface_hdmi);
		if(connect == 1)
			return;
	}

	if(iface_hdmi == iface_enabled && iface_hdmi != NULL) {
		if(ENABLE_AUTO_SWITCH) {
//			iface_hdmi->enable = 0;
//			operateIfaceEnable(iface_hdmi, DISPLAY_OPERATE_WRITE);
//			for(i=0; i<100; i++)
//			{
//				usleep(10000);
//				operateIfaceEnable(iface_hdmi, DISPLAY_OPERATE_READ);
//				if(iface_hdmi->enable == 0)
//				break;
//			}

			for(node = head; node != NULL; node = node->next) {
				readIfaceConnect(node);
				if(node->connect && node->enable == 0) {
					node->enable = 1;
					operateIfaceMode(node, DISPLAY_OPERATE_WRITE, node->mode);
					operateIfaceEnable(node, DISPLAY_OPERATE_WRITE);
				}
			}
		}
	}
}

void DisplayManager::selectNextIface(int display) {
	struct displaynode *head, *node, *iface_enabled = NULL, *iface_next = NULL;
	int enable;

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next) {
		if(node->enable == 1) {
			iface_enabled = node;
			break;
		}
	}
	// If there is no iface enabled or iface enabled is last interface, set first iface as next selected iface.
	if(iface_enabled != node)
		iface_next = head;
	else if(node->next == NULL)
		iface_next = head;
	else
		iface_next = node->next;

	// Disable current interface.
	if(iface_enabled != NULL) {
		iface_enabled->enable = 0;
		operateIfaceEnable(iface_enabled, DISPLAY_OPERATE_WRITE);
	}
	// Enable next interface.
	iface_next->enable = 1;
	operateIfaceMode(iface_next, DISPLAY_OPERATE_WRITE, iface_next->mode);
	operateIfaceEnable(iface_next, DISPLAY_OPERATE_WRITE);
}

void DisplayManager::switchFramebuffer(int display, int xres, int yres)
{
	char name[64];
	if(display == 0)
		snprintf(name, 64, "/dev/graphics/fb0");
	else
		snprintf(name, 64, "/dev/graphics/fb2");
	ALOGD("%s display %d new res is x %d y %d", __FUNCTION__, display, xres, yres);
	int fd = open(name, O_RDWR, 0);
	if(fd < 0) {
		ALOGE("%s open fb %s failed", __FUNCTION__, name);
		return;
	}

	struct fb_var_screeninfo info;
	if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
	{
		ALOGE("%s %s get fb_var_screeninfo failed", __FUNCTION__, name);
		return;
	}
	info.xres = xres;
	info.yres = yres;
//	info.xres_virtual = xres;
//	info.yres_virtual = info.yres * 2;
	if (ioctl(fd, FBIOPUT_VSCREENINFO, &info) == -1) {
		ALOGE("%s %s set fb_var_screeninfo failed", __FUNCTION__, name);
	}

	close(fd);
}

void DisplayManager::get3DModes(SocketClient *cli, int display, char* iface)
{
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];
	struct displaynode *head, *node;
	int type = string2type(iface);
	char *mode;

	if(type != DISPLAY_INTERFACE_HDMI) {
		cli->sendMsg(ResponseCode::CommandParameterError, "Wrong HDMI iface", false);
		return;
	}

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next)
	{
		if(node->type == type)
			break;
	}

	if(node == NULL) {
		cli->sendMsg(ResponseCode::CommandParameterError, "Missing iface", false);
		return;
	}

	// Read 3d modes;
	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, node->path);
	strcat(buf, "/3dmode");
	fd = fopen(buf, "r");
	if(fd == NULL) {
		cli->sendMsg(ResponseCode::CommandOkay, "0", false);
		return;
	}
	memset(buf, 0, BUFFER_LENGTH);
	fgets(buf, BUFFER_LENGTH, fd);
	if(strlen(buf)) {
		ALOGD("%s", buf);
		buf[strlen(buf) - 1] = 0;
		mode = strrchr(buf, '=');
		cli->sendMsg(ResponseCode::CommandOkay, mode + 1, false);
	}
	fclose(fd);
}

void DisplayManager::get3DMode(SocketClient *cli, int display, char* iface)
{
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];
	struct displaynode *head, *node;
	int type = string2type(iface);
	char *mode;

	if(type != DISPLAY_INTERFACE_HDMI) {
		cli->sendMsg(ResponseCode::CommandParameterError, "Wrong HDMI iface", false);
		return;
	}

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next)
	{
		if(node->type == type)
			break;
	}

	if(node == NULL) {
		cli->sendMsg(ResponseCode::CommandParameterError, "Missing iface", false);
		return;
	}

	// Read 3d modes;
	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, node->path);
	strcat(buf, "/3dmode");
	fd = fopen(buf, "r");
	if(fd == NULL) {
		cli->sendMsg(ResponseCode::CommandOkay, "-1", false);
		return;
	}
	fgets(buf, BUFFER_LENGTH, fd);
	memset(buf, 0, BUFFER_LENGTH);
	fgets(buf, BUFFER_LENGTH, fd);
	if(strlen(buf)) {
		ALOGD("%s", buf);
		mode = strchr(buf, '\n');
		if(mode)
			*mode = 0;
		mode = strrchr(buf, '=');
		cli->sendMsg(ResponseCode::CommandOkay, mode + 1, false);
	}
	fclose(fd);
}

int	DisplayManager::set3DMode(int display, char* iface, char* mode)
{
	FILE *fd = NULL;
	char buf[BUFFER_LENGTH];
	struct displaynode *head, *node;
	int type = string2type(iface);

	if(type != DISPLAY_INTERFACE_HDMI) {
		return -1;
	}

	if(display == MAIN_DISPLAY)
		head = main_display_list;
	else
		head = aux_display_list;

	for(node = head; node != NULL; node = node->next)
	{
		if(node->type == type)
			break;
	}

	if(node == NULL) {
		return -1;
	}

	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, node->path);
	strcat(buf, "/3dmode");
	fd = fopen(buf, "w");
	if(fd == NULL)
		return -1;
	fputs(mode, fd);
	fclose(fd);
	return 0;
}


void DisplayManager::updatesinkaudioinfo(struct displaynode *node) {
	ALOGI("updatesinkaudioinfo: %p\n", node);
	if(NULL == node)
		return;
	FILE *fd = NULL;
	char buf[PROPERTY_VALUE_MAX]="";

	// Read audioinfo;
	strcpy(buf, node->path);
	strcat(buf, "/audioinfo");

	fd = fopen(buf, "r");
	memset(buf, 0, PROPERTY_VALUE_MAX);
	if(fd != NULL) {
		fread(buf, 1, PROPERTY_VALUE_MAX, fd);
		ALOGI("BUF: %s\n", buf);
		fclose(fd);
	}
	property_set("media.sink.audio", buf);
}
