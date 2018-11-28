
#include "stm32f7xx_hal.h"
#include "ext_buttons.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32_t Ext_Buttons_Init (void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  /* Configure GPIO pin: PI2 (BTN_0) */
  GPIO_InitStruct.Pin  = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
  /* Configure GPIO pin: PI3 (BTN_1) */
  GPIO_InitStruct.Pin  = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
  /* Configure GPIO pin: PG7 (BTN_2) */
  GPIO_InitStruct.Pin  = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
  /* Configure GPIO pin: PG6 (BTN_3) */
  GPIO_InitStruct.Pin  = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
uint32_t Ext_Buttons_GetState (void) {
  uint32_t val = 0;

  if (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_2) == GPIO_PIN_RESET) {
    val |= 1;
  }
  if (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_3) == GPIO_PIN_RESET) {
    val |= 2;
  }
  if (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_7) == GPIO_PIN_RESET) {
    val |= 4;
  }
  if (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_6) == GPIO_PIN_RESET) {
    val |= 8;
  }

  return val;
}

