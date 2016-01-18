#include "../inc/gps_ptypes.h"
#include "../inc/GN_GPS_api.h"
#include "../inc/GN_GPS_Example.h"
//#include "Baseband_Patch_205.c"  // The baseband patch code to upload
//#include "Baseband_Patch_301.c"
#include "../patch/Baseband_Patch_510_P076.c"

#define   MAX_NUM_MESS   50      // Send 10 messages every 10 ms
//*****************************************************************************
// Prototypes of functions local to this file
void GN_GPS_UploadPatch( void );


//*****************************************************************************
// Global variables
extern s_CircBuff   UARTtx_cb;              // UART Tx circular buffer

//-----------------------------------------------------------------
// The following are parameters relating to uploading a patch file
// to the baseband chip
U1           patch_status;       // Status of GPS baseband patch transmission
U2           cur_mess[5];        // Current messages to send, of each 'stage'
U2		GPS_ROM_Version;
//-----------------------------------------------------------------
// update 2007.7.4
extern	UINT UART_GetBytes(int Pport, U1 *Buffaddr);
extern	UINT UART_PutBytes(int Pport, U1 *Buffaddr, U2 Length);
extern	void UART_Close (int Pport);
extern module_type Port_ID;
extern int DevCOMID;
extern void GN_Close_Logs(void);

//*****************************************************************************
// This is the main function in the Task
//
void UART_Output_Task(/*cyg_addrword_t param*/)
{
   int num;
	 int remain_len;
	 int S_Len;
   num = 0;

   // If there is any data in the output circular buffer, send it to the UART
   if ( UARTtx_cb.write != UARTtx_cb.read )
   {
         while ( UARTtx_cb.write != UARTtx_cb.read )
         {
            // Determine the amount of data in the buffer
 						num = UARTtx_cb.write - UARTtx_cb.read;
            if ( num > 0 )
            {
               // The buffer has not wrapped, so write all the data in one go
                //UART_PutBytes(DevCOMID, (kal_uint8 *)UARTtx_cb.read, num);
               remain_len = num;
				      // Send the data
				       do 
				       {
				       		S_Len = UART_PutBytes(DevCOMID, (kal_uint8 *)UARTtx_cb.read+(num-remain_len), remain_len);
				       		if (S_Len < 0)
				       		{
				       				//re-send
				       				
				       		}
				       		else
				       		{
				       				remain_len = remain_len - S_Len;
				       		}
				       		
				       }while(remain_len>0);
				       
          		 UARTtx_cb.read = UARTtx_cb.read + num;
            }
            else
            {
               // The buffer has wrapped, so write only the data up to the
               // wrap-point - the remainder will be written the next time
               // through this while loop
               num = UARTtx_cb.end_buf - UARTtx_cb.read;
               //UART_PutBytes(DevCOMID, (kal_uint8 *)UARTtx_cb.read, num);
               remain_len = num;
				      // Send the data
				       do 
				       {
				       		S_Len = UART_PutBytes(DevCOMID, (kal_uint8 *)UARTtx_cb.read+(num-remain_len), remain_len);
				       		if (S_Len < 0)
				       		{
				       				//re-send
				       				
				       		}
				       		else
				       		{
				       				remain_len = remain_len - S_Len;
				       		}
				       		
				       }while(remain_len>0);
				       
               UARTtx_cb.read = UARTtx_cb.read + num;
               // Reset the buffer to the start point
               UARTtx_cb.read = UARTtx_cb.start_buf;
            }
   		};
   }
   //GN_GPS_Write_Event_Log(23,(CH*)"UART Output task ended\n");
   
   return;
}

//*****************************************************************************
// Upload the next set of patch code to the GPS baseband.
// This function is called every UART_OUT_TASK_SLEEP ms. It sends up
// to a maximum of MAX_NUM_MESS sentences each time.
// The complete set of patch data is divided into six stages.
void GN_GPS_UploadPatch( void )
{
  const U1 *p_Comm;       // Pointer to the array of sentence identifiers to send
   const U2 *p_Addr;       // Pointer to the array of addresses to send
   const U2 *p_Data;       // pointer to the array of data to send
   U2       StartAdr;      // First address of type 23 consecutive addresses
   U1       index;         // Current 'index' of data to be sent
   U1       ckSum;         // Checksum
   U1       CS1;           // First character of checksum in Hex
   U1       CS2;           // Second character of checksum in Hex
   U2       Addr;          // Address to be sent
   U4       i;             // Index into array for checksum calculation
   U4       tot_num;       // Total number of sentences in current stage available
   U2       size;          // Size of patch data to send
   U4       num_sent;      // Number of sentences sent in the current session
   CH       patch_data[30];// Single sentence to be sent
   U4       tot_mess;      // Total number of messages sent
	 int 			remain_len, S_Len;
	 int			PATCH_SPEED;
   // When a patch file is available for upload we must write it out here.
   // We must write full sentences only.

   switch( patch_status )
   {
         case 1:               // First stage
         {
            index    = 0;
            tot_num  = NUM_1;
            StartAdr = 0;                                // Remove compiler warning
            p_Comm   = &Comm1[cur_mess[index]];
            p_Addr   = &Addr1[cur_mess[index]];
            p_Data   = &Data1[cur_mess[index]];
            tot_mess = cur_mess[0];
            break;
         }
         case 2:               // Second stage
         {                     // All the second stage are COMD 23
            index    = 1;      // and are consecutive addresses
            tot_num  = NUM_2;
            StartAdr = START_ADDR_2;
            p_Comm   = NULL;                             // Remove compiler warning
            p_Addr   = NULL;                             // Remove compiler warning
            p_Data   = &Data2[cur_mess[index]];
            tot_mess = NUM_1 + cur_mess[1];
            break;
         }
         case 3:               // Third stage
         {
            index    = 2;
            tot_num  = NUM_3;
            StartAdr = 0;                                // Remove compiler warning
            p_Comm   = &Comm3[cur_mess[index]];
            p_Addr   = &Addr3[cur_mess[index]];
            p_Data   = &Data3[cur_mess[index]];
            tot_mess = NUM_1 + NUM_2 + cur_mess[2];
            break;
         }
         case 4:
         {                    // All the 4th stage are COMD 23
            index    = 3;     // and are consecutive addresses
            tot_num  = NUM_4;
            StartAdr = START_ADDR_4;
            p_Comm   = NULL;                             // Remove compiler warning
            p_Addr   = NULL;                             // Remove compiler warning
            p_Data   = &Data4[cur_mess[index]];
            tot_mess = NUM_1 + NUM_2  + NUM_3 + cur_mess[3];
            break;
         }
         case 5:             // Fifth stage
         {
            index    = 4;
            tot_num  = NUM_5;
            StartAdr = 0;                                // Remove compiler warning
            p_Comm   = &Comm5[cur_mess[index]];
            p_Addr   = &Addr5[cur_mess[index]];
            p_Data   = &Data5[cur_mess[index]];
            tot_mess = NUM_1 + NUM_2  + NUM_3 + NUM_4 + cur_mess[4];
            break;
         }
         default : return;          // Error
   }


   // Re-constitute the next message to be sent
   num_sent = 0;
   
   if (patch_status <= 3)
   {
   		PATCH_SPEED = 200;
   }
   else
   {
   		PATCH_SPEED = 50;
   }
   while ( (num_sent < PATCH_SPEED) && (cur_mess[index] < tot_num) )
   {
      if ( (index == 1) || (index == 3) )
      {
         // All the index 1 and 3 sentences are of type 23, with consecutive addresses
         size = 25;
         Addr = StartAdr + cur_mess[index];
         sprintf( (char*)patch_data,"#COMD 23 %05d %05d &   ",
                  Addr, p_Data[num_sent] );
      }
      else
      {
         if ( p_Comm[num_sent] == 11 )
         {
            size = 15;
            sprintf( (char*)patch_data,"#COMD %2d %1d &   ",
                     p_Comm[num_sent], p_Addr[num_sent]);
         }
         else if ( p_Comm[num_sent] == 25 )
         {
            size = 19;
            sprintf( (char*)patch_data,"#COMD %2d %05d &   ",
                     p_Comm[num_sent], p_Addr[num_sent]);
         }
         else
         {
            size = 25;
            sprintf( (char*)patch_data, "#COMD %2d %05d %05d &   ",
                     p_Comm[num_sent], p_Addr[num_sent], p_Data[num_sent]);
         }
      }

      // Add the checksum
      ckSum = 0;   // Checksum counter.
      i     = 1;   // Don't include the '#' in the checksum validation.
      do
      {
         ckSum = (U1)(ckSum + (U1)patch_data[i++]);
      } while ( patch_data[i] != '&' );
      i++;
      CS1 = ckSum/16;
      if ( CS1 <= 9 )  patch_data[i] = (CH)(CS1+48);
      else             patch_data[i] = (CH)(CS1+55);
      i++;
      CS2 = ckSum % 16;
      if ( CS2 <= 9 )  patch_data[i] = (CH)(CS2+48);
      else             patch_data[i] = (CH)(CS2+55);
      i++;

      // Add the LF
      patch_data[i] = (CH)(0x0A);

			remain_len = size;
      // Send the data
       do 
       {
       		S_Len = UART_PutBytes(DevCOMID, (kal_uint8 *)patch_data+(size-remain_len), remain_len);
       		if (S_Len < 0)
       		{
       				//re-send
       				
       		}
       		else
       		{
       				remain_len = remain_len - S_Len;
       		}
       		
       }while(remain_len>0);
       
#if 0
      if ( GN_GPS_Write_GNB_Ctrl( size, patch_data ) != size )
      {
         U2 num;
         CH temp_buffer[64];
         num = sprintf( temp_buffer,
                        "GN_Upload_GNB_Patch_510: ERROR:  Some Patch data not taken by UART TX driver.\n\r" );
         GN_GPS_Write_Event_Log( num, (CH*)temp_buffer );
      }
#endif
      cur_mess[index]++;
      num_sent++;
   }

   // Check if all the data has been sent
   if ( cur_mess[index] == tot_num )
   {
      patch_status++;
   }

   if ( patch_status == 6 )
   {
      U2 num;
      CH temp_buffer[64];
      num = sprintf( temp_buffer, "GN_Upload_GNB_Patch_510: COMPLETED.\n\r" );
      GN_GPS_Write_Event_Log( num, (CH*)temp_buffer );

      // Clear the counter of messages sent ready for a future time.
      cur_mess[0]    = 0;
      cur_mess[1]    = 0;
      cur_mess[2]    = 0;
      cur_mess[3]    = 0;
      cur_mess[4]    = 0;

      patch_status   = 7;   // Flag the end of the patch data
   }

   return;
}

//*****************************************************************************
// This function is not called from within the UART_Output_Task, but is
// called as part of the GPS 'callback' functions
U2 Setup_GNB_Patch( U2 ROM_version, U2 Patch_CkSum )
{
	
   // Check that the patch upload is not currently in progress
   if ( (patch_status > 0) && (patch_status < 5) )
   {
      GN_GPS_Write_Event_Log(21, (CH*)"Already in progress\n\r");
      return(Patch_CkSum);
   }
   
   if ( ROM_version != PatchRomVersion )
   {
      int n;
      char str[64];   
      n = sprintf(str, "Do not have patch data - available version = %d\n", PatchRomVersion);
      GN_GPS_Write_Event_Log(n, (CH*)str);
      patch_status = 0;
      return(0);
   }
   
   // Check that the patch available is not already uploaded
   if ( (U2)PatchCheckSum == Patch_CkSum)
   {
      GN_GPS_Write_Event_Log(31, (CH*)"Patch available already in use\n");
      return(PatchCheckSum);
   }
   
   // Set the patch status to indicate that the upload should start
   patch_status = 1;

   cur_mess[0]  = 0;
   cur_mess[1]  = 0;
   cur_mess[2]  = 0;
   cur_mess[3]  = 0;
   cur_mess[4]  = 0;
   
   return(PatchCheckSum);
}
