/*

  eeprom.c
  
  write and read to the intermal EEPROM
  
  Source: https://www.lpcware.com/content/forum/writing-eeprom-lpc13u37


  Status codes

  0 	CMD_SUCCESS 			Command is executed successfully.
  1 	INVALID_COMMAND 		Invalid command.
  2 	SRC_ADDR_ERROR 		Source address is not on a word boundary.
  3 	DST_ADDR_ERROR 		Destination address is not on a correct boundary.
  4 	SRC_ADDR_NOT_MAPPED	Source address is not mapped in the memory map. Count value is taken in to consideration where applicable.
  5 	DST_ADDR_NOT_MAPPED	Destination address is not mapped in the memory map. Count value is taken in to consideration where applicable.
  6 	COUNT_ERROR 			Byte count is not multiple of 4 or is not a permitted value.
  7 	INVALID_SECTOR 			Sector number is invalid.
  8 	SECTOR_NOT_BLANK 		Sector is not blank.
  9 	SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION Command to prepare sector for write operation was not executed.
  10 	COMPARE_ERROR 			Source and destination data is not same.
  11 	BUSY 					flash programming hardware interface is busy.

*/

#include "chip.h"
#include "irq.h"


unsigned int EEPROM_Write(uint8_t AddressToWrite, void* DataArray, uint8_t BytesToWrite)
{
  uint32_t irq_state;
  unsigned int IAP_Command[5];
  unsigned int IAP_Result[4];
  IAP_Command[0] = 61;					//IAP EEPROM write command
  IAP_Command[1] = (uint32_t)AddressToWrite;			//EEPROM address to write to
  IAP_Command[2] = (uint32_t)DataArray;				//RAM address of the data to write
  IAP_Command[3] = (uint32_t)BytesToWrite;			//Number of bytes to write
  IAP_Command[4] = SystemCoreClock/1000;				//System clock frequency in kHz

  irq_state = disable_irq();
  iap_entry(IAP_Command, IAP_Result);
  restore_irq(irq_state);

  // Status code CMD_SUCCESS (0) | SRC_ADDR_NOT_MAPPED (4) | DST_ADDR_NOT_MAPPED (5)
  return (uint8_t)IAP_Result[0];
}

unsigned int EEPROM_Read(uint8_t AddressToRead, void* DataArray, uint8_t BytesToRead)
{
  uint32_t irq_state;
  unsigned int IAP_Command[5];
  unsigned int IAP_Result[4];

  IAP_Result[0] = 0;

  IAP_Command[0] = 62;					//IAP EEPROM read command
  IAP_Command[1] = (uint32_t)AddressToRead;			//EEPROM address to read from
  IAP_Command[2] = (uint32_t)DataArray;				//RAM address to copy the EEPROM data to
  IAP_Command[3] = (uint32_t)BytesToRead;				//Number of bytes to read
  IAP_Command[4] = SystemCoreClock/1000;				//System clock frequency in kHz

  irq_state = disable_irq();
  iap_entry(IAP_Command, IAP_Result);
  restore_irq(irq_state);

  // Status code CMD_SUCCESS (0) | SRC_ADDR_NOT_MAPPED (4) | DST_ADDR_NOT_MAPPED (5)
  return (uint8_t)IAP_Result[0];
}

/*
  
*/
uint8_t EEPROM_Test(void)
{
  uint32_t x, y, a;
  x = 123456789;
  y = 0;
  if ( EEPROM_Write(0, &x, sizeof(x)) != 0 )
    return 0;
  if ( EEPROM_Read(0, &y, sizeof(y)) != 0 )
    return 0;
  if ( x != y )
    return 0;
  a = 0x0ffaa5533;
  if ( EEPROM_Write(sizeof(x), &a, sizeof(a)) != 0 )
    return 0;
  if ( EEPROM_Read(0, &y, sizeof(y)) != 0 )
    return 0;
  if ( x != y )
    return 0;
  if ( EEPROM_Read(sizeof(x), &y, sizeof(y)) != 0 )
    return 0;
  if ( a != y )
    return 0;
  
  return 1;
}
