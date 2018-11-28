/*-----------------------------------------------------------------------------
 * Name:    Audio_756G_EVAL.c
 * Purpose: Audio interface for STM32756G-EVAL Board
 * Rev.:    1.0.0
 *----------------------------------------------------------------------------*/

/* Copyright (c) 2015 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/

#include "stm32f7xx_hal.h"

#include "cmsis_os2.h"
#include "Driver_I2C.h"
#include "Driver_SAI.h"
#include "Board_Audio.h"


// WM8994 Outputs (Available: Headphone1 and speaker output)
#define WM8994_OUTPUT_NONE     (0U)
#define WM8994_OUTPUT_HPOUT1   (1U)
#define WM8994_OUTPUT_SPKOUT   (2U)

#ifndef WM8994_OUTPUT
#define WM8994_OUTPUT          (WM8994_OUTPUT_HPOUT1 | WM8994_OUTPUT_SPKOUT)
#endif

// WM8994 Inputs (Available: Digital microphone 2 input and Line IN1)
#define WM8994_INPUT_NONE      (0U)
#define WM8994_INPUT_DMIC2     (1U)
#define WM8994_INPUT_IN1       (2U)

#ifndef WM8994_INPUT
#define WM8994_INPUT           (WM8994_INPUT_DMIC2)
#endif

#if (WM8994_INPUT == (WM8994_INPUT_DMIC2 | WM8994_INPUT_IN1))
#error "Digital microphone 2 and Line IN1, can not be used together. Please select just one!"
#endif


// I2C
#ifndef WM8994_I2C_PORT
#define WM8994_I2C_PORT        3         // I2C Port number
#endif

#define WM8994_I2C_ADDR        0x1A      // WM8994 I2C address

// I2C Driver
#define _I2C_Driver_(n)  Driver_I2C##n
#define  I2C_Driver_(n) _I2C_Driver_(n)
extern ARM_DRIVER_I2C    I2C_Driver_(WM8994_I2C_PORT);
#define ptrI2C         (&I2C_Driver_(WM8994_I2C_PORT))

// SAI
#ifndef WM8994_SAI_PORT_OUT
#define WM8994_SAI_PORT_OUT    2         // SAI Port number
#endif

#ifndef WM8994_SAI_PORT_IN
#define WM8994_SAI_PORT_IN     2         // SAI Port number
#endif

// SAI Driver
#define _SAI_Driver_(n)  Driver_SAI##n
#define  SAI_Driver_(n) _SAI_Driver_(n)
extern ARM_DRIVER_SAI    SAI_Driver_(WM8994_SAI_PORT_OUT);
#define ptrSAI_OUT      (&SAI_Driver_(WM8994_SAI_PORT_OUT))
extern ARM_DRIVER_SAI    SAI_Driver_(WM8994_SAI_PORT_IN);
#define ptrSAI_IN       (&SAI_Driver_(WM8994_SAI_PORT_IN))

// Pointer to Stream info
#define STREAM_PTR(stream) ((stream & 0x80U) == 0U ? (&Audio.Out) : (&Audio.In))


// Audio Stream run-time information
typedef struct _STREAM_INFO {
  void                *pData;
  uint32_t             DataSize;
  uint8_t              Volume_L;
  uint8_t              Volume_R;
  uint8_t              Mono;
  uint8_t              Running;
  uint8_t              Paused;
  uint8_t              UnderflowPending;
  uint8_t              SendCompletePending;
} STREAM_INFO;

// Audio Stream
typedef struct _STREAM {
  ARM_DRIVER_SAI      *SAI;
  STREAM_INFO         *Info;
} STREAM;

// Audio run-time information
typedef const struct _AUDIO_RESOURCES {
  STREAM               Out;
  STREAM               In;
} AUDIO_RESOURCES;

static uint8_t     DataBits      = 16U;
static uint32_t    SamplingFreq  = 16000U;
static STREAM_INFO StreamInfoOut = { 0U };
static STREAM_INFO StreamInfoIn  = { 0U };

static const AUDIO_RESOURCES Audio = {
  ptrSAI_OUT,
  &StreamInfoOut,
  ptrSAI_IN,
  &StreamInfoIn,
};

static Audio_SignalEvent_t CB_Event;


// Callback
/**
  \fn          void (uint16_t reg, uint16_t reg_val)
  \brief       SAI callback function
  \param[in]   event
*/
void SAI_callback (uint32_t event) {

  if (event & ARM_SAI_EVENT_FRAME_ERROR) {
    // Audio interface does not support FRAME ERRORS
    event &= ~ARM_SAI_EVENT_FRAME_ERROR;
  }

  if (event & ARM_SAI_EVENT_TX_UNDERFLOW) {
    if ((Audio.Out.Info->Running == 0U)  ||
        (Audio.Out.Info->Paused  == 1U)) {
       Audio.Out.Info->UnderflowPending = 1U;

       // Invalid underflow: Transmitter is running to generate clock for codec
       event &= ~ARM_SAI_EVENT_TX_UNDERFLOW;
     }
  }

  if (event & ARM_SAI_EVENT_SEND_COMPLETE) {
    if (Audio.Out.Info->Paused  == 1U) {
       Audio.Out.Info->SendCompletePending = 1U;

       // Invalid SendComplete: Transmitter is running to generate clock for codec
       event &= ~ARM_SAI_EVENT_SEND_COMPLETE;
     }
  }

  if ((CB_Event != NULL) && (event != 0U)) {
    CB_Event (event);
  }
}

// Local functions
/**
  \fn          int32_t WM8994_RegRead (uint16_t reg, uint16_t * reg_val)
  \brief       Read value from WM8994 register
  \param[in]   reg      WM8994 register address
  \param[Out]  reg_val  Pointer to value read from WM8994 register
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
static int32_t WM8994_RegRead (uint16_t reg, uint16_t *reg_val) {
  uint8_t  buf[2], reg_addr[2];

  reg_addr[0] = (uint8_t)(reg >> 8);
  reg_addr[1] = (uint8_t)reg;

  if (ptrI2C->MasterTransmit (WM8994_I2C_ADDR, reg_addr, 2U, true) != ARM_DRIVER_OK) {
    return -1;
  }
  while (ptrI2C->GetStatus().busy);

  if (ptrI2C->MasterReceive  (WM8994_I2C_ADDR, buf, 2U, false) != ARM_DRIVER_OK) {
    return -1;
  }
  while (ptrI2C->GetStatus().busy);

  *reg_val  = buf[0] << 8;
  *reg_val |= buf[1];

  return 0;
}

/**
  \fn          int32_t WM8994_RegWrite (uint8_16 reg, uint16_t reg_val)
  \brief       Write value to WM8994 register
  \param[in]   reg      WM8994 register address
  \param[in]   reg_val  Value to be written to WM8994 register
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
static int32_t WM8994_RegWrite (uint16_t reg, uint16_t reg_val) {
  uint8_t   buf[4];

  buf[0] = (uint8_t)(reg     >> 8);
  buf[1] = (uint8_t)(reg         );
  buf[2] = (uint8_t)(reg_val >> 8);
  buf[3] = (uint8_t)(reg_val     );

  if (ptrI2C->MasterTransmit (WM8994_I2C_ADDR, buf, 4U, false) != ARM_DRIVER_OK) {
    return -1;
  }
  while (ptrI2C->GetStatus().busy);

  return 0;
}

/**
  \fn          int32_t WM8994_RegModify (uint8_16 reg, uint16_t mask, uint16_t val)
  \brief       Write value to WM8994 register
  \param[in]   reg      WM8994 register address
  \param[in]   mask     Mask bits to be changed
  \param[in]   val      Value of masked bits
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
static int32_t WM8994_RegModify (uint16_t reg, uint16_t mask, uint16_t val) {
  uint8_t   buf[4];
  uint16_t  rd_val;

  WM8994_RegRead (reg, &rd_val);
  val |= (rd_val & ~mask);

  buf[0] = (uint8_t)(reg >> 8);
  buf[1] = (uint8_t)(reg     );
  buf[2] = (uint8_t)(val >> 8);
  buf[3] = (uint8_t)(val     );

  if (ptrI2C->MasterTransmit (WM8994_I2C_ADDR, buf, 4U, false) != ARM_DRIVER_OK) {
    return -1;
  }
  while (ptrI2C->GetStatus().busy);

  return 0;
}

/**
  \fn          int32_t SAI_Configure (STREAM *ptrStream)
  \brief       Configure SAI transmitter
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
static int32_t SAI_Configure (const STREAM  *ptrStream) {
  uint32_t cfg, arg1, arg2;

  if (ptrStream == &Audio.Out) {
    // Master transmitter, User protocol
    cfg =  ARM_SAI_CONFIGURE_TX                                   |
           ARM_SAI_MODE_MASTER                                    |
           ARM_SAI_PROTOCOL_USER                                  |
           ARM_SAI_CLOCK_POLARITY_0                               |
           ARM_SAI_MCLK_PIN_OUTPUT                                |
           ARM_SAI_DATA_SIZE(DataBits);
    arg2 = (SamplingFreq & ARM_SAI_AUDIO_FREQ_Msk) | ARM_SAI_MCLK_PRESCALER(256);
  }

  if (ptrStream == &Audio.In) {
    // Slave receiver, User protocol
    cfg =  ARM_SAI_CONFIGURE_RX                                   |
           ARM_SAI_MODE_SLAVE                                     |
           ARM_SAI_PROTOCOL_USER                                  |
           ARM_SAI_CLOCK_POLARITY_0                               |
           ARM_SAI_SYNCHRONOUS                                    |
           ARM_SAI_DATA_SIZE(DataBits);
  }

  if (ptrStream->Info->Mono == 1U) {
    cfg |= ARM_SAI_MONO_MODE;
  }
  arg1 = ARM_SAI_FRAME_LENGTH(DataBits * 4U)    |
         ARM_SAI_FRAME_SYNC_WIDTH(DataBits *2U) |
         ARM_SAI_FRAME_SYNC_EARLY               |
         ARM_SAI_FRAME_SYNC_POLARITY_LOW        |
         ARM_SAI_SLOT_COUNT(4);
  if (ptrStream->SAI->Control (cfg, arg1, arg2) != ARM_DRIVER_OK) {
    return -1;
  }

  return 0;
}

// Board Audio Interface
/**
  \fn          int32_t Audio_Initialize (Audio_SignalEvent_t cb_event)
  \brief       Initialize Audio Interface
  \param[in]   cb_event  pointer to event notification function
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_Initialize (Audio_SignalEvent_t cb_event) {
  uint16_t reg_val;

  CB_Event = cb_event;

  // Clear Stream info
  Audio.Out.Info->DataSize     = 0U;
  Audio.Out.Info->pData        = NULL;
  Audio.In.Info->DataSize      = 0U;
  Audio.In.Info->pData         = NULL;

  // Set Default values
  DataBits     = 16U;
  SamplingFreq = 16000;

  // SAI Initialization
  Audio.Out.SAI->Initialize   (SAI_callback);
  Audio.In.SAI->Initialize    (SAI_callback);
  Audio.Out.SAI->PowerControl (ARM_POWER_FULL);
  Audio.In.SAI->PowerControl  (ARM_POWER_FULL);

  // Set default TX SAI configuration
  // Enable slot 0 and 2
  if (Audio.Out.SAI->Control (ARM_SAI_MASK_SLOTS_TX, ~(0x05U), 0) != ARM_DRIVER_OK) {
    return -1;
  }
  if (SAI_Configure (&Audio.Out) != 0U) return -1;

  // Set default RX SAI configuration
#if (WM8994_INPUT == WM8994_INPUT_IN1)
  // Enable slot 0 and 2
  if (Audio.In.SAI->Control (ARM_SAI_MASK_SLOTS_RX, ~(0x05U), 0) != ARM_DRIVER_OK) {
    return -1;
  }
#endif
#if (WM8994_INPUT == WM8994_INPUT_DMIC2)
  // Enable slot 1 and 3
  if (Audio.In.SAI->Control (ARM_SAI_MASK_SLOTS_RX, ~(0x0AU), 0) != ARM_DRIVER_OK) {
    return -1;
  }
#endif
  if (SAI_Configure (&Audio.In) != 0U) return -1;

  // Start transmitter to run Codec clock
  Audio.Out.SAI->Control (ARM_SAI_CONTROL_TX, 1U, 0U);

  // I2C Initialization and configuration
  ptrI2C->Initialize   (NULL);
  ptrI2C->PowerControl (ARM_POWER_FULL);
  ptrI2C->Control      (ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
  ptrI2C->Control      (ARM_I2C_BUS_CLEAR, 0U);


  // WM8994 Reset
  WM8994_RegWrite (0x0000, 0x0000);

  // WM8994 ReadID
  WM8994_RegRead (0x0000, &reg_val);
  if (reg_val != 0x8994) { return -1; }

  // WM8994 Codec Setup

  // Errata Work-Arounds
  WM8994_RegWrite(0x0102, 0x0003);
  WM8994_RegWrite(0x0817, 0x0000);
  WM8994_RegWrite(0x0102, 0x0000);

  // VMID Soft fast start, VMID buffer enabled
  WM8994_RegWrite(0x0039, 0x006C);

  // Enable Bias Generator, Enable VMID
  WM8994_RegWrite(0x0001, 0x0003);

  osDelay (50);

#if ((WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1) || \
     (WM8994_OUTPUT & WM8994_INPUT_SPKOUT ))

  // Configure AIF1 (Timeslot 0) to DAC1

  // Enable AIF1DAC1(left and right) input path (Timeslot 0), Enable ADAC1(left and right)
  WM8994_RegWrite(0x0005, 0x0303);

  // Enable AIF1 (Timeslot 0, left) to DAC1L
  WM8994_RegWrite(0x0601, 0x0001);

  // Enable AIF1 (Timeslot 0, Right) to DAC1R
  WM8994_RegWrite(0x0602, 0x0001);
#endif

#if (WM8994_INPUT == WM8994_INPUT_IN1)

  // Enable ADCL and ADCR, Enable AIF1ADC1 (left) and AIF1ADC1 (right) path
  WM8994_RegWrite (0x0004, 0x0303);

  // Enable IN1L and IN1R, Enable Thermal sensor & shout down
  WM8994_RegWrite (0x0002, 0x6350);

  // Enable the ADCL(Left) to AIF1 Timeslot 0 (Left) mixer path
  WM8994_RegWrite (0x0606, 0x0002);

  // Enable the ADCR(Right) to AIF1 Timeslot 0 (Right) mixer path
  WM8994_RegWrite (0x0607, 0x0002);

  // GPIO1 Function = AIF1 DRC1
  WM8994_RegWrite (0x0700, 0x000D);

  // Enable Bias generator and VMID
  WM8994_RegModify(0x0001, 0x0003, 0x0003);

  WM8994_RegWrite (0x0018, 0x000B);
  WM8994_RegWrite (0x001A, 0x000B);

  WM8994_RegWrite (0x0029, 0x0025);
  WM8994_RegWrite (0x002A, 0x0025);

  // IN1L PGA Inverting Input Select, IN1R PGA Inverting Input Select
  WM8994_RegWrite (0x0028, 0x0011);

  // Enable AIF1ADC1 Digital HPF
  WM8994_RegWrite(0x0410, 0x1800);
#endif

#if (WM8994_INPUT == WM8994_INPUT_DMIC2)

  // MIC bias1 enable, VDIM divider enable,  Normal bias current enable
  WM8994_RegModify(0x0001, 0x0013, 0x0013);
  osDelay (50);

  // Configure DMIC2 to AIF1 (Timeslot 1)

  // DMIC2 configuration
  WM8994_RegWrite(0x0700, 0x8101);

  // Enable AIF1ADC2(Left) and AIF1ADC2(Right) output path
  WM8994_RegWrite(0x0004, 0x0C30);

  // Enable DMIC2 (Left) to AIF1 (Timeslot 1, Left) output
  WM8994_RegWrite(0x0608, 0x0002);

  // Enable DMIC2 (Right) to AIF1 (Timeslot 1, Right) output
  WM8994_RegWrite(0x0609, 0x0002);

  // Enable AIF1ADC2 Digital HPF
  WM8994_RegWrite(0x0411, 0x1800);
#endif

#if ((WM8994_INPUT  != WM8994_INPUT_NONE )  && \
     (WM8994_OUTPUT != WM8994_OUTPUT_NONE))

  // default sample rate = 16kHz; AIF1CLK/fs ratio = 256
  WM8994_RegWrite(0x0210, 0x0033);

  // Right ADC data is output on right channel, Digital audio interface = I2S,
  // Word Length = 16bits,
  WM8994_RegWrite(0x0300, 0x4010);

  // Slave mode
  WM8994_RegWrite(0x0302, 0x0000);

  // Enable AIF1 Processing clock and Digital mixing processor clock
  WM8994_RegWrite(0x0208, 0x000A);

  // AIF1CLK Source = MCLK1, Enable AIF1CLK
  WM8994_RegWrite(0x0200, 0x0001);
#endif

#if (WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1)

  // Digital audio source for envelope tracking: AIF1, DAC Timeslot 0
  // Enable Class W
  WM8994_RegWrite(0x0051, 0x0005);

  // Enable HPOUT1L and HPOUT1R input stage
  WM8994_RegModify(0x0001, 0x0300, 0x0300);

  // Enable HPOUT1L and HPOUT1R intermediate stage
  WM8994_RegWrite(0x0060, 0x0022);

  // Enable Charge pump
  WM8994_RegWrite(0x004c, 0x9F25);
  osDelay(15);

  // DAC1L to Left HP Output PGA (HPOUT1LVOL)
  WM8994_RegWrite(0x002D, 0x0001);

  // DAC1R to Right HP Output PGA (HPOUT1RVOL)
  WM8994_RegWrite(0x002E, 0x0001);

  // Enable MIXOUTL and MIXOUTR output mixer
  WM8994_RegModify(0x0003, 0x00F0, 0x00F0);

  // Enable and Start-Up DC Servo for HPOUT1L and HPOUT1R
  WM8994_RegWrite(0x0054, 0x0033);
  osDelay(250);

  // Remove HPOUT1L and HPOUT1R short
  // Enable HPOUT1L and HPOUT1R output stage
  WM8994_RegModify(0x0060, 0x00CC, 0x00CC);
#endif

#if (WM8994_OUTPUT & WM8994_OUTPUT_SPKOUT)
  // Enable SPKMIXL and SPKMIXR output mixer
  WM8994_RegModify(0x0003, 0x0300, 0x0300);

  // Un-mute Left speaker mixer volume control
  WM8994_RegWrite(0x0022, 0x0000);

  // Un-mute Right speaker mixer volume control
  WM8994_RegWrite(0x0023, 0x0000);

  // UN-mute DAC1 to Speaker mixer
  WM8994_RegWrite(0x0036, 0x0003);

  // Enable Mixer, SPKRVOL PGA and SOKOUTR output
  // Enable SPKMIXR Mixer, SPKLVOL PGA  and SPKOUTL output
  WM8994_RegModify(0x0001, 0x3000, 0x3000);

  // Digital audio source for envelope tracking: AIF1, DAC Timeslot 0
  // Enable Class W
  WM8994_RegWrite(0x0051, 0x0005);
  // Un-mute AIF1 Timeslot 0 path
#endif

#if ((WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1) || \
     (WM8994_OUTPUT & WM8994_INPUT_SPKOUT ))

  // Un-mute DAC1L, DAC1L Digital Volume = 0dB
  WM8994_RegWrite(0x0610, 0x00C0);

  // Un-mute DAC1R, DAC1R Digital Volume = 0dB
  WM8994_RegWrite(0x0611, 0x00C0);

  // Un-mute AIF1DAC1 input path (AIF1 Timeslot0)
  WM8994_RegWrite(0x0420, 0x0000);
#endif

  return 0;
}

/**
  \fn          int32_t Audio_Uninitialize (void)
  \brief       De-initialize Audio Interface
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_Uninitialize (void) {

  // Abort SAI Send and Receive
  Audio.In.SAI->Control  (ARM_SAI_ABORT_RECEIVE, 0U, 0U);
  Audio.Out.SAI->Control (ARM_SAI_ABORT_SEND,    0U, 0U);

  // WM8994 Reset
  WM8994_RegWrite (0x0000, 0x0000);

  // Disable SAI transmitter and receiver
  Audio.In.SAI->Control  (ARM_SAI_CONTROL_RX, 0U, 0U);
  Audio.Out.SAI->Control (ARM_SAI_CONTROL_TX, 0U, 0U);

  return 0;
}

/**
  \fn          int32_t Audio_SendData (const void *data, uint32_t num)
  \brief       Start sending data to Audio output stream
  \param[in]   data  pointer to buffer with data to send
  \param[in]   num   number of data items to send
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_SendData (const void *data, uint32_t num) {

  if ((Audio.Out.Info->Running == 0U)  ||
      (Audio.Out.Info->Paused  == 1U)) {
    // Save data info
    Audio.Out.Info->pData    = (void *)data;
    Audio.Out.Info->DataSize = num;
  } else {
    Audio.Out.Info->pData    = (void *)data;
    Audio.Out.Info->DataSize = num;
    if (Audio.Out.SAI->Send (data, num) != ARM_DRIVER_OK) {
      return -1;
    }
  }

  return 0;
}

/**
  \fn          int32_t Audio_ReceiveData (void *data, uint32_t num)
  \brief       Start receiving data from Audio input stream
  \param[out]  data  pointer to buffer for data to receive
  \param[in]   num   number of data items to send
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_ReceiveData (void *data, uint32_t num) {

  if (Audio.In.SAI->Receive (data, num) != ARM_DRIVER_OK) {
    return -1;
  }

  return 0;
}

/**
  \fn          uint32_t Audio_GetDataTxCount (void)
  \brief       Get transmitted data count
  \returns     number of data items transmitted
*/
uint32_t Audio_GetDataTxCount (void) {
  return (Audio.Out.SAI->GetTxCount());
}

/**
  \fn          uint32_t Audio_GetDataRxCount (void)
  \brief       Get received data count
  \returns     number of data items received
*/
uint32_t Audio_GetDataRxCount (void) {
  return (Audio.Out.SAI->GetRxCount());
}

/**
  \fn          int32_t Audio_Start (uint8_t stream)
  \brief       Start Audio stream
  \param[in]   stream  stream identifier
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_Start (uint8_t stream) {
  void * data;
  uint32_t num, evt = 0U;

  switch (stream) {
    case AUDIO_STREAM_OUT:
      // Set Running flag
      Audio.Out.Info->Running = 1U;
      if (Audio.Out.Info->Paused == 0U) {

        // Check for valid data to be send
        if (Audio.Out.Info->DataSize != 0U) {
          // Clear pending event flags
          Audio.Out.Info->UnderflowPending    = 0U;
          Audio.Out.Info->SendCompletePending = 0U;

          // Save data information, before clearing it
          num  = Audio.Out.Info->DataSize;
          data = Audio.Out.Info->pData;
          Audio.Out.Info->DataSize = 0U;
          Audio.Out.Info->pData    = NULL;

          // Send data
          if (Audio.Out.SAI->Send (data, num) != ARM_DRIVER_OK) {
            return -1;
          }
        } else {
          // Check for pending events
          if (Audio.Out.Info->UnderflowPending    == 1U) { evt |= AUDIO_EVENT_TX_UNDERFLOW;  }
          if (Audio.Out.Info->SendCompletePending == 1U) { evt |= AUDIO_EVENT_SEND_COMPLETE; }

          // Clear pending event flags
          Audio.Out.Info->UnderflowPending    = 0U;
          Audio.Out.Info->SendCompletePending = 0U;

          if ((CB_Event != NULL) && (evt != 0U)) {
            CB_Event (evt);
          }
        }
      }
      break;
    case AUDIO_STREAM_IN:
      Audio.In.SAI->Control (ARM_SAI_CONTROL_RX, 1U, 0U);
      break;
    default: return -1;
  }

  return 0;
}

/**
  \fn          int32_t Audio_Stop (uint8_t stream)
  \brief       Stop Audio stream
  \param[in]   stream  stream identifier
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_Stop (uint8_t stream) {

  switch (stream) {
    case AUDIO_STREAM_OUT:
      // Clear running flag
      Audio.Out.Info->Running = 0U;
      // Abort transfer
      Audio.Out.SAI->Control (ARM_SAI_ABORT_SEND, 1U, 0U);
      break;
    case AUDIO_STREAM_IN:
      // Clear running flag
      Audio.In.Info->Running = 0U;
      Audio.In.SAI->Control (ARM_SAI_CONTROL_RX, 0U, 0U);
      // Abort transfer
      Audio.In.SAI->Control (ARM_SAI_ABORT_RECEIVE, 1U, 0U);
      break;
    default: return (-1);
  }

  return 0;
}

/**
  \fn          int32_t Audio_Pause (uint8_t stream)
  \brief       Pause Audio stream
  \param[in]   stream  stream identifier
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_Pause (uint8_t stream) {

  switch (stream) {
    case AUDIO_STREAM_OUT:
      if (Audio.Out.Info->Running == 0U) { return 0U; }

      // Clear pending event flags
      Audio.Out.Info->UnderflowPending    = 0U;
      Audio.Out.Info->SendCompletePending = 0U;

      // Set paused flag
      Audio.Out.Info->Paused = 1U;
      break;
    case AUDIO_STREAM_IN:
      if (Audio.In.Info->Running == 0U) { return 0U; }

      // Disable SAI receiver
      Audio.In.SAI->Control (ARM_SAI_CONTROL_RX, 0U, 0U);
      break;
    default: return (-1);
  }
  return 0;
}

/**
  \fn          int32_t Audio_Resume (uint8_t stream)
  \brief       Resume Audio stream
  \param[in]   stream  stream identifier
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_Resume (uint8_t stream) {
  void * data;
  uint32_t num, evt = 0U;

  switch (stream) {
    case AUDIO_STREAM_OUT:
      if ((Audio.Out.Info->Paused  == 1U) &&
          (Audio.Out.Info->Running == 1U)) {

        // Clear Paused flag
        Audio.Out.Info->Paused = 0U;

        // Check for valid data to be send
        if (Audio.Out.Info->DataSize != 0U) {
          // Clear pending event flags
          Audio.Out.Info->UnderflowPending    = 0U;
          Audio.Out.Info->SendCompletePending = 0U;

          // Save data information, before clearing it
          num  = Audio.Out.Info->DataSize;
          data = Audio.Out.Info->pData;
          Audio.Out.Info->DataSize = 0U;
          Audio.Out.Info->pData    = NULL;

          // Send data
          if (Audio.Out.SAI->Send (data, num) != ARM_DRIVER_OK) {
            return -1;
          }
        } else {
          // Check for pending events
          if (Audio.Out.Info->UnderflowPending    == 1U) { evt |= AUDIO_EVENT_TX_UNDERFLOW;  }
          if (Audio.Out.Info->SendCompletePending == 1U) { evt |= AUDIO_EVENT_SEND_COMPLETE; }

          // Clear pending event flags
          Audio.Out.Info->UnderflowPending    = 0U;
          Audio.Out.Info->SendCompletePending = 0U;

          if ((CB_Event != NULL) && (evt != 0U)) {
            CB_Event (evt);
          }
        }
      }
      break;
    case AUDIO_STREAM_IN:
      if (Audio.In.Info->Paused == 1U) {
        // Clear paused flag
        Audio.In.Info->Paused = 0U;

        // Enable Receiver
        Audio.In.SAI->Control (ARM_SAI_CONTROL_RX, 1U, 0U);
      }
      break;
    default: return -1;
  }

  return 0;
}

/**
  \fn          int32_t Audio_SetVolume (uint8_t stream, uint8_t channel, uint8_t volume)
  \brief       Set volume level for Audio stream
  \param[in]   stream   stream identifier
  \param[in]   channel  channel identifier
  \param[in]   volume   volume level (0..100)
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_SetVolume (uint8_t stream, uint8_t channel, uint8_t volume) {

  if (volume > 100) { return -1; }

  switch (stream) {
    case AUDIO_STREAM_OUT:
      // Scale volume
      volume = (volume * 63) / 100;
      switch (channel) {
        case AUDIO_CHANNEL_MASTER:
#if (WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1)
          // Update Left and Right channel volume
          WM8994_RegWrite(0x001C, 0x0040 | volume);
          WM8994_RegWrite(0x001D, 0x0140 | volume);
#endif
#if (WM8994_OUTPUT & WM8994_OUTPUT_SPKOUT)
          // Update Left and Right channel volume
          WM8994_RegWrite(0x0026, 0x0040 | volume);
          WM8994_RegWrite(0x0027, 0x0140 | volume);
#endif
          break;

        case AUDIO_CHANNEL_LEFT:
#if (WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1)
          // Update Left channel volume
          WM8994_RegWrite(0x001C, 0x0140 | volume);
#endif
#if (WM8994_OUTPUT & WM8994_OUTPUT_SPKOUT)
          // Update Left channel volume
          WM8994_RegWrite(0x0026, 0x0140 | volume);
#endif
          break;
        case AUDIO_CHANNEL_RIGHT:
#if (WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1)
          // Update Right channel volume
          WM8994_RegWrite(0x001D, 0x0140 | volume);
#endif
#if (WM8994_OUTPUT & WM8994_OUTPUT_SPKOUT)
          // Update Right channel volume
          WM8994_RegWrite(0x0027, 0x0140 | volume);
#endif
          break;
      }
    break;
    case AUDIO_STREAM_IN:
      // Scale Volume
      volume = (volume * 239) / 100;
      switch (channel) {
        case AUDIO_CHANNEL_MASTER:
          // Save Audio IN volume settings
          Audio.In.Info->Volume_L = volume;
          Audio.In.Info->Volume_R = volume;

          // Update Left and Right channel volume
#if (WM8994_INPUT == WM8994_INPUT_DMIC2)
          WM8994_RegWrite(0x0404, volume);
          WM8994_RegWrite(0x0405, volume);
#endif
#if (WM8994_INPUT == WM8994_INPUT_IN1)
          WM8994_RegWrite(0x0400, volume);
          WM8994_RegWrite(0x0401, volume);
#endif
          break;
        case AUDIO_CHANNEL_LEFT:
          // Save Audio IN volume settings
          Audio.In.Info->Volume_L = volume;

          // Update Left channel volume
#if (WM8994_INPUT == WM8994_INPUT_DMIC2)
          WM8994_RegWrite(0x0404, volume);
#endif
#if (WM8994_INPUT == WM8994_INPUT_IN1)
          WM8994_RegWrite(0x0400, volume);
#endif
          break;
        case AUDIO_CHANNEL_RIGHT:
          // Save Audio IN volume settings
          Audio.In.Info->Volume_R = volume;

          // Update Right channel volume
#if (WM8994_INPUT == WM8994_INPUT_DMIC2)
          WM8994_RegWrite(0x0405, volume);
#endif
#if (WM8994_INPUT == WM8994_INPUT_IN1)
          WM8994_RegWrite(0x0401, volume);
#endif
          break;
      }

    break;

    default: return -1;
  }
  return 0;
}

/**
  \fn          int32_t Audio_SetMute (uint8_t stream, uint8_t channel, bool mute)
  \brief       Set mute state for Audio stream
  \param[in]   stream   stream identifier
  \param[in]   channel  channel identifier
  \param[in]   mute     mute state
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_SetMute (uint8_t stream, uint8_t channel, bool mute) {
#if (WM8994_OUTPUT != WM8994_OUTPUT_NONE)
  uint16_t unmute;
#endif
#if (WM8994_INPUT != WM8994_INPUT_NONE)
  uint16_t vol_l, vol_r;
#endif

  switch (stream) {
#if (WM8994_OUTPUT != WM8994_OUTPUT_NONE)
    case AUDIO_STREAM_OUT:
      if (mute == false) { unmute = 0x0100 | (1U << 6); }
      else               { unmute = 0x0100;             }

      switch (channel) {
        case AUDIO_CHANNEL_MASTER:
#if (WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1)
          WM8994_RegModify(0x001C, 0x0040,  unmute);
          WM8994_RegModify(0x001D, 0x00140, unmute);
#endif
#if (WM8994_OUTPUT & WM8994_OUTPUT_SPKOUT)
          WM8994_RegModify(0x0026, 0x0040,  unmute);
          WM8994_RegModify(0x0027, 0x00140, unmute);
#endif
          break;

        case AUDIO_CHANNEL_LEFT:
#if (WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1)
          WM8994_RegModify(0x001C, 0x00140, unmute);
#endif
#if (WM8994_OUTPUT & WM8994_OUTPUT_SPKOUT)
          WM8994_RegModify(0x0026, 0x00140, unmute);
#endif
          break;
        case AUDIO_CHANNEL_RIGHT:
#if (WM8994_OUTPUT & WM8994_OUTPUT_HPOUT1)
          WM8994_RegModify(0x001D, 0x0140, unmute);
#endif
#if (WM8994_OUTPUT & WM8994_OUTPUT_SPKOUT)
          WM8994_RegModify(0x0027, 0x0140, unmute);
#endif
          break;
      }
    break;
#endif

#if (WM8994_INPUT != WM8994_INPUT_NONE)
    case AUDIO_STREAM_IN:
      if (mute == true) {
        vol_l = vol_r = 0U;
      } else {
        vol_l = Audio.In.Info->Volume_L;
        vol_r = Audio.In.Info->Volume_R;
      }
      switch (channel) {
        case AUDIO_CHANNEL_MASTER:
          // Update Left and Right channel volume
#if (WM8994_INPUT == WM8994_INPUT_DMIC2)
          WM8994_RegWrite(0x0404, vol_l);
          WM8994_RegWrite(0x0405, vol_r);
#endif
#if (WM8994_INPUT == WM8994_INPUT_IN1)
          WM8994_RegWrite(0x0400, vol_l);
          WM8994_RegWrite(0x0401, vol_r);
#endif
          break;

        case AUDIO_CHANNEL_LEFT:
          // Update Left channel volume
#if (WM8994_INPUT == WM8994_INPUT_DMIC2)
          WM8994_RegWrite(0x0404, vol_l);
#endif
#if (WM8994_INPUT == WM8994_INPUT_IN1)
          WM8994_RegWrite(0x0400, vol_l);
#endif
          break;
        case AUDIO_CHANNEL_RIGHT:
          // Update Right channel volume
#if (WM8994_INPUT == WM8994_INPUT_DMIC2)
          WM8994_RegWrite(0x0405, vol_r);
#endif
#if (WM8994_INPUT == WM8994_INPUT_IN1)
          WM8994_RegWrite(0x0401, vol_r);
#endif
          break;
      }
    break;
#endif

    default: return -1;
  }
  return 0;
}

/**
  \fn          int32_t Audio_SetDataFormat (uint8_t stream, uint8_t format)
  \brief       Set Audio data format
  \param[in]   stream  stream identifier
  \param[in]   format  data format
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_SetDataFormat (uint8_t stream, uint8_t format) {
  const STREAM *ptrStream;

  ptrStream = STREAM_PTR(stream);

  switch (format) {
    case AUDIO_DATA_8_MONO:
      // Not Supported
      return -1;
    case AUDIO_DATA_16_MONO:
      DataBits              = 16U;
      ptrStream->Info->Mono = 1U;
      break;
    case AUDIO_DATA_32_MONO:
      DataBits              = 32U;
      ptrStream->Info->Mono = 1U;
      break;
    case AUDIO_DATA_8_STEREO:
      // Not supported
      return -1;
    case AUDIO_DATA_16_STEREO:
      DataBits             = 16U;
      ptrStream->Info->Mono = 0U;
      break;
    case AUDIO_DATA_32_STEREO:
      DataBits              = 32U;
      ptrStream->Info->Mono = 0U;
      break;
    default: return -1;
  }

  // Update SAI Data format configuration
  if (SAI_Configure (ptrStream) == -1) { return -1; }

  if (DataBits == 16U) {
    // 16-bit data
    WM8994_RegModify (0x0300, 0x0060, 0x0000);
  }
  if (DataBits == 32U) {
    // 32-bit data
    WM8994_RegModify (0x0300, 0x0060, 0x0060);
  }

  return 0;
}

/**
  \fn          int32_t Audio_SetFrequency (uint8_t stream, uint32_t frequency)
  \brief       Set Audio stream frequency
  \param[in]   stream     stream identifier
  \param[in]   frequency  Audio frequency in Hz
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Audio_SetFrequency (uint8_t stream, uint32_t frequency) {
  uint16_t val;

  /*  Clock Configurations */
  switch (frequency){
    case  8000:
      // AIF1 Sample Rate = 8 kHz
      val = 0x0003;
      break;

    case  16000:
      // AIF1 Sample Rate = 16 kHz
      val = 0x0033;
      break;

    case  32000:
      // AIF1 Sample Rate = 32 kHz
      val = 0x0063;
      break;

    case  48000:
      // AIF1 Sample Rate = 48 kHz
      val = 0x0083;
      break;

    case  96000:
      // AIF1 Sample Rate = 96 kHz
      val = 0x00A3;
      break;

    case  11000:
      // AIF1 Sample Rate = 11 kHz
      val = 0x0013;
      break;

    case  22000:
      // AIF1 Sample Rate = 22 kHz
      val = 0x0043;
      break;

    case  44000:
      // AIF1 Sample Rate = 44 kHz
      val = 0x0073;
      break; 
    
    default: return -1;
  }

  // Save Audio frequency value
  SamplingFreq = frequency;

  // Update SAI Audio frequency configuration
  SAI_Configure (&Audio.Out);

  // Update WM8994 codec Audio frequency
  WM8994_RegWrite (0x0210, val);

  return 0;
}
