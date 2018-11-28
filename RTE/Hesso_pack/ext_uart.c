
#include "stm32f7xx_hal.h"
#include "ext_uart.h"

UART_HandleTypeDef ext_uart;			// extension uart handler
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(huart->Instance==USART6)
  {
		__HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();
  
    /**USART6 GPIO Configuration    
    PC7     ------> USART6_RX
    PC6     ------> USART6_TX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Ext_UART_Init(uint32_t speed)
{
  ext_uart.Instance = USART6;
  ext_uart.Init.BaudRate = speed;
  ext_uart.Init.WordLength = UART_WORDLENGTH_8B;
  ext_uart.Init.StopBits = UART_STOPBITS_1;
  ext_uart.Init.Parity = UART_PARITY_NONE;
  ext_uart.Init.Mode = UART_MODE_TX_RX;
  ext_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  ext_uart.Init.OverSampling = UART_OVERSAMPLING_16;
  ext_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  ext_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  HAL_UART_Init(&ext_uart);
	   /* USART6 interrupt Init */
  HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART6_IRQn);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void USART6_IRQHandler(void)
{
  HAL_UART_IRQHandler(&ext_uart);
}


