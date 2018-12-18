//////////////////////////////////////////////////////////////////////////////////
/// \file mac_sender.c
/// \brief MAC sender thread
/// \author Pascal Sartoretti (pascal dot sartoretti at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "ext_led.h" 


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	osStatus_t retCode; 			// get the status after an operation on the OS
	uint8_t* receivedTokenPtr;
	
	uint8_t* originalMsgPtr;
	uint8_t firstEverToken = 1;
	
	for(;;)
	{
		/********** QUEUE GET : get a message from the macS queue **********/									
		retCode = osMessageQueueGet( 	
			queue_macS_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		
		//----------------------------------------------------------------------------
		// DATABACK RECEIVED									
		//----------------------------------------------------------------------------
		if(queueMsg.type == DATABACK)
		{
			uint8_t needToReleaseToken = 0; 
			uint8_t* databackFramePtr = queueMsg.anyPtr; //ptr on the frame		
			uint8_t length = databackFramePtr[2]; //get the length
			uint8_t statusDataBackFlagsAckRead = (databackFramePtr[3+length]) & 0x03; //get the 2 status bits
			uint8_t* stringPtr = &(databackFramePtr[3]); //ptr on the data inside the frame

			// ACK and READ are SET or we send us a msg (we are destination)
			if(statusDataBackFlagsAckRead == 0x03)
			{
				/********** MEMORY RELEASE	of the original data pointer **********/	
				retCode = osMemoryPoolFree(memPool,originalMsgPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
				needToReleaseToken = 1; //msg was sent and handled correctly : release token
			}
			
			// ACK or READ are SET, but not both
			else
			{
				// Send a MAC_ERROR to the lcd queue
				struct queueMsg_t macErrMsg;
				macErrMsg.type = MAC_ERROR;
				
				/********** MEMORY ALLOC **********/	
				char* lcdStringPtr = osMemoryPoolAlloc(memPool,osWaitForever);
				lcdStringPtr[0] = 'b';
				lcdStringPtr[1] = 'a';
				lcdStringPtr[2] = 'd';
				lcdStringPtr[3] = '\r';
				lcdStringPtr[4] = '\n';
				lcdStringPtr[5] = '\0';
				
				macErrMsg.anyPtr = lcdStringPtr;
				macErrMsg.addr = databackFramePtr[0] >> 0x03;

				/********** QUEUE PUT : inform the lcd that destination sapi was not available **********/
				retCode = osMessageQueuePut(
					queue_lcd_id,
					&macErrMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
				
				// READ is 0  : destination sapi is not connected
				if( ((statusDataBackFlagsAckRead & 0x02) == 0) )
				{
					/********** MEMORY RELEASE	of the original data pointer **********/
					retCode = osMemoryPoolFree(memPool,originalMsgPtr);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
					needToReleaseToken = 1;
				}
				
				// ACK is 0 and READ is 1 => sapi destination is connected but error somewhere, send again
				//else if(((statusDataBackFlagsAckRead & 0x01) == 0) && ((statusDataBackFlagsAckRead & 0x02) == 1)) 
				else if(statusDataBackFlagsAckRead == 0x02)
				{
					struct queueMsg_t sendAgainMsg;
					sendAgainMsg.type = TO_PHY;
					sendAgainMsg.addr = MYADDRESS;
					
					/********** MEMORY ALLOC : save some space for the string sent to the lcd **********/
					uint8_t* sendAgainDataPtr = osMemoryPoolAlloc(memPool,osWaitForever);
					memcpy(sendAgainDataPtr, originalMsgPtr, (originalMsgPtr[2]+4));
					
					sendAgainMsg.anyPtr = sendAgainDataPtr;

					/********** QUEUE PUT : send again the message **********/
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&sendAgainMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);			
					needToReleaseToken = 0; 
				}
			}
			

			//release the token if it was correctly sent and handled
			if(needToReleaseToken == 1)
			{
				struct queueMsg_t releaseTokenMsg;
				releaseTokenMsg.type = TO_PHY;
				
				releaseTokenMsg.anyPtr = receivedTokenPtr;
				
				/********** QUEUE PUT : give back the token to the next station **********/	
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&releaseTokenMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
			}
			
			/********** MEMORY RELEASE	of the databackFramePtr **********/
			retCode = osMemoryPoolFree(memPool,databackFramePtr);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
		}
		
		//----------------------------------------------------------------------------
		// TOKEN RECEIVED									
		//----------------------------------------------------------------------------
		else if(queueMsg.type == TOKEN)
		{	
			// this token will be released it's not our alloc
			receivedTokenPtr = queueMsg.anyPtr;
			
			//update our station list is it's the first time ever we receive a token
			if(firstEverToken == 1)
			{
				gTokenInterface.station_list[gTokenInterface.myAddress] = 0x0A; 
				firstEverToken = 0;
			}
		
			
			//first update our list with the informations from the token
			for(uint8_t i = 0;i < 15;i++)
			{
				//for our station, update the token with our informations
				if(i == gTokenInterface.myAddress)
				{
					receivedTokenPtr[i+1] = gTokenInterface.station_list[i];
				} 
				else
				{
					gTokenInterface.station_list[i] = receivedTokenPtr[i+1];
				}
			}
			
			//prepare a frame to send to the lcd 
			struct queueMsg_t lcdMsg;
			lcdMsg.type = TOKEN_LIST;
			
			/********** QUEUE PUT : put info to the lcd **********/
			retCode = osMessageQueuePut(
				queue_lcd_id,
				&lcdMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			
			/********** QUEUE GET : get sth from our local queue **********/
			struct queueMsg_t toSendMsg;
			retCode = osMessageQueueGet( 	
				local_queue_id,
				&toSendMsg,
				NULL,
				1); 	
			//	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			
			//if there is sth inside the local queue -> send it !
			if(retCode == osOK)
			{
				toSendMsg.type = TO_PHY;

				/********** MEMORY ALLOC : save some space for the retainer of the original message **********/
				originalMsgPtr = osMemoryPoolAlloc(memPool,osWaitForever);
				uint8_t* originalFramePtr = toSendMsg.anyPtr;
				memcpy(originalMsgPtr,originalFramePtr,(uint8_t) (originalFramePtr[2]) + 4);	
				
				/********** QUEUE PUT : give the message to phyS **********/
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&toSendMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			}
			
			//if not -> release TOKEN
			else
			{
				struct queueMsg_t releaseTokenMsg;
				releaseTokenMsg.type = TO_PHY;
				releaseTokenMsg.anyPtr = receivedTokenPtr;
				
				/********** QUEUE PUT : give back the token to the next station **********/
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&releaseTokenMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			}
		}
		

		//--------------------------------------------------------------------------
		// NEW TOKEN REQUEST
		//--------------------------------------------------------------------------
		else if(queueMsg.type == NEW_TOKEN)
		{
			//set the time sapi as active (0x03) chat sapi will be at 0x01
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0x0A; 
			firstEverToken = 0; //means we don't need to do that again 
			
			/********** MEMORY ALLOC : save some space for the token **********/
			uint8_t* tokenPtr = osMemoryPoolAlloc(memPool,osWaitForever);				
			tokenPtr[0] = TOKEN_TAG;
			
			//copy the list of the stations into the new token
			for(uint8_t i = 0;i<15;i++)
			{
				tokenPtr[i+1] = gTokenInterface.station_list[i];
			}

			//create the token frame
			struct queueMsg_t tokenMsg;
			tokenMsg.type = TO_PHY;
			tokenMsg.anyPtr = tokenPtr;
			
			/********** QUEUE PUT : forward the new token to the next station **********/
			retCode = osMessageQueuePut(
				queue_phyS_id,
				&tokenMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
		}
		
		//--------------------------------------------------------------------------
		// START REQUEST (connect chat)
		//--------------------------------------------------------------------------
		else if(queueMsg.type == START)
		{
			//set the time sapi as active (0x03) and chat sapi as active too (0x01)
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0x0A; 
		}
		
		//--------------------------------------------------------------------------
		// STOP REQUEST (disconnect chat)
		//--------------------------------------------------------------------------		
		else if(queueMsg.type == STOP)
		{
			//set the time sapi as active (0x03) and chat sapi as  not active (0x01)
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0x08;
		}
		
		
		//--------------------------------------------------------------------------
		// GOT DATA FROM OUR SAPIs
		//--------------------------------------------------------------------------
		else if(queueMsg.type == DATA_IND)
		{
			struct queueMsg_t dataMsg;
			uint8_t* stringFromChatPtr = queueMsg.anyPtr; //this one won't move

			//compute length
			uint8_t* stringPtr = queueMsg.anyPtr; //this one will move
			uint8_t length = 0;
			while(*stringPtr != 0)
			{
				length++;
				stringPtr++;
			}
			
			/********** MEMORY ALLOC : save some space for the data **********/
			uint8_t* framePtr = osMemoryPoolAlloc(memPool,osWaitForever);			
			uint8_t addrDest = queueMsg.addr;
			uint8_t srcSapi = queueMsg.sapi;
			
			//complete control src byte
			uint8_t controlSrc = 0;
			controlSrc = MYADDRESS << 3;
			controlSrc = controlSrc + srcSapi;
			framePtr[0] = controlSrc;
			
			//complete control dst byte
			uint8_t controldst = 0;
			controldst = addrDest << 3;
			controldst = controldst + srcSapi;
			framePtr[1] = controldst;

			//set frame length, computed before
			framePtr[2] = length;

			//copy data into frame 
			stringPtr = stringFromChatPtr; //reset stringPtr
			for(uint8_t i = 0; i < length; i++)
			{
				framePtr[i+3] = *stringPtr;
				stringPtr++;
			}
			
			/********** MEMORY RELEASE : free the memory from the string pointer **********/
			retCode = osMemoryPoolFree(memPool,stringFromChatPtr);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
			
			//and calculate checksum
			uint8_t checksum = 0;
			for(uint8_t i = 0; i < (length + 3); i++)
			{		
			  	checksum+= framePtr[i];			
			}
			checksum = checksum << 2 ;//reset ACK and READ
			framePtr[3+length] = checksum;
			
			//set the ptr of the message to the one we just filled
			dataMsg.anyPtr = framePtr;

			/********** QUEUE PUT : put into the local queue **********/
			retCode = osMessageQueuePut(
				local_queue_id,
				&dataMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
					
		}
	}
}
