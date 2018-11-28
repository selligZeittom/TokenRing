#include "gfx.h"

#include "stm32f7_i2c.h"
#include "Driver_I2C.h"

/* I2C Driver */
extern ARM_DRIVER_I2C Driver_I2C3;
static ARM_DRIVER_I2C * I2Cdrv = &Driver_I2C3;
 
 
#ifndef EEPROM_I2C_PORT
#define EEPROM_I2C_PORT       3         /* I2C Port number                    */
#endif
 
 
#define A_WR                  0         /* Master will write to the I2C       */
#define A_RD                  1         /* Master will read from the I2C      */

static uint8_t wr_buf[256];
uint8_t  gI2CAccess = 0;
bool_t i2cInit(I2C_TypeDef* i2c)
{
	gI2CAccess = 1;
  I2Cdrv->Initialize   (NULL);
  I2Cdrv->PowerControl (ARM_POWER_FULL);
  I2Cdrv->Control      (ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
  I2Cdrv->Control      (ARM_I2C_BUS_CLEAR, 0);
	gI2CAccess = 0;
 
 
  return 1;			// just says no error
}


void i2cWriteReg(I2C_TypeDef* i2c, uint8_t slaveAddr, uint8_t regAddr, uint8_t value)
{
	wr_buf[0] = regAddr;
	wr_buf[1] = value;
	gI2CAccess = 1;

  I2Cdrv->MasterTransmit (slaveAddr/2, wr_buf, 2, false);
  while (I2Cdrv->GetStatus().busy);
  if (I2Cdrv->GetDataCount () != 2) return;
  /* Acknowledge polling */	
		gI2CAccess = 0;

//  do {
//    I2Cdrv->MasterReceive (DeviceAddr, &wr_buf[0], 1, false);
//    while (I2Cdrv->GetStatus().busy);
//  } while (I2Cdrv->GetDataCount () < 0);
 
}

uint8_t i2cReadByte(I2C_TypeDef* i2c, uint8_t slaveAddr, uint8_t regAddr)
{
	uint8_t ret = 0;
	gI2CAccess = 1;

  I2Cdrv->MasterTransmit (slaveAddr/2, &regAddr, 1, true);
  while (I2Cdrv->GetStatus().busy);
  I2Cdrv->MasterReceive (slaveAddr/2, &ret, 1, false);
  while (I2Cdrv->GetStatus().busy);
  if (I2Cdrv->GetDataCount () != 1) return 0xAA;
 	gI2CAccess = 0;

	return ret;
}

uint16_t i2cReadWord(I2C_TypeDef* i2c, uint8_t slaveAddr, uint8_t regAddr)
{
	uint8_t ret[2] = { 0, 0 };
	gI2CAccess = 1;

  I2Cdrv->MasterTransmit (slaveAddr/2, &regAddr, 1, true);
  while (I2Cdrv->GetStatus().busy);
  I2Cdrv->MasterReceive (slaveAddr/2, ret, 2, false);
  while (I2Cdrv->GetStatus().busy);
  if (I2Cdrv->GetDataCount () != 2) return 0xAAAA;
 	gI2CAccess = 0;

	return (uint16_t)((ret[0] << 8) | (ret[1] & 0x00FF));
}
