#include "../inc/gps_ptypes.h"

#ifndef Android_OS
#define	Android_OS
#endif

#ifdef Android_OS
#include <termios.h>
#include <time.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <errno.h>
#endif


#define LOG_TAG "API_Interface_new"

#define	FS_NO_ERR	1
static const char GPS_POWER_DEV[]= "/dev/rk29_gps";
static int gps_fd = -1;

/**********************************
 ** Function: GPS Power ON       **
 ** Param: NONE						       **
 ** Return: TRUE - Success       **
 **         FALSE - Failed       **
 **********************************/
int GPS_PowerON()
{
#ifdef Android_OS
    int ret;
	int uart_id = -1;
	LOGD("%s: ...\n", __func__);

    gps_fd = open(GPS_POWER_DEV, O_RDWR | O_NOCTTY | O_NDELAY); 
    if(gps_fd < 0){
		LOGD("open /dev/rk29_gps err:%s,err=%d,fd=%d\n",strerror(errno),errno,gps_fd);
		return FALSE;
	}

    ret = ioctl(gps_fd, 1, 0);
	if(ret < 0){
		LOGD("%s:%s,err=%d\n",__func__, strerror(errno), errno);
		return FALSE;
	}
	
#endif
	return TRUE;
}

/**********************************
 ** Function: GPS Power OFF      **
 ** Param: NONE						       **
 ** Return: TRUE - Success       **
 **         FALSE - Failed       **
 **********************************/
int GPS_PowerDOWN()
{
#ifdef Android_OS
    int ret;
	
	LOGD("%s: ...\n", __func__);

	ret = ioctl(gps_fd, 0, 0);
	if(ret < 0){
		LOGD("%s:%s,err=%d\n",__func__, strerror(errno), errno);
		return FALSE;
	}
	close(gps_fd);
    gps_fd = -1;

#endif
	return TRUE;
}

/**********************************
 ** Function: Open uart          **
 ** Param: port - ID number      **
 ** 	   owner - Open mode     **
 ** Return: TRUE - Success       **
 **         FALSE - Failed       **
 **********************************/
int UART_Open(UART_PORT port, module_type owner)
{
	int hPort = 0;
#ifdef Android_OS		
	int num;
    CH 	COMDes[11];
	
	num = sprintf(COMDes, "/dev/ttyS%d", port);

    COMDes[10]='\0';
    LOGD("%s:%s\n",__func__,COMDes);	
	hPort = open(COMDes, O_RDWR );		
	if ( hPort <= 0 )
    {
      LOGD( "GN_Port_Setup: open(%s,*) = %d,error=%s\r\n", COMDes, hPort, strerror(errno));
      return( -1 );
    }

#endif
	return hPort;
}

/**********************************
 ** Function: Close uart         **
 ** Param: port - ID number      **
 **********************************/
void UART_Close (int Pport)
{
#ifdef Android_OS
	// close port;
	int ret;
	LOGD("%s,port=%d\n",__func__,Pport);
	close(Pport);
#endif
	return;
}

/*****************************************
 ** Function: Send data to GPS by uart  **
 ** Param: port - ID number             **
 **        Buffaddr - TX Point          **
 ** 	   Length - Size of send data   **
 ** Return: length of sent data         **
 *****************************************/
int UART_PutBytes(int Pport, U1 *Buffaddr, U2 Length)
{
	int ret = 0;
#ifdef Android_OS
	ret= write(Pport, Buffaddr, Length);
#endif
	//if (ret > 0)
		//LOGD("UART: tx data = %s\n", Buffaddr);
	return ret;
}

/*****************************************
 ** Function : Receive data from GPS    **
 ** Param: port - ID number             **
 **        Bufferaddr - RX Point        **
 ** Return: Length of received data     **
 *****************************************/
int UART_GetBytes(int Pport, U1 *Buffaddr)
{
	 int  G_Len = 0;
#ifdef Android_OS   
	  G_Len = read(Pport, Buffaddr, 256);
	  //G_Len = read(Pport, Buffaddr, 2048);
#endif
	  // if (G_Len > 0)
		  // LOGD("UART: rx data = %s\n", Buffaddr);
	  return G_Len;
}

void UART_SetDCBConfig (int Pport, UARTDCBStruct *DCB)
{
#ifdef Android_OS	
	struct termios  ti;
    
    tcflush(Pport, TCIOFLUSH);
	tcgetattr(Pport, &ti);
    cfmakeraw(&ti);
    ti.c_cflag |= (CLOCAL | CREAD);
    ti.c_cflag &= ~(PARENB | CSIZE | CSTOPB | CRTSCTS);
    ti.c_cflag |= CS8;
    ti.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    cfsetospeed(&ti, B115200);
    cfsetispeed(&ti, B115200);
    tcsetattr(Pport, TCSANOW, &ti);
    tcflush(Pport, TCIOFLUSH);

	LOGD("UART_SetDCBConfig,Pport=%d\n",Pport);
#endif
	 return;
}

/*****************************************
 ** Function: Open file                 **
 ** Param: FileName - Name & Path       **
 **        Flags - Open mode            **
 ** Return: Fp - File point             **
 *****************************************/
FS_HANDLE FS_Open(const WCHAR * FileName, UINT Flags)
{
	FS_HANDLE Fp = 0;
#ifdef Android_OS
	//LOGD("FS_Open %s\n",FileName);
	Fp = open(FileName, O_RDWR | O_CREAT,0666);
	//Fp = open(FileName, Flags, 0666);
	 if (Fp < 0) {
          LOGD("could not open gps serial device %s: %s", FileName, strerror(errno) );
            
    }
	 //LOGD("FS_Open %d\n",Fp);
#endif
	return Fp;
}

/*****************************************
 ** Function: Close file                **
 ** Param: File - File point            **
 ** Return: Status                      **
 *****************************************/
int FS_Close (FS_HANDLE File)
{
#ifdef Android_OS
	close(File);
	//LOGD("FS_Close\n");
	return FS_NO_ERR;
#endif
	return 0;
}

/*****************************************
 ** Function: Read file                 **
 ** Param: File - File point            **
 *****************************************/
int FS_Read(FS_HANDLE File, void * DataPtr, UINT Length, UINT * Read)
{
	int ret = 0;
#ifdef Android_OS
	//	LOGD("FS_Read %d,%d",File, Length);
	ret = read(File, DataPtr, Length);
	if(Read)
	{
		*Read = ret;
	}
		
	//	LOGD("FS_Read %d bytes: %.*s", ret, ret, DataPtr);
#endif
	return ret;
}

/*****************************************
 ** Function: Write file                **
 ** Param: File - File point            **
 *****************************************/
int FS_Write(FS_HANDLE File, void * DataPtr, UINT Length, UINT * Written)
{
	int ret = 0;
#ifdef Android_OS
	//	LOGD("FS_Write %d,%d",File, Length);
	if(Length <=0 )
	{
		return 0;
	}
	ret = write(File, DataPtr, Length);
	
	if(Written)
	{
		*Written = ret;
	}
	//	LOGD("FS_Write %d bytes: %.*s", ret, ret, DataPtr);
#endif
	return ret;
}

/*****************************************
 ** Function: Get tick from host        **
 ** Return: Ticks                       **
 *****************************************/
U4 get_tick_count()
{
#ifdef Android_OS
	struct timeval tv;
	
	if (gettimeofday(&tv, 0) < 0) {
		GN_GPS_Write_Event_Log(14, "time invaild!\n");
		return 0;
	}
	
	{
	//	LOGD("get_tick_count = %d", tv.tv_usec);
		int  n;
		char str[128];
		n = sprintf(str,"Systime = %d,\n", tv.tv_usec);
			
		GN_GPS_Write_Event_Log( (U2)n, (CH*)str );
	}
	return (tv.tv_usec/1000);
#endif
	return 0;
}

