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
		}
		
		//TOKEN RECEIVED
		else if(queueMsg.type == TOKEN)
		{
			//uint8_t* dataToken = queueMsg.anyPtr;
			tokenPtr = queueMsg.anyPtr;
			//first update our list with the informations from the token
			for(uint8_t i = 0;i<15;i++){
				//for our station, update the token with our informations
				if(i == gTokenInterface.myAddress)
				{
					tokenPtr[i+1] = gTokenInterface.station_list[i];
				} 
				else
				{
					gTokenInterface.station_list[i] = tokenPtr[i+1];
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
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			
			//if there is sth inside the local queue -> send it !
			if(retCode == osOK)
			{
				queueMsgSend.type = TO_PHY;
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
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0x04; 
			
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
			//set the chat sapi as active (0x03) and chat sapi as  not active too (0x01)
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0x04;
		}
		
		//GOT DATA FROM OUR SAPIs
		else if(queueMsg.type == DATA_IND)
		{
		}
	}
}


