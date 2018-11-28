/************************************************************************//**
 * \file		ext_keyboard.h
 * \brief		Function to use the extension keyboard
 * \author	pascal (dot) sartoretti (at) hevs (dot) ch
 ***************************************************************************/


#ifndef __EXT_KEYBOARD_H
#define __EXT_KEYBOARD_H

#include <stdint.h>
#include "stm32f7xx_hal.h"

extern uint8_t ext_kbChar;

/************************************************************************//**
 * \brief 		Inits the extension keyboard
 * The extension keyboard use interrupt from keyboard (PF8)
 *
 * Read keyboard non blocking (interrupt):
 * ---------------------------------------
 * To read the keyboard, the callback function HAL_GPIO_EXTI_Callback has
 * to be implemented.
 * the example below send the keyboard char to the serial port
 * void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
 * {
 * 	 if(GPIO_Pin == GPIO_PIN_8)
 * 	 {
 * 		 HAL_UART_Transmit(&ext_uart, &ext_kbChar, 1,100);;	
 *   }
 * }
 * Read keyboard blocking (pooling until key pressed):
 * ---------------------------------------------------
 * The functions below have to be used:
 * pressed = Ext_Keyboard_Read();
 *
 * \warning The external interrupts (5,6,7,9) have to be implemented
 * in the ext_keyboard.c file because they share the same processor irq 
 * EXTI9_5_IRQHandler.
 ***************************************************************************/
 void Ext_Keyboard_Init(void);

/************************************************************************//**
 * \brief Read the pressed key on extension keyboard
 * \return The ASCII code of key pressed.
 *
 * \warning   This function is blocking until a char is received
 ***************************************************************************/
 uint8_t Ext_Keyboard_Read(void);


#endif /* __BOARD_LED_H */

