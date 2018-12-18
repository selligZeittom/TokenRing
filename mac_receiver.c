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
	osStatus_t retCode;
	
	for(;;)
	{
		/********** QUEUE GET : get a message from from queue_macR_id **********/		
		retCode = osMessageQueueGet( 	
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		
		//if we got a message
		if(retCode == osOK)
		{
			uint8_t* qPtr = queueMsg.anyPtr; // pointer on the frame (frame without STX/ETX)
				
			//----------------------------------------------------------------------------
			// TOKEN RECEIVED
			//----------------------------------------------------------------------------
			if(qPtr[0] == TOKEN_TAG)   
			{
				queueMsg.type = TOKEN;

				/********** QUEUE PUT : forward token to the mac sender **********/
				retCode = osMessageQueuePut(
					queue_macS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
			
			}	
			
			//----------------------------------------------------------------------------
			// DATA FRAME RECEIVED
			//----------------------------------------------------------------------------
			else												
			{				
				//get the size
				size_t	size = qPtr[2]+4;
				
				// it's a data so get the msg
				uint8_t * dataPtr = &qPtr[3];
				
				// We are the destination it's a DATA_IND for one of our SAPI
				if(((qPtr[1]>>3) == gTokenInterface.myAddress) ||	// is destination my address (CHAT SAPI)
					(((qPtr[1]>>3) == BROADCAST_ADDRESS) && ((qPtr[0]>>3)!=gTokenInterface.myAddress)))	// is a broadcast frame (TIME SAPI)
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
					uint8_t checksumRx = status & 0xFC;// 2;
					
					//calculate our checksum
					uint8_t checksumCalc = 0;
					for(uint8_t i = 0; i< (qPtr[2] + 3); i++)
					{
						checksumCalc += qPtr[i];
					}
					checksumCalc = checksumCalc << 2;
					
					// check if checksums match
					if(checksumRx == checksumCalc)
					{
						// write ACK
						qPtr[size-1] = qPtr[size-1] | A_flag;
						
						// check our SAPI state
						uint8_t sapiToReach = qPtr[1] & 0x07;
						
						// check if our sapi is connected
						if(((gTokenInterface.station_list[MYADDRESS] >> sapiToReach) &0x01) == 1)
						{
							// if SAPI available : set READ
							qPtr[size-1] = qPtr[size-1] | R_flag; //force the bit1 to 1
							
							// prepare the message to forward to our SAPI (chat or time)
							struct queueMsg_t toSapiMsg;
							toSapiMsg.type = DATA_IND;
							
							
							/********** MEMORY ALLOC : save some space for the data **********/
							uint8_t* sapiDataPtr = osMemoryPoolAlloc(memPool,osWaitForever);			
							memcpy(sapiDataPtr, dataPtr, qPtr[2]); //copy
							sapiDataPtr[size-4] = 0x00;		// here put the terminaison c-style character known as a 0x00 aka NULL
							toSapiMsg.anyPtr = sapiDataPtr;
							toSapiMsg.sapi = sapiToReach;
							toSapiMsg.addr = MYADDRESS;
							
							/********** QUEUE PUT : send string to the right sapi **********/
							if(sapiToReach == CHAT_SAPI)
							{
								retCode = osMessageQueuePut(
									queue_chatR_id,
									&toSapiMsg,
									osPriorityNormal,
									osWaitForever);
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
							}
							else if(sapiToReach == TIME_SAPI)
							{
									retCode = osMessageQueuePut(
									queue_timeR_id,
									&toSapiMsg,
									osPriorityNormal,
									osWaitForever);
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
							}
						}							
						else
						{
							// SAPI not available : reset READ 
							qPtr[size-1] = qPtr[size-1] & (~R_flag); //force the bit1 to 0							
						}
					}
					// ERROR Checksum indicate by setting the READ bit but not the ACK
					else
					{
						// write NOK
						//qPtr[size-1] = qPtr[size-1] & (~A_flag); //force the bit0 to 0
						// if SAPI available : set READ
						qPtr[size-1] = qPtr[size-1] | R_flag; //force the bit1 to 1
					}
					
					/********** QUEUE PUT : send frame back to phy... **********/
					queueMsg.type = FROM_PHY;
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);								
				
				}
				
				// We were the source of a msg or a Broadcast so this is a DATABACK
				else if(((qPtr[0]>>3) == gTokenInterface.myAddress) || 
					(((qPtr[1]>>3) == BROADCAST_ADDRESS) && ((qPtr[0]>>3)==gTokenInterface.myAddress)))	// is source my address
				{
					//update type
					queueMsg.type = DATABACK;
					//add src and src sapi
					queueMsg.addr = gTokenInterface.myAddress;
					queueMsg.sapi = CHAT_SAPI;
					
					/********** QUEUE PUT : send databack **********/
					retCode = osMessageQueuePut(
						queue_macS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							
					/********** QUEUE PUT : send string to the right sapi **********/
					if((qPtr[1]>>3) == BROADCAST_ADDRESS)
					{
						// prepare the message to forward to our SAPI (chat or time)
						struct queueMsg_t toSapiMsg;
						toSapiMsg.type = DATA_IND;
						// check our SAPI state
						uint8_t sapiToReach = qPtr[1] & 0x07;
						/********** MEMORY ALLOC : save some space for the data **********/
						uint8_t* sapiDataPtr = osMemoryPoolAlloc(memPool,osWaitForever);			
						memcpy(sapiDataPtr, dataPtr, qPtr[2]); //copy
						sapiDataPtr[size-4] = 0x00;		// here put the terminaison c-style character known as a 0x00 aka NULL
						toSapiMsg.anyPtr = sapiDataPtr;
						toSapiMsg.sapi = sapiToReach;
						toSapiMsg.addr = MYADDRESS;
						
						retCode = osMessageQueuePut(
							queue_timeR_id,
							&toSapiMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
					}
				}
			}
		}
	}
}


