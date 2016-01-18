#include <string.h>
#include <fcntl.h>

#include "../inc/gps_ptypes.h"
#include "../inc/GN_GPS_DataLogs.h"

extern FS_HANDLE FS_Open(const WCHAR * FileName, UINT Flags);
extern int FS_Close (FS_HANDLE File);
extern int FS_Write(FS_HANDLE File, void * DataPtr, UINT Length, UINT * Written);

//*****************************************************************************
// Global data
static BL           g_Enable_Log[NUM_LOGS];   // Flag which logs are active
static FS_HANDLE	hFile_Log[NUM_LOGS];

//*****************************************************************************
// Open the GPS log files
void GN_Open_Logs(void)
{
   int  i;

   memset( g_Enable_Log, 0, sizeof(g_Enable_Log) );

 // Update 2007.7.4
   i = (int)NMEA_LOG;
   hFile_Log[i] = FS_Open((const WCHAR *)"/sdcard/GN_NMEA_Log.TXT", O_RDWR | O_CREAT | O_TRUNC);
   //g_Enable_Log[i] = TRUE;
   g_Enable_Log[i] = FALSE;

   i = (int)EVENT_LOG;
   hFile_Log[i] = FS_Open((const WCHAR *)"/sdcard/GN_Event_Log.TXT", O_RDWR | O_CREAT | O_TRUNC);
   //g_Enable_Log[i] = TRUE;
   g_Enable_Log[i] = FALSE;
   
   i = (int)ME_LOG;
   hFile_Log[i] = FS_Open((const WCHAR *)"/sdcard/GN_ME_Log.TXT", O_RDWR | O_CREAT | O_TRUNC);
   //g_Enable_Log[i] = TRUE;
   g_Enable_Log[i] = FALSE;
   
   i = (int)NAV_LOG;
   hFile_Log[i] = FS_Open((const WCHAR *)"/sdcard/GN_Nav_Log.TXT", O_RDWR | O_CREAT | O_TRUNC);
   //g_Enable_Log[i] = TRUE;
   g_Enable_Log[i] = FALSE;

   return;
}

//*****************************************************************************
// Close the GPS log files
void GN_Close_Logs(void)
{
   int i;

   // Close each of the log files
   for ( i = 0 ; i < NUM_LOGS ; i++ )
   {
      if ( g_Enable_Log[i] == TRUE )
      {
			FS_Close(hFile_Log[i]);
      }
   }

   return;
}

//*****************************************************************************
// Write data to the specified log file
// Write data to the specified log file
void Write_Data_To_Log(
       e_Data_Log  log,          // Data log type
       U2          num_bytes,    // Number of bytes to Write
       CH          *p_data )     // Pointer to the data
{
	UINT	WriteCnt;
	
   if ( g_Enable_Log[log] == TRUE )
   {
		FS_Write(hFile_Log[log], (void *) p_data, (UINT) num_bytes, &WriteCnt);
   }

   return;
}
//*****************************************************************************


