//////////////////////////////////////////////////////////////////////////////////
/// \file touch.c
/// \brief LCD control thread
/// \author Pascal Sartoretti (pascal dot sartoretti at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"

GListener 	gl;

//////////////////////////////////////////////////////////////////////////////////
// THREAD LCD
//////////////////////////////////////////////////////////////////////////////////
void LCD(void *argument)
{
	struct queueMsg_t queueMsg;						// message queue
	GEvent* pe;														// uGFX event
	char tmpMsg[] = {0,0,0,0,0};							// to send chars
	char * msgPtr;												// any pointer of string
	char tempStr[30];											// temp string usage
	char smallStr[5];
	osStatus_t	retCode;
	uint8_t i;
	GHandle tmpHndl;
	
//	const char escapeBlack[] = {0x1B,'0',0};
	const char escapeRed[] = {0x1B,'1',0};
	const char escapeGreen[] = {0x1B,'2',0};
	const char escapeBold[] = {0x1B,'b',0};
	const char escapeNoBold[] = {0x1B,'B',0};
	const char escapeUnderline[] = {0x1B,'u',0};
	const char escapeNoUnderline[] = {0x1B,'U',0};
	const char escapeBlue[] = {0x1B,'4',0};
//	const char escapeWhite[] = {0x1B,'7',0};

	//------------------------------------------------------------------------------
	// Init the LCD and create Touch thread after
	//------------------------------------------------------------------------------
	gfxInit();											// init LCD screen	
	gdispClear(White);							// clear it
	gwinSetDefaultFont(gdispOpenFont("DejaVuSans12_aa"));
	gwinSetDefaultStyle(&WhiteWidgetStyle, FALSE);
	guiInit();											// create interface
	geventListenerInit(&gl);				// init listener
	gwinAttachListener(&gl);				// listen to LCD touch events
	guiShowPage(STARTUP);						// first window
	//------------------------------------------------------------------------------
	// special setup inits (not made in ugfx-studio)
	//------------------------------------------------------------------------------
	gwinSetColor(cnslSend,Black);
	gwinSetBgColor(cnslSend,White);
	gwinSetColor(cnslReceive,Black);
	gwinSetBgColor(cnslReceive,White);
	sprintf(tempStr,"%d",gTokenInterface.destinationAddress+1);
	gwinSetText(btnDestination, tempStr, TRUE);
	sprintf(tempStr,"%d",gTokenInterface.debugAddress+1);
	gwinSetText(lblADDR, tempStr, TRUE);
	sprintf(tempStr," %d",gTokenInterface.debugSAPI);
	gwinSetText(lblSAPI, tempStr, TRUE);
	sprintf(tempStr,"Local address is: %d",gTokenInterface.myAddress+1);
	gwinSetText(lblAddress, tempStr, TRUE);
#if DEBUG_MODE == 0
	sprintf(tempStr,"Debug mode is: OFF");
	gwinSetText(lblDebug, tempStr, TRUE);	
	gwinDisable(cbDebugConnected);
	gwinDisable(cbRecCRCError);
	gwinDisable(cbSendCRCError);
	gwinDisable(btnADDRESSMinus);
	gwinDisable(btnADDRESSPlus);
	gwinDisable(btnSAPIMinus);
	gwinDisable(btnSAPIPlus);
	gwinDisable(lblSAPI);
	gwinDisable(lblADDR);
	gwinDisable(btnSendDebug);
#else
	sprintf(tempStr,"Debug mode is: ON");
	gwinSetText(lblDebug, tempStr, TRUE);	
#endif
	
	//------------------------------------------------------------------------------
	for(;;)													// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_lcd_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);						
		switch(queueMsg.type)								// check message
		{
			//**************************************************************************
			case TOUCH_EVENT:												// event from touchscreen
				pe = queueMsg.anyPtr;
				switch(pe->type) 
				{
				//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
				case GEVENT_GWIN_BUTTON:								// button has been pressed
					tmpHndl = ((GEventGWinButton*)pe)->gwin;
					//----------------------------------------------------------------------
					if((tmpHndl == btnToken)||						// leave startup window
						(tmpHndl == btnStart))
					{
						gTokenInterface.currentView = MAINDISPLAY;
						guiShowPage(MAINDISPLAY);
					}
					//----------------------------------------------------------------------
					if(tmpHndl == btnDestination)					// enter address select window
					{
						gTokenInterface.currentView = ADDRESSSELECTDISPLAY;
						guiShowPage(ADDRESSSELECTDISPLAY);
					}
					//----------------------------------------------------------------------
					if(tmpHndl == btnConfiguration)				// enter config window
					{
						gTokenInterface.currentView = CONFIGDISPLAY;
						guiShowPage(CONFIGDISPLAY);
					}
					//----------------------------------------------------------------------
					if((tmpHndl == btnBack)||							// leave config window
						(tmpHndl == btnSendDebug))
					{
						gTokenInterface.currentView = MAINDISPLAY;
						guiShowPage(MAINDISPLAY);
					}
					//----------------------------------------------------------------------
					if(tmpHndl == btnSelect)							// destination changed
					{
						if(gTokenInterface.destinationAddress != 15)
						{
							sprintf(tempStr,"%d",gTokenInterface.destinationAddress+1);
						}
						else
						{
							sprintf(tempStr,"All");
						}
						gwinSetText(btnDestination, tempStr, TRUE);	// display it on widget
						gTokenInterface.currentView = MAINDISPLAY;
						guiShowPage(MAINDISPLAY);
					}
					//----------------------------------------------------------------------
					if(tmpHndl == btnSAPIMinus)						// SAPI changed
					{
						if(gTokenInterface.debugSAPI > 0)
						{
							gTokenInterface.debugSAPI--;
							tmpMsg[0] = ' ';
							tmpMsg[1] = gTokenInterface.debugSAPI + '0';
							tmpMsg[2] = 0;
							gwinSetText(lblSAPI, tmpMsg, TRUE);
						}						
					}
					//----------------------------------------------------------------------
					if(tmpHndl == btnSAPIPlus)						// SAPI changed
					{
						if(gTokenInterface.debugSAPI < 7)
						{
							gTokenInterface.debugSAPI++;
							tmpMsg[0] = ' ';
							tmpMsg[1] = gTokenInterface.debugSAPI + '0';
							tmpMsg[2] = 0;
							gwinSetText(lblSAPI, tmpMsg, TRUE);
						}
					}
					//----------------------------------------------------------------------
					if(tmpHndl == btnADDRESSMinus)				// address changed
					{
						if(gTokenInterface.debugAddress > 0)
						{
							gTokenInterface.debugAddress--;
							if(gTokenInterface.debugAddress == gTokenInterface.myAddress)
							{
								if(gTokenInterface.debugAddress > 0)
								{
									gTokenInterface.debugAddress--;
								}
								else
								{
									gTokenInterface.debugAddress++;
								}
							}
							if(gTokenInterface.debugAddress > 8)
							{
								sprintf(tmpMsg,"%d",(int)gTokenInterface.debugAddress+1);
							}
							else
							{
								sprintf(tmpMsg," %d",(int)gTokenInterface.debugAddress+1);
							}
							gwinSetText(lblADDR, tmpMsg, TRUE);
						}					
					}
					//----------------------------------------------------------------------
					if(tmpHndl == btnADDRESSPlus)					// address changed
					{
						if(gTokenInterface.debugAddress < 14)
						{
							gTokenInterface.debugAddress++;
							if(gTokenInterface.debugAddress == gTokenInterface.myAddress)
							{
								if(gTokenInterface.debugAddress < 14)
								{
									gTokenInterface.debugAddress++;
								}
								else
								{
									gTokenInterface.debugAddress--;
								}
							}
							if(gTokenInterface.debugAddress > 8)
							{
								sprintf(tmpMsg,"%d",(int)gTokenInterface.debugAddress+1);
							}
							else
							{
								sprintf(tmpMsg," %d",(int)gTokenInterface.debugAddress+1);
							}
							gwinSetText(lblADDR, tmpMsg, TRUE);
						}		
					}
				break;
				//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
				case GEVENT_GWIN_CHECKBOX:							// checkbox has been pressed
					tmpHndl = ((GEventGWinButton*)pe)->gwin;
					//----------------------------------------------------------------------
					if(tmpHndl == cbConnectoed)						// connection changed
					{
						gTokenInterface.connected = gwinCheckboxIsChecked(cbConnectoed);						
					}
					//----------------------------------------------------------------------
					if(tmpHndl == cbBroadcastTime)				// broadcast time changed
					{
						gTokenInterface.broadcastTime = gwinCheckboxIsChecked(cbBroadcastTime);						
					}
					//----------------------------------------------------------------------
					if(tmpHndl == cbDebugConnected)				// debug connected changed
					{
						gTokenInterface.debugOnline = gwinCheckboxIsChecked(cbDebugConnected);						
					}
					//----------------------------------------------------------------------
					if(tmpHndl == cbRecCRCError)					// receive CRC error changed
					{
						gTokenInterface.needReceiveCRCError = gwinCheckboxIsChecked(cbRecCRCError);						
					}
					//----------------------------------------------------------------------
					if(tmpHndl == cbSendCRCError)					// send CRC error changed
					{
						gTokenInterface.needSendCRCError = gwinCheckboxIsChecked(cbSendCRCError);						
					}
				break;
				//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
				case GEVENT_GWIN_RADIO:									// radio button has been pressed
					tmpHndl = ((GEventGWinButton*)pe)->gwin;
					//----------------------------------------------------------------------
					gTokenInterface.destinationAddress = gwinGetTag(tmpHndl);
				break;
			}
			break;
			//**************************************************************************
			case TIME_MSG:														// needs to display the time
				
				msgPtr = queueMsg.anyPtr;
				sprintf(tempStr,"Time is: %s",msgPtr);	// create string
				gwinSetText(lblTime, tempStr, TRUE);		// display it on widget
				//------------------------------------------------------------------------
				// MEMORY RELEASE	(time frame from timeReceiver)
				//------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,msgPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				//------------------------------------------------------------------------
				// set event flag to audio player
				//------------------------------------------------------------------------
				retCode = osEventFlagsSet(eventFlag_id, AUDIO_CLOCK_EVT);	// set flag
				if(retCode < 0)
				{
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);									
				}				
			break;
			//--------------------------------------------------------------------------
			case CHAR_MSG:														// a char has been pressed
				msgPtr = queueMsg.anyPtr;
				gwinPutString(cnslSend,msgPtr);					// display char
				//------------------------------------------------------------------------
				// MEMORY RELEASE	(character from chatSender)
				//------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,msgPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
			break;
			//--------------------------------------------------------------------------
			case CHAT_MSG:														// a message is incoming
				if(gTokenInterface.currentView != MAINDISPLAY)
				{
					guiShowPage(MAINDISPLAY);
				}
				msgPtr = queueMsg.anyPtr;
				sprintf(tempStr,"Msg from : %d\r\n",queueMsg.addr+1);
				gwinPutString(cnslReceive,escapeBlue);
				gwinPutString(cnslReceive,escapeUnderline);
				gwinPutString(cnslReceive,tempStr);
				gwinPutString(cnslReceive,escapeNoUnderline);
				gwinPutString(cnslReceive,escapeGreen);
				gwinPutString(cnslReceive,msgPtr);
				gwinPutString(cnslReceive,"\r\n");
				//------------------------------------------------------------------------
				// MEMORY RELEASE	(message from chatReceiver)
				//------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,msgPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
				//------------------------------------------------------------------------
				// set event flag to audio player
				//------------------------------------------------------------------------
				retCode = osEventFlagsSet(eventFlag_id, AUDIO_MSG_EVT);	// set flag
				if(retCode < 0)
				{
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);									
				}				
			break;
			//--------------------------------------------------------------------------
			case MAC_ERROR:											// a communication error occurs
				if(gTokenInterface.currentView != MAINDISPLAY)
				{
					guiShowPage(MAINDISPLAY);
				}
				msgPtr = queueMsg.anyPtr;
				gwinPutString(cnslReceive,escapeRed);
				gwinPutString(cnslReceive,escapeBold);
				gwinPutString(cnslReceive,msgPtr);
				gwinPutString(cnslReceive,escapeNoBold);
				//------------------------------------------------------------------------
				// MEMORY RELEASE	(message from macSenderReceiver)
				//------------------------------------------------------------------------
				retCode = osMemoryPoolFree(memPool,msgPtr);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
				//------------------------------------------------------------------------
				// set event flag to audio player
				//------------------------------------------------------------------------
				retCode = osEventFlagsSet(eventFlag_id, AUDIO_ERROR_EVT);	// set flag
				if(retCode < 0)
				{
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);									
				}				
			break;
			//--------------------------------------------------------------------------
			case TOKEN_LIST:									// token list update
				
				sprintf(tempStr,"Online stations: ");	// create string
				for(i=0;i<15;i++)
				{
					//----------------------------------------------------------------------
					// Not connected station
					//----------------------------------------------------------------------
					if((gTokenInterface.station_list[i] & (1 << CHAT_SAPI)) == 0)
					{
						// station not connected
					}
					//----------------------------------------------------------------------
					// Any station number connected (CHAT_SAPI)
					//----------------------------------------------------------------------
					else
					{
						sprintf(smallStr,"%d, ",i+1);
						strcat(tempStr,smallStr);						
					}
				}
				tempStr[strlen(tempStr)-2] = 0;								// discare last ', '
				gwinSetText(lblList, tempStr, TRUE);				// display it on widget
			break;
	//------------------------------------------------------------------------------
	default:
		break;
	}	// end of SWITCH
	} 	// end of forever
}


