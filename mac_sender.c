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
	uint8_t * msg;		// pointer on the message (frame without STX/ETX)
	uint8_t * qPtr;  // pointer on the frame
	size_t	size;
	osStatus_t retCode;
	
	osMessageQueueId_t local_buff_id;
	const osMessageQueueAttr_t local_buff_attr = {
		.name = "LOCAL_BUFF"  	
	};

	local_buff_id = osMessageQueueNew(4,sizeof(struct queueMsg_t),NULL); 		
	
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
		qPtr = queueMsg.anyPtr;
		
		//DATABACK RECEIVED
		if(queueMsg.type == DATABACK)
		{
		}
		
		//TOKEN RECEIVED
		else if(queueMsg.type == TOKEN)
		{
			
		}
		
		//NEW TOKEN REQUEST
		else if(queueMsg.type == NEW_TOKEN)
		{
			//set the time sapi as active
			gTokenInterface.station_list[gTokenInterface.myAddress] = 0b00001000;
			
			//create the token frame
			
		}
		
		//START REQUEST (connect chat)
		else if(queueMsg.type == START)
		{
		}
		
		//STOP REQUEST (disconnect chat)
		else if(queueMsg.type == STOP)
		{
		}
		
		//GOT DATA FROM OUR SAPIs
		else if(queueMsg.type == DATA_IND)
		{
		}
	}
}


