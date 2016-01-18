#include "../inc/GN_GPS_Example.h"
#include "../inc/gps_ptypes.h"
#include <cutils/log.h>

//*****************************************************************************
// Global variables
extern int          fdr;                   // UART for reading
extern s_CircBuff   UARTrx_cb;             // UART Rx circular buffer

/*********************************************
 * The following is 2 functions for UART	 *
 * update 2007.7.4							 *
 *********************************************/
extern int UART_GetBytes(int Pport, U1 *Buffaddr);
extern void UART_Close (int Pport);
extern int DevCOMID;
extern module_type Port_ID;
//*****************************************************************************
// This is the main function in the Task
//
U1 UART_Input_Task()
{
   int bytes_read;
   int read_more;
	char tmpbuf[256];
	//char tmpbuf[2048];
	int Remain_space;
	
    read_more = 0;
	
	//do
	{
         // Determine the amount of space to the wrap-point of the buffer
		bytes_read = UART_GetBytes(DevCOMID, (U1 *)tmpbuf);
		//LOGD("%s: bytes_read = %d\n", __func__, bytes_read);

		//bytes_read = UART_GetBytes(Port_ID, tmpbuf,256); 
		//memcpy(UARTrx_cb.write, tmpbuf, bytes_read);
#if 0
	 	{
          	int n;
          	int i;
          	char str[256];
          	n = sprintf(str, "UART_GetBytes: %d,", bytes_read);
          	GN_GPS_Write_Event_Log((U2)n,str);
          	
          	for(i = 0; i < bytes_read; i++)
          	{
          		n = sprintf(str, "%d", tmpbuf);
          		GN_GPS_Write_Event_Log((U2)n,str);
         	}
         	n = sprintf(str, "\r\n");
         	GN_GPS_Write_Event_Log((U2)n,str);
         }
#endif         
         if ( bytes_read > 0 )
         {
	         if ((UARTrx_cb.write + bytes_read) >= UARTrx_cb.end_buf)
	         {
	         	Remain_space = UARTrx_cb.end_buf - UARTrx_cb.write;
	         	memcpy(UARTrx_cb.write,tmpbuf, Remain_space);
	         	UARTrx_cb.write = UARTrx_cb.start_buf;
	         	memcpy(UARTrx_cb.write, tmpbuf+Remain_space, bytes_read - Remain_space);
	         	UARTrx_cb.write = UARTrx_cb.start_buf + bytes_read - Remain_space;
	         }
	         else
	         {
	         	memcpy(UARTrx_cb.write,tmpbuf, bytes_read);
	         	UARTrx_cb.write = UARTrx_cb.write + bytes_read;
	         }
         
            // If we read the maximum amount, there might be more
            // available, so try again
			read_more = 1;
		}

    } //while ( bytes_read != 0 );

	return read_more;
}
