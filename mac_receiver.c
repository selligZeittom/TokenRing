//////////////////////////////////////////////////////////////////////////////////
/// \file mac_receiver.c
/// \brief MAC receiver thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"

#define R_flag 1<<1
#define A_flag 1<<0


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * dataPtr;		// pointer on the data 
	uint8_t * qPtr;  // pointer on the frame (frame without STX/ETX)
	size_t	size;
	osStatus_t retCode;
	
	for(;;)
	{
		//----------------------------------------------------------------------------
		// QUEUE READ	from queue_macR_id									
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		
		if(retCode == osOK)
		{
			
			qPtr = queueMsg.anyPtr;
			
			//---------------------------------------------------------------------------
			// DETECT TYPE OF FRAME
			//---------------------------------------------------------------------------
				
			// TOKEN RECEIVED
			if(qPtr[0] == TOKEN_TAG)   
			{
				queueMsg.type = TOKEN;
				//--------------------------------------------------------------------------
				// QUEUE SEND	(send received frame to mac layer sender)
				//--------------------------------------------------------------------------
				retCode = osMessageQueuePut(
					queue_macS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
			
			}	
			
			// DATA FRAME RECEIVED
			else												
			{				
				//get the size
				size = qPtr[2]+4;
				
				// it's a data so get the msg
				dataPtr = &qPtr[3];
				
				// We are the destination it's a DATA_IND for one of our SAPI
				if(((qPtr[1]>>3) == gTokenInterface.myAddress) ||	// is destination my address (CHAT SAPI)
					((qPtr[1]>>3) == BROADCAST_ADDRESS))	// is a broadcast frame (TIME SAPI)
				{
					/******************************************************
					*	1) get and calculate checksum
					* 	-	ok : Set ACK
					*					 Check own SAPI state
					*							- ok :  Set Read
					*										  Create String Frame
					*										  Put DATA_IND into SAPI RX Queue
					*							- nok : Reset Read
					*		- nok : Set ACK
					* 2) Send back TO_PHY msg
					*******************************************************/
					
					//checksum of the received frame
					uint8_t status = qPtr[size-1];
					uint8_t checksumRx = status>> 2;
					
					//calculate our checksum
					uint8_t checksumCalc = 0;
					for(uint8_t i = 0; i< qPtr[2] ; i++)
					{
							checksumCalc += dataPtr[i];
					}
					checksumCalc = checksumCalc >> 2;
					
					// check if checksums match
					if(checksumRx == checksumCalc)
					{
						// write ACK
						//qPtr[size-1] = qPtr[size-1] | 0x01; //force the bit0 to 1
						qPtr[size-1] = qPtr[size-1] | A_flag;
						
						// check our SAPI state
						uint8_t sapiToReach = qPtr[1] & 0x07;
						
						if((gTokenInterface.station_list[MYADDRESS] >> sapiToReach) == 1)
						{
							// if SAPI available
							// write Read @ 1
							qPtr[size-1] = qPtr[size-1] | R_flag; //force the bit1 to 1
							
							// prepare the message to forward to our SAPI (chat or time)
							// beware the DEBUG STATION send a msg without \0
							struct queueMsg_t queueMsgForSAPI;
							queueMsgForSAPI.type = DATA_IND;
							queueMsgForSAPI.anyPtr = dataPtr;
							queueMsgForSAPI.sapi = sapiToReach;
							queueMsgForSAPI.addr = MYADDRESS;
							
							// push it man
							//--------------------------------------------------------------------------
							// QUEUE SEND	(send string to right sapi)
							//--------------------------------------------------------------------------
							
							if(sapiToReach == CHAT_SAPI)
							{
								retCode = osMessageQueuePut(
									queue_chatR_id,
									&queueMsgForSAPI,
									osPriorityNormal,
									osWaitForever);
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
							}
							else if(sapiToReach == TIME_SAPI)
							{
									retCode = osMessageQueuePut(
									queue_timeR_id,
									&queueMsgForSAPI,
									osPriorityNormal,
									osWaitForever);
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
							}
						}							
						else
						{
							// SAPI not available
							// write Read @ 0
							qPtr[size-1] = qPtr[size-1] & (~R_flag); //force the bit1 to 0
							
						}
					}
					// ERROR Checksum
					else
					{
						// write NACK
						qPtr[size-1] = qPtr[size-1] & (~A_flag); //force the bit0 to 0
					}
					
					//--------------------------------------------------------------------------
					// QUEUE SEND	(send back to phy 
					//--------------------------------------------------------------------------
					queueMsg.type = TO_PHY;
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
				}
				
				// We were the source so this is a DATABACK
				else if(((qPtr[0]>>3) == gTokenInterface.myAddress))	// is source my address
				{
					//update type
					queueMsg.type = DATABACK;
					//add src and src sapi
					queueMsg.addr = gTokenInterface.myAddress;
					queueMsg.sapi = CHAT_SAPI;
					
					//--------------------------------------------------------------------------
					// QUEUE SEND	(send received frame to mac layer sender)
					//--------------------------------------------------------------------------
					retCode = osMessageQueuePut(
						queue_macS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				}
			}
		}
	}
}


