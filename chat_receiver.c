//////////////////////////////////////////////////////////////////////////////////
/// \file chat_receiver.c
/// \brief Chat receiver thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"

//////////////////////////////////////////////////////////////////////////////////
// THREAD CHAT RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void ChatReceiver(void *argument)
{
	struct queueMsg_t queueMsg;							// queue message
	osStatus_t retCode;											// return error code

	//------------------------------------------------------------------------------
	for (;;)																// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_chatR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
		queueMsg.type = CHAT_MSG;
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
		//----------------------------------------------------------------------------
		// QUEUE SEND								
		//----------------------------------------------------------------------------
		retCode = osMessageQueuePut(
			queue_lcd_id,
			&queueMsg,
			osPriorityNormal,
			osWaitForever);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);			
	}	
}


