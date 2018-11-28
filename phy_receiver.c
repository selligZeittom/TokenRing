//////////////////////////////////////////////////////////////////////////////////
/// \file phy_receiver.c
/// \brief Physical receiver thread
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

uint8_t gInBuffer[256];											// generic byte receive buffer
uint8_t recByte;
uint8_t recPtr;

//////////////////////////////////////////////////////////////////////////////////
/// \brief Called on interrupt on RS232 received char
/// \param The uart handler (ext_uart)
//////////////////////////////////////////////////////////////////////////////////
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	struct queueMsg_t queueMsg;		// queue message	
	static uint8_t secondSTX;									// STX repeated control
  uint32_t 	size;														// size of builded frame
	osStatus_t retCode;

	//------------------------------------------------------------------------------
	// RECEIVED CHAR
	//------------------------------------------------------------------------------
  if (recByte == STX)												// is it an STX char ?
  {
		//----------------------------------------------------------------------------
		if(recPtr == 0)													// is it first char ?
		{
			secondSTX = 0;			    							// this is the first STX
		}
		//----------------------------------------------------------------------------
		else																		// is not the first char
		{
			if (secondSTX == 1)				   					// is it the second STX
			{
				secondSTX = 0;				    					// clear secondSTX flag
				HAL_UART_Receive_IT(&ext_uart,&recByte,1);	// enable uart receiver 1 char

				return;															// and quit
			}
			secondSTX = 1;									    	// set secondSTX to 
		}
  }
	//------------------------------------------------------------------------------
  else																			// is not an STX char
  {
		if(secondSTX == 1)							    		// if was a second STX
		{
			secondSTX = 0;										    // clearswap second STX	flag
			gInBuffer[0] = STX;										// set first STX received
			recPtr = 1;														// set byte counter at 1
			HAL_UART_Receive_IT(&ext_uart,&recByte,1);	// enable uart receiver 1 char
			return;																// and quit
		}
  }
  gInBuffer[recPtr] = recByte;							// copy data in input buffer
  recPtr++;																	// increment byte counter			
	//------------------------------------------------------------------------------
  if(recPtr > 4)														// received more than 4 bytes
  {
		if(gInBuffer[1]== TOKEN_TAG)						// is it a token frame
		{
			size = TOKENSIZE;											// size is token size
		}
		else																		// not a token frame
		{
			size = gInBuffer[3]+6;								// get size in frame
		}
		if (recPtr == size)											// check all bytes received
		{
			if(recByte == ETX)										// last char received	was ETX ?
			{
				queueMsg.type = FROM_PHY;
				queueMsg.anyPtr = gInBuffer;
				//------------------------------------------------------------------------
				// QUEUE SEND	(send received frame to physical receiver)
				//------------------------------------------------------------------------
				retCode = osMessageQueuePut(
					queue_usartR_id,
					&queueMsg,
					osPriorityNormal,
					0);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);										
			}
			recPtr = 0;														// reset bytes counter
		}
  }
	HAL_UART_Receive_IT(&ext_uart,&recByte,1);	// enable uart receiver 1 char
}

//////////////////////////////////////////////////////////////////////////////////
// THREAD PHYSICAL RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void PhReceiver(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * msg;
	uint8_t * qPtr;
	size_t	size;
	osStatus_t retCode;
	
	HAL_UART_Receive_IT(&ext_uart,&recByte,1);	// enable uart receiver 1 char
	//------------------------------------------------------------------------------
	for (;;)						// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_usartR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		qPtr = queueMsg.anyPtr;
    //----------------------------------------------------------------------------
		// DEBUG DISPLAY FRAME
    //----------------------------------------------------------------------------
    putchar('R');                				    // insert a R for Receive       
    putchar(':');                				    // insert a :                   	
    DebugFrame((char*)qPtr);					  		// display frame on TERMINAL    		
		if (qPtr[1] == TOKEN_TAG)    						// is it a token frame ?
		{
		  size = TOKENSIZE;       							// yes -> token frame size
		  Ext_LED_PWM(1,100);										// token is in station
		}
		else
		{
			size = qPtr[3]+6;    									// size of string + 6 (ETX,...)
		}
		//----------------------------------------------------------------------------
		// MEMORY ALLOCATION				
		//----------------------------------------------------------------------------
		msg = osMemoryPoolAlloc(memPool,osWaitForever);											
		memcpy(msg,&qPtr[1],size-2);
		queueMsg.anyPtr = msg;
		queueMsg.type = FROM_PHY;
		if((msg[0] == TOKEN_TAG) ||				// is a token frame
			((msg[1]>>3) == gTokenInterface.myAddress) ||	// is destination my address
			((msg[0]>>3) == gTokenInterface.myAddress) ||	// is source my address
			((msg[1]>>3) == BROADCAST_ADDRESS))	// is a broadcast frame
		{
			//--------------------------------------------------------------------------
			// QUEUE SEND	(send received frame to mac layer receiver)
			//--------------------------------------------------------------------------
			retCode = osMessageQueuePut(
				queue_macR_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);							
		}
		else
		{
			//--------------------------------------------------------------------------
			// QUEUE SEND	(send received frame to physical layer sender)
			//--------------------------------------------------------------------------
			retCode = osMessageQueuePut(
				queue_phyS_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);							
		}
	}	
}


