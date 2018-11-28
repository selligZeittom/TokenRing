
#include "stm32f7xx_hal.h"
#include "ext_led.h"
#include <stdint.h>


//------------------------------------------------------------------------------
static const uint8_t cie1931[101] =
{
	  0,   0,   1,   1,  1,    1,   2,   2,   2,   3,
	  3,   3,   4,   4,  4,    5,   5,   6,   6,   7,
	  8,   8,   9,  10,  10,  11,  12,  13,  14,  15,
	 16,  17,  18,  19,  20,  22,  23,  24,  26,  27,
	 29,  30,  32,  34,  35,  37,  39,  41,  43,  45,
	 47,  49,  51,  54,  56,  58,  61,  64,  66,  69,
	 72,  75,  78,  81,  84,  87,  90,  93,  97, 100,
	104, 108, 111, 115, 119, 123, 127, 131, 136, 140,
	145, 149, 154, 159, 163, 168, 173, 179, 184, 189,
	195, 200, 206, 212, 217, 223, 230, 236, 242, 248,
	255,
};

#if LIGHTNESS_PWM_STEP != 100
#error this cie1931 array supports only 100 steps, feel free to implement your own
#endif

//------------------------------------------------------------------------------
uint8_t lightness_to_pwm(uint8_t percentage)
{
	if(percentage > (LIGHTNESS_PWM_STEP-1))
		percentage = (LIGHTNESS_PWM_STEP-1);

	return cie1931[percentage];
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32_t Ext_LED_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct;
	//----------------------------------------------------------------------------
  // Configure GPIO pin: PA15 (LED0) 
  __HAL_RCC_GPIOA_CLK_ENABLE();							// enable GPIO timer
	__HAL_RCC_TIM2_CLK_ENABLE();							// enable timer 2 clock
  GPIO_InitStruct.Pin   = GPIO_PIN_15;			// used pin is PA15
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;	// alternate function use
  GPIO_InitStruct.Pull  = GPIO_NOPULL;			// no pullup
	GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;// timer 2 is used
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;	// speed is fast
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	TIM2->CCER = TIM_CCER_CC1E;								// compare for PWM usage
  TIM2->PSC = 16;														// timer prescaler 
	TIM2->ARR = 255;												  // max count value
  TIM2->CCR1 = lightness_to_pwm(0);				// duty cycle
	TIM2->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
	TIM2->EGR |= TIM_EGR_UG;									// update register now
	TIM2->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;		// start the timer
	//----------------------------------------------------------------------------
  // Configure GPIO pin: PH6 (LED1) 
  __HAL_RCC_GPIOH_CLK_ENABLE();							// enable GPIO timer
	__HAL_RCC_TIM12_CLK_ENABLE();							// enable timer 12 clock
  GPIO_InitStruct.Pin   = GPIO_PIN_6;				// used pin is PH6
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;	// alternate function use
  GPIO_InitStruct.Pull  = GPIO_NOPULL;			// no pullup
	GPIO_InitStruct.Alternate = GPIO_AF9_TIM12;// timer 12 is used
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;	// speed is fast
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

	TIM12->CCER = TIM_CCER_CC1E;							// compare for PWM usage
  TIM12->PSC = 16;													// timer prescaler 
	TIM12->ARR = 255;												  // max count value
  TIM12->CCR1 = lightness_to_pwm(0);				// duty cycle
	TIM12->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
	TIM12->EGR |= TIM_EGR_UG;									// update register now
	TIM12->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;	// start the timer
	//----------------------------------------------------------------------------
  // Configure GPIO pin: PA8 (LED2) 
  __HAL_RCC_GPIOA_CLK_ENABLE();							// enable GPIO timer
	__HAL_RCC_TIM1_CLK_ENABLE();							// enable timer 1 clock
		
  GPIO_InitStruct.Pin   = GPIO_PIN_8;				// used pin is PA8
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;	// alternate function use
  GPIO_InitStruct.Pull  = GPIO_NOPULL;			// no pullup
	GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;// timer 5 is used
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;	// speed is fast
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  TIM1->CCER = TIM_CCER_CC1E;								// compare for PWM usage
  TIM1->PSC = 16;														// timer prescaler 
	TIM1->ARR = 255;												  // max count value
  TIM1->CCR1 = lightness_to_pwm(0);				// duty cycle
	TIM1->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
	TIM1->EGR |= TIM_EGR_UG;									// update register now
	TIM1->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;		// start the timer	
	TIM1->BDTR = TIM_BDTR_MOE;								// master output enable
	//----------------------------------------------------------------------------
  // Configure GPIO pin: PB4 (LED3) 
  __HAL_RCC_GPIOB_CLK_ENABLE();							// enable GPIO timer
	__HAL_RCC_TIM3_CLK_ENABLE();							// enable timer 3 clock
  GPIO_InitStruct.Pin   = GPIO_PIN_4;				// used pin is PB4
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;	// alternate function use
  GPIO_InitStruct.Pull  = GPIO_NOPULL;			// no pullup
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;// timer 3 is used
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;	// speed is fast
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	TIM3->CCER = TIM_CCER_CC1E;								// compare for PWM usage
  TIM3->PSC = 16;														// timer prescaler 
	TIM3->ARR = 255;												  // max count value
  TIM3->CCR1 = lightness_to_pwm(0);				// duty cycle
	TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
	TIM3->EGR |= TIM_EGR_UG;									// update register now
	TIM3->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;		// start the timer	
  return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32_t Ext_LED_On (uint32_t num) {

	if((num & 1) != 0)
	{
      TIM2->CCR1 = lightness_to_pwm(LIGHTNESS_PWM_STEP);		
	}
	if((num & 2) != 0)
	{
     TIM12->CCR1 = lightness_to_pwm(LIGHTNESS_PWM_STEP);		
	}
	if((num & 4) != 0)
	{
     TIM1->CCR1 = lightness_to_pwm(LIGHTNESS_PWM_STEP);		
	}
	if((num & 8) != 0)
	{
     TIM3->CCR1 = lightness_to_pwm(LIGHTNESS_PWM_STEP);		
	}
  return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32_t Ext_LED_Off (uint32_t num) {

 	if((num & 1) != 0)
	{
     TIM2->CCR1 = lightness_to_pwm(0);			
	}
	if((num & 2) != 0)
	{
     TIM12->CCR1 = lightness_to_pwm(0);		
	}
	if((num & 4) != 0)
	{
     TIM1->CCR1 = lightness_to_pwm(0);		
	}
	if((num & 8) != 0)
	{
     TIM3->CCR1 = lightness_to_pwm(0);		
	}
  return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32_t Ext_LED_PWM (uint32_t num, uint32_t duty) {

	if((num & 1) != 0)
	{
      TIM2->CCR1 = lightness_to_pwm(duty);		
	}
	if((num & 2) != 0)
	{
     TIM12->CCR1 = lightness_to_pwm(duty);		
	}
	if((num & 4) != 0)
	{
     TIM1->CCR1 = lightness_to_pwm(duty);		
	}
	if((num & 8) != 0)
	{
     TIM3->CCR1 = lightness_to_pwm(duty);		
	}
  return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32_t Ext_LEDs(uint32_t num) {

  if((num & 1) != 0)
	{
      TIM2->CCR1 = lightness_to_pwm(LIGHTNESS_PWM_STEP);		
	}
	else
	{
      TIM2->CCR1 = lightness_to_pwm(0);		
	}
	if((num & 2) != 0)
	{
     TIM12->CCR1 = lightness_to_pwm(LIGHTNESS_PWM_STEP);		
	}
	else
	{
    TIM12->CCR1 = lightness_to_pwm(0);		
	}
	if((num & 4) != 0)
	{
     TIM1->CCR1 = lightness_to_pwm(LIGHTNESS_PWM_STEP);	
	}
	else
	{
     TIM1->CCR1 = lightness_to_pwm(0);			
	}
	if((num & 8) != 0)
	{
     TIM3->CCR1 = lightness_to_pwm(LIGHTNESS_PWM_STEP);
	}
	else
	{
     TIM3->CCR1 = lightness_to_pwm(0);			
	}
  return 0;
}
