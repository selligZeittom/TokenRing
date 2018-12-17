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
	uint8_t * tokenPtr;		// pointer on the message (frame without STX/ETX)
	size_t	size;
	osStatus_t retCode;
	
	uint8_t* originalMsgPtr;
	
	for(;;)
	{
		//----------------------------------------------------------------------------
		// QUEUE READ	from queue_macS_id									
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macS_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		
		//DATABACK RECEIVED
		if(queueMsg.type == DATABACK)
		{
			uint8_t releaseToken = 0;
			
			uint8_t* dataBackPtr = queueMsg.anyPtr;
			
			uint8_t length = dataBackPtr[2];

			uint8_t statusDataBackFlagsAckRead = (dataBackPtr[3+length]) & 0x03;
			
			uint8_t* strPtr = &(dataBackPtr[3]);

			// ACK and READ are SET 
			if(statusDataBackFlagsAckRead == 0x03)
			{
				// Everything is fine so destroy original
				//------------------------------------------------------------------------
				// MEMORY RELEASE	of the original data pointer
				//------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,originalMsgPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
				releaseToken = 1;
			}
			
			// THEN READ @ 0 or ACK @ 0
			else
			{
				// Send a MAC_ERROR to the lcd queue
				
				struct queueMsg_t macErrMsg;
				macErrMsg.type = MAC_ERROR;
				macErrMsg.anyPtr = strPtr;
				macErrMsg.addr = dataBackPtr[0] >> 0x03;
				//--------------------------------------------------------------------------
				// QUEUE SEND	(inform the lcd that destination sapi was not available)
				//--------------------------------------------------------------------------
				retCode = osMessageQueuePut(
					queue_lcd_id,
					&macErrMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
				
						// READ is 0 destination sapi not connected
						if( ((statusDataBackFlagsAckRead & 0x02) == 0) )// && ((statusDataBackFlagsAckRead & 0x01) == 1))
						{
							//------------------------------------------------------------------------
							// MEMORY RELEASE	of the original data pointer
							//------------------------------------------------------------------------
							retCode = osMemoryPoolFree(memPool,originalMsgPtr);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
							releaseToken = 1;
						}

						// ACK is 0 and READ is 1 => sapi destination is connected but error somewhere send again
						/*
						
						
						
						
						BEWARE IF 2 ERRORS HAPPENED on the same msg => original no more there...................
						
						
						
						
						
						*/
						else if(((statusDataBackFlagsAckRead & 0x01) == 0) && ((statusDataBackFlagsAckRead & 0x02) == 1)) 
						{
							struct queueMsg_t sendAgainMsg;
							sendAgainMsg.type = TO_PHY;
							sendAgainMsg.addr = MYADDRESS;
							sendAgainMsg.anyPtr = originalMsgPtr;
							//--------------------------------------------------------------------------
							// QUEUE SEND	(send back our msg)
							//--------------------------------------------------------------------------
							retCode = osMessageQueuePut(
								queue_phyS_id,
								&sendAgainMsg,
								osPriorityNormal,
								osWaitForever);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);			
							releaseToken = 0;
						}
			}
			
			//------------------------------------------------------------------------
			// MEMORY RELEASE	of the dataBack pointer
			//------------------------------------------------------------------------
			dataBackPtr[0] = 0xAA;
			dataBackPtr[1] = 0xBB;
			retCode = osMemoryPoolFree(memPool,dataBackPtr);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	

			if(releaseToken == 1)
			{
				struct queueMsg_t queueReleaseToken;
				queueReleaseToken.type = TO_PHY;
				uint8_t* tokenReleasePtr = tokenPtr;
				tokenReleasePtr[0] = TOKEN_TAG;
				for(uint8_t i = 0;i<15;i++){
					tokenReleasePtr[i+1] = gTokenInterface.station_list[i];
				}
				queueReleaseToken.anyPtr = tokenReleasePtr;
					
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&queueReleaseToken,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
			}
	
		}
		
		//TOKEN RECEIVED
		else if(queueMsg.type == TOKEN)
		{
			
			// tokenPtr = queueMsg.anyPtr;
			
			// this token will be released it's not our alloc
			uint8_t* receivedTokenPtr = queueMsg.anyPtr;
			//first update our list with the informations from the token
			for(uint8_t i = 0;i<15;i++){
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
			struct queueMsg_t queueLcdMsg;
			queueLcdMsg.type = TOKEN_LIST;
			
			//--------------------------------------------------------------------------
			// QUEUE SEND	(inform the lcd that new informations are available)
			//--------------------------------------------------------------------------
			retCode = osMessageQueuePut(
				queue_lcd_id,
				&queueLcdMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			
			
			//--------------------------------------------------------------------------
			// QUEUE GET	(get sth inside the local queue)
			//--------------------------------------------------------------------------
			struct queueMsg_t queueMsgSend;
			retCode = osMessageQueueGet( 	
			local_queue_id,
			&queueMsgSend,
			NULL,
			1); 	
		//	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			
			//if there is sth inside the local queue -> send it !
			if(retCode == osOK)
			{
				queueMsgSend.type = TO_PHY;
				
				//--------------------------------------------------------------------------
				// SAVE ORIGINAL MESSAGE POINTER IN CASE OF RESEND NEEDED
				//--------------------------------------------------------------------------
				//	originalMsgPtr = queueMsgSend.anyPtr; // aka framePtr précédemment alloué
					originalMsgPtr = osMemoryPoolAlloc(memPool,osWaitForever);
					uint8_t* originalFramePtr = queueMsgSend.anyPtr;
					memcpy(originalMsgPtr,queueMsgSend.anyPtr,(uint8_t) (originalFramePtr[2]) + 4);	
				
				//--------------------------------------------------------------------------
				// QUEUE SEND	(put into phy queue)
				//--------------------------------------------------------------------------
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&queueMsgSend,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			}
			
			//if not -> release TOKEN
			else
			{
				queueMsg.type = TO_PHY;
			//	tokenPtr[0] = TOKEN_TAG;
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			}
		}
		
		//NEW TOKEN REQUEST
		else if(queueMsg.type == NEW_TOKEN)
		{
			//set the time sapi as active (0x03) chat sapi will be at 0x01
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0x0A; 
			
			//malloc and then full this no need stx etx done by phy layer
			//----------------------------------------------------------------------------
			// MEMORY ALLOCATION				
			//----------------------------------------------------------------------------
			tokenPtr = osMemoryPoolAlloc(memPool,osWaitForever);				
			tokenPtr[0] = TOKEN_TAG;
			for(uint8_t i = 0;i<15;i++){
				tokenPtr[i+1] = gTokenInterface.station_list[i];
			}
			
			//create the token frame
			struct queueMsg_t queueToken;
			queueToken.type = TO_PHY;
			queueToken.anyPtr = tokenPtr;
			
			//--------------------------------------------------------------------------
			// QUEUE SEND	(forward the new token)
			//--------------------------------------------------------------------------
			retCode = osMessageQueuePut(
				queue_phyS_id,
				&queueToken,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
		}
		
		//START REQUEST (connect chat)
		else if(queueMsg.type == START)
		{
			//set the time sapi as active (0x03) and chat sapi as active too (0x01)
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0x0A; 
		}
		
		//STOP REQUEST (disconnect chat)
		else if(queueMsg.type == STOP)
		{
			//set the time sapi as active (0x03) and chat sapi as  not active (0x01)
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0x08;
		}
		
		//GOT DATA FROM OUR SAPIs
		else if(queueMsg.type == DATA_IND)
		{
			struct queueMsg_t msgSend;
			uint8_t* stringFromChatPtr = queueMsg.anyPtr;
			//----------------------------------------------------------------------------
			// MEMORY ALLOCATION				
			//----------------------------------------------------------------------------

						//complete length
			uint8_t* stringPtr = queueMsg.anyPtr;
			uint8_t length = 0;
			while(*stringPtr != 0)
			{
				length++;
				stringPtr++;
			}
			
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
			

			framePtr[2] = length;
			
			//copy data into frame 
			uint8_t checksum = 0;
			stringPtr = queueMsg.anyPtr; //reset stringPtr
			for(uint8_t i = 0; i < length; i++)
			{
				framePtr[i+3] = *stringPtr;
			//	checksum+= framePtr[i+3];
				stringPtr++;
			}
			
			// and calculate checksum
			for(uint8_t i = 0; i < (length + 3); i++)
			{		
			  	checksum+= framePtr[i];			
			}
			checksum = checksum << 2 ;// & 0xfc; //reset ACK and READ
			framePtr[3+length] = checksum;
			
			msgSend.anyPtr = framePtr;
			//--------------------------------------------------------------------------
			// QUEUE SEND	(put into the local queue)
			//--------------------------------------------------------------------------
			retCode = osMessageQueuePut(
				local_queue_id,
				&msgSend,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
			
			//------------------------------------------------------------------------
			// MEMORY RELEASE	of the string pointer
			//------------------------------------------------------------------------

			retCode = osMemoryPoolFree(memPool,stringFromChatPtr);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		
		}
	}
}
