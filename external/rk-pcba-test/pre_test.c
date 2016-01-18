#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <inttypes.h>
#include <errno.h>

#include "./minuitwrp/minui.h"
#include "recovery_ui.h"
#include "rtc_test.h"
#include "screen_test.h"
#include "key_test.h"
#include "cutils/android_reboot.h"

#include "wlan_test.h"
#include "bt_test.h"
#include "gsensor_test.h"
#include "sdcard_test.h"
#include "udisk_test.h"
#include "common.h"
#include "gui/gui.h"
#include "extra-functions.h"
#include "data.h"
#include "script.h"
#include "test_case.h"
#include "script_parser.h"
#include "debug.h"
#include "hdmi_test.h"
#include "sim_test.h"
#include "battery_test.h"
#include "ddr_test.h"
#include "cpu_test.h"
#include "codec_test.h"
#include "vibrator.h"
#include "lan_test.h"
#include "flashlight_test.h"
#include "nand_test.h"
#include <signal.h>
#include "language.h"
#ifdef RK3288_PCBA
#include "rk3288-camera/camera_test.h"
#else
#ifdef RK312X_PCBA
#include "rk312x-camera/camera_test.h"
#else
#ifdef SOFIA3GR_PCBA
#include "sofia_camera/camera_test.h"
#include "at_util_extern.h"
#else
#include "camera_test.h"
#endif
#endif
#endif
#include "lightsensor_test.h"
#include "gnss_test.h"
#include "psensor_test.h"
#include "fm_test.h"
#include "compass_test.h"


#ifdef SOFIA3GR_PCBA
#define SCRIPT_NAME                     "/system/etc/test_config.cfg"
#else
#define SCRIPT_NAME                     "/res/test_config.cfg"
#endif


#define ITEM_H				2			//height of test item
#define ITEM_X				0			//x positon of test item

#define LOG(x...) printf(x)

pid_t g_codec_pid = -1;

static const char *TEMPORARY_LOG_FILE = "/tmp/recovery.log";

const char* man_title[] = { "manual test",
                        "",
                        NULL};
const char* man_items[]= {
				"Codec",
				"Backlight",
				"tp",
                        	NULL};

struct manual_item m_item[] = {
		/*name,  	x, 	y, 	w,	h,	argc, 	func*/
		{"Codec",	0,	3,	40,	5,	NULL,	NULL},
		{"KEY",		0,	9,	40,	5,	NULL,	NULL},
		{NULL,		0,	0,	0,	0,	NULL,	NULL}, //end of item
	};


int manual_p_y = 1;

static int breakFirstUi = 0;
static int hasCodec = 0;

int cur_p_y;		//current position for auto test tiem in y direction
pthread_t rtc_tid;  
char *rtc_res;
char dt[30]={"20120927.143045"};
struct rtc_msg *rtc_msg;
int err_rtc;
int rtc_p_y; //rtc position in y direction
pthread_t battery_tid;  

pthread_t screen_tid;  
char *screen_res;
struct screen_msg *screen_msg;
int screen_err = -1;


pthread_t codec_tid;
char *codec_res;
struct codec_msg *codec_msg;


pthread_t key_tid;
char *key_res;
struct key_msg *key_msg;

pthread_t fm_tid;
pthread_t nand_tid;
char *fm_res;
struct fm_msg *fm_msg;

pthread_t vibrator_tid;
char *vibrator_res;
struct fm_msg *vibrator_msg;

pthread_t falshlight_tid;
char *falshlight_res;
struct fm_msg *falshlight_msg;


pthread_t camera_tid;  
char *camera_res;
struct camera_msg *camera_msg;
int camera_err = -1;

pthread_t camera1_tid;  


pthread_t wlan_tid;  
char *wlan_res;
struct wlan_msg *wlan_msg;
int wlan_err = -1;


pthread_t bt_tid;  
char *bt_res;
struct bt_msg *bt_msg;
int bt_err = -1;

pthread_t gsensor_tid;  
char *gsensor_res;
struct gsensor_msg *gsensor_msg;
int gsensor_err = -1;


pthread_t sd_tid;  
char *sd_res;
struct sd_msg *sd_msg;
int sd_err = -1;

pthread_t udisk_tid;  
char *udisk_res;
struct udisk_msg *udisk_msg;
int udisk_err = -1;

pthread_t hdmi_tid;  
char *hdmi_res;
struct sd_msg *hdmi_msg;
int hdmi_err = -1;

pthread_t sim_tid;
pthread_t at_util_extern_tid;

pthread_t ddr_tid;  
int ddr_err = -1;

pthread_t cpu_tid;
int cpu_err = -1;

pthread_t lan_tid;
int lan_err = -1;

pthread_t lsensor_tid; 
pthread_t gps_tid; 
pthread_t psensor_tid; 
pthread_t compass_tid;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int lock = 1;
int rf_cal_result;
int wifi_cal_result;
char imei_result[50];
int UI_LEVEL = 1;
extern int simCounts;
int reboot_normal = 0;

static pthread_mutex_t gCur_p_y = PTHREAD_MUTEX_INITIALIZER;

#ifdef SOFIA3GR_PCBA
extern void ptest_set_key_wait_status(int key_wait);
#endif

int get_cur_print_y(void)
{
	int tmp;
	pthread_mutex_lock(&gCur_p_y);
	if (gr_fb_height() >600)
		cur_p_y--;
	 tmp = cur_p_y--;
	pthread_mutex_unlock(&gCur_p_y);
	printf("cur_print_y:%d\n",tmp);
	return tmp;
	
}

static int total_testcases = 0;
static struct testcase_base_info *base_info = NULL;
static struct list_head auto_test_list_head;
static struct list_head manual_test_list_head;


static int parse_testcase()
{
    int i, j, mainkey_cnt;
    struct testcase_base_info *info;
    char mainkey_name[32], display_name[64], binary[16];
    int activated, category, run_type, sim_counts;
    int len;

    mainkey_cnt = script_mainkey_cnt();
    info = (struct testcase_base_info*)malloc(sizeof(struct testcase_base_info) * mainkey_cnt);
    if (info == NULL) {
        db_error("core: allocate memory for temporary test case basic "
                "information failed(%s)\n", strerror(errno));
        return -1;
    }
    memset(info, 0, sizeof(struct testcase_base_info) * mainkey_cnt);

    for (i = 0, j = 0; i < mainkey_cnt; i++) {
		struct testcase_info *tc_info;
        memset(mainkey_name, 0, 32);
        script_mainkey_name(i, mainkey_name);

        if (script_fetch(mainkey_name, "display_name", (int *)display_name, 16))
            continue;

        if (script_fetch(mainkey_name, "activated", &activated, 1))
            continue;

        if (display_name[0] && activated == 1) {
            strncpy(info[j].name, mainkey_name, 32);
            strncpy(info[j].display_name, display_name, 64);
            info[j].activated = activated;

            if (script_fetch(mainkey_name, "program", (int *)binary, 4) == 0) {
                strncpy(info[j].binary, binary, 16);
            }

            info[j].id = j;

            if (script_fetch(mainkey_name, "category", &category, 1) == 0) {
                info[j].category = category;
            }

            if (script_fetch(mainkey_name, "run_type", &run_type, 1) == 0) {
                info[j].run_type = run_type;
            }
			
			if (script_fetch(mainkey_name, "sim_counts", &sim_counts, 1) == 0) {
                simCounts = sim_counts;
            }
			tc_info = (struct testcase_info*) malloc(sizeof(struct testcase_info));
			if(tc_info == NULL)
			{
				printf("malloc for tc_info[%d] fail\n",j);
				return -1;
			}
			tc_info->x = 0;
			tc_info->y =0;
			tc_info->base_info = &info[j];
			if(tc_info->base_info->category)   
				 list_add(&tc_info->list, &manual_test_list_head);
			else
				 list_add(&tc_info->list, &auto_test_list_head);
			//printf("name:%s>>disp_name:%s>>activated:%d>>binary:%s>>id:%d>>category:%d>>run_type:%d\n",
			//	info[j].name,info[j].display_name, info[j].activated,info[j].binary,info[j].id, info[j].category,
			//	 info[j].run_type);
            j++;
        }
    }
    total_testcases = j;

    db_msg("core: total test cases #%d\n", total_testcases);
    if (total_testcases == 0) {
        return 0;
    }

    len = sizeof(struct testcase_base_info) * total_testcases;
	#if 0
    base_info_shmid = shmget(IPC_PRIVATE, len, IPC_CREAT | 0666);
    if (base_info_shmid == -1) {
        db_error("core: allocate share memory segment for test case basic "
                "information failed(%s)\n", strerror(errno));
        return -1;
    }

    base_info = shmat(base_info_shmid, 0, 0);
    if (base_info == (void *)-1) {
        db_error("core: attach the share memory for test case basic "
                "information failed(%s)\n", strerror(errno));
        shmctl(base_info_shmid, IPC_RMID, 0);
        return -1;
    }
    memcpy(base_info, info, sizeof(struct testcase_base_info) * 
            total_testcases);
	#endif

    return total_testcases;
}

void write_test_result_to_nvm() {	
	int writeCounts = 1;
	struct list_head *pos;
	
	list_for_each(pos, &auto_test_list_head) {
		struct testcase_info *tc_info = list_entry(pos, struct testcase_info, list);
		if(tc_info->result == -1) {
			writeCounts = 1;
			while(commit_pcba_test_value(0) < 0 && writeCounts <= 3) {
				writeCounts++;
			}
			return;
		}
	}

	writeCounts = 1;
	while(commit_pcba_test_value(1) < 0 && writeCounts <= 3) {
		writeCounts++;
	}

}

void change_bootmode_to_nvm(int mode) {	
	int writeCounts = 1;
	while(change_bootmode(mode)) {
		if(writeCounts >= 3) {
			printf("sofia-3gr:change_bootmode_to_nvm faile.mode=%d", mode);
			return;
		}
		writeCounts++;
	}
	android_reboot(ANDROID_RB_RESTART, 0, 0);
}

int start_test_pthread(struct testcase_info *tc_info)
{
	int err;
	printf("%s\n",tc_info->base_info->name);

	if(!strcmp(tc_info->base_info->name, "Lcd"))
	{
		err = pthread_create(&screen_tid, NULL, screen_test,tc_info); //
		if(err != 0)
		{  
			   printf("create screen test thread error: %s/n",strerror(err));
			   return -1;
			   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "rtc"))
	{
		err = pthread_create(&rtc_tid, NULL, rtc_test,tc_info); //
		if(err != 0)
		{  
		   printf("create rtc test thread error: %s/n",strerror(err));
		   return -1;
		   
		}  
	}else if(!strcmp(tc_info->base_info->name, "battery"))
	{
		err = pthread_create(&battery_tid, NULL, battery_test,tc_info); //
		if(err != 0)
		{  
		   printf("create battery_test test thread error: %s/n",strerror(err));
		   return -1;
		   
		}  
	}else if(!strcmp(tc_info->base_info->name, "Codec"))
    {
    		hasCodec = 1;
            err = pthread_create(&codec_tid, NULL, codec_test,tc_info); //
            if(err != 0)
            {
               printf("create codec test thread error: %s/n",strerror(err));
               return -1;

            }
    }else if(!strcmp(tc_info->base_info->name, "Key"))
	{
		err = pthread_create(&key_tid, NULL, key_test,tc_info); //
		if(err != 0)
		{  
		   printf("create key test thread error: %s/n",strerror(err));
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "fm"))
	{
		err = pthread_create(&fm_tid, NULL, fm_test,tc_info); //
		if(err != 0)
		{  
		   printf("create fm test thread error: %s/n",strerror(err));
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "camera"))
	{	
		tc_info->dev_id = 0;
		err = pthread_create(&camera_tid, NULL, camera_test,tc_info); //
		if(err != 0)
		{  
		   printf("create camera test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "wifi"))
	{
		//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,255,"iiiiiiiiii wifi 1\n");
		
		err = pthread_create(&wlan_tid, NULL, wlan_test,tc_info); //
		if(err != 0)
		{  
		   printf("create wifi test thread error: %s/n",strerror(err));	
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "nand"))
	{
		err = pthread_create(&nand_tid, NULL, nand_test,tc_info); //
		if(err != 0)
		{  
		   printf("create nandflash test thread error: %s/n",strerror(err));	
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "bluetooth"))
	{
		printf("bluetooth_test thread created\n");

		err = pthread_create(&bt_tid, NULL, bt_test,tc_info); //
		if(err != 0)
		{  
		   printf("create bt(bluetooth) test thread error: %s/n",strerror(err));	
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "gsensor"))
	{
#ifdef SOFIA3GR_SENSOR_MPU
		err = pthread_create(&gsensor_tid, NULL, gsensor_test_mpu,tc_info); //
		if(err != 0)
		{  
		   printf("create gsensor test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		} 
#else
		err = pthread_create(&gsensor_tid, NULL, gsensor_test,tc_info); //
		if(err != 0)
		{  
		   printf("create gsensor test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		}  
#endif
	}
	else if(!strcmp(tc_info->base_info->name, "lsensor"))
	{
		err = pthread_create(&lsensor_tid, NULL, lightsensor_test,tc_info); //
		if(err != 0)
		{  
		   printf("create lsensor test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "gps"))
	{
		err = pthread_create(&gps_tid, NULL, gps_test,tc_info); //
		if(err != 0)
		{  
		   printf("create gps test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "psensor"))
	{
		err = pthread_create(&psensor_tid, NULL, psensor_test,tc_info); //
		if(err != 0)
		{  
		   printf("create psensor test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "compass"))
	{
#ifdef SOFIA3GR_SENSOR_MPU
		err = pthread_create(&compass_tid, NULL, compass_test_mpu,tc_info); //
		if(err != 0)
		{  
		   printf("create mpu compass test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		} 
#else
		err = pthread_create(&compass_tid, NULL, compass_test,tc_info); //
		if(err != 0)
		{  
		   printf("create ST compass test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		}  
#endif
	}
	else if(!strcmp(tc_info->base_info->name, "udisk"))
	{
		err = pthread_create(&udisk_tid, NULL, udisk_test,tc_info); //
		if(err != 0)
		{  
		   printf("create sdcard test thread error: %s/n",strerror(err)); 
		   return -1;
		   
		}
	}
	else if(!strcmp(tc_info->base_info->name, "sdcard"))
	{
		sd_err = pthread_create(&sd_tid, NULL, sdcard_test,tc_info); //
		if(sd_err != 0)
		{  
		   printf("create sdcard test thread error: %s/n",strerror(sd_err));
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "hdmi"))
	{
		hdmi_err = pthread_create(&hdmi_tid, NULL, hdmi_test, tc_info); //
		if(hdmi_err != 0)
		{  
		   printf("create hdmi test thread error: %s/n",strerror(hdmi_err));
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "sim"))
	{
		err = pthread_create(&sim_tid, NULL, sim_test,tc_info); //
		if(err != 0)
		{  
		   printf("create sim test thread error: %s/n",strerror(err));
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "vibrator"))
	{
		err = pthread_create(&vibrator_tid, NULL, vibrator_test,tc_info); //
		if(err != 0)
		{  
		   printf("create vibrator test thread error: %s/n",strerror(err));
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "falshlight"))
	{
		err = pthread_create(&falshlight_tid, NULL, flashlight_test,tc_info); //
		if(err != 0)
		{  
		   printf("create flashlight test thread error: %s/n",strerror(err));
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "ddr"))
	{
		ddr_err = pthread_create(&ddr_tid, NULL, ddr_test,tc_info); //
		if(ddr_err != 0)
		{  
		   printf("create ddr test thread error: %s/n",strerror(ddr_err));
		   return -1;
		   
		}  
	}
	else if(!strcmp(tc_info->base_info->name, "cpu")){
		cpu_err = pthread_create(&cpu_tid, NULL, cpu_test, tc_info);
		if(cpu_err != 0){
		   printf("create cpu test thread error: %s/n",strerror(cpu_err));
		   return -1;
		}
	}
	else if(!strcmp(tc_info->base_info->name, "lan")){
		lan_err = pthread_create(&lan_tid, NULL, lan_test, tc_info); //
		if(lan_err != 0)
		{  
		   printf("create lan test thread error: %s/n",strerror(lan_err));
		   return -1;
		}  
	}
	else
	{
		printf("unsupport test item:%s\n",tc_info->base_info->name);
		return -1;
	}

	return 0;
			
}

int init_manual_test_item(struct testcase_info *tc_info)
{
	int err = 0;

	manual_p_y += 1;
	tc_info->y=manual_p_y;

	start_test_pthread(tc_info);
	
	return 0;
}

int start_manual_test_item(int x,int y)
{
	return 1;Camera_Click_Event(x,y); 
}

int start_auto_test_item(struct testcase_info *tc_info)
{
	printf("start_auto_test_item : %d, %s \r\n",tc_info->y,tc_info->base_info->name);
	start_test_pthread(tc_info);
	
	return 0;
}

int ensure_path_mounted(const char* path) {return 0;}

int 
get_menu_selection(char** headers, char** items, int menu_only,
                   int initial_selection) {
    // throw away keys pressed previously, so user doesn't
    // accidentally trigger menu items.
    ui_clear_key_queue();

    ui_start_menu(headers, items, initial_selection);
    int selected = initial_selection;
    int chosen_item = -1;
    
    while (chosen_item < 0) {
        int key = ui_wait_key();
        int visible = ui_text_visible();

        int action = device_handle_key(key, visible);
		if (action < 0) {
		    switch (action) {
			case HIGHLIGHT_UP:
			    --selected;
			    selected = ui_menu_select(selected);
			    break;
			case HIGHLIGHT_DOWN:
			    ++selected;
			    selected = ui_menu_select(selected);
			    break;
			case KEY_POWER:
			case SELECT_ITEM:
			    chosen_item = selected;
			    break;
			case UP_A_LEVEL:
				if (menu_loc_idx != 0)
				{
					chosen_item = menu_loc[menu_loc_idx];
				}
			    break;
			case HOME_MENU:
				if (menu_loc_idx != 0)
				{
					go_home = 1;
					chosen_item = menu_loc[menu_loc_idx];
				}
			    break;
			case MENU_MENU:
				if (menu_loc_idx == 0)
				{
				    return 3;
				} else
				{
				go_home = 1;
				go_menu = 1;
				chosen_item = menu_loc[menu_loc_idx];
				}
			    break;
			case NO_ACTION:
			    break;
		    }
		} else if (!menu_only) {
		    chosen_item = action;
		}
	    }

	    ui_end_menu();
	    return chosen_item;
	}


	char**
	prepend_title(const char** headers) {
	    char* title1 = (char*)malloc(40);
	    strcpy(title1, "Team Win Recovery Project (twrp) v");
	    char* header1 = strcat(title1, DataManager_GetStrValue(TW_VERSION_VAR));
	    char* title[] = { header1,
			      "Based on Android System Recovery <"
			      EXPAND(RECOVERY_API_VERSION) "e>",
			      "", //
			      print_batt_cap(),
			      "", //
			      NULL };

	    // count the number of lines in our title, plus the
	    // caller-provided headers.
	    int count = 0;
	    char** p;
	    for (p = title; *p; ++p, ++count);
	    for (p = (char**) headers; *p; ++p, ++count);

	    char** new_headers = (char**)malloc((count+1) * sizeof(char*));
	    char** h = new_headers;
	    for (p = title; *p; ++p, ++h) *h = *p;
	    for (p = (char**) headers; *p; ++p, ++h) *h = *p;
	    *h = NULL;

	    return new_headers;
	}

	void prompt_and_wait()
	{

		// Main Menu
		#define START_FAKE_MAIN          0
		#define REALMENU_REBOOT     	 1

		go_reboot = 0;
	    //ui_reset_progress();

		char** headers = prepend_title((const char**)MENU_HEADERS);
	    char* MENU_ITEMS[] = {  "Start Recovery",
				    "Reboot",
				    NULL };
		
	    for (;;)
		{

		go_home = 0;
		go_menu = 0;
		menu_loc_idx = 0;
			

		if (go_reboot)
			{
				return;
			}
			show_fake_main_menu();
	    }
	}


#ifdef SOFIA3GR_PCBA
int sync_screen_for_prompt(void)
{
	if(breakFirstUi)
	{
		return 0;
	}

	ui_print_xy_rgba(0,4,0,255,0,255,"\n");
	return 0;
}
#endif

int main(int argc, char **argv)
{
	int ret,w;
	char *script_buf;
	struct list_head *pos;
	int success = 0;
	char rfCalResult[10];
	
#ifdef SOFIA3GR_PCBA
	freopen("/dev/ttyFIQ0", "a", stdout); setbuf(stdout, NULL);
	freopen("/dev/ttyFIQ0", "a", stderr); setbuf(stderr, NULL);
	
	ui_init();
	ui_print_init();

	int at_err = pthread_create(&at_util_extern_tid, NULL, getImei_testresult, NULL); //
	if(at_err != 0)
	{  
	   printf("create get cal result thread error: %s\n",strerror(at_err));
	   return -1;
	} 

	w =  gr_fb_width() >> 1;
	ui_print_xy_rgba(((w>>1)/CHAR_WIDTH-2),(gr_fb_height()/CHAR_HEIGHT)/2 - 1,0,255,0,255,"%s\n",PCBA_INTEL_PTEST_MODE);
	ui_print_xy_rgba(0,0,0,255,0,255,"%s\n",PCBA_BOOT_IN_ANDROID_FUCTION);
	ui_print_xy_rgba(0,1,0,255,0,255,"%s\n",PCBA_BOOT_IN_CORE_FUNCTION);
	ui_print_xy_rgba(0,2,0,255,0,255,"%s\n",PCBA_BOOT_IN_TOUCH_FUNCTION);

	gui_start();
	//start_input_thread_for_key_check();
	start_input_thread();

	//prompt_and_wait();
	printf("ptest wait key...\n");
	int key_code, key_count = 0, key_power = 0, key_vol_plus = 0, key_vol_cut = 0;
	while(key_code = ui_wait_key()) {
		printf("get key_code = %d\n\n", key_code);
		switch(key_code) {
			case KEY_POWER:
				ui_print_xy_rgba(0,4,255,0,0,255,"%s\n",PCBA_POWER_KEY);
				key_count++;
				key_power++;
				if(key_count == 5) {
					//reboot and set nvm state
					printf("Pre_test: power key 5 times setBootMode\n");
					reboot_normal = 1;
					change_bootmode_to_nvm(1);
				}
				break;
			case KEY_VOLUMEUP:
				printf("Volueme up key is press! \n");
				ui_print_xy_rgba(0,4,255,0,0,255,"%s\n",PCBA_VOLUME_UP_KEY);
				key_vol_plus++;				
				break;
			case KEY_VOLUMEDOWN:
				printf("Voluem down key is press! \n");
				ui_print_xy_rgba(0,4,255,0,0,255,"%s\n",PCBA_VOLUME_DOWN_KEY);
				key_vol_cut++;
				break;
			default:
				key_count = 0;
				break;
		}

		if(key_power>= 1 && key_vol_plus >= 1 && key_vol_cut >= 1) {
			UI_LEVEL = 2;
			break;
		}
	}

	printf("exit wait key...\n");

	ui_print_xy_rgba(((w>>1)/CHAR_WIDTH-2),(gr_fb_height()/CHAR_HEIGHT)/2 - 1,0,255,0,255,"%s\n",""); //clear ptest mode
	ui_print_xy_rgba(0,4,0,255,0,255,"%s\n","");
	ui_print_xy_rgba(0,3,0,255,0,255,"%s\n","");
	ui_print_xy_rgba(0,2,0,255,0,255,"%s\n","");
	ui_print_xy_rgba(0,1,255,0,0,255,"%s\n","");
	breakFirstUi = 1;

	printf("set key wait status to exit \n");

#else
	freopen("/dev/ttyFIQ0", "a", stdout); setbuf(stdout, NULL);
	freopen("/dev/ttyFIQ0", "a", stderr); setbuf(stderr, NULL);

#ifdef RK3368_PCBA
	freopen("/dev/ttyS2", "a", stdout); setbuf(stdout, NULL);
	freopen("/dev/ttyS2", "a", stderr); setbuf(stderr, NULL);
#endif
	if (gui_init())
	{
		ui_init();
		ui_set_background(BACKGROUND_ICON_INSTALLING);
	}
	ui_print_init();
	gui_loadResources();
#endif

#if 1
	w =  gr_fb_width() >> 1;

#ifdef SOFIA3GR_PCBA
	ui_print_xy_rgba(((w>>1)/CHAR_WIDTH-9),0,0,255,0,255,"%s\n",PCBA_TIP_IN_PCBA_FUNCTION);
	ui_print_xy_rgba(0,1,0,255,0,255,"[%s]%s [%s]%s %s\n",PCBA_RF_CAL, rf_cal_result == 0 ? PCBA_CAL_NO : PCBA_CAL_YES,
																PCBA_WIFI_CAL, wifi_cal_result == 0 ? PCBA_CAL_NO : PCBA_CAL_YES,
																imei_result);
#else
	ui_print_xy_rgba(((w>>1)/CHAR_WIDTH-9),0,0,255,0,255,"%s\n",PCBA_VERSION_NAME);
#endif
//	ui_print_xy_rgba(0,1,255,0,0,255,"%s %s\n",__DATE__,__TIME__);
#ifndef SOFIA3GR_PCBA
	ui_print_xy_rgba(((w>>1)/CHAR_WIDTH-3),1,255,255,0,255,"%s\n",PCBA_MANUAL_TEST);
    drawline_4(255,255,0,255,0,(1*CHAR_HEIGHT-CHAR_HEIGHT/4),w,CHAR_HEIGHT,3);
#endif
	cur_p_y = (gr_fb_height()/CHAR_HEIGHT) - 1;
	INIT_LIST_HEAD(&manual_test_list_head);
	INIT_LIST_HEAD(&auto_test_list_head);
	script_buf = parse_script(SCRIPT_NAME);
	if (!script_buf)
	{
		   printf("parse script failed\n");
		   return -1;
	}
	
	ret = init_script(script_buf);
	if (ret) {
		   db_error("core: init script failed(%d)\n", ret);
		   return -1;
	}

	ret = parse_testcase();
	if (ret < 0) {
		db_error("core: parse all test case from script failed(%d)\n", ret);
		return -1;
	}
	else if (ret == 0) {
		db_warn("core: NO TEST CASE to be run\n");
		return -1;
	}
	
	printf("manual testcase:\n");
	list_for_each(pos, &manual_test_list_head) {
		struct testcase_info *tc_info = list_entry(pos, struct testcase_info, list);
		init_manual_test_item(tc_info);
		
	}

#ifndef SOFIA3GR_PCBA
	ui_print_xy_rgba(((w>>1)/CHAR_WIDTH-3),manual_p_y+1,255,255,0,255,"%s\n",PCBA_AUTO_TEST);
        drawline_4(255,255,0,255,0,(CHAR_HEIGHT*(manual_p_y+1)-CHAR_HEIGHT/4),w,CHAR_HEIGHT,3); 
#endif

	printf("\n\nauto testcase:\n");
	list_for_each(pos, &auto_test_list_head) {
		struct testcase_info *tc_info = list_entry(pos, struct testcase_info, list);
		start_auto_test_item(tc_info);
	}
	
#endif


	//while(1);
	
#ifdef SOFIA3GR_PCBA
	//start_input_thread();
	if(!hasCodec) {
		ptest_set_key_wait_status(1);
	}
	join_input_thread();
	pthread_mutex_destroy(&mutex);  
    pthread_cond_destroy(&cond); 
#else
	gui_start();
	start_input_thread();
#endif
	
#if 0
	//prompt_and_wait();
	stopCameraTest();
	list_for_each(pos, &auto_test_list_head) {
		struct testcase_info *tc_info = list_entry(pos, struct testcase_info, list);
		if(tc_info->result == -1){
			success = -1;
			printf("%s test fail!\n",tc_info->base_info->name);
			
		}
	}
	finishCameraTest();
#endif
	/*if(g_codec_pid > 0){
		kill(g_codec_pid,SIGKILL);
		printf("pcba-test-over\n");
		sleep(3);
	}*/

	printf("pcba test over!\n");
	return success;
}

