
#include "stm32f7xx_hal.h"
#include "ext_keyboard.h"

static SPI_HandleTypeDef keyboard;								// SPI keyboard handle
uint8_t ext_kbChar;																// exported to global use
static uint32_t newChar=0;												// internal flag
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void EXTI9_5_IRQHandler(void)
{
	// read keyboard data and store it on global variable
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_RESET);	// SPI CS activate
	HAL_SPI_Receive(&keyboard,&ext_kbChar,1,100);					// SPI read
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_SET);		// SPI CS desactivate
	// process irq
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);									// clear irq and callback
	newChar = 1;																					// set flag
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Ext_Keyboard_Init(void)
{
	volatile uint8_t dummy;
  GPIO_InitTypeDef GPIO_InitStruct;

	/* Peripheral clock enable */
	__HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_SPI2_CLK_ENABLE();
  
  /**SPI2 GPIO Configuration    
  PI1      ------>  SPI2_SCK
  PB14     ------>  SPI2_MISO
  PB15     ------>  SPI2_MOSI 
	PI0      ------>  /SPI_CB_SS
	PF8      ------>  /SPI_INT
  */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	GPIO_InitStruct.Pin   = GPIO_PIN_0;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);	
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  keyboard.Instance = SPI2;
  keyboard.Init.Mode = SPI_MODE_MASTER;
  keyboard.Init.Direction = SPI_DIRECTION_2LINES;
  keyboard.Init.DataSize = SPI_DATASIZE_8BIT;
  keyboard.Init.CLKPolarity = SPI_POLARITY_HIGH;
  keyboard.Init.CLKPhase = SPI_PHASE_1EDGE;
  keyboard.Init.NSS = SPI_NSS_SOFT;
  keyboard.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;	//3.333MHz -> (Fosc/4) / 16
  keyboard.Init.FirstBit = SPI_FIRSTBIT_MSB;
  keyboard.Init.TIMode = SPI_TIMODE_DISABLE;
  keyboard.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  keyboard.Init.CRCPolynomial = 7;
  keyboard.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  keyboard.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	HAL_SPI_Init(&keyboard);
	// dummy read to clear any glitch
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_SPI_Receive(&keyboard,(uint8_t *)&dummy,1,10);
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_SET);
	// enable interrupts on keyboard ISR
  HAL_NVIC_SetPriority(EXTI9_5_IRQn,5,5);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
uint8_t Ext_Keyboard_Read(void)
{
	while(newChar == 0)										// wait new char coming
	{}
	newChar = 0;													// clear it
	return ext_kbChar;										// return read char
}



