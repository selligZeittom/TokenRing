//////////////////////////////////////////////////////////////////////////////////
/// \file phy_sender.c
/// \brief Physical sender thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "ext_uart.h"
#include "ext_led.h"

//////////////////////////////////////////////////////////////////////////////////
/// \brief Called on interrupt on RS232 sended char
/// \param The uart handler (ext_uart)
//////////////////////////////////////////////////////////////////////////////////
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	osEventFlagsSet(eventFlag_id, RS232_TX_EVENT);			// set flag for next send
}

//////////////////////////////////////////////////////////////////////////////////
/// \brief Send bytes to physical line
/// \param byte The byte to send
/// \param counter The number of the byte in the frame
///
/// The STX is sent twice if not the first byte
//////////////////////////////////////////////////////////////////////////////////
void rs232_send(uint8_t byte, uint8_t counter)
{
	int32_t	eventFlag;											// current flag
	
	//------------------------------------------------------------------------------
	//	EVENT GET wait 10 ticks max
	//------------------------------------------------------------------------------
	eventFlag = osEventFlagsWait(
		eventFlag_id,
		RS232_TX_EVENT,
		osFlagsWaitAny,
		10); 	
	if((eventFlag < 0)&&				// case of error
		(eventFlag != -2))				// but not a timeout
			CheckRetCode(eventFlag,__LINE__,__FILE__,CONTINUE);	
	
	//------------------------------------------------------------------------------
  if((counter == 0)&&(byte == STX)) 				// first STX to send					
  {
		HAL_UART_Transmit_IT(&ext_uart,&byte, 1);
  }
	//------------------------------------------------------------------------------
  if ((counter != 0)&&(byte != STX))				// any data to send	not STX	
  {
		HAL_UART_Transmit_IT(&ext_uart,&byte, 1);
  }
	//------------------------------------------------------------------------------
  if ((counter != 0)&&(byte == STX))				// STX in frame -> double it
  {
		HAL_UART_Transmit_IT(&ext_uart,&byte, 1);	// send first STX
		//----------------------------------------------------------------------------
		// EVENT GET wait 10 ticks max
		//----------------------------------------------------------------------------
		eventFlag = osEventFlagsWait(
			eventFlag_id,
			RS232_TX_EVENT,
			osFlagsWaitAny,
			10); 	
		if((eventFlag < 0)&&				// case of error
			(eventFlag != -2))				// but not a timeout
				CheckRetCode(eventFlag | 0x7FFFFFFF,__LINE__,__FILE__,CONTINUE);	
		HAL_UART_Transmit_IT(&ext_uart,&byte, 1);	// send second STX
  }	    
}


//////////////////////////////////////////////////////////////////////////////////
// THREAD PHYSICAL SENDER
//////////////////////////////////////////////////////////////////////////////////
void PhSender(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * qPtr;
	osStatus_t retCode;
	
	//------------------------------------------------------------------------------
	for (;;)											// loop until doomsday
	{
    //----------------------------------------------------------------------------
		//	QUEUE READ										
    //----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_phyS_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		qPtr = queueMsg.anyPtr;
		if(qPtr[0] == TOKEN_TAG)
		{
			Ext_LED_PWM(1,0);										// token is out of station
		}
#if DEBUG_MODE != 0
		//----------------------------------------------------------------------------
		// DEBUG DISPLAY FRAME				
		//----------------------------------------------------------------------------
		DebugMacFrame('S',qPtr);		
		//----------------------------------------------------------------------------
		// QUEUE SEND	(send frame to debug station)
		//----------------------------------------------------------------------------
		retCode = osMessageQueuePut(
			queue_dbg_id,
			&queueMsg,
			osPriorityNormal,
			0);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);												
#else
		uint8_t * msg;
		uint8_t i;
		size_t	size;
		
		if(qPtr[0] == TOKEN_TAG)
		{
			size = TOKENSIZE;										// this is a token
		}
		else
		{
			size = qPtr[2] + 6;				// = txtSize + STX + SRC + DST + LN + CS + ETX
		}
		//----------------------------------------------------------------------------
		// MEMORY ALLOCATION				
		//----------------------------------------------------------------------------
		msg = osMemoryPoolAlloc(memPool,osWaitForever);													
		msg[0] = STX;
		memcpy(&msg[1],qPtr,size-2);
		msg[size-1] = ETX;
		//----------------------------------------------------------------------------
		// DEBUG DISPLAY FRAME				
		//----------------------------------------------------------------------------
		putchar('S');                				  // insert a S for Send         	
		putchar(':');                			    // insert a :                  
		DebugFrame((char *)msg);							// for debug info only
		for(i=0;i<size;i++)
		{
			rs232_send(msg[i],i);
		}
		//----------------------------------------------------------------------------
		// MEMORY RELEASE	(received frame : mac layer style)
		//----------------------------------------------------------------------------
		retCode = osMemoryPoolFree(memPool,qPtr);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		//----------------------------------------------------------------------------
		// MEMORY RELEASE	(created frame : phy layer style)
		//----------------------------------------------------------------------------
		retCode = osMemoryPoolFree(memPool,msg);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
#endif		
	}
}


