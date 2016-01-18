/*
 * board_id_ctrl.c
 *
 *  Created on: 2013-4-27
 *      Author: mmk
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "board_id_ctrl.h"

#include "common.h"
// #include "custom_log.h"

static int sCtrlFd = -1;
int board_id_open_device(void)
{
	if (sCtrlFd < 0)
	{
		sCtrlFd = open("/dev/board_id_misc", O_RDWR);
		if(sCtrlFd < 0)
		{
			E("error=%s.", strerror(errno));
			return -1;
		}
	}

    D("success to open board_id_misc");
        return 0;
}

int board_id_close_device(void)
{
        D("enter.");

        if(sCtrlFd >= 0)
        {
	        close(sCtrlFd);
	        sCtrlFd = -1;
    	}

        return 0;
}

int board_id_get_locale_region(enum type_devices type, char *country_area, char *locale_language, char *locale_region, char *country_geo,  char *timezone, char *user_define) 
{
	int result = 0;
	struct area_id_name area_select;

	if(type != DEVICE_TYPE_AREA)	
	{
		E("type = %d, error", type);
		goto error;
	}

	if ( 0 > ioctl(sCtrlFd, BOARD_ID_IOCTL_READ_AREA_ID, &area_select) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}

    D("tid=%d, bid=%d, area=\"%s\", locale=\"%s\", region=\"%s\", timezone=\"%s\".",
        area_select.type,
        area_select.id,
        area_select.country_area,
		area_select.locale_language,
        area_select.locale_region,
        area_select.timezone);
	
	strcpy(country_area, area_select.country_area);
	strcpy(locale_language, area_select.locale_language);	
	strcpy(locale_region, area_select.locale_region);	
	strcpy(country_geo, area_select.country_geo);	
	strcpy(timezone, area_select.timezone);	
	strcpy(user_define, area_select.user_define);

	return result;

error:
	strcpy(country_area, "United_States");
	strcpy(locale_language, "en");	
	strcpy(locale_region, "US");	
	strcpy(country_geo, "no");	
	strcpy(timezone, "Atlantic/Azores");	
	strcpy(user_define, "no");
	
	return result;
}


int board_id_get_operator_name(enum type_devices type, char *locale_region, char *operator_name) 
{
	int result = 0;
	struct operator_id_name operator_select;

	if(type != DEVICE_TYPE_OPERATOR)	
	{
		E("type = %d is error.", type);
		goto error;
	}

	if ( 0 > ioctl(sCtrlFd, BOARD_ID_IOCTL_READ_OPERATOR_ID, &operator_select) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}

	strcpy(operator_name, operator_select.operator_name);	
	strcpy(locale_region, operator_select.locale_region);

	return result;

error:
	strcpy(operator_name, "CHINA_MOBILE");	
	strcpy(locale_region, "CN");
	return result;
}


int board_id_get_reserve_name(enum type_devices type, char *locale_region, char *reserve_name) 
{
	int result = 0;
	struct reserve_id_name reserve_select;

	if(type != DEVICE_TYPE_RESERVE)	
	{
		E("type = %d is error.", type);
		goto error;
	}

	if ( 0 > ioctl(sCtrlFd, BOARD_ID_IOCTL_READ_RESERVE_ID, &reserve_select) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}

	strcpy(reserve_name, reserve_select.reserve_name);	
	strcpy(locale_region, reserve_select.locale_region);

	return result;

error:
	strcpy(reserve_name, "no");	
	strcpy(locale_region, "CN");
	return result;
}



int board_id_get_device_name(enum type_devices type, char *type_name, char *dev_name)
{
	int result = 0;
	struct device_id_name device_selected_temp;
	int ioctl_cmd_temp = BOARD_ID_IOCTL_READ_TP_ID;

	if((type < DEVICE_TYPE_TP) || (type >= DEVICE_NUM_TYPES))
	{
		E("type = %d is error.", type);
		goto error;
	}

	switch(type)
	{	
		case DEVICE_TYPE_TP:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_TP_ID;
		break;
		
		case DEVICE_TYPE_LCD:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_LCD_ID;
		break;

		case DEVICE_TYPE_KEY:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_KEY_ID;
		break;
		
		case DEVICE_TYPE_CODEC:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_CODEC_ID;
		break;

		case DEVICE_TYPE_WIFI:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_WIFI_ID;
		break;

		case DEVICE_TYPE_BT:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_BT_ID;
		break;
		
		case DEVICE_TYPE_GPS:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_GPS_ID;
		break;
		
		case DEVICE_TYPE_FM:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_FM_ID;
		break;
		
		case DEVICE_TYPE_MODEM:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_MODEM_ID;
		break;
		
		case DEVICE_TYPE_DDR:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_DDR_ID;
		break;

		case DEVICE_TYPE_FLASH:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_FLASH_ID;
		break;

		case DEVICE_TYPE_HDMI:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_HDMI_ID;
		break;

		case DEVICE_TYPE_BATTERY:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_BATTERY_ID;
		break;

		case DEVICE_TYPE_CHARGE:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_CHARGE_ID;
		break;
		
		case DEVICE_TYPE_BACKLIGHT:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_BACKLIGHT_ID;
		break;

		case DEVICE_TYPE_HEADSET:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_HEADSET_ID;
		break;

		case DEVICE_TYPE_MICPHONE:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_MICPHONE_ID;
		break;

		case DEVICE_TYPE_SPEAKER:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SPEAKER_ID;
		break;

		case DEVICE_TYPE_VIBRATOR:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_VIBRATOR_ID;
		break;

		case DEVICE_TYPE_TV:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_TV_ID;
		break;
		
		case DEVICE_TYPE_ECHIP:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_ECHIP_ID;
		break;

		
		case DEVICE_TYPE_PMIC:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_PMIC_ID;
		break;

		case DEVICE_TYPE_REGULATOR:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_REGULATOR_ID;
		break;

		case DEVICE_TYPE_RTC:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_RTC_ID;
		break;

		case DEVICE_TYPE_CAMERA_FRONT:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_CAMERA_FRONT_ID;
		break;

		case DEVICE_TYPE_CAMERA_BACK:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_CAMERA_BACK_ID;
		break;

		case DEVICE_TYPE_ANGLE:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SENSOR_ANGLE_ID;
		break;

		case DEVICE_TYPE_ACCEL:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SENSOR_ACCEL_ID;
		break;

		case DEVICE_TYPE_COMPASS:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SENSOR_COMPASS_ID;
		break;

		case DEVICE_TYPE_GYRO:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SENSOR_GYRO_ID;
		break;

		case DEVICE_TYPE_LIGHT:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SENSOR_LIGHT_ID;
		break;

		case DEVICE_TYPE_PROXIMITY:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SENSOR_PROXIMITY_ID;
		break;

		case DEVICE_TYPE_TEMPERATURE:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SENSOR_TEMPERATURE_ID;
		break;
		
		case DEVICE_TYPE_PRESSURE:
		ioctl_cmd_temp = BOARD_ID_IOCTL_READ_SENSOR_PRESSURE_ID;
		break;

		default:
			D("type = %d, is error.", type);
			goto error;
	}

	if ( 0 > ioctl(sCtrlFd, ioctl_cmd_temp, &device_selected_temp) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}

	D("tid=%d, bid=%d, type=\"%s\", dev=\"%s\", desc=\"%s\"",
	    device_selected_temp.type,
        device_selected_temp.id,
        device_selected_temp.type_name,
		device_selected_temp.dev_name,
        device_selected_temp.description);

	strcpy(type_name, device_selected_temp.type_name);
	strcpy(dev_name, device_selected_temp.dev_name);

	return result;

error:
	strcpy(type_name, "gps");	
	strcpy(dev_name, "no_gps");
	return result;
};



int board_id_get_locale_region_by_id(enum type_devices type, char *id, char *country_area, char *locale_language, char *locale_region, char *country_geo,  char *timezone, char *user_define)
{
	int result = 0;
	struct area_id_name area_last_select;

	if(!id)
	{
		E("id is null");
		goto error;
	}	
		
	area_last_select.type = type;
	area_last_select.id = id[type];

	if(type != DEVICE_TYPE_AREA)
	{
		E("type = %d is error.", type);
		goto error;
	}

	if ( 0 > ioctl(sCtrlFd, BOARD_ID_IOCTL_READ_AREA_NAME_BY_ID, &area_last_select) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}

	if((area_last_select.id != (int)id[type]) || (area_last_select.type != (int)type))
	{
		E("type=%d,%d,id=%d,%d", area_last_select.type, type, area_last_select.id, id[type]);
		result = -1;
		goto error;
	}

	strcpy(country_area, area_last_select.country_area);
	strcpy(locale_language, area_last_select.locale_language);	
	strcpy(locale_region, area_last_select.locale_region);	
	strcpy(country_geo, area_last_select.country_geo);	
	strcpy(timezone, area_last_select.timezone); 
	strcpy(user_define, area_last_select.user_define);

	return result;

error:
	strcpy(country_area, "United_States");
	strcpy(locale_language, "en");	
	strcpy(locale_region, "US");	
	strcpy(country_geo, "no");	
	strcpy(timezone, "Atlantic/Azores");	
	strcpy(user_define, "no");

	return result;
}


int board_id_get_operator_name_by_id(enum type_devices type, char *id, char *locale_region, char *operator_name)
{
	int result = 0;
	struct operator_id_name operator_last_select;

	if(!id)
	{
		E("id is null.");
		goto error;
	}	

	operator_last_select.type = type;
	operator_last_select.id = ((id[type] << 8) | id[type+1]);

	if(type != DEVICE_TYPE_OPERATOR)
	{
		E("type = %d is error.", type);
		goto error;
	}

	if ( 0 > ioctl(sCtrlFd, BOARD_ID_IOCTL_READ_OPERATOR_NAME_BY_ID, &operator_last_select) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}

	if((operator_last_select.id != (int)((id[type] << 8) | id[type+1])) || (operator_last_select.type != (int)type))
	{
		E("type = %d, %d; id=%d, %d", operator_last_select.type, type, operator_last_select.id, ((id[type] << 8) | id[type+1]));
		result = -1;
		goto error;
	}

	strcpy(operator_name, operator_last_select.operator_name);	
	strcpy(locale_region, operator_last_select.locale_region);

	return result;

error:
	strcpy(operator_name, "CHINA_MOBILE");	
	strcpy(locale_region, "CN");
	return result;
}


int board_id_get_reserve_name_by_id(enum type_devices type, char *id, char *locale_region, char *reserve_name)
{
	int result = 0;
	struct reserve_id_name reserve_last_select;

	if(!id)
	{
		E("id is null");
		goto error;
	}	
		
	reserve_last_select.type = type;
	reserve_last_select.id = id[type];

	if(type != DEVICE_TYPE_RESERVE)
	{
		E("type = %d is error.", type);
		goto error;
	}

	if ( 0 > ioctl(sCtrlFd, BOARD_ID_IOCTL_READ_RESERVE_NAME_BY_ID, &reserve_last_select) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}

	if((reserve_last_select.id != (int)id[type]) || (reserve_last_select.type != (int)type))
	{
		E("type=%d,%d; id=%d,%d", reserve_last_select.type, type, reserve_last_select.id, id[type]);
		result = -1;
		goto error;
	}

	strcpy(reserve_name, reserve_last_select.reserve_name);	
	strcpy(locale_region, reserve_last_select.locale_region);

	return result;

error:
	strcpy(reserve_name, "no");	
	strcpy(locale_region, "CN");
	return result;
}




int board_id_get_device_name_by_id(enum type_devices type, char *id, char *type_name, char *dev_name)
{
	int result = 0;
	struct device_id_name device_selected_temp;

	if(!id)
	{
		D("id is null");
		goto error;
	}

	device_selected_temp.type = type;
	device_selected_temp.id = id[type];

	if((type < DEVICE_TYPE_TP) || (type >= DEVICE_NUM_TYPES))
	{
		E("unexpected 'type' = %d.", type);
		goto error;
	}
	
	int ioctl_cmd_temp = BOARD_ID_IOCTL_READ_DEVICE_NAME_BY_ID;

	if ( 0 > ioctl(sCtrlFd, ioctl_cmd_temp, &device_selected_temp) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}

	if((device_selected_temp.type != (int)type) || (device_selected_temp.id != (int)id[type]) )
	{
		E("type=%d,%d,id=%d,%d", device_selected_temp.type, type, device_selected_temp.id, id[type]);
		result = -1;
		goto error;
	}	

	D("id=%d, device_id=%d, dev_name=%s, type=%d, type_name=%s",
	    device_selected_temp.id,
        device_selected_temp.device_id,
        device_selected_temp.dev_name,
		device_selected_temp.type,
        device_selected_temp.type_name);

	strcpy(type_name, device_selected_temp.type_name);
	strcpy(dev_name, device_selected_temp.dev_name);

	
	return result;

error:
	strcpy(type_name, "gps");	
	strcpy(dev_name, "no_gps");
	return result;

}


int board_id_get(char *id)
{	
	int result = 0;
	int ioctl_cmd_temp = BOARD_ID_IOCTL_READ_VENDOR_DATA;

	if(!id)
	{
		E("id is null.");
		goto error;
	}

	if ( 0 > ioctl(sCtrlFd, ioctl_cmd_temp, id) )
	{
		E("fail to ioctl, error = %s.", strerror(errno));
		result = -1;
		goto error;
	}
	
	return result;
	
error:
	strcpy(id, "123456789");
	return result;
}


