/************************************************************************//**
 * \file		ext_uart.h
 * \brief		Function to use the extension uart
 * \author	pascal (dot) sartoretti (at) hevs (dot) ch
 ***************************************************************************/


#ifndef __EXT_UART_H
#define __EXT_UART_H

#include <stdint.h>
#include "stm32f7xx_hal.h"

extern UART_HandleTypeDef ext_uart;		// extension uart handle

/************************************************************************//**
 * \brief 		Inits the extension uart
 * \param speed This si the uart speed selected for example 115200.
 * The extension uart could be use with or without interrupts.
 *
 * Without interrupts:
 * -------------------
 * To send something on the uart, you have to use HAL_UART_Transmit function
 * as the example below.
 * error = HAL_UART_Transmit(&ext_uart, msg, sizeof(msg),50);
 * To receive you have to use HAL_UART_Receive as example below.
 * error = HAL_UART_Receive(&ext_uart, msg, sizeof(msg),HAL_MAX_DELAY);
 * The HAL_MAX_DELAY waits until receive is finished.
 *
 * With interrupts:
 * ----------------
 * The functions below have to be used:
 * HAL_UART_Transmit_IT(&ext_uart," Welcome\n\r", 10);
 * HAL_UART_Receive_IT(&ext_uart,data,8);
 * 
 * The callback functions above could be implemented for usage on interrupt
 * mode when the full size is transmitted (or received).
 * void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 * void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
 *
 ***************************************************************************/
extern void Ext_UART_Init(uint32_t speed);

#endif /* __BOARD_LED_H */
