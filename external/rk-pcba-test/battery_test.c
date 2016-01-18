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

#define LOG_TAG "Battery"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/ioctl.h>
#include <pthread.h>
#include "language.h"
#include "test_case.h"
#include "battery_test.h"

static  int BATTERY_STATUS_UNKNOWN = 1;
static  int BATTERY_STATUS_CHARGING = 2;
static  int BATTERY_STATUS_DISCHARGING = 3;
static  int BATTERY_STATUS_NOT_CHARGING = 4;
static  int BATTERY_STATUS_FULL = 5;

#define POWER_SUPPLY_PATH "/sys/class/power_supply"

struct PowerSupplyPaths {
    char* acOnlinePath;
    char* usbOnlinePath;
    char* wirelessOnlinePath;
    char* batteryStatusPath;
    char* batteryHealthPath;
    char* batteryPresentPath;
    char* batteryCapacityPath;
    char* batteryVoltagePath;
    char* batteryTemperaturePath;
    char* batteryTechnologyPath;
};

struct PowerSupplyPaths gPaths;

int getBatteryVoltage(const char* status)
{
   return atoi(status);
}

int getBatteryStatus(const char* status)
{    
	switch (status[0]) 
	{        
		case 'C': return BATTERY_STATUS_CHARGING;         // Charging        
		case 'D': return BATTERY_STATUS_DISCHARGING;      // Discharging        
		case 'F': return BATTERY_STATUS_FULL;             // Full        
		case 'N': return BATTERY_STATUS_NOT_CHARGING;      // Not charging        
		case 'U': return BATTERY_STATUS_UNKNOWN;          // Unknown                    
		default: {            
			printf("Unknown battery status '%s'", status);            
			return -1;        
		}    
	}
}

static int readFromFile(const char* path, char* buf, size_t size)
{    
	if (!path)       
		return -1;    
	int fd = open(path, O_RDONLY, 0);    
	if (fd == -1) 
	{        
		printf("Could not open '%s'", path);        
		return -1;    
	}        
	ssize_t count = read(fd, buf, size);    
	if (count > 0) 
	{        
		while (count > 0 && buf[count-1] == '\n')            
			count--;        
			buf[count] = '\0';    
		} else {        
			buf[0] = '\0';   
		}     
	close(fd);   
	return count;
}

int BatteryPathInit()
{
	char    path[PATH_MAX];
    struct dirent* entry;

    DIR* dir = opendir(POWER_SUPPLY_PATH);
    if (dir == NULL) {
        printf("Could not open %s\n", POWER_SUPPLY_PATH);
		return -1;
    } else {
        while ((entry = readdir(dir))) {
            const char* name = entry->d_name;

            // ignore "." and ".."
            if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
                continue;
            }

            char buf[20];
            // Look for "type" file in each subdirectory
            snprintf(path, sizeof(path), "%s/%s/type", POWER_SUPPLY_PATH, name);
            int length = readFromFile(path, buf, sizeof(buf));
            if (length > 0) {
                if (buf[length - 1] == '\n')
                    buf[length - 1] = 0;

                if (strcmp(buf, "Mains") == 0) {
                    snprintf(path, sizeof(path), "%s/%s/online", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0)
                        gPaths.acOnlinePath = strdup(path);
                }
                else if (strcmp(buf, "USB") == 0) {
                    snprintf(path, sizeof(path), "%s/%s/online", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0)
                        gPaths.usbOnlinePath = strdup(path);
                }
                else if (strcmp(buf, "Wireless") == 0) {
                    snprintf(path, sizeof(path), "%s/%s/online", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0)
                        gPaths.wirelessOnlinePath = strdup(path);
                }
                else if (strcmp(buf, "Battery") == 0) {
                    snprintf(path, sizeof(path), "%s/%s/status", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0)
                        gPaths.batteryStatusPath = strdup(path);
                    snprintf(path, sizeof(path), "%s/%s/health", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0)
                        gPaths.batteryHealthPath = strdup(path);
                    snprintf(path, sizeof(path), "%s/%s/present", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0)
                        gPaths.batteryPresentPath = strdup(path);
                    snprintf(path, sizeof(path), "%s/%s/capacity", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0)
                        gPaths.batteryCapacityPath = strdup(path);

                    snprintf(path, sizeof(path), "%s/%s/voltage_now", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0) {
                        gPaths.batteryVoltagePath = strdup(path);   
                    } else {
                        snprintf(path, sizeof(path), "%s/%s/batt_vol", POWER_SUPPLY_PATH, name);
                        if (access(path, R_OK) == 0)
                            gPaths.batteryVoltagePath = strdup(path);
                    }

                    snprintf(path, sizeof(path), "%s/%s/temp", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0) {
                        gPaths.batteryTemperaturePath = strdup(path);
                    } else {
                        snprintf(path, sizeof(path), "%s/%s/batt_temp", POWER_SUPPLY_PATH, name);
                        if (access(path, R_OK) == 0)
                            gPaths.batteryTemperaturePath = strdup(path);
                    }

                    snprintf(path, sizeof(path), "%s/%s/technology", POWER_SUPPLY_PATH, name);
                    if (access(path, R_OK) == 0)
                        gPaths.batteryTechnologyPath = strdup(path);
                }
            }
        }
        closedir(dir);
    }

   
	if (!gPaths.batteryStatusPath) 
	{
		printf("%s is not Exist\n", gPaths.batteryStatusPath);
		return -1;
	}

	if (!gPaths.batteryVoltagePath) 	
	{
		printf("%s is not Exist\n", gPaths.batteryVoltagePath);
		return -1;
	}
	return 0;
}

 void* battery_test(void *argv)
 {
 	char Voltagebuf[20];
	char Statusbuf[20];
	int ret;
	int result;
	int curprint;
	char *strbatstatus;
	char *strbatVoltage;
	struct testcase_info *tc_info = (struct testcase_info*)argv;

	/*remind ddr test*/
	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	
 #ifdef SOFIA3GR_PCBA
	ui_print_xy_rgba(0,tc_info->y,255,255,0,255,"%s:[%s]\n",PCBA_BATTERY,PCBA_TESTING);
 	tc_info->result = 0;
 #else
    ui_print_xy_rgba(0,tc_info->y,255,255,0,255,"%s \n",PCBA_BATTERY);
 #endif
	
 	if(BatteryPathInit()<0)
 	{
 		ui_print_xy_rgba(0,tc_info->y,255,0,0,255,"%s:[%s]\n",PCBA_BATTERY,PCBA_FAILED);
		tc_info->result = -1;
		return argv;
 	}

	for(;;)
	{
		memset(Voltagebuf,0,sizeof(Voltagebuf));

		ret=readFromFile(gPaths.batteryVoltagePath,Voltagebuf,sizeof(Voltagebuf));
		
		if(ret<0)
			break;
		
		memset(Statusbuf,0,sizeof(Statusbuf));
		ret=readFromFile(gPaths.batteryStatusPath,Statusbuf,sizeof(Statusbuf));
		if((ret<0)||(getBatteryStatus(Statusbuf)<0))
			break;

		result=getBatteryStatus(Statusbuf);
		if(result==BATTERY_STATUS_CHARGING){
			ui_display_sync(0,tc_info->y,0,255,0,255,"%s:[%s] (%s:%d)\n",\
			PCBA_BATTERY,PCBA_BATTERY_CHARGE,PCBA_BATTERY_VOLTAGE, atoi(Voltagebuf)/1000);
			tc_info->result = 0;
			return argv;
		}
		else if(result==BATTERY_STATUS_FULL){
			ui_display_sync(0,tc_info->y,0,255,0,255,"%s:[%s] (%s:%d)\n",\
			PCBA_BATTERY,PCBA_BATTERY_FULLCHARGE,PCBA_BATTERY_VOLTAGE,atoi(Voltagebuf)/1000);
			tc_info->result = 0;
			return argv;
		}else{
			ui_display_sync(0,tc_info->y,0,255,0,255,"%s:[%s] (%s:%d)\n",\
			PCBA_BATTERY,PCBA_BATTERY_DISCHARGE,PCBA_BATTERY_VOLTAGE,atoi(Voltagebuf)/1000);
			tc_info->result = 0;
			return argv;
		}
		
		
					
		//ui_print_xy_rgba(0,g_msg->y,0,0,255,255,"gsensor x:%f y:%f z:%f\n",g_x,g_y,g_z);
		#ifndef SOFIA3GR_PCBA
		sleep(1);
		#else
		sleep(1);
		#endif
	}

    ui_print_xy_rgba(0,tc_info->y,255,0,0,255,"%s:[%s]\n",PCBA_BATTERY,PCBA_FAILED);
	tc_info->result = -1;
	return argv;
 }
 
