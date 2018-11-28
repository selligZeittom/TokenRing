#include "armebs3.h"			                    /* peripherals definitions		*/
#include "mezza_lcd\mezza_lcd.h" 	            /* LCD Mezza library 					*/
#include <tx_api.h>														/* threadX def. header file		*/
#include <stdlib.h>														/* std lib definitions file		*/
#include <stdio.h>              							/* standard IO functions			*/
#include <tokenring.h>												/* tokenring constants				*/


/*--------------------------------------------------------------------------*/
/* EXTERNAL DEFINED GLOBAL VARIABLES																				*/
/*--------------------------------------------------------------------------*/
extern UINT8	gDebugSend;
extern UINT8	gDebugAddress;
extern UINT8	gDebugSapi;
extern UINT8	gDebugSendCrcError;
extern UINT8	gDebugReceiveCrcError;
extern UINT8	gInBuffer[300];
extern UINT8  MYADDRESS;
extern TX_QUEUE	gDebug_Q;
extern TX_EVENT_FLAGS_GROUP	gEvents_E;

/****************************************************************************/
/* Name			: CheckRetCodeDebug																							*/
/*--------------------------------------------------------------------------*/
/* Inputs		: retCode, lineNumber, mode																			*/
/*--------------------------------------------------------------------------*/
/* Function	:	- displays the error when an error happends										*/
/****************************************************************************/
void CheckRetCodeDebug(UINT32 retCode,UINT32 lineNumber,UINT8 mode)
{
  if(retCode != 0)															/* when error occurs				*/
  {
		Debug("In Debugger line : ",lineNumber);		/* display line number			*/
		Debug("Error   : ",retCode);								/* with error number				*/
		if (mode == HALT)														/* when mode is HALT				*/
    {
			while(1){}																/* stays here forever				*/
    }
  }
}

/****************************************************************************/
/* TaskName	: DebugManager																									*/
/*--------------------------------------------------------------------------*/
/* Function	:	- simulate a station with the tokenring interface							*/
/****************************************************************************/
void DebugManager(void)
{
  HEADER 	msg;
  UINT8		*ptr;
  UINT8		*tokenPtr;
  INT32		retCode;
  UINT8		msgStr[]="Test Message from Debug !";
  UINT8		srcControl;
  UINT8		dstControl;
  UINT8		size;
  UINT8		checksum;
  UINT8		i;
  UINT8		lastDebugAddress;
  UINT8		statusByte;
  
  gDebugSend = 0;
  gDebugAddress = 6;
  gDebugSapi = 1;
  gDebugSendCrcError = 0;
  gDebugReceiveCrcError = 0;
  lastDebugAddress = gDebugAddress;
	/*------------------------------------------------------------------------*/
  while(1)																					/* forever loop					*/
  {
		/*----------------------------------------------------------------------*/
		/* TX_CALL   >>>>>>>>>>>>>>>>>                     QUEUE READ						*/
		/*----------------------------------------------------------------------*/
		retCode = tx_queue_receive(
			&gDebug_Q,
			&msg,
			TX_WAIT_FOREVER);
		CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
		ptr = msg.data.ptr;
		/*----------------------------------------------------------------------*/
		/* 																								 TOKEN RECEIVED				*/
		/*----------------------------------------------------------------------*/
		if(*(ptr+1) == TOKEN)
		{
			ptr[gDebugAddress+2] = (1 << gDebugSapi);				/* update dbg SAPI		*/
			if(lastDebugAddress != gDebugAddress)						/* if address changed	*/
			{
				ptr[lastDebugAddress+2] &= ~(1 << gDebugSapi);	/* update last			*/
				lastDebugAddress = gDebugAddress;	  						/* set new<->last		*/
			}
			/*--------------------------------------------------------------------*/
			/*																							 WAIT TO SEND MSG			*/
			/*--------------------------------------------------------------------*/
			if(gDebugSend == 1)													
			{
				gDebugSend = 0;														/* stop send nxt time	*/
				tokenPtr = ptr;
				srcControl = (gDebugAddress << 3) + gDebugSapi;	
				dstControl = (MYADDRESS << 3) + gDebugSapi;
				size = strlen(msgStr);
				*gInBuffer = 0x02;
				/*------------------------------------------------------------------*/
				/*																							 MSG CREATION				*/
				/*------------------------------------------------------------------*/
				*(gInBuffer+1) = srcControl;
				*(gInBuffer+2) = dstControl;
				*(gInBuffer+3) = size;
				strcpy(gInBuffer+4,msgStr);
				*(gInBuffer+3+size) = 0x0D;
				checksum = 0;
				for(i=0;i<size+3;i++)
					checksum += *(gInBuffer+i+1);
				*(gInBuffer+size+4) = checksum << 2;
				*(gInBuffer+size+5) = 0x03;
				/*------------------------------------------------------------------*/
				if(gDebugSendCrcError == 1)
				{
					*(gInBuffer+size+4) += 4;										/* simulate error CS	*/
				}
				/*------------------------------------------------------------------*/
				/* TX_CALL   >>>>>>>>>>>>>>>										 EVENT SET					*/
				/*------------------------------------------------------------------*/
				retCode = tx_event_flags_set(
						&gEvents_E,
						RS232_FR_EVENT,
						TX_OR);
				CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
			}
			/*--------------------------------------------------------------------*/
			/*																							 NO MSG TO SEND				*/
			/*--------------------------------------------------------------------*/			
			else																						/* token back					*/
			{
				for(i=0;i<19;i++)
				{
					gInBuffer[i] = *(ptr+i);  
				}
				retCode = tx_byte_release((void*)ptr);
				CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
				/*------------------------------------------------------------------*/
				/* TX_CALL   >>>>>>>>>>>>>>>										 EVENT SET					*/
				/*------------------------------------------------------------------*/
				retCode = tx_event_flags_set(
						&gEvents_E,
						RS232_FR_EVENT,
						TX_OR);
				CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
			}
		}
		/*----------------------------------------------------------------------*/
		/* 																								 DATABACK RECEIVED		*/
		/*----------------------------------------------------------------------*/
		else if (*(ptr+1)>>3 == gDebugAddress)
		{
			statusByte = *(ptr+*(ptr+3)+4);
			statusByte &= 0x03;
			/*--------------------------------------------------------------------*/
			switch(statusByte)
			{
				/*------------------------------------------------------------------*/
				/*																						 RD = 1, ACK = 1			*/
				/*------------------------------------------------------------------*/
				case 0x03:																	/* RD = 1, ACK = 1			*/
					/*----------------------------------------------------------------*/
					/* TX_CALL   >>>>>>>>>>>>>									 MEMORY RELEASE				*/
					/*----------------------------------------------------------------*/
					retCode = tx_byte_release((void*)ptr);
					for(i=0;i<19;i++)
					{
						gInBuffer[i] = *(tokenPtr+i);  
					}
					/*----------------------------------------------------------------*/
					/* TX_CALL   >>>>>>>>>>>>>									 MEMORY RELEASE				*/
					/*----------------------------------------------------------------*/
					retCode = tx_byte_release((void*)tokenPtr);
					CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
					/*----------------------------------------------------------------*/
					/* TX_CALL   >>>>>>>>>>>>>									 EVENT SET						*/
					/*----------------------------------------------------------------*/
					retCode = tx_event_flags_set(
						&gEvents_E,
						RS232_FR_EVENT,
						TX_OR);
					CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
				break;
				/*------------------------------------------------------------------*/
				/*																						 RD = 1, ACK = 0			*/
				/*------------------------------------------------------------------*/
	      case 0x02:																	/* RD = 1, ACK = 0			*/
					/*----------------------------------------------------------------*/
					/* TX_CALL   >>>>>>>>>>>>>									 MEMORY RELEASE				*/
					/*----------------------------------------------------------------*/
					retCode = tx_byte_release((void*)ptr);
					srcControl = (gDebugAddress << 3) + gDebugSapi;
					dstControl = (MYADDRESS << 3) + CHAT_SAPI;
					size = strlen(msgStr);
					*gInBuffer = 0x02;
					*(gInBuffer+1) = srcControl;
					*(gInBuffer+2) = dstControl;
					*(gInBuffer+3) = size;
					strcpy(gInBuffer+4,msgStr);
					*(gInBuffer+3+size) = 0x0D;
					checksum = 0;
					for(i=0;i<size+3;i++)
						checksum += *(gInBuffer+i+1);
					*(gInBuffer+size+4) = checksum << 2;
					*(gInBuffer+size+5) = 0x03;
					/*----------------------------------------------------------------*/
					if(gDebugSendCrcError == 1)
					{
						*(gInBuffer+size+4) += 4;									/* simulate error CS	*/
					}
					/*----------------------------------------------------------------*/
					/* TX_CALL   >>>>>>>>>>>>>									 EVENT SET						*/
					/*----------------------------------------------------------------*/
					retCode = tx_event_flags_set(
						&gEvents_E,
						RS232_FR_EVENT,
						TX_OR);
					CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
					Debug("DBL>> MESSAGE ACK ERROR -> SEND MESSAGE AGAIN ",0);
				break;
				/*------------------------------------------------------------------*/
				/*																						 RD = 0, ACK = x			*/
				/*------------------------------------------------------------------*/
	      case 0x01:																	/* RD = 0, ACK = 1			*/
	      case 0x00:																	/* RD = 0, ACK = 0			*/
					/*----------------------------------------------------------------*/
					/* TX_CALL   >>>>>>>>>>>>>									 MEMORY RELEASE				*/
					/*----------------------------------------------------------------*/
					retCode = tx_byte_release((void*)ptr);
					for(i=0;i<19;i++)
					{
						gInBuffer[i] = *(tokenPtr+i);  
					}
					/*----------------------------------------------------------------*/
					/* TX_CALL   >>>>>>>>>>>>>									 MEMORY RELEASE				*/
					/*----------------------------------------------------------------*/
					retCode = tx_byte_release((void*)tokenPtr);
					CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
					/*----------------------------------------------------------------*/
					/* TX_CALL   >>>>>>>>>>>>>									 EVENT SET						*/
					/*----------------------------------------------------------------*/
					retCode = tx_event_flags_set(
							&gEvents_E,
							RS232_FR_EVENT,
							TX_OR);
					CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
					Debug("DBL>> MESSAGE NOT READ -> SEND BACK TOKEN ",0);		  
				break;
			}
		}
		/*----------------------------------------------------------------------*/
		/* 																								 MESSAGE RECEIVED			*/
		/*----------------------------------------------------------------------*/
		else if (*(ptr+2)>>3 == gDebugAddress)
		{
			statusByte = *(ptr+*(ptr+3)+4);
			/*--------------------------------------------------------------------*/
			if((gDebugReceiveCrcError == 1) &&					/* pseudo error					*/
					((*(ptr+2) && 0x03) == gDebugSapi))
			{
				*(ptr+*(ptr+3)+4) |= 0x02;									/* set RD bit						*/
				*(ptr+*(ptr+3)+4) &= 0xFE;									/* clear ACK bit				*/
				Debug("DBL>> PSEUDO ACK ERROR ",0);
			}
			/*--------------------------------------------------------------------*/
			else if((*(ptr+2) && 0x03) == gDebugSapi)			/* control is OK				*/
			{
				checksum = 0;
				for(i=0;i<*(ptr+3)+3;i++)										/* calculate checksum		*/
					checksum += *(ptr+i+1);
				checksum = checksum << 2;
				if(checksum == (statusByte & 0xFC))
				{
					*(ptr+*(ptr+3)+4) = statusByte | 0x03;		/* RD = 1, ACK = 1			*/
					Debug("DBL>> ACK OK ",0);
				}
				else
				{
					*(ptr+*(ptr+3)+4) = statusByte | 0x02;		/* RD = 1, ACK = 0			*/
					Debug("DBL>> ACK ERROR ",0);
				}
			}
			/*--------------------------------------------------------------------*/
			else																					/* bad SAPI							*/
			{
				Debug("DBL>> BAD SAPI ",0);		
			}
			for(i=0;i<*(ptr+3)+6;i++)
			{
				gInBuffer[i] = *(ptr+i);
			}
			/*--------------------------------------------------------------------*/
			/* TX_CALL   >>>>>>>>>>>>>>>									   MEMORY RELEASE				*/
			/*--------------------------------------------------------------------*/
			retCode = tx_byte_release((void*)ptr);	  
			/*--------------------------------------------------------------------*/
			/* TX_CALL   >>>>>>>>>>>>>>>									   EVENT SET						*/
			/*--------------------------------------------------------------------*/
			retCode = tx_event_flags_set(
				&gEvents_E,
				RS232_FR_EVENT,
				TX_OR);
			CheckRetCodeDebug(retCode,__LINE__,CONTINUE);
		}
		/*----------------------------------------------------------------------*/
		/* 																								 ANOTHER FRAME				*/
		/*----------------------------------------------------------------------*/
		else
		{
			for(i=0;i<*(ptr+3)+6;i++)												/* copy frame					*/
			{
				gInBuffer[i] = *(ptr+i);
			}
			/*--------------------------------------------------------------------*/
			/* TX_CALL   >>>>>>>>>>>>>>>									   MEMORY RELEASE				*/
			/*--------------------------------------------------------------------*/
			retCode = tx_byte_release((void*)ptr);	  
			/*--------------------------------------------------------------------*/
			/* TX_CALL   >>>>>>>>>>>>>>>									   EVENT SET						*/
			/*--------------------------------------------------------------------*/
			retCode = tx_event_flags_set(
				&gEvents_E,
				RS232_FR_EVENT,
				TX_OR);
			CheckRetCodeDebug(retCode,__LINE__,CONTINUE);		
		}
  }
}
