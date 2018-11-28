//////////////////////////////////////////////////////////////////////////////////
/// \file debug.c
/// \brief Debug thread (simulation of a connected station)
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "ext_led.h"

extern uint8_t gInBuffer[256];											// generic byte receive buffer


//////////////////////////////////////////////////////////////////////////////////
// THREAD DEBUG
//////////////////////////////////////////////////////////////////////////////////
void DebugStation(void *argument)
{
	struct queueMsg_t queueMsg;			// queue message
	uint8_t * tokenPtr=0;
	uint8_t * qPtr;
	uint8_t checksum;
	uint8_t statusByte;
	uint8_t i;
	uint8_t * msg;
	uint8_t lastDebugAddress=0;
	uint8_t waitForDataback=0;
	const uint8_t debugMsg[] = "Msg from debug !";
	osStatus_t retCode;

	enum
	{
		isTOKEN,         	  	        	// a TOKEN frame is received
		isSOURCE,          			    		// a SOURCE frame is received
		isDEST,          		          	// a DEST. frame is received
		isERROR,           				  		// a BAD frame is received
		isBROADCAST,
	}frameType;
	//------------------------------------------------------------------------------
	for (;;)						// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_dbg_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
		qPtr = queueMsg.anyPtr;
		if (qPtr[0]==TOKEN_TAG)
		{
			frameType = isTOKEN;
		}
		//----------------------------------------------------------------------------
		else if ((qPtr[1]>>3) == gTokenInterface.debugAddress)  // is a dest. frame
		{
			if(gTokenInterface.debugOnline != FALSE)
			{
				frameType = isDEST;
			}
			else
			{
				frameType = isERROR;	// station not connected
			}
		}
		//----------------------------------------------------------------------------
		else if (qPtr[0]>>3 ==  gTokenInterface.debugAddress) 	// is it a source frame
		{
			frameType = isSOURCE;
		}
		//----------------------------------------------------------------------------
		else if (qPtr[1]>>3 == BROADCAST_ADDRESS) 	// is it a broadcast
		{
			frameType = isBROADCAST;
		}
		//----------------------------------------------------------------------------
		else                                    		// is unknow
		{
			frameType = isERROR;
		}
		switch(frameType)
		{
		//****************************************************************************
		case isTOKEN:
			qPtr[gTokenInterface.debugAddress+1] = (1 << TIME_SAPI);
			if(gTokenInterface.debugOnline != FALSE)
			{
				qPtr[gTokenInterface.debugAddress+1] |= (1 << gTokenInterface.debugSAPI);
			}
			if(lastDebugAddress != gTokenInterface.debugAddress)	// if address changed
			{
				qPtr[lastDebugAddress+1] = 0;	// update last
				lastDebugAddress = gTokenInterface.debugAddress;	// set new<->last
			}
			//--------------------------------------------------------------------------
			if(gTokenInterface.debugMsgToSend != FALSE)
			{
				waitForDataback = 1;
				gTokenInterface.debugMsgToSend = FALSE;
				tokenPtr = qPtr;	// keep copy of token
				//------------------------------------------------------------------------
				// MEMORY ALLOCATION				
				//------------------------------------------------------------------------
				msg = osMemoryPoolAlloc(memPool,osWaitForever);													
				msg[0] = (gTokenInterface.debugAddress << 3) | gTokenInterface.debugSAPI;
				msg[1] = (gTokenInterface.myAddress << 3) | gTokenInterface.debugSAPI;
				msg[2] = sizeof(debugMsg)-1;
				memcpy(&msg[3],debugMsg,sizeof(debugMsg)-1);
				checksum = 0;
				for(i=0;i<(msg[2]+3);i++)	// calculate checksum
					checksum += msg[i];
				if(gTokenInterface.needSendCRCError != FALSE)
				{
					checksum += 1;
					printf(">> Debug send pseudo error <<\r\n");
				}
				else
				{
					printf(">> Debug send message ok <<\r\n");
				}
				checksum = checksum << 2;
				msg[sizeof(debugMsg) -1 + 3] = checksum;
				queueMsg.anyPtr = msg;
			}
			break;
		//****************************************************************************
		case isDEST:
			statusByte = qPtr[qPtr[2] + 3];
			//--------------------------------------------------------------------------
			if((gTokenInterface.needReceiveCRCError != FALSE) &&		// pseudo error
					((qPtr[1] && 0x03) == gTokenInterface.debugSAPI))
			{
				printf(">> Debug answer pseudo error <<\r\n");
				qPtr[qPtr[2] + 3] |= 0x02;	// set RD bit
				qPtr[qPtr[2] + 3] &= 0xFE;	// clear ACK bit
			}
			//--------------------------------------------------------------------------
			else if((qPtr[1] && 0x03) == gTokenInterface.debugSAPI)		// control is OK
			{
				checksum = 0;
				for(i=0;i<(qPtr[2]+3);i++)	// calculate checksum
					checksum += qPtr[i];
				checksum = checksum << 2;
				if(checksum == (statusByte & 0xFC))	// checksum OK
				{
					printf(">> Debug answer ok <<\r\n");
					qPtr[qPtr[2] + 3] |= 0x03;	// set RD & ACK bits
				}
				else								// checksum real error
				{
					printf(">> Debug answer error detected <<\r\n");
					qPtr[qPtr[2] + 3] |= 0x02;	// set RD bit
					qPtr[qPtr[2] + 3] &= 0xFE;	// clear ACK bit
				}
			}
			//--------------------------------------------------------------------------
			else							// bad SAPI
			{
			}
			break;
		//****************************************************************************
		case isSOURCE:
			if(waitForDataback == 0)					// I've send nothing ...
			{
				break;
			}
			waitForDataback = 0;
			checksum = qPtr[qPtr[2] + 3];
			switch(checksum & 0x03)
			{
			case 0x03:				// all was OK
			case 0x01:				// or not RD
			case 0x00:				// or not RD
				//------------------------------------------------------------------------
				// MEMORY RELEASE	
				//------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,qPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
				queueMsg.anyPtr = tokenPtr;
				break;
			case 0x02:				// RD but not ACK
				waitForDataback = 1;
				//------------------------------------------------------------------------
				// MEMORY RELEASE	
				//------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,qPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
				//------------------------------------------------------------------------
				// MEMORY ALLOCATION				
				//------------------------------------------------------------------------
				msg = osMemoryPoolAlloc(memPool,osWaitForever);													
				msg[0] = (gTokenInterface.debugAddress << 3) | gTokenInterface.debugSAPI;
				msg[1] = (gTokenInterface.myAddress << 3) | gTokenInterface.debugSAPI;
				msg[2] = sizeof(debugMsg)-1;
				memcpy(&msg[3],debugMsg,sizeof(debugMsg)-1);
				checksum = 0;
				for(i=0;i<(msg[2]+3);i++)	// calculate checksum
					checksum += msg[i];
				if(gTokenInterface.needSendCRCError != FALSE)
				{
					printf(">> Debug RE-send pseudo error <<\r\n");
					checksum += 1;
				}
				else
				{
					printf(">> Debug RE-send without error <<\r\n");
				}
				checksum = checksum << 2;
				msg[sizeof(debugMsg) -1 + 3] = checksum;
				queueMsg.anyPtr = msg;
				break;
			}
			break;
		default:	// IS Error or unknow
			break;
		}
		osDelay(300);						// wait 500 ms for simulation delay
		qPtr = queueMsg.anyPtr;
		if(qPtr[0] == TOKEN_TAG)
		{
		  Ext_LED_PWM(1,100);		// token is in station
		}
		queueMsg.type = FROM_PHY;
		//----------------------------------------------------------------------------
		// DEBUG DISPLAY FRAME				
		//----------------------------------------------------------------------------
		DebugMacFrame('R',qPtr);
		//----------------------------------------------------------------------------
		// QUEUE SEND	(send message to mac receiver)
		//----------------------------------------------------------------------------
		retCode = osMessageQueuePut(
			queue_macR_id,
			&queueMsg,
			osPriorityNormal,
			0);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);								
	}
}


