
//*************************************************************************
// Start of GloNav GPS code
//*************************************************************************

#include "../inc/gps_ptypes.h"
#include "../inc/GN_GPS_api.h"
#include "../inc/GN_GPS_Example.h"
#include "../inc/GN_GPS_DataLogs.h"
#include <utils/Log.h>
#include <errno.h>


//*****************************************************************************
// Function prototypes external to this file
extern int fdr;
extern int fdw;
extern int len;

extern U1           patch_status;       // Status of GPS baseband patch transmission
extern U2           cur_mess[5];        // Current messages to send, of each 'stage'

//*****************************************************************************
// Global variables
U1           UARTrx_buffer[UART_RX_BUF_SIZE];   // UART data circular buffer (to be given to GPS)
s_CircBuff   UARTrx_cb;                         // UART Rx circular buffer
U1           UARTtx_buffer[UART_TX_BUF_SIZE];   // UART data circular buffer (to be given to bb)
s_CircBuff   UARTtx_cb;                         // UART Tx circular buffer
module_type Port_ID;
int DevCOMID;
/********************************************************************
 * The following is some function for UART Communication 			*
 * Update 2007.7.4													*
 ********************************************************************/
extern int UART_Open(UART_PORT port, module_type owner);
extern void UART_Close (int Pport);
extern void UART_SetDCBConfig (int Pport, UARTDCBStruct *DCB);
extern void GPS_Configure(void);
extern U1 UART_Input_Task(void);
extern void GPS_Task(void);
extern void UART_Output_Task(void);
extern void GN_GPS_UploadPatch( void );
extern int GPS_PowerON();
extern int GPS_PowerDOWN();

void GN_GPS_Thread(void);

U1 UART_Init()
{
	UARTDCBStruct *DCB1;
	char COM_Info[20];
	
	DCB1 = (UARTDCBStruct *)COM_Info;
	
	// Initialise the i/o circular buffers.
	// (NB. end_buf points to the next byte after the buffer)
	UARTrx_cb.start_buf = &UARTrx_buffer[0];
	UARTrx_cb.end_buf   = UARTrx_cb.start_buf + UART_RX_BUF_SIZE;
	UARTrx_cb.read      = UARTrx_cb.start_buf;
	UARTrx_cb.write     = UARTrx_cb.start_buf;
	
	UARTtx_cb.start_buf = &UARTtx_buffer[0];
	UARTtx_cb.end_buf   = UARTtx_cb.start_buf + UART_TX_BUF_SIZE;
	UARTtx_cb.read      = UARTtx_cb.start_buf;
	UARTtx_cb.write     = UARTtx_cb.start_buf;
	
	// Open the UART
	DevCOMID = UART_Open(uart_port2, Port_ID);
	if (DevCOMID < 0)
	{
		LOGD("GPS UART_Init: uart init failed ............ttyS%d\n", uart_port2);	
		return FALSE; // Open is failed.
	}

	DCB1->baud = 115200;
	DCB1->dataBits = 8;
	DCB1->stopBits = 1;
	DCB1->parity = 0;
	DCB1->flowControl = 0;
	
	UART_SetDCBConfig (DevCOMID, DCB1);

	return TRUE;
}
//****************************************************************************
U1 GN_GPS_Init()
{

#if 0
	if (UART_Init() == FALSE)
	{
		return FALSE;
	}
    // Open the Data Log files
    GN_Open_Logs();

    // Initialise the patch code upload parameters
    patch_status = 0;
    cur_mess[0]  = 0;
    cur_mess[1]  = 0;
    cur_mess[2]  = 0;
    cur_mess[3]  = 0;
    cur_mess[4]  = 0;
   

    // Initialise the GPS software. This API function MUST be called.
    GN_GPS_Initialise();

    // Setup the GPS configuration. This is not a GPS API function call.
    // This is optional.
	GPS_Configure();
	
	//GPS_PowerON();

    if (!GPS_PowerON())
		return FALSE;
#endif
	return TRUE;
}

U1 GN_GPS_Start()
{
	if (UART_Init() == FALSE)
	{
		LOGD("%s:uart init error\n",__func__);
		return FALSE;
	}
	
    // Open the Data Log files
    GN_Open_Logs();

    // Initialise the patch code upload parameters
    patch_status = 0;
    cur_mess[0]  = 0;
    cur_mess[1]  = 0;
    cur_mess[2]  = 0;
    cur_mess[3]  = 0;
    cur_mess[4]  = 0;

	if (GPS_PowerON() == FALSE) 
	{
		LOGD("%s:power up error\n",__func__);
		GN_Close_Logs();
		UART_Close(DevCOMID);
		return FALSE;
	}
  
    // Initialise the GPS software. This API function MUST be called.
    GN_GPS_Initialise();

    // Setup the GPS configuration. This is not a GPS API function call.
    // This is optional.
	GPS_Configure();

	return TRUE;
}

void GN_GPS_Stop()
{
	GN_GPS_Shutdown();
	GPS_PowerDOWN();
   	GN_Close_Logs();
	UART_Close(DevCOMID);
}
void GN_GPS_Exit()
{
	//GN_GPS_Shutdown();
   	//GN_Close_Logs();
	//GPS_PowerDOWN();
   	//UART_Close(DevCOMID);
}

void GN_GPS_Thread(void)
{
	char Status_Task;
	s_GN_GPS_Nav_Data  Nav_Data;       // The latest Nav solution

	Status_Task = UART_Input_Task();
    if (Status_Task == TRUE)
    {
   		GPS_Task();
   		{
			 U2 num;
			 CH temp_buffer[64];
			 num = sprintf( temp_buffer,
                        "GN_GPS_Thread Patch status = %d.\n\r", patch_status);
			 GN_GPS_Write_Event_Log( num, (CH*)temp_buffer );
		}
		if ( GN_GPS_Get_Nav_Data( &Nav_Data ) == TRUE )
		{
   	        // A new fix has just been generated, so send the control
   	        // data to the baseband
			        //LOGD("====%s: uart output 1\n", __func__);
					UART_Output_Task();
			        //LOGD("====%s: uart output 2\n", __func__);
       	}
    }
	//LOGD("=====%s: patch_status = %d\n", __func__, patch_status);
    if ( (patch_status > 0) && (patch_status < 7) )
	{
    	GN_GPS_UploadPatch();
    }
}

//*************************************************************************
// End of GloNav GPS code ...
//*************************************************************************
#if 0
__align(32) int main(int argc, char *argv[]) // just example
{

	GN_GPS_Init();

	while (1)
	{
		GN_GPS_Thread();
	}
		
	GN_GPS_Exit();
	return 0;
}
#endif

