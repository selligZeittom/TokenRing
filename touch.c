//////////////////////////////////////////////////////////////////////////////////
/// \file touch.c
/// \brief Touch control thread
/// \author Pascal Sartoretti (pascal dot sartoretti at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"

//////////////////////////////////////////////////////////////////////////////////
// THREAD TOUCH
//////////////////////////////////////////////////////////////////////////////////
void Touch(void *argument)
{
	struct queueMsg_t queueMsg;					// queue message
	GEvent* pe;													// uGFX event type
	osStatus_t retCode;
	GHandle tmpHndl;
	
	//------------------------------------------------------------------------------
	for (;;)														// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// get an UGFX_Event from uGFX task touchscreen control
		//----------------------------------------------------------------------------
		pe = geventEventWait(&gl, TIME_INFINITE);
		//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
		// BUTTON PRESSED case -------------------------------------------------------
		//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
		if(pe->type == GEVENT_GWIN_BUTTON)
		{
			tmpHndl = ((GEventGWinButton*)pe)->gwin;	// Get handle
			// NEW_TOKEN case ----------------------------------------------------------
			if((tmpHndl == btnToken)||
				(tmpHndl == btnSendToken))
			{
				queueMsg.type = NEW_TOKEN;			// message type
				queueMsg.anyPtr = NULL;					// pointer
				//------------------------------------------------------------------------
				// QUEUE SEND	(send NEW_TOKEN to mac sender)
				//------------------------------------------------------------------------
				retCode = osMessageQueuePut(
					queue_macS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);									
			}
#if DEBUG_MODE != 0
			if(tmpHndl == btnSendDebug)
			{
				gTokenInterface.debugMsgToSend = TRUE;		// message type
			}
#endif
		}
		//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
		// CHECKBOX PRESSED case -----------------------------------------------------
		//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
		if(pe->type == GEVENT_GWIN_CHECKBOX)
		{
			tmpHndl = ((GEventGWinCheckbox*)pe)->gwin;
			// Connection change -------------------------------------------------------
			if(tmpHndl == cbConnectoed)
			{
				if((((GEventGWinCheckbox*)pe)->isChecked) != FALSE)
				{
					queueMsg.type = START;						// message type
				}
				else
				{
					queueMsg.type = STOP;							// message type
				}
				queueMsg.anyPtr = NULL;							// pointer
				//------------------------------------------------------------------------
				// QUEUE SEND	(send START of STOP to mac sender)
				//------------------------------------------------------------------------
				retCode = osMessageQueuePut(
					queue_macS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);									
			}
			//--------------------------------------------------------------------------
			if(tmpHndl == cbDebugConnected)				// debug connected changed
			{
				gTokenInterface.debugOnline = gwinCheckboxIsChecked(cbDebugConnected);						
			}
			// Time to send on broadcast------------------------------------------------
			if(tmpHndl == cbBroadcastTime)
			{
				gTokenInterface.broadcastTime = ((GEventGWinCheckbox*)pe)->isChecked;
				if(gTokenInterface.broadcastTime != FALSE)
				{
					//----------------------------------------------------------------------
					// set event flag to broadcast time
					//----------------------------------------------------------------------
					retCode = osEventFlagsSet(eventFlag_id, BROADCAST_TIME_EVT);	// set flag
					if(retCode < 0)
					{
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);									
					}
				}
				else
				{
					//----------------------------------------------------------------------
					// clear event flag to broadcast time
					//----------------------------------------------------------------------
					retCode = osEventFlagsClear(eventFlag_id, BROADCAST_TIME_EVT);	// clr flag
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);									
				}
			}
			// checkbox receive CRC error ----------------------------------------------
			if(tmpHndl == cbRecCRCError)
			{
				gTokenInterface.needReceiveCRCError = ((GEventGWinCheckbox*)pe)->isChecked;
			}
			// checkbox send CRC error -------------------------------------------------
			if(tmpHndl == cbSendCRCError)
			{
				gTokenInterface.needSendCRCError = ((GEventGWinCheckbox*)pe)->isChecked;
			}
		}
		// All action ----------------------------------------------------------------
		else
		{
			queueMsg.type = TOUCH_EVENT;	// message type
			queueMsg.anyPtr = pe;			// pointer
			//--------------------------------------------------------------------------
			// QUEUE SEND	(send all messages to LCD manager)
			//--------------------------------------------------------------------------
			retCode = osMessageQueuePut(
				queue_lcd_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);									
		}
	}
}


