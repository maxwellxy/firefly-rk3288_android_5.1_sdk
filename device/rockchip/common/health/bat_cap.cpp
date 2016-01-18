#define CHARGER_KLOG_LEVEL 6

#include <stdio.h>
#include <string.h>
#include <cutils/klog.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#define LOGE(x...) do { KLOG_ERROR("bat_cap", x); } while (0)
#define LOGI(x...) do { KLOG_INFO("bat_cap", x); } while (0)
#define LOGV(x...) do { KLOG_DEBUG("bat_cap", x); } while (0)

//#define CHARGER_SAVE_WHEN_RECOVERY 1
#ifdef CHARGER_SAVE_WHEN_RECOVERY
#define CAPACITY_PATH "/metadata/battery.bat"
#else
#define CAPACITY_PATH "/data/battery.bat"
#endif

char oldcap_path[PATH_MAX];

int get_oldcap_path(void)
{
        const char *dirname = "/sys/devices"; 
        DIR* dir = opendir(dirname);
        oldcap_path[0] = '\n';

        if (dir == NULL) {
                LOGE("Could not open %s\n", dirname);
                return -1;
        }

        struct dirent* entry;

        while ((entry = readdir(dir))) {
                const char* name = entry->d_name;
                if (strstr(name, ".adc")){ 
                        sprintf(oldcap_path, "%s/%s", dirname, name);
                        LOGI("oldcap path is %s\n", oldcap_path); 
                        break;
                }
        }
        closedir(dir);
        if (oldcap_path[0] == '\n')
                return -1;


        dir = opendir(oldcap_path);
        if (dir == NULL) {
                LOGE("Could not open %s\n", oldcap_path);
                oldcap_path[0] = '\n';
                return -1;
        }

        while ((entry = readdir(dir))) {
                const char* name_adc = entry->d_name;
                if (strstr(name_adc, "adc-battery.")){ 
                        sprintf(oldcap_path, "%s/%s/oldcap", oldcap_path, name_adc);
                        LOGI("oldcap path is %s\n", oldcap_path); 
                        closedir(dir);
                        return 0;
                }
        }
        closedir(dir);
        oldcap_path[0] = '\n';
        return -1;
}


int put_old_adc_cap(int bat_cap)
{
    // add by xhc
    FILE* fd = NULL;
    static int s_old_capacity = -1; 
    char cap_buf[8];

    if(oldcap_path[0] == '\n') {
       return -1;
    }

    if ((s_old_capacity != bat_cap) && (bat_cap >= 0 && bat_cap <= 100)) {

        if (s_old_capacity == -1) {                        
                s_old_capacity = bat_cap;
                return 0;
        }
        
        LOGI("rk set new_capacity; new_capacity = %d, old_capacity = %d\n", bat_cap, s_old_capacity);
        s_old_capacity = bat_cap;
        fd = fopen(CAPACITY_PATH, "w");
        if (fd == NULL) {
            LOGE("write capacity fopen error\n");

        } else {
            sprintf(cap_buf, "%d\n", bat_cap);
            fputs(cap_buf, fd);
            fclose(fd);
        }
    }
    return 0;
}

int load_old_adc_cap(void)
{

    FILE *fd = NULL;
    char cap_buf[8];

    get_oldcap_path();

    if(oldcap_path[0] == '\n') {
       LOGE("oldcap_path no exist\n"); 
       return -1;
    }
    

    fd = fopen(CAPACITY_PATH, "r");
    if (fd == NULL) {
        LOGE("read capacity fopen error\n");
    } else {
        fgets(cap_buf, 6, fd);
        //fread(cap_buf, 1, 6, fd);
        fclose(fd);
        LOGI("read_capacity is %s\n", cap_buf);

      
        fd = fopen(oldcap_path, "w");
        if (fd == NULL) {
            LOGE("read oldcap fopen error\n");
        } else {
            LOGI("write oldcap is %s\n", cap_buf); 
            fputs(cap_buf, fd);
            fclose(fd);
        }
    }
    return 0;

}

