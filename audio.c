//////////////////////////////////////////////////////////////////////////////////
/// \file audio.c
/// \brief Audio thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include "main.h"
#include "audio_msg.c"
#include "audio_error.c"
#include "audio_clock.c"
#include "Board_Audio.h"

extern uint8_t gI2CAccess;

//////////////////////////////////////////////////////////////////////////////////
// THREAD AUDIO
//////////////////////////////////////////////////////////////////////////////////
void AudioPlayer(void *argument)
{
	int32_t	eventFlag;											// current flag	

	//------------------------------------------------------------------------------
	// Initialize the audio interface (need to be intialised first)
	// this thread has a most important priority and block kernel switch when
	// intilalising the audio interface
	//------------------------------------------------------------------------------
	while(gI2CAccess != 0){}
	osKernelLock();
  Audio_Initialize   (NULL);
  Audio_SetDataFormat(AUDIO_STREAM_OUT, AUDIO_DATA_16_MONO);
  Audio_SetFrequency (AUDIO_STREAM_OUT,16000);
  Audio_SetMute      (AUDIO_STREAM_OUT, AUDIO_CHANNEL_MASTER, false);
  Audio_SetVolume    (AUDIO_STREAM_OUT, AUDIO_CHANNEL_MASTER, 50);
  Audio_Start        (AUDIO_STREAM_OUT);
	//------------------------------------------------------------------------------
	// Bacause of audio initialize use the I2C interrupts and the uGFX touch
	// protocol don't use it, we neeed to clear this register
//	I2C3->CR1 = 0;	
	osKernelUnlock();
	
	//------------------------------------------------------------------------------
  while (1) 															// forever
	{
		//----------------------------------------------------------------------------
		//	EVENT GET wait always (AUDIO_xxx_EVT)
		//----------------------------------------------------------------------------
		eventFlag = osEventFlagsWait(
			eventFlag_id,
			AUDIO_MSG_EVT | AUDIO_ERROR_EVT| AUDIO_CLOCK_EVT,
			osFlagsWaitAny,
			osWaitForever); 	
		if(eventFlag < 0)				// case of error
			CheckRetCode(eventFlag,__LINE__,__FILE__,CONTINUE);	
		//----------------------------------------------------------------------------
		if((eventFlag & AUDIO_MSG_EVT) == AUDIO_MSG_EVT)		// play incoming message
		{
				Audio_SendData(audio_msg, sizeof(audio_msg)/2);
		}
		if((eventFlag & AUDIO_ERROR_EVT) == AUDIO_ERROR_EVT)	// play error message
		{
				Audio_SendData(audio_error, sizeof(audio_error)/2);
		}
		if((eventFlag & AUDIO_CLOCK_EVT) == AUDIO_CLOCK_EVT)	// play clock message
		{
				Audio_SendData(audio_clock, sizeof(audio_clock)/2);
		}
  }
}


