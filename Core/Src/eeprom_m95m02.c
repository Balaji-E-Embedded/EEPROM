/*
 * eeprom_m95m02.c
 *
 *  Created on: 18-Jun-2026
 *      Author: balaj
 */


#include "eeprom_m95m02.h"

static SPI_HandleTypeDef *eeprom_spi = NULL;
static GPIO_TypeDef      *eeprom_cs_port = NULL;
static uint16_t           eeprom_cs_pin = 0;

#define EEPROM_CS_LOW()  HAL_GPIO_WritePin(eeprom_cs_port, eeprom_cs_pin, GPIO_PIN_RESET)
#define EEPROM_CS_HIGH() HAL_GPIO_WritePin(eeprom_cs_port, eeprom_cs_pin, GPIO_PIN_SET)

HAL_StatusTypeDef EEPROM_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
    if (hspi == NULL || cs_port == NULL) return HAL_ERROR;
    eeprom_spi     = hspi;
    eeprom_cs_port = cs_port;
    eeprom_cs_pin  = cs_pin;

    EEPROM_CS_HIGH(); // Keep device deselected initially
    return HAL_OK;
}

HAL_StatusTypeDef EEPROM_ReadStatusRegister(uint8_t *pStatus) {
    uint8_t cmd = EEPROM_CMD_RDSR;
    HAL_StatusTypeDef status;

    EEPROM_CS_LOW(); // Drive CS Low to initiate sequence
    status = HAL_SPI_Transmit(eeprom_spi, &cmd, 1, EEPROM_SPI_TIMEOUT);
    if (status == HAL_OK) {
        status = HAL_SPI_Receive(eeprom_spi, pStatus, 1, EEPROM_SPI_TIMEOUT);
    }
    EEPROM_CS_HIGH();

    return status;
}

static HAL_StatusTypeDef EEPROM_WaitForWriteComplete(void) {
    uint8_t status_reg = 0;
    uint32_t tickstart = HAL_GetTick();

    while (1) {
        if (EEPROM_ReadStatusRegister(&status_reg) != HAL_OK) {
            return HAL_ERROR;
        }
        // WIP bit is 0 when internal self-timed write cycle completes
        if ((status_reg & EEPROM_SR_WIP) == 0) {
            return HAL_OK;
        }
        // Failure Handling: Block stuck indefinitely
        if ((HAL_GetTick() - tickstart) > EEPROM_WRITE_TIMEOUT_MS) {
            return HAL_TIMEOUT;
        }
    }
}

HAL_StatusTypeDef EEPROM_ReadByte(uint32_t address, uint8_t *pData) {
    if (address > 0x03FFFF) return HAL_ERROR; // Out of bounds safety check (max address limit)
    if (EEPROM_WaitForWriteComplete() != HAL_OK) return HAL_ERROR;

    // Header packet format: Opcode [1 byte] + 24-bit Address [3 bytes]
    uint8_t header[4];
    header[0] = EEPROM_CMD_READ;
    header[1] = (uint8_t)((address >> 16) & 0xFF);
    header[2] = (uint8_t)((address >> 8)  & 0xFF);
    header[3] = (uint8_t)(address         & 0xFF);

    HAL_StatusTypeDef status;

    EEPROM_CS_LOW();
    status = HAL_SPI_Transmit(eeprom_spi, header, 4, EEPROM_SPI_TIMEOUT);
    if (status == HAL_OK) {
        status = HAL_SPI_Receive(eeprom_spi, pData, 1, EEPROM_SPI_TIMEOUT);
    }
    EEPROM_CS_HIGH(); // Terminate by driving chip select high

    return status;
}

HAL_StatusTypeDef EEPROM_WriteByte(uint32_t address, uint8_t data) {
    if (address > 0x03FFFF) return HAL_ERROR;
    if (EEPROM_WaitForWriteComplete() != HAL_OK) return HAL_ERROR;

    // 1. Send Write Enable (WREN) command
    uint8_t wren_cmd = EEPROM_CMD_WREN;
    EEPROM_CS_LOW();
    if (HAL_SPI_Transmit(eeprom_spi, &wren_cmd, 1, EEPROM_SPI_TIMEOUT) != HAL_OK) {
        EEPROM_CS_HIGH();
        return HAL_ERROR;
    }
    EEPROM_CS_HIGH(); // Deselecting sets the Write Enable Latch

    // Checking Mechanism: Verify WEL bit was unlocked successfully
    uint8_t status_reg;
    if (EEPROM_ReadStatusRegister(&status_reg) != HAL_OK || !(status_reg & EEPROM_SR_WEL)) {
        return HAL_ERROR;
    }

    // 2. Transmit Write Opcode, 24-bit address, and payload data byte
    uint8_t write_packet[5];
    write_packet[0] = EEPROM_CMD_WRITE;
    write_packet[1] = (uint8_t)((address >> 16) & 0xFF);
    write_packet[2] = (uint8_t)((address >> 8)  & 0xFF);
    write_packet[3] = (uint8_t)(address         & 0xFF);
    write_packet[4] = data;

    EEPROM_CS_LOW();
    HAL_StatusTypeDef status = HAL_SPI_Transmit(eeprom_spi, write_packet, 5, EEPROM_SPI_TIMEOUT);
    EEPROM_CS_HIGH(); // CS rising edge triggers actual self-timed write cycle

    if (status != HAL_OK) return HAL_ERROR;

    // 3. Robust Checking Mechanism: Validation read-back verification
    uint8_t verification_byte = 0;
    if (EEPROM_ReadByte(address, &verification_byte) != HAL_OK || verification_byte != data) {
        return HAL_ERROR; // Data verification failed (e.g. block protected, hardware fault)
    }

    return HAL_OK;
}
