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
#include "common.h"
#include "sim_test.h"
#include "script.h"
#include "test_case.h"
#include "language.h"

#define MODEM_POWER_UP  1

#define BP_IOCTL_BASE 0x1a 
#define BP_IOCTL_POWON      _IOW(BP_IOCTL_BASE, 0x03, int)
#define BP_IOCTL_POWOFF    _IOW(BP_IOCTL_BASE, 0x02, int) 
#define BP_IOCTL_BASE 0x1a
#define BP_IOCTL_GET_BPID  _IOR(BP_IOCTL_BASE, 0x07, int)
#define MODEM_DEV_PATH	  "/dev/voice_modem"

#define LOG(x...) printf("[Sim_TEST] "x)

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

char *atChannelTab[]={"","/dev/ttyS1","/dev/ttyS1","/dev/ttyUSB0","/dev/ttyS1","5","/dev/ttyS1","/dev/ttyS1","/dev/ttyS1","/dev/ttyS1","/dev/ttyS1","/dev/ttyS1","/dev/ttyS1","/dev/ttyS1","/dev/ttyS1"};


static pthread_mutex_t s_commandmutex = PTHREAD_MUTEX_INITIALIZER;
int gFd;
static pthread_t s_tid_reader;
#define MAX_AT_RESPONSE 1024
static char s_ATBuffer[MAX_AT_RESPONSE+1];
static char *s_ATBufferCur = s_ATBuffer;
static int s_readCount = 0;
#ifdef SOFIA3GR_PCBA
static int is_sim_test_done = 0;
#endif

#ifdef SOFIA3GR_PCBA
int get_is_sim_test_done(void)
{
	return is_sim_test_done;
}

void set_is_sim_test_done(int is_done)
{
	is_sim_test_done = is_done;
}
#endif

static char * findNextEOL(char *cur)
{
    if (cur[0] == '>' && cur[1] == ' ' && cur[2] == '\0') {
        /* SMS prompt character...not \r terminated */
        return cur+2;
    }

    // Find next newline
    while (*cur != '\0' && *cur != '\r' && *cur != '\n') cur++;

    return *cur == '\0' ? NULL : cur;
}

static const char *readline()
{
    int count;

    char *p_read = NULL;
    char *p_eol = NULL;
    char *ret;

    if (*s_ATBufferCur == '\0') {

        s_ATBufferCur = s_ATBuffer;
        *s_ATBufferCur = '\0';
        p_read = s_ATBuffer;
    } else { 
        while (*s_ATBufferCur == '\r' || *s_ATBufferCur == '\n')
            s_ATBufferCur++;

        p_eol = findNextEOL(s_ATBufferCur);

        if (p_eol == NULL) {
            size_t len;

            len = strlen(s_ATBufferCur);

            memmove(s_ATBuffer, s_ATBufferCur, len + 1);
            p_read = s_ATBuffer + len;
            s_ATBufferCur = s_ATBuffer;
        }
    }

    while (p_eol == NULL) {
        if (0 == MAX_AT_RESPONSE - (p_read - s_ATBuffer)) {
            //LOGE("ERROR: Input line exceeded buffer\n");
            s_ATBufferCur = s_ATBuffer;
            *s_ATBufferCur = '\0';
            p_read = s_ATBuffer;
        }

        do {
        	//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"start to read\n");
            count = read(gFd, p_read,
                            MAX_AT_RESPONSE - (p_read - s_ATBuffer));
        } while (count < 0 && errno == EINTR);

        if (count > 0) {
            //AT_DUMP( "<< ", p_read, count );
            s_readCount += count;
//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"count:%d\n",count);
            p_read[count] = '\0';

            while (*s_ATBufferCur == '\r' || *s_ATBufferCur == '\n')
                s_ATBufferCur++;

            p_eol = findNextEOL(s_ATBufferCur);
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


    ret = s_ATBufferCur;
    *p_eol = '\0';
    s_ATBufferCur = p_eol + 1; 

    //LOGD("AT< %s\n", ret);
    return ret;
}

int isAtResponse = 0;
int isResponseSuc = 0;
int isAtSended = 0;
char gAtAck_command[100]={"ERROR"};
char ISMI[30]={"\0"};
char ISMI1[30]={"\0"};
char ISMI2[30]={"\0"};
int simcard1 = 0;
int simcard2 = 0;
int simCounts = 2;

static void *readerLoop(void *arg){
	 const char  *line;
	int count;
	char sread;
	int cur=0;

	while(1){
		#ifdef SOFIA3GR_PCBA
		//LOG("%s line=%d  reader loop  \n", __FUNCTION__, __LINE__);
		if(get_is_sim_test_done())
		{
			LOG("%s line=%d sim_text_is_done! exit readerLoop \n", __FUNCTION__, __LINE__);
			return NULL;
		}
		#endif
		line = readline();
		pthread_mutex_lock(&s_commandmutex);
		
		if(isAtSended){
			if(line != NULL){
				
				//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"line:%s\n",line);
				#ifdef SOFIA3GR_PCBA
				if(line != NULL && strstr(line,"AT+CIMI")!=NULL){
					line = readline();
					if(line != NULL && strstr(line,"ERROR")==NULL){
						sprintf(ISMI,"%s",line);
						isResponseSuc = 1;
						isAtResponse = 1;
						isAtSended =0;
					}
  				}
				#endif
				
				if(strstr(line,gAtAck_command)!=NULL){
					isResponseSuc = 1;
					isAtResponse = 1;
					isAtSended =0;
				}else if(strstr(line,"ERROR")!=NULL){
					isResponseSuc = 0;
					isAtResponse = 1;
					isAtSended =0;
				}
	  		
	  	}
	 	 //else
	  	//	ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"line:err\n");
		}else{
				/*if(line != NULL){
					if(strlen(line)<20)				
					ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"line1:%s\n",line);
					else
						ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"line1:>20\n");
				}
			 	else
	  			ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"line1:err\n");*/
		}
		
		pthread_mutex_unlock(&s_commandmutex);


	}
	return NULL;
}

int at_send(int fd,char *send_command,char *ack_command)
{
	int cur = 0;
	int len = strlen(send_command);
	int written;
	int i = 3;
	int j=0;
for(j=0;j<3;j++){
	
	pthread_mutex_lock(&s_commandmutex);
	
	isResponseSuc = 0;
	isAtResponse = 0;
	isAtSended =0;	
	strcpy(gAtAck_command,ack_command);
	cur = 0;
	while (cur < len) {
		written = write (fd, send_command + cur, len - cur);
		if (written < 0) {
			//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"at_send_error\n");
			pthread_mutex_unlock(&s_commandmutex);
			return -1;
		}
		cur += written;
	}
	isAtSended = 1;
	//ui_print_xy_rgba(0,get_cur_print_y(),0,0,255,100,"at_send_suc\n");
	
	pthread_mutex_unlock(&s_commandmutex);
	
	i = 3;
  while(1){
  	sleep(1);
  	if(isAtResponse && isResponseSuc ){
  		isResponseSuc = 0;
			isAtResponse = 0;
			isAtSended =0;
  		return 1;
  	}else  if(isAtResponse && isResponseSuc==0){
  		isResponseSuc = 0;
			isAtResponse = 0;
			isAtSended =0;
  		break;
  	}
  	
  	i--;
  	if(i<=0){
  		isResponseSuc = 0;
			isAtResponse = 0;
			isAtSended =0;
  		break;
  	}
  }
}
	return -1;
}

void* sim_test(void *argc)

{
	pthread_t tid;
	pthread_attr_t attr;
	int modem_fd;
	int serial_fd;
	//gFd = -1;
	int err;
	int biID =-1;
	int loop = 10;
	struct testcase_info *tc_info = (struct testcase_info*)argc;
	int arg = 1; 
	int y;

	if(tc_info->y <= 0)
		tc_info->y  = get_cur_print_y();	

	y = tc_info->y;
	
	ui_print_xy_rgba(0,y,255,255,0,255,"%s:[%s..] \n",PCBA_SIM,PCBA_TESTING);

#ifdef SOFIA3GR_PCBA
	sleep(5);
	test_simcard();
	LOG("sim_counts=%d  simcard1=%d  simcard2=%d\n", simCounts);

	if(simCounts == 2) {
		if(simcard1 && simcard2){
			tc_info->result = 0;
			ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] { IMSI[sim]=%s, IMSI[sim2]=%s }\n",PCBA_SIM,PCBA_SECCESS, ISMI1, ISMI2);
			return argc;
		}
		else if(simcard1) {
			tc_info->result = -1;
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] { IMSI[sim]=%s, IMSI[sim2]=%s }\n",PCBA_SIM,PCBA_FAILED, ISMI1, PCBA_FAILED);
			return argc;
		}
		else if(simcard2) {
			tc_info->result = -1;
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] { IMSI[sim]=%s, IMSI[sim2]=%s }\n",PCBA_SIM,PCBA_FAILED, PCBA_FAILED, ISMI2);
			return argc;
		}
		else {
			tc_info->result = -1;
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] { IMSI[sim]=%s, IMSI[sim2]=%s }\n",PCBA_SIM,PCBA_FAILED, PCBA_FAILED, PCBA_FAILED);
			return argc;
		}
	}
	else {
		if(!simcard1 && !simcard2) {
			tc_info->result = -1;
			ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] \n",PCBA_SIM,PCBA_FAILED);
			return argc;
		}
		else if(simcard1)
		{
			tc_info->result = 0;
			ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] { IMSI[sim]=%s }\n",PCBA_SIM,PCBA_SECCESS, ISMI1);
			return argc;
		}
		else if(simcard2)
		{
			tc_info->result = 0;
			ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] { IMSI[sim]=%s }\n",PCBA_SIM,PCBA_SECCESS, ISMI2);
			return argc;
		}
	}
#else
	modem_fd = open("/dev/voice_modem", O_RDWR); 
 	if(modem_fd > 0){
 		//sleep(5);
		//ioctl(modem_fd,BP_IOCTL_POWOFF,&arg);
		//sleep(2);
   	 	ioctl(modem_fd,BP_IOCTL_POWON,&arg);
		sleep(2);     
  	}else{
  		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] \n",PCBA_SIM,PCBA_FAILED);
		LOG("modem open fail !\n");
		tc_info->result = -1; 
		return argc;
  	} 
	err = ioctl(modem_fd,BP_IOCTL_GET_BPID,&biID);
	if(err < 0){
		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] \n",PCBA_SIM,PCBA_FAILED);
		LOG("biID fail !\n");
		tc_info->result = -1;
		return argc;
	}
	
	do{
		serial_fd = open(atChannelTab[biID],O_RDWR );
		sleep(1);
		loop --;
		if(loop<0)break;
	}while(serial_fd < 0);
#endif


	if(serial_fd <0)
	{
		#ifdef SOFIA3GR_PCBA
			set_is_sim_test_done(1);
		#endif
		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] \n",PCBA_SIM,PCBA_FAILED);
		LOG("open at port fail!\n");
		tc_info->result = -1;
		return argc;
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

	    if(pthread_create(&s_tid_reader, &attr, readerLoop, &attr)<0)
	    {
	     	ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] \n",PCBA_SIM,PCBA_FAILED);
	     	LOG("%s line=%d pthread_create err\n", __FUNCTION__, __LINE__);
			tc_info->result = -1;
			#ifdef SOFIA3GR_PCBA
				set_is_sim_test_done(1);
				//close(serial_fd);
				return argc;
			#endif
	    }
     	//while(1){sleep(5);write (gFd, "AT+CPIN?\r\n", 10);}
	}

	sleep(5);

#ifndef SOFIA3GR_PCBA
	if(at_send(serial_fd,"AT+CPIN?\r\n","READY") >= 0){
		tc_info->result = 0;
		ui_print_xy_rgba(0,y,0,255,0,255,"%s:[%s] \n",PCBA_SIM,PCBA_SECCESS);
	}else{
		tc_info->result = -1;
		ui_print_xy_rgba(0,y,255,0,0,255,"%s:[%s] \n",PCBA_SIM,PCBA_FAILED);
	}
	ioctl(modem_fd,BP_IOCTL_POWOFF,&arg);
#endif
	return argc;

}

