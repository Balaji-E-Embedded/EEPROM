/*
 * eeprom_m95m02.h
 *
 *  Created on: 18-Jun-2026
 *      Author: balaj
 */

#ifndef INC_EEPROM_M95M02_H_
#define INC_EEPROM_M95M02_H_

#include "stm32f4xx_hal.h"

/* Instruction Opcodes */
#define EEPROM_CMD_WREN  0x06  // Write Enable
#define EEPROM_CMD_WRDI  0x04  // Write Disable
#define EEPROM_CMD_RDSR  0x05  // Read Status Register
#define EEPROM_CMD_WRSR  0x01  // Write Status Register
#define EEPROM_CMD_READ  0x03  // Read from Memory Array
#define EEPROM_CMD_WRITE 0x02  // Write to Memory Array

/* Status Register Masks */
#define EEPROM_SR_WIP    0x01  // Write In Progress flag
#define EEPROM_SR_WEL    0x02  // Write Enable Latch flag

/* Hardware limitations */
#define EEPROM_SPI_TIMEOUT      1000
#define EEPROM_WRITE_TIMEOUT_MS 15   // 10ms max byte write time + buffer allowance

/* Driver Function Prototypes */
HAL_StatusTypeDef EEPROM_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
HAL_StatusTypeDef EEPROM_ReadByte(uint32_t address, uint8_t *pData);
HAL_StatusTypeDef EEPROM_WriteByte(uint32_t address, uint8_t data);
HAL_StatusTypeDef EEPROM_ReadStatusRegister(uint8_t *pStatus);

#endif /* INC_EEPROM_M95M02_H_ */
