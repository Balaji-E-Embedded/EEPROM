M95M02 SPI EEPROM C Driver
A foundational, platform-independent, and robust C driver for the STMicroelectronics M95M02 2-Mbit (256 Kbytes) Serial SPI EEPROM module. This driver provides robust byte-level read and write primitives with built-in state verification and proactive error handling.

HARDWARE INTERFACE

The M95M02 functions as a standard SPI slave device. Because the chip features a 2-Mbit storage space, it requires a 24-bit (3-byte) address structure to map out its memory cells.

HARDWARE RULES

1)Chip Select ($\overline{S}$): Active-low framing pin. Driving $\overline{S}$ low wakes up the internal active decoders. Driving $\overline{S}$ high releases the bus interface and triggers physical programming cycles.
2)Write Protect ($\overline{W}$) & Hold ($\overline{HOLD}$): These functional safety pins are active-low. They should be tied high via $10\text{ k}\Omega$ pull-up resistors to prevent accidental bus pauses or unexpected register freeze conditions.

DRIVER API REFERENCE

Error Status Codes (EEPROM_Status)
The API yields custom status structures to guarantee robust diagnostics and intercept firmware lockups:
1)EEPROM_SUCCESS — Task finalized smoothly.
2)EEPROM_ERROR_OUT_OF_BOUNDS — Address pointer exceeds 0x03FFFF check barrier.
3)EEPROM_ERROR_WREN_FAILED — The device was locked against programming (e.g., block protection active).
4)EEPROM_ERROR_WRITE_TIMEOUT — Internal physical self-timed cycle ($t_W$) failed to complete within bounds.
5)EEPROM_ERROR_VERIFY_FAILED — Post-write memory readback evaluation failed.

CORE PRIMITIVES

uint8_t EEPROM_ReadStatusRegister(void);
1)Intent: Directly polls the volatile internal module parameters.
2)Use Case: Checks bit flags such as WIP (Write in Progress, bit 0) or WEL (Write Enable Latch, bit 1).

EEPROM_Status EEPROM_ReadByte(uint32_t address, uint8_t *pData);
1)Intent: Retrieves an 8-bit chunk from the targeted 24-bit memory cell.
2)Execution: Safely verifies that the system isn't busy, pushes out the structural headers (0x03 followed by 3 address bytes), and assigns the response payload back to pData.

EEPROM_Status EEPROM_WriteByte(uint32_t address, uint8_t data);
1)Intent: Rewrites an 8-bit memory cell with specific payload values.
2)Safety Logic:
  1)Issues an explicit WREN frame and inspects the WEL bit to verify access.
  2)Transmits the payload and pulses the Chip Select line high to trigger structural programming.
  3)Polls the WIP tracking bit to wait for the internal self-timed page architecture write cycle to complete.
  4)Runs a proactive validation step by re-reading the byte to ensure the payload matches the target cell.

PORTING REQUIREMENTS
To link this logical architecture up to any hardware layer (such as STM32 HAL, ESP-IDF, or AVR bare-metal), define these low-level platform dependencies inside your code:
  1)void    Set_CS_Low(void);    // Pulls Chip Select low to initiate communication
  2)void    Set_CS_High(void);   // Drives Chip Select high to finalize transactions
  3)void    SPI_WriteByte(uint8_t data); // Sends 1 byte across the MOSI line
  4)uint8_t SPI_ReadByte(void);  // Receives 1 byte across the MISO line
  5)void    Delay_Ms(uint32_t ms); // Blocks task execution for X milliseconds
