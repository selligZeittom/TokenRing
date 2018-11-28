/************************************************************************//**
 * \file		ext_led.h
 * \brief		Function to use the extension LEDs
 * \author	pascal (dot) sartoretti (at) hevs (dot) ch
 ***************************************************************************/


#ifndef __EXT_LED_H
#define __EXT_LED_H

#include <stdint.h>

#define LIGHTNESS_PWM_STEP 100


/************************************************************************//**
 * \brief Inits the external Leds usage.
 * \return Always #0
 ***************************************************************************/
extern int32_t  Ext_LED_Init   (void);

/************************************************************************//**
 * \brief Turn on one led.
 * \param num The led to turn on (1,2,4,8)
 * \return Always 0
 ***************************************************************************/
extern int32_t  Ext_LED_On           (uint32_t num);

/************************************************************************//**
 * \brief Turn off one led.
 * \param num The led to turn off (1,2,4,8)
 * \return Always 0
 ***************************************************************************/
extern int32_t  Ext_LED_Off          (uint32_t num);

/************************************************************************//**
 * \brief Set a power on a led.
 * \param num The led to turn set the power (1,2,4,8)
 * \param duty The power of the led (0 to 255)
 * \return Always 0
 ***************************************************************************/
extern int32_t  Ext_LED_PWM (uint32_t num, uint32_t duty);

/************************************************************************//**
 * \brief Set the state on all leds.
 * \param val The binary state of the four leds (example 0b1101).
 * \return Always 0
 ***************************************************************************/
extern int32_t  Ext_LEDs(uint32_t val);

#endif /* __BOARD_LED_H */
