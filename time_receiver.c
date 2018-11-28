//////////////////////////////////////////////////////////////////////////////////
/// \file time_receiver.c
/// \brief Time receiver thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"


//////////////////////////////////////////////////////////////////////////////////
// THREAD TIME RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void TimeReceiver(void *argument)
{
	struct queueMsg_t queueMsg;					// queue message
	osStatus_t retCode;									// return error code
	
	//------------------------------------------------------------------------------
	for (;;)														// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// QUEUE READ									
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_timeR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		queueMsg.type = TIME_MSG;
		//----------------------------------------------------------------------------
		// QUEUE SEND	(send the time message on LCD)
		//----------------------------------------------------------------------------
		retCode = osMessageQueuePut(
			queue_lcd_id,
			&queueMsg,
			osPriorityNormal,
			osWaitForever);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);							
	}		
}


