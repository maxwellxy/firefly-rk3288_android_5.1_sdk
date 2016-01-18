#include "../inc/gps_ptypes.h"
#include "../inc/GN_GPS_api.h"
#include "../inc/GN_GPS_Example.h"


//*****************************************************************************
// Prototypes of functions local to this file
void GPS_Configure( void );


//*****************************************************************************
// Global variables
//extern int          Running;                // Flag to indicate the GPS status


//*****************************************************************************
// This is the main function in the Task
//
void GPS_Task(/*cyg_addrword_t param*/)
{
    // Update the GPS at regular intervals. This API function MUST be called.
    GN_GPS_Update();
	return;
}


//*****************************************************************************
// Setup the GPS baseband configuration. This is optional.
//
void GPS_Configure( void )
{
   BL curr_config;
   BL set_config;
   s_GN_GPS_Config   Host_GN_GPS_Config;

   // Take a copy of the default GN GPS Configuration Settings.
   // Make the required changes and apply it back.
   curr_config = GN_GPS_Get_Config( &Host_GN_GPS_Config );
   if ( curr_config == TRUE )
   {
      Host_GN_GPS_Config.FixInterval      = 1000;     // Fix at 1 sec intervals
      Host_GN_GPS_Config.BGA_Chip	= 1;
			Host_GN_GPS_Config.SBAS_Enabled = 0;
			Host_GN_GPS_Config.GPVTG_Rate = 1;
			
      set_config = GN_GPS_Set_Config( &Host_GN_GPS_Config );
      if ( set_config == FALSE )
      {
         GN_GPS_Write_Event_Log(42,
                          (CH*)"Failed to set GPS configuration correctly\n");
      }
      else
      {
         GN_GPS_Write_Event_Log(22,(CH*)"GPS configuration set\n");
      }
   }
   else
   {
      GN_GPS_Write_Event_Log(37,(CH*)"Cannot get current GPS configuration\n");
   }

   return;
}

