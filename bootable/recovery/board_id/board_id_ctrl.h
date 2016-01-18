/*
 * board_id_ctrl.h
 *
 *  Created on: 2013-4-27
 *      Author: mmk
 */

#ifndef BOARD_ID_CTRL_H_
#define BOARD_ID_CTRL_H_
#include <sys/ioctl.h>


enum type_devices{
	DEVICE_TYPE_NULL = 0x0,	

	DEVICE_TYPE_SUM = 0x20,	
	DEVICE_TYPE_AREA = 0x24,	//
	DEVICE_TYPE_OPERATOR = 0x25,	
	DEVICE_TYPE_OPERATOR2 = 0x26,
	DEVICE_TYPE_RESERVE = 0x27,	
	DEVICE_TYPE_STATUS = 0x28,

	DEVICE_TYPE_TP = 0x29,		//one byte size
	DEVICE_TYPE_LCD,	
	DEVICE_TYPE_KEY,	
	DEVICE_TYPE_CODEC,
	DEVICE_TYPE_WIFI,
	DEVICE_TYPE_BT,	
	DEVICE_TYPE_GPS,	
	DEVICE_TYPE_FM,	
	DEVICE_TYPE_MODEM,	
	DEVICE_TYPE_DDR,
	DEVICE_TYPE_FLASH,
	DEVICE_TYPE_HDMI,
	DEVICE_TYPE_BATTERY,
	DEVICE_TYPE_CHARGE,	
	DEVICE_TYPE_BACKLIGHT,
	DEVICE_TYPE_HEADSET,
	DEVICE_TYPE_MICPHONE,
	DEVICE_TYPE_SPEAKER,
	DEVICE_TYPE_VIBRATOR,
	DEVICE_TYPE_TV,	
	DEVICE_TYPE_ECHIP,	//30
	DEVICE_TYPE_HUB,
	DEVICE_TYPE_TPAD,
	
	DEVICE_TYPE_PMIC,
	DEVICE_TYPE_REGULATOR,
	DEVICE_TYPE_RTC,
	DEVICE_TYPE_CAMERA_FRONT,
	DEVICE_TYPE_CAMERA_BACK,	//35
	DEVICE_TYPE_ANGLE,
	DEVICE_TYPE_ACCEL,
	DEVICE_TYPE_COMPASS,
	DEVICE_TYPE_GYRO,
	DEVICE_TYPE_LIGHT,
	DEVICE_TYPE_PROXIMITY,
	DEVICE_TYPE_TEMPERATURE,	
	DEVICE_TYPE_PRESSURE,
	
	DEVICE_NUM_TYPES,
};


enum id_language{
	LANGUAGE_ID_NULL,
	LANGUAGE_ID_EN,// 英文
	LANGUAGE_ID_EN_US,// 英文 (美国)
	LANGUAGE_ID_AR,// 阿拉伯文
	LANGUAGE_ID_AR_AE,// 阿拉伯文 (阿拉伯联合酋长国)
	LANGUAGE_ID_AR_BH,// 阿拉伯文 (巴林)
	LANGUAGE_ID_AR_DZ,// 阿拉伯文 (阿尔及利亚)
	LANGUAGE_ID_AR_EG,// 阿拉伯文 (埃及)
	LANGUAGE_ID_AR_IQ,// 阿拉伯文 (伊拉克)
	LANGUAGE_ID_AR_JO,// 阿拉伯文 (约旦)
	LANGUAGE_ID_AR_KW,// 阿拉伯文 (科威特)
	LANGUAGE_ID_AR_LB,// 阿拉伯文 (黎巴嫩)
	LANGUAGE_ID_AR_LY,// 阿拉伯文 (利比亚)
	LANGUAGE_ID_AR_MA,// 阿拉伯文 (摩洛哥)
	LANGUAGE_ID_AR_OM,// 阿拉伯文 (阿曼)
	LANGUAGE_ID_AR_QA,// 阿拉伯文 (卡塔尔)
	LANGUAGE_ID_AR_SA,// 阿拉伯文 (沙特阿拉伯)
	LANGUAGE_ID_AR_SD,// 阿拉伯文 (苏丹)
	LANGUAGE_ID_AR_SY,// 阿拉伯文 (叙利亚)
	LANGUAGE_ID_AR_TN,// 阿拉伯文 (突尼斯)
	LANGUAGE_ID_AR_YE,// 阿拉伯文 (也门)
	LANGUAGE_ID_BE,// 白俄罗斯文
	LANGUAGE_ID_BE_BY,// 白俄罗斯文 (白俄罗斯)
	LANGUAGE_ID_BG,// 保加利亚文
	LANGUAGE_ID_BG_BG,// 保加利亚文 (保加利亚)
	LANGUAGE_ID_CA,// 加泰罗尼亚文
	LANGUAGE_ID_CA_ES,// 加泰罗尼亚文 (西班牙)
	LANGUAGE_ID_CA_ES_EURO,// 加泰罗尼亚文 (西班牙,EURO)
	LANGUAGE_ID_CS,// 捷克文
	LANGUAGE_ID_CS_CZ,// 捷克文 (捷克共和国)
	LANGUAGE_ID_DA,// 丹麦文
	LANGUAGE_ID_DA_DK,// 丹麦文 (丹麦)
	LANGUAGE_ID_DE,// 德文
	LANGUAGE_ID_DE_AT,// 德文 (奥地利)
	LANGUAGE_ID_DE_AT_EURO,// 德文 (奥地利,EURO)
	LANGUAGE_ID_DE_CH,// 德文 (瑞士)
	LANGUAGE_ID_DE_DE,// 德文 (德国)
	LANGUAGE_ID_DE_DE_EURO,// 德文 (德国,EURO)
	LANGUAGE_ID_DE_LU,// 德文 (卢森堡)
	LANGUAGE_ID_DE_LU_EURO,// 德文 (卢森堡,EURO)
	LANGUAGE_ID_EL,// 希腊文
	LANGUAGE_ID_EL_GR,// 希腊文 (希腊)
	LANGUAGE_ID_EN_AU,// 英文 (澳大利亚)
	LANGUAGE_ID_EN_CA,// 英文 (加拿大)
	LANGUAGE_ID_EN_GB,// 英文 (英国)
	LANGUAGE_ID_EN_IE,// 英文 (爱尔兰)
	LANGUAGE_ID_EN_IE_EURO,// 英文 (爱尔兰,EURO)
	LANGUAGE_ID_EN_NZ,// 英文 (新西兰)
	LANGUAGE_ID_EN_ZA,// 英文 (南非)
	LANGUAGE_ID_ES,// 西班牙文
	LANGUAGE_ID_ES_BO,// 西班牙文 (玻利维亚)
	LANGUAGE_ID_ES_AR,// 西班牙文 (阿根廷)
	LANGUAGE_ID_ES_CL,// 西班牙文 (智利)
	LANGUAGE_ID_ES_CO,// 西班牙文 (哥伦比亚)
	LANGUAGE_ID_ES_CR,// 西班牙文 (哥斯达黎加)
	LANGUAGE_ID_ES_DO,// 西班牙文 (多米尼加共和国)
	LANGUAGE_ID_ES_EC,// 西班牙文 (厄瓜多尔)
	LANGUAGE_ID_ES_ES,// 西班牙文 (西班牙)
	LANGUAGE_ID_ES_ES_EURO,// 西班牙文 (西班牙,EURO)
	LANGUAGE_ID_ES_GT,// 西班牙文 (危地马拉)
	LANGUAGE_ID_ES_HN,// 西班牙文 (洪都拉斯)
	LANGUAGE_ID_ES_MX,// 西班牙文 (墨西哥)
	LANGUAGE_ID_ES_NI,// 西班牙文 (尼加拉瓜)
	LANGUAGE_ID_ET,// 爱沙尼亚文
	LANGUAGE_ID_ES_PA,// 西班牙文 (巴拿马)
	LANGUAGE_ID_ES_PE,// 西班牙文 (秘鲁)
	LANGUAGE_ID_ES_PR,// 西班牙文 (波多黎哥)
	LANGUAGE_ID_ES_PY,// 西班牙文 (巴拉圭)
	LANGUAGE_ID_ES_SV,// 西班牙文 (萨尔瓦多)
	LANGUAGE_ID_ES_UY,// 西班牙文 (乌拉圭)
	LANGUAGE_ID_ES_VE,// 西班牙文 (委内瑞拉)
	LANGUAGE_ID_ET_EE,// 爱沙尼亚文 (爱沙尼亚)
	LANGUAGE_ID_FI,// 芬兰文
	LANGUAGE_ID_FI_FI,// 芬兰文 (芬兰)
	LANGUAGE_ID_FI_FI_EURO,// 芬兰文 (芬兰,EURO)
	LANGUAGE_ID_FR,// 法文
	LANGUAGE_ID_FR_BE,// 法文 (比利时)
	LANGUAGE_ID_FR_BE_EURO,// 法文 (比利时,EURO)
	LANGUAGE_ID_FR_CA,// 法文 (加拿大)
	LANGUAGE_ID_FR_CH,// 法文 (瑞士)
	LANGUAGE_ID_FR_FR,// 法文 (法国)
	LANGUAGE_ID_FR_FR_EURO,// 法文 (法国,EURO)
	LANGUAGE_ID_FR_LU,// 法文 (卢森堡)
	LANGUAGE_ID_FR_LU_EURO,// 法文 (卢森堡,EURO)
	LANGUAGE_ID_HR,// 克罗地亚文
	LANGUAGE_ID_HR_HR,// 克罗地亚文 (克罗地亚)
	LANGUAGE_ID_HU,// 匈牙利文
	LANGUAGE_ID_HU_HU,// 匈牙利文 (匈牙利)
	LANGUAGE_ID_IS,// 冰岛文
	LANGUAGE_ID_IS_IS,// 冰岛文 (冰岛)
	LANGUAGE_ID_IT,// 意大利文
	LANGUAGE_ID_IT_CH,// 意大利文 (瑞士)
	LANGUAGE_ID_IT_IT,// 意大利文 (意大利)
	LANGUAGE_ID_IT_IT_EURO,// 意大利文 (意大利,EURO)
	LANGUAGE_ID_IW,// 希伯来文
	LANGUAGE_ID_IW_IL,// 希伯来文 (以色列)
	LANGUAGE_ID_JA,// 日文
	LANGUAGE_ID_JA_JP,// 日文 (日本)
	LANGUAGE_ID_KO,// 朝鲜文
	LANGUAGE_ID_KO_KR,// 朝鲜文 (南朝鲜)
	LANGUAGE_ID_LT,// 立陶宛文
	LANGUAGE_ID_LT_LT,// 立陶宛文 (立陶宛)
	LANGUAGE_ID_LV,// 拉托维亚文(列托)
	LANGUAGE_ID_LV_LV,// 拉托维亚文(列托) (拉脱维亚)
	LANGUAGE_ID_MK,// 马其顿文
	LANGUAGE_ID_MK_MK,// 马其顿文 (马其顿王国)
	LANGUAGE_ID_NL,// 荷兰文
	LANGUAGE_ID_NL_BE,// 荷兰文 (比利时)
	LANGUAGE_ID_NL_BE_EURO,// 荷兰文 (比利时,EURO)
	LANGUAGE_ID_NL_NL,// 荷兰文 (荷兰)
	LANGUAGE_ID_NL_NL_EURO,// 荷兰文 (荷兰,EURO)
	LANGUAGE_ID_NO,// 挪威文
	LANGUAGE_ID_NO_NO,// 挪威文 (挪威)
	LANGUAGE_ID_NO_NO_NY,// 挪威文 (挪威,NYNORSK)
	LANGUAGE_ID_PL,// 波兰文
	LANGUAGE_ID_PL_PL,// 波兰文 (波兰)
	LANGUAGE_ID_PT,// 葡萄牙文
	LANGUAGE_ID_PT_BR,// 葡萄牙文 (巴西)
	LANGUAGE_ID_PT_PT,// 葡萄牙文 (葡萄牙)
	LANGUAGE_ID_PT_PT_EURO,// 葡萄牙文 (葡萄牙,EURO)
	LANGUAGE_ID_RO,// 罗马尼亚文
	LANGUAGE_ID_RO_RO,// 罗马尼亚文 (罗马尼亚)
	LANGUAGE_ID_RU,// 俄文
	LANGUAGE_ID_RU_RU,// 俄文 (俄罗斯)
	LANGUAGE_ID_SH,// 塞波尼斯-克罗地亚文
	LANGUAGE_ID_SH_YU,// 塞波尼斯-克罗地亚文 (南斯拉夫)
	LANGUAGE_ID_SK,// 斯洛伐克文
	LANGUAGE_ID_SK_SK,// 斯洛伐克文 (斯洛伐克)
	LANGUAGE_ID_SL,// 斯洛文尼亚文
	LANGUAGE_ID_SL_SI,// 斯洛文尼亚文 (斯洛文尼亚)
	LANGUAGE_ID_SQ,// 阿尔巴尼亚文
	LANGUAGE_ID_SQ_AL,// 阿尔巴尼亚文 (阿尔巴尼亚)
	LANGUAGE_ID_SR,// 塞尔维亚文
	LANGUAGE_ID_SR_YU,// 塞尔维亚文 (南斯拉夫)
	LANGUAGE_ID_SV,// 瑞典文
	LANGUAGE_ID_SV_SE,// 瑞典文 (瑞典)
	LANGUAGE_ID_TH,// 泰文
	LANGUAGE_ID_TH_TH,// 泰文 (泰国)
	LANGUAGE_ID_TR,// 土耳其文
	LANGUAGE_ID_TR_TR,// 土耳其文 (土耳其)
	LANGUAGE_ID_UK,// 乌克兰文
	LANGUAGE_ID_UK_UA,// 乌克兰文 (乌克兰)
	LANGUAGE_ID_ZH,// 中文
	LANGUAGE_ID_ZH_CN,// 中文 (中国)
	LANGUAGE_ID_ZH_HK,// 中文 (香港)
	LANGUAGE_ID_ZH_TW,// 中文 (台湾)
	LANGUAGE_ID_NUMS,
};

struct area_id_name{
	int type;
	int id;
	char country_area[32];		//country or area name such as china
	char locale_language[4];	//locale language name such as zh
	char locale_region[8];		//locale region name such as CN
	char country_geo[20];		//country geographical position such as asia		
	char timezone[32];		//time zone such as Asia/Shanghai
	char user_define[20];		//user-defined name such as A10,A12,A13 
};


struct operator_id_name{
	int type;		//type
	int id;	
	char operator_name[20];	//operator name such as CHINA MOBILE
	char locale_region[8];	//area name such as CN
};

struct reserve_id_name{
	int type;		//type
	int id;			
	char reserve_name[20];	//reserve name	
	char locale_region[20];	
};


struct device_id_name{
	char type;	//device type
	char id;	//board id
	char type_name[14];
	char driver_name[16];
	char dev_name[16];	//name
	char description[30];	// description
	unsigned short device_id;//device_id and only one
	//short select;	// 1:device is selected 0:not
};



#define BOARD_ID_IOCTL_BASE 'b'

//#define BOARD_ID_IOCTL_READ_ALL 			_IOWR(BOARD_ID_IOCTL_BASE, 0x00, struct board_id_private_data)
//#define BOARD_ID_IOCTL_WRITE_ALL 			_IOWR(BOARD_ID_IOCTL_BASE, 0x30, struct board_id_private_data)


#define BOARD_ID_IOCTL_READ_AREA_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x80, struct area_id_name)
#define BOARD_ID_IOCTL_READ_OPERATOR_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x81, struct operator_id_name)
#define BOARD_ID_IOCTL_READ_RESERVE_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x82, struct reserve_id_name)

#define BOARD_ID_IOCTL_READ_AREA_NAME_BY_ID 		_IOWR(BOARD_ID_IOCTL_BASE, 0x70, struct area_id_name)
#define BOARD_ID_IOCTL_READ_OPERATOR_NAME_BY_ID 	_IOWR(BOARD_ID_IOCTL_BASE, 0x71, struct operator_id_name)
#define BOARD_ID_IOCTL_READ_RESERVE_NAME_BY_ID 		_IOWR(BOARD_ID_IOCTL_BASE, 0x72, struct reserve_id_name)
#define BOARD_ID_IOCTL_READ_DEVICE_NAME_BY_ID 		_IOWR(BOARD_ID_IOCTL_BASE, 0x73, struct device_id_name)



#define BOARD_ID_IOCTL_READ_TP_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x01, struct device_id_name)
#define BOARD_ID_IOCTL_READ_LCD_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x02, struct device_id_name)
#define BOARD_ID_IOCTL_READ_KEY_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x03, struct device_id_name)
#define BOARD_ID_IOCTL_READ_CODEC_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x04, struct device_id_name)
#define BOARD_ID_IOCTL_READ_WIFI_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x05, struct device_id_name)
#define BOARD_ID_IOCTL_READ_BT_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x06, struct device_id_name)	
#define BOARD_ID_IOCTL_READ_GPS_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x07, struct device_id_name)
#define BOARD_ID_IOCTL_READ_FM_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x08, struct device_id_name)
#define BOARD_ID_IOCTL_READ_MODEM_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x09, struct device_id_name)	
#define BOARD_ID_IOCTL_READ_DDR_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x0a, struct device_id_name)
#define BOARD_ID_IOCTL_READ_FLASH_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x0b, struct device_id_name)
#define BOARD_ID_IOCTL_READ_HDMI_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x0c, struct device_id_name)
#define BOARD_ID_IOCTL_READ_BATTERY_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x0d, struct device_id_name)
#define BOARD_ID_IOCTL_READ_CHARGE_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x0e, struct device_id_name)	
#define BOARD_ID_IOCTL_READ_BACKLIGHT_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x0f, struct device_id_name)
#define BOARD_ID_IOCTL_READ_HEADSET_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x10, struct device_id_name)
#define BOARD_ID_IOCTL_READ_MICPHONE_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x11, struct device_id_name)
#define BOARD_ID_IOCTL_READ_SPEAKER_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x12, struct device_id_name)
#define BOARD_ID_IOCTL_READ_VIBRATOR_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x13, struct device_id_name)
#define BOARD_ID_IOCTL_READ_TV_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x14, struct device_id_name)
#define BOARD_ID_IOCTL_READ_ECHIP_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x15, struct device_id_name)		
#define BOARD_ID_IOCTL_READ_HUB_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x16, struct device_id_name)	
#define BOARD_ID_IOCTL_READ_TPAD_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x17, struct device_id_name)	


#define BOARD_ID_IOCTL_READ_PMIC_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x20, struct device_id_name)
#define BOARD_ID_IOCTL_READ_REGULATOR_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x21, struct device_id_name)
#define BOARD_ID_IOCTL_READ_RTC_ID 			_IOR(BOARD_ID_IOCTL_BASE, 0x22, struct device_id_name)
#define BOARD_ID_IOCTL_READ_CAMERA_FRONT_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x23, struct device_id_name)
#define BOARD_ID_IOCTL_READ_CAMERA_BACK_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x24, struct device_id_name)	
#define BOARD_ID_IOCTL_READ_SENSOR_ANGLE_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x25, struct device_id_name)
#define BOARD_ID_IOCTL_READ_SENSOR_ACCEL_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x26, struct device_id_name)
#define BOARD_ID_IOCTL_READ_SENSOR_COMPASS_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x27, struct device_id_name)
#define BOARD_ID_IOCTL_READ_SENSOR_GYRO_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x28, struct device_id_name)
#define BOARD_ID_IOCTL_READ_SENSOR_LIGHT_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x29, struct device_id_name)
#define BOARD_ID_IOCTL_READ_SENSOR_PROXIMITY_ID 	_IOR(BOARD_ID_IOCTL_BASE, 0x2A, struct device_id_name)
#define BOARD_ID_IOCTL_READ_SENSOR_TEMPERATURE_ID 	_IOR(BOARD_ID_IOCTL_BASE, 0x2B, struct device_id_name)	
#define BOARD_ID_IOCTL_READ_SENSOR_PRESSURE_ID 		_IOR(BOARD_ID_IOCTL_BASE, 0x2C, struct device_id_name)


#define BOARD_ID_IOCTL_WRITE_AREA_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x90, struct area_id_name)
#define BOARD_ID_IOCTL_WRITE_OPERATOR_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x91, struct operator_id_name)
#define BOARD_ID_IOCTL_WRITE_RESERVE_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x92, struct reserve_id_name)


#define BOARD_ID_IOCTL_WRITE_TP_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x31, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_LCD_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x32, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_KEY_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x33, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_CODEC_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x34, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_WIFI_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x35, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_BT_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x36, struct device_id_name)	
#define BOARD_ID_IOCTL_WRITE_GPS_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x37, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_FM_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x38, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_MODEM_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x39, struct device_id_name)	
#define BOARD_ID_IOCTL_WRITE_DDR_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x3a, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_FLASH_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x3b, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_HDMI_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x3c, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_BATTERY_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x3d, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_CHARGE_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x3e, struct device_id_name)	
#define BOARD_ID_IOCTL_WRITE_BACKLIGHT_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x3f, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_HEADSET_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x40, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_MICPHONE_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x41, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_SPEAKER_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x42, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_VIBRATOR_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x43, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_TV_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x44, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_ECHIP_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x45, struct device_id_name)		
#define BOARD_ID_IOCTL_WRITE_HUB_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x46, struct device_id_name)		
#define BOARD_ID_IOCTL_WRITE_TPAD_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x47, struct device_id_name)		


#define BOARD_ID_IOCTL_WRITE_PMIC_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x50, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_REGULATOR_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x51, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_RTC_ID 			_IOW(BOARD_ID_IOCTL_BASE, 0x52, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_CAMERA_FRONT_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x53, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_CAMERA_BACK_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x54, struct device_id_name)	
#define BOARD_ID_IOCTL_WRITE_SENSOR_ANGLE_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x55, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_SENSOR_ACCEL_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x56, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_SENSOR_COMPASS_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x57, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_SENSOR_GYRO_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x58, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_SENSOR_LIGHT_ID 		_IOW(BOARD_ID_IOCTL_BASE, 0x59, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_SENSOR_PROXIMITY_ID 	_IOW(BOARD_ID_IOCTL_BASE, 0x5A, struct device_id_name)
#define BOARD_ID_IOCTL_WRITE_SENSOR_TEMPERATURE_ID 	_IOW(BOARD_ID_IOCTL_BASE, 0x5B, struct device_id_name)	
#define BOARD_ID_IOCTL_WRITE_SENSOR_PRESSURE_ID 	_IOW(BOARD_ID_IOCTL_BASE, 0x5C, struct device_id_name)

#define BOARD_ID_IOCTL_WRITE_AREA_FLASH 		_IOW(BOARD_ID_IOCTL_BASE, 0x60, struct area_id_name)
#define BOARD_ID_IOCTL_WRITE_DEVICE_FLASH 		_IOW(BOARD_ID_IOCTL_BASE, 0x61, struct device_id_name)
#define BOARD_ID_IOCTL_READ_STATUS 			_IOR(BOARD_ID_IOCTL_BASE, 0x62,	char)
#define BOARD_ID_IOCTL_READ_VENDOR_DATA 		_IOR(BOARD_ID_IOCTL_BASE, 0x63,	char[DEVICE_NUM_TYPES])


int board_id_open_device(void);
int board_id_close_device(void);

int board_id_get_locale_region(enum type_devices type, char *country_area, char *locale_language, char *locale_region, char *country_geo,  char *timezone, char *user_define);
int board_id_get_operator_name(enum type_devices type, char *locale_region, char *operator_name);
int board_id_get_reserve_name(enum type_devices type, char *locale_region, char *reserve_name);
int board_id_get_device_name(enum type_devices type, char *type_name, char *dev_name);

int board_id_get_locale_region_by_id(enum type_devices type, char *id, char *country_area, char *locale_language, char *locale_region, char *country_geo,  char *timezone, char *user_define);
int board_id_get_operator_name_by_id(enum type_devices type, char *id, char *locale_region, char *operator_name);
int board_id_get_reserve_name_by_id(enum type_devices type, char *id, char *locale_region, char *reserve_name);
int board_id_get_device_name_by_id(enum type_devices type, char *id, char *type_name, char *dev_name);
int board_id_get(char *id);
#endif /* BOARD_ID_CTRL_H_ */
