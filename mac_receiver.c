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


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * msg;		// pointer on the message (frame without STX/ETX)
	uint8_t * qPtr;  // pointer on the frame
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
		qPtr = queueMsg.anyPtr;
		
		//---------------------------------------------------------------------------
		// DETECT TYPE OF FRAME
		//---------------------------------------------------------------------------
			
		// TOKEN RECEIVED
		if(qPtr[1] == TOKEN_TAG)   
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
			// it's a data so get the msg
			memcpy(msg,&qPtr[1],size-2);
			
			// We are the destination it's a DATA_IND for one of our SAPI
			if(((msg[1]>>3) == gTokenInterface.myAddress) ||	// is destination my address (CHAT SAPI)
				((msg[1]>>3) == BROADCAST_ADDRESS))	// is a broadcast frame (TIME SAPI)
			{
				// to be done update type please
				
			}
			
			// We were the source so this is a DATABACK
			else if(((msg[0]>>3) == gTokenInterface.myAddress))	// is source my address
			{
				// to be done
				queueMsg.type = DATABACK;
			}
		}
		
		
	}
}


