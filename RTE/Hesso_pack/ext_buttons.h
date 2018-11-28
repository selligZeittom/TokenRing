/************************************************************************//**
 * \file		ext_buttons.h
 * \brief		Function to use the extension uart
 * \author	pascal (dot) sartoretti (at) hevs (dot) ch
 ***************************************************************************/
#ifndef __EXT_BUTTONS_H
#define __EXT_BUTTONS_H

#include <stdint.h>

/************************************************************************//**
 * \brief Inits the external buttons usage.
 * \return Always #0
 ***************************************************************************/
extern int32_t  Ext_Buttons_Init(void);

/************************************************************************//**
 * \brief Reads the buttons status
 * \return The binary state of the buttons (example 4 for button 2 pressed).
 ***************************************************************************/
extern uint32_t Ext_Buttons_GetState(void);

#endif /* __BOARD_BUTTONS_H */
