#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/android_alarm.h>
#include <linux/rtc.h>
#include <termios.h>

#include <stdlib.h>

#include "test_case.h"
#include "at_util_extern.h"
#include "language.h"

static pthread_mutex_t s_commandmutexExtern = PTHREAD_MUTEX_INITIALIZER;
int gFd;
static pthread_t s_tid_reader_extern;
#define MAX_AT_RESPONSE 1024
static char s_ATBufferExtern[MAX_AT_RESPONSE+1];
static char *s_ATBufferExternCur = s_ATBufferExtern;
static int s_readCount = 0;
static int is_sim_test_done = 0;
int isAtResponseExtern = 0;
int isResponseSucExtern = 0;
int isAtSendedExtern = 0;
char gAtAck_command_extern[20]={"\0"};
char returnResult[30]={"\0"};
extern int rf_cal_result;
extern int wifi_cal_result;
extern char imei_result[50];
extern int UI_LEVEL;
extern char ISMI1[30];
extern char ISMI2[30];
extern int simcard1;
extern int simcard2;
extern int simCounts;
extern int reboot_normal;
static int start_change_bootmode = 0;

#define LOG(x...) printf("[At_UTIL_EXTERN] "x)

enum bp_id{ 
	BP_ID_INVALID = 0,//no bp 
	BP_ID_MT6229,  //USI MT6229 WCDMA 
	BP_ID_MU509,  //huawei MU509 WCDMA 
	BP_ID_MI700,  //thinkwill MI700 WCDMA 
	BP_ID_MW100,  //thinkwill MW100 WCDMA 
	BP_ID_TD8801,  //spreadtrum SC8803 TD-SCDMA 
	BP_ID_SC6610, //spreadtrum SC6610 GSM 
	BP_ID_M50,    //spreadtrum RDA GSM
	BP_ID_MT6250,   //8 ZINN M50  EDGE
	BP_ID_C66A,     //9 ZHIGUAN C66A GSM
	BP_ID_SEW290,   //10 SCV SEW290 WCDMA
	BP_ID_U5501,    //11 LONGSUNG U5501 WCDMA
	BP_ID_U7501,    //12 LONGSUNG U7501 WCDMA/HSPA+
	BP_ID_AW706,    //13 ANICARE AW706 EDGE
	BP_ID_A85XX,    //14 LONGSUNG A8520/A8530 GSM

	BP_ID_NUM, 
};

int get_is_sim_test_done_extern(void)
{
	return is_sim_test_done;
}

void set_is_sim_test_done_extern(int is_done)
{
	is_sim_test_done = is_done;
}

static char * findNextEOLExtern(char *cur)
{
    if (cur[0] == '>' && cur[1] == ' ' && cur[2] == '\0') {
        /* SMS prompt character...not \r terminated */
        return cur+2;
    }

    // Find next newline
    while (*cur != '\0' && *cur != '\r' && *cur != '\n') cur++;

    return *cur == '\0' ? NULL : cur;
}

static const char *readlineExtern()
{
    int count;

    char *p_read = NULL;
    char *p_eol = NULL;
    char *ret;

    if (*s_ATBufferExternCur == '\0') {

        s_ATBufferExternCur = s_ATBufferExtern;
        *s_ATBufferExternCur = '\0';
        p_read = s_ATBufferExtern;
    } else { 
        while (*s_ATBufferExternCur == '\r' || *s_ATBufferExternCur == '\n')
            s_ATBufferExternCur++;

        p_eol = findNextEOLExtern(s_ATBufferExternCur);

        if (p_eol == NULL) {
            size_t len;

            len = strlen(s_ATBufferExternCur);

            memmove(s_ATBufferExtern, s_ATBufferExternCur, len + 1);
            p_read = s_ATBufferExtern + len;
            s_ATBufferExternCur = s_ATBufferExtern;
        }
    }

    while (p_eol == NULL) {
        if (0 == MAX_AT_RESPONSE - (p_read - s_ATBufferExtern)) {
            //LOGE("ERROR: Input line exceeded buffer\n");
            s_ATBufferExternCur = s_ATBufferExtern;
            *s_ATBufferExternCur = '\0';
            p_read = s_ATBufferExtern;
        }

        do {
        	//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"start to read\n");
            count = read(gFd, p_read,
                            MAX_AT_RESPONSE - (p_read - s_ATBufferExtern));
        } while (count < 0 && errno == EINTR);

        if (count > 0) {
            //AT_DUMP( "<< ", p_read, count );
            s_readCount += count;
//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"count:%d\n",count);
            p_read[count] = '\0';

            while (*s_ATBufferExternCur == '\r' || *s_ATBufferExternCur == '\n')
                s_ATBufferExternCur++;

            p_eol = findNextEOLExtern(s_ATBufferExternCur);
            p_read += count;
        } else if (count <= 0) {
            if(count == 0) {
                //LOGD("atchannel: EOF reached");
            } else {
                //LOGD("atchannel: read error %s", strerror(errno));
            }
            return NULL;
        }
    }


    ret = s_ATBufferExternCur;
    *p_eol = '\0';
    s_ATBufferExternCur = p_eol + 1; 

    //LOGD("AT< %s\n", ret);
    return ret;
}

static void *readerLoopExtern(void *arg){
	 const char  *line;
	int count;
	char sread;
	int cur=0;

	while(1){
		LOG("%s line=%d  reader loop  \n", __FUNCTION__, __LINE__);
		if(get_is_sim_test_done_extern())
		{
			LOG("%s line=%d sim_text_is_done! exit readerLoopExtern \n", __FUNCTION__, __LINE__);
			return NULL;
		}
		line = readlineExtern();
		pthread_mutex_lock(&s_commandmutexExtern);
		
		if(isAtSendedExtern){
			LOG("%s line=%d  readline=%s, gAtAck_command_extern=%s\n", __FUNCTION__, __LINE__, line, gAtAck_command_extern);
			if(line != NULL){
				if(line != NULL && strstr(line, gAtAck_command_extern)!=NULL){
					line = readlineExtern();
					LOG("%s line=%d readline=%s  \n", __FUNCTION__, __LINE__, line);
					if(line != NULL && strstr(line,"ERROR") == NULL){
						sprintf(returnResult,"%s",line);
						isResponseSucExtern = 1;
						isAtResponseExtern = 1;
						isAtSendedExtern =0;
					} else if(line != NULL && strstr(line,"ERROR") != NULL) {
						isResponseSucExtern = 0;
						isAtResponseExtern = 1;
						isAtSendedExtern =0;
					}
  				}
	  		
			}
		}
		
		pthread_mutex_unlock(&s_commandmutexExtern);


	}
	return NULL;
}

int at_send_extern(int fd,char *send_command)
{
	int cur = 0;
	int len = strlen(send_command);
	int written;
	int i = 3;
	int j=0;
	for(j=0;j<3;j++){
		
		pthread_mutex_lock(&s_commandmutexExtern);
		
		isResponseSucExtern = 0;
		isAtResponseExtern = 0;
		isAtSendedExtern =0;
		cur = 0;
		while (cur < len) {
			written = write (fd, send_command + cur, len - cur);
			if (written < 0) {
				//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"at_send_extern_error\n");
				pthread_mutex_unlock(&s_commandmutexExtern);
				return -1;
			}
			cur += written;
		}
		isAtSendedExtern = 1;
		
		pthread_mutex_unlock(&s_commandmutexExtern);
		
		i = 3;
	  while(1){
		sleep(1);
		if(isAtResponseExtern && isResponseSucExtern ){
			isResponseSucExtern = 0;
			isAtResponseExtern = 0;
			isAtSendedExtern =0;
			return 1;
		}else  if(isAtResponseExtern && isResponseSucExtern==0){
			isResponseSucExtern = 0;
			isAtResponseExtern = 0;
			isAtSendedExtern =0;
			break;
		}
		
		i--;
		if(i<=0){
			isResponseSucExtern = 0;
			isAtResponseExtern = 0;
			isAtSendedExtern =0;
			break;
		}
	  }
	}
	return -1;
}

int test_simcard()
{
	LOG("%s line=%d\n", __FUNCTION__, __LINE__);
	
	strncpy(gAtAck_command_extern, "AT+CFUN=1", 9);
	gAtAck_command_extern[9] = '\0';
	if(at_send_extern(gFd,"AT+CFUN=1\r\n") < 0){
		LOG("%s line=%d execute AT+CFUN=1 fail\n", __FUNCTION__, __LINE__);
		goto ERROR;
	}
	
	strncpy(gAtAck_command_extern, "AT+XSIMSEL=0", 12);
	gAtAck_command_extern[12] = '\0';
	if(at_send_extern(gFd,"AT+XSIMSEL=0\r\n") < 0){
		LOG("%s line=%d execute AT+XSIMSEL=0 fail\n", __FUNCTION__, __LINE__);
		goto ERROR;
	}

	strncpy(gAtAck_command_extern, "AT+CIMI", 7);
	gAtAck_command_extern[7] = '\0';
	if(at_send_extern(gFd,"AT+CIMI\r\n") >= 0) {
		simcard1 = 1;
		LOG("ISMI1 result is %s\n", returnResult);
		sprintf(ISMI1,"%s", returnResult);
	}
	else {
		LOG("%s line=%d execute AT+CIMI fail\n", __FUNCTION__, __LINE__);
		goto ERROR;
	}

	strncpy(gAtAck_command_extern, "AT+XSIMSEL=1", 12);
	gAtAck_command_extern[12] = '\0';
	if(at_send_extern(gFd,"AT+XSIMSEL=1\r\n") < 0) {
		LOG("%s line=%d execute AT+XSIMSEL=1 fail\n", __FUNCTION__, __LINE__);
		goto ERROR;
	}
	
	strncpy(gAtAck_command_extern, "AT+CIMI", 7);
	gAtAck_command_extern[7] = '\0';
	if(at_send_extern(gFd,"AT+CIMI\r\n") >= 0) {
		simcard2 = 1;
		LOG("ISMI2 result is %s\n", returnResult);
		sprintf(ISMI2,"%s", returnResult);
	}
	else
	{
		if(simCounts == 2) {
			LOG("%s line=%d execute AT+CIMI fail\n", __FUNCTION__, __LINE__);
			goto ERROR;
		}
	}

	return 0;

ERROR:
	strncpy(gAtAck_command_extern, "UTA_MODE_CALIBRATION", 19);
	gAtAck_command_extern[19] = '\0';
	if(at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n") < 0) {
		at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n");
	}
	//set_is_sim_test_done_extern(1);
	start_change_bootmode = 1;
	return -1;

}


//values=0 change to ptest mode
//values=1 change to normal mode
int change_bootmode(int values)
{
	LOG("%s line=%d values=%d.\n", __FUNCTION__, __LINE__, values);
	while(!start_change_bootmode) {
		usleep(500000);
	}

	LOG("%s line=%d start change boot mode.\n", __FUNCTION__, __LINE__);
	
	if(values == 0) {
		strncpy(gAtAck_command_extern, "UTA_MODE_CALIBRATIO", 19);
		gAtAck_command_extern[19] = '\0';
		if(at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n") < 0)
		{
			return -1;
		}
	} else {
		strncpy(gAtAck_command_extern, "(UTA_MODE_NORMAL)", 17);
		gAtAck_command_extern[17] = '\0';
		if(at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_NORMAL)\r\n") < 0)
		{
			return -1;
		}
	}

	return 0;

}

int commit_pcba_test_value(int values)
{
	LOG("%s line=%d at_command values=%d.\n", __FUNCTION__, __LINE__, values);

	if(values == 0) {
		
		strncpy(gAtAck_command_extern, "parms.param_3=0", 15);
			gAtAck_command_extern[15] = '\0';
		if(at_send_extern(gFd, "at@nvm:cal_prodparm.cust_parms.param_3=0\r\n") < 0){
			if(at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n") < 0)
			{
				at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n");
			}
			return -1;
		}
	} else {
		strncpy(gAtAck_command_extern, "parms.param_3=1", 15);
		gAtAck_command_extern[15] = '\0';
		if(at_send_extern(gFd, "at@nvm:cal_prodparm.cust_parms.param_3=1\r\n") < 0){
			if(at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n") < 0)
			{
				at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n");
			}
			return -1;
		}
	}

	strncpy(gAtAck_command_extern, "store_nvm(cal_prodp", 19);
	gAtAck_command_extern[19] = '\0';
	if(at_send_extern(gFd, "at@nvm:store_nvm(cal_prodparm)\r\n") < 0) {
		if(at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n") < 0)
		{
			at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n");
		}
		return -1;
	}

	strncpy(gAtAck_command_extern, "nvm:wait_nvm_idle()", 19);
	gAtAck_command_extern[19] = '\0';
	if(at_send_extern(gFd, "at@nvm:wait_nvm_idle()\r\n") < 0) {
		if(at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n") < 0)
		{
			at_send_extern(gFd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n");
		}
		return -1;
	}

	return 0;

}

void* getImei_testresult(void *argc) {
	//struct testcase_info *tc_info = (struct testcase_info*)argc;

	pthread_t tid;
	pthread_attr_t attr;
	int modem_fd;
	int serial_fd = -1;
	gFd = -1;
	int err;
	int biID =-1;
	
	int loop=15;
	set_is_sim_test_done_extern(0);

	sleep(10);
	
    do{
		serial_fd = open("/dev/mvpipe-atc",O_RDWR );
		if(serial_fd < 0)
		{
			sleep(1);
		}
		loop --;
		if(loop<0) break;
	}while(serial_fd < 0);


	if(serial_fd <0)
	{
		LOG("%s line=%d open serial_fd faile.\n", __FUNCTION__, __LINE__);
		set_is_sim_test_done_extern(1);
		return -1;
	}

	if ( serial_fd >= 0 ) {
		struct termios  ios;
		tcgetattr( serial_fd, &ios );
		ios.c_lflag = 0;
		if(biID == 13){ 
			ios.c_cflag |= CRTSCTS;
			cfsetispeed(&ios, B460800);
			cfsetospeed(&ios, B460800);
		}else if(biID == 14){
			ios.c_cflag |= CRTSCTS;
			cfsetispeed(&ios, B115200);
                        cfsetospeed(&ios, B115200);
		}else{
			cfsetispeed(&ios, B115200);
			cfsetospeed(&ios, B115200);
		}
		cfmakeraw(&ios);		 		
		tcsetattr( serial_fd, TCSANOW, &ios );
		
        gFd = serial_fd;     
    
        pthread_attr_init (&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

		if(pthread_create(&s_tid_reader_extern, &attr, readerLoopExtern, &attr)<0)
		{
			LOG("%s line=%d pthread_create err\n", __FUNCTION__, __LINE__);
			set_is_sim_test_done_extern(1);
			close(serial_fd);
			return -1;
		}
	}

	if(reboot_normal) {
		start_change_bootmode = 1;
		LOG("reboot: change boot mode and break getImei_testresult.\n");
		return 0;
	}

	//wifi cal result
	strncpy(gAtAck_command_extern, "cust_parms.param_2?", 19);
	gAtAck_command_extern[19] = '\0';
	if(at_send_extern(serial_fd,"at@nvm:cal_prodparm.cust_parms.param_2?\r\n") < 0) {
		LOG("%s line=%d execute at@nvm:cal_prodparm.cust_parms.param_2? fail\n", __FUNCTION__, __LINE__);
		goto ERROR;
	}
	wifi_cal_result = atoi(returnResult);
	LOG("rf cal result is %s wifi_cal_result=%d\n", returnResult, wifi_cal_result);

	if(reboot_normal) {
		start_change_bootmode = 1;
		LOG("reboot: change boot mode and break getImei_testresult.\n");
		return 0;
	}

#ifndef WIFI_ONLY_PCBA
	//rf cal result
	strncpy(gAtAck_command_extern, "cust_parms.param_1?", 19);
	gAtAck_command_extern[19] = '\0';
	if(at_send_extern(serial_fd,"at@nvm:cal_prodparm.cust_parms.param_1?\r\n") < 0) {
		LOG("%s line=%d execute at@nvm:cal_prodparm.cust_parms.param_1? fail\n", __FUNCTION__, __LINE__);
		goto ERROR;
	}

	rf_cal_result = atoi(returnResult);
	LOG("rf cal result is %s rf_cal_result=%d\n", returnResult, rf_cal_result);

	if(reboot_normal) {
		start_change_bootmode = 1;
		LOG("reboot: change boot mode and break getImei_testresult.\n");
		return 0;
	}

	//imei result
	strncpy(gAtAck_command_extern, "sec:imei_read(0)", 16);
	gAtAck_command_extern[16] = '\0';
	if(at_send_extern(serial_fd,"at@sec:imei_read(0)\r\n") < 0) {
		LOG("%s line=%d execute at@sec:imei_read(0) fail\n", __FUNCTION__, __LINE__);
		goto ERROR;
	}
	
	LOG("rf cal result is %s\n", returnResult);
	sprintf(imei_result,"%s",returnResult);

	if(reboot_normal) {
		start_change_bootmode = 1;
		LOG("reboot: change boot mode and break getImei_testresult.\n");
		return 0;
	}

	if(UI_LEVEL == 1) {
		ui_print_xy_rgba(0,3,0,255,0,255,"[%s]%s  [%s]%s  %s\n",PCBA_RF_CAL, rf_cal_result == 0 ? PCBA_CAL_NO : PCBA_CAL_YES,
															PCBA_WIFI_CAL, wifi_cal_result == 0 ? PCBA_CAL_NO : PCBA_CAL_YES,
															imei_result);
	}else if(UI_LEVEL == 2) {
		ui_print_xy_rgba(0,1,0,255,0,255,"[%s]%s [%s]%s %s\n",PCBA_RF_CAL, rf_cal_result == 0 ? PCBA_CAL_NO : PCBA_CAL_YES,
																PCBA_WIFI_CAL, wifi_cal_result == 0 ? PCBA_CAL_NO : PCBA_CAL_YES,
																imei_result);
	}
#else
	if(UI_LEVEL == 1) {
		ui_print_xy_rgba(0,3,0,255,0,255,"[%s]%s\n",PCBA_WIFI_CAL, wifi_cal_result == 0 ? PCBA_CAL_NO : PCBA_CAL_YES);
	}else if(UI_LEVEL == 2) {
		ui_print_xy_rgba(0,1,0,255,0,255,"[%s]%s [%s]%s %s\n",PCBA_WIFI_CAL, wifi_cal_result == 0 ? PCBA_CAL_NO : PCBA_CAL_YES);
	}
#endif
	
	//set_is_sim_test_done_extern(1);
	start_change_bootmode = 1;
	return 0;

ERROR:
	strncpy(gAtAck_command_extern, "UTA_MODE_CALIBRATION", 19);
	gAtAck_command_extern[19] = '\0';
	if(at_send_extern(serial_fd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n") < 0) {
		at_send_extern(serial_fd,"at@bmm:UtaModePresetReq(UTA_MODE_CALIBRATION)\r\n");
	}
	//set_is_sim_test_done_extern(1);
	start_change_bootmode = 1;
	return -1;
}


